/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  note.h: notes system header file                                     */
/*                                                                       */
/*************************************************************************/

#pragma once

#include "buffer.h"
#include "common/Time.hpp"

/* Data structure for notes. */
struct NOTE_DATA {
    NOTE_DATA *next;
    NOTE_DATA *prev;
    char *sender;
    char *date;
    char *to_list;
    char *subject;
    BUFFER *text;
    Time date_stamp;
};

struct CHAR_DATA;

#define NOTE_FILE "notes.txt" /* For 'notes'                  */

void do_note(CHAR_DATA *ch, const char *argument);
void note_initialise();
int note_count(CHAR_DATA *ch);
int is_note_to(const CHAR_DATA *ch, const NOTE_DATA *note);
void note_announce(CHAR_DATA *chsender, NOTE_DATA *note);
