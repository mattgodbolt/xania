/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Socials.hpp"
#include "db.h"

struct social_type social_table[MAX_SOCIALS];
int social_count = 0;

/* snarf a socials file */
void load_socials(FILE *fp) {
    for (;;) {
        struct social_type social;
        char *temp;
        /* clear social */
        social.char_no_arg = nullptr;
        social.others_no_arg = nullptr;
        social.char_found = nullptr;
        social.others_found = nullptr;
        social.vict_found = nullptr;
        social.char_auto = nullptr;
        social.others_auto = nullptr;

        temp = fread_word(fp);
        if (!strcmp(temp, "#0"))
            return; /* done */

        strcpy(social.name, temp);
        fread_to_eol(fp);

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_no_arg = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.char_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_no_arg = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.others_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_found = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.char_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_found = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.others_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.vict_found = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.vict_found = temp;

        temp = fread_string_eol(fp);
        // MRG char_not_found wasn't used anywhere
        if (!strcmp(temp, "$")) {
            /*social.char_not_found = nullptr*/;
        } else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        }

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.char_auto = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.char_auto = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp, "$"))
            social.others_auto = nullptr;
        else if (!strcmp(temp, "#")) {
            social_table[social_count] = social;
            social_count++;
            continue;
        } else
            social.others_auto = temp;

        social_table[social_count] = social;
        social_count++;
    }
}
