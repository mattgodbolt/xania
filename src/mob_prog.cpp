/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
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

#include "mob_prog.hpp"
#include "AffectFlag.hpp"
#include "ArgParser.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
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

#include <fmt/format.h>

#include <string>
#include <vector>

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

/* This function performs the evaluation of the if checks.  It is
 * here that you can add any ifchecks which you so desire. Hopefully
 * it is clear from what follows how one would go about adding your
 * own. The syntax for an if check is: ifchck ( arg ) [opr val]
 * where the parenthesis are required and the opr and val fields are
 * optional but if one is there then both must be. The spaces are all
 * optional. The evaluation of the opr expressions is farmed out
 * to reduce the redundancy of the mammoth if statement list.
 */
bool evaluate_if(std::string_view ifchck, Char *mob, const Char *actor, const Object *obj, const MProg::Target target,
                 const Char *rndm) {
    const auto *targ_ch = std::holds_alternative<const Char *>(target) ? *std::get_if<const Char *>(&target) : nullptr;
    const auto *targ_obj =
        std::holds_alternative<const Object *>(target) ? *std::get_if<const Object *>(&target) : nullptr;
    const auto opt_ifexpr = IfExpr::parse_if(ifchck);
    if (!opt_ifexpr) {
        return false;
    }
    const auto ifexpr = *opt_ifexpr;
    if (matches(ifexpr.function, "rand")) {
        if (!is_number(ifexpr.arg)) {
            bug("Mob: {} bad argument to 'rand': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        return number_percent() <= parse_number(ifexpr.arg);
    }
    if (matches(ifexpr.function, "ispc")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'ispc': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        switch (ifexpr.arg[1]) { // arg should be "$*" so just get the letter
        case 'i': return false; // Self
        case 'n': return actor && actor->is_pc();
        case 't': return targ_ch && targ_ch->is_pc();
        case 'r': return rndm && rndm->is_pc();
        default: bug("Mob: {} bad argument to 'ispc': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "isnpc")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'isnpc': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        switch (ifexpr.arg[1]) {
        case 'i': return true; // Self
        case 'n': return actor && actor->is_npc();
        case 't': return targ_ch && targ_ch->is_npc();
        case 'r': return rndm && rndm->is_npc();
        default: bug("Mob: {} bad argument to 'isnpc': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "isgood")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'isgood': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        switch (ifexpr.arg[1]) {
        case 'i': return mob->is_good();
        case 'n': return actor && actor->is_good();
        case 't': return targ_ch && targ_ch->is_good();
        case 'r': return rndm && rndm->is_good();
        default: bug("Mob: {} bad argument to 'isgood': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "isfight")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'isfight': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        switch (ifexpr.arg[1]) {
        case 'i': return mob->fighting;
        case 'n': return actor && actor->fighting;
        case 't': return targ_ch && targ_ch->fighting;
        case 'r': return rndm && rndm->fighting;
        default: bug("Mob: {} bad argument to 'isfight': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "isimmort")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'isimmort': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        switch (ifexpr.arg[1]) {
        case 'i': return mob->get_trust() > LEVEL_IMMORTAL;
        case 'n': return actor && actor->get_trust() > LEVEL_IMMORTAL;
        case 't': return targ_ch && targ_ch->get_trust() > LEVEL_IMMORTAL;
        case 'r': return rndm && rndm->get_trust() > LEVEL_IMMORTAL;
        default: bug("Mob: {} bad argument to 'isimmort': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "ischarmed")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'ischarmed': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        switch (ifexpr.arg[1]) {
        case 'i': return mob->is_aff_charm();
        case 'n': return actor && actor->is_aff_charm();
        case 't': return targ_ch && targ_ch->is_aff_charm();
        case 'r': return rndm && rndm->is_aff_charm();
        default: bug("Mob: {} bad argument to 'ischarmed': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "isfollow")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'isfollow': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        switch (ifexpr.arg[1]) {
        case 'i': return mob->master && mob->master->in_room == mob->in_room;
        case 'n': return actor && actor->master && actor->master->in_room == actor->in_room;
        case 't': return targ_ch && targ_ch->master && targ_ch->master->in_room == targ_ch->in_room;
        case 'r': return rndm && rndm->master && rndm->master->in_room == rndm->in_room;
        default: bug("Mob: {} bad argument to 'isfollow': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "isaffected")) {
        if (!is_number(ifexpr.arg)) {
            bug("Mob: {} bad argument to 'isaffected': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        const auto affect_bit = parse_number(ifexpr.arg);
        switch (ifexpr.arg[1]) {
        case 'i': return mob->affected_by & affect_bit;
        case 'n': return actor && actor->affected_by & affect_bit;
        case 't': return targ_ch && targ_ch->affected_by & affect_bit;
        case 'r': return rndm && rndm->affected_by & affect_bit;
        default: bug("Mob: {} bad argument to 'isaffected': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "hitprcnt")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'hitprcnt': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        if (!is_number(ifexpr.operand)) {
            bug("Mob: {} bad operand to 'hitprcnt': {}", mob->mobIndex->vnum, ifexpr.operand);
            return false;
        }
        const auto percent = parse_number(ifexpr.operand);
        switch (ifexpr.arg[1]) {
        case 'i': return compare_ints(mob->hit / mob->max_hit, ifexpr.op, percent);
        case 'n': return actor && compare_ints(actor->hit / actor->max_hit, ifexpr.op, percent);
        case 't': return targ_ch && compare_ints(targ_ch->hit / targ_ch->max_hit, ifexpr.op, percent);
        case 'r': return rndm && compare_ints(rndm->hit / rndm->max_hit, ifexpr.op, percent);
        default: bug("Mob: {} bad argument to 'hitprcnt': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "inroom")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'inroom': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        if (!is_number(ifexpr.operand)) {
            bug("Mob: {} bad operand to 'inroom': {}", mob->mobIndex->vnum, ifexpr.operand);
            return false;
        }
        const auto room_vnum = parse_number(ifexpr.operand);
        switch (ifexpr.arg[1]) {
        case 'i': return compare_ints(mob->in_room->vnum, ifexpr.op, room_vnum);
        case 'n': return actor && compare_ints(actor->in_room->vnum, ifexpr.op, room_vnum);
        case 't': return targ_ch && compare_ints(targ_ch->in_room->vnum, ifexpr.op, room_vnum);
        case 'r': return rndm && compare_ints(rndm->in_room->vnum, ifexpr.op, room_vnum);
        default: bug("Mob: {} bad argument to 'inroom': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "sex")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'sex': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        if (!is_number(ifexpr.operand)) {
            bug("Mob: {} bad operand to 'sex': {}", mob->mobIndex->vnum, ifexpr.operand);
            return false;
        }
        const auto sex = parse_number(ifexpr.operand);
        switch (ifexpr.arg[1]) {
        case 'i': return compare_ints(mob->sex.integer(), ifexpr.op, sex);
        case 'n': return actor && compare_ints(actor->sex.integer(), ifexpr.op, sex);
        case 't': return targ_ch && compare_ints(targ_ch->sex.integer(), ifexpr.op, sex);
        case 'r': return rndm && compare_ints(rndm->sex.integer(), ifexpr.op, sex);
        default: bug("Mob: {} bad argument to 'sex': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "position")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'position': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        if (!is_number(ifexpr.operand)) {
            bug("Mob: {} bad operand to 'position': {}", mob->mobIndex->vnum, ifexpr.operand);
            return false;
        }
        const auto position = parse_number(ifexpr.operand);
        switch (ifexpr.arg[1]) {
        case 'i': return compare_ints(mob->position.integer(), ifexpr.op, position);
        case 'n': return actor && compare_ints(actor->position.integer(), ifexpr.op, position);
        case 't': return targ_ch && compare_ints(targ_ch->position.integer(), ifexpr.op, position);
        case 'r': return rndm && compare_ints(rndm->position.integer(), ifexpr.op, position);
        default: bug("Mob: {} bad argument to 'position': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "level")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'level': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        if (!is_number(ifexpr.operand)) {
            bug("Mob: {} bad operand to 'level': {}", mob->mobIndex->vnum, ifexpr.operand);
            return false;
        }
        const auto level = parse_number(ifexpr.operand);
        switch (ifexpr.arg[1]) {
        case 'i': return compare_ints(mob->get_trust(), ifexpr.op, level);
        case 'n': return actor && compare_ints(actor->get_trust(), ifexpr.op, level);
        case 't': return targ_ch && compare_ints(targ_ch->get_trust(), ifexpr.op, level);
        case 'r': return rndm && compare_ints(rndm->get_trust(), ifexpr.op, level);
        default: bug("Mob: {} bad argument to 'level': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "class")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'class': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        if (!is_number(ifexpr.operand)) {
            bug("Mob: {} bad operand to 'class': {}", mob->mobIndex->vnum, ifexpr.operand);
            return false;
        }
        const auto class_num = parse_number(ifexpr.operand);
        switch (ifexpr.arg[1]) {
        case 'i': return compare_ints(mob->class_num, ifexpr.op, class_num);
        case 'n': return actor && compare_ints(actor->class_num, ifexpr.op, class_num);
        case 't': return targ_ch && compare_ints(targ_ch->class_num, ifexpr.op, class_num);
        case 'r': return rndm && compare_ints(rndm->class_num, ifexpr.op, class_num);
        default: bug("Mob: {} bad argument to 'class': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "goldamt")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'goldamt': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        if (!is_number(ifexpr.operand)) {
            bug("Mob: {} bad operand to 'goldamt': {}", mob->mobIndex->vnum, ifexpr.operand);
            return false;
        }
        const auto gold = parse_number(ifexpr.operand);
        switch (ifexpr.arg[1]) {
        case 'i': return compare_ints(mob->gold, ifexpr.op, gold);
        case 'n': return actor && compare_ints(actor->gold, ifexpr.op, gold);
        case 't': return targ_ch && compare_ints(targ_ch->gold, ifexpr.op, gold);
        case 'r': return rndm && compare_ints(rndm->gold, ifexpr.op, gold);
        default: bug("Mob: {} bad argument to 'goldamt': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "objtype")) {
        if (ifexpr.arg.length() != 2) {
            bug("Mob: {} bad argument to 'objtype': {}", mob->mobIndex->vnum, ifexpr.arg);
            return false;
        }
        if (!is_number(ifexpr.operand)) {
            bug("Mob: {} bad operand to 'objtype': {}", mob->mobIndex->vnum, ifexpr.operand);
            return false;
        }
        const auto obj_type = parse_number(ifexpr.operand);
        switch (ifexpr.arg[1]) {
        case 'o': return obj && compare_ints(magic_enum::enum_integer<ObjectType>(obj->type), ifexpr.op, obj_type);
        case 'p':
            return targ_obj && compare_ints(magic_enum::enum_integer<ObjectType>(targ_obj->type), ifexpr.op, obj_type);
        default: bug("Mob: {} bad argument to 'objtype': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "name")) {
        switch (ifexpr.arg[1]) {
        case 'i': return compare_strings(mob->name, ifexpr.op, ifexpr.operand);
        case 'n': return actor && compare_strings(actor->name, ifexpr.op, ifexpr.operand);
        case 't': return targ_ch && compare_strings(targ_ch->name, ifexpr.op, ifexpr.operand);
        case 'r': return rndm && compare_strings(rndm->name, ifexpr.op, ifexpr.operand);
        case 'o': return obj && compare_strings(obj->name, ifexpr.op, ifexpr.operand);
        case 'p': return targ_obj && compare_strings(targ_obj->name, ifexpr.op, ifexpr.operand);
        default: bug("Mob: {} bad argument to 'name': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    bug("Mob: {} unknown ifchck: {}", mob->mobIndex->vnum, ifexpr.function);
    return false;
}

std::string expand_var(const char c, const Char *mob, const Char *actor, const Object *obj, const MProg::Target target,
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

void interpret_command(std::string_view cmnd, Char *mob, const Char *actor, const Object *obj,
                       const MProg::Target target, const Char *rndm) {
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

void process_if_block(std::string_view ifchck, Char *mob, const Char *actor, const Object *obj,
                      const MProg::Target target, const Char *rndm, auto &line_it, auto end_it) {
    if (evaluate_if(ifchck, mob, actor, obj, target, rndm)) {
        while (true) { // ifchck was true, do commands but ignore else to endif
            if (++line_it == end_it) {
                bug("Mob: {} unexpected end of input in if block", mob->mobIndex->vnum);
                return;
            }
            ArgParser args{*line_it};
            if (args.empty()) {
                bug("Mob: {} missing else or endif", mob->mobIndex->vnum);
                return;
            }
            auto cmnd = args.shift();
            if (matches(cmnd, "if")) {
                process_if_block(args.remaining(), mob, actor, obj, target, rndm, line_it, end_it);
                continue;
            }
            if (matches(cmnd, "endif"))
                return;
            if (matches(cmnd, "else")) {
                // Skip over the commands in this else block.
                for (;;) {
                    if (++line_it == end_it) {
                        bug("Mob: {} unexpected end of input in else block", mob->mobIndex->vnum);
                        return;
                    }
                    ArgParser args{*line_it};
                    auto cmnd = args.shift();
                    if (matches(cmnd, "endif"))
                        return;
                }
            }
            interpret_command(*line_it, mob, actor, obj, target, rndm);
        }
    } else {
        // Skip over the commands in the block until an endif.
        while (true) {
            if (++line_it == end_it) {
                bug("Mob: {} unexpected end of input in if block", mob->mobIndex->vnum);
                return;
            }
            ArgParser args{*line_it};
            auto cmnd = args.shift();
            if (matches(cmnd, "endif"))
                return;
            if (matches(cmnd, "else"))
                break;
        }
        // Perform the remaining commands up until the next endif
        while (true) {
            if (++line_it == end_it) {
                bug("Mob: {} unexpected end of input in else block", mob->mobIndex->vnum);
                return;
            }
            ArgParser args{*line_it};
            auto cmnd = args.shift();
            if (matches(cmnd, "endif"))
                return;
            if (matches(cmnd, "if")) {
                process_if_block(args.remaining(), mob, actor, obj, target, rndm, line_it, end_it);
                continue;
            }
            if (matches(cmnd, "else")) {
                bug("Mob: {} found else in an else section", mob->mobIndex->vnum);
                return;
            }
            if (matches(cmnd, "endif"))
                return;
            interpret_command(*line_it, mob, actor, obj, target, rndm);
        }
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

void mprog_driver(Char *mob, const Program &prog, const Char *actor, const Object *obj, const MProg::Target target) {
    if (mob->is_aff_charm())
        return;
    const auto *rndm = impl::random_mortal_in_room(mob);
    auto end_it = prog.lines.end();
    for (auto line_it = prog.lines.begin(); line_it != end_it; line_it++) {
        ArgParser args{*line_it};
        auto command = args.shift();
        if (matches(command, "if")) {
            impl::process_if_block(args.remaining(), mob, actor, obj, target, rndm, line_it, end_it);
        } else {
            impl::interpret_command(*line_it, mob, actor, obj, target, rndm);
        }
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
    return IfExpr{function, arg, op, text};
}

} // namespace impl

Target to_target(const Char *ch, const Object *obj) {
    if (ch)
        return MProg::Target{ch};
    else if (obj)
        return MProg::Target{obj};
    else
        return MProg::Target{nullptr};
}

/* The next two routines are the basic trigger types. Either trigger
 *  on a certain percent, or trigger on a keyword or word phrase.
 *  To see how this works, look at the various trigger routines..
 */
void wordlist_check(std::string_view arg, Char *mob, const Char *actor, const Object *obj, const Target target,
                    const TypeFlag type) {
    for (const auto &mprg : mob->mobIndex->mobprogs) {
        if (mprg.type == type) {
            // Player message matches the phrase in the program.
            if ((mprg.arglist[0] == 'p') && (std::isspace(mprg.arglist[1]))) {
                auto prog_phrase = mprg.arglist.substr(2, mprg.arglist.length());
                if (matches_inside(prog_phrase, arg)) {
                    impl::mprog_driver(mob, mprg, actor, obj, target);
                    break;
                }
            } else {
                // Is any keyword in the program trigger present anywhere in the character's acted message?
                auto prog_keywords = ArgParser(mprg.arglist);
                for (const auto &prog_keyword : prog_keywords) {
                    if (matches_inside(prog_keyword, arg)) {
                        impl::mprog_driver(mob, mprg, actor, obj, target);
                        break;
                    }
                }
            }
        }
    }
}

void percent_check(Char *mob, Char *actor, Object *obj, const Target target, const TypeFlag type) {
    for (const auto &mprg : mob->mobIndex->mobprogs) {
        if (mprg.type == type) {
            if (!is_number(mprg.arglist)) {
                bug("percent_check in mob #{} expected number argument: {}", mob->mobIndex->vnum, mprg.arglist);
                continue;
            }
            if (number_percent() < parse_number(mprg.arglist)) {
                impl::mprog_driver(mob, mprg, actor, obj, target);
                if (type != TypeFlag::Greet && type != TypeFlag::AllGreet)
                    break;
            }
        }
    }
}

void act_trigger(std::string_view buf, Char *mob, const Char *ch, const Object *obj, const MProg::Target target) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, TypeFlag::Act)) {
        mob->mpact.emplace_back(std::string(buf), ch, obj, target);
    }
}

void bribe_trigger(Char *mob, Char *ch, int amount) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, TypeFlag::Bribe)) {
        for (const auto &mprg : mob->mobIndex->mobprogs)
            if (mprg.type == TypeFlag::Bribe) {
                if (!is_number(mprg.arglist)) {
                    bug("bribe_trigger in mob #{} expected number argument: {}", mob->mobIndex->vnum, mprg.arglist);
                    continue;
                }
                if (amount >= parse_number(mprg.arglist)) {
                    /* this function previously created a gold object and gave it to ch
                       but there is zero point - the gold transfer is handled in do_give now */
                    impl::mprog_driver(mob, mprg, ch, nullptr, nullptr);
                    break;
                }
            }
    }
}

void death_trigger(Char *mob) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, TypeFlag::Death)) {
        percent_check(mob, nullptr, nullptr, nullptr, TypeFlag::Death);
    }
}

void entry_trigger(Char *mob) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, TypeFlag::Entry))
        percent_check(mob, nullptr, nullptr, nullptr, TypeFlag::Entry);
}

void fight_trigger(Char *mob, Char *ch) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, TypeFlag::Fight))
        percent_check(mob, ch, nullptr, nullptr, TypeFlag::Fight);
}

void give_trigger(Char *mob, Char *ch, Object *obj) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, TypeFlag::Give)) {
        for (const auto &mprg : mob->mobIndex->mobprogs) {
            if (mprg.type == TypeFlag::Give) {
                auto prog_args = ArgParser(mprg.arglist);
                const auto first_prog_arg = prog_args.shift();
                if (matches(obj->name, mprg.arglist) || matches("all", first_prog_arg)) {
                    impl::mprog_driver(mob, mprg, ch, obj, nullptr);
                    break;
                }
            }
        }
    }
}

void greet_trigger(Char *mob) {
    for (auto *vmob : mob->in_room->people)
        if (vmob->is_npc() && mob != vmob && can_see(vmob, mob) && (vmob->fighting == nullptr) && vmob->is_pos_awake()
            && check_enum_bit(vmob->mobIndex->progtypes, TypeFlag::Greet))
            percent_check(vmob, mob, nullptr, nullptr, TypeFlag::Greet);
        else if (vmob->is_npc() && (vmob->fighting == nullptr) && vmob->is_pos_awake()
                 && check_enum_bit(vmob->mobIndex->progtypes, TypeFlag::AllGreet))
            percent_check(vmob, mob, nullptr, nullptr, TypeFlag::AllGreet);
}

void hitprcnt_trigger(Char *mob, Char *ch) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, TypeFlag::HitPercent)) {
        for (const auto &mprg : mob->mobIndex->mobprogs) {
            if (mprg.type == TypeFlag::HitPercent) {
                if (!is_number(mprg.arglist)) {
                    bug("hitprcnt_trigger in mob #{} expected number argument: {}", mob->mobIndex->vnum, mprg.arglist);
                    continue;
                }
                if (100 * mob->hit / mob->max_hit < parse_number(mprg.arglist)) {
                    impl::mprog_driver(mob, mprg, ch, nullptr, nullptr);
                    break;
                }
            }
        }
    }
}

void random_trigger(Char *mob) {
    if (check_enum_bit(mob->mobIndex->progtypes, TypeFlag::Random))
        percent_check(mob, nullptr, nullptr, nullptr, TypeFlag::Random);
}

void speech_trigger(std::string_view txt, const Char *mob) {
    for (auto *vmob : mob->in_room->people)
        if (vmob->is_npc() && (check_enum_bit(vmob->mobIndex->progtypes, TypeFlag::Speech)))
            wordlist_check(txt, vmob, mob, nullptr, nullptr, TypeFlag::Speech);
}

void show_programs(Char *ch, ArgParser args) {
    Char *victim;
    const auto arg = args.shift();
    if (arg.empty()) {
        ch->send_line("Show the programs of whom?");
        return;
    }
    if ((victim = get_char_world(ch, arg)) == nullptr) {
        ch->send_line("They aren't here.");
        return;
    }
    if (victim->is_pc()) {
        ch->send_line("Only Mobiles can have Programs!");
        return;
    }
    if (!(victim->mobIndex->progtypes)) {
        ch->send_line("That Mobile has no Programs set.");
        return;
    }
    ch->send_line("Name: {}.  Vnum: {}.", victim->name, victim->mobIndex->vnum);

    ch->send_line(fmt::format("Short description:{}.\n\rLong  description: {}", victim->short_descr,
                              victim->long_descr.empty() ? "(none)." : victim->long_descr));

    ch->send_line("Hp: {}/{}.  Mana: {}/{}.  Move: {}/{}.", victim->hit, victim->max_hit, victim->mana,
                  victim->max_mana, victim->move, victim->max_move);

    ch->send_line("Lv: {}.  Class: {}.  Align: {}.   Gold: {}.  Exp: {}.", victim->level, victim->class_num,
                  victim->alignment, victim->gold, victim->exp);

    for (const auto &mprg : victim->mobIndex->mobprogs) {
        ch->send_line(">{} {}\n\r{}", impl::type_to_name(mprg.type), mprg.arglist, fmt::join(mprg.lines, "\n\r"));
    }
}

} // namespace mprog
