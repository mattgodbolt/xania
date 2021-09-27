#include "Rng.hpp"

#include <stdexcept>

int Rng::dice(int number, int size) noexcept {
    if (size == 0)
        return 0;
    if (size == 1)
        return number;

    int sum = 0;
    for (auto idice = 0; idice < number; idice++)
        sum += number_range(1, static_cast<int>(size));

    return sum;
}

int Rng::number_range(int from, int to) noexcept {
    if (from == 0 && to == 0)
        return 0;

    if ((to = to - from + 1) <= 1)
        return from;

    int power;
    for (power = 2; power < to; power <<= 1u)
        ;

    int number;
    while ((number = number_mm() & (power - 1)) >= to)
        ;

    return from + number;
}

int Rng::number_percent() noexcept {
    int percent;

    while ((percent = number_mm() & (128 - 1)) > 99)
        ;

    return 1 + percent;
}

namespace {
Rng *the_rng{};
}

Rng &Rng::global_rng() {
    if (!the_rng)
        throw std::runtime_error("No RNG set!");
    return *the_rng;
}

void Rng::set_global_rng(Rng &rng) { the_rng = &rng; }

KnuthRng::KnuthRng(int seed) {
    auto *piState = &state_[2];

    piState[-2] = StateSize - StateSize;
    piState[-1] = StateSize - StateSize / 2;

    piState[0] = seed & ((1u << 30) - 1);
    piState[1] = 1;
    for (auto iState = 2u; iState < StateSize; iState++) {
        piState[iState] = (piState[iState - 1] + piState[iState - 2]) & ((1u << 30) - 1);
    }
}

int KnuthRng::number_mm() noexcept {
    auto *piState = &state_[2];
    auto iState1 = piState[-2];
    auto iState2 = piState[-1];
    auto iRand = (piState[iState1] + piState[iState2]) & ((1u << 30) - 1);
    piState[iState1] = iRand;
    if (++iState1 == StateSize)
        iState1 = 0;
    if (++iState2 == StateSize)
        iState2 = 0;
    piState[-2] = iState1;
    piState[-1] = iState2;
    return static_cast<int>(iRand >> 6);
}
