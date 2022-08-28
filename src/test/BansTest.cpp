/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Ban.hpp"
#include "Char.hpp"
#include "MemFile.hpp"

#include <catch2/catch_test_macros.hpp>

namespace {

class TestDependencies : public Bans::Dependencies {
public:
    TestDependencies(test::MemFile &file) : file_(file) {}
    FILE *open_read() {
        file_.rewind();
        return file_.file();
    }
    FILE *open_write() {
        file_.truncate();
        return file_.file();
    }
    // Don't close the MemFile, it'll be closed on destruction.
    void close([[maybe_unused]] FILE *fp) {}
    // Unlink for MemFile is nonsensical.
    void unlink() {}

private:
    test::MemFile &file_;
};

void assert_ban_site_result(const Descriptor &desc, const bool result, const bool expected_result,
                            std::string_view expected_output, test::MemFile &file,
                            std::string_view expected_file_body) {
    CHECK(result == expected_result);
    CHECK(desc.buffered_output() == expected_output);
    CHECK(file.as_string_view() == expected_file_body);
}

void assert_ban_site_success(const Descriptor &desc, const bool result, test::MemFile &file) {
    assert_ban_site_result(desc, result, true, "\n\rBans updated.\n\r", file, "");
}

void assert_ban_site_success(const Descriptor &desc, const bool result, test::MemFile &file,
                             std::string_view expected_file_body) {
    assert_ban_site_result(desc, result, true, "\n\rBans updated.\n\r", file, expected_file_body);
}

}

