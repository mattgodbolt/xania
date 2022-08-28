#pragma once

#include <catch2/catch_test_macros.hpp>

// Some helper functions for dealing with test data files.
namespace test {

std::string read_whole_file(const std::string &name) {
    INFO("reading whole file " << name);
    auto fp = ::fopen(name.c_str(), "rb");
    REQUIRE(fp);
    ::fseek(fp, 0, SEEK_END);
    auto len = ::ftell(fp);
    REQUIRE(len > 0);
    size_t len_ok = (size_t)len;
    ::fseek(fp, 0, SEEK_SET);
    std::string result;
    result.resize(len);
    REQUIRE(::fread(result.data(), 1, len, fp) == len_ok);
    ::fclose(fp);
    return result;
}

void write_file(const std::string &filename, std::string_view data) {
    INFO("writing file " << filename);
    auto fp = ::fopen(filename.c_str(), "wb");
    REQUIRE(fp);
    REQUIRE(::fwrite(data.data(), 1, data.size(), fp) == data.size());
    ::fclose(fp);
}

// short up deficiencies in diff reporting.
void diff_mismatch(std::string_view lhs, std::string_view rhs) {
    // If you find yourself debugging, set this ENV var to run meld.
    if (!::getenv("MELD_ON_DIFF"))
        return;
    write_file("/tmp/lhs", lhs);
    write_file("/tmp/rhs", rhs);
    CHECK(system("meld /tmp/lhs /tmp/rhs") == 0);
}

}
