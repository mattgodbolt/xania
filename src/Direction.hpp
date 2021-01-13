#pragma once

#include "PerEnum.hpp"
#include "Types.hpp"

#include <array>
#include <optional>
#include <stdexcept>
#include <string_view>

// Used in #ROOMS, so numeric value is important.
enum class Direction { North = 0, East = 1, South = 2, West = 3, Up = 4, Down = 5 };

static inline constexpr std::array<Direction, 6> all_directions = {
    {Direction::North, Direction::East, Direction::South, Direction::West, Direction::Up, Direction::Down}};

Direction random_direction();
Direction reverse(Direction dir);
std::string_view to_string(Direction dir);
std::optional<Direction> try_parse_direction(std::string_view name);
std::optional<Direction> try_cast_direction(int ordinal);

template <typename T>
using PerDirection = PerEnum<Direction, T, 6u>;