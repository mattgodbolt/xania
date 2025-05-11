/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/

#include "Act.hpp"
#include "Char.hpp"
#include "Logging.hpp"
#include "MProg.hpp"
#include "Note.hpp"
#include "Object.hpp"
#include "Pronouns.hpp"
#include "db.h"
#include "string_utils.hpp"

#include <csignal>

namespace {

std::string_view he_she(const Char *ch) { return subjective(*ch); }
std::string_view him_her(const Char *ch) { return objective(*ch); }
std::string_view his_her(const Char *ch) { return possessive(*ch); }
std::string_view himself_herself(const Char *ch) { return reflexive(*ch); }

// TODO the act code could to move into a dedicated class
std::string format_act(std::string_view format, const Char *ch, Act1Arg arg1, Act2Arg arg2, const Char *to,
                       const Char *targ_ch) {
    std::string buf;
    const Logger &logger = ch->mud_.logger();
    bool prev_was_dollar = false;
    for (auto c : format) {
        if (!std::exchange(prev_was_dollar, false)) {
            if (c != '$') {
                prev_was_dollar = false;
                buf.push_back(c);
            } else {
                prev_was_dollar = true;
            }
            continue;
        }

        if (std::holds_alternative<std::nullptr_t>(arg2) && c >= 'A' && c <= 'Z') {
            logger.bug("Act: missing arg2 for code {} in format '{}'", c, format);
            continue;
        }

        switch (c) {
        default:
            logger.bug("Act: bad code {} in format '{}'", c, format);
            break;
            /* Thx alex for 't' idea */
        case 't':
            if (auto arg1_as_string_ptr = std::get_if<std::string_view>(&arg1)) {
                buf += *arg1_as_string_ptr;
            } else {
                logger.bug("$t passed but arg1 was not a string in '{}'", format);
            }
            break;
        case 'T':
            if (auto arg2_as_string_ptr = std::get_if<std::string_view>(&arg2)) {
                buf += *arg2_as_string_ptr;
            } else {
                logger.bug("$T passed but arg2 was not a string in '{}'", format);
            }
            break;
        case 'n': {
            buf += to->describe(*ch);
        } break;
        case 'N': {
            buf += to->describe(*targ_ch);
        } break;
        case 'e': buf += he_she(ch); break;
        case 'E': buf += he_she(targ_ch); break;
        case 'm': buf += him_her(ch); break;
        case 'M': buf += him_her(targ_ch); break;
        case 's': buf += his_her(ch); break;
        case 'S': buf += his_her(targ_ch); break;
        case 'r': buf += himself_herself(ch); break;
        case 'R': buf += himself_herself(targ_ch); break;

        case 'p':
            if (auto arg1_as_obj_ptr = std::get_if<const Object *>(&arg1)) {
                auto &obj1 = *arg1_as_obj_ptr;
                buf += to->can_see(*obj1) ? obj1->short_descr : "something";
            } else {
                logger.bug("$p passed but arg1 was not an object in '{}'", format);
                buf += "something";
            }
            break;

        case 'P':
            if (auto arg2_as_obj_ptr = std::get_if<const Object *>(&arg2)) {
                auto &obj2 = *arg2_as_obj_ptr;
                buf += to->can_see(*obj2) ? obj2->short_descr : "something";
            } else {
                logger.bug("$p passed but arg2 was not an object in '{}'", format);
                buf += "something";
            }
            break;

        case 'd':
            if (auto arg2_as_string_ptr = std::get_if<std::string_view>(&arg2);
                arg2_as_string_ptr != nullptr && !arg2_as_string_ptr->empty()) {
                buf += ArgParser(*arg2_as_string_ptr).shift();
            } else {
                buf += "door";
            }
            break;
        }
    }

    return upper_first_character(buf) + "\n\r";
}

bool act_to_person(const Char *person, const Position::Type min_position) {
    // Ignore folks below minimum position.
    // Allow NPCs so that act-driven triggers can fire.
    return person->position >= min_position;
}

std::vector<const Char *> folks_in_room(const Room *room, const Char *ch, const Char *vch, const To &type,
                                        const Position::Type min_position) {
    std::vector<const Char *> result;
    if (!room) {
        return result;
    }
    for (auto *person : room->people) {
        if (!act_to_person(person, min_position))
            continue;
        // Never consider the character themselves (they're handled explicitly elsewhere).
        if (person == ch)
            continue;
        // Ignore the victim if necessary.
        if (type == To::NotVict && person == vch)
            continue;
        result.emplace_back(person);
    }
    return result;
}

std::vector<const Char *> collect_folks(const Char *ch, const Char *targ_ch, Act2Arg arg2, To type,
                                        const Position::Type min_position) {
    const Room *room{};
    const Logger &logger = ch->mud_.logger();
    switch (type) {
    case To::Char:
        if (act_to_person(ch, min_position))
            return {ch};
        else
            return {};

    case To::Vict:
        if (!targ_ch) {
            logger.bug("Act: null or incorrect type of target ch");
            return {};
        }
        if (!targ_ch->in_room || ch == targ_ch || !act_to_person(targ_ch, min_position))
            return {};

        return {targ_ch};

    case To::GivenRoom:
        if (auto arg2_as_room_ptr = std::get_if<const Room *>(&arg2)) {
            room = *arg2_as_room_ptr;
        } else {
            logger.bug("Act: null room with To::GivenRoom.");
            return {};
        }
        break;

    case To::Room:
    case To::NotVict: room = ch->in_room; break;
    }

    auto result = folks_in_room(room, ch, targ_ch, type, min_position);
    return result;
}

}

void act(std::string_view format, const Char *ch, Act1Arg arg1, Act2Arg arg2, const To type, const MobTrig mob_trig,
         const Position::Type min_position) {
    if (format.empty() || !ch || !ch->in_room)
        return;

    const auto arg1_as_obj_ptr =
        std::holds_alternative<const Object *>(arg1) ? std::get<const Object *>(arg1) : nullptr;
    const auto *targ_ch = std::holds_alternative<const Char *>(arg2) ? std::get<const Char *>(arg2) : nullptr;
    const auto *targ_obj = std::holds_alternative<const Object *>(arg2) ? std::get<const Object *>(arg2) : nullptr;
    const auto mprog_target = MProg::to_target(targ_ch, targ_obj);

    for (auto *to : collect_folks(ch, targ_ch, arg2, type, min_position)) {
        auto formatted = format_act(format, ch, arg1, arg2, to, targ_ch);
        to->send_to(formatted);
        if (mob_trig == MobTrig::Yes) {
            // TODO: heinous const_cast here. Safe, but annoying and worth unpicking deeper down.
            MProg::act_trigger(formatted, const_cast<Char *>(to), ch, arg1_as_obj_ptr, mprog_target);
        }
    }
}
