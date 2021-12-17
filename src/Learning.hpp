/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2020 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#pragma once

#include "Types.hpp"
#include <string>

class Char;
class Rng;

// Determines if a character has improved at using a skill.
class Learning {
public:
    explicit Learning(Char *ch, const int skill_num, const bool used_successfully, const int difficulty_multiplier,
                      Rng &rng);

    // True if Char can possibly learn and if they actually did.
    [[nodiscard]] bool has_learned() const;
    // Description Char sees when learning.
    [[nodiscard]] std::string learning_message() const;
    // If Char has learned, how many skill points they will improve.
    [[nodiscard]] sh_int gainable_points() const;
    // If Char has learned, how many experience points they will gain.
    [[nodiscard]] ush_int experience_reward() const;

private:
    [[nodiscard]] bool init_has_learned(const int difficulty_multiplier);
    [[nodiscard]] int chance_of_learning() const;
    Char *ch_;
    Rng &rng_;
    const int skill_num_;
    const bool used_successfully_;
    const bool has_learned_;
};
