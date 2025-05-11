#include "Char.hpp"
#include "AffectFlag.hpp"
#include "CharActFlag.hpp"
#include "PlayerActFlag.hpp"
#include "Room.hpp"
#include "lookup.h"

#include <catch2/catch_test_macros.hpp>

#include <string_view>

#include "MockMud.hpp"

using namespace std::literals;

namespace {

test::MockMud mock_mud{};

}

TEST_CASE("Character tests", "[Char]") {
    Room some_room;
    Char bob{mock_mud};
    bob.in_room = &some_room;
    bob.name = "Bob";
    bob.description = "Bob the blacksmith";

    SECTION("should describe correctly") {
        Char other{mock_mud};
        other.in_room = &some_room;
        other.name = "Other";
        other.description = "Other the other char";
        SECTION("when characters can see each other") {
            CHECK(bob.describe(bob) == "Bob"sv);
            CHECK(other.describe(bob) == "Bob"sv);
            CHECK(bob.describe(other) == "Other"sv);
        }
        SECTION("when one character can't see the other") {
            set_enum_bit(bob.affected_by, AffectFlag::Blind);
            CHECK(bob.describe(bob) == "Bob"sv);
            CHECK(other.describe(bob) == "Bob"sv);
            CHECK(bob.describe(other) == "someone"sv);
        }
    }
    SECTION("flags") {
        Descriptor ch_desc{0, mock_mud};
        bob.desc = &ch_desc;
        ch_desc.character(&bob);
        bob.pcdata = std::make_unique<PcData>();
        SECTION("format extra flags") {
            SECTION("no flags set") {
                const auto result = bob.format_extra_flags();

                CHECK(result.empty());
            }
            SECTION("all extra flags set") {
                set_all_bits(bob.extra_flags);
                const auto result = bob.format_extra_flags();

                CHECK(result == "wnet wn_debug wn_mort wn_imm wn_bug permit wn_tick info_mes tip_wiz tip_adv");
            }
        }
        SECTION("serialize extra flags") {
            SECTION("no flags set") {
                const auto all_off = std::string(64u, '0');
                const auto result = bob.serialize_extra_flags();

                CHECK(result == all_off);
            }
            SECTION("all flags set") {
                const auto all_on = std::string(64u, '1');
                set_all_bits(bob.extra_flags);
                const auto result = bob.serialize_extra_flags();

                CHECK(result == all_on);
            }
        }
        SECTION("format act flags") {
            SECTION("no flags set") {
                const auto result = bob.format_act_flags();

                CHECK(result == "none");
            }
            SECTION("npc all act flags set") {
                set_all_bits(bob.act);
                const auto result = bob.format_act_flags();

                CHECK(result
                      == "|Cnpc sentinel scavenger aggressive stay_area wimpy pet train practice sentient talkative "
                         "undead cleric mage thief warrior no_align no_purge healer skill_train update_always "
                         "rideable|w");
            }
            SECTION("pc all act flags set") {
                set_all_bits(bob.act);
                clear_enum_bit(bob.act, PlayerActFlag::PlrNpc);
                const auto result = bob.format_act_flags();

                CHECK(result
                      == "|Cowner autoassist autoexit autoloot autosac autogold autosplit holy_light wizinvis "
                         "loot_corpse no_summon no_follow afk log deny freeze thief killer autopeek prowl|w");
            }
        }
        SECTION("format comm flags") {
            SECTION("no flags set") {
                const auto result = bob.format_comm_flags();

                CHECK(result == "none");
            }
            SECTION("all comm flags set") {
                set_all_bits(bob.comm);
                const auto result = bob.format_comm_flags();

                CHECK(result
                      == "|Cquiet deaf no_wiz no_action no_gossip no_question no_music no_gratz no_allege "
                         "no_philosophise no_qwest compact brief prompt combine show_afk show_def no_emote no_yell "
                         "no_tell no_channels no_allege affect|w");
            }
        }
        SECTION("format offensive flags") {
            SECTION("no flags set") {
                const auto result = bob.format_offensive_flags();

                CHECK(result == "none");
            }
            SECTION("all offensive flags set") {
                set_all_bits(bob.off_flags);
                const auto result = bob.format_offensive_flags();

                CHECK(result
                      == "|Carea_attack backstab bash berserk disarm dodge fade fast slow kick kick_dirt parry rescue "
                         "tail trip crush assist_all assist_align assist_race assist_players assist_guard assist_vnum "
                         "headbutt|w");
            }
        }
        SECTION("format immune flags") {
            SECTION("no flags set") {
                const auto result = bob.format_immune_flags();

                CHECK(result == "none");
            }
            SECTION("all immune flags set") {
                set_all_bits(bob.imm_flags);
                const auto result = bob.format_immune_flags();

                CHECK(result
                      == "|Csummon charm magic weapon bash pierce slash fire cold lightning acid poison negative holy "
                         "energy mental disease drowning light wood silver iron|w");
            }
        }
        SECTION("format resist flags") {
            SECTION("no flags set") {
                const auto result = bob.format_resist_flags();

                CHECK(result == "none");
            }
            SECTION("all resist flags set") {
                set_all_bits(bob.res_flags);
                const auto result = bob.format_resist_flags();

                CHECK(result
                      == "|Csummon charm magic weapon bash pierce slash fire cold lightning acid poison negative holy "
                         "energy mental disease drowning light wood silver iron|w");
            }
        }
        SECTION("format vuln flags") {
            SECTION("no flags set") {
                const auto result = bob.format_vuln_flags();

                CHECK(result == "none");
            }
            SECTION("all vuln flags set") {
                set_all_bits(bob.vuln_flags);
                const auto result = bob.format_vuln_flags();

                CHECK(result
                      == "|Csummon charm magic weapon bash pierce slash fire cold lightning acid poison negative holy "
                         "energy mental disease drowning light wood silver iron|w");
            }
        }
        SECTION("format morphology flags") {
            SECTION("no flags set") {
                const auto result = bob.format_morphology_flags();

                CHECK(result == "none");
            }
            SECTION("all morphology flags set") {
                set_all_bits(bob.morphology);
                const auto result = bob.format_morphology_flags();

                CHECK(result
                      == "|Cedible poison magical instant_rot other animal sentient undead construct mist intangible "
                         "biped centaur insect spider crustacean worm blob mammal bird reptile snake dragon amphibian "
                         "fish cold_blooded|w");
            }
        }
        SECTION("format body part flags") {
            SECTION("no flags set") {
                const auto result = bob.format_body_part_flags();

                CHECK(result == "none");
            }
            SECTION("all body part flags set") {
                set_all_bits(bob.parts);
                const auto result = bob.format_body_part_flags();

                CHECK(result
                      == "|Chead arms legs heart brains guts hands feet fingers ears eyes long_tongue eyestalks "
                         "tentacles fins wings tail claws fangs horns scales tusks|w");
            }
        }
    }
    SECTION("apply skill points") {
        SECTION("not applied as npc") {
            set_enum_bit(bob.act, CharActFlag::Npc);
            bob.apply_skill_points(1, 1);
        }
        SECTION("pc") {
            bob.level = 1;
            Descriptor ch_desc{0, mock_mud};
            bob.desc = &ch_desc;
            ch_desc.character(&bob);
            bob.pcdata = std::make_unique<PcData>();
            const auto skill = skill_lookup("dagger");
            SECTION("applied ok") {
                bob.pcdata->learned[skill] = 1;

                bob.apply_skill_points(skill, 1);
                CHECK(bob.pcdata->learned[skill] == 2);
                CHECK(bob.get_skill(skill) == 2);
                bob.apply_skill_points(skill, 2);
                CHECK(bob.pcdata->learned[skill] == 4);
                CHECK(bob.get_skill(skill) == 4);
            }
            SECTION("clamped to max") {
                bob.pcdata->learned[skill] = 99;

                bob.apply_skill_points(skill, 2);

                CHECK(bob.pcdata->learned[skill] == 100);
                CHECK(bob.get_skill(skill) == 100);
            }
            SECTION("clamped to min") {
                bob.pcdata->learned[skill] = 1;

                bob.apply_skill_points(skill, -1);

                CHECK(bob.pcdata->learned[skill] == 1);
                CHECK(bob.get_skill(skill) == 1);
            }
        }
    }
    SECTION("is link dead player") {
        SECTION("false: npc") {
            set_enum_bit(bob.act, CharActFlag::Npc);
            CHECK_FALSE(bob.is_link_dead_pc());
        }
        SECTION("false: has descriptor") {
            Descriptor ch_desc{0, mock_mud};
            bob.desc = &ch_desc;
            ch_desc.character(&bob);
            CHECK_FALSE(bob.is_link_dead_pc());
        }
        SECTION("true") { CHECK(bob.is_link_dead_pc()); }
    }
}