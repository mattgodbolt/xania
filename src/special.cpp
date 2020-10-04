/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  special.c:  special func's for mobiles...generally magic.            */
/*                                                                       */
/*************************************************************************/

#include "Logging.hpp"
#include "TimeInfoData.hpp"
#include "comm.hpp"
#include "db.h"
#include "interp.h"
#include "lookup.h"
#include "magic.h"
#include "merc.h"

#include <fmt/format.h>

/* The following special functions are available for mobiles. */
/* Note that MOB special functions are called every 4 seconds. */
bool spec_breath_any(Char *ch);
bool spec_breath_acid(Char *ch);
bool spec_breath_fire(Char *ch);
bool spec_breath_frost(Char *ch);
bool spec_breath_gas(Char *ch);
bool spec_breath_lightning(Char *ch);
bool spec_cast_adept(Char *ch);
bool spec_cast_cleric(Char *ch);
bool spec_cast_judge(Char *ch);
bool spec_cast_mage(Char *ch);
bool spec_cast_undead(Char *ch);
bool spec_cast_bastard(Char *ch);
bool spec_executioner(Char *ch);
bool spec_fido(Char *ch);
bool spec_guard(Char *ch);
bool spec_janitor(Char *ch);
bool spec_mayor(Char *ch);
bool spec_poison(Char *ch);
bool spec_thief(Char *ch);
bool spec_puff(Char *ch);
bool spec_DEATH(Char *ch);
bool spec_greasy_joe(Char *ch);
bool spec_phil(Char *ch);
bool spec_concordius(Char *ch);
bool spec_aquila_pet(Char *ch);
bool spec_summoner(Char *ch);
/* Given a name, return the appropriate spec fun. */
SpecialFunc spec_lookup(const char *name) {
    if (!str_cmp(name, "spec_breath_any"))
        return spec_breath_any;
    if (!str_cmp(name, "spec_breath_acid"))
        return spec_breath_acid;
    if (!str_cmp(name, "spec_breath_fire"))
        return spec_breath_fire;
    if (!str_cmp(name, "spec_breath_frost"))
        return spec_breath_frost;
    if (!str_cmp(name, "spec_breath_gas"))
        return spec_breath_gas;
    if (!str_cmp(name, "spec_breath_lightning"))
        return spec_breath_lightning;
    if (!str_cmp(name, "spec_cast_adept"))
        return spec_cast_adept;
    if (!str_cmp(name, "spec_cast_cleric"))
        return spec_cast_cleric;
    if (!str_cmp(name, "spec_cast_judge"))
        return spec_cast_judge;
    if (!str_cmp(name, "spec_cast_mage"))
        return spec_cast_mage;
    if (!str_cmp(name, "spec_cast_undead"))
        return spec_cast_undead;
    if (!str_cmp(name, "spec_cast_bastard"))
        return spec_cast_bastard;
    if (!str_cmp(name, "spec_executioner"))
        return spec_executioner;
    if (!str_cmp(name, "spec_fido"))
        return spec_fido;
    if (!str_cmp(name, "spec_guard"))
        return spec_guard;
    if (!str_cmp(name, "spec_janitor"))
        return spec_janitor;
    if (!str_cmp(name, "spec_mayor"))
        return spec_mayor;
    if (!str_cmp(name, "spec_poison"))
        return spec_poison;
    if (!str_cmp(name, "spec_thief"))
        return spec_thief;
    if (!str_cmp(name, "spec_DEATH"))
        return spec_DEATH;
    if (!str_cmp(name, "spec_puff"))
        return spec_puff;
    if (!str_cmp(name, "spec_greasy_joe"))
        return spec_greasy_joe;
    if (!str_cmp(name, "spec_phil"))
        return spec_phil;
    if (!str_cmp(name, "spec_concordius"))
        return spec_concordius;
    if (!str_cmp(name, "spec_aquila_pet"))
        return spec_aquila_pet;
    if (!str_cmp(name, "spec_summoner"))
        return spec_summoner;
    return 0;
}

