/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  olc_room.c:  adapted from TheMoog's original by Faramir              */
/*                                                                       */
/*************************************************************************/


#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#if defined(riscos)
#include <time.h>
#include "sys/types.h"
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "db.h"
#include "flags.h"
#include "buffer.h"
#include "olc_ctrl.h"
#include "olc_room.h"


/*
 * unlike the object and mobiles editors, room editing does not
 * involve changing a template and then ending the session to
 * update the mud. This is because there is only ever one
 * instantiation of a room, and to work merely on a template
 * would make things -very- complicated, with respects to
 * exit data etc etc. Thus, someone working on a room will
 * work on the _actual_ room rather than a template
 */


/*
 * the main room command which leads to everything else...
 */

void
do_room ( CHAR_DATA *ch, char *argument  ) {

          char       buf[MAX_STRING_LENGTH];

    if (!IS_SET (ch->comm, COMM_OLC_MODE)) {
	send_to_char("Huh?\n\r", ch);
	return;
    }
    if (!olc_user_authority(ch)) {
	send_to_char("OLC: sorry, only Faramir can use OLC commands atm.\n\r", ch);
	return;
    }
    /* shouldn't EVER happen, but oh well...*/
    if ((is_set_extra(ch, EXTRA_OLC_ROOM)) &&
	ch->in_room == NULL) {
	bug( "do_room: OLC: error! player %s is in OLC_ROOM mode, but room is NULL\n\r",
	     ch->name);
	send_to_char("OLC: error! Your current room is NULL. Consult an IMP now!\n\r", ch);
	return;
    }

    if (ch->in_room->vnum != 0 ) {
	if (argument[0] != '\0') {
	    olc_edit_room( ch, argument);
	    return;
	}
	send_to_char("Syntax: room [command] ? to list.\n\r", ch);
	return;
    }
    if (is_set_extra(ch, EXTRA_OLC_MOB ) ||
	is_set_extra(ch, EXTRA_OLC_OBJ )) {

	send_to_char("OLC: leave your current olc mode and enter room mode first.\n\r", ch);
	return;
    }
    if (is_number(argument)) {
	if ( olc_can_edit_room ( ch, argument )) {
	    ROOM_INDEX_DATA *new_room;
	    new_room = get_room_index(atoi(argument));
	    set_extra( ch, EXTRA_OLC_OBJ );
	    snprintf (buf, sizeof(buf), "OLC: entering room mode to edit existing room #%d.\n\r",
		     atoi(argument) );
	    send_to_char( buf, ch);
	    send_to_char("Syntax: room [command] ? to list.\n\r", ch);
	    return;
	}
	send_to_char("OLC: either that room does not exist, or it cannot be edited.\n\r", ch);
	send_to_char("Syntax: room <vnum> {add}\n\r", ch);
	send_to_char("        (to begin editing an existing, or new room)\n\r" , ch);
	return;
    } else {
	olc_parse_room ( ch, argument );
	return;
    }
}

/*
 * if they want to add a room
 */

void
olc_parse_room ( CHAR_DATA * ch, char * argument ) {

    char arg1[MAX_INPUT_LENGTH],
      command[MAX_INPUT_LENGTH];
    int  vnum = 0;

    argument = one_argument(argument, arg1);
    argument = one_argument(argument, command);

    if ( command[0] == '\0' ) {
	send_to_char("Syntax: room <vnum> {add}\n\r", ch);
	return;
    }
    if (is_number(arg1)) {
	if (!str_cmp(command, "add")) {
	    if (olc_can_create_room(ch, arg1)) {
		send_to_char("OLC: attempting to create room ...\n\r", ch);
		vnum = atoi(arg1);
		olc_create_room(ch, vnum );
		return;
	    }
	    send_to_char("OLC: that vnum is already used, or is unavailable.\n\r",
			 ch);
	    return;
	}
    }
    send_to_char("Syntax: room <vnum> {add}\n\r", ch);
    return;
}


/*
 * the core of the room editor
 */

