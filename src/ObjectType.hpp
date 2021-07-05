/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include <magic_enum.hpp>
#include <optional>
#include <vector>

enum class ObjectType {
    // There's no 0 type. Shopkeepers have a list of type slots they will trade in
    // can be initialized with 0 which means they don't trade.
    Light = 1,
    Scroll = 2,
    Wand = 3,
    Staff = 4,
    Weapon = 5,
    Treasure = 8,
    Armor = 9,
    Potion = 10,
    Clothing = 11,
    Furniture = 12,
    Trash = 13,
    Container = 15,
    Drink = 17,
    Key = 18,
    Food = 19,
    Money = 20,
    Boat = 22,
    Npccorpse = 23,
    Pccorpse = 24,
    Fountain = 25,
    Pill = 26,
    Map = 28,
    Bomb = 29,
    Portal = 30
};

class ObjectTypes {
public:
    [[nodiscard]] static std::optional<ObjectType> try_from_ordinal(const int num);
    // Lookup an item type by its type name or type number.
    // Returns a default type if no match is found, which is a bug.
    [[nodiscard]] static ObjectType lookup_with_default(std::string_view name);
    [[nodiscard]] static std::optional<ObjectType> try_lookup(std::string_view name);
    // Returns all the object type enum names in lower case.
    [[nodiscard]] static std::vector<std::string> sorted_type_names();
    ObjectTypes() = delete;

private:
    [[nodiscard]] static std::optional<ObjectType> lookup_impl(std::string_view name);
};
