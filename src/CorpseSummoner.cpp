/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2020 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/

#include "CorpseSummoner.hpp"
#include "AFFECT_DATA.hpp"
#include "Char.hpp"
#include "TimeInfoData.hpp"
#include "merc.h"
#include "string_utils.hpp"
#include <fmt/format.h>

using namespace std::chrono;
using namespace std::literals;
using namespace fmt::literals;

CorpseSummoner::CorpseSummoner(Dependencies &dependencies) : mud_{dependencies}, last_advice_time_{0} {}

time_t CorpseSummoner::last_advice_time() { return last_advice_time_; }

void CorpseSummoner::summoner_awaits(Char *ch, const time_t time_secs) {
    if (time_secs - last_advice_time_ > 30) {
        last_advice_time_ = time_secs;
        mud_.interpret(ch,
                       "say If you wish to summon your corpse, purchase a shard that is powerful enough for your level "
                       "and give it to me. Please read the sign for more details.");
    }
}

void CorpseSummoner::summon_corpse(Char *player, Char *summoner, OBJ_DATA *catalyst) {
    mud_.act("$n clutches $p between $s bony fingers and begins to whisper.", summoner, catalyst, nullptr, To::Room);
    mud_.act("The runes on the summoning stone begin to glow more brightly!", summoner, catalyst, nullptr, To::Room);
    std::string corpse_name = "corpse of {}"_format(player->name);
    if (auto corpse = get_pc_corpse_world(summoner, corpse_name)) {
        mud_.obj_from_room(*corpse);
        mud_.obj_to_room(*corpse, summoner->in_room);
        mud_.act("|BThere is a flash of light and a corpse materialises on the ground before you!|w", summoner, nullptr,
                 nullptr, To::Room);
        apply_summoning_fatigue(player);
    } else {
        mud_.act("The runes dim and the summoner tips $s head in shame.", summoner, nullptr, nullptr, To::Room);
    }
    mud_.extract_obj(catalyst);
}

bool CorpseSummoner::check_summoner_preconditions(Char *player, Char *summoner) {
    if (player->is_npc() || summoner->is_pc() || summoner->spec_fun != mud_.spec_fun_summoner()) {
        return false;
    } else
        return true;
}

std::optional<std::string_view> CorpseSummoner::is_catalyst_invalid(Char *player, OBJ_DATA *catalyst) {
    if (!IS_SET(catalyst->extra_flags, ITEM_SUMMON_CORPSE)) {
        return object_wrong_type;
    } else if (catalyst->level + ShardLevelRange < player->level) {
        return object_too_weak;
    } else
        return {};
}

