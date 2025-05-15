/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

#include "Mud.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/trompeloeil.hpp>

namespace test {

struct MockMud : public trompeloeil::mock_interface<Mud> {
    IMPLEMENT_MOCK0(config);
    IMPLEMENT_MOCK0(descriptors);
    IMPLEMENT_MOCK0(logger);
    IMPLEMENT_MOCK0(interpreter);
    IMPLEMENT_MOCK0(bans);
    IMPLEMENT_MOCK0(areas);
    IMPLEMENT_MOCK0(help);
    IMPLEMENT_MOCK0(socials);
    IMPLEMENT_CONST_MOCK2(send_to_doorman);
    IMPLEMENT_MOCK0(current_tick);
    IMPLEMENT_CONST_MOCK0(boot_time);
    IMPLEMENT_CONST_MOCK0(current_time);
    IMPLEMENT_MOCK1(terminate);
    IMPLEMENT_MOCK0(toggle_wizlock);
    IMPLEMENT_MOCK0(toggle_newlock);
    IMPLEMENT_CONST_MOCK0(max_players_today);
    IMPLEMENT_MOCK0(reset_max_players_today);
    IMPLEMENT_MOCK0(send_tips);
};

}