void
olc_edit_room ( CHAR_DATA *ch, char * argument) {
    char command[MAX_INPUT_LENGTH];
    int temp = 0;

    if ( argument[0] == '?' || argument[0] == '\0' ) {
	send_to_char("Syntax:       room <command>\n\r", ch);
	send_to_char("              room <command> ?  (to get help on command)\n\r", ch);
	send_to_char("for editing:  exit flags sector string\n\r", ch);
	send_to_char("control:      destroy end undo update\n\r", ch);
	return;
    }
    /* just in fubing case ..*/
    if ( ch->in_room == NULL ) {
	bug("olc_edit_obj: OLC: error! player %s is in OLC_ROOM mode, but room is NULL\n\r",
	    ch->name );
	send_to_char("OLC: error! your current room is NULL, consult an IMP immediately.\n\r", ch);
	return;
    }
    argument = one_argument( argument, command );
    if ( ( temp = atoi (command) ) == (ch->in_room->vnum) ) {
	send_to_char("OLC: you are already editing that room!\n\r", ch);
	return;
    }
    if (!str_cmp( command, "destroy")) {
	olc_destroy_room( ch, argument );
	return;
    }
    if (!str_cmp( command, "end")) {
	/*		olc_end_rooms( ch ); */
	return;
    }
    if (!str_prefix( command, "flags")) {
/*	olc_flag_room( ch, argument ); */
	return;
    }
    if (!str_prefix( command, "string")) {
	olc_string_room( ch, argument);
	return;
    }
    if (!str_cmp( command, "undo")) {
	/*		olc_undo_current_room( ch ); */
	return;
    }
    if (!str_cmp( command, "update")) {
	/*		olc_update_room( ch ); */
	return;
    }
    send_to_char("OLC: unrecognised room command: room ? to list.\n\r", ch);
    return;
}

/*
 * checks against area editing permissions
 */

bool
olc_can_edit_room ( CHAR_DATA *ch, char * argument) {

    int vnum = 0;

    /* temporary */

    return TRUE;

    vnum = atoi(argument);

    if ( vnum < ch->pcdata->olc->room_vnums[0] ||
	 vnum > ch->pcdata->olc->room_vnums[1] )

	return FALSE;

    return TRUE;
}

/*
 * are they going beyond the vnum ranges prescribed?
 */

bool
olc_can_create_room (CHAR_DATA *ch, char * argument ) {

    int vnum = 0;
    vnum = atoi(argument);
    if ( (get_room_index( vnum )) != NULL )
	return FALSE;
    if ( vnum < ch->pcdata->olc->master_vnums[0] ||
	 vnum > ch->pcdata->olc->master_vnums[1] )
	return FALSE;
    return TRUE;
}

/*
 * awoo - initiate the creation of a new room
 */

void
olc_create_room ( CHAR_DATA *ch, int vnum ) {

    ROOM_INDEX_DATA *pRoomIndex;
    char buf[MAX_STRING_LENGTH];
    int newvnum = 0;

    if ((olc_make_new_room(vnum)==0)) {
	snprintf(buf, sizeof(buf), "char: %s room:%d",ch->name,ch->in_room->vnum );
	send_to_char( "OLC: room creation failed.\n\r", ch);
	bug("OLC: error: room creation failed.");
	bug( buf );
	return;
    }
    if ( (pRoomIndex = get_room_index(vnum)) == NULL ) {
	snprintf(buf, sizeof(buf), "char: %s room: %d", ch->name, vnum );
	bug("OLC: error: olc_create_room new room lost!");
	bug(buf);
	send_to_char("OLC: error! new room has been lost!\n\r", ch);
	return;
    }
    pRoomIndex->area = ch->in_room->area;
    snprintf(buf, sizeof(buf), "A desolate wasteland, bereft of shape and form, just waiting to be forged into\n\ra new and exciting room.  At least, I hope it is, %s!\n\r", ch->name);
    pRoomIndex->description = str_dup(buf);
    snprintf( buf, sizeof(buf), "Room %d created OK.\n\r", newvnum);
    send_to_char( buf, ch);
    return;
}

/*
 * making a new room and linking it in..
 */

