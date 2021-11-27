/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

/* Merc-2.2 MOBProgs - Faramir 31/8/1998*/

#include "MobProgTypeFlag.hpp"

struct Char;
struct Object;
struct Area;
class ArgParser;

void do_mpstat(Char *ch, ArgParser args);

void mprog_wordlist_check(std::string_view arg, Char *mob, const Char *actor, const Object *obj, const void *vo,
                          const MobProgTypeFlag type);
void mprog_percent_check(Char *mob, Char *actor, Object *object, void *vo, int type);
void mprog_act_trigger(std::string_view buf, Char *mob, const Char *ch, const Object *obj, const void *vo);
void mprog_bribe_trigger(Char *mob, Char *ch, int amount);
void mprog_entry_trigger(Char *mob);
void mprog_give_trigger(Char *mob, Char *ch, Object *obj);
void mprog_greet_trigger(Char *mob);
void mprog_fight_trigger(Char *mob, Char *ch);
void mprog_hitprcnt_trigger(Char *mob, Char *ch);
void mprog_death_trigger(Char *mob);
void mprog_random_trigger(Char *mob);
void mprog_speech_trigger(const char *txt, const Char *mob);

class MobProgAct {
public:
    MobProgAct(std::string &&act_message_trigger, const Char *ch, const Object *obj, const void *vo)
        : act_message_trigger_(std::move(act_message_trigger)), ch_(ch), obj_(obj), vo_(vo) {}

    [[nodiscard]] std::string_view act_message_trigger() const { return act_message_trigger_; }
    [[nodiscard]] const Char *character() const { return ch_; }
    [[nodiscard]] const Object *object() const { return obj_; }
    [[nodiscard]] const void *void_arg() const { return vo_; }

private:
    const std::string act_message_trigger_;
    const Char *ch_;
    const Object *obj_;
    const void *vo_;
};

struct MobProg {
    const MobProgTypeFlag type;
    const std::string arglist;
    const std::string comlist;
};
