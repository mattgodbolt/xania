/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

#include "MProgTriggerCtx.hpp"

#include <optional>

// Internal Mob Prog functions. App code shouldn't need to include this header.
namespace MProg {

struct Program;

namespace impl {

bool compare_strings(std::string_view lhs, std::string_view opr, std::string_view rhs);
bool compare_ints(const int lhs, std::string_view opr, const int rhs);
bool evaluate_if(std::string_view ifchck, Char *mob, const Char *actor, const Object *obj, const Target target,
                 const Char *rndm);
std::string expand_var(const char c, const Char *mob, const Char *actor, const Object *obj, const Target target,
                       const Char *rndm);
void interpret_command(std::string_view cmnd, Char *mob, const Char *actor, const Object *obj, const Target target,
                       const Char *rndm);
Char *random_mortal_in_room(Char *mob);
void mprog_driver(Char *mob, const Program &prog, const Char *actor, const Object *obj, const Target target);
std::string_view type_to_name(const TypeFlag type);
// Execute at least one mob prog of the specified type on mob, but
// only if a dice roll against the program's probability succeeds.
void exec_with_chance(Char *mob, Char *actor, Object *obj, const Target target, const TypeFlag type);

// IfExpr provides a view into a parsed if expression. Instances of this must not
// live beyond the life of the string that was parsed.
struct IfExpr {
    std::string_view function;
    std::string_view arg;
    std::string_view op;
    std::string_view operand;
    bool operator==(const IfExpr &other) const = default;

    static std::optional<IfExpr> parse_if(std::string_view text);
};

}
}