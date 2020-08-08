// akey.cpp

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "KeywordResponses.hpp"

using namespace chat;

namespace {

int random(int x) { return rand() % x; }

}

const char *KeywordResponses::get_random_response() {
    const char *defaultmsg = "Mmm, sounds interesting...";
    if (total_weight_ == 0)
        return defaultmsg;
    int rnd_num = random(total_weight_), total = 0;
    for (int i = 0; i < num_responses_; i++) {
        if ((total += (responses_[i].weight_)) > rnd_num) {
            return responses_[i].message_;
        }
    }
    return defaultmsg;
}

int KeywordResponses::add_keywords(char *keyword) { return (keywords_ = strdup(keyword)) != nullptr; }

int KeywordResponses::add_response(int w, char *response_message) {
    if (responses_ == nullptr) {
        if ((responses_ = (WeightedResponse *)malloc((num_responses_ + 1) * sizeof(WeightedResponse))) == nullptr) {
            return 0;
        }
    } else if ((responses_ = (WeightedResponse *)realloc(responses_, (num_responses_ + 1) * sizeof(WeightedResponse)))
               == nullptr) {
        return 0;
    }
    total_weight_ += w;
    responses_[num_responses_].weight_ = w;
    if ((responses_[num_responses_].message_ = strdup(response_message)) == nullptr) {
        return 0;
    }
    num_responses_++;
    return 1;
}
