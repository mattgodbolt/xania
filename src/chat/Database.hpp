#pragma once

#include "KeywordResponses.hpp"
#include <vector>

namespace chat {

/**
 * A database represents a section of a chat data file. The database contains a vector
 * of KeywordResponses records. The chat engine can have many databases. Each database has one or more
 * names assigned to it. Each assigned name, in the context of the mud, is meant to be
 * the name of an NPC a player talks to or interacts with.
 *
 * A database can optionally be linked to one other database using the @ syntax in the file.
 * This provides a way to share database records between databases.
 */
class Database {
public:
    Database() = default;
    Database(const Database &) = delete;
    Database(Database &&) = delete;
    Database &operator=(const Database &) = delete;
    Database &operator=(Database &) = delete;

    std::vector<KeywordResponses> &keyword_responses() { return keyword_responses_; }
    [[nodiscard]] Database *linked_database() { return linked_database_; }
    void linked_database(Database *linked_database) noexcept { linked_database_ = linked_database; }

private:
    std::vector<KeywordResponses> keyword_responses_;
    // An optional non-owning pointer to this database's linked database.
    Database *linked_database_;
};

}
