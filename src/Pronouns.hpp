#pragma once

#include "CHAR_DATA.hpp"

#include <string_view>

struct Pronouns {
    std::string_view possessive;
    std::string_view subjective;
    std::string_view objective;
};

inline const Pronouns &pronouns_for(int sex) { // TODO: strong type for sex.
    static constexpr Pronouns male{"his", "him", "he"};
    static constexpr Pronouns female{"hers", "her", "she"};
    static constexpr Pronouns neutral{"its", "it", "it"};
    switch (sex) {
    case 0: return neutral;
    case 1: return male;
    case 2: return female;
    default: break;
    }
    // TODO: reintroduce? throw? out of line?
    //    bug("%s", "Bad sex {} in pronouns "_format(ch->sex).c_str());
    return neutral;
}

inline const Pronouns &pronouns_for(const CHAR_DATA &ch) { return pronouns_for(ch.sex); }

inline auto possessive(const CHAR_DATA &ch) { return pronouns_for(ch).possessive; }
inline auto subjective(const CHAR_DATA &ch) { return pronouns_for(ch).subjective; }
inline auto objective(const CHAR_DATA &ch) { return pronouns_for(ch).objective; }
