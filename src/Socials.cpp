/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Socials.hpp"
#include "db.h"
#include "string_utils.hpp"

struct social_type social_table[MAX_SOCIALS];
int social_count = 0;

/* snarf a socials file */
void load_socials(FILE *fp) {
    for (;;) {
        struct social_type social;
        const auto name = fread_word(fp);
        if (matches(name, "#0"))
            return; /* done */
        strcpy(social.name, name);
        fread_to_eol(fp);
        std::string temp = fread_stdstring_eol(fp);
        if (matches(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else if (!matches(temp, "$"))
            social.char_no_arg = temp;

        temp = fread_stdstring_eol(fp);
        if (matches(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else if (!matches(temp, "$"))
            social.others_no_arg = temp;

        temp = fread_stdstring_eol(fp);
        if (matches(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else if (!matches(temp, "$"))
            social.char_found = temp;

        temp = fread_stdstring_eol(fp);
        if (matches(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else if (!matches(temp, "$"))
            social.others_found = temp;

        temp = fread_stdstring_eol(fp);
        if (matches(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else if (!matches(temp, "$"))
            social.vict_found = temp;

        temp = fread_stdstring_eol(fp);
        // MRG char_not_found wasn't used anywhere
        if (matches(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }

        temp = fread_stdstring_eol(fp);
        if (matches(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else if (!matches(temp, "$"))
            social.char_auto = temp;

        temp = fread_stdstring_eol(fp);
        if (matches(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else if (!matches(temp, "$"))
            social.others_auto = temp;

        social_table[social_count] = social;
        social_count++;
    }
}
