#include "AffectList.hpp"

#include <catch2/catch_test_macros.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>

#include <vector>

TEST_CASE("Affect lists") {
    AffectList l;
    SECTION("should start empty") {
        CHECK(l.empty());
        CHECK(l.size() == 0);
    }
    auto mk_test = [](int i) {
        AFFECT_DATA ad;
        ad.level = i;
        return ad;
    };
    SECTION("should keep most recent first") {
        l.add(mk_test(1));
        l.add(mk_test(2));
        l.add(mk_test(3));
        l.add(mk_test(4));
        CHECK(l.size() == 4);
        auto res = l | ranges::views::transform([](const auto &af) { return af.level; }) | ranges::to<std::vector<int>>;
        CHECK(res == std::vector<int>{4, 3, 2, 1});
    }
    SECTION("should allow add_at_end") {
        l.add_at_end(mk_test(1));
        l.add_at_end(mk_test(2));
        l.add_at_end(mk_test(3));
        l.add_at_end(mk_test(4));
        CHECK(l.size() == 4);
        auto res = l | ranges::views::transform([](const auto &af) { return af.level; }) | ranges::to<std::vector<int>>;
        CHECK(res == std::vector<int>{1, 2, 3, 4});
    }
}
