/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  handler.c:  a core module with a huge amount of utility functions    */
/*                                                                       */
/*************************************************************************/
/***************************************************************************
 *	ROM 2.4 is copyright 1993-1996 Russ Taylor			   *
 *	ROM has been brought to you by the ROM consortium		   *
 *	    Russ Taylor (rtaylor@pacinfo.com)				   *
 *	    Gabrielle Taylor (gtaylor@pacinfo.com)			   *
 *	    Brian Moore (rom@rom.efn.org)				   *
 *	By using this code, you have agreed to follow the terms of the	   *
 *	ROM license, in the file Rom24/doc/rom.license			   *
 ***************************************************************************/

#include "tables.h"
#include "merc.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

struct flag_stat_type {
    const struct flag_type *structure;
    bool stat;
};

/*****************************************************************************
 Name:		flag_stat_table
 Purpose:	This table catagorizes the tables following the lookup
                functions below into stats and flags.  Flags can be toggled
                but stats can only be assigned.  Update this table when a
                new set of flags is installed.
 ****************************************************************************/
const struct flag_stat_type flag_stat_table[] = {
    /*  {	structure		stat	}, */
    {area_flags, false},
    {sex_flags, true},
    {exit_flags, false},
    {door_resets, true},
    {room_flags, false},
    {sector_flags, true},
    {type_flags, true},
    {extra_flags, false},
    /*    {   extra2_flags,		false	}, */
    {wear_flags, false},
    {act_flags, false},
    {off_flags, false},
    {imm_flags, false},
    {res_flags, false},
    {vuln_flags, false},
    {affect_flags, false},
    /*    {   affect2_flags,		false	}, */
    {apply_flags, true},
    {wear_loc_flags, true},
    {wear_loc_strings, true},
    {weapon_flags, true},
    {container_flags, false},
    {liquid_flags, true},
    {weapontype_flags, false},
    {gate_flags, false},
    {furniture_flags, false},
    {mprog_flags, false},
    {0, 0}};

/*****************************************************************************
 Name:		is_stat( table )
 Purpose:	Returns true if the table is a stat table and false if flag.
 Called by:	flag_value and flag_string.
 Note:		This function is local and used only in bit.c.
 ****************************************************************************/
bool is_stat(const struct flag_type *flag_table) {
    int flag;

    for (flag = 0; flag_stat_table[flag].structure; flag++) {
        if (flag_stat_table[flag].structure == flag_table && flag_stat_table[flag].stat)
            return true;
    }
    return false;
}

/* for position */
const struct position_type position_table[] = {{"dead", "dead"},           {"mortally wounded", "mort"},
                                               {"incapacitated", "incap"}, {"stunned", "stun"},
                                               {"sleeping", "sleep"},      {"resting", "rest"},
                                               {"sitting", "sit"},         {"fighting", "fight"},
                                               {"standing", "stand"},      {nullptr, nullptr}};

/* for sex */
// TM> fixed to allow some areas to load in properly
const struct sex_type sex_table[] = {{"none", 0}, {"neither", 0}, {"neutral", 0}, {"either", 0},
                                     {"male", 1}, {"female", 2},  {nullptr}};

/* for sizes */
const struct size_type size_table[] = {{"tiny"},
                                       {"small"},
                                       {"medium"},
                                       {"large"},
                                       {
                                           "huge",
                                       },
                                       {"giant"},
                                       {nullptr}};

const struct flag_type act_flags[] = {{"npc", ACT_IS_NPC, false},
                                      {"sentinel", ACT_SENTINEL, true},
                                      {"scavenger", ACT_SCAVENGER, true},
                                      {"aggressive", ACT_AGGRESSIVE, true},
                                      {"stay_area", ACT_STAY_AREA, true},
                                      {"wimpy", ACT_WIMPY, true},
                                      {"pet", ACT_PET, true},
                                      {"train", ACT_TRAIN, true},
                                      {"practice", ACT_PRACTICE, true},
                                      {"sentient", ACT_SENTIENT, true},
                                      {"talkative", ACT_TALKATIVE, true},
                                      {"undead", ACT_UNDEAD, true},
                                      {"cleric", ACT_CLERIC, true},
                                      {"mage", ACT_MAGE, true},
                                      {"thief", ACT_THIEF, true},
                                      {"warrior", ACT_WARRIOR, true},
                                      {"noalign", ACT_NOALIGN, true},
                                      {"nopurge", ACT_NOPURGE, true},
                                      {"is_healer", ACT_IS_HEALER, true},
                                      {"gain", ACT_GAIN, true},
                                      {"update_always", ACT_UPDATE_ALWAYS, true},
                                      {"ride", ACT_CAN_BE_RIDDEN, true},
                                      {"", 0, 0}};

