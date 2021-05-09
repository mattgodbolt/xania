#pragma once

#include "ArgParser.hpp"

#include <string>

struct Char;

constexpr unsigned BAN_PERMANENT = 1;
constexpr unsigned BAN_NEWBIES = 2;
constexpr unsigned BAN_PERMIT = 4;
constexpr unsigned BAN_PREFIX = 8;
constexpr unsigned BAN_SUFFIX = 16;
constexpr unsigned BAN_ALL = 32;

void load_bans();
bool check_ban(const std::string &host, int type);
