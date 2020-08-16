/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2020 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Chat bot originally written by Chris Busch in 1993-5, this file is a */
/*  reimplementation of that work.                                       */
/*************************************************************************/
#pragma once

#include <string>
#include <vector>

namespace chat {

/**
 * A response message with an assigned weight that's used when
 * determining a random response to a keyword from the user.
 */
struct WeightedResponse {
    const int weight_;
    const std::string message_;
    explicit WeightedResponse(int weight, std::string_view message) : weight_(weight), message_(message) {}
};

/**
 * Represents an keyword rule that is matched against the message supplied
 * by the player, and the collection of response messages that can be sent
 * if a keyword match is found.  Each keyword rule can contain logical operators.
 * e.g. (what is your name|who are you).
 * Each instance of KeywordResponses belongs to a specific Database.
 */
class KeywordResponses {
public:
    explicit KeywordResponses(std::string_view keywords) : keywords_(keywords) {}
    std::string_view get_keywords() const { return keywords_; }
    void add_response(int weight, std::string_view response_message);
    const std::string &get_random_response() const;

private:
    inline static const std::string default_response_ = "Mmm, sounds interesting...";
    const std::string keywords_{};
    int total_weight_{};
    std::vector<WeightedResponse> responses_;
};

}
