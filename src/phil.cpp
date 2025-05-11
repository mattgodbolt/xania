/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 1995-2000 Xania Development Team                                */
/*  See the header to file: merc.h for original code copyrights         */
/*                                                                      */
/*  phil.c:  special functions for Phil the meerkat                     */
/*                                                                      */
/************************************************************************/

#include "Act.hpp"
#include "Char.hpp"
#include "Exit.hpp"
#include "Interpreter.hpp"
#include "Logging.hpp"
#include "Room.hpp"
#include "VnumRooms.hpp"
#include "act_move.hpp"
#include "db.h"
#include "handler.hpp"
#include "string_utils.hpp"

#include <range/v3/iterator/operations.hpp>
#include <tuple>

namespace {
/* Note that Death's interest is less, since Phil is unlikely to like someone who keeps slaying him for pleasure ;-) */
/* Note also the cludgy hack so that Phil doesn't become interested in himself and ignore other people around him who */
/* don't have a special interest. */
const std::array<std::tuple<std::string_view, int>, 6> interestingChars = {
    {std::make_tuple("Forrey", 1000), std::make_tuple("Faramir", 900), std::make_tuple("TheMoog", 900),
     std::make_tuple("Death", 850), std::make_tuple("Luxor", 900), std::make_tuple("Phil", 0)}};

/* Internal data for calculating what to do at any particular point in time. */
int sleepiness = 500;

constexpr auto SleepAt = 1000;
constexpr auto YawnAt = 900;
constexpr auto StirAt = 100;
constexpr auto WakeAt = 0;
constexpr auto SleepPtAsleep = 10;
constexpr auto SleepPtAwake = 3;

std::string_view randomSocial() {
    switch (number_range(1, 6)) {
    case 1: return "gack";
    case 2: return "gibber";
    case 3: return "thumb";
    case 4: return "smeghead";
    case 5: return "nuzzle";
    case 6: return "howl";
    }
    return "";
}

/* do the right thing depending on the current state of sleepiness */
/* returns true if something happened, otherwise false if everything's boring */
bool doSleepActions(Char *ch, Room *home) {
    int sleepFactor = sleepiness;
    int random;

    if (ch->is_pos_sleeping()) {
        sleepiness -= SleepPtAsleep;
        if (sleepFactor < WakeAt) {
            do_wake(ch, ArgParser(""));
            return true;
        }
        if (sleepFactor < StirAt) {
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
    sleepiness += SleepPtAwake;
    random = number_percent();
    if (sleepiness > SleepAt) {
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
    auto &interpreter = ch->mud_.interpreter();
    if (sleepFactor > YawnAt) {
        if (random > 97) {
            interpreter.interpret(ch, "yawn");
            return true;
        }
        if (random > 94) {
            interpreter.interpret(ch, "stretch");
            return true;
        }
    }

    return false;
}

/* does a random social on a randomly selected person in the current room */
void doRandomSocial(Char *ch) {
    auto charsInRoom = ranges::distance(ch->in_room->people);
    if (!charsInRoom)
        return;
    auto charSelected = (number_percent() * charsInRoom) / 100;
    if (charSelected >= charsInRoom)
        charSelected = charsInRoom - 1;
    auto *countChar = *std::next(ch->in_room->people.begin(), charSelected);
    auto &interpreter = ch->mud_.interpreter();
    auto social = fmt::format("{} {}", randomSocial(), countChar->name);
    interpreter.interpret(ch, social);
}

/* Find the amount of interest Phil will show in the given character, by looking up the */
/* character's name and its associated interest number on the table */
/* Defaults:  ch=nullptr: 0  ch=<unknown>: 1 */
/* Unknown characters are marginally more interesting than nothing */
int charInterest(Char *ch) {
    if (ch == nullptr)
        return 0;
    for (const auto &interestingChar : interestingChars) {
        if (is_name(std::get<0>(interestingChar), ch->name))
            return std::get<1>(interestingChar);
    }
    return 1;
}

/* Check if there's a more interesting char in this room than has been found before */
bool findInterestingChar(Room *room, Char **follow, int *interest) {
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

} // namespace

/* Special program for 'Phil' - Forrey's meerkat 'pet'. */
/* Note that this function will be called once every 4 seconds. */
bool spec_phil(Char *ch) {
    Room *home;
    Room *room;
    Char *follow = nullptr;
    Direction takeExit = Direction::North;
    int interest = 0;

    /* Check fighting state */
    if (ch->is_pos_fighting())
        return false;
    const Logger &logger = ch->mud_.logger();
    /* Check sleep state */
    if ((home = get_room(Rooms::ForreyHome, logger)) == nullptr) {
        ch->mud_.logger().bug("Couldn't get Forrey's home index.");
        return false;
    }

    /* Check general awakeness state */
    /* Return if something was done in the sleepactions routine */
    if (doSleepActions(ch, home))
        return true;

    /* If Phil is asleep, just end it there */
    if (ch->is_pos_sleeping())
        return false;

    /* Check for known people in this, and neighbouring, rooms */
    room = ch->in_room;
    if (!room) {
        return false;
    }
    findInterestingChar(room, &follow, &interest);
    for (auto direction : all_directions) {
        if (const auto &exit = room->exits[direction])
            if (findInterestingChar(exit->u1.to_room, &follow, &interest))
                takeExit = direction;
    }
    if (follow != nullptr && (follow->in_room != room) && (ch->is_pos_standing())) {
        move_char(ch, takeExit);
    }

    /* Do a random social on someone in the room */
    if (number_percent() >= 93) {
        doRandomSocial(ch);
        return true;
    }

    return false;
}
