#pragma once

#include "Descriptor.hpp"
#include "doorman/doorman_protocol.h"

#include <cstddef>
#include <string_view>
#include <variant>

struct CHAR_DATA;
typedef struct obj_data OBJ_DATA;
typedef struct room_index_data ROOM_INDEX_DATA;

void close_socket(Descriptor *dclose);
void send_to_char(std::string_view txt, const CHAR_DATA *ch);
void page_to_char(const char *txt, CHAR_DATA *ch);

/*
 * TO types for act.
 */
enum class To { Room, NotVict, Vict, Char, GivenRoom };

using Act1Arg = std::variant<nullptr_t, const OBJ_DATA *, std::string_view>;
using Act2Arg = std::variant<nullptr_t, const OBJ_DATA *, std::string_view, const CHAR_DATA *, const ROOM_INDEX_DATA *>;
void act(const char *format, CHAR_DATA *ch, Act1Arg arg1, Act2Arg arg2, To type);
void act(const char *format, CHAR_DATA *ch, Act1Arg arg1, Act2Arg arg2, To type, int min_pos);
inline void act(const char *format, CHAR_DATA *ch, To type = To::Room) { act(format, ch, nullptr, nullptr, type); }

bool SendPacket(Packet *p, const void *extra);

extern Descriptor *descriptor_list;
