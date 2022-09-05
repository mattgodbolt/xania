/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include <map>
#include <string>
#include <vector>

struct Char;

class SkillsTabulator {
public:
    // Create a tabulator that shows victim's skills to ch.
    [[nodiscard]] static SkillsTabulator skills(Char *ch, const Char *victim);
    // Create a tabulator that shows ch's skills to ch.
    [[nodiscard]] static SkillsTabulator skills(Char *ch);
    // Create a tabulator that shows victim's spells to ch.
    [[nodiscard]] static SkillsTabulator spells(Char *ch, const Char *victim);
    // Create a tabulator that shows ch's spells to ch.
    [[nodiscard]] static SkillsTabulator spells(Char *ch);

    enum class Mode { SkillsOnly = 0, SpellsOnly = 1 };

    SkillsTabulator(Char *ch, const Char *victim, const Mode mode);
    // Output's victim's skills or spells to ch. Returns false if victim
    // had no spells or spells.
    bool tabulate() const;

private:
    void init_skill_map();
    std::vector<int> &vector_for_key(const int skill_num);
    std::string describe_skill(const int skill_num, const int level) const;

    Char *ch_;
    const Char *victim_;
    const Mode mode_;
    std::map<int, std::vector<int>> level_to_skill_nums_{};
};
