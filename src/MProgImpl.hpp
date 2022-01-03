/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

#include "MProgTriggerCtx.hpp"
#include "Types.hpp"

#include <optional>
#include <stack>
#include <type_traits>
#include <variant>

class Rng;

// Internal Mob Prog functions. App code shouldn't need to include this header.
namespace MProg {

struct Program;

namespace impl {

using Operand = std::variant<std::string_view, const int>;

// IfExpr provides a view into a parsed if expression. Instances of this must not
// live beyond the life of the string that was parsed.
struct IfExpr {
    std::string_view function;
    std::string_view arg;
    std::string_view op;
    Operand operand;
    bool operator==(const IfExpr &other) const = default;

    static std::optional<IfExpr> parse_if(std::string_view text);
};

// Track if the current execution frame is executable (inherited from the parent frame)
// and whether the current frame is executing or skipping commands.
// The executing flag can change if there's an if/else block.
struct Frame {
    bool executable{};
    bool executing{};
};

// Container for characters, objects and random number generation involved in one execution of a mob prog
// as well as the stack of execution frames.
struct ExecutionCtx {

    ExecutionCtx(Rng &rng, Char *pmob, const Char *pactor, const Char *prandom, const Char *pact_targ_char,
                 const Object *pobj, const Object *pact_targ_obj)
        : rng(rng), mob(pmob), actor(pactor), random(prandom), act_targ_char(pact_targ_char), obj(pobj),
          act_targ_obj(pact_targ_obj) {
        frames.push({true, true});
    }
    // Returns the Char referenced by ifexpr's dollar argument, can be null.
    [[nodiscard]] const Char *select_char(const IfExpr &ifexpr) const;
    // Returns the Object referenced by ifexpr's dollar argument, can be null.
    [[nodiscard]] const Object *select_obj(const IfExpr &ifexpr) const;

    Rng &rng;
    // The mob the program is running on
    Char *mob;
    // Typically the player involved when the trigger fired. Nullable.
    const Char *actor;
    // A random mortal in the same room. Nullable.
    const Char *random;
    // This will be populated if the prog was triggered via an act() event and if the act
    // message referenced a target character. Can be different to the 'actor' involved. Nullable.
    const Char *act_targ_char;
    // Typically the object involved when the trigger fired. Nullable.
    const Object *obj;
    // This will be populated if the prog was triggered via an act() event and if the act
    // message referenced a target object. Can be different to the 'object' involved. Nullable.
    const Object *act_targ_obj;

    std::stack<Frame> frames{};
};

bool compare_strings(std::string_view lhs, std::string_view opr, std::string_view rhs);
bool compare_ints(const int lhs, std::string_view opr, const int rhs);
bool evaluate_if(std::string_view ifchck, const ExecutionCtx &ctx);
std::string expand_var(const char c, const ExecutionCtx &ctx);
void interpret_command(std::string_view cmnd, const ExecutionCtx &ctx);
Char *random_mortal_in_room(Char *mob, Rng &rng);
void mprog_driver(Char *mob, const Program &prog, const Char *actor, const Object *obj, const Target target, Rng &rng);
std::string_view type_to_name(const TypeFlag type);
// Execute at least one mob prog of the specified type on mob, but
// only if a dice roll against the program's probability succeeds.
void exec_with_chance(Char *mob, Char *actor, Object *obj, const Target target, const TypeFlag type, Rng &rng);

}
}