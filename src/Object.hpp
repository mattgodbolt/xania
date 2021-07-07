/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "AffectList.hpp"
#include "GenericList.hpp"
#include "Types.hpp"

#include <array>
#include <string>
#include <vector>

enum class Material;
struct Char;
struct ExtraDescription;
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
    Material material{};
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
};