const struct flag_type plr_flags[] = {{"npc", PLR_IS_NPC, false},
                                      {"boughtpet", PLR_BOUGHT_PET, false},
                                      {"autoassist", PLR_AUTOASSIST, false},
                                      {"autoexit", PLR_AUTOEXIT, false},
                                      {"autoloot", PLR_AUTOLOOT, false},
                                      {"autosac", PLR_AUTOSAC, false},
                                      {"autogold", PLR_AUTOGOLD, false},
                                      {"autosplit", PLR_AUTOSPLIT, false},
                                      {"holylight", PLR_HOLYLIGHT, false},
                                      {"wizinvis", PLR_WIZINVIS, false},
                                      {"can_loot", PLR_CANLOOT, false},
                                      {"nosummon", PLR_NOSUMMON, false},
                                      {"nofollow", PLR_NOFOLLOW, false},
                                      {"afk", PLR_AFK, false},
                                      {"prowl", PLR_PROWL, false},
                                      {"log", PLR_LOG, false},
                                      {"deny", PLR_DENY, false},
                                      {"freeze", PLR_FREEZE, false},
                                      {"thief", PLR_THIEF, false},
                                      {"killer", PLR_KILLER, false},
                                      {"", 0, 0}};

const struct flag_type affect_flags[] = {{"blind", AFF_BLIND, true},
                                         {"invisible", AFF_INVISIBLE, true},
                                         {"detect_evil", AFF_DETECT_EVIL, true},
                                         {"detect_invis", AFF_DETECT_INVIS, true},
                                         {"detect_magic", AFF_DETECT_MAGIC, true},
                                         {"detect_hidden", AFF_DETECT_HIDDEN, true},
                                         {"talon", AFF_TALON, true},
                                         {"sanctuary", AFF_SANCTUARY, true},
                                         {"faerie_fire", AFF_FAERIE_FIRE, true},
                                         {"infrared", AFF_INFRARED, true},
                                         {"curse", AFF_CURSE, true},
                                         {"poison", AFF_POISON, true},
                                         {"protect_evil", AFF_PROTECTION_EVIL, true},
                                         {"protect_good", AFF_PROTECTION_GOOD, true},
                                         {"sneak", AFF_SNEAK, true},
                                         {"hide", AFF_HIDE, true},
                                         {"sleep", AFF_SLEEP, false},
                                         {"charm", AFF_CHARM, false},
                                         {"flying", AFF_FLYING, true},
                                         {"pass_door", AFF_PASS_DOOR, true},
                                         {"haste", AFF_HASTE, true},
                                         {"lethargy", AFF_LETHARGY, true},
                                         {"calm", AFF_CALM, true},
                                         {"plague", AFF_PLAGUE, false},
                                         {"weaken", AFF_WEAKEN, true},
                                         {"dark_vision", AFF_DARK_VISION, true},
                                         {"berserk", AFF_BERSERK, false},
                                         {"swim", AFF_SWIM, true},
                                         {"regeneration", AFF_REGENERATION, true},
                                         {"octarine_fire", AFF_OCTARINE_FIRE, true},
                                         {"", 0, 0}};

const struct flag_type off_flags[] = {{"area_attack", OFF_AREA_ATTACK, true},
                                      {"backstab", OFF_BACKSTAB, true},
                                      {"bash", OFF_BASH, true},
                                      {"berserk", OFF_BERSERK, true},
                                      {"disarm", OFF_DISARM, true},
                                      {"dodge", OFF_DODGE, true},
                                      {"fade", OFF_FADE, true},
                                      {"fast", OFF_FAST, true},
                                      {"kick", OFF_KICK, true},
                                      {"dirt_kick", OFF_KICK_DIRT, true},
                                      {"parry", OFF_PARRY, true},
                                      {"rescue", OFF_RESCUE, true},
                                      {"use_tail", OFF_TAIL, true},
                                      {"trip", OFF_TRIP, true},
                                      {"crush", OFF_CRUSH, true},
                                      {"assist_all", ASSIST_ALL, true},
                                      {"assist_align", ASSIST_ALIGN, true},
                                      {"assist_race", ASSIST_RACE, true},
                                      {"assist_players", ASSIST_PLAYERS, true},
                                      {"assist_guard", ASSIST_GUARD, true},
                                      {"assist_vnum", ASSIST_VNUM, true},
                                      {"headbutt", OFF_HEADBUTT, true},
                                      {"", 0, 0}};

