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
#include "Format.hpp"
#include "WeatherData.hpp"
#include "challeng.h"
#include "comm.hpp"
#include "handler.hpp"
#include "interp.h"
#include "lookup.h"
#include "merc.h"
#include "string_utils.hpp"

#include <fmt/format.h>
#include <range/v3/algorithm/find_if.hpp>

#include <cstdio>
#include <cstring>

/*
 * Local functions.
 */
void say_spell(Char *ch, int sn);
void explode_bomb(OBJ_DATA *bomb, Char *ch, Char *thrower);

namespace {

static constexpr std::array AcidBlastExorciseDemonfireDamage = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,   108, 110, 112, 114, 116, 118, 120, 122, 124, 126, 128, 130, 131, 132,
    133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 148, 149, 149, 150, 151,
    152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 166, 168, 170, 172, 174, 176, 178, 170,
    171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 188, 188, 189};

static constexpr std::array BurningHandsCauseSerDamage = {
    0,  0,  0,  0,  0,  14, 17, 20, 23, 26, 29, 29, 29, 30, 30, 31, 31, 32, 32, 33, 33, 34, 34,
    35, 35, 36, 36, 37, 37, 38, 38, 39, 39, 40, 40, 41, 41, 42, 42, 43, 43, 44, 44, 45, 45, 46,
    46, 47, 47, 48, 48, 48, 49, 50, 50, 51, 51, 52, 52, 53, 53, 54, 54, 55, 55, 56, 56, 56, 57,
    57, 57, 58, 58, 58, 59, 59, 59, 59, 60, 60, 61, 61, 62, 62, 62, 63, 64, 65, 66, 67, 68, 69};

static constexpr std::array ChillTouchDamage = {
    0,  0,  0,  6,  7,  8,  9,  12, 13, 13, 13, 14, 14, 14, 15, 15, 15, 16, 16, 16, 17, 17, 17,
    18, 18, 18, 19, 19, 19, 20, 20, 20, 21, 21, 21, 22, 22, 22, 23, 23, 23, 24, 24, 24, 25, 25,
    25, 26, 26, 26, 27, 27, 28, 29, 29, 30, 30, 31, 31, 32, 32, 33, 33, 34, 34, 35, 35, 35, 36,
    36, 36, 37, 37, 37, 38, 38, 38, 38, 39, 39, 40, 40, 41, 41, 41, 42, 43, 44, 45, 46, 47, 48};

static const std::array ColourSprayDispelGoodEvilDamage = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  30, 35, 40, 45, 50, 55, 55, 55, 56, 57, 58, 58,
    59, 60, 61, 61, 62, 63, 64, 64, 65, 66, 67, 67, 68, 69, 70, 70, 71, 72, 73, 73, 74, 75, 76,
    76, 77, 78, 79, 79, 79, 80, 81, 81, 82, 82, 83, 83, 84, 84, 85, 85, 86, 86, 87, 87, 87, 88,
    88, 88, 87, 87, 87, 88, 88, 88, 88, 89, 89, 90, 90, 91, 91, 91, 92, 93, 94, 95, 96, 97, 98};

static constexpr std::array FireballHarmDamage = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   30,  35,  40,  45,  50,  55,  60,
    65,  70,  75,  80,  82,  84,  86,  88,  90,  92,  94,  96,  98,  100, 102, 104, 106, 108, 110, 112, 114, 116,
    118, 120, 122, 124, 126, 128, 130, 131, 132, 133, 133, 134, 135, 136, 136, 137, 138, 138, 139, 140, 140, 141,
    142, 143, 144, 145, 146, 147, 148, 148, 149, 149, 150, 150, 151, 152, 153, 154, 155, 156, 156, 157, 158, 159,
    160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 170, 171, 171, 171, 171, 172, 172};

static constexpr std::array FlamestrikeDamage = {
    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   50,  55,
    60,  65,  70,  75,  80,  82,  84,  86,  88,  90,  92,  94,  96,  98,  100, 102, 104, 106, 108, 110, 112, 114, 116,
    118, 120, 122, 124, 126, 128, 130, 131, 132, 133, 133, 134, 135, 136, 136, 137, 138, 138, 139, 140, 140, 141, 141,
    142, 143, 143, 144, 144, 145, 145, 146, 146, 147, 147, 148, 148, 149, 149, 150, 151, 152, 153, 154, 155, 156};

static constexpr std::array LightningBoltCauseCritDamage = {
    0,  0,  0,  0,  0,  0,  0,  0,  0,  25, 28, 31, 34, 37, 40, 40, 41, 42, 42, 43, 44, 44, 45,
    46, 46, 47, 48, 48, 49, 50, 50, 51, 52, 52, 53, 54, 54, 55, 56, 56, 57, 58, 58, 59, 60, 60,
    61, 62, 62, 63, 64, 64, 65, 66, 66, 67, 67, 68, 68, 69, 69, 70, 70, 71, 71, 72, 72, 72, 73,
    73, 73, 74, 74, 74, 75, 75, 75, 75, 76, 76, 77, 77, 78, 78, 78, 79, 80, 81, 82, 83, 84, 85};

static constexpr std::array MagicMissileCauseLightDamage = {
    0,  3,  3,  4,  4,  5,  6,  6,  6,  6,  6,  7,  7,  7,  7,  7,  8,  8,  8,  8,  8,  9,  9,
    9,  9,  9,  10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 13, 13, 13, 13, 13,
    14, 14, 14, 14, 14, 15, 15, 16, 16, 17, 17, 18, 18, 19, 19, 20, 20, 21, 21, 22, 22, 22, 23,
    23, 23, 24, 24, 24, 25, 25, 25, 25, 26, 26, 26, 27, 27, 28, 28, 29, 30, 31, 32, 33, 34, 35};

static constexpr std::array ShockingGraspDamage = {
    0,  0,  0,  0,  0,  0,  0,  20, 25, 29, 33, 36, 39, 39, 39, 40, 40, 41, 41, 42, 42, 43, 43,
    44, 44, 45, 45, 46, 46, 47, 47, 48, 48, 49, 49, 50, 50, 51, 51, 52, 52, 53, 53, 54, 54, 55,
    55, 56, 56, 57, 57, 57, 58, 59, 59, 60, 60, 61, 61, 62, 62, 63, 63, 64, 64, 65, 65, 65, 66,
    66, 66, 67, 67, 67, 68, 68, 68, 68, 69, 69, 70, 70, 71, 71, 71, 72, 73, 74, 75, 76, 77, 78};

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

}

/*
 * Utter mystical words for an sn.
 */
void say_spell(Char *ch, int sn) {
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH * 2];
    char buf3[MAX_STRING_LENGTH];
    int length;

    struct syl_type {
        const char *old;
        const char *new_t;
    };

    static const struct syl_type syl_table[] = {
        {" ", " "},        {"ar", "abra"},    {"au", "kada"},   {"bless", "fido"}, {"blind", "nose"}, {"bur", "mosa"},
        {"cu", "judi"},    {"de", "oculo"},   {"en", "unso"},   {"light", "dies"}, {"lo", "hi"},      {"mor", "zak"},
        {"move", "sido"},  {"ness", "lacri"}, {"ning", "illa"}, {"per", "duda"},   {"ra", "gru"},     {"fresh", "ima"},
        {"gate", "imoff"}, {"re", "candus"},  {"son", "sabru"}, {"tect", "infra"}, {"tri", "cula"},   {"ven", "nofo"},
        {"oct", "bogusi"}, {"rine", "dude"},  {"a", "a"},       {"b", "b"},        {"c", "q"},        {"d", "e"},
        {"e", "z"},        {"f", "y"},        {"g", "o"},       {"h", "p"},        {"i", "u"},        {"j", "y"},
        {"k", "t"},        {"l", "r"},        {"m", "w"},       {"n", "i"},        {"o", "a"},        {"p", "s"},
        {"q", "d"},        {"r", "f"},        {"s", "g"},       {"t", "h"},        {"u", "j"},        {"v", "z"},
        {"w", "x"},        {"x", "n"},        {"y", "l"},       {"z", "k"},        {"", ""}};

    buf[0] = '\0';
    for (const char *pName = skill_table[sn].name; *pName != '\0'; pName += length) {
        for (int iSyl = 0; (length = strlen(syl_table[iSyl].old)) != 0; iSyl++) {
            if (!str_prefix(syl_table[iSyl].old, pName)) {
                strcat(buf, syl_table[iSyl].new_t);
                break;
            }
        }

        if (length == 0)
            length = 1;
    }

    snprintf(buf2, sizeof(buf2), "$n utters the words, '%s'.", buf);
    snprintf(buf, sizeof(buf), "$n utters the words, '%s'.", skill_table[sn].name);

    for (Char *rch = ch->in_room->people; rch; rch = rch->next_in_room) {
        if (rch->is_pc() && rch->pcdata->colour) {
            snprintf(buf3, sizeof(buf3), "%c[0;33m", 27);
            rch->send_to(buf3);
        }
        if (rch != ch)
            act(ch->class_num == rch->class_num ? buf : buf2, ch, nullptr, rch, To::Vict);
        if (rch->is_pc() && rch->pcdata->colour) {
            snprintf(buf3, sizeof(buf3), "%c[0;37m", 27);
            rch->send_to(buf3);
        }
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
    if (IS_AFFECTED(victim, AFF_BERSERK))
        save -= victim->level / 3;
    save = URANGE(5, save, 95);
    return number_percent() < save;
}

/* RT save for dispels */

bool saves_dispel(int dis_level, int spell_level, int duration) {
    if (duration == -1)
        spell_level += 5;
    /* very hard to dispel permanent effects */

    const int save = URANGE(5, 50 + (spell_level - dis_level) * 5, 95);
    return number_percent() < save;
}

bool check_dispel(int dis_level, Char *victim, int sn) {
    // This behaviour probably needs a look:
    // * multiple affects of the same SN get multiple chances of being dispelled.
    // * any succeeding chance strips _all_ affects of that SN.
    for (auto &af : victim->affected) {
        if (af.type != sn)
            continue;
        if (!saves_dispel(dis_level, af.level, af.duration)) {
            affect_strip(victim, sn);
            if (skill_table[sn].msg_off)
                victim->send_line("{}", skill_table[sn].msg_off);
            return true;
        }
        af.level--;
    }
    return false;
}

/* for finding mana costs -- temporary version */
int mana_cost(Char *ch, int min_mana, int level) {
    if (ch->level + 2 == level)
        return 1000;
    return UMAX(min_mana, (100 / (2 + ch->level - level)));
}

int mana_for_spell(Char *ch, int sn) {
    if (ch->level + 2 == get_skill_level(ch, sn))
        return 50;
    return UMAX(skill_table[sn].min_mana, 100 / (2 + ch->level - get_skill_level(ch, sn)));
}

/*
 * The kludgy global is for spells who want more stuff from command line.
 */
const char *target_name;

