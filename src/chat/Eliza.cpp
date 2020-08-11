
// Christopher Busch
//(c) 1993 all rights reserved.
// eliza.cpp

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <search.h>

#include "../string_utils.hpp"
#include "Database.hpp"
#include "Eliza.hpp"
#include "chatconstants.hpp"

using namespace chat;

/////YOU MAY NOT change the next 2 lines.
const char eliza_title[] = "chat by Christopher Busch  Copyright (c)1993";
const char eliza_version[] = "version 1.0.0";

// Gets a word out of a string.
int Eliza::get_word(const char *&input, char *outword, char &outother) {
    outword[0] = 0;
    char *outword0 = outword;
    int curchar;
    curchar = input[0];

    while (isalnum(curchar) || curchar == '_') {
        *outword = curchar;
        outword++;
        input++;
        curchar = *input;
    }
    if (*input != '\0')
        input++;
    *outword = '\0';
    outother = curchar;
    if (curchar == 0 && outword0[0] == 0)
        return 0;
    return 1;
}

bool Eliza::register_database_name(char *name, int dbnum) {
    if (num_names_ < MaxNamedDatabases) {
        for (int i = strlen(name) - 1; i >= 0; i--)
            if (name[i] == '_')
                name[i] = ' ';
        return named_databases_[num_names_++].set(name, dbnum) != nullptr;
    }
    return false;
}

int Eliza::compare_database_name(const void *a, const void *b) {
    return strcasecmp(((Eliza::NamedDatabase *)a)->name, ((Eliza::NamedDatabase *)b)->name);
}

void Eliza::sort_databases_by_name() {
    qsort((char *)named_databases_, num_names_, sizeof(Eliza::NamedDatabase), compare_database_name);
}

int Eliza::get_database_num_by_exact_name(const char *n) {
    NamedDatabase finder;
    if (finder.set(n, 0) == nullptr)
        return 0;
    NamedDatabase *found = (NamedDatabase *)bsearch((char *)&finder, (char *)named_databases_, num_names_,
                                                    sizeof(NamedDatabase), compare_database_name);
    if (found == nullptr)
        return 0;
    return found->database_num;
}

// This tries more exhaustively to find the number of the database from a given name
// for example if 'Bobby Jo' fails, it will try Bobby then Jo.
int Eliza::get_database_num_by_partial_name(const char *n) {
    int dbnum = get_database_num_by_exact_name(n);
    if (dbnum == 0) {
        char aword[MaxInputLength + 3];
        char otherch;
        const char *theinput = n;
        while (get_word(theinput, aword, otherch)) {
            dbnum = get_database_num_by_exact_name(aword);
            if (dbnum != 0)
                return dbnum;
        }
    }
    return dbnum;
}

char *Eliza::reduce_spaces(char *outbuf, const char *m) {
    strncpy(outbuf, m, MaxInputLength - 1);
    int len = strlen(outbuf), space = outbuf[0] < ' ', count = 0;
    for (int i = 0; i < len; i++) {
        if (!space || m[i] > ' ') {
            outbuf[count] = (outbuf[i] < ' ' ? ' ' : outbuf[i]);
            count++;
        }
        space = outbuf[i] < ' ';
    }
    if (count)
        outbuf[count] = '\0';
    return outbuf;
}

/**
 * Registers all of the words in 'names' with the specified database number.
 */
bool Eliza::register_database_names(char *names, int dbnum) {
    char name[MaxInputLength + 3];
    char otherch;
    const char *theinput = names;
    while (get_word(theinput, name, otherch)) {
        if (register_database_name(name, dbnum) == false)
            return false;
    }
    return true;
}

char *Eliza::swap_term(char *in) {
    static const char pairs[][10] = {"am", "are", "I", "you", "mine", "yours", "my", "your", "me", "you", "myself",
                                     "yourself",
                                     // swapped order:
                                     "you", "I", "yours", "mine", "your", "my", "yourself", "myself", "", ""};

    for (int i = 0; pairs[i * 2][0] != '\0'; i++) {
        if (strcasecmp(in, pairs[i * 2]) == 0) {
            return (char *)pairs[i * 2 + 1];
        }
    }
    return in;
}

void Eliza::swap_pronouns_and_possessives(char s[]) {
    char buf[MaxInputLength + 20];
    buf[0] = '\0';

    char next_word[MaxInputLength + 3];
    const char *theinput = s;
    char otherch;
    char separator[] = " ";

    while (get_word(theinput, next_word, otherch)) {
        separator[0] = otherch;
        strcat(buf, swap_term(next_word));
        strcat(buf, separator);
    }
    strcpy(s, buf);
}