const struct flag_type imm_flags[] = {{"summon", IMM_SUMMON, true},
                                      {"charm", IMM_CHARM, true},
                                      {"magic", IMM_MAGIC, true},
                                      {"weapon", IMM_WEAPON, true},
                                      {"bash", IMM_BASH, true},
                                      {"pierce", IMM_PIERCE, true},
                                      {"slash", IMM_SLASH, true},
                                      {"fire", IMM_FIRE, true},
                                      {"cold", IMM_COLD, true},
                                      {"lightning", IMM_LIGHTNING, true},
                                      {"acid", IMM_ACID, true},
                                      {"poison", IMM_POISON, true},
                                      {"negative", IMM_NEGATIVE, true},
                                      {"holy", IMM_HOLY, true},
                                      {"energy", IMM_ENERGY, true},
                                      {"mental", IMM_MENTAL, true},
                                      {"disease", IMM_DISEASE, true},
                                      {"drowning", IMM_DROWNING, true},
                                      {"light", IMM_LIGHT, true},
                                      /*    {	"sound",		IMM_SOUND,	true	},
                                          {	"wood",			IMM_WOOD,	true	},
                                          {	"silver",		IMM_SILVER,	true	},
                                          {	"iron",			IMM_IRON,	true	},*/
                                      {"", 0, 0}};

const struct flag_type res_flags[] = {
    /*    {	"summon",		RES_SUMMON,	true	}, */
    {"charm", RES_CHARM, true},
    {"magic", RES_MAGIC, true},
    {"weapon", RES_WEAPON, true},
    {"bash", RES_BASH, true},
    {"pierce", RES_PIERCE, true},
    {"slash", RES_SLASH, true},
    {"fire", RES_FIRE, true},
    {"cold", RES_COLD, true},
    {"lightning", RES_LIGHTNING, true},
    {"acid", RES_ACID, true},
    {"poison", RES_POISON, true},
    {"negative", RES_NEGATIVE, true},
    {"holy", RES_HOLY, true},
    {"energy", RES_ENERGY, true},
    {"mental", RES_MENTAL, true},
    {"disease", RES_DISEASE, true},
    {"drowning", RES_DROWNING, true},
    {"light", RES_LIGHT, true},
    /*    {	"sound",		RES_SOUND,	true	},
        {	"wood",			RES_WOOD,	true	},
        {	"silver",		RES_SILVER,	true	},
        {	"iron",			RES_IRON,	true	},*/
    {"", 0, 0}};

const struct flag_type vuln_flags[] = {
    /*    {	"summon",		VULN_SUMMON,	true	},
          {	"charm",		VULN_CHARM,	true	},*/
    {"magic", VULN_MAGIC, true},
    {"weapon", VULN_WEAPON, true},
    {"bash", VULN_BASH, true},
    {"pierce", VULN_PIERCE, true},
    {"slash", VULN_SLASH, true},
    {"fire", VULN_FIRE, true},
    {"cold", VULN_COLD, true},
    {"lightning", VULN_LIGHTNING, true},
    {"acid", VULN_ACID, true},
    {"poison", VULN_POISON, true},
    {"negative", VULN_NEGATIVE, true},
    {"holy", VULN_HOLY, true},
    {"energy", VULN_ENERGY, true},
    {"mental", VULN_MENTAL, true},
    {"disease", VULN_DISEASE, true},
    {"drowning", VULN_DROWNING, true},
    {"light", VULN_LIGHT, true},
    /*    {	"sound",		VULN_SOUND,	true	},*/
    {"wood", VULN_WOOD, true},
    {"silver", VULN_SILVER, true},
    {"iron", VULN_IRON, true},
    {"", 0, 0}};
const struct flag_type form_flags[] = {{"edible", FORM_EDIBLE, true},
                                       {"poison", FORM_POISON, true},
                                       {"magical", FORM_MAGICAL, true},
                                       {"instant_decay", FORM_INSTANT_DECAY, true},
                                       {"other", FORM_OTHER, true},
                                       {"animal", FORM_ANIMAL, true},
                                       {"sentient", FORM_SENTIENT, true},
                                       {"undead", FORM_UNDEAD, true},
                                       {"construct", FORM_CONSTRUCT, true},
                                       {"mist", FORM_MIST, true},
                                       {"intangible", FORM_INTANGIBLE, true},
                                       {"biped", FORM_BIPED, true},
                                       {"centaur", FORM_CENTAUR, true},
                                       {"insect", FORM_INSECT, true},
                                       {"spider", FORM_SPIDER, true},
                                       {"crustacean", FORM_CRUSTACEAN, true},
                                       {"worm", FORM_WORM, true},
                                       {"blob", FORM_BLOB, true},
                                       {"mammal", FORM_MAMMAL, true},
                                       {"bird", FORM_BIRD, true},
                                       {"reptile", FORM_REPTILE, true},
                                       {"snake", FORM_SNAKE, true},
                                       {"dragon", FORM_DRAGON, true},
                                       {"amphibian", FORM_AMPHIBIAN, true},
                                       {"fish", FORM_FISH, true},
                                       {"cold_blood", FORM_COLD_BLOOD, true},
                                       {"", 0, 0}};

