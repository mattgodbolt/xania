/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 1995-2000 Xania Development Team                                   */
/*  See the header to file: merc.h for original code copyrights         */
/*                                                                      */
/*  phil.c:  special functions for Phil the meerkat						*/
/*																		*/
/************************************************************************/

#if defined(macintosh)
#include <types.h>
#else
#if defined(riscos)
#include "sys/types.h"
#else
#include <sys/types.h>
#endif
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"

/* command procedures needed */
DECLARE_DO_FUN(do_yell		);
DECLARE_DO_FUN(do_open		);
DECLARE_DO_FUN(do_close		);
DECLARE_DO_FUN(do_say		);
DECLARE_DO_FUN(do_sleep		);
DECLARE_DO_FUN(do_wake		);
DECLARE_DO_FUN(do_socials	);



/* Note that Death's interest is less, since Phil is unlikely to like someone who keeps slaying him for pleasure ;-) */
/* Note also the cludgy hack so that Phil doesn't become interested in himself and ignore other people around him who */
/* don't have a special interest. */
#define PEOPLEONLIST	6
char	*nameList[]={
	"Forrey",		"Faramir",		"TheMoog",		"Death",
	"Luxor",		"Phil"
};
int		interestList[]={
	1000,			900,			900,			850,
	900,			0
};



/* Internal data for calculating what to do at any particular point in time. */
int		sleepiness	= 500;
int		boredness	= 500;

#define SLEEP_AT			1000
#define YAWN_AT				900
#define	STIR_AT				100
#define WAKE_AT				000
#define SLEEP_PT_ASLEEP		10
#define SLEEP_PT_AWAKE		3



char *randomSocial()
{
	switch ( number_range(1,6) )
	{
	case 1:
		return "gack";
	case 2:
		return "gibber";
	case 3:
		return "thumb";
	case 4:
		return "smeghead";
	case 5:
		return "nuzzle";
	case 6:
		return "howl";
	}

	bug( "in randomSocial() in phil.c : number_range outside of bounds!", 0);
	return NULL;
}



/* do the right thing depending on the current state of sleepiness */
/* returns TRUE if something happened, otherwise FALSE if everything's boring */
bool doSleepActions( CHAR_DATA *ch, ROOM_INDEX_DATA *home )
{
	int					sleepFactor=sleepiness;
	int					random;

	if ( ch->position == POS_SLEEPING ) {
		sleepiness -= SLEEP_PT_ASLEEP;
		if ( sleepFactor<WAKE_AT ) {
			do_wake( ch, "\0" );
			return TRUE;
		}
		if ( sleepFactor<STIR_AT ) {
			random = number_percent();
			if ( random > 97 ) {
				act( "$n stirs in $s sleep.", ch, NULL, NULL, TO_ROOM );
				return TRUE;
			}
			if ( random > 94 ) {
				act( "$n rolls over.", ch, NULL, NULL, TO_ROOM );
				return TRUE;
			}
			if ( random > 91 ) {
				act( "$n sniffles and scratches $s nose.", ch, NULL, NULL, TO_ROOM );
				return TRUE;
			}
		}
		return FALSE;
	}
	sleepiness += SLEEP_PT_AWAKE;
	random = number_percent();
	if ( sleepiness>SLEEP_AT ) {
		if ( ch->in_room == home ) {
			do_sleep( ch, 0 );
		} else {
			act( "$n tiredly waves $s hands in a complicated pattern and is gone.",
					ch, NULL, NULL, TO_ROOM );
			act( "You transport yourself back home.", ch, NULL, NULL, TO_CHAR );
			char_from_room( ch );
			char_to_room( ch, home );
			act( "$n appears in a confused whirl of mist.", ch, NULL, NULL, TO_ROOM );
		}
		return TRUE;
	}
	if ( sleepFactor>YAWN_AT ) {
		if ( random > 97 ) {
			check_social( ch, "yawn", "\0" );
			return TRUE;
		}
		if ( random > 94 ) {
			check_social( ch, "stretch", "\0" );
			return TRUE;
		}
	}

	return FALSE;
}




/* does a random social on a randomly selected person in the current room */
void doRandomSocial( CHAR_DATA *ch, ROOM_INDEX_DATA *home )
{
	int			charsInRoom = 0;
	int			charSelected;
	CHAR_DATA	*firstChar;
	CHAR_DATA	*countChar;

	firstChar = ch->in_room->people;
	for ( countChar=firstChar ; countChar ; countChar=countChar->next_in_room )
		charsInRoom++;
	charSelected = (number_percent()*charsInRoom)/100;
	if ( charSelected>charsInRoom )
		charSelected=charsInRoom;
	for ( countChar=firstChar ; charSelected-- ; countChar=countChar->next_in_room );
	if ( countChar ) {
		check_social( ch, randomSocial(), countChar->name );
	}
}



/* Find the amount of interest Phil will show in the given character, by looking up the */
/* character's name and its associated interest number on the table */
/* Defaults:  ch=NULL: 0  ch=<unknown>: 1 */
/* Unknown characters are marginally more interesting than nothing */
int charInterest( CHAR_DATA *ch )
{
	int				listOffset=0;

	if ( ch==NULL )
		return 0;

	for (  ; listOffset<PEOPLEONLIST ; listOffset++ ) {
		if ( is_name( nameList[listOffset], ch->name ) == TRUE )
			return interestList[listOffset];
	}

	return 1;
}



/* Check if there's a more interesting char in this room than has been found before */
bool findInterestingChar( ROOM_INDEX_DATA *room, CHAR_DATA **follow, int *interest )
{
	CHAR_DATA			*current;
	bool				retVal=FALSE;
	int					currentInterest;

	for ( current = room->people ; current ;
			current = current->next_in_room )
	{
		currentInterest = charInterest(current);
		if ( currentInterest > *interest ) {
			*follow = current;
			*interest = currentInterest;
			retVal = TRUE;
		}
	}

	return retVal;
}



/* Special program for 'Phil' - Forrey's meerkat 'pet'. */
/* Note that this function will be called once every 4 seconds. */
bool spec_phil( CHAR_DATA *ch )
{
	ROOM_INDEX_DATA		*home;
	ROOM_INDEX_DATA		*room;
	CHAR_DATA			*follow=NULL;
	sh_int				exit=0;
	sh_int				takeExit=0;
	EXIT_DATA			*exitData;
	int					interest=0;

/* Check fighting state */
	if ( ch->position == POS_FIGHTING )
		return FALSE;

/* Check sleep state */
	if ( (home = get_room_index( ROOM_VNUM_FORREYSPLACE )) == NULL ) {
		bug( "Couldn't get Forrey's home index.", 0 );
		return FALSE;
	}

/* Check general awakeness state */
/* Return if something was done in the sleepactions routine */
	if ( doSleepActions( ch, home ) )
		return TRUE;

/* If Phil is asleep, just end it there */
	if ( ch->position == POS_SLEEPING )
		return FALSE;

/* Check for known people in this, and neighbouring, rooms */
	room = ch->in_room;
	findInterestingChar( room, &follow, &interest );
	for (  ; exit<6 ; exit++ ) {
		if ( (exitData = room->exit[exit]) != NULL )
			if ( findInterestingChar( exitData->u1.to_room, &follow, &interest ) )
				takeExit = exit;
	}
	if ( follow != NULL && (follow->in_room != room) && (ch->position==POS_STANDING) ) {
		move_char( ch, takeExit, FALSE );
	}

/* Do a random social on someone in the room */
	if ( number_percent() >= 99 ) {
		doRandomSocial( ch, home );
		return TRUE;
	}

	return FALSE;
}
