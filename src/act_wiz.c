/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  act_wiz.c: commands for immortals                                    */
/*                                                                       */
/*************************************************************************/


#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#if defined(riscos)
#include "sys/types.h"
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "merc.h"
#include "db.h"
#include "magic.h"

#include "buffer.h"

#include "flags.h"  /* for the setting of flags blah blah blah :) */


/* command procedures needed */
DECLARE_DO_FUN(do_rstat		);
DECLARE_DO_FUN(do_mstat		);
DECLARE_DO_FUN(do_ostat		);
DECLARE_DO_FUN(do_rset		);
DECLARE_DO_FUN(do_mset		);
DECLARE_DO_FUN(do_oset		);
DECLARE_DO_FUN(do_sset		);
DECLARE_DO_FUN(do_mfind		);
DECLARE_DO_FUN(do_ofind		);
DECLARE_DO_FUN(do_slookup	);
DECLARE_DO_FUN(do_mload		);
DECLARE_DO_FUN(do_oload		);
DECLARE_DO_FUN(do_force		);
DECLARE_DO_FUN(do_quit		);
DECLARE_DO_FUN(do_save		);
DECLARE_DO_FUN(do_look		);
DECLARE_DO_FUN(do_stand         );
DECLARE_DO_FUN(do_maffects	);
DECLARE_DO_FUN(do_mspells	);
DECLARE_DO_FUN(do_mskills	);
DECLARE_DO_FUN(do_mpracs	);
DECLARE_DO_FUN(do_minfo         );


/*
 * Local functions.
 */
ROOM_INDEX_DATA *	find_location	args( ( CHAR_DATA *ch, char *arg ) );

/* Permits or denies a player from playing the Mud from a PERMIT banned site */
void do_permit ( CHAR_DATA *ch, char *argument ) {
  CHAR_DATA *victim;
  char buf[MAX_STRING_LENGTH];
  int flag=1;
  if (IS_NPC(ch))
    return;
  if (argument[0]=='-') {
    argument++;
    flag = 0;
  }
  if (argument[0]=='+')
    argument++;
  victim = get_char_room(ch,argument);
  if (victim==NULL || IS_NPC(victim)) {
    send_to_char("Permit whom?\n\r",ch);
    return;
  }
  if (flag) {
    set_extra(victim, EXTRA_PERMIT);
  } else {
    remove_extra(victim, EXTRA_PERMIT);
  }
  sprintf(buf,"PERMIT flag %s for %s.\n\r", (flag)?"set":"removed", victim->
name);
  send_to_char(buf,ch);
}


/* equips a character */
void do_outfit ( CHAR_DATA *ch, char *argument )
{
  (void)argument;
   OBJ_DATA *obj;
   char buf [MAX_STRING_LENGTH];

   if (ch->level > 5 || IS_NPC(ch))
   {
      send_to_char("Find it yourself!\n\r",ch);
      return;
   }

   if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) == NULL )
   {
      obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_BANNER), 0 );
      obj->cost = 0;
      obj_to_char( obj, ch );
      equip_char( ch, obj, WEAR_LIGHT );
   }

   if ( ( obj = get_eq_char( ch, WEAR_BODY ) ) == NULL )
   {
      obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_VEST), 0 );
      obj->cost = 0;
      obj_to_char( obj, ch );
      equip_char( ch, obj, WEAR_BODY );
   }

   if ( ( obj = get_eq_char( ch, WEAR_SHIELD ) ) == NULL )
   {
      obj = create_object( get_obj_index(OBJ_VNUM_SCHOOL_SHIELD), 0 );
      obj->cost = 0;
      obj_to_char( obj, ch );
      equip_char( ch, obj, WEAR_SHIELD );
   }

   if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL )
   {
      obj = create_object( get_obj_index(class_table[ch->class].weapon),0);
      obj_to_char( obj, ch );
      equip_char( ch, obj, WEAR_WIELD );
   }

   sprintf( buf, "You have been equipped by %s.\n\r", deity_name);
   send_to_char( buf,ch);
}



/* RT nochannels command, for those spammers */
void do_nochannels( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if ( arg[0] == '\0' )
   {
      send_to_char( "Nochannel whom?", ch );
      return;
   }

   if ( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }

   if ( IS_SET(victim->comm, COMM_NOCHANNELS) )
   {
      REMOVE_BIT(victim->comm, COMM_NOCHANNELS);
      send_to_char( "The gods have restored your channel priviliges.\n\r",
      victim );
      send_to_char( "NOCHANNELS removed.\n\r", ch );
   }
   else
   {
      SET_BIT(victim->comm, COMM_NOCHANNELS);
      send_to_char( "The gods have revoked your channel priviliges.\n\r",
      victim );
      send_to_char( "NOCHANNELS set.\n\r", ch );
   }

   return;
}

void do_bamfin( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];

   if ( !IS_NPC(ch) )
   {
      smash_tilde( argument );

      if (argument[0] == '\0')
      {
         sprintf(buf,"Your poofin is %s\n\r",ch->pcdata->bamfin);
         send_to_char(buf,ch);
         return;
      }

      if ( strstr(argument,ch->name) == NULL)
      {
         send_to_char("You must include your name.\n\r",ch);
         return;
      }

      free_string( ch->pcdata->bamfin );
      ch->pcdata->bamfin = str_dup( argument );

      sprintf(buf,"Your poofin is now %s\n\r",ch->pcdata->bamfin);
      send_to_char(buf,ch);
   }
   return;
}



void do_bamfout( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];

   if ( !IS_NPC(ch) )
   {
      smash_tilde( argument );

      if (argument[0] == '\0')
      {
         sprintf(buf,"Your poofout is %s\n\r",ch->pcdata->bamfout);
         send_to_char(buf,ch);
         return;
      }

      if ( strstr(argument,ch->name) == NULL)
      {
         send_to_char("You must include your name.\n\r",ch);
         return;
      }

      free_string( ch->pcdata->bamfout );
      ch->pcdata->bamfout = str_dup( argument );

      sprintf(buf,"Your poofout is now %s\n\r",ch->pcdata->bamfout);
      send_to_char(buf,ch);
   }
   return;
}



void do_deny( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );
   if ( arg[0] == '\0' )
   {
      send_to_char( "Deny whom?\n\r", ch );
      return;
   }

   if ( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( IS_NPC(victim) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   if ( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }

   SET_BIT(victim->act, PLR_DENY);
   send_to_char( "You are denied access!\n\r", victim );
   send_to_char( "OK.\n\r", ch );
   save_char_obj(victim);
   do_quit( victim, "" );

   return;
}



void do_disconnect( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   DESCRIPTOR_DATA *d;
   CHAR_DATA *victim;

   one_argument( argument, arg );
   if ( arg[0] == '\0' )
   {
      send_to_char( "Disconnect whom?\n\r", ch );
      return;
   }

   if (argument[0]=='+') {
      argument++;
      for (d = descriptor_list; d ; d=d->next) {
         if (d->character && !str_cmp(d->character->name,argument)) {
            close_socket(d);
            send_to_char( "Ok.\n\r", ch);
            return;
         }
      }
      send_to_char( "Couldn't find a matching descriptor.\n\r", ch);
      return;
   }
   else {
      if ( ( victim = get_char_world( ch, arg ) ) == NULL )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }

      if ( victim->desc == NULL )
      {
         act( "$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR );
         return;
      }

      for ( d = descriptor_list; d != NULL; d = d->next )
      {
         if ( d == victim->desc )
         {
            close_socket( d );
            send_to_char( "Ok.\n\r", ch );
            return;
         }
      }

      bug( "Do_disconnect: desc not found.", 0 );
      send_to_char( "Descriptor not found!\n\r", ch );
      return;
   }
}

void do_pardon( CHAR_DATA *ch, char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if ( arg1[0] == '\0' || arg2[0] == '\0' )
   {
      send_to_char( "Syntax: pardon <character> <killer|thief>.\n\r", ch );
      return;
   }

   if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( IS_NPC(victim) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   if ( !str_cmp( arg2, "killer" ) )
   {
      if ( IS_SET(victim->act, PLR_KILLER) )
      {
         REMOVE_BIT( victim->act, PLR_KILLER );
         send_to_char( "Killer flag removed.\n\r", ch );
         send_to_char( "You are no longer a KILLER.\n\r", victim );
      }
      return;
   }

   if ( !str_cmp( arg2, "thief" ) )
   {
      if ( IS_SET(victim->act, PLR_THIEF) )
      {
         REMOVE_BIT( victim->act, PLR_THIEF );
         send_to_char( "Thief flag removed.\n\r", ch );
         send_to_char( "You are no longer a THIEF.\n\r", victim );
      }
      return;
   }

   send_to_char( "Syntax: pardon <character> <killer|thief>.\n\r", ch );
   return;
}



void do_echo( CHAR_DATA *ch, char *argument )
{
   DESCRIPTOR_DATA *d;

   if ( argument[0] == '\0' )
   {
      send_to_char( "Global echo what?\n\r", ch );
      return;
   }

   for ( d = descriptor_list; d; d = d->next )
   {
      if ( d->connected == CON_PLAYING )
      {
         if (get_trust(d->character) >= get_trust(ch))
            send_to_char( "global> ",d->character);
         send_to_char( argument, d->character );
         send_to_char( "\n\r",   d->character );
      }
   }

   return;
}



void do_recho( CHAR_DATA *ch, char *argument )
{
   DESCRIPTOR_DATA *d;

   if ( argument[0] == '\0' )
   {
      send_to_char( "Local echo what?\n\r", ch );

      return;
   }

   for ( d = descriptor_list; d; d = d->next )
   {
      if ( d->connected == CON_PLAYING
      &&   d->character->in_room == ch->in_room )
      {
         if (get_trust(d->character) >= get_trust(ch) && ch->in_room->vnum !=
1222)
            send_to_char( "local> ",d->character);
         send_to_char( argument, d->character );
         send_to_char( "\n\r",   d->character );
      }
   }

   return;
}


void do_zecho(CHAR_DATA *ch, char *argument)
{
    DESCRIPTOR_DATA *d;

    if (argument[0] == '\0')
    {
	send_to_char("Zone echo what?\n\r",ch);
	return;
    }

    for (d = descriptor_list; d; d = d->next)
    {
	if (d->connected == CON_PLAYING
	&&  d->character->in_room != NULL && ch->in_room != NULL
	&&  d->character->in_room->area == ch->in_room->area)
	{
	    if (get_trust(d->character) >= get_trust(ch))
		send_to_char("zone> ",d->character);
	    send_to_char(argument,d->character);
	    send_to_char("\n\r",d->character);
	}
    }
}



void do_pecho( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   argument = one_argument(argument, arg);

   if ( argument[0] == '\0' || arg[0] == '\0' )
   {
      send_to_char("Personal echo what?\n\r", ch);
      return;
   }

   if  ( (victim = get_char_world(ch, arg) ) == NULL )
   {
      send_to_char("Target not found.\n\r",ch);
      return;
   }

   if (get_trust(victim) >= get_trust(ch) && get_trust(ch) != MAX_LEVEL)
      send_to_char( "personal> ",victim);

   send_to_char(argument,victim);
   send_to_char("\n\r",victim);
   send_to_char( "personal> ",ch);
   send_to_char(argument,ch);
   send_to_char("\n\r",ch);
}


ROOM_INDEX_DATA *find_location( CHAR_DATA *ch, char *arg )
{
   CHAR_DATA *victim;
   OBJ_DATA *obj;

   if ( is_number(arg) )
      return get_room_index( atoi( arg ) );

   if ( ( !str_cmp(arg, "here") ) )
     return ch->in_room;

   if ( ( victim = get_char_world( ch, arg ) ) != NULL )
      return victim->in_room;

   if ( ( obj = get_obj_world( ch, arg ) ) != NULL )
      return obj->in_room;

   return NULL;
}



void do_transfer( CHAR_DATA *ch, char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   ROOM_INDEX_DATA *location;
   DESCRIPTOR_DATA *d;
   CHAR_DATA *victim;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if ( arg1[0] == '\0' )
   {
      send_to_char( "Transfer whom (and where)?\n\r", ch );
      return;
   }

   if ( !str_cmp( arg1, "all" ) )
   {
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
         if ( d->connected == CON_PLAYING
         &&   d->character != ch
         &&   d->character->in_room != NULL
         &&   can_see( ch, d->character ) )
         {
            char buf[MAX_STRING_LENGTH];
            sprintf( buf, "%s %s", d->character->name, arg2 );
            do_transfer( ch, buf );
         }
      }
      return;
   }

   /*
        * Thanks to Grodyn for the optional location parameter.
        */
   if ( arg2[0] == '\0' )
   {
      location = ch->in_room;
   }
   else
   {
      if ( ( location = find_location( ch, arg2 ) ) == NULL )
      {
         send_to_char( "No such location.\n\r", ch );
         return;
      }

     	if ( room_is_private( location ) &&
	     (get_trust(ch) < IMPLEMENTOR ) )
      	{
      	    send_to_char( "That room is private right now.\n\r", ch );
      	    return;
      	}
   }

   if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( victim->in_room == NULL )
   {
      send_to_char( "They are in limbo.\n\r", ch );
      return;
   }

   if ( victim->fighting != NULL )
      stop_fighting( victim, TRUE );
   if (victim->riding != NULL)
      unride_char (victim, victim->riding);
   act( "$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM );
   char_from_room( victim );
   char_to_room( victim, location );
   act( "$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM );
   if ( ch != victim )
      act( "$n has transferred you.", ch, NULL, victim, TO_VICT );
   do_look( victim, "auto" );
   send_to_char( "Ok.\n\r", ch );
}

void do_wizlock( CHAR_DATA *ch, char *argument )
{
  (void)argument;
   extern bool wizlock;
   wizlock = !wizlock;

   if ( wizlock )
      send_to_char( "Game wizlocked.\n\r", ch );
   else
      send_to_char( "Game un-wizlocked.\n\r", ch );

   return;
}

/* RT anti-newbie code */

void do_newlock( CHAR_DATA *ch, char *argument )
{
  (void)argument;
   extern bool newlock;
   newlock = !newlock;

   if ( newlock )
      send_to_char( "New characters have been locked out.\n\r", ch );
   else
      send_to_char( "Newlock removed.\n\r", ch );

   return;
}

void do_at( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   ROOM_INDEX_DATA *location;
   ROOM_INDEX_DATA *original;
   CHAR_DATA *wch;

   argument = one_argument( argument, arg );

   if ( arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "At where what?\n\r", ch );
      return;
   }

   if ( ( location = find_location( ch, arg ) ) == NULL )
   {
      send_to_char( "No such location.\n\r", ch );
      return;
   }
   if ( (ch->in_room != NULL) && location == ch->in_room ) {
     send_to_char("But that's in here.......\n\r", ch);
     return;
   }
   if ( room_is_private( location ) &&
	( get_trust(ch) < IMPLEMENTOR ) ) {
       send_to_char( "That room is private right now.\n\r", ch );
       return;
     }
   original = ch->in_room;
   char_from_room( ch );
   char_to_room( ch, location );
   interpret( ch, argument );

   /*
        * See if 'ch' still exists before continuing!
        * Handles 'at XXXX quit' case.
        */
   for ( wch = char_list; wch != NULL; wch = wch->next )
   {
      if ( wch == ch )
      {
         char_from_room( ch );
         char_to_room( ch, original );
         break;
      }
   }

   return;
}



void do_goto( CHAR_DATA *ch, char *argument )
{
   ROOM_INDEX_DATA *location;
   CHAR_DATA *rch;

   if ( argument[0] == '\0' )
   {
      send_to_char( "Goto where?\n\r", ch );
      return;
   }

   if ( ( location = find_location( ch, argument ) ) == NULL )
   {
      send_to_char( "No such location.\n\r", ch );
      return;
   }

   if ( room_is_private( location ) &&
	(get_trust(ch) < IMPLEMENTOR ) ) {
   	send_to_char( "That room is private right now.\n\r", ch );
   	return;
   }

   if ( ch->fighting != NULL )
      stop_fighting( ch, TRUE );

   for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
   {
      if (get_trust(rch) >= ch->invis_level)
      {
         if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
            act("$t",ch,ch->pcdata->bamfout,rch,TO_VICT);
         else
            act("$n leaves in a swirling mist.",ch,NULL,rch,TO_VICT);
      }
   }

   char_from_room( ch );
   char_to_room( ch, location );
   if (ch->pet)
   {
      char_from_room( ch->pet );
      char_to_room( ch->pet, location );
   }

   for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room)
   {
      if (get_trust(rch) >= ch->invis_level)
      {
         if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
            act("$t",ch,ch->pcdata->bamfin,rch,TO_VICT);
         else
            act("$n appears in a swirling mist.",ch,NULL,rch,TO_VICT);
      }
   }

   do_look( ch, "auto" );
   return;
}

/* RT to replace the 3 stat commands */

void do_stat ( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   char *string;
   OBJ_DATA *obj;
   ROOM_INDEX_DATA *location;
   CHAR_DATA *victim;

   string = one_argument(argument, arg);
   if ( arg[0] == '\0')
   {
      send_to_char("Syntax:\n\r",ch);
      send_to_char("  stat <name>\n\r",ch);
      send_to_char("  stat obj <name>\n\r",ch);
      send_to_char("  stat mob <name>\n\r",ch);
      send_to_char("  stat room <number>\n\r",ch);
      send_to_char("  stat <skills/spells/info/pracs/affects> <name>\n\r",ch);
      send_to_char("  stat prog <mobprog>\n\r",ch);
      return;
   }

   if (!str_cmp(arg,"room"))
   {
      do_rstat(ch,string);
      return;
   }

   if (!str_cmp(arg,"obj"))
   {
      do_ostat(ch,string);
      return;
   }

   if(!str_cmp(arg,"char")  || !str_cmp(arg,"mob"))
   {
      do_mstat(ch,string);
      return;
   }

   if(!str_cmp(arg, "skills"))
   {
      do_mskills(ch,string);
      return;
   }
   if(!str_cmp(arg, "affects"))
   {
      do_maffects(ch,string);
      return;
   }
   if(!str_cmp(arg, "pracs"))
   {
      do_mpracs(ch,string);
      return;
   }
   if(!str_cmp(arg, "info"))
   {
      do_minfo(ch,string);
      return;
   }
   if(!str_cmp(arg, "spells"))
   {
      do_mspells(ch,string);
      return;
   }

/* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
   if(!str_cmp(arg, "prog"))
   {
	   do_mpstat(ch,string);     /* in mob_commands.c */
      return;
   }

   /* do it the old way */

   obj = get_obj_world(ch,argument);
   if (obj != NULL)
   {
      do_ostat(ch,argument);
      return;
   }

   victim = get_char_world(ch,argument);
   if (victim != NULL)
   {
      do_mstat(ch,argument);
      return;
   }

   location = find_location(ch,argument);
   if (location != NULL)
   {
      do_rstat(ch,argument);
      return;
   }

   send_to_char("Nothing by that name found anywhere.\n\r",ch);
}