const struct flag_type part_flags[] = {{"head", PART_HEAD, true},
                                       {"arms", PART_ARMS, true},
                                       {"legs", PART_LEGS, true},
                                       {"heart", PART_HEART, true},
                                       {"brains", PART_BRAINS, true},
                                       {"guts", PART_GUTS, true},
                                       {"hands", PART_HANDS, true},
                                       {"feet", PART_FEET, true},
                                       {"fingers", PART_FINGERS, true},
                                       {"ear", PART_EAR, true},
                                       {"eye", PART_EYE, true},
                                       {"long_tongue", PART_LONG_TONGUE, true},
                                       {"eyestalks", PART_EYESTALKS, true},
                                       {"tentacles", PART_TENTACLES, true},
                                       {"fins", PART_FINS, true},
                                       {"wings", PART_WINGS, true},
                                       {"tail", PART_TAIL, true},
                                       {"claws", PART_CLAWS, true},
                                       {"fangs", PART_FANGS, true},
                                       {"horns", PART_HORNS, true},
                                       {"scales", PART_SCALES, true},
                                       {"tusks", PART_TUSKS, true},
                                       {"", 0, 0}};

const struct flag_type comm_flags[] = {{"quiet", COMM_QUIET, true},
                                       {"deaf", COMM_DEAF, true},
                                       {"nowiz", COMM_NOWIZ, true},
                                       {"noauction", COMM_NOAUCTION, true},
                                       {"nogossip", COMM_NOGOSSIP, true},
                                       {"noquestion", COMM_NOQUESTION, true},
                                       {"nomusic", COMM_NOMUSIC, true},
                                       {"announce", COMM_NOANNOUNCE, true},
                                       {"noquest", COMM_NOQWEST, true},
                                       {"noallege", COMM_NOALLEGE, true},
                                       {"nophilosophise", COMM_NOPHILOSOPHISE, true},
                                       {"compact", COMM_COMPACT, true},
                                       {"brief", COMM_BRIEF, true},
                                       {"prompt", COMM_PROMPT, true},
                                       {"combine", COMM_COMBINE, true},
                                       {"telnet_ga", COMM_TELNET_GA, true},
                                       {"showafk", COMM_SHOWAFK, true},
                                       {"showdef", COMM_SHOWDEFENCE, true},
                                       {"affect", COMM_AFFECT, true},
                                       {"nogratz", COMM_NOGRATZ, true},
                                       {"noemote", COMM_NOEMOTE, false},
                                       {"noshout", COMM_NOSHOUT, false},
                                       {"notell", COMM_NOTELL, false},
                                       {"nochannels", COMM_NOCHANNELS, false},

                                       {"", 0, 0}};

const struct flag_type area_flags[] = {{"none", AREA_NONE, false},
                                       {"changed", AREA_CHANGED, false},
                                       {"added", AREA_ADDED, false},
                                       {"loading", AREA_LOADING, false},
                                       {"verbose", AREA_VERBOSE, false},
                                       {"delete", AREA_DELETE, false},
                                       {"unfinished", AREA_UNFINISHED, false},
                                       {"pkzone", AREA_PKZONE, true},
                                       {"conquest", AREA_CONQUEST, true},
                                       {"arena", AREA_ARENA, true},
                                       {"", 0, 0}};

const struct flag_type sex_flags[] = {
    {"male", SEX_MALE, true}, {"female", SEX_FEMALE, true}, {"neutral", SEX_NEUTRAL, true}, {"", 0, 0}};

const struct flag_type exit_flags[] = {{"door", EX_ISDOOR, true},
                                       {"closed", EX_CLOSED, true},
                                       {"locked", EX_LOCKED, true},
                                       {"pickproof", EX_PICKPROOF, true},
                                       {"passproof", EX_PASSPROOF, true},
                                       {"easy", EX_EASY, true},
                                       {"hard", EX_HARD, true},
                                       {"infuriating", EX_INFURIATING, true},
                                       {"noclose", EX_NOCLOSE, true},
                                       {"nolock", EX_NOLOCK, true},
                                       {"trap", EX_TRAP, true},
                                       {"trap_darts", EX_TRAP_DARTS, true},
                                       {"trap_poison", EX_TRAP_POISON, true},
                                       {"trap_exploding", EX_TRAP_EXPLODING, true},
                                       {"trap_sleepgas", EX_TRAP_SLEEPGAS, true},
                                       {"trap_death", EX_TRAP_DEATH, true},
                                       {"secret", EX_SECRET, true},
                                       {"", 0, 0}};

const struct flag_type door_resets[] = {
    {"open and unlocked", 0, true}, {"closed and unlocked", 1, true}, {"closed and locked", 2, true}, {"", 0, 0}};

