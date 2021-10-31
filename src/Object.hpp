/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "AffectList.hpp"
#include "ExtraDescription.hpp"
#include "Flag.hpp"
#include "GenericList.hpp"
#include "Materials.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectWearFlag.hpp"
#include "Types.hpp"

#include <array>
#include <string>
#include <vector>

struct Char;
struct ObjectIndex;
enum class ObjectType;
struct Room;

/*
 * One object.
 */
struct Object {
    GenericList<Object *> contains;
    Object *in_obj{};
    Char *carried_by{};
    std::vector<ExtraDescription> extra_descr;
    AffectList affected{};
    ObjectIndex *objIndex{};
    Room *in_room{};
    bool enchanted{};
    std::string owner;
    std::string name;
    std::string short_descr;
    std::string description;
    ObjectType type{};
    unsigned int extra_flags{};
    unsigned int wear_flags{};
    std::string wear_string;
    int wear_loc{};
    sh_int weight{};
    int cost{};
    sh_int level{};
    sh_int condition{};
    Material::Type material{};
    sh_int timer{};
    std::array<int, 5> value{};
    Room *destination{};

    [[nodiscard]] std::string type_name() const;
    [[nodiscard]] bool is_holdable() const;
    [[nodiscard]] bool is_takeable() const;
    [[nodiscard]] bool is_two_handed() const;
    [[nodiscard]] bool is_wieldable() const;
    [[nodiscard]] bool is_wear_about() const;
    [[nodiscard]] bool is_wear_arms() const;
    [[nodiscard]] bool is_wear_body() const;
    [[nodiscard]] bool is_wear_ears() const;
    [[nodiscard]] bool is_wear_feet() const;
    [[nodiscard]] bool is_wear_finger() const;
    [[nodiscard]] bool is_wear_hands() const;
    [[nodiscard]] bool is_wear_head() const;
    [[nodiscard]] bool is_wear_legs() const;
    [[nodiscard]] bool is_wear_neck() const;
    [[nodiscard]] bool is_wear_shield() const;
    [[nodiscard]] bool is_wear_waist() const;
    [[nodiscard]] bool is_wear_wrist() const;
    // Various Object attribute checkers.
    [[nodiscard]] bool is_anti_evil() const;
    [[nodiscard]] bool is_anti_good() const;
    [[nodiscard]] bool is_anti_neutral() const;
    [[nodiscard]] bool is_blessed() const;
    [[nodiscard]] bool is_dark() const;
    [[nodiscard]] bool is_evil() const;
    [[nodiscard]] bool is_glowing() const;
    [[nodiscard]] bool is_humming() const;
    [[nodiscard]] bool is_inventory() const;
    [[nodiscard]] bool is_invisible() const;
    [[nodiscard]] bool is_lock() const; // unused?
    [[nodiscard]] bool is_magic() const;
    [[nodiscard]] bool is_no_drop() const;
    [[nodiscard]] bool is_no_locate() const;
    [[nodiscard]] bool is_no_purge() const; // Won't be destroyed if the purge command is used.
    [[nodiscard]] bool is_no_remove() const;
    [[nodiscard]] bool is_protect_container() const;
    [[nodiscard]] bool is_rot_death() const; // Decays quickly after death of owner.
    [[nodiscard]] bool is_summon_corpse() const; // Reagent consumed by corpse summoners.
    [[nodiscard]] bool is_unique() const;
    [[nodiscard]] bool is_vis_death() const; // Invisible until its owner dies.
    // Weapon specific attribute checks.
    [[nodiscard]] bool is_weapon_two_handed() const;

    inline static constexpr std::array<Flag, 21> AllExtraFlags = {{
        // clang-format off
        {to_int(ObjectExtraFlag::Glow), 0, "glow"},
        {to_int(ObjectExtraFlag::Hum), 0, "hum"},
        {to_int(ObjectExtraFlag::Dark), 0, "dark"},
        {to_int(ObjectExtraFlag::Lock), 0, "lock"},
        {to_int(ObjectExtraFlag::Evil), 0, "evil"},
        {to_int(ObjectExtraFlag::Invis), 0, "invis"},
        {to_int(ObjectExtraFlag::Magic), 0, "magic"},
        {to_int(ObjectExtraFlag::NoDrop), 0, "nodrop"},
        {to_int(ObjectExtraFlag::Bless), 0, "bless"},
        {to_int(ObjectExtraFlag::AntiGood), 0, "antigood"},
        {to_int(ObjectExtraFlag::AntiEvil), 0, "antievil"},
        {to_int(ObjectExtraFlag::AntiNeutral), 0, "antineutral"},
        {to_int(ObjectExtraFlag::NoRemove), 0, "noremove"}, // Only weapons are meant to have this, it prevents disarm.
        {to_int(ObjectExtraFlag::Inventory), 0, "inventory"},
        {to_int(ObjectExtraFlag::NoPurge), 0, "nopurge"},
        {to_int(ObjectExtraFlag::RotDeath), 0, "rotdeath"},
        {to_int(ObjectExtraFlag::VisDeath), 0, "visdeath"},
        {to_int(ObjectExtraFlag::ProtectContainer), 0, "protected"},
        {to_int(ObjectExtraFlag::NoLocate), 0, "nolocate"},
        {to_int(ObjectExtraFlag::SummonCorpse), 0, "summon_corpse"},
        {to_int(ObjectExtraFlag::Unique), 0, "unique"},
        // clang-format on
    }};

    inline static constexpr std::array<Flag, 17> AllWearFlags = {{
        // clang-format off
        {to_int(ObjectWearFlag::Take), 0, "take"},
        {to_int(ObjectWearFlag::Finger), 0, "finger"},
        {to_int(ObjectWearFlag::Neck), 0, "neck"},
        {to_int(ObjectWearFlag::Body), 0, "body"},
        {to_int(ObjectWearFlag::Head), 0, "head"},
        {to_int(ObjectWearFlag::Legs), 0, "legs"},
        {to_int(ObjectWearFlag::Feet), 0, "feet"},
        {to_int(ObjectWearFlag::Hands), 0, "hands"},
        {to_int(ObjectWearFlag::Arms), 0, "arms"},
        {to_int(ObjectWearFlag::Shield), 0, "shield"},
        {to_int(ObjectWearFlag::About), 0, "about"},
        {to_int(ObjectWearFlag::Waist), 0, "waist"},
        {to_int(ObjectWearFlag::Wrist), 0, "wrist"},
        {to_int(ObjectWearFlag::Wield), 0, "wield"},
        {to_int(ObjectWearFlag::Hold), 0, "hold"},
        {to_int(ObjectWearFlag::TwoHands), 0, "twohands"},
        {to_int(ObjectWearFlag::Ears), 0, "ears"},
        // clang-format on
    }};
};