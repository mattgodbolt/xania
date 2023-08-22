/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2023 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  save.cpp:  data saving routines                                      */
/*                                                                       */
/*************************************************************************/

#include "save.hpp"
#include "AFFECT_DATA.hpp"
#include "Attacks.hpp"
#include "CharFileMeta.hpp"
#include "CommFlag.hpp"
#include "Logging.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "PlayerActFlag.hpp"
#include "Races.hpp"
#include "Room.hpp"
#include "SkillNumbers.hpp"
#include "SkillTables.hpp"
#include "TimeInfoData.hpp"
#include "VnumMobiles.hpp"
#include "VnumRooms.hpp"
#include "Worn.hpp"
#include "common/Configuration.hpp"
#include "db.h"
#include "handler.hpp"
#include "lookup.h"
#include "skills.hpp"
#include "string_utils.hpp"

#include <fmt/format.h>
#include <magic_enum.hpp>
#include <range/v3/algorithm/fill.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/reverse.hpp>
#include <unordered_map>

namespace { // Hide the local routines.

constexpr auto MaxObjectNest = 100u;
using ObjectNestMap = std::unordered_map<ush_int, Object *>;

void set_bits_from_pfile(Char *ch, FILE *fp) {
    for (auto extra_flag : magic_enum::enum_values<CharExtraFlag>()) {
        char c = fread_letter(fp);
        if (c == '0')
            continue;
        else if (c == '1') {
            ch->set_extra(extra_flag);
        } else {
            break;
        }
    }
}

/*
 * Write the char.
 */
void fwrite_char(const Char *ch, FILE *fp) {
    namespace cf = charfilemeta;
    fmt::print(fp, "#{}\n", ch->is_npc() ? cf::SectionMobile : cf::SectionPlayer);
    fmt::print(fp, "{} {}~\n", cf::Name, ch->name);
    // Whenever a player file is written it's automatically using the latest version.
    fmt::print(fp, "{} {}\n", cf::Version, magic_enum::enum_integer<CharVersion>(CharVersion::Latest));
    if (!ch->short_descr.empty())
        fmt::print(fp, "{} {}~\n", cf::ShortDescription, ch->short_descr);
    if (!ch->long_descr.empty())
        fmt::print(fp, "{} {}~\n", cf::LongDescription, ch->long_descr);
    if (!ch->description.empty())
        fmt::print(fp, "{} {}~\n", cf::Description, ch->description);
    fmt::print(fp, "{} {}~\n", cf::Race, pc_race_table[ch->race].name);
    fmt::print(fp, "{}  {}\n", cf::Sex, ch->sex.integer());
    fmt::print(fp, "{}  {}\n", cf::Class, ch->class_num);
    fmt::print(fp, "{} {}\n", cf::Level, ch->level);
    if (auto *pc_clan = ch->pc_clan()) {
        fmt::print(fp, "{} {}\n", cf::Clan, (int)pc_clan->clan.clanchar);
        fmt::print(fp, "{} {}\n", cf::ClanLevel, pc_clan->clanlevel);
        fmt::print(fp, "{} {}\n", cf::ClanChanFlags, pc_clan->channelflags);
    }
    if (ch->trust != 0)
        fmt::print(fp, "{} {}\n", cf::Trust, ch->trust);
    using namespace std::chrono;
    fmt::print(fp, "{} {}\n", cf::PlayedTime, (int)duration_cast<seconds>(ch->total_played()).count());
    fmt::print(fp, "{} {}\n", cf::LastNote, (int)Clock::to_time_t(ch->last_note));
    fmt::print(fp, "{} {}\n", cf::Scroll, ch->lines);
    fmt::print(fp, "{} {}\n", cf::Room,
               (ch->in_room == get_room(Rooms::Limbo) && ch->was_in_room != nullptr) ? ch->was_in_room->vnum
               : ch->in_room == nullptr                                              ? Rooms::MidgaardTemple
                                                                                     : ch->in_room->vnum);

    fmt::print(fp, "{}  {} {} {} {} {} {}\n", cf::HitManaMove, ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move,
               ch->max_move);
    fmt::print(fp, "{} {}\n", cf::Gold, ch->gold > 0 ? ch->gold : 0);
    fmt::print(fp, "{}  {}\n", cf::Experience, ch->exp);
    if (ch->act != 0)
        fmt::print(fp, "{}  {}\n", cf::ActFlags, ch->act);
    if (ch->affected_by != 0)
        fmt::print(fp, "{} {}\n", cf::AffectedBy, ch->affected_by);
    fmt::print(fp, "{} {}\n", cf::CommFlags, ch->comm);
    if (ch->invis_level != 0)
        fmt::print(fp, "{} {}\n", cf::InvisLevel, ch->invis_level);
    fmt::print(
        fp, "{}  {}\n", cf::Position,
        magic_enum::enum_integer<Position::Type>(ch->is_pos_fighting() ? Position::Type::Standing : ch->position));
    if (ch->practice != 0)
        fmt::print(fp, "{} {}\n", cf::Practice, ch->practice);
    if (ch->train != 0)
        fmt::print(fp, "{} {}\n", cf::Train, ch->train);
    if (ch->saving_throw != 0)
        fmt::print(fp, "{} {}\n", cf::SavingThrow, ch->saving_throw);
    fmt::print(fp, "{}  {}\n", cf::Alignment, ch->alignment);
    if (ch->hitroll != 0)
        fmt::print(fp, "{}   {}\n", cf::HitRoll, ch->hitroll);
    if (ch->damroll != 0)
        fmt::print(fp, "{}   {}\n", cf::DamRoll, ch->damroll);
    fmt::print(fp, "{} {} {} {} {}\n", cf::ArmourClasses, ch->armor[0], ch->armor[1], ch->armor[2], ch->armor[3]);
    if (ch->wimpy != 0)
        fmt::print(fp, "{}  {}\n", cf::Wimpy, ch->wimpy);
    fmt::print(fp, "{} {} {} {} {} {}\n", cf::Attribs, ch->perm_stat[Stat::Str], ch->perm_stat[Stat::Int],
               ch->perm_stat[Stat::Wis], ch->perm_stat[Stat::Dex], ch->perm_stat[Stat::Con]);

    fmt::print(fp, "{} {} {} {} {} {}\n", cf::AttribModifiers, ch->mod_stat[Stat::Str], ch->mod_stat[Stat::Int],
               ch->mod_stat[Stat::Wis], ch->mod_stat[Stat::Dex], ch->mod_stat[Stat::Con]);

    if (ch->is_npc()) {
        fmt::print(fp, "{} {}\n", cf::Vnum, ch->mobIndex->vnum);
    } else {
        fmt::print(fp, "{} {}~\n", cf::Password, ch->pcdata->pwd);
        if (!ch->pcdata->bamfin.empty())
            fmt::print(fp, "{}  {}~\n", cf::BamfIn, ch->pcdata->bamfin);
        if (!ch->pcdata->bamfout.empty())
            fmt::print(fp, "{} {}~\n", cf::BamfOut, ch->pcdata->bamfout);
        fmt::print(fp, "{} {}~\n", cf::Title, ch->pcdata->title);
        fmt::print(fp, "{} {}~\n", cf::Afk, ch->pcdata->afk);
        fmt::print(fp, "{} {}\n", cf::Colour, (int)ch->pcdata->colour);
        fmt::print(fp, "{} {}~\n", cf::Prompt, ch->pcdata->prompt);
        fmt::print(fp, "{} {}\n", cf::CreationPoints, ch->pcdata->points);
        fmt::print(fp, "{} {}\n", cf::TrueSex, ch->pcdata->true_sex.integer());
        fmt::print(fp, "{} {}\n", cf::LastLevelTime, ch->pcdata->last_level);
        fmt::print(fp, "{} {} {} {}\n", cf::HitManaMovePerm, ch->pcdata->perm_hit, ch->pcdata->perm_mana,
                   ch->pcdata->perm_move);
        fmt::print(fp, "{} {}~\n", cf::InfoMsg, ch->pcdata->info_message);
        if (ch->desc) {
            fmt::print(fp, "{} {}~\n", cf::LastLoginFrom, ch->desc->host());
            fmt::print(fp, "{} {}~\n", cf::LastLoginAt, ch->desc->login_time());
        }

        /* save prefix PCFN 19-05-97 */
        fmt::print(fp, "{} {}~\n", cf::Prefix, ch->pcdata->prefix);

        // Timezone hours and minutes offset are unused currently.
        fmt::print(fp, "{} {}\n", cf::HourOffset, ch->pcdata->houroffset);
        fmt::print(fp, "{} {}\n", cf::MinOffset, ch->pcdata->minoffset);
        fmt::print(fp, "{} {}~\n", cf::ExtraBits, ch->serialize_extra_flags());
        fmt::print(fp, "{} {} {} {}\n", cf::Condition, ch->pcdata->inebriation.get(), ch->pcdata->hunger.get(),
                   ch->pcdata->thirst.get());
        fmt::print(fp, "{} {}~\n", cf::PronounPossessive, ch->pcdata->pronouns.possessive);
        fmt::print(fp, "{} {}~\n", cf::PronounSubjective, ch->pcdata->pronouns.subjective);
        fmt::print(fp, "{} {}~\n", cf::PronounObjective, ch->pcdata->pronouns.objective);
        fmt::print(fp, "{} {}~\n", cf::PronounReflexive, ch->pcdata->pronouns.reflexive);

        for (auto sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].name != nullptr && ch->pcdata->learned[sn] > 0) {
                fmt::print(fp, "{} {} '{}'\n", cf::Skill, ch->pcdata->learned[sn],
                           skill_table[sn].name); // NOT ch.get_skill()
            }
        }

