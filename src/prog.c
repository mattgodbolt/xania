/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  prog.c:  programmable mobiles                                        */
/*                                                                       */
/*************************************************************************/


/* Standard includes */

#if defined(riscos)
#include "sys/types.h"
#else
#include <sys/types.h>
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* MERC includes */
#include "merc.h"
#include "magic.h"
#include "interp.h"
#include "prog.h"

/* External globals */
extern const struct race_type race_table [];
extern const struct materials_type material_table[];

/* Internal globals */

const struct mob_function mob_fun_table[] =
{
  { "signal",               mobdo_signal },
  { "parse",                mobdo_parse },
  { "ifstring",             mobdo_ifstring },
  { "ifname",               mobdo_ifname },
  { "if!string",            mobdo_ifnotstring },
  { "if!name",              mobdo_ifnotname },
  { "ifinstring",           mobdo_ifinstring },
  { "if!instring",          mobdo_ifnotinstring },
  { "ifrange",              mobdo_ifrange },
  { "ifge",                 mobdo_ifge },
  { "ifle",                 mobdo_ifle },
  { "ifgt",                 mobdo_ifgt },
  { "iflt",                 mobdo_iflt },
  { "ifeq",                 mobdo_ifeq },
  { "ifne",                 mobdo_ifne },
  { "return",               mobdo_return },
  { "goto",                 mobdo_goto },
  { "chance",               mobdo_chance },
  { "scan",                 mobdo_scan },
  { "scanon",               mobdo_scanon },
  { "set",                  mobdo_set },
  { "seteval",              mobdo_seteval },
  { NULL,                   NULL }
};

/* Horrid #define : */
#define do_number( string, bit ) if (!str_cmp(buf, string)) { \
                                   sprintf( parambuf, "%d", (int) them->bit ); \
                                   copy = parambuf; \
                                 }
#define do_numberobj( string, bit ) if (!str_cmp(buf, string)) { \
                                   sprintf( parambuf, "%d", (int) obj->bit ); \
                                   copy = parambuf; \
                                 }

/* Procedures */

void program_error( CHAR_DATA *ch ) {
  /* For when everything goes horribly wrong */
  bug( "program_error: Disabling %s's MOBPROG to prevent bug-file spamming",
       ch->short_descr );
  ch->program->semaphore = -1;
}

char * next_line(char * line) {
  for ( ; (*line != '\n' && *line != '\0') ; line++ );
  if (*line == '\n')
    line++; /* move past the \n */
  return line;
}

#define EVAL_ADD 0
#define EVAL_SUB 1
#define EVAL_MUL 2
#define EVAL_DIV 3
#define EVAL_MOD 4

int num_eval( char *text, CHAR_DATA *ch )
{
  /* Evaluate a number from a set of commands eg 2+1 */
  int accumulator = 0, result = 0, type = EVAL_ADD;
  char *p = text;

  for ( ;*p != 0; ) {
    for ( ;(*p == 32) || (*p == 9); p++);
    if ((*p >= '0') && (*p <= '9')) {
      accumulator = 0;
      for ( ;((*p >= '0') && (*p <= '9'));p++) {
	accumulator *= 10;
	accumulator += (*p-'0');
      }
      switch(type) {
      case EVAL_ADD:
	result+=accumulator;
	break;
      case EVAL_SUB:
	result-=accumulator;
	break;
      case EVAL_MUL:
	result*=accumulator;
	break;
      case EVAL_DIV:
	result/=accumulator;
	break;
      case EVAL_MOD:
	result%=accumulator;
	break;
      }
      continue;
    }
    switch(*p++) {
    default:
      bug( "Eval_num: Bad character %c", *--p);
      program_error(ch);
      break;
    case '\0':
      p--;
      break;
    case '+':
      type = EVAL_ADD;
      break;
    case '-':
      type = EVAL_SUB;
      break;
    case '/':
      type = EVAL_DIV;
      break;
    case '*':
      type = EVAL_MUL;
      break;
    case '%':
      type = EVAL_MOD;
      break;
    }
  }
  return result;
}

char *parse_one_argument(CHAR_DATA *ch, char *args, char *buf);

