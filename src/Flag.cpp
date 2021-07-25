/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2021 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/

#include "Flag.hpp"
#include "common/BitOps.hpp"
#include <range/v3/numeric/accumulate.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/transform.hpp>

std::string serialize_flags(const unsigned int value) {
    const static std::string_view table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef";
    if (value == 0) {
        return "0";
    }
    namespace rv = ranges::views;
    auto result = ranges::accumulate(rv::iota(0u, table.length()) | rv::filter([&value](const auto i) {
                                         return check_bit(value, (1 << i));
                                     }) | rv::transform([](const auto i) { return table[i]; }),
                                     std::string());
    return result;
}
