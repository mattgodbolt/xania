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

#include <string>

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
bool evaluate_if(std::string_view ifchck, Char *mob, const Char *actor, const Object *obj, const Target target,
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

void process_if_block(std::string_view ifchck, Char *mob, const Char *actor, const Object *obj, const Target target,
                      const Char *rndm, auto &line_it, auto &end_it) {
    const auto expect_next_line = [&line_it, &end_it, &mob](const auto where) {
        if (++line_it == end_it) {
            bug("Mob: #{} Unexpected EOF in {}", mob->mobIndex->vnum, where);
            return false;
        }
        return true;
    };
    if (evaluate_if(ifchck, mob, actor, obj, target, rndm)) {
        while (true) { // ifchck was true, do commands but ignore else to endif
            if (!expect_next_line("if success exec block"))
                return;
            ArgParser args{*line_it};
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
                    if (!expect_next_line("if success skip block"))
                        return;
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
            if (!expect_next_line("if failure skip block"))
                return;
            ArgParser args{*line_it};
            auto cmnd = args.shift();
            if (matches(cmnd, "endif"))
                return;
            if (matches(cmnd, "else"))
                break;
        }
        // Perform the remaining commands up until the next endif
        while (true) {
            if (!expect_next_line("if failure exec block"))
                return;
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

void mprog_driver(Char *mob, const Program &prog, const Char *actor, const Object *obj, const Target target) {
    if (mob->is_aff_charm())
        return;
    const auto *rndm = impl::random_mortal_in_room(mob);
    auto line_it = prog.lines.begin();
    auto end_it = prog.lines.end();
    // All mobprog scripts are expected to have at least 1 line.
    while (true) {
        ArgParser args{*line_it};
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
    return IfExpr{function, arg, op, text};
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