char * label_lookup( CHAR_DATA *ch, char *label)
{
  char * line, *linein;
  char buf[MAX_STRING_LENGTH], *bufptr;

  if (ch==NULL) {
    bug( "label_lookup: NULL ch passed", NULL );
    return NULL;
  }
  if (ch->program == NULL) {
    bug( "label_lookup: ch passed with no program!", NULL );
    return NULL;
  }

  line = ch->program->first_line;

  for ( ; ; ) {
    linein = line;

    /* I hope line is left on the first non-white character in the line below */
    for ( ; (*linein == 9 || *linein == 32 || *linein == '\r') ; linein++);

    if (*linein == 0)
      return NULL;            /* NULL means end of the mobprog - we didn't find the label */
    if (*linein == '.') { /* It's a dot so the rest of the line is a label name */
      linein++; /* move past the dot */
      for ( bufptr = buf ; (*linein != '\n' && *linein != '\0') ; ) {
        *bufptr++ = *linein++;
      }
      *bufptr = '\0'; /* put the oh-so-important zero in */
      if (!str_cmp(buf, label))  /* Tra-lar! */
        return line;
    }

    /* So we skip to end of this line and try again */
    line = next_line(line);
  }
  /* If we got here - we didn't find the label! */
  return NULL;
}


void parse_line( CHAR_DATA *ch, char *dest, char * source)
{
  /* NB dest *must* be big enough or horrible things may happen!!!!! */

  /* Things to parse :
   * $a-z<whtspce>  = variable 1-26
   * $victim[_thing]<whtspce> = thing (or name) of victim
   * $me[_thing]<whtspce> = thing (or name) of 'me'
   * $fighting[_thing]<whtspce> = thing (or name) of fighting
   */

  char * destptr, * sourceptr;
  char buf[MAX_STRING_LENGTH], *bufptr, *copy;
  char parambuf[MAX_STRING_LENGTH];
  CHAR_DATA *them;
  OBJ_DATA *obj;
  extern char * const dir_name[];

  return; /*bananas*/
  if (ch==NULL) {
    bug( "parse_line: NULL ch passed", NULL );
    return;
  }
  if (ch->program == NULL) {
    bug( "parse_line: ch passed with no program!", NULL );
    return;
  }

  for ( destptr = dest, sourceptr = source ; ((*sourceptr != '\0') && (*sourceptr != '\n')); )
  {
    if (*sourceptr != '$') {
      *destptr++ = *sourceptr++;
      continue;
    }
    sourceptr++;
    if ((*sourceptr == '\0') || (*sourceptr == '\n'))
     continue;         /* paranoia ... eg "sdsdsadaa ahifdshjk $" */
    if (*sourceptr == '$') {
      *destptr++ = '$';
      sourceptr++;
      continue;
    }

    /* Okay - fill a buffer with the first bit ie 'victim' or 'a'.
       Leave *sourceptr at the whitespace or '_' */
    for (bufptr = buf ; ( isalpha(*sourceptr)) ; )
      *bufptr++ = *sourceptr++;
    *bufptr = '\0';

    /* Now check the contents of buf against those we're interested in */

    them = NULL;
    obj  = NULL;
    copy = NULL;

    if (!str_cmp( buf, "victim" ))
     them = ch->program->victim;
    if (!str_cmp( buf, "teller" ))
     them = ch->program->victim;
    if (!str_cmp( buf, "leaver" ))
     them = ch->program->victim;
    if (!str_cmp( buf, "enterer" ))
     them = ch->program->victim;
    if (!str_cmp( buf, "fighter" ))
     them = ch->program->victim;
    if (!str_cmp( buf, "caster" ))
     them = ch->program->caster;
    if (!str_cmp( buf, "fighting" ))
     them = ch->fighting;
    if (!str_cmp( buf, "master" ))
     them = ch->master;
    if (!str_cmp( buf, "leader" ))
     them = ch->leader;
    if (!str_cmp( buf, "pet" ))
     them = ch->pet;
    if (!str_cmp( buf, "me" ))
     them = ch;
    if (!str_cmp( buf, "object" ))
      obj = ch->program->object;

    if (!str_cmp(buf, "text" ))
     copy = ch->program->text;
    if (!str_cmp(buf, "door" ))
     copy = dir_name[ch->program->door_num];
    if (!str_cmp(buf, "amount")) {
      sprintf( parambuf, "%d", ch->program->door_num);
      copy = parambuf;
    }
    if (!str_cmp(buf, "random" )) {
      sprintf( parambuf, "%d", number_percent ());
      copy = parambuf;
    }

    if (them == NULL && obj==NULL) {
      /* either unknown name or value from ch-> was null */
      if (buf[1] <=32 && buf[0] >= 'a' && buf[0] <= 'z') { /* was it a variable we were
        interested in? */
        copy = ch->program->variables[(buf[0]-'a')];
      } /* else leave as null */
    } else {
      /* them wasn't NULL so set up copy to point to the relevant bit of data */
      buf[0] = '\0';
      bufptr = buf;
      if (*sourceptr == '_')
      { /* aha! something other than the name! */
	sourceptr++;  /* Move pointer past the underline */
        for ( ; (isalpha(*sourceptr)) ; )
         *bufptr++ = *sourceptr++;
      }
      *bufptr = '\0';
      /* Now buf is the _'xxx' bit or '\0' if none */
      if (obj == NULL) {
	if ((buf[0] == '\0') || !str_cmp(buf, "name"))
	  copy = them->name; /* default is name */

	if (!str_cmp(buf, "race"))
	  copy = race_table[them->race].name;

	if (!str_cmp(buf, "size")) {
	  if (!IS_NPC(them)) {
	    sprintf( parambuf, "%d", them->size );
	  } else {
	    sprintf( parambuf, "%d", pc_race_table[them->race].size);
	  }
	  copy = parambuf;
	}

	if (!str_cmp(buf, "uniquely") || !str_cmp(buf, "unique")) {
	  sprintf( parambuf, "%x", (int) &(*them) );
	  copy = parambuf;
	}

	if (!str_cmp(buf, "isnpc") || !str_cmp(buf,"is!pc")) {
	  copy = IS_NPC(them)? "Yes":"No";
	}

	if (!str_cmp(buf, "ispc") || !str_cmp(buf,"is!npc")) {
	  copy = IS_NPC(them)? "No":"Yes";
	}

	if (!str_cmp(buf, "shortdescr"))
	  copy = them->short_descr;

	if (!str_cmp(buf, "longdescr"))
	  copy = them->long_descr;

	if (!str_cmp(buf, "vnum")) {
	  if (IS_NPC(them)) {
	    sprintf( parambuf, "%d", them->pIndexData->vnum);
	    copy = parambuf;
	  }
	}

	if (!str_cmp(buf, "trust")) {
	  sprintf( parambuf, "%d", get_trust(them));
	  copy = parambuf;
	}


	do_number( "level", level );
	do_number( "sex", sex );
	do_number( "hp", hit );
	do_number( "mv", move );
	do_number( "mana", mana );
	do_number( "maxhp", max_hit );
	do_number( "maxmv", max_move );
	do_number( "maxmana", max_mana );
	do_number( "gold", gold );
	do_number( "money", gold );
	do_number( "position", position );
	do_number( "align", alignment );
	do_number( "alignment", alignment );

      } else {

	if ((buf[0] == '\0') || !str_cmp(buf, "name"))
	  copy = obj->name; /* default is name */

	if (!str_cmp(buf, "owner"))
	  copy = obj->owner;

	if (!str_cmp(buf, "shortdescr"))
	  copy=obj->short_descr;

	if (!str_cmp(buf, "isenchanted"))
	  copy = (obj->enchanted)? "Yes" : "No";

	if (!str_cmp(buf, "carriedby"))
	  copy = (obj->carried_by->name);

	if (!str_cmp(buf, "longdescr"))
	  copy = (obj->description);

	if (!str_cmp(buf, "material"))
	  copy = material_table[obj->material].material_name;

	do_numberobj("weight", weight);
	do_numberobj("cost", cost);
	do_numberobj("level", level);
	do_numberobj("condition", condition);
      }
    }
    if (copy == NULL)
      copy = "NULL";

    if (copy != NULL) {
      /* If we got this far then we need to replace the $xxxx with 'copy' */
      for ( bufptr = copy ; (*bufptr != 0) ; )
	*destptr++ = *bufptr++;
    }
  }
  *destptr = '\0';
}

