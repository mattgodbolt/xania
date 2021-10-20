/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "ObjectType.hpp"
#include "Logging.hpp"
#include "lookup.h"
#include "string_utils.hpp"

#include <magic_enum.hpp>

namespace ObjectTypes {

std::optional<ObjectType> try_from_integer(const int num) { return magic_enum::enum_cast<ObjectType>(num); }

std::optional<ObjectType> lookup_impl(std::string_view name) {
    for (const auto &enum_name : magic_enum::enum_names<ObjectType>()) {
        // This doesn't use enum_cast() here as we want a case insensitive prefix match.
        if (matches_start(name, enum_name)) {
            if (const auto val = magic_enum::enum_cast<ObjectType>(enum_name)) {
                return val;
            }
        }
    }
    // Try lookup by number.
    const auto num = parse_number(name);
    return magic_enum::enum_cast<ObjectType>(num);
}

ObjectType lookup_with_default(std::string_view name) {
    if (const auto opt_type = lookup_impl(name)) {
        return *opt_type;
    }
    bug("Unknown item type '{}' - defaulting!", name);
    return ObjectType::Light;
}

/**
 * Lookup an item type by its type name or type number.
 * This is stricter than item_lookup() as it doesn't fallback to a default type
 * and will instead return std::nullopt if no match is found.
 */
std::optional<ObjectType> try_lookup(std::string_view name) { return lookup_impl(name); }

std::vector<std::string> sorted_type_names() {
    std::vector<std::string> result;
    for (const auto &enum_name : magic_enum::enum_names<ObjectType>()) {
        result.emplace_back(lower_case(enum_name));
    }
    std::sort(result.begin(), result.end());
    return result;
}

}