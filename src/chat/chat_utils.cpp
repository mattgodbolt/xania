#include "chat_utils.hpp"
#include <utility>

namespace chat {

int strpos(std::string_view input_msg, std::string_view current_db_keyword) {
    auto input_msg_len = input_msg.size();
    auto keyword_len = current_db_keyword.size();
    std::string input_lower = lower_case(input_msg);
    if (current_db_keyword[0] == '=') { // = exact equality operator
        return current_db_keyword.substr(1) == input_lower ? input_msg_len : 0;
    }
    if (current_db_keyword[0] == '^') { // match start operator
        auto pos = input_lower.rfind(current_db_keyword.substr(1), 0);
        if (pos == 0) {
            return input_msg_len;
        } else {
            return 0;
        }
    }

    if (keyword_len > input_msg_len)
        return 0;
    auto pos = input_lower.find(current_db_keyword);
    return pos == std::string::npos ? 0 : pos + keyword_len;
}

std::string_view swap_term(const std::string &msgbuf) {
    auto it = pronoun_and_possessives_.find(msgbuf);
    if (it != pronoun_and_possessives_.end()) {
        return it->second;
    } else {
        return msgbuf;
    }
}

std::string swap_pronouns_and_possessives(const std::string &msgbuf) {
    std::string result{};
    result.reserve(msgbuf.size());
    std::string word{};
    bool in_word{false};
    for (auto &c : msgbuf) {
        if (std::isalpha(c)) {
            in_word = true;
            word.push_back(c);
        } else {
            if (std::exchange(in_word, false)) {
                result.append(swap_term(word));
                word.clear();
            }
            result.push_back(c);
        }
    }
    // Process the final word if there was one at the very end.
    if (in_word) {
        result.append(swap_term(word));
    }
    return result;
}

}