/* Core procedure for dragons. */
bool dragon(Char *ch, const char *spell_name) {
    Char *victim;
    Char *v_next;
    int sn;

    if (ch->position != POS_FIGHTING)
        return false;

    for (victim = ch->in_room->people; victim != nullptr; victim = v_next) {
        v_next = victim->next_in_room;
        if (victim->fighting == ch && number_bits(3) == 0)
            break;
    }

    if (victim == nullptr)
        return false;

    if ((sn = skill_lookup(spell_name)) < 0)
        return false;
    (*skill_table[sn].spell_fun)(sn, ch->level, ch, victim);
    return true;
}

/* Special procedures for mobiles. */
bool spec_breath_any(Char *ch) {
    if (ch->position != POS_FIGHTING)
        return false;

    switch (number_bits(3)) {
    case 0: return spec_breath_fire(ch);
    case 1:
    case 2: return spec_breath_lightning(ch);
    case 3: return spec_breath_gas(ch);
    case 4: return spec_breath_acid(ch);
    case 5:
    case 6:
    case 7: return spec_breath_frost(ch);
    }

    return false;
}

bool spec_breath_acid(Char *ch) { return dragon(ch, "acid breath"); }

bool spec_breath_fire(Char *ch) { return dragon(ch, "fire breath"); }

bool spec_breath_frost(Char *ch) { return dragon(ch, "frost breath"); }

bool spec_breath_gas(Char *ch) {
    int sn;

    if (ch->position != POS_FIGHTING)
        return false;

    if ((sn = skill_lookup("gas breath")) < 0)
        return false;
    (*skill_table[sn].spell_fun)(sn, ch->level, ch, nullptr);
    return true;
}

bool spec_breath_lightning(Char *ch) { return dragon(ch, "lightning breath"); }

bool spec_DEATH(Char *ch) {
    Char *lowest_person = nullptr;
    Char *phil;
    ROOM_INDEX_DATA *home; /* Death's house */
    int lowest_percent = 15; /* Lowest percentage of hp Death gates to */

    if (ch->position < POS_STANDING)
        return false;

    if ((home = get_room_index(ROOM_VNUM_DEATH)) == nullptr) {
        bug("Couldn't get Death's home index.");
        return false;
    }

    for (auto *victim : char_list) {
        if ((((victim->hit * 100) / victim->max_hit) < lowest_percent) && (victim->is_pc())) {
            lowest_percent = ((victim->hit * 100) / victim->max_hit);
            lowest_person = victim;
        }
    }

    /* check for Phil the meerkat being beaten the shit out of */
    phil = get_mob_by_vnum(PHIL_THE_MEERKAT_VNUM);
    if (phil && (phil->position == POS_FIGHTING) && /* if phil is fighting */
        ((phil->hit * 100) / phil->max_hit < 10)) /* and has less than 10% hp */
        lowest_person = phil;
    /* end */

    if (lowest_person == nullptr) {
        if ((number_percent() > 30) || (ch->in_room == home))
            return false;
        act("$n disappears through a gate in search of more souls.", ch);
        char_from_room(ch);
        char_to_room(ch, home);
        act("$n returns home in search of a nice cup of tea.", ch);
        return true;
    }

    if ((lowest_person->in_room != ch->in_room) && (lowest_person->position == POS_FIGHTING)) {
        act("$n disappears through a gate seeking to usher souls elsewhere.", ch);
        char_from_room(ch);
        char_to_room(ch, lowest_person->in_room);
        act("There is a shimmering light and $n appears through a gate.", ch);
        /* Check if Phil needs rescuing */
        if (lowest_person == phil) {
            act("$n tuts loudly at $N.", ch, nullptr, phil, To::Room);
            act("$n picks $N up by the scruff of the neck, and disappears through a gate.", ch, nullptr, phil,
                To::Room);
            char_from_room(ch);
            char_from_room(phil);
            char_to_room(ch, home);
            char_to_room(phil, home);
            act("$n appears through a gate holding a frightened-looking meerkat.", ch);
        }
        return true;
    }

    /* Sanity test below :-) */

    if ((lowest_person->in_room != ch->in_room))
        return false;

    /* End sanity test ... the less bugs the better, eh? */

    if ((lowest_person->position != POS_FIGHTING) && (number_percent() > 60)) {
        act("$n sighs as another soul slips through his fingers.", ch);
        act("$n disappears through a gate to seek souls elsewhere.", ch);
        char_from_room(ch);
        char_to_room(ch, home);
        act("$n returns home after a hard day's work.", ch);
    }

    switch (number_bits(4)) {
    default: return false;
    case 0: act("$n starts sharpening his scythe.", ch); return true;
    case 1: act("$n reaches into his long robe and pulls out an hourglass.", ch); return true;
    case 2: act("$n says, YOUR TIME IS UP SOON, MORTAL.", ch); return true;
    case 3: act("$n watches the last few grains of sand trickle through an hourglass.", ch); return true;
    case 4:
        act("$N is tapped on the shoulder by a bony finger.", ch, nullptr, lowest_person, To::NotVict);
        act("You are tapped on the shoulder by a bony finger.", ch, nullptr, lowest_person, To::Vict);
        return true;
    case 5: act("$n drums his bony fingers on the top of his scythe.", ch); return true;
    }
}

