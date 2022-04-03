/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  magic.c:  wild and wonderful spells                                  */
/*                                                                       */
/*************************************************************************/

#include "magic.h"
#include "AFFECT_DATA.hpp"
#include "AffectFlag.hpp"
#include "Alignment.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "DamageTolerance.hpp"
#include "DamageType.hpp"
#include "FlagFormat.hpp"
#include "Format.hpp"
#include "Logging.hpp"
#include "Materials.hpp"
#include "Object.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "OffensiveFlag.hpp"
#include "PlayerActFlag.hpp"
#include "Room.hpp"
#include "RoomFlag.hpp"
#include "SkillNumbers.hpp"
#include "SkillTables.hpp"
#include "Target.hpp"
#include "ToleranceFlag.hpp"
#include "VnumObjects.hpp"
#include "VnumRooms.hpp"
#include "Weapon.hpp"
#include "WeaponFlag.hpp"
#include "Wear.hpp"
#include "WeatherData.hpp"
#include "act_comm.hpp"
#include "act_info.hpp"
#include "act_wiz.hpp"
#include "challenge.hpp"
#include "comm.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"
#include "lookup.h"
#include "ride.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "update.hpp"

#include <array>
#include <fmt/format.h>
#include <fmt/printf.h>
#include <functional>
#include <range/v3/algorithm/find_if.hpp>

namespace {

constexpr std::array AcidBlastExorciseDemonfireDamage = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 131, 132,
    133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 148, 149, 149, 150, 151,
    153, 155, 157, 159, 161, 163, 165, 167, 169, 171, 173, 175, 177, 179, 181, 183, 185, 187, 189, 191, 193,
    195, 197, 199, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218};

constexpr std::array BurningHandsCauseSerDamage = {
    0,  0,  0,  0,  0,  14, 17, 20, 23, 26, 29, 29, 29, 30, 30, 31, 31, 32, 32, 33, 33, 34, 34,
    35, 35, 36, 36, 37, 37, 38, 38, 39, 39, 40, 40, 41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 46,
    46, 47, 47, 48, 48, 48, 49, 50, 50, 51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 56, 56, 56, 57,
    57, 57, 58, 58, 58, 59, 59, 59, 59, 60, 60, 61, 61, 62, 62, 62, 63, 64, 65, 66, 67, 68, 69};

constexpr std::array ChillTouchDamage = {0,  0,  0,  6,  7,  8,  9,  12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16,
                                         16, 17, 17, 17, 18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 22,
                                         23, 23, 23, 24, 24, 24, 25, 25, 25, 26, 26, 26, 27, 27, 28, 29, 29, 30, 30,
                                         31, 31, 32, 32, 33, 33, 34, 34, 35, 35, 35, 36, 36, 36, 37, 37, 37, 38, 38,
                                         38, 38, 39, 39, 40, 40, 41, 41, 41, 42, 43, 44, 45, 46, 47, 48};

const std::array ColourSprayDispelGoodEvilDamage = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  30, 35, 40, 45, 50, 55, 55, 55, 56, 57, 58, 58,
    59, 60, 61, 61, 62, 63, 64, 64, 65, 66, 67, 67, 68, 69, 70, 70, 71, 72, 73, 73, 74, 75, 76,
    76, 77, 78, 79, 79, 79, 80, 81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 86, 86, 87, 87, 87, 88,
    88, 88, 87, 87, 87, 88, 88, 88, 88, 89, 89, 90, 90, 91, 91, 91, 92, 93, 94, 95, 96, 97, 98};

constexpr std::array FireballHarmDamage = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   30,  35,  40,  45,  50,  55,  60,
    65,  70,  75,  80,  82,  84,  86,  88,  90,  92,  94,  96,  98,  100, 102, 104, 106, 108, 110, 112, 114, 116,
    118, 120, 122, 124, 126, 128, 130, 131, 132, 133, 133, 134, 135, 136, 136, 137, 138, 138, 139, 140, 140, 141,
    142, 143, 144, 146, 148, 150, 152, 154, 156, 158, 160, 162, 164, 166, 168, 170, 172, 174, 176, 178, 180, 182,
    184, 186, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201, 201, 201};

constexpr std::array FlamestrikeDamage = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   50,  55,
    60,  65,  70,  75,  80,  82,  84,  86,  88,  90,  92,  94,  96,  98,  100, 102, 104, 106, 108, 110, 112, 114, 116,
    118, 120, 122, 124, 126, 128, 130, 131, 132, 133, 133, 134, 135, 136, 136, 137, 138, 138, 139, 140, 140, 141, 141,
    142, 143, 143, 144, 144, 145, 145, 146, 146, 147, 147, 148, 148, 149, 149, 150, 151, 152, 153, 154, 155, 156};

constexpr std::array LightningBoltCauseCritDamage = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  25, 28, 31, 34, 37, 40, 40, 41, 42, 42, 43, 44, 44, 45,
    46, 46, 47, 48, 48, 49, 50, 50, 51, 52, 52, 53, 54, 54, 55, 56, 56, 57, 58, 58, 59, 60, 60,
    61, 62, 62, 63, 64, 64, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72, 72, 73,
    73, 73, 74, 74, 74, 75, 75, 75, 75, 76, 76, 77, 77, 78, 78, 78, 79, 80, 81, 82, 83, 84, 85};

constexpr std::array MagicMissileCauseLightDamage = {
    0,  3,  3,  4,  4,  5,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  9,  9,
    9,  9,  9,  10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13,
    14, 14, 14, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 22, 23,
    23, 23, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 27, 27, 28, 28, 29, 30, 31, 32, 33, 34, 35};

constexpr std::array ShockingGraspDamage = {0,  0,  0,  0,  0,  0,  0,  20, 25, 29, 33, 36, 39, 39, 39, 40, 40, 41, 41,
                                            42, 42, 43, 43, 44, 44, 45, 45, 46, 46, 47, 47, 48, 48, 49, 49, 50, 50, 51,
                                            51, 52, 52, 53, 53, 54, 54, 55, 55, 56, 56, 57, 57, 57, 58, 59, 59, 60, 60,
                                            61, 61, 62, 62, 63, 63, 64, 64, 65, 65, 65, 66, 66, 66, 67, 67, 67, 68, 68,
                                            68, 68, 69, 69, 70, 70, 71, 71, 71, 72, 73, 74, 75, 76, 77, 78};

/**
 * Most direct damage spells calculate their damage using the same
 * formula but seeded with different damage tables.
 */
template <std::size_t Size>
std::pair<int, int> get_direct_dmg_and_level(int level, const std::array<int, Size> &damage_table) {
    int clamped_level = std::clamp(level, 0, static_cast<int>(damage_table.size() - 1));
    int dam = number_range(damage_table[clamped_level] / 2, damage_table[clamped_level] * 2);
    return {dam, clamped_level};
}

/**
 * During spell casting we need to determine the SpellTarget based on what the caster stated in entity_name
 * and the Target type of the spell.
 *
 * The original_args param contains the original arguments provided by the caller which is something we must
 * pass on through to the underlying spell for spells that perform their own custom targeting, like 'undo spell' or
 * 'gate'.
 *
 * Returns an empty optional SpellTarget that wraps the real target, or nullopt if no valid SpellTarget could be
 * created.
 */
std::optional<SpellTarget> get_casting_spell_target(Char *ch, const int sn, std::string_view entity_name,
                                                    std::string_view original_args) {
    Char *victim;
    switch (skill_table[sn].target) {
    default: bug("Do_cast: bad target for sn {}.", sn); return std::nullopt;

    case Target::CharOffensive:
        if (entity_name.empty()) {
            if ((victim = ch->fighting) == nullptr) {
                ch->send_line("Cast the spell on whom?");
                return std::nullopt;
            }
        } else {
            if ((victim = get_char_room(ch, entity_name)) == nullptr) {
                ch->send_line("They aren't here.");
                return std::nullopt;
            }
        }
        if (ch->is_pc()) {
            if (is_safe_spell(ch, victim, false) && victim != ch) {
                ch->send_line("Not on that target.");
                return std::nullopt;
            }
        }
        if (ch->is_aff_charm() && ch->master == victim) {
            ch->send_line("You can't do that on your own follower.");
            return std::nullopt;
        }
        return SpellTarget(victim);

    case Target::CharDefensive:
        if (entity_name.empty()) {
            victim = ch;
        } else {
            if ((victim = get_char_room(ch, entity_name)) == nullptr) {
                ch->send_line("They aren't here.");
                return std::nullopt;
            }
        }
        return SpellTarget(victim);
    case Target::Ignore:
    case Target::CharObject:
    case Target::CharOther: return SpellTarget(original_args);

    case Target::CharSelf:
        if (!entity_name.empty() && !is_name(entity_name, ch->name)) {
            ch->send_line("You cannot cast this spell on another.");
            return std::nullopt;
        }
        return SpellTarget(ch);

    case Target::ObjectInventory:
        if (entity_name.empty()) {
            ch->send_line("What should the spell be cast upon?");
            return std::nullopt;
        }
        Object *obj;
        if ((obj = ch->find_in_inventory(entity_name)) == nullptr) {
            ch->send_line("You are not carrying that.");
            return std::nullopt;
        }
        return SpellTarget(obj);
    }
}

/**
 * During spell casting using an object like a Scroll, we need to determine the SpellTarget based on what the caster
 * stated in entity_name and the Target type of the spell. Returns an empty optional SpellTarget that wraps the real
 * target, or nullopt if no valid SpellTarget could be created.
 * TODO: perhaps the code for this and do_cast can be unified as they're similar.
 */
std::optional<SpellTarget> get_obj_casting_spell_target(Char *ch, Char *victim, Object *obj, std::string_view arguments,
                                                        const int sn) {
    switch (skill_table[sn].target) {
    default: bug("Obj_cast_spell: bad target for sn {}.", sn); return std::nullopt; // FIXME?

    case Target::Ignore:
    case Target::CharObject:
    case Target::CharOther: return SpellTarget(arguments);

    case Target::CharOffensive:
        if (victim == nullptr)
            victim = ch->fighting;
        if (victim == nullptr) {
            ch->send_line("You can't do that.");
            return std::nullopt;
        }
        if (is_safe_spell(ch, victim, false) && ch != victim) {
            ch->send_line("Something isn't right...");
            return std::nullopt;
        }
        return SpellTarget(victim);

    case Target::CharDefensive:
        if (victim == nullptr)
            victim = ch;
        return SpellTarget(victim);

    case Target::CharSelf: return SpellTarget(ch);

    case Target::ObjectInventory:
        if (obj == nullptr) {
            ch->send_line("You can't do that.");
            return std::nullopt;
        }
        return SpellTarget(obj);
    }
}

/**
 * After casting an offensive spell (directly or when using an object), if the target of the spell is in the
 * room, hit it and provoke a fight if one is not already under way.
 */
void casting_may_provoke_victim(Char *ch, const SpellTarget &spell_target, const int sn) {
    if (skill_table[sn].target == Target::CharOffensive && spell_target.getChar() != ch
        && spell_target.getChar()->master != ch
        && (spell_target.getChar()->is_npc() || fighting_duel(ch, spell_target.getChar()))) {
        if (ch && ch->in_room) {
            for (auto *vch : ch->in_room->people) {
                if (spell_target.getChar() == vch && spell_target.getChar()->fighting == nullptr) {
                    multi_hit(spell_target.getChar(), ch);
                    break;
                }
            }
        }
    }
}

/* MG's rather more dubious bomb-making routine */
void try_create_bomb(Char *ch, const int sn, const int mana) {
    if (ch->class_num != 0) {
        ch->send_line("You're more than likely gonna kill yourself!");
        return;
    }

    if (ch->is_pos_preoccupied()) {
        ch->send_line("You can't concentrate enough.");
        return;
    }

    if (ch->is_pc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }

    if (ch->is_pc() && ch->mana < (mana * 2)) {
        ch->send_line("You don't have enough mana.");
        return;
    }

    ch->wait_state(skill_table[sn].beats * 2);

    if (ch->is_pc() && number_percent() > ch->get_skill(sn)) {
        ch->send_line("You lost your concentration.");
        check_improve(ch, sn, false, 8);
        ch->mana -= mana / 2;
    } else {
        // Array holding the innate chance of explosion per
        // additional spell for bombs
        static const int bomb_chance[5] = {0, 0, 20, 35, 70};
        ch->mana -= (mana * 2);
        ch->gold -= (mana * 100);
        auto *bomb = get_eq_char(ch, Wear::Hold);
        if (bomb) {
            if (bomb->type != ObjectType::Bomb) {
                ch->send_line("You must be holding a bomb to add to it.");
                ch->send_line("Or, to create a new bomb you must have free hands.");
                ch->mana += (mana * 2);
                ch->gold += (mana * 100);
                return;
            }
            // God alone knows what I was on when I originally wrote this....
            /* do detonation if things go wrong */
            if ((bomb->value[0] > (ch->level + 1)) && (number_percent() < 90)) {
                act("Your naive fumblings on a higher-level bomb cause it to go off!", ch, nullptr, nullptr, To::Char);
                act("$n is delving with things $s doesn't understand - BANG!!", ch, nullptr, nullptr, To::Char);
                explode_bomb(bomb, ch, ch);
                return;
            }

            // Find first free slot
            auto pos = 2;
            for (; pos < 5; ++pos) {
                if (bomb->value[pos] <= 0)
                    break;
            }
            if ((pos == 5) || // If we've run out of spaces in the bomb
                (number_percent() > (get_curr_stat(ch, Stat::Int) * 4)) || // test against int
                (number_percent() < bomb_chance[pos])) { // test against the number of spells in the bomb
                act("You try to add another spell to your bomb but it can't take anymore!!!", ch, nullptr, nullptr,
                    To::Char);
                act("$n tries to add more power to $s bomb - but has pushed it too far!!!", ch, nullptr, nullptr,
                    To::Room);
                explode_bomb(bomb, ch, ch);
                return;
            }
            bomb->value[pos] = sn;
            act("$n adds more potency to $s bomb.", ch);
            act("You carefully add another spell to your bomb.", ch, nullptr, nullptr, To::Char);

        } else {
            bomb = create_object(get_obj_index(Objects::Bomb));
            bomb->level = ch->level;
            bomb->value[0] = ch->level;
            bomb->value[1] = sn;

            bomb->description = fmt::sprintf(bomb->description, ch->name);
            bomb->name = fmt::sprintf(bomb->name, ch->name);

            obj_to_char(bomb, ch);
            equip_char(ch, bomb, Wear::Hold);
            act("$n creates $p.", ch, bomb, nullptr, To::Room);
            act("You have created $p.", ch, bomb, nullptr, To::Char);

            check_improve(ch, sn, true, 8);
        }
    }
    return;
}

