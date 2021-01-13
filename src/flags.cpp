/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  flags.c:  a front end and management system for flags                */
/*                                                                       */
/*************************************************************************/

#include "flags.h"
#include "buffer.h"
#include "comm.hpp"
#include "merc.h"

#include <cctype>
#include <cstdlib>
#include <cstring>

void display_flags(const char *format, Char *ch, unsigned long current_val) {
    char buf[MAX_STRING_LENGTH];
    const char *src;
    char *dest;
    const char *ptr;
    BUFFER *buffer;
    int chars;
    if (current_val) {
        unsigned long bit;
        ptr = format;
        buffer = buffer_create();
        buffer_addline(buffer, "|C");
        chars = 0;
        for (bit = 0; bit < 32; bit++) {
            src = ptr;
            dest = buf;
            for (; *src && !isspace(*src);)
                *dest++ = *src++;
            *dest = '\0';
            if (IS_SET(current_val, (1u << bit)) && *buf != '*') {
                int num;
                char *bufptr = buf;
                if (isdigit(*buf)) {
                    num = atoi(buf);
                    while (isdigit(*bufptr))
                        bufptr++;
                } else {
                    num = 0;
                }
                if (ch->get_trust() >= num) {
                    if (strlen(bufptr) + chars > 70) {
                        buffer_addline(buffer, "\n\r");
                        chars = 0;
                    }
                    buffer_addline(buffer, bufptr);
                    buffer_addline(buffer, " ");
                    chars += strlen(bufptr) + 1;
                }
            }
            ptr = src;
            while (*ptr && !isspace(*ptr))
                ptr++;
            while (*ptr && isspace(*ptr))
                ptr++;
            if (*ptr == '\0')
                break;
        }
        if (chars != 0)
            buffer_addline(buffer, "\n\r");
        buffer_addline(buffer, "|w");
        buffer_send(buffer, ch); /* This frees buffer */
    } else {
        ch->send_line("|CNone.|w");
    }
}

unsigned long flag_bit(const char *format, const char *flag, int level) {
    char buf[MAX_INPUT_LENGTH];
    char buf2[MAX_INPUT_LENGTH];
    const char *src = flag;
    char *dest = buf;
    const char *ptr = format;
    int bit = 0;

    for (; *src && !isspace(*src);)
        *dest++ = *src++;
    *dest = '\0'; /* Copy just the flagname into buf */

    for (; bit < 32; bit++) {
        int lev;
        if (isdigit(*ptr)) {
            lev = atoi(ptr);
            while (isdigit(*ptr))
                ptr++;
        } else {
            lev = 0;
        } /* Work out level restriction */
        src = ptr;
        dest = buf2;
        for (; *src && !isspace(*src);)
            *dest++ = *src++;
        *dest = '\0'; /* Copy just the current flagname into buf2 */
        if (!str_prefix(buf, buf2) && (level >= lev))
            return bit; /* Match and within level restrictions? */
        ptr = src;
        while (isspace(*ptr))
            ptr++; /* Skip to next flag */
        if (*ptr == '\0')
            break; /* At end? */
    }
    return INVALID_BIT;
}

unsigned long flag_set(const char *format, const char *arg, unsigned long current_val, Char *ch) {
    auto retval = current_val;
    if (arg[0] == '\0') {
        display_flags(format, ch, (int)current_val);
        ch->send_line("Allowed flags are:");
        display_flags(format, ch, -1);
        return current_val;
    }
    enum class BitOp { Toggle, Set, Clear };

    for (;;) {
        auto flag = BitOp::Toggle;
        switch (*arg) {
        case '+':
            flag = BitOp::Set;
            arg++;
            break;
        case '-':
            flag = BitOp::Clear;
            arg++;
            break;
        }
        auto bit = flag_bit(format, arg, ch->get_trust());
        if (bit == INVALID_BIT) {
            display_flags(format, ch, (int)current_val);
            ch->send_line("Allowed flags are:");
            display_flags(format, ch, -1);
            return current_val;
        }
        switch (flag) {
        case BitOp::Toggle: retval ^= (1u << bit); break;
        case BitOp::Set: retval |= (1u << bit); break;
        case BitOp::Clear: retval &= ~(1u << bit); break;
        }
        while (*arg && !isspace(*arg))
            arg++;
        while (*arg && isspace(*arg))
            arg++;
        if (*arg == '\0')
            break;
    }
    display_flags(format, ch, (int)retval);
    return retval;
}