int
olc_make_new_room ( int vnum ) {

    int door,iHash;
    ROOM_INDEX_DATA *pRoomIndex;

    if (get_room_index(vnum) != NULL)
	return 0;

    pRoomIndex			= alloc_perm( sizeof(*pRoomIndex) );
    pRoomIndex->people		= NULL;
    pRoomIndex->contents	= NULL;
    pRoomIndex->extra_descr	= NULL;
    pRoomIndex->area		= area_last;
    pRoomIndex->vnum		= vnum;
    pRoomIndex->name		= NULL;
    pRoomIndex->description	= NULL;
    pRoomIndex->room_flags	= 0;
    pRoomIndex->sector_type     = 0;
    pRoomIndex->light		= 0;
    for ( door = 0; door <= 5; door++ )
	pRoomIndex->exit[door] = NULL;

    iHash			= vnum % MAX_KEY_HASH;
    pRoomIndex->next	= room_index_hash[iHash];
    room_index_hash[iHash]	= pRoomIndex;

    return vnum;
}

/*
 *  prepare to annihilate a room
 */

void
olc_destroy_room ( CHAR_DATA * ch, char * argument ) {

    char buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *room;
    int num;

    num = atoi ( argument );
    if ( (room = get_room_index ( num ))==NULL ) {
	snprintf (buf, sizeof(buf), "char: %s room: %d", ch->name, num );
	bug("OLC: error: olc_destroy_room could not find room!");
	bug(buf);
	send_to_char("OLC: error! destroy room failed - seek help!\n\r", ch);
	return;
    }
    if (!olc_can_create_room(ch, argument)) {
	send_to_char("OLC: you do not have permission to destroy that room.\n\r", ch);
	return;
    }
    if (room->people != NULL ) {
	CHAR_DATA *rch;
	send_to_char("OLC: room is not empty.\n\r", ch);
	for ( rch = room->people; rch ;rch=rch->next_in_room) {
	    if (rch == ch) {
		send_to_char( "Yourself!\n\r",ch);
	    } else {
		snprintf(buf, sizeof(buf), "%s (%s)\n\r", rch->name, IS_NPC(rch)?"NPC":"PC");
		send_to_char( buf, ch);
	    }
	}
	send_to_char("OLC: please clear remove room inhabitants before destroying.\n\r", ch);
	return;
    }
    if (!olc_erase_room (num) ) {
	snprintf(buf, sizeof(buf), "char: %s room: %d", ch->name, num );
	send_to_char("OLC: room was not deleted from the database.\n\r", ch);
	bug("OLC: error: olc_erase_room erasure failed");
	bug(buf);
	return;
    }
    send_to_char("OLC: room destroyed.\n\r", ch);
    return;
}

/*
 * erase his ass!
 */

bool
olc_erase_room(int vnum) {
    int iHash;
    ROOM_INDEX_DATA *pRoomIndex;
    ROOM_INDEX_DATA *prev;
    EXTRA_DESCR_DATA *extras;

    if ( (pRoomIndex = get_room_index( vnum )) == NULL )
	return FALSE;
    iHash			= vnum % MAX_KEY_HASH;
    prev			= room_index_hash[iHash];
    if ( prev == pRoomIndex ) {
	room_index_hash[iHash] = pRoomIndex->next;
    }
    else {
	for ( ; prev; prev = prev->next) {
	    if ( prev->next == pRoomIndex ) {
		prev->next = pRoomIndex->next;
		break;
	    }
	}
	if ( prev == NULL )
	    return 0;
    }
    pRoomIndex->next = NULL;
    free_string(pRoomIndex->name);
    free_string(pRoomIndex->description);

    if (pRoomIndex->extra_descr != NULL) {
	for ( extras = pRoomIndex->extra_descr ; extras != NULL ; extras = extras->next ) {
	    free_string(extras->keyword);
	    free_string(extras->description);
	}
	pRoomIndex->extra_descr = NULL;
    }
    return TRUE;
}


/*
 * various utils to change the strings assoc with a room
 */

void
olc_string_room( CHAR_DATA * ch, char * argument ) {

    char arg1[MAX_STRING_LENGTH];

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0' || arg1[0] == '?' ) {
	send_to_char("Syntax: room string [extras||description||name]\n\r", ch);
	return;
    }
    if (!str_prefix(arg1, "extras")) {
	olc_room_string_extras(ch, argument);
	return;
    }
    if (!str_prefix(arg1, "name")) {
	olc_room_string_name(ch, argument);
	return;
    }
    if (!str_prefix(arg1, "description")) {
	olc_room_string_description(ch, argument);
	return;
    }
    send_to_char( "OLC:    unrecognised room string command.\n\r", ch);
    send_to_char( "        room string ? to list", ch);
    return;
}