        for (auto gn = 0; gn < MAX_GROUP; gn++) {
            if (group_table[gn].name != nullptr && ch->pcdata->group_known[gn]) {
                fmt::print(fp, "{} '{}'\n", cf::SkillGroup, group_table[gn].name);
            }
        }
    }

    std::stringstream ss;
    for (const auto &af : ch->affected) {
        if (af.type < 0 || af.type >= MAX_SKILL)
            continue;
        ss = std::stringstream();
        ss << "'" << skill_table[af.type].name << "'";
        fmt::print(fp, "{} {:<17} {:>3} {:>3} {:>3} {:>3} {:>10}\n", cf::Affected, ss.str(), af.level, af.duration,
                   af.modifier, static_cast<int>(af.location), af.bitvector);
    }
    fmt::print(fp, "{}\n\n", cf::End);
}

void fwrite_objs(const Char *ch, const GenericList<Object *> &objs, FILE *fp, ush_int nest_level);

/*
 * Write a single object and its contents.
 */
void fwrite_one_obj(const Char *ch, const Object *obj, FILE *fp, ush_int nest_level) {
    /*
     * Scupper storage characters.
     */
    if ((ch->level < obj->level - 2 && obj->type != ObjectType::Container) || obj->type == ObjectType::Key
        || (obj->type == ObjectType::Map && !obj->value[0]))
        return;

    namespace cf = charfilemeta;
    fmt::print(fp, "#{}\n", cf::SectionObject);
    fmt::print(fp, "{} {}\n", cf::Vnum, obj->objIndex->vnum);
    if (obj->enchanted)
        fmt::print(fp, "{}\n", cf::Enchanted);
    fmt::print(fp, "{} {}\n", cf::Nest, nest_level);

    /* these data are only used if they do not match the defaults */

    if (obj->name != obj->objIndex->name)
        fmt::print(fp, "{} {}~\n", cf::Name, obj->name);
    if (obj->short_descr != obj->objIndex->short_descr)
        fmt::print(fp, "{}  {}~\n", cf::ShortDescription, obj->short_descr);
    if (obj->description != obj->objIndex->description)
        fmt::print(fp, "{} {}~\n", cf::Description, obj->description);
    if (obj->extra_flags != obj->objIndex->extra_flags)
        fmt::print(fp, "{} {}\n", cf::ExtraFlags, obj->extra_flags);
    if (obj->wear_flags != obj->objIndex->wear_flags)
        fmt::print(fp, "{} {}\n", cf::WearFlags, obj->wear_flags);
    if (obj->wear_string != obj->objIndex->wear_string)
        fmt::print(fp, "{} {}~\n", cf::WearString, obj->wear_string);
    if (obj->type != obj->objIndex->type)
        fmt::print(fp, "{} {}\n", cf::ItemType, magic_enum::enum_integer<ObjectType>(obj->type));
    if (obj->weight != obj->objIndex->weight)
        fmt::print(fp, "{}   {}\n", cf::Weight, obj->weight);

    /* variable data */

    fmt::print(fp, "{} {}\n", cf::WearLoc, magic_enum::enum_integer<Worn>(obj->worn_loc));
    if (obj->level != 0)
        fmt::print(fp, "{}  {}\n", cf::ObjectLevel, obj->level);
    if (obj->timer != 0)
        fmt::print(fp, "{} {}\n", cf::Time, obj->timer);
    fmt::print(fp, "{} {}\n", cf::Cost, obj->cost);
    if (obj->value[0] != obj->objIndex->value[0] || obj->value[1] != obj->objIndex->value[1]
        || obj->value[2] != obj->objIndex->value[2] || obj->value[3] != obj->objIndex->value[3]
        || obj->value[4] != obj->objIndex->value[4])
        fmt::print(fp, "{}  {} {} {} {} {}\n", cf::Val, obj->value[0], obj->value[1], obj->value[2], obj->value[3],
                   obj->value[4]);

    switch (obj->type) {
    case ObjectType::Potion:
    case ObjectType::Scroll:
        for (auto i = 1; i < 4; i++) { // potion & scroll spells occupy object value slots 1-3
            if (obj->value[i] > 0) {
                fmt::print(fp, "{} {} '{}'\n", cf::Spell, i, skill_table[obj->value[i]].name);
            }
        }
        break;

    case ObjectType::Pill:
    case ObjectType::Staff:
    case ObjectType::Wand:
        if (obj->value[3] > 0) {
            fmt::print(fp, "{} 3 '{}'\n", cf::Spell, skill_table[obj->value[3]].name);
        }

        break;
    default:;
    }

    for (auto &af : obj->affected) {
        if (af.type < 0 || af.type >= MAX_SKILL)
            continue;
        fmt::print(fp, "{} '{}' {} {} {} {} {}\n", cf::Affected, skill_table[af.type].name, af.level, af.duration,
                   af.modifier, static_cast<int>(af.location), af.bitvector);
    }

    for (const auto &ed : obj->extra_descr)
        fmt::print(fp, "{} {}~ {}~\n", cf::ExtraDescription, ed.keyword, ed.description);

    fmt::print(fp, "{}\n\n", cf::End);

    fwrite_objs(ch, obj->contains, fp, nest_level + 1);
}

