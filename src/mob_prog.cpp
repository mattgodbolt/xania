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

using namespace std::literals;

/*
 * Local function prototypes
 */

char *mprog_next_command(char *clist);
bool mprog_seval(std::string_view lhs, std::string_view opr, std::string_view rhs);
bool mprog_veval(const int lhs, std::string_view opr, const int rhs);
bool mprog_do_ifchck(std::string_view ifchck, Char *mob, const Char *actor, const Object *obj,
                     const mprog::Target target, Char *rndm);
char *mprog_process_if(std::string_view ifchck, char *com_list, Char *mob, const Char *actor, const Object *obj,
                       const mprog::Target target, Char *rndm);
std::string mprog_expand_var(const char c, const Char *mob, const Char *actor, const Object *obj,
                             const mprog::Target target, const Char *rndm);
void mprog_process_cmnd(std::string_view cmnd, Char *mob, const Char *actor, const Object *obj,
                        const mprog::Target target, Char *rndm);
void mprog_driver(Char *mob, const MobProg &prog, const Char *actor, const Object *obj, const mprog::Target target);

namespace mprog {

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

Target to_target(const Char *ch, const Object *obj) {
    if (ch)
        return mprog::Target{ch};
    else if (obj)
        return mprog::Target{obj};
    else
        return mprog::Target{nullptr};
}

}

/***************************************************************************
 * Local function code and brief comments.
 */

/* Used to get sequential lines of a multi line string (separated by "\n\r")
 * Thus its like one_argument(), but a trifle different. It is destructive
 * to the multi line string argument, and thus clist must not be shared.
 */
char *mprog_next_command(char *clist) {

    if (!clist) {
        return nullptr;
    }
    char *pointer = clist;

    while (*pointer != '\n' && *pointer != '\0')
        pointer++;
    if (*pointer == '\n')
        *pointer++ = '\0';
    if (*pointer == '\r')
        *pointer++ = '\0';

    return (pointer);
}

/* These two functions do the basic evaluation of ifcheck operators.
 *  It is important to note that the string operations are not what
 *  you probably expect.  Equality is exact and division is substring.
 *  remember that lhs has been stripped of leading space, but can
 *  still have trailing spaces so be careful when editing since:
 *  "guard" and "guard " are not equal.
 */
