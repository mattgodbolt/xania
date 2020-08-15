
// chris busch (c) copyright 1995 all rights reserved
#pragma once

#include "Database.hpp"
#include "chatconstants.hpp"
#include <cstring>
#include <unordered_map>

namespace chat {

class Eliza {
public:
    bool load_databases(const char *);
    /**
     * Handle a message from a player to a talkative NPC. This works
     * by accessing the database associated with the npc_name then looking up
     * within that db an appropriate "keyword response map" based on what the player said/did,
     * then pseudo-randomly selecting a response from the responses and sending it back to the player.
     * It also expands $variables found in the response.
     */
    const char *handle_player_message(char *response_buf, const char *player_name, std::string_view message,
                                      std::string_view npc_name);

private:
    std::unordered_map<int, Database> databases_;
    std::unordered_map<std::string, Database &> named_databases_;

    void ensure_database_open(const int current_db_num, const int line_count);
    [[nodiscard]] int parse_new_database_num(std::string_view str, const int linecount);
    Database *parse_database_link(std::string_view str, const int linecount);
    void register_database_names(std::string &names, Database &database);
    [[nodiscard]] Database &get_database_by_name(std::string names);
};

}
