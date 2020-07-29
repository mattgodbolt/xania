#include "TelnetProtocol.hpp"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <arpa/telnet.h>
#include <catch2/catch.hpp>
#include <catch2/trompeloeil.hpp>

namespace {
class MockHandler : public TelnetProtocol::Handler {
public:
    MAKE_MOCK1(send_bytes, void(gsl::span<const byte>), override);
    MAKE_MOCK1(on_line, void(std::string_view), override);
    MAKE_MOCK2(on_terminal_size, void(int, int), override);
    MAKE_MOCK2(on_terminal_type, void(std::string_view type, bool ansi_supported), override);
};

using bytes = std::vector<byte>;
auto wrap(std::string_view sv) {
    bytes b;
    for (auto c : sv)
        b.emplace_back(static_cast<byte>(c));
    return b;
}
auto match_bytes(const bytes &bytes) {
    return trompeloeil::make_matcher<gsl::span<const byte>>(
        [bytes](gsl::span<const byte> span) {
            return span.size() == bytes.size() && memcmp(span.data(), bytes.data(), span.size()) == 0;
        },
        [bytes](std::ostream &os) { fmt::print(os, " is bytes{{{:02x}}}", fmt::join(bytes, ", ")); });
}
}

TEST_CASE("Telnet protocol", "[TelnetProtocol]") {
    MockHandler mock;
    TelnetProtocol tp(mock);
    SECTION("Should start out not supporting ANSI") { CHECK(!tp.supports_ansi()); }
    SECTION("Should start out empty") { CHECK(tp.buffered_data_size() == 0); }
    SECTION("Should send appropriate telnet requests and options on request") {
        REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, WONT, TELOPT_ECHO})));
        REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, DO, TELOPT_NAWS})));
        REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, DO, TELOPT_TTYPE})));
        tp.send_telopts();
    }
    SECTION("Should send echo on") {
        REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, WONT, TELOPT_ECHO})));
        tp.set_echo(true);
    }
    SECTION("Should send echo off") {
        REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, WILL, TELOPT_ECHO})));
        tp.set_echo(false);
    }
    SECTION("Should handle WILLs") {
        SECTION("handling terminal type") {
            {
                REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, SB, TELOPT_TTYPE, TELQUAL_SEND, IAC, SE})));
                tp.add_data(bytes{IAC, WILL, TELOPT_TTYPE});
            }
            SECTION("but not the second time") { tp.add_data(bytes{IAC, WILL, TELOPT_TTYPE}); }
        }
        SECTION("ignoring NAWS") { tp.add_data(bytes{IAC, WILL, TELOPT_NAWS}); }
        SECTION("refusing any option we don't understand") {
            REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, DONT, TELOPT_BINARY})));
            tp.add_data(bytes{IAC, WILL, TELOPT_BINARY});
        }
    }
    SECTION("Should handle WONTs by saying DONT") {
        REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, WONT, TELOPT_BINARY})));
        tp.add_data(bytes{IAC, DONT, TELOPT_BINARY});
    }
    SECTION("Should handle DONTs by saying WONT") {
        REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, DONT, TELOPT_BINARY})));
        tp.add_data(bytes{IAC, WONT, TELOPT_BINARY});
    }
    SECTION("Should handle DO") {
        // This is confusing as heck, but passes with the original implementation.
        SECTION("handling echo when we're echoing") {
            REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, WONT, TELOPT_ECHO})));
            tp.add_data(bytes{IAC, DO, TELOPT_ECHO});
        }
        SECTION("handling echo when we're not echoing") {
            {
                REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, WILL, TELOPT_ECHO})));
                tp.set_echo(false);
            }
            REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, WILL, TELOPT_ECHO})));
            tp.add_data(bytes{IAC, DO, TELOPT_ECHO});
        }
        SECTION("ignoring anything else") {
            REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, WONT, TELOPT_AUTHENTICATION})));
            tp.add_data(bytes{IAC, DO, TELOPT_AUTHENTICATION});
        }
    }
    SECTION("Should handle sub options") {
        SECTION("terminal types") {
            SECTION("supporting ansi") {
                REQUIRE_CALL(mock, on_terminal_type("xterm", true));
                tp.add_data(bytes{IAC, SB, TELOPT_TTYPE, TELQUAL_IS, 'x', 't', 'e', 'r', 'm', IAC, SE});
                CHECK(tp.supports_ansi());
            }
            SECTION("not supporting ansi") {
                REQUIRE_CALL(mock, on_terminal_type("badger", false));
                tp.add_data(bytes{IAC, SB, TELOPT_TTYPE, TELQUAL_IS, 'b', 'a', 'd', 'g', 'e', 'r', IAC, SE});
                CHECK(!tp.supports_ansi());
            }
        }
        SECTION("window sizes") {
            SECTION("small") {
                REQUIRE_CALL(mock, on_terminal_size(80, 40));
                tp.add_data(bytes{IAC, SB, TELOPT_NAWS, 0, 80, 0, 40, IAC, SE});
            }
            SECTION("giant") {
                REQUIRE_CALL(mock, on_terminal_size(0x1234, 0x4556));
                tp.add_data(bytes{IAC, SB, TELOPT_NAWS, 0x12, 0x34, 0x45, 0x56, IAC, SE});
            }
        }
        SECTION("ignore anything else") { tp.add_data(bytes{IAC, SB, TELOPT_BINARY, 0, 1, 2, 3, 4, 5, 6, 7, IAC, SE}); }
    }

    // TODO handle IAC IAC, and byte at a time

    SECTION("Should ignore any IAC,char seq") { tp.add_data(bytes{IAC, '\n'}); }
    SECTION("Should handle more complex situations") {
        auto data = bytes{
            IAC, DONT, TELOPT_BINARY, IAC, WONT, TELOPT_ENCRYPT, IAC, '\n', IAC, SB, TELOPT_TTYPE, TELQUAL_IS, 'x', 't',
            'e', 'r',  'm',           IAC, SE};
        REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, WONT, TELOPT_BINARY})));
        REQUIRE_CALL(mock, send_bytes(match_bytes(bytes{IAC, DONT, TELOPT_ENCRYPT})));
        REQUIRE_CALL(mock, on_terminal_type("xterm", true));
        SECTION("Should handle IAC commands one byte at a time") {
            for (auto x : data)
                tp.add_data(gsl::span<const byte>(&x, 1));
        }
        SECTION("Should handle IAC commands not at the beginning of the buffer") {
            auto cruft = bytes{'H', 'e', 'l', 'l', 'o'};
            data.insert(data.begin(), begin(cruft), end(cruft));
            tp.add_data(data);
            REQUIRE_CALL(mock, on_line("Hello"));
            tp.add_data(bytes{'\n'});
        }
        SECTION("Should handle IAC commands with subsequent text") {
            auto cruft = bytes{'H', 'e', 'l', 'l', 'o'};
            data.insert(data.end(), begin(cruft), end(cruft));
            tp.add_data(data);
            REQUIRE_CALL(mock, on_line("Hello"));
            tp.add_data(bytes{'\n'});
        }
    }

    SECTION("Should handle lines") {
        SECTION("Simple case with just newlines") {
            REQUIRE_CALL(mock, on_line("Hello"));
            tp.add_data(wrap("Hello\n"));
        }
        SECTION("Simple case with just returns") {
            REQUIRE_CALL(mock, on_line("Hello"));
            tp.add_data(wrap("Hello\n"));
        }
        SECTION("Simple case with newlines and returns") {
            REQUIRE_CALL(mock, on_line("Hello"));
            tp.add_data(wrap("Hello\n\r"));
        }
        SECTION("Simple case with returns and newlines") {
            REQUIRE_CALL(mock, on_line("Hello"));
            tp.add_data(wrap("Hello\r\n"));
        }
        SECTION("Empty lines") {
            SECTION("Just newline") {
                REQUIRE_CALL(mock, on_line(""));
                tp.add_data(wrap("\n"));
            }
            SECTION("Just return") {
                REQUIRE_CALL(mock, on_line(""));
                tp.add_data(wrap("\r"));
            }
            SECTION("Newline return") {
                REQUIRE_CALL(mock, on_line(""));
                tp.add_data(wrap("\n\r"));
            }
            SECTION("Return newline") {
                REQUIRE_CALL(mock, on_line(""));
                tp.add_data(wrap("\r\n"));
            }
            // TODO this is actually broken... See bug #58
            //            SECTION("Char at a time") {
            //                SECTION("Newline return") {
            //                    REQUIRE_CALL(mock, on_line(""));
            //                    tp.add_data(wrap("\n"));
            //                    tp.add_data(wrap("\r"));
            //                }
            //                SECTION("Return newline") {
            //                    REQUIRE_CALL(mock, on_line(""));
            //                    tp.add_data(wrap("\r"));
            //                    tp.add_data(wrap("\n"));
            //                }
            //                SECTION("Two newlines") {
            //                    REQUIRE_CALL(mock, on_line(""));
            //                    REQUIRE_CALL(mock, on_line(""));
            //                    tp.add_data(wrap("\n"));
            //                    tp.add_data(wrap("\n"));
            //                }
            //                SECTION("Two returns") {
            //                    REQUIRE_CALL(mock, on_line(""));
            //                    REQUIRE_CALL(mock, on_line(""));
            //                    tp.add_data(wrap("\r"));
            //                    tp.add_data(wrap("\r"));
            //                }
            //                SECTION("Two CRLF") {
            //                    REQUIRE_CALL(mock, on_line(""));
            //                    REQUIRE_CALL(mock, on_line(""));
            //                    tp.add_data(wrap("\r"));
            //                    tp.add_data(wrap("\n"));
            //                    tp.add_data(wrap("\r"));
            //                    tp.add_data(wrap("\n"));
            //                }
            //                SECTION("Two LFCR") {
            //                    REQUIRE_CALL(mock, on_line(""));
            //                    REQUIRE_CALL(mock, on_line(""));
            //                    tp.add_data(wrap("\n"));
            //                    tp.add_data(wrap("\r"));
            //                    tp.add_data(wrap("\n"));
            //                    tp.add_data(wrap("\r"));
            //                }
            //            }
        }

        SECTION("Multiple lines with varying newlines") {
            trompeloeil::sequence seq;
            REQUIRE_CALL(mock, on_line("Newline")).IN_SEQUENCE(seq);
            REQUIRE_CALL(mock, on_line("")).IN_SEQUENCE(seq);
            REQUIRE_CALL(mock, on_line("Return")).IN_SEQUENCE(seq);
            REQUIRE_CALL(mock, on_line("")).IN_SEQUENCE(seq);
            REQUIRE_CALL(mock, on_line("Both1")).IN_SEQUENCE(seq);
            REQUIRE_CALL(mock, on_line("")).IN_SEQUENCE(seq);
            REQUIRE_CALL(mock, on_line("Both2")).IN_SEQUENCE(seq);
            REQUIRE_CALL(mock, on_line("")).IN_SEQUENCE(seq);

            tp.add_data(wrap("Newline\n\nReturn\r\rBoth1\n\r\n\rBoth2\r\n\r\n"));
        }

        SECTION("Multiple lines ending on an incomplete") {
            trompeloeil::sequence seq;
            REQUIRE_CALL(mock, on_line("I am the very model of a modern Major-General,")).IN_SEQUENCE(seq);
            REQUIRE_CALL(mock, on_line("I've information vegetable, animal, and mineral,")).IN_SEQUENCE(seq);
            REQUIRE_CALL(mock, on_line("I know the kings of England, and I quote the fights historical,"))
                .IN_SEQUENCE(seq);

            tp.add_data(wrap(R"(I am the very model of a modern Major-General,
I've information vegetable, animal, and mineral,
I know the kings of England, and I quote the fights historical,
From Marathon to Waterloo, in order categorical;)"));
            SECTION("...and then completing") {
                REQUIRE_CALL(mock, on_line("From Marathon to Waterloo, in order categorical;")).IN_SEQUENCE(seq);
                tp.add_data(bytes{'\n'});
            }
        }
    }

    // TODO fix this...
//    SECTION("Should handle complete lines followed by incomplete IAC sequences") {
//        REQUIRE_CALL(mock, on_line("Hi"));
//        tp.add_data(bytes{'H', 'i', '\n', IAC, SB});
//    }
}