const struct flag_type room_flags[] = {{"dark", ROOM_DARK, true},
                                       {"no_mob", ROOM_NO_MOB, true},
                                       {"indoors", ROOM_INDOORS, true},
                                       {"private", ROOM_PRIVATE, true},
                                       {"safe", ROOM_SAFE, true},
                                       {"solitary", ROOM_SOLITARY, true},
                                       {"pet_shop", ROOM_PET_SHOP, true},
                                       {"no_recall", ROOM_NO_RECALL, true},
                                       {"imp_only", ROOM_IMP_ONLY, true},
                                       {"gods_only", ROOM_GODS_ONLY, true},
                                       {"heroes_only", ROOM_HEROES_ONLY, true},
                                       {"newbies_only", ROOM_NEWBIES_ONLY, true},
                                       {"law", ROOM_LAW, true},
                                       {"saveobj", ROOM_SAVEOBJ, true},
                                       {"", 0, 0}};

const struct flag_type sector_flags[] = {
    {"inside", SECT_INSIDE, true},   {"city", SECT_CITY, true},           {"field", SECT_FIELD, true},
    {"forest", SECT_FOREST, true},   {"hills", SECT_HILLS, true},         {"mountain", SECT_MOUNTAIN, true},
    {"swim", SECT_WATER_SWIM, true}, {"noswim", SECT_WATER_NOSWIM, true}, {"unused", SECT_UNUSED, true},
    {"air", SECT_AIR, true},         {"desert", SECT_DESERT, true},       {"", 0, 0}};

const struct flag_type type_flags[] = {{"light", ITEM_LIGHT, true},
                                       {"scroll", ITEM_SCROLL, true},
                                       {"wand", ITEM_WAND, true},
                                       {"staff", ITEM_STAFF, true},
                                       {"weapon", ITEM_WEAPON, true},
                                       {"treasure", ITEM_TREASURE, true},
                                       {"armor", ITEM_ARMOR, true},
                                       {"potion", ITEM_POTION, true},
                                       {"clothing", ITEM_CLOTHING, true},
                                       {"furniture", ITEM_FURNITURE, true},
                                       {"trash", ITEM_TRASH, true},
                                       {"container", ITEM_CONTAINER, true},
                                       {"drink-container", ITEM_DRINK_CON, true},
                                       {"key", ITEM_KEY, true},
                                       {"food", ITEM_FOOD, true},
                                       {"money", ITEM_MONEY, true},
                                       {"boat", ITEM_BOAT, true},
                                       {"npc corpse", ITEM_CORPSE_NPC, true},
                                       {"pc corpse", ITEM_CORPSE_PC, false},
                                       {"fountain", ITEM_FOUNTAIN, true},
                                       {"pill", ITEM_PILL, true},
                                       {"protect", ITEM_PROTECT, true},
                                       {"map", ITEM_MAP, true},
                                       {"portal", ITEM_PORTAL, true},
                                       /*    {	"room key",		ITEM_ROOM_KEY,		true	}, */
                                       {"bomb", ITEM_BOMB, true},
                                       {"", 0, 0}};

const struct flag_type extra_flags[] = {{"glow", ITEM_GLOW, true},
                                        {"hum", ITEM_HUM, true},
                                        {"dark", ITEM_DARK, true},
                                        {"lock", ITEM_LOCK, true},
                                        {"evil", ITEM_EVIL, true},
                                        {"invis", ITEM_INVIS, true},
                                        {"magic", ITEM_MAGIC, true},
                                        {"nodrop", ITEM_NODROP, true},
                                        {"bless", ITEM_BLESS, true},
                                        {"anti-good", ITEM_ANTI_GOOD, true},
                                        {"anti-evil", ITEM_ANTI_EVIL, true},
                                        {"anti-neutral", ITEM_ANTI_NEUTRAL, true},
                                        {"noremove", ITEM_NOREMOVE, true},
                                        {"inventory", ITEM_INVENTORY, true},
                                        {"nopurge", ITEM_NOPURGE, true},
                                        {"rot_death", ITEM_ROT_DEATH, true},
                                        {"vis_death", ITEM_VIS_DEATH, true},
                                        /*    {	"noshow",		ITEM_NOSHOW,		true	},*/
                                        {"nolocate", ITEM_NO_LOCATE, true},
                                        {"", 0, 0}};

const struct flag_type wear_flags[] = {{"take", ITEM_TAKE, true},
                                       {"finger", ITEM_WEAR_FINGER, true},
                                       {"neck", ITEM_WEAR_NECK, true},
                                       {"body", ITEM_WEAR_BODY, true},
                                       {"head", ITEM_WEAR_HEAD, true},
                                       {"legs", ITEM_WEAR_LEGS, true},
                                       {"feet", ITEM_WEAR_FEET, true},
                                       {"hands", ITEM_WEAR_HANDS, true},
                                       {"arms", ITEM_WEAR_ARMS, true},
                                       {"shield", ITEM_WEAR_SHIELD, true},
                                       {"about", ITEM_WEAR_ABOUT, true},
                                       {"waist", ITEM_WEAR_WAIST, true},
                                       {"wrist", ITEM_WEAR_WRIST, true},
                                       {"wield", ITEM_WIELD, true},
                                       {"hold", ITEM_HOLD, true},
                                       /*    {	"no_sac",		ITEM_NO_SAC,		true	},*/
                                       {"two_hands", ITEM_TWO_HANDS, true},
                                       /*    {	"wear_float",		ITEM_WEAR_FLOAT,	true	}, */
                                       {"", 0, 0}};