bool mprog_seval(std::string_view lhs, std::string_view opr, std::string_view rhs) {
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

bool mprog_veval(const int lhs, std::string_view opr, const int rhs) {
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
 * If there are errors, then return -1 otherwise return boolean 1,0
 */
bool mprog_do_ifchck(std::string_view ifchck, Char *mob, const Char *actor, const Object *obj,
                     const mprog::Target target, Char *rndm) {
    using namespace mprog;
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
        case 'i': return mprog_veval(mob->hit / mob->max_hit, ifexpr.op, percent);
        case 'n': return actor && mprog_veval(actor->hit / actor->max_hit, ifexpr.op, percent);
        case 't': return targ_ch && mprog_veval(targ_ch->hit / targ_ch->max_hit, ifexpr.op, percent);
        case 'r': return rndm && mprog_veval(rndm->hit / rndm->max_hit, ifexpr.op, percent);
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
        case 'i': return mprog_veval(mob->in_room->vnum, ifexpr.op, room_vnum);
        case 'n': return actor && mprog_veval(actor->in_room->vnum, ifexpr.op, room_vnum);
        case 't': return targ_ch && mprog_veval(targ_ch->in_room->vnum, ifexpr.op, room_vnum);
        case 'r': return rndm && mprog_veval(rndm->in_room->vnum, ifexpr.op, room_vnum);
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
        case 'i': return mprog_veval(mob->sex.integer(), ifexpr.op, sex);
        case 'n': return actor && mprog_veval(actor->sex.integer(), ifexpr.op, sex);
        case 't': return targ_ch && mprog_veval(targ_ch->sex.integer(), ifexpr.op, sex);
        case 'r': return rndm && mprog_veval(rndm->sex.integer(), ifexpr.op, sex);
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
        case 'i': return mprog_veval(mob->position.integer(), ifexpr.op, position);
        case 'n': return actor && mprog_veval(actor->position.integer(), ifexpr.op, position);
        case 't': return targ_ch && mprog_veval(targ_ch->position.integer(), ifexpr.op, position);
        case 'r': return rndm && mprog_veval(rndm->position.integer(), ifexpr.op, position);
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
        case 'i': return mprog_veval(mob->get_trust(), ifexpr.op, level);
        case 'n': return actor && mprog_veval(actor->get_trust(), ifexpr.op, level);
        case 't': return targ_ch && mprog_veval(targ_ch->get_trust(), ifexpr.op, level);
        case 'r': return rndm && mprog_veval(rndm->get_trust(), ifexpr.op, level);
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
        case 'i': return mprog_veval(mob->class_num, ifexpr.op, class_num);
        case 'n': return actor && mprog_veval(actor->class_num, ifexpr.op, class_num);
        case 't': return targ_ch && mprog_veval(targ_ch->class_num, ifexpr.op, class_num);
        case 'r': return rndm && mprog_veval(rndm->class_num, ifexpr.op, class_num);
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
        case 'i': return mprog_veval(mob->gold, ifexpr.op, gold);
        case 'n': return actor && mprog_veval(actor->gold, ifexpr.op, gold);
        case 't': return targ_ch && mprog_veval(targ_ch->gold, ifexpr.op, gold);
        case 'r': return rndm && mprog_veval(rndm->gold, ifexpr.op, gold);
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
        case 'o': return obj && mprog_veval(magic_enum::enum_integer<ObjectType>(obj->type), ifexpr.op, obj_type);
        case 'p':
            return targ_obj && mprog_veval(magic_enum::enum_integer<ObjectType>(targ_obj->type), ifexpr.op, obj_type);
        default: bug("Mob: {} bad argument to 'objtype': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }
    if (matches(ifexpr.function, "name")) {
        switch (ifexpr.arg[1]) {
        case 'i': return mprog_seval(mob->name, ifexpr.op, ifexpr.operand);
        case 'n': return actor && mprog_seval(actor->name, ifexpr.op, ifexpr.operand);
        case 't': return targ_ch && mprog_seval(targ_ch->name, ifexpr.op, ifexpr.operand);
        case 'r': return rndm && mprog_seval(rndm->name, ifexpr.op, ifexpr.operand);
        case 'o': return obj && mprog_seval(obj->name, ifexpr.op, ifexpr.operand);
        case 'p': return targ_obj && mprog_seval(targ_obj->name, ifexpr.op, ifexpr.operand);
        default: bug("Mob: {} bad argument to 'name': {}", mob->mobIndex->vnum, ifexpr.arg); return false;
        }
    }

    /* Ok... all the ifchcks are done, so if we didnt find ours then something
     * odd happened.  So report the bug and abort the MOBprogram (return error)
     */
    bug("Mob: {} unknown ifchck: {}", mob->mobIndex->vnum, ifexpr.function);
    return false;
}

/* Quite a long and arduous function, this guy handles the control
 * flow part of MOBprograms.  Basicially once the driver sees an
 * 'if' attention shifts to here.  While many syntax errors are
 * caught, some will still get through due to the handling of break
 * and errors in the same fashion.  The desire to break out of the
 * recursion without catastrophe in the event of a mis-parse was
 * believed to be high. Thus, if an error is found, it is bugged and
 * the parser acts as though a break were issued and just bails out
 * at that point. I havent tested all the possibilites, so I'm speaking
 * in theory, but it is 'guaranteed' to work on syntactically correct
 * MOBprograms, so if the mud crashes here, check the mob carefully!
 */
char *mprog_process_if(std::string_view ifchck, char *com_list, Char *mob, const Char *actor, const Object *obj,
                       const mprog::Target target, Char *rndm) {
    char buf[MAX_INPUT_LENGTH];
    char *morebuf = nullptr;
    char *cmnd = nullptr;
    if (mprog_do_ifchck(ifchck, mob, actor, obj, target, rndm)) {
        for (;;) { // ifcheck was true, do commands but ignore else to endif
            cmnd = com_list;
            com_list = mprog_next_command(com_list);
            while (std::isspace(*cmnd))
                cmnd++;
            if (*cmnd == '\0') {
                bug("Mob: {} missing else or endif", mob->mobIndex->vnum);
                return nullptr;
            }
            morebuf = one_argument(cmnd, buf);
            if (!str_cmp(buf, "if")) {
                com_list = mprog_process_if(morebuf, com_list, mob, actor, obj, target, rndm);
                while (std::isspace(*cmnd))
                    cmnd++;
                if (*com_list == '\0')
                    return nullptr;
                cmnd = com_list;
                com_list = mprog_next_command(com_list);
                morebuf = one_argument(cmnd, buf);
                continue;
            }
            if (!str_cmp(buf, "break"))
                return nullptr;
            if (!str_cmp(buf, "endif"))
                return com_list;
            if (!str_cmp(buf, "else")) {
                while (str_cmp(buf, "endif")) {
                    cmnd = com_list;
                    com_list = mprog_next_command(com_list);
                    while (std::isspace(*cmnd))
                        cmnd++;
                    if (*cmnd == '\0') {
                        bug("Mob: {} missing endif after else", mob->mobIndex->vnum);
                        return nullptr;
                    }
                    morebuf = one_argument(cmnd, buf);
                }
                return com_list;
            }
            mprog_process_cmnd(cmnd, mob, actor, obj, target, rndm);
        }
    } else { // false ifcheck, find else and do existing commands or quit at endif
        while ((str_cmp(buf, "else")) && (str_cmp(buf, "endif"))) {
            cmnd = com_list;
            com_list = mprog_next_command(com_list);
            while (std::isspace(*cmnd))
                cmnd++;
            if (*cmnd == '\0') {
                bug("Mob: {} missing an else or endif", mob->mobIndex->vnum);
                return nullptr;
            }
            morebuf = one_argument(cmnd, buf);
        }

        /* found either an else or an endif.. act accordingly */
        if (!str_cmp(buf, "endif"))
            return com_list;
        cmnd = com_list;
        com_list = mprog_next_command(com_list);
        while (std::isspace(*cmnd))
            cmnd++;
        if (*cmnd == '\0') {
            bug("Mob: {} missing endif", mob->mobIndex->vnum);
            return nullptr;
        }
        morebuf = one_argument(cmnd, buf);

        for (;;) { // process the post-else commands until an endif is found.
            if (!str_cmp(buf, "if")) {
                com_list = mprog_process_if(morebuf, com_list, mob, actor, obj, target, rndm);
                while (std::isspace(*cmnd))
                    cmnd++;
                if (*com_list == '\0')
                    return nullptr;
                cmnd = com_list;
                com_list = mprog_next_command(com_list);
                morebuf = one_argument(cmnd, buf);
                continue;
            }
            if (!str_cmp(buf, "else")) {
                bug("Mob: {} found else in an else section", mob->mobIndex->vnum);
                return nullptr;
            }
            if (!str_cmp(buf, "break"))
                return nullptr;
            if (!str_cmp(buf, "endif"))
                return com_list;
            mprog_process_cmnd(cmnd, mob, actor, obj, target, rndm);
            cmnd = com_list;
            com_list = mprog_next_command(com_list);
            while (std::isspace(*cmnd))
                cmnd++;
            if (*cmnd == '\0') {
                bug("Mob:{} missing endif in else section", mob->mobIndex->vnum);
                return nullptr;
            }
            morebuf = one_argument(cmnd, buf);
        }
    }
}

std::string mprog_expand_var(const char c, const Char *mob, const Char *actor, const Object *obj,
                             const mprog::Target target, const Char *rndm) {
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

void mprog_process_cmnd(std::string_view cmnd, Char *mob, const Char *actor, const Object *obj,
                        const mprog::Target target, Char *rndm) {
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
        buf += mprog_expand_var(c, mob, actor, obj, target, rndm);
    }
    interpret(mob, buf);
}

/* The main focus of the MOBprograms.  This routine is called
 *  whenever a trigger is successful.  It is responsible for parsing
 *  the command list and figuring out what to do. However, like all
 *  complex procedures, everything is farmed out to the other guys.
 */
void mprog_driver(Char *mob, const MobProg &prog, const Char *actor, const Object *obj, const mprog::Target target) {

    char tmpcmndlst[MAX_STRING_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    char *morebuf;
    char *command_list;
    char *cmnd;
    Char *rndm = nullptr;
    int count = 0;

    if (mob->is_aff_charm())
        return;

    /* get a random visible mortal player who is in the room with the mob */
    for (auto *vch : mob->in_room->people) {
        if (vch->is_pc() && vch->level < LEVEL_IMMORTAL && can_see(mob, vch)) {
            if (number_range(0, count) == 0)
                rndm = vch;
            count++;
        }
    }
    // Take a copy of the original program because currently the mprog parsing routines
    // mutate the input.
    // TODO: this needs to be rewritten to be more string friendly and use things like line_iter().
    auto com_str = std::string(prog.comlist);
    strncpy(tmpcmndlst, com_str.c_str(), MAX_STRING_LENGTH - 1);
    command_list = tmpcmndlst;
    cmnd = command_list;
    while (cmnd && *cmnd != '\0') {
        command_list = mprog_next_command(command_list);
        morebuf = one_argument(cmnd, buf);
        if (!str_cmp(buf, "if")) {
            command_list = mprog_process_if(morebuf, command_list, mob, actor, obj, target, rndm);
        } else {
            mprog_process_cmnd(cmnd, mob, actor, obj, target, rndm);
        }
        cmnd = command_list;
    }
}

/***************************************************************************
 * Global function code and brief comments.
 */

/* The next two routines are the basic trigger types. Either trigger
 *  on a certain percent, or trigger on a keyword or word phrase.
 *  To see how this works, look at the various trigger routines..
 */
void mprog_wordlist_check(std::string_view arg, Char *mob, const Char *actor, const Object *obj,
                          const mprog::Target target, const MobProgTypeFlag type) {
    for (const auto &mprg : mob->mobIndex->mobprogs) {
        if (mprg.type == type) {
            // Player message matches the phrase in the program.
            if ((mprg.arglist[0] == 'p') && (std::isspace(mprg.arglist[1]))) {
                auto prog_phrase = mprg.arglist.substr(2, mprg.arglist.length());
                if (matches_inside(prog_phrase, arg)) {
                    mprog_driver(mob, mprg, actor, obj, target);
                    break;
                }
            } else {
                // Is any keyword in the program trigger present anywhere in the character's acted message?
                auto prog_keywords = ArgParser(mprg.arglist);
                for (const auto &prog_keyword : prog_keywords) {
                    if (matches_inside(prog_keyword, arg)) {
                        mprog_driver(mob, mprg, actor, obj, target);
                        break;
                    }
                }
            }
        }
    }
}

void mprog_percent_check(Char *mob, Char *actor, Object *obj, const mprog::Target target, const MobProgTypeFlag type) {
    for (const auto &mprg : mob->mobIndex->mobprogs) {
        if (mprg.type == type) {
            if (!is_number(mprg.arglist)) {
                bug("mprog_percent_check in mob #{} expected number argument: {}", mob->mobIndex->vnum, mprg.arglist);
                continue;
            }
            if (number_percent() < parse_number(mprg.arglist)) {
                mprog_driver(mob, mprg, actor, obj, target);
                if (type != MobProgTypeFlag::Greet && type != MobProgTypeFlag::AllGreet)
                    break;
            }
        }
    }
}
/* The triggers.. These are really basic, and since most appear only
 * once in the code (hmm. i think they all do) it would be more efficient
 * to substitute the code in and make the mprog_xxx_check routines global.
 * However, they are all here in one nice place at the moment to make it
 * easier to see what they look like. If you do substitute them back in,
 * make sure you remember to modify the variable names to the ones in the
 * trigger calls.
 */
void mprog_act_trigger(std::string_view buf, Char *mob, const Char *ch, const Object *obj, const mprog::Target target) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, MobProgTypeFlag::Act)) {
        mob->mpact.emplace_back(std::string(buf), ch, obj, target);
    }
}

void mprog_bribe_trigger(Char *mob, Char *ch, int amount) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, MobProgTypeFlag::Bribe)) {
        for (const auto &mprg : mob->mobIndex->mobprogs)
            if (mprg.type == MobProgTypeFlag::Bribe) {
                if (!is_number(mprg.arglist)) {
                    bug("mprog_bribe_trigger in mob #{} expected number argument: {}", mob->mobIndex->vnum,
                        mprg.arglist);
                    continue;
                }
                if (amount >= parse_number(mprg.arglist)) {
                    /* this function previously created a gold object and gave it to ch
                       but there is zero point - the gold transfer is handled in do_give now */
                    mprog_driver(mob, mprg, ch, nullptr, nullptr);
                    break;
                }
            }
    }
}

