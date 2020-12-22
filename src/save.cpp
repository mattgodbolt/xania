/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  save.c:  data saving routines                                        */
/*                                                                       */
/*************************************************************************/

#include "save.hpp"
#include "AFFECT_DATA.hpp"
#include "TimeInfoData.hpp"
#include "common/Configuration.hpp"
#include "db.h"
#include "handler.hpp"
#include "lookup.h"
#include "merc.h"
#include "string_utils.hpp"

#include <fmt/format.h>
#include <range/v3/algorithm/fill.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/reverse.hpp>

#include <cstdio>
#include <cstring>

std::string filename_for_player(std::string_view player_name) {
    return fmt::format("{}{}", Configuration::singleton().player_dir(), initial_caps_only(player_name));
}

std::string filename_for_god(std::string_view player_name) {
    return fmt::format("{}{}", Configuration::singleton().gods_dir(), initial_caps_only(player_name));
}

char *login_from;
char *login_at;

/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST 100
static OBJ_DATA *rgObjNest[MAX_NEST];

extern void char_ride(Char *ch, Char *pet);

/*
 * Local functions.
 */
void fwrite_char(const Char *ch, FILE *fp);
void fwrite_objs(const Char *ch, const GenericList<OBJ_DATA *> &objs, FILE *fp, int iNest = 0);
void fwrite_one_obj(const Char *ch, const OBJ_DATA *obj, FILE *fp, int iNest);
void fwrite_pet(const Char *ch, const Char *pet, FILE *fp);
void fread_char(Char *ch, FILE *fp);
void fread_pet(Char *ch, FILE *fp);
void fread_obj(Char *ch, FILE *fp);

std::string extra_bit_string(const Char &ch) {
    std::string buf(MAX_EXTRA_FLAGS, '0');
    for (int n = 0; n < MAX_EXTRA_FLAGS; n++)
        if (ch.is_set_extra(n))
            buf[n] = '1';
    return buf;
}

void set_bits_from_pfile(Char *ch, FILE *fp) {
    int n;
    char c;
    for (n = 0; n <= MAX_EXTRA_FLAGS; n++) {
        c = fread_letter(fp);
        if (c == '0')
            continue;
        if (c == '1') {
            set_extra(ch, n);
        } else {
            break;
        }
    }
}

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj(const Char *ch) {
    FILE *fp;

    if (ch = ch->player(); !ch)
        return;

    /* create god log */
    if (ch->is_immortal() || ch->level >= LEVEL_IMMORTAL) {
        auto godsave = filename_for_god(ch->name);
        if ((fp = fopen(godsave.c_str(), "w")) == nullptr) {
            bug("Save_char_obj: fopen");
            perror(godsave.c_str());
        }

        fprintf(fp, "Lev %2d Trust %2d  %s%s\n", ch->level, ch->get_trust(), ch->name.c_str(),
                ch->pcdata->title.c_str());
        fclose(fp);
    }

    auto player_temp = filename_for_player(ch->name + ".tmp");
    if ((fp = fopen(player_temp.c_str(), "w")) == nullptr) {
        bug("Save_char_obj: fopen {} {}", player_temp, strerror(errno));
    } else {
        save_char_obj(ch, fp);
        fclose(fp);
        /* move the file */
        auto player_file = filename_for_player(ch->name);
        if (rename(player_temp.c_str(), player_file.c_str()) != 0) {
            bug("Unable to move temporary player name {}!! rename failed: {}!", player_file.c_str(), strerror(errno));
        }
    }
}

void save_char_obj(const Char *ch, FILE *fp) {
    fwrite_char(ch, fp);
    fwrite_objs(ch, ch->carrying, fp);
    /* save the pets */
    if (ch->pet != nullptr && ch->pet->in_room == ch->in_room)
        fwrite_pet(ch, ch->pet, fp);
    fprintf(fp, "#END\n");
}

/*
 * Write the char.
 */