bool spec_cast_adept(Char *ch) {
    Char *victim;
    Char *v_next;

    if (!IS_AWAKE(ch))
        return false;

    for (victim = ch->in_room->people; victim != nullptr; victim = v_next) {
        v_next = victim->next_in_room;
        if (victim != ch && can_see(ch, victim) && number_bits(1) == 0 && victim->is_pc() && victim->level < 11)
            break;
    }

    if (victim == nullptr)
        return false;

    switch (number_bits(4)) {
    case 0:
        act("$n utters the word 'abrazak'.", ch);
        spell_armor(skill_lookup("armor"), (ch->level / 4), ch, victim);
        return true;

    case 1:
        act("$n utters the word 'fido'.", ch);
        spell_bless(skill_lookup("bless"), (ch->level / 4), ch, victim);
        return true;

    case 2:
        act("$n utters the word 'judicandus noselacri'.", ch);
        spell_cure_blindness(skill_lookup("cure blindness"), ch->level, ch, victim);
        return true;

    case 3:
        act("$n utters the word 'judicandus dies'.", ch);
        spell_cure_light(skill_lookup("cure light"), ch->level, ch, victim);
        return true;

    case 4:
        act("$n utters the words 'judicandus sausabru'.", ch);
        spell_cure_poison(skill_lookup("cure poison"), ch->level, ch, victim);
        return true;

    case 5:
        act("$n utters the words 'candusima'.", ch);
        spell_refresh(skill_lookup("refresh"), ch->level, ch, victim);
        return true;
    }

    return false;
}

bool spec_cast_cleric(Char *ch) {
    Char *victim;
    Char *v_next;
    const char *spell;
    int sn;

    if (ch->position != POS_FIGHTING)
        return false;

    for (victim = ch->in_room->people; victim != nullptr; victim = v_next) {
        v_next = victim->next_in_room;
        if (victim->fighting == ch && number_bits(2) == 0)
            break;
    }

    if (victim == nullptr)
        return false;

    for (;;) {
        int min_level;

        switch (number_bits(4)) {
        case 0:
            min_level = 0;
            spell = "blindness";
            break;
        case 1:
            min_level = 7;
            spell = "earthquake";
            break;
        case 2:
            min_level = 10;
            spell = "dispel evil";
            break;
        case 3:
            min_level = 10;
            spell = "dispel good";
            break;
        case 4:
            min_level = 12;
            spell = "curse";
            break;
        case 5:
            min_level = 15;
            spell = "plague";
            break;
        case 6:
            min_level = 30;
            spell = "demonfire";
            break;
        case 7:
            min_level = 30;
            spell = "exorcise";
            break;
        case 8:
        case 9:
        case 10:
        case 11:
        default:
            min_level = 16;
            spell = "dispel magic";
            break;
        }

        if (ch->level >= min_level)
            break;
    }

    if ((sn = skill_lookup(spell)) < 0)
        return false;
    (*skill_table[sn].spell_fun)(sn, ch->level, ch, victim);
    return true;
}

bool spec_greasy_joe(Char *ch) {
    if (number_percent() > 99)
        ch->say("...with onions?...");
    return true;
}

bool spec_cast_judge(Char *ch) {
    Char *victim;
    Char *v_next;
    const char *spell;
    int sn;

    if (ch->position != POS_FIGHTING)
        return false;

    for (victim = ch->in_room->people; victim != nullptr; victim = v_next) {
        v_next = victim->next_in_room;
        if (victim->fighting == ch && number_bits(2) == 0)
            break;
    }

    if (victim == nullptr)
        return false;

    spell = "high explosive";
    if ((sn = skill_lookup(spell)) < 0)
        return false;
    (*skill_table[sn].spell_fun)(sn, ch->level, ch, victim);
    return true;
}