void mobile_signal( CHAR_DATA *ch, sh_int signum)
{
  return; /*bananas*/
  if (!IS_NPC(ch) || (ch->desc != NULL))
    return;

  if (ch==NULL) {
    bug( "mobile_signal: NULL ch passed", NULL );
    return;
  }
  if (ch->program == NULL) {
    bug( "mobile_signal: ch passed with no program!", NULL );
    return;
  }

  if (ch->program->semaphore < 0)
    return; /* MOBPROG has been disabled! */

  if ((ch->program->semaphore == 0) || (signum == SIG_LOWLEVEL)) {
    if (ch->program->signals[signum] != NULL) {
      ch->program->semaphore++;
      mobile_run( ch, ch->program->signals[signum] );
      ch->program->semaphore--;
    }
  }
}

void mobile_init( CHAR_DATA *ch )
{
  int a;

  return; /*bananas*/
  if (!IS_NPC(ch) || (ch->desc != NULL))
    return;

  if (ch==NULL) {
    bug( "mobile_init: NULL ch passed", NULL );
    return;
  }
  if (ch->program == NULL) {
    bug( "mobile_init: ch passed with no program!", NULL );
    return;
  }
  ch->program->victim = NULL;
  for ( a = 0; a<26;a++)
   ch->program->variables[a] = NULL;
  ch->program->semaphore = 0;
  mobile_run( ch, ch->program->first_line );
}

