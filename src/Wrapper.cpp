#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include "merc.h"

/////////////////////////////////////////////////

/////////////////////////////////////////////////
class Mob
{
private:
	CHAR_DATA		*theChar;	// Wrappered character
public:
	Mob() { theChar = NULL; }
	virtual ~Mob();

	Mob(CHAR_DATA *cData);

	// Acquiring a mob from various places, returns argument as one_argument used to
	char *FromRoom (Mob &whosLooking, char *argument);
	char *FromWorld (Mob &whosLooking, char *argument);

	// Checking the validity of a mob
	inline bool Valid() const 
	{ return (theChar != NULL); }

	// Sending messages to a mobile
	Mob &operator << (const char *string);

	// Casts to allow other functions to work with the Mob as they did CHAR_DATA*
	operator CHAR_DATA* () 
	{ assert (theChar); return theChar; }

	// Comparisons
	inline bool operator == (const Mob &other) 
	{ return other.theChar == theChar; }
	inline bool operator != (const Mob &other) 
	{ return other.theChar != theChar; }
};

Mob::Mob(CHAR_DATA *cData)
{
	theChar = cData;
}

Mob::~Mob()
{
}

Mob &Mob::operator << (const char *string)
{
	if (theChar)
		send_to_char (string, theChar);

	return (*this);
}

char *Mob::FromRoom (Mob &whosRoom, char *argument)
{
	char buf[MAX_INPUT_LENGTH];
	char *retVal;

	retVal = one_argument (argument, buf);

	theChar = get_char_room (whosRoom, buf);
	return retVal;
}

char *Mob::FromWorld (Mob &whosRoom, char *argument)
{
	char buf[MAX_INPUT_LENGTH];
	char *retVal;

	retVal = one_argument (argument, buf);

	theChar = get_char_world (whosRoom, buf);
	return retVal;
}


/////////////////////////////////////////////////
// test code
/////////////////////////////////////////////////

extern "C" void do_cpptest( CHAR_DATA *ch, char *argument)
{
	Mob user(ch);

	Mob mobInRoom;
	argument = mobInRoom.FromRoom(user, argument);

	if (mobInRoom.Valid() && mobInRoom != user)
		user << "It worked!\n";
	else
		user << "They're not here.\n";

}