bool spec_cast_mage(Char *ch) {
    Char *victim;
    Char *v_next;
    const char *spell;
    int sn;

    if (ch->position != POS_FIGHTING)
        return false;

    for (victim = ch->in_room->people; victim != nullptr; victim = v_next) {
        v_next = victim->next_in_room;
        if (victim->fighting == ch && number_bits(3) == 0)
            break;
    }

    if (victim == nullptr)
        return false;

    for (;;) {
        int min_level, max_level;

        max_level = 500;

        /* NB for every level there *MUST* be at least one spell that
                   can be cast or the following code goes into an infinite loop MrG */

        switch (number_bits(4)) {
        case 0:
            min_level = 0;
            max_level = 20;
            spell = "magic missile";
            break;
        case 1:
            min_level = 3;
            max_level = 30;
            spell = "chill touch";
            break;
        case 2:
            min_level = 6;
            max_level = 40;
            spell = "burning hands";
            break;
        case 3:
            min_level = 8;
            max_level = 60;
            spell = "shocking grasp";
            break;
        case 4:
            min_level = 11;
            max_level = 60;
            spell = "colour spray";
            break;
        case 5:
            min_level = 14;
            max_level = 80;
            spell = "lightning bolt";
            break;
        case 6:
            min_level = 20;
            max_level = 105;
            spell = "fireball";
            break;
        case 7:
        case 8:
        case 9:
        case 10:
        default:
            min_level = 30;
            spell = "acid blast";
            break;
        }

        if ((ch->level >= min_level) && (ch->level <= max_level))
            break;
    }

    if ((sn = skill_lookup(spell)) < 0)
        return false;
    (*skill_table[sn].spell_fun)(sn, ch->level, ch, victim);
    return true;
}

bool spec_cast_undead(Char *ch) {
    Char *victim;
    Char *v_next;
    const char *spell;
    int sn;

    if (ch->position != POS_FIGHTING)
        return false;

    for (victim = ch->in_room->people; victim != nullptr; victim = v_next) {
        v_next = victim->next_in_room;
        if (victim->fighting == ch && number_bits(3) == 0)
            break;
    }

    if (victim == nullptr)
        return false;

    for (;;) {
        int min_level, max_level;

        max_level = 500;

        /* NB for every level there *MUST* be at least one spell that
                   can be cast or the following code goes into an infinite loop MrG */

        switch (number_bits(4)) {
        case 0:
            min_level = 0;
            spell = "poison";
            break;
        case 1:
            min_level = 3;
            spell = "blind";
            break;
        case 2:
            min_level = 6;
            spell = "curse";
            break;
        case 3:
            min_level = 8;
            spell = "weaken";
            break;
        case 4:
            min_level = 11;
            spell = "plague";
            break;
        case 5:
            min_level = 14;
            spell = "energy drain";
            break;
        case 6:
            min_level = 20;
            spell = "insanity";
            break;
        case 7:
            min_level = 0;
            max_level = 10;
            spell = "cause light";
            break;
        case 8:
            min_level = 10;
            max_level = 20;
            spell = "cause serious";
            break;
        case 9:
            min_level = 20;
            max_level = 30;
            spell = "cause critical";
            break;
        case 10:
            min_level = 30;
            spell = "harm";
            break;
        default:
            min_level = 30;
            spell = "change sex";
            break;
        }

        if ((ch->level >= min_level) && (ch->level <= max_level))
            break;
    }

    if ((sn = skill_lookup(spell)) < 0)
        return false;
    (*skill_table[sn].spell_fun)(sn, ch->level, ch, victim);
    return true;
}