void fwrite_char(const Char *ch, FILE *fp) {
    int sn, gn;

    fprintf(fp, "#%s\n", ch->is_npc() ? "MOB" : "PLAYER");

    fprintf(fp, "Name %s~\n", ch->name.c_str());
    fprintf(fp, "Vers %d\n", 4);
    if (!ch->short_descr.empty())
        fprintf(fp, "ShD  %s~\n", ch->short_descr.c_str());
    if (!ch->long_descr.empty())
        fprintf(fp, "LnD  %s~\n", ch->long_descr.c_str());
    if (!ch->description.empty())
        fprintf(fp, "Desc %s~\n", ch->description.c_str());
    fprintf(fp, "Race %s~\n", pc_race_table[ch->race].name);
    fprintf(fp, "Sex  %d\n", ch->sex);
    fprintf(fp, "Cla  %d\n", ch->class_num);
    fprintf(fp, "Levl %d\n", ch->level);
    if (auto *pc_clan = ch->pc_clan()) {
        fprintf(fp, "Clan %d\n", (int)pc_clan->clan.clanchar);
        fprintf(fp, "CLevel %d\n", pc_clan->clanlevel);
        fprintf(fp, "CCFlags %d\n", pc_clan->channelflags);
    }
    if (ch->trust != 0)
        fprintf(fp, "Tru  %d\n", ch->trust);
    using namespace std::chrono;
    fprintf(fp, "Plyd %d\n", (int)duration_cast<seconds>(ch->total_played()).count());
    fprintf(fp, "Note %d\n", (int)Clock::to_time_t(ch->last_note));
    fprintf(fp, "Scro %d\n", ch->lines);
    fprintf(fp, "Room %d\n",
            (ch->in_room == get_room_index(ROOM_VNUM_LIMBO) && ch->was_in_room != nullptr)
                ? ch->was_in_room->vnum
                : ch->in_room == nullptr ? 3001 : ch->in_room->vnum);

    fprintf(fp, "HMV  %d %d %d %d %d %d\n", ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move);
    if (ch->gold > 0)
        fprintf(fp, "Gold %ld\n", ch->gold);
    else
        fprintf(fp, "Gold %d\n", 0);
    fprintf(fp, "Exp  %ld\n", ch->exp);
    if (ch->act != 0)
        fprintf(fp, "Act  %ld\n", ch->act);
    if (ch->affected_by != 0)
        fprintf(fp, "AfBy %d\n", ch->affected_by);
    fprintf(fp, "Comm %ld\n", ch->comm);
    if (ch->invis_level != 0)
        fprintf(fp, "Invi %d\n", ch->invis_level);
    fprintf(fp, "Pos  %d\n", ch->position == POS_FIGHTING ? POS_STANDING : ch->position);
    if (ch->practice != 0)
        fprintf(fp, "Prac %d\n", ch->practice);
    if (ch->train != 0)
        fprintf(fp, "Trai %d\n", ch->train);
    if (ch->saving_throw != 0)
        fprintf(fp, "Save  %d\n", ch->saving_throw);
    fprintf(fp, "Alig  %d\n", ch->alignment);
    if (ch->hitroll != 0)
        fprintf(fp, "Hit   %d\n", ch->hitroll);
    if (ch->damroll != 0)
        fprintf(fp, "Dam   %d\n", ch->damroll);
    fprintf(fp, "ACs %d %d %d %d\n", ch->armor[0], ch->armor[1], ch->armor[2], ch->armor[3]);
    if (ch->wimpy != 0)
        fprintf(fp, "Wimp  %d\n", ch->wimpy);
    fprintf(fp, "Attr %d %d %d %d %d\n", ch->perm_stat[Stat::Str], ch->perm_stat[Stat::Int], ch->perm_stat[Stat::Wis],
            ch->perm_stat[Stat::Dex], ch->perm_stat[Stat::Con]);

    fprintf(fp, "AMod %d %d %d %d %d\n", ch->mod_stat[Stat::Str], ch->mod_stat[Stat::Int], ch->mod_stat[Stat::Wis],
            ch->mod_stat[Stat::Dex], ch->mod_stat[Stat::Con]);

    if (ch->is_npc()) {
        fprintf(fp, "Vnum %d\n", ch->pIndexData->vnum);
    } else {
        fprintf(fp, "Pass %s~\n", ch->pcdata->pwd.c_str());
        if (!ch->pcdata->bamfin.empty())
            fprintf(fp, "Bin  %s~\n", ch->pcdata->bamfin.c_str());
        if (!ch->pcdata->bamfout.empty())
            fprintf(fp, "Bout %s~\n", ch->pcdata->bamfout.c_str());
        fprintf(fp, "Titl %s~\n", ch->pcdata->title.c_str());
        fprintf(fp, "Afk %s~\n", ch->pcdata->afk.c_str());
        fprintf(fp, "Colo %d\n", ch->pcdata->colour);
        fprintf(fp, "Prmt %s~\n", ch->pcdata->prompt.c_str());
        fprintf(fp, "Pnts %d\n", ch->pcdata->points);
        fprintf(fp, "TSex %d\n", ch->pcdata->true_sex);
        fprintf(fp, "LLev %d\n", ch->pcdata->last_level);
        fprintf(fp, "HMVP %d %d %d\n", ch->pcdata->perm_hit, ch->pcdata->perm_mana, ch->pcdata->perm_move);
        /* Rohan: Save info data */
        fprintf(fp, "Info_message %s~\n", ch->pcdata->info_message.c_str());
        if (ch->desc) {
            fprintf(fp, "LastLoginFrom %s~\n", ch->desc->host().c_str());
            fprintf(fp, "LastLoginAt %s~\n", ch->desc->login_time().c_str());
        }

        /* save prefix PCFN 19-05-97 */
        fprintf(fp, "Prefix %s~\n", ch->pcdata->prefix.c_str());

        // Timezone hours and minutes offset are unused currently.
        fprintf(fp, "HourOffset %d\n", ch->pcdata->houroffset);
        fprintf(fp, "MinOffset %d\n", ch->pcdata->minoffset);

        fmt::print(fp, "ExtraBits {}~\n", extra_bit_string(*ch));

        fprintf(fp, "Cond %d %d %d\n", ch->pcdata->condition[0], ch->pcdata->condition[1], ch->pcdata->condition[2]);

        fprintf(fp, "Possessive %s~\n", ch->pcdata->pronouns.possessive.c_str());
        fprintf(fp, "Subjective %s~\n", ch->pcdata->pronouns.subjective.c_str());
        fprintf(fp, "Objective %s~\n", ch->pcdata->pronouns.objective.c_str());
        fprintf(fp, "Reflexive %s~\n", ch->pcdata->pronouns.reflexive.c_str());

        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name != nullptr && ch->pcdata->learned[sn] > 0) // NOT get_skill_learned
            {
                fprintf(fp, "Sk %d '%s'\n", ch->pcdata->learned[sn], skill_table[sn].name); // NOT get_skill_learned
            }
        }

        for (gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name != nullptr && ch->pcdata->group_known[gn]) {
                fprintf(fp, "Gr '%s'\n", group_table[gn].name);
            }
        }
    }

    for (const auto &af : ch->affected) {
        if (af.type < 0 || af.type >= MAX_SKILL)
            continue;

        fprintf(fp, "AffD '%s' %3d %3d %3d %3d %10d\n", skill_table[af.type].name, af.level, af.duration, af.modifier,
                static_cast<int>(af.location), af.bitvector);
    }
    fprintf(fp, "End\n\n");
}

