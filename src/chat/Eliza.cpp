
// Christopher Busch
//(c) 1993 all rights reserved.
// eliza.cpp

#include <cctype>
#include <cstdio>
#include <cstring>
#include <search.h>

#include "../string_utils.hpp"
#include "Database.hpp"
#include "Eliza.hpp"
#include "chatconstants.hpp"
#include "utils.hpp"

using namespace chat;

const char *Eliza::handle_player_message(char *response_buf, std::string_view player_name, std::string_view message,
                                         std::string_view npc_name) {
    if (message.size() > MaxInputLength)
        return "You talk too much.";
    std::string npc_name_str(npc_name);
    auto &database = get_database_by_name(npc_name_str);

    strcpy(response_buf, "I dont really know much about that, say more.");
    std::string msgbuf = reduce_spaces(message);
    int overflow = 10; // runtime check so we dont have circular database links
    return database.find_match(response_buf, player_name, msgbuf, npc_name, overflow);
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
    // Split on space and register each word to the database.
    // The individual words used when searching for a database
    // by words in an NPC name.
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
        printf("no database is being loaded at %d\n", line_count);
        exit(1);
    }
}

bool Eliza::load_databases(const char *file) {
    FILE *data;
    char str[MaxInputLength];
    if ((data = fopen(file, "rt")) == nullptr) {
        return false;
    }
    int line_count = 0;
    int current_db_num = -1;
    std::string current_database_names{};
    std::vector<KeywordResponses> current_keyword_responses{};
    Database *current_linked_database{nullptr};

    while (fgets(str, MaxInputLength - 1, data) != nullptr) {
        line_count++;
        trim(str);
        if (strlen(str) > 0) {
            if (str[0] >= '1' && str[0] <= '9') {
                ensure_database_open(current_db_num, line_count);
                // Add a new WeightedResponse to the current KeywordResponses entry.
                current_keyword_responses.back().add_response(str[0] - '0', &(str[1]));
            } else
                switch (str[0]) {
                case '\0': break;
                case '(':
                    ensure_database_open(current_db_num, line_count);
                    // Add a new KeywordResponses object to the current keyword_responses.
                    current_keyword_responses.emplace_back(str);
                    break;
                case '#': break;
                case '"': fprintf(stderr, "%s\n", &str[1]); break;
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
                    trim(str + 1);
                    if (current_db_num >= 0) {
                        printf("& another database is being loaded at %d\n", line_count);
                        exit(1);
                    }
                    current_db_num = parse_new_database_num(str + 1, line_count);
                    if (current_db_num < 0) {
                        printf("& bad database number %s at %d\n", str + 1, line_count);
                        exit(1);
                    }
                    break;
                case '>':
                    // Store a copy of the words that will trigger use of the current database,
                    // these are parsed and associated to the database in the completion stage.
                    ensure_database_open(current_db_num, line_count);
                    current_database_names = &str[1];
                    break;
                case '@': // link the current database to an earlier database to reuse its entries.
                    ensure_database_open(current_db_num, line_count);
                    if (current_linked_database == nullptr) {
                        trim(str + 1);
                        current_linked_database = parse_database_link(str + 1, line_count);
                    } else {
                        printf("@ database %d already has a database link at %d\n", current_db_num, line_count);
                    }
                    break;
                default: printf("extraneous line: '%s' at %d\n", str, line_count);
                }
        }
    }
    fclose(data);
    return true;
}

int Eliza::parse_new_database_num(std::string_view str, const int line_count) {
    if (is_number(str.data())) {
        auto db_num = atoi(str.data());
        if (databases_.find(db_num) != databases_.end()) {
            printf("& database num already exists! %d on line %d\n", db_num, line_count);
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
        printf("@ database could not be found: %d on line %d\n", db_num, line_count);
        return nullptr;
    }
    printf("@ invalid database link number %s on line %d\n", str.data(), line_count);
    return nullptr;
}
