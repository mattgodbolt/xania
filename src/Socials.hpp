/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include <cstdio>

/*
 * Structure for a social in the socials table.
 */
struct social_type {
    char name[20];
    const char *char_no_arg;
    const char *others_no_arg;
    const char *char_found;
    const char *others_found;
    const char *vict_found;
    const char *char_auto;
    const char *others_auto;
};

extern struct social_type social_table[];
extern int social_count;

void load_socials(FILE *fp);
