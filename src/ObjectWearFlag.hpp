/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "Flag.hpp"
#include "common/StandardBits.hpp"

#include <magic_enum.hpp>

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

constexpr auto NumWearFlags = 17;
constexpr std::array<Flag, NumWearFlags> AllObjectWearFlags = {{
    // clang-format off
    {to_int(ObjectWearFlag::Take), "take"},
    {to_int(ObjectWearFlag::Finger), "finger"},
    {to_int(ObjectWearFlag::Neck), "neck"},
    {to_int(ObjectWearFlag::Body), "body"},
    {to_int(ObjectWearFlag::Head), "head"},
    {to_int(ObjectWearFlag::Legs), "legs"},
    {to_int(ObjectWearFlag::Feet), "feet"},
    {to_int(ObjectWearFlag::Hands), "hands"},
    {to_int(ObjectWearFlag::Arms), "arms"},
    {to_int(ObjectWearFlag::Shield), "shield"},
    {to_int(ObjectWearFlag::About), "about"},
    {to_int(ObjectWearFlag::Waist), "waist"},
    {to_int(ObjectWearFlag::Wrist), "wrist"},
    {to_int(ObjectWearFlag::Wield), "wield"},
    {to_int(ObjectWearFlag::Hold), "hold"},
    {to_int(ObjectWearFlag::TwoHands), "twohands"},
    {to_int(ObjectWearFlag::Ears), "ears"},
    // clang-format on
}};

}