void mobile_run( CHAR_DATA *ch, char *line)
{
  char *ptr;
  int comnum;
  char buf[MAX_STRING_LENGTH];
  int numrunlines = 0;

  return; /*bananas*/

  if (ch==NULL) {
    bug( "mobile_run: NULL ch passed", NULL );
    return;
  }
  if (ch->program == NULL) {
    bug( "mobile_run: ch passed with no program!", NULL );
    return;
  }
  if (line==NULL) {
    bug( "mobile_run: NULL program passed", NULL);
    return;
  }

/* OK ... lets do it !!!!!!!!!!!!!!!!!!!!! */

  for ( ; (line != NULL) && (numrunlines<400) ; ) /* each line of program */
  {
    for ( ptr=line ; (*ptr <= 32) ; ptr++);
       /* skip the whitespaces */

    if (*ptr == '#') {
      bug( "mobile_run: Missing 'return' in %s", ch->name);
      program_error(ch);
    }

    if (*ptr != '.' && *ptr != ';') {
      /* It was a command, then */
      ptr = one_argument( ptr, buf );

      for ( comnum = 0; (mob_fun_table[comnum].name != NULL); comnum++) {
        if (!str_prefix(buf, mob_fun_table[comnum].name))
          break;
      }
      if (mob_fun_table[comnum].name == NULL) {
        bug( "mobile_run: Unknown command '%s' in %s", buf, ch->name);
        program_error(ch);
      }
      line = (*mob_fun_table[comnum].function)(ch, ptr, line);
    } else {
      /* It was a comment - move on */
      line = next_line(line);
    }
    numrunlines++;
  }
  if (numrunlines >= 400) {
    bug("mobile_run:  Mobile exceeded 400 lines - possible infinite loop!");
    program_error(ch);
  }
}

#define SIG_DO(str,num) if (!str_cmp(buf, str)) signum = num

char * mobdo_signal( CHAR_DATA *ch, char *args, char *line)
{
  char buf[MAX_STRING_LENGTH];
  sh_int signum = 1024;
  char *sigline;

  return 0; /*bananas*/
  args = one_argument(args, buf);

  SIG_DO("tick",        SIG_TICK);
  SIG_DO("mobenter",    SIG_MOBENTER);
  SIG_DO("mobleave",    SIG_MOBLEAVE);
  SIG_DO("mobfight",    SIG_MOBFIGHT);
  SIG_DO("mobendfight", SIG_MOBENDFIGHT);
  SIG_DO("objenter",    SIG_OBJENTER);
  SIG_DO("objleave",    SIG_OBJLEAVE);
  SIG_DO("dooropen",    SIG_DOOROPEN);
  SIG_DO("doorclose",   SIG_DOORCLOSE);
  SIG_DO("spokento",    SIG_SPOKENTO);
  SIG_DO("objgiven",    SIG_OBJGIVEN);
  SIG_DO("magic",       SIG_MAGIC);
  SIG_DO("sleep",       SIG_SLEEP);
  SIG_DO("lowlevel",    SIG_LOWLEVEL);
  SIG_DO("payed",       SIG_PAYED);

  if (signum == 1024) {
    bug( "mobdo_signal: Unknown signal '%s'", buf);
    program_error(ch);
  }

  args = one_argument(args,buf);

  if (buf[0] == 0) {
    ch->program->signals[signum] = NULL;
  } else {
    sigline = label_lookup( ch, buf );
    if (sigline == 0) {
      bug( "mobdo_signal: Unknown label '%s'", buf);
      program_error(ch);
    }
    ch->program->signals[signum] = sigline;
  }
  return next_line(line);
}

