#include "Configuration.hpp"

#include <catch2/catch.hpp>

TEST_CASE("Configuration initialized") {

    REQUIRE(setenv(MUD_AREA_DIR_ENV, TEST_DATA_DIR "/area", 1) == 0);
    REQUIRE(setenv(MUD_DATA_DIR_ENV, TEST_DATA_DIR "/data", 1) == 0);
    REQUIRE(setenv(MUD_HTML_DIR_ENV, TEST_DATA_DIR "/html", 1) == 0);
    Configuration config;

    SECTION("area dir") {
        auto dir = config.area_dir();

        REQUIRE(dir == TEST_DATA_DIR "/area/");
    }
    SECTION("html dir") {
        auto dir = config.html_dir();

        REQUIRE(dir == TEST_DATA_DIR "/html/");
    }
    SECTION("player dir") {
        auto dir = config.player_dir();

        REQUIRE(dir == TEST_DATA_DIR "/data/player/");
    }
    SECTION("system dir") {
        auto dir = config.system_dir();

        REQUIRE(dir == TEST_DATA_DIR "/data/system/");
    }
    SECTION("gods dir") {
        auto dir = config.gods_dir();

        REQUIRE(dir == TEST_DATA_DIR "/data/gods/");
    }
    SECTION("area file") {
        auto file = config.area_file();

        REQUIRE(file == TEST_DATA_DIR "/area/area.lst");
    }
    SECTION("tip file") {
        auto file = config.tip_file();

        REQUIRE(file == TEST_DATA_DIR "/area/tipfile.txt");
    }
    SECTION("chat data file") {
        auto file = config.chat_data_file();

        REQUIRE(file == TEST_DATA_DIR "/area/chat.data");
    }
    SECTION("ban file") {
        auto file = config.ban_file();

        REQUIRE(file == TEST_DATA_DIR "/data/system/ban.lst");
    }
    SECTION("bug file") {
        auto file = config.bug_file();

        REQUIRE(file == TEST_DATA_DIR "/data/system/bugs.txt");
    }
    SECTION("typo file") {
        auto file = config.typo_file();

        REQUIRE(file == TEST_DATA_DIR "/data/system/typos.txt");
    }
    SECTION("ideas file") {
        auto file = config.ideas_file();

        REQUIRE(file == TEST_DATA_DIR "/data/system/ideas.txt");
    }
    SECTION("notes file") {
        auto file = config.notes_file();

        REQUIRE(file == TEST_DATA_DIR "/data/system/notes.txt");
    }
}

TEST_CASE("Missing directory") {

    REQUIRE(setenv(MUD_AREA_DIR_ENV, TEST_DATA_DIR "/missing", 1) == 0);

    SECTION("Throws exception") {
        REQUIRE_THROWS_WITH(Configuration(),
                            "An environment variable called MUD_AREA_DIR must specify a directory path");
    }
}

TEST_CASE("Missing path env var") {

    REQUIRE(unsetenv(MUD_AREA_DIR_ENV) == 0);

    SECTION("Throws exception") {
        REQUIRE_THROWS_WITH(Configuration(),
                            "An environment variable called MUD_AREA_DIR must specify a directory path");
    }
}
