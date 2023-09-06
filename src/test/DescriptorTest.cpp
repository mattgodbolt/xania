/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2023 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Descriptor.hpp"
#include "Char.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <utility>

using namespace std::literals;

TEST_CASE("Descriptor tests") {
    Descriptor desc{0};
    CHECK(desc.person() == nullptr);
    CHECK(desc.character() == nullptr);
    CHECK_FALSE(desc.is_playing());
    CHECK(desc.state() == DescriptorState::GetName);

    SECTION("enter lobby and proceed") {
        auto uch = std::make_unique<Char>();
        auto *ch = uch.get();

        desc.enter_lobby(std::move(uch));

        CHECK(desc.person() == ch);
        CHECK(desc.character() == ch);
        CHECK(ch->desc == &desc);
        CHECK(desc.state() == DescriptorState::GetName);

        auto uch2 = desc.proceed_from_lobby();

        CHECK(uch2.get() == ch);
        CHECK(desc.state() == DescriptorState::Playing);
        CHECK(desc.is_playing());
    }
    SECTION("restart lobby") {
        auto uch = std::make_unique<Char>();
        desc.enter_lobby(std::move(uch));

        desc.restart_lobby();

        CHECK(desc.person() == nullptr);
        CHECK(desc.character() == nullptr);
        CHECK(desc.state() == DescriptorState::GetName);
    }
    SECTION("reconnect from lobby") {
        auto uch = std::make_unique<Char>();
        auto ch = uch.get();
        ch->timer = 1;

        desc.reconnect_from_lobby(ch);

        CHECK(desc.person() == ch);
        CHECK(desc.character() == ch);
        CHECK(ch->desc == &desc);
        CHECK(desc.state() == DescriptorState::Playing);
        CHECK(desc.is_playing());
        CHECK(ch->timer == 0);
    }
}