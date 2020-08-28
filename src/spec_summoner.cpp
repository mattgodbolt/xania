/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2020 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/

#include "CHAR_DATA.hpp"
#include "TimeInfoData.hpp"
#include "comm.hpp"
#include "merc.h"
#include "string_utils.hpp"
#include <fmt/format.h>

using namespace std::chrono;
using namespace std::literals;
using namespace fmt::literals;

namespace {

static inline constexpr auto object_wrong_type{"Sorry, this item cannot be used to summon your corpse."sv};
static inline constexpr auto object_too_weak{"Sorry, this item is not powerful enough to summon your corpse."sv};
static inline constexpr auto ShardLevelRange{9};

time_t last_advice_time{0};

void summoner_awaits(CHAR_DATA *ch) {
    auto current = system_clock::to_time_t(current_time);
    if (current - last_advice_time > 30) {
        last_advice_time = current;
        interpret(ch, "say If you wish to summon your corpse, purchase a shard that is powerful enough for your level "
                      "and give it to me. Please read the sign for more details.");
    }
}

bool check_summoner_preconditions(CHAR_DATA *player, CHAR_DATA *summoner) {
    if (player->is_npc() || summoner->is_pc() || summoner->spec_fun != spec_lookup("spec_summoner")) {
        return false;
    } else
        return true;
}

std::optional<std::string_view> is_catalyst_invalid(CHAR_DATA *player, OBJ_DATA *catalyst) {
    if (!IS_SET(catalyst->extra_flags, ITEM_SUMMON_CORPSE)) {
        return object_wrong_type;
    } else if (catalyst->level + ShardLevelRange < player->level) {
        return object_too_weak;
    } else
        return {};
}

bool check_catalyst(CHAR_DATA *player, CHAR_DATA *summoner, OBJ_DATA *catalyst) {
    if (auto reason = is_catalyst_invalid(player, catalyst)) {
        act("|C$n tells you '$t|C'|w", summoner, *reason, player, To::Vict, POS_DEAD);
        obj_from_char(catalyst);
        obj_to_char(catalyst, player);
        act("$n gives $p to $N.", summoner, catalyst, player, To::NotVict);
        act("$n gives you $p.", summoner, catalyst, player, To::Vict);
        return false;
    } else
        return true;
}

/*
 * Find a player corpse in the world.
 * Similar to get_obj_world(), it ignores corpses in ch's current room as well as object visibility and it
 * matches on short description because corpse names (keywords) don't include the
 * corpse owner's name.
 */
std::optional<OBJ_DATA *> get_pc_corpse_world(CHAR_DATA *ch, std::string_view corpse_short_descr) {
    for (auto obj = object_list; obj; obj = obj->next) {
        if (obj->item_type == ITEM_CORPSE_PC && obj->in_room && obj->in_room != ch->in_room) {
            if (matches(corpse_short_descr, obj->short_descr))
                return obj;
        }
    }

    return {};
}

/**
 * Temporarily weaken the person who requested the summoning.
 */
void apply_summoning_fatigue(CHAR_DATA *player) {
    AFFECT_DATA af;
    af.type = skill_lookup("weaken");
    af.level = 5;
    af.duration = 2;
    af.location = APPLY_STR;
    af.modifier = -3;
    af.bitvector = AFF_WEAKEN;
    affect_to_char(player, &af);
    send_to_char("You are stunned and fall the ground.\n\r", player);
    act("$n is knocked off $s feet!", player);
    player->position = POS_RESTING;
}

/**
 * Find a player's corpse and teleport it into the room. Consumes the catalyst used to trigger.b
 */
void summon_corpse(CHAR_DATA *player, CHAR_DATA *summoner, OBJ_DATA *catalyst) {
    act("$n clutches $p between his bony fingers and begins to whisper.", summoner, catalyst, nullptr, To::Room);
    act("The runes on the summoning stone begin to glow more brightly!", summoner, catalyst, nullptr, To::Room);
    std::string corpse_name = "corpse of {}"_format(player->name);
    if (auto corpse = get_pc_corpse_world(summoner, corpse_name)) {
        obj_from_room(*corpse);
        obj_to_room(*corpse, summoner->in_room);
        act("|BThere is a flash of light and a corpse materialises on the ground before you!|w", summoner, nullptr,
            nullptr, To::Room);
        apply_summoning_fatigue(player);
    } else {
        act("The runes dim and the summoner tips his head in shame.", summoner, nullptr, nullptr, To::Room);
    }
    extract_obj(catalyst);
}

}

/**
 * Special program for corpse summoner NPCs. Currently just one of
 * these in the Necropolis, but there could be more in future.
 */
bool spec_summoner(CHAR_DATA *ch) {
    if (ch->position < POS_STANDING || ch->in_room->vnum != ROOM_VNUM_NECROPOLIS)
        return false;
    summoner_awaits(ch);
    return true;
}

void handle_corpse_summoner(CHAR_DATA *player, CHAR_DATA *summoner, OBJ_DATA *catalyst) {
    if (!check_summoner_preconditions(player, summoner)) {
        return;
    }
    if (!check_catalyst(player, summoner, catalyst)) {
        return;
    }
    summon_corpse(player, summoner, catalyst);
}