void fwrite_objs(const Char *ch, const GenericList<Object *> &objs, FILE *fp, ush_int nest_level) {
    // TODO: if/when we support reverse iteration of GenericLists, we can reverse directly here.
    auto obj_ptrs = objs | ranges::to<std::vector<Object *>>;
    for (auto *obj : obj_ptrs | ranges::views::reverse)
        fwrite_one_obj(ch, obj, fp, nest_level);
}

/* write a pet */
void fwrite_pet(const Char *ch, const Char *pet, FILE *fp) {
    namespace cf = charfilemeta;
    fmt::print(fp, "#{}\n", cf::SectionPet);

    fmt::print(fp, "{} {}\n", cf::Vnum, pet->mobIndex->vnum);

    fmt::print(fp, "{} {}~\n", cf::Name, pet->name);
    if (pet->short_descr != pet->mobIndex->short_descr)
        fmt::print(fp, "{}  {}~\n", cf::ShortDescription, pet->short_descr);
    if (pet->long_descr != pet->mobIndex->long_descr)
        fmt::print(fp, "{}  {}~\n", cf::LongDescription, pet->long_descr);
    if (pet->description != pet->mobIndex->description)
        fmt::print(fp, "{} {}~\n", cf::Description, pet->description);
    if (pet->race != pet->mobIndex->race)
        fmt::print(fp, "{} {}~\n", cf::Race, race_table[pet->race].name);
    fmt::print(fp, "{}  {}\n", cf::Sex, pet->sex.integer());
    if (pet->level != pet->mobIndex->level)
        fmt::print(fp, "{} {}\n", cf::Level, pet->level);
    fmt::print(fp, "{}  {} {} {} {} {} {}\n", cf::HitManaMove, pet->hit, pet->max_hit, pet->mana, pet->max_mana,
               pet->move, pet->max_move);
    if (pet->gold > 0)
        fmt::print(fp, "{} {}\n", cf::Gold, pet->gold);
    if (pet->exp > 0)
        fmt::print(fp, "{}  {}\n", cf::Experience, pet->exp);
    if (pet->act != pet->mobIndex->act)
        fmt::print(fp, "{}  {}\n", cf::ActFlags, pet->act);
    if (pet->affected_by != pet->mobIndex->affected_by)
        fmt::print(fp, "{} {}\n", cf::AffectedBy, pet->affected_by);
    if (pet->comm != 0)
        fmt::print(fp, "{} {}\n", cf::CommFlags, pet->comm);
    fmt::print(
        fp, "{}  {}\n", cf::Position,
        magic_enum::enum_integer<Position::Type>(pet->is_pos_fighting() ? Position::Type::Standing : pet->position));
    if (pet->saving_throw != 0)
        fmt::print(fp, "{} {}\n", cf::SavingThrow, pet->saving_throw);
    if (pet->alignment != pet->mobIndex->alignment)
        fmt::print(fp, "{} {}\n", cf::Alignment, pet->alignment);
    if (pet->hitroll != pet->mobIndex->hitroll)
        fmt::print(fp, "{}  {}\n", cf::HitRoll, pet->hitroll);
    if (pet->damroll != pet->mobIndex->damage.bonus())
        fmt::print(fp, "{}  {}\n", cf::DamRoll, pet->damroll);
    fmt::print(fp, "{}  {} {} {} {}\n", cf::ArmourClasses, pet->armor[0], pet->armor[1], pet->armor[2], pet->armor[3]);
    fmt::print(fp, "{} {} {} {} {} {}\n", cf::Attribs, pet->perm_stat[Stat::Str], pet->perm_stat[Stat::Int],
               pet->perm_stat[Stat::Wis], pet->perm_stat[Stat::Dex], pet->perm_stat[Stat::Con]);
    fmt::print(fp, "{} {} {} {} {} {}\n", cf::AttribModifiers, pet->mod_stat[Stat::Str], pet->mod_stat[Stat::Int],
               pet->mod_stat[Stat::Wis], pet->mod_stat[Stat::Dex], pet->mod_stat[Stat::Con]);

    std::stringstream ss;
    for (const auto &af : pet->affected) {
        if (af.type < 0 || af.type >= MAX_SKILL)
            continue;
        ss = std::stringstream();
        ss << "'" << skill_table[af.type].name << "'";
        fmt::print(fp, "{} {:<17} {:>3} {:>3} {:>3} {:>3} {:>10}\n", cf::Affected, ss.str(), af.level, af.duration,
                   af.modifier, static_cast<int>(af.location), af.bitvector);
    }

    if (ch->riding == pet) {
        fmt::print(fp, "{}\n", cf::Ridden);
    }

    fwrite_objs(pet, pet->carrying, fp, 0);

    fmt::print(fp, "{}\n", cf::End);
}

