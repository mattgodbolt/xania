/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  Note.cpp: notes system                                               */
/*                                                                       */
/*************************************************************************/

#include "Note.hpp"
#include "Char.hpp"
#include "CommFlag.hpp"
#include "DescriptorList.hpp"
#include "TimeInfoData.hpp"
#include "common/BitOps.hpp"
#include "common/Configuration.hpp"
#include "db.h"
#include "interp.h"
#include "string_utils.hpp"

#include <fmt/format.h>
#include <range/v3/algorithm/count_if.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include <range/v3/view/filter.hpp>

#include <functional>

bool Note::is_to(const Char &ch) const {
    if (matches(ch.name, sender_)) {
        return true;
    }
    if (is_name("all", to_list_)) {
        return true;
    }
    if (ch.is_immortal() && is_name("immortal", to_list_)) {
        return true;
    }
    if (is_name(ch.name, to_list_)) {
        return true;
    }
    return false;
}

bool Note::sent_by(const Char &ch) const { return matches(ch.name, sender_); }

void Note::save(FILE *file) const {
    fmt::print(file, "Sender {}~\n", sender_);
    fmt::print(file, "Date {}~\n", date_);
    fmt::print(file, "Stamp {}\n", Clock::to_time_t(date_stamp_));
    fmt::print(file, "To {}~\n", to_list_);
    fmt::print(file, "Subject {}~\n", subject_);
    fmt::print(file, "Text\n{}~\n", text_);
}

void Notes::erase(const Note &note) {
    auto it = ranges::find_if(notes_, [&](const Note &data) { return &data == &note; });
    if (it != notes_.end())
        notes_.erase(it);
}
Note &Notes::add(Note note) { return notes_.emplace_back(std::move(note)); }

int Notes::num_unread(const Char &ch) const {
    return ranges::count_if(
        notes_, [&ch](auto &note) { return note.is_to(ch) && !note.sent_by(ch) && note.date_stamp() > ch.last_note; });
}

Note *Notes::lookup(int index, const Char &ch) {
    for (auto &note : notes_) {
        if (note.is_to(ch)) {
            if (index-- <= 0) {
                return &note;
            }
        }
    }
    return nullptr;
}

std::pair<Note *, int> Notes::lookup(Time date, const Char &ch) {
    int count = 0;

    for (auto &note : notes_) {
        if (note.is_to(ch) && !note.sent_by(ch)) {
            if (date < note.date_stamp()) {
                return {&note, count};
            }
            count++;
        }
    }
    return {nullptr, 0};
}

void Notes::save(FILE *file) const {
    for (auto &note : notes_)
        note.save(file);
}

static Note &ensure_note(Char &ch) {
    if (!ch.pnote)
        ch.pnote = std::make_unique<Note>(ch.name);
    return *ch.pnote;
}

Note Note::from_file(FILE *fp) {
    auto expect = [&](std::string_view expected) {
        auto *word = fread_word(fp);
        if (!matches(word, expected))
            throw std::runtime_error(fmt::format("Expected '{}'", expected));
    };
    auto expect_str = [&](std::string_view expected) {
        expect(expected);
        return fread_stdstring(fp);
    };
    Note note(expect_str("sender"));
    note.date_ = expect_str("date");
    expect("stamp");
    note.date_stamp_ = Clock::from_time_t(fread_number(fp));
    note.to_list_ = expect_str("to");
    note.subject_ = expect_str("subject");
    note.text_ = expect_str("text");
    return note;
}

void Note::subject(std::string_view subject) { subject_ = subject; }
void Note::date(Time point) {
    date_ = formatted_time(point);
    date_stamp_ = point;
}
void Note::to_list(std::string_view to_list) { to_list_ = to_list; }
void Note::add_line(std::string_view line) { text_ += fmt::format("{}\n\r", line); }
void Note::remove_line() { text_ = remove_last_line(text_); }

