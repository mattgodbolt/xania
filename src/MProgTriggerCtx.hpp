/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

#include <string>
#include <variant>

struct Char;
struct Object;

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

} // namespace MProg
