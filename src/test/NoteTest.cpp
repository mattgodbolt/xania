#include "Note.hpp"

#include "Char.hpp"
#include "DescriptorList.hpp"
#include "MemFile.hpp"

#include "catch2/catch_test_macros.hpp"

#include "MockMud.hpp"

using namespace std::literals;

namespace {
std::string crlfify(const std::string &s) { return replace_strings(s, "\n", "\n\r"); }

test::MockMud mock_mud{};
DescriptorList descriptors{};
Logger logger{descriptors};
const auto now = Clock::now();

}

TEST_CASE("Note handling") {
    // Annoyingly we can't "move" Chars even for testing, so we have to set them up here and not use a function.
    Char ian{mock_mud};
    ian.name = "ian";
    Char bob{mock_mud};
    bob.name = "bob";
    Char brian{mock_mud};
    brian.name = "brian";
    brian.level = 100;
    Char jenny{mock_mud};
    jenny.name = "jenny";

    SECTION("should find players on the to line") {
        Note note("jenny");
        note.to_list("ian bob");
        CHECK(note.is_to(ian));
        CHECK(note.is_to(bob));
        CHECK(!note.is_to(brian));
    }
    SECTION("should find the sender on the to line") {
        Note note("jenny");
        note.to_list("ian bob");
        CHECK(note.is_to(jenny));
    }
    SECTION("should identify the sender") {
        Note note("jenny");
        CHECK(note.sent_by(jenny));
        CHECK(!note.sent_by(brian));
    }
    SECTION("should find everyone for 'all' on the to line") {
        Note note("jenny");
        note.to_list("all");
        CHECK(note.is_to(ian));
        CHECK(note.is_to(bob));
        CHECK(note.is_to(brian));
        CHECK(note.is_to(jenny));
    }
    SECTION("should find IMMs for 'immortal' on the to line") {
        Note note("jenny");
        note.to_list("immortal");
        CHECK(!note.is_to(ian));
        CHECK(!note.is_to(bob));
        CHECK(note.is_to(brian));
        CHECK(note.is_to(jenny));
    }
    SECTION("should load from raw data") {
        auto file = test::MemFile(R"(Sender Ian~
Date 2020-12-31 16:08:13~
Stamp 1609430893
To bob~
Subject testing~
Text
This is line one
This is line two
~
)");
        auto note = Note::from_file(file.file(), logger);
        CHECK(note.is_to(bob));
        CHECK(note.sent_by(ian));
        CHECK(note.date() == "2020-12-31 16:08:13");
        CHECK(note.date_stamp() == Time(1609430893s));
        CHECK(note.text() == "This is line one\n\rThis is line two\n\r");
    }
    SECTION("should throw on junk data") {
        auto file = test::MemFile(R"(Sender Ian~
Oh no this broke!@313)");
        CHECK_THROWS(Note::from_file(file.file(), logger));
    }
    SECTION("should save and load back in") {
        auto save_file = test::MemFile();
        Note note("jenny");
        note.to_list("ian bob");
        note.date(Time(1609430893s));
        note.subject("I will survive");
        note.add_line("At first I was afraid");
        note.add_line("I was petrified");
        note.save(save_file.file());
        save_file.rewind();
        auto loaded = Note::from_file(save_file.file(), logger);
        CHECK(loaded.to_list() == note.to_list());
        CHECK(loaded.subject() == note.subject());
        CHECK(loaded.text() == note.text());
        CHECK(loaded.sent_by(jenny));
        CHECK(loaded.date_stamp() == note.date_stamp());
        CHECK(loaded.date() == note.date());
    }
    SECTION("should allow text edit") {
        Note note("bob");
        CHECK(note.text().empty());
        note.add_line("one");
        CHECK(note.text() == "one\n\r");
        note.add_line("two");
        CHECK(note.text() == "one\n\rtwo\n\r");
        note.remove_line();
        CHECK(note.text() == "one\n\r");
    }
}