NoteHandler &NoteHandler::singleton() {
    static auto on_change = [](NoteHandler &handler) {
        const auto notes_file = Configuration::singleton().notes_file();
        if (auto *file = fopen(notes_file.c_str(), "w")) {
            handler.write_to(file);
            fclose(file);
        } else {
            perror(notes_file.c_str());
        }
    };
    static NoteHandler singleton(on_change);
    return singleton;
}

void note_initialise() {
    const auto notes_file = Configuration::singleton().notes_file();
    auto &handler = NoteHandler::singleton();
    if (auto *fp = fopen(notes_file.c_str(), "r")) {
        handler.read_from(fp);
        fclose(fp);
    }
}

void do_note(Char *ch, const char *argument) {
    if (ch->is_npc()) {
        return;
    }
    char arg[MAX_INPUT_LENGTH];
    auto note_remainder = smash_tilde(one_argument(argument, arg));

    auto &handler = NoteHandler::singleton();
    handler.on_command(*ch, ArgParser(argument));
}

NoteHandler::NoteHandler(OnChangeFunc on_change_func) : on_change_func_(std::move(on_change_func)) {
    sub_commands.add("read", &NoteHandler::read);
    sub_commands.add("list", &NoteHandler::list);
    sub_commands.add("+", &NoteHandler::add_line);
    sub_commands.add("-", &NoteHandler::remove_line);
    sub_commands.add("subject", &NoteHandler::subject);
    sub_commands.add("to", &NoteHandler::to);
    sub_commands.add("clear", &NoteHandler::clear);
    sub_commands.add("show", &NoteHandler::show);
    sub_commands.add("post", &NoteHandler::post);
    sub_commands.add("send", &NoteHandler::post);
    sub_commands.add("remove", &NoteHandler::remove);
    sub_commands.add("delete", &NoteHandler::delet, MAX_LEVEL - 2);
}

void NoteHandler::write_to(FILE *fp) { notes_.save(fp); }

void NoteHandler::read_from(FILE *fp) {
    for (;;) {
        int letter;
        do {
            letter = getc(fp);
            if (feof(fp) || ferror(fp))
                return;
        } while (isspace(letter));
        ungetc(letter, fp);

        auto note = Note::from_file(fp);
        if (note.date_stamp() < current_time - date::weeks(2))
            continue;
        notes_.add(std::move(note));
    }
}

void NoteHandler::on_command(Char &ch, ArgParser args) {
    auto sub = args.shift();
    auto note_fn = sub_commands.get(sub.empty() ? "read" : sub, ch.get_trust());
    if (note_fn.has_value()) {
        (this->*note_fn.value())(ch, std::move(args));
    } else {
        ch.send_line("Huh?  Type 'help note' for usage.");
    }
}

void NoteHandler::read(Char &ch, ArgParser args) {
    auto show_note = [&ch](int note_index, const Note &note) {
        ch.send_line("[{:3}] {}|w: {}|w", note_index, note.sender(), note.subject());
        ch.send_line("{}|w", note.date());
        ch.send_line("To: {}|w", note.to_list());
        if (!note.text().empty())
            ch.page_to(note.text());
        ch.send_line("|w");
        ch.last_note = std::max(ch.last_note, note.date_stamp());
    };

    if (auto num = args.try_shift_number()) {
        if (auto *note = notes_.lookup(*num, ch)) {
            show_note(*num, *note);
        } else {
            ch.send_line("No such note.");
        }
    } else if (args.empty() || matches_start(args.shift(), "next")) {
        if (auto note_pair = notes_.lookup(ch.last_note, ch); note_pair.first) {
            show_note(note_pair.second, *note_pair.first);
        } else {
            ch.send_line("You have no unread notes.");
        }
    } else {
        ch.send_line("Note read which number?");
    }
}

void NoteHandler::list(Char &ch, [[maybe_unused]] ArgParser) {
    int num = 0;
    for (auto &note : notes_.notes()) {
        if (note.is_to(ch)) {
            bool is_new = note.date_stamp() > ch.last_note && !note.sent_by(ch);
            ch.send_line("[{:3}{}] {}: {}|w", num, is_new ? "N" : " ", note.sender(), note.subject());
            num++;
        }
    }
}