/*
 * Read in a Char.
 */
void fread_char(Char *ch, LastLoginInfo &last_login, FILE *fp) {
    namespace cf = charfilemeta;
    for (;;) {
        const std::string word = feof(fp) ? cf::End : fread_word(fp);
        if (word.empty() || word[0] == '*') {
            fread_to_eol(fp);
        } else if (word == cf::ActFlags) {
            ch->act = fread_number(fp);
        } else if (word == cf::AffectedBy) {
            ch->affected_by = fread_number(fp);
        } else if (word == cf::Afk) {
            ch->pcdata->afk = fread_string(fp);
        } else if (word == cf::Alignment) {
            ch->alignment = fread_number(fp);
        } else if (word == cf::ArmourClasses) {
            for (size_t i = 0; i < ch->armor.size(); i++) {
                ch->armor[i] = fread_number(fp);
            }
        } else if (word == cf::Affected) {
            AFFECT_DATA af;
            int sn = skill_lookup(fread_word(fp));
            if (sn < 0)
                bug("fread_char: unknown skill.");
            else
                af.type = sn;
            af.level = fread_number(fp);
            af.duration = fread_number(fp);
            af.modifier = fread_number(fp);
            af.location = static_cast<AffectLocation>(fread_number(fp));
            af.bitvector = fread_number(fp);
            ch->affected.add_at_end(af);
        } else if (word == cf::AttribModifiers) {
            for (auto &stat : ch->mod_stat)
                stat = fread_number(fp);
        } else if (word == cf::Attribs) {
            for (auto &stat : ch->perm_stat)
                stat = fread_number(fp);
        } else if (word == cf::BamfIn) {
            ch->pcdata->bamfin = fread_string(fp);
        } else if (word == cf::BamfOut) {
            ch->pcdata->bamfout = fread_string(fp);
        } else if (word == cf::Clan) {
            const auto clan_char = fread_number(fp);
            bool found = false;
            for (auto &clan : clantable) {
                if (clan.clanchar == clan_char) {
                    ch->pcdata->pcclan.emplace(PcClan{clan});
                    found = true;
                }
            }
            if (!found) {
                bug("fread_char: unable to find clan '{}'", clan_char);
            }
        } else if (word == cf::Class) {
            ch->class_num = fread_number(fp);
        } else if (word == cf::ClanLevel) {
            if (ch->pc_clan()) {
                ch->pc_clan()->clanlevel = fread_number(fp);
            } else {
                bug("fread_char: CLAN level with no clan");
                fread_to_eol(fp);
            }
        } else if (word == cf::ClanChanFlags) {
            if (ch->pc_clan()) {
                ch->pc_clan()->channelflags = fread_number(fp);
            } else {
                bug("fread_char: CLAN channelflags with no clan");
                fread_to_eol(fp);
            }
        } else if (word == cf::Condition) {
            ch->pcdata->inebriation.set(fread_number(fp));
            ch->pcdata->hunger.set(fread_number(fp));
            ch->pcdata->thirst.set(fread_number(fp));
        } else if (word == cf::Colour) {
            ch->pcdata->colour = fread_number(fp);
        } else if (word == cf::CommFlags) {
            ch->comm = fread_number(fp);
        } else if (word == cf::DamRoll) {
            ch->damroll = fread_number(fp);
        } else if (word == cf::Description) {
            ch->description = fread_string(fp);
        } else if (word == cf::End) {
            return;
        } else if (word == cf::Experience) {
            ch->exp = fread_number(fp);
        } else if (word == cf::ExtraBits) {
            set_bits_from_pfile(ch, fp);
            fread_to_eol(fp);
        } else if (word == cf::Gold) {
            ch->gold = fread_number(fp);
        } else if (word == cf::SkillGroup) {
            const auto group = fread_word(fp);
            int gn = group_lookup(group);
            if (gn < 0) {
                bug("fread_char: unknown group: {}", group);
            } else {
                gn_add(ch, gn);
            }
        } else if (word == cf::HitRoll) {
            ch->hitroll = fread_number(fp);
        } else if (word == cf::HourOffset) {
            ch->pcdata->houroffset = 0;
            // Timezone hours and minutes offset are unused currently but we've kept
            // them in the pfiles for compatibility/potential reuse in future.
            fread_number(fp);
        } else if (word == cf::HitManaMove) {
            ch->hit = fread_number(fp);
            ch->max_hit = fread_number(fp);
            ch->mana = fread_number(fp);
            ch->max_mana = fread_number(fp);
            ch->move = fread_number(fp);
            ch->max_move = fread_number(fp);
        } else if (word == cf::HitManaMovePerm) {
            ch->pcdata->perm_hit = fread_number(fp);
            ch->pcdata->perm_mana = fread_number(fp);
            ch->pcdata->perm_move = fread_number(fp);
        } else if (word == cf::InvisLevel) {
            ch->invis_level = fread_number(fp);
        } else if (word == cf::InfoMsg) {
            ch->pcdata->info_message = fread_string(fp);
        } else if (word == cf::LastLevelTime) {
            ch->pcdata->last_level = fread_number(fp);
        } else if (word == cf::Level) {
            ch->level = fread_number(fp);
        } else if (word == cf::LongDescription) {
            ch->long_descr = fread_string(fp);
        } else if (word == cf::LastLoginFrom) {
            last_login.login_from = fread_string(fp);
        } else if (word == cf::LastLoginAt) {
            last_login.login_at = fread_string(fp);
        } else if (word == cf::MinOffset) {
            ch->pcdata->minoffset = 0;
            // Timezone hours and minutes offset are unused currently but we've kept
            // them in the pfiles for compatibility/potential reuse in future.
            fread_number(fp);
        } else if (word == cf::Name) {
            ch->name = fread_string(fp);
        } else if (word == cf::LastNote) {
            ch->last_note = Clock::from_time_t(fread_number(fp));
        } else if (word == cf::Password) {
            ch->pcdata->pwd = fread_string(fp);
        } else if (word == cf::PlayedTime) {
            ch->played = Seconds(fread_number(fp));
        } else if (word == cf::CreationPoints) {
            ch->pcdata->points = fread_number(fp);
        } else if (word == cf::Position) {
            ch->position = Position::read_from_number(fp);
        } else if (word == cf::Practice) {
            ch->practice = fread_number(fp);
        } else if (word == cf::Prompt) {
            ch->pcdata->prompt = fread_string(fp);
        } else if (word == cf::Prefix) {
            ch->pcdata->prefix = fread_string(fp);
        } else if (word == cf::Race) {
            ch->race = race_lookup(fread_string(fp));
        } else if (word == cf::Room) {
            ch->in_room = get_room(fread_number(fp));
            if (ch->in_room == nullptr)
                ch->in_room = get_room(Rooms::Limbo);
        } else if (word == cf::SavingThrow) {
            ch->saving_throw = fread_number(fp);
        } else if (word == cf::Scroll) {
            ch->lines = fread_number(fp);
            // kludge to avoid memory bug
            if (ch->lines == 0 || ch->lines > 52)
                ch->lines = 52;
        } else if (word == cf::Sex) {
            if (auto sex = Sex::try_from_integer(fread_number(fp))) {
                ch->sex = *sex;
            } else {
                bug("fread_char: unknown sex.");
            }
        } else if (word == cf::ShortDescription) {
            ch->short_descr = fread_string(fp);
        } else if (word == cf::Skill) {
            const int value = fread_number(fp);
            const auto skill = fread_word(fp);
            const int sn = skill_lookup(skill);
            if (sn < 0) {
                bug("fread_char: unknown skill: {}", skill);
            } else
                ch->pcdata->learned[sn] = value;
        } else if (word == cf::TrueSex) {
            if (auto sex = Sex::try_from_integer(fread_number(fp))) {
                ch->pcdata->true_sex = *sex;
            } else {
                bug("fread_char: unknown truesex.");
            }
        } else if (word == cf::Train) {
            ch->train = fread_number(fp);
        } else if (word == cf::Trust) {
            ch->trust = fread_number(fp);
        } else if (word == cf::Title) {
            ch->set_title(fread_string(fp));
        } else if (word == cf::Version) {
            auto raw_version = fread_number(fp);
            auto version = magic_enum::enum_cast<CharVersion>(raw_version);
            if (version.has_value()) {
                ch->version = version.value();
            } else {
                bug("fread_char: unknown version: {}", raw_version);
            }
        } else if (word == cf::Vnum) {
            ch->mobIndex = get_mob_index(fread_number(fp));
        } else if (word == cf::Wimpy) {
            ch->wimpy = fread_number(fp);
        } else if (word == cf::PronounPossessive) {
            ch->pcdata->pronouns.possessive = fread_string(fp);
        } else if (word == cf::PronounSubjective) {
            ch->pcdata->pronouns.subjective = fread_string(fp);
        } else if (word == cf::PronounObjective) {
            ch->pcdata->pronouns.objective = fread_string(fp);
        } else if (word == cf::PronounReflexive) {
            ch->pcdata->pronouns.reflexive = fread_string(fp);
        }
    }
}