void do_rstat( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   ROOM_INDEX_DATA *location;
   OBJ_DATA *obj;
   CHAR_DATA *rch;
   int door;

   one_argument( argument, arg );
   location = ( arg[0] == '\0' ) ? ch->in_room : find_location( ch, arg );
   if ( location == NULL )
   {
      send_to_char( "No such location.\n\r", ch );
      return;
   }

   if ( ch->in_room != location && room_is_private( location ) &&
   	 get_trust(ch) < IMPLEMENTOR )  {
     send_to_char( "That room is private right now.\n\r", ch );
     return;
   }

   sprintf( buf, "Name: '%s.'\n\rArea: '%s'.\n\r",
   location->name,
   location->area->name );
   send_to_char( buf, ch );

   sprintf( buf,
   "Vnum: %d.  Sector: %d.  Light: %d.\n\r",
   location->vnum,
   location->sector_type,
   location->light );
   send_to_char( buf, ch );
   send_to_char("Flags:", ch);
   display_flags( ROOM_FLAGS, ch, location->room_flags);
   sprintf( buf, "Description:\n\r%s",
   location->description );
   send_to_char( buf, ch );

   if ( location->extra_descr != NULL )
   {
      EXTRA_DESCR_DATA *ed;

      send_to_char( "Extra description keywords: '", ch );
      for ( ed = location->extra_descr; ed; ed = ed->next )
      {
         send_to_char( ed->keyword, ch );
         if ( ed->next != NULL )
            send_to_char( " ", ch );
      }
      send_to_char( "'.\n\r", ch );
   }

   send_to_char( "Characters:", ch );
   for ( rch = location->people; rch; rch = rch->next_in_room )
   {
      if (can_see(ch,rch))
      {
         send_to_char( " ", ch );
         one_argument( rch->name, buf );
         send_to_char( buf, ch );
      }
   }

   send_to_char( ".\n\rObjects:   ", ch );
   for ( obj = location->contents; obj; obj = obj->next_content )
   {
      send_to_char( " ", ch );
      one_argument( obj->name, buf );
      send_to_char( buf, ch );
   }
   send_to_char( ".\n\r", ch );

   for ( door = 0; door <= 5; door++ )
   {
      EXIT_DATA *pexit;

      if ( ( pexit = location->exit[door] ) != NULL )
      {
         sprintf( buf,
         "Door: %d.  To: %d.  Key: %d.  Exit flags: %d.\n\rKeyword: '%s'.  Description: %s",

         door,
         (pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
         pexit->key,
         pexit->exit_info,
         pexit->keyword,
         pexit->description[0] != '\0'
         ? pexit->description : "(none).\n\r" );
         send_to_char( buf, ch );
      }
   }

   return;
}



void do_ostat( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   AFFECT_DATA *paf;
   OBJ_DATA *obj;
   OBJ_INDEX_DATA *pObjIndex;
   sh_int dam_type;
   int vnum;


   /* this will help prevent major memory allocations - a crash bug!
    --Faramir */
   if( strlen( argument ) < 2 && !isdigit( argument[0] ) ) {
	   send_to_char( "Please be more specific.\n\r", ch );
	   return;
   }
   one_argument( argument, arg );

   if ( arg[0] == '\0' )
   {
      send_to_char( "Stat what?\n\r", ch );
      return;
   }

   if( isdigit( argument[0] )) {
	   vnum = atoi( argument );
	   if((pObjIndex = get_obj_index( vnum )) == NULL ) {
		   send_to_char( "Nothing like that in hell, earth, or heaven.\n\r", ch );
		   return;
	   }
	   send_to_char( "Template of object:\n\r", ch );
	   sprintf( buf, "Name(s): %s\n\r", pObjIndex->name );
	   send_to_char( buf, ch );

	   sprintf( buf, "Vnum: %d  Format: %s  Type: %s\n\r",
		    pObjIndex->vnum, pObjIndex->new_format ? "new" : "old",
		    item_index_type_name( pObjIndex ));
	   send_to_char( buf, ch );

	   sprintf( buf, "Short description: %s\n\rLong description: %s\n\r",
		    pObjIndex->short_descr, pObjIndex->description );
	   send_to_char( buf, ch );
	   
	   sprintf( buf, "Wear bits: %s\n\rExtra bits: %s\n\r",
		    wear_bit_name(pObjIndex->wear_flags), 
		    extra_bit_name( pObjIndex->extra_flags ) );
	   send_to_char( buf, ch );
	   
	   sprintf( buf, "Wear string: %s\n\r",
		    pObjIndex->wear_string);
	   send_to_char( buf, ch );
	   
	   sprintf( buf, "Weight: %d\n\r",
		    pObjIndex->weight );
	   send_to_char( buf, ch );
	   
	   sprintf( buf, "Level: %d  Cost: %d  Condition: %d\n\r",
		    pObjIndex->level, pObjIndex->cost, 
		    pObjIndex->condition );
	   send_to_char( buf, ch );
	   
	   sprintf( buf, "Values: %d %d %d %d %d\n\r",
		    pObjIndex->value[0], pObjIndex->value[1], 
		    pObjIndex->value[2], pObjIndex->value[3],
		    pObjIndex->value[4] );
	   send_to_char( buf, ch );
	   send_to_char( "Please load this object if you need to know more about it.\n\r", ch );
	   return;
   }

   if ( ( obj = get_obj_world( ch, argument ) ) == NULL )
   {
      send_to_char( "Nothing like that in hell, earth, or heaven.\n\r", ch );
      return;
   }

   sprintf( buf, "Name(s): %s\n\r",
   obj->name );
   send_to_char( buf, ch );

   sprintf( buf, "Vnum: %d  Format: %s  Type: %s  Resets: %d\n\r",
   obj->pIndexData->vnum, obj->pIndexData->new_format ? "new" : "old",
   item_type_name(obj), obj->pIndexData->reset_num );
   send_to_char( buf, ch );

   sprintf( buf, "Short description: %s\n\rLong description: %s\n\r",
   obj->short_descr, obj->description );
   send_to_char( buf, ch );

   sprintf( buf, "Wear bits: %s\n\rExtra bits: %s\n\r",
   wear_bit_name(obj->wear_flags), extra_bit_name( obj->extra_flags ) );
   send_to_char( buf, ch );

   sprintf( buf, "Wear string: %s\n\r",
	    obj->wear_string);
   send_to_char( buf, ch );

   sprintf( buf, "Number: %d/%d  Weight: %d/%d\n\r",
   1,           get_obj_number( obj ),
   obj->weight, get_obj_weight( obj ) );
   send_to_char( buf, ch );

   sprintf( buf, "Level: %d  Cost: %d  Condition: %d  Timer: %d\n\r",
   obj->level, obj->cost, obj->condition, obj->timer );
   send_to_char( buf, ch );

   sprintf( buf,
   "In room: %d  In object: %s  Carried by: %s  Wear_loc: %d\n\r",
   obj->in_room    == NULL    ?        0 : obj->in_room->vnum,
   obj->in_obj     == NULL    ? "(none)" : obj->in_obj->short_descr,
   obj->carried_by == NULL    ? "(none)" :
   can_see(ch,obj->carried_by) ? obj->carried_by->name
   : "someone",
   obj->wear_loc );
   send_to_char( buf, ch );

   sprintf( buf, "Values: %d %d %d %d %d\n\r",
   obj->value[0], obj->value[1], obj->value[2], obj->value[3],
   obj->value[4] );
   send_to_char( buf, ch );

   /* now give out vital statistics as per identify */

   switch ( obj->item_type )
   {
   case ITEM_SCROLL:
   case ITEM_POTION:
   case ITEM_PILL:
   case ITEM_BOMB:
      sprintf( buf, "Level %d spells of:", obj->value[0] );
      send_to_char( buf, ch );

      if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
      {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[1]].name, ch );
         send_to_char( "'", ch );
      }

      if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
      {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[2]].name, ch );
         send_to_char( "'", ch );
      }

      if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
      {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[3]].name, ch );
         send_to_char( "'", ch );
      }

      if ( (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL)
      && obj->item_type == ITEM_BOMB )
      {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[4]].name, ch );
         send_to_char( "'", ch );
      }

      send_to_char( ".\n\r", ch );
      break;

   case ITEM_WAND:
   case ITEM_STAFF:
      sprintf( buf, "Has %d(%d) charges of level %d",
      obj->value[1], obj->value[2], obj->value[0] );
      send_to_char( buf, ch );

      if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
      {
         send_to_char( " '", ch );
         send_to_char( skill_table[obj->value[3]].name, ch );
         send_to_char( "'", ch );
      }

      send_to_char( ".\n\r", ch );
      break;

   case ITEM_WEAPON:
      send_to_char("Weapon type is ",ch);
      switch (obj->value[0])
      {
         case(WEAPON_EXOTIC) 	:
         send_to_char("exotic\n\r",ch);
         break;
         case(WEAPON_SWORD)  	:
         send_to_char("sword\n\r",ch);
         break;
         case(WEAPON_DAGGER) 	:
         send_to_char("dagger\n\r",ch);
         break;
         case(WEAPON_SPEAR)	:
         send_to_char("spear/staff\n\r",ch);
         break;
         case(WEAPON_MACE) 	:
         send_to_char("mace/club\n\r",ch);
         break;
         case(WEAPON_AXE)	:
         send_to_char("axe\n\r",ch);
         break;
         case(WEAPON_FLAIL)	:
         send_to_char("flail\n\r",ch);
         break;
         case(WEAPON_WHIP)	:
         send_to_char("whip\n\r",ch);
         break;
         case(WEAPON_POLEARM)	:
         send_to_char("polearm\n\r",ch);
         break;
      default			:
         send_to_char("unknown\n\r",ch);
         break;
      }
      if (obj->pIndexData->new_format)
         sprintf(buf,"Damage is %dd%d (average %d)\n\r",
         obj->value[1],obj->value[2],
         (1 + obj->value[2]) * obj->value[1] / 2);
      else
      sprintf( buf, "Damage is %d to %d (average %d)\n\r",
      obj->value[1], obj->value[2],
      ( obj->value[1] + obj->value[2] ) / 2 );
      send_to_char( buf, ch );

      if (obj->value[4])  /* weapon flags */
      {
         sprintf(buf,"Weapons flags: %s\n\r",weapon_bit_name(obj->value[4]));
         send_to_char(buf,ch);
      }

      dam_type = attack_table[obj->value[3]].damage;
      send_to_char("Damage type is ", ch);
      switch(dam_type)
      {
         case(DAM_NONE):
         sprintf(buf, "none.\n\r");
         break;
         case(DAM_BASH):
         sprintf(buf, "bash.\n\r");
         break;
         case(DAM_PIERCE):
         sprintf(buf, "pierce.\n\r");
         break;
         case(DAM_SLASH):
         sprintf(buf, "slash.\n\r");
         break;
         case(DAM_FIRE):
         sprintf(buf, "fire.\n\r");
         break;
         case(DAM_COLD):
         sprintf(buf, "cold.\n\r");
         break;
         case(DAM_LIGHTNING):
         sprintf(buf, "lightning.\n\r");
         break;
         case(DAM_ACID):
         sprintf(buf, "acid.\n\r");
         break;
         case(DAM_POISON):
         sprintf(buf, "poison.\n\r");
         break;
         case(DAM_NEGATIVE):
         sprintf(buf, "negative.\n\r");
         break;
         case(DAM_HOLY):
         sprintf(buf, "holy.\n\r");
         break;
         case(DAM_ENERGY):
         sprintf(buf, "energy.\n\r");
         break;
         case(DAM_MENTAL):
         sprintf(buf, "mental.\n\r");
         break;
         case(DAM_DISEASE):
         sprintf(buf, "disease.\n\r");
         break;
         case(DAM_DROWNING):
         sprintf(buf, "drowning.\n\r");
         break;
         case(DAM_LIGHT):
         sprintf(buf, "light.\n\r");
         break;
         case(DAM_OTHER):
         sprintf(buf, "other.\n\r");
         break;
         case(DAM_HARM):
         sprintf(buf, "harm.\n\r");
         break;
      default:
         sprintf(buf, "unknown!!!!\n\r");
         bug( "ostat: Unknown damage type %d", dam_type);
         break;
      }
      send_to_char(buf, ch);
      break;

   case ITEM_ARMOR:
      sprintf( buf,
      "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n\r",
      obj->value[0], obj->value[1], obj->value[2], obj->value[3] );
      send_to_char( buf, ch );
      break;

   case ITEM_PORTAL:
      sprintf( buf, "Portal to %s (%d).\n\r", obj->destination->name,
      obj->destination->vnum);
      send_to_char( buf, ch);
      break;
   }

   if ( obj->extra_descr != NULL || obj->pIndexData->extra_descr != NULL )
   {
      EXTRA_DESCR_DATA *ed;

      send_to_char( "Extra description keywords: '", ch );

      for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
      {
         send_to_char( ed->keyword, ch );
         if ( ed->next != NULL )
            send_to_char( " ", ch );
      }

      for ( ed = obj->pIndexData->extra_descr; ed != NULL; ed = ed->next )
      {
         send_to_char( ed->keyword, ch );
         if ( ed->next != NULL )
            send_to_char( " ", ch );
      }

      send_to_char( "'\n\r", ch );
   }

   for ( paf = obj->affected; paf != NULL; paf = paf->next )
   {
      sprintf( buf, "Affects %s by %d, level %d.\n\r",
      affect_loc_name( paf->location ), paf->modifier,paf->level );
      send_to_char( buf, ch );
   }

   if (!obj->enchanted)
      for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
      {
         sprintf( buf, "Affects %s by %d, level %d.\n\r",
         affect_loc_name( paf->location ), paf->modifier,paf->level );
         send_to_char( buf, ch );
      }

   return;
}

