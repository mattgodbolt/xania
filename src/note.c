/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  note.c: notes system                                                 */
/*                                                                       */
/*************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "merc.h"
#include "note.h"
#include "trie.h"
#include "buffer.h"

static NOTE_DATA *note_first;
static NOTE_DATA *note_last;

static void *trie;

typedef void(note_fn_t)(CHAR_DATA *, char *);

/* returns the number of unread notes for the given user. */
int note_count(CHAR_DATA *ch) {
    NOTE_DATA *note;
    int notes = 0;
    for (note = note_first; note != NULL; note = note->next) {
        if (is_note_to(ch, note) && str_cmp(ch->name, note->sender) && note->date_stamp > ch->last_note) {
            notes++;
        }
    }
    return notes;
}

int is_note_to(struct char_data *ch, NOTE_DATA *note) {
    if (!str_cmp(ch->name, note->sender)) {
        return TRUE;
    }
    if (is_name("all", note->to_list)) {
        return TRUE;
    }
    if (IS_IMMORTAL(ch) && is_name("immortal", note->to_list)) {
        return TRUE;
    }
    if (is_name(ch->name, note->to_list)) {
        return TRUE;
    }
    return FALSE;
}

static NOTE_DATA *create_note(void) {
    NOTE_DATA *note = calloc(1, sizeof(NOTE_DATA));
    note->text = buffer_create();
    if (!note->text) {
        free(note);
        note = NULL;
    }
    return note;
}

static void destroy_note(NOTE_DATA *note) {
    if (note->prev) {
        note->prev->next = note->next;
    } else if (note == note_first) {
        note_first = note->next;
    }
    if (note->next) {
        note->next->prev = note->prev;
    } else if (note == note_last) {
        note_last = note->prev;
    }
    if (note->sender) {
        free_string(note->sender);
    }
    if (note->date) {
        free_string(note->date);
    }
    if (note->to_list) {
        free_string(note->to_list);
    }
    if (note->subject) {
        free_string(note->subject);
    }
    if (note->text) {
        buffer_destroy(note->text);
    }
    free(note);
}

static void note_link(NOTE_DATA *note) {
    note->next = NULL;
    note->prev = note_last;
    if (note_last) {
        note_last->next = note;
    } else {
        note_first = note;
    }
    note_last = note;
}

static NOTE_DATA *lookup_note(int index, CHAR_DATA *ch) {
    NOTE_DATA *note;

    for (note = note_first; note; note = note->next) {
        if (is_note_to(ch, note)) {
            if (index-- <= 0) {
                return note;
            }
        }
    }
    return NULL;
}

static NOTE_DATA *lookup_note_date(int date, CHAR_DATA *ch, int *index) {
    NOTE_DATA *note;
    int count = 0;

    for (note = note_first; note; note = note->next) {
        if (is_note_to(ch, note) && str_cmp(ch->name, note->sender)) {
            if (date < note->date_stamp) {
                *index = count;
                return note;
            }
            count++;
        }
    }
    return NULL;
}

static NOTE_DATA *ensure_note(CHAR_DATA *ch) {
    if (!ch->pnote) {
        ch->pnote = create_note();
        if (ch->pnote) {
            ch->pnote->sender = str_dup(ch->name);
            if (!ch->pnote->sender) {
                free(ch->pnote);
                ch->pnote = NULL;
            }
        }
    }
    return ch->pnote;
}

static void save_note(FILE *file, NOTE_DATA *note) {
    fprintf(file, "Sender %s~\n", note->sender);
    fprintf(file, "Date %s~\n", note->date);
    fprintf(file, "Stamp %d\n", (int)note->date_stamp);
    fprintf(file, "To %s~\n", note->to_list);
    fprintf(file, "Subject %s~\n", note->subject);
    fprintf(file, "Text\n%s~\n", note->text ? buffer_string(note->text) : "");
}

static void save_notes(void) {
    FILE *file;
    NOTE_DATA *note;

    if ((file = fopen(NOTE_FILE, "w"))) {
        for (note = note_first; note != NULL; note = note->next) {
            save_note(file, note);
        }
        fclose(file);
    } else {
        perror(NOTE_FILE);
    }
}

static void note_list(CHAR_DATA *ch, char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];
    int num = 0;
    NOTE_DATA *note = note_first;

    for (; note; note = note->next) {
        if (is_note_to(ch, note)) {
            int is_new = note->date_stamp > ch->last_note && str_cmp(note->sender, ch->name);

            snprintf(buf, sizeof(buf), "[%3d%s] %s: %s|w\n\r", num, is_new ? "N" : " ", note->sender, note->subject);
            send_to_char(buf, ch);
            num++;
        }
    }
}