/* MG's scribing command ... */
void try_create_scroll(Char *ch, const int sn, const int mana) {
    if ((ch->class_num != 0) && (ch->class_num != 1)) {
        ch->send_line("You can't scribe! You can't read or write!");
        return;
    }

    if (ch->is_pos_preoccupied()) {
        ch->send_line("You can't concentrate enough.");
        return;
    }

    if (ch->is_pc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }

    if (ch->is_pc() && ch->mana < (mana * 2)) {
        ch->send_line("You don't have enough mana.");
        return;
    }
    auto *scroll_index = get_obj_index(Objects::Scroll);
    if (ch->carry_weight + scroll_index->weight > can_carry_w(ch)) {
        act("You cannot carry that much weight.", ch, nullptr, nullptr, To::Char);
        return;
    }
    if (ch->carry_number + 1 > can_carry_n(ch)) {
        act("You can't carry any more items.", ch, nullptr, nullptr, To::Char);
        return;
    }

    ch->wait_state(skill_table[sn].beats * 2);
    if (ch->is_pc() && number_percent() > ch->get_skill(sn)) {
        ch->send_line("You lost your concentration.");
        check_improve(ch, sn, false, 8);
        ch->mana -= mana / 2;
    } else {
        ch->mana -= (mana * 2);
        ch->gold -= (mana * 100);

        auto *scroll = create_object(get_obj_index(Objects::Scroll));
        scroll->level = ch->level;
        scroll->value[0] = ch->level;
        scroll->value[1] = sn;
        scroll->value[2] = -1;
        scroll->value[3] = -1;
        scroll->value[4] = -1;

        scroll->short_descr = fmt::sprintf(scroll->short_descr, skill_table[sn].name);
        scroll->description = fmt::sprintf(scroll->description, skill_table[sn].name);
        scroll->name = fmt::sprintf(scroll->name, skill_table[sn].name);

        obj_to_char(scroll, ch);
        act("$n creates $p.", ch, scroll, nullptr, To::Room);
        act("You have created $p.", ch, scroll, nullptr, To::Char);

        check_improve(ch, sn, true, 8);
    }
}

/* MG's brewing command ... */
void try_create_potion(Char *ch, const int sn, const int mana) {
    if ((ch->class_num != 0) && (ch->class_num != 1)) {
        ch->send_line("You can't make potions! You don't know how!");
        return;
    }

    if (ch->is_pos_preoccupied()) {
        ch->send_line("You can't concentrate enough.");
        return;
    }

    if (ch->is_pc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }

    if (ch->is_pc() && ch->mana < (mana * 2)) {
        ch->send_line("You don't have enough mana.");
        return;
    }
    const auto *potion_index = get_obj_index(Objects::Potion);
    if (ch->carry_weight + potion_index->weight > can_carry_w(ch)) {
        act("You cannot carry that much weight.", ch, nullptr, nullptr, To::Char);
        return;
    }
    if (ch->carry_number + 1 > can_carry_n(ch)) {
        act("You can't carry any more items.", ch, nullptr, nullptr, To::Char);
        return;
    }

    ch->wait_state(skill_table[sn].beats * 2);

    if (ch->is_pc() && number_percent() > ch->get_skill(sn)) {
        ch->send_line("You lost your concentration.");
        check_improve(ch, sn, false, 8);
        ch->mana -= mana / 2;
    } else {
        ch->mana -= (mana * 2);
        ch->gold -= (mana * 100);

        auto *potion = create_object(get_obj_index(Objects::Potion));
        potion->level = ch->level;
        potion->value[0] = ch->level;
        potion->value[1] = sn;
        potion->value[2] = -1;
        potion->value[3] = -1;
        potion->value[4] = -1;

        potion->short_descr = fmt::sprintf(potion->short_descr, skill_table[sn].name);
        potion->description = fmt::sprintf(potion->description, skill_table[sn].name);
        potion->name = fmt::sprintf(potion->name, skill_table[sn].name);

        obj_to_char(potion, ch);
        act("$n creates $p.", ch, potion, nullptr, To::Room);
        act("You have created $p.", ch, potion, nullptr, To::Char);

        check_improve(ch, sn, true, 8);
    }
}

struct Syllable {
    std::string_view old;
    std::string_view new_t;
};

constexpr std::array<Syllable, 52> Syllables = {
    {{" ", " "},        {"ar", "abra"},    {"au", "kada"},   {"bless", "fido"}, {"blind", "nose"}, {"bur", "mosa"},
     {"cu", "judi"},    {"de", "oculo"},   {"en", "unso"},   {"light", "dies"}, {"lo", "hi"},      {"mor", "zak"},
     {"move", "sido"},  {"ness", "lacri"}, {"ning", "illa"}, {"per", "duda"},   {"ra", "gru"},     {"fresh", "ima"},
     {"gate", "imoff"}, {"re", "candus"},  {"son", "sabru"}, {"tect", "infra"}, {"tri", "cula"},   {"ven", "nofo"},
     {"oct", "bogusi"}, {"rine", "dude"},  {"a", "a"},       {"b", "b"},        {"c", "q"},        {"d", "e"},
     {"e", "z"},        {"f", "y"},        {"g", "o"},       {"h", "p"},        {"i", "u"},        {"j", "y"},
     {"k", "t"},        {"l", "r"},        {"m", "w"},       {"n", "i"},        {"o", "a"},        {"p", "s"},
     {"q", "d"},        {"r", "f"},        {"s", "g"},       {"t", "h"},        {"u", "j"},        {"v", "z"},
     {"w", "x"},        {"x", "n"},        {"y", "l"},       {"z", "k"}}};

} // namespace

std::pair<std::string, std::string> casting_messages(const int sn) {
    std::string translation{};
    size_t length{};
    for (const auto *pName = skill_table[sn].name; *pName != '\0'; pName += length) {
        for (const auto &syllable : Syllables) {
            if (matches_start(syllable.old, pName)) {
                translation += syllable.new_t;
                length = syllable.old.length();
                break;
            }
        }
        if (length == 0)
            length = 1;
    }
    auto to_same_class = fmt::format("|y$n utters the words '{}'.|w", skill_table[sn].name);
    auto to_other_class = fmt::format("|y$n utters the words '{}'.|w", translation);
    return {to_same_class, to_other_class};
}

/*
 * Utter mystical words for an sn.
 */
void say_spell(Char *ch, const int sn) {
    const auto messages = casting_messages(sn);
    for (auto *rch : ch->in_room->people) {
        if (rch != ch)
            act(ch->class_num == rch->class_num ? messages.first : messages.second, ch, nullptr, rch, To::Vict);
    }
}

/*
 * Compute a saving throw.
 * Chars of equal level and no resist gear will have 50/50 chance landing or resisting.
 * Gear with negative saving_throw increases the chance that the victim will resist.
 * If the victim is berserk, this actually makes them more vulnerable to attack
 * from melee and magic, offsetting the damage & healing benefits it provides.
 */
bool saves_spell(int level, const Char *victim) {
    int save = 50 + (victim->level - level - victim->saving_throw);
    if (victim->is_aff_berserk())
        save -= victim->level / 3;
    save = std::clamp(save, 5, 95);
    return number_percent() < save;
}

/* RT save for dispels */

bool saves_dispel(int dis_level, int spell_level) {
    const int save = std::clamp(50 + (spell_level - dis_level) * 3, 8, 95);
    return number_percent() < save;
}

bool check_dispel(int dis_level, Char *victim, int spell_num) {
    // This behaviour probably needs a look:
    // * multiple affects of the same SN get multiple chances of being dispelled.
    // * any succeeding chance strips _all_ affects of that SN.
    for (auto &af : victim->affected) {
        if (af.type != spell_num)
            continue;
        if (!saves_dispel(dis_level, af.level)) {
            affect_strip(victim, spell_num);
            if (skill_table[spell_num].msg_off)
                victim->send_line("{}", skill_table[spell_num].msg_off);
            return true;
        }
        af.level--;
    }
    return false;
}

int mana_for_spell(const Char *ch, const int sn) {
    if (ch->level + 2 == get_skill_level(ch, sn))
        return 50;
    const sh_int mana = 100 / (2 + ch->level - get_skill_level(ch, sn));
    return std::max(skill_table[sn].min_mana, mana);
}

void do_cast(Char *ch, ArgParser args) {
    /* Switched NPC's can cast spells, but others can't. */
    if (ch->is_npc() && (ch->desc == nullptr))
        return;
    if (args.empty()) {
        ch->send_line("Cast which what where?");
        return;
    }
    const auto spell_name = args.shift();
    const auto sn = skill_lookup(spell_name);
    if (sn <= 0 || (ch->is_pc() && ch->level < get_skill_level(ch, sn))) {
        ch->send_line("You don't know any spells of that name.");
        return;
    }
    // Copy the remaining arguments because for some spells we must pass them through to
    // the underlying spell_ routine and as SpellTarget wraps them as a string_view, the object lifetime matters.
    const auto original_args = std::string(args.remaining());

    if (ch->position < skill_table[sn].minimum_position) {
        ch->send_line("You can't concentrate enough.");
        return;
    }
    const int mana = mana_for_spell(ch, sn);
    const auto entity_name = args.shift();
    if (matches(entity_name, "explosive")) {
        try_create_bomb(ch, sn, mana);
        return;
    }
    if (matches(entity_name, "scribe") && (skill_table[sn].spell_fun != spell_null)) {
        try_create_scroll(ch, sn, mana);
        return;
    }
    if (matches(entity_name, "brew") && (skill_table[sn].spell_fun != spell_null)) {
        try_create_potion(ch, sn, mana);
        return;
    }
    auto opt_spell_target = get_casting_spell_target(ch, sn, entity_name, original_args);
    if (!opt_spell_target) {
        return;
    }
    if (ch->is_pc() && ch->mana < mana) {
        ch->send_line("You don't have enough mana.");
        return;
    }

    if (!matches(skill_table[sn].name, "ventriloquate"))
        say_spell(ch, sn);

    ch->wait_state(skill_table[sn].beats);

    if (ch->is_pc() && number_percent() > ch->get_skill(sn)) {
        ch->send_line("You lost your concentration.");
        check_improve(ch, sn, false, 1);
        ch->mana -= mana / 2;
    } else {
        ch->mana -= mana;
        (*skill_table[sn].spell_fun)(sn, ch->level, ch, *opt_spell_target);
        check_improve(ch, sn, true, 1);
    }
    casting_may_provoke_victim(ch, *opt_spell_target, sn);
}

/*
 * Cast spells at targets using a magical object.
 */
void obj_cast_spell(const int sn, const int level, Char *ch, Char *victim, Object *obj, std::string_view arguments) {
    if (sn <= 0)
        return;

    if (sn >= MAX_SKILL || skill_table[sn].spell_fun == 0) {
        bug("Obj_cast_spell: bad sn {}.", sn);
        return;
    }

    auto opt_spell_target = get_obj_casting_spell_target(ch, victim, obj, arguments, sn);
    if (!opt_spell_target) {
        return;
    }
    (*skill_table[sn].spell_fun)(sn, level, ch, *opt_spell_target);
    casting_may_provoke_victim(ch, *opt_spell_target, sn);
}

/*
 * Spell functions.
 */
void spell_acid_blast(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    auto dam_level = get_direct_dmg_and_level(level, AcidBlastExorciseDemonfireDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Acid);
}

void spell_acid_wash(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->type != ObjectType::Weapon) {
        ch->send_line("That isn't a weapon.");
        return;
    }

    if (obj->wear_loc != Wear::None) {
        ch->send_line("You do not have that item in your inventory.");
        return;
    }

    if (check_enum_bit(obj->value[4], WeaponFlag::Lightning)) {
        ch->send_line("Acid and lightning don't mix.");
        return;
    }
    obj->value[3] = Attacks::index_of("acbite");
    set_enum_bit(obj->value[4], WeaponFlag::Acid);
    ch->send_to("With a mighty scream you draw acid from the earth.\n\rYou wash your weapon in the acid pool.\n\r");
}