TEST_CASE("Note list handling") {
    Char bob{mock_mud};
    bob.name = "bob";
    Char jenny{mock_mud};
    jenny.name = "jenny";

    Notes notes;
    Note moog_note("TheMoog");
    moog_note.subject("Don't spam all");
    moog_note.date(Time(90s));
    moog_note.to_list("jenny");
    notes.add(moog_note);
    Note jenny_note("jenny");
    jenny_note.subject("Hey");
    jenny_note.date(Time(100s));
    jenny_note.to_list("all");
    notes.add(jenny_note);
    Note bob_note("bob");
    bob_note.subject("Re: Hey");
    bob_note.date(Time(120s));
    bob_note.to_list("jenny");
    notes.add(bob_note);

    SECTION("unread notes") {
        SECTION("should initially be correct") {
            CHECK(notes.num_unread(bob) == 1);
            CHECK(notes.num_unread(jenny) == 2);
        }
        SECTION("should account for updated last notes (1)") {
            jenny.last_note = Time(100s);
            CHECK(notes.num_unread(jenny) == 1);
            jenny.last_note = Time(120s);
            CHECK(notes.num_unread(jenny) == 0);
        }
        SECTION("should account for updated last notes (2)") {
            bob.last_note = Time(110s);
            CHECK(notes.num_unread(bob) == 0);
        }
    }
    SECTION("lookup notes") {
        SECTION("should find all messages, including ones we wrote (jenny)") {
            auto *first = notes.lookup(0, jenny);
            REQUIRE(first);
            CHECK(first->subject() == "Don't spam all");
            auto *second = notes.lookup(1, jenny);
            REQUIRE(second);
            CHECK(second->subject() == "Hey");
            auto *third = notes.lookup(2, jenny);
            REQUIRE(third);
            CHECK(third->subject() == "Re: Hey");
            CHECK(!notes.lookup(3, jenny));
        }
        SECTION("should find all messages (bob)") {
            auto *first = notes.lookup(0, bob);
            REQUIRE(first);
            CHECK(first->subject() == "Hey");
            auto *second = notes.lookup(1, bob);
            REQUIRE(second);
            CHECK(second->subject() == "Re: Hey");
            CHECK(!notes.lookup(2, bob));
        }
        SECTION("should find messages by time") {
            SECTION("first") {
                auto &&[note, index] = notes.lookup(Time(0s), jenny);
                REQUIRE(note);
                CHECK(note->subject() == "Don't spam all");
                CHECK(index == 0);
            }
            SECTION("second") {
                auto &&[note, index] = notes.lookup(Time(90s), jenny);
                REQUIRE(note);
                CHECK(note->subject() == "Re: Hey");
                CHECK(index == 1);
            }
            SECTION("no more") { CHECK(!notes.lookup(Time(120s), jenny).first); }
        }
    }
}