char * mobdo_parse( CHAR_DATA *ch, char *args, char *line)
{
  /* nice 'n' easy */
  char parsed_buf[MAX_STRING_LENGTH];

  return 0; /*bananas*/

  parse_line(ch, parsed_buf, args );
  interpret( ch, parsed_buf );
  return next_line(line);
}

char *parse_one_argument(CHAR_DATA *ch, char *args, char *buf) {
  char *retval;
  char notparsed_buf[MAX_STRING_LENGTH];

  return 0; /*bananas*/
  retval = one_argument(args,&notparsed_buf);
  parse_line(ch,buf,notparsed_buf);
  return retval;
}

char * mobdo_ifstring( CHAR_DATA *ch, char *args, char *line)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  return 0; /*bananas*/
  args = parse_one_argument(ch, args, buf1);
  args = parse_one_argument(ch, args, buf2);

  if (!str_cmp(buf1,buf2)) {
    if (!str_cmp(args,"return"))
      return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_ifstring: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);
}
char * mobdo_ifinstring( CHAR_DATA *ch, char *args, char *line)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  return 0; /*bananas*/
  args = parse_one_argument(ch, args,buf1);
  args = parse_one_argument(ch, args,buf2);

  if (!str_infix(buf1,buf2)) {
    if (!str_cmp(args,"return"))
      return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_ifinstring: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);
}
char * mobdo_ifnotinstring( CHAR_DATA *ch, char *args, char *line)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  return 0; /*bananas*/
  args = parse_one_argument(ch,args,buf1);
  args = parse_one_argument(ch,args,buf2);

  if (str_infix(buf1,buf2)) {
    if (!str_cmp(args,"return"))
      return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_ifnotinstring: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);
}

char * mobdo_ifname( CHAR_DATA *ch, char *args, char *line)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  return 0; /*bananas*/
  args = parse_one_argument(ch,args,buf1);
  args = parse_one_argument(ch,args,buf2);

  if (is_name(buf1,buf2)) {
    if (!str_cmp(args,"return"))
      return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_ifname: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);
}

char * mobdo_ifnotstring( CHAR_DATA *ch, char *args, char *line)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  args = parse_one_argument(ch,args,buf1);
  args = parse_one_argument(ch,args,buf2);

  if (str_cmp(buf1,buf2)) {
    if (!str_cmp(args,"return"))
      return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_if!string: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);
}

char * mobdo_ifnotname( CHAR_DATA *ch, char *args, char *line)
{
  char buf1[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];

  args = parse_one_argument(ch,args,buf1);
  args = parse_one_argument(ch,args,buf2);

  if (!is_name(buf1,buf2)) {
    if (!str_cmp(args,"return"))
      return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_if!name: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);
}

#define ENUMERATE(number) args = parse_one_argument(ch,args,buf); number = num_eval(buf,ch)
char * mobdo_ifrange( CHAR_DATA *ch, char *args, char *line)
{
  char buf[MAX_STRING_LENGTH];
  int a,b,c;

  ENUMERATE(a);
  ENUMERATE(b);
  ENUMERATE(c);

  if ((a <= b) && (b <= c)) {
    if (!str_cmp(args,"return"))
     return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_ifrange: Bad label %s", args);
    return 0;
  }
  return next_line(line);

}

char * mobdo_ifge( CHAR_DATA *ch, char *args, char *line)
{
  char buf[MAX_STRING_LENGTH];
  int a,b;

  ENUMERATE(a);
  ENUMERATE(b);

  if (a >= b) {
    if (!str_cmp(args,"return"))
     return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_ifge: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);

}

char * mobdo_ifle( CHAR_DATA *ch, char *args, char *line)
{
  char buf[MAX_STRING_LENGTH];
  int a,b;

  ENUMERATE(a);
  ENUMERATE(b);

  if (a <= b) {
    if (!str_cmp(args,"return"))
     return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_ifle: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);

}

