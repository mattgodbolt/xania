/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2025 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "AffectFlag.hpp"
#include "CharActFlag.hpp"
#include "DescriptorList.hpp"
#include "MemFile.hpp"
#include "Room.hpp"
#include <catch2/catch_test_macros.hpp>

#include "MockMud.hpp"

extern void do_duel(Char *ch, ArgParser args);

namespace {

test::MockMud mock_mud{};
DescriptorList descriptors{};
Logger logger{descriptors};

auto make_char(const std::string &name, Room &room, MobIndexData *mob_idx) {
    auto ch = std::make_unique<Char>(mock_mud);
    ch->name = name;
    ch->short_descr = name + " descr";
    ch->in_room = &room;
    ch->level = 20;
    if (mob_idx) {
        ch->mobIndex = mob_idx;
        set_enum_bit(ch->act, CharActFlag::Npc);
    } else {
        ch->pcdata = std::make_unique<PcData>();
    }
    // Put it in the Room directly rather than using char_to_room() as that would require other
    // dependencies to be set up.
    room.people.add_back(ch.get());
    return ch;
};

auto set_descriptor(Char *ch, Descriptor *desc) {
    ch->desc = desc;
    desc->character(ch);
}

auto make_mob_index() {
    test::MemFile orcfile(R"mob(
#13000
orc~
the orc shaman~
~
~
orc~
0 0 0 0
0 0 1d6+1 1d6+1 1d1+1 blast
0 0 0 0
0 0 0 0
stand stand male 200
0 0 medium 0

#0
)mob");
    return *MobIndexData::from_file(orcfile.file(), logger);
}

}

