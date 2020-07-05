/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Additional: ROM2.4 copyright as stated at top of file: buffer.c      */
/*                                                                       */
/*  buffer.c: flexible text management routines, based loosely on those  */
/*            seen in ROM2.4                                             */
/*************************************************************************/


#if defined(macintosh)
#include <types.h>
#else
#if defined(riscos)
#include "sys/types.h"
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#endif
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "merc.h"
#include "buffer.h"

#define BUFFER_GRANULARITY 4096  /* Must be a power of 2. */


/* Creates a new buffer. */
BUFFER *buffer_create(void) {
	BUFFER *new = malloc(sizeof(BUFFER));
	if (new) {
		new->size = BUFFER_GRANULARITY;
		new->buffer = malloc(BUFFER_GRANULARITY);
		if (new->buffer) {
			new->buffer[0] = '\0';
		} else {
			free(new);
			new = NULL;
		}
	}
	if (!new) {
		bug("Failed to create a new buffer.");
	}
	return new;
}


/* Destroys the buffer. */
void buffer_destroy(BUFFER *buffer) {
	free(buffer->buffer);  /* Yes, it is guaranteed to be non-null. */
	free(buffer);
}


/* Returns the buffer's contents. */
char *buffer_string(BUFFER *buffer) {
	return buffer->buffer;
}


/* Adds a line of text to the buffer. */
static void buffer_addline_internal(BUFFER *buffer, const char *text, int linelen) {
	int buflen = buffer->buffer ? strlen(buffer->buffer) : 0;

	if ((linelen + buflen + 1) > buffer->size) {
		int needed = (linelen + buflen + 1 + BUFFER_GRANULARITY) &~(BUFFER_GRANULARITY-1);
		char *newtext = (char *)realloc(buffer->buffer, needed);
		if (newtext == NULL) {
			bug("Failed to realloc buffer to %d in add_buf.", needed);
		} else {
			buffer->buffer = newtext;
			buffer->size = needed;
			memcpy(buffer->buffer + buflen, text, linelen + 1);
		}
	} else {
		memcpy(buffer->buffer + buflen, text, linelen + 1);
	}
}

void buffer_addline(BUFFER *buffer, const char *text)
{
	buffer_addline_internal(buffer, text, strlen(text));
}

void buffer_addline_fmt(BUFFER *buffer, const char *text_format, ...) 
{
	char text[MAX_STRING_LENGTH];
	int linelen;
	va_list arglist;

	va_start(arglist,text_format);
	linelen = vsprintf(text, text_format, arglist);
	va_end(arglist);

	buffer_addline_internal(buffer, text, linelen);
}

/* Deletes a line from the buffer. */
void buffer_removeline(BUFFER *buffer) {
	char *text = buffer->buffer;
	char *lastline;

	if (text == NULL || strlen(text) == 0) {
		return;
	}
	lastline = strrchr(text, '\n');
	if (lastline) {
		*lastline = '\0';  /* We don't need this text anyway. */
		lastline = strrchr(text, '\n');
	}
	if (lastline) {
		lastline[2] = '\0';
	} else {
		text[0] = '\0';
	}
}


/* Shrinks the buffer to the smallest size which can still contain its text. */
void buffer_shrink(BUFFER *buffer) {
	int new_size = strlen(buffer->buffer) + 1;
	char *newbuf = realloc(buffer->buffer, new_size);
	if (newbuf) {
		buffer->buffer = newbuf;
		buffer->size = new_size;
	}
}


/* Pages the buffer to the given char, and then destroys the buffer. */
void buffer_send(BUFFER *buffer, CHAR_DATA *ch) {
	page_to_char(buffer->buffer, ch);
	buffer_destroy(buffer);
}
