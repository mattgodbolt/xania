/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

#include "MProgTriggerCtx.hpp"

#include <optional>

class ArgParser;

// The primary header for the mud to interface with the Mob Prog features
// mainly via "triggers" that fire when specific events occur.
namespace MProg {

// If either a Char or Object are non null, map them to the MProg Target variant.
Target to_target(const Char *ch, const Object *obj);
void wordlist_check(std::string_view arg, Char *mob, const Char *actor, const Object *obj, const Target target,
                    const TypeFlag type);
void act_trigger(std::string_view buf, Char *mob, const Char *ch, const Object *obj, const Target target);
void bribe_trigger(Char *mob, Char *ch, int amount);
void entry_trigger(Char *mob);
void give_trigger(Char *mob, Char *ch, Object *obj);
void greet_trigger(Char *mob);
void fight_trigger(Char *mob, Char *ch);
void hitprcnt_trigger(Char *mob, Char *ch);
void death_trigger(Char *mob);
void random_trigger(Char *mob);
void speech_trigger(std::string_view txt, const Char *mob);
void show_programs(Char *ch, ArgParser args);

}
