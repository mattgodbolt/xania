/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2020 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/

#include "CHAR_DATA.hpp"
#include "DescriptorFilter.hpp"
#include "DescriptorList.hpp"
#include "comm.hpp"
#include "merc.h"
#include "string_utils.hpp"
#include <fmt/format.h>
#include <string_view>
#include <vector>

using namespace fmt::literals;
using namespace std::literals;

namespace {

struct PersonalEmote {
    std::string_view good_msg;
    std::string_view not_good_msg;
};

inline static const std::vector<PersonalEmote> personal_emotes{
    {"bow", "bow"},       {"flip", "eye"},    {"salute", "salute"}, {"punch", "judge"},
    {"highfive", "peer"}, {"smile", "smile"}, {"cheer", "bonk"}};

inline static const std::array general_emotes{
    "$n polishes $s golden claw."sv,
    "$n looks skyward and closes $s eyes in thought."sv,
    "$n raises $s hand and begins to whistle birdsong."sv,
    "$n says 'I used to be an adventurer like you. But then Aquila talked some sense into me.'"sv,
    "$n sits down and begins to sharpen the edge of his shield against his heel."sv,
    "$n says 'Beware the denizens of Yggdrasil to the west!'"sv,
    "$n tightens $s toga."sv,
    "$n says 'How I long to visit Ultima once again..."sv,
    "$n says 'I'm told it's never sunny in Underdark. I don't like the sound of that.'"sv,
    "$n says 'Did you know there are over sixty different species of eagle?!'"sv};

bool concordius_stands_guard(CHAR_DATA *ch) {

    uint random = number_range(0, 100);
    if (random > 90) {
        act(general_emotes[random % general_emotes.size()], ch, nullptr, nullptr, To::Room);
        return true;
    }
    bool found_someone = false;
    for (auto victim = ch->in_room->people; victim; victim = victim->next_in_room) {
        if (victim->is_npc() || !ch->can_see(*victim)) {
            continue;
        }
        uint random = number_range(0, 100);
        if (random < 80)
            continue;
        found_someone = true;
        auto &pers_emote = personal_emotes[random % personal_emotes.size()];
        auto msg = "{} {}"_format(victim->is_good() ? pers_emote.good_msg : pers_emote.not_good_msg, victim->name);
        interpret(ch, msg.c_str());
        break;
    }
    return found_someone;
}

bool concordius_fights(CHAR_DATA *ch) {
    (void)ch; // TODO
    return true;
}

}

/**
 * Special program for Concordius.
 * Note that this function will be called once every 4 seconds.
 */
bool spec_concordius(CHAR_DATA *ch) {

    if (ch->position < POS_FIGHTING)
        return false;
    if (ch->position == POS_FIGHTING) {
        return concordius_fights(ch);
    } else {
        return concordius_stands_guard(ch);
    }
}
