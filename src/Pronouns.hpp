#pragma once

#include "Char.hpp"

#include <string_view>

struct Pronouns {
    std::string_view possessive;
    std::string_view subjective;
    std::string_view objective;
};

inline const Pronouns &pronouns_for(int sex) { // TODO: strong type for sex.
    static constexpr Pronouns male{"his", "him", "he"};
    static constexpr Pronouns female{"her", "her", "she"};
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

inline const Pronouns &pronouns_for(const Char &ch) { return pronouns_for(ch.sex); }

inline auto possessive(const Char &ch) { return pronouns_for(ch).possessive; }
inline auto subjective(const Char &ch) { return pronouns_for(ch).subjective; }
inline auto objective(const Char &ch) { return pronouns_for(ch).objective; }