static void note_read(CHAR_DATA *ch, char *argument) {
    NOTE_DATA *note = NULL;
    int note_index;

    if (argument[0] == '\0' || !str_prefix(argument, "next")) {
        note = lookup_note_date(ch->last_note, ch, &note_index);
        if (!note) {
            send_to_char("You have no unread notes.\n\r", ch);
        }
    } else if (is_number(argument)) {
        note_index = atoi(argument);
        note = lookup_note(note_index, ch);
        if (!note || note_index < 0) {
            send_to_char("No such note.\n\r", ch);
        }
    } else {
        send_to_char("Note read which number?\n\r", ch);
    }

    if (note) {
        char buf[MAX_STRING_LENGTH];

        snprintf(buf, sizeof(buf), "[%3d] %s|w: %s|w\n\r%s|w\n\rTo: %s|w\n\r", note_index, note->sender, note->subject,
                 note->date, note->to_list);
        send_to_char(buf, ch);
        if (note->text) {
            page_to_char(buffer_string(note->text), ch);
        }
        send_to_char("|w", ch);
        ch->last_note = UMAX(ch->last_note, note->date_stamp);
    }
}

static void note_addline(CHAR_DATA *ch, char *argument) {
    NOTE_DATA *note = ensure_note(ch);
    if (!note) {
        send_to_char("Failed to create new note.\n\r", ch);
        return;
    }
    buffer_addline_fmt(note->text, "%s\n\r", argument);
    send_to_char("Ok.\n\r", ch);
}

static void note_removeline(CHAR_DATA *ch, char *argument) {
    (void)argument;
    NOTE_DATA *note = ch->pnote;
    if (!note || !note->text) {
        send_to_char("There is no text to delete.\n\r", ch);
        return;
    }
    buffer_removeline(note->text);
    send_to_char("Ok.\n\r", ch);
}

static void note_subject(CHAR_DATA *ch, char *argument) {
    NOTE_DATA *note = ensure_note(ch);
    if (note) {
        if (note->subject) {
            free_string(note->subject);
        }
        note->subject = str_dup(argument);
        send_to_char("Ok.\n\r", ch);
    } else {
        send_to_char("Failed to create new note.\n\r", ch);
    }
}

static void note_to(CHAR_DATA *ch, char *argument) {
    NOTE_DATA *note = ensure_note(ch);

    if (note) {
        if (note->to_list) {
            free_string(note->to_list);
        }
        note->to_list = str_dup(argument);
        send_to_char("Ok.\n\r", ch);
    } else {
        send_to_char("Failed to create new note.\n\r", ch);
    }
}

static void note_clear(CHAR_DATA *ch, char *argument) {
    (void)argument;
    if (ch->pnote) {
        destroy_note(ch->pnote);
        ch->pnote = NULL;
    }
    send_to_char("Ok.\n\r", ch);
}

static void note_show(CHAR_DATA *ch, char *argument) {
    (void)argument;
    char buf[MAX_STRING_LENGTH];
    NOTE_DATA *note = ch->pnote;

    if (!note) {
        send_to_char("You have no note in progress.\n\r", ch);
        return;
    }
    snprintf(buf, sizeof(buf), "%s|w: %s|w\n\rTo: %s|w\n\r", note->sender ? note->sender : "(No sender)",
             note->subject ? note->subject : "(No subject)", note->to_list ? note->to_list : "(No recipients)");
    send_to_char(buf, ch);
    send_to_char(note->text ? buffer_string(note->text) : "(No message body)\n\r", ch);
    send_to_char("|w", ch);
}

static void note_post(CHAR_DATA *ch, char *argument) {
    (void)argument;
    NOTE_DATA *note = ch->pnote;
    FILE *fp;
    char *strtime;

    if (!note) {
        send_to_char("You have no note in progress.\n\r", ch);
        return;
    }
    if (!note->to_list) {
        send_to_char("You need to provide a recipient (name, all, or immortal).\n\r", ch);
        return;
    }
    if (!note->subject) {
        send_to_char("You need to provide a subject.\n\r", ch);
        return;
    }
    strtime = ctime(&current_time);
    strtime[strlen(strtime) - 1] = '\0';
    note->date = str_dup(strtime);
    note->date_stamp = current_time;

    ch->pnote = NULL;
    buffer_shrink(note->text);
    note_link(note);

    if ((fp = fopen(NOTE_FILE, "a")) == NULL) {
        perror(NOTE_FILE);
    } else {
        save_note(fp, note);
        fclose(fp);
    }
    note_announce(ch, note);
    send_to_char("Ok.\n\r", ch);
}

void note_announce(CHAR_DATA *chsender, NOTE_DATA *note) {
    DESCRIPTOR_DATA *d;

    if (note == NULL) {
        log_string("note_announce() note is null");
        return;
    }
    for (d = descriptor_list; d; d = d->next) {
        CHAR_DATA *chtarg;
        chtarg = d->original ? d->original : d->character;
        if (d->connected == CON_PLAYING && d->character != chsender && chtarg && !IS_SET(chtarg->comm, COMM_NOANNOUNCE)
            && !IS_SET(chtarg->comm, COMM_QUIET) && is_note_to(chtarg, note)) {
            send_to_char("The Spirit of Hermes announces the arrival of a new note.\n\r", chtarg);
        }
    }
}

