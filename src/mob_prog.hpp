/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

/* Merc-2.2 MOBProgs - Faramir 31/8/1998*/

#include "MobProgTypeFlag.hpp"

#include <optional>
#include <variant>
#include <vector>

struct Char;
struct Object;
struct Area;
class ArgParser;
struct MobIndexData;

namespace MProg {

MobProgTypeFlag mprog_name_to_type(std::string_view name);
bool read_program(std::string_view file_name, FILE *prog_file, MobIndexData *mobIndex);
void load_mobprogs(FILE *fp);

// When a mobprog is triggered via an 'act()' call (often using MobProgAct), the caller may have specified an
// optional target object or target character. These can be referenced within the act format string with $ variables.
// The target is made available to act_trigger() and the lower level mprog routines using this variant.
using Target = std::variant<nullptr_t, const Char *, const Object *>;

struct IfExpr {
    std::string_view function;
    std::string_view arg;
    std::string_view op;
    std::string_view operand;
    bool operator==(const IfExpr &other) const = default;

    static std::optional<IfExpr> parse_if(std::string_view text);
};

Target to_target(const Char *vict, const Object *vict_obj);

void wordlist_check(std::string_view arg, Char *mob, const Char *actor, const Object *obj, const MProg::Target target,
                    const MobProgTypeFlag type);
void percent_check(Char *mob, Char *actor, Object *object, void *vo, int type);
void act_trigger(std::string_view buf, Char *mob, const Char *ch, const Object *obj, const MProg::Target target);
void bribe_trigger(Char *mob, Char *ch, int amount);
void entry_trigger(Char *mob);
void give_trigger(Char *mob, Char *ch, Object *obj);
void greet_trigger(Char *mob);
void fight_trigger(Char *mob, Char *ch);
void hitprcnt_trigger(Char *mob, Char *ch);
void death_trigger(Char *mob);
void random_trigger(Char *mob);
void speech_trigger(std::string_view txt, const Char *mob);

}

void do_mpstat(Char *ch, ArgParser args);

class MobProgAct {
public:
    MobProgAct(std::string &&act_message_trigger, const Char *ch, const Object *obj, const MProg::Target target)
        : act_message_trigger_(std::move(act_message_trigger)), ch_(ch), obj_(obj), target_(target) {}

    [[nodiscard]] std::string_view act_message_trigger() const { return act_message_trigger_; }
    [[nodiscard]] const Char *character() const { return ch_; }
    [[nodiscard]] const Object *object() const { return obj_; }
    [[nodiscard]] const MProg::Target target() const { return target_; }

private:
    const std::string act_message_trigger_;
    const Char *ch_;
    const Object *obj_;
    const MProg::Target target_;
};

struct MobProg {
    const MobProgTypeFlag type;
    const std::string arglist;
    const std::vector<std::string> lines;
};