/* write a pet */
void fwrite_pet(const Char *ch, const Char *pet, FILE *fp) {
    fprintf(fp, "#PET\n");

    fprintf(fp, "Vnum %d\n", pet->pIndexData->vnum);

    fprintf(fp, "Name %s~\n", pet->name.c_str());
    if (pet->short_descr != pet->pIndexData->short_descr)
        fprintf(fp, "ShD  %s~\n", pet->short_descr.c_str());
    if (pet->long_descr != pet->pIndexData->long_descr)
        fprintf(fp, "LnD  %s~\n", pet->long_descr.c_str());
    if (pet->description != pet->pIndexData->description)
        fprintf(fp, "Desc %s~\n", pet->description.c_str());
    if (pet->race != pet->pIndexData->race)
        fprintf(fp, "Race %s~\n", race_table[pet->race].name);
    fprintf(fp, "Sex  %d\n", pet->sex);
    if (pet->level != pet->pIndexData->level)
        fprintf(fp, "Levl %d\n", pet->level);
    fprintf(fp, "HMV  %d %d %d %d %d %d\n", pet->hit, pet->max_hit, pet->mana, pet->max_mana, pet->move, pet->max_move);
    if (pet->gold > 0)
        fprintf(fp, "Gold %ld\n", pet->gold);
    if (pet->exp > 0)
        fprintf(fp, "Exp  %ld\n", pet->exp);
    if (pet->act != pet->pIndexData->act)
        fprintf(fp, "Act  %ld\n", pet->act);
    if (pet->affected_by != pet->pIndexData->affected_by)
        fprintf(fp, "AfBy %d\n", pet->affected_by);
    if (pet->comm != 0)
        fprintf(fp, "Comm %ld\n", pet->comm);
    fprintf(fp, "Pos  %d\n", pet->position == POS_FIGHTING ? POS_STANDING : pet->position);
    if (pet->saving_throw != 0)
        fprintf(fp, "Save %d\n", pet->saving_throw);
    if (pet->alignment != pet->pIndexData->alignment)
        fprintf(fp, "Alig %d\n", pet->alignment);
    if (pet->hitroll != pet->pIndexData->hitroll)
        fprintf(fp, "Hit  %d\n", pet->hitroll);
    if (pet->damroll != pet->pIndexData->damage.bonus())
        fprintf(fp, "Dam  %d\n", pet->damroll);
    fprintf(fp, "ACs  %d %d %d %d\n", pet->armor[0], pet->armor[1], pet->armor[2], pet->armor[3]);
    fprintf(fp, "Attr %d %d %d %d %d\n", pet->perm_stat[Stat::Str], pet->perm_stat[Stat::Int],
            pet->perm_stat[Stat::Wis], pet->perm_stat[Stat::Dex], pet->perm_stat[Stat::Con]);
    fprintf(fp, "AMod %d %d %d %d %d\n", pet->mod_stat[Stat::Str], pet->mod_stat[Stat::Int], pet->mod_stat[Stat::Wis],
            pet->mod_stat[Stat::Dex], pet->mod_stat[Stat::Con]);

    for (const auto &af : pet->affected) {
        if (af.type < 0 || af.type >= MAX_SKILL)
            continue;

        fprintf(fp, "AffD '%s' %3d %3d %3d %3d %10d\n", skill_table[af.type].name, af.level, af.duration, af.modifier,
                static_cast<int>(af.location), af.bitvector);
    }

    if (ch->riding == pet) {
        fprintf(fp, "Ridden\n");
    }

    fwrite_objs(pet, pet->carrying, fp);

    fprintf(fp, "End\n");
}

void fwrite_objs(const Char *ch, const GenericList<OBJ_DATA *> &objs, FILE *fp, int iNest) {
    // TODO: if/when we support reverse iteration of GenericLists, we can reverse directly here.
    auto obj_ptrs = objs | ranges::to<std::vector<OBJ_DATA *>>;
    for (auto *obj : obj_ptrs | ranges::views::reverse)
        fwrite_one_obj(ch, obj, fp, iNest);
}

/*
 * Write a single object and its contents.
 */