void
olc_room_string_name( CHAR_DATA * ch, char * argument) {

    char buf[MAX_STRING_LENGTH],
	*ptr;

    if (argument[0] == '\0' || argument[0] == '?' ) {
	send_to_char("Syntax: room string name [clipboard||...]\n\r", ch);
	return;
    }
    if (!str_prefix(argument, "clipboard")) {
	argument = ch->clipboard;
	if (argument == NULL) {
	    send_to_char("OLC: your clipboard is empty.\n\r", ch);
	    return;
	}
    }
    ptr = strcpy(buf, argument);
    for ( ; *ptr ; ptr++ ) {
	if (*ptr=='\n')
	    *ptr='\0';
    }
    free_string(ch->in_room->name );
    ch->in_room->name = str_dup(buf);
    send_to_char("OLC: room name changed.\n\r", ch);
}


void
olc_room_string_description ( CHAR_DATA * ch, char * argument ) {

    if (ch->clipboard == NULL) {
	send_to_char("OLC: your clipboard is empty.\n\r", ch);
	return;
    }
    free_string(ch->in_room->description);
    ch->in_room->description = str_dup(ch->clipboard);
    send_to_char( "OLC: clipboard pasted into current room description.\n\r", ch);
}


/*
 * umm, monster function! I just modified TheMoog's original version of this and
 * couldn't be bothered to split it into smaller chunks :)
 * Linus wouldn't be happy, TM!
 */

void
olc_room_string_extras ( CHAR_DATA * ch, char * argument ) {

    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    EXTRA_DESCR_DATA *extras;

    argument = one_argument(argument, arg1);

    if (arg1[0] == '\0' || arg1[0] == '?' ) {
	send_to_char( "Syntax: room string extras [list||add||remove]\n\r", ch);
	return;
    }
    if (!str_prefix(arg1, "list")) {
	olc_display_room_vars( ch, OLC_R_EXTRAS );
	return;
    }
    if (!str_prefix(arg1,"add"))  {
	char keyword[MAX_STRING_LENGTH];
	argument = one_argument(argument, keyword);
	if ((keyword[0] == '\0') || (argument[0] == '\0')) {
	    send_to_char( "Syntax: room string extras add '<keyword(s)>' [clipboard]||...\n\r", ch);
	    return;
	}
	if (!str_prefix(argument, "clipboard")) {
	    if (ch->clipboard == NULL) {
		send_to_char( "OLC: your clipboard is empty.\n\r", ch);
		return;
	    }
	    argument = ch->clipboard;
	} else {
	    snprintf(buf, sizeof(buf), "%s\n\r", argument);
	    argument = buf;
	}
	if ( extra_descr_free == NULL) {
	    extras = alloc_perm (sizeof(*extras));
	} else {
	    extras = extra_descr_free;
	    extra_descr_free = extras->next;
	}
	extras->keyword = str_dup(keyword);
	extras->description = str_dup(argument);
	extras->next = ch->in_room->extra_descr;
	ch->in_room->extra_descr = extras;
	snprintf( buf, sizeof(buf), "OLC: extra description for keyword '|W%s|w' added as :\n\r", keyword);
	send_to_char(buf, ch);
	send_to_char(argument,ch);
	return;
    }
    if (!str_prefix(arg1,"remove")) {
	char keyword[MAX_STRING_LENGTH];
	EXTRA_DESCR_DATA *prev;
	argument = one_argument(argument, keyword);
	if ( keyword[0] == '\0' || keyword[0] == '?' ) {
	    send_to_char("Syntax: room string extras remove '<keyword(s)>'\n\r", ch);
	    return;
	}
	prev = NULL;
	for ( extras = ch->in_room->extra_descr ;
	      extras ; extras = extras->next) {
	    if (is_name(keyword, extras->keyword)) {
		if (prev==NULL) { /* Then we've matched with the first entry of
				     the linked list */
		    ch->in_room->extra_descr = extras->next;
		} else { /* prev points to the previous element in the list, so */
		    prev->next = extras->next;
		}
		free_string(extras->keyword);
		free_string(extras->description);
		extras->next = extra_descr_free;
		extra_descr_free = extras; /* chain onto free list */
		send_to_char( "OLC: extra descr deleted.\n\r", ch);
		return;
	    }
	    prev = extras;
	}
	send_to_char( "OLC: extra description not found!\n\r", ch);
	return;
    }
    send_to_char( "OLC: unrecognised room string extras command.\n\r", ch);
    send_to_char( "     room string extras ? to list", ch);
    return;
}