void fread_obj(Char *ch, FILE *fp, ObjectNestMap &nest_level_to_obj) {
    namespace cf = charfilemeta;
    Object *obj = nullptr;
    std::string word;
    ush_int nest_level = 0;
    bool fNest = false;
    bool fVnum = false;
    bool first = true; /* used to counter fp offset */
    bool make_new = false; /* update object */
    word = feof(fp) ? cf::End : fread_word(fp);
    if (word == cf::Vnum) {
        const auto vnum = fread_number(fp);
        first = false; /* fp will be in right place */
        if (get_obj_index(vnum) == nullptr) {
            bug("fread_obj: bad vnum {}.", vnum);
        } else {
            auto obj_uptr = get_obj_index(vnum)->create_object();
            obj = obj_uptr.get();
            object_list.push_back(std::move(obj_uptr));
        }
    }

    fVnum = true;

    for (;;) {
        if (first)
            first = false;
        else
            word = feof(fp) ? cf::End : fread_word(fp);
        if (word.empty() || word[0] == '*') {
            fread_to_eol(fp);
        } else if (word == cf::Affected) {
            AFFECT_DATA af;
            // Spell effects on chars and customized objects in PFiles
            const auto affected_by = fread_word(fp);
            const auto sn = skill_lookup(affected_by);
            if (sn < 0)
                bug("fread_obj: unknown skill {}.", affected_by);
            else
                af.type = sn;
            af.level = fread_number(fp);
            af.duration = fread_number(fp);
            af.modifier = fread_number(fp);
            af.location = static_cast<AffectLocation>(fread_number(fp));
            af.bitvector = fread_number(fp);
            obj->affected.add_at_end(af);
        } else if (word == cf::Cost) {
            obj->cost = fread_number(fp);
        } else if (word == cf::Description) {
            obj->description = fread_string(fp);
        } else if (word == cf::Enchanted) {
            obj->enchanted = true;
        } else if (word == cf::ExtraFlags) {
            obj->extra_flags = fread_number(fp);
        } else if (word == cf::ExtraDescription) {
            auto keyword = fread_string(fp);
            auto description = fread_string(fp);
            obj->extra_descr.emplace_back(ExtraDescription{keyword, description});
        } else if (word == cf::End) {
            if (!fNest || !fVnum || obj->objIndex == nullptr) {
                bug("fread_obj: incomplete object.");
                extract_obj(obj);
                return;
            } else {
                if (make_new) {
                    const auto wear = obj->worn_loc;
                    extract_obj(obj);
                    auto obj_uptr = obj->objIndex->create_object();
                    auto *obj = obj_uptr.get();
                    object_list.push_back(std::move(obj_uptr));
                    obj->worn_loc = wear;
                }
                if (nest_level == 0 || nest_level_to_obj.find(nest_level) == nest_level_to_obj.end())
                    obj_to_char(obj, ch);
                else
                    obj_to_obj(obj, nest_level_to_obj[nest_level - 1]);
                return;
            }
        } else if (word == cf::ItemType) {
            const auto raw_obj_type = fread_number(fp);
            if (const auto opt_obj_type = ObjectTypes::try_from_integer(raw_obj_type)) {
                obj->type = *opt_obj_type;
            } else {
                bug("fread_obj: bad object type: {}", raw_obj_type);
            }
        } else if (word == cf::ObjectLevel) {
            obj->level = fread_number(fp);
        } else if (word == cf::Name) {
            obj->name = fread_string(fp);
        } else if (word == cf::Nest) {
            nest_level = fread_number(fp);
            if (nest_level >= MaxObjectNest) {
                bug("fread_obj: bad nest {}.", nest_level);
            } else {
                nest_level_to_obj[nest_level] = obj;
                fNest = true;
            }
        } else if (word == cf::ShortDescription) {
            obj->short_descr = fread_string(fp);
        } else if (word == cf::Spell) {
            int iValue = fread_number(fp);
            const auto spell = fread_word(fp);
            int sn = skill_lookup(spell);
            if (iValue < 0 || iValue > 3) {
                bug("fread_obj: bad iValue {}.", iValue);
            } else if (sn < 0) {
                bug("fread_obj: unknown skill {}.", spell);
            } else {
                obj->value[iValue] = sn;
            }
        } else if (word == cf::Time) {
            obj->timer = fread_number(fp);
        } else if (word == cf::Val) {
            obj->value[0] = fread_number(fp);
            obj->value[1] = fread_number(fp);
            obj->value[2] = fread_number(fp);
            obj->value[3] = fread_number(fp);
            obj->value[4] = fread_number(fp);
        } else if (word == cf::Vnum) {
            int vnum = fread_number(fp);
            if ((obj->objIndex = get_obj_index(vnum)) == nullptr)
                bug("fread_obj: bad vnum {}.", vnum);
            else
                fVnum = true;
        } else if (word == cf::WearFlags) {
            obj->wear_flags = fread_number(fp);
        } else if (word == cf::WearLoc) {
            if (const auto opt_worn_loc = magic_enum::enum_cast<Worn>(fread_number(fp))) {
                obj->worn_loc = *opt_worn_loc;
            } else {
                bug("fread_obj: bad worn location {}.", word);
            }
        } else if (word == cf::Weight) {
            obj->weight = fread_number(fp);
        } else if (word == cf::WearString) {
            obj->wear_string = fread_string(fp);
        } else {
            bug("fread_obj: no match for {}.", word);
            fread_to_eol(fp);
        }
    }
}