/*
 * Used when adding an affect to tell where it goes.
 * See addaffect and delaffect in act_olc.c
 */
const struct flag_type apply_flags[] = {{"none", APPLY_NONE, true},
                                        {"str", APPLY_STR, true},
                                        {"dex", APPLY_DEX, true},
                                        {"int", APPLY_INT, true},
                                        {"wis", APPLY_WIS, true},
                                        {"con", APPLY_CON, true},
                                        {"sex", APPLY_SEX, true},
                                        {"class", APPLY_CLASS, true},
                                        {"level", APPLY_LEVEL, true},
                                        {"age", APPLY_AGE, true},
                                        {"height", APPLY_HEIGHT, true},
                                        {"weight", APPLY_WEIGHT, true},
                                        {"mana", APPLY_MANA, true},
                                        {"hp", APPLY_HIT, true},
                                        {"move", APPLY_MOVE, true},
                                        {"gold", APPLY_GOLD, true},
                                        {"exp", APPLY_EXP, true},
                                        {"ac", APPLY_AC, true},
                                        {"hitroll", APPLY_HITROLL, true},
                                        {"damroll", APPLY_DAMROLL, true},
                                        {"saving-para", APPLY_SAVING_PARA, true},
                                        {"saving-rod", APPLY_SAVING_ROD, true},
                                        {"saving-petri", APPLY_SAVING_PETRI, true},
                                        {"saving-breath", APPLY_SAVING_BREATH, true},
                                        {"saving-spell", APPLY_SAVING_SPELL, true},
                                        /*    {	"spell",		APPLY_SPELL_AFFECT,	true	},*/
                                        {"", 0, 0}};

/*
 * What is seen.
 */
const struct flag_type wear_loc_strings[] = {{"in the inventory", WEAR_NONE, true},
                                             {"as a light", WEAR_LIGHT, true},
                                             {"on the left finger", WEAR_FINGER_L, true},
                                             {"on the right finger", WEAR_FINGER_R, true},
                                             {"around the neck (1)", WEAR_NECK_1, true},
                                             {"around the neck (2)", WEAR_NECK_2, true},
                                             {"on the body", WEAR_BODY, true},
                                             {"over the head", WEAR_HEAD, true},
                                             {"on the legs", WEAR_LEGS, true},
                                             {"on the feet", WEAR_FEET, true},
                                             {"on the hands", WEAR_HANDS, true},
                                             {"on the arms", WEAR_ARMS, true},
                                             {"as a shield", WEAR_SHIELD, true},
                                             {"about the shoulders", WEAR_ABOUT, true},
                                             {"around the waist", WEAR_WAIST, true},
                                             {"on the left wrist", WEAR_WRIST_L, true},
                                             {"on the right wrist", WEAR_WRIST_R, true},
                                             {"wielded", WEAR_WIELD, true},
                                             {"held in the hands", WEAR_HOLD, true},
                                             /*    {	"floating",		WEAR_FLOAT,	true	},*/
                                             /*    {	"wielded secondary",	WEAR_SECONDARY,	true	},*/
                                             {"", 0, false}};

/*
 * What is typed.
 * Neck2 should not be settable for loaded mobiles.
 */
const struct flag_type wear_loc_flags[] = {{"none", WEAR_NONE, true},
                                           {"light", WEAR_LIGHT, true},
                                           {"lfinger", WEAR_FINGER_L, true},
                                           {"rfinger", WEAR_FINGER_R, true},
                                           {"neck1", WEAR_NECK_1, true},
                                           {"neck2", WEAR_NECK_2, true},
                                           {"body", WEAR_BODY, true},
                                           {"head", WEAR_HEAD, true},
                                           {"legs", WEAR_LEGS, true},
                                           {"feet", WEAR_FEET, true},
                                           {"hands", WEAR_HANDS, true},
                                           {"arms", WEAR_ARMS, true},
                                           {"shield", WEAR_SHIELD, true},
                                           {"about", WEAR_ABOUT, true},
                                           {"waist", WEAR_WAIST, true},
                                           {"lwrist", WEAR_WRIST_L, true},
                                           {"rwrist", WEAR_WRIST_R, true},
                                           {"wielded", WEAR_WIELD, true},
                                           {"hold", WEAR_HOLD, true},
                                           /*    {	"floating",	WEAR_FLOAT,	true	},*/
                                           /*    {	"secondary",	WEAR_SECONDARY,	true	}, */
                                           {"", 0, 0}};

