/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2020 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Chat bot originally written by Chris Busch in 1993-5, this file is a */
/*  reimplementation of that work.                                       */
/*************************************************************************/
#include "Database.hpp"
#include "../string_utils.hpp"
#include "chatconstants.hpp"
#include "utils.hpp"
#include <cstring>

using namespace chat;

std::string Database::find_response(std::string_view player_name, std::string &msgbuf, std::string_view npc_name,
                                 int &overflow) const {
    for (auto &keyword_response : keyword_responses_) {
        auto keywords = keyword_response.get_keywords();
        std::string_view::iterator it = keywords.begin();
        uint remaining_input_pos = 0;
        if (match(keywords, msgbuf, it, remaining_input_pos)) {
            auto &response = keyword_response.get_random_response();
            swap_pronouns_and_possessives(&msgbuf[remaining_input_pos]);
            return expand_variables(npc_name, response, player_name, &msgbuf[remaining_input_pos]);
        }
    }
    if (--overflow <= 0 || !linked_database_) {
        return default_response_;
    } else {
        const Database &next_database = *linked_database_;
        return next_database.find_response(player_name, msgbuf, npc_name, overflow);
    }
}

bool Database::eval_operator(const char op, const int a, const int b) const {
    switch (op) {
    case '\0':
    case '&': return a && b;
    case '|': return a || b;
    case '~': return a && !b;
    default: return false;
    }
}

int Database::strpos(std::string_view input_msg, std::string_view current_db_keyword) const {
    auto input_msg_len = input_msg.size();
    auto keyword_len = current_db_keyword.size();
    size_t a;
    int run;
    int space = 1;
    if (current_db_keyword[0] == '=') { // = exact equality operator
        return strcasecmp(&current_db_keyword[1], input_msg.data()) ? 0 : input_msg_len;
    }
    if (current_db_keyword[0] == '^') { // match start operator
        return strncasecmp(&current_db_keyword[1], input_msg.data(), keyword_len - 1) ? 0 : input_msg_len;
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

int Database::match(std::string_view db_keywords, std::string_view input_msg, std::string_view::iterator &it,
                    uint &remaining_input_pos) const {
    // Records the match result through a sequence of logical expressions (e.g. (foo|bar|baz)
    int progressive_match_result = 0;
    // Records the index into input_msg that the current_db_keyword was matched at, if it was matched,
    // or zero if it didn't match.
    int next_match_pos = 0;
    char logical_operator = 0;
    // db_keywords is split on ( ) & | and ~
    // and current_db_keyword var stores a copy of the current token from it.
    std::string current_db_keyword{};
    for (; it != db_keywords.end(); it++) {
        switch (*it) {
        case '(':
            next_match_pos = match(db_keywords, input_msg, ++it, remaining_input_pos);
            if (logical_operator == '\0')
                progressive_match_result = next_match_pos;
            else
                progressive_match_result = eval_operator(logical_operator, progressive_match_result, next_match_pos);
            break;
        case '&': {
            logical_operator = '&';
            current_db_keyword = reduce_spaces(current_db_keyword);
            handle_operator(input_msg, current_db_keyword, logical_operator, progressive_match_result, next_match_pos,
                            remaining_input_pos);
            current_db_keyword.clear();
            break;
        }
        case '|': {
            logical_operator = '|';
            current_db_keyword = reduce_spaces(current_db_keyword);
            handle_operator(input_msg, current_db_keyword, logical_operator, progressive_match_result, next_match_pos,
                            remaining_input_pos);
            current_db_keyword.clear();
            break;
        }
        case '~': {
            logical_operator = '~';
            handle_operator(input_msg, current_db_keyword, logical_operator, progressive_match_result, next_match_pos,
                            remaining_input_pos);
            current_db_keyword.clear();
            break;
        }
        case ')':
            current_db_keyword = reduce_spaces(current_db_keyword);
            if (!current_db_keyword.empty()) {
                // If we encountered no logic in the current db_keywords at this level of recursion
                // and hit a closing paren, set this to 1 as it will be anded with the result of strpos()
                if (logical_operator == 0)
                    progressive_match_result = 1;
                next_match_pos = strpos(input_msg, current_db_keyword);
                if (next_match_pos > 0)
                    remaining_input_pos = next_match_pos;
                return eval_operator(logical_operator, progressive_match_result, next_match_pos);
            } else
                return progressive_match_result;

            break;
        default: current_db_keyword.push_back(*it);
        }
    }
    return progressive_match_result;
}

void Database::handle_operator(std::string_view input_msg, std::string_view current_db_keyword,
                               const char logical_operator, int &progressive_match_result, int &next_match_pos,
                               uint &remaining_input_pos) const {
    if (!current_db_keyword.empty()) {
        next_match_pos = strpos(input_msg, current_db_keyword);
        if (next_match_pos > 0)
            remaining_input_pos = next_match_pos;
        progressive_match_result = eval_operator(logical_operator, progressive_match_result, next_match_pos);
    }
}

char *Database::swap_term(char *in) const {
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

void Database::swap_pronouns_and_possessives(char s[]) const {
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
std::string Database::expand_variables(std::string_view npc_name, const std::string &response,
                                       std::string_view player_name, char *rest) const {
    trim(rest);
    std::string updated = replace_strings(response, "$t", player_name);
    updated = replace_strings(updated, "$n", npc_name);
    updated = replace_strings(updated, "$$", "$");
    updated = replace_strings(updated, "$r", rest);
    updated = replace_strings(updated, "$A", eliza_title);
    updated = replace_strings(updated, "$V", help_version_);
    updated = replace_strings(updated, "$C", compile_time_);
    return updated;
}
