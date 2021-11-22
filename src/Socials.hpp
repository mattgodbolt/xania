/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include <cstdio>
#include <string>

/*
 * Structure for a social in the socials table.
 */
struct social_type {
    char name[20];
    std::string char_no_arg{};
    std::string others_no_arg{};
    std::string char_found{};
    std::string others_found{};
    std::string vict_found{};
    std::string char_auto{};
    std::string others_auto{};
};

extern struct social_type social_table[];
extern int social_count;

void load_socials(FILE *fp);
