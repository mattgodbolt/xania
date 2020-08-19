#pragma once

#include "Descriptor.hpp"
#include "ExtraFlags.hpp"
#include "Stats.hpp"
#include "Types.hpp"

struct AFFECT_DATA;
struct MOB_INDEX_DATA;
struct NOTE_DATA;
struct OBJ_DATA;
struct ROOM_INDEX_DATA;
struct PC_DATA;
struct GEN_DATA;
struct MPROG_ACT_LIST;

/*
 * One character (PC or NPC).
 */
struct CHAR_DATA {
    CHAR_DATA *next;
    CHAR_DATA *next_in_room;
    CHAR_DATA *master;
    CHAR_DATA *leader;
    CHAR_DATA *fighting;
    CHAR_DATA *reply;
    CHAR_DATA *pet;
    CHAR_DATA *riding;
    CHAR_DATA *ridden_by;
    SpecialFunc spec_fun;
    MOB_INDEX_DATA *pIndexData;
    Descriptor *desc;
    AFFECT_DATA *affected;
    NOTE_DATA *pnote;
    OBJ_DATA *carrying;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *was_in_room;
    PC_DATA *pcdata;
    GEN_DATA *gen_data;
    char *name;

    sh_int version;
    char *short_descr;
    char *long_descr;
    char *description;
    char *sentient_victim;
    sh_int sex;
    sh_int class_num;
    sh_int race;
    sh_int level;
    sh_int trust;
    Seconds played;
    int lines; /* for the pager */
    Time logon;
    Time last_note;
    sh_int timer;
    sh_int wait;
    sh_int hit;
    sh_int max_hit;
    sh_int mana;
    sh_int max_mana;
    sh_int move;
    sh_int max_move;
    long gold;
    long exp;
    unsigned long act;
    unsigned long comm; /* RT added to pad the vector */
    unsigned long imm_flags;
    unsigned long res_flags;
    unsigned long vuln_flags;
    sh_int invis_level;
    unsigned int affected_by;
    sh_int position;
    sh_int practice;
    sh_int train;
    sh_int carry_weight;
    sh_int carry_number;
    sh_int saving_throw;
    sh_int alignment;
    sh_int hitroll;
    sh_int damroll;
    sh_int armor[4];
    sh_int wimpy;
    /* stats */
    Stats perm_stat;
    Stats mod_stat;
    /* parts stuff */
    unsigned long form;
    unsigned long parts;
    sh_int size;
    sh_int material;
    unsigned long hit_location; /* for verbose combat sequences */
    /* mobile stuff */
    unsigned long off_flags;
    sh_int damage[3];
    sh_int dam_type;
    sh_int start_pos;
    sh_int default_pos;

    char *clipboard;
    unsigned long extra_flags[(MAX_EXTRA_FLAGS / 32) + 1];

    MPROG_ACT_LIST *mpact; /* Used by MOBprogram */
    int mpactnum; /* Used by MOBprogram */

    [[nodiscard]] Seconds total_played() const;
};