void do_mskills( CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  char skill_list[LEVEL_HERO][MAX_STRING_LENGTH];
  char skill_columns[LEVEL_HERO];
  int sn,lev;
  bool found = FALSE;

  one_argument( argument, arg );

  if ( arg[0] == '\0' )
    {
      send_to_char( "Stat skills whom?\n\r", ch );
      return;
    }

  if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
      send_to_char( "They aren't here.\n\r", ch );
      return;
    }

   if (IS_NPC(victim))
      return;

   sprintf(buf, "Skill list for %s:\n\r", victim->name);
   send_to_char( buf, ch );

   /* initilize data */
   for (lev = 0; lev < LEVEL_HERO; lev++)
     {
       skill_columns[lev] = 0;
       skill_list[lev][0] = '\0';
     }

   for (sn = 0; sn < MAX_SKILL; sn++)
     {
       if (skill_table[sn].name == NULL )
         break;


       if (get_skill_level(victim,sn) < LEVEL_HERO &&
	   skill_table[sn].spell_fun == spell_null &&
	   victim->pcdata->learned[sn] > 0) // NOT get_skill_learned
	 {
	   found = TRUE;
	   lev = get_skill_level(victim,sn);
	   if (victim->level < lev)
	     sprintf(buf,"%-18s n/a      ", skill_table[sn].name);
	   else
	     sprintf(buf,"%-18s %3d%%      ",skill_table[sn].name,
		     victim->pcdata->learned[sn]); // NOT get_skill_

	   if (skill_list[lev][0] == '\0')
	     sprintf(skill_list[lev],"\n\rLevel %2d: %s",lev,buf);
	   else /* append */
	     {
	       if ( ++skill_columns[lev] % 2 == 0)
		 strcat(skill_list[lev],"\n\r          ");
	       strcat(skill_list[lev],buf);
	     }
	 }
     }

   /* return results */

   if (!found)
     {
       send_to_char("They know no skills.\n\r",ch);
       return;
     }

   for (lev = 0; lev < LEVEL_HERO; lev++)
     if (skill_list[lev][0] != '\0')
       send_to_char(skill_list[lev],ch);
   send_to_char("\n\r",ch);


   return;
}

/* Corrected 28/8/96 by Oshea to give correct list of spell costs. */
void do_mspells( CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  char spell_list[LEVEL_HERO][MAX_STRING_LENGTH];
  char spell_columns[LEVEL_HERO];
  int sn,lev,mana;
  bool found = FALSE;

  one_argument( argument, arg );

  if ( arg[0] == '\0' )
    {
      send_to_char( "Stat spells whom?\n\r", ch );
      return;
    }

  if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
      send_to_char( "They aren't here.\n\r", ch );
      return;
    }

  if (IS_NPC(victim))
    return;

  sprintf(buf, "Spell list for %s:\n\r", victim->name);
  send_to_char( buf, ch );

  /* initilize data */
  for (lev = 0; lev < LEVEL_HERO; lev++)
    {
      spell_columns[lev] = 0;
      spell_list[lev][0] = '\0';
    }

  for (sn = 0; sn < MAX_SKILL; sn++)
    {
      if (skill_table[sn].name == NULL)
	break;

      if (get_skill_level(victim,sn) < LEVEL_HERO &&
	  skill_table[sn].spell_fun != spell_null &&
	  victim->pcdata->learned[sn] > 0) // NOT get_skill_learned
	{
	  found = TRUE;
	  lev = get_skill_level(victim, sn);
	  if (victim->level < lev)
            sprintf(buf,"%-18s  n/a      ", skill_table[sn].name);
	  else
	    {
	      mana = UMAX(skill_table[sn].min_mana,
			  100/(2 + victim->level - lev));
	      sprintf(buf,"%-18s  %3d mana  ",skill_table[sn].name,mana);
	    }

	  if (spell_list[lev][0] == '\0')
            sprintf(spell_list[lev],"\n\rLevel %2d: %s",lev,buf);
	  else /* append */
	    {
	      if ( ++spell_columns[lev] % 2 == 0)
		strcat(spell_list[lev],"\n\r          ");
	      strcat(spell_list[lev],buf);
	    }
	}
    }

  /* return results */

  if (!found)
    {
      send_to_char("They know no spells.\n\r",ch);
      return;
    }

  for (lev = 0; lev < LEVEL_HERO; lev++)
    if (spell_list[lev][0] != '\0')
      send_to_char(spell_list[lev],ch);
  send_to_char("\n\r",ch);

  return;
}

void do_maffects( CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  AFFECT_DATA *paf;
  int flag=0;

  one_argument( argument, arg );

  if ( arg[0] == '\0' )
    {
      send_to_char( "Stat affects whom?\n\r", ch );
      return;
    }

  if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
      send_to_char( "They aren't here.\n\r", ch );
      return;
    }

  sprintf(buf, "Affect list for %s:\n\r", victim->name);
  send_to_char( buf, ch );

  if (victim->affected != NULL)
    {
      for ( paf = victim->affected; paf != NULL; paf = paf->next )
	{
	  if ( (paf->type==gsn_sneak) || (paf->type==gsn_ride) ) {
            sprintf( buf, "Skill: '%s'", skill_table[paf->type].name );
            flag=1;
	  }
	  else {
            sprintf( buf, "Spell: '%s'", skill_table[paf->type].name );
            flag=0;
	  }
	  send_to_char( buf, ch );

	  if (flag==0)
	    {
	      sprintf( buf,
		       " modifies %s by %d for %d hours",
		       affect_loc_name( paf->location ),
		       paf->modifier,
		       paf->duration );
	      send_to_char( buf, ch );
	    }

	  send_to_char( ".\n\r", ch );
	}
    }
  else
    {
      send_to_char( "Nothing.\n\r", ch);
    }

  return;
}

/* Corrected 28/8/96 by Oshea to give correct list of spells/skills. */
void do_mpracs( CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  int sn;
  int col;

  one_argument( argument, arg );

  if ( arg[0] == '\0' )
    {
      send_to_char( "Stat pracs whom?\n\r", ch );
      return;
    }

  if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
      send_to_char( "They aren't here.\n\r", ch );
      return;
    }

   if ( IS_NPC(victim) )
     return;

   sprintf(buf, "Practice list for %s:\n\r", victim->name);
   send_to_char( buf, ch );

   col    = 0;
   for ( sn = 0; sn < MAX_SKILL; sn++ )
     {
       if ( skill_table[sn].name == NULL )
	 break;
       if ( victim->level < get_skill_level(victim,sn)
	    || victim->pcdata->learned[sn] < 1 /* skill is not known NOT get_skill_learned */)
	 continue;

       sprintf( buf, "%-18s %3d%%  ",
		skill_table[sn].name, victim->pcdata->learned[sn] );
       send_to_char( buf, ch );
       if ( ++col % 3 == 0 )
	 send_to_char( "\n\r", ch );
     }

   if ( col % 3 != 0 )
     send_to_char( "\n\r", ch );

   sprintf( buf, "They have %d practice sessions left.\n\r",
	    victim->practice );
   send_to_char( buf, ch );

   return;
}

/* Correct on 28/8/96 by Oshea to give correct cp's */
void do_minfo( CHAR_DATA *ch, char *argument)
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  int gn,col;


  one_argument( argument, arg );

  if ( arg[0] == '\0' )
    {
      send_to_char( "Stat info whom?\n\r", ch );
      return;
    }

  if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
      send_to_char( "They aren't here.\n\r", ch );
      return;
    }

  if (IS_NPC(victim))
    return;

  sprintf(buf, "Info list for %s:\n\r", victim->name);

  col = 0;

  /* show all groups */

  for (gn = 0; gn < MAX_GROUP; gn++)
    {
      if (group_table[gn].name == NULL)
	break;
      if (victim->pcdata->group_known[gn])
	{
	  sprintf(buf,"%-20s ",group_table[gn].name);
	  send_to_char(buf,ch);
	  if (++col % 3 == 0)
	    send_to_char("\n\r",ch);
	}
    }
  if ( col % 3 != 0 )
    send_to_char( "\n\r", ch );
  sprintf(buf,"Creation points: %d\n\r",victim->pcdata->points);

  send_to_char(buf,ch);
  return;
}

