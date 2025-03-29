/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
#include "common/StandardBits.hpp"

#include <magic_enum/magic_enum.hpp>

enum class ObjectWearFlag : unsigned int {
    Take = A,
    Finger = B,
    Neck = C,
    Body = D,
    Head = E,
    Legs = F,
    Feet = G,
    Hands = H,
    Arms = I,
    Shield = J,
    About = K,
    Waist = L,
    Wrist = M,
    Wield = N,
    Hold = O,
    TwoHands = P,
    Ears = Q
};

[[nodiscard]] constexpr auto to_int(const ObjectWearFlag flag) noexcept {
    return magic_enum::enum_integer<ObjectWearFlag>(flag);
}

namespace ObjectWearFlags {

using WF = Flag<ObjectWearFlag>;
constexpr auto Take = WF{ObjectWearFlag::Take, "take"};
constexpr auto Finger = WF{ObjectWearFlag::Finger, "finger"};
constexpr auto Neck = WF{ObjectWearFlag::Neck, "neck"};
constexpr auto Body = WF{ObjectWearFlag::Body, "body"};
constexpr auto Head = WF{ObjectWearFlag::Head, "head"};
constexpr auto Legs = WF{ObjectWearFlag::Legs, "legs"};
constexpr auto Feet = WF{ObjectWearFlag::Feet, "feet"};
constexpr auto Hands = WF{ObjectWearFlag::Hands, "hands"};
constexpr auto Arms = WF{ObjectWearFlag::Arms, "arms"};
constexpr auto Shield = WF{ObjectWearFlag::Shield, "shield"};
constexpr auto About = WF{ObjectWearFlag::About, "about"};
constexpr auto Waist = WF{ObjectWearFlag::Waist, "waist"};
constexpr auto Wrist = WF{ObjectWearFlag::Wrist, "wrist"};
constexpr auto Wield = WF{ObjectWearFlag::Wield, "wield"};
constexpr auto Hold = WF{ObjectWearFlag::Hold, "hold"};
constexpr auto TwoHands = WF{ObjectWearFlag::TwoHands, "twohands"};
constexpr auto Ears = WF{ObjectWearFlag::Ears, "ears"};

constexpr auto NumWearFlags = 17;
constexpr std::array<WF, NumWearFlags> AllObjectWearFlags = {{Take, Finger, Neck, Body, Head, Legs, Feet, Hands, Arms,
                                                              Shield, About, Waist, Wrist, Wield, Hold, TwoHands,
                                                              Ears}};

}
