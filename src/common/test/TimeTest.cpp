#include "common/Time.hpp"

#include <catch2/catch.hpp>

namespace {

TEST_CASE("Time tests", "[Time]") {
    auto test_time = Time(Seconds(953945168));
    CHECK(formatted_time(test_time) == "2000-03-25 00:46:08Z");
}

}