// enables $variable translation
void Eliza::expand_variables(char *response_buf, const char *npc_name, const std::string &response,
                             const char *player_name, char *rest) {
    trim(rest);
    std::string updated = replace_strings(response, "$t", player_name);
    updated = replace_strings(updated, "$n", npc_name);
    updated = replace_strings(updated, "$$", "$");
    updated = replace_strings(updated, "$r", rest);
    updated = replace_strings(updated, "$A", eliza_title);
    updated = replace_strings(updated, "$V", eliza_version);
    updated = replace_strings(updated, "$C", compile_time_);
    // Until response_buf can be replaced with string, limit its length to the response buffer size.
    std::string truncated = updated.substr(0, MaxChatReplyLength - 1);
    updated.copy(response_buf, truncated.size(), 0);
    response_buf[truncated.size()] = '\0';
}

const char *Eliza::handle_player_message(char *response_buf, const char *player_name, const char *message,
                                         const char *npc_name) {
    char msgbuf[MaxInputLength];

    auto dbnum = get_database_num_by_partial_name(npc_name);
    if (dbnum < 0 || dbnum > num_databases_)
        return "invalid database number";

    if (strlen(message) > MaxInputLength)
        return "You talk too much.";

    strcpy(response_buf, "I dont really know much about that, say more.");

    reduce_spaces(msgbuf, message);
    trim(msgbuf);
    int overflow = 10; // runtime check so we dont have circular database links
    do {
        databases_[dbnum].reset();
        while (databases_[dbnum].current_record()) {
            uint db_keywords_pos = 0, remaining_input_pos = 0;
            if (match(databases_[dbnum].current_record()->keyword_responses.get_keywords(), msgbuf, db_keywords_pos,
                      remaining_input_pos)) {
                auto &response = databases_[dbnum].current_record()->keyword_responses.get_random_response();
                swap_pronouns_and_possessives(&msgbuf[remaining_input_pos]);
                expand_variables(response_buf, npc_name, response, player_name, &msgbuf[remaining_input_pos]);
                return response_buf;
            }
            databases_[dbnum].next_record();
        }
        dbnum = databases_[dbnum].linked_database_num;
        overflow--;
    } while (dbnum >= 0 && overflow > 0);
    if (overflow <= 0) {
        return "potential circular cont (@) jump in databases";
    }
    return response_buf;
}

bool Eliza::load_databases(const char *file, char recurflag) {
    if (num_databases_ >= MaxDatabases)
        return false;

    FILE *data;
    char str[MaxInputLength];
    if ((data = fopen(file, "rt")) == nullptr) {
        return false;
    }
    int linecount = 0;

    while (fgets(str, MaxInputLength - 1, data) != nullptr) {
        linecount++;
        trim(str);
        if (strlen(str) > 0) {
            if (str[0] >= '1' && str[0] <= '9') {
                databases_[num_databases_].current_record()->keyword_responses.add_response(str[0] - '0', &(str[1]));
            } else
                switch (str[0]) {
                case '\0': break;
                case '(':
                    databases_[num_databases_].add_record();
                    databases_[num_databases_].current_record()->keyword_responses.add_keywords(str);
                    break;
                case '#': break;
                case '"': fprintf(stderr, "%s\n", &str[1]); break;
                case '\'': fprintf(stdout, "%s\n", &str[1]); break;
                case '>': // add another database
                    if (num_databases_ < MaxDatabases - 1)
                        num_databases_++;
                    else
                        fputs("response database is full. numdbases>maxdbases", stderr);
                    if (!register_database_names(&str[1], num_databases_)) {
                        puts("name database is full.");
                    }
                    break;
                case '%': // include another file inline
                    trim(str + 1);
                    if (!load_databases(str + 1, 1)) // recurflag set
                        printf("including inline '%s' failed at %d!\n", str + 1, linecount);
                    break;
                case '@': // link the current database to an earlier database to reuse its entries.
                    if (databases_[num_databases_].linked_database_num == NoDatabaseLink) {
                        trim(str + 1);
                        for (char *i = str + 1; *i != 0; i++)
                            if (*i == '_')
                                *i = ' ';
                        sort_databases_by_name();
                        int linked_db_num = get_database_num_by_exact_name(str + 1);
                        databases_[num_databases_].linked_database_num = linked_db_num;
                    } else
                        printf(" @ database already linked %d at %d\n", num_databases_, linecount);
                    break;
                default: printf("extraneous line: '%s' at %d\n", str, linecount);
                }
        }
    }
    fclose(data);
    if (!recurflag) {
        num_databases_++;
        sort_databases_by_name();
    }
    return true;
}