bool spec_cast_bastard(Char *ch) {
    Char *victim;
    Char *v_next;
    const char *spell;
    int sn;

    if (ch->position != POS_FIGHTING)
        return false;

    for (victim = ch->in_room->people; victim != nullptr; victim = v_next) {
        v_next = victim->next_in_room;
        if (victim->fighting == ch && number_bits(3) == 0)
            break;
    }

    if (victim == nullptr)
        return false;

    for (;;) {
        int min_level, max_level;

        max_level = 500;

        /* NB for every level there *MUST* be at least one spell that
                   can be cast or the following code goes into an infinite loop MrG */

        switch (number_bits(4)) {
        case 0:
        case 1:
            min_level = 1;
            spell = "blind";
            break;
        case 2:
        case 3:
            min_level = 10;
            spell = "curse";
            break;
        case 4:
        case 5:
            min_level = 10;
            spell = "energy drain";
            break;
        case 6:
        case 7:
            min_level = 10;
            spell = "dispel magic";
            break;
        case 8:
        case 9:
            min_level = 10;
            spell = "lethargy";
            break;
        case 10:
        default:
            min_level = 10;
            spell = "gas breath";
            break;
        }

        if ((ch->level >= min_level) && (ch->level <= max_level))
            break;
    }

    if ((sn = skill_lookup(spell)) < 0)
        return false;
    (*skill_table[sn].spell_fun)(sn, ch->level, ch, victim);
    return true;
}

bool spec_executioner(Char *ch) {
    if (!IS_AWAKE(ch) || ch->fighting != nullptr)
        return false;

    auto *crime = "";
    Char *victim{};
    for (victim = ch->in_room->people; victim; victim = victim->next_in_room) {
        if (victim->is_pc() && IS_SET(victim->act, PLR_KILLER)) {
            crime = "KILLER";
            break;
        }

        if (victim->is_pc() && IS_SET(victim->act, PLR_THIEF)) {
            crime = "THIEF";
            break;
        }
    }

    if (!victim)
        return false;

    ch->yell(fmt::format("{} is a {}!  PROTECT THE INNOCENT!  MORE BLOOOOD!!!", victim->name, crime));
    multi_hit(ch, victim, TYPE_UNDEFINED);
    char_to_room(create_mobile(get_mob_index(MOB_VNUM_CITYGUARD)), ch->in_room);
    char_to_room(create_mobile(get_mob_index(MOB_VNUM_CITYGUARD)), ch->in_room);
    return true;
}

/* A procedure for Puff the Fractal Dragon--> it gives her an attitude.
        Note that though this procedure may make Puff look busy, she in
        fact does nothing quite more often than she did in Merc 1.0;
        due to null victim traps, my own do-nothing options, and various ways
        to return without doing much, Puff is... well, not as BAD of a gadfly
        as she may look, I assure you.  But I think she's fun this way ;)

        (btw--- should you ever want to test out your socials, just tweak
        the percentage table ('silliness') to make her do lots of socials,
        and then go to a quiet room and load up about thirty Puffs... ;)

                written by Seth of Rivers of Mud         */

