#pragma once

#include "Descriptor.hpp"
#include "common/Fd.hpp"
#include "common/doorman_protocol.h"

#include <cstddef>
#include <string_view>
#include <variant>

struct CHAR_DATA;
struct OBJ_DATA;
struct ROOM_INDEX_DATA;

void game_loop_unix(Fd control);
Fd init_socket(const char *file);

void page_to_char(const char *txt, CHAR_DATA *ch);

/*
 * TO types for act.
 */
enum class To { Room, NotVict, Vict, Char, GivenRoom };

using Act1Arg = std::variant<nullptr_t, const OBJ_DATA *, std::string_view>;
using Act2Arg = std::variant<nullptr_t, const OBJ_DATA *, std::string_view, const CHAR_DATA *, const ROOM_INDEX_DATA *>;
void act(std::string_view format, const CHAR_DATA *ch, Act1Arg arg1, Act2Arg arg2, To type);
void act(std::string_view format, const CHAR_DATA *ch, Act1Arg arg1, Act2Arg arg2, To type, int min_pos);
inline void act(std::string_view format, const CHAR_DATA *ch, To type = To::Room) {
    act(format, ch, nullptr, nullptr, type);
}

// Support for wacky nullptr format things in older code (e.g. socials and puff use null here as a "don't do this").
// (see #148). Ideally we'd remove this, but for now...
template <typename... Args>
inline void act(const char *format, const CHAR_DATA *ch, Args &&... args) {
    if (format)
        act(std::string_view(format), ch, std::forward<Args>(args)...);
}

bool send_to_doorman(const Packet *p, const void *extra);
std::string format_prompt(const CHAR_DATA &ch, std::string_view prompt);