void do_mstat( CHAR_DATA *ch, char *argument )
{
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_INPUT_LENGTH];
  AFFECT_DATA *paf;
  CHAR_DATA *victim;

  /* this will help prevent major memory allocations */
   if( strlen( argument ) < 2 ) {
	   send_to_char( "Please be more specific.\n\r", ch );
	   return;
   }

  one_argument( argument, arg );

  if ( arg[0] == '\0' )
    {
      send_to_char( "Stat whom?\n\r", ch );
      return;
    }

  if ( ( victim = get_char_world( ch, argument ) ) == NULL )
    {
      send_to_char( "They aren't here.\n\r", ch );
      return;
    }

  sprintf( buf, "Name: %s     Clan: %s     Rank: %s.\n\r",
	   victim->name,
	   (!IS_NPC(victim) && victim->pcdata->pcclan) ?
	   victim->pcdata->pcclan->clan->name : "(none)",
	   (!IS_NPC(victim) && victim->pcdata->pcclan) ?
	   victim->pcdata->pcclan->clan->levelname[victim->pcdata->pcclan->clanlevel]
: "(none)" );
  send_to_char( buf, ch );

  sprintf( buf, "Vnum: %d  Format: %s  Race: %s  Sex: %s  Room: %d\n\r",
	   IS_NPC(victim) ? victim->pIndexData->vnum : 0,
	   IS_NPC(victim) ? victim->pIndexData->new_format ? "new" : "old" :
	   "pc",
	   race_table[victim->race].name,
	   victim->sex == SEX_MALE    ? "male"   :
	   victim->sex == SEX_FEMALE  ? "female" : "neutral",
	   victim->in_room == NULL    ?        0 : victim->in_room->vnum
	   );
  send_to_char( buf, ch );

  if (IS_NPC(victim))
    {
      sprintf(buf,"Count: %d  Killed: %d\n\r",
	      victim->pIndexData->count,victim->pIndexData->killed);
      send_to_char(buf,ch);
    }

  sprintf( buf,
	   "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)\n\r",
	   victim->perm_stat[STAT_STR],
	   get_curr_stat(victim,STAT_STR),
	   victim->perm_stat[STAT_INT],
	   get_curr_stat(victim,STAT_INT),
	   victim->perm_stat[STAT_WIS],
	   get_curr_stat(victim,STAT_WIS),
	   victim->perm_stat[STAT_DEX],
	   get_curr_stat(victim,STAT_DEX),
	   victim->perm_stat[STAT_CON],
	   get_curr_stat(victim,STAT_CON) );
  send_to_char( buf, ch );

  sprintf( buf, "Hp: %d/%d  Mana: %d/%d  Move: %d/%d  Practices: %d\n\r",
	   victim->hit,         victim->max_hit,
	   victim->mana,        victim->max_mana,
	   victim->move,        victim->max_move,
	   IS_NPC(ch) ? 0 : victim->practice );
  send_to_char( buf, ch );

  sprintf( buf,
	   "Lv: %d  Class: %s  Align: %d  Gold: %ld  Exp: %ld\n\r",
	   victim->level,
	   IS_NPC(victim) ? "mobile" : class_table[victim->class].name,
	   victim->alignment,
	   victim->gold,         victim->exp );
  send_to_char( buf, ch );

  sprintf(buf,"Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r",
	  GET_AC(victim,AC_PIERCE), GET_AC(victim,AC_BASH),
	  GET_AC(victim,AC_SLASH),  GET_AC(victim,AC_EXOTIC));
  send_to_char(buf,ch);

  sprintf( buf, "Hit: %d  Dam: %d  Saves: %d  Position: %d  Wimpy: %d\n\r",
	   GET_HITROLL(victim), GET_DAMROLL(victim), victim->saving_throw,
	   victim->position,    victim->wimpy );
  send_to_char( buf, ch );

  if (IS_NPC(victim) && victim->pIndexData->new_format)
    {
      sprintf(buf, "Damage: %dd%d  Message:  %s\n\r",
	      victim->damage[DICE_NUMBER],victim->damage[DICE_TYPE],
	      attack_table[victim->dam_type].noun);
      send_to_char(buf,ch);
    }
  sprintf( buf, "Fighting: %s\n\r",
	   victim->fighting ? victim->fighting->name : "(none)" );
  send_to_char( buf, ch );

  sprintf( buf, "Sentient 'victim': %s\n\r",
	   victim->sentient_victim != NULL ? victim->sentient_victim : "(none)" );
  send_to_char( buf, ch );

  if ( !IS_NPC(victim) )
    {
      sprintf( buf,
	       "Thirst: %d  Full: %d  Drunk: %d\n\r",
	       victim->pcdata->condition[COND_THIRST],
	       victim->pcdata->condition[COND_FULL],
	       victim->pcdata->condition[COND_DRUNK] );
      send_to_char( buf, ch );
    }

  sprintf( buf, "Carry number: %d  Carry weight: %d\n\r",
	   victim->carry_number, victim->carry_weight );
  send_to_char( buf, ch );


  if (!IS_NPC(victim))
    {
      sprintf( buf,
	       "Age: %d  Played: %d  Last Level: %d  Timer: %d\n\r",
	       get_age(victim),
	       (int) (victim->played + current_time - victim->logon) / 3600,
	       victim->pcdata->last_level,
	       victim->timer );
      send_to_char( buf, ch );
    }

  sprintf(buf, "Act: %s\n\r",(char *)act_bit_name(victim->act));
  send_to_char(buf,ch);

  if (!IS_NPC(victim)) {
    int n;
    sprintf(buf,"Extra: ");
    for (n=0; n< MAX_EXTRA_FLAGS; n++) {
	    if (is_set_extra(ch,n)) {
		    strcat(buf,flagname_extra[n]);
	    }
    }
    strcat(buf,"\n\r");
    send_to_char(buf,ch);
  }

  if (victim->comm)
    {
      sprintf(buf,"Comm: %s\n\r",(char *)comm_bit_name(victim->comm));
      send_to_char(buf,ch);
    }

  if (IS_NPC(victim) && victim->off_flags)
    {
      sprintf(buf, "Offense: %s\n\r",(char *)off_bit_name(victim->off_flags));
      send_to_char(buf,ch);
    }

  if (victim->imm_flags)
    {
      sprintf(buf, "Immune: %s\n\r",(char *)imm_bit_name(victim->imm_flags));
      send_to_char(buf,ch);
    }

  if (victim->res_flags)
    {
      sprintf(buf, "Resist: %s\n\r", (char *)imm_bit_name(victim->res_flags));
      send_to_char(buf,ch);
    }

  if (victim->vuln_flags)
    {
      sprintf(buf, "Vulnerable: %s\n\r", (char *)imm_bit_name(victim->vuln_flags));
      send_to_char(buf,ch);
    }

  sprintf(buf, "Form: %s\n\rParts: %s\n\r",
	  form_bit_name(victim->form), (char *)part_bit_name(victim->parts));
  send_to_char(buf,ch);

  if (victim->affected_by)
    {
      sprintf(buf, "Affected by %s\n\r",
	      (char *)affect_bit_name(victim->affected_by));
      send_to_char(buf,ch);
    }

  sprintf( buf, "Master: %s  Leader: %s  Pet: %s\n\r",
	   victim->master      ? victim->master->name   : "(none)",
	   victim->leader      ? victim->leader->name   : "(none)",
	   victim->pet 	    ? victim->pet->name	     : "(none)");
  send_to_char( buf, ch );

  sprintf( buf, "Riding: %s  Ridden by: %s\n\r",
	   victim->riding      ? victim->riding->name   : "(none)",
	   victim->ridden_by   ? victim->ridden_by->name : "(none)");
  send_to_char( buf, ch );

  sprintf( buf, "Short description: %s\n\rLong  description: %s",
	   victim->short_descr,
	   victim->long_descr[0] != '\0' ? victim->long_descr : "(none)\n\r" );
  send_to_char( buf, ch );

  if ( IS_NPC(victim) && victim->spec_fun != 0 )
    send_to_char( "Mobile has special procedure.\n\r", ch );

  if ( IS_NPC(victim) && victim->pIndexData->progtypes  ) {
	  sprintf( buf, "Mobile has MOBPROG: view with \"stat prog '%s'\"\n\r",
		   victim->name);
      send_to_char( buf, ch );
    }

  for ( paf = victim->affected; paf != NULL; paf = paf->next )
    {
      sprintf( buf,
	       "Spell: '%s' modifies %s by %d for %d hours with bits %s, level %d.\n\r",
	       (skill_table[(int) paf->type]).name,
	       affect_loc_name( paf->location ),
	       paf->modifier,
	       paf->duration,
	       affect_bit_name( paf->bitvector ),
	       paf->level);
      send_to_char( buf, ch );
    }
    	send_to_char("\n\r", ch );
  return;
}

/* ofind and mfind replaced with vnum, vnum skill also added */

void do_vnum(CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   char *string;

   string = one_argument(argument,arg);

   if (arg[0] == '\0')
   {
      send_to_char("Syntax:\n\r",ch);
      send_to_char("  vnum obj <name>\n\r",ch);
      send_to_char("  vnum mob <name>\n\r",ch);
      send_to_char("  vnum skill <skill or spell>\n\r",ch);
      return;
   }

   if (!str_cmp(arg,"obj"))
   {
      do_ofind(ch,string);
      return;
   }

   if (!str_cmp(arg,"mob") || !str_cmp(arg,"char"))
   {
      do_mfind(ch,string);
      return;
   }

   if (!str_cmp(arg,"skill") || !str_cmp(arg,"spell"))
   {
      do_slookup(ch,string);
      return;
   }
   /* do both */
   do_mfind(ch,argument);
   do_ofind(ch,argument);
}


void do_mfind(CHAR_DATA *ch, char *argument) {
	extern int top_mob_index;
	char arg[MAX_INPUT_LENGTH];
	MOB_INDEX_DATA *pMobIndex;
	int vnum;
	int nMatch;
	bool fAll;
	bool found;
	BUFFER *buffer;

	one_argument( argument, arg );
	if (arg[0] == '\0') {
		send_to_char( "Find whom?\n\r", ch );
		return;
	}

	fAll = FALSE; /* !str_cmp( arg, "all" ); */
	found = FALSE;
	nMatch = 0;

/*
 * Yeah, so iterating over all vnum's takes 10,000 loops.
 * Get_mob_index is fast, and I don't feel like threading another link.
 * Do you?
 * -- Furey
 */
	buffer = buffer_create();
	for ( vnum = 0; nMatch < top_mob_index; vnum++ ) {
		if ( ( pMobIndex = get_mob_index( vnum ) ) != NULL ) {
			nMatch++;
			if ( fAll || is_name( argument, pMobIndex->player_name ) ) {
				found = TRUE;
				buffer_addline_fmt(buffer, "[%5d] %s\n\r", pMobIndex->vnum,
						pMobIndex->short_descr);
			}
		}
	}

	buffer_send( buffer, ch ); /* frees buffer */
	if (!found)
		send_to_char( "No mobiles by that name.\n\r", ch );
}



void do_ofind( CHAR_DATA *ch, char *argument )
{
	extern int top_obj_index;
	char arg[MAX_INPUT_LENGTH];
	OBJ_INDEX_DATA *pObjIndex;
	int vnum;
	int nMatch;
	bool fAll;
	bool found;
	BUFFER *buffer;

	one_argument( argument, arg );
	if (arg[0] == '\0') {
		send_to_char( "Find what?\n\r", ch );
		return;
	}

	fAll = FALSE; /* !str_cmp( arg, "all" ); */
	found = FALSE;
	nMatch = 0;

/*
 * Yeah, so iterating over all vnum's takes 10,000 loops.
 * Get_obj_index is fast, and I don't feel like threading another link.
 * Do you?
 * -- Furey
 */
	buffer = buffer_create();
	for ( vnum = 0; nMatch < top_obj_index; vnum++ ) {
		if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL ) {
			nMatch++;
			if ( fAll || is_name( argument, pObjIndex->name ) ) {
				found = TRUE;
				buffer_addline_fmt(buffer, "[%5d] %s\n\r", pObjIndex->vnum,
						pObjIndex->short_descr);
			}
		}
	}

	buffer_send( buffer, ch ); /* NB this frees the buffer */
	if ( !found )
		send_to_char( "No objects by that name.\n\r", ch );
}



void do_mwhere(CHAR_DATA *ch, char *argument) {
	CHAR_DATA *victim;
	bool found;
	bool findPC = FALSE;
	int number = 0;
	BUFFER *buffer;

	if ( argument[0] == '\0' ) {
	  findPC = TRUE;
	} else if (strlen( argument ) < 2 ) {
		send_to_char( "Please be more specific.\n\r", ch );
		return;
	}
	
	found = FALSE;
	number = 0;
	buffer = buffer_create();

	for ( victim = char_list; victim != NULL; victim = victim->next) {
		if ( ( IS_NPC(victim)
				&& victim->in_room != NULL
				&& is_name( argument, victim->name )
				&& findPC == FALSE ) 
				|| (!IS_NPC(victim) && (findPC == TRUE) && can_see(ch, victim)) ) {
			found = TRUE;
			number++;
			buffer_addline_fmt(buffer, "%3d [%5d] %-28.28s [%5d] %20.20s\n\r", number,
					(findPC == TRUE) ? 0 : victim->pIndexData->vnum,
					(findPC == TRUE) ? victim->name : victim->short_descr,
					victim->in_room->vnum, victim->in_room->name);
		}
	}
	buffer_send(buffer, ch);

	if ( !found )
		act( "You didn't find any $T.", ch, NULL, argument, TO_CHAR );
}



void do_reboo( CHAR_DATA *ch, char *argument )
{
  (void)argument;
   send_to_char( "If you want to REBOOT, spell it out.\n\r", ch );
   return;
}



void do_reboot( CHAR_DATA *ch, char *argument )
{
  (void)argument;
   char buf[MAX_STRING_LENGTH];
   extern bool merc_down;

   if (!IS_SET(ch->act,PLR_WIZINVIS))
   {
      sprintf( buf, "Reboot by %s.", ch->name );
      do_echo( ch, buf );
   }
   do_force ( ch, "all save");
   do_save (ch, "");
   merc_down = TRUE;
   // Unlike do_shutdown(), do_reboot() does not explicitly call close_socket()
   // for every connected player. That's because close_socket() actually
   // sends a PACKET_DISCONNECT to doorman, causing the client to get booted and
   // that's not the desired behaviour. Instead, setting the merc_down flag will
   // cause the main process to terminate while doorman holds onto the client's
   // connection and then reconnect them to the mud once it's back up.
   return;
}



void do_shutdow( CHAR_DATA *ch, char *argument )
{
  (void)argument;
   send_to_char( "If you want to SHUTDOWN, spell it out.\n\r", ch );
   return;
}