void fwrite_one_obj(const Char *ch, const OBJ_DATA *obj, FILE *fp, int iNest) {
    /*
     * Scupper storage characters.
     */
    if ((ch->level < obj->level - 2 && obj->item_type != ITEM_CONTAINER) || obj->item_type == ITEM_KEY
        || (obj->item_type == ITEM_MAP && !obj->value[0]))
        return;

    fprintf(fp, "#O\n");
    fprintf(fp, "Vnum %d\n", obj->pIndexData->vnum);
    if (obj->enchanted)
        fprintf(fp, "Enchanted\n");
    fprintf(fp, "Nest %d\n", iNest);

    /* these data are only used if they do not match the defaults */

    if (obj->name != obj->pIndexData->name)
        fmt::print(fp, "Name {}~\n", obj->name);
    if (obj->short_descr != obj->pIndexData->short_descr)
        fmt::print(fp, "ShD  {}~\n", obj->short_descr);
    if (obj->description != obj->pIndexData->description)
        fmt::print(fp, "Desc {}~\n", obj->description);
    if (obj->extra_flags != obj->pIndexData->extra_flags)
        fprintf(fp, "ExtF %d\n", obj->extra_flags);
    if (obj->wear_flags != obj->pIndexData->wear_flags)
        fprintf(fp, "WeaF %d\n", obj->wear_flags);
    if (obj->wear_string != obj->pIndexData->wear_string)
        fmt::print(fp, "WStr {}~\n", obj->wear_string);
    if (obj->item_type != obj->pIndexData->item_type)
        fprintf(fp, "Ityp %d\n", obj->item_type);
    if (obj->weight != obj->pIndexData->weight)
        fprintf(fp, "Wt   %d\n", obj->weight);

    /* variable data */

    fprintf(fp, "Wear %d\n", obj->wear_loc);
    if (obj->level != 0)
        fprintf(fp, "Lev  %d\n", obj->level);
    if (obj->timer != 0)
        fprintf(fp, "Time %d\n", obj->timer);
    fprintf(fp, "Cost %d\n", obj->cost);
    if (obj->value[0] != obj->pIndexData->value[0] || obj->value[1] != obj->pIndexData->value[1]
        || obj->value[2] != obj->pIndexData->value[2] || obj->value[3] != obj->pIndexData->value[3]
        || obj->value[4] != obj->pIndexData->value[4])
        fprintf(fp, "Val  %d %d %d %d %d\n", obj->value[0], obj->value[1], obj->value[2], obj->value[3], obj->value[4]);

    switch (obj->item_type) {
    case ITEM_POTION:
    case ITEM_SCROLL:
        if (obj->value[1] > 0) {
            fprintf(fp, "Spell 1 '%s'\n", skill_table[obj->value[1]].name);
        }

        if (obj->value[2] > 0) {
            fprintf(fp, "Spell 2 '%s'\n", skill_table[obj->value[2]].name);
        }

        if (obj->value[3] > 0) {
            fprintf(fp, "Spell 3 '%s'\n", skill_table[obj->value[3]].name);
        }

        break;

    case ITEM_PILL:
    case ITEM_STAFF:
    case ITEM_WAND:
        if (obj->value[3] > 0) {
            fprintf(fp, "Spell 3 '%s'\n", skill_table[obj->value[3]].name);
        }

        break;
    }

    for (auto &af : obj->affected) {
        if (af.type < 0 || af.type >= MAX_SKILL)
            continue;
        fprintf(fp, "AffD '%s' %d %d %d %d %d\n", skill_table[af.type].name, af.level, af.duration, af.modifier,
                static_cast<int>(af.location), af.bitvector);
    }

    for (const auto &ed : obj->extra_descr)
        fmt::print(fp, "ExDe {}~ {}~\n", ed.keyword, ed.description);

    fprintf(fp, "End\n\n");

    fwrite_objs(ch, obj->contains, fp, iNest + 1);
}

void load_into_char(Char &character, FILE *fp) {
    int iNest;

    for (iNest = 0; iNest < MAX_NEST; iNest++)
        rgObjNest[iNest] = nullptr;

    for (;;) {
        auto letter = fread_letter(fp);
        if (letter == '*') {
            fread_to_eol(fp);
            continue;
        }

        if (letter != '#') {
            bug("Load_char_obj: # not found.");
            break;
        }

        auto *word = fread_word(fp);
        if (!str_cmp(word, "PLAYER")) {
            fread_char(&character, fp);
            affect_strip(&character, gsn_ride);
        } else if (!str_cmp(word, "OBJECT") || !str_cmp(word, "O"))
            fread_obj(&character, fp);
        else if (!str_cmp(word, "PET"))
            fread_pet(&character, fp);
        else if (!str_cmp(word, "END"))
            break;
        else {
            bug("Load_char_obj: bad section.");
            break;
        }
    }
}

