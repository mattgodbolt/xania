/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Materials.hpp"

/*
 * Liquid properties.
 */
const struct liq_type liq_table[] = {{"water", "clear", {0, 1, 10}}, /*  0 */
                                     {"beer", "amber", {3, 2, 5}},
                                     {"wine", "rose", {5, 2, 5}},
                                     {"ale", "brown", {2, 2, 5}},
                                     {"dark ale", "dark", {1, 2, 5}},

                                     {"whisky", "golden", {6, 1, 4}}, /*  5 */
                                     {"lemonade", "pink", {0, 1, 8}},
                                     {"firebreather", "boiling", {10, 0, 0}},
                                     {"local specialty", "everclear", {3, 3, 3}},
                                     {"slime mold juice", "green", {0, 4, -8}},

                                     {"milk", "white", {0, 3, 6}}, /* 10 */
                                     {"tea", "tan", {0, 1, 6}},
                                     {"coffee", "black", {0, 1, 6}},
                                     {"blood", "red", {0, 2, -1}},
                                     {"salt water", "clear", {0, 1, -2}},

                                     {"cola", "cherry", {0, 1, 5}} /* 15 */
                                     ,
                                     {"red wine", "red", {5, 2, 5}}
                                     /* 16 */
                                     ,
                                     {nullptr, nullptr, {}}};

const struct materials_type material_table[] = {
    /* { percentage resilience, name } */
    {0, "none"},      {40, "default"}, {90, "adamantite"}, {70, "iron"},      {15, "glass"},
    {71, "bronze"},   {30, "cloth"},   {35, "wood"},       {10, "paper"},     {75, "steel"},
    {85, "stone"},    {0, "food"},     {55, "silver"},     {55, "gold"},      {30, "leather"},
    {20, "vellum"},   {5, "china"},    {10, "clay"},       {75, "brass"},     {45, "bone"},
    {82, "platinum"}, {40, "pearl"},   {65, "mithril"},    {100, "octarine"}, {0, nullptr}};
