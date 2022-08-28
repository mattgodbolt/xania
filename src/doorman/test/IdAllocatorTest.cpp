#include "doorman/IdAllocator.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("Id allocator", "[IdAllocator]") {
    IdAllocator ia;
    SECTION("should hand out zero first") { CHECK(ia.reserve()->id() == 0); }
    SECTION("should hand out sequential numbers") {
        std::vector<IdAllocator::Reservation> reservations;
        for (auto i = 0u; i < 100u; ++i) {
            reservations.emplace_back(ia.reserve());
            REQUIRE(reservations.back()->id() == i);
        }
        SECTION("and releases") {
            reservations.clear(); // it's not specified which order the STL
            CHECK(ia.reserve()->id() < 100u);
        }
        SECTION("handing back the most recently used first") {
            for (auto &r : reservations)
                r.reset();
            CHECK(ia.reserve()->id() == 99u);
        }
    }
    SECTION("should keep reservations active out of order") {
        auto res0 = ia.reserve();
        auto res1 = ia.reserve();
        auto res2 = ia.reserve();
        SECTION("recycles the middle id") {
            res1.reset();
            CHECK(ia.reserve()->id() == 1);
        }
        SECTION("recycles the first id") {
            res0.reset();
            CHECK(ia.reserve()->id() == 0);
        }
        SECTION("recycles the last id") {
            res2.reset();
            CHECK(ia.reserve()->id() == 2);
        }
    }
    SECTION("should reference count references") {
        auto res = ia.reserve();
        auto res_copy = res;
        CHECK(ia.reserve()->id() == 1);
        res.reset();
        CHECK(ia.reserve()->id() == 1);
        res_copy.reset();
        CHECK(ia.reserve()->id() == 0);
    }
}