void do_cast(Char *ch, const char *argument) {
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    Char *victim;
    OBJ_DATA *obj;
    OBJ_DATA *potion;
    OBJ_INDEX_DATA *i_Potion;
    OBJ_DATA *scroll;
    OBJ_INDEX_DATA *i_Scroll;
    OBJ_DATA *bomb;
    void *vo;
    int sn;
    int pos;
    /* Switched NPC's can cast spells, but others can't. */
    if (ch->is_npc() && (ch->desc == nullptr))
        return;

    target_name = one_argument(argument, arg1);
    one_argument(target_name, arg2);

    if (arg1[0] == '\0') {
        ch->send_line("Cast which what where?");
        return;
    }
    /* Changed this bit, to '<=' 0 : spells you didn't know, were starting
           fights =) */
    if ((sn = skill_lookup(arg1)) <= 0 || (ch->is_pc() && ch->level < get_skill_level(ch, sn))) {
        ch->send_line("You don't know any spells of that name.");
        return;
    }

    if (ch->position < skill_table[sn].minimum_position) {
        ch->send_line("You can't concentrate enough.");
        return;
    }

    const int mana = mana_for_spell(ch, sn);
    /* MG's rather more dubious bomb-making routine */

    if (!str_cmp(arg2, "explosive")) {

        if (ch->class_num != 0) {
            ch->send_line("You're more than likely gonna kill yourself!");
            return;
        }

        if (ch->position < POS_STANDING) {
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

        WAIT_STATE(ch, skill_table[sn].beats * 2);

        if (ch->is_pc() && number_percent() > get_skill_learned(ch, sn)) {
            ch->send_line("You lost your concentration.");
            check_improve(ch, sn, false, 8);
            ch->mana -= mana / 2;
        } else {
            // Array holding the innate chance of explosion per
            // additional spell for bombs
            static const int bomb_chance[5] = {0, 0, 20, 35, 70};
            ch->mana -= (mana * 2);
            ch->gold -= (mana * 100);

            if (((bomb = get_eq_char(ch, WEAR_HOLD)) != nullptr)) {
                if (bomb->item_type != ITEM_BOMB) {
                    ch->send_line("You must be holding a bomb to add to it.");
                    ch->send_line("Or, to create a new bomb you must have free hands.");
                    ch->mana += (mana * 2);
                    ch->gold += (mana * 100);
                    return;
                }
                // God alone knows what I was on when I originally wrote this....
                /* do detonation if things go wrong */
                if ((bomb->value[0] > (ch->level + 1)) && (number_percent() < 90)) {
                    act("Your naive fumblings on a higher-level bomb cause it to go off!", ch, nullptr, nullptr,
                        To::Char);
                    act("$n is delving with things $s doesn't understand - BANG!!", ch, nullptr, nullptr, To::Char);
                    explode_bomb(bomb, ch, ch);
                    return;
                }

                // Find first free slot
                for (pos = 2; pos < 5; ++pos) {
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
                bomb = create_object(get_obj_index(OBJ_VNUM_BOMB));
                bomb->level = ch->level;
                bomb->value[0] = ch->level;
                bomb->value[1] = sn;

                bomb->description = fmt::sprintf(bomb->description, ch->name);
                bomb->name = fmt::sprintf(bomb->name, ch->name);

                obj_to_char(bomb, ch);
                equip_char(ch, bomb, WEAR_HOLD);
                act("$n creates $p.", ch, bomb, nullptr, To::Room);
                act("You have created $p.", ch, bomb, nullptr, To::Char);

                check_improve(ch, sn, true, 8);
            }
        }
        return;
    }

    /* MG's scribing command ... */
    if (!str_cmp(arg2, "scribe") && (skill_table[sn].spell_fun != spell_null)) {

        if ((ch->class_num != 0) && (ch->class_num != 1)) {
            ch->send_line("You can't scribe! You can't read or write!");
            return;
        }

        if (ch->position < POS_STANDING) {
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
        i_Scroll = get_obj_index(OBJ_VNUM_SCROLL);
        if (ch->carry_weight + i_Scroll->weight > can_carry_w(ch)) {
            act("You cannot carry that much weight.", ch, nullptr, nullptr, To::Char);
            return;
        }
        if (ch->carry_number + 1 > can_carry_n(ch)) {
            act("You can't carry any more items.", ch, nullptr, nullptr, To::Char);
            return;
        }

        WAIT_STATE(ch, skill_table[sn].beats * 2);

        if (ch->is_pc() && number_percent() > get_skill_learned(ch, sn)) {
            ch->send_line("You lost your concentration.");
            check_improve(ch, sn, false, 8);
            ch->mana -= mana / 2;
        } else {
            ch->mana -= (mana * 2);
            ch->gold -= (mana * 100);

            scroll = create_object(get_obj_index(OBJ_VNUM_SCROLL));
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
        return;
    }

    /* MG's brewing command ... */
    if (!str_cmp(arg2, "brew") && (skill_table[sn].spell_fun != spell_null)) {

        if ((ch->class_num != 0) && (ch->class_num != 1)) {
            ch->send_line("You can't make potions! You don't know how!");
            return;
        }

        if (ch->position < POS_STANDING) {
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
        i_Potion = get_obj_index(OBJ_VNUM_POTION);
        if (ch->carry_weight + i_Potion->weight > can_carry_w(ch)) {
            act("You cannot carry that much weight.", ch, nullptr, nullptr, To::Char);
            return;
        }
        if (ch->carry_number + 1 > can_carry_n(ch)) {
            act("You can't carry any more items.", ch, nullptr, nullptr, To::Char);
            return;
        }

        WAIT_STATE(ch, skill_table[sn].beats * 2);

        if (ch->is_pc() && number_percent() > get_skill_learned(ch, sn)) {
            ch->send_line("You lost your concentration.");
            check_improve(ch, sn, false, 8);
            ch->mana -= mana / 2;
        } else {
            ch->mana -= (mana * 2);
            ch->gold -= (mana * 100);

            potion = create_object(get_obj_index(OBJ_VNUM_POTION));
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
        return;
    }

    /*
     * Locate targets.
     */
    victim = nullptr;
    obj = nullptr;
    vo = nullptr;

    switch (skill_table[sn].target) {
    default: bug("Do_cast: bad target for sn {}.", sn); return;

    case TAR_IGNORE: break;

    case TAR_CHAR_OFFENSIVE:
        if (arg2[0] == '\0') {
            if ((victim = ch->fighting) == nullptr) {
                ch->send_line("Cast the spell on whom?");
                return;
            }
        } else {
            if ((victim = get_char_room(ch, arg2)) == nullptr) {
                ch->send_line("They aren't here.");
                return;
            }
        }
        /*
                if ( ch == victim )
                {
                    ch ->send_to( "You can't do that to yourself.\n\r");
                    return;
                }
        */

        if (ch->is_pc()) {

            if (is_safe_spell(ch, victim, false) && victim != ch) {
                ch->send_line("Not on that target.");
                return;
            }
        }

        if (IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) {
            ch->send_line("You can't do that on your own follower.");
            return;
        }

        vo = (void *)victim;
        break;

    case TAR_CHAR_DEFENSIVE:
        if (arg2[0] == '\0') {
            victim = ch;
        } else {
            if ((victim = get_char_room(ch, arg2)) == nullptr) {
                ch->send_line("They aren't here.");
                return;
            }
        }

        vo = (void *)victim;
        break;

    case TAR_CHAR_OBJECT:
    case TAR_CHAR_OTHER: vo = (void *)arg2; break;

    case TAR_CHAR_SELF:
        if (arg2[0] != '\0' && !is_name(arg2, ch->name)) {
            ch->send_line("You cannot cast this spell on another.");
            return;
        }

        vo = (void *)ch;
        break;

    case TAR_OBJ_INV:
        if (arg2[0] == '\0') {
            ch->send_line("What should the spell be cast upon?");
            return;
        }

        if ((obj = get_obj_carry(ch, arg2)) == nullptr) {
            ch->send_line("You are not carrying that.");
            return;
        }

        vo = (void *)obj;
        break;
    }

    if (ch->is_pc() && ch->mana < mana) {
        ch->send_line("You don't have enough mana.");
        return;
    }

    if (str_cmp(skill_table[sn].name, "ventriloquate"))
        say_spell(ch, sn);

    WAIT_STATE(ch, skill_table[sn].beats);

    if (ch->is_pc() && number_percent() > get_skill_learned(ch, sn)) {
        ch->send_line("You lost your concentration.");
        check_improve(ch, sn, false, 1);
        ch->mana -= mana / 2;
    } else {
        ch->mana -= mana;
        (*skill_table[sn].spell_fun)(sn, ch->level, ch, vo);
        check_improve(ch, sn, true, 1);
    }

    if (skill_table[sn].target == TAR_CHAR_OFFENSIVE && victim != ch && victim->master != ch
        && (victim->is_npc() || fighting_duel(ch, victim))) {
        Char *vch;
        Char *vch_next;

        if (ch && ch->in_room && ch->in_room->people != nullptr) {
            for (vch = ch->in_room->people; vch; vch = vch_next) {
                vch_next = vch->next_in_room;
                if (victim == vch && victim->fighting == nullptr) {
                    multi_hit(victim, ch, TYPE_UNDEFINED);
                    break;
                }
            }
        }
    }
}

/*
 * Cast spells at targets using a magical object.
 */
void obj_cast_spell(int sn, int level, Char *ch, Char *victim, OBJ_DATA *obj) {
    void *vo;

    if (sn <= 0)
        return;

    if (sn >= MAX_SKILL || skill_table[sn].spell_fun == 0) {
        bug("Obj_cast_spell: bad sn {}.", sn);
        return;
    }

    switch (skill_table[sn].target) {
    default: bug("Obj_cast_spell: bad target for sn {}.", sn); return;

    case TAR_IGNORE: vo = nullptr; break;

    case TAR_CHAR_OFFENSIVE:
        if (victim == nullptr)
            victim = ch->fighting;
        if (victim == nullptr) {
            ch->send_line("You can't do that.");
            return;
        }
        if (is_safe_spell(ch, victim, false) && ch != victim) {
            ch->send_line("Something isn't right...");
            return;
        }
        vo = (void *)victim;
        break;

    case TAR_CHAR_DEFENSIVE:
        if (victim == nullptr)
            victim = ch;
        vo = (void *)victim;
        break;

    case TAR_CHAR_SELF: vo = (void *)ch; break;

    case TAR_OBJ_INV:
        if (obj == nullptr) {
            ch->send_line("You can't do that.");
            return;
        }
        vo = (void *)obj;
        break;
    }

    /*   target_name = ""; - no longer needed */
    (*skill_table[sn].spell_fun)(sn, level, ch, vo);

    if (skill_table[sn].target == TAR_CHAR_OFFENSIVE && victim != ch && victim->master != ch) {
        Char *vch;
        Char *vch_next;

        for (vch = ch->in_room->people; vch; vch = vch_next) {
            vch_next = vch->next_in_room;
            if (victim == vch && victim->fighting == nullptr) {
                multi_hit(victim, ch, TYPE_UNDEFINED);
                break;
            }
        }
    }
}

/*
 * Spell functions.
 */
void spell_acid_blast(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    auto dam_level = get_direct_dmg_and_level(level, AcidBlastExorciseDemonfireDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, sn, DAM_ACID);
}

void spell_acid_wash(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->item_type != ITEM_WEAPON) {
        ch->send_line("That isn't a weapon.");
        return;
    }

    if (obj->wear_loc != -1) {
        ch->send_line("You do not have that item in your inventory.");
        return;
    }

    if (IS_SET(obj->value[4], WEAPON_LIGHTNING)) {
        ch->send_line("Acid and lightning don't mix.");
        return;
    }
    obj->value[3] = ATTACK_TABLE_INDEX_ACID_WASH;
    SET_BIT(obj->value[4], WEAPON_ACID);
    ch->send_to("With a mighty scream you draw acid from the earth.\n\rYou wash your weapon in the acid pool.\n\r");
}

void spell_armor(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    if (is_affected(victim, sn)) {
        if (victim == ch)
            ch->send_line("You are already protected.");
        else
            act("$N is already armored.", ch, nullptr, victim, To::Char);
        return;
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

void spell_bless(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    if (is_affected(victim, sn)) {
        if (victim == ch)
            ch->send_line("You are already blessed.");
        else
            act("$N already has divine favor.", ch, nullptr, victim, To::Char);

        return;
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

void spell_blindness(int sn, int level, Char *ch, void *vo) {
    (void)ch;
    Char *victim = (Char *)vo;
    if (IS_AFFECTED(victim, AFF_BLIND)) {
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
    af.bitvector = AFF_BLIND;
    affect_to_char(victim, af);
    victim->send_line("You are blinded!");
    act("$n appears to be blinded.", victim);
}

void spell_burning_hands(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    auto dam_level = get_direct_dmg_and_level(level, BurningHandsCauseSerDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, sn, DAM_FIRE);
}

void spell_call_lightning(int sn, int level, Char *ch, void *vo) {
    (void)vo;
    if (!IS_OUTSIDE(ch)) {
        ch->send_line("You must be out of doors.");
        return;
    }

    if (!weather_info.is_raining()) {
        ch->send_line("You need bad weather.");
        return;
    }

    const int dam = dice(level / 2, 8);

    char buf[MAX_STRING_LENGTH];
    snprintf(buf, sizeof(buf), "%s's lightning strikes your foes!\n\r", deity_name);
    ch->send_to(buf);

    snprintf(buf, sizeof(buf), "$n calls %s's lightning to strike $s foes!", deity_name);
    act(buf, ch);

    Char *vch_next;
    for (Char *vch = char_list; vch != nullptr; vch = vch_next) {
        vch_next = vch->next;
        if (vch->in_room == nullptr)
            continue;
        if (vch->in_room == ch->in_room) {
            if (vch != ch && (ch->is_npc() ? vch->is_pc() : vch->is_npc()))
                damage(ch, vch, saves_spell(level, vch) ? dam / 2 : dam, sn, DAM_LIGHTNING);
            continue;
        }

        if (vch->in_room->area == ch->in_room->area && IS_OUTSIDE(vch) && IS_AWAKE(vch))
            vch->send_line("Lightning flashes in the sky.");
    }
}

/* RT calm spell stops all fighting in the room */

void spell_calm(int sn, int level, Char *ch, void *vo) {
    (void)vo;
    Char *vch;
    int mlevel = 0;
    int count = 0;
    int high_level = 0;

    /* get sum of all mobile levels in the room */
    for (vch = ch->in_room->people; vch != nullptr; vch = vch->next_in_room) {
        if (vch->position == POS_FIGHTING) {
            count++;
            if (vch->is_npc())
                mlevel += vch->level;
            else
                mlevel += vch->level / 2;
            high_level = UMAX(high_level, vch->level);
        }
    }

    /* compute chance of stopping combat */
    const int chance = 4 * level - high_level + 2 * count;

    if (ch->is_immortal()) /* always works */
        mlevel = 0;

    if (number_range(0, chance) >= mlevel) /* hard to stop large fights */
    {
        for (vch = ch->in_room->people; vch != nullptr; vch = vch->next_in_room) {
            if (vch->is_npc() && (IS_SET(vch->imm_flags, IMM_MAGIC) || IS_SET(vch->act, ACT_UNDEAD)))
                return;

            if (IS_AFFECTED(vch, AFF_CALM) || IS_AFFECTED(vch, AFF_BERSERK) || is_affected(vch, skill_lookup("frenzy")))
                return;

            vch->send_line("A wave of calm passes over you.");

            if (vch->fighting || vch->position == POS_FIGHTING)
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
            af.bitvector = AFF_CALM;
            affect_to_char(vch, af);

            af.location = AffectLocation::Damroll;
            affect_to_char(vch, af);
        }
    }
}

void spell_cancellation(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    Char *victim = (Char *)vo;
    bool found = false;

    level += 2;

    if ((ch->is_pc() && victim->is_npc() && !(IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim))
        || (ch->is_npc() && victim->is_pc())) {
        ch->send_line("You failed, try dispel magic.");
        return;
    }

    /* unlike dispel magic, the victim gets NO save */

    /* begin running through the spells */

    if (check_dispel(level, victim, skill_lookup("armor")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("bless")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("blindness"))) {
        found = true;
        act("$n is no longer blinded.", victim);
    }

    if (check_dispel(level, victim, skill_lookup("calm"))) {
        found = true;
        act("$n no longer looks so peaceful...", victim);
    }

    if (check_dispel(level, victim, skill_lookup("change sex"))) {
        found = true;
        act("$n looks more like $mself again.", victim);
        victim->send_line("You feel more like yourself again.");
    }

    if (check_dispel(level, victim, skill_lookup("charm person"))) {
        found = true;
        act("$n regains $s free will.", victim);
    }

    if (check_dispel(level, victim, skill_lookup("chill touch"))) {
        found = true;
        act("$n looks warmer.", victim);
    }

    if (check_dispel(level, victim, skill_lookup("curse")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("detect evil")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("detect invis")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("detect hidden")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("protection evil")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("protection good")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("lethargy")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("regeneration")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("talon")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("octarine fire")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("detect magic")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("faerie fire"))) {
        act("$n's outline fades.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("fly"))) {
        act("$n falls to the ground!", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("frenzy"))) {
        act("$n no longer looks so wild.", victim);
        ;
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("giant strength"))) {
        act("$n no longer looks so mighty.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("haste"))) {
        act("$n is no longer moving so quickly.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("talon"))) {
        act("$n's hands return to normal.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("lethargy"))) {
        act("$n's heart-beat quickens.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("insanity"))) {
        act("$n looks less confused.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("infravision")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("invis"))) {
        act("$n fades into existance.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("mass invis"))) {
        act("$n fades into existance.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("octarine fire"))) {
        act("$n's outline fades.", victim);
        found = true;
    }
    if (check_dispel(level, victim, skill_lookup("pass door")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("sanctuary"))) {
        act("The white aura around $n's body vanishes.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("shield"))) {
        act("The shield protecting $n vanishes.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("sleep")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("stone skin"))) {
        act("$n's skin regains its normal texture.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("weaken"))) {
        act("$n looks stronger.", victim);
        found = true;
    }

    if (found)
        ch->send_line("Ok.");
    else
        ch->send_line("Spell failed.");
}

void spell_cause_light(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    auto dam_level = get_direct_dmg_and_level(level, MagicMissileCauseLightDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, sn, DAM_HARM);
}

void spell_cause_critical(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    auto dam_level = get_direct_dmg_and_level(level, LightningBoltCauseCritDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, sn, DAM_HARM);
}

void spell_cause_serious(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    auto dam_level = get_direct_dmg_and_level(level, BurningHandsCauseSerDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, sn, DAM_HARM);
}

void spell_chain_lightning(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    Char *tmp_vict, *last_vict, *next_vict;
    bool found;

    /* first strike */

    act("A lightning bolt leaps from $n's hand and arcs to $N.", ch, nullptr, victim, To::Room);
    act("A lightning bolt leaps from your hand and arcs to $N.", ch, nullptr, victim, To::Char);
    act("A lightning bolt leaps from $n's hand and hits you!", ch, nullptr, victim, To::Vict);

    auto dam_level = get_direct_dmg_and_level(level, LightningBoltCauseCritDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 3;
    damage(ch, victim, dam_level.first, sn, DAM_LIGHTNING);
    last_vict = victim;
    level -= 4; /* decrement damage */

    /* new targets */
    while (level > 0) {
        found = false;
        for (tmp_vict = ch->in_room->people; tmp_vict != nullptr; tmp_vict = next_vict) {
            next_vict = tmp_vict->next_in_room;
            if (!is_safe_spell(ch, tmp_vict, true) && tmp_vict != last_vict) {
                found = true;
                last_vict = tmp_vict;
                act("The bolt arcs to $n!", tmp_vict);
                act("The bolt hits you!", tmp_vict, nullptr, nullptr, To::Char);
                dam_level = get_direct_dmg_and_level(level, LightningBoltCauseCritDamage);
                if (saves_spell(dam_level.second, tmp_vict))
                    dam_level.first /= 3;
                damage(ch, tmp_vict, dam_level.first, sn, DAM_LIGHTNING);
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
            damage(ch, ch, dam_level.first, sn, DAM_LIGHTNING);
            level -= 4; /* decrement damage */
            if ((ch == nullptr) || (ch->in_room == nullptr))
                return;
        }
        /* now go back and find more targets */
    }
}

void spell_change_sex(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn)) {
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
        af.modifier = number_range(0, 2) - victim->sex;
    } while (af.modifier == 0);
    af.bitvector = 0;
    affect_to_char(victim, af);
    victim->send_line("You feel different.");
    act("$n doesn't look like $mself anymore...", victim);
}

void spell_charm_person(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (victim == ch) {
        ch->send_line("You like yourself even better!");
        return;
    }

    if (IS_AFFECTED(victim, AFF_CHARM) || IS_AFFECTED(ch, AFF_CHARM) || ch->get_trust() < victim->get_trust()
        || IS_SET(victim->imm_flags, IMM_CHARM) || saves_spell(level, victim))
        return;

    if (IS_SET(victim->in_room->room_flags, ROOM_LAW)) {
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
    af.bitvector = AFF_CHARM;
    affect_to_char(victim, af);
    act("Isn't $n just so nice?", ch, nullptr, victim, To::Vict);
    if (ch != victim)
        act("$N looks at you with adoring eyes.", ch, nullptr, victim, To::Char);
}

void spell_chill_touch(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
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

    damage(ch, victim, dam_level.first, sn, DAM_COLD);
}

void spell_colour_spray(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    auto dam_level = get_direct_dmg_and_level(level, ColourSprayDispelGoodEvilDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    else
        spell_blindness(skill_lookup("blindness"), dam_level.second / 2, ch, (void *)victim);

    damage(ch, victim, dam_level.first, sn, DAM_LIGHT);
}

void spell_continual_light(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    (void)vo;
    OBJ_DATA *light;

    light = create_object(get_obj_index(OBJ_VNUM_LIGHT_BALL));
    obj_to_room(light, ch->in_room);
    act("$n twiddles $s thumbs and $p appears.", ch, light, nullptr, To::Room);
    act("You twiddle your thumbs and $p appears.", ch, light, nullptr, To::Char);
}

void spell_control_weather(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)vo;
    if (!str_cmp(target_name, "better"))
        weather_info.control(dice(level / 3, 4));
    else if (!str_cmp(target_name, "worse"))
        weather_info.control(-dice(level / 3, 4));
    else {
        ch->send_line("Do you want it to get better or worse?");
        return;
    }
    ch->send_line("Ok.");
}

void spell_create_food(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)vo;
    OBJ_DATA *mushroom = create_object(get_obj_index(OBJ_VNUM_MUSHROOM));
    mushroom->value[0] = 5 + level;
    obj_to_room(mushroom, ch->in_room);
    act("$p suddenly appears.", ch, mushroom, nullptr, To::Room);
    act("$p suddenly appears.", ch, mushroom, nullptr, To::Char);
}

void spell_create_spring(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)vo;
    OBJ_DATA *spring = create_object(get_obj_index(OBJ_VNUM_SPRING));
    spring->timer = level;
    obj_to_room(spring, ch->in_room);
    act("$p flows from the ground.", ch, spring, nullptr, To::Room);
    act("$p flows from the ground.", ch, spring, nullptr, To::Char);
}

void spell_create_water(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->item_type != ITEM_DRINK_CON) {
        ch->send_line("It is unable to hold water.");
        return;
    }

    if (obj->value[2] != LIQ_WATER && obj->value[1] != 0) {
        ch->send_line("It contains some other liquid.");
        return;
    }

    int water = UMIN(level * (weather_info.is_raining() ? 4 : 2), obj->value[0] - obj->value[1]);

    if (water > 0) {
        obj->value[2] = LIQ_WATER;
        obj->value[1] += water;
        if (!is_name("water", obj->name))
            obj->name = fmt::format("{} water", obj->name);
        act("$p is filled.", ch, obj, nullptr, To::Char);
    } else {
        act("$p is full.", ch, obj, nullptr, To::Char);
    }
}

void spell_cure_blindness(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    Char *victim = (Char *)vo;

    if (!is_affected(victim, gsn_blindness)) {
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

void spell_cure_critical(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    Char *victim = (Char *)vo;

    int heal = dice(3, 8) + level - 6;
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);
    victim->send_line("You feel better!");
    if (ch != victim)
        ch->send_line("Ok.");
}

/* RT added to cure plague */
void spell_cure_disease(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    Char *victim = (Char *)vo;

    if (!is_affected(victim, gsn_plague)) {
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

void spell_cure_light(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    Char *victim = (Char *)vo;

    int heal = dice(1, 8) + level / 3;
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);
    victim->send_line("You feel better!");
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_cure_poison(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    Char *victim = (Char *)vo;

    if (!is_affected(victim, gsn_poison)) {
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

void spell_cure_serious(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    Char *victim = (Char *)vo;

    int heal = dice(2, 8) + level / 2;
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);
    victim->send_line("You feel better!");
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_curse(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_CURSE)) {
        act("A curse has already befallen $N.", ch, nullptr, victim, To::Char);
        return;
    }
    if (saves_spell(level, victim)) {
        act("$N looks uncomfortable for a moment but it passes.", ch, nullptr, victim, To::Char);
        return;
    }

    af.type = sn;
    af.level = level;
    af.duration = 3 + (level / 6);
    af.location = AffectLocation::Hitroll;
    af.modifier = -1 * (level / 8);
    af.bitvector = AFF_CURSE;
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
void spell_exorcise(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;

    if (ch->is_pc() && (victim->alignment > (ch->alignment + 100))) {
        victim = ch;
        ch->send_line("Your exorcism turns upon you!");
    }

    ch->alignment = UMIN(1000, ch->alignment + 50);

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
    damage(ch, victim, dam_level.first, sn, DAM_HOLY);
}

/*
 * Demonfire is the 'evil' version of exorcise.
 */
void spell_demonfire(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;

    if (ch->is_pc() && (victim->alignment < (ch->alignment - 100))) {
        victim = ch;
        ch->send_line("The demons turn upon you!");
    }

    ch->alignment = UMAX(-1000, ch->alignment - 50);

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
    damage(ch, victim, dam_level.first, sn, DAM_NEGATIVE);
}

void spell_detect_evil(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    if (IS_AFFECTED(victim, AFF_DETECT_EVIL)) {
        if (victim == ch)
            ch->send_line("You can already sense evil.");
        else
            act("$N can already detect evil.", ch, nullptr, victim, To::Char);
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.bitvector = AFF_DETECT_EVIL;
    affect_to_char(victim, af);
    victim->send_line("Your eyes tingle.");
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_detect_hidden(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    if (IS_AFFECTED(victim, AFF_DETECT_HIDDEN)) {
        if (victim == ch)
            ch->send_line("You are already as alert as you can be. ");
        else
            act("$N can already sense hidden lifeforms.", ch, nullptr, victim, To::Char);
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.bitvector = AFF_DETECT_HIDDEN;
    affect_to_char(victim, af);
    victim->send_line("Your awareness improves.");
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_detect_invis(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    if (IS_AFFECTED(victim, AFF_DETECT_INVIS)) {
        if (victim == ch)
            ch->send_line("You can already see invisible.");
        else
            act("$N can already see invisible things.", ch, nullptr, victim, To::Char);
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.bitvector = AFF_DETECT_INVIS;
    affect_to_char(victim, af);
    victim->send_line("Your eyes tingle.");
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_detect_magic(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    if (IS_AFFECTED(victim, AFF_DETECT_MAGIC)) {
        if (victim == ch)
            ch->send_line("You can already sense magical auras.");
        else
            act("$N can already detect magic.", ch, nullptr, victim, To::Char);
        return;
    }
    AFFECT_DATA af;
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.bitvector = AFF_DETECT_MAGIC;
    affect_to_char(victim, af);
    victim->send_line("Your eyes tingle.");
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_detect_poison(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD) {
        if (obj->value[3] != 0)
            ch->send_line("You smell poisonous fumes.");
        else
            ch->send_line("It looks delicious.");
    } else {
        ch->send_line("It doesn't look poisoned.");
    }
}

void spell_dispel_evil(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
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
    damage(ch, victim, dam_level.first, sn, DAM_HOLY);
}

void spell_dispel_good(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;

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
    damage(ch, victim, dam_level.first, sn, DAM_HOLY);
}

/* modified for enhanced use */

void spell_dispel_magic(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    Char *victim = (Char *)vo;
    bool found = false;

    if (saves_spell(level, victim)) {
        victim->send_line("You feel a brief tingling sensation.");
        ch->send_line("You failed.");
        return;
    }

    /* begin running through the spells */

    if (check_dispel(level, victim, skill_lookup("armor")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("bless")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("blindness"))) {
        found = true;
        act("$n is no longer blinded.", victim);
    }

    if (check_dispel(level, victim, skill_lookup("calm"))) {
        found = true;
        act("$n no longer looks so peaceful...", victim);
    }

    if (check_dispel(level, victim, skill_lookup("change sex"))) {
        found = true;
        act("$n looks more like $mself again.", victim);
    }

    if (check_dispel(level, victim, skill_lookup("charm person"))) {
        found = true;
        act("$n regains $s free will.", victim);
    }

    if (check_dispel(level, victim, skill_lookup("chill touch"))) {
        found = true;
        act("$n looks warmer.", victim);
    }

    if (check_dispel(level, victim, skill_lookup("curse")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("detect evil")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("detect hidden")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("detect invis")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("detect hidden")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("detect magic")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("faerie fire"))) {
        act("$n's outline fades.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("fly"))) {
        act("$n falls to the ground!", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("frenzy"))) {
        act("$n no longer looks so wild.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("giant strength"))) {
        act("$n no longer looks so mighty.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("haste"))) {
        act("$n is no longer moving so quickly.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("talon"))) {
        act("$n hand's return to normal.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("lethargy"))) {
        act("$n is looking more lively.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("insanity"))) {
        act("$n looks less confused.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("infravision")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("invis"))) {
        act("$n fades into existance.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("mass invis"))) {
        act("$n fades into existance.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("pass door")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("protection")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("sanctuary"))) {
        act("The white aura around $n's body vanishes.", victim);
        found = true;
    }

    if (IS_AFFECTED(victim, AFF_SANCTUARY) && !saves_dispel(level, victim->level, -1)
        && !is_affected(victim, skill_lookup("sanctuary"))) {
        REMOVE_BIT(victim->affected_by, AFF_SANCTUARY);
        act("The white aura around $n's body vanishes.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("shield"))) {
        act("The shield protecting $n vanishes.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("sleep")))
        found = true;

    if (check_dispel(level, victim, skill_lookup("stone skin"))) {
        act("$n's skin regains its normal texture.", victim);
        found = true;
    }

    if (check_dispel(level, victim, skill_lookup("weaken"))) {
        act("$n looks stronger.", victim);
        found = true;
    }

    if (found)
        ch->send_line("Ok.");
    else
        ch->send_line("Spell failed.");
}

void spell_earthquake(int sn, int level, Char *ch, void *vo) {
    (void)vo;
    Char *vch;
    Char *vch_next;

    ch->send_line("The earth trembles beneath your feet!");
    act("$n makes the earth tremble and shiver.", ch);

    for (vch = char_list; vch != nullptr; vch = vch_next) {
        vch_next = vch->next;
        if (vch->in_room == nullptr)
            continue;
        if (vch->in_room == ch->in_room) {
            if (vch != ch && !is_safe_spell(ch, vch, true)) {
                if (IS_AFFECTED(vch, AFF_FLYING))
                    damage(ch, vch, 0, sn, DAM_BASH);
                else
                    damage(ch, vch, level + dice(2, 8), sn, DAM_BASH);
            }
            continue;
        }

        if (vch->in_room->area == ch->in_room->area)
            vch->send_line("The earth trembles and shivers.");
    }
}

void spell_remove_invisible(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->wear_loc != -1) {
        ch->send_line("You have to be carrying it to remove invisible on it!");
        return;
    }

    if (!IS_SET(obj->extra_flags, ITEM_INVIS)) {
        ch->send_line("That object is not invisible!");
        return;
    }

    int chance = URANGE(5,
                        (30 + URANGE(-20, (ch->level - obj->level), 20)
                         + (material_table[obj->material].magical_resilience / 2)),
                        100)
                 - number_percent();

    if (chance >= 0) {
        act("$p appears from nowhere!", ch, obj, nullptr, To::Room);
        act("$p appears from nowhere!", ch, obj, nullptr, To::Char);
        REMOVE_BIT(obj->extra_flags, ITEM_INVIS);
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

void spell_remove_alignment(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->wear_loc != -1) {
        ch->send_line("You have to be carrying it to remove alignment on it!");
        return;
    }

    if ((!IS_SET(obj->extra_flags, ITEM_ANTI_EVIL)) && (!IS_SET(obj->extra_flags, ITEM_ANTI_GOOD))
        && (!IS_SET(obj->extra_flags, ITEM_ANTI_NEUTRAL))) {
        ch->send_line("That object has no alignment!");
        return;
    }

    const int levdif = URANGE(-20, (ch->level - obj->level), 20);
    int chance = URANGE(5, levdif / 2 + material_table[obj->material].magical_resilience, 100);
    const int score = chance - number_percent();

    if ((IS_SET(obj->extra_flags, ITEM_ANTI_EVIL) && ch->is_good())
        || (IS_SET(obj->extra_flags, ITEM_ANTI_GOOD) && ch->is_evil()))
        chance -= 10;

    if ((score <= 20)) {
        act("The powerful nature of $n's spell removes some of $m alignment!", ch, obj, nullptr, To::Room);
        act("Your spell removes some of your alignment!", ch, obj, nullptr, To::Char);
        if (ch->is_good())
            ch->alignment -= (30 - score);
        if (ch->is_evil())
            ch->alignment += (30 - score);
        ch->alignment = URANGE(-1000, ch->alignment, 1000);
    }

    if (score >= 0) {
        act("$p glows grey.", ch, obj, nullptr, To::Room);
        act("$p glows grey.", ch, obj, nullptr, To::Char);
        REMOVE_BIT(obj->extra_flags, ITEM_ANTI_GOOD);
        REMOVE_BIT(obj->extra_flags, ITEM_ANTI_EVIL);
        REMOVE_BIT(obj->extra_flags, ITEM_ANTI_NEUTRAL);
    } else if (score < -20) {
        act("$p shivers violently and explodes!", ch, obj, nullptr, To::Room);
        act("$p shivers violently and explodes!", ch, obj, nullptr, To::Char);
        extract_obj(obj);
    } else {
        act("$p appears unaffected.", ch, obj, nullptr, To::Room);
        act("$p appears unaffected.", ch, obj, nullptr, To::Char);
    }
}

void spell_enchant_armor(int sn, int level, Char *ch, void *vo) {
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->item_type != ITEM_ARMOR) {
        ch->send_line("That isn't an armor.");
        return;
    }

    if (obj->wear_loc != -1) {
        ch->send_line("The item must be carried to be enchanted.");
        return;
    }

    /* this means they have no bonus */
    int ac_bonus = 0;
    int fail = 15; /* base 15% chance of failure */
    /* TheMoog added material fiddling */
    fail += ((100 - material_table[obj->material].magical_resilience) / 3);

    /* find the bonuses */
    bool ac_found = false;
    if (!obj->enchanted)
        for (auto &af : obj->pIndexData->affected) {
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

    if (IS_OBJ_STAT(obj, ITEM_BLESS))
        fail -= 15;
    if (IS_OBJ_STAT(obj, ITEM_GLOW))
        fail -= 5;

    fail = URANGE(5, fail, 95);

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

        for (auto af_clone : obj->pIndexData->affected) {
            af_clone.type = UMAX(0, af_clone.type);
            obj->affected.add(af_clone);
        }
    }

    int added;
    if (result <= (100 - level / 5)) { /* success! */
        act("$p shimmers with a gold aura.", ch, obj, nullptr, To::Char);
        act("$p shimmers with a gold aura.", ch, obj, nullptr, To::Room);
        SET_BIT(obj->extra_flags, ITEM_MAGIC);
        added = -1;
    } else { /* exceptional enchant */
        act("$p glows a brillant gold!", ch, obj, nullptr, To::Char);
        act("$p glows a brillant gold!", ch, obj, nullptr, To::Room);
        SET_BIT(obj->extra_flags, ITEM_MAGIC);
        SET_BIT(obj->extra_flags, ITEM_GLOW);
        added = -2;
    }

    /* now add the enchantments */

    if (obj->level < LEVEL_HERO)
        obj->level = UMIN(LEVEL_HERO - 1, obj->level + 1);

    if (ac_found) {
        for (auto &af : obj->affected) {
            if (af.location == AffectLocation::Ac) {
                af.type = sn;
                af.modifier += added;
                af.level = UMAX(af.level, level);
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

void spell_enchant_weapon(int sn, int level, Char *ch, void *vo) {
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if ((obj->item_type != ITEM_WEAPON) && (obj->item_type != ITEM_ARMOR)) {
        ch->send_line("That isn't a weapon or armour.");
        return;
    }

    if (obj->wear_loc != -1) {
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
    fail += ((100 - material_table[obj->material].magical_resilience) / 3);
    if (obj->item_type == ITEM_ARMOR) {
        fail += 25; /* harder to enchant armor with weapon */
        modifier = 2;
    }
    /* find the bonuses */

    bool hit_found = false, dam_found = false;
    if (!obj->enchanted) {
        for (auto &af : obj->pIndexData->affected) {
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

    if (IS_OBJ_STAT(obj, ITEM_BLESS))
        fail -= 15;
    if (IS_OBJ_STAT(obj, ITEM_GLOW))
        fail -= 5;

    fail = URANGE(5, fail, 95);

    const int result = number_percent();

    /* We don't want armor, with more than 2 ench. hit&dam */
    int no_ench_num = 0;
    if (obj->item_type == ITEM_ARMOR && (ch->get_trust() < MAX_LEVEL)) {
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

        for (auto af_clone : obj->pIndexData->affected) {
            af_clone.type = UMAX(0, af_clone.type);
            obj->affected.add(af_clone);
        }
    }

    int added;
    if (result <= (100 - level / 5)) { /* success! */
        act("$p glows blue.", ch, obj, nullptr, To::Char);
        act("$p glows blue.", ch, obj, nullptr, To::Room);
        SET_BIT(obj->extra_flags, ITEM_MAGIC);
        added = 1;
    } else { /* exceptional enchant */
        act("$p glows a brillant blue!", ch, obj, nullptr, To::Char);
        act("$p glows a brillant blue!", ch, obj, nullptr, To::Room);
        SET_BIT(obj->extra_flags, ITEM_MAGIC);
        SET_BIT(obj->extra_flags, ITEM_GLOW);
        added = 2;
    }

    /* now add the enchantments */
    if (obj->level < LEVEL_HERO - 1)
        obj->level = UMIN(LEVEL_HERO - 1, obj->level + 1);

    if (dam_found) {
        for (auto &af : obj->affected) {
            if (af.location == AffectLocation::Damroll) {
                af.type = sn;
                af.modifier += added;
                af.level = UMAX(af.level, level);
                if (af.modifier > 4)
                    SET_BIT(obj->extra_flags, ITEM_HUM);
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
                af.level = UMAX(af.level, level);
                if (af.modifier > 4)
                    SET_BIT(obj->extra_flags, ITEM_HUM);
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
    if ((obj->item_type == ITEM_ARMOR) && (obj->level < 50)) {
        act("$p looks way better than it did before!", ch, obj, nullptr, To::Char);
        obj->level = 50;
    }
}

void spell_protect_container(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->item_type != ITEM_CONTAINER) {
        ch->send_line("That isn't a container.");
        return;
    }

    if (IS_OBJ_STAT(obj, ITEM_PROTECT_CONTAINER)) {
        act("$p is already protected!", ch, obj, nullptr, To::Char);
        return;
    }

    if (ch->gold < 50000) {
        ch->send_line("You need 50,000 gold coins to protect a container");
        return;
    }

    ch->gold -= 50000;
    SET_BIT(obj->extra_flags, ITEM_PROTECT_CONTAINER);
    act("$p is now protected!", ch, obj, nullptr, To::Char);
}

/*PGW A new group to give Barbarians a helping hand*/
void spell_vorpal(int sn, int level, Char *ch, void *vo) {
    (void)level;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->item_type != ITEM_WEAPON) {
        ch->send_line("This isn't a weapon.");
        return;
    }

    if (obj->wear_loc != -1) {
        ch->send_line("The item must be carried to be made vorpal.");
        return;
    }

    const int mana = mana_for_spell(ch, sn);
    if (ch->is_npc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }

    ch->gold -= (mana * 100);
    SET_BIT(obj->value[4], WEAPON_VORPAL);
    ch->send_line("You create a flaw in the universe and place it on your blade!");
}

void spell_venom(int sn, int level, Char *ch, void *vo) {
    (void)level;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->item_type != ITEM_WEAPON) {
        ch->send_line("That isn't a weapon.");
        return;
    }

    if (obj->wear_loc != -1) {
        ch->send_line("The item must be carried to be venomed.");
        return;
    }

    const int mana = mana_for_spell(ch, sn);
    if (ch->is_npc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }

    ch->gold -= (mana * 100);
    SET_BIT(obj->value[4], WEAPON_POISONED);
    ch->send_line("You coat the blade in poison!");
}

void spell_black_death(int sn, int level, Char *ch, void *vo) {
    (void)level;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->item_type != ITEM_WEAPON) {
        ch->send_line("That isn't a weapon.");
        return;
    }

    if (obj->wear_loc != -1) {
        ch->send_line("The item must be carried to be plagued.");
        return;
    }

    const int mana = mana_for_spell(ch, sn);
    if (ch->is_npc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }

    ch->gold -= (mana * 100);
    SET_BIT(obj->value[4], WEAPON_PLAGUED);
    ch->send_line("Your use your cunning and skill to plague the weapon!");
}

void spell_damnation(int sn, int level, Char *ch, void *vo) {
    (void)level;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->item_type != ITEM_WEAPON) {
        ch->send_line("That isn't a weapon.");
        return;
    }

    if (obj->wear_loc != -1) {
        ch->send_line("You do not have that item in your inventory.");
        return;
    }

    const int mana = mana_for_spell(ch, sn);
    if (ch->is_npc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }

    ch->gold -= (mana * 100);
    SET_BIT(obj->extra_flags, ITEM_NODROP);
    SET_BIT(obj->extra_flags, ITEM_NOREMOVE);
    ch->send_line("You turn red in the face and curse your weapon into the pits of hell!");
}

void spell_vampire(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->item_type != ITEM_WEAPON) {
        ch->send_line("That isn't a weapon.");
        return;
    }

    if (obj->wear_loc != -1) {
        ch->send_line("The item must be carried to be vampiric.");
        return;
    }

    if (ch->is_immortal()) {
        SET_BIT(obj->value[4], WEAPON_VAMPIRIC);
        ch->send_line("You suck the life force from the weapon leaving it hungry for blood.");
    }
}

void spell_tame_lightning(int sn, int level, Char *ch, void *vo) {
    (void)level;
    OBJ_DATA *obj = (OBJ_DATA *)vo;

    if (obj->item_type != ITEM_WEAPON) {
        ch->send_line("That isn't a weapon.");
        return;
    }

    if (obj->wear_loc != -1) {
        ch->send_line("You do not have that item in your inventory.");
        return;
    }

    if (IS_SET(obj->value[4], WEAPON_ACID)) {
        ch->send_line("Acid and lightning do not mix.");
        return;
    }

    const int mana = mana_for_spell(ch, sn);
    if (ch->is_npc() && ch->gold < (mana * 100)) {
        ch->send_line("You can't afford to!");
        return;
    }

    ch->gold -= (mana * 100);
    obj->value[3] = ATTACK_TABLE_INDEX_TAME_LIGHTNING;
    SET_BIT(obj->value[4], WEAPON_LIGHTNING);
    ch->send_to("You summon a MASSIVE storm.\n\rHolding your weapon aloft you call lightning down from the sky. "
                "\n\rThe lightning swirls around it - you have |YTAMED|w the |YLIGHTNING|w.\n\r");
}

/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
void spell_energy_drain(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    int dam;

    if (saves_spell(level, victim)) {
        victim->send_line("You feel a momentary chill.");
        return;
    }

    ch->alignment = UMAX(-1000, ch->alignment - 50);
    if (victim->level <= 2) {
        dam = victim->hit + 1;
    } else {
        if (victim->in_room->vnum != CHAL_ROOM)
            gain_exp(victim, 0 - 2.5 * number_range(level / 2, 3 * level / 2));
        victim->mana /= 2;
        victim->move /= 2;
        dam = dice(1, level);
        ch->hit += dam;
    }

    victim->send_line("You feel your life slipping away!");
    ch->send_line("Wow....what a rush!");
    damage(ch, victim, dam, sn, DAM_NEGATIVE);
}

void spell_fireball(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    auto dam_level = get_direct_dmg_and_level(level, FireballHarmDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, sn, DAM_FIRE);
}

/* Flamestrike is the Attack spell group's equivalent of fireball. It's slightly
 * less powerful and less mana efficient, largely as the cleric role is not
 * meant to specialize in damage dealing. See also Harm, which is similar to Fireball
 * in the Harmful group.
 */
void spell_flamestrike(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    auto dam_level = get_direct_dmg_and_level(level, FlamestrikeDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, sn, DAM_FIRE);
}

void spell_faerie_fire(int sn, int level, Char *ch, void *vo) {
    (void)ch;
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_FAERIE_FIRE)) {
        act("$N is already surrounded by a pink outline.", ch, nullptr, victim, To::Char);
        return;
    }
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = AffectLocation::Ac;
    af.modifier = 2 * level;
    af.bitvector = AFF_FAERIE_FIRE;
    affect_to_char(victim, af);
    victim->send_line("You are surrounded by a pink outline.");
    act("$n is surrounded by a pink outline.", victim);
}

void spell_faerie_fog(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)vo;
    Char *ich;

    act("$n conjures a cloud of purple smoke.", ch);
    ch->send_line("You conjure a cloud of purple smoke.");

    for (ich = ch->in_room->people; ich != nullptr; ich = ich->next_in_room) {
        if (ich->is_pc() && IS_SET(ich->act, PLR_WIZINVIS))
            continue;
        if (ich->is_pc() && IS_SET(ich->act, PLR_PROWL))
            continue;

        if (ich == ch || saves_spell(level, ich))
            continue;

        affect_strip(ich, gsn_invis);
        affect_strip(ich, gsn_mass_invis);
        affect_strip(ich, gsn_sneak);
        REMOVE_BIT(ich->affected_by, AFF_HIDE);
        REMOVE_BIT(ich->affected_by, AFF_INVISIBLE);
        REMOVE_BIT(ich->affected_by, AFF_SNEAK);
        act("$n is revealed!", ich);
        ich->send_line("You are revealed!");
    }
}

void spell_fly(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_FLYING)) {
        if (victim == ch)
            ch->send_line("You are already airborne.");
        else
            act("$N doesn't need your help to fly.", ch, nullptr, victim, To::Char);
        return;
    }
    af.type = sn;
    af.level = level;
    af.duration = level + 3;
    af.bitvector = AFF_FLYING;
    affect_to_char(victim, af);
    victim->send_line("Your feet rise off the ground.");
    act("$n's feet rise off the ground.", victim);
}

/* RT clerical berserking spell */

void spell_frenzy(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    /*  OBJ_DATA *wield = get_eq_char( ch, WEAR_WIELD );*/

    AFFECT_DATA af;

    if (is_affected(victim, sn) || IS_AFFECTED(victim, AFF_BERSERK)) {
        if (victim == ch)
            ch->send_line("You are already in a frenzy.");
        else
            act("$N is already in a frenzy.", ch, nullptr, victim, To::Char);
        return;
    }

    if (is_affected(victim, skill_lookup("calm"))) {
        if (victim == ch)
            ch->send_line("Why don't you just relax for a while?");
        else
            act("$N doesn't look like $e wants to fight anymore.", ch, nullptr, victim, To::Char);
        return;
    }

    if ((ch->is_good() && !victim->is_good()) || (ch->is_neutral() && !victim->is_neutral())
        || (ch->is_evil() && !victim->is_evil())) {
        act("Your god doesn't seem to like $N", ch, nullptr, victim, To::Char);
        return;
    }

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

    /*  if ( (wield !=nullptr) && (wield->item_type == ITEM_WEAPON) &&
          (IS_SET(wield->value[4], WEAPON_FLAMING)))
        ch->send_line("Your great energy causes your weapon to burst into flame.");
      wield->value[3] = 29;*/
}

/* RT ROM-style gate */

void spell_gate(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)vo;
    Char *victim;
    bool gate_pet;

    if ((victim = get_char_world(ch, target_name)) == nullptr || victim == ch || victim->in_room == nullptr
        || !can_see_room(ch, victim->in_room) || IS_SET(victim->in_room->room_flags, ROOM_SAFE)
        || IS_SET(victim->in_room->room_flags, ROOM_PRIVATE) || IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
        || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL) || IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
        || victim->level >= level + 3 || (victim->is_pc() && victim->level >= LEVEL_HERO) /* NOT trust */
        || (victim->is_npc() && IS_SET(victim->imm_flags, IMM_SUMMON))
        || (victim->is_pc() && IS_SET(victim->act, PLR_NOSUMMON)) || (victim->is_npc() && saves_spell(level, victim))) {
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

void spell_giant_strength(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn)) {
        if (victim == ch)
            ch->send_line("You are already as strong as you can get!");
        else
            act("$N can't get any stronger.", ch, nullptr, victim, To::Char);
        return;
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
void spell_harm(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    auto dam_level = get_direct_dmg_and_level(level, FireballHarmDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, sn, DAM_FIRE);
}

void spell_regeneration(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn) || IS_AFFECTED(victim, AFF_REGENERATION)) {
        if (victim == ch)
            ch->send_line("You are already vibrant!");
        else
            act("$N is already as vibrant as $e can be.", ch, nullptr, victim, To::Char);
        return;
    }
    af.type = sn;
    af.level = level;
    if (victim == ch)
        af.duration = level / 2;
    else
        af.duration = level / 4;
    af.bitvector = AFF_REGENERATION;
    affect_to_char(victim, af);
    victim->send_line("You feel vibrant!");
    act("$n is feeling more vibrant.", victim);
    if (ch != victim)
        ch->send_line("Ok.");
}

/* RT haste spell */

void spell_haste(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn) || IS_AFFECTED(victim, AFF_HASTE) || IS_SET(victim->off_flags, OFF_FAST)) {
        if (victim == ch)
            ch->send_line("You can't move any faster!");
        else
            act("$N is already moving as fast as $e can.", ch, nullptr, victim, To::Char);
        return;
    }
    af.type = sn;
    af.level = level;
    if (victim == ch)
        af.duration = level / 2;
    else
        af.duration = level / 4;
    af.location = AffectLocation::Dex;
    af.modifier = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = AFF_HASTE;
    affect_to_char(victim, af);
    victim->send_line("You feel yourself moving more quickly.");
    act("$n is moving more quickly.", victim);
    if (ch != victim)
        ch->send_line("Ok.");
}

/* Moog's insanity spell */
void spell_insanity(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn)) {
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

void spell_lethargy(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn) || IS_AFFECTED(victim, AFF_LETHARGY) || IS_SET(victim->off_flags, OFF_SLOW)) {
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
    af.type = sn;
    af.level = level;
    if (victim == ch)
        af.duration = level / 2;
    else
        af.duration = level / 4;
    af.location = AffectLocation::Dex;
    af.modifier = -(1 + (level >= 18) + (level >= 25) + (level >= 32));
    af.bitvector = AFF_LETHARGY;
    affect_to_char(victim, af);
    victim->send_line("You feel your heart-beat slowing.");
    act("$n slows nearly to a stand-still.", victim);
    if (ch != victim)
        ch->send_line("Ok.");
}

void spell_heal(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    Char *victim = (Char *)vo;
    int heal;

    heal = 2 * (dice(3, 8) + level - 6);
    victim->hit = UMIN(victim->hit + heal, victim->max_hit);
    update_pos(victim);
    victim->send_line("A warm feeling fills your body.");
    if (ch != victim)
        ch->send_line("Ok.");
    else
        act("$n glows with warmth.", ch, nullptr, victim, To::NotVict);
}

/* RT really nasty high-level attack spell */
/* PGW it was crap so i changed it*/
void spell_holy_word(int sn, int level, Char *ch, void *vo) {
    (void)vo;
    Char *vch;
    Char *vch_next;
    int dam;
    int bless_num, curse_num, frenzy_num;

    bless_num = skill_lookup("bless");
    curse_num = skill_lookup("curse");
    frenzy_num = skill_lookup("frenzy");

    act("$n utters a word of divine power!", ch);
    ch->send_line("You utter a word of divine power.");

    for (vch = ch->in_room->people; vch != nullptr; vch = vch_next) {
        vch_next = vch->next_in_room;

        if ((ch->is_good() && vch->is_good()) || (ch->is_evil() && vch->is_evil())
            || (ch->is_neutral() && vch->is_neutral())) {
            vch->send_line("You feel full more powerful.");
            spell_frenzy(frenzy_num, level, ch, (void *)vch);
            spell_bless(bless_num, level, ch, (void *)vch);
        }

        else if ((ch->is_good() && vch->is_evil()) || (ch->is_evil() && vch->is_good())) {
            if (!is_safe_spell(ch, vch, true)) {
                spell_curse(curse_num, level, ch, (void *)vch);
                vch->send_line("You are struck down!");
                dam = dice(level, 14);
                damage(ch, vch, dam, sn, DAM_ENERGY);
            }
        }

        else if (ch->is_neutral()) {
            if (!is_safe_spell(ch, vch, true)) {
                spell_curse(curse_num, level / 2, ch, (void *)vch);
                vch->send_line("You are struck down!");
                dam = dice(level, 10);
                damage(ch, vch, dam, sn, DAM_ENERGY);
            }
        }
    }

    ch->send_line("You feel drained.");
    ch->move = 10;
    ch->hit = (ch->hit * 3) / 4;
}

void spell_identify(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    OBJ_DATA *obj = (OBJ_DATA *)vo;
    char buf[MAX_STRING_LENGTH];

    ch->send_line("Object '{}' is type {}, extra flags {}.", obj->name, item_type_name(obj),
                  extra_bit_name(obj->extra_flags));
    ch->send_line("Weight is {}, value is {}, level is {}.", obj->weight, obj->cost, obj->level);

    if ((obj->material != MATERIAL_NONE) && (obj->material != MATERIAL_DEFAULT)) {
        snprintf(buf, sizeof(buf), "Made of %s.\n\r", material_table[obj->material].material_name);
        ch->send_to(buf);
    }

    switch (obj->item_type) {
    case ITEM_SCROLL:
    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_BOMB:
        snprintf(buf, sizeof(buf), "Level %d spells of:", obj->value[0]);
        ch->send_to(buf);

        if (obj->value[1] >= 0 && obj->value[1] < MAX_SKILL) {
            ch->send_line(" '");
            ch->send_to(skill_table[obj->value[1]].name);
            ch->send_line("'");
        }

        if (obj->value[2] >= 0 && obj->value[2] < MAX_SKILL) {
            ch->send_line(" '");
            ch->send_to(skill_table[obj->value[2]].name);
            ch->send_line("'");
        }

        if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL) {
            ch->send_line(" '");
            ch->send_to(skill_table[obj->value[3]].name);
            ch->send_line("'");
        }

        if (obj->value[4] >= 0 && obj->value[4] < MAX_SKILL && obj->item_type == ITEM_BOMB) {
            ch->send_line(" '");
            ch->send_to(skill_table[obj->value[4]].name);
            ch->send_line("'");
        }

        ch->send_line(".");
        break;

    case ITEM_WAND:
    case ITEM_STAFF:
        snprintf(buf, sizeof(buf), "Has %d(%d) charges of level %d", obj->value[1], obj->value[2], obj->value[0]);
        ch->send_to(buf);

        if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL) {
            ch->send_line(" '");
            ch->send_to(skill_table[obj->value[3]].name);
            ch->send_line("'");
        }

        ch->send_line(".");
        break;

    case ITEM_WEAPON:
        ch->send_line("Weapon type is ");
        switch (obj->value[0]) {
        case (WEAPON_EXOTIC): ch->send_line("exotic."); break;
        case (WEAPON_SWORD): ch->send_line("sword."); break;
        case (WEAPON_DAGGER): ch->send_line("dagger."); break;
        case (WEAPON_SPEAR): ch->send_line("spear/staff."); break;
        case (WEAPON_MACE): ch->send_line("mace/club."); break;
        case (WEAPON_AXE): ch->send_line("axe."); break;
        case (WEAPON_FLAIL): ch->send_line("flail."); break;
        case (WEAPON_WHIP): ch->send_line("whip."); break;
        case (WEAPON_POLEARM): ch->send_line("polearm."); break;
        default: ch->send_line("unknown."); break;
        }
        if ((obj->value[4] != 0) && (obj->item_type == ITEM_WEAPON)) {
            ch->send_line("Weapon flags:");
            if (IS_SET(obj->value[4], WEAPON_FLAMING))
                ch->send_line(" flaming");
            if (IS_SET(obj->value[4], WEAPON_FROST))
                ch->send_line(" frost");
            if (IS_SET(obj->value[4], WEAPON_VAMPIRIC))
                ch->send_line(" vampiric");
            if (IS_SET(obj->value[4], WEAPON_SHARP))
                ch->send_line(" sharp");
            if (IS_SET(obj->value[4], WEAPON_VORPAL))
                ch->send_line(" vorpal");
            if (IS_SET(obj->value[4], WEAPON_TWO_HANDS))
                ch->send_line(" two-handed");
            if (IS_SET(obj->value[4], WEAPON_POISONED))
                ch->send_line(" poisoned");
            if (IS_SET(obj->value[4], WEAPON_PLAGUED))
                ch->send_line(" death");
            if (IS_SET(obj->value[4], WEAPON_ACID))
                ch->send_line(" acid");
            if (IS_SET(obj->value[4], WEAPON_LIGHTNING))
                ch->send_line(" lightning");
            ch->send_line(".");
        }
        snprintf(buf, sizeof(buf), "Damage is %dd%d (average %d).\n\r", obj->value[1], obj->value[2],
                 (1 + obj->value[2]) * obj->value[1] / 2);
        ch->send_to(buf);
        break;

    case ITEM_ARMOR:
        snprintf(buf, sizeof(buf), "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n\r", obj->value[0],
                 obj->value[1], obj->value[2], obj->value[3]);
        ch->send_to(buf);
        break;
    }

    if (!obj->enchanted)
        for (auto &af : obj->pIndexData->affected) {
            if (af.affects_stats())
                ch->send_line("Affects {}.", af.describe_item_effect());
        }

    for (auto &af : obj->affected) {
        if (af.affects_stats())
            ch->send_line("Affects {}.", af.describe_item_effect());
    }
}

void spell_infravision(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_INFRARED)) {
        if (victim == ch)
            ch->send_line("You can already see in the dark.");
        else
            act("$N already has infravision.\n\r", ch, nullptr, victim, To::Char);
        return;
    }
    act("$n's eyes glow red.\n\r", ch);
    af.type = sn;
    af.level = level;
    af.duration = 2 * level;
    af.bitvector = AFF_INFRARED;
    affect_to_char(victim, af);
    victim->send_line("Your eyes glow red.");
}

void spell_invis(int sn, int level, Char *ch, void *vo) {
    (void)ch;
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_INVISIBLE))
        return;

    act("$n fades out of existence.", victim);
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char(victim, af);
    victim->send_line("You fade out of existence.");
}

void spell_know_alignment(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    Char *victim = (Char *)vo;
    const char *msg;
    int ap;

    ap = victim->alignment;

    if (ap > 700)
        msg = "$N has a pure and good aura.";
    else if (ap > 350)
        msg = "$N is of excellent moral character.";
    else if (ap > 100)
        msg = "$N is often kind and thoughtful.";
    else if (ap > -100)
        msg = "$N doesn't have a firm moral commitment.";
    else if (ap > -350)
        msg = "$N lies to $S friends.";
    else if (ap > -700)
        msg = "$N is a black-hearted murderer.";
    else
        msg = "$N is the embodiment of pure evil!.";

    act(msg, ch, nullptr, victim, To::Char);
}

void spell_lightning_bolt(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    auto dam_level = get_direct_dmg_and_level(level, LightningBoltCauseCritDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, sn, DAM_LIGHTNING);
}

void spell_locate_object(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)vo;

    bool found = false;
    int number = 0;
    int max_found = ch->is_immortal() ? 200 : 2 * level;
    std::string buffer;
    for (auto *obj = object_list; obj != nullptr; obj = obj->next) {
        if (!ch->can_see(*obj) || !is_name(target_name, obj->name) || (ch->is_mortal() && number_percent() > 2 * level)
            || ch->level < obj->level || IS_SET(obj->extra_flags, ITEM_NO_LOCATE))
            continue;

        found = true;
        number++;

        auto *in_obj = obj;
        for (; in_obj->in_obj != nullptr; in_obj = in_obj->in_obj)
            ;

        if (in_obj->carried_by != nullptr /*&& can_see TODO wth was this supposed to be?*/) {
            if (ch->is_immortal()) {
                buffer += fmt::format("{} carried by {} in {} [Room {}]\n\r", InitialCap{obj->short_descr},
                                      pers(in_obj->carried_by, ch), in_obj->carried_by->in_room->name,
                                      in_obj->carried_by->in_room->vnum);
            } else
                buffer +=
                    fmt::format("{} carried by {}\n\r", InitialCap{obj->short_descr}, pers(in_obj->carried_by, ch));
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

void spell_magic_missile(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    auto dam_level = get_direct_dmg_and_level(level, MagicMissileCauseLightDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, sn, DAM_ENERGY);
}

void spell_mass_healing(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)vo;
    Char *gch;
    int heal_num, refresh_num;

    heal_num = skill_lookup("heal");
    refresh_num = skill_lookup("refresh");

    for (gch = ch->in_room->people; gch != nullptr; gch = gch->next_in_room) {
        if ((ch->is_npc() && gch->is_npc()) || (ch->is_pc() && gch->is_pc())) {
            spell_heal(heal_num, level, ch, (void *)gch);
            spell_refresh(refresh_num, level, ch, (void *)gch);
        }
    }
}

void spell_mass_invis(int sn, int level, Char *ch, void *vo) {
    (void)vo;
    AFFECT_DATA af;
    Char *gch;

    for (gch = ch->in_room->people; gch != nullptr; gch = gch->next_in_room) {
        if (!is_same_group(gch, ch) || IS_AFFECTED(gch, AFF_INVISIBLE))
            continue;
        act("$n slowly fades out of existence.", gch);
        gch->send_line("You slowly fade out of existence.");
        af.type = sn;
        af.level = level / 2;
        af.duration = 24;
        af.bitvector = AFF_INVISIBLE;
        affect_to_char(gch, af);
    }
    ch->send_line("Ok.");
}

void spell_null(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    (void)vo;
    ch->send_line("That's not a spell!");
}

void spell_octarine_fire(int sn, int level, Char *ch, void *vo) {
    (void)ch;
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_OCTARINE_FIRE))
        return;
    af.type = sn;
    af.level = level;
    af.duration = 2;
    af.location = AffectLocation::Ac;
    af.modifier = 10 * level;
    af.bitvector = AFF_OCTARINE_FIRE;
    affect_to_char(victim, af);
    victim->send_line("You are surrounded by an octarine outline.");
    act("$n is surrounded by a octarine outline.", victim);
}

void spell_pass_door(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_PASS_DOOR)) {
        if (victim == ch)
            ch->send_line("You are already out of phase.");
        else
            act("$N is already shifted out of phase.", ch, nullptr, victim, To::Char);
        return;
    }
    af.type = sn;
    af.level = level;
    af.duration = number_fuzzy(level / 4);
    af.bitvector = AFF_PASS_DOOR;
    affect_to_char(victim, af);
    act("$n turns translucent.", victim);
    victim->send_line("You turn translucent.");
}

/* RT plague spell, very nasty */

void spell_plague(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (saves_spell(level, victim) || (victim->is_npc() && IS_SET(victim->act, ACT_UNDEAD))) {
        if (ch == victim)
            ch->send_line("You feel momentarily ill, but it passes.");
        else
            act("$N seems to be unaffected.", ch, nullptr, victim, To::Char);
        return;
    }

    af.type = sn;
    af.level = level * 3 / 4;
    af.duration = level;
    af.location = AffectLocation::Str;
    af.modifier = -5;
    af.bitvector = AFF_PLAGUE;
    affect_join(victim, af);

    victim->send_line("You scream in agony as plague sores erupt from your skin.");
    act("$n screams in agony as plague sores erupt from $s skin.", victim);
}

void spell_portal(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)vo;
    const auto *victim = get_char_world(ch, target_name);
    if (!victim || victim == ch || victim->in_room == nullptr || !can_see_room(ch, victim->in_room)
        || IS_SET(victim->in_room->room_flags, ROOM_SAFE) || IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
        || IS_SET(victim->in_room->room_flags, ROOM_SOLITARY) || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
        || IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) || IS_SET(victim->in_room->room_flags, ROOM_LAW)
        || victim->level >= level + 3 || (victim->is_pc() && victim->level >= LEVEL_HERO) /* NOT trust */
        || (victim->is_npc() && IS_SET(victim->imm_flags, IMM_SUMMON))
        || (victim->is_pc() && IS_SET(victim->act, PLR_NOSUMMON)) || (victim->is_npc() && saves_spell(level, victim))) {
        ch->send_line("You failed.");
        return;
    }
    if (IS_SET(ch->in_room->room_flags, ROOM_LAW)) {
        ch->send_line("You cannot portal from this room.");
        return;
    }
    auto *source_portal = create_object(get_obj_index(OBJ_VNUM_PORTAL));
    source_portal->timer = (ch->level / 10);
    source_portal->destination = victim->in_room;

    // Put portal in current room.
    source_portal->description = fmt::sprintf(source_portal->description, victim->in_room->name);
    obj_to_room(source_portal, ch->in_room);

    // Create second portal.
    auto *dest_portal = create_object(get_obj_index(OBJ_VNUM_PORTAL));
    dest_portal->timer = (ch->level / 10);
    dest_portal->destination = ch->in_room;

    /* Put portal, in destination room, for this room */
    dest_portal->description = fmt::sprintf(dest_portal->description, ch->in_room->name);
    obj_to_room(dest_portal, victim->in_room);

    ch->send_line("You wave your hands madly, and create a portal.");
}

void spell_poison(int sn, int level, Char *ch, void *vo) {
    (void)ch;
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (saves_spell(level, victim)) {
        act("$n turns slightly green, but it passes.", victim);
        victim->send_line("You feel momentarily ill, but it passes.");
        return;
    }
    af.type = sn;
    af.level = level;
    af.duration = level;
    af.location = AffectLocation::Str;
    af.modifier = -2;
    af.bitvector = AFF_POISON;
    affect_join(victim, af);
    victim->send_line("You feel very sick.");
    act("$n looks very ill.", victim);
}

void spell_protection_evil(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_PROTECTION_EVIL)) {
        if (victim == ch)
            ch->send_line("You are already protected from evil.");
        else
            act("$N is already protected from evil.", ch, nullptr, victim, To::Char);
        return;
    }
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.bitvector = AFF_PROTECTION_EVIL;
    affect_to_char(victim, af);
    victim->send_line("You feel protected from evil.");
    if (ch != victim)
        act("$N is protected from harm.", ch, nullptr, victim, To::Char);
}

void spell_protection_good(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_PROTECTION_GOOD)) {
        if (victim == ch)
            ch->send_line("You are already protected from good.");
        else
            act("$N is already protected from good.", ch, nullptr, victim, To::Char);
        return;
    }
    af.type = sn;
    af.level = level;
    af.duration = 24;
    af.bitvector = AFF_PROTECTION_GOOD;
    affect_to_char(victim, af);
    victim->send_line("You feel protected from good.");
    if (ch != victim)
        act("$N is protected from harm.", ch, nullptr, victim, To::Char);
}

void spell_refresh(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    Char *victim = (Char *)vo;
    victim->move = UMIN(victim->move + level, victim->max_move);
    if (victim->max_move == victim->move)
        victim->send_line("You feel fully refreshed!");
    else
        victim->send_line("You feel less tired.");
    if (ch != victim)
        act("$N looks invigorated.", ch, nullptr, victim, To::Char);
}

void spell_remove_curse(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)ch;
    Char *victim = (Char *)vo;
    bool found = false;
    OBJ_DATA *obj;
    int iWear;

    if (check_dispel(level, victim, gsn_curse)) {
        victim->send_line("You feel better.");
        act("$n looks more relaxed.", victim);
    }

    for (iWear = 0; (iWear < MAX_WEAR && !found); iWear++) {
        if ((obj = get_eq_char(victim, iWear)) == nullptr)
            continue;

        if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOREMOVE)) { /* attempt to remove curse */
            if (!saves_dispel(level, obj->level, 0)) {
                found = true;
                REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
                REMOVE_BIT(obj->extra_flags, ITEM_NOREMOVE);
                act("$p glows blue.", victim, obj, nullptr, To::Char);
                act("$p glows blue.", victim, obj, nullptr, To::Room);
            }
        }
    }

    for (obj = victim->carrying; (obj != nullptr && !found); obj = obj->next_content) {
        if (IS_OBJ_STAT(obj, ITEM_NODROP) || IS_OBJ_STAT(obj, ITEM_NOREMOVE)) { /* attempt to remove curse */
            if (!saves_dispel(level, obj->level, 0)) {
                found = true;
                REMOVE_BIT(obj->extra_flags, ITEM_NODROP);
                REMOVE_BIT(obj->extra_flags, ITEM_NOREMOVE);
                act("Your $p glows blue.", victim, obj, nullptr, To::Char);
                act("$n's $p glows blue.", victim, obj, nullptr, To::Room);
            }
        }
    }
}

void spell_sanctuary(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_SANCTUARY)) {
        if (victim == ch)
            ch->send_line("You are already in sanctuary.");
        else
            act("$N is already in sanctuary.", ch, nullptr, victim, To::Char);
        return;
    }
    af.type = sn;
    af.level = level;
    af.duration = number_fuzzy(level / 5);
    af.bitvector = AFF_SANCTUARY;
    affect_to_char(victim, af);
    act("$n is surrounded by a white aura.", victim);
    victim->send_line("You are surrounded by a white aura.");
}

void spell_talon(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn)) {
        if (victim == ch)
            ch->send_line("Your hands are already as strong as talons.");
        else
            act("$N already has talon like hands.", ch, nullptr, victim, To::Char);
        return;
    }
    af.type = sn;
    af.level = level;
    af.duration = number_fuzzy(level / 6);
    af.bitvector = AFF_TALON;
    affect_to_char(victim, af);
    act("$n's hands become as strong as talons.", victim);
    victim->send_line("You hands become as strong as talons.");
}

void spell_shield(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn)) {
        if (victim == ch)
            ch->send_line("You are already shielded from harm.");
        else
            act("$N is already protected by a shield.", ch, nullptr, victim, To::Char);
        return;
    }
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

void spell_shocking_grasp(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    auto dam_level = get_direct_dmg_and_level(level, ShockingGraspDamage);
    if (saves_spell(dam_level.second, victim))
        dam_level.first /= 2;
    damage(ch, victim, dam_level.first, sn, DAM_LIGHTNING);
}

void spell_sleep(int sn, int level, Char *ch, void *vo) {
    (void)ch;
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (IS_AFFECTED(victim, AFF_SLEEP) || (victim->is_npc() && IS_SET(victim->act, ACT_UNDEAD)) || level < victim->level
        || saves_spell(level, victim))
        return;

    af.type = sn;
    af.level = level;
    af.duration = 4 + level;
    af.bitvector = AFF_SLEEP;
    affect_join(victim, af);

    if (IS_AWAKE(victim)) {
        victim->send_line("You feel very sleepy ..... zzzzzz.");
        act("$n goes to sleep.", victim);
        victim->position = POS_SLEEPING;
    }
}

void spell_stone_skin(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (is_affected(ch, sn)) {
        if (victim == ch)
            ch->send_line("Your skin is already as hard as a rock.");
        else
            act("$N is already as hard as can be.", ch, nullptr, victim, To::Char);
        return;
    }
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

void spell_summon(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)vo;
    Char *victim;

    if ((victim = get_char_world(ch, target_name)) == nullptr || victim == ch || victim->in_room == nullptr
        || ch->in_room == nullptr || IS_SET(victim->in_room->room_flags, ROOM_SAFE)
        || IS_SET(victim->in_room->room_flags, ROOM_PRIVATE) || IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
        || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
        || (victim->is_npc() && IS_SET(victim->act, ACT_AGGRESSIVE)) || victim->level >= level + 3
        || (victim->is_pc() && victim->level >= LEVEL_HERO) || victim->fighting != nullptr
        || (victim->is_npc() && IS_SET(victim->imm_flags, IMM_SUMMON))
        || (victim->is_pc() && IS_SET(victim->act, PLR_NOSUMMON)) || (victim->is_npc() && saves_spell(level, victim))
        || (IS_SET(ch->in_room->room_flags, ROOM_SAFE))) {
        ch->send_line("You failed.");
        return;
    }
    if (IS_SET(ch->in_room->room_flags, ROOM_LAW)) {
        ch->send_line("You'd probably get locked behind bars for that!");
        return;
    }
    if (victim->is_npc()) {
        if (victim->pIndexData->pShop != nullptr || IS_SET(victim->act, ACT_IS_HEALER) || IS_SET(victim->act, ACT_GAIN)
            || IS_SET(victim->act, ACT_PRACTICE)) {
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

void spell_teleport(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    Char *victim = (Char *)vo;
    ROOM_INDEX_DATA *pRoomIndex;

    if (victim->in_room == nullptr || IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
        || (ch->is_pc() && victim->fighting != nullptr)
        || (victim != ch && (saves_spell(level, victim) || saves_spell(level, victim)))) {
        ch->send_line("You failed.");
        return;
    }

    for (;;) {
        pRoomIndex = get_room_index(number_range(0, 65535));
        if (pRoomIndex != nullptr)
            if (can_see_room(ch, pRoomIndex) && !IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)
                && !IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY))
                break;
    }

    if (victim != ch)
        victim->send_line("You have been teleported!");

    act("$n vanishes!", victim);
    char_from_room(victim);
    char_to_room(victim, pRoomIndex);
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

void spell_ventriloquate(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)vo;
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char speaker[MAX_INPUT_LENGTH];
    Char *vch;

    target_name = one_argument(target_name, speaker);

    snprintf(buf1, sizeof(buf1), "%s says '%s'.\n\r", speaker, target_name);
    snprintf(buf2, sizeof(buf2), "Someone makes %s say '%s'.\n\r", speaker, target_name);
    buf1[0] = UPPER(buf1[0]);

    for (vch = ch->in_room->people; vch != nullptr; vch = vch->next_in_room) {
        if (!is_name(speaker, vch->name))
            vch->send_to(saves_spell(level, vch) ? buf2 : buf1);
    }
}

void spell_weaken(int sn, int level, Char *ch, void *vo) {
    (void)ch;
    Char *victim = (Char *)vo;
    AFFECT_DATA af;

    if (is_affected(victim, sn) || saves_spell(level, victim)) {
        act("$n looks unsteady for a moment, but it passes.", victim);
        victim->send_line("You feel unsteady for a moment, but it passes.");
        return;
    }
    af.type = sn;
    af.level = level;
    af.duration = level / 2;
    af.location = AffectLocation::Str;
    af.modifier = -1 * (level / 5);
    af.bitvector = AFF_WEAKEN;
    affect_to_char(victim, af);
    victim->send_line("You feel weaker.");
    act("$n looks tired and weak.", victim);
}

/* RT recall spell is back */

void spell_word_of_recall(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    Char *victim = (Char *)vo;
    ROOM_INDEX_DATA *location;

    if (victim->is_npc())
        return;

    if ((location = get_room_index(ROOM_VNUM_TEMPLE)) == nullptr) {
        victim->send_line("You are completely lost.");
        return;
    }

    if (IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL) || IS_AFFECTED(victim, AFF_CURSE)) {
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
void spell_acid_breath(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    OBJ_DATA *obj_lose;
    OBJ_DATA *obj_next;
    OBJ_DATA *t_obj, *n_obj;
    int dam;
    int hpch;
    int i;

    if (number_percent() < 2 * level && !saves_spell(level, victim) && ch->in_room->vnum != CHAL_ROOM) {
        for (obj_lose = victim->carrying; obj_lose != nullptr; obj_lose = obj_next) {
            int iWear;

            obj_next = obj_lose->next_content;

            if (number_bits(2) != 0)
                continue;

            switch (obj_lose->item_type) {
            case ITEM_ARMOR:
                if (obj_lose->value[0] > 0) {
                    act("$p is pitted and etched!", victim, obj_lose, nullptr, To::Char);
                    if ((iWear = obj_lose->wear_loc) != WEAR_NONE)
                        for (i = 0; i < 4; i++)
                            victim->armor[i] -= apply_ac(obj_lose, iWear, i);
                    for (i = 0; i < 4; i++)
                        obj_lose->value[i] -= 1;
                    obj_lose->cost = 0;
                    if (iWear != WEAR_NONE)
                        for (i = 0; i < 4; i++)
                            victim->armor[i] += apply_ac(obj_lose, iWear, i);
                }
                break;

            case ITEM_CONTAINER:
                if (!IS_OBJ_STAT(obj_lose, ITEM_PROTECT_CONTAINER)) {
                    act("$p fumes and dissolves, destroying some of the contents.", victim, obj_lose, nullptr,
                        To::Char);
                    /* save some of  the contents */

                    for (t_obj = obj_lose->contains; t_obj != nullptr; t_obj = n_obj) {
                        n_obj = t_obj->next_content;
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
            }
        }
    }

    hpch = UMAX(10, ch->hit);
    if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && ch->is_pc())
        hpch = 1000;
    dam = number_range(hpch / 20 + 1, hpch / 10);
    if (saves_spell(level, victim))
        dam /= 2;
    damage(ch, victim, dam, sn, DAM_ACID);
}

void spell_fire_breath(int sn, int level, Char *ch, void *vo) {
    /* Limit damage for PCs added by Rohan on all draconian*/
    Char *victim = (Char *)vo;
    OBJ_DATA *obj_lose;
    OBJ_DATA *obj_next;
    OBJ_DATA *t_obj, *n_obj;
    int dam;
    int hpch;

    if (number_percent() < 2 * level && !saves_spell(level, victim) && ch->in_room->vnum != CHAL_ROOM) {
        for (obj_lose = victim->carrying; obj_lose != nullptr; obj_lose = obj_next) {
            const char *msg;

            obj_next = obj_lose->next_content;
            if (number_bits(2) != 0)
                continue;

            switch (obj_lose->item_type) {
            default: continue;
            case ITEM_POTION: msg = "$p bubbles and boils!"; break;
            case ITEM_SCROLL: msg = "$p crackles and burns!"; break;
            case ITEM_STAFF: msg = "$p smokes and chars!"; break;
            case ITEM_WAND: msg = "$p sparks and sputters!"; break;
            case ITEM_FOOD: msg = "$p blackens and crisps!"; break;
            case ITEM_PILL: msg = "$p melts and drips!"; break;
            }

            act(msg, victim, obj_lose, nullptr, To::Char);
            if (obj_lose->item_type == ITEM_CONTAINER) {
                /* save some of  the contents */

                if (!IS_OBJ_STAT(obj_lose, ITEM_PROTECT_CONTAINER)) {
                    msg = "$p ignites and burns!";
                    break;
                    act(msg, victim, obj_lose, nullptr, To::Char);

                    for (t_obj = obj_lose->contains; t_obj != nullptr; t_obj = n_obj) {
                        n_obj = t_obj->next_content;
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
            if (!IS_OBJ_STAT(obj_lose, ITEM_PROTECT_CONTAINER))
                extract_obj(obj_lose);
        }
    }

    hpch = UMAX(10, ch->hit);
    if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && ch->is_pc())
        hpch = 1000;
    dam = number_range(hpch / 20 + 1, hpch / 10);
    if (saves_spell(level, victim))
        dam /= 2;
    damage(ch, victim, dam, sn, DAM_FIRE);
}

void spell_frost_breath(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    OBJ_DATA *obj_lose;
    OBJ_DATA *obj_next;

    if (number_percent() < 2 * level && !saves_spell(level, victim) && ch->in_room->vnum != CHAL_ROOM) {
        for (obj_lose = victim->carrying; obj_lose != nullptr; obj_lose = obj_next) {
            const char *msg;

            obj_next = obj_lose->next_content;
            if (number_bits(2) != 0)
                continue;

            switch (obj_lose->item_type) {
            default: continue;
            case ITEM_DRINK_CON:
            case ITEM_POTION: msg = "$p freezes and shatters!"; break;
            }

            act(msg, victim, obj_lose, nullptr, To::Char);
            extract_obj(obj_lose);
        }
    }

    int hpch = UMAX(10, ch->hit);
    if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && ch->is_pc())
        hpch = 1000;
    int dam = number_range(hpch / 20 + 1, hpch / 10);
    if (saves_spell(level, victim))
        dam /= 2;
    damage(ch, victim, dam, sn, DAM_COLD);
}

void spell_gas_breath(int sn, int level, Char *ch, void *vo) {
    (void)vo;
    Char *vch;
    Char *vch_next;
    int dam;
    int hpch;

    for (vch = ch->in_room->people; vch != nullptr; vch = vch_next) {
        vch_next = vch->next_in_room;
        if (!is_safe_spell(ch, vch, true)) {
            hpch = UMAX(10, ch->hit);
            if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && ch->is_pc())
                hpch = 1000;
            dam = number_range(hpch / 20 + 1, hpch / 10);
            if (saves_spell(level, vch))
                dam /= 2;
            damage(ch, vch, dam, sn, DAM_POISON);
        }
    }
}

void spell_lightning_breath(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    int dam;
    int hpch;

    hpch = UMAX(10, ch->hit);
    if (hpch > 1000 && ch->level < MAX_LEVEL - 7 && ch->is_pc())
        hpch = 1000;
    dam = number_range(hpch / 20 + 1, hpch / 10);
    if (saves_spell(level, victim))
        dam /= 2;
    damage(ch, victim, dam, sn, DAM_LIGHTNING);
}

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
void spell_general_purpose(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    int dam;

    dam = number_range(level, level * 3);
    if (saves_spell(level, victim))
        dam /= 2;
    damage(ch, victim, dam, sn, DAM_PIERCE);
}

void spell_high_explosive(int sn, int level, Char *ch, void *vo) {
    Char *victim = (Char *)vo;
    int dam;
    dam = number_range(level, level * 3);
    if (saves_spell(level, victim))
        dam /= 2;
    damage(ch, victim, dam, sn, DAM_PIERCE);
}

void explode_bomb(OBJ_DATA *bomb, Char *ch, Char *thrower) {
    int chance;
    int sn, position;
    void *vo = (void *)ch;

    chance = URANGE(5, (50 + ((bomb->value[0] - ch->level) * 8)), 100);
    if (number_percent() > chance) {
        act("$p emits a loud bang and disappears in a cloud of smoke.", ch, bomb, nullptr, To::Room);
        act("$p emits a loud bang, but thankfully does not affect you.", ch, bomb, nullptr, To::Char);
        extract_obj(bomb);
        return;
    }

    for (position = 1; ((position <= 4) && (bomb->value[position] < MAX_SKILL) && (bomb->value[position] != -1));
         position++) {
        sn = bomb->value[position];
        if (skill_table[sn].target == TAR_CHAR_OFFENSIVE)
            (*skill_table[sn].spell_fun)(sn, bomb->value[0], thrower, vo);
    }
    extract_obj(bomb);
}

void spell_teleport_object(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    (void)vo;
    Char *victim;
    OBJ_DATA *object;
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    ROOM_INDEX_DATA *old_room;

    target_name = one_argument(target_name, arg1);
    one_argument(target_name, arg2);

    if (arg1[0] == '\0') {
        ch->send_line("Teleport what and to whom?");
        return;
    }

    if ((object = get_obj_carry(ch, arg1)) == nullptr) {
        ch->send_line("You're not carrying that.");
        return;
    }

    if (arg2[0] == '\0') {
        ch->send_line("Teleport to whom?");
        return;
    }

    if ((victim = get_char_world(ch, arg2)) == nullptr) {
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

    if (IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)) {
        ch->send_line("You failed.");
        return;
    }

    if (IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)) {
        ch->send_line("You failed.");
        return;
    }

    /* Check to see if the victim is actually already in the same room */
    // TODO: we can break do_give() into pieces and call it directly instead of this nonsense.
    if (ch->in_room != victim->in_room) {
        act("You feel a brief presence in the room.", victim, nullptr, nullptr, To::Char);
        act("You feel a brief presence in the room.", victim);
        snprintf(buf, sizeof(buf), "'%s' %s", object->name.c_str(), victim->name.c_str());
        old_room = ch->in_room;
        char_from_room(ch);
        char_to_room(ch, victim->in_room);
        do_give(ch, buf);
        char_from_room(ch);
        char_to_room(ch, old_room);
    } else {
        snprintf(buf, sizeof(buf), "'%s' %s", object->name.c_str(), victim->name.c_str());
        do_give(ch, buf);
    } /* ..else... if not in same room */
}

void spell_undo_spell(int sn, int level, Char *ch, void *vo) {
    (void)sn;
    (void)level;
    (void)vo;
    Char *victim;
    int spell_undo;
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];

    target_name = one_argument(target_name, arg1);
    one_argument(target_name, arg2);

    if (arg2[0] == '\0') {
        victim = ch;
    } else {
        if ((victim = get_char_room(ch, arg2)) == nullptr) {
            ch->send_line("They're not here.");
            return;
        }
    }

    if (arg1[0] == '\0') {
        ch->send_line("Undo which spell?");
        return;
    }

    if ((spell_undo = skill_lookup(arg1)) < 0) {
        ch->send_line("What kind of spell is that?");
        return;
    }

    if (!is_affected(victim, spell_undo)) {
        if (victim == ch) {
            ch->send_line("You're not affected by that.");
        } else {
            act("$N doesn't seem to be affected by that.", ch, nullptr, victim, To::Char);
        }
        return;
    }

    if (check_dispel(ch->level, victim, spell_undo)) {
        if (!str_cmp(skill_table[spell_undo].name, "blindness"))
            act("$n is no longer blinded.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "calm"))
            act("$n no longer looks so peaceful...", victim);
        if (!str_cmp(skill_table[spell_undo].name, "change sex"))
            act("$n looks more like $mself again.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "charm person"))
            act("$n regains $s free will.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "chill touch"))
            act("$n looks warmer.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "faerie fire"))
            act("$n's outline fades.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "octarine fire"))
            act("$n's octarine outline fades.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "fly"))
            act("$n falls to the ground!", victim);
        if (!str_cmp(skill_table[spell_undo].name, "frenzy"))
            act("$n no longer looks so wild.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "giant strength"))
            act("$n no longer looks so mighty.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "haste"))
            act("$n is no longer moving so quickly.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "talon"))
            act("$n hand's return to normal.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "lethargy"))
            act("$n is looking more lively.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "insanity"))
            act("$n looks less confused.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "invis"))
            act("$n fades into existance.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "mass invis"))
            act("$n fades into existance.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "sanctuary"))
            act("The white aura around $n's body vanishes.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "shield"))
            act("The shield protecting $n vanishes.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "stone skin"))
            act("$n's skin regains its normal texture.", victim);
        if (!str_cmp(skill_table[spell_undo].name, "weaken"))
            act("$n looks stronger.", victim);

        ch->send_line("Ok.");
    } else {
        ch->send_line("Spell failed.");
    }
}
