
// chris busch (c) copyright 1995 all rights reserved
#pragma once

#include "Database.hpp"
#include <cstdlib>
#include <cstring>

constexpr inline auto MaxDatabases = 100;
constexpr inline auto MaxNamedDatabases = 200;
constexpr inline auto MaxInputLength = 100;

namespace chat {

class Eliza {
public:
    /**
     * When loading the chat database file, this is used to map the name of a mob
     * to the index number of a database that contains the keywords that it will
     * recognise and the list of responses it can use.
     */
    struct NamedDatabase {
        char *name{};
        int database_num{};

        char *set(const char *a_name, int dbnum) {
            name = strdup(a_name);
            database_num = dbnum;
            return name;
        }
        ~NamedDatabase() {
            if (name)
                free(name);
        }
    };

    Eliza() {
        char de[] = "default";
        register_database_name(de, 0);
    }

    bool load_databases(const char *, char recurflag = 0);
    /**
     * Handle a message from a player to a talkative NPC. This works
     * by accessing the database associated with the npc_name then looking up
     * within that db an appropriate "keyword response map" based on what the player said/did,
     * then pseudo-randomly selecting a response from the responses and sending it back to the player.
     * It also expands $variables found in the response.
     */
    const char *handle_player_message(char *response_buf, const char *player_name, const char *message,
                                      const char *npc_name);

private:
    int num_databases_{};
    unsigned int num_names_{};
    Database databases_[MaxDatabases];
    NamedDatabase named_databases_[MaxNamedDatabases];

    int get_word(const char *&input, char *outword, char &outother);
    char *trim(char str[]);
    bool eval_operator(const char op, const int a, const int b);
    int strpos(char *s, char *sub);
    int match(const char db_keyword[], char input_msg[], uint &db_keyword_pos, uint &remaining_input_pos);

    bool register_database_name(char *name, int dbnum);
    bool register_database_names(char *names, int dbnum);
    void sort_databases_by_name();
    int get_database_num_by_exact_name(const char *);
    int get_database_num_by_partial_name(const char *n);
    char *reduce_spaces(char *outbuf, const char *);
    void expand_variables(char *replied, const char *talker, const char *rep, const char *target, char *rest);
    char *swap_term(char *in);
    void swap_pronouns_and_possessives(char s[]);
    static int compare_database_name(const void *a, const void *b);
};

}