/* load a pet from the forgotten reaches */
void fread_pet(Char *ch, FILE *fp) {
    namespace cf = charfilemeta;
    std::string word;
    ObjectNestMap nest_level_to_obj{};
    Char *pet;
    /* first entry had BETTER be the vnum or we barf */
    word = feof(fp) ? cf::End : fread_word(fp);
    if (word == cf::Vnum) {
        int vnum = fread_number(fp);
        if (get_mob_index(vnum) == nullptr) {
            bug("fread_pet: bad vnum {}.", vnum);
            pet = create_mobile(get_mob_index(Mobiles::Fido));
        } else
            pet = create_mobile(get_mob_index(vnum));
    } else {
        bug("fread_pet: no vnum in file.");
        pet = create_mobile(get_mob_index(Mobiles::Fido));
    }

    for (;;) {
        word = feof(fp) ? cf::End : fread_word(fp);
        if (word.empty() || word[0] == '*') {
            fread_to_eol(fp);
        } else if (word[0] == '#') {
            fread_obj(pet, fp, nest_level_to_obj);
        } else if (word == cf::ActFlags) {
            pet->act = fread_number(fp);
        } else if (word == cf::AffectedBy) {
            pet->affected_by = fread_number(fp);
        } else if (word == cf::Alignment) {
            pet->alignment = fread_number(fp);
        } else if (word == cf::ArmourClasses) {
            for (int i = 0; i < 4; i++)
                pet->armor[i] = fread_number(fp);
        } else if (word == cf::Affected) {
            AFFECT_DATA af;
            const auto sn = skill_lookup(fread_word(fp));
            if (sn < 0)
                bug("fread_pet: unknown skill #{}", sn);
            else
                af.type = sn;

            af.level = fread_number(fp);
            af.duration = fread_number(fp);
            af.modifier = fread_number(fp);
            af.location = static_cast<AffectLocation>(fread_number(fp));
            af.bitvector = fread_number(fp);
            pet->affected.add_at_end(af);
        } else if (word == cf::AttribModifiers) {
            for (auto &stat : pet->mod_stat)
                stat = fread_number(fp);
        } else if (word == cf::Attribs) {
            for (auto &stat : pet->perm_stat)
                stat = fread_number(fp);
        } else if (word == cf::CommFlags) {
            pet->comm = fread_number(fp);
        } else if (word == cf::DamRoll) {
            pet->damroll = fread_number(fp);
        } else if (word == cf::Description) {
            pet->description = fread_string(fp);
        } else if (word == cf::End) {
            pet->leader = ch;
            pet->master = ch;
            ch->pet = pet;
            return;
        } else if (word == cf::Experience) {
            pet->exp = fread_number(fp);
        } else if (word == cf::Gold) {
            pet->gold = fread_number(fp);
        } else if (word == cf::HitRoll) {
            pet->hitroll = fread_number(fp);
        } else if (word == cf::HitManaMove) {
            pet->hit = fread_number(fp);
            pet->max_hit = fread_number(fp);
            pet->mana = fread_number(fp);
            pet->max_mana = fread_number(fp);
            pet->move = fread_number(fp);
            pet->max_move = fread_number(fp);
        } else if (word == cf::Level) {
            pet->level = fread_number(fp);
        } else if (word == cf::LongDescription) {
            pet->long_descr = fread_string(fp);
        } else if (word == cf::Name) {
            pet->name = fread_string(fp);
        } else if (word == cf::Position) {
            pet->position = Position::read_from_number(fp);
        } else if (word == cf::Race) {
            pet->race = race_lookup(fread_string(fp));
        } else if (word == cf::SavingThrow) {
            pet->saving_throw = fread_number(fp);
        } else if (word == cf::Sex) {
            if (auto sex = Sex::try_from_integer(fread_number(fp))) {
                pet->sex = *sex;
            } else {
                bug("fread_pet: unknown sex.");
            }
        } else if (word == cf::ShortDescription) {
            pet->short_descr = fread_string(fp);
        } else {
            bug("fread_pet: no match for {}.", word);
            fread_to_eol(fp);
        }
    }
}

} // namespace