void do_shutdown( CHAR_DATA *ch, char *argument )
{
  (void)argument;
   char buf[MAX_STRING_LENGTH];
   extern bool merc_down;
   DESCRIPTOR_DATA *d,*d_next;

   sprintf( buf, "Shutdown by %s.", ch->name );
   append_file( ch, SHUTDOWN_FILE, buf );

   strcat( buf, "\n\r" );
   do_echo( ch, buf );
   do_force ( ch, "all save");
   do_save (ch, "");
   merc_down = TRUE;
   for ( d = descriptor_list; d != NULL; d = d_next)
   {
      d_next = d->next;
      close_socket(d);
   }
   return;
}



void do_snoop( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   DESCRIPTOR_DATA *d;
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if ( arg[0] == '\0' )
   {
      send_to_char( "Snoop whom?\n\r", ch );
      return;
   }

   if ( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( victim->desc == NULL )
   {
      send_to_char( "No descriptor to snoop.\n\r", ch );
      return;
   }

   if ( victim == ch )
   {
      send_to_char( "Cancelling all snoops.\n\r", ch );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
         if ( d->snoop_by == ch->desc )
            d->snoop_by = NULL;
      }
      return;
   }

   if ( victim->desc->snoop_by != NULL )
   {
      send_to_char( "Busy already.\n\r", ch );
      return;
   }

   if ( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }

   if ( ch->desc != NULL )
   {
      for ( d = ch->desc->snoop_by; d != NULL; d = d->snoop_by )
      {
         if ( d->character == victim || d->original == victim )
         {
            send_to_char( "No snoop loops.\n\r", ch );
            return;
         }
      }
   }

   victim->desc->snoop_by = ch->desc;
   send_to_char( "Ok.\n\r", ch );
   return;
}



void do_switch( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if ( arg[0] == '\0' )
   {
      send_to_char( "Switch into whom?\n\r", ch );
      return;
   }

   if ( ch->desc == NULL )
      return;

   if ( ch->desc->original != NULL )
   {
      send_to_char( "You are already switched.\n\r", ch );
      return;
   }

   if ( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( victim == ch )
   {
      send_to_char( "Ok.\n\r", ch );
      return;
   }

   if (!IS_NPC(victim))
   {
      send_to_char("You can only switch into mobiles.\n\r",ch);
      return;
   }

   if ( victim->desc != NULL )
   {
      send_to_char( "Character in use.\n\r", ch );
      return;
   }

   ch->desc->character = victim;
   ch->desc->original  = ch;
   victim->desc        = ch->desc;
   ch->desc            = NULL;
   /* change communications to match */
   victim->comm = ch->comm;
   victim->lines = ch->lines;
   send_to_char( "Ok.\n\r", victim );
   return;
}



void do_return( CHAR_DATA *ch, char *argument )
{
  (void)argument;
   if ( ch->desc == NULL )
      return;

   if ( ch->desc->original == NULL )
   {
      send_to_char( "You aren't switched.\n\r", ch );
      return;
   }

   send_to_char( "You return to your original body.\n\r", ch );
   ch->desc->character       = ch->desc->original;
   ch->desc->original        = NULL;
   ch->desc->character->desc = ch->desc;
   ch->desc                  = NULL;
   return;
}

/* trust levels for load and clone */
/* cut out by Faramir but func retained in case of any
   calls I don't know about. */
bool obj_check (CHAR_DATA *ch, OBJ_DATA *obj) {

	if (obj->level > get_trust(ch))
		return FALSE;
	return TRUE;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *clone)
{
   OBJ_DATA *c_obj, *t_obj;


   for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content)
   {
      if (obj_check(ch,c_obj))
      {
         t_obj = create_object(c_obj->pIndexData,0);
         clone_object(c_obj,t_obj);
         obj_to_obj(t_obj,clone);
         recursive_clone(ch,c_obj,t_obj);
      }
   }
}

/* command that is similar to load */
void do_clone(CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   char *rest;
   CHAR_DATA *mob;
   OBJ_DATA  *obj;

   rest = one_argument(argument,arg);

   if (arg[0] == '\0')
   {
      send_to_char("Clone what?\n\r",ch);
      return;
   }

   if (!str_prefix(arg,"object"))
   {
      mob = NULL;
      obj = get_obj_here(ch,rest);
      if (obj == NULL)
      {
         send_to_char("You don't see that here.\n\r",ch);
         return;
      }
   }
   else if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character"))
   {
      obj = NULL;
      mob = get_char_room(ch,rest);
      if (mob == NULL)
      {
         send_to_char("You don't see that here.\n\r",ch);
         return;
      }
   }
   else /* find both */
   {
      mob = get_char_room(ch,argument);
      obj = get_obj_here(ch,argument);
      if (mob == NULL && obj == NULL)
      {
         send_to_char("You don't see that here.\n\r",ch);
         return;
      }
   }

   /* clone an object */
   if (obj != NULL)
   {
      OBJ_DATA *clone;

      if (!obj_check(ch,obj))
      {
         send_to_char(
         "Your powers are not great enough for such a task.\n\r",ch);
         return;
      }

      clone = create_object(obj->pIndexData,0);
      clone_object(obj,clone);
      if (obj->carried_by != NULL)
         obj_to_char(clone,ch);
      else
      obj_to_room(clone,ch->in_room);
      recursive_clone(ch,obj,clone);

      act("$n has created $p.",ch,clone,NULL,TO_ROOM);
      act("You clone $p.",ch,clone,NULL,TO_CHAR);
      return;
   }
   else if (mob != NULL)
   {
      CHAR_DATA *clone;
      OBJ_DATA *new_obj;

      if (!IS_NPC(mob))
      {
         send_to_char("You can only clone mobiles.\n\r",ch);
         return;
      }

      if ((mob->level > 20 && !IS_TRUSTED(ch,GOD))
      ||  (mob->level > 10 && !IS_TRUSTED(ch,IMMORTAL))
      ||  (mob->level >  5 && !IS_TRUSTED(ch,DEMI))
      ||  (mob->level >  0 && !IS_TRUSTED(ch,ANGEL))
      ||  !IS_TRUSTED(ch,AVATAR))
      {
         send_to_char(
         "Your powers are not great enough for such a task.\n\r",ch);
         return;
      }

      clone = create_mobile(mob->pIndexData);
      clone_mobile(mob,clone);

      for (obj = mob->carrying; obj != NULL; obj = obj->next_content)
      {
         if (obj_check(ch,obj))
         {
            new_obj = create_object(obj->pIndexData,0);
            clone_object(obj,new_obj);
            recursive_clone(ch,obj,new_obj);
            obj_to_char(new_obj,clone);
            new_obj->wear_loc = obj->wear_loc;
         }
      }
      char_to_room(clone,ch->in_room);
      act("$n has created $N.",ch,NULL,clone,TO_ROOM);
      act("You clone $N.",ch,NULL,clone,TO_CHAR);
      return;
   }
}

/* RT to replace the two load commands */

void do_load(CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];

   argument = one_argument(argument,arg);

   if (arg[0] == '\0')
   {
      send_to_char("Syntax:\n\r",ch);
      send_to_char("  load mob <vnum>\n\r",ch);
      send_to_char("  load obj <vnum> <level>\n\r",ch);
      return;
   }

   if (!str_cmp(arg,"mob") || !str_cmp(arg,"char"))
   {
      do_mload(ch,argument);
      return;
   }

   if (!str_cmp(arg,"obj"))
   {
      do_oload(ch,argument);
      return;
   }
   /* echo syntax */
   do_load(ch,"");
}


void do_mload( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   MOB_INDEX_DATA *pMobIndex;
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if ( arg[0] == '\0' || !is_number(arg) )
   {
      send_to_char( "Syntax: load mob <vnum>.\n\r", ch );
      return;
   }

   if ( ( pMobIndex = get_mob_index( atoi( arg ) ) ) == NULL )
   {
      send_to_char( "No mob has that vnum.\n\r", ch );
      return;
   }

   victim = create_mobile( pMobIndex );
   char_to_room( victim, ch->in_room );
   act( "$n has created $N!", ch, NULL, victim, TO_ROOM );
   send_to_char( "Ok.\n\r", ch );
   return;
}



void do_oload( CHAR_DATA *ch, char *argument )
{
   char arg1[MAX_INPUT_LENGTH] ,arg2[MAX_INPUT_LENGTH];
   OBJ_INDEX_DATA *pObjIndex;
   OBJ_DATA *obj;
   int level;

   argument = one_argument( argument, arg1 );
   one_argument( argument, arg2 );

   if ( arg1[0] == '\0' || !is_number(arg1))
   {
      send_to_char( "Syntax: load obj <vnum> <level>.\n\r", ch );
      return;
   }

   level = get_trust(ch); /* default */

   if ( arg2[0] != '\0')  /* load with a level */
   {
      if (!is_number(arg2))
      {
         send_to_char( "Syntax: oload <vnum> <level>.\n\r", ch );
         return;
      }
      level = atoi(arg2);
      if (level < 0 || level > get_trust(ch))
      {
         send_to_char( "Level must be be between 0 and your level.\n\r",ch);
         return;
      }
   }

   if ( ( pObjIndex = get_obj_index( atoi( arg1 ) ) ) == NULL )
   {
      send_to_char( "No object has that vnum.\n\r", ch );
      return;
   }

   obj = create_object( pObjIndex, level );
   if ( CAN_WEAR(obj, ITEM_TAKE) )
      obj_to_char( obj, ch );
   else
   obj_to_room( obj, ch->in_room );
   act( "$n has created $p!", ch, obj, NULL, TO_ROOM );
   send_to_char( "Ok.\n\r", ch );
   return;
}