bool spec_puff(Char *ch) {
    int rnd_social, sn, silliness;
    bool pc_found = true;
    Char *v_next;
    Char *nch;
    Char *ch_next;
    Char *vch;
    Char *vch_next;
    Char *victim;
    extern int social_count;

    if (!IS_AWAKE(ch))
        return false;

    victim = nullptr;

    /* Here's Furey's aggress routine, with some surgery done to it.
         All it does is pick a potential victim for a social.
         (Thank you, Furey-- I screwed this up many times until I
         learned of your way of doing it)                      */

    for (auto *wch : char_list) {
        if (wch->is_npc() || wch->in_room == nullptr)
            continue;

        for (nch = wch->in_room->people; nch != nullptr; nch = ch_next) {
            int count;

            ch_next = nch->next_in_room;

            if (nch->is_pc() || number_bits(1) == 0)
                continue;
            /*
             * Ok we have a 'wch' player character and a 'nch' npc aggressor.
             * Now make the aggressor fight a RANDOM pc victim in the room,
             *   giving each 'vch' an equal chance of selection.
             */
            count = 0;
            victim = nullptr;
            for (vch = wch->in_room->people; vch != nullptr; vch = vch_next) {
                vch_next = vch->next_in_room;

                if (vch->is_pc()) {
                    if (number_range(0, count) == 0)
                        victim = vch;
                    count++;
                }
            }

            if (victim == nullptr)
                return false;
        }
    }
    rnd_social = (number_range(0, (social_count - 1)));

    /* Choose some manner of silliness to perpetrate.  */

    silliness = number_range(1, 100);

    if (silliness <= 20)
        return true;
    else if (silliness <= 30) {
        ch->say("Tongue-tied and twisted, just an earthbound misfit, ...");
    } else if (silliness <= 40) {
        ch->say("The colors, the colors!");
    } else if (silliness <= 55) {
        ch->say("Did you know that I'm written in C++?");
    } else if (silliness <= 75) {
        act(social_table[rnd_social].others_no_arg, ch);
        act(social_table[rnd_social].char_no_arg, ch, nullptr, nullptr, To::Char);
    } else if (silliness <= 85) {
        if ((!pc_found) || (victim != ch->in_room->people))
            return false;
        act(social_table[rnd_social].others_found, ch, nullptr, victim, To::NotVict);
        act(social_table[rnd_social].char_found, ch, nullptr, victim, To::Char);
        act(social_table[rnd_social].vict_found, ch, nullptr, victim, To::Vict);
    }

    else if (silliness <= 97) {
        act("For a moment, $n flickers and phases.", ch);
        act("For a moment, you flicker and phase.", ch, nullptr, nullptr, To::Char);
    }

    /* The Fractal Dragon sometimes teleports herself around, to check out
         new and stranger things.  HOWEVER, to stave off some possible Puff
         repop problems, and to make it possible to play her as a mob without
         teleporting helplessly, Puff does NOT teleport if she's in Limbo,
         OR if she's not fighting or standing.  If you're playing Puff and
         you want to talk with someone, just rest or sit!
    */

    else {
        if (ch->position < POS_FIGHTING) {
            act("For a moment, $n seems lucid...", ch);
            act("   ...but then $e returns to $s contemplations once again.", ch);
            act("For a moment, the world's mathematical beauty is lost to you!", ch, nullptr, nullptr, To::Char);
            act("   ...but joy! yet another novel phenomenon seizes your attention.", ch, nullptr, nullptr, To::Char);
            return true;
        }
        if ((sn = skill_lookup("teleport")) < 0)
            return false;
        (*skill_table[sn].spell_fun)(sn, ch->level, ch, ch);
    }

    /* Puff has only one spell, and it's the most annoying one, of course.
         (excepting energy drain, natch)  But to a bemused mathematician,
         what could possibly be a better resolution to conflict? ;)
         Oh-- and notice that Puff casts her one spell VERY well.     */

    if (ch->position != POS_FIGHTING)
        return false;

    for (victim = ch->in_room->people; victim != nullptr; victim = v_next) {
        v_next = victim->next_in_room;
        if (victim->fighting == ch && number_bits(2) == 0)
            break;
    }

    if (victim == nullptr)
        return false;

    if ((sn = skill_lookup("teleport")) < 0)
        return false;
    (*skill_table[sn].spell_fun)(sn, 50, ch, victim);
    return true;
}

bool spec_fido(Char *ch) {
    if (!IS_AWAKE(ch))
        return false;

    for (auto *corpse : ch->in_room->contents) {
        if (corpse->item_type != ITEM_CORPSE_NPC)
            continue;

        act("$n savagely devours a corpse.", ch);
        for (auto *obj : corpse->contains) {
            obj_from_obj(obj);
            obj_to_room(obj, ch->in_room);
        }
        extract_obj(corpse);
        return true;
    }

    return false;
}

bool spec_guard(Char *ch) {
    Char *victim;
    Char *v_next;
    Char *ech;
    const char *crime;
    int max_evil;

    if (!IS_AWAKE(ch) || ch->fighting != nullptr)
        return false;

    max_evil = 300;
    ech = nullptr;
    crime = "";

    for (victim = ch->in_room->people; victim != nullptr; victim = v_next) {
        v_next = victim->next_in_room;

        if (victim->is_pc() && IS_SET(victim->act, PLR_KILLER)) {
            crime = "KILLER";
            break;
        }

        if (victim->is_pc() && IS_SET(victim->act, PLR_THIEF)) {
            crime = "THIEF";
            break;
        }

        if (victim->fighting != nullptr && victim->fighting != ch && victim->alignment < max_evil) {
            max_evil = victim->alignment;
            ech = victim;
        }
    }

    if (victim != nullptr) {
        ch->yell(fmt::format("{} is a {}!  PROTECT THE INNOCENT!  BANZAI!!", victim->name, crime));
        multi_hit(ch, victim, TYPE_UNDEFINED);
        return true;
    }

    if (ech != nullptr) {
        act("$n screams 'PROTECT THE INNOCENT!!  BANZAI!!", ch);
        multi_hit(ch, ech, TYPE_UNDEFINED);
        return true;
    }

    return false;
}

