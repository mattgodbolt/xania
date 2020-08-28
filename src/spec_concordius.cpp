/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2020 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/

#include "CHAR_DATA.hpp"
#include "comm.hpp"
#include "merc.h"
#include "string_utils.hpp"
#include <fmt/format.h>
#include <map>
#include <string_view>
#include <vector>

using namespace fmt::literals;
using namespace std::literals;

namespace {

struct CombatEmote {
    std::string_view to_vict;
    std::string_view to_not_vict;
};

/**
 * Maps the threshold minimum % of hitpoints that he has to get to in order to start
 * performing the emotes. A random emote will be chosen. He will stop doing the
 * emote once he gets less than 10% below that threshhold.
 */
static inline const std::multimap<int, CombatEmote> conc_combat_emotes{
    {2, {"$n says 'It seems you have bested me, traveller.'"sv, "$n says 'It seems you have bested me, traveller.'"sv}},
    {2, {"$n glances around, looking for an escape route!"sv, "$n glances around, looking for an escape route!"sv}},
    {10,
     {"$n spins his shield furiously, deflecting your attacks."sv,
      "$n spins $s shield furiously, deflecting $N's attacks."sv}},
    {10,
     {"$n says 'Perhaps we can settle this some other way?'"sv,
      "$n says 'Perhaps we can settle this some other way?'"sv}},
    {20, {"$n says 'It seems I have underestimated you.'"sv, "$n says 'It seems I have underestimated you.'"sv}},
    {20, {"$n says 'Is that all you've got, outsider?'"sv, "$n says 'Is that all you've got, outsider?"sv}},
    {20,
     {"$n slams into you with a brutal shoulder charge!"sv, "$n knocks $N sideways with a brutal shoulder charge!"sv}},
    {30, {"$n sneers at you in contempt."sv, "$n sneers at $N in contempt."sv}},
    {30, {"$n says 'Is that all you've got, impostor?'"sv, "$n says 'Is that all you've got, impostor?"sv}},
    {30,
     {"$n slaps you disdainfully with the back of $s hand."sv, "$n slaps $N disdainfully with the back of $s hand."sv}},
    {40,
     {
         "$n retreats behind $s shield."sv,
         "$n retreats behind $s shield."sv,
     }},
    {40, {"$n inches backward nervously."sv, "$n inches backward nervously."sv}},
    {40, {"$n thrusts $n golden claw at your jugular vein!"sv, "$n thrusts $s golden claw at $N's neck!"sv}},
    {50,
     {
         "$n lashes out with a vicious jab."sv,
         "$n lashes out with a vicious jab."sv,
     }},
    {50, {"$n attempts to wrestle you to the ground!"sv, "$n tries to wrestle $N to the ground!"sv}},
    {50, {"$n drops to the ground and attempts a leg sweep!"sv, "$n drops to the ground and attempts a leg sweep!"sv}},
    {60, {"$n's movements seem to quicken."sv, "$n's movements seem to quicken."sv}},
    {60,
     {"$n grabs a fistful of dirt at tosses it in your eyes!"sv, "$n grabs a fistful of dirt and tosses it at $N!"sv}},
    {60, {"$n backflips casually out of your reach.", "$n backflips casually out of $N's reach."}},
    {70, {"$n launches into a spinning heel kick!"sv, "$n lashes out at $N with the ball of his foot!"sv}},
    {70,
     {"$n lunges, thrusting the point of his shield at your head!"sv,
      "$n thrusts the point of his shield at $N's face!"sv}},
    {70,
     {"$n begins to twist like an Iscarian sword-dancer."sv, "$n begins to twist like an Iscarian sword-dancer."sv}},
    {80, {"$n dazzles you with a spinning lotus attack!"sv, "$N is dazzled by $n's spinning lotus attack!"sv}},
    {80, {"$n cartwheels away from your advances."sv, "$n's cartwheels away from $N's advances."sv}},
    {80,
     {"$n grabs hold of your head and claws at your eyes!"sv,
      "$n grabs hold of $N's head and tries to gouge their eyes!"sv}},
    {90, {"$n deflects your attacks effortlessly."sv, "$n deflects $N's blows with ease."sv}},
    {95, {"$n pounds you with a flurry of blows!"sv, "$n executes a flurry of attacks on $N!"sv}}};

struct PersonalEmote {
    std::string_view good_msg;
    std::string_view not_good_msg;
};

static inline const std::vector<PersonalEmote> conc_personal_emotes{
    {"bow", "bow"},       {"flip", "eye"},    {"salute", "salute"}, {"punch", "judge"},
    {"highfive", "peer"}, {"smile", "smile"}, {"cheer", "bonk"}};

static inline const std::array conc_general_emotes{
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

static inline const std::array aq_general_emotes{
    "$n flutters $s wings."sv, "$n makes a piercing screech!"sv, "$n preens $s feathers."sv,
    "$n dances back and forth on $s large talons."sv, "$n leaps into the air and arcs overhead."sv};

/**
 * Patrol the rooms around the temple.
 */
static inline constexpr std::array patrol_directions{"n", "s", "s", "w", "e", "e", "w",
                                                     "s", "e", "w", "w", "e", "n", "n"};
static inline uint patrol_index = 0;
static inline uint patrol_pause = 0;

void concordius_patrols(CHAR_DATA *ch) {
    uint random = number_range(0, 100);
    if (random > 90) {
        act(conc_general_emotes[random % conc_general_emotes.size()], ch, nullptr, nullptr, To::Room);
        return;
    }
    for (auto victim = ch->in_room->people; victim; victim = victim->next_in_room) {
        if (victim->is_npc() || !ch->can_see(*victim)) {
            continue;
        }
        uint random = number_range(0, 100);
        if (random < 80)
            continue;
        auto &pers_emote = conc_personal_emotes[random % conc_personal_emotes.size()];
        auto msg = "{} {}"_format(victim->is_good() ? pers_emote.good_msg : pers_emote.not_good_msg, victim->name);
        interpret(ch, msg.c_str());
        break;
    }
    // After socialising in the room, continue with patrol route.
    if (matches(ch->in_room->area->areaname, "Midgaard") && patrol_pause++ % 3 == 2) {
        interpret(ch, patrol_directions[patrol_index++ % patrol_directions.size()]);
    }
}

/**
 * Choose a random combat emote based on the proportion of damage he has taken.
 * The emotes at lower health get increasingly desperate...
 */
void combat_emote(CHAR_DATA *ch, const int percent_hit_lower) {
    auto it = conc_combat_emotes.lower_bound(percent_hit_lower);
    auto end = conc_combat_emotes.upper_bound(percent_hit_lower + 10);
    for (; it != end; ++it) {
        if (number_range(0, 8) == 0) {
            auto &emote = it->second;
            act(emote.to_vict, ch, nullptr, ch->fighting, To::Vict);
            act(emote.to_not_vict, ch, nullptr, ch->fighting, To::NotVict);
            break;
        }
    }
}

void concordius_fights(CHAR_DATA *ch) {
    auto percent_hit_lower = (ch->hit * 100) / ch->max_hit;
    combat_emote(ch, percent_hit_lower);
    if (percent_hit_lower < 10 && number_range(0, 5) == 0) {
        int sn = skill_lookup("heal");
        if (sn > 0) {
            act("|y$n begins to incant a shamanistic verse.|w", ch, nullptr, nullptr, To::Room);
            (*skill_table[sn].spell_fun)(sn, ch->level, ch, ch);
        }
    }
}

/**
 * Aquila routines
 */

void aquila_patrols(CHAR_DATA *ch) {
    if (!ch->master) {
        interpret(ch, "follow Concordius");
    }
    uint random = number_range(0, 100);
    if (random > 90) {
        act(aq_general_emotes[random % aq_general_emotes.size()], ch, nullptr, nullptr, To::Room);
        return;
    }
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
        concordius_fights(ch);
    } else {
        concordius_patrols(ch);
    }
    return true;
}

/**
 * Special program for Concordius' pet eagle Aquila.
 * She follows him around and makes a fuss.
 */
bool spec_aquila_pet(CHAR_DATA *ch) {
    if (ch->position < POS_FIGHTING) {
        return false;
    }
    aquila_patrols(ch);
    return true;
}