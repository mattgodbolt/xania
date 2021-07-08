#include "MobIndexData.hpp"
#include "ArmourClass.hpp"
#include "BitsCharAct.hpp"
#include "Races.hpp"
#include "common/BitOps.hpp"

#include "db.h"
#include "handler.hpp"
#include "lookup.h"
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
    player_name = fread_stdstring(fp);
    short_descr = lower_case_articles(fread_stdstring(fp));
    long_descr = upper_first_character(fread_stdstring(fp));
    description = upper_first_character(fread_stdstring(fp));
    race = race_lookup(fread_stdstring(fp));

    act = fread_flag(fp) | ACT_IS_NPC | race_table[race].act;
    affected_by = fread_flag(fp) | race_table[race].aff;
    alignment = fread_number(fp);

    group = fread_number(fp);

    level = fread_number(fp);
    hitroll = fread_number(fp);

    hit = Dice::from_file(fp);
    mana = Dice::from_file(fp);
    damage = Dice::from_file(fp);
    dam_type = attack_lookup(fread_word(fp));

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

    form = fread_flag(fp) | race_table[race].form;
    parts = fread_flag(fp) | race_table[race].parts;
    size = size_lookup(fread_word(fp));
    material = material_lookup(fread_word(fp));

    for (;;) {
        // TODO: I'm pretty sure this is not exercised anywhere: the old code would unconditionally bug() and exit after
        // reading the "S". Confirm and remove if unused.
        auto letter = fread_letter(fp);

        if (letter == 'F') {
            auto *word = fread_word(fp);
            auto vector = fread_flag(fp);

            if (!str_prefix(word, "act")) {
                clear_bit(act, vector);
            } else if (!str_prefix(word, "aff")) {
                clear_bit(affected_by, vector);
            } else if (!str_prefix(word, "off")) {
                clear_bit(off_flags, vector);
            } else if (!str_prefix(word, "imm")) {
                clear_bit(imm_flags, vector);
            } else if (!str_prefix(word, "res")) {
                clear_bit(res_flags, vector);
            } else if (!str_prefix(word, "vul")) {
                clear_bit(vuln_flags, vector);
            } else if (!str_prefix(word, "for")) {
                clear_bit(form, vector);
            } else if (!str_prefix(word, "par")) {
                clear_bit(parts, vector);
            } else {
                bug("Flag remove: flag '{}' not found.", word);
                exit(1);
            }
        } else {
            ungetc(letter, fp);
            break;
        }
    }

    auto letter = fread_letter(fp);
    if (letter == '>') {
        ungetc(letter, fp);
        mprog_read_programs(fp, this);
    } else
        ungetc(letter, fp);
}