void spell_armor(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("You are already protected by more powerful armor.");
            else
                act("$N is already protected by more powerful armor.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.modifier = -20;
    af.location = AffectLocation::Ac;
    af.bitvector = 0;
    affect_to_char(victim, af);
    victim->send_line("You feel someone protecting you.");
    if (ch != victim)
        act("$N is protected by your magic.", ch, nullptr, victim, To::Char);
}

void spell_bless(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("Greater blessings have already been granted to you.");
            else
                act("$N is already has divine favour.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = 6 + level;
    af.location = AffectLocation::Hitroll;
    af.modifier = level / 8;
    af.bitvector = 0;
    affect_to_char(victim, af);

    af.location = AffectLocation::SavingSpell;
    af.modifier = 0 - level / 8;
    affect_to_char(victim, af);
    victim->send_line("You feel righteous.");
    if (ch != victim)
        act("You grant $N the favor of your god.", ch, nullptr, victim, To::Char);
}

void spell_blindness(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)ch;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (victim->is_aff_blind()) {
        if (ch == victim) {
            ch->send_line("You are already blind.");
        } else {
            act("$N is already blind.", ch, nullptr, victim, To::Char);
        }
        return;
    }
    if (saves_spell(level, victim)) {
        act("$n shakes off an attempt to blind $m.", victim, nullptr, victim, To::Room);
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.location = AffectLocation::Hitroll;
    af.modifier = -4;
    af.duration = 1;
    af.bitvector = to_int(AffectFlag::Blind);
    affect_to_char(victim, af);
    victim->send_line("You are blinded!");
    act("$n appears to be blinded.", victim);
}

void spell_burning_hands(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    auto dam_level = get_direct_dmg_and_level(level, BurningHandsCauseSerDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Fire);
}

void spell_call_lightning(int sn, int level, Char *ch, [[maybe_unused]] const SpellTarget &spell_target) {
    if (ch->is_inside()) {
        ch->send_line("You must be outside to invoke the power of lightning.");
        return;
    }

    if (!weather_info.is_raining()) {
        ch->send_line("You need bad weather.");
        return;
    }

    const int dam = dice(level / 2, 8);

    ch->send_line("{}'s lightning strikes your foes!", deity_name);
    act(fmt::format("$n calls {}'s lightning to strike $s foes!", deity_name), ch);

    for (auto *vch : char_list) {
        if (!vch->in_room)
            continue;
        if (vch->in_room == ch->in_room) {
            if (vch != ch && (ch->is_npc() ? vch->is_pc() : vch->is_npc()))
                damage(ch, vch, saves_spell(level, vch) ? dam / 2 : dam, &skill_table[sn], DamageType::Lightning);
            continue;
        }

        if (vch->in_room->area == ch->in_room->area && vch->is_outside() && vch->is_pos_awake())
            vch->send_line("Lightning flashes in the sky.");
    }
}

/* RT calm spell stops all fighting in the room */

void spell_calm(int sn, int level, Char *ch, [[maybe_unused]] const SpellTarget &spell_target) {
    int mlevel = 0;
    int count = 0;
    sh_int high_level = 0;

    /* get sum of all mobile levels in the room */
    for (auto *vch : ch->in_room->people) {
        if (vch->is_pos_fighting()) {
            count++;
            if (vch->is_npc())
                mlevel += vch->level;
            else
                mlevel += vch->level / 2;
            high_level = std::max(high_level, vch->level);
        }
    }

    /* compute chance of stopping combat */
    const int chance = 4 * level - high_level + 2 * count;

    if (ch->is_immortal()) /* always works */
        mlevel = 0;

    if (number_range(0, chance) >= mlevel) /* hard to stop large fights */
    {
        for (auto *vch : ch->in_room->people) {
            if (vch->is_npc()
                && (check_enum_bit(vch->imm_flags, ToleranceFlag::Magic)
                    || check_enum_bit(vch->act, CharActFlag::Undead)))
                return;

            if (vch->is_aff_calm() || vch->is_aff_berserk() || vch->is_affected_by(skill_lookup("frenzy")))
                return;

            vch->send_line("A wave of calm passes over you.");

            if (vch->fighting || vch->is_pos_fighting())
                stop_fighting(vch, false);

            if (vch->is_npc())
                vch->sentient_victim.clear();

            AFFECT_DATA af;
            af.type = sn;
            af.level = level;
            af.duration = level / 4;
            af.location = AffectLocation::Hitroll;
            if (vch->is_pc())
                af.modifier = -5;
            else
                af.modifier = -2;
            af.bitvector = to_int(AffectFlag::Calm);
            affect_to_char(vch, af);

            af.location = AffectLocation::Damroll;
            affect_to_char(vch, af);
        }
    }
}

/**
 * Try to dispel/cancel all dynamic effects from the victim.
 * Also if the victim is an NPC with a 'permanent' affect applied,
 * it has a chance of stripping at most one of those.
 * Returns true if anything was dispelled.
 */
bool try_dispel_all_dispellables(int level, Char *victim) {
    bool found = false;
    bool dispelled_perm_affect = false;
    for (auto sn = 0; sn < MAX_SKILL; sn++) {
        auto &spell = skill_table[sn];
        if (spell.dispel_fun) {
            // Dispel dynamic effects.
            if (spell.dispel_fun(level, victim, sn)) {
                act(spell.dispel_victim_msg_to_room, victim);
                found = true;
            }
            // Dispel permanent effects on NPCs. Only try one per cast of dispel magic
            // otherwise it'll be too easy potentially fully debuff a mob.
            if (!dispelled_perm_affect && victim->is_npc() && victim->has_affect_bit(spell.dispel_npc_perm_affect_bit)
                && !saves_dispel(level, victim->level) && victim->is_affected_by(sn)) {

                clear_bit(victim->affected_by, spell.dispel_npc_perm_affect_bit);
                act(spell.dispel_victim_msg_to_room, victim);
                dispelled_perm_affect = true;
                found = true;
            }
        }
    }
    return found;
}

void spell_cancellation(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    level += 2;

    if ((ch->is_pc() && victim->is_npc() && !(ch->is_aff_charm() && ch->master == victim))
        || (ch->is_npc() && victim->is_pc())) {
        ch->send_line("You failed, try dispel magic.");
        return;
    }
    /* unlike dispel magic, the victim gets NO save */
    const auto found = try_dispel_all_dispellables(level, victim);
    if (found)
        ch->send_line("Ok.");
    else
        ch->send_line("Spell failed.");
}

void spell_cause_light(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    auto dam_level = get_direct_dmg_and_level(level, MagicMissileCauseLightDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Harm);
}

void spell_cause_critical(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    auto dam_level = get_direct_dmg_and_level(level, LightningBoltCauseCritDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Harm);
}

void spell_cause_serious(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    auto dam_level = get_direct_dmg_and_level(level, BurningHandsCauseSerDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Harm);
}

void spell_chain_lightning(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    Char *last_vict;
    bool found;

    /* first strike */

    act("A lightning bolt leaps from $n's hand and arcs to $N.", ch, nullptr, victim, To::Room);
    act("A lightning bolt leaps from your hand and arcs to $N.", ch, nullptr, victim, To::Char);
    act("A lightning bolt leaps from $n's hand and hits you!", ch, nullptr, victim, To::Vict);

    auto dam_level = get_direct_dmg_and_level(level, LightningBoltCauseCritDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 3;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Lightning);
    last_vict = victim;
    level -= 4; /* decrement damage */

    /* new targets */
    while (level > 0) {
        found = false;
        for (auto *tmp_vict : ch->in_room->people) {
            if (!is_safe_spell(ch, tmp_vict, true) && tmp_vict != last_vict) {
                found = true;
                last_vict = tmp_vict;
                act("The bolt arcs to $n!", tmp_vict);
                act("The bolt hits you!", tmp_vict, nullptr, nullptr, To::Char);
                dam_level = get_direct_dmg_and_level(level, LightningBoltCauseCritDamage);
                if (saves_spell(dam_level.second, tmp_vict))
                    dam_level.first /= 3;
                damage(ch, tmp_vict, dam_level.first, &skill_table[sn], DamageType::Lightning);
                level -= 4; /* decrement damage */
            }
        } /* end target searching loop */

        if (!found) /* no target found, hit the caster */
        {
            if (ch == nullptr || ch->in_room == nullptr)
                return;

            if (last_vict == ch) /* no double hits */
            {
                act("The bolt seems to have fizzled out.", ch);
                act("The bolt grounds out through your body.", ch, nullptr, nullptr, To::Char);
                return;
            }

            last_vict = ch;
            act("The bolt arcs to $n...whoops!", ch);
            ch->send_line("You are struck by your own lightning!");
            dam_level = get_direct_dmg_and_level(level, LightningBoltCauseCritDamage);
            if (saves_spell(dam_level.second, ch))
                dam_level.first /= 3;
            damage(ch, ch, dam_level.first, &skill_table[sn], DamageType::Lightning);
            level -= 4; /* decrement damage */
            if ((ch == nullptr) || (ch->in_room == nullptr))
                return;
        }
        /* now go back and find more targets */
    }
}

void spell_change_sex(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    AFFECT_DATA af;

    if (victim->is_affected_by(sn)) {
        if (victim == ch)
            ch->send_line("You've already been changed.");
        else
            act("$N has already had $s(?) sex changed.", ch, nullptr, victim, To::Char);
        return;
    }
    if (saves_spell(level, victim))
        return;
    af.type = sn;
    af.level = level;
    af.duration = 2 * level;
    af.location = AffectLocation::Sex;
    do {
        af.modifier = number_range(0, 2) - victim->sex.integer();
    } while (af.modifier == 0);
    af.bitvector = 0;
    affect_to_char(victim, af);
    victim->send_line("You feel different.");
    act("$n doesn't look like $r anymore...", victim);
}

void spell_charm_person(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    AFFECT_DATA af;

    if (victim == ch) {
        ch->send_line("You like yourself even better!");
        return;
    }

    if (victim->is_aff_charm() || ch->is_aff_charm() || ch->get_trust() < victim->get_trust()
        || check_enum_bit(victim->imm_flags, ToleranceFlag::Charm) || saves_spell(level, victim))
        return;

    if (check_enum_bit(victim->in_room->room_flags, RoomFlag::Law)) {
        ch->send_line("Charming is not permitted here.");
        return;
    }

    if (victim->master)
        stop_follower(victim);
    add_follower(victim, ch);
    victim->leader = ch;
    af.type = sn;
    af.level = level;
    af.duration = number_fuzzy(level / 4);
    af.bitvector = to_int(AffectFlag::Charm);
    affect_to_char(victim, af);
    act("Isn't $n just so nice?", ch, nullptr, victim, To::Vict);
    if (ch != victim)
        act("$N looks at you with adoring eyes.", ch, nullptr, victim, To::Char);
}

void spell_chill_touch(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    AFFECT_DATA af;
    auto dam_level = get_direct_dmg_and_level(level, ChillTouchDamage);
    if (!saves_spell(dam_level.second, victim)) {
        act("$n turns blue and shivers.", victim);
        af.type = sn;
        af.level = dam_level.second;
        af.duration = 6;
        af.location = AffectLocation::Str;
        af.modifier = -1;
        af.bitvector = 0;
        affect_join(victim, af);
    } else {
        dam_level.first /= 2;
    }

    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Cold);
}

void spell_colour_spray(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    auto dam_level = get_direct_dmg_and_level(level, ColourSprayDispelGoodEvilDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    else
        spell_blindness(skill_lookup("blindness"), dam_level.second / 2, ch, spell_target);

    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Light);
}

void spell_continual_light(int sn, int level, Char *ch, [[maybe_unused]] const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Object *light;

    light = create_object(get_obj_index(Objects::LightBall));
    obj_to_room(light, ch->in_room);
    act("$n twiddles $s thumbs and $p appears.", ch, light, nullptr, To::Room);
    act("You twiddle your thumbs and $p appears.", ch, light, nullptr, To::Char);
}

void spell_control_weather(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    std::string_view arguments = spell_target.getArguments();
    if (matches(arguments, "better"))
        weather_info.control(dice(level / 3, 4));
    else if (matches(arguments, "worse"))
        weather_info.control(-dice(level / 3, 4));
    else {
        ch->send_line("Do you want it to get better or worse?");
        return;
    }
    ch->send_line("As you raise your head, a swirl of energy spirals upward into the heavens.");
}

void spell_create_food(int sn, int level, Char *ch, [[maybe_unused]] const SpellTarget &spell_target) {
    (void)sn;
    Object *mushroom = create_object(get_obj_index(Objects::Mushroom));
    mushroom->value[0] = 5 + level;
    obj_to_room(mushroom, ch->in_room);
    act("$p suddenly appears.", ch, mushroom, nullptr, To::Room);
    act("$p suddenly appears.", ch, mushroom, nullptr, To::Char);
}

void spell_create_spring(int sn, int level, Char *ch, [[maybe_unused]] const SpellTarget &spell_target) {
    (void)sn;
    Object *spring = create_object(get_obj_index(Objects::Spring));
    spring->timer = level;
    obj_to_room(spring, ch->in_room);
    act("$p flows from the ground.", ch, spring, nullptr, To::Room);
    act("$p flows from the ground.", ch, spring, nullptr, To::Char);
}

void spell_create_water(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->type != ObjectType::Drink) {
        ch->send_line("It is unable to hold water.");
        return;
    }

    if (magic_enum::enum_cast<Liquid::Type>(obj->value[2]) != Liquid::Type::Water && obj->value[1] != 0) {
        ch->send_line("It contains some other liquid.");
        return;
    }

    int water = std::min(level * (weather_info.is_raining() ? 4 : 2), obj->value[0] - obj->value[1]);

    if (water > 0) {
        obj->value[2] = magic_enum::enum_integer<Liquid::Type>(Liquid::Type::Water);
        obj->value[1] += water;
        if (!is_name("water", obj->name))
            obj->name = fmt::format("{} water", obj->name);
        act("$p is filled.", ch, obj, nullptr, To::Char);
    } else {
        act("$p is full.", ch, obj, nullptr, To::Char);
    }
}

void spell_cure_blindness(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (!victim->is_affected_by(gsn_blindness)) {
        if (victim == ch)
            ch->send_line("You aren't blind.");
        else
            act("$N doesn't appear to be blinded.", ch, nullptr, victim, To::Char);
        return;
    }

    if (check_dispel((level + 5), victim, gsn_blindness)) {
        victim->send_line("Your vision returns!");
        act("$n is no longer blinded.", victim);
    } else
        ch->send_line("Spell failed.");
}

void spell_cure_critical(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const sh_int adjusted = victim->hit + (dice(3, 8) + level - 6);
    victim->hit = std::min(adjusted, victim->max_hit);
    update_pos(victim);
    victim->send_line("You feel better!");
    if (ch != victim)
        ch->send_line("Ok.");
}

/* RT added to cure plague */
void spell_cure_disease(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (!victim->is_affected_by(gsn_plague)) {
        if (victim == ch)
            ch->send_line("You aren't ill.");
        else
            act("$N doesn't appear to be diseased.", ch, nullptr, victim, To::Char);
        return;
    }

    if (check_dispel((level + 5), victim, gsn_plague)) {
        act("$n looks relieved as $s sores vanish.", victim);
    } else
        ch->send_line("Spell failed.");
}

