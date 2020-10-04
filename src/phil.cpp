/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 1995-2000 Xania Development Team                                */
/*  See the header to file: merc.h for original code copyrights         */
/*                                                                      */
/*  phil.c:  special functions for Phil the meerkat                     */
/*                                                                      */
/************************************************************************/

#include "Logging.hpp"
#include "comm.hpp"
#include "interp.h"
#include "merc.h"
#include "string_utils.hpp"

#include <range/v3/iterator/operations.hpp>

/* Note that Death's interest is less, since Phil is unlikely to like someone who keeps slaying him for pleasure ;-) */
/* Note also the cludgy hack so that Phil doesn't become interested in himself and ignore other people around him who */
/* don't have a special interest. */
const char *nameList[] = {"Forrey", "Faramir", "TheMoog", "Death", "Luxor", "Phil"};
const int interestList[] = {1000, 900, 900, 850, 900, 0};
#define PEOPLEONLIST int(sizeof(nameList) / sizeof(nameList[0]))

/* Internal data for calculating what to do at any particular point in time. */
int sleepiness = 500;
int boredness = 500;

#define SLEEP_AT 1000
#define YAWN_AT 900
#define STIR_AT 100
#define WAKE_AT 000
#define SLEEP_PT_ASLEEP 10
#define SLEEP_PT_AWAKE 3

const char *randomSocial() {
    switch (number_range(1, 6)) {
    case 1: return "gack";
    case 2: return "gibber";
    case 3: return "thumb";
    case 4: return "smeghead";
    case 5: return "nuzzle";
    case 6: return "howl";
    }

    bug("in randomSocial() in phil.c : number_range outside of bounds!");
    return nullptr;
}

/* do the right thing depending on the current state of sleepiness */
/* returns true if something happened, otherwise false if everything's boring */
bool doSleepActions(Char *ch, ROOM_INDEX_DATA *home) {
    int sleepFactor = sleepiness;
    int random;

    if (ch->position == POS_SLEEPING) {
        sleepiness -= SLEEP_PT_ASLEEP;
        if (sleepFactor < WAKE_AT) {
            do_wake(ch, ArgParser(""));
            return true;
        }
        if (sleepFactor < STIR_AT) {
            random = number_percent();
            if (random > 97) {
                act("$n stirs in $s sleep.", ch);
                return true;
            }
            if (random > 94) {
                act("$n rolls over.", ch);
                return true;
            }
            if (random > 91) {
                act("$n sniffles and scratches $s nose.", ch);
                return true;
            }
        }
        return false;
    }
    sleepiness += SLEEP_PT_AWAKE;
    random = number_percent();
    if (sleepiness > SLEEP_AT) {
        if (ch->in_room == home) {
            do_sleep(ch);
        } else {
            act("$n tiredly waves $s hands in a complicated pattern and is gone.", ch);
            act("You transport yourself back home.", ch, nullptr, nullptr, To::Char);
            char_from_room(ch);
            char_to_room(ch, home);
            act("$n appears in a confused whirl of mist.", ch);
        }
        return true;
    }
    if (sleepFactor > YAWN_AT) {
        if (random > 97) {
            check_social(ch, "yawn", "");
            return true;
        }
        if (random > 94) {
            check_social(ch, "stretch", "");
            return true;
        }
    }

    return false;
}

/* does a random social on a randomly selected person in the current room */
void doRandomSocial(Char *ch, ROOM_INDEX_DATA *home) {
    (void)home;

    auto charsInRoom = ranges::distance(ch->in_room->people);
    if (!charsInRoom)
        return;
    auto charSelected = (number_percent() * charsInRoom) / 100;
    if (charSelected >= charsInRoom)
        charSelected = charsInRoom - 1;
    auto *countChar = *std::next(ch->in_room->people.begin(), charSelected);
    check_social(ch, randomSocial(), countChar->name);
}

/* Find the amount of interest Phil will show in the given character, by looking up the */
/* character's name and its associated interest number on the table */
/* Defaults:  ch=nullptr: 0  ch=<unknown>: 1 */
/* Unknown characters are marginally more interesting than nothing */
int charInterest(Char *ch) {
    int listOffset = 0;

    if (ch == nullptr)
        return 0;

    for (; listOffset < PEOPLEONLIST; listOffset++) {
        if (is_name(nameList[listOffset], ch->name) == true)
            return interestList[listOffset];
    }

    return 1;
}

/* Check if there's a more interesting char in this room than has been found before */
bool findInterestingChar(ROOM_INDEX_DATA *room, Char **follow, int *interest) {
    bool retVal = false;
    int currentInterest;

    for (auto *current : room->people) {
        currentInterest = charInterest(current);
        if (currentInterest > *interest) {
            *follow = current;
            *interest = currentInterest;
            retVal = true;
        }
    }

    return retVal;
}

/* Special program for 'Phil' - Forrey's meerkat 'pet'. */
/* Note that this function will be called once every 4 seconds. */
bool spec_phil(Char *ch) {
    ROOM_INDEX_DATA *home;
    ROOM_INDEX_DATA *room;
    Char *follow = nullptr;
    Direction takeExit = Direction::North;
    EXIT_DATA *exitData;
    int interest = 0;

    /* Check fighting state */
    if (ch->position == POS_FIGHTING)
        return false;

    /* Check sleep state */
    if ((home = get_room_index(ROOM_VNUM_FORREYSPLACE)) == nullptr) {
        bug("Couldn't get Forrey's home index.");
        return false;
    }

    /* Check general awakeness state */
    /* Return if something was done in the sleepactions routine */
    if (doSleepActions(ch, home))
        return true;

    /* If Phil is asleep, just end it there */
    if (ch->position == POS_SLEEPING)
        return false;

    /* Check for known people in this, and neighbouring, rooms */
    room = ch->in_room;
    findInterestingChar(room, &follow, &interest);
    for (auto exit : all_directions) {
        if ((exitData = room->exit[exit]) != nullptr)
            if (findInterestingChar(exitData->u1.to_room, &follow, &interest))
                takeExit = exit;
    }
    if (follow != nullptr && (follow->in_room != room) && (ch->position == POS_STANDING)) {
        move_char(ch, takeExit);
    }

    /* Do a random social on someone in the room */
    if (number_percent() >= 99) {
        doRandomSocial(ch, home);
        return true;
    }

    return false;
}
