/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  note.h: notes system header file                                     */
/*                                                                       */
/*************************************************************************/

#pragma once

#include "ArgParser.hpp"
#include "CommandSet.hpp"
#include "Logging.hpp"
#include "common/Time.hpp"

#include <functional>
#include <list>
#include <string>
#include <string_view>
#include <utility>

struct Char;

// Data structure for notes.
class Note {
    std::string sender_;
    std::string date_;
    Time date_stamp_;
    std::string subject_;
    std::string to_list_;
    std::string text_;

public:
    explicit Note(std::string sender) : sender_(std::move(sender)) {}

    [[nodiscard]] bool is_to(const Char &ch) const;
    [[nodiscard]] bool sent_by(const Char &ch) const;

    [[nodiscard]] const std::string &sender() const { return sender_; }
    [[nodiscard]] const std::string &date() const { return date_; }
    [[nodiscard]] Time date_stamp() const { return date_stamp_; }
    [[nodiscard]] const std::string &subject() const { return subject_; }
    [[nodiscard]] const std::string &to_list() const { return to_list_; }
    [[nodiscard]] const std::string &text() const { return text_; }

    void subject(std::string_view subject);
    void date(Time point);
    void to_list(std::string_view to_list);

    void add_line(std::string_view line);
    void remove_line();

    void save(FILE *file) const;
    static Note from_file(FILE *file, const Logger &logger);
};

class Notes {
    std::list<Note> notes_;

public:
    Note &add(Note note);
    void erase(const Note &note);

    [[nodiscard]] int num_unread(const Char &ch) const;
    [[nodiscard]] Note *lookup(int index, const Char &ch);
    [[nodiscard]] std::pair<Note *, int> lookup(Time date, const Char &ch);

    [[nodiscard]] const std::list<Note> &notes() const { return notes_; }

    void save(FILE *file) const;
};

class NoteHandler {
public:
    using OnChangeFunc = std::function<void(NoteHandler &)>;

private:
    const std::string notes_file_;
    OnChangeFunc on_change_func_;
    Notes notes_;
    CommandSet<void (NoteHandler::*)(Char &, ArgParser)> sub_commands;

public:
    explicit NoteHandler(const std::string &notes_file);
    // For testing
    explicit NoteHandler(const std::string &notes_file, OnChangeFunc on_change_func);
    void add_for_testing(Note note) { notes_.add(std::move(note)); }
    size_t load(const std::string &notes_file, const Time current_time, const Logger &logger);
    void write_to(FILE *fp);
    void on_command(Char &ch, ArgParser args);
    [[nodiscard]] int num_unread(const Char &ch) const { return notes_.num_unread(ch); }

    [[nodiscard]] const Notes &notes() const { return notes_; }

private:
    OnChangeFunc on_note_change();
    void read_from(FILE *fp, Time current_time, const Logger &logger);
    void read(Char &ch, ArgParser args);
    void list(Char &ch, ArgParser args);
    void add_line(Char &ch, ArgParser args);
    void remove_line(Char &ch, ArgParser args);
    void subject(Char &ch, ArgParser args);
    void to(Char &ch, ArgParser args);
    void clear(Char &ch, ArgParser args);
    void show(Char &ch, ArgParser args);
    void post(Char &ch, ArgParser args);
    void remove(Char &ch, ArgParser args);
    void remove(Char &ch, Note &note);
    void delet(Char &ch, ArgParser args);
};

void do_note(Char *ch, std::string_view argument);
