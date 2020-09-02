/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2020 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Chat bot originally written by Chris Busch in 1993-5, this file is a */
/*  reimplementation of that work.                                       */
/*************************************************************************/
#include <fmt/format.h>
#include <fstream>

#include "../Logging.hpp"
#include "../string_utils.hpp"
#include "Database.hpp"
#include "Eliza.hpp"
#include "chatconstants.hpp"

using namespace chat;

std::string Eliza::handle_player_message(std::string_view player_name, std::string_view message,
                                         std::string_view npc_name) {
    if (message.size() > MaxInputLength)
        return "You talk too much.";
    std::string npc_name_str(npc_name);
    auto &database = get_database_by_name(npc_name_str);

    std::string msgbuf = reduce_spaces(message);
    int overflow = 10; // runtime check so we dont have circular database links
    return database.find_response(player_name, msgbuf, npc_name, overflow);
}

/**
 * Find the number of a database from a given name by splitting
 * it on spaces and looking for each word.
 * If no database is found it returns the default database.
 */
Database &Eliza::get_database_by_name(std::string names) {
    std::string::size_type pos = 0, last = 0;
    while ((pos = names.find(" ", last)) != std::string::npos) {
        std::string name(lower_case(names.substr(last, pos - last)));
        auto entry = named_databases_.find(name);
        if (entry != named_databases_.end())
            return entry->second;
        last = ++pos;
    }
    std::string name(lower_case(names.substr(last, names.size() - pos)));
    auto entry = named_databases_.find(name);
    if (entry != named_databases_.end())
        return entry->second;
    return databases_[0]; // Default database
}

/**
 * Registers all of the words in 'names' with the specified Database reference.
 */
void Eliza::register_database_names(std::string &names, Database &database) {
    std::string::size_type pos = 0, last = 0;
    while ((pos = names.find(" ", pos)) != std::string::npos) {
        std::string name(lower_case(names.substr(last, pos - last)));
        named_databases_.emplace(name, database);
        last = ++pos;
    }
    std::string name(lower_case(names.substr(last, names.size() - pos)));
    if (!name.empty())
        named_databases_.emplace(name, database);
}

void Eliza::ensure_database_open(const int current_db_num, const int line_count) {
    if (current_db_num < 0) {
        log_string("no database is being loaded at #{}", line_count);
        exit(1);
    }
}

bool Eliza::load_databases(std::string_view file) {
    std::ifstream fs(file.data());
    if (!fs.is_open()) {
        log_string("Could not open chat database {}", file);
        return false;
    }
    int line_count = 0;
    int current_db_num = -1;
    std::string current_database_names{};
    std::vector<KeywordResponses> current_keyword_responses{};
    Database *current_linked_database{nullptr};
    std::string line{};
    while (std::getline(fs, line)) {
        line_count++;
        line = trim(line);
        if (!line.empty()) {
            if (line[0] >= '1' && line[0] <= '9') {
                ensure_database_open(current_db_num, line_count);
                current_keyword_responses.back().add_response(line[0] - '0', line.substr(1));
            } else
                switch (line[0]) {
                case '\0': break;
                case '(':
                    ensure_database_open(current_db_num, line_count);
                    current_keyword_responses.emplace_back(line);
                    break;
                case '#': break;
                case '"': log_string("{}", line.substr(1)); break;
                case '$':
                    // Complete the current database by emplacing a new Database in the map
                    // and moving the locals into it.
                    ensure_database_open(current_db_num, line_count);
                    databases_.emplace(current_db_num, Database{current_keyword_responses, current_linked_database});
                    register_database_names(current_database_names, databases_[current_db_num]);
                    current_db_num = -1;
                    current_linked_database = nullptr;
                    break;
                case '&': // Start a new database.
                    if (current_db_num >= 0) {
                        log_string("& another database is being loaded at #{}", line_count);
                        exit(1);
                    }
                    current_db_num = parse_new_database_num(line.substr(1), line_count);
                    if (current_db_num < 0) {
                        log_string("& bad database number {} at #{}", line.substr(1), line_count);
                        exit(1);
                    }
                    break;
                case '>':
                    // Store a copy of the words that will trigger use of the current database,
                    // these are parsed and associated to the database in the completion stage.
                    ensure_database_open(current_db_num, line_count);
                    current_database_names = line.substr(1);
                    break;
                case '@': // link the current database to an earlier database to reuse its entries.
                    ensure_database_open(current_db_num, line_count);
                    if (current_linked_database == nullptr) {
                        current_linked_database = parse_database_link(line.substr(1), line_count);
                    } else {
                        log_string(fmt::format("@ database #{} already has a database link at #{}", current_db_num,
                                               line_count));
                    }
                    break;
                default: log_string("extraneous line: {} at #{}", line, line_count);
                }
        }
    }
    return true;
}

int Eliza::parse_new_database_num(std::string_view str, const int line_count) {
    if (is_number(str.data())) {
        auto db_num = atoi(str.data());
        if (databases_.find(db_num) != databases_.end()) {
            log_string("& database num already exists! #{} on line #{}", db_num, line_count);
            return -1;
        }
        return db_num;
    }
    return -1;
}

Database *Eliza::parse_database_link(std::string_view str, const int line_count) {
    if (is_number(str.data())) {
        auto db_num = atoi(str.data());
        auto entry = databases_.find(db_num);
        if (entry != databases_.end()) {
            return &entry->second;
        }
        log_string("@ database could not be found: #{} on line #{}", db_num, line_count);
        return nullptr;
    }
    log_string("@ invalid database link number {} on line #{}", str, line_count);
    return nullptr;
}
