#include "Direction.hpp"

#include "merc.h"
#include "string_utils.hpp"

#include <fmt/format.h>

Direction reverse(Direction dir) {
    switch (dir) {
    case Direction::North: return Direction::South;
    case Direction::East: return Direction::West;
    case Direction::South: return Direction::North;
    case Direction::West: return Direction::East;
    case Direction::Up: return Direction::Down;
    case Direction::Down: return Direction::Up;
    }
    throw std::runtime_error(fmt::format("Bad direction {}", static_cast<int>(dir)));
}

std::string_view to_string(Direction dir) {
    using namespace std::literals;
    switch (dir) {
    case Direction::North: return "north"sv;
    case Direction::East: return "east"sv;
    case Direction::South: return "south"sv;
    case Direction::West: return "west"sv;
    case Direction::Up: return "up"sv;
    case Direction::Down: return "down"sv;
    }
    throw std::runtime_error(fmt::format("Bad direction {}", static_cast<int>(dir)));
}

std::optional<Direction> try_parse_direction(std::string_view name) {
    for (auto dir : all_directions)
        if (matches_start(name, to_string(dir)))
            return dir;
    return {};
}

Direction random_direction() {
    for (;;) {
        auto door = static_cast<unsigned>(number_mm()) & 7u;
        if (door < all_directions.size())
            return Direction(door);
    }
}

std::optional<Direction> try_cast_direction(int ordinal) {
    switch (ordinal) {
    case static_cast<int>(Direction::North): return Direction::North;
    case static_cast<int>(Direction::East): return Direction::East;
    case static_cast<int>(Direction::South): return Direction::South;
    case static_cast<int>(Direction::West): return Direction::West;
    case static_cast<int>(Direction::Up): return Direction::Up;
    case static_cast<int>(Direction::Down): return Direction::Down;
    }
    return {};
}
