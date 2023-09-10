/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2020 Xania Development Team                                 */
/*  See the header to file: merc.h for original code copyrights          */
/*  Chat bot originally written by Chris Busch in 1993-5, this file is a */
/*  reimplementation of that work.                                       */
/*************************************************************************/
#include "KeywordResponses.hpp"

using namespace chat;

namespace {

int random(int x) { return rand() % x; }

}

const std::string &KeywordResponses::get_random_response() const {
    if (total_weight_ == 0)
        return default_response_;
    int rnd_num = random(total_weight_), total = 0;
    for (size_t i = 0; i < responses_.size(); i++) {
        if ((total += (responses_[i].weight_)) > rnd_num) {
            return responses_[i].message_;
        }
    }
    return default_response_;
}

void KeywordResponses::add_response(int weight, std::string_view response_message) {
    total_weight_ += weight;
    responses_.emplace_back(weight, response_message);
}