void spell_cure_light(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const sh_int adjusted = victim->hit + (dice(1, 8) + level / 3);
    victim->hit = std::min(adjusted, victim->max_hit);
    update_pos(victim);
    victim->send_line("You feel better!");
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_cure_poison(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (!victim->is_affected_by(gsn_poison)) {
        if (victim == ch)
            ch->send_line("You aren't poisoned.");
        else
            act("$N doesn't appear to be poisoned.", ch, nullptr, victim, To::Char);
        return;
    }

    if (check_dispel((level + 5), victim, gsn_poison)) {
        victim->send_line("A warm feeling runs through your body.");
        act("$n looks much better.", victim);
    } else
        ch->send_line("Spell failed.");
}

void spell_cure_serious(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const sh_int adjusted = victim->hit + (dice(2, 8) + level / 2);
    victim->hit = std::min(adjusted, victim->max_hit);
    update_pos(victim);
    victim->send_line("You feel better!");
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_curse(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (victim->is_aff_curse()) {
        act("A curse has already befallen $N.", ch, nullptr, victim, To::Char);
        return;
    }
    if (saves_spell(level, victim)) {
        act("$N looks uncomfortable for a moment but it passes.", ch, nullptr, victim, To::Char);
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = 3 + (level / 6);
    af.location = AffectLocation::Hitroll;
    af.modifier = -1 * (level / 8);
    af.bitvector = to_int(AffectFlag::Curse);
    affect_to_char(victim, af);

    af.location = AffectLocation::SavingSpell;
    af.modifier = level / 8;
    affect_to_char(victim, af);

    victim->send_line("You feel unclean.");
    if (ch != victim)
        act("$N looks very uncomfortable.", ch, nullptr, victim, To::Char);
}

/* PGW hard hitting spell in the attack group, the big boy of the bunch.
 * It's equivalent to Acid Blast, but more situational due to the alignment checks.
 * Like Harm, this gives clerics some firepower but at a price.
 */
void spell_exorcise(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (ch->is_pc() && (victim->alignment > (ch->alignment + 100))) {
        victim = ch;
        ch->send_line("Your exorcism turns upon you!");
    }

    ch->alignment = std::min(Alignment::Angelic, static_cast<sh_int>(ch->alignment + 50));

    if (victim != ch) {
        act("$n calls forth the wrath of the Gods upon $N!", ch, nullptr, victim, To::NotVict);
        act("$n has assailed you with the wrath of the Gods!", ch, nullptr, victim, To::Vict);
        ch->send_line("You conjure forth the wrath of the Gods!");
    }

    auto dam_level = get_direct_dmg_and_level(level, AcidBlastExorciseDemonfireDamage);
    if ((ch->alignment <= victim->alignment + 100) && (ch->alignment >= victim->alignment - 100))
        dam_level.first /= 2;
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Holy);
}

/*
 * Demonfire is the 'evil' version of exorcise.
 */
void spell_demonfire(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (ch->is_pc() && (victim->alignment < (ch->alignment - 100))) {
        victim = ch;
        ch->send_line("The demons turn upon you!");
    }

    ch->alignment = std::max(Alignment::Satanic, static_cast<sh_int>(ch->alignment - 50));

    if (victim != ch) {
        act("$n calls forth the demons of Hell upon $N!", ch, nullptr, victim, To::Room);
        act("$n has assailed you with the demons of Hell!", ch, nullptr, victim, To::Vict);
        ch->send_line("You conjure forth the demons of hell!");
    }
    auto dam_level = get_direct_dmg_and_level(level, AcidBlastExorciseDemonfireDamage);
    if (ch->alignment <= victim->alignment + 100 && ch->alignment >= victim->alignment - 100)
        dam_level.first /= 2;
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Negative);
}

void spell_detect_evil(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("You already possess a greater awareness of evil.");
            else
                act("$N already possesses a greater awareness of evil.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.bitvector = to_int(AffectFlag::DetectEvil);
    affect_to_char(victim, af);
    victim->send_line("Your eyes tingle.");
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_detect_hidden(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("You already have superior awareness.");
            else
                act("$N already has superior awareness.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.bitvector = to_int(AffectFlag::DetectHidden);
    affect_to_char(victim, af);
    victim->send_line("Your awareness improves.");
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_detect_invis(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("Your vision is already augmented.");
            else
                act("$N already has superior vision.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.bitvector = to_int(AffectFlag::DetectInvis);
    affect_to_char(victim, af);
    victim->send_line("Your eyes tingle.");
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_detect_magic(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("You already have superior sensitivity to magic.");
            else
                act("$N already has superior sensitivity to magic.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.bitvector = to_int(AffectFlag::DetectMagic);
    affect_to_char(victim, af);
    victim->send_line("Your eyes tingle.");
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_detect_poison(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->type == ObjectType::Drink || obj->type == ObjectType::Food) {
        if (obj->value[3] != 0)
            ch->send_line("You smell poisonous fumes.");
        else
            ch->send_line("It looks delicious.");
    } else {
        ch->send_line("It doesn't look poisoned.");
    }
}

void spell_dispel_evil(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (ch->is_pc() && ch->is_evil())
        victim = ch;

    if (victim->is_good()) {
        act(fmt::format("{} protects $N", deity_name), ch, nullptr, victim, To::Char);
        return;
    }

    if (victim->is_neutral()) {
        act("$N does not seem to be affected.", ch, nullptr, victim, To::Char);
        return;
    }

    auto dam_level = get_direct_dmg_and_level(level, ColourSprayDispelGoodEvilDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Holy);
}

void spell_dispel_good(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (ch->is_pc() && ch->is_good())
        victim = ch;

    if (victim->is_evil()) {
        act(fmt::format("{} protects $N", deity_name), ch, nullptr, victim, To::Char);
        return;
    }

    if (victim->is_neutral()) {
        act("$N does not seem to be affected.", ch, nullptr, victim, To::Char);
        return;
    }
    auto dam_level = get_direct_dmg_and_level(level, ColourSprayDispelGoodEvilDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Holy);
}

void spell_dispel_magic(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (saves_spell(level, victim)) {
        victim->send_line("You feel a brief tingling sensation.");
        ch->send_line("You failed.");
        return;
    }
    const auto found = try_dispel_all_dispellables(level, victim);
    if (found)
        ch->send_line("Ok.");
    else
        ch->send_line("Spell failed.");
}

void spell_earthquake(int sn, int level, Char *ch, [[maybe_unused]] const SpellTarget &spell_target) {
    ch->send_line("The earth trembles beneath your feet!");
    act("$n makes the earth tremble and shiver.", ch);

    for (auto *vch : char_list) {
        if (vch->in_room == nullptr)
            continue;
        if (vch->in_room == ch->in_room) {
            if (vch != ch && !is_safe_spell(ch, vch, true)) {
                if (vch->is_aff_fly())
                    damage(ch, vch, 0, &skill_table[sn], DamageType::Bash);
                else
                    damage(ch, vch, level + dice(2, 8), &skill_table[sn], DamageType::Bash);
            }
            continue;
        }

        if (vch->in_room->area == ch->in_room->area)
            vch->send_line("The earth trembles and shivers.");
    }
}

void spell_remove_invisible(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->wear_loc != Wear::None) {
        ch->send_line("You have to be carrying it to remove invisible on it!");
        return;
    }

    if (!check_enum_bit(obj->extra_flags, ObjectExtraFlag::Invis)) {
        ch->send_line("That object is not invisible!");
        return;
    }

    const auto base_level_diff = 30 + std::clamp((ch->level - obj->level), -20, 20);
    const auto material_resilience_bonus = Material::get_magical_resilience(obj->material) / 2;
    const auto chance = std::clamp(base_level_diff + material_resilience_bonus, 5, 100) - number_percent();

    if (chance >= 0) {
        act("$p appears from nowhere!", ch, obj, nullptr, To::Room);
        act("$p appears from nowhere!", ch, obj, nullptr, To::Char);
        clear_enum_bit(obj->extra_flags, ObjectExtraFlag::Invis);
    }

    if ((chance >= -20) && (chance < 0)) {
        act("$p becomes semi-solid for a moment - then disappears.", ch, obj, nullptr, To::Room);
        act("$p becomes semi-solid for a moment - then disappears.", ch, obj, nullptr, To::Char);
    }

    if ((chance < -20)) {
        act("$p evaporates under the magical strain", ch, obj, nullptr, To::Room);
        act("$p evaporates under the magical strain", ch, obj, nullptr, To::Char);
        extract_obj(obj);
    }
}

void spell_remove_alignment(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->wear_loc != Wear::None) {
        ch->send_line("You have to be carrying it to remove alignment on it!");
        return;
    }

    if ((!check_enum_bit(obj->extra_flags, ObjectExtraFlag::AntiEvil))
        && (!check_enum_bit(obj->extra_flags, ObjectExtraFlag::AntiGood))
        && (!check_enum_bit(obj->extra_flags, ObjectExtraFlag::AntiNeutral))) {
        ch->send_line("That object has no alignment!");
        return;
    }

    const auto base_level_diff = std::clamp((ch->level - obj->level), -20, 20);
    const auto chance = std::clamp(base_level_diff / 2 + Material::get_magical_resilience(obj->material), 5, 100);
    const auto score = chance - number_percent();

    if ((score <= 20)) {
        act("The powerful nature of $n's spell removes some of $m alignment!", ch, obj, nullptr, To::Room);
        act("Your spell removes some of your alignment!", ch, obj, nullptr, To::Char);
        if (ch->is_good())
            ch->alignment -= (30 - score);
        if (ch->is_evil())
            ch->alignment += (30 - score);
        ch->alignment = std::clamp(ch->alignment, static_cast<sh_int>(Alignment::Satanic), Alignment::Angelic);
    }

    if (score >= 0) {
        act("$p glows grey.", ch, obj, nullptr, To::Room);
        act("$p glows grey.", ch, obj, nullptr, To::Char);
        clear_enum_bit(obj->extra_flags, ObjectExtraFlag::AntiGood);
        clear_enum_bit(obj->extra_flags, ObjectExtraFlag::AntiEvil);
        clear_enum_bit(obj->extra_flags, ObjectExtraFlag::AntiNeutral);
    } else if (score < -40) {
        act("$p shivers violently and explodes!", ch, obj, nullptr, To::Room);
        act("$p shivers violently and explodes!", ch, obj, nullptr, To::Char);
        extract_obj(obj);
    } else {
        act("$p appears unaffected.", ch, obj, nullptr, To::Room);
        act("$p appears unaffected.", ch, obj, nullptr, To::Char);
    }
}

void spell_enchant_armor(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->type != ObjectType::Armor) {
        ch->send_line("That isn't an armor.");
        return;
    }

    if (obj->wear_loc != Wear::None) {
        ch->send_line("The item must be carried to be enchanted.");
        return;
    }

    /* this means they have no bonus */
    int ac_bonus = 0;
    int fail = 15; /* base 15% chance of failure */
    /* TheMoog added material fiddling */
    fail += ((100 - Material::get_magical_resilience(obj->material)) / 3);

    /* find the bonuses */
    bool ac_found = false;
    if (!obj->enchanted)
        for (auto &af : obj->objIndex->affected) {
            if (af.location == AffectLocation::Ac) {
                ac_bonus = af.modifier;
                ac_found = true;
                fail += 5 * (ac_bonus * ac_bonus);
            }

            else /* things get a little harder */
                fail += 20;
        }

    for (auto &af : obj->affected) {
        if (af.location == AffectLocation::Ac) {
            ac_bonus = af.modifier;
            ac_found = true;
            fail += 5 * (ac_bonus * ac_bonus);
        }

        else /* things get a little harder */
            fail += 20;
    }

    /* apply other modifiers */
    fail -= level;

    if (obj->is_blessed())
        fail -= 15;
    if (obj->is_glowing())
        fail -= 5;

    fail = std::clamp(fail, 5, 95);

    const int result = number_percent();

    /* the moment of truth */
    if (result < (fail / 5)) { /* item destroyed */
        act("$p flares blindingly... and evaporates!", ch, obj, nullptr, To::Char);
        act("$p flares blindingly... and evaporates!", ch, obj, nullptr, To::Room);
        extract_obj(obj);
        return;
    }

    if (result < (fail / 2)) { /* item disenchanted */
        act("$p glows brightly, then fades...oops.", ch, obj, nullptr, To::Char);
        act("$p glows brightly, then fades.", ch, obj, nullptr, To::Room);
        obj->enchanted = true;

        /* remove all affects */
        obj->affected.clear();

        /* clear all flags */
        obj->extra_flags = 0;
        return;
    }

    if (result <= fail) { /* failed, no bad result */
        ch->send_line("Nothing seemed to happen.");
        return;
    }

    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted) {
        obj->enchanted = true;

        for (auto af_clone : obj->objIndex->affected) {
            af_clone.type = std::max(0_s, af_clone.type);
            obj->affected.add(af_clone);
        }
    }

    int added;
    if (result <= (100 - level / 5)) { /* success! */
        act("$p shimmers with a gold aura.", ch, obj, nullptr, To::Char);
        act("$p shimmers with a gold aura.", ch, obj, nullptr, To::Room);
        set_enum_bit(obj->extra_flags, ObjectExtraFlag::Magic);
        added = -1;
    } else { /* exceptional enchant */
        act("$p glows a brillant gold!", ch, obj, nullptr, To::Char);
        act("$p glows a brillant gold!", ch, obj, nullptr, To::Room);
        set_enum_bit(obj->extra_flags, ObjectExtraFlag::Magic);
        set_enum_bit(obj->extra_flags, ObjectExtraFlag::Glow);
        added = -2;
    }

    /* now add the enchantments */

    if (obj->level < LEVEL_HERO)
        obj->level = std::min(LEVEL_HERO - 1, obj->level + 1);

    if (ac_found) {
        for (auto &af : obj->affected) {
            if (af.location == AffectLocation::Ac) {
                af.type = sn;
                af.modifier += added;
                af.level = std::max(af.level, static_cast<sh_int>(level));
            }
        }
    } else { /* add a new affect */
        AFFECT_DATA af;
        af.type = sn;
        af.level = level;
        af.duration = -1;
        af.location = AffectLocation::Ac;
        af.modifier = added;
        af.bitvector = 0;
        obj->affected.add(af);
    }
}

void spell_enchant_weapon(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if ((obj->type != ObjectType::Weapon) && (obj->type != ObjectType::Armor)) {
        ch->send_line("That isn't a weapon or armour.");
        return;
    }

    if (obj->wear_loc != Wear::None) {
        ch->send_line("The item must be carried to be enchanted.");
        return;
    }

    /* this means they have no bonus */
    int hit_bonus = 0;
    int dam_bonus = 0;
    int modifier = 1;
    int fail = 15; /* base 15% chance of failure */

    /* Nice little touch for IMPs */
    if (ch->get_trust() == MAX_LEVEL)
        fail = -16535;

    /* TheMoog added material fiddling */
    fail += ((100 - Material::get_magical_resilience(obj->material)) / 3);
    if (obj->type == ObjectType::Armor) {
        fail += 25; /* harder to enchant armor with weapon */
        modifier = 2;
    }
    /* find the bonuses */

    bool hit_found = false, dam_found = false;
    if (!obj->enchanted) {
        for (auto &af : obj->objIndex->affected) {
            if (af.location == AffectLocation::Hitroll) {
                hit_bonus = af.modifier;
                hit_found = true;
                fail += 2 * (hit_bonus * hit_bonus) * modifier;
            }

            else if (af.location == AffectLocation::Damroll) {
                dam_bonus = af.modifier;
                dam_found = true;
                fail += 2 * (dam_bonus * dam_bonus) * modifier;
            }

            else /* things get a little harder */
                fail += 25 * modifier;
        }
    }

    for (auto &af : obj->affected) {
        if (af.location == AffectLocation::Hitroll) {
            hit_bonus = af.modifier;
            hit_found = true;
            fail += 2 * (hit_bonus * hit_bonus) * modifier;
        } else if (af.location == AffectLocation::Damroll) {
            dam_bonus = af.modifier;
            dam_found = true;
            fail += 2 * (dam_bonus * dam_bonus) * modifier;
        } else /* things get a little harder */
            fail += 25 * modifier;
    }

    /* apply other modifiers */
    fail -= 3 * level / 2;

    if (obj->is_blessed())
        fail -= 15;
    if (obj->is_glowing())
        fail -= 5;

    fail = std::clamp(fail, 5, 95);

    const int result = number_percent();

    /* We don't want armor, with more than 2 ench. hit&dam */
    int no_ench_num = 0;
    if (obj->type == ObjectType::Armor && (ch->get_trust() < MAX_LEVEL)) {
        if (auto it = ranges::find_if(
                obj->affected, [&](const auto &af) { return af.type == sn && af.location == AffectLocation::Damroll; });
            it != obj->affected.end()) {
            if (it->modifier >= 2)
                no_ench_num = number_range(1, 3);
        }
    }

    /* The moment of truth */
    if (result < (fail / 5) || (no_ench_num == 1)) { /* item destroyed */
        act("$p shivers violently and explodes!", ch, obj, nullptr, To::Char);
        act("$p shivers violently and explodes!", ch, obj, nullptr, To::Room);
        extract_obj(obj);
        return;
    }

    if (result < (fail / 2) || (no_ench_num == 2)) { /* item disenchanted */
        act("$p glows brightly, then fades...oops.", ch, obj, nullptr, To::Char);
        act("$p glows brightly, then fades.", ch, obj, nullptr, To::Room);
        obj->enchanted = true;

        /* remove all affects */
        obj->affected.clear();

        /* clear all flags */
        obj->extra_flags = 0;
        return;
    }

    if (result <= fail || (no_ench_num == 3)) { /* failed, no bad result */
        ch->send_line("Nothing seemed to happen.");
        return;
    }

    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted) {
        obj->enchanted = true;

        for (auto af_clone : obj->objIndex->affected) {
            af_clone.type = std::max(0_s, af_clone.type);
            obj->affected.add(af_clone);
        }
    }

    int added;
    if (result <= (100 - level / 5)) { /* success! */
        act("$p glows blue.", ch, obj, nullptr, To::Char);
        act("$p glows blue.", ch, obj, nullptr, To::Room);
        set_enum_bit(obj->extra_flags, ObjectExtraFlag::Magic);
        added = 1;
    } else { /* exceptional enchant */
        act("$p glows a brillant blue!", ch, obj, nullptr, To::Char);
        act("$p glows a brillant blue!", ch, obj, nullptr, To::Room);
        set_enum_bit(obj->extra_flags, ObjectExtraFlag::Magic);
        set_enum_bit(obj->extra_flags, ObjectExtraFlag::Glow);
        added = 2;
    }

    /* now add the enchantments */
    if (obj->level < LEVEL_HERO - 1)
        obj->level = std::min(LEVEL_HERO - 1, obj->level + 1);

    if (dam_found) {
        for (auto &af : obj->affected) {
            if (af.location == AffectLocation::Damroll) {
                af.type = sn;
                af.modifier += added;
                af.level = std::max(af.level, static_cast<sh_int>(level));
                if (af.modifier > 4)
                    set_enum_bit(obj->extra_flags, ObjectExtraFlag::Hum);
            }
        }
    } else { /* add a new affect */
        AFFECT_DATA af;
        af.type = sn;
        af.level = level;
        af.duration = -1;
        af.location = AffectLocation::Damroll;
        af.modifier = added;
        af.bitvector = 0;
        obj->affected.add(af);
    }

    if (hit_found) {
        for (auto &af : obj->affected) {
            if (af.location == AffectLocation::Hitroll) {
                af.type = sn;
                af.modifier += added;
                af.level = std::max(af.level, static_cast<sh_int>(level));
                if (af.modifier > 4)
                    set_enum_bit(obj->extra_flags, ObjectExtraFlag::Hum);
            }
        }
    } else { /* add a new affect */
        AFFECT_DATA af;
        af.type = sn;
        af.level = level;
        af.duration = -1;
        af.location = AffectLocation::Hitroll;
        af.modifier = added;
        af.bitvector = 0;
        obj->affected.add(af);
    }
    /* Make armour become level 50 */
    if ((obj->type == ObjectType::Armor) && (obj->level < 50)) {
        act("$p looks way better than it did before!", ch, obj, nullptr, To::Char);
        obj->level = 50;
    }
}

void spell_protect_container(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->type != ObjectType::Container) {
        ch->send_line("That isn't a container.");
        return;
    }

    if (obj->is_protect_container()) {
        act("$p is already protected!", ch, obj, nullptr, To::Char);
        return;
    }

    if (ch->gold < 50000) {
        ch->send_line("You need 50,000 gold coins to protect a container");
        return;
    }

    ch->gold -= 50000;
    set_enum_bit(obj->extra_flags, ObjectExtraFlag::ProtectContainer);
    act("$p is now protected!", ch, obj, nullptr, To::Char);
}

/*PGW A new group to give Barbarians a helping hand*/
void spell_vorpal(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)level;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->type != ObjectType::Weapon) {
        ch->send_line("This isn't a weapon.");
        return;
    }

    if (obj->wear_loc != Wear::None) {
        ch->send_line("The item must be carried to be made vorpal.");
        return;
    }

    const int mana = mana_for_spell(ch, sn);
    if (ch->is_npc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }

    ch->gold -= (mana * 100);
    set_enum_bit(obj->value[4], WeaponFlag::Vorpal);
    ch->send_line("You create a flaw in the universe and place it on your blade!");
}

