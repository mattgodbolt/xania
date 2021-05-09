#pragma once

#include "Types.hpp"

#include <limits>
#include <type_traits>

// CharVersion is used in the Char.version field and tracks the version format that
// player files are saved as. Originally this was significant because the mud took
// a lazy approach to handling "upgrades" to the content of player files. When a player
// logged in, it'd check their version field and then perform any required updates to their data.
// But these versions and the actions to perform were always coded in a haphazard way,
// and the logic to perform those upgrades had to be kept in the mud indefinitely.
// Now CharVersion is used by `pfu` upgrade tasks.
enum class CharVersion : ush_int {
    Zero = 0,
    // Skip 1 & 2 as we have no data with those versions
    Three = 3,
    Four = 4,
    Five = 5,
    // Latest is the value that fwrite_char() is meant to use when saving a Char.
    Latest = Five,
    Max = std::numeric_limits<std::underlying_type_t<CharVersion>>::max()
};
