#include "MobIndexData.hpp"
#include "Attacks.hpp"
#include "BodySize.hpp"
#include "CharActFlag.hpp"
#include "Logging.hpp"
#include "Races.hpp"
#include "common/BitOps.hpp"

#include "db.h"
#include "handler.hpp"
#include "string_utils.hpp"

std::optional<MobIndexData> MobIndexData::from_file(FILE *fp) {
    auto letter = fread_letter(fp);
    if (letter != '#') {
        bug("Load_mobiles: # not found.");
        exit(1);
    }

    auto vnum = fread_number(fp);
    if (vnum == 0)
        return {};

    return MobIndexData(vnum, fp);
}
#include "Materials.hpp"

MobIndexData::MobIndexData(sh_int vnum, FILE *fp) : vnum(vnum) {
    player_name = fread_string(fp);
    short_descr = lower_case_articles(fread_string(fp));
    long_descr = upper_first_character(fread_string(fp));
    description = upper_first_character(fread_string(fp));
    race = race_lookup(fread_string(fp));

    act = fread_flag(fp) | race_table[race].act;
    set_enum_bit(act, CharActFlag::Npc);
    affected_by = fread_flag(fp) | race_table[race].aff;
    alignment = fread_number(fp);

    group = fread_number(fp);

    level = fread_number(fp);
    hitroll = fread_number(fp);

    hit = Dice::from_file(fp);
    mana = Dice::from_file(fp);
    damage = Dice::from_file(fp);
    const auto attack_name = fread_word(fp);
    const auto attack_index = Attacks::index_of(attack_name);
    if (attack_index < 0) {
        bug("Invalid attack type {} in mob #{}, defaulting to hit", vnum, attack_name);
        attack_type = Attacks::index_of("hit");
    } else {
        attack_type = attack_index;
    }

    // read armor class
    for (size_t i = 0; i < ac.size(); i++) {
        ac[i] = fread_number(fp);
    }

    // read flags and add in data from the race table
    off_flags = fread_flag(fp) | race_table[race].off;
    imm_flags = fread_flag(fp) | race_table[race].imm;
    res_flags = fread_flag(fp) | race_table[race].res;
    vuln_flags = fread_flag(fp) | race_table[race].vuln;

    start_pos = Position::read_from_word(fp);
    default_pos = Position::read_from_word(fp);
    if (auto opt_sex = Sex::try_from_name(fread_word(fp))) {
        sex = *opt_sex;
    } else {
        bug("Unrecognized sex.");
        exit(1);
    }
    gold = fread_number(fp);

    morphology = fread_flag(fp) | race_table[race].morphology;
    parts = fread_flag(fp) | race_table[race].parts;
    const auto raw_body_size = fread_word(fp);
    if (const auto opt_body_size = BodySizes::try_lookup(raw_body_size)) {
        body_size = *opt_body_size;
    } else {
        bug("Unrecognized body size: {},  defaulting!", raw_body_size);
        body_size = BodySize::Medium;
    }
    material = Material::lookup_with_default(fread_word(fp))->material;
}
