/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2021 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/

#include "Flag.hpp"
#include "AffectFlag.hpp"
#include "Constants.hpp"
#include "FlagFormat.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectWearFlag.hpp"
#include "RoomFlag.hpp"
#include "WeaponFlag.hpp"
#include "common/StandardBits.hpp"

#include <catch2/catch.hpp>

namespace {

enum class TestFlag {
    Flag1 = A,
    Flag2 = B,
    Flag3 = C
};

// TODO this may be moving into Flag 
constexpr auto to_int(const TestFlag flag) noexcept {
    return magic_enum::enum_integer<TestFlag>(flag);
}

const Flag<TestFlag> Flag1{TestFlag::Flag1, "flag1"};
const Flag<TestFlag> Flag2{TestFlag::Flag2, "flag2"};
const Flag<TestFlag> Flag3{TestFlag::Flag3, "flag3", 100};

const std::array<Flag<TestFlag>, 3> AllTestFlags = {{
    Flag1,
    Flag2,
    Flag3
}};

}

TEST_CASE("serialize flags") {
    SECTION("no flags set") {
        const auto result = serialize_flags(0);

        CHECK(result == "0");
    }
    SECTION("A or B") {
        const auto result = serialize_flags(A | B);

        CHECK(result == "AB");
    }
    SECTION("Z or a or f") {
        const auto result = serialize_flags(Z | aa | ff);

        CHECK(result == "Zaf");
    }
    SECTION("all flags") {
        auto all_bits = ~(0u);
        const auto result = serialize_flags(all_bits);

        CHECK(result == "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdef");
    }
}

TEST_CASE("get flag bit by name") {
    SECTION("first") {
        auto result = get_flag_bit_by_name(AllTestFlags, "flag1", 1);

        CHECK(result == A);
    }
    SECTION("second") {
        auto result = get_flag_bit_by_name(AllTestFlags, "flag2", 1);

        CHECK(result == B);
    }
    SECTION("third not found due to trust") {
        auto result = get_flag_bit_by_name(AllTestFlags, "flag3", 99);

        CHECK(!result);
    }
}

TEST_CASE("Flag type") {
    SECTION("flag1 members") {
        CHECK(Flag1.bit == 1);
        CHECK(Flag1.enum_value == TestFlag::Flag1);
        CHECK(Flag1.min_level == 0);
        CHECK(Flag1.name == "flag1");
    }
    SECTION("flag2 members") {
        CHECK(Flag2.bit == 2);
        CHECK(Flag2.enum_value == TestFlag::Flag2);
        CHECK(Flag2.min_level == 0);
        CHECK(Flag2.name == "flag2");
    }
    SECTION("flag3 members") {
        CHECK(Flag3.bit == 4);
        CHECK(Flag3.enum_value == TestFlag::Flag3);
        CHECK(Flag3.min_level == 100);
        CHECK(Flag3.name == "flag3");
    }
}

