#include "AffectList.hpp"

#include <catch2/catch.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

namespace {
using vi = std::vector<int>;
vi to_vi(const AffectList &list) {
    return list | ranges::view::transform([](auto &ad) { return ad.type; }) | ranges::to<std::vector<int>>;
}

}

TEST_CASE("Affect list") {
    AffectList list;
    SECTION("starts empty") { CHECK(list.empty()); }
    SECTION("adds to the front") {
        list.add(AFFECT_DATA{1});
        list.add(AFFECT_DATA{2});
        list.add(AFFECT_DATA{3});
        CHECK(!list.empty());
        CHECK(list.size() == 3);
        CHECK(to_vi(list) == vi{3, 2, 1});
        SECTION("and can remove them") {
            SECTION("from the front") {
                list.remove(list.front());
                CHECK(to_vi(list) == vi{2, 1});
                list.remove(list.front());
                CHECK(to_vi(list) == vi{1});
                list.remove(list.front());
                CHECK(to_vi(list).empty());
            }
            SECTION("from the middle") {
                list.remove(*std::next(list.begin()));
                CHECK(to_vi(list) == vi{3, 1});
            }
            SECTION("from the end") {
                list.remove(*std::next(list.begin(), 2));
                CHECK(to_vi(list) == vi{3, 2});
            }
        }
        SECTION("can remove them while iterating") {
            list.modification_safe_for_each([&list](const AFFECT_DATA &af) { list.remove(af); });
            CHECK(list.empty());
        }
    }
}