std::string filename_for_player(std::string_view player_name) {
    return fmt::format("{}{}", Configuration::singleton().player_dir(), initial_caps_only(player_name));
}

std::string filename_for_god(std::string_view player_name) {
    return fmt::format("{}{}", Configuration::singleton().gods_dir(), initial_caps_only(player_name));
}

void CharSaver::save(const Char &ch) const {
    if (!ch.player()) // At the moment NPCs can't be persisted.
        return;
    const Char *player = ch.player();
    FILE *god_file = nullptr;
    // Only open the god file if it's an imm so we don't end up an empty god file for mortals.
    if (player->is_immortal()) {
        god_file = fopen(filename_for_god(ch.name).c_str(), "w");
    }
    FILE *player_file = fopen(filename_for_player(ch.name).c_str(), "w");
    save(ch, god_file, player_file);
    if (god_file)
        fclose(god_file);
    if (player_file)
        fclose(player_file);
}

void CharSaver::save(const Char &ch, FILE *god_file, FILE *player_file) const {
    if (!ch.player()) // At the moment NPCs can't be persisted.
        return;
    const Char *player = ch.player();
    if (player->is_immortal()) {
        if (god_file) {
            fmt::print(god_file, "Lev {:<2} Trust {:<2}  {}{}\n", player->level, player->get_trust(), player->name,
                       player->pcdata->title);
        } else {
            bug("Unable to write god file for {}", player->name);
        }
    }
    if (player_file) {
        save_char_obj(player, player_file);
    } else {
        bug("Unable to write player file for {}", player->name);
    }
}