// Load a char and inventory into a new ch structure.
LoadCharObjResult try_load_player(std::string_view player_name) {
    LoadCharObjResult res{true, std::make_unique<Char>()};

    auto *ch = res.character.get();
    ch->pcdata = std::make_unique<PcData>();

    ch->name = player_name;
    ch->race = race_lookup("human");
    ch->act = PLR_AUTOPEEK | PLR_AUTOASSIST | PLR_AUTOEXIT | PLR_AUTOGOLD | PLR_AUTOLOOT | PLR_AUTOSAC | PLR_NOSUMMON;
    ch->comm = COMM_COMBINE | COMM_PROMPT | COMM_SHOWAFK | COMM_SHOWDEFENCE;
    ranges::fill(ch->perm_stat, 13);

    auto *fp = fopen(filename_for_player(player_name).c_str(), "r");
    if (fp) {
        res.newly_created = false;
        load_into_char(*ch, fp);
        fclose(fp);
    }

    /* initialize race */
    if (!res.newly_created) {
        if (ch->race == 0)
            ch->race = race_lookup("human");

        ch->size = pc_race_table[ch->race].size;
        ch->dam_type = attack_lookup("punch");

        for (auto *group : pc_race_table[ch->race].skills) {
            if (!group)
                break;
            group_add(ch, group, false);
        }
        ch->affected_by = ch->affected_by | race_table[ch->race].aff;
        ch->imm_flags = ch->imm_flags | race_table[ch->race].imm;
        ch->res_flags = ch->res_flags | race_table[ch->race].res;
        ch->vuln_flags = ch->vuln_flags | race_table[ch->race].vuln;
        ch->form = race_table[ch->race].form;
        ch->parts = race_table[ch->race].parts;
    }

    if (!res.newly_created && ch->version < 4) {
        // #216  In PFile Version 4, the strength of all armour is increased by 100 to go along
        // with the corresponding resetting of Char's default armour value of 0 from 100.
        // This is part of a general reorganization of hit calculations.
        for (int i = 0; i < 4; i++) {
            ch->armor[i] -= 101;
        }
    }
    return res;
}

/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY(literal, field, value)                                                                                     \
    if (!str_cmp(word, literal)) {                                                                                     \
        field = value;                                                                                                 \
        fMatch = true;                                                                                                 \
        break;                                                                                                         \
    }

