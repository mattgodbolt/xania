#pragma once

#include "../string_utils.hpp"
#include <unordered_map>

namespace chat {

/**
 * Finds the end position within input_msg that current_db_keyword ends at, with case insensitive matching.
 * If the db keyword starts with = then the input_msg must be an exact match (again, case insensitive).
 * If the db keyword start with ^ then the input_msg must begin with the db keyword.
 * Returns the position at which the remainder of input_msg begins, or 0 if no match was found.
 * The remainder is used in some response messages to echo back the rest of the user's input using the $r variable.
 * Although this could go into string_utils, it's pretty specialized so keeping it here for the time being.
 */
int strpos(std::string_view input_msg, std::string_view current_db_keyword);

/**
 * Walk the characters and append any non-alpha characters to the result string. As
 * soon as it encounters an alpha char, accumulate it in 'word' then attempt to swap
 * the word for its grammatical alternative. It's crude but for basic sentences it works.
 */
std::string swap_pronouns_and_possessives(const std::string &msgbuf);

std::string_view swap_term(const std::string &in);

/**
 * A map of words: if a key is found in the "remaining" input message, it will be swapped
 * out with a replacement. Note that only those responses in the database containing $r
 * actually echo back the user's input including any swapped words.
 * Keys are intentionally lower case as we match on a lower case input.
 */
inline static const std::unordered_map<std::string, std::string> pronoun_and_possessives_{
    {"am", "are"},          {"i", "you"}, {"mine", "yours"}, {"my", "your"}, {"me", "you"},
    {"myself", "yourself"}, {"you", "I"}, {"yours", "mine"}, {"your", "my"}, {"yourself", "myself"}};

}
