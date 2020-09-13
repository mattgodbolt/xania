#pragma once

#include <array>
#include <cstdint>

class Rng {
public:
    virtual ~Rng() = default;
    virtual int number_mm() noexcept = 0;
    virtual int number_bits(unsigned int width) noexcept { return number_mm() & ((1u << width) - 1); }
    virtual int dice(int number, int size) noexcept;
    virtual int number_range(int from, int to) noexcept;
    virtual int number_percent() noexcept;

    static Rng &global_rng();
    static void set_global_rng(Rng &rng);
};

class KnuthRng final : public Rng {
    /*
     * I've gotten too many bad reports on OS-supplied random number generators.
     * This is the Mitchell-Moore algorithm from Knuth Volume II.
     * Best to leave the constants alone unless you've read Knuth.
     * -- Furey
     */
    static constexpr auto StateSize = 55u;
    std::array<uint32_t, 2 + StateSize> state_{};

public:
    explicit KnuthRng(int seed);
    int number_mm() noexcept override;
};

class FakeRng final : public Rng {
    int result_;

public:
    explicit FakeRng(int result) : result_(result) {}
    int number_mm() noexcept override { return result_; }
};