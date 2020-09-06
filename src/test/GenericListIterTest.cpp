#include "GenericListIter.hpp"

#include <catch2/catch.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/subrange.hpp>
#include <range/v3/view/transform.hpp>
#include <vector>

namespace {

struct Thing {
    int ordinal{};
    Thing *next{};
    Thing *next_in_room{};
};

struct NextInRoom {
    Thing *operator()(Thing *thing) { return thing->next_in_room; };
};

}

TEST_CASE("Generic list iterator") {
    // first and last are "in room"
    Thing first{1};
    Thing second{2};
    Thing third{3};
    Thing last{4};
    first.next = &second;
    second.next = &third;
    third.next = &last;
    first.next_in_room = &last;
    SECTION("should visit using next by default") {
        auto all = ranges::subrange(GenericListIter<Thing>(&first), GenericListIter<Thing>());
        CHECK((all | ranges::view::transform([](const Thing &t) { return t.ordinal; }) | ranges::to<std::vector>)
              == std::vector<int>{1, 2, 3, 4});
    }
    SECTION("should be able to visit using a non-default next") {
        auto in_room =
            ranges::subrange(GenericListIter<Thing, NextInRoom>(&first), GenericListIter<Thing, NextInRoom>());
        CHECK((in_room | ranges::view::transform([](const Thing &t) { return t.ordinal; }) | ranges::to<std::vector>)
              == std::vector<int>{1, 4});
    }
    SECTION("should handle empty lists") {
        auto all = ranges::subrange(GenericListIter<Thing>(nullptr), GenericListIter<Thing>());
        CHECK(ranges::distance(all) == 0);
    }
}