static void note_remove(CHAR_DATA *ch, NOTE_DATA *note) {
    char to_one[MAX_INPUT_LENGTH];
    char to_new[MAX_INPUT_LENGTH];
    char *to_list;
    int index = 0;

    if (!str_cmp(ch->name, note->sender)) {
        destroy_note(note);
        save_notes();
        return;
    }

    /* Build a new to_list.  Strip out this recipient. */
    to_list = note->to_list;
    while (*to_list != '\0') {
        to_list = one_argument(to_list, to_one);
        if (to_one[0] != '\0' && str_cmp(ch->name, to_one)) {
            int len = strlen(to_one);
            memcpy(&to_new[index], to_one, len);
            index += len;
            to_new[++index] = ' ';
        }
    }

    /* Destroy completely only if there are no recipients left. */
    if (index) {
        free_string(note->to_list);
        to_new[index - 1] = '\0';
        note->to_list = str_dup(to_new);

        /*
          char *shorter = realloc(note->to_list, index);
          if (shorter) {
          memcpy(shorter, to_new, index - 1);
          shorter[index - 1] = '\0';
          note->to_list = shorter;
          }
        */
    } else {
        destroy_note(note);
    }
    save_notes();
}

static void note_removecmd(CHAR_DATA *ch, char *argument) {
    NOTE_DATA *note;

    if (!is_number(argument)) {
        send_to_char("Note remove which number?\n\r", ch);
        return;
    }
    note = lookup_note(atoi(argument), ch);
    if (note) {
        note_remove(ch, note);
        send_to_char("Ok.\n\r", ch);
    } else {
        send_to_char("No such note.\n\r", ch);
    }
}

static void note_delete(CHAR_DATA *ch, char *argument) {
    NOTE_DATA *note;

    if (!is_number(argument)) {
        send_to_char("Note delete which number?\n\r", ch);
        return;
    }
    note = lookup_note(atoi(argument), ch);
    if (note) {
        destroy_note(note);
        save_notes();
        send_to_char("Ok.\n\r", ch);
    } else {
        send_to_char("No such note.\n\r", ch);
    }
}

void do_note(CHAR_DATA *ch, char *argument) {
    char arg[MAX_INPUT_LENGTH];
    note_fn_t *note_fn;

    if (IS_NPC(ch)) {
        return;
    }
    argument = one_argument(argument, arg);
    smash_tilde(argument);

    note_fn = trie_get(trie, arg[0] ? arg : "read", get_trust(ch));
    if (note_fn) {
        note_fn(ch, argument);
    } else {
        send_to_char("Huh?  Type 'help note' for usage.\n\r", ch);
    }
}

static void note_readfile(void) {
    FILE *fp;

    if ((fp = fopen(NOTE_FILE, "r")) == NULL) {
        return;
    }
    for (;;) {
        NOTE_DATA *note = create_note();
        char letter;

        if (!note) {
            break;
        }
        do {
            letter = getc(fp);
            if (feof(fp) || ferror(fp)) {
                fclose(fp);
                return;
            }
        } while (isspace(letter));
        ungetc(letter, fp);

        note = create_note();

        if (str_cmp(fread_word(fp), "sender"))
            break;
        note->sender = fread_string(fp);

        if (str_cmp(fread_word(fp), "date"))
            break;
        note->date = fread_string(fp);

        if (str_cmp(fread_word(fp), "stamp"))
            break;
        note->date_stamp = fread_number(fp);

        if (str_cmp(fread_word(fp), "to"))
            break;
        note->to_list = fread_string(fp);

        if (str_cmp(fread_word(fp), "subject"))
            break;
        note->subject = fread_string(fp);

        if (str_cmp(fread_word(fp), "text"))
            break;
        note->text = fread_string_tobuffer(fp);

        if (note->date_stamp < current_time - (14 * 24 * 60 * 60) /* 2 wks */) {
            destroy_note(note);
            continue;
        }
        note_link(note);
    }
    fclose(fp);
    bug("Load_notes: bad key word.", 0);
    exit(1);
}

void note_initialise(void) {
    note_readfile();
    trie = trie_create(1);
    if (!trie) {
        bug("note_initialise: couldn't create trie.", 0);
        exit(1);
    }
    trie_add(trie, "read", note_read, 0);
    trie_add(trie, "list", note_list, 0);
    trie_add(trie, "+", note_addline, 0);
    trie_add(trie, "-", note_removeline, 0);
    trie_add(trie, "subject", note_subject, 0);
    trie_add(trie, "to", note_to, 0);
    trie_add(trie, "clear", note_clear, 0);
    trie_add(trie, "show", note_show, 0);
    trie_add(trie, "post", note_post, 0);
    trie_add(trie, "send", note_post, 0);
    trie_add(trie, "remove", note_removecmd, 0);
    trie_add(trie, "delete", note_delete, MAX_LEVEL - 2);
}