const struct flag_type weapon_flags[] = {
    {"hit", 0, true}, /*  0 */
    {"slice", 1, true},         {"stab", 2, true},           {"slash", 3, true},
    {"whip", 4, true},          {"claw", 5, true}, /*  5 */
    {"blast", 6, true},         {"pound", 7, true},          {"crush", 8, true},
    {"grep", 9, true},          {"bite", 10, true}, /* 10 */
    {"pierce", 11, true},       {"suction", 12, true},       {"beating", 13, true},
    {"digestion", 14, true},    {"charge", 15, true}, /* 15 */
    {"slap", 16, true},         {"punch", 17, true},         {"wrath", 18, true},
    {"magic", 19, true},        {"divine power", 20, true}, /* 20 */
    {"cleave", 21, true},       {"scratch", 22, true},       {"peck pierce", 23, true},
    {"peck bash", 24, true},    {"chop", 25, true}, /* 25 */
    {"sting", 26, true},        {"smash", 27, true},         {"shocking bite", 28, true},
    {"flaming bite", 29, true}, {"freezing bite", 30, true}, /* 30 */
    {"acidic bite", 31, true},  {"chomp", 32, true},         {"", 0, 0}};

const struct flag_type container_flags[] = {{"closeable", CONT_CLOSEABLE, true},
                                            {"pickproof", CONT_PICKPROOF, true},
                                            {"closed", CONT_CLOSED, true},
                                            {"locked", CONT_LOCKED, true},
                                            /*    {	"put_on",		CONT_PUT_ON,		true	},
                                                {	"trap",			CONT_TRAP,		true	},
                                                {	"trap_darts",		CONT_TRAP_DARTS,        true    },
                                                {	"trap_poison",		CONT_TRAP_POISON,	true	},
                                                {	"trap_exploding",	CONT_TRAP_EXPLODING,	true	},
                                                {	"trap_extract",		CONT_TRAP_EXTRACT,	true	},
                                                {	"trap_sleepgas",	CONT_TRAP_SLEEPGAS,	true	},
                                                {	"trap_death",		CONT_TRAP_DEATH,	true	}, */
                                            {"", 0, 0}};

const struct flag_type liquid_flags[] = {{"water", 0, true},
                                         {"beer", 1, true},
                                         {"red wine", 2, true},
                                         {"ale", 3, true},
                                         {"dark ale", 4, true},
                                         {"whisky", 5, true},
                                         {"lemonade", 6, true},
                                         {"firebreather", 7, true},
                                         {"local specialty", 8, true},
                                         {"slime mold juice", 9, true},
                                         {"milk", 10, true},
                                         {"tea", 11, true},
                                         {"coffee", 12, true},
                                         {"blood", 13, true},
                                         {"salt water", 14, true},
                                         {"coke", 15, true},
                                         {"root beer", 16, true},
                                         {"elvish wine", 17, true},
                                         {"white wine", 18, true},
                                         {"champagne", 19, true},
                                         {"mead", 20, true},
                                         {"rose wine", 21, true},
                                         {"benedictine wine", 22, true},
                                         {"vodka", 23, true},
                                         {"cranberry juice", 24, true},
                                         {"orange juice", 25, true},
                                         {"absinthe", 26, true},
                                         {"brandy", 27, true},
                                         {"aquavit", 28, true},
                                         {"schnapps", 29, true},
                                         {"icewine", 30, true},
                                         {"amontillado", 31, true},
                                         {"sherry", 32, true},
                                         {"framboise", 33, true},
                                         {"rum", 34, true},
                                         {"cordial", 35, true},
                                         {"", 0, 0}};

const struct flag_type weapontype_flags[] = {{"flaming", WEAPON_FLAMING, true},
                                             {"frost", WEAPON_FROST, true},
                                             {"vampiric", WEAPON_VAMPIRIC, true},
                                             {"sharp", WEAPON_SHARP, true},
                                             {"vorpal", WEAPON_VORPAL, true},
                                             {"two_hands", WEAPON_TWO_HANDS, true},
                                             {"lightning", WEAPON_LIGHTNING, true},
                                             {"poison", WEAPON_POISONED, true},
                                             {"acid", WEAPON_ACID, true},
                                             {"plagued", WEAPON_PLAGUED, true},

                                             {"", 0, 0}};

const struct flag_type gate_flags[] = {
    /*    { "normal_exit",		GATE_NORMAL_EXIT,	true	},
        { "nocurse",		GATE_NOCURSE,		true	},
        { "gowith",			GATE_GOWITH,		true	},
        { "buggy",			GATE_BUGGY,		true	},
        { "random",			GATE_RANDOM,		true	},
        { "climb",			GATE_CLIMB,		true	},
        { "jump",			GATE_JUMP,		true	}, */
    {"", 0, 0}};