void spell_venom(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)level;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->type != ObjectType::Weapon) {
        ch->send_line("That isn't a weapon.");
        return;
    }

    if (obj->wear_loc != Wear::None) {
        ch->send_line("The item must be carried to be venomed.");
        return;
    }

    const int mana = mana_for_spell(ch, sn);
    if (ch->is_npc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }
    ch->gold -= (mana * 100);
    set_enum_bit(obj->value[4], WeaponFlag::Poisoned);
    ch->send_line("You coat the blade in poison!");
}

void spell_black_death(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)level;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->type != ObjectType::Weapon) {
        ch->send_line("That isn't a weapon.");
        return;
    }

    if (obj->wear_loc != Wear::None) {
        ch->send_line("The item must be carried to be plagued.");
        return;
    }

    const int mana = mana_for_spell(ch, sn);
    if (ch->is_npc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }
    ch->gold -= (mana * 100);
    set_enum_bit(obj->value[4], WeaponFlag::Plagued);
    ch->send_line("Your use your cunning and skill to plague the weapon!");
}

void spell_damnation(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)level;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->type != ObjectType::Weapon) {
        ch->send_line("That isn't a weapon.");
        return;
    }

    if (obj->wear_loc != Wear::None) {
        ch->send_line("You do not have that item in your inventory.");
        return;
    }

    const int mana = mana_for_spell(ch, sn);
    if (ch->is_npc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }

    ch->gold -= (mana * 100);
    set_enum_bit(obj->extra_flags, ObjectExtraFlag::NoRemove);
    ch->send_line("You turn red in the face and curse your weapon into the pits of hell!");
}

void spell_vampire(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->type != ObjectType::Weapon) {
        ch->send_line("That isn't a weapon.");
        return;
    }

    if (obj->wear_loc != Wear::None) {
        ch->send_line("The item must be carried to be vampiric.");
        return;
    }

    if (ch->is_immortal()) {
        set_enum_bit(obj->value[4], WeaponFlag::Vampiric);
        ch->send_line("You suck the life force from the weapon leaving it hungry for blood.");
    }
}

void spell_tame_lightning(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)level;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    if (obj->type != ObjectType::Weapon) {
        ch->send_line("That isn't a weapon.");
        return;
    }

    if (obj->wear_loc != Wear::None) {
        ch->send_line("You do not have that item in your inventory.");
        return;
    }

    if (check_enum_bit(obj->value[4], WeaponFlag::Acid)) {
        ch->send_line("Acid and lightning do not mix.");
        return;
    }

    const int mana = mana_for_spell(ch, sn);
    if (ch->is_npc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }

    ch->gold -= (mana * 100);
    obj->value[3] = Attacks::index_of("shbite");
    set_enum_bit(obj->value[4], WeaponFlag::Lightning);
    ch->send_to("You summon a MASSIVE storm.\n\rHolding your weapon aloft you call lightning down from the sky. "
                "\n\rThe lightning swirls around it - you have |YTAMED|w the |YLIGHTNING|w.\n\r");
}

/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
void spell_energy_drain(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (saves_spell(level, victim)) {
        act("$N resists your magic through sheer force of will.", ch, nullptr, victim, To::Char);
        victim->send_line("You feel a momentary chill.");
        return;
    }
    ch->alignment = std::max(Alignment::Satanic, static_cast<sh_int>(ch->alignment - 50));
    int dam;
    if (victim->level <= 2) {
        dam = victim->hit + 1;
    } else {
        if (victim->in_room->vnum != Rooms::ChallengeArena)
            gain_exp(victim, 0 - 2.5 * number_range(level / 2, 3 * level / 2));
        victim->mana /= 2;
        victim->move /= 2;
        dam = dice(1, level);
        ch->hit += dam;
    }

    victim->send_line("You feel your life slipping away!");
    ch->send_line("Wow....what a rush!");
    damage(ch, victim, dam, &skill_table[sn], DamageType::Negative);
}

void spell_fireball(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    auto dam_level = get_direct_dmg_and_level(level, FireballHarmDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Fire);
}

/* Flamestrike is the Attack spell group's equivalent of fireball. It's slightly
 * less powerful and less mana efficient, largely as the cleric role is not
 * meant to specialize in damage dealing. See also Harm, which is similar to Fireball
 * in the Harmful group.
 */
void spell_flamestrike(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    auto dam_level = get_direct_dmg_and_level(level, FlamestrikeDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Fire);
}

void spell_faerie_fire(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)ch;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (victim->is_aff_faerie_fire()) {
        act("$N is already surrounded by a pink outline.", ch, nullptr, victim, To::Char);
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = AffectLocation::Ac;
    af.modifier = 2 * level;
    af.bitvector = to_int(AffectFlag::FaerieFire);
    affect_to_char(victim, af);
    victim->send_line("You are surrounded by a pink outline.");
    act("$n is surrounded by a pink outline.", victim);
}

void spell_faerie_fog(int sn, int level, Char *ch, [[maybe_unused]] const SpellTarget &spell_target) {
    (void)sn;

    act("$n conjures a cloud of purple smoke.", ch);
    ch->send_line("You conjure a cloud of purple smoke.");

    for (auto *ich : ch->in_room->people) {
        if (ich->is_pc() && check_enum_bit(ich->act, PlayerActFlag::PlrWizInvis))
            continue;
        if (ich->is_pc() && check_enum_bit(ich->act, PlayerActFlag::PlrProwl))
            continue;

        if (ich == ch || saves_spell(level, ich))
            continue;

        affect_strip(ich, gsn_invis);
        affect_strip(ich, gsn_mass_invis);
        affect_strip(ich, gsn_sneak);
        clear_enum_bit(ich->affected_by, AffectFlag::Hide);
        clear_enum_bit(ich->affected_by, AffectFlag::Invisible);
        clear_enum_bit(ich->affected_by, AffectFlag::Sneak);
        act("$n is revealed!", ich);
        ich->send_line("You are revealed!");
    }
}

void spell_fly(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)ch;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    AFFECT_DATA af;
    affect_strip(victim, sn);
    af.type = sn;
    af.level = level;
    af.duration = level + 3;
    af.bitvector = to_int(AffectFlag::Flying);
    affect_to_char(victim, af);
    victim->send_line("Your feet rise off the ground.");
    act("$n's feet rise off the ground.", victim);
}

/* RT clerical berserking spell */