void fread_char(Char *ch, FILE *fp) {
    for (;;) {
        const std::string word = lower_case(feof(fp) ? "end" : fread_word(fp));
        if (word.empty() || word[0] == '*') {
            fread_to_eol(fp);
        } else if (word == "act") {
            ch->act = fread_number(fp);
        } else if (word == "affectedby" || word == "afby") {
            ch->affected_by = fread_number(fp);
        } else if (word == "afk") {
            ch->pcdata->afk = fread_stdstring(fp);
        } else if (matches_start(word, "alig")) {
            ch->alignment = fread_number(fp);
        } else if (word == "armor" || word == "ac") {
            fread_to_eol(fp);
        } else if (word == "acs") {
            for (int i = 0; i < 4; i++) {
                ch->armor[i] = fread_number(fp);
            }
        } else if (word == "affect" || word == "aff" || word == "affd") {
            AFFECT_DATA af;

            // Ick.
            if (word == "affd") {
                int sn;
                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_char: unknown skill.");
                else
                    af.type = sn;
            } else /* old form */
                af.type = fread_number(fp);
            if (ch->version == 0)
                af.level = ch->level;
            else
                af.level = fread_number(fp);
            af.duration = fread_number(fp);
            af.modifier = fread_number(fp);
            af.location = static_cast<AffectLocation>(fread_number(fp));
            af.bitvector = fread_number(fp);
            ch->affected.add_at_end(af);
        } else if (word == "attrmod" || word == "amod") {
            for (auto &stat : ch->mod_stat)
                stat = fread_number(fp);
        } else if (word == "attrperm" || word == "attr") {
            for (auto &stat : ch->perm_stat)
                stat = fread_number(fp);
        } else if (word == "bamfin" || word == "bin") {
            ch->pcdata->bamfin = fread_stdstring(fp);
        } else if (word == "bamfout" || word == "bout") {
            ch->pcdata->bamfout = fread_stdstring(fp);
        } else if (word == "clan") {
            const auto clan_char = fread_number(fp);
            bool found = false;
            for (auto &clan : clantable) {
                if (clan.clanchar == clan_char) {
                    ch->pcdata->pcclan.emplace(PcClan{clan});
                    found = true;
                }
            }
            if (!found) {
                bug("Unable to find clan '{}'", clan_char);
            }
        } else if (word == "class" || word == "cla") {
            ch->class_num = fread_number(fp);
        } else if (word == "clevel") {
            if (ch->pc_clan()) {
                ch->pc_clan()->clanlevel = fread_number(fp);
            } else {
                bug("fread_char: CLAN level with no clan");
                fread_to_eol(fp);
            }
        } else if (word == "ccflags") {
            if (ch->pc_clan()) {
                ch->pc_clan()->channelflags = fread_number(fp);
            } else {
                bug("fread_char: CLAN channelflags with no clan");
                fread_to_eol(fp);
            }
        } else if (word == "condition" || word == "cond") {
            // TODO: look at this - is there some constant for the number 3?
            ch->pcdata->condition[0] = fread_number(fp);
            ch->pcdata->condition[1] = fread_number(fp);
            ch->pcdata->condition[2] = fread_number(fp);
        } else if (word == "colo") {
            ch->pcdata->colour = fread_number(fp);
        } else if (word == "comm") {
            ch->comm = fread_number(fp);
        } else if (word == "damroll" || word == "dam") {
            ch->damroll = fread_number(fp);
        } else if (word == "description" || word == "desc") {
            ch->description = fread_stdstring(fp);
        } else if (word == "end") {
            return;
        } else if (word == "exp") {
            ch->exp = fread_number(fp);
        } else if (word == "extrabits") {
            set_bits_from_pfile(ch, fp);
            fread_to_eol(fp);
        } else if (word == "gold") {
            ch->gold = fread_number(fp);
        } else if (word == "group" || word == "gr") {
            char *temp = fread_word(fp);
            int gn = group_lookup(temp);
            if (gn < 0) {
                fprintf(stderr, "%s", temp);
                bug("Fread_char: unknown group.");
            } else {
                gn_add(ch, gn);
            }
        } else if (word == "hitroll" || word == "hit") {
            ch->hitroll = fread_number(fp);
        } else if (word == "houroffset") {
            ch->pcdata->houroffset = 0;
            // Timezone hours and minutes offset are unused currently but we've kept
            // them in the pfiles for compatibility/potential reuse in future.
            fread_number(fp);
        } else if (word == "hpmanamove" || word == "hmv") {
            ch->hit = fread_number(fp);
            ch->max_hit = fread_number(fp);
            ch->mana = fread_number(fp);
            ch->max_mana = fread_number(fp);
            ch->move = fread_number(fp);
            ch->max_move = fread_number(fp);
        } else if (word == "hpmanamoveperm" || word == "hmvp") {
            ch->pcdata->perm_hit = fread_number(fp);
            ch->pcdata->perm_mana = fread_number(fp);
            ch->pcdata->perm_move = fread_number(fp);
        } else if (word == "invislevel" || word == "invi") {
            ch->invis_level = fread_number(fp);
        } else if (word == "info_message") {
            ch->pcdata->info_message = fread_stdstring(fp);
        } else if (word == "lastlevel" || word == "llev") {
            ch->pcdata->last_level = fread_number(fp);
        } else if (word == "level" || word == "lev" || word == "levl") {
            ch->level = fread_number(fp);
        } else if (word == "longdescr" || word == "lnd") {
            ch->long_descr = fread_stdstring(fp);
        } else if (word == "lastloginfrom") {
            login_from = fread_string(fp);
        } else if (word == "lastloginat") {
            login_at = fread_string(fp);
        } else if (word == "minoffset") {
            ch->pcdata->minoffset = 0;
            // Timezone hours and minutes offset are unused currently but we've kept
            // them in the pfiles for compatibility/potential reuse in future.
            fread_number(fp);
        } else if (word == "name") {
            ch->name = fread_stdstring(fp);
        } else if (word == "note") {
            ch->last_note = Clock::from_time_t(fread_number(fp));
        } else if (word == "password" || word == "pass") {
            ch->pcdata->pwd = fread_stdstring(fp);
        } else if (word == "played" || word == "plyd") {
            ch->played = Seconds(fread_number(fp));
        } else if (word == "points" || word == "pnts") {
            ch->pcdata->points = fread_number(fp);
        } else if (word == "position" || word == "pos") {
            ch->position = fread_number(fp);
        } else if (word == "practice" || word == "prac") {
            ch->practice = fread_number(fp);
        } else if (word == "prompt" || word == "prmt") {
            ch->pcdata->prompt = fread_stdstring(fp);
        } else if (word == "prefix") {
            ch->pcdata->prefix = fread_stdstring(fp);
        } else if (word == "race") {
            ch->race = race_lookup(fread_string(fp));
        } else if (word == "room") {
            ch->in_room = get_room_index(fread_number(fp));
            if (ch->in_room == nullptr)
                ch->in_room = get_room_index(ROOM_VNUM_LIMBO);
        } else if (word == "savingthrow" || word == "save") {
            ch->saving_throw = fread_number(fp);
        } else if (word == "scro") {
            ch->lines = fread_number(fp);
            // kludge to avoid memory bug
            if (ch->lines == 0 || ch->lines > 52)
                ch->lines = 52;
        } else if (word == "sex") {
            ch->sex = fread_number(fp);
        } else if (word == "shortdescr" || word == "shd") {
            ch->short_descr = fread_stdstring(fp);
        } else if (word == "skill" || word == "sk") {
            const int value = fread_number(fp);
            const char *temp = fread_word(fp);
            const int sn = skill_lookup(temp);
            if (sn < 0) {
                fprintf(stderr, "%s", temp);
                bug("Fread_char: unknown skill.");
            } else
                ch->pcdata->learned[sn] = value;
        } else if (word == "truesex" || word == "tsex") {
            ch->pcdata->true_sex = fread_number(fp);
        } else if (word == "trai") {
            ch->train = fread_number(fp);
        } else if (word == "trust" || word == "tru") {
            ch->trust = fread_number(fp);
        } else if (word == "title" || word == "titl") {
            ch->set_title(fread_stdstring(fp));
        } else if (word == "version" || word == "vers") {
            ch->version = fread_number(fp);
        } else if (word == "vnum") {
            ch->pIndexData = get_mob_index(fread_number(fp));
        } else if (word == "wimpy" || word == "wimp") {
            ch->wimpy = fread_number(fp);
        } else if (word == "possessive") {
            ch->pcdata->pronouns.possessive = fread_stdstring(fp);
        } else if (word == "subjective") {
            ch->pcdata->pronouns.subjective = fread_stdstring(fp);
        } else if (word == "objective") {
            ch->pcdata->pronouns.objective = fread_stdstring(fp);
        } else if (word == "reflexive") {
            ch->pcdata->pronouns.reflexive = fread_stdstring(fp);
        }
    }
}

