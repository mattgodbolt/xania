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

#include "MProg.hpp"
#include "ArgParser.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "Logging.hpp"
#include "MProgImpl.hpp"
#include "MProgProgram.hpp"
#include "MProgTypeFlag.hpp"
#include "Object.hpp"
#include "Room.hpp"
#include "common/BitOps.hpp"
#include "db.h"
#include "string_utils.hpp"

#include <fmt/format.h>

#include <string>
#include <vector>

using namespace std::literals;

[[nodiscard]] extern Char *get_char_world(Char *ch, std::string_view argument);

namespace MProg {

Target to_target(const Char *ch, const Object *obj) {
    if (ch)
        return MProg::Target{ch};
    else if (obj)
        return MProg::Target{obj};
    else
        return MProg::Target{nullptr};
}

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
        impl::exec_with_chance(mob, nullptr, nullptr, nullptr, TypeFlag::Death);
    }
}

void entry_trigger(Char *mob) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, TypeFlag::Entry))
        impl::exec_with_chance(mob, nullptr, nullptr, nullptr, TypeFlag::Entry);
}

void fight_trigger(Char *mob, Char *ch) {
    if (mob->is_npc() && check_enum_bit(mob->mobIndex->progtypes, TypeFlag::Fight))
        impl::exec_with_chance(mob, ch, nullptr, nullptr, TypeFlag::Fight);
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
    const auto &mob_ref = *mob;
    for (auto *vmob : mob->in_room->people)
        if (vmob->is_npc() && mob != vmob && vmob->can_see(mob_ref) && !vmob->fighting && vmob->is_pos_awake()
            && check_enum_bit(vmob->mobIndex->progtypes, TypeFlag::Greet))
            impl::exec_with_chance(vmob, mob, nullptr, nullptr, TypeFlag::Greet);
        else if (vmob->is_npc() && !vmob->fighting && vmob->is_pos_awake()
                 && check_enum_bit(vmob->mobIndex->progtypes, TypeFlag::AllGreet))
            impl::exec_with_chance(vmob, mob, nullptr, nullptr, TypeFlag::AllGreet);
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
        impl::exec_with_chance(mob, nullptr, nullptr, nullptr, TypeFlag::Random);
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
