#include "Char.hpp"
#include "MemFile.hpp"
#include "Object.hpp"
#include "ObjectType.hpp"
#include "Room.hpp"
#include "common/Configuration.hpp"
#include "fileutils.hpp"
#include "handler.hpp"
#include "save.hpp"
#include "string_utils.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

using namespace std::literals;

extern void boot_db();
extern void delete_globals_on_shutdown();

struct LoadTinyMudOnce : Catch::EventListenerBase {
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting([[maybe_unused]] Catch::TestRunInfo const &testInfo) override {
        setenv(MUD_AREA_DIR_ENV, TEST_DATA_DIR "/area", 1);
        setenv(MUD_DATA_DIR_ENV, TEST_DATA_DIR, 1);
        setenv(MUD_HTML_DIR_ENV, TEST_DATA_DIR, 1);
        setenv(MUD_PORT_ENV, "9000", 1);
        boot_db();
    }
    void testRunEnded([[maybe_unused]] Catch::TestRunStats const &testRunStats) override {
        delete_globals_on_shutdown();
    }
};
CATCH_REGISTER_LISTENER(LoadTinyMudOnce)

TEST_CASE("loading and saving player files") {
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
            std::string expected = test::read_whole_file(TEST_DATA_DIR "/player/" + initial_caps_only(name));
            // remove all `\r`s - bit of a workaround. Don't fully understand what's going on,
            // but the loader currently ignores then, and replaces all \n with \n\r anyway. Seems
            // likely there's some newline conversion going on in mem_file that isn't happening in
            // real files. Or vice versa.
            auto replaced = replace_strings(std::string(mem_file.as_string_view()), "\r", "");
            CHECK(replaced == expected);
            if (replaced != expected)
                test::diff_mismatch(replaced, expected);
        }
    }
}