TEST_CASE("Note command handling") {
    // Pretty nasty way of hacking out something that looks enough like a PC to write tests.
    // Ideally we'd extract a lot more interfaces and test using mocks.
    ALLOW_CALL(mock_mud, descriptors()).LR_RETURN(descriptors);
    ALLOW_CALL(mock_mud, current_time()).RETURN(now);
    Char jenny{mock_mud};
    jenny.name = "jenny";
    Descriptor jenny_desc{0, mock_mud};
    jenny.desc = &jenny_desc;
    jenny.pcdata = std::make_unique<PcData>();
    jenny_desc.character(&jenny);

    bool was_saved{};
    NoteHandler handler([&](NoteHandler &) { was_saved = true; });
    SECTION("reading") {
        Note moog_note("TheMoog");
        moog_note.subject("So here it is");
        moog_note.date(Time(1608854400s));
        moog_note.to_list("jenny");
        moog_note.add_line("Merry Christmas");
        moog_note.add_line("Everybody's having fun");
        handler.add_for_testing(moog_note);

        auto formatted_msg = crlfify(R"(
[  0] TheMoog: So here it is
2020-12-25 00:00:00Z
To: jenny
Merry Christmas
Everybody's having fun

)");
        SECTION("should read by default") {
            handler.on_command(jenny, ArgParser(""));
            CHECK(jenny_desc.buffered_output() == formatted_msg);
        }
        SECTION("should read when asked") {
            handler.on_command(jenny, ArgParser("read"));
            CHECK(jenny_desc.buffered_output() == formatted_msg);
        }
        SECTION("should read when asked") {
            handler.on_command(jenny, ArgParser("read 0"));
            CHECK(jenny_desc.buffered_output() == formatted_msg);
        }
        SECTION("should report no such note") {
            handler.on_command(jenny, ArgParser("read 1"));
            CHECK(jenny_desc.buffered_output() == "\n\rNo such note.\n\r");
        }
    }

    SECTION("composing") {
        auto &notes = handler.notes().notes();
        SECTION("should work on the happy path") {
            handler.on_command(jenny, ArgParser("to themoog"));
            handler.on_command(jenny, ArgParser("subject I prefer mariah"));
            handler.on_command(jenny, ArgParser("+ shes much better"));
            handler.on_command(jenny, ArgParser("-"));
            handler.on_command(jenny, ArgParser("+ she's much better"));
            handler.on_command(jenny, ArgParser("post"));
            REQUIRE(notes.size() == 1);
            CHECK(was_saved);
            auto &note = notes.front();
            CHECK(note.subject() == "I prefer mariah");
            CHECK(note.to_list() == "themoog");
            CHECK(note.text() == "she's much better\n\r");
        }
        SECTION("should complain about missing subject") {
            handler.on_command(jenny, ArgParser("to themoog"));
            handler.on_command(jenny, ArgParser("+ she's much better"));
            jenny_desc.clear_output_buffer();
            handler.on_command(jenny, ArgParser("post"));
            CHECK(jenny_desc.buffered_output() == "\n\rYou need to provide a subject.\n\r");
            CHECK(notes.empty());
            CHECK(!was_saved);
        }
        SECTION("should complain about missing to") {
            handler.on_command(jenny, ArgParser("subject splat"));
            handler.on_command(jenny, ArgParser("+ she's much better"));
            jenny_desc.clear_output_buffer();
            handler.on_command(jenny, ArgParser("post"));
            CHECK(jenny_desc.buffered_output() == "\n\rYou need to provide a recipient (name, all, or immortal).\n\r");
            CHECK(notes.empty());
            CHECK(!was_saved);
        }
    }
    SECTION("removing") {
        Note moog_note("TheMoog");
        moog_note.subject("For you");
        moog_note.date(Time(1608854400s));
        moog_note.to_list("jenny");
        moog_note.add_line("Hi");

        SECTION("with no number") {
            handler.add_for_testing(moog_note);
            handler.on_command(jenny, ArgParser("remove"));
            CHECK(jenny_desc.buffered_output() == "\n\rNote remove which number?\n\r");
            CHECK(!was_saved);
        }
        SECTION("with a bad number") {
            handler.add_for_testing(moog_note);
            handler.on_command(jenny, ArgParser("remove 123"));
            CHECK(jenny_desc.buffered_output() == "\n\rNo such note.\n\r");
            CHECK(!was_saved);
        }
        auto &notes = handler.notes().notes();
        SECTION("with a correct number") {
            SECTION("when we're the only recipient") {
                handler.add_for_testing(moog_note);
                handler.on_command(jenny, ArgParser("remove 0"));
                CHECK(jenny_desc.buffered_output() == "\n\rOk.\n\r");
                CHECK(notes.empty());
                CHECK(was_saved);
            }
            SECTION("when we're one of the recipients") {
                moog_note.to_list("jenny bob");
                handler.add_for_testing(moog_note);
                handler.on_command(jenny, ArgParser("remove 0"));
                CHECK(jenny_desc.buffered_output() == "\n\rOk.\n\r");
                REQUIRE(notes.size() == 1);
                CHECK(was_saved);
                CHECK(notes.front().to_list() == "bob");
            }
        }
    }
}