TEST_CASE("flag set") {
    Char admin{};
    Descriptor admin_desc(0);
    admin.desc = &admin_desc;
    admin_desc.character(&admin);
    admin.pcdata = std::make_unique<PcData>();
    admin.level = 99;
    unsigned long current_val = 0;
    SECTION("flag1 toggled on") {
        auto args = ArgParser("flag1");
        auto result = flag_set(AllTestFlags, args, current_val, &admin);

        CHECK(check_bit(result, A));
        CHECK(admin_desc.buffered_output() == "\n\rflag1\n\r");
    }
    SECTION("flag1 toggled") {
        set_bit(current_val, A);
        auto args = ArgParser("flag1");
        auto result = flag_set(AllTestFlags, args, current_val, &admin);

        CHECK(!check_bit(result, A));
        CHECK(admin_desc.buffered_output() == "\n\rnone\n\r");
    }
    SECTION("flag1 set on") {
        auto args = ArgParser("+flag1");
        auto result = flag_set(AllTestFlags, args, current_val, &admin);

        CHECK(check_bit(result, A));
        CHECK(admin_desc.buffered_output() == "\n\rflag1\n\r");
    }
    SECTION("flag1 set off") {
        set_bit(current_val, A);
        auto args = ArgParser("-flag1");
        auto result = flag_set(AllTestFlags, args, current_val, &admin);

        CHECK(!check_bit(result, A));
        CHECK(admin_desc.buffered_output() == "\n\rnone\n\r");
    }
    SECTION("unrecognized flag name") {
        auto args = ArgParser("flag0");
        auto result = flag_set(AllTestFlags, args, current_val, &admin);

        CHECK(result == 0);
        CHECK(admin_desc.buffered_output() == "\n\rnone\n\rAvailable flags:\n\rflag1 flag2\n\r");
    }
    SECTION("insufficient trust for flag") {
        auto args = ArgParser("flag3");
        auto result = flag_set(AllTestFlags, args, current_val, &admin);

        CHECK(result == 0);
        CHECK(admin_desc.buffered_output() == "\n\rnone\n\rAvailable flags:\n\rflag1 flag2\n\r");
    }
}
// See CharTest for similar coverage of ToleranceFlag, PlayerActFlag, CharActFlag, MorphologyFlag,
// OffensiveFlag and BodyPartFlag.
TEST_CASE("misc format set flags") {
    auto flags = 0ul;
    Char bob{};
    bob.level = 1;
    SECTION("all room flags") {
        SECTION("none set") {
            auto result = format_set_flags(RoomFlags::AllRoomFlags, flags);

            CHECK(result == "none");
        }
        SECTION("all set no char") {
            flags = ~(0ul);
            auto result = format_set_flags(RoomFlags::AllRoomFlags, flags);

            CHECK(result
                  == "|Cdark nomob indoors private safe solitary petshop recall imponly godonly heroonly newbieonly "
                     "law|w");
        }
        SECTION("all set as mortal") {
            flags = ~(0ul);
            auto result = format_set_flags(RoomFlags::AllRoomFlags, &bob, flags);

            CHECK(result == "|Cdark nomob indoors private safe solitary petshop recall heroonly newbieonly law|w");
        }
        SECTION("all set as immortal") {
            bob.level = LEVEL_IMMORTAL;
            flags = ~(0ul);
            auto result = format_set_flags(RoomFlags::AllRoomFlags, &bob, flags);

            CHECK(result
                  == "|Cdark nomob indoors private safe solitary petshop recall godonly heroonly newbieonly law|w");
        }
        SECTION("all set as implementor") {
            bob.level = MAX_LEVEL;
            flags = ~(0ul);
            auto result = format_set_flags(RoomFlags::AllRoomFlags, &bob, flags);

            CHECK(result
                  == "|Cdark nomob indoors private safe solitary petshop recall imponly godonly heroonly newbieonly "
                     "law|w");
        }
    }
    SECTION("all weapon flags") {
        SECTION("none set") {
            auto result = format_set_flags(WeaponFlags::AllWeaponFlags, flags);

            CHECK(result == "none");
        }
        SECTION("all set") {
            flags = ~(0ul);
            auto result = format_set_flags(WeaponFlags::AllWeaponFlags, flags);

            CHECK(result == "|Cflaming frost vampiric sharp vorpal two-handed poisoned plagued lightning acid|w");
        }
    }
    SECTION("all affect flags") {
        SECTION("none set") {
            auto result = format_set_flags(AffectFlags::AllAffectFlags, flags);

            CHECK(result == "none");
        }
        SECTION("all set") {
            flags = ~(0ul);
            auto result = format_set_flags(AffectFlags::AllAffectFlags, flags);

            CHECK(
                result
                == "|Cblind invisible detect_evil detect_invis detect_magic detect_hidden talon sanctuary faerie_fire "
                   "infrared curse protection_evil poison protection_good sneak hide sleep charm flying pass_door "
                   "haste calm plague weaken dark_vision berserk swim regeneration octarine_fire lethargy|w");
        }
    }
    SECTION("all object wear flags") {
        SECTION("none set") {
            auto result = format_set_flags(ObjectWearFlags::AllObjectWearFlags, flags);

            CHECK(result == "none");
        }
        SECTION("all set") {
            flags = ~(0ul);
            auto result = format_set_flags(ObjectWearFlags::AllObjectWearFlags, flags);

            CHECK(result
                  == "|Ctake finger neck body head legs feet hands arms shield about waist wrist wield hold twohands "
                     "ears|w");
        }
    }
    SECTION("all object extra flags") {
        SECTION("none set") {
            auto result = format_set_flags(ObjectExtraFlags::AllObjectExtraFlags, flags);

            CHECK(result == "none");
        }
        SECTION("all set") {
            flags = ~(0ul);
            auto result = format_set_flags(ObjectExtraFlags::AllObjectExtraFlags, flags);

            CHECK(result
                  == "|Cglow hum dark lock evil invis magic nodrop bless antigood antievil antineutral noremove "
                     "inventory nopurge rotdeath visdeath protected nolocate summon_corpse unique|w");
        }
    }
}