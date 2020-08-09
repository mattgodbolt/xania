// akey.cpp

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "KeywordResponses.hpp"

using namespace chat;

namespace {

int random(int x) { return rand() % x; }

}

const char *KeywordResponses::get_random_response() const {
    const char *defaultmsg = "Mmm, sounds interesting...";
    if (total_weight_ == 0)
        return defaultmsg;
    int rnd_num = random(total_weight_), total = 0;
    for (size_t i = 0; i < responses_.size(); i++) {
        if ((total += (responses_[i].weight_)) > rnd_num) {
            return responses_[i].message_.c_str();
        }
    }
    return defaultmsg;
}

int KeywordResponses::add_keywords(char *keyword) { return (keywords_ = strdup(keyword)) != nullptr; }

void KeywordResponses::add_response(int weight, char *response_message) {
    total_weight_ += weight;
    responses_.emplace_back(weight, response_message);
}
