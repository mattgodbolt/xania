#pragma once

#include "KeywordResponses.hpp"
#include <string_view>
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

    char *find_match(char *response_buf, std::string_view player_name, std::string &msgbuf, std::string_view npc_name,
                     int &overflow) const;

private:
    int match(std::string_view db_keywords, std::string_view input_msg, std::string_view::iterator &it,
              uint &remaining_input_pos) const;

    void expand_variables(char *response_buf, std::string_view npc_name, const std::string &response,
                          std::string_view player_name, char *rest) const;
    int strpos(std::string_view input_msg, std::string_view current_db_keyword) const;
    char *swap_term(char *in) const;
    void swap_pronouns_and_possessives(char s[]) const;
    bool eval_operator(const char op, const int a, const int b) const;
    void handle_operator(std::string_view input_msg, std::string_view current_db_keyword, const char logical_operator,
                         int &progressive_match_result, int &next_match_pos, uint &remaining_input_pos) const;

    const std::vector<KeywordResponses> keyword_responses_;
    // An optional non-owning pointer to this database's linked database.
    const Database *linked_database_;

    inline static const std::string compile_time_{__DATE__ " " __TIME__};
    /////YOU MAY NOT change the next 2 lines.
    inline static const std::string eliza_title{"chat by Christopher Busch  Copyright (c)1993"};
    inline static const std::string eliza_version{"version 1.0.0"};
};

}
