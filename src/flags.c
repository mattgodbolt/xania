/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  flags.c:  a front end and management system for flags                */
/*                                                                       */
/*************************************************************************/


#if defined(macintosh)
#include <types.h>
#else
#if defined(riscos)
#include <time.h>
#include "sys/types.h"
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "merc.h"
#include "flags.h"
#include "buffer.h"


void display_flags(char *template, CHAR_DATA *ch, int current_val) {
  char buf[MAX_STRING_LENGTH];
  char *src,*dest,*ptr;
  BUFFER *buffer;
  int chars;
  if (current_val) {
    int bit;
    ptr = template;
    buffer = buffer_create();
    buffer_addline(buffer, "|C");
    chars=0;
    for (bit=0;bit<32;bit++) {
      src=ptr;dest=buf;
      for ( ; *src && !isspace(*src) ; )
        *dest++ = *src++;
      *dest='\0';
      if (IS_SET(current_val,(1<<bit)) && *buf!='*') {
        int num;
        char *bufptr=buf;
        if (isdigit(*buf)) {
          num=atoi(buf);
          while (isdigit(*bufptr)) bufptr++;
        } else {
          num=0;
        }
        if (get_trust(ch)>=num) {
          if (strlen(bufptr)+chars > 70) {
            buffer_addline(buffer,"\n\r");
            chars=0;
          }
          buffer_addline(buffer,bufptr);
          buffer_addline(buffer," ");
          chars+=strlen(bufptr)+1;
        }
      }
      ptr=src;
      while (*ptr && !isspace(*ptr)) ptr++;
      while (*ptr && isspace(*ptr)) ptr++;
      if (*ptr=='\0')
        break;
    }
    if (chars!=0)
      buffer_addline(buffer,"\n\r");
    buffer_addline(buffer,"|w");
    buffer_send(buffer,ch); /* This frees buffer */
  } else {
    send_to_char("|CNone.|w\n\r",ch);
  }
}

int flag_bit(char *template, char *flag, int level) {
  char buf[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];
  char *src=flag,*dest=buf;
  char *ptr = template;
  int bit = 0;

  for ( ; *src && !isspace(*src) ; )
    *dest++ = *src++;
  *dest='\0';					/* Copy just the flagname into buf */

  for ( ; bit < 32; bit++) {
    int lev;
    if (isdigit(*ptr)) {
      lev = atoi(ptr);
      while (isdigit(*ptr)) ptr++;
    } else {
      lev = 0;
    }						/* Work out level restriction */
    src = ptr; dest=buf2;
    for ( ; *src && !isspace(*src) ; )
      *dest++ = *src++;
    *dest='\0';					/* Copy just the current flagname into buf2 */
    if (!str_prefix(buf, buf2) && (level >= lev))
      return bit;				/* Match and within level restrictions? */
    ptr = src;
    while (isspace(*ptr)) ptr++;		/* Skip to next flag */
    if (*ptr=='\0')
      break;					/* At end? */
  }
  return INVALID_BIT;
}

long flag_set(char *template, char *arg, long current_val, CHAR_DATA *ch) {
  long retval = current_val;
  if (arg[0]=='\0') {
    display_flags(template,ch,(int)current_val);
    send_to_char("Allowed flags are:\n\r",ch);
    display_flags(template,ch,-1);
    return current_val;
  }
  for ( ; ; ) {
    int bit,flag=0;
    switch (*arg) {
      case '+':
        flag = 1;
        arg++;
        break;
      case '-':
        flag = 2;
        arg++;
        break;
    }
    bit = flag_bit(template,arg,get_trust(ch));
    if (bit==INVALID_BIT) {
      display_flags(template,ch,(int)current_val);
      send_to_char("Allowed flags are:\n\r",ch);
      display_flags(template,ch,-1);
      return current_val;
    }
    switch (flag) {
      case 0: /* toggle */
        retval^=(1<<bit);
        break;
      case 1: /* set */
        retval|=(1<<bit);
        break;
      case 2: /* clear */
        retval&=~(1<<bit);
        break;
    }
    while (*arg && !isspace(*arg)) arg++;
    while (*arg && isspace(*arg))  arg++;
    if (*arg=='\0')
      break;
  }
  display_flags(template,ch,(int)retval);
  return retval;
}
