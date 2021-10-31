/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/

#pragma once

#include <magic_enum.hpp>
#include <range/v3/algorithm/count_if.hpp>
#include <range/v3/view/filter.hpp>

// Locations an Object can be worn in, and the None slot meaning the Object isn't worn.
enum class Wear {
    None = -1,
    Light = 0,
    FingerL = 1,
    FingerR = 2,
    Neck1 = 3,
    Neck2 = 4,
    Body = 5,
    Head = 6,
    Legs = 7,
    Feet = 8,
    Hands = 9,
    Arms = 10,
    Shield = 11,
    About = 12,
    Waist = 13,
    WristL = 14,
    WristR = 15,
    Wield = 16,
    Hold = 17,
    Ears = 18
};

[[nodiscard]] constexpr auto to_int(const Wear wear) noexcept { return magic_enum::enum_integer<Wear>(wear); }

struct WearFilter {
    // Wear locations that are valid slots an object can be equipped in.
    [[nodiscard]] static auto wearable() noexcept {
        return magic_enum::enum_values<Wear>() | ranges::views::filter([](const auto w) { return w != Wear::None; });
    }

    [[nodiscard]] constexpr static auto wearable_count() noexcept { return magic_enum::enum_count<Wear>() - 1; }
    ~WearFilter() = delete;
};
