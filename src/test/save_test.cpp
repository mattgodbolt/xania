#include "common/Configuration.hpp"
#include "save.hpp"

#include "MemFile.hpp"
#include "merc.h"
#include "string_utils.hpp"

#include <catch2/catch.hpp>

using namespace std::literals;

namespace {

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

TEST_CASE("loading and saving player files") {
    // For now: prevent us from loading multiple times, as there's no tear-down, and it takes ages.
    static bool massive_hack = false;
    if (!massive_hack) {
        // boot_db() uses Configuration and that expects some path env vars to point
        // to 'area', 'player', 'html' etc. As the env vars can be relative paths,
        // just point them at the test data dir, or dirs within it. The 'player' dir under
        // TEST_DATA_DIR is where the test pfiles reside.
        REQUIRE(setenv(MUD_AREA_DIR_ENV, TEST_DATA_DIR "/area", 1) == 0);
        REQUIRE(setenv(MUD_DATA_DIR_ENV, TEST_DATA_DIR, 1) == 0);
        REQUIRE(setenv(MUD_HTML_DIR_ENV, TEST_DATA_DIR, 1) == 0);
        boot_db();
        massive_hack = true;
    }
    SECTION("should be able to load a char") {
        auto res = try_load_player("Khirsah");
        CHECK(!res.newly_created);
        REQUIRE(res.character);
        auto *ch = res.character.get();
        CHECK(ch->name == "Khirsah");
        CHECK(ch->is_pc());
        CHECK(ch->in_room->vnum == 30005);
    }
    SECTION("should create a new char") {
        auto res = try_load_player("Noobie");
        CHECK(res.newly_created);
        REQUIRE(res.character);
        auto *ch = res.character.get();
        CHECK(ch->name == "Noobie");
        CHECK(ch->is_pc());
        CHECK(!ch->in_room);
    }
    SECTION("should roundtrip characters") {
        auto name = GENERATE("Khirsah"s, "TheMoog"s);
        auto res = try_load_player(name);
        auto *ch = res.character.get();
        test::MemFile mem_file;
        save_char_obj(ch, mem_file.file());
        SECTION("should vaguely look like a pfile") {
            CHECK(matches_start(fmt::format("#PLAYER\nName {}~\n", name), mem_file.as_string_view()));
        }
        // This works as we deliberately strip the Last* things from the pfiles in the test dir.
        SECTION("and it should save (nearly) exactly") {
            std::string expected = read_whole_file(TEST_DATA_DIR "/player/" + initial_caps_only(name));
            // remove all `\r`s - bit of a workaround. Don't fully understand what's going on,
            // but the loader currently ignores then, and replaces all \n with \n\r anyway. Seems
            // likely there's some newline conversion going on in mem_file that isn't happening in
            // real files. Or vice versa.
            auto replaced = replace_strings(std::string(mem_file.as_string_view()), "\r", "");
            CHECK(replaced == expected);
            if (replaced != expected)
                diff_mismatch(replaced, expected);
        }
    }
}
