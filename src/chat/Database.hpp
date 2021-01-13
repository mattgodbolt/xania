/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2020 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Chat bot originally written by Chris Busch in 1993-5, this file is a */
/*  reimplementation of that work.                                       */
/*************************************************************************/
#pragma once

#include "KeywordResponses.hpp"
#include <string_view>
#include <unordered_map>
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
    explicit Database(std::vector<KeywordResponses> &keyword_responses, Database *linked_database)
        : keyword_responses_(std::move(keyword_responses)), linked_database_(linked_database) {}

    /**
     * The main entry point for interrogating a Database: this attempts to match
     * keywords found in msgbuf and find a pseudo-random response. If no response
     * is found, it'll walk the chain of linked Databases. If there's no response
     * then a default response is returned.
     */
    [[nodiscard]] std::string find_response(std::string_view player_name, const std::string &msgbuf,
                                            std::string_view npc_name, int &overflow) const;

private:
    int match(std::string_view db_keywords, std::string_view input_msg, std::string_view::iterator &it,
              uint &remaining_input_pos) const;

    std::string expand_variables(std::string_view npc_name, const std::string &response, std::string_view player_name,
                                 std::string &remaining_input) const;
    bool eval_operator(const char op, const int a, const int b) const;
    void handle_operator(std::string_view input_msg, std::string_view current_db_keyword, const char logical_operator,
                         int &progressive_match_result, int &next_match_pos, uint &remaining_input_pos) const;

    const std::vector<KeywordResponses> keyword_responses_;
    // An optional non-owning pointer to this database's linked database.
    const Database *linked_database_;

    inline static const std::string compile_time_{__DATE__ " " __TIME__};
    inline static const std::string help_version_{"The version number can be seen using 'help version'."};
    inline static const std::string default_response_{"I don't really know much about that, say more."};
    inline static const std::string eliza_title{
        "Originally by Christopher Busch  Copyright (c)1993. Rewritten by the Xania team."};
};

}
