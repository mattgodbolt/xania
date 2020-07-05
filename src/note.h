/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  note.h: notes system header file                                     */
/*                                                                       */
/*************************************************************************/


#ifndef __note_h
#define __note_h


#include "merc.h"


#define NOTE_FILE       "notes.txt"     /* For 'notes'                  */


void do_note(struct char_data *ch, char *argument);
void note_initialise(void);
int note_count(struct char_data *ch);
int is_note_to(struct char_data *ch, NOTE_DATA *note);
void note_announce( CHAR_DATA* chsender, NOTE_DATA *note );

#endif