void mprog_death_trigger(Char *mob) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, MobProgTypeFlag::Death)) {
        mprog_percent_check(mob, nullptr, nullptr, nullptr, MobProgTypeFlag::Death);
    }
}

void mprog_entry_trigger(Char *mob) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, MobProgTypeFlag::Entry))
        mprog_percent_check(mob, nullptr, nullptr, nullptr, MobProgTypeFlag::Entry);
}

void mprog_fight_trigger(Char *mob, Char *ch) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, MobProgTypeFlag::Fight))
        mprog_percent_check(mob, ch, nullptr, nullptr, MobProgTypeFlag::Fight);
}

void mprog_give_trigger(Char *mob, Char *ch, Object *obj) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, MobProgTypeFlag::Give)) {
        for (const auto &mprg : mob->mobIndex->mobprogs) {
            if (mprg.type == MobProgTypeFlag::Give) {
                auto prog_args = ArgParser(mprg.arglist);
                const auto first_prog_arg = prog_args.shift();
                if (matches(obj->name, mprg.arglist) || matches("all", first_prog_arg)) {
                    mprog_driver(mob, mprg, ch, obj, nullptr);
                    break;
                }
            }
        }
    }
}

void mprog_greet_trigger(Char *mob) {
    for (auto *vmob : mob->in_room->people)
        if (vmob->is_npc() && mob != vmob && can_see(vmob, mob) && (vmob->fighting == nullptr) && vmob->is_pos_awake()
            && check_enum_bit(vmob->mobIndex->progtypes, MobProgTypeFlag::Greet))
            mprog_percent_check(vmob, mob, nullptr, nullptr, MobProgTypeFlag::Greet);
        else if (vmob->is_npc() && (vmob->fighting == nullptr) && vmob->is_pos_awake()
                 && check_enum_bit(vmob->mobIndex->progtypes, MobProgTypeFlag::AllGreet))
            mprog_percent_check(vmob, mob, nullptr, nullptr, MobProgTypeFlag::AllGreet);
}

