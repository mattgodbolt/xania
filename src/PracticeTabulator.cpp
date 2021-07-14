/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "PracticeTabulator.hpp"
#include "Columner.hpp"
#include "SkillTables.hpp"
#include "skills.hpp"

namespace PracticeTabulator {

void tabulate(Char *ch, const Char *victim) {
    Columner col(*ch, 3);
    for (auto sn = 0; sn < MAX_SKILL; sn++) {
        if (skill_table[sn].name == nullptr)
            break;
        auto skill_level = victim->pcdata->learned[sn]; // NOT victim.get_skill()
        if (victim->level >= get_skill_level(victim, sn) && skill_level > 0)
            col.add("{:<18} {:3}%", skill_table[sn].name, skill_level);
    }
    col.flush();
    ch->send_line("{} have {} practice sessions left.", ch == victim ? "You" : "They", victim->practice);
}

void tabulate(Char *ch) { tabulate(ch, ch); }

}