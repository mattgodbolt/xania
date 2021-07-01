/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "ExtraFlags.hpp"

/*
 * Extra flag names.
 * The info_ flags are unused, except info_mes.
 */

const char *flagname_extra[64] = {
    "wnet ", /* 0 */
    "wn_debug ", "wn_mort ",  "wn_imm ", "wn_bug ",    "permit ", /* 5 */
    "wn_tick",   "",          "",        "info_name ", "info_email ", /* 10 */
    "info_mes ", "info_url ", "",        "tip_std ",   "tip_olc", /*15 */
    "tip_adv",   "",          "",        "",           "", /*20*/
    "",          "",          "",        "",           "", /* 25 */
    "",          "",          "",        "",           "", /* 30 */
    "",          "",          "",        "",           "", /* 35 */
    "",          "",          "",        "",           "", /* 40 */
    "",          "",          "",        "",           "", /* 45 */
    "",          "",          "",        "",           "", /* 50 */
    "",          "",          "",        "",           "", /*55*/
    "",          "",          "",        "",           "", /*60*/
    "",          "",          "" /* 64 */
};