void spell_frenzy(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (victim->is_affected_by(sn) || victim->is_aff_berserk()) {
        if (victim == ch)
            ch->send_line("You are already in a frenzy.");
        else
            act("$N is already in a frenzy.", ch, nullptr, victim, To::Char);
        return;
    }
    if (victim->is_aff_calm()) {
        if (victim == ch)
            ch->send_line("Why don't you just relax for a while?");
        else
            act("$N doesn't look like $E wants to fight anymore.", ch, nullptr, victim, To::Char);
        return;
    }
    if ((ch->is_good() && !victim->is_good()) || (ch->is_neutral() && !victim->is_neutral())
        || (ch->is_evil() && !victim->is_evil())) {
        act("Your god doesn't seem to like $N", ch, nullptr, victim, To::Char);
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level / 3;
    af.modifier = level / 6;
    af.bitvector = 0;

    af.location = AffectLocation::Hitroll;
    affect_to_char(victim, af);

    af.location = AffectLocation::Damroll;
    affect_to_char(victim, af);

    af.modifier = victim->level;
    af.location = AffectLocation::Ac;
    affect_to_char(victim, af);

    victim->send_line("You are filled with holy wrath!");
    act("$n gets a wild look in $s eyes!", victim);

    /*  if ( (wield !=nullptr) && (wield->type == ObjectType::Weapon) &&
          (check_bit(wield->value[4], WeaponFlaming)))
        ch->send_line("Your great energy causes your weapon to burst into flame.");
      wield->value[3] = 29;*/
}

/* RT ROM-style gate */

void spell_gate(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    std::string_view arguments = spell_target.getArguments();
    Char *victim;
    bool gate_pet;
    if (arguments.empty()) {
        ch->send_line("To whom do you wish to gate?");
        return;
    }
    if ((victim = get_char_world(ch, arguments)) == nullptr || victim == ch || victim->in_room == nullptr
        || !can_see_room(ch, victim->in_room) || check_enum_bit(victim->in_room->room_flags, RoomFlag::Safe)
        || check_enum_bit(victim->in_room->room_flags, RoomFlag::Private)
        || check_enum_bit(victim->in_room->room_flags, RoomFlag::Solitary)
        || check_enum_bit(victim->in_room->room_flags, RoomFlag::NoRecall)
        || check_enum_bit(ch->in_room->room_flags, RoomFlag::NoRecall) || victim->level >= level + 3
        || (victim->is_pc() && victim->level >= LEVEL_HERO) /* NOT trust */
        || (victim->is_npc() && check_enum_bit(victim->imm_flags, ToleranceFlag::Summon))
        || (victim->is_pc() && check_enum_bit(victim->act, PlayerActFlag::PlrNoSummon))
        || (victim->is_npc() && saves_spell(level, victim))) {
        ch->send_line("You failed.");
        return;
    }

    if (ch->pet != nullptr && ch->in_room == ch->pet->in_room)
        gate_pet = true;
    else
        gate_pet = false;

    act("$n steps through a gate and vanishes.", ch);
    ch->send_line("You step through a gate and vanish.");
    char_from_room(ch);
    char_to_room(ch, victim->in_room);

    act("$n has arrived through a gate.", ch);
    look_auto(ch);

    if (gate_pet && (ch->pet->in_room != ch->in_room)) {
        act("$n steps through a gate and vanishes.", ch->pet);
        ch->pet->send_to("You step through a gate and vanish.\n\r");
        char_from_room(ch->pet);
        char_to_room(ch->pet, victim->in_room);
        act("$n has arrived through a gate.", ch->pet);
        look_auto(ch->pet);
    }
}

void spell_giant_strength(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    AFFECT_DATA af;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("You are already as strong as you can get!");
            else
                act("$N can't get any stronger.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = AffectLocation::Str;
    af.modifier = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = 0;
    affect_to_char(victim, af);
    victim->send_line("Your muscles surge with heightened power!");
    act("$n's muscles surge with heightened power.", victim);
}

// Clerics can opt in to the Harm group fairly cheaply. They'll get harm at level 23
// vs mages getting Fireball at 22. Also, Harm is less mana efficient.
void spell_harm(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    auto dam_level = get_direct_dmg_and_level(level, FireballHarmDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Fire);
}

void spell_regeneration(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("You are already regenerating with great rapidity.");
            else
                act("$N is already regenerating with great rapidity.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    if (victim == ch)
        af.duration = level / 2;
    else
        af.duration = level / 4;
    af.bitvector = to_int(AffectFlag::Regeneration);
    affect_to_char(victim, af);
    victim->send_line("You feel vibrant!");
    act("$n is feeling more vibrant.", victim);
    if (ch != victim)
        ch->send_line("Ok.");
}

/* RT haste spell */

void spell_haste(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    AFFECT_DATA af;
    affect_strip(victim, sn);
    af.type = sn;
    af.level = level;
    if (victim == ch)
        af.duration = level / 2;
    else
        af.duration = level / 4;
    af.location = AffectLocation::Dex;
    af.modifier = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = to_int(AffectFlag::Haste);
    affect_to_char(victim, af);
    victim->send_line("You feel yourself moving more quickly.");
    act("$n is moving more quickly.", victim);
    if (ch != victim)
        ch->send_line("Ok.");
}

/* Moog's insanity spell */
void spell_insanity(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (victim->is_affected_by(sn)) {
        if (victim == ch)
            ch->send_line("You're mad enough!");
        else
            ch->send_line("They're as insane as they can be!");
        return;
    }

    if (saves_spell(level, victim)) {
        act("$N is unaffected.", ch, nullptr, victim, To::Char);
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = 5;
    af.location = AffectLocation::Wis;
    af.modifier = -(1 + (level >= 20) + (level >= 30) + (level >= 50) + (level >= 75) + (level >= 91));
    af.bitvector = 0;

    affect_to_char(victim, af);
    af.location = AffectLocation::Int;
    affect_to_char(victim, af);
    if (ch != victim)
        act("$N suddenly appears very confused.", ch, nullptr, victim, To::Char);
    victim->send_line("You feel a cloud of confusion and madness descend upon you.");
}

/* PGW  lethargy spell */

void spell_lethargy(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (victim->is_affected_by(sn) || victim->is_aff_lethargy()
        || check_enum_bit(victim->off_flags, OffensiveFlag::Slow)) {
        if (victim == ch)
            ch->send_line("Your heart beat is as low as it can go!");
        else
            act("$N has a slow enough heart-beat already.", ch, nullptr, victim, To::Char);
        return;
    }
    // The bonus makes it harder to resist than most spells.
    if (victim != ch && saves_spell(level + 10, victim)) {
        act("$N resists your magic through sheer force of will.", ch, nullptr, victim, To::Char);
        victim->send_line("You feel slower momentarily but it passes.");
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    if (victim == ch)
        af.duration = level / 2;
    else
        af.duration = level / 4;
    af.location = AffectLocation::Dex;
    af.modifier = -(1 + (level >= 18) + (level >= 25) + (level >= 32));
    af.bitvector = to_int(AffectFlag::Lethargy);
    affect_to_char(victim, af);
    victim->send_line("You feel your heart-beat slowing.");
    act("$n slows nearly to a stand-still.", victim);
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_heal(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const sh_int adjusted = victim->hit + (2 * (dice(3, 8) + level - 6));
    victim->hit = std::min(adjusted, victim->max_hit);
    update_pos(victim);
    victim->send_line("A warm feeling fills your body.");
    if (ch != victim)
        ch->send_line("Ok.");
    else
        act("$n glows with warmth.", ch, nullptr, victim, To::NotVict);
}

// Holy Word is an area of effect spell.
void spell_holy_word(int sn, int level, Char *ch, [[maybe_unused]] const SpellTarget &spell_target) {
    int dam;
    int bless_num, curse_num, frenzy_num;

    bless_num = skill_lookup("bless");
    curse_num = skill_lookup("curse");
    frenzy_num = skill_lookup("frenzy");

    act("$n utters a word of divine power!", ch);
    ch->send_line("You utter a word of divine power.");

    for (auto *vch : ch->in_room->people) {
        auto aoe_spell_target = SpellTarget(vch);
        if ((ch->is_good() && vch->is_good()) || (ch->is_evil() && vch->is_evil())
            || (ch->is_neutral() && vch->is_neutral())) {
            vch->send_line("You feel full more powerful.");
            spell_frenzy(frenzy_num, level, ch, aoe_spell_target);
            spell_bless(bless_num, level, ch, aoe_spell_target);
        }

        else if ((ch->is_good() && vch->is_evil()) || (ch->is_evil() && vch->is_good())) {
            if (!is_safe_spell(ch, vch, true)) {
                spell_curse(curse_num, level, ch, aoe_spell_target);
                vch->send_line("You are struck down!");
                dam = dice(level, 14);
                damage(ch, vch, dam, &skill_table[sn], DamageType::Energy);
            }
        }

        else if (ch->is_neutral()) {
            if (!is_safe_spell(ch, vch, true)) {
                spell_curse(curse_num, level / 2, ch, aoe_spell_target);
                vch->send_line("You are struck down!");
                dam = dice(level, 10);
                damage(ch, vch, dam, &skill_table[sn], DamageType::Energy);
            }
        }
    }

    ch->send_line("You feel drained.");
    ch->move = 10;
    ch->hit = (ch->hit * 3) / 4;
}

void spell_identify(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Object *obj = spell_target.getObject();
    if (!obj)
        return;
    ch->send_line("Object '{}' is type {}, extra flags {}.", obj->name, obj->type_name(),
                  format_set_flags(ObjectExtraFlags::AllObjectExtraFlags, ch, obj->extra_flags));
    ch->send_line("Weight is {}, value is {}, level is {}.", obj->weight, obj->cost, obj->level);

    if ((obj->material != Material::Type::None) && (obj->material != Material::Type::Default)) {
        ch->send_line("Made of {}.", lower_case(magic_enum::enum_name<Material::Type>(obj->material)));
    }

    switch (obj->type) {
    case ObjectType::Scroll:
    case ObjectType::Potion:
    case ObjectType::Pill:
    case ObjectType::Bomb:
        ch->send_to("Level {} spells of:", obj->value[0]);

        if (obj->value[1] >= 0 && obj->value[1] < MAX_SKILL) {
            ch->send_to(" '{}'", skill_table[obj->value[1]].name);
        }

        if (obj->value[2] >= 0 && obj->value[2] < MAX_SKILL) {
            ch->send_to(" '{}'", skill_table[obj->value[2]].name);
        }

        if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL) {
            ch->send_to(" '{}'", skill_table[obj->value[3]].name);
        }

        if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL && obj->type == ObjectType::Bomb) {
            ch->send_to(" '{}'", skill_table[obj->value[4]].name);
        }

        ch->send_line(".");
        break;

    case ObjectType::Wand:
    case ObjectType::Staff:
        ch->send_to("Has {}({}) charges of level {}", obj->value[1], obj->value[2], obj->value[0]);

        if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL) {
            ch->send_to(" '{}'", skill_table[obj->value[3]].name);
        }

        ch->send_line(".");
        break;

    case ObjectType::Container: ch->send_line("Weight capacity: {}.", obj->value[0]); break;

    case ObjectType::Drink: ch->send_line("Liquid capacity: {}.", obj->value[0]); break;

    case ObjectType::Weapon:
        ch->send_line("Weapon type is {}.", Weapons::name_from_integer(obj->value[0]));
        if ((obj->value[4] != 0) && (obj->type == ObjectType::Weapon)) {
            ch->send_to("Weapon flags:");
            if (check_enum_bit(obj->value[4], WeaponFlag::Flaming))
                ch->send_to(" flaming");
            if (check_enum_bit(obj->value[4], WeaponFlag::Frost))
                ch->send_to(" frost");
            if (check_enum_bit(obj->value[4], WeaponFlag::Vampiric))
                ch->send_to(" vampiric");
            if (check_enum_bit(obj->value[4], WeaponFlag::Sharp))
                ch->send_to(" sharp");
            if (check_enum_bit(obj->value[4], WeaponFlag::Vorpal))
                ch->send_to(" vorpal");
            if (check_enum_bit(obj->value[4], WeaponFlag::TwoHands))
                ch->send_to(" two-handed");
            if (check_enum_bit(obj->value[4], WeaponFlag::Poisoned))
                ch->send_to(" poisoned");
            if (check_enum_bit(obj->value[4], WeaponFlag::Plagued))
                ch->send_to(" death");
            if (check_enum_bit(obj->value[4], WeaponFlag::Acid))
                ch->send_to(" acid");
            if (check_enum_bit(obj->value[4], WeaponFlag::Lightning))
                ch->send_to(" lightning");
            ch->send_line(".");
        }
        ch->send_line("Damage is {}d{} (average {}).", obj->value[1], obj->value[2],
                      (1 + obj->value[2]) * obj->value[1] / 2);
        break;

    case ObjectType::Armor:
        ch->send_line("Armor class is {} pierce, {} bash, {} slash, and {} vs. magic.", obj->value[0], obj->value[1],
                      obj->value[2], obj->value[3]);
        break;
    default:; // TODO #259 support more types
    }

    if (!obj->enchanted)
        for (auto &af : obj->objIndex->affected) {
            if (af.affects_stats())
                ch->send_line("Affects {}.", af.describe_item_effect());
        }

    for (auto &af : obj->affected) {
        if (af.affects_stats())
            ch->send_line("Affects {}.", af.describe_item_effect());
    }
}

void spell_infravision(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    AFFECT_DATA af;
    affect_strip(victim, sn);
    act("$n's eyes glow red.\n\r", ch);
    af.type = sn;
    af.level = level;
    af.duration = 2 * level;
    af.bitvector = to_int(AffectFlag::Infrared);
    affect_to_char(victim, af);
    victim->send_line("Your eyes glow red.");
}

void spell_invis(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)ch;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (victim->is_aff_invisible())
        return;

    act("$n fades out of existence.", victim);
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.bitvector = to_int(AffectFlag::Invisible);
    affect_to_char(victim, af);
    victim->send_line("You fade out of existence.");
}

void spell_know_alignment(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const char *msg;
    int ap = victim->alignment;
    if (ap >= Alignment::Saintly)
        msg = "$N has a pure and good aura.";
    else if (ap >= Alignment::Amiable)
        msg = "$N is of excellent moral character.";
    else if (ap >= Alignment::Kind)
        msg = "$N is often kind and thoughtful.";
    else if (ap >= Alignment::Neutral)
        msg = "$N doesn't have a firm moral commitment.";
    else if (ap >= Alignment::Depraved)
        msg = "$N lies to $S friends.";
    else if (ap >= Alignment::Demonic)
        msg = "$N is a black-hearted murderer.";
    else
        msg = "$N is the embodiment of pure evil!.";

    act(msg, ch, nullptr, victim, To::Char);
}

void spell_lightning_bolt(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    auto dam_level = get_direct_dmg_and_level(level, LightningBoltCauseCritDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Lightning);
}

void spell_locate_object(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    std::string_view arguments = spell_target.getArguments();
    bool found = false;
    int number = 0;
    int max_found = ch->is_immortal() ? 200 : 2 * level;
    std::string buffer;
    for (auto *obj : object_list) {
        if (!ch->can_see(*obj) || !is_name(arguments, obj->name) || (ch->is_mortal() && number_percent() > 2 * level)
            || ch->level < obj->level || check_enum_bit(obj->extra_flags, ObjectExtraFlag::NoLocate))
            continue;

        found = true;
        number++;

        auto *in_obj = obj;
        for (; in_obj->in_obj != nullptr; in_obj = in_obj->in_obj)
            ;

        if (in_obj->carried_by != nullptr /*&& can_see TODO wth was this supposed to be?*/) {
            if (ch->is_immortal()) {
                buffer += fmt::format("{} carried by {} in {} [Room {}]\n\r", InitialCap{obj->short_descr},
                                      ch->describe(*in_obj->carried_by), in_obj->carried_by->in_room->name,
                                      in_obj->carried_by->in_room->vnum);
            } else {
                buffer += fmt::format("{} carried by {}\n\r", InitialCap{obj->short_descr},
                                      ch->describe(*in_obj->carried_by));
            }
        } else {
            if (ch->is_immortal() && in_obj->in_room != nullptr)
                buffer += fmt::format("{} in {} [Room {}]\n\r", InitialCap{obj->short_descr}, in_obj->in_room->name,
                                      in_obj->in_room->vnum);
            else
                buffer += fmt::format("{} in {}\n\r", InitialCap{obj->short_descr},
                                      in_obj->in_room == nullptr ? "somewhere" : in_obj->in_room->name);
        }

        if (number >= max_found)
            break;
    }

    if (!found)
        ch->send_line("Nothing like that in heaven or earth.");
    else if (ch->lines)
        ch->page_to(buffer);
    else
        ch->send_to(buffer);
}

void spell_magic_missile(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    auto dam_level = get_direct_dmg_and_level(level, MagicMissileCauseLightDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Energy);
}

// Mass Healing is an area of effect spell.
void spell_mass_healing(int sn, int level, Char *ch, [[maybe_unused]] const SpellTarget &spell_target) {
    (void)sn;
    int heal_num, refresh_num;

    heal_num = skill_lookup("heal");
    refresh_num = skill_lookup("refresh");

    for (auto *gch : ch->in_room->people) {
        auto aoe_spell_target = SpellTarget(gch);
        if ((ch->is_npc() && gch->is_npc()) || (ch->is_pc() && gch->is_pc())) {
            spell_heal(heal_num, level, ch, aoe_spell_target);
            spell_refresh(refresh_num, level, ch, aoe_spell_target);
        }
    }
}

void spell_mass_invis(int sn, int level, Char *ch, [[maybe_unused]] const SpellTarget &spell_target) {
    AFFECT_DATA af;
    for (auto *gch : ch->in_room->people) {
        if (!is_same_group(gch, ch) || gch->is_aff_invisible())
            continue;
        act("$n slowly fades out of existence.", gch);
        gch->send_line("You slowly fade out of existence.");
        af.type = sn;
        af.level = level / 2;
        af.duration = 24;
        af.bitvector = to_int(AffectFlag::Invisible);
        affect_to_char(gch, af);
    }
    ch->send_line("Ok.");
}

void spell_null(int sn, int level, Char *ch, [[maybe_unused]] const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    ch->send_line("That's not a spell!");
}

void spell_octarine_fire(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)ch;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (victim->is_aff_octarine_fire())
        return;
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = 2;
    af.location = AffectLocation::Ac;
    af.modifier = 10 * level;
    af.bitvector = to_int(AffectFlag::OctarineFire);
    affect_to_char(victim, af);
    victim->send_line("You are surrounded by an octarine outline.");
    act("$n is surrounded by a octarine outline.", victim);
}

void spell_pass_door(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("You are already shifted out of phase to a greater degree.");
            else
                act("$N is already shifted out of phase to a greater degree.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = number_fuzzy(level / 4);
    af.bitvector = to_int(AffectFlag::PassDoor);
    affect_to_char(victim, af);
    act("$n turns translucent.", victim);
    victim->send_line("You turn translucent.");
}

/* RT plague spell, very nasty */

void spell_plague(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (saves_spell(level, victim) || (victim->is_npc() && check_enum_bit(victim->act, CharActFlag::Undead))) {
        if (ch == victim)
            ch->send_line("You feel momentarily ill, but it passes.");
        else
            act("$N seems to be unaffected.", ch, nullptr, victim, To::Char);
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level * 3 / 4;
    af.duration = level;
    af.location = AffectLocation::Str;
    af.modifier = -5;
    af.bitvector = to_int(AffectFlag::Plague);
    affect_join(victim, af);

    victim->send_line("You scream in agony as plague sores erupt from your skin.");
    act("$n screams in agony as plague sores erupt from $s skin.", victim);
}

void spell_portal(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    std::string_view arguments = spell_target.getArguments();
    const auto *victim = get_char_world(ch, arguments);
    if (!victim || victim == ch || victim->in_room == nullptr || !can_see_room(ch, victim->in_room)
        || check_enum_bit(victim->in_room->room_flags, RoomFlag::Safe)
        || check_enum_bit(victim->in_room->room_flags, RoomFlag::Private)
        || check_enum_bit(victim->in_room->room_flags, RoomFlag::Solitary)
        || check_enum_bit(victim->in_room->room_flags, RoomFlag::NoRecall)
        || check_enum_bit(ch->in_room->room_flags, RoomFlag::NoRecall)
        || check_enum_bit(victim->in_room->room_flags, RoomFlag::Law) || victim->level >= level + 3
        || (victim->is_pc() && victim->level >= LEVEL_HERO) /* NOT trust */
        || (victim->is_npc() && check_enum_bit(victim->imm_flags, ToleranceFlag::Summon))
        || (victim->is_pc() && check_enum_bit(victim->act, PlayerActFlag::PlrNoSummon))
        || (victim->is_npc() && saves_spell(level, victim))) {
        ch->send_line("You failed.");
        return;
    }
    if (check_enum_bit(ch->in_room->room_flags, RoomFlag::Law)) {
        ch->send_line("You cannot portal from this room.");
        return;
    }
    auto *source_portal = create_object(get_obj_index(Objects::Portal));
    source_portal->timer = (ch->level / 10);
    source_portal->destination = victim->in_room;

    // Put portal in current room.
    source_portal->description = fmt::sprintf(source_portal->description, victim->in_room->name);
    obj_to_room(source_portal, ch->in_room);

    // Create second portal.
    auto *dest_portal = create_object(get_obj_index(Objects::Portal));
    dest_portal->timer = (ch->level / 10);
    dest_portal->destination = ch->in_room;

    /* Put portal, in destination room, for this room */
    dest_portal->description = fmt::sprintf(dest_portal->description, ch->in_room->name);
    obj_to_room(dest_portal, victim->in_room);

    ch->send_line("You wave your hands madly, and create a portal.");
}

void spell_poison(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)ch;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (saves_spell(level, victim)) {
        act("$n turns slightly green, but it passes.", victim);
        victim->send_line("You feel momentarily ill, but it passes.");
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = AffectLocation::Str;
    af.modifier = -2;
    af.bitvector = to_int(AffectFlag::Poison);
    affect_join(victim, af);
    victim->send_line("You feel very sick.");
    act("$n looks very ill.", victim);
}

void spell_protection_evil(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("Greater magic already protects you from evil.");
            else
                act("$N is already protected from evil by more powerful magic.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.bitvector = to_int(AffectFlag::ProtectionEvil);
    affect_to_char(victim, af);
    victim->send_line("You feel protected from evil.");
    if (ch != victim)
        act("$N is protected from harm.", ch, nullptr, victim, To::Char);
}

void spell_protection_good(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("Greater magic already protects you from good.");
            else
                act("$N is already protected from good by more powerful magic.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.bitvector = to_int(AffectFlag::ProtectionGood);
    affect_to_char(victim, af);
    victim->send_line("You feel protected from good.");
    if (ch != victim)
        act("$N is protected from harm.", ch, nullptr, victim, To::Char);
}

void spell_refresh(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const sh_int adjusted = victim->move + level;
    victim->move = std::min(adjusted, victim->max_move);
    if (victim->max_move == victim->move)
        victim->send_line("You feel fully refreshed!");
    else
        victim->send_line("You feel less tired.");
    if (ch != victim)
        act("$N looks invigorated.", ch, nullptr, victim, To::Char);
}

namespace {
void try_strip_noremove(const Char *victim, int level, Object *obj) {
    if (!saves_dispel(level, obj->level)) {
        clear_enum_bit(obj->extra_flags, ObjectExtraFlag::NoRemove);
        act("$p glows blue.", victim, obj, nullptr, To::Char);
        act("$p glows blue.", victim, obj, nullptr, To::Room);
    } else {
        act("$p whispers with the voice of an unclean spirit.", victim, obj, nullptr, To::Char);
        act("$p whispers with the voice of an unclean spirit.", victim, obj, nullptr, To::Room);
    }
}
}

void spell_remove_curse(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)ch;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (victim->is_affected_by(gsn_curse)) {
        if (check_dispel(level, victim, gsn_curse)) {
            victim->send_line("You feel better.");
            act("$n looks more relaxed.", victim);
        } else {
            act("$n still looks uncomfortable.", victim);
        }
        return;
    }
    for (const auto &wear : WearFilter::wearable()) {
        auto *obj = get_eq_char(victim, wear);
        if (obj && obj->is_no_remove()) {
            try_strip_noremove(victim, level, obj);
            return;
        }
    }

    for (auto *obj : victim->carrying) {
        if (obj->is_no_remove()) {
            try_strip_noremove(victim, level, obj);
            return;
        }
    }
    victim->send_line("You are not afflicted by a curse.");
    act("$n doesn't appear to be afflicted by a curse.", victim);
}

void spell_sanctuary(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("You are already shrouded in a scintillating aura.");
            else
                act("$N is already shrouded in a scintillating aura.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = number_fuzzy(level / 5);
    af.bitvector = to_int(AffectFlag::Sanctuary);
    affect_to_char(victim, af);
    act("$n is surrounded by a white aura.", victim);
    victim->send_line("You are surrounded by a white aura.");
}

void spell_talon(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("You talons are already as hard as they can be!");
            else
                act("$N already has hardened talons.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = number_fuzzy(level / 6);
    af.bitvector = to_int(AffectFlag::Talon);
    affect_to_char(victim, af);
    act("$n's hands become as strong as talons.", victim);
    victim->send_line("You hands become as strong as talons.");
}

void spell_shield(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("You are already affected by a resplendent force shield.");
            else
                act("$N is already protected by a resplendent force shield.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = 8 + level;
    af.location = AffectLocation::Ac;
    af.modifier = -20;
    af.bitvector = 0;
    affect_to_char(victim, af);
    act("$n is surrounded by a force shield.", victim);
    victim->send_line("You are surrounded by a force shield.");
}

void spell_shocking_grasp(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    auto dam_level = get_direct_dmg_and_level(level, ShockingGraspDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, &skill_table[sn], DamageType::Lightning);
}

void spell_sleep(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)ch;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (victim->is_aff_sleep() || (victim->is_npc() && check_enum_bit(victim->act, CharActFlag::Undead))
        || level < victim->level || saves_spell(level, victim))
        return;
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = 4 + level;
    af.bitvector = to_int(AffectFlag::Sleep);
    affect_join(victim, af);

    if (victim->is_pos_awake()) {
        victim->send_line("You feel very sleepy ..... zzzzzz.");
        act("$n goes to sleep.", victim);
        victim->position = Position::Type::Sleeping;
    }
}

void spell_stone_skin(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    const auto curr_affect = find_affect(victim, sn);
    if (curr_affect) {
        if (curr_affect->level > level) {
            if (victim == ch)
                ch->send_line("Your spell cannot harden your skin any further!");
            else
                act("$N already has hardened stone skin.", ch, nullptr, victim, To::Char);
            return;
        }
        affect_strip(victim, sn);
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = AffectLocation::Ac;
    af.modifier = -40;
    af.bitvector = 0;
    affect_to_char(victim, af);
    act("$n's skin turns to stone.", victim);
    victim->send_line("Your skin turns to stone.");
}

/*
 * improved by Faramir 7/8/96 because of various silly
 * summonings, like shopkeepers into Midgaard, thieves to the pit etc
 */

void spell_summon(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    std::string_view arguments = spell_target.getArguments();
    if (arguments.empty()) {
        ch->send_line("As your casting completes, you forget who, or what, you were aiming to summon!");
        return;
    }
    Char *victim;
    if ((victim = get_char_world(ch, arguments)) == nullptr || victim == ch || victim->in_room == nullptr
        || ch->in_room == nullptr || check_enum_bit(victim->in_room->room_flags, RoomFlag::Safe)
        || check_enum_bit(victim->in_room->room_flags, RoomFlag::Private)
        || check_enum_bit(victim->in_room->room_flags, RoomFlag::Solitary)
        || check_enum_bit(victim->in_room->room_flags, RoomFlag::NoRecall)
        || (victim->is_npc() && check_enum_bit(victim->act, CharActFlag::Aggressive)) || victim->level >= level + 3
        || (victim->is_pc() && victim->level >= LEVEL_HERO) || victim->fighting != nullptr
        || (victim->is_npc() && check_enum_bit(victim->imm_flags, ToleranceFlag::Summon))
        || (victim->is_pc() && check_enum_bit(victim->act, PlayerActFlag::PlrNoSummon))
        || (victim->is_npc() && saves_spell(level, victim))
        || (check_enum_bit(ch->in_room->room_flags, RoomFlag::Safe))) {
        ch->send_line("You failed.");
        return;
    }
    if (check_enum_bit(ch->in_room->room_flags, RoomFlag::Law)) {
        ch->send_line("You'd probably get locked behind bars for that!");
        return;
    }
    if (victim->is_npc()) {
        if (victim->mobIndex->shop || check_enum_bit(victim->act, CharActFlag::Healer)
            || check_enum_bit(victim->act, CharActFlag::Gain) || check_enum_bit(victim->act, CharActFlag::Practice)) {
            act("The guildspersons' convention prevents your summons.", ch, nullptr, nullptr, To::Char);
            act("The guildspersons' convention protects $n from summons.", victim);
            act("$n attempted to summon you!", ch, nullptr, victim, To::Vict);
            return;
        }
    }

    if (victim->riding != nullptr)
        unride_char(victim, victim->riding);
    act("$n disappears suddenly.", victim);
    char_from_room(victim);
    char_to_room(victim, ch->in_room);
    act("$n arrives suddenly.", victim);
    act("$n has summoned you!", ch, nullptr, victim, To::Vict);
    look_auto(victim);
}

void spell_teleport(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    Room *room;

    if (victim->in_room == nullptr || check_enum_bit(victim->in_room->room_flags, RoomFlag::NoRecall)
        || (ch->is_pc() && victim->fighting != nullptr) || (victim != ch && saves_spell(level, victim))) {
        ch->send_line("You failed.");
        return;
    }

    for (;;) {
        room = get_room(number_range(0, 65535));
        if (room != nullptr)
            if (can_see_room(ch, room) && !check_enum_bit(room->room_flags, RoomFlag::Private)
                && !check_enum_bit(room->room_flags, RoomFlag::Solitary))
                break;
    }

    if (victim != ch)
        victim->send_line("You have been teleported!");

    act("$n vanishes!", victim);
    char_from_room(victim);
    char_to_room(victim, room);
    if (!ch->riding) {
        act("$n slowly fades into existence.", victim);
    } else {
        act("$n slowly fades into existence, about 5 feet off the ground!", victim);
        act("$n falls to the ground with a thud!", victim);
        act("You fall to the ground with a thud!", victim, nullptr, nullptr, To::Char);
        fallen_off_mount(ch);
    } /* end ride check */
    look_auto(victim);
}

void spell_ventriloquate(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    std::string_view arguments = spell_target.getArguments();
    auto parser = ArgParser(arguments);
    auto first = parser.shift();
    const auto speaker_name = upper_first_character(first);
    const auto message = parser.remaining();
    const auto successful_speech = fmt::format("{} says '{}'.", speaker_name, message);
    const auto suspicious_speech = fmt::format("Someone makes {} say '{}'.", speaker_name, message);
    for (auto *vch : ch->in_room->people) {
        if (!is_name(speaker_name, vch->name))
            vch->send_line(saves_spell(level, vch) ? suspicious_speech : successful_speech);
    }
}

void spell_weaken(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)ch;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (victim->is_affected_by(sn) || saves_spell(level, victim)) {
        act("$n looks unsteady for a moment, but it passes.", victim);
        victim->send_line("You feel unsteady for a moment, but it passes.");
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level / 2;
    af.location = AffectLocation::Str;
    af.modifier = -1 * (level / 5);
    af.bitvector = to_int(AffectFlag::Weaken);
    affect_to_char(victim, af);
    victim->send_line("You feel weaker.");
    act("$n looks tired and weak.", victim);
}

/* RT recall spell is back */

void spell_word_of_recall(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    Room *location;

    if (victim->is_npc())
        return;

    if ((location = get_room(Rooms::MidgaardTemple)) == nullptr) {
        victim->send_line("You are completely lost.");
        return;
    }

    if (check_enum_bit(victim->in_room->room_flags, RoomFlag::NoRecall) || victim->is_aff_curse()) {
        victim->send_line("Spell failed.");
        return;
    }

    if (victim->fighting != nullptr)
        stop_fighting(victim, true);
    if ((victim->riding) && (victim->riding->fighting))
        stop_fighting(victim->riding, true);

    ch->move /= 2;
    if (victim->invis_level < HERO)
        act("$n disappears.", victim);
    if (victim->riding)
        char_from_room(victim->riding);
    char_from_room(victim);
    char_to_room(victim, location);
    if (victim->riding)
        char_to_room(victim->riding, location);
    if (victim->invis_level < HERO)
        act("$n appears in the room.", victim);
    look_auto(victim);
    if (ch->pet != nullptr)
        do_recall(ch->pet, ArgParser(""));
}

/*
 * NPC spells.
 */
void spell_acid_breath(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    int dam;
    int hpch;
    int i;

    if (number_percent() < 2 * level && !saves_spell(level, victim) && ch->in_room->vnum != Rooms::ChallengeArena) {
        for (auto *obj_lose : victim->carrying) {
            if (number_bits(2) != 0)
                continue;

            switch (obj_lose->type) {
            case ObjectType::Armor:
                if (obj_lose->value[0] > 0) {
                    Wear wear;
                    act("$p is pitted and etched!", victim, obj_lose, nullptr, To::Char);
                    if ((wear = obj_lose->wear_loc) != Wear::None)
                        for (i = 0; i < 4; i++)
                            victim->armor[i] -= apply_ac(obj_lose, wear, i);
                    for (i = 0; i < 4; i++)
                        obj_lose->value[i] -= 1;
                    obj_lose->cost = 0;
                    if (wear != Wear::None)
                        for (i = 0; i < 4; i++)
                            victim->armor[i] += apply_ac(obj_lose, wear, i);
                }
                break;

            case ObjectType::Container:
                if (!obj_lose->is_protect_container()) {
                    act("$p fumes and dissolves, destroying some of the contents.", victim, obj_lose, nullptr,
                        To::Char);
                    /* save some of  the contents */

                    for (auto *t_obj : obj_lose->contains) {
                        obj_from_obj(t_obj);

                        if (number_bits(2) == 0 || victim->in_room == nullptr)
                            extract_obj(t_obj);
                        else
                            obj_to_room(t_obj, victim->in_room);
                    }
                    extract_obj(obj_lose);
                    break;
                } else {
                    act("$p was protected from damage, saving the contents!", victim, obj_lose, nullptr, To::Char);
                }

            default:;
            }
        }
    }

    hpch = std::clamp(ch->hit, 10_s, 2000_s);
    if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && ch->is_pc())
        hpch = 1000;
    dam = number_range(hpch / 20 + 16, hpch / 15);
    if (saves_spell(level, victim))
        dam /= 2;
    damage(ch, victim, dam, &skill_table[sn], DamageType::Acid);
}

void spell_fire_breath(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    /* Limit damage for PCs added by Rohan on all draconian*/
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    int dam;
    int hpch;

    if (number_percent() < 2 * level && !saves_spell(level, victim) && ch->in_room->vnum != Rooms::ChallengeArena) {
        for (auto *obj_lose : victim->carrying) {
            const char *msg;

            if (number_bits(2) != 0)
                continue;

            switch (obj_lose->type) {
            default: continue;
            case ObjectType::Potion: msg = "$p bubbles and boils!"; break;
            case ObjectType::Scroll: msg = "$p crackles and burns!"; break;
            case ObjectType::Staff: msg = "$p smokes and chars!"; break;
            case ObjectType::Wand: msg = "$p sparks and sputters!"; break;
            case ObjectType::Food: msg = "$p blackens and crisps!"; break;
            case ObjectType::Pill: msg = "$p melts and drips!"; break;
            }

            act(msg, victim, obj_lose, nullptr, To::Char);
            if (obj_lose->type == ObjectType::Container) {
                /* save some of  the contents */

                if (!obj_lose->is_protect_container()) {
                    msg = "$p ignites and burns!";
                    act(msg, victim, obj_lose, nullptr, To::Char);

                    for (auto *t_obj : obj_lose->contains) {
                        obj_from_obj(t_obj);

                        if (number_bits(2) == 0 || ch->in_room == nullptr)
                            extract_obj(t_obj);
                        else
                            obj_to_room(t_obj, ch->in_room);
                    }
                } else {
                    act("$p was protected from damage, saving the contents!", victim, obj_lose, nullptr, To::Char);
                }
            }
            if (!obj_lose->is_protect_container())
                extract_obj(obj_lose);
        }
    }

    hpch = std::clamp(ch->hit, 10_s, 2000_s);
    if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && ch->is_pc())
        hpch = 1000;
    dam = number_range(hpch / 20 + 16, hpch / 15);
    if (saves_spell(level, victim))
        dam /= 2;
    damage(ch, victim, dam, &skill_table[sn], DamageType::Fire);
}

void spell_frost_breath(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    if (number_percent() < 2 * level && !saves_spell(level, victim) && ch->in_room->vnum != Rooms::ChallengeArena) {
        for (auto *obj_lose : victim->carrying) {
            const char *msg;

            if (number_bits(2) != 0)
                continue;

            switch (obj_lose->type) {
            default: continue;
            case ObjectType::Drink:
            case ObjectType::Potion: msg = "$p freezes and shatters!"; break;
            }

            act(msg, victim, obj_lose, nullptr, To::Char);
            extract_obj(obj_lose);
        }
    }

    int hpch = std::clamp(ch->hit, 10_s, 2000_s);
    if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && ch->is_pc())
        hpch = 1000;
    int dam = number_range(hpch / 20 + 16, hpch / 15);
    if (saves_spell(level, victim))
        dam /= 2;
    damage(ch, victim, dam, &skill_table[sn], DamageType::Cold);
}

void spell_gas_breath(int sn, int level, Char *ch, [[maybe_unused]] const SpellTarget &spell_target) {
    int dam;
    int hpch;

    for (auto *vch : ch->in_room->people) {
        if (!is_safe_spell(ch, vch, true)) {
            hpch = std::clamp(ch->hit, 10_s, 2000_s);
            if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && ch->is_pc())
                hpch = 1000;
            dam = number_range(hpch / 20 + 16, hpch / 15);
            if (saves_spell(level, vch))
                dam /= 2;
            damage(ch, vch, dam, &skill_table[sn], DamageType::Poison);
        }
    }
}

void spell_lightning_breath(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    int dam;
    int hpch;

    hpch = std::clamp(ch->hit, 10_s, 2000_s);
    if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && ch->is_pc())
        hpch = 1000;
    dam = number_range(hpch / 20 + 16, hpch / 15);
    if (saves_spell(level, victim))
        dam /= 2;
    damage(ch, victim, dam, &skill_table[sn], DamageType::Lightning);
}

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
void spell_general_purpose(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    int dam = number_range(level, level * 3);
    if (saves_spell(level, victim))
        dam /= 2;
    damage(ch, victim, dam, &skill_table[sn], DamageType::Pierce);
}

void spell_high_explosive(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    Char *victim = spell_target.getChar();
    if (!victim)
        return;
    int dam = number_range(level, level * 3);
    if (saves_spell(level, victim))
        dam /= 2;
    damage(ch, victim, dam, &skill_table[sn], DamageType::Pierce);
}

void explode_bomb(Object *bomb, Char *ch, Char *thrower) {
    int sn, position;
    const int chance = std::clamp((50 + ((bomb->value[0] - ch->level) * 8)), 5, 100);
    if (number_percent() > chance) {
        act("$p emits a loud bang and disappears in a cloud of smoke.", ch, bomb, nullptr, To::Room);
        act("$p emits a loud bang, but thankfully does not affect you.", ch, bomb, nullptr, To::Char);
        extract_obj(bomb);
        return;
    }
    auto spell_target = SpellTarget(ch);
    for (position = 1; ((position <= 4) && (bomb->value[position] < MAX_SKILL) && (bomb->value[position] != -1));
         position++) {
        sn = bomb->value[position];
        if (skill_table[sn].target == Target::CharOffensive)
            (*skill_table[sn].spell_fun)(sn, bomb->value[0], thrower, spell_target);
    }
    extract_obj(bomb);
}

void spell_teleport_object(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Char *victim;
    Object *object;
    Room *old_room;
    std::string_view arguments = spell_target.getArguments();
    auto parser = ArgParser(arguments);
    const auto obj_name = parser.shift();
    const auto dest_name = parser.remaining();
    if (obj_name.empty()) {
        ch->send_line("Teleport what and to whom?");
        return;
    }

    if ((object = ch->find_in_inventory(obj_name)) == nullptr) {
        ch->send_line("You're not carrying that.");
        return;
    }

    if (dest_name.empty()) {
        ch->send_line("Teleport to whom?");
        return;
    }

    if ((victim = get_char_world(ch, dest_name)) == nullptr) {
        ch->send_line("Teleport to whom?");
        return;
    }

    if (victim == ch) {
        ch->send_to("|CYou decide that you want to show off and you teleport an object\n\r|Cup to the highest heavens "
                    "and straight back into your inventory!\n\r|w");
        act("|C$n decides that $e wants to show off and $e teleports an object\n\r|Cup to the highest heavens and "
            "straight back into $s own inventory!|w",
            ch);
        return;
    }

    if (!can_see(ch, victim)) {
        ch->send_line("Teleport to whom?");
        return;
    }

    if (check_enum_bit(ch->in_room->room_flags, RoomFlag::NoRecall)) {
        ch->send_line("You failed.");
        return;
    }

    if (check_enum_bit(victim->in_room->room_flags, RoomFlag::NoRecall)) {
        ch->send_line("You failed.");
        return;
    }

    /* Check to see if the victim is actually already in the same room */
    if (ch->in_room != victim->in_room) {
        act("You feel a brief presence in the room.", victim, nullptr, nullptr, To::Char);
        act("You feel a brief presence in the room.", victim);
        old_room = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, victim->in_room);
        ch->try_give_item_to(object, victim);
        char_from_room(ch);
        char_to_room(ch, old_room);
    } else {
        ch->try_give_item_to(object, victim);
    }
}

void spell_undo_spell(int sn, int level, Char *ch, const SpellTarget &spell_target) {
    (void)sn;
    (void)level;
    Char *victim;
    int undo_spell_num;
    std::string_view arguments = spell_target.getArguments();
    auto parser = ArgParser(arguments);
    const auto spell_name = parser.shift();
    const auto dest_name = parser.remaining();
    if (dest_name.empty()) {
        victim = ch;
    } else {
        if ((victim = get_char_room(ch, dest_name)) == nullptr) {
            ch->send_line("They're not here.");
            return;
        }
    }

    if (spell_name.empty()) {
        ch->send_line("Undo which spell?");
        return;
    }

    if ((undo_spell_num = skill_lookup(spell_name)) < 0) {
        ch->send_line("What kind of spell is that?");
        return;
    }
    if (!victim->is_affected_by(undo_spell_num)) {
        if (victim == ch) {
            ch->send_line("You're not affected by that.");
        } else {
            act("$N doesn't seem to be affected by that.", ch, nullptr, victim, To::Char);
        }
        return;
    }
    const auto &undo_spell = skill_table[undo_spell_num];
    if (!undo_spell.dispel_fun) {
        ch->send_line("That spell cannot be undone.");
        return;
    }
    if (undo_spell.dispel_fun(ch->level, victim, undo_spell_num)) {
        act(undo_spell.dispel_victim_msg_to_room, victim);
    } else {
        ch->send_line("Spell failed.");
    }
}
