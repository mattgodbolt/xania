#include "mask_hostname.hpp"

#include <catch2/catch.hpp>

TEST_CASE("mask hostnames") {
    SECTION("should handle empty hostnames") { CHECK(get_masked_hostname("") == "*** [#5381]"); }
    SECTION("should handle short hostnames") { CHECK(get_masked_hostname("lclh") == "lclh*** [#6385433576]"); }
    SECTION("should handle long hostnames") {
        CHECK(get_masked_hostname("ec2-192.123.112.33.aws-services.amazon.com") == "ec2-19*** [#18004094125380531885]");
    }
    SECTION("should handle just IPs") { CHECK(get_masked_hostname("127.0.0.1") == "127.0.*** [#249811315400949274]"); }
}