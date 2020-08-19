
/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2020 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Chat bot originally written by Chris Busch in 1993-5, this file is a */
/*  reimplementation of that work.                                       */
/*************************************************************************/
#pragma once

#include "Database.hpp"
#include <unordered_map>

namespace chat {

class Eliza {
public:
    bool load_databases(std::string_view file);
    /**
     * Handle a message from a player to a talkative NPC. This works
     * by accessing the database associated with the npc_name then looking up
     * within that db an appropriate "keyword response map" based on what the player said/did,
     * then pseudo-randomly selecting a response from the responses and sending it back to the player.
     * It also expands $variables found in the response.
     */
    [[nodiscard]] std::string handle_player_message(std::string_view player_name, std::string_view message,
                                                    std::string_view npc_name);

private:
    std::unordered_map<int, Database> databases_;
    std::unordered_map<std::string, Database &> named_databases_;

    void ensure_database_open(const int current_db_num, const int line_count);
    [[nodiscard]] int parse_new_database_num(std::string_view str, const int line_count);
    Database *parse_database_link(std::string_view str, const int line_count);
    void register_database_names(std::string &names, Database &database);
    [[nodiscard]] Database &get_database_by_name(std::string names);
};

}
