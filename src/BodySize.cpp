/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "BodySize.hpp"
#include "string_utils.hpp"

#include <magic_enum/magic_enum.hpp>

namespace BodySizes {

std::optional<BodySize> try_lookup(std::string_view name) {
    for (const auto &enum_name : magic_enum::enum_names<BodySize>()) {
        // This doesn't use enum_cast() here as we want a case insensitive prefix match.
        if (matches_start(name, enum_name)) {
            if (const auto val = magic_enum::enum_cast<BodySize>(enum_name)) {
                return val;
            }
        }
    }
    // Unlike enums such as ObjectType, BodySize lookups never needed to support
    // number->size so it gives up here.
    return std::nullopt;
}

sh_int size_diff(const BodySize size_a, const BodySize size_b) {
    return magic_enum::enum_integer<BodySize>(size_a) - magic_enum::enum_integer<BodySize>(size_b);
}

sh_int get_mob_str_bonus(const BodySize body_size) {
    return magic_enum::enum_integer<BodySize>(body_size) - magic_enum::enum_integer<BodySize>(BodySize::Medium);
}

sh_int get_mob_con_bonus(const BodySize body_size) {
    return (magic_enum::enum_integer<BodySize>(body_size) - magic_enum::enum_integer<BodySize>(BodySize::Medium)) / 2;
}

}
