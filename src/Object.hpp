/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "AffectList.hpp"
#include "ExtraDescription.hpp"
#include "GenericList.hpp"
#include "Materials.hpp"
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
    int wear_flags{};
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
};