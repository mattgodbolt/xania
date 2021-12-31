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
#include "Room.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "handler.hpp"
#include "interp.h"
#include "string_utils.hpp"

#include <optional>
#include <string>
#include <unordered_map>

using namespace std::literals;

namespace MProg {

// Mob Prog implementation internals.
namespace impl {

bool compare_strings(std::string_view lhs, std::string_view opr, std::string_view rhs) {
    if (opr == "=="sv)
        return matches(lhs, rhs);
    if (opr == "!="sv)
        return !matches(lhs, rhs);
    if (opr == "/"sv)
        return matches_inside(rhs, lhs);
    if (opr == "!/"sv)
        return !matches_inside(rhs, lhs);

    bug("Improper MOBprog operator '{}'", opr);
    return false;
}

bool compare_ints(const int lhs, std::string_view opr, const int rhs) {
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
    bool test(const IfExpr ifexpr, Char *mob, const Char *actor, const Object *obj, const Char *targ_ch,
              const Object *targ_obj, const Char *rndm) const {
        return validate(ifexpr, mob) && condition(ifexpr, mob, actor, obj, targ_ch, targ_obj, rndm);
    }
    typename std::conditional<std::is_function<V>::value, typename std::add_pointer<V>::type, V>::type validate;
    typename std::conditional<std::is_function<Cond>::value, typename std::add_pointer<Cond>::type, Cond>::type
        condition;
};

const Char *select_char(const char letter, const Char *mob, const Char *actor, const Char *targ_ch, const Char *rndm) {
    switch (letter) {
    case 'i': return mob; // Self
    case 'n': return actor;
    case 't': return targ_ch;
    case 'r': return rndm;
    default: return nullptr;
    }
}

const Object *select_obj(const char letter, const Object *obj, const Object *targ_obj) {
    switch (letter) {
    case 'o': return obj;
    case 'p': return targ_obj;
    default: return nullptr;
    }
}

bool expect_number_arg(const IfExpr ifexpr, Char *mob) {
    if (!is_number(ifexpr.arg)) {
        bug("expect_number_arg: #{} bad argument to '{}': {}", mob->mobIndex->vnum, ifexpr.function, ifexpr.arg);
        return false;
    }
    return true;
};

bool expect_dollar_var(const IfExpr ifexpr, Char *mob) {
    if (ifexpr.arg.length() != 2 || ifexpr.arg[0] != '$' || !std::isalpha(ifexpr.arg[1])) {
        bug("expect_dollar_var: #{} function expects a $var specifying a character or object, got: {}",
            mob->mobIndex->vnum, ifexpr.arg);
        return false;
    }
    return true;
}

bool expect_dollar_var_and_number_operand(const IfExpr ifexpr, Char *mob) {
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

bool expect_dollar_var_and_sv_operand(const IfExpr ifexpr, Char *mob) {
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

bool rand(const IfExpr ifexpr, [[maybe_unused]] Char *mob, [[maybe_unused]] const Char *actor,
          [[maybe_unused]] const Object *obj, [[maybe_unused]] const Char *targ_ch,
          [[maybe_unused]] const Object *targ_obj, [[maybe_unused]] const Char *rndm) {
    return number_percent() <= parse_number(ifexpr.arg);
};

bool ispc(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj, const Char *targ_ch,
          [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && ch->is_pc();
}

bool isnpc(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj, const Char *targ_ch,
           [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && ch->is_npc();
}

bool isgood(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj, const Char *targ_ch,
            [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && ch->is_good();
}

bool isfight(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj, const Char *targ_ch,
             [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && ch->fighting;
}

bool isimmort(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj,
              const Char *targ_ch, [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && ch->get_trust() > LEVEL_IMMORTAL;
}

bool ischarmed(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj,
               const Char *targ_ch, [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && ch->is_aff_charm();
}

bool isfollow(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj,
              const Char *targ_ch, [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && ch->master && ch->master->in_room == ch->in_room;
}

bool isaffected(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj,
                const Char *targ_ch, [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && ch->affected_by & std::get<const int>(ifexpr.operand);
}

bool hitprcnt(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj,
              const Char *targ_ch, [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && compare_ints(ch->hit / ch->max_hit, ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool inroom(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj, const Char *targ_ch,
            [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && compare_ints(ch->in_room->vnum, ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool sex(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj, const Char *targ_ch,
         [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && compare_ints(ch->sex.integer(), ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool position(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj,
              const Char *targ_ch, [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && compare_ints(ch->position.integer(), ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool level(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj, const Char *targ_ch,
           [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && compare_ints(ch->level, ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool class_(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj, const Char *targ_ch,
            [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && compare_ints(ch->class_num, ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool goldamt(const IfExpr ifexpr, Char *mob, const Char *actor, [[maybe_unused]] const Object *obj, const Char *targ_ch,
             [[maybe_unused]] const Object *targ_obj, const Char *rndm) {
    const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm);
    return ch && compare_ints(ch->gold, ifexpr.op, std::get<const int>(ifexpr.operand));
}

bool objtype(const IfExpr ifexpr, [[maybe_unused]] Char *mob, [[maybe_unused]] const Char *actor, const Object *obj,
             [[maybe_unused]] const Char *targ_ch, const Object *targ_obj, [[maybe_unused]] const Char *rndm) {
    const auto *sobj = select_obj(ifexpr.arg[1], obj, targ_obj);
    return sobj
           && compare_ints(magic_enum::enum_integer<ObjectType>(sobj->type), ifexpr.op,
                           std::get<const int>(ifexpr.operand));
}

bool name(const IfExpr ifexpr, Char *mob, [[maybe_unused]] const Char *actor, const Object *obj,
          [[maybe_unused]] const Char *targ_ch, const Object *targ_obj, [[maybe_unused]] const Char *rndm) {

    if (const auto *ch = select_char(ifexpr.arg[1], mob, actor, targ_ch, rndm)) {
        return compare_strings(ch->name, ifexpr.op, std::get<std::string_view>(ifexpr.operand));
    }
    if (const auto *sobj = select_obj(ifexpr.arg[1], obj, targ_obj)) {
        return compare_strings(sobj->name, ifexpr.op, std::get<std::string_view>(ifexpr.operand));
    }
    bug("Mob: #{} Unrecognized char or obj variable: {}", mob->mobIndex->vnum, ifexpr.arg);
    return false;
}

using Validator = decltype(expect_number_arg);
using Cond = decltype(rand);

// Weave together the mob prog predicate functions with their names, along with
// the validators required by each.
std::unordered_map<std::string_view, Predicate<Validator, Cond>> predicates({
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

bool evaluate_if(std::string_view ifchck, Char *mob, const Char *actor, const Object *obj, const Target target,
                 const Char *rndm) {
    const auto *targ_ch = std::holds_alternative<const Char *>(target) ? std::get<const Char *>(target) : nullptr;
    const auto *targ_obj = std::holds_alternative<const Object *>(target) ? std::get<const Object *>(target) : nullptr;
    const auto ifexpr = IfExpr::parse_if(ifchck);
    if (!ifexpr) {
        return false;
    }
    if (auto pred = predicates.find(ifexpr->function); pred != predicates.end())
        return pred->second.test(*ifexpr, mob, actor, obj, targ_ch, targ_obj, rndm);
    bug("Mob: #{} unknown predicate: {}", mob->mobIndex->vnum, ifexpr->function);
    return false;
}

std::string expand_var(const char c, const Char *mob, const Char *actor, const Object *obj, const Target target,
                       const Char *rndm) {
    const auto *targ_ch = std::holds_alternative<const Char *>(target) ? *std::get_if<const Char *>(&target) : nullptr;
    const auto *targ_obj =
        std::holds_alternative<const Object *>(target) ? *std::get_if<const Object *>(&target) : nullptr;
    std::string buf{};
    switch (c) {
    case 'i':
    case 'I': buf = mob->describe(*mob); break;
    case 'n':
    case 'N':
        if (actor)
            buf = mob->describe(*actor);
        break;
    case 't':
    case 'T':
        if (targ_ch)
            buf = mob->describe(*targ_ch);
        break;
    case 'r':
    case 'R':
        if (rndm)
            buf = mob->describe(*rndm);
        break;
    case 'e':
        if (actor)
            buf = subjective(*actor);
        break;
    case 'm':
        if (actor)
            buf = objective(*actor);
        break;
    case 's':
        if (actor)
            buf = possessive(*actor);
        break;
    case 'E':
        if (targ_ch)
            buf = subjective(*targ_ch);
        break;
    case 'M':
        if (targ_ch)
            buf = objective(*targ_ch);
        break;
    case 'S':
        if (targ_ch)
            buf = possessive(*targ_ch);
        break;
    case 'j': buf = subjective(*mob); break;
    case 'k': buf = objective(*mob); break;
    case 'l': buf = possessive(*mob); break;
    case 'J':
        if (rndm)
            buf = subjective(*rndm);
        break;
    case 'K':
        if (rndm)
            buf = objective(*rndm);
        break;
    case 'L':
        if (rndm)
            buf = possessive(*rndm);
        break;
    case 'o':
        if (obj)
            buf = can_see_obj(mob, obj) ? obj->name : "something";
        break;
    case 'O':
        if (obj)
            buf = can_see_obj(mob, obj) ? obj->short_descr : "something";
        break;
    case 'p':
        if (targ_obj)
            buf = can_see_obj(mob, targ_obj) ? targ_obj->name : "something";
        break;
    case 'P':
        if (targ_obj)
            buf = can_see_obj(mob, targ_obj) ? targ_obj->short_descr : "something";
        break;
    case 'a':
        if (obj)
            switch (obj->name.front()) {
            case 'a':
            case 'e':
            case 'i':
            case 'o':
            case 'u': buf = "an"; break;
            default: buf = "a";
            }
        break;
    case 'A':
        if (targ_obj)
            switch (targ_obj->name.front()) {
            case 'a':
            case 'e':
            case 'i':
            case 'o':
            case 'u': buf = "an"; break;
            default: buf = "a";
            }
        break;
    case '$': buf = "$"; break;
    default: bug("Mob: {} bad $var", mob->mobIndex->vnum); break;
    }
    return buf;
}

void interpret_command(std::string_view cmnd, Char *mob, const Char *actor, const Object *obj, const Target target,
                       const Char *rndm) {
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
        buf += expand_var(c, mob, actor, obj, target, rndm);
    }
    ::interpret(mob, buf);
}

// Makes a new ArgParser from an iterator over a string container
// using the string's string_view to avoid a copy.
auto parse_string_view(auto line_it) {
    std::string_view line{*line_it};
    return ArgParser{line};
}

void process_if_block(std::string_view ifchck, Char *mob, const Char *actor, const Object *obj, const Target target,
                      const Char *rndm, auto &line_it, auto &end_it) {
    const auto expect_next_line = [&](const auto where, const auto more_expected) -> std::optional<ArgParser> {
        if (++line_it == end_it) {
            if (more_expected)
                bug("Mob: #{} Unexpected EOF in {}", mob->mobIndex->vnum, where);
            return std::nullopt;
        }
        return parse_string_view(line_it);
    };
    const auto process_block = [&](const auto where, const auto skip_flag) {
        auto skipping = skip_flag;
        while (auto opt_args = expect_next_line(where, skipping)) {
            auto cmnd = opt_args->shift();
            if (matches(cmnd, "endif")) {
                return;
            } else if (matches(cmnd, "else")) {
                skipping = !skipping;
            } else if (!skipping) {
                if (matches(cmnd, "if")) {
                    // Recurse into the next if block.
                    process_if_block(opt_args->remaining(), mob, actor, obj, target, rndm, line_it, end_it);
                } else {
                    interpret_command(*line_it, mob, actor, obj, target, rndm);
                }
            }
        }
    };
    if (evaluate_if(ifchck, mob, actor, obj, target, rndm)) {
        process_block("if pass block", false);
    } else {
        process_block("if fail block", true);
    }
}

Char *random_mortal_in_room(Char *mob) {
    auto count = 0;
    for (auto *vch : mob->in_room->people) {
        if (vch->is_pc() && vch->level < LEVEL_IMMORTAL && can_see(mob, vch)) {
            if (number_range(0, count++) == 0)
                return vch;
        }
    }
    return nullptr;
}

void mprog_driver(Char *mob, const Program &prog, const Char *actor, const Object *obj, const Target target) {
    if (mob->is_aff_charm())
        return;
    const auto *rndm = impl::random_mortal_in_room(mob);
    auto line_it = prog.lines.begin();
    auto end_it = prog.lines.end();
    // All mobprog scripts are expected to have at least 1 line.
    while (true) {
        ArgParser args = parse_string_view(line_it);
        auto command = args.shift();
        if (matches(command, "if")) {
            process_if_block(args.remaining(), mob, actor, obj, target, rndm, line_it, end_it);
        } else {
            interpret_command(*line_it, mob, actor, obj, target, rndm);
        }
        // If process_if_block() may have reached the end of input.
        // As a precaution, check for end of input before accessing the next line too.
        if (line_it == end_it || ++line_it == end_it)
            break;
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

void exec_with_chance(Char *mob, Char *actor, Object *obj, const Target target, const TypeFlag type) {
    for (const auto &mprg : mob->mobIndex->mobprogs) {
        if (mprg.type == type) {
            if (!is_number(mprg.arglist)) {
                bug("exec_with_chance in mob #{} expected number argument: {}", mob->mobIndex->vnum, mprg.arglist);
                continue;
            }
            if (number_percent() < parse_number(mprg.arglist)) {
                mprog_driver(mob, mprg, actor, obj, target);
                if (type != TypeFlag::Greet && type != TypeFlag::AllGreet)
                    break;
            }
        }
    }
}

} // namespace impl

} // namespace mprog
