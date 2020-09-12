#pragma once

#include "Rng.hpp"

#include <catch2/trompeloeil.hpp>

namespace test {

struct MockRng : Rng {
    MAKE_MOCK0(number_mm, int(), noexcept);
    MAKE_MOCK1(number_bits, int(unsigned int), noexcept);
    MAKE_MOCK2(dice, int(int, int), noexcept);
    MAKE_MOCK2(number_range, int(int, int), noexcept);
    MAKE_MOCK0(number_percent, int(), noexcept);
};

}