bool CorpseSummoner::check_catalyst(Char *player, Char *summoner, OBJ_DATA *catalyst) {
    if (auto reason = is_catalyst_invalid(player, catalyst)) {
        mud_.act("|C$n tells you '$t|C'|w", summoner, *reason, player, To::Vict, POS_DEAD);
        mud_.obj_from_char(catalyst);
        mud_.obj_to_char(catalyst, player);
        mud_.act("$n gives $p to $N.", summoner, catalyst, player, To::NotVict);
        mud_.act("$n gives you $p.", summoner, catalyst, player, To::Vict);
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
std::optional<OBJ_DATA *> CorpseSummoner::get_pc_corpse_world(Char *ch, std::string_view corpse_short_descr) {
    for (auto obj = mud_.object_list(); obj; obj = obj->next) {
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
void CorpseSummoner::apply_summoning_fatigue(Char *player) {
    AFFECT_DATA af;
    af.type = mud_.weaken_sn();
    af.level = 5;
    af.duration = 2;
    af.location = AffectLocation::Str;
    af.modifier = -3;
    af.bitvector = AFF_WEAKEN;
    mud_.affect_to_char(player, af);
    player->send_to("You are stunned and fall the ground.\n\r");
    mud_.act("$n is knocked off $s feet!", player, nullptr, nullptr, To::Room);
    player->position = POS_RESTING;
}

namespace {

/**
 * The default implementation of CorpseSummoner's dependencies is intentionally
 * hidden in this file and anon namespace.
 */
class DependenciesImpl : public CorpseSummoner::Dependencies {
public:
    DependenciesImpl();
    void interpret(Char *ch, std::string msg);
    void act(std::string_view msg, const Char *ch, Act1Arg arg1, Act2Arg arg2, To to, int position);
    void act(std::string_view msg, const Char *ch, Act1Arg arg1, Act2Arg arg2, To to);
    void obj_from_char(OBJ_DATA *obj);
    void obj_to_char(OBJ_DATA *obj, Char *ch);
    void obj_from_room(OBJ_DATA *obj);
    void obj_to_room(OBJ_DATA *obj, ROOM_INDEX_DATA *room);
    void extract_obj(OBJ_DATA *obj);
    void affect_to_char(Char *ch, const AFFECT_DATA &af);
    OBJ_DATA *object_list();
    [[nodiscard]] SpecialFunc spec_fun_summoner() const;
    [[nodiscard]] int weaken_sn() const;

private:
    const SpecialFunc spec_fun_summoner_;
    const int weaken_sn_;
};

DependenciesImpl::DependenciesImpl()
    : spec_fun_summoner_{spec_lookup("spec_summoner")}, weaken_sn_{skill_lookup("weaken")} {}

void DependenciesImpl::interpret(Char *ch, std::string msg) {
    ::interpret(ch, msg.c_str());
} // TODO interpret() should take a string_view really

void DependenciesImpl::act(std::string_view msg, const Char *ch, Act1Arg arg1, Act2Arg arg2, To to, int position) {
    ::act(msg, ch, arg1, arg2, to, position);
}

void DependenciesImpl::act(std::string_view msg, const Char *ch, Act1Arg arg1, Act2Arg arg2, To to) {
    ::act(msg, ch, arg1, arg2, to);
}

void DependenciesImpl::obj_from_char(OBJ_DATA *obj) { ::obj_from_char(obj); }

void DependenciesImpl::obj_to_char(OBJ_DATA *obj, Char *ch) { ::obj_to_char(obj, ch); }

void DependenciesImpl::obj_from_room(OBJ_DATA *obj) { ::obj_from_room(obj); }

void DependenciesImpl::obj_to_room(OBJ_DATA *obj, ROOM_INDEX_DATA *room) { ::obj_to_room(obj, room); }

void DependenciesImpl::extract_obj(OBJ_DATA *obj) { ::extract_obj(obj); }

void DependenciesImpl::affect_to_char(Char *ch, const AFFECT_DATA &af) { ::affect_to_char(ch, af); }

OBJ_DATA *DependenciesImpl::object_list() { return ::object_list; }

SpecialFunc DependenciesImpl::spec_fun_summoner() const { return spec_fun_summoner_; }

int DependenciesImpl::weaken_sn() const { return weaken_sn_; }

DependenciesImpl dependencies;
CorpseSummoner corpse_summoner(dependencies);
}

/**
 * Special program for corpse summoner NPCs. Currently just one of
 * these in the Necropolis, but there could be more in future.
 */
bool spec_summoner(Char *ch) {
    if (ch->position < POS_STANDING || ch->in_room->vnum != ROOM_VNUM_NECROPOLIS)
        return false;
    corpse_summoner.summoner_awaits(ch, system_clock::to_time_t(current_time));
    return true;
}

void handle_corpse_summoner(Char *player, Char *summoner, OBJ_DATA *catalyst) {
    if (!corpse_summoner.check_summoner_preconditions(player, summoner)) {
        return;
    }
    if (!corpse_summoner.check_catalyst(player, summoner, catalyst)) {
        return;
    }
    corpse_summoner.summon_corpse(player, summoner, catalyst);
}