void do_purge( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   char buf[100];
   CHAR_DATA *victim;
   OBJ_DATA *obj;
   DESCRIPTOR_DATA *d;

   one_argument( argument, arg );

   if ( arg[0] == '\0' )
   {
      /* 'purge' */
      CHAR_DATA *vnext;
      OBJ_DATA  *obj_next;

      for ( victim = ch->in_room->people; victim != NULL; victim = vnext )
      {
         vnext = victim->next_in_room;
         if ( IS_NPC(victim) && !IS_SET(victim->act,ACT_NOPURGE)
         &&   victim != ch /* safety precaution */ )
            extract_char( victim, TRUE );
      }

      for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
      {
         obj_next = obj->next_content;
         if (!IS_OBJ_STAT(obj,ITEM_NOPURGE))
            extract_obj( obj );
      }

      act( "$n purges the room!", ch, NULL, NULL, TO_ROOM);
      send_to_char( "Ok.\n\r", ch );
      return;
   }

   if ( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( !IS_NPC(victim) )
   {

      if (ch == victim)
      {
         send_to_char("Ho ho ho.\n\r",ch);
         return;
      }

      if (get_trust(ch) <= get_trust(victim))
      {
         send_to_char("Maybe that wasn't a good idea...\n\r",ch);
         sprintf(buf,"%s tried to purge you!\n\r",ch->name);
         send_to_char(buf,victim);
         return;
      }

      act("$n disintegrates $N.",ch,0,victim,TO_NOTVICT);

      if (victim->level > 1)
         save_char_obj( victim );
      d = victim->desc;
      extract_char( victim, TRUE );
      if ( d != NULL )
         close_socket( d );

      return;
   }

   act( "$n purges $N.", ch, NULL, victim, TO_NOTVICT );
   extract_char( victim, TRUE );
   return;
}



void do_advance( CHAR_DATA *ch, char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   int level;
   int iLevel;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if ( arg1[0] == '\0' || arg2[0] == '\0' || !is_number( arg2 ) )
   {
      send_to_char( "Syntax: advance <char> <level>.\n\r", ch );
      return;
   }

   if ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
   {
      send_to_char( "That player is not here.\n\r", ch);
      return;
   }

   if ( IS_NPC(victim) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   if ( ( level = atoi( arg2 ) ) < 1 || level > MAX_LEVEL )
   {
	   char buf[32];
	   sprintf( buf, "Level must be 1 to %d.\n\r", MAX_LEVEL );
	   
	   send_to_char( buf, ch );
	   return;
   }

   if ( level > get_trust( ch ) )
   {
      send_to_char( "Limited to your trust level.\n\r", ch );
      return;
   }

   /*
        * Lower level:
        *   Reset to level 1.
        *   Then raise again.
        *   Currently, an imp can lower another imp.
        *   -- Swiftest
        */
   if ( level <= victim->level && ( ch->level > victim->level ) )
   {
      int temp_prac;

      send_to_char( "Lowering a player's level!\n\r", ch );
      send_to_char( "**** OOOOHHHHHHHHHH  NNNNOOOO ****\n\r", victim );
      temp_prac = victim->practice;
      victim->level    = 1;
      victim->exp      = exp_per_level(victim,victim->pcdata->points);
      victim->max_hit  = 10;
      victim->max_mana = 100;
      victim->max_move = 100;
      victim->practice = 0;
      victim->hit      = victim->max_hit;
      victim->mana     = victim->max_mana;
      victim->move     = victim->max_move;
      advance_level( victim );
      victim->practice = temp_prac;
   }
   else
   {
      send_to_char( "Raising a player's level!\n\r", ch );
      send_to_char( "|R**** OOOOHHHHHHHHHH  YYYYEEEESSS ****|w\n\r", victim );
   }

   if (ch->level > victim->level)
   {
      for ( iLevel = victim->level ; iLevel < level; iLevel++ )
      {
         send_to_char( "You raise a level!!  ", victim );
         victim->level += 1;
         advance_level( victim );
      }
      victim->exp   = (exp_per_level(victim,victim->pcdata->points)
         * UMAX( 1, victim->level) );
      victim->trust = 0;
#if defined(msdos)
#else
      save_char_obj(victim);
#endif
   }
   return;
}



void do_trust( CHAR_DATA *ch, char *argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *victim;
   int level;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );

   if ( (arg1[0] == '\0' && arg2[0] == '\0')
	|| (arg2[0] != '\0' && !is_number( arg2 )) )
   {
      send_to_char( "Syntax: trust <char> <level>.\n\r", ch );
      return;
   }

   if ( ( victim = get_char_room( ch, arg1 ) ) == NULL )
   {
      send_to_char( "That player is not here.\n\r", ch);
      return;
   }

   if (arg2[0] == '\0')
   {
      sprintf(buf, "%s has a trust of %d.\n\r",
	      victim->name,
	      victim->trust);
      send_to_char(buf, ch);
      return;
   }

   if ( IS_NPC(victim) )
   {
       send_to_char( "Not on NPCs.\n\r", ch);
       return;
   }


   if ( ( level = atoi( arg2 ) ) < 0 || level > 100 )
   {
      send_to_char( "Level must be 0 (reset) or 1 to 100.\n\r", ch );
      return;
   }

   if ( level > get_trust( ch ) )
   {
      send_to_char( "Limited to your trust.\n\r", ch );
      return;
   }

   victim->trust = level;
   return;
}



void do_restore( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   CHAR_DATA *vch;
   DESCRIPTOR_DATA *d;

   one_argument( argument, arg );
   if (arg[0] == '\0' || !str_cmp(arg,"room"))
   {
      /* cure room */

      for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
      {
         affect_strip(vch,gsn_plague);
         affect_strip(vch,gsn_poison);
         affect_strip(vch,gsn_blindness);
         affect_strip(vch,gsn_sleep);
         affect_strip(vch,gsn_curse);

         vch->hit 	= vch->max_hit;
         vch->mana	= vch->max_mana;
         vch->move	= vch->max_move;
         update_pos( vch);
         act("$n has restored you.",ch,NULL,vch,TO_VICT);
      }

      send_to_char("Room restored.\n\r",ch);
      return;

   }

   if ( get_trust(ch) >=  MAX_LEVEL && !str_cmp(arg,"all"))
   {
      /* cure all */

      for (d = descriptor_list; d != NULL; d = d->next)
      {
         victim = d->character;

         if (victim == NULL || IS_NPC(victim))
            continue;

         affect_strip(victim,gsn_plague);
         affect_strip(victim,gsn_poison);
         affect_strip(victim,gsn_blindness);
         affect_strip(victim,gsn_sleep);
         affect_strip(victim,gsn_curse);

         victim->hit 	= victim->max_hit;
         victim->mana	= victim->max_mana;
         victim->move	= victim->max_move;
         update_pos( victim);
         if (victim->in_room != NULL)
            act("$n has restored you.",ch,NULL,victim,TO_VICT);
      }
      send_to_char("All active players restored.\n\r",ch);
      return;
   }

   if ( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   affect_strip(victim,gsn_plague);
   affect_strip(victim,gsn_poison);
   affect_strip(victim,gsn_blindness);
   affect_strip(victim,gsn_sleep);
   affect_strip(victim,gsn_curse);
   victim->hit  = victim->max_hit;
   victim->mana = victim->max_mana;
   victim->move = victim->max_move;
   update_pos( victim );
   act( "$n has restored you.", ch, NULL, victim, TO_VICT );
   send_to_char( "Ok.\n\r", ch );
   return;
}


void do_freeze( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if ( arg[0] == '\0' )
   {
      send_to_char( "Freeze whom?\n\r", ch );
      return;
   }

   if ( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( IS_NPC(victim) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   if ( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }

   if ( IS_SET(victim->act, PLR_FREEZE) )
   {
      REMOVE_BIT(victim->act, PLR_FREEZE);
      send_to_char( "You can play again.\n\r", victim );
      send_to_char( "FREEZE removed.\n\r", ch );
   }
   else
   {
      SET_BIT(victim->act, PLR_FREEZE);
      send_to_char( "You can't do ANYthing!\n\r", victim );
      send_to_char( "FREEZE set.\n\r", ch );
   }

   save_char_obj( victim );

   return;
}



void do_log( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if ( arg[0] == '\0' )
   {
      send_to_char( "Log whom?\n\r", ch );
      return;
   }

   if ( !str_cmp( arg, "all" ) )
   {
      if ( fLogAll )
      {
         fLogAll = FALSE;
         send_to_char( "Log ALL off.\n\r", ch );
      }
      else
      {
         fLogAll = TRUE;
         send_to_char( "Log ALL on.\n\r", ch );
      }
      return;
   }

   if ( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( IS_NPC(victim) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   /*
        * No level check, gods can log anyone.
        */
   if ( IS_SET(victim->act, PLR_LOG) )
   {
      REMOVE_BIT(victim->act, PLR_LOG);
      send_to_char( "LOG removed.\n\r", ch );
   }
   else
   {
      SET_BIT(victim->act, PLR_LOG);
      send_to_char( "LOG set.\n\r", ch );
   }

   return;
}



void do_noemote( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if ( arg[0] == '\0' )
   {
      send_to_char( "Noemote whom?\n\r", ch );
      return;
   }

   if ( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }


   if ( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }

   if ( IS_SET(victim->comm, COMM_NOEMOTE) )
   {
      REMOVE_BIT(victim->comm, COMM_NOEMOTE);
      send_to_char( "You can emote again.\n\r", victim );
      send_to_char( "NOEMOTE removed.\n\r", ch );
   }
   else
   {
      SET_BIT(victim->comm, COMM_NOEMOTE);
      send_to_char( "You can't emote!\n\r", victim );
      send_to_char( "NOEMOTE set.\n\r", ch );
   }

   return;
}



void do_noshout( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if ( arg[0] == '\0' )
   {
      send_to_char( "Noshout whom?\n\r",ch);
      return;
   }

   if ( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( IS_NPC(victim) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   if ( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }

   if ( IS_SET(victim->comm, COMM_NOSHOUT) )
   {
      REMOVE_BIT(victim->comm, COMM_NOSHOUT);
      send_to_char( "You can shout again.\n\r", victim );
      send_to_char( "NOSHOUT removed.\n\r", ch );
   }
   else
   {
      SET_BIT(victim->comm, COMM_NOSHOUT);
      send_to_char( "You can't shout!\n\r", victim );
      send_to_char( "NOSHOUT set.\n\r", ch );
   }

   return;
}



void do_notell( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];
   CHAR_DATA *victim;

   one_argument( argument, arg );

   if ( arg[0] == '\0' )
   {
      send_to_char( "Notell whom?", ch );
      return;
   }

   if ( ( victim = get_char_world( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( get_trust( victim ) >= get_trust( ch ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }

   if ( IS_SET(victim->comm, COMM_NOTELL) )
   {
      REMOVE_BIT(victim->comm, COMM_NOTELL);
      send_to_char( "You can tell again.\n\r", victim );
      send_to_char( "NOTELL removed.\n\r", ch );
   }
   else
   {
      SET_BIT(victim->comm, COMM_NOTELL);
      send_to_char( "You can't tell!\n\r", victim );
      send_to_char( "NOTELL set.\n\r", ch );
   }

   return;
}



void do_peace( CHAR_DATA *ch, char *argument )
{
  (void)argument;
   CHAR_DATA *rch;

   for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
   {
      if ( rch->fighting != NULL )
         stop_fighting( rch, TRUE );
      if (IS_NPC(rch) && IS_SET(rch->act,ACT_AGGRESSIVE))
         REMOVE_BIT(rch->act,ACT_AGGRESSIVE);
      if (IS_NPC(rch) && (rch->sentient_victim)) {
         free_string(rch->sentient_victim);
         rch->sentient_victim = NULL;
      }
   }

   send_to_char( "Ok.\n\r", ch );
   return;
}

void do_awaken( CHAR_DATA *ch, char *argument )
{
   CHAR_DATA *victim;
   char arg[MAX_INPUT_LENGTH];

   one_argument( argument, arg);

   if ( arg[0] == '\0' )
   {
      send_to_char( "Awaken whom?\n\r", ch );
      return;
   }

   if ( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }
   if ( IS_AWAKE(victim) )
   {
      send_to_char( "Duh!  They're not even asleep!\n\r", ch);
      return;
   }

   if ( ch == victim )
   {
      send_to_char( "Duh!  If you wanna wake up, get COFFEE!\n\r", ch );
      return;
   }

   REMOVE_BIT( victim->affected_by, AFF_SLEEP);
   victim->position = POS_STANDING;

   act_new( "$n gives $t a kick, and wakes them up.",
   ch, victim->short_descr, NULL, TO_ROOM, POS_RESTING );

   return;
}

void do_owhere(CHAR_DATA *ch, char *argument) {
	char target_name[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	OBJ_DATA *in_obj;
	BUFFER *buffer;
	bool found;
	int number = 0;

	found = FALSE;
	number = 0;

	if (argument[0] == '\0') {
		send_to_char("Owhere which object?\n\r", ch);
		return;
	}
	if( strlen( argument ) < 2 ) {
		send_to_char( "Please be more specific.\n\r", ch );
		return;
	}
	one_argument (argument, target_name);
	buffer = buffer_create();

	for ( obj = object_list; obj != NULL; obj = obj->next ) {
		if (!is_name( target_name, obj->name ))
			continue;

		found = TRUE;
		number++;

		for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj);

		if (in_obj->carried_by != NULL) {
			buffer_addline_fmt(buffer, "%3d %-25.25s carried by %-20.20s in room %d\n\r",
					number, obj->short_descr, PERS(in_obj->carried_by, ch),
					in_obj->carried_by->in_room->vnum);
		} else if (in_obj->in_room != NULL) {
			buffer_addline_fmt(buffer, "%3d %-25.25s in %-30.30s [%d]\n\r", number,
					obj->short_descr, in_obj->in_room->name, in_obj->in_room->vnum);
		}
	}

	buffer_send(buffer,ch);
	if ( !found )
		send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
}

/* Death's command */
void do_coma( CHAR_DATA *ch, char *argument )
{
   CHAR_DATA *victim;
   char arg[MAX_INPUT_LENGTH];
   AFFECT_DATA af;

   one_argument( argument, arg);

   if ( arg[0] == '\0' )
   {
      send_to_char( "Comatoze whom?\n\r", ch );
      return;
   }

   if ( ( victim = get_char_room( ch, arg ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( IS_AFFECTED(victim, AFF_SLEEP) )
      return;

   if ( ch == victim )
   {
      send_to_char( "Duh!  Don't you dare fall asleep on the job!\n\r", ch );
      return;
   }
   if ( (get_trust(ch) <= get_trust(victim))
   ||   !( (IS_IMMORTAL(ch)) && IS_NPC(victim) ) )
   {
      send_to_char( "You failed.\n\r", ch );
      return;
   }

   af.type      = 38;  /* SLEEP */
   af.level     = ch->trust;
   af.duration  = 4 + ch->trust;
   af.location  = APPLY_NONE;
   af.modifier  = 0;
   af.bitvector = AFF_SLEEP;
   affect_join( victim, &af );

   if ( IS_AWAKE(victim) )
   {
      send_to_char( "You feel very sleepy ..... zzzzzz.\n\r", victim );
      act( "$n goes to sleep.", victim, NULL, NULL, TO_ROOM );
      victim->position = POS_SLEEPING;
   }

   return;
}


void do_slookup( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   int sn;

   one_argument( argument, arg );
   if ( arg[0] == '\0' )
   {
      send_to_char( "Lookup which skill or spell?\n\r", ch );
      return;
   }

   if ( !str_cmp( arg, "all" ) )
   {
      for ( sn = 0; sn < MAX_SKILL; sn++ )
      {
         if ( skill_table[sn].name == NULL )
            break;
         sprintf( buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r",
         sn, skill_table[sn].slot, skill_table[sn].name );
         send_to_char( buf, ch );
      }
   }
   else
   {
      if ( ( sn = skill_lookup( arg ) ) < 0 )
      {
         send_to_char( "No such skill or spell.\n\r", ch );
         return;
      }

      sprintf( buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r",
      sn, skill_table[sn].slot, skill_table[sn].name );
      send_to_char( buf, ch );
   }

   return;
}

/* RT set replaces sset, mset, oset, and rset */

void do_set( CHAR_DATA *ch, char *argument )
{
   char arg[MAX_INPUT_LENGTH];

   argument = one_argument(argument,arg);

   if (arg[0] == '\0')
   {
      send_to_char("Syntax:\n\r",ch);
      send_to_char("  set mob   <name> <field> <value>\n\r",ch);
      send_to_char("  set obj   <name> <field> <value>\n\r",ch);
      send_to_char("  set room  <room> <field> <value>\n\r",ch);
      send_to_char("  set skill <name> <spell or skill> <value>\n\r",ch);
      return;
   }

   if (!str_prefix(arg,"mobile") || !str_prefix(arg,"character"))
   {
      do_mset(ch,argument);
      return;
   }

   if (!str_prefix(arg,"skill") || !str_prefix(arg,"spell"))
   {
      do_sset(ch,argument);
      return;
   }

   if (!str_prefix(arg,"object"))
   {
      do_oset(ch,argument);
      return;
   }

   if (!str_prefix(arg,"room"))
   {
      do_rset(ch,argument);
      return;
   }
   /* echo syntax */
   do_set(ch,"");
}


void do_sset( CHAR_DATA *ch, char *argument )
{
   char arg1 [MAX_INPUT_LENGTH];
   char arg2 [MAX_INPUT_LENGTH];
   char arg3 [MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   int value;
   int sn;
   bool fAll;

   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   argument = one_argument( argument, arg3 );

   if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
   {
      send_to_char( "Syntax:\n\r",ch);
      send_to_char( "  set skill <name> <spell or skill> <value>\n\r", ch);
      send_to_char( "  set skill <name> all <value>\n\r",ch);
      send_to_char("   (use the name of the skill, not the number)\n\r",ch);
      return;
   }

   if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   if ( IS_NPC(victim) )
   {
      send_to_char( "Not on NPC's.\n\r", ch );
      return;
   }

   fAll = !str_cmp( arg2, "all" );
   sn   = 0;
   if ( !fAll && ( sn = skill_lookup( arg2 ) ) < 0 )
   {
      send_to_char( "No such skill or spell.\n\r", ch );
      return;
   }

   /*
        * Snarf the value.
        */
   if ( !is_number( arg3 ) )
   {
      send_to_char( "Value must be numeric.\n\r", ch );
      return;
   }

   value = atoi( arg3 );
   if ( value < 0 || value > 100 )
   {
      send_to_char( "Value range is 0 to 100.\n\r", ch );
      return;
   }

   if ( fAll )
   {
      for ( sn = 0; sn < MAX_SKILL; sn++ )
      {
         if ( skill_table[sn].name != NULL )
            victim->pcdata->learned[sn]	= value;
      }
   }
   else
   {
      victim->pcdata->learned[sn] = value;
   }

   return;
}



void do_mset( CHAR_DATA *ch, char *argument )
{
   char arg1 [MAX_INPUT_LENGTH];
   char arg2 [MAX_INPUT_LENGTH];
   char arg3 [MAX_INPUT_LENGTH];
   char buf[100];
   CHAR_DATA *victim;
   int value;

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   strcpy( arg3, argument );

   if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
   {
      send_to_char("Syntax:\n\r",ch);
      send_to_char("  set char <name> <field> <value>\n\r",ch);
      send_to_char( "  Field being one of:\n\r",			ch );
      send_to_char( "    str int wis dex con sex class level\n\r",	ch );
      send_to_char( "    race gold hp mana move practice align\n\r",	ch );
      send_to_char( "    dam hit train thirst drunk full hours\n\r",       	ch
);
      return;
   }

   if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
   {
      send_to_char( "They aren't here.\n\r", ch );
      return;
   }

   /*
        * Snarf the value (which need not be numeric).
        */
   value = is_number( arg3 ) ? atoi( arg3 ) : -1;

   /*
        * Set something.
        */
   if ( !str_cmp( arg2, "str" ) )
   {
      if ( value < 3 || value > get_max_train(victim,STAT_STR) )
      {
         sprintf(buf,
         "Strength range is 3 to %d\n\r.",
         get_max_train(victim,STAT_STR));
         send_to_char(buf,ch);
         return;
      }

      victim->perm_stat[STAT_STR] = value;
      return;
   }

   if ( !str_cmp( arg2, "int" ) )
   {
      if ( value < 3 || value > get_max_train(victim,STAT_INT) )
      {
         sprintf(buf,
         "Intelligence range is 3 to %d.\n\r",
         get_max_train(victim,STAT_INT));
         send_to_char(buf,ch);
         return;
      }

      victim->perm_stat[STAT_INT] = value;
      return;
   }

   if ( !str_cmp( arg2, "wis" ) )
   {
      if ( value < 3 || value > get_max_train(victim,STAT_WIS) )
      {
         sprintf(buf,
         "Wisdom range is 3 to %d.\n\r",get_max_train(victim,STAT_WIS));
         send_to_char( buf, ch );
         return;
      }

      victim->perm_stat[STAT_WIS] = value;
      return;
   }

   if ( !str_cmp( arg2, "dex" ) )
   {
      if ( value < 3 || value > get_max_train(victim,STAT_DEX) )
      {
         sprintf(buf,
         "Dexterity ranges is 3 to %d.\n\r",
         get_max_train(victim,STAT_DEX));
         send_to_char( buf, ch );
         return;
      }

      victim->perm_stat[STAT_DEX] = value;
      return;
   }

   if ( !str_cmp( arg2, "con" ) )
   {
      if ( value < 3 || value > get_max_train(victim,STAT_CON) )
      {
         sprintf(buf,
         "Constitution range is 3 to %d.\n\r",
         get_max_train(victim,STAT_CON));
         send_to_char( buf, ch );
         return;
      }

      victim->perm_stat[STAT_CON] = value;
      return;
   }

   if ( !str_prefix( arg2, "sex" ) )
   {
      if ( value < 0 || value > 2 )
      {
         send_to_char( "Sex range is 0 to 2.\n\r", ch );
         return;
      }
      victim->sex = value;
      if (!IS_NPC(victim))
         victim->pcdata->true_sex = value;
      return;
   }

   if ( !str_prefix( arg2, "class" ) )
   {
      int class;

      if (IS_NPC(victim))
      {
         send_to_char("Mobiles have no class.\n\r",ch);
         return;
      }

      class = class_lookup(arg3);
      if ( class == -1 )
      {
         char buf[MAX_STRING_LENGTH];

         strcpy( buf, "Possible classes are: " );
         for ( class = 0; class < MAX_CLASS; class++ )
         {
            if ( class > 0 )
               strcat( buf, " " );
            strcat( buf, class_table[class].name );
         }
         strcat( buf, ".\n\r" );

         send_to_char(buf,ch);
         return;
      }

      victim->class = class;
      return;
   }

   if ( !str_prefix( arg2, "level" ) )
   {
      if ( !IS_NPC(victim) )
      {
         send_to_char( "Not on PC's.\n\r", ch );
         return;
      }

      if ( value < 0 || value > 200 )
      {
         send_to_char( "Level range is 0 to 200.\n\r", ch );
         return;
      }
      victim->level = value;
      return;
   }

   if ( !str_prefix( arg2, "gold" ) )
   {
      victim->gold = value;
      return;
   }

   if ( !str_prefix( arg2, "hp" ) )
   {
      if ( value < 1 || value > 30000 )
      {
         send_to_char( "Hp range is 1 to 30,000 hit points.\n\r", ch );
         return;
      }
      victim->max_hit = value;
      if (!IS_NPC(victim))
         victim->pcdata->perm_hit = value;
      return;
   }

   if ( !str_prefix( arg2, "mana" ) )
   {
      if ( value < 0 || value > 30000 )
      {
         send_to_char( "Mana range is 0 to 30,000 mana points.\n\r", ch );
         return;
      }
      victim->max_mana = value;
      if (!IS_NPC(victim))
         victim->pcdata->perm_mana = value;
      return;
   }

   if ( !str_prefix( arg2, "move" ) )
   {
      if ( value < 0 || value > 30000 )
      {
         send_to_char( "Move range is 0 to 30,000 move points.\n\r", ch );
         return;
      }
      victim->max_move = value;
      if (!IS_NPC(victim))
         victim->pcdata->perm_move = value;
      return;
   }

   if ( !str_prefix( arg2, "practice" ) )
   {
      if ( value < 0 || value > 250 )
      {
         send_to_char( "Practice range is 0 to 250 sessions.\n\r", ch );
         return;
      }
      victim->practice = value;
      return;
   }

   if ( !str_prefix( arg2, "train" ))
   {
      if (value < 0 || value > 50 )
      {
         send_to_char("Training session range is 0 to 50 sessions.\n\r",ch);
         return;
      }
      victim->train = value;
      return;
   }

   if ( !str_prefix( arg2, "align" ) )
   {
      if ( value < -1000 || value > 1000 )
      {
         send_to_char( "Alignment range is -1000 to 1000.\n\r", ch );
         return;
      }
      victim->alignment = value;
      return;
   }

   if ( !str_cmp( arg2, "dam" ) )
   {
      if ( value < 1 || value > 100 )
      {
         sprintf(buf,
         "|RDamroll range is 1 to 100.|w\n\r");
         send_to_char(buf,ch);
         return;
      }

      victim->damroll = value;
      return;
   }

   if ( !str_cmp( arg2, "hit" ) )
   {
      if ( value < 1 || value > 100 )
      {
         sprintf(buf,
         "|RHitroll range is 1 to 100.|w\n\r");
         send_to_char(buf,ch);
         return;
      }

      victim->hitroll = value;
      return;
   }


   if ( !str_prefix( arg2, "thirst" ) )
   {
      if ( IS_NPC(victim) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      if ( value < -1 || value > 100 )
      {
         send_to_char( "Thirst range is -1 to 100.\n\r", ch );
         return;
      }

      victim->pcdata->condition[COND_THIRST] = value;
      return;
   }

   if ( !str_prefix( arg2, "drunk" ) )
   {
      if ( IS_NPC(victim) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      if ( value < -1 || value > 100 )
      {
         send_to_char( "Drunk range is -1 to 100.\n\r", ch );
         return;
      }

      victim->pcdata->condition[COND_DRUNK] = value;
      return;
   }

   if ( !str_prefix( arg2, "full" ) )
   {
      if ( IS_NPC(victim) )
      {
         send_to_char( "Not on NPC's.\n\r", ch );
         return;
      }

      if ( value < -1 || value > 100 )
      {
         send_to_char( "Full range is -1 to 100.\n\r", ch );
         return;
      }

      victim->pcdata->condition[COND_FULL] = value;
      return;
   }

   if (!str_prefix( arg2, "race" ) )
   {
      int race;

      race = race_lookup(arg3);

      if ( race == 0)
      {
         send_to_char("That is not a valid race.\n\r",ch);
         return;
      }

      if (!IS_NPC(victim) && !race_table[race].pc_race)
      {
         send_to_char("That is not a valid player race.\n\r",ch);
         return;
      }

      victim->race = race;
      return;
   }

   if ( !str_cmp( arg2, "hours" ) )
   {
      if ( value < 1 || value > 999 )
      {
         sprintf(buf,
         "|RHours range is 1 to 999.|w\n\r");
         send_to_char(buf,ch);
         return;
      }

      victim->played = (value * 3600);
      return;
   }

   /*
        * Generate usage message.
        */
   do_mset( ch, "" );
   return;
}

void do_string( CHAR_DATA *ch, char *argument )
{
   char type [MAX_INPUT_LENGTH];
   char arg1 [MAX_INPUT_LENGTH];
   char arg2 [MAX_INPUT_LENGTH];
   char arg3 [MAX_INPUT_LENGTH];
   CHAR_DATA *victim;
   OBJ_DATA *obj;

   smash_tilde( argument );
   argument = one_argument( argument, type );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   strcpy( arg3, argument );

   if ( type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] ==
'\0' )
   {
      send_to_char("Syntax:\n\r",ch);
      send_to_char("  string char <name> <field> <string>\n\r",ch);
      send_to_char("    fields: name short long desc title spec\n\r",ch);
      send_to_char("  string obj  <name> <field> <string>\n\r",ch);
      send_to_char("    fields: name short long extended wear\n\r",ch);
      return;
   }

   if (!str_prefix(type,"character") || !str_prefix(type,"mobile"))
   {
      if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }

      /* string something */

      if ( !str_prefix( arg2, "name" ) )
      {
         if ( !IS_NPC(victim) )
         {
            send_to_char( "Not on PC's.\n\r", ch );
            return;
         }

         free_string( victim->name );
         victim->name = str_dup( arg3 );
         return;
      }

      if ( !str_prefix( arg2, "description" ) )
      {
         free_string(victim->description);
         victim->description = str_dup(arg3);
         return;
      }

      if ( !str_prefix( arg2, "short" ) )
      {
         free_string( victim->short_descr );
         victim->short_descr = str_dup( arg3 );
         return;
      }

      if ( !str_prefix( arg2, "long" ) )
      {
         free_string( victim->long_descr );
         strcat(arg3,"\n\r");
         victim->long_descr = str_dup( arg3 );
         return;
      }

      if ( !str_prefix( arg2, "title" ) )
      {
         if ( IS_NPC(victim) )
         {
            send_to_char( "Not on NPC's.\n\r", ch );
            return;
         }

         set_title( victim, arg3 );
         return;
      }

      if ( !str_prefix( arg2, "spec" ) )
      {
         if ( !IS_NPC(victim) )
         {
            send_to_char( "Not on PC's.\n\r", ch );
            return;
         }

         if ( ( victim->spec_fun = spec_lookup( arg3 ) ) == 0 )
         {
            send_to_char( "No such spec fun.\n\r", ch );
            return;
         }

         return;
      }
   }

   if (!str_prefix(type,"object"))
   {
      /* string an obj */

      if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
      {
         send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
         return;
      }

      if ( !str_prefix( arg2, "name" ) )
      {
         free_string( obj->name );
         obj->name = str_dup( arg3 );
         return;
      }

      if ( !str_prefix( arg2, "short" ) )
      {
         free_string( obj->short_descr );
         obj->short_descr = str_dup( arg3 );
         return;
      }

      if ( !str_prefix( arg2, "long" ) )
      {
         free_string( obj->description );
         obj->description = str_dup( arg3 );
         return;
      }

      if ( !str_prefix( arg2, "wear" ) )
	{
	  if (strlen(arg3) > 17)
	    {
	      send_to_char( "Wear_Strings may not be longer than 17 chars.\n\r"
			    , ch);
	    }
	  else
	    {
	      free_string( obj->wear_string );
	      obj->wear_string = str_dup( arg3 );
	    }
	  return;
      }

      if ( !str_prefix( arg2, "ed" ) || !str_prefix( arg2, "extended"))
      {
         EXTRA_DESCR_DATA *ed;

         argument = one_argument( argument, arg3 );
         if ( argument == NULL )
         {
            send_to_char( "Syntax: oset <object> ed <keyword> <string>\n\r",
            ch );
            return;
         }

         strcat(argument,"\n\r");

         if ( extra_descr_free == NULL )
         {
            ed			= alloc_perm( sizeof(*ed) );
         }
         else
         {
            ed			= extra_descr_free;
            extra_descr_free	= ed->next;
         }

         ed->keyword		= str_dup( arg3     );
         ed->description	= str_dup( argument );
         ed->next		= obj->extra_descr;
         obj->extra_descr	= ed;
         return;
      }
   }


   /* echo bad use message */
   do_string(ch,"");
}



void do_oset( CHAR_DATA *ch, char *argument )
{
   char arg1 [MAX_INPUT_LENGTH];
   char arg2 [MAX_INPUT_LENGTH];
   char arg3 [MAX_INPUT_LENGTH];
   OBJ_DATA *obj;
   int value;

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   strcpy( arg3, argument );

   if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
   {
      send_to_char("Syntax:\n\r",ch);
      send_to_char("  set obj <object> <field> <value>\n\r",ch);
      send_to_char("  Field being one of:\n\r",				ch );
      send_to_char("    value0 value1 value2 value3 value4 (v1-v4)\n\r",	ch );
      send_to_char("    extra wear level weight cost timer nolocate\n\r",		ch
);
      send_to_char("    (for extra and wear, use set obj <o> <extra/wear> ? to list\n\r", ch);
      return;
   }

   if ( ( obj = get_obj_world( ch, arg1 ) ) == NULL )
   {
      send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
      return;
   }

   /*
        * Snarf the value (which need not be numeric).
        */
   value = atoi( arg3 );

   /*
        * Set something.
        */
   if ( !str_cmp( arg2, "value0" ) || !str_cmp( arg2, "v0" ) )
   {
      obj->value[0] = UMIN(75,value);
      return;
   }

   if ( !str_cmp( arg2, "value1" ) || !str_cmp( arg2, "v1" ) )
   {
      obj->value[1] = value;
      return;
   }

   if ( !str_cmp( arg2, "value2" ) || !str_cmp( arg2, "v2" ) )
   {
      obj->value[2] = value;
      return;
   }

   if ( !str_cmp( arg2, "value3" ) || !str_cmp( arg2, "v3" ) )
   {
      obj->value[3] = value;
      return;
   }

   if ( !str_cmp( arg2, "value4" ) || !str_cmp( arg2, "v4" ) )
   {
      obj->value[4] = value;
      return;
   }

   if ( !str_prefix( arg2, "extra" ) )
   {

      send_to_char("Current extra flags are: \n\r", ch);
      obj->extra_flags = (int)flag_set( ITEM_EXTRA_FLAGS, arg3, obj->extra_flags, ch);
      return;
   }

   if ( !str_prefix( arg2, "wear" ) )
   {
      send_to_char("Current wear flags are: \n\r", ch);
      obj->wear_flags = (int)flag_set( ITEM_WEAR_FLAGS, arg3, obj->wear_flags, ch);
      return;

   }

   if ( !str_prefix( arg2, "level" ) )
   {
      obj->level = value;
      return;
   }

   if ( !str_prefix( arg2, "weight" ) )
   {
      obj->weight = value;
      return;
   }

   if ( !str_prefix( arg2, "cost" ) )
   {
      obj->cost = value;
      return;
   }

   if ( !str_prefix( arg2, "timer" ) )
   {
      obj->timer = value;
      return;
   }

   /* NO_LOCATE flag.  0 turns it off, 1 turns it on *WHAHEY!* */
   if ( !str_prefix( arg2, "nolocate" ) )
     {
       if (value == 0)
	 REMOVE_BIT(obj->extra_flags, ITEM_NO_LOCATE);
       else
	 SET_BIT(obj->extra_flags, ITEM_NO_LOCATE);
       return;
     }

   /*
        * Generate usage message.
        */
   do_oset( ch, "" );
   return;
}



void do_rset( CHAR_DATA *ch, char *argument )
{
   char arg1 [MAX_INPUT_LENGTH];
   char arg2 [MAX_INPUT_LENGTH];
   char arg3 [MAX_INPUT_LENGTH];
   ROOM_INDEX_DATA *location;
   int value;

   smash_tilde( argument );
   argument = one_argument( argument, arg1 );
   argument = one_argument( argument, arg2 );
   strcpy( arg3, argument );

   if ( arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' )
   {
      send_to_char( "Syntax:\n\r",ch);
      send_to_char( "  set room <location> <field> <value>\n\r",ch);
      send_to_char( "  Field being one of:\n\r",			ch );
      send_to_char( "    flags sector\n\r",				ch );
      send_to_char( "  (use set room <location> flags ? to list flags\n\r", ch
);
      return;
   }

   if ( ( location = find_location( ch, arg1 ) ) == NULL )
   {
      send_to_char( "No such location.\n\r", ch );
      return;
   }

   if ( !str_prefix( arg2, "flags" ) )
   {
      send_to_char( "The current room flags are:\n\r", ch);
      location->room_flags	= (int)flag_set( ROOM_FLAGS, arg3, location->room_flags,
ch);
      return;
   }

       /*
        * Snarf the value.
        */
   if ( !is_number( arg3 ) )
   {
      send_to_char( "Value must be numeric.\n\r", ch );
      return;
   }
   value = atoi( arg3 );

   /*
        * Set something.
        */
   if ( !str_prefix( arg2, "sector" ) )
   {
      location->sector_type	= value;
      return;
   }

   /*
        * Generate usage message.
        */
   do_rset( ch, "" );
   return;
}

void do_sockets( CHAR_DATA *ch, char *argument )
{
   char buf[2 * MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   DESCRIPTOR_DATA *d;
   int count;

   count	= 0;
   buf[0]	= '\0';

   one_argument(argument,arg);
   for ( d = descriptor_list; d != NULL; d = d->next )
   {
      if ( d->character != NULL && can_see( ch, d->character )
      && (arg[0] == '\0' || is_name(arg,d->character->name)
      || (d->original && is_name(arg,d->original->name))))
      {
         count++;
         sprintf( buf + strlen(buf), "[%3d %8u %2d] %s:%s@%s\n\r",
         d->descriptor,
         (d->localport & 0xffff),
         d->connected,
         d->original  ? d->original->name  :
         d->character ? d->character->name : "(none)",
	 d->realname,
         d->host
         );
      } else if (d->character == NULL && get_trust(ch)==MAX_LEVEL) {
	   /*
	    * New: log even connections that haven't logged in yet 
	    * Level 100s only, mind
	    */
	   count++;
	   sprintf (buf+strlen(buf), "[%3d %8u %2d] (unknown):%s@%s\n\r",
		    d->descriptor,
		    (d->localport & 0xffff),
		    d->connected,
		    d->realname,
		    d->host
		    );
      }
   }
   if (count == 0)
   {
      send_to_char("No one by that name is connected.\n\r",ch);
      return;
   }

   sprintf( buf2, "%d user%s\n\r", count, count == 1 ? "" : "s" );
   strcat(buf,buf2);
   page_to_char( buf, ch );
   return;
}



/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force( CHAR_DATA *ch, char *argument )
{
   char buf[MAX_STRING_LENGTH];
   char arg[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];

   argument = one_argument( argument, arg );

   if ( arg[0] == '\0' || argument[0] == '\0' )
   {
      send_to_char( "Force whom to do what?\n\r", ch );
      return;
   }

   one_argument(argument,arg2);

   if (!str_cmp(arg2,"delete"))
   {
      send_to_char("That will NOT be done.\n\r",ch);
      return;
   }

   if (!str_cmp(arg2,"private")) {
      send_to_char("That will NOT be done.\n\r", ch);
      return;
   }

   sprintf( buf, "$n forces you to '%s'.", argument );

   if ( !str_cmp( arg, "all" ) )
   {
      CHAR_DATA *vch;
      CHAR_DATA *vch_next;

      if (get_trust(ch) < MAX_LEVEL - 3)
      {
         send_to_char("Not at your level!\n\r",ch);
         return;
      }

      for ( vch = char_list; vch != NULL; vch = vch_next )
      {
         vch_next = vch->next;

         if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch ) )
         {
/* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
		 MOBtrigger = FALSE;
            act( buf, ch, NULL, vch, TO_VICT );
            interpret( vch, argument );
         }
      }
   }
   else if (!str_cmp(arg,"players"))
   {
      CHAR_DATA *vch;
      CHAR_DATA *vch_next;

      if (get_trust(ch) < MAX_LEVEL - 2)
      {
         send_to_char("Not at your level!\n\r",ch);
         return;
      }

      for ( vch = char_list; vch != NULL; vch = vch_next )
      {
         vch_next = vch->next;

         if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch )
         &&	 vch->level < LEVEL_HERO)
         {
/* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
		 MOBtrigger = FALSE;
            act( buf, ch, NULL, vch, TO_VICT );
            interpret( vch, argument );
         }
      }
   }
   else if (!str_cmp(arg,"gods"))
   {
      CHAR_DATA *vch;
      CHAR_DATA *vch_next;

      if (get_trust(ch) < MAX_LEVEL - 2)
      {
         send_to_char("Not at your level!\n\r",ch);
         return;
      }

      for ( vch = char_list; vch != NULL; vch = vch_next )
      {
         vch_next = vch->next;

         if ( !IS_NPC(vch) && get_trust( vch ) < get_trust( ch )
         &&   vch->level >= LEVEL_HERO)
         {
/* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
		 MOBtrigger = FALSE;
            act( buf, ch, NULL, vch, TO_VICT );
            interpret( vch, argument );
         }
      }
   }
   else
   {
      CHAR_DATA *victim;

      if ( ( victim = get_char_world( ch, arg ) ) == NULL )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }

      if ( victim == ch )
      {
         send_to_char( "Aye aye, right away!\n\r", ch );
         return;
      }

      if ( get_trust( victim ) >= get_trust( ch ) )
      {
         send_to_char( "Do it yourself!\n\r", ch );
         return;
      }

      if ( !IS_NPC(victim) && get_trust(ch) < MAX_LEVEL -3)
      {
         send_to_char("Not at your level!\n\r",ch);
         return;
      }
/* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
		 MOBtrigger = FALSE;
      act( buf, ch, NULL, victim, TO_VICT );
      interpret( victim, argument );
   }

   send_to_char( "Ok.\n\r", ch );
   return;
}



/*
 * New routines by Dionysos.
 */
void do_invis( CHAR_DATA *ch, char *argument )
{
   int level;
   char arg[MAX_STRING_LENGTH];

   if ( IS_NPC(ch) )
      return;

   /* RT code for taking a level argument */
   one_argument( argument, arg );

   if ( arg[0] == '\0' )
      /* take the default path */

      if ( IS_SET(ch->act, PLR_WIZINVIS) )
      {
         REMOVE_BIT(ch->act, PLR_WIZINVIS);
         ch->invis_level = 0;
         act( "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM );
         send_to_char( "You slowly fade back into existence.\n\r", ch );
      }
      else
      {
         SET_BIT(ch->act, PLR_WIZINVIS);
         if (IS_SET(ch->act, PLR_PROWL))
            REMOVE_BIT(ch->act, PLR_PROWL);
         ch->invis_level = get_trust(ch);
         act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
         send_to_char( "You slowly vanish into thin air.\n\r", ch );
         if (ch->pet != NULL)
         {
            SET_BIT(ch->pet->act, PLR_WIZINVIS);
            if (IS_SET(ch->pet->act, PLR_PROWL))
               REMOVE_BIT(ch->pet->act, PLR_PROWL);
            ch->pet->invis_level = get_trust(ch);
         }
      }
   else
   /* do the level thing */
   {
      level = atoi(arg);
      if (level < 2 || level > get_trust(ch))
      {
         send_to_char("Invis level must be between 2 and your level.\n\r",ch);
         return;
      }
      else
      {
         ch->reply = NULL;
         SET_BIT(ch->act, PLR_WIZINVIS);
         ch->invis_level = level;
         act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
         send_to_char( "You slowly vanish into thin air.\n\r", ch );
      }
   }

   return;
}

void do_prowl( CHAR_DATA *ch, char *argument)
{
   char arg[MAX_STRING_LENGTH];
   int  level = 0;

   if IS_NPC(ch)
      return;

   if (ch->level < LEVEL_HERO)
   {
      send_to_char("Huh?\n\r",ch);
      return;
   }

   argument = one_argument(argument, arg);
   if (arg[0] == '\0')
   {
      if (IS_SET(ch->act, PLR_PROWL))
      {
         REMOVE_BIT(ch->act, PLR_PROWL);
         ch->invis_level = 0;
         if (ch->pet != NULL)
         {
            REMOVE_BIT(ch->pet->act, PLR_PROWL);
            ch->pet->invis_level = 0;
         }
         return;
      }
      else
      {
         ch->invis_level = get_trust(ch);
         SET_BIT(ch->act, PLR_PROWL);
         if (ch->pet != NULL)
         {
            ch->pet->invis_level = get_trust(ch);
            SET_BIT(ch->pet->act, PLR_PROWL);
         }
         act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
         send_to_char( "You slowly vanish into thin air.\n\r", ch );
         REMOVE_BIT(ch->act, PLR_WIZINVIS);
         if (ch->pet != NULL)
            REMOVE_BIT(ch->pet->act, PLR_WIZINVIS);
         return;
      }
   }

   level = atoi(arg);

   if ( (level > get_trust(ch)) || (level < 2) )
   {
      send_to_char("You must specify a level between 2 and your level.\n\r"
      ,ch);
      return;
   }

   if( IS_SET(ch->act, PLR_PROWL) )
   {
      REMOVE_BIT(ch->act, PLR_PROWL);
      ch->invis_level = 0;
      if (ch->pet != NULL)
      {
         REMOVE_BIT(ch->pet->act, PLR_PROWL);
         ch->pet->invis_level = 0;
      }
      return;
   }
   else
   {
      ch->invis_level = level;
      SET_BIT(ch->act, PLR_PROWL);
      if (ch->pet != NULL)
      {
         SET_BIT(ch->pet->act, PLR_PROWL);
         ch->pet->invis_level = level;
      }
      act( "$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM );
      send_to_char( "You slowly vanish into thin air.\n\r", ch );
      REMOVE_BIT(ch->act, PLR_WIZINVIS);
      if (ch->pet != NULL)
         REMOVE_BIT(ch->pet->act, PLR_WIZINVIS);
      return;
   }
}

void do_holylight( CHAR_DATA *ch, char *argument )
{
  (void)argument;
   if ( IS_NPC(ch) )
      return;

   if ( IS_SET(ch->act, PLR_HOLYLIGHT) )
   {
      REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
      send_to_char( "Holy light mode off.\n\r", ch );
   }
   else
   {
      SET_BIT(ch->act, PLR_HOLYLIGHT);
      send_to_char( "Holy light mode on.\n\r", ch );
   }

   return;
}

void do_sacname( CHAR_DATA *ch, char *argument)
{

   char buf[MAX_STRING_LENGTH];

   if (argument[0] == '\0')
   {
      send_to_char( "You must tell me who they're gonna sacrifice to!\n\r", ch );
      sprintf(buf, "Currently sacrificing to: %s\n\r", deity_name);
      send_to_char(buf, ch);
      return;
   }
   strcpy( deity_name_area, argument);
   sprintf( buf, "Players now sacrifice to %s.\n\r", deity_name);
   send_to_char( buf, ch );
}