bool spec_janitor(Char *ch) {
    if (!IS_AWAKE(ch))
        return false;

    for (auto *trash : ch->in_room->contents) {
        if (!IS_SET(trash->wear_flags, ITEM_TAKE) || !can_loot(ch, trash))
            continue;
        if (trash->item_type == ITEM_DRINK_CON || trash->item_type == ITEM_TRASH || trash->cost < 10) {
            act("$n picks up some trash.", ch);
            obj_from_room(trash);
            obj_to_char(trash, ch);
            return true;
        }
    }

    return false;
}

bool spec_mayor(Char *ch) {
    static const char open_path[] = "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

    static const char close_path[] = "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

    static const char *path;
    static int pos;
    static bool move;

    if (!move) {
        if (time_info.hour() == 6) {
            path = open_path;
            move = true;
            pos = 0;
        }

        if (time_info.hour() == 20) {
            path = close_path;
            move = true;
            pos = 0;
        }
    }

    if (ch->fighting != nullptr)
        return spec_cast_cleric(ch);
    if (!move || ch->position < POS_SLEEPING)
        return false;

    switch (path[pos]) {
    case '0': move_char(ch, Direction::North); break;
    case '1': move_char(ch, Direction::East); break;
    case '2': move_char(ch, Direction::South); break;
    case '3': move_char(ch, Direction::West); break;

    case 'W':
        ch->position = POS_STANDING;
        act("$n awakens and groans loudly.", ch);
        break;

    case 'S':
        ch->position = POS_SLEEPING;
        act("$n lies down and falls asleep.", ch);
        break;

    case 'a': act("$n says 'Hello Honey!'", ch); break;

    case 'b': act("$n says 'What a view!  I must do something about that dump!'", ch); break;

    case 'c': act("$n says 'Vandals!  Youngsters have no respect for anything!'", ch); break;

    case 'd': act("$n says 'Good day, citizens!'", ch); break;

    case 'e': act("$n says 'I hereby declare the city of Midgaard open!'", ch); break;

    case 'E': act("$n says 'I hereby declare the city of Midgaard closed!'", ch); break;

    case 'O':
        /* do_unlock( ch, "gate" ); */
        do_open(ch, ArgParser("gate"));
        break;

    case 'C':
        do_close(ch, ArgParser("gate"));
        /* do_lock( ch, "gate" ); */
        break;

    case '.': move = false; break;
    }

    pos++;
    return false;
}

bool spec_poison(Char *ch) {
    Char *victim;

    if (ch->position != POS_FIGHTING || (victim = ch->fighting) == nullptr || number_percent() > 2 * ch->level)
        return false;

    act("You bite $N!", ch, nullptr, victim, To::Char);
    act("$n bites $N!", ch, nullptr, victim, To::NotVict);
    act("$n bites you!", ch, nullptr, victim, To::Vict);
    spell_poison(gsn_poison, ch->level, ch, victim);
    return true;
}

bool spec_thief(Char *ch) {
    Char *victim;
    Char *v_next;
    long gold;

    if (ch->position != POS_STANDING)
        return false;

    for (victim = ch->in_room->people; victim != nullptr; victim = v_next) {
        v_next = victim->next_in_room;

        if (victim->is_npc() || victim->level >= LEVEL_IMMORTAL || number_bits(5) != 0 || !can_see(ch, victim))
            continue;

        if (IS_AWAKE(victim) && number_range(0, ch->level) == 0) {
            act("You discover $n's hands in your wallet!", ch, nullptr, victim, To::Vict);
            act("$N discovers $n's hands in $S wallet!", ch, nullptr, victim, To::NotVict);
            return true;
        } else {
            gold = victim->gold * UMIN(number_range(1, 15), ch->level) / 100;
            gold = UMIN(gold, ch->level * ch->level * 15);
            if (gold > victim->gold)
                gold = victim->gold;
            ch->gold += gold;
            victim->gold -= gold;
            return true;
        }
    }

    return false;
}