/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj(const Char *ch) {
    CharSaver saver;
    saver.save(*ch);
}

void save_char_obj(const Char *ch, FILE *fp) {
    fwrite_char(ch, fp);
    fwrite_objs(ch, ch->carrying, fp, 0);
    /* save the pets */
    if (ch->pet != nullptr && ch->pet->in_room == ch->in_room)
        fwrite_pet(ch, ch->pet, fp);
    namespace cf = charfilemeta;
    fmt::print(fp, "#{}\n", cf::SectionEnd);
}

// Although the LastLogin* fields are read from the player file, it's transient info unused
// by the mud, and when a Char is saved it's read from the Char's Descriptor. But capturing
// LastLogin* makes it easier to do offline processing of player files.
void load_into_char(Char &character, LastLoginInfo &last_login, FILE *fp) {
    // Each entry in this map tracks the last Object loaded for the Char from its pfile
    // at a nesting level specified with the "Nest" keyword. It doesn't own the pointers.
    // Pets can also load objects, and use their own ObjectNesting.
    ObjectNestMap nest_level_to_obj{};
    namespace cf = charfilemeta;
    for (;;) {
        auto letter = fread_letter(fp);
        if (letter == '*') {
            fread_to_eol(fp);
            continue;
        }

        if (letter != '#') {
            bug("load_char_obj: # not found.");
            break;
        }
        auto word = fread_string_eol(fp);
        if (word == cf::SectionPlayer) {
            fread_char(&character, last_login, fp);
            affect_strip(&character, gsn_ride);
        } else if (word == cf::SectionObject)
            fread_obj(&character, fp, nest_level_to_obj);
        else if (word == cf::SectionPet)
            fread_pet(&character, fp);
        else if (word == cf::SectionEnd)
            break;
        else {
            bug("load_char_obj: bad section: '{}'", word);
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
    ch->act = to_int(PlayerActFlag::PlrAutoPeek) | to_int(PlayerActFlag::PlrAutoAssist)
              | to_int(PlayerActFlag::PlrAutoExit) | to_int(PlayerActFlag::PlrAutoGold)
              | to_int(PlayerActFlag::PlrAutoLoot) | to_int(PlayerActFlag::PlrAutoSac)
              | to_int(PlayerActFlag::PlrNoSummon);
    ch->comm = to_int(CommFlag::Combine) | to_int(CommFlag::Prompt) | to_int(CommFlag::ShowAfk)
               | to_int(CommFlag::ShowDefence);
    ranges::fill(ch->perm_stat, 13);

    auto *fp = fopen(filename_for_player(player_name).c_str(), "r");
    if (fp) {
        res.newly_created = false;
        LastLoginInfo ignored;
        load_into_char(*ch, ignored, fp);
        fclose(fp);
    }

    /* initialize race */
    if (!res.newly_created) {
        if (ch->race == 0)
            ch->race = race_lookup("human");

        ch->body_size = pc_race_table[ch->race].body_size;
        ch->attack_type = Attacks::index_of("punch");

        for (auto *group : pc_race_table[ch->race].skills) {
            if (!group)
                break;
            group_add(ch, group, false);
        }
        ch->affected_by = ch->affected_by | race_table[ch->race].aff;
        ch->imm_flags = ch->imm_flags | race_table[ch->race].imm;
        ch->res_flags = ch->res_flags | race_table[ch->race].res;
        ch->vuln_flags = ch->vuln_flags | race_table[ch->race].vuln;
        ch->morphology = race_table[ch->race].morphology;
        ch->parts = race_table[ch->race].parts;
    }
    return res;
}
