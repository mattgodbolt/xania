#pragma once
#include <string>

// String and memory management parameters.
static inline constexpr auto MAX_STRING_LENGTH = 4096;
static inline constexpr auto MAX_INPUT_LENGTH = 256;
static inline constexpr auto PAGELEN = 22;

// Game parameters.
// Increase the max'es if you add more of something.
// Adjust the pulse numbers to suit yourself.
static inline constexpr auto MAX_SOCIALS = 256;
static inline constexpr auto MAX_SKILL = 150;
static inline constexpr auto MAX_GROUP = 30;
static inline constexpr auto MAX_IN_GROUP = 15;
static inline constexpr auto MAX_CLASS = 4;
static inline constexpr auto MAX_PC_RACE = 12;
static inline constexpr auto MAX_LEVEL = 100;
static inline constexpr auto LEVEL_HERO = MAX_LEVEL - 9;
static inline constexpr auto LEVEL_IMMORTAL = MAX_LEVEL - 8;
static inline constexpr auto LEVEL_CONSIDER = 80;

static inline constexpr auto PULSE_PER_SECOND = 4;
static inline constexpr auto PULSE_VIOLENCE = 3 * PULSE_PER_SECOND;
static inline constexpr auto PULSE_MOBILE = 4 * PULSE_PER_SECOND;
static inline constexpr auto PULSE_TICK = 30 * PULSE_PER_SECOND;
static inline constexpr auto PULSE_AREA = 60 * PULSE_PER_SECOND;

static inline constexpr auto IMPLEMENTOR = MAX_LEVEL;
static inline constexpr auto CREATOR = MAX_LEVEL - 1;
static inline constexpr auto SUPREME = MAX_LEVEL - 2;
static inline constexpr auto DEITY = MAX_LEVEL - 3;
static inline constexpr auto GOD = MAX_LEVEL - 4;
static inline constexpr auto IMMORTAL = MAX_LEVEL - 5;
static inline constexpr auto DEMI = MAX_LEVEL - 6;
static inline constexpr auto ANGEL = MAX_LEVEL - 7;
static inline constexpr auto AVATAR = MAX_LEVEL - 8;
static inline constexpr auto HERO = LEVEL_HERO;
