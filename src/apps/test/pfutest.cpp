#include "pfu.hpp"
#include "MudImpl.hpp"
#include "common/Configuration.hpp"
#include "db.h"
#include "save.hpp"
#include "test/MemFile.hpp"
#include "test/fileutils.hpp"
#include "update.hpp"

#include <date/date.h>

#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>

using namespace pfu;

namespace {

spdlog::logger logger = spdlog::logger("pfutest");
std::unique_ptr<Mud> mud;

}

struct LoadTinyMudOnce : Catch::EventListenerBase {
    using Catch::EventListenerBase::EventListenerBase;

    void testRunStarting([[maybe_unused]] Catch::TestRunInfo const &testInfo) override {
        setenv(MUD_AREA_DIR_ENV, TEST_DATA_DIR "/area", 1);
        setenv(MUD_DATA_DIR_ENV, TEST_DATA_DIR "/data", 1);
        setenv(MUD_HTML_DIR_ENV, TEST_DATA_DIR "/html", 1);
        setenv(MUD_PORT_ENV, "9000", 1);
        mud = std::make_unique<MudImpl>();
        boot_db(*mud.get());
    }
    void testRunEnded([[maybe_unused]] Catch::TestRunStats const &testRunStats) override { collect_all_garbage(); }
};
CATCH_REGISTER_LISTENER(LoadTinyMudOnce)

TEST_CASE("collect player names") {
    SECTION("all non-empty player files found") {

        const auto names = collect_player_names(mud->config().player_dir(), logger);

        std::vector<std::string> expected = {"Versionfour"};
        REQUIRE(names == expected);
    }
}

TEST_CASE("register tasks") {
    SECTION("expected task count") {
        const auto all_tasks = register_tasks();

        REQUIRE(all_tasks.size() == 2);
    }
}

TEST_CASE("upgrade player") {
    SECTION("v4 to latest") {
        const auto all_tasks = register_tasks();
        test::MemFile god_file;
        test::MemFile player_file;
        const CharSaver saver;
        auto result = upgrade_player(
            *mud.get(), "Versionfour", all_tasks, logger,
            [&god_file, &player_file, &saver](const Char &ch) { saver.save(ch, god_file.file(), player_file.file()); });
        REQUIRE(result == true);
        SECTION("upgraded file matches expected") {
            auto expected = test::read_whole_file(mud->config().player_dir() + "/expected-upgrades/Versionfour");
            auto actual = player_file.as_string_view();

            CHECK(actual == expected);
            if (actual != expected)
                test::diff_mismatch(actual, expected);
        }
    }
}

TEST_CASE("login time format parsing") {
    SECTION("valid timestamps") {
        using namespace date::literals;
        using namespace std::literals;
        const auto expected = date::sys_days(2000_y / date::February / 22_d) + 10h + 33min + 21s;
        std::string last_login = GENERATE("Tue Feb 22 10:33:21 2000", "2000-02-22 10:33:21", "2000-02-22 10:33:21Z");

        SECTION("are parsed and converted") {
            auto parsed = try_parse_login_at(last_login);

            CHECK(parsed == expected);
        }
    }
    SECTION("unsupported timestamp ignored") {
        std::string last_login = "Feb 22 10:33:21 2000";
        auto parsed = try_parse_login_at(last_login);

        REQUIRE(!parsed);
    }
    SECTION("empty timestamp ignored") {
        std::string last_login;
        auto parsed = try_parse_login_at(last_login);

        REQUIRE(!parsed);
    }
}

TEST_CASE("required tasks") {
    ResetModifiableAttrs task(CharVersion::Five);
    Char ch(*mud);
    SECTION("required") {
        ch.version = CharVersion::Four;
        auto required = task.is_required(ch);

        REQUIRE(required);
    }
    SECTION("not required") {
        ch.version = CharVersion::Five;
        auto required = task.is_required(ch);

        REQUIRE(!required);
    }
}
