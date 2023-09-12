/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include <string_view>

class ArgParser;
struct Char;

bool parse_customizations(Char *ch, ArgParser args);
void list_available_group_costs(Char *ch);
unsigned int exp_per_level(const Char *ch, int points);
void check_improve(Char *ch, const int skill_num, const bool success, const int multiplier);
int group_lookup(std::string_view name);
void gn_add(Char *ch, const int gn);
void gn_remove(Char *ch, const int gn);
void group_add(Char *ch, std::string_view name, const bool deduct);
void group_remove(Char *ch, const char *name);
int get_skill_level(const Char *ch, const int gsn);
int get_skill_difficulty(Char *ch, const int gsn);
int get_skill_trains(Char *ch, const int gsn);
int get_group_trains(Char *ch, const int gsn);
int get_group_level(Char *ch, const int gsn);
