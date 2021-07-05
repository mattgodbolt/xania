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

std::optional<ObjectType> ObjectTypes::try_from_ordinal(const int num) {
    if (auto enum_val = magic_enum::enum_cast<ObjectType>(num)) {
        return *enum_val;
    } else {
        return std::nullopt;
    }
}

ObjectType ObjectTypes::lookup_with_default(std::string_view name) {
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
std::optional<ObjectType> ObjectTypes::try_lookup(std::string_view name) { return lookup_impl(name); }

std::string ObjectTypes::list_type_names() {
    std::string result = "\t";
    for (size_t i = 1; i < magic_enum::enum_count<ObjectType>(); i++) {
        const auto val = magic_enum::enum_value<ObjectType>(i);
        result += lower_case(magic_enum::enum_name(val));
        result += " ";
        if (i % 7 == 0) {
            result += "\n\r\t";
        }
    }
    result += "\n\r";
    return result;
}

std::optional<ObjectType> ObjectTypes::lookup_impl(std::string_view name) {
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
    const auto val = magic_enum::enum_cast<ObjectType>(num);
    if (val.has_value()) {
        return *val;
    } else {
        return std::nullopt;
    }
}
