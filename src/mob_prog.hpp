/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

#include <optional>
#include <string>
#include <variant>

struct Char;
struct Object;
struct Area;
class ArgParser;
struct MobIndexData;

namespace MProg {

enum class TypeFlag;

// When a mobprog is triggered via an 'act()' call (often using ActTriggerCtx), the caller may have specified an
// optional target object or target character. These can be referenced within the act format string with $ variables.
// The target is made available to act_trigger() and the lower level mprog routines using this variant.
using Target = std::variant<std::nullptr_t, const Char *, const Object *>;

// Carries context about an act() event that may potentially trigger a mob prog that is
// configured to be triggered by acts.
class ActTriggerCtx {
public:
    ActTriggerCtx(std::string &&act_message_trigger, const Char *ch, const Object *obj, const Target target)
        : act_message_trigger_(std::move(act_message_trigger)), ch_(ch), obj_(obj), target_(target) {}

    [[nodiscard]] std::string_view act_message_trigger() const { return act_message_trigger_; }
    [[nodiscard]] const Char *character() const { return ch_; }
    [[nodiscard]] const Object *object() const { return obj_; }
    [[nodiscard]] const Target target() const { return target_; }

private:
    const std::string act_message_trigger_;
    const Char *ch_;
    const Object *obj_;
    const Target target_;
};

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

namespace impl {

struct IfExpr {
    std::string_view function;
    std::string_view arg;
    std::string_view op;
    std::string_view operand;
    bool operator==(const IfExpr &other) const = default;

    static std::optional<IfExpr> parse_if(std::string_view text);
};

} // namespace impl

} // namespace MProg