void NoteHandler::post(Char &ch, [[maybe_unused]] ArgParser) {
    if (!ch.pnote) {
        ch.send_line("You have no note in progress.");
        return;
    }
    if (ch.pnote->to_list().empty()) {
        ch.send_line("You need to provide a recipient (name, all, or immortal).");
        return;
    }
    if (ch.pnote->subject().empty()) {
        ch.send_line("You need to provide a subject.");
        return;
    }
    ch.pnote->date(current_time);
    auto &note = notes_.add(std::move(*ch.pnote));
    ch.pnote.reset();

    on_change_func_(*this);

    for (auto &chtarg : descriptors().all_but(ch) | DescriptorFilter::to_person()) {
        if (!check_enum_bit(chtarg.comm, CommFlag::NoAnnounce) && !check_enum_bit(chtarg.comm, CommFlag::Quiet)
            && note.is_to(chtarg)) {
            chtarg.send_line("The Spirit of Hermes announces the arrival of a new note.");
        }
    }
    ch.send_line("Ok.");
}

void NoteHandler::add_line(Char &ch, ArgParser args) {
    ensure_note(ch).add_line(args.remaining());
    ch.send_line("Ok.");
}

void NoteHandler::remove_line(Char &ch, [[maybe_unused]] ArgParser) {
    if (!ch.pnote || ch.pnote->text().empty()) {
        ch.send_line("There is no text to delete.");
        return;
    }
    ch.pnote->remove_line();
    ch.send_line("Ok.");
}

void NoteHandler::subject(Char &ch, ArgParser args) {
    ensure_note(ch).subject(args.remaining());
    ch.send_line("Ok.");
}

void NoteHandler::to(Char &ch, ArgParser args) {
    ensure_note(ch).to_list(args.remaining());
    ch.send_line("Ok.");
}

void NoteHandler::clear(Char &ch, [[maybe_unused]] ArgParser) {
    ch.pnote.reset();
    ch.send_line("Ok.");
}

void NoteHandler::show(Char &ch, [[maybe_unused]] ArgParser) {
    if (!ch.pnote) {
        ch.send_line("You have no note in progress.");
        return;
    }
    const auto &note = *ch.pnote;
    ch.send_line("{}|w: {}|w", note.sender(), note.subject().empty() ? "(No subject)" : note.subject());
    ch.send_line("To: {}|w", note.to_list().empty() ? "(No recipients)" : note.to_list());
    if (note.text().empty())
        ch.send_line("(No message body)");
    else
        ch.send_to(note.text());

    ch.send_line("|w");
}

void NoteHandler::remove(Char &ch, ArgParser args) {
    auto num = args.try_shift_number();
    if (!num) {
        ch.send_line("Note remove which number?");
        return;
    }
    if (auto *note = notes_.lookup(*num, ch)) {
        remove(ch, *note);
        ch.send_line("Ok.");
    } else {
        ch.send_line("No such note.");
    }
}

void NoteHandler::remove(Char &ch, Note &note) {
    if (note.sent_by(ch)) {
        notes_.erase(note);
        on_change_func_(*this);
        return;
    }

    // Build a new to_list.  Strip out this recipient.
    std::string to_list;
    for (auto recip : ArgParser(note.to_list())) {
        if (matches(ch.name, recip))
            continue;
        if (!to_list.empty())
            to_list += " ";
        to_list += recip;
    }

    // Destroy completely only if there are no recipients left.
    if (!to_list.empty()) {
        note.to_list(to_list);
    } else {
        notes_.erase(note);
    }
    on_change_func_(*this);
}

void NoteHandler::delet(Char &ch, ArgParser args) {
    auto num = args.try_shift_number();
    if (!num) {
        ch.send_line("Note delete which number?");
        return;
    }
    if (auto *note = notes_.lookup(*num, ch)) {
        notes_.erase(*note);
        on_change_func_(*this);
        ch.send_line("Ok.");
    } else {
        ch.send_line("No such note.");
    }
}
