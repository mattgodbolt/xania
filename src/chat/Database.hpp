#pragma once

#include "KeywordResponses.hpp"

namespace chat {

/**
 * Databases may be configured to link to one other database.
 * This indicates that a Database does not have a link.
 */
constexpr inline auto NoDatabaseLink = -1;

/**
 * A Record is a node in a linked list of KeywordResponses.
 */
struct Record {
    KeywordResponses keyword_responses;
    Record *next_;
};

/**
 * A database represents a section a chat data file. The database contains a linked list
 * of database records. The engine can have many databases. Each database has one or more
 * names assigned to it. Each assigned name, in the context of the mud, is meant to be
 * the name of an NPC a player talks to or interacts with.
 *
 * A database can optionally be linked to one other database using the @ syntax in the file.
 * This provides a way to share database records between databases.
 */
class Database {
public:
    int linked_database_num;
    Database() : linked_database_num(NoDatabaseLink) {}
    [[nodiscard]] Record *current_record() { return current_; }
    void add_record();
    void reset();
    void next_record();
    ~Database();

private:
    Record *first_{};
    Record *current_{};
    Record *top_{};
    int record_count_{};
};

}
