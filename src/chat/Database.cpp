/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2020 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Chat bot originally written by Chris Busch in 1993-5, this file is a */
/*  reimplementation of that work.                                       */
/*************************************************************************/
#include "Database.hpp"
#include "../string_utils.hpp"
#include "chat_utils.hpp"
#include "chatconstants.hpp"
#include <utility>

using namespace chat;

std::string Database::find_response(std::string_view player_name, const std::string &msgbuf, std::string_view npc_name,
                                    int &overflow) const {
    for (auto &keyword_response : keyword_responses_) {
        auto keywords = keyword_response.get_keywords();
        std::string_view::iterator it = keywords.begin();
        uint remaining_input_pos = 0;
        if (match(keywords, msgbuf, it, remaining_input_pos)) {
            // At this point we've found a match so we're free to copy & modify the original message.
            auto &response = keyword_response.get_random_response();
            std::string remaining_input = ltrim_copy(msgbuf.substr(remaining_input_pos));
            std::string rewritten_input = swap_pronouns_and_possessives(remaining_input);
            return expand_variables(npc_name, response, player_name, rewritten_input);
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
            if (logical_operator == 0)
                progressive_match_result = next_match_pos;
            else
                progressive_match_result = eval_operator(logical_operator, progressive_match_result, next_match_pos);
            break;
        case '&': {
            if (logical_operator == 0)
                progressive_match_result = 1;
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
        progressive_match_result = eval_operator(logical_operator, next_match_pos, progressive_match_result);
    }
}

// enables $variable translation
std::string Database::expand_variables(std::string_view npc_name, const std::string &response,
                                       std::string_view player_name, std::string &remaining_input) const {
    std::string updated{};
    bool last_dollar{false};
    for (auto &c : response) {
        if (c == '$' && !last_dollar) {
            last_dollar = true;
        } else {
            if (std::exchange(last_dollar, false)) {
                switch (c) {
                case 't': updated.append(player_name); break;
                case 'n': updated.append(npc_name); break;
                case '$': updated.push_back(c); break;
                case 'r': updated.append(remaining_input); break;
                case 'A': updated.append(eliza_title); break;
                case 'V': updated.append(help_version_); break;
                case 'C': updated.append(compile_time_); break;
                default:
                    // ignore unrecognized $variables
                    break;
                }
            } else {
                updated.push_back(c);
            }
        }
    }
    return updated;
}
