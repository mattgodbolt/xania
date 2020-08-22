/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Additional: ROM2.4 copyright as stated at top of file: buffer.c      */
/*                                                                       */
/*  buffer.h: flexible text management routine declarations and stuff    */
/*                                                                       */
/*************************************************************************/

#pragma once

struct CHAR_DATA;

typedef struct _BUFFER {
    int size;
    char *buffer;
} BUFFER;

/* Creates a new buffer. */
BUFFER *buffer_create();

/* Destroys the buffer. */
void buffer_destroy(BUFFER *buffer);

/* Returns the buffer's contents. */
const char *buffer_string(const BUFFER *buffer);

// TM this was split due to some text being sent in 'text' with no ... - users could
// do 'note + %s' and BANG! addline(buf, "note + %s"); - no parm! whoops

/* Adds a line of text to the buffer. */
void buffer_addline(BUFFER *buffer, const char *text);

/* Adds a line of text to the buffer, formatted.
 * ONLY CALL IF YOU CONTROL 'text' - no user stuff at all should go into 'text' */
void buffer_addline_fmt(BUFFER *buffer, const char *text, ...) __attribute__((format(printf, 2, 3)));

/* Deletes a line from the buffer. */
void buffer_removeline(BUFFER *buffer);

/* Shrinks the buffer to the smallest size which can still contain its text. */
void buffer_shrink(BUFFER *buffer);

/* Pages the buffer to the given char, and then destroys the buffer. */
void buffer_send(BUFFER *buffer, const CHAR_DATA *ch);
