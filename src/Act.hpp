#pragma once

#include "MobTrig.hpp"
#include "Position.hpp"

#include <string_view>
#include <variant>

struct Char;
struct Object;
struct Room;

// Functions that act out events in a room. The 'To' types act as a filter, targeting specific types of character,
// for whom the message text will make sense.

enum class To { Room, NotVict, Vict, Char, GivenRoom };

using Act1Arg = std::variant<std::nullptr_t, const Object *, std::string_view>;
using Act2Arg = std::variant<std::nullptr_t, const Object *, std::string_view, const Char *, const Room *>;
void act(std::string_view format, const Char *ch, Act1Arg arg1, Act2Arg arg2, const To type, const MobTrig mob_trig);
void act(std::string_view format, const Char *ch, Act1Arg arg1, Act2Arg arg2, const To type, const MobTrig mob_trig,
         const Position::Type min_position);
inline void act(std::string_view format, const Char *ch, const To type = To::Room) {
    act(format, ch, nullptr, nullptr, type, MobTrig::Yes);
}
inline void act(std::string_view format, const Char *ch, const To type, const MobTrig mob_trig = MobTrig::Yes) {
    act(format, ch, nullptr, nullptr, type, mob_trig);
}
inline void act(std::string_view format, const Char *ch, Act1Arg arg1, Act2Arg arg2, const To type,
                const MobTrig mob_trig = MobTrig::Yes) {
    act(format, ch, arg1, arg2, type, mob_trig, Position::Type::Resting);
}

// Support for wacky nullptr format things in older code (e.g. socials and puff use null here as a "don't do this").
// (see #148). Ideally we'd remove this, but for now...
template <typename... Args>
inline void act(const char *format, const Char *ch, Args &&...args) {
    if (format)
        act(std::string_view(format), ch, std::forward<Args>(args)...);
}