TEST_CASE("do_duel") {
    ALLOW_CALL(mock_mud, logger()).LR_RETURN(logger);
    Room room{};
    auto player0 = make_char("vic", room, nullptr);
    Descriptor player0_desc{0, mock_mud};
    set_descriptor(player0.get(), &player0_desc);
    auto player1 = make_char("bob", room, nullptr);
    Descriptor player1_desc{1, mock_mud};
    set_descriptor(player1.get(), &player1_desc);
    auto player2 = make_char("sid", room, nullptr);
    Descriptor player2_desc{2, mock_mud};
    set_descriptor(player2.get(), &player2_desc);

    SECTION("no target") {
        ArgParser args("");

        do_duel(player0.get(), args);

        CHECK(player0_desc.buffered_output() == "\n\rDuel whom?\n\r");
    }
    SECTION("safe room") {
        set_enum_bit(room.room_flags, RoomFlag::Safe);
        ArgParser args("");

        do_duel(player0.get(), args);

        CHECK(player0_desc.buffered_output() == "\n\rDuels aren't permitted here.\n\r");
    }
    SECTION("target not found") {
        ArgParser args("nobody");

        do_duel(player0.get(), args);

        CHECK(player0_desc.buffered_output() == "\n\rThere's nobody here by that name.\n\r");
    }
    SECTION("no NPCs") {
        auto mob_idx = make_mob_index();
        auto npc = make_char("npc", room, &mob_idx);
        ArgParser args("npc");

        do_duel(player0.get(), args);

        CHECK(player0_desc.buffered_output() == "\n\rYou can only duel other players!\n\r");
    }
    SECTION("no suicide") {
        ArgParser args("self");

        do_duel(player0.get(), args);

        CHECK(player0_desc.buffered_output() == "\n\rSuicide is a mortal sin.\n\r");
    }
    SECTION("no deities") {
        player1->level = LEVEL_IMMORTAL;
        ArgParser args("bob");

        do_duel(player0.get(), args);

        CHECK(player0_desc.buffered_output() == "\n\rDeities are not permitted to duel.\n\r");
    }
    SECTION("no switched") {
        auto mob_idx = make_mob_index();
        auto switched_player = make_char("switched", room, &mob_idx);
        switched_player->pcdata = std::make_unique<PcData>();
        Descriptor switched_desc{1, mock_mud};
        set_descriptor(switched_player.get(), &switched_desc);
        ArgParser args("vic");

        do_duel(switched_player.get(), args);

        CHECK(switched_desc.buffered_output()
              == "\n\rCreatures being controlled by a player are not permitted to duel.\n\r");
    }
    SECTION("target level too low") {
        player1->level = 19;
        ArgParser args("bob");

        do_duel(player0.get(), args);

        CHECK(player0_desc.buffered_output()
              == "\n\rYou can duel other players who are at least level 20 and within 10 levels of you.\n\r");
    }
    SECTION("target level outside range") {
        player1->level = 31;
        ArgParser args("bob");

        do_duel(player0.get(), args);

        CHECK(player0_desc.buffered_output()
              == "\n\rYou can duel other players who are at least level 20 and within 10 levels of you.\n\r");
    }
    SECTION("target not standing") {
        player1->position = Position::Type::Sitting;
        ArgParser args("bob");

        do_duel(player0.get(), args);

        CHECK(player0_desc.buffered_output() == "\n\rThey don't look ready to consider a duel at the moment.\n\r");
    }
    SECTION("target charmed") {
        set_enum_bit(player1->affected_by, AffectFlag::Charm);
        ArgParser args("bob");

        do_duel(player0.get(), args);

        CHECK(player0_desc.buffered_output() == "\n\rThey don't look ready to consider a duel at the moment.\n\r");
    }
    SECTION("target link dead") {
        player1->desc = nullptr;
        ArgParser args("bob");

        do_duel(player0.get(), args);

        CHECK(player0_desc.buffered_output() == "\n\rThey can't duel right now as they've been disconnected.\n\r");
        CHECK(!player0->pcdata->duel);
        CHECK(!Duels::is_duel_in_progress(player0.get(), player1.get()));
        CHECK(!Duels::get_duel_rival_name(player1.get()));
    }
    SECTION("invitation sent") {
        ArgParser args("bob");

        do_duel(player0.get(), args);

        CHECK(player0_desc.buffered_output()
              == "\n\rYou challenge bob to a duel to the death!\nYou must await their response.\n\r");
        CHECK(player1_desc.buffered_output()
              == "\n\rVic has challenged you to a duel to the death!\nYou have about 60 seconds to consent by "
                 "entering: duel vic\n\r");
        CHECK(player2_desc.buffered_output() == "\n\rVic has challenged bob to a duel to the death!\n\r");
        CHECK(player0->pcdata->duel);
        CHECK(player0->pcdata->duel->is_accepted());
        CHECK(player0->pcdata->duel->timeout_ticks == 2);
        CHECK(player0->pcdata->duel->rival == player1.get());
        CHECK(!player0->is_aff_curse());

        CHECK(player1->pcdata->duel);
        CHECK(!player1->pcdata->duel->is_accepted());
        CHECK(player1->pcdata->duel->timeout_ticks == 2);
        CHECK(player1->pcdata->duel->rival == player0.get());
        CHECK(!player1->is_aff_curse());

        CHECK(!player2->pcdata->duel);
        CHECK(!Duels::is_duel_in_progress(player0.get(), player1.get()));
        CHECK(Duels::get_duel_rival_name(player0.get()) == "bob");
        CHECK(Duels::get_duel_rival_name(player1.get()) == "vic");

        SECTION("cannot duel second target with active invitation") {
            player0_desc.clear_output_buffer();
            ArgParser args("sid");

            do_duel(player0.get(), args);

            CHECK(player0_desc.buffered_output()
                  == "\n\rYou're already duelling bob. You can only duel one player at a time.\n\r");
        }
        SECTION("target with invitation cannot duel someone else") {
            player1_desc.clear_output_buffer();
            ArgParser response_args("sid");

            do_duel(player1.get(), response_args);

            CHECK(player1_desc.buffered_output()
                  == "\n\rYou've already challenged vic. You can only duel one player at a time.\n\r");
        }
        SECTION("invitation not accepted and active after tick 1") {
            Duels::check_duel_timeout(player0.get());
            CHECK(player0->pcdata->duel);
            CHECK(player0->pcdata->duel->is_accepted());
            CHECK(player0->pcdata->duel->timeout_ticks == 1);
            CHECK(player1->pcdata->duel);
        }
        SECTION("invitation not accepted and expired after tick 2") {
            player0_desc.clear_output_buffer();
            player1_desc.clear_output_buffer();

            Duels::check_duel_timeout(player0.get());
            Duels::check_duel_timeout(player0.get());

            CHECK(!player0->pcdata->duel);
            CHECK(!player1->pcdata->duel);
            CHECK(player0_desc.buffered_output() == "\n\rYour invitation to duel has expired.\n\r");
            CHECK(player1_desc.buffered_output() == "\n\rYour invitation to duel has expired.\n\r");
        }
        SECTION("invitation accepted") {
            player0_desc.clear_output_buffer();
            player1_desc.clear_output_buffer();
            ArgParser response_args("vic");

            do_duel(player1.get(), response_args);

            CHECK(player0_desc.buffered_output()
                  == "\n\rBob consents to your invitation to duel. En garde!\n\rYou feel vulnerable.\n\r");
            CHECK(player1_desc.buffered_output()
                  == "\n\rYou consent to a duel with vic. Good luck!\n\rYou feel vulnerable.\n\r");

            CHECK(player0->pcdata->duel);
            CHECK(player0->pcdata->duel->is_accepted());
            CHECK(player0->pcdata->duel->timeout_ticks == 10);
            CHECK(player0->pcdata->duel->rival == player1.get());
            CHECK(player0->is_aff_curse());

            CHECK(player1->pcdata->duel);
            CHECK(player1->pcdata->duel->is_accepted());
            CHECK(player1->pcdata->duel->timeout_ticks == 10);
            CHECK(player1->pcdata->duel->rival == player0.get());
            CHECK(player1->is_aff_curse());
            CHECK(Duels::is_duel_in_progress(player0.get(), player1.get()));

            SECTION("cannot challenge someone who's accepted an invitation") {
                player2_desc.clear_output_buffer();
                ArgParser another_args("vic");

                do_duel(player2.get(), another_args);

                CHECK(player2_desc.buffered_output() == "\n\rvic is duelling someone else, try again later.\n\r");
            }
            SECTION("invitation accepted and active after tick 1") {
                Duels::check_duel_timeout(player0.get());
                CHECK(player0->pcdata->duel);
                CHECK(player0->pcdata->duel->is_accepted());
                CHECK(player0->pcdata->duel->timeout_ticks == 9);
                CHECK(player1->pcdata->duel);
            }
            SECTION("invitation accepted and expired after tick 10") {
                player0_desc.clear_output_buffer();
                player1_desc.clear_output_buffer();
                for (int i = 0; i < 10; i++)
                    Duels::check_duel_timeout(player0.get());
                CHECK(!player0->pcdata->duel);
                CHECK(!player1->pcdata->duel);
                CHECK(player0_desc.buffered_output()
                      == "\n\rYour duel has taken longer than the Gods will allow and is deemed to be a draw.\n\r");
                CHECK(player1_desc.buffered_output()
                      == "\n\rYour duel has taken longer than the Gods will allow and is deemed to be a draw.\n\r");
                CHECK(!Duels::get_duel_rival_name(player0.get()));
                CHECK(!Duels::get_duel_rival_name(player1.get()));
                CHECK(!Duels::is_duel_in_progress(player0.get(), player1.get()));
            }
        }
    }
}
