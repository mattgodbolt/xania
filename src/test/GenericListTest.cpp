#include "GenericList.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/transform.hpp>
#include <vector>

namespace {

struct Thing {
    int id{};
    explicit Thing(int id) : id(id) {}
};

}

TEST_CASE("Generic list") {
    using vi = std::vector<int>;
    auto ids_of = [](auto &list) { return list | ranges::views::transform(&Thing::id) | ranges::to<std::vector>; };

    GenericList<Thing *> four_things;
    Thing first{1};
    Thing second{2};
    Thing third{3};
    Thing last{4};
    four_things.add_back(&first);
    four_things.add_back(&second);
    four_things.add_back(&third);
    four_things.add_back(&last);

    SECTION("iteration") { CHECK(ids_of(four_things) == vi{1, 2, 3, 4}); }

    SECTION("size and emptiness") {
        CHECK(four_things.size() == 4);
        CHECK(!four_things.empty());
    }

    SECTION("empty lists") {
        GenericList<Thing *> empty;
        CHECK(ranges::distance(empty) == 0);
        CHECK(begin(empty) == end(empty));
        CHECK(ids_of(empty) == vi{});
        CHECK(empty.size() == 0);
        CHECK(empty.empty());
    }

    SECTION("removing") {

        SECTION("with no other iterators active") {
            SECTION("first elem") {
                four_things.remove(&first);
                CHECK(four_things.debug_count_all_nodes() == 3);
                CHECK(ids_of(four_things) == vi{2, 3, 4});
            }
            SECTION("second elem") {
                four_things.remove(&second);
                CHECK(four_things.debug_count_all_nodes() == 3);
                CHECK(ids_of(four_things) == vi{1, 3, 4});
            }
            SECTION("last elem") {
                four_things.remove(&last);
                CHECK(four_things.debug_count_all_nodes() == 3);
                CHECK(ids_of(four_things) == vi{1, 2, 3});
            }
        }

        SECTION("with an iterator active") {
            SECTION("first elem") {
                {
                    auto first_it = four_things.begin();
                    four_things.remove(&first);
                    CHECK(ids_of(four_things) == vi{2, 3, 4});
                    CHECK(four_things.debug_count_all_nodes() == 4);
                }
                CHECK(four_things.debug_count_all_nodes() == 3);
            }
            SECTION("second elem") {
                {
                    auto second_it = std::next(four_things.begin());
                    four_things.remove(&second);
                    CHECK(ids_of(four_things) == vi{1, 3, 4});
                    CHECK(four_things.debug_count_all_nodes() == 4);
                }
                CHECK(four_things.debug_count_all_nodes() == 3);
            }
            SECTION("last elem") {
                {
                    auto last_it = std::next(four_things.begin(), 3);
                    four_things.remove(&last);
                    CHECK(ids_of(four_things) == vi{1, 2, 3});
                    CHECK(four_things.debug_count_all_nodes() == 4);
                }
                CHECK(four_things.debug_count_all_nodes() == 3);
            }
        }
        SECTION("removing the current node while iterating") {
            vi visited;
            for (auto thing : four_things) {
                visited.push_back(thing->id);
                four_things.remove(thing);
            }
            CHECK(visited == vi{1, 2, 3, 4});
        }
        SECTION("removing the next node while iterating") {
            vi visited;
            for (auto thing : four_things) {
                visited.push_back(thing->id);
                auto it = ranges::find_if(
                    four_things, [id = thing->id + 1](const Thing *maybe_next) { return maybe_next->id == id; });
                REQUIRE(it != four_things.end());
                four_things.remove(*it);
            }
            CHECK(visited == vi{1, 3});
        }
    }
}

TEST_CASE("Generic list of unique_ptr") {
    using vi = std::vector<int>;
    auto to_id = [](auto const &entry) -> int { return entry->id; };
    auto ids_of = [&to_id](auto &list) { return list | ranges::views::transform(to_id) | ranges::to<std::vector>; };

    GenericList<std::unique_ptr<Thing>> list;
    auto up0 = std::make_unique<Thing>(9);
    auto p0 = up0.get();
    auto up1 = std::make_unique<Thing>(10);
    auto p1 = up1.get();
    auto up2 = std::make_unique<Thing>(11);
    auto p2 = up2.get();
    list.add_back(std::move(up0));
    list.add_back(std::move(up1));

    SECTION("remove pointer") {
        SECTION("first elem") {
            list.remove_pointer(p0);
            CHECK(list.debug_count_all_nodes() == 1);
            CHECK(ids_of(list) == vi{10});
        }
        SECTION("second elem") {
            list.remove_pointer(p1);
            CHECK(list.debug_count_all_nodes() == 1);
            CHECK(ids_of(list) == vi{9});
        }
        SECTION("not present") {
            list.remove_pointer(p2);
            CHECK(list.debug_count_all_nodes() == 2);
            CHECK(ids_of(list) == vi{9, 10});
        }
    }
}
