/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  imm/wiznet.c: wiznet debugging and general information facility      */
/*                                                                       */
/*************************************************************************/

#include <sys/types.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "merc.h"
#include "trie.h"

extern FILE *fpArea;

extern void copy_areaname(char *dest);

/* Reports a bug. */
void bug(const char *str, ...) {
    char buf[MAX_STRING_LENGTH * 2];
    char buf2[MAX_STRING_LENGTH];
    va_list arglist;

    if (fpArea != NULL) {
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

    return;
}

/* Writes a string to the log. */
void log_string(const char *str) { log_new(str, EXTRA_WIZNET_DEBUG, 0); }

/* New log - takes a level and broadcasts to IMMs on WIZNET */
void log_new(const char *str, int loglevel, int level) {
    char *strtime;
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    strtime = ctime(&current_time);
    strtime[strlen(strtime) - 1] = '\0';
    fprintf(stderr, "%s :: %s\n", strtime, str); /* Prints to the log */

    snprintf(buf, sizeof(buf), "|GWIZNET:|g %s|w\n\r", str); /* Prepare the Wiznet message */

    if (loglevel == EXTRA_WIZNET_DEBUG)
        level = UMAX(level, 96); /* Prevent non-SOCK ppl finding out sin_addrs */

    for (d = descriptor_list; d; d = d->next) {
        CHAR_DATA *ch = d->original ? d->original : d->character;
        if ((d->connected != CON_PLAYING) || (ch == NULL) || (IS_NPC(ch)) || !is_set_extra(ch, EXTRA_WIZNET_ON)
            || !is_set_extra(ch, loglevel) || (get_trust(ch) < level))
            continue;
        send_to_char(buf, d->character);
    }
}

void print_status(CHAR_DATA *ch, char *name, char *master_name, int state, int master_state) {
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

static void print_wiznet_statusline(CHAR_DATA *ch, char *name, int state) {
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

typedef void(wiznet_fn_t)(CHAR_DATA *);

static void *trie;

void wiznet_on(CHAR_DATA *ch) {
    set_extra(ch, EXTRA_WIZNET_ON);
    send_to_char("|cWIZNET is now |gON|c.|w\n\r", ch);
}

void wiznet_off(CHAR_DATA *ch) {
    remove_extra(ch, EXTRA_WIZNET_ON);
    send_to_char("|cWIZNET is now |rOFF|c.|w\n\r", ch);
}

static void toggle_wizchan(CHAR_DATA *ch, int flag, char *name) {
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

void wiznet_initialise(void) {
    trie = trie_create(0);
    if (!trie) {
        bug("wiznet_initialise: couldn't create trie.", 0);
        exit(1);
    }
    trie_add(trie, "on", wiznet_on, 0);
    trie_add(trie, "off", wiznet_off, 0);
    trie_add(trie, "bug", wiznet_bug, 0);
    trie_add(trie, "debug", wiznet_debug, 0);
    trie_add(trie, "mortal", wiznet_mortal, 0);
    trie_add(trie, "immortal", wiznet_immortal, 0);
    trie_add(trie, "tick", wiznet_tick, 0);
}

void do_wiznet(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    wiznet_fn_t *wiznet_fn;

    argument = one_argument(argument, arg);

    wiznet_fn = trie_get(trie, arg, 0);
    if (wiznet_fn) {
        wiznet_fn(ch);
    } else {
        print_wiznet_status(ch);
    }
}
