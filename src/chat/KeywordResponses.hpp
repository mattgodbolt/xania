// chris busch

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
    int add_keywords(char *);
    const char *get_keywords() const { return keywords_; }
    void add_response(int weight, char *response_message);
    const WeightedResponse &get_response(int num) const { return responses_[num]; }
    const std::string &get_random_response() const;

private:
    inline static const std::string default_response_ = "Mmm, sounds interesting...";
    const char *keywords_{};
    int total_weight_{};
    std::vector<WeightedResponse> responses_;
};

}