void mprog_hitprcnt_trigger(Char *mob, Char *ch) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, MobProgTypeFlag::HitPercent)) {
        for (const auto &mprg : mob->mobIndex->mobprogs) {
            if (mprg.type == MobProgTypeFlag::HitPercent) {
                if (!is_number(mprg.arglist)) {
                    bug("mprog_hitprcnt_trigger in mob #{} expected number argument: {}", mob->mobIndex->vnum,
                        mprg.arglist);
                    continue;
                }
                if (100 * mob->hit / mob->max_hit < parse_number(mprg.arglist)) {
                    mprog_driver(mob, mprg, ch, nullptr, nullptr);
                    break;
                }
            }
        }
    }
}

void mprog_random_trigger(Char *mob) {
    if (check_enum_bit(mob->mobIndex->progtypes, MobProgTypeFlag::Random))
        mprog_percent_check(mob, nullptr, nullptr, nullptr, MobProgTypeFlag::Random);
}

void mprog_speech_trigger(std::string_view txt, const Char *mob) {
    for (auto *vmob : mob->in_room->people)
        if (vmob->is_npc() && (check_enum_bit(vmob->mobIndex->progtypes, MobProgTypeFlag::Speech)))
            mprog_wordlist_check(txt, vmob, mob, nullptr, nullptr, MobProgTypeFlag::Speech);
}
