/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  imm/wiznet.c: wiznet debugging and general information facility      */
/*                                                                       */
/*************************************************************************/

#include "CommandSet.hpp"
#include "Descriptor.hpp"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "merc.h"

extern FILE *fpArea;

extern void copy_areaname(char *dest);

/* Reports a bug. */
void bug(const char *str, ...) {
    char buf[MAX_STRING_LENGTH * 2];
    char buf2[MAX_STRING_LENGTH];
    va_list arglist;

    if (fpArea != nullptr) {
        int iLine;
        int iChar;

        if (fpArea == stdin) {
            iLine = 0;
        } else {
            iChar = ftell(fpArea);
            fseek(fpArea, 0, 0);
            for (iLine = 0; ftell(fpArea) < iChar; iLine++) {
                while (getc(fpArea) != '\n')
                    ;
            }
            fseek(fpArea, iChar, 0);
        }
        /* FIXME : 'strArea' is not safely readable from here (I think because it's an
         * array, not a pointer) - fix the filename printing so it works properly. */
        copy_areaname(buf2);
        snprintf(buf, sizeof(buf), "[*****] FILE: %s LINE: %d", buf2, iLine);
        log_string(buf);
    }

    strcpy(buf, "[*****] BUG: ");
    va_start(arglist, str);
    vsnprintf(buf + strlen(buf), sizeof(buf), str, arglist);
    va_end(arglist); /* TM added */
    log_new(buf, EXTRA_WIZNET_BUG, 0); /* TM added */
}

/* Writes a string to the log. */
void log_string(const char *str) { log_new(str, EXTRA_WIZNET_DEBUG, 0); }

/* New log - takes a level and broadcasts to IMMs on WIZNET */
void log_new(const char *str, int loglevel, int level) {
    char *strtime;
    char buf[MAX_STRING_LENGTH];
    Descriptor *d;

    strtime = ctime(&current_time);
    strtime[strlen(strtime) - 1] = '\0';
    fprintf(stderr, "%s :: %s\n", strtime, str); /* Prints to the log */

    snprintf(buf, sizeof(buf), "|GWIZNET:|g %s|w\n\r", str); /* Prepare the Wiznet message */

    if (loglevel == EXTRA_WIZNET_DEBUG)
        level = UMAX(level, 96); /* Prevent non-SOCK ppl finding out sin_addrs */

    for (d = descriptor_list; d; d = d->next) {
        CHAR_DATA *ch = d->person();
        if ((!d->is_playing()) || (ch == nullptr) || (IS_NPC(ch)) || !is_set_extra(ch, EXTRA_WIZNET_ON)
            || !is_set_extra(ch, loglevel) || (get_trust(ch) < level))
            continue;
        send_to_char(buf, d->character());
    }
}

void print_status(CHAR_DATA *ch, const char *name, const char *master_name, int state, int master_state) {
    char buff[MAX_STRING_LENGTH];
    const size_t prefix_len = 16;

    memset(buff, ' ', prefix_len);
    memcpy(buff, name, strlen(name));

    /* Check if the channel argument is the Quiet mode one */
    if (state) {
        if (master_state) {
            strcpy(buff + prefix_len, "|gON|w\n\r");
        } else {
            snprintf(buff + prefix_len, sizeof(buff) - prefix_len, "|rON (%s)|w\n\r", master_name);
        }
    } else {
        strcpy(buff + prefix_len, "|rOFF|w\n\r");
    }
    send_to_char(buff, ch);
}

static void print_wiznet_statusline(CHAR_DATA *ch, const char *name, int state) {
    print_status(ch, name, "wiznet is off", state, is_set_extra(ch, EXTRA_WIZNET_ON));
}

static void print_wiznet_status(CHAR_DATA *ch) {
    send_to_char("|Woption          status|w\n\r", ch);
    send_to_char("----------------------------\n\r", ch);

    print_wiznet_statusline(ch, "bug", is_set_extra(ch, EXTRA_WIZNET_BUG));
    print_wiznet_statusline(ch, "debug", is_set_extra(ch, EXTRA_WIZNET_DEBUG));
    print_wiznet_statusline(ch, "immortal", is_set_extra(ch, EXTRA_WIZNET_IMM));
    print_wiznet_statusline(ch, "mortal", is_set_extra(ch, EXTRA_WIZNET_MORT));
    print_wiznet_statusline(ch, "tick", is_set_extra(ch, EXTRA_WIZNET_TICK));
    print_status(ch, "wiznet", "", is_set_extra(ch, EXTRA_WIZNET_ON), 1);
}

using wiznet_fn = std::function<void(CHAR_DATA *ch)>;
static CommandSet<wiznet_fn> wiznet_commands;

void wiznet_on(CHAR_DATA *ch) {
    set_extra(ch, EXTRA_WIZNET_ON);
    send_to_char("|cWIZNET is now |gON|c.|w\n\r", ch);
}

void wiznet_off(CHAR_DATA *ch) {
    remove_extra(ch, EXTRA_WIZNET_ON);
    send_to_char("|cWIZNET is now |rOFF|c.|w\n\r", ch);
}

static void toggle_wizchan(CHAR_DATA *ch, int flag, const char *name) {
    char buf[MAX_STRING_LENGTH];

    if (is_set_extra(ch, flag)) {
        snprintf(buf, sizeof(buf), "|GWIZNET %s is now |rOFF|G.|w\n\r", name);
        remove_extra(ch, flag);
    } else {
        snprintf(buf, sizeof(buf), "|GWIZNET %s is now %s|G.|w\n\r", name,
                 is_set_extra(ch, EXTRA_WIZNET_ON) ? "|gON" : "|rON (WIZNET OFF)");
        set_extra(ch, flag);
    }
    send_to_char(buf, ch);
}

void wiznet_bug(CHAR_DATA *ch) { toggle_wizchan(ch, EXTRA_WIZNET_BUG, "bug"); }

void wiznet_debug(CHAR_DATA *ch) { toggle_wizchan(ch, EXTRA_WIZNET_DEBUG, "debug"); }

void wiznet_mortal(CHAR_DATA *ch) { toggle_wizchan(ch, EXTRA_WIZNET_MORT, "mortal"); }

void wiznet_immortal(CHAR_DATA *ch) { toggle_wizchan(ch, EXTRA_WIZNET_IMM, "immortal"); }

void wiznet_tick(CHAR_DATA *ch) { toggle_wizchan(ch, EXTRA_WIZNET_TICK, "tick"); }

void wiznet_initialise() {
    wiznet_commands.add("on", wiznet_on, 0);
    wiznet_commands.add("off", wiznet_off, 0);
    wiznet_commands.add("bug", wiznet_bug, 0);
    wiznet_commands.add("debug", wiznet_debug, 0);
    wiznet_commands.add("mortal", wiznet_mortal, 0);
    wiznet_commands.add("immortal", wiznet_immortal, 0);
    wiznet_commands.add("tick", wiznet_tick, 0);
}

void do_wiznet(CHAR_DATA *ch, const char *argument) {
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument(argument, arg);

    auto wiznet_fn = wiznet_commands.get(arg, 0);
    if (wiznet_fn.has_value()) {
        (*wiznet_fn)(ch);
    } else {
        print_wiznet_status(ch);
    }
}
