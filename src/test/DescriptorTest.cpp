/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2023 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Descriptor.hpp"
#include "Char.hpp"
#include "PcData.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <utility>

#include "MockMud.hpp"

using namespace std::literals;

namespace {

test::MockMud mock_mud{};

}

TEST_CASE("Descriptor tests") {
    Descriptor desc{0, mock_mud};
    CHECK(desc.person() == nullptr);
    CHECK(desc.character() == nullptr);
    CHECK_FALSE(desc.is_playing());
    CHECK(desc.state() == DescriptorState::GetName);

    SECTION("enter lobby and proceed") {
        auto uch = std::make_unique<Char>(mock_mud);
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
        auto uch = std::make_unique<Char>(mock_mud);
        desc.enter_lobby(std::move(uch));

        desc.restart_lobby();

        CHECK(desc.person() == nullptr);
        CHECK(desc.character() == nullptr);
        CHECK(desc.state() == DescriptorState::GetName);
    }
    SECTION("reconnect from lobby") {
        auto uch = std::make_unique<Char>(mock_mud);
        auto ch = uch.get();
        ch->idle_timer_ticks = 1;

        desc.reconnect_from_lobby(ch);

        CHECK(desc.person() == ch);
        CHECK(desc.character() == ch);
        CHECK(ch->desc == &desc);
        CHECK(desc.state() == DescriptorState::Playing);
        CHECK(desc.is_playing());
        CHECK(ch->idle_timer_ticks == 0);
    }
}
TEST_CASE("Descriptor pager") {
    Descriptor desc{0, mock_mud};
    auto uch = std::make_unique<Char>(mock_mud);
    auto *ch = uch.get();
    ch->pcdata = std::make_unique<PcData>();
    ch->lines = 3;
    desc.enter_lobby(std::move(uch));
    // write() starts an empty buffer with a newline; the pager tests are only interested in what follows.
    const auto sent = [&desc] {
        auto output = desc.buffered_output();
        desc.clear_output_buffer();
        return output.starts_with("\n\r") ? output.substr(2) : output;
    };

    SECTION("fewer lines than a page are all sent") {
        desc.page_to("a\n\rb\n\r");
        CHECK(sent() == "a\n\rb\n\r");
        CHECK_FALSE(desc.is_paging());
    }
    SECTION("exactly one page is all sent, with nothing left to show") {
        desc.page_to("a\n\rb\n\rc\n\r");
        CHECK(sent() == "a\n\rb\n\rc\n\r");
        CHECK_FALSE(desc.is_paging());
    }
    SECTION("one line over a page waits for the last line") {
        desc.page_to("a\n\rb\n\rc\n\rd\n\r");
        CHECK(sent() == "a\n\rb\n\rc\n\r");
        CHECK(desc.is_paging());
        desc.show_next_page("");
        CHECK(sent() == "d\n\r");
        CHECK_FALSE(desc.is_paging());
    }
    SECTION("blank lines at the end are dropped") {
        SECTION("one blank line") {
            desc.page_to("a\n\rb\n\rc\n\r\n\r");
            CHECK(sent() == "a\n\rb\n\rc\n\r");
            CHECK_FALSE(desc.is_paging());
        }
        SECTION("several blank lines, including whitespace-only ones") {
            desc.page_to("a\n\rb\n\rc\n\r\n\r   \n\r\n\r");
            CHECK(sent() == "a\n\rb\n\rc\n\r");
            CHECK_FALSE(desc.is_paging());
        }
        SECTION("only blank lines") {
            desc.page_to("\n\r\n\r\n\r\n\r");
            CHECK(sent().empty());
            CHECK_FALSE(desc.is_paging());
        }
    }
    SECTION("blank lines before the end are kept") {
        desc.page_to("a\n\r\n\r\n\r\n\rb\n\r");
        CHECK(sent() == "a\n\r\n\r\n\r");
        CHECK(desc.is_paging());
        desc.show_next_page("");
        CHECK(sent() == "\n\rb\n\r");
        CHECK_FALSE(desc.is_paging());
    }
    SECTION("any input cancels the rest") {
        desc.page_to("a\n\rb\n\rc\n\rd\n\re\n\r");
        sent();
        desc.show_next_page("q");
        CHECK(sent().empty());
        CHECK_FALSE(desc.is_paging());
    }
}