TEST_CASE("bans") {
    Char admin{};
    Descriptor admin_desc(0);
    admin.desc = &admin_desc;
    admin_desc.character(&admin);
    admin.pcdata = std::make_unique<PcData>();
    admin.level = 100;

    test::MemFile initially_empty_file;
    test::MemFile one_ban_file(R"(localhost 100 AF
)");
    test::MemFile two_bans_file(R"(localhost 100 AF
straylight.net 95 AF
)");
    test::MemFile one_ban_wild_suffix_file(R"(local 100 AEF
)");
    test::MemFile one_ban_wild_prefix_file(R"(straylight.net 95 ADF
)");
    test::MemFile one_ban_newbies_file(R"(localhost 100 AB
)");
    test::MemFile one_ban_permitted_file(R"(localhost 100 AC
)");

    SECTION("load") {
        SECTION("empty file") {
            TestDependencies dependencies(initially_empty_file);
            Bans bans(dependencies);

            CHECK(bans.load() == 0);
        }
        SECTION("1 ban") {
            TestDependencies dependencies(one_ban_file);
            Bans bans(dependencies);

            CHECK(bans.load() == 1);
        }
        SECTION("2 bans") {
            TestDependencies dependencies(two_bans_file);
            Bans bans(dependencies);

            CHECK(bans.load() == 2);
        }
    }
    SECTION("ban site") {
        SECTION("temporary, no wildcard, all") {
            TestDependencies dependencies(initially_empty_file);
            Bans bans(dependencies);
            ArgParser args("straylight.net all");

            const auto result = bans.ban_site(&admin, args, false);
            assert_ban_site_success(admin_desc, result, initially_empty_file);
        }
        SECTION("temporary, prefix wildcard, all") {
            TestDependencies dependencies(initially_empty_file);
            Bans bans(dependencies);
            ArgParser args("*straylight.net all");

            const auto result = bans.ban_site(&admin, args, false);

            assert_ban_site_success(admin_desc, result, initially_empty_file);
        }
        SECTION("temporary, no wildcard, newbies") {
            TestDependencies dependencies(initially_empty_file);
            Bans bans(dependencies);
            ArgParser args("straylight.net newbies");

            const auto result = bans.ban_site(&admin, args, false);

            assert_ban_site_success(admin_desc, result, initially_empty_file);
        }
        SECTION("temporary, no wildcard, permit") {
            TestDependencies dependencies(initially_empty_file);
            Bans bans(dependencies);
            ArgParser args("straylight.net permit");

            const auto result = bans.ban_site(&admin, args, false);

            assert_ban_site_success(admin_desc, result, initially_empty_file);
        }
        SECTION("temporary, bad type") {
            TestDependencies dependencies(initially_empty_file);
            Bans bans(dependencies);
            ArgParser args("straylight.net immortals");

            const auto result = bans.ban_site(&admin, args, false);

            assert_ban_site_result(admin_desc, result, false,
                                   "\n\rAcceptable ban types are 'all', 'newbies', and 'permit'.\n\r",
                                   initially_empty_file, "");
        }
        SECTION("temporary, overrule existing ban") {
            TestDependencies dependencies(one_ban_file);
            Bans bans(dependencies);
            ArgParser args("localhost all");
            bans.load();

            const auto result = bans.ban_site(&admin, args, false);

            // The new ban, although the same site as the existng one, is being created
            // as temporary so it will not appear in the file afterwards.
            assert_ban_site_success(admin_desc, result, one_ban_file);
        }
        SECTION("temporary, existing ban overrules") {
            TestDependencies dependencies(one_ban_file);
            Bans bans(dependencies);
            ArgParser args("localhost all");
            bans.load();
            // Reduce admin level to test the level check.
            admin.level = 99;

            const auto result = bans.ban_site(&admin, args, false);

            assert_ban_site_result(admin_desc, result, false, "\n\rThat ban was set by a higher power.\n\r",
                                   one_ban_file, "localhost 100 AF\n");
        }
        SECTION("permanent, wildcard, all") {
            TestDependencies dependencies(initially_empty_file);
            Bans bans(dependencies);
            ArgParser args("*straylight.net all");

            const auto result = bans.ban_site(&admin, args, true);

            assert_ban_site_success(admin_desc, result, initially_empty_file, "straylight.net 100 ADF\n");
        }
    }
    SECTION("add with no site lists existing") {
        TestDependencies dependencies(two_bans_file);
        Bans bans(dependencies);
        bans.load();
        ArgParser args("");

        const auto result = bans.ban_site(&admin, args, false);

        CHECK(!result);
        CHECK(admin_desc.buffered_output()
              == "\n\r No.  Banned sites       Level  Type     Status\n\r  0)  localhost            100  all      "
                 "perm\n\r  1)  straylight.net       95   all      perm\n\r");
    }
    SECTION("allow site") {
        SECTION("allow index 1") {
            TestDependencies dependencies(two_bans_file);
            Bans bans(dependencies);
            bans.load();
            ArgParser args("1");

            const auto result = bans.allow_site(&admin, args);

            CHECK(result);
            CHECK(admin_desc.buffered_output() == "\n\rBans updated.\n\r");
            const auto sv = two_bans_file.as_string_view();
            CHECK(sv == "localhost 100 AF\n");
        }
        SECTION("allow index out of range") {
            TestDependencies dependencies(two_bans_file);
            Bans bans(dependencies);
            bans.load();
            ArgParser args("2");

            const auto result = bans.allow_site(&admin, args);

            CHECK(!result);
            CHECK(admin_desc.buffered_output() == "\n\rPlease specify a value between 0 and 1.\n\r");
            const auto sv = two_bans_file.as_string_view();
            CHECK(sv == "localhost 100 AF\nstraylight.net 95 AF\n");
        }
        SECTION("allow invalid index") {
            TestDependencies dependencies(two_bans_file);
            Bans bans(dependencies);
            bans.load();
            ArgParser args("localhost"); // allow requires an index, not a hostname

            const auto result = bans.allow_site(&admin, args);

            CHECK(!result);
            CHECK(admin_desc.buffered_output() == "\n\rPlease specify the number of the ban you wish to lift.\n\r");
        }
    }
    SECTION("check ban") {
        SECTION("not banned") {
            TestDependencies dependencies(one_ban_file);
            Bans bans(dependencies);
            CHECK(bans.load() == 1);

            const auto result = bans.check_ban("desiderata.net", BanFlag::All);

            CHECK(!result);
        }
        SECTION("case insensitive") {
            TestDependencies dependencies(one_ban_file);
            Bans bans(dependencies);
            CHECK(bans.load() == 1);

            const auto result = bans.check_ban("LOCALHOST", BanFlag::All);

            CHECK(result);
        }
        SECTION("newbies, but not all, or either") {
            TestDependencies dependencies(one_ban_newbies_file);
            Bans bans(dependencies);
            CHECK(bans.load() == 1);

            const auto all_result = bans.check_ban("localhost", BanFlag::All);

            CHECK(!all_result);

            const auto newbies_result = bans.check_ban("localhost", BanFlag::Newbies);

            CHECK(newbies_result);

            const auto either_result = bans.check_ban("localhost", to_int(BanFlag::All) | to_int(BanFlag::Newbies));

            CHECK(either_result);
        }
        SECTION("by site prefix") {
            TestDependencies dependencies(one_ban_wild_prefix_file);
            Bans bans(dependencies);
            CHECK(bans.load() == 1);

            const auto result = bans.check_ban("ashpool.straylight.net", BanFlag::All);

            CHECK(result);
        }
        SECTION("not by site prefix") {
            TestDependencies dependencies(one_ban_wild_prefix_file);
            Bans bans(dependencies);
            CHECK(bans.load() == 1);

            const auto result = bans.check_ban("straylight.net.bogus", BanFlag::All);

            CHECK(!result);
        }
        SECTION("by site suffix") {
            TestDependencies dependencies(one_ban_wild_suffix_file);
            Bans bans(dependencies);
            CHECK(bans.load() == 1);

            const auto result = bans.check_ban("localhost", BanFlag::All);

            CHECK(result);
        }
        SECTION("no longer banned after allowed") {
            TestDependencies dependencies(one_ban_file);
            Bans bans(dependencies);
            ArgParser args("0");
            CHECK(bans.load() == 1);

            const auto allow_result = bans.allow_site(&admin, args);

            CHECK(allow_result);

            const auto check_result = bans.check_ban("localhost", BanFlag::All);

            CHECK(!check_result);
        }
        SECTION("permitted players only") {
            TestDependencies dependencies(one_ban_permitted_file);
            Bans bans(dependencies);
            CHECK(bans.load() == 1);

            const auto all_result = bans.check_ban("localhost", BanFlag::All);

            CHECK(!all_result);

            const auto newbies_result = bans.check_ban("localhost", BanFlag::Newbies);

            CHECK(!newbies_result);

            const auto permitted_result = bans.check_ban("localhost", BanFlag::Permit);

            CHECK(permitted_result);
        }
    }
}