/* load a pet from the forgotten reaches */
void fread_pet(Char *ch, FILE *fp) {
    std::string word;
    Char *pet;
    /* first entry had BETTER be the vnum or we barf */
    word = feof(fp) ? "END" : fread_word(fp);
    if (matches(word, "Vnum")) {
        int vnum = fread_number(fp);
        if (get_mob_index(vnum) == nullptr) {
            bug("Fread_pet: bad vnum {}.", vnum);
            pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
        } else
            pet = create_mobile(get_mob_index(vnum));
    } else {
        bug("Fread_pet: no vnum in file.");
        pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
    }

    for (;;) {
        word = feof(fp) ? "END" : fread_word(fp);
        if (word.empty() || word[0] == '*') {
            fread_to_eol(fp);
        } else if (word[0] == '#') {
            fread_obj(pet, fp);
        } else if (matches(word, "Act")) {
            pet->act = fread_number(fp);
        } else if (matches(word, "AfBy")) {
            pet->affected_by = fread_number(fp);
        } else if (matches(word, "Alig")) {
            pet->alignment = fread_number(fp);
        } else if (matches(word, "ACs")) {
            for (int i = 0; i < 4; i++)
                pet->armor[i] = fread_number(fp);
        } else if (matches(word, "AffD")) {
            AFFECT_DATA af;
            int sn = skill_lookup(fread_word(fp));
            if (sn < 0)
                bug("Fread_pet: unknown skill #{}", sn);
            else
                af.type = sn;

            af.level = fread_number(fp);
            af.duration = fread_number(fp);
            af.modifier = fread_number(fp);
            af.location = static_cast<AffectLocation>(fread_number(fp));
            af.bitvector = fread_number(fp);
            pet->affected.add_at_end(af);
        } else if (matches(word, "AMod")) {
            for (auto &stat : pet->mod_stat)
                stat = fread_number(fp);
        } else if (matches(word, "Attr")) {
            for (auto &stat : pet->perm_stat)
                stat = fread_number(fp);
        } else if (matches(word, "Comm")) {
            pet->comm = fread_number(fp);
        } else if (matches(word, "Dam")) {
            pet->damroll = fread_number(fp);
        } else if (matches(word, "Desc")) {
            pet->description = fread_stdstring(fp);
        } else if (matches(word, "End")) {
            pet->leader = ch;
            pet->master = ch;
            ch->pet = pet;
            return;
        } else if (matches(word, "Exp")) {
            pet->exp = fread_number(fp);
        } else if (matches(word, "Gold")) {
            pet->gold = fread_number(fp);
        } else if (matches(word, "Hit")) {
            pet->hitroll = fread_number(fp);
        } else if (matches(word, "HMV")) {
            pet->hit = fread_number(fp);
            pet->max_hit = fread_number(fp);
            pet->mana = fread_number(fp);
            pet->max_mana = fread_number(fp);
            pet->move = fread_number(fp);
            pet->max_move = fread_number(fp);
        } else if (matches(word, "Levl")) {
            pet->level = fread_number(fp);
        } else if (matches(word, "LnD")) {
            pet->long_descr = fread_stdstring(fp);
        } else if (matches(word, "Name")) {
            pet->name = fread_stdstring(fp);
        } else if (matches(word, "Pos")) {
            pet->position = fread_number(fp);
        } else if (matches(word, "Race")) {
            pet->race = race_lookup(fread_string(fp));
        } else if (matches(word, "Save")) {
            pet->saving_throw = fread_number(fp);
        } else if (matches(word, "Sex")) {
            pet->sex = fread_number(fp);
        } else if (matches(word, "ShD")) {
            pet->short_descr = fread_stdstring(fp);
        } else {
            bug("Fread_pet: no match for {}.", word);
            fread_to_eol(fp);
        }
    }
}