char * mobdo_ifgt( CHAR_DATA *ch, char *args, char *line)
{
  char buf[MAX_STRING_LENGTH];
  int a,b;

  ENUMERATE(a);
  ENUMERATE(b);

  if (a > b) {
    if (!str_cmp(args,"return"))
     return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_ifgt: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);

}

char * mobdo_iflt( CHAR_DATA *ch, char *args, char *line)
{
  char buf[MAX_STRING_LENGTH];
  int a,b;

  ENUMERATE(a);
  ENUMERATE(b);

  if (a < b) {
    if (!str_cmp(args,"return"))
     return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_iflt: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);

}

char * mobdo_ifeq( CHAR_DATA *ch, char *args, char *line)
{
  char buf[MAX_STRING_LENGTH];
  int a,b;

  ENUMERATE(a);
  ENUMERATE(b);

  if (a == b) {
    if (!str_cmp(args,"return"))
     return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_ifeq: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);

}

char * mobdo_ifne( CHAR_DATA *ch, char *args, char *line)
{
  char buf[MAX_STRING_LENGTH];
  int a,b;

  ENUMERATE(a);
  ENUMERATE(b);

  if (a != b) {
    if (!str_cmp(args,"return"))
     return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_ifne: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);

}

char * mobdo_return( CHAR_DATA *ch, char *args, char *line)
{
  return 0;
}

char * mobdo_goto( CHAR_DATA *ch, char *args, char *line)
{
  if (label_lookup(ch, args))
    return label_lookup(ch, args);
  bug( "mobdo_goto: Bad label %s", args);
  program_error(ch);
  return NULL;
}

char * mobdo_chance( CHAR_DATA *ch, char *args, char *line)
{
  char buf[MAX_STRING_LENGTH];
  int chance;
  args=parse_one_argument(ch, args, buf);

  chance = num_eval(buf,ch);

  if ((chance > 100) || (chance < 0)) {
    bug( "mobdo_chance: !( 0<= %d <= 100)", chance);
    program_error(ch);
  }

  if (number_percent() <= chance) {
    if (!str_cmp(args,"return"))
      return 0;
    if (label_lookup(ch, args))
      return label_lookup(ch, args);
    bug( "mobdo_chance: Bad label %s", args);
    program_error(ch);
  }
  return next_line(line);
}

CHAR_DATA * mobscan( CHAR_DATA *me, CHAR_DATA *ch )
{

  if (ch == NULL)
    return NULL;

  for ( ; ; ) {
    if (can_see(me,ch) || (ch==NULL))
      break;
    ch = ch->next_in_room;
  }
  return ch;
}

char * mobdo_scan( CHAR_DATA *ch, char *args, char *line)
{
  ch->program->victim = mobscan(ch, ch->in_room->people);
  return next_line(line);
}

char * mobdo_scanon( CHAR_DATA *ch, char *args, char *line)
{
  if (ch->program->victim != NULL)
    ch->program->victim = ch->program->victim->next_in_room;
  ch->program->victim = mobscan(ch, ch->program->victim);
  return next_line(line);
}

char * mobdo_set( CHAR_DATA *ch, char *args, char *line)
{
  char buf[MAX_STRING_LENGTH];
  int var;
  args = one_argument(args,buf);
  if (buf[0] < 'a' && buf[0] > 'z') {
    bug("mobdo_set: bad variable '%s'",buf);
    program_error(ch);
  }
  var = buf[0]-'a';
  if (ch->program->variables[var] != NULL)
    free_string(ch->program->variables[var]);
  for ( ; isspace(*args) ; args++);
  parse_line(ch, buf, args);
  ch->program->variables[var] = str_dup(buf);
  return next_line(line);
}

char * mobdo_seteval( CHAR_DATA *ch, char *args, char *line)
{
  char buf[MAX_STRING_LENGTH];
  int var;
  args = one_argument(args,buf);
  if (buf[0] < 'a' && buf[0] > 'z') {
    bug("mobdo_set: bad variable '%s'",buf);
    program_error(ch);
  }
  var = buf[0]-'a';
  parse_line(ch, buf, args);
  if (ch->program->variables[var] != NULL)
    free_string(ch->program->variables[var]);
  sprintf( buf, "%d", num_eval(buf,ch));
  ch->program->variables[var] = str_dup(buf);
  return next_line(line);
}

