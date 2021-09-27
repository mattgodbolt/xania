/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "Types.hpp"

#include <optional>
#include <string_view>

// Char body sizes.
enum class BodySize { Tiny = 0, Small = 1, Medium = 2, Large = 3, Huge = 4, Giant = 5 };

namespace BodySizes {

[[nodiscard]] std::optional<BodySize> try_lookup(std::string_view name);
[[nodiscard]] sh_int size_diff(const BodySize size_a, const BodySize size_b);

// NPCs that are larger than Medium get a small bonus to their strength and constitution.
[[nodiscard]] sh_int get_mob_str_bonus(const BodySize body_size);
[[nodiscard]] sh_int get_mob_con_bonus(const BodySize body_size);
}