void fread_obj(Char *ch, FILE *fp) {
    OBJ_DATA *obj;
    std::string word;
    int iNest;
    bool fNest;
    bool fVnum;
    bool first;
    bool new_format; /* to prevent errors */
    bool make_new; /* update object */

    fVnum = false;
    obj = nullptr;
    first = true; /* used to counter fp offset */
    new_format = false;
    make_new = false;

    word = feof(fp) ? "End" : fread_word(fp);
    if (matches(word, "Vnum")) {
        int vnum;
        first = false; /* fp will be in right place */

        vnum = fread_number(fp);
        if (get_obj_index(vnum) == nullptr) {
            bug("Fread_obj: bad vnum {}.", vnum);
        } else {
            obj = create_object(get_obj_index(vnum));
            new_format = true;
        }
    }

    if (obj == nullptr) /* either not found or old style */
        obj = new OBJ_DATA;

    fNest = false;
    fVnum = true;
    iNest = 0;

    for (;;) {
        if (first)
            first = false;
        else
            word = feof(fp) ? "End" : fread_word(fp);
        if (word.empty() || word[0] == '*') {
            fread_to_eol(fp);
        } else if (matches(word, "Affect") || matches(word, "Aff") || matches(word, "AffD")) {
            AFFECT_DATA af;

            // Spell effects on chars and customized objects in PFiles
            if (matches(word, "AffD")) {
                int sn;
                const char *affected_by = fread_word(fp);
                sn = skill_lookup(affected_by);
                if (sn < 0)
                    bug("Fread_obj: unknown skill {}.", affected_by);
                else
                    af.type = sn;
            } else /* old form */
                af.type = fread_number(fp);
            af.level = fread_number(fp);
            af.duration = fread_number(fp);
            af.modifier = fread_number(fp);
            af.location = static_cast<AffectLocation>(fread_number(fp));
            af.bitvector = fread_number(fp);
            obj->affected.add_at_end(af);
        } else if (matches(word, "Cost")) {
            obj->cost = fread_number(fp);
        } else if (matches(word, "Description") || matches(word, "Desc")) {
            obj->description = fread_string(fp);
        } else if (matches(word, "Enchanted")) {
            obj->enchanted = true;
        } else if (matches(word, "ExtraFlags") || matches(word, "ExtF")) {
            obj->extra_flags = fread_number(fp);
        } else if (matches(word, "ExtraDescr") || matches(word, "ExDe")) {
            auto keyword = fread_stdstring(fp);
            auto description = fread_stdstring(fp);
            obj->extra_descr.emplace_back(EXTRA_DESCR_DATA{keyword, description});
        } else if (matches(word, "End")) {
            if (!fNest || !fVnum || obj->pIndexData == nullptr) {
                bug("Fread_obj: incomplete object.");
                extract_obj(obj);
                return;
            } else {
                if (!new_format) {
                    object_list.add_front(obj);
                    obj->pIndexData->count++;
                }
                if (make_new) {
                    int wear = obj->wear_loc;
                    extract_obj(obj);
                    obj = create_object(obj->pIndexData);
                    obj->wear_loc = wear;
                }
                if (iNest == 0 || rgObjNest[iNest] == nullptr)
                    obj_to_char(obj, ch);
                else
                    obj_to_obj(obj, rgObjNest[iNest - 1]);
                return;
            }
        } else if (matches(word, "ItemType") || matches(word, "Ityp")) {
            obj->item_type = fread_number(fp);
        } else if (matches(word, "Level") || matches(word, "Lev")) {
            obj->level = fread_number(fp);
        } else if (matches(word, "Name")) {
            obj->name = fread_stdstring(fp);
        } else if (matches(word, "Nest")) {
            iNest = fread_number(fp);
            if (iNest < 0 || iNest >= MAX_NEST) {
                bug("Fread_obj: bad nest {}.", iNest);
            } else {
                rgObjNest[iNest] = obj;
                fNest = true;
            }
        } else if (matches(word, "Oldstyle")) {
            // TODO(Forrey): I don't believe 'Oldstyle' can be used any more.
            // Figure out whether this can ever happen, and if not, remove this
            // code and the 'make_new' handling.
            if (obj->pIndexData != nullptr)
                make_new = true;
        } else if (matches(word, "ShortDescr") || matches(word, "ShD")) {
            obj->short_descr = fread_string(fp);
        } else if (matches(word, "Spell")) {
            int iValue = fread_number(fp);
            const char *spell = fread_word(fp);
            int sn = skill_lookup(spell);
            if (iValue < 0 || iValue > 3) {
                bug("Fread_obj: bad iValue {}.", iValue);
            } else if (sn < 0) {
                bug("Fread_obj: unknown skill {}.", spell);
            } else {
                obj->value[iValue] = sn;
            }
        } else if (matches_start(word, "Timer")) {
            obj->timer = fread_number(fp);
        } else if (matches(word, "Values") || matches(word, "Vals")) {
            obj->value[0] = fread_number(fp);
            obj->value[1] = fread_number(fp);
            obj->value[2] = fread_number(fp);
            obj->value[3] = fread_number(fp);
            if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
                obj->value[0] = obj->pIndexData->value[0];
        } else if (matches(word, "Val")) {
            obj->value[0] = fread_number(fp);
            obj->value[1] = fread_number(fp);
            obj->value[2] = fread_number(fp);
            obj->value[3] = fread_number(fp);
            obj->value[4] = fread_number(fp);
        } else if (matches(word, "Vnum")) {
            int vnum = fread_number(fp);
            if ((obj->pIndexData = get_obj_index(vnum)) == nullptr)
                bug("Fread_obj: bad vnum {}.", vnum);
            else
                fVnum = true;
        } else if (matches(word, "WearFlags") || matches(word, "WeaF")) {
            obj->wear_flags = fread_number(fp);
        } else if (matches(word, "WearLoc") || matches(word, "Wear")) {
            obj->wear_loc = fread_number(fp);
        } else if (matches(word, "Weight") || matches(word, "Wt")) {
            obj->weight = fread_number(fp);
        } else if (matches(word, "WStr")) {
            obj->wear_string = fread_stdstring(fp);
        } else {
            bug("Fread_obj: no match for {}.", word);
            fread_to_eol(fp);
        }
    }
}