const struct flag_type furniture_flags[] = {
    /*    { "stand_at",		STAND_AT,		true	},
        { "stand_on",		STAND_ON,		true	},
        { "stand_in",		STAND_IN,		true	},
        { "sit_at",			SIT_AT,			true	},
        { "sit_on",			SIT_ON,			true	},
        { "sit_in",			SIT_IN,			true	},
        { "rest_at",		REST_AT,		true	},
        { "rest_on",		REST_ON,		true	},
        { "rest_in",		REST_IN,		true	},
        { "sleep_at",		SLEEP_AT,		true	},
        { "sleep_on",		SLEEP_ON,		true	},
        { "sleep_in",		SLEEP_IN,		true	},
        { "put_at",			PUT_AT,			true	},
        { "put_on",			PUT_ON,			true	},
        { "put_in",			PUT_IN,			true	},
        { "put_inside",		PUT_INSIDE,		true	}, */
    {"", 0, 0}};

const struct flag_type mprog_flags[] = {
    /*    {	"act",			TRIG_ACT,		true	},
        {	"bribe",		TRIG_BRIBE,		true 	},
        {	"death",		TRIG_DEATH,		true    },
        {	"entry",		TRIG_ENTRY,		true	},
        {	"fight",		TRIG_FIGHT,		true	},
        {	"give",			TRIG_GIVE,		true	},
        {	"greet",		TRIG_GREET,		true    },
        {	"grall",		TRIG_GRALL,		true	},
        {	"kill",			TRIG_KILL,		true	},
        {	"hpcnt",		TRIG_HPCNT,		true    },
        {	"random",		TRIG_RANDOM,		true	},
        {	"speech",		TRIG_SPEECH,		true	},
        {	"speech",		TRIG_SPEECH,		true	},
        {	"exit",			TRIG_EXIT,		true    },
        {	"exall",		TRIG_EXALL,		true    },
        {	"delay",		TRIG_DELAY,		true    },
        {	"surr",			TRIG_SURR,		true    },
        {	"leave",		TRIG_LEAVE,		true	},
        {	"leall",		TRIG_LEALL,		true	},
        {   "timer",		TRIG_TIMER,		true	},
        {	"hour",			TRIG_HOUR,		true	},  */
    {"", 0, 0}};

const struct item_type item_table[] = {{ITEM_LIGHT, "light"},
                                       {ITEM_SCROLL, "scroll"},
                                       {ITEM_WAND, "wand"},
                                       {ITEM_STAFF, "staff"},
                                       {ITEM_WEAPON, "weapon"},
                                       {ITEM_TREASURE, "treasure"},
                                       {ITEM_ARMOR, "armor"},
                                       {ITEM_POTION, "potion"},
                                       {ITEM_CLOTHING, "clothing"},
                                       {ITEM_FURNITURE, "furniture"},
                                       {ITEM_TRASH, "trash"},
                                       {ITEM_CONTAINER, "container"},
                                       {ITEM_DRINK_CON, "drink"},
                                       {ITEM_KEY, "key"},
                                       {ITEM_FOOD, "food"},
                                       {ITEM_MONEY, "money"},
                                       {ITEM_BOAT, "boat"},
                                       {ITEM_CORPSE_NPC, "npc_corpse"},
                                       {ITEM_CORPSE_PC, "pc_corpse"},
                                       {ITEM_FOUNTAIN, "fountain"},
                                       {ITEM_PILL, "pill"},
                                       {ITEM_MAP, "map"},
                                       {ITEM_PORTAL, "portal"},
                                       {ITEM_BOMB, "bomb"},
                                       {0, nullptr}};

const struct weapon_type weapon_table[] = {{"sword", OBJ_VNUM_SCHOOL_SWORD, WEAPON_SWORD, &gsn_sword},
                                           {"mace", OBJ_VNUM_SCHOOL_MACE, WEAPON_MACE, &gsn_mace},
                                           {"dagger", OBJ_VNUM_SCHOOL_DAGGER, WEAPON_DAGGER, &gsn_dagger},
                                           {"axe", OBJ_VNUM_SCHOOL_AXE, WEAPON_AXE, &gsn_axe},

                                           {"exotic", OBJ_VNUM_SCHOOL_SWORD, WEAPON_EXOTIC, nullptr},
                                           {"spear", OBJ_VNUM_SCHOOL_MACE, WEAPON_SPEAR, &gsn_spear},
                                           {"flail", OBJ_VNUM_SCHOOL_DAGGER, WEAPON_FLAIL, &gsn_flail},
                                           {"whip", OBJ_VNUM_SCHOOL_AXE, WEAPON_WHIP, &gsn_whip},
                                           {"polearm", OBJ_VNUM_SCHOOL_AXE, WEAPON_POLEARM, &gsn_polearm},
                                           {nullptr, 0, 0, nullptr}};
