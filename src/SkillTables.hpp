/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include "Constants.hpp"
#include "Position.hpp"
#include "Types.hpp"

enum class Target;

/*
 *  Skill rating magic numbers.
 */
#define SKILL_UNATTAINABLE 0
#define SKILL_ATTAINABLE -1
#define SKILL_ASSASSIN -2 /* Hacky kludge, ohyes. */

/*
 * Skills include spells as a particular case.
 */
struct skill_type {
    const char *name; /* Name of skill                */
    SpellFunc spell_fun; /* Spell function pointer  */
    sh_int *pgsn; /* Pointer to associated gsn    */
    const char *verb; /* Damage message               */
    const char *msg_off; /* Wear off message for the wearer  */
    DispelMagicFunc dispel_fun;
    /* If a spell effect can be dispelled or cancelled, set this to contain the
     * message sent to others in the room about a victim */
    const char *dispel_victim_msg_to_room;
    /* AFFECT bit if this is a dispellable affect that can permanently affect an NPC
     * i.e. it's a spell applied in the area file that is not going to appear
     * as an entry in an AffectList.
     */
    const unsigned int dispel_npc_perm_affect_bit;
    Target target; /* Legal targets                */
    Position::Type minimum_position;
    sh_int slot; /* Slot for #OBJECT loading     */
    sh_int min_mana; /* Minimum mana used            */
    sh_int beats; /* Waiting time after use       */
    sh_int skill_level[MAX_CLASS]; /* Level needed by class        */
    sh_int rating[MAX_CLASS]; /* How hard it is to learn      */
};

struct group_type {
    const char *name;
    sh_int rating[MAX_CLASS];
    const char *spells[MAX_IN_GROUP];
};

extern const struct skill_type skill_table[MAX_SKILL];
extern const struct group_type group_table[MAX_GROUP];
