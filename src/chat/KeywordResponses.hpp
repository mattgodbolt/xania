// chris busch

#pragma once

namespace chat {

/**
 * A response message with an assigned weight that's used when
 * determining a random response to a keyword from the user.
 */
struct WeightedResponse {
    int weight_;
    char *message_;
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
    char *get_keywords() { return keywords_; }
    int add_response(int weight, char *response_message);
    WeightedResponse &get_response(int num) { return responses_[num]; }
    const char *get_random_response();

private:
    char *keywords_{};
    int num_responses_{};
    int total_weight_{};
    WeightedResponse *responses_{};
};

}