/*
 * mega monster function to modify exits....yikes
 */

void
room_edit_exit(CHAR_DATA *ch,char *argument) {
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int direction;
    char c;
    ROOM_INDEX_DATA *room_to;
    bool connect = FALSE;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    c = arg1[0];
    if (c == '+') {
	connect = TRUE;
	c = arg1[1];
    }
    switch (c) {
    case 'N':
    case 'n':
	direction = 0;
	break;
    case 'S':
    case 's':
	direction = 2;
	break;
    case 'E':
    case 'e':
	direction = 1;
	break;
    case 'W':
    case 'w':
	direction = 3;
	break;
    case 'U':
    case 'u':
	direction = 4;
	break;
    case 'D':
    case 'd':
	direction = 5;
	break;
    default:
	send_to_char( "Syntax: room exit {+}[n||s||e||w||u||d] [vnum||close||keyword||description||door||key] {params}\n\r", ch);
	return;
    }
    if (!str_prefix(arg2, "key")) {
	OBJ_DATA *obj = NULL;
	int key_vnum;
	char buf[MAX_STRING_LENGTH];
	if (ch->in_room->exit[direction] == NULL) {
	    send_to_char("OLC: there is no exit in that direction!\n\r", ch);
	    return;
	}
	if (!str_prefix(argument,"none")) {
	    key_vnum = -1;
	} else {
	    one_argument(argument, buf);
	    if ((obj = get_obj_carry(ch, buf)) == NULL) {
		send_to_char( "Syntax: room exit {+}[n||s||e||w||u||d] key [none||object in your inventory]\n\r", ch);
		return;
	    }
	    if (obj->item_type != ITEM_KEY) {
		snprintf(buf, sizeof(buf), "OLC: you could try locking and unlocking doors with a %s, but it's not a key!\n\r", obj->short_descr);
		send_to_char( buf, ch);
		return;
	    }
	    key_vnum = obj->pIndexData->vnum;
	}
	ch->in_room->exit[direction]->key = key_vnum;
	if (key_vnum != -1) {
	    snprintf( buf, sizeof(buf), "OLC: the %s will now unlock and lock the door to the %s.\n\r", obj->short_descr, dir_name[direction]);
	} else {
	    snprintf(buf, sizeof(buf), "OLC: the door to the %s requires no key now.\n\r", dir_name[direction]);
	}
	send_to_char( buf, ch );
	if (connect == TRUE) {
	    if (ch->in_room == ch->in_room->exit[direction]->u1.to_room->
		exit[rev_dir[direction]]->u1.to_room) {
		ch->in_room->exit[direction]->u1.to_room->
		    exit[rev_dir[direction]]->key = key_vnum;
		send_to_char( "OLC: reverse exit also changed.\n\r", ch);
	    } else {
		send_to_char("OLC: couldn't change reverse direction.\n\r",ch);
	    }
	}
    } else if (!str_prefix(arg2, "door")) {
	int type = -1;
	char *door_type = "";
	char buf[MAX_STRING_LENGTH];
	if (ch->in_room->exit[direction] == NULL) {
	    send_to_char("OLC: there is no exit in that direction!\n\r", ch);
	    return;
	}
	if (!str_prefix(argument, "none")) {
	    type = 0;
	    door_type = "none";
	}
	if (!str_prefix(argument, "door")) {
	    type = EX_ISDOOR;
	    door_type = "door";
	}
	if (!str_prefix(argument, "unpickable")) {
	    type = (EX_ISDOOR | EX_PICKPROOF);
	    door_type = "unpickable door";
	}
	if (!str_prefix(argument, "unpassable")) {
	    type = (EX_ISDOOR | EX_PICKPROOF | EX_PASSPROOF);
	    door_type = "unpickable and unpassable door";
	}
	if (type == -1) {
	    send_to_char("Syntax: room exit {+}[n||e||s||w||u||d] door [door||none||unpickable||unpassable]\n\r", ch);
	    return;
	}
	ch->in_room->exit[direction]->exit_info = type;
	snprintf( buf, sizeof(buf), "OLC: changed exit type of the %s exit to '%s'.\n\r",
		 dir_name[direction], door_type);
	send_to_char( buf, ch );
	if (connect == TRUE) {
	    if (ch->in_room == ch->in_room->exit[direction]->u1.to_room->
		exit[rev_dir[direction]]->u1.to_room) {
		ch->in_room->exit[direction]->u1.to_room->
		    exit[rev_dir[direction]]->exit_info = type;
		send_to_char( "OLC: reverse exit also changed.\n\r", ch);
	    } else {
		send_to_char("OLC: couldn't change reverse direction.\n\r",ch);
	    }
	}
    } else if (!str_prefix(arg2, "description")) {
	char buffer[MAX_STRING_LENGTH];
	snprintf(buffer, sizeof(buffer), "%s\n\r", argument);
	if (ch->in_room->exit[direction] == NULL) {
	    send_to_char("OLC: there is no exit in that direction!\n\r", ch);
	    return;
	}
	if (ch->in_room->exit[direction]->description != NULL)
	    free_string(ch->in_room->exit[direction]->description);
	ch->in_room->exit[direction]->description = str_dup(buffer);
	snprintf( buffer, sizeof(buffer), 
		 "OLC: description of %s exit changed.\n\r", dir_name[direction]);
	send_to_char(buffer, ch);
	if (connect==TRUE)
	    send_to_char( "'+' ignored in this case.\n\r", ch);
    } else {
	if (!str_prefix(arg2, "keyword")) {
	    char buffer[MAX_STRING_LENGTH];
	    if (ch->in_room->exit[direction] == NULL) {
		send_to_char("OLC: there is no exit in that direction!\n\r", ch);
		return;
	    }
	    if (ch->in_room->exit[direction]->keyword != NULL)
		free_string(ch->in_room->exit[direction]->keyword);
	    ch->in_room->exit[direction]->keyword = str_dup(argument);
	    snprintf( buffer, sizeof(buffer), "OLC: keyword for exit to the %s changed.\n\r",
		     dir_name[direction]);
	    send_to_char( buffer, ch );
	    if (connect == TRUE) {
		if (ch->in_room->exit[direction]->u1.to_room->exit[rev_dir[direction]]
		    ->u1.to_room== ch->in_room) {
		    if (ch->in_room->exit[direction]->
			u1.to_room->exit[rev_dir[direction]]->keyword != NULL)
			free_string(ch->in_room->exit[direction]->
				    u1.to_room->exit[rev_dir[direction]]->keyword);
		    ch->in_room->exit[direction]->
			u1.to_room->exit[rev_dir[direction]]->keyword = str_dup(argument);
		    send_to_char( "OLC: reverse keyword also changed.\n\r", ch);
		} else {
		    send_to_char( "OLC: could not change reverse keyword.\n\r", ch);
		}
	    }
	} else {
	    if (!str_prefix(arg2, "close")) {
		if (ch->in_room->exit[direction] == NULL) {
		    send_to_char( "OLC: there is no exit in that direction.\n\r", ch);
		    return;
		}
		snprintf( buf, sizeof(buf), "OLC: the %s exit has been destroyed.\n\r", dir_name[direction]);
		send_to_char( buf, ch );
		if (connect == TRUE) {
		    /* Oogle check that there is *a* reverse exit */
		    if (ch->in_room->exit[direction]->u1.to_room->exit[rev_dir[direction]] != NULL) {
			/* Check for reverse exit actually pointing back to the
			   room you're in
			   */
			if (ch->in_room->exit[direction]->u1.to_room->exit[rev_dir[direction]]
			    ->u1.to_room == ch->in_room) {
			    CHAR_DATA *wch = ch->in_room->exit[direction]->u1.to_room->people;
			    ch->in_room->exit[direction]->u1.to_room->exit[rev_dir[direction]]
				= NULL;
			    send_to_char( "OLC: reverse exit also destroyed!\n\r", ch);
			    snprintf( buf, sizeof(buf), "The exit to the %s disappears in a mushroom cloud!\n\r", dir_name[rev_dir[direction]] );
			    for ( ; wch ; wch=wch->next_in_room )
				send_to_char( buf, wch);
			} else {
			    send_to_char( "OLC: couldn't delete reverse exit!\n\r",ch);
			}
		    } else {
			send_to_char( "OLC: no reverse exit to delete!\n\r",ch);
		    }
		}
		ch->in_room->exit[direction] = NULL;
		snprintf( buf, sizeof(buf), "$n has destroyed the %s exit forever!",
			 dir_name[direction] );
		act( buf, ch, NULL, NULL, TO_ROOM);
		return;
	    } else {
		char desc[MAX_STRING_LENGTH];
		if ( (room_to = get_room_index(atoi(arg2))) == NULL) {
		    send_to_char( "OLC: vnum does not exist as far as I can tell.\n\r", ch);
		    return;
		}
		snprintf(desc, sizeof(desc), "%s\n\r", room_to->name);
		if (ch->in_room->exit[direction] == NULL) {
		    EXIT_DATA *pExitData;
		    pExitData = (EXIT_DATA *) alloc_perm(sizeof(*pExitData));
		    pExitData->exit_info   = 0;
		    pExitData->key         =-1;
		    pExitData->keyword     = str_dup("");
		    pExitData->description = str_dup("");
		    ch->in_room->exit[direction] = pExitData;
		}
		if (ch->in_room->exit[direction]->description != NULL)
		    free_string(ch->in_room->exit[direction]->description);
		ch->in_room->exit[direction]->description = str_dup(desc);
		ch->in_room->exit[direction]->u1.to_room = room_to;
		snprintf(buf, sizeof(buf), "OLC: exit created to the %s.\n\r", dir_name[direction]);
		send_to_char( buf, ch );
		if (connect==TRUE) {
		    if (room_to->exit[rev_dir[direction]] == NULL) {
			CHAR_DATA *wch = room_to->people;
			char desc[MAX_STRING_LENGTH];
			EXIT_DATA *pExitData;

			snprintf( desc, sizeof(desc), "%s\n\r", ch->in_room->name);
			pExitData = (EXIT_DATA *) alloc_perm(sizeof(*pExitData));
			pExitData->exit_info   = 0;
			pExitData->key         =-1;
			pExitData->keyword     = str_dup("");
			pExitData->description = str_dup(desc);
			room_to->exit[rev_dir[direction]] = pExitData;
			room_to->exit[rev_dir[direction]]->u1.to_room = ch->in_room;
			send_to_char( "OLC: exit back to this room also created.\n\r", ch);
			snprintf( buf, sizeof(buf), "The earth trembles and an exit to the %s miraculously appears!\n\r", dir_name[rev_dir[direction]]);
			for ( ; wch; wch=wch->next_in_room )
			    send_to_char( buf, wch);
		    } else {
			send_to_char( "OLC: Couldn't link back to this room because room already has an exit\n\rin that direction!\n\r", ch );
		    }
		}

		snprintf( buf, sizeof(buf),
			 "$n calls forth mighty powers and creates a new exit to the %s!",
			 dir_name[direction] );
		act(buf, ch, NULL, NULL, TO_ROOM );
		return;
	    }
	}
    }
}



/*
 * the all-in-one func to show the current room status
 */

void
olc_display_room_vars ( CHAR_DATA * ch, int info ) {

    EXTRA_DESCR_DATA *extras;
    char buf[MAX_STRING_LENGTH];

    switch (info) {
    case OLC_R_EXTRAS:
	if (ch->in_room->extra_descr != NULL) {
	    send_to_char("INFO: current room extended descriptions...", ch);
	    for (extras = ch->in_room->extra_descr ;
		 extras ; extras = extras->next) {
		snprintf(buf, sizeof(buf), "Keyword(s): '%s'\n\r", extras->keyword);
		send_to_char( buf, ch );
		send_to_char( "Description:\n\r", ch);
		send_to_char( extras->description, ch);
	    }
	} else
	    send_to_char( "INFO: room has no extra descriptions.\n\r", ch);

	return;
	break;
    default:
	snprintf(buf, sizeof(buf), "key: %d", info );
	bug("olc_display_room_vars: unrecognised key reference!");
	bug(buf);
	return;
    }
}

