#include "MobIndexData.hpp"

#include "db.h"
#include "handler.hpp"
#include "lookup.h"
#include "merc.h"
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

    /* read armor class */
    ac[AC_PIERCE] = fread_number(fp) * 10;
    ac[AC_BASH] = fread_number(fp) * 10;
    ac[AC_SLASH] = fread_number(fp) * 10;
    ac[AC_EXOTIC] = fread_number(fp) * 10;

    /* read flags and add in data from the race table */
    off_flags = fread_flag(fp) | race_table[race].off;
    imm_flags = fread_flag(fp) | race_table[race].imm;
    res_flags = fread_flag(fp) | race_table[race].res;
    vuln_flags = fread_flag(fp) | race_table[race].vuln;

    /* vital statistics */
    start_pos = position_lookup(fread_word(fp));
    default_pos = position_lookup(fread_word(fp));
    sex = sex_lookup(fread_word(fp));
    gold = fread_number(fp);

    form = fread_flag(fp) | race_table[race].form;
    parts = fread_flag(fp) | race_table[race].parts;
    /* size */
    size = size_lookup(fread_word(fp));
    material = material_lookup(fread_word(fp));

    for (;;) {
        auto letter = fread_letter(fp);

        if (letter == 'F') {
            auto *word = fread_word(fp);
            auto vector = fread_flag(fp);

            if (!str_prefix(word, "act")) {
                REMOVE_BIT(act, vector);
            } else if (!str_prefix(word, "aff")) {
                REMOVE_BIT(affected_by, vector);
            } else if (!str_prefix(word, "off")) {
                REMOVE_BIT(off_flags, vector);
            } else if (!str_prefix(word, "imm")) {
                REMOVE_BIT(imm_flags, vector);
            } else if (!str_prefix(word, "res")) {
                REMOVE_BIT(res_flags, vector);
            } else if (!str_prefix(word, "vul")) {
                REMOVE_BIT(vuln_flags, vector);
            } else if (!str_prefix(word, "for")) {
                REMOVE_BIT(form, vector);
            } else if (!str_prefix(word, "par")) {
                REMOVE_BIT(parts, vector);
            } else {
                bug("Flag remove: flag '{}' not found.", word);
                exit(1);
            }
        } else {
            ungetc(letter, fp);
            break;
        }

        if (letter != 'S') {
            bug("Load_mobiles: vnum {} non-S.", vnum);
            exit(1);
        }
    }

    /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
    auto letter = fread_letter(fp);
    if (letter == '>') {
        ungetc(letter, fp);
        mprog_read_programs(fp, this);
    } else
        ungetc(letter, fp);
}
