/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/

/***************************************************************************
 *  The MOBprograms have been contributed by N'Atas-ha.  Any support for   *
 *  these routines should not be expected from Merc Industries.  However,  *
 *  under no circumstances should the blame for bugs, etc be placed on     *
 *  Merc Industries.  They are not guaranteed to work on all systems due   *
 *  to their frequent use of strxxx functions.  They are also not the most *
 *  efficient way to perform their tasks, but hopefully should be in the   *
 *  easiest possible way to install and begin using. Documentation for     *
 *  such installation can be found in INSTALL.  Enjoy...         N'Atas-Ha *
 ***************************************************************************/

#include "MProgImpl.hpp"
#include "ArgParser.hpp"
#include "Char.hpp"
#include "Logging.hpp"
#include "MProgProgram.hpp"
#include "MProgTypeFlag.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"
#include "Pronouns.hpp"
#include "Rng.hpp"
#include "Room.hpp"
#include "common/BitOps.hpp"
#include "interp.h"
#include "string_utils.hpp"

#include <optional>
#include <string>
#include <unordered_map>

namespace MProg {

// Mob Prog implementation internals.
namespace impl {

bool compare_strings(std::string_view lhs, std::string_view opr, std::string_view rhs) {
    using namespace std::literals;
    if (opr == "=="sv)
        return matches(lhs, rhs);
    if (opr == "!="sv)
        return !matches(lhs, rhs);
    if (opr == "/"sv)
        return matches_inside(lhs, rhs);
    if (opr == "!/"sv)
        return !matches_inside(lhs, rhs);

    bug("Improper MOBprog operator '{}'", opr);
    return false;
}

bool compare_ints(const int lhs, std::string_view opr, const int rhs) {
    using namespace std::literals;
    if (opr == "=="sv)
        return lhs == rhs;
    if (opr == "!="sv)
        return lhs != rhs;
    if (opr == ">"sv)
        return lhs > rhs;
    if (opr == "<"sv)
        return lhs < rhs;
    if (opr == "<="sv)
        return lhs <= rhs;
    if (opr == ">="sv)
        return lhs >= rhs;
    if (opr == "&"sv)
        return lhs & rhs;
    if (opr == "|"sv)
        return lhs | rhs;

    bug("Improper MOBprog operator '{}'", opr);
    return false;
}

template <typename V, typename Cond>
struct Predicate {
    Predicate(V v, Cond cond) : validate(v), condition(cond) {}
    bool test(const IfExpr &ifexpr, const ExecutionCtx &ctx) const {
        return validate(ifexpr, ctx.mob) && condition(ifexpr, ctx);
    }
    typename std::conditional<std::is_function<V>::value, typename std::add_pointer<V>::type, V>::type validate;
    typename std::conditional<std::is_function<Cond>::value, typename std::add_pointer<Cond>::type, Cond>::type
        condition;
};

const Char *ExecutionCtx::select_char(const IfExpr &ifexpr) const {
    switch (ifexpr.arg[1]) {
    case 'i': return mob; // Self
    case 'n': return actor;
    case 't': return act_targ_char;
    case 'r': return random;
    default: return nullptr;
    }
}

const Object *ExecutionCtx::select_obj(const IfExpr &ifexpr) const {
    switch (ifexpr.arg[1]) {
    case 'o': return obj;
    case 'p': return act_targ_obj;
    default: return nullptr;
    }
}

bool expect_number_arg(const IfExpr &ifexpr, Char *mob) {
    if (!is_number(ifexpr.arg)) {
        bug("expect_number_arg: #{} bad argument to '{}': {}", mob->mobIndex->vnum, ifexpr.function, ifexpr.arg);
        return false;
    }
    return true;
};

bool expect_dollar_var(const IfExpr &ifexpr, Char *mob) {
    if (ifexpr.arg.length() != 2 || ifexpr.arg[0] != '$' || !std::isalpha(ifexpr.arg[1])) {
        bug("expect_dollar_var: #{} function expects a $var specifying a character or object, got: {}",
            mob->mobIndex->vnum, ifexpr.arg);
        return false;
    }
    return true;
}

bool expect_dollar_var_and_number_operand(const IfExpr &ifexpr, Char *mob) {
    if (expect_dollar_var(ifexpr, mob)) {
        if (std::holds_alternative<const int>(ifexpr.operand))
            return true;
        else {
            bug("expect_dollar_var_and_number_operand: #{} expected a number operand", mob->mobIndex->vnum);
            return false;
        }
    }
    return false;
};

bool expect_dollar_var_and_sv_operand(const IfExpr &ifexpr, Char *mob) {
    if (expect_dollar_var(ifexpr, mob)) {
        if (std::holds_alternative<std::string_view>(ifexpr.operand))
            return true;
        else {
            bug("expect_dollar_var_and_sv_operand: #{} expected a string view operand", mob->mobIndex->vnum);
            return false;
        }
    }
    return false;
};

bool rand(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    return ctx.rng.number_percent() <= parse_number(ifexpr.arg);
};

bool ispc(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && ch->is_pc();
}

bool isnpc(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && ch->is_npc();
}

bool isgood(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && ch->is_good();
}

bool isfight(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && ch->fighting;
}

bool isimmort(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && ch->is_immortal();
}

bool ischarmed(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && ch->is_aff_charm();
}

bool isfollow(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && ch->master && ch->master->in_room == ch->in_room;
}

bool isaffected(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && ch->affected_by & std::get<const int>(ifexpr.operand);
}

bool hitprcnt(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && compare_ints(ch->hit / ch->max_hit, ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool inroom(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && compare_ints(ch->in_room->vnum, ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool sex(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && compare_ints(ch->sex.integer(), ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool position(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && compare_ints(ch->position.integer(), ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool level(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && compare_ints(ch->level, ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool class_(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && compare_ints(ch->class_num, ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool goldamt(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *ch = ctx.select_char(ifexpr);
    return ch && compare_ints(ch->gold, ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool objtype(const IfExpr &ifexpr, const ExecutionCtx &ctx) {
    const auto *sobj = ctx.select_obj(ifexpr);
    return sobj
           && compare_ints(magic_enum::enum_integer<ObjectType>(sobj->type), ifexpr.op,
                           std::get<const int>(ifexpr.operand));
}

bool name(const IfExpr &ifexpr, const ExecutionCtx &ctx) {

    if (const auto *ch = ctx.select_char(ifexpr)) {
        return compare_strings(ch->name, ifexpr.op, std::get<std::string_view>(ifexpr.operand));
    }
    if (const auto *sobj = ctx.select_obj(ifexpr)) {
        return compare_strings(sobj->name, ifexpr.op, std::get<std::string_view>(ifexpr.operand));
    }
    bug("Mob: #{} Unrecognized char or obj variable: {}", ctx.mob->mobIndex->vnum, ifexpr.arg);
    return false;
}

using Validator = decltype(expect_number_arg);
using Cond = decltype(rand);

// Weave together the mob prog predicate functions with their names, along with
// the validators required by each.
const std::unordered_map<std::string_view, Predicate<Validator, Cond>> predicates({
    // clang-format off
    {"rand", {expect_number_arg, rand}},
    {"ispc", {expect_dollar_var, ispc}},
    {"isnpc", {expect_dollar_var, isnpc}},
    {"isgood", {expect_dollar_var, isgood}},
    {"isfight", {expect_dollar_var, isfight}},
    {"isimmort", {expect_dollar_var, isimmort}},
    {"ischarmed", {expect_dollar_var, ischarmed}},
    {"isfollow", {expect_dollar_var, isfollow}},
    {"isaffected", {expect_dollar_var_and_number_operand, isaffected}},
    {"hitprcnt", {expect_dollar_var_and_number_operand, hitprcnt}},
    {"inroom", {expect_dollar_var_and_number_operand, inroom}},
    {"sex", {expect_dollar_var_and_number_operand, sex}},
    {"position", {expect_dollar_var_and_number_operand, position}},
    {"level", {expect_dollar_var_and_number_operand, level}},
    {"class", {expect_dollar_var_and_number_operand, class_}},
    {"goldamt", {expect_dollar_var_and_number_operand, goldamt}},
    {"objtype", {expect_dollar_var_and_number_operand, objtype}},
    {"name", {expect_dollar_var_and_sv_operand, name}}
    // clang-format on
});

bool evaluate_if(std::string_view ifchck, const ExecutionCtx &ctx) {
    const auto ifexpr = IfExpr::parse_if(ifchck);
    if (!ifexpr) {
        return false;
    }
    if (auto pred = predicates.find(ifexpr->function); pred != predicates.end())
        return pred->second.test(*ifexpr, ctx);
    bug("Mob: #{} unknown predicate: {}", ctx.mob->mobIndex->vnum, ifexpr->function);
    return false;
}

std::string expand_var(const char c, const ExecutionCtx &ctx) {
    std::string buf{};
    switch (c) {
    case 'i': buf = ctx.mob->name; break;
    case 'I': buf = ctx.mob->describe(*ctx.mob); break;
    case 'n':
        if (ctx.actor)
            buf = ctx.actor->name;
        break;
    case 'N':
        if (ctx.actor)
            buf = ctx.mob->describe(*ctx.actor);
        break;
    case 't':
        if (ctx.act_targ_char)
            buf = ctx.act_targ_char->name;
        break;
    case 'T':
        if (ctx.act_targ_char)
            buf = ctx.mob->describe(*ctx.act_targ_char);
        break;
    case 'r':
        if (ctx.random)
            buf = ctx.random->name;
        break;
    case 'R':
        if (ctx.random)
            buf = ctx.mob->describe(*ctx.random);
        break;
    case 'e':
        if (ctx.actor)
            buf = subjective(*ctx.actor);
        break;
    case 'm':
        if (ctx.actor)
            buf = objective(*ctx.actor);
        break;
    case 's':
        if (ctx.actor)
            buf = possessive(*ctx.actor);
        break;
    case 'E':
        if (ctx.act_targ_char)
            buf = subjective(*ctx.act_targ_char);
        break;
    case 'M':
        if (ctx.act_targ_char)
            buf = objective(*ctx.act_targ_char);
        break;
    case 'S':
        if (ctx.act_targ_char)
            buf = possessive(*ctx.act_targ_char);
        break;
    case 'j': buf = subjective(*ctx.mob); break;
    case 'k': buf = objective(*ctx.mob); break;
    case 'l': buf = possessive(*ctx.mob); break;
    case 'J':
        if (ctx.random)
            buf = subjective(*ctx.random);
        break;
    case 'K':
        if (ctx.random)
            buf = objective(*ctx.random);
        break;
    case 'L':
        if (ctx.random)
            buf = possessive(*ctx.random);
        break;
    case 'o':
        if (ctx.obj)
            buf = ctx.mob->can_see(*ctx.obj) ? ctx.obj->name : "something";
        break;
    case 'O':
        if (ctx.obj)
            buf = ctx.mob->can_see(*ctx.obj) ? ctx.obj->short_descr : "something";
        break;
    case 'p':
        if (ctx.act_targ_obj)
            buf = ctx.mob->can_see(*ctx.act_targ_obj) ? ctx.act_targ_obj->name : "something";
        break;
    case 'P':
        if (ctx.act_targ_obj)
            buf = ctx.mob->can_see(*ctx.act_targ_obj) ? ctx.act_targ_obj->short_descr : "something";
        break;
    case 'a':
        if (ctx.obj)
            switch (ctx.obj->name.front()) {
            case 'a':
            case 'e':
            case 'i':
            case 'o':
            case 'u': buf = "an"; break;
            default: buf = "a";
            }
        break;
    case 'A':
        if (ctx.act_targ_obj)
            switch (ctx.act_targ_obj->name.front()) {
            case 'a':
            case 'e':
            case 'i':
            case 'o':
            case 'u': buf = "an"; break;
            default: buf = "a";
            }
        break;
    case '$': buf = "$"; break;
    default: bug("Mob: {} bad $var", ctx.mob->mobIndex->vnum); break;
    }
    return buf;
}

void interpret_command(std::string_view cmnd, const ExecutionCtx &ctx) {
    std::string buf{};
    bool prev_was_dollar = false;
    for (auto c : cmnd) {
        if (!std::exchange(prev_was_dollar, false)) {
            if (c != '$') {
                prev_was_dollar = false;
                buf.push_back(c);
            } else {
                prev_was_dollar = true;
            }
            continue;
        }
        buf += expand_var(c, ctx);
    }
    ::interpret(ctx.mob, buf);
}

// Makes a new ArgParser from an iterator over a string container
// using the string's string_view to avoid a copy.
auto parse_string_view(auto line_it) {
    std::string_view line{*line_it};
    return ArgParser{line};
}

void process_block(ExecutionCtx &ctx, auto &line_it, auto &end_it) {
    const auto expect_next_line = [&]() -> std::optional<ArgParser> {
        if (line_it == end_it) {
            return std::nullopt;
        }
        return parse_string_view(line_it);
    };
    while (auto args = expect_next_line()) {
        auto &current = ctx.frames.top();
        auto command = args->shift();
        if (matches(command, "endif")) {
            ctx.frames.pop();
        } else if (matches(command, "else")) {
            if (current.executable) {
                current.executing = !current.executing;
            }
        } else if (matches(command, "if")) {
            const auto exec_next = current.executing && evaluate_if(args->remaining(), ctx);
            ctx.frames.push({current.executing, exec_next});
            line_it++;
            process_block(ctx, line_it, end_it);
            continue;
        } else if (current.executing) {
            interpret_command(*line_it, ctx);
        }
        line_it++;
    }
}

Char *random_mortal_in_room(Char *mob, Rng &rng) {
    auto count = 0;
    for (auto *vch : mob->in_room->people) {
        if (vch->is_pc() && vch->level < LEVEL_IMMORTAL && mob->can_see(*vch)) {
            if (rng.number_range(0, count++) == 0)
                return vch;
        }
    }
    return nullptr;
}

void mprog_driver(Char *mob, const Program &prog, const Char *actor, const Object *obj, const Target target, Rng &rng) {
    if (mob->is_aff_charm())
        return;
    const auto *rndm = impl::random_mortal_in_room(mob, rng);
    const auto *act_targ_ch = std::holds_alternative<const Char *>(target) ? std::get<const Char *>(target) : nullptr;
    const auto *act_targ_obj =
        std::holds_alternative<const Object *>(target) ? std::get<const Object *>(target) : nullptr;

    ExecutionCtx ctx{rng, mob, actor, rndm, act_targ_ch, obj, act_targ_obj};
    auto line_it = prog.lines.begin();
    auto end_it = prog.lines.end();
    process_block(ctx, line_it, end_it);
    if (ctx.frames.size() > 1) {
        bug("Mob: {} possibly unbalanced if/else/endif", ctx.mob->mobIndex->vnum);
    }
}

std::string_view type_to_name(const TypeFlag type) {
    switch (type) {
    case TypeFlag::InFile: return "in_file_prog";
    case TypeFlag::Act: return "act_prog";
    case TypeFlag::Speech: return "speech_prog";
    case TypeFlag::Random: return "rand_prog";
    case TypeFlag::Fight: return "fight_prog";
    case TypeFlag::HitPercent: return "hitprcnt_prog";
    case TypeFlag::Death: return "death_prog";
    case TypeFlag::Entry: return "entry_prog";
    case TypeFlag::Greet: return "greet_prog";
    case TypeFlag::AllGreet: return "all_greet_prog";
    case TypeFlag::Give: return "give_prog";
    case TypeFlag::Bribe: return "bribe_prog";
    default: return "ERROR_PROG";
    }
}

// Parses an if expression taking these forms (the if has already been processed)
//   func_name(func_arg)
//   func_name(func_arg) operator operand
// Returns an empty optional if it couldn't be parsed.
std::optional<IfExpr> IfExpr::parse_if(std::string_view text) {
    auto orig = text;
    text = trim(text);
    if (text.empty()) {
        return std::nullopt;
    }
    auto open_paren = text.find_first_of("(");
    if (open_paren == std::string_view::npos) {
        bug("parse_if: missing '(': {}", orig);
        return std::nullopt;
    }
    auto function = trim(text.substr(0, open_paren));
    if (function.empty()) {
        bug("parse_if: missing function name: {}", orig);
        return std::nullopt;
    }
    text = ltrim(text.substr(open_paren + 1));
    auto close_paren = text.find_first_of(")");
    if (close_paren == std::string_view::npos) {
        bug("parse_if: missing ')': {}", orig);
        return std::nullopt;
    }
    auto arg = trim(text.substr(0, close_paren));
    if (arg.empty()) {
        bug("parse_if: missing argument: {}", orig);
        return std::nullopt;
    }
    text = ltrim(text.substr(close_paren + 1));
    if (text.empty()) {
        return IfExpr{function, arg, "", ""};
    }
    auto after_op = text.find_first_of(" \t");
    if (after_op == std::string_view::npos) {
        bug("parse_if: operator missing operand: {}", orig);
        return std::nullopt;
    }
    auto op = trim(text.substr(0, after_op));
    text = ltrim(text.substr(after_op + 1));
    if (text.empty()) {
        bug("parse_if: operator missing operand: {}", orig);
        return std::nullopt;
    }
    if (is_number(text)) {
        const auto operand_num = parse_number(text);
        return IfExpr{function, arg, op, operand_num};
    } else {
        return IfExpr{function, arg, op, text};
    }
}

std::optional<int> to_operand_num(std::string_view text) {
    if (is_number(text)) {
        return parse_number(text);
    } else
        return std::nullopt;
}

void exec_with_chance(Char *mob, Char *actor, Object *obj, const Target target, const TypeFlag type, Rng &rng) {
    for (const auto &mprg : mob->mobIndex->mobprogs) {
        if (mprg.type == type) {
            if (!is_number(mprg.arglist)) {
                bug("exec_with_chance in mob #{} expected number argument: {}", mob->mobIndex->vnum, mprg.arglist);
                continue;
            }
            if (rng.number_percent() < parse_number(mprg.arglist)) {
                mprog_driver(mob, mprg, actor, obj, target, rng);
                if (type != TypeFlag::Greet && type != TypeFlag::AllGreet)
                    break;
            }
        }
    }
}

} // namespace impl

} // namespace mprog