char *Eliza::trim(char str[]) {
    int i, a, ln;
    for (ln = strlen(str); (ln > 0) && (str[ln - 1] <= ' '); ln--)
        ;
    str[ln] = 0;
    for (i = 0; (i < ln) && (str[i] <= ' '); i++)
        ;
    if (i > 0)
        for (ln -= i, a = 0; a <= ln; a++)
            str[a] = str[a + i];
    return str;
}

bool Eliza::eval_operator(const char op, const int a, const int b) {
    switch (op) {
    case '\0':
    case '&': return a && b;
    case '|': return a || b;
    case '~': return a && !b;
    default: return false;
    }
}

int Eliza::strpos(char *input_msg, char *current_db_keyword) {
    auto input_msg_len = strlen(input_msg);
    auto keyword_len = strlen(current_db_keyword);
    size_t a;
    int run;
    int space = 1;
    if (current_db_keyword[0] == '=') { // = exact equality operator
        return strcasecmp(&current_db_keyword[1], input_msg) ? 0 : input_msg_len;
    }
    if (current_db_keyword[0] == '^') { // match start operator
        return strncasecmp(&current_db_keyword[1], input_msg, keyword_len - 1) ? 0 : input_msg_len;
    }
    if (keyword_len > input_msg_len)
        return 0;

    run = input_msg_len - keyword_len;
    for (int i = 0; i <= run; i++) {
        if (space) {
            for (a = 0; a < keyword_len && (tolower(input_msg[i + a]) == current_db_keyword[a]); a++)
                ;
            if (a == keyword_len)
                return (i + a);
        }
        space = input_msg[i] == ' ';
    }

    return 0;
}

#define handle_operator                                                                                                \
    trim(current_db_keyword);                                                                                          \
    if (strlen(current_db_keyword)) {                                                                                  \
        next_match_pos = strpos(input_msg, current_db_keyword);                                                        \
        if (next_match_pos > 0)                                                                                        \
            remaining_input_pos = next_match_pos;                                                                      \
        progressive_match_result = eval_operator(logical_operator, progressive_match_result, next_match_pos);          \
    }                                                                                                                  \
    current_db_keyword_len = current_db_keyword[0] = 0;

int Eliza::match(const char db_keywords[], char input_msg[], uint &db_keywords_pos, uint &remaining_input_pos) {
    auto db_keywords_len = strlen(db_keywords);
    // Records the match result through a sequence of logical expressions (e.g. (foo|bar|baz)
    int progressive_match_result = 1;
    // Records the index into input_msg that the current_db_keyword was matched at, if it was matched,
    // or zero if it didn't match.
    int next_match_pos;
    int logical_operator = '\0';
    char current_db_keyword[MaxInputLength];
    // db_keyword is split on ( ) & | and ~
    // and this var stores a copy of the current token from it.
    current_db_keyword[0] = 0;
    size_t current_db_keyword_len = 0;

    while (db_keywords[db_keywords_pos] != '(')
        db_keywords_pos++;
    db_keywords_pos++;
    for (; db_keywords_pos < db_keywords_len; db_keywords_pos++) {
        switch (db_keywords[db_keywords_pos]) {
        case '(':
            next_match_pos = match(db_keywords, input_msg, db_keywords_pos, remaining_input_pos);
            if (logical_operator == '\0')
                progressive_match_result = next_match_pos;
            else
                progressive_match_result = eval_operator(logical_operator, progressive_match_result, next_match_pos);
            break;
        case '&': handle_operator logical_operator = '&'; break;
        case '|': handle_operator logical_operator = '|'; break;
        case '~': handle_operator logical_operator = '~'; break;
        case ')':
            trim(current_db_keyword);
            if (strlen(current_db_keyword)) {
                next_match_pos = strpos(input_msg, current_db_keyword);
                if (next_match_pos > 0)
                    remaining_input_pos = next_match_pos;
                return eval_operator(logical_operator, progressive_match_result, next_match_pos);
            } else
                return progressive_match_result;

            break;
        default:
            current_db_keyword[current_db_keyword_len++] = db_keywords[db_keywords_pos];
            current_db_keyword[current_db_keyword_len] = '\0';
        }
    }
    return progressive_match_result;
}
