#include "MobIndexData.hpp"
#include "Attacks.hpp"
#include "BodySize.hpp"
#include "CharActFlag.hpp"
#include "Logging.hpp"
#include "Materials.hpp"
#include "Races.hpp"
#include "common/BitOps.hpp"

#include "db.h"
#include "handler.hpp"
#include "string_utils.hpp"

std::optional<MobIndexData> MobIndexData::from_file(FILE *fp, const Logger &logger) {
    auto letter = fread_letter(fp);
    if (letter != '#') {
        logger.bug("Load_mobiles: # not found.");
        exit(1);
    }

    auto vnum = fread_number(fp, logger);
    if (vnum == 0)
        return {};

    return MobIndexData(vnum, fp, logger);
}

MobIndexData::MobIndexData(sh_int vnum, FILE *fp, const Logger &logger) : vnum(vnum) {
    player_name = fread_string(fp);
    short_descr = lower_case_articles(fread_string(fp));
    long_descr = upper_first_character(fread_string(fp));
    description = upper_first_character(fread_string(fp));
    race = race_lookup(fread_string(fp));

    act = fread_flag(fp, logger) | race_table[race].act;
    set_enum_bit(act, CharActFlag::Npc);
    affected_by = fread_flag(fp, logger) | race_table[race].aff;
    alignment = fread_number(fp, logger);

    group = fread_number(fp, logger);

    level = fread_number(fp, logger);
    hitroll = fread_number(fp, logger);

    hit = Dice::from_file(fp, logger);
    mana = Dice::from_file(fp, logger);
    damage = Dice::from_file(fp, logger);
    const auto attack_name = fread_word(fp);
    const auto attack_index = Attacks::index_of(attack_name);
    if (attack_index < 0) {
        logger.bug("Invalid attack type {} in mob #{}, defaulting to hit", vnum, attack_name);
        attack_type = Attacks::index_of("hit");
    } else {
        attack_type = attack_index;
    }

    // read armor class
    for (size_t i = 0; i < ac.size(); i++) {
        ac[i] = fread_number(fp, logger);
    }

    // read flags and add in data from the race table
    off_flags = fread_flag(fp, logger) | race_table[race].off;
    imm_flags = fread_flag(fp, logger) | race_table[race].imm;
    res_flags = fread_flag(fp, logger) | race_table[race].res;
    vuln_flags = fread_flag(fp, logger) | race_table[race].vuln;

    start_pos = Position::read_from_word(fp, logger);
    default_pos = Position::read_from_word(fp, logger);
    if (auto opt_sex = Sex::try_from_name(fread_word(fp))) {
        sex = *opt_sex;
    } else {
        logger.bug("Unrecognized sex.");
        exit(1);
    }
    gold = fread_number(fp, logger);

    morphology = fread_flag(fp, logger) | race_table[race].morphology;
    parts = fread_flag(fp, logger) | race_table[race].parts;
    const auto raw_body_size = fread_word(fp);
    if (const auto opt_body_size = BodySizes::try_lookup(raw_body_size)) {
        body_size = *opt_body_size;
    } else {
        logger.bug("Unrecognized body size: {},  defaulting!", raw_body_size);
        body_size = BodySize::Medium;
    }
    material = Material::lookup_with_default(fread_word(fp))->material;
}
