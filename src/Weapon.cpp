/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Weapon.hpp"
#include "string_utils.hpp"

#include <magic_enum.hpp>

std::optional<Weapon> Weapons::try_from_name(std::string_view name) {
    for (const auto &enum_name : magic_enum::enum_names<Weapon>()) {
        // This doesn't use enum_cast() here as we want a case insensitive prefix match.
        if (matches_start(name, enum_name)) {
            if (const auto val = magic_enum::enum_cast<Weapon>(enum_name)) {
                return val;
            }
        }
    }
    // Note: Unlike ObjectType::lookup_impl(), this function does not fallback to
    // looking up the type by number. That's because there are no weapon objects in
    // the database files that specify the weapon type as a number.
    return std::nullopt;
}

std::optional<Weapon> Weapons::try_from_ordinal(const int num) {
    if (const auto enum_val = magic_enum::enum_cast<Weapon>(num)) {
        return enum_val;
    } else {
        return std::nullopt;
    }
}

std::string Weapons::name_from_ordinal(const int num) {
    if (const auto opt_enum = try_from_ordinal(num)) {
        std::string name = lower_case(magic_enum::enum_name<Weapon>(*opt_enum));
        return name;
    } else {
        return "unknown";
    }
}
