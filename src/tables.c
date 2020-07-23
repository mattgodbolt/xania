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

#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include "merc.h"
#include "tables.h"
#include <stdio.h>
#include <string.h>
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
    {area_flags, FALSE},
    {sex_flags, TRUE},
    {exit_flags, FALSE},
    {door_resets, TRUE},
    {room_flags, FALSE},
    {sector_flags, TRUE},
    {type_flags, TRUE},
    {extra_flags, FALSE},
    /*    {   extra2_flags,		FALSE	}, */
    {wear_flags, FALSE},
    {act_flags, FALSE},
    {off_flags, FALSE},
    {imm_flags, FALSE},
    {res_flags, FALSE},
    {vuln_flags, FALSE},
    {affect_flags, FALSE},
    /*    {   affect2_flags,		FALSE	}, */
    {apply_flags, TRUE},
    {wear_loc_flags, TRUE},
    {wear_loc_strings, TRUE},
    {weapon_flags, TRUE},
    {container_flags, FALSE},
    {liquid_flags, TRUE},
    {weapontype_flags, FALSE},
    {gate_flags, FALSE},
    {furniture_flags, FALSE},
    {mprog_flags, FALSE},
    {0, 0}};

/*****************************************************************************
 Name:		is_stat( table )
 Purpose:	Returns TRUE if the table is a stat table and FALSE if flag.
 Called by:	flag_value and flag_string.
 Note:		This function is local and used only in bit.c.
 ****************************************************************************/
bool is_stat(const struct flag_type *flag_table) {
    int flag;

    for (flag = 0; flag_stat_table[flag].structure; flag++) {
        if (flag_stat_table[flag].structure == flag_table && flag_stat_table[flag].stat)
            return TRUE;
    }
    return FALSE;
}

/* for position */
const struct position_type position_table[] = {{"dead", "dead"},           {"mortally wounded", "mort"},
                                               {"incapacitated", "incap"}, {"stunned", "stun"},
                                               {"sleeping", "sleep"},      {"resting", "rest"},
                                               {"sitting", "sit"},         {"fighting", "fight"},
                                               {"standing", "stand"},      {NULL, NULL}};

/* for sex */
// TM> fixed to allow some areas to load in properly
const struct sex_type sex_table[] = {{"none", 0}, {"neither", 0}, {"neutral", 0}, {"either", 0},
                                     {"male", 1}, {"female", 2},  {NULL}};

/* for sizes */
const struct size_type size_table[] = {{"tiny"},
                                       {"small"},
                                       {"medium"},
                                       {"large"},
                                       {
                                           "huge",
                                       },
                                       {"giant"},
                                       {NULL}};

const struct flag_type act_flags[] = {{"npc", ACT_IS_NPC, FALSE},
                                      {"sentinel", ACT_SENTINEL, TRUE},
                                      {"scavenger", ACT_SCAVENGER, TRUE},
                                      {"aggressive", ACT_AGGRESSIVE, TRUE},
                                      {"stay_area", ACT_STAY_AREA, TRUE},
                                      {"wimpy", ACT_WIMPY, TRUE},
                                      {"pet", ACT_PET, TRUE},
                                      {"train", ACT_TRAIN, TRUE},
                                      {"practice", ACT_PRACTICE, TRUE},
                                      {"sentient", ACT_SENTIENT, TRUE},
                                      {"talkative", ACT_TALKATIVE, TRUE},
                                      {"undead", ACT_UNDEAD, TRUE},
                                      {"cleric", ACT_CLERIC, TRUE},
                                      {"mage", ACT_MAGE, TRUE},
                                      {"thief", ACT_THIEF, TRUE},
                                      {"warrior", ACT_WARRIOR, TRUE},
                                      {"noalign", ACT_NOALIGN, TRUE},
                                      {"nopurge", ACT_NOPURGE, TRUE},
                                      {"is_healer", ACT_IS_HEALER, TRUE},
                                      {"gain", ACT_GAIN, TRUE},
                                      {"update_always", ACT_UPDATE_ALWAYS, TRUE},
                                      {"ride", ACT_CAN_BE_RIDDEN, TRUE},
                                      {"", 0, 0}};

const struct flag_type plr_flags[] = {{"npc", PLR_IS_NPC, FALSE},
                                      {"boughtpet", PLR_BOUGHT_PET, FALSE},
                                      {"autoassist", PLR_AUTOASSIST, FALSE},
                                      {"autoexit", PLR_AUTOEXIT, FALSE},
                                      {"autoloot", PLR_AUTOLOOT, FALSE},
                                      {"autosac", PLR_AUTOSAC, FALSE},
                                      {"autogold", PLR_AUTOGOLD, FALSE},
                                      {"autosplit", PLR_AUTOSPLIT, FALSE},
                                      {"holylight", PLR_HOLYLIGHT, FALSE},
                                      {"wizinvis", PLR_WIZINVIS, FALSE},
                                      {"can_loot", PLR_CANLOOT, FALSE},
                                      {"nosummon", PLR_NOSUMMON, FALSE},
                                      {"nofollow", PLR_NOFOLLOW, FALSE},
                                      {"afk", PLR_AFK, FALSE},
                                      {"prowl", PLR_PROWL, FALSE},
                                      {"log", PLR_LOG, FALSE},
                                      {"deny", PLR_DENY, FALSE},
                                      {"freeze", PLR_FREEZE, FALSE},
                                      {"thief", PLR_THIEF, FALSE},
                                      {"killer", PLR_KILLER, FALSE},
                                      {"", 0, 0}};

const struct flag_type affect_flags[] = {{"blind", AFF_BLIND, TRUE},
                                         {"invisible", AFF_INVISIBLE, TRUE},
                                         {"detect_evil", AFF_DETECT_EVIL, TRUE},
                                         {"detect_invis", AFF_DETECT_INVIS, TRUE},
                                         {"detect_magic", AFF_DETECT_MAGIC, TRUE},
                                         {"detect_hidden", AFF_DETECT_HIDDEN, TRUE},
                                         {"talon", AFF_TALON, TRUE},
                                         {"sanctuary", AFF_SANCTUARY, TRUE},
                                         {"faerie_fire", AFF_FAERIE_FIRE, TRUE},
                                         {"infrared", AFF_INFRARED, TRUE},
                                         {"curse", AFF_CURSE, TRUE},
                                         {"poison", AFF_POISON, TRUE},
                                         {"protect_evil", AFF_PROTECTION_EVIL, TRUE},
                                         {"protect_good", AFF_PROTECTION_GOOD, TRUE},
                                         {"sneak", AFF_SNEAK, TRUE},
                                         {"hide", AFF_HIDE, TRUE},
                                         {"sleep", AFF_SLEEP, FALSE},
                                         {"charm", AFF_CHARM, FALSE},
                                         {"flying", AFF_FLYING, TRUE},
                                         {"pass_door", AFF_PASS_DOOR, TRUE},
                                         {"haste", AFF_HASTE, TRUE},
                                         {"lethargy", AFF_LETHARGY, TRUE},
                                         {"calm", AFF_CALM, TRUE},
                                         {"plague", AFF_PLAGUE, FALSE},
                                         {"weaken", AFF_WEAKEN, TRUE},
                                         {"dark_vision", AFF_DARK_VISION, TRUE},
                                         {"berserk", AFF_BERSERK, FALSE},
                                         {"swim", AFF_SWIM, TRUE},
                                         {"regeneration", AFF_REGENERATION, TRUE},
                                         {"octarine_fire", AFF_OCTARINE_FIRE, TRUE},
                                         {"", 0, 0}};

const struct flag_type off_flags[] = {{"area_attack", OFF_AREA_ATTACK, TRUE},
                                      {"backstab", OFF_BACKSTAB, TRUE},
                                      {"bash", OFF_BASH, TRUE},
                                      {"berserk", OFF_BERSERK, TRUE},
                                      {"disarm", OFF_DISARM, TRUE},
                                      {"dodge", OFF_DODGE, TRUE},
                                      {"fade", OFF_FADE, TRUE},
                                      {"fast", OFF_FAST, TRUE},
                                      {"kick", OFF_KICK, TRUE},
                                      {"dirt_kick", OFF_KICK_DIRT, TRUE},
                                      {"parry", OFF_PARRY, TRUE},
                                      {"rescue", OFF_RESCUE, TRUE},
                                      {"use_tail", OFF_TAIL, TRUE},
                                      {"trip", OFF_TRIP, TRUE},
                                      {"crush", OFF_CRUSH, TRUE},
                                      {"assist_all", ASSIST_ALL, TRUE},
                                      {"assist_align", ASSIST_ALIGN, TRUE},
                                      {"assist_race", ASSIST_RACE, TRUE},
                                      {"assist_players", ASSIST_PLAYERS, TRUE},
                                      {"assist_guard", ASSIST_GUARD, TRUE},
                                      {"assist_vnum", ASSIST_VNUM, TRUE},
                                      {"headbutt", OFF_HEADBUTT, TRUE},
                                      {"", 0, 0}};

const struct flag_type imm_flags[] = {{"summon", IMM_SUMMON, TRUE},
                                      {"charm", IMM_CHARM, TRUE},
                                      {"magic", IMM_MAGIC, TRUE},
                                      {"weapon", IMM_WEAPON, TRUE},
                                      {"bash", IMM_BASH, TRUE},
                                      {"pierce", IMM_PIERCE, TRUE},
                                      {"slash", IMM_SLASH, TRUE},
                                      {"fire", IMM_FIRE, TRUE},
                                      {"cold", IMM_COLD, TRUE},
                                      {"lightning", IMM_LIGHTNING, TRUE},
                                      {"acid", IMM_ACID, TRUE},
                                      {"poison", IMM_POISON, TRUE},
                                      {"negative", IMM_NEGATIVE, TRUE},
                                      {"holy", IMM_HOLY, TRUE},
                                      {"energy", IMM_ENERGY, TRUE},
                                      {"mental", IMM_MENTAL, TRUE},
                                      {"disease", IMM_DISEASE, TRUE},
                                      {"drowning", IMM_DROWNING, TRUE},
                                      {"light", IMM_LIGHT, TRUE},
                                      /*    {	"sound",		IMM_SOUND,	TRUE	},
                                          {	"wood",			IMM_WOOD,	TRUE	},
                                          {	"silver",		IMM_SILVER,	TRUE	},
                                          {	"iron",			IMM_IRON,	TRUE	},*/
                                      {"", 0, 0}};

const struct flag_type res_flags[] = {
    /*    {	"summon",		RES_SUMMON,	TRUE	}, */
    {"charm", RES_CHARM, TRUE},
    {"magic", RES_MAGIC, TRUE},
    {"weapon", RES_WEAPON, TRUE},
    {"bash", RES_BASH, TRUE},
    {"pierce", RES_PIERCE, TRUE},
    {"slash", RES_SLASH, TRUE},
    {"fire", RES_FIRE, TRUE},
    {"cold", RES_COLD, TRUE},
    {"lightning", RES_LIGHTNING, TRUE},
    {"acid", RES_ACID, TRUE},
    {"poison", RES_POISON, TRUE},
    {"negative", RES_NEGATIVE, TRUE},
    {"holy", RES_HOLY, TRUE},
    {"energy", RES_ENERGY, TRUE},
    {"mental", RES_MENTAL, TRUE},
    {"disease", RES_DISEASE, TRUE},
    {"drowning", RES_DROWNING, TRUE},
    {"light", RES_LIGHT, TRUE},
    /*    {	"sound",		RES_SOUND,	TRUE	},
        {	"wood",			RES_WOOD,	TRUE	},
        {	"silver",		RES_SILVER,	TRUE	},
        {	"iron",			RES_IRON,	TRUE	},*/
    {"", 0, 0}};

const struct flag_type vuln_flags[] = {
    /*    {	"summon",		VULN_SUMMON,	TRUE	},
          {	"charm",		VULN_CHARM,	TRUE	},*/
    {"magic", VULN_MAGIC, TRUE},
    {"weapon", VULN_WEAPON, TRUE},
    {"bash", VULN_BASH, TRUE},
    {"pierce", VULN_PIERCE, TRUE},
    {"slash", VULN_SLASH, TRUE},
    {"fire", VULN_FIRE, TRUE},
    {"cold", VULN_COLD, TRUE},
    {"lightning", VULN_LIGHTNING, TRUE},
    {"acid", VULN_ACID, TRUE},
    {"poison", VULN_POISON, TRUE},
    {"negative", VULN_NEGATIVE, TRUE},
    {"holy", VULN_HOLY, TRUE},
    {"energy", VULN_ENERGY, TRUE},
    {"mental", VULN_MENTAL, TRUE},
    {"disease", VULN_DISEASE, TRUE},
    {"drowning", VULN_DROWNING, TRUE},
    {"light", VULN_LIGHT, TRUE},
    /*    {	"sound",		VULN_SOUND,	TRUE	},*/
    {"wood", VULN_WOOD, TRUE},
    {"silver", VULN_SILVER, TRUE},
    {"iron", VULN_IRON, TRUE},
    {"", 0, 0}};
const struct flag_type form_flags[] = {{"edible", FORM_EDIBLE, TRUE},
                                       {"poison", FORM_POISON, TRUE},
                                       {"magical", FORM_MAGICAL, TRUE},
                                       {"instant_decay", FORM_INSTANT_DECAY, TRUE},
                                       {"other", FORM_OTHER, TRUE},
                                       {"animal", FORM_ANIMAL, TRUE},
                                       {"sentient", FORM_SENTIENT, TRUE},
                                       {"undead", FORM_UNDEAD, TRUE},
                                       {"construct", FORM_CONSTRUCT, TRUE},
                                       {"mist", FORM_MIST, TRUE},
                                       {"intangible", FORM_INTANGIBLE, TRUE},
                                       {"biped", FORM_BIPED, TRUE},
                                       {"centaur", FORM_CENTAUR, TRUE},
                                       {"insect", FORM_INSECT, TRUE},
                                       {"spider", FORM_SPIDER, TRUE},
                                       {"crustacean", FORM_CRUSTACEAN, TRUE},
                                       {"worm", FORM_WORM, TRUE},
                                       {"blob", FORM_BLOB, TRUE},
                                       {"mammal", FORM_MAMMAL, TRUE},
                                       {"bird", FORM_BIRD, TRUE},
                                       {"reptile", FORM_REPTILE, TRUE},
                                       {"snake", FORM_SNAKE, TRUE},
                                       {"dragon", FORM_DRAGON, TRUE},
                                       {"amphibian", FORM_AMPHIBIAN, TRUE},
                                       {"fish", FORM_FISH, TRUE},
                                       {"cold_blood", FORM_COLD_BLOOD, TRUE},
                                       {"", 0, 0}};

const struct flag_type part_flags[] = {{"head", PART_HEAD, TRUE},
                                       {"arms", PART_ARMS, TRUE},
                                       {"legs", PART_LEGS, TRUE},
                                       {"heart", PART_HEART, TRUE},
                                       {"brains", PART_BRAINS, TRUE},
                                       {"guts", PART_GUTS, TRUE},
                                       {"hands", PART_HANDS, TRUE},
                                       {"feet", PART_FEET, TRUE},
                                       {"fingers", PART_FINGERS, TRUE},
                                       {"ear", PART_EAR, TRUE},
                                       {"eye", PART_EYE, TRUE},
                                       {"long_tongue", PART_LONG_TONGUE, TRUE},
                                       {"eyestalks", PART_EYESTALKS, TRUE},
                                       {"tentacles", PART_TENTACLES, TRUE},
                                       {"fins", PART_FINS, TRUE},
                                       {"wings", PART_WINGS, TRUE},
                                       {"tail", PART_TAIL, TRUE},
                                       {"claws", PART_CLAWS, TRUE},
                                       {"fangs", PART_FANGS, TRUE},
                                       {"horns", PART_HORNS, TRUE},
                                       {"scales", PART_SCALES, TRUE},
                                       {"tusks", PART_TUSKS, TRUE},
                                       {"", 0, 0}};

const struct flag_type comm_flags[] = {{"quiet", COMM_QUIET, TRUE},
                                       {"deaf", COMM_DEAF, TRUE},
                                       {"nowiz", COMM_NOWIZ, TRUE},
                                       {"noauction", COMM_NOAUCTION, TRUE},
                                       {"nogossip", COMM_NOGOSSIP, TRUE},
                                       {"noquestion", COMM_NOQUESTION, TRUE},
                                       {"nomusic", COMM_NOMUSIC, TRUE},
                                       {"announce", COMM_NOANNOUNCE, TRUE},
                                       {"noquest", COMM_NOQWEST, TRUE},
                                       {"noallege", COMM_NOALLEGE, TRUE},
                                       {"nophilosophise", COMM_NOPHILOSOPHISE, TRUE},
                                       {"compact", COMM_COMPACT, TRUE},
                                       {"brief", COMM_BRIEF, TRUE},
                                       {"prompt", COMM_PROMPT, TRUE},
                                       {"combine", COMM_COMBINE, TRUE},
                                       {"telnet_ga", COMM_TELNET_GA, TRUE},
                                       {"showafk", COMM_SHOWAFK, TRUE},
                                       {"showdef", COMM_SHOWDEFENCE, TRUE},
                                       {"affect", COMM_AFFECT, TRUE},
                                       {"nogratz", COMM_NOGRATZ, TRUE},
                                       {"noemote", COMM_NOEMOTE, FALSE},
                                       {"noshout", COMM_NOSHOUT, FALSE},
                                       {"notell", COMM_NOTELL, FALSE},
                                       {"nochannels", COMM_NOCHANNELS, FALSE},

                                       {"", 0, 0}};

const struct flag_type area_flags[] = {{"none", AREA_NONE, FALSE},
                                       {"changed", AREA_CHANGED, FALSE},
                                       {"added", AREA_ADDED, FALSE},
                                       {"loading", AREA_LOADING, FALSE},
                                       {"verbose", AREA_VERBOSE, FALSE},
                                       {"delete", AREA_DELETE, FALSE},
                                       {"unfinished", AREA_UNFINISHED, FALSE},
                                       {"pkzone", AREA_PKZONE, TRUE},
                                       {"conquest", AREA_CONQUEST, TRUE},
                                       {"arena", AREA_ARENA, TRUE},
                                       {"", 0, 0}};

const struct flag_type sex_flags[] = {
    {"male", SEX_MALE, TRUE}, {"female", SEX_FEMALE, TRUE}, {"neutral", SEX_NEUTRAL, TRUE}, {"", 0, 0}};

const struct flag_type exit_flags[] = {{"door", EX_ISDOOR, TRUE},
                                       {"closed", EX_CLOSED, TRUE},
                                       {"locked", EX_LOCKED, TRUE},
                                       {"pickproof", EX_PICKPROOF, TRUE},
                                       {"passproof", EX_PASSPROOF, TRUE},
                                       {"easy", EX_EASY, TRUE},
                                       {"hard", EX_HARD, TRUE},
                                       {"infuriating", EX_INFURIATING, TRUE},
                                       {"noclose", EX_NOCLOSE, TRUE},
                                       {"nolock", EX_NOLOCK, TRUE},
                                       {"trap", EX_TRAP, TRUE},
                                       {"trap_darts", EX_TRAP_DARTS, TRUE},
                                       {"trap_poison", EX_TRAP_POISON, TRUE},
                                       {"trap_exploding", EX_TRAP_EXPLODING, TRUE},
                                       {"trap_sleepgas", EX_TRAP_SLEEPGAS, TRUE},
                                       {"trap_death", EX_TRAP_DEATH, TRUE},
                                       {"secret", EX_SECRET, TRUE},
                                       {"", 0, 0}};

const struct flag_type door_resets[] = {
    {"open and unlocked", 0, TRUE}, {"closed and unlocked", 1, TRUE}, {"closed and locked", 2, TRUE}, {"", 0, 0}};

const struct flag_type room_flags[] = {{"dark", ROOM_DARK, TRUE},
                                       {"no_mob", ROOM_NO_MOB, TRUE},
                                       {"indoors", ROOM_INDOORS, TRUE},
                                       {"private", ROOM_PRIVATE, TRUE},
                                       {"safe", ROOM_SAFE, TRUE},
                                       {"solitary", ROOM_SOLITARY, TRUE},
                                       {"pet_shop", ROOM_PET_SHOP, TRUE},
                                       {"no_recall", ROOM_NO_RECALL, TRUE},
                                       {"imp_only", ROOM_IMP_ONLY, TRUE},
                                       {"gods_only", ROOM_GODS_ONLY, TRUE},
                                       {"heroes_only", ROOM_HEROES_ONLY, TRUE},
                                       {"newbies_only", ROOM_NEWBIES_ONLY, TRUE},
                                       {"law", ROOM_LAW, TRUE},
                                       {"saveobj", ROOM_SAVEOBJ, TRUE},
                                       {"", 0, 0}};

const struct flag_type sector_flags[] = {
    {"inside", SECT_INSIDE, TRUE},   {"city", SECT_CITY, TRUE},           {"field", SECT_FIELD, TRUE},
    {"forest", SECT_FOREST, TRUE},   {"hills", SECT_HILLS, TRUE},         {"mountain", SECT_MOUNTAIN, TRUE},
    {"swim", SECT_WATER_SWIM, TRUE}, {"noswim", SECT_WATER_NOSWIM, TRUE}, {"unused", SECT_UNUSED, TRUE},
    {"air", SECT_AIR, TRUE},         {"desert", SECT_DESERT, TRUE},       {"", 0, 0}};

const struct flag_type type_flags[] = {{"light", ITEM_LIGHT, TRUE},
                                       {"scroll", ITEM_SCROLL, TRUE},
                                       {"wand", ITEM_WAND, TRUE},
                                       {"staff", ITEM_STAFF, TRUE},
                                       {"weapon", ITEM_WEAPON, TRUE},
                                       {"treasure", ITEM_TREASURE, TRUE},
                                       {"armor", ITEM_ARMOR, TRUE},
                                       {"potion", ITEM_POTION, TRUE},
                                       {"clothing", ITEM_CLOTHING, TRUE},
                                       {"furniture", ITEM_FURNITURE, TRUE},
                                       {"trash", ITEM_TRASH, TRUE},
                                       {"container", ITEM_CONTAINER, TRUE},
                                       {"drink-container", ITEM_DRINK_CON, TRUE},
                                       {"key", ITEM_KEY, TRUE},
                                       {"food", ITEM_FOOD, TRUE},
                                       {"money", ITEM_MONEY, TRUE},
                                       {"boat", ITEM_BOAT, TRUE},
                                       {"npc corpse", ITEM_CORPSE_NPC, TRUE},
                                       {"pc corpse", ITEM_CORPSE_PC, FALSE},
                                       {"fountain", ITEM_FOUNTAIN, TRUE},
                                       {"pill", ITEM_PILL, TRUE},
                                       {"protect", ITEM_PROTECT, TRUE},
                                       {"map", ITEM_MAP, TRUE},
                                       {"portal", ITEM_PORTAL, TRUE},
                                       /*    {	"room key",		ITEM_ROOM_KEY,		TRUE	}, */
                                       {"bomb", ITEM_BOMB, TRUE},
                                       {"", 0, 0}};

const struct flag_type extra_flags[] = {{"glow", ITEM_GLOW, TRUE},
                                        {"hum", ITEM_HUM, TRUE},
                                        {"dark", ITEM_DARK, TRUE},
                                        {"lock", ITEM_LOCK, TRUE},
                                        {"evil", ITEM_EVIL, TRUE},
                                        {"invis", ITEM_INVIS, TRUE},
                                        {"magic", ITEM_MAGIC, TRUE},
                                        {"nodrop", ITEM_NODROP, TRUE},
                                        {"bless", ITEM_BLESS, TRUE},
                                        {"anti-good", ITEM_ANTI_GOOD, TRUE},
                                        {"anti-evil", ITEM_ANTI_EVIL, TRUE},
                                        {"anti-neutral", ITEM_ANTI_NEUTRAL, TRUE},
                                        {"noremove", ITEM_NOREMOVE, TRUE},
                                        {"inventory", ITEM_INVENTORY, TRUE},
                                        {"nopurge", ITEM_NOPURGE, TRUE},
                                        {"rot_death", ITEM_ROT_DEATH, TRUE},
                                        {"vis_death", ITEM_VIS_DEATH, TRUE},
                                        /*    {	"noshow",		ITEM_NOSHOW,		TRUE	},*/
                                        {"nolocate", ITEM_NO_LOCATE, TRUE},
                                        {"", 0, 0}};

const struct flag_type wear_flags[] = {{"take", ITEM_TAKE, TRUE},
                                       {"finger", ITEM_WEAR_FINGER, TRUE},
                                       {"neck", ITEM_WEAR_NECK, TRUE},
                                       {"body", ITEM_WEAR_BODY, TRUE},
                                       {"head", ITEM_WEAR_HEAD, TRUE},
                                       {"legs", ITEM_WEAR_LEGS, TRUE},
                                       {"feet", ITEM_WEAR_FEET, TRUE},
                                       {"hands", ITEM_WEAR_HANDS, TRUE},
                                       {"arms", ITEM_WEAR_ARMS, TRUE},
                                       {"shield", ITEM_WEAR_SHIELD, TRUE},
                                       {"about", ITEM_WEAR_ABOUT, TRUE},
                                       {"waist", ITEM_WEAR_WAIST, TRUE},
                                       {"wrist", ITEM_WEAR_WRIST, TRUE},
                                       {"wield", ITEM_WIELD, TRUE},
                                       {"hold", ITEM_HOLD, TRUE},
                                       /*    {	"no_sac",		ITEM_NO_SAC,		TRUE	},*/
                                       {"two_hands", ITEM_TWO_HANDS, TRUE},
                                       /*    {	"wear_float",		ITEM_WEAR_FLOAT,	TRUE	}, */
                                       {"", 0, 0}};

/*
 * Used when adding an affect to tell where it goes.
 * See addaffect and delaffect in act_olc.c
 */
const struct flag_type apply_flags[] = {{"none", APPLY_NONE, TRUE},
                                        {"str", APPLY_STR, TRUE},
                                        {"dex", APPLY_DEX, TRUE},
                                        {"int", APPLY_INT, TRUE},
                                        {"wis", APPLY_WIS, TRUE},
                                        {"con", APPLY_CON, TRUE},
                                        {"sex", APPLY_SEX, TRUE},
                                        {"class", APPLY_CLASS, TRUE},
                                        {"level", APPLY_LEVEL, TRUE},
                                        {"age", APPLY_AGE, TRUE},
                                        {"height", APPLY_HEIGHT, TRUE},
                                        {"weight", APPLY_WEIGHT, TRUE},
                                        {"mana", APPLY_MANA, TRUE},
                                        {"hp", APPLY_HIT, TRUE},
                                        {"move", APPLY_MOVE, TRUE},
                                        {"gold", APPLY_GOLD, TRUE},
                                        {"exp", APPLY_EXP, TRUE},
                                        {"ac", APPLY_AC, TRUE},
                                        {"hitroll", APPLY_HITROLL, TRUE},
                                        {"damroll", APPLY_DAMROLL, TRUE},
                                        {"saving-para", APPLY_SAVING_PARA, TRUE},
                                        {"saving-rod", APPLY_SAVING_ROD, TRUE},
                                        {"saving-petri", APPLY_SAVING_PETRI, TRUE},
                                        {"saving-breath", APPLY_SAVING_BREATH, TRUE},
                                        {"saving-spell", APPLY_SAVING_SPELL, TRUE},
                                        /*    {	"spell",		APPLY_SPELL_AFFECT,	TRUE	},*/
                                        {"", 0, 0}};

/*
 * What is seen.
 */
const struct flag_type wear_loc_strings[] = {{"in the inventory", WEAR_NONE, TRUE},
                                             {"as a light", WEAR_LIGHT, TRUE},
                                             {"on the left finger", WEAR_FINGER_L, TRUE},
                                             {"on the right finger", WEAR_FINGER_R, TRUE},
                                             {"around the neck (1)", WEAR_NECK_1, TRUE},
                                             {"around the neck (2)", WEAR_NECK_2, TRUE},
                                             {"on the body", WEAR_BODY, TRUE},
                                             {"over the head", WEAR_HEAD, TRUE},
                                             {"on the legs", WEAR_LEGS, TRUE},
                                             {"on the feet", WEAR_FEET, TRUE},
                                             {"on the hands", WEAR_HANDS, TRUE},
                                             {"on the arms", WEAR_ARMS, TRUE},
                                             {"as a shield", WEAR_SHIELD, TRUE},
                                             {"about the shoulders", WEAR_ABOUT, TRUE},
                                             {"around the waist", WEAR_WAIST, TRUE},
                                             {"on the left wrist", WEAR_WRIST_L, TRUE},
                                             {"on the right wrist", WEAR_WRIST_R, TRUE},
                                             {"wielded", WEAR_WIELD, TRUE},
                                             {"held in the hands", WEAR_HOLD, TRUE},
                                             /*    {	"floating",		WEAR_FLOAT,	TRUE	},*/
                                             /*    {	"wielded secondary",	WEAR_SECONDARY,	TRUE	},*/
                                             {"", 0, FALSE}};

/*
 * What is typed.
 * Neck2 should not be settable for loaded mobiles.
 */
const struct flag_type wear_loc_flags[] = {{"none", WEAR_NONE, TRUE},
                                           {"light", WEAR_LIGHT, TRUE},
                                           {"lfinger", WEAR_FINGER_L, TRUE},
                                           {"rfinger", WEAR_FINGER_R, TRUE},
                                           {"neck1", WEAR_NECK_1, TRUE},
                                           {"neck2", WEAR_NECK_2, TRUE},
                                           {"body", WEAR_BODY, TRUE},
                                           {"head", WEAR_HEAD, TRUE},
                                           {"legs", WEAR_LEGS, TRUE},
                                           {"feet", WEAR_FEET, TRUE},
                                           {"hands", WEAR_HANDS, TRUE},
                                           {"arms", WEAR_ARMS, TRUE},
                                           {"shield", WEAR_SHIELD, TRUE},
                                           {"about", WEAR_ABOUT, TRUE},
                                           {"waist", WEAR_WAIST, TRUE},
                                           {"lwrist", WEAR_WRIST_L, TRUE},
                                           {"rwrist", WEAR_WRIST_R, TRUE},
                                           {"wielded", WEAR_WIELD, TRUE},
                                           {"hold", WEAR_HOLD, TRUE},
                                           /*    {	"floating",	WEAR_FLOAT,	TRUE	},*/
                                           /*    {	"secondary",	WEAR_SECONDARY,	TRUE	}, */
                                           {"", 0, 0}};

const struct flag_type weapon_flags[] = {
    {"hit", 0, TRUE}, /*  0 */
    {"slice", 1, TRUE},         {"stab", 2, TRUE},           {"slash", 3, TRUE},
    {"whip", 4, TRUE},          {"claw", 5, TRUE}, /*  5 */
    {"blast", 6, TRUE},         {"pound", 7, TRUE},          {"crush", 8, TRUE},
    {"grep", 9, TRUE},          {"bite", 10, TRUE}, /* 10 */
    {"pierce", 11, TRUE},       {"suction", 12, TRUE},       {"beating", 13, TRUE},
    {"digestion", 14, TRUE},    {"charge", 15, TRUE}, /* 15 */
    {"slap", 16, TRUE},         {"punch", 17, TRUE},         {"wrath", 18, TRUE},
    {"magic", 19, TRUE},        {"divine power", 20, TRUE}, /* 20 */
    {"cleave", 21, TRUE},       {"scratch", 22, TRUE},       {"peck pierce", 23, TRUE},
    {"peck bash", 24, TRUE},    {"chop", 25, TRUE}, /* 25 */
    {"sting", 26, TRUE},        {"smash", 27, TRUE},         {"shocking bite", 28, TRUE},
    {"flaming bite", 29, TRUE}, {"freezing bite", 30, TRUE}, /* 30 */
    {"acidic bite", 31, TRUE},  {"chomp", 32, TRUE},         {"", 0, 0}};

const struct flag_type container_flags[] = {{"closeable", CONT_CLOSEABLE, TRUE},
                                            {"pickproof", CONT_PICKPROOF, TRUE},
                                            {"closed", CONT_CLOSED, TRUE},
                                            {"locked", CONT_LOCKED, TRUE},
                                            /*    {	"put_on",		CONT_PUT_ON,		TRUE	},
                                                {	"trap",			CONT_TRAP,		TRUE	},
                                                {	"trap_darts",		CONT_TRAP_DARTS,        TRUE    },
                                                {	"trap_poison",		CONT_TRAP_POISON,	TRUE	},
                                                {	"trap_exploding",	CONT_TRAP_EXPLODING,	TRUE	},
                                                {	"trap_extract",		CONT_TRAP_EXTRACT,	TRUE	},
                                                {	"trap_sleepgas",	CONT_TRAP_SLEEPGAS,	TRUE	},
                                                {	"trap_death",		CONT_TRAP_DEATH,	TRUE	}, */
                                            {"", 0, 0}};

const struct flag_type liquid_flags[] = {{"water", 0, TRUE},
                                         {"beer", 1, TRUE},
                                         {"red wine", 2, TRUE},
                                         {"ale", 3, TRUE},
                                         {"dark ale", 4, TRUE},
                                         {"whisky", 5, TRUE},
                                         {"lemonade", 6, TRUE},
                                         {"firebreather", 7, TRUE},
                                         {"local specialty", 8, TRUE},
                                         {"slime mold juice", 9, TRUE},
                                         {"milk", 10, TRUE},
                                         {"tea", 11, TRUE},
                                         {"coffee", 12, TRUE},
                                         {"blood", 13, TRUE},
                                         {"salt water", 14, TRUE},
                                         {"coke", 15, TRUE},
                                         {"root beer", 16, TRUE},
                                         {"elvish wine", 17, TRUE},
                                         {"white wine", 18, TRUE},
                                         {"champagne", 19, TRUE},
                                         {"mead", 20, TRUE},
                                         {"rose wine", 21, TRUE},
                                         {"benedictine wine", 22, TRUE},
                                         {"vodka", 23, TRUE},
                                         {"cranberry juice", 24, TRUE},
                                         {"orange juice", 25, TRUE},
                                         {"absinthe", 26, TRUE},
                                         {"brandy", 27, TRUE},
                                         {"aquavit", 28, TRUE},
                                         {"schnapps", 29, TRUE},
                                         {"icewine", 30, TRUE},
                                         {"amontillado", 31, TRUE},
                                         {"sherry", 32, TRUE},
                                         {"framboise", 33, TRUE},
                                         {"rum", 34, TRUE},
                                         {"cordial", 35, TRUE},
                                         {"", 0, 0}};

const struct flag_type weapontype_flags[] = {{"flaming", WEAPON_FLAMING, TRUE},
                                             {"frost", WEAPON_FROST, TRUE},
                                             {"vampiric", WEAPON_VAMPIRIC, TRUE},
                                             {"sharp", WEAPON_SHARP, TRUE},
                                             {"vorpal", WEAPON_VORPAL, TRUE},
                                             {"two_hands", WEAPON_TWO_HANDS, TRUE},
                                             {"lightning", WEAPON_LIGHTNING, TRUE},
                                             {"poison", WEAPON_POISONED, TRUE},
                                             {"acid", WEAPON_ACID, TRUE},
                                             {"plagued", WEAPON_PLAGUED, TRUE},

                                             {"", 0, 0}};

const struct flag_type gate_flags[] = {
    /*    { "normal_exit",		GATE_NORMAL_EXIT,	TRUE	},
        { "nocurse",		GATE_NOCURSE,		TRUE	},
        { "gowith",			GATE_GOWITH,		TRUE	},
        { "buggy",			GATE_BUGGY,		TRUE	},
        { "random",			GATE_RANDOM,		TRUE	},
        { "climb",			GATE_CLIMB,		TRUE	},
        { "jump",			GATE_JUMP,		TRUE	}, */
    {"", 0, 0}};

const struct flag_type furniture_flags[] = {
    /*    { "stand_at",		STAND_AT,		TRUE	},
        { "stand_on",		STAND_ON,		TRUE	},
        { "stand_in",		STAND_IN,		TRUE	},
        { "sit_at",			SIT_AT,			TRUE	},
        { "sit_on",			SIT_ON,			TRUE	},
        { "sit_in",			SIT_IN,			TRUE	},
        { "rest_at",		REST_AT,		TRUE	},
        { "rest_on",		REST_ON,		TRUE	},
        { "rest_in",		REST_IN,		TRUE	},
        { "sleep_at",		SLEEP_AT,		TRUE	},
        { "sleep_on",		SLEEP_ON,		TRUE	},
        { "sleep_in",		SLEEP_IN,		TRUE	},
        { "put_at",			PUT_AT,			TRUE	},
        { "put_on",			PUT_ON,			TRUE	},
        { "put_in",			PUT_IN,			TRUE	},
        { "put_inside",		PUT_INSIDE,		TRUE	}, */
    {"", 0, 0}};

const struct flag_type mprog_flags[] = {
    /*    {	"act",			TRIG_ACT,		TRUE	},
        {	"bribe",		TRIG_BRIBE,		TRUE 	},
        {	"death",		TRIG_DEATH,		TRUE    },
        {	"entry",		TRIG_ENTRY,		TRUE	},
        {	"fight",		TRIG_FIGHT,		TRUE	},
        {	"give",			TRIG_GIVE,		TRUE	},
        {	"greet",		TRIG_GREET,		TRUE    },
        {	"grall",		TRIG_GRALL,		TRUE	},
        {	"kill",			TRIG_KILL,		TRUE	},
        {	"hpcnt",		TRIG_HPCNT,		TRUE    },
        {	"random",		TRIG_RANDOM,		TRUE	},
        {	"speech",		TRIG_SPEECH,		TRUE	},
        {	"speech",		TRIG_SPEECH,		TRUE	},
        {	"exit",			TRIG_EXIT,		TRUE    },
        {	"exall",		TRIG_EXALL,		TRUE    },
        {	"delay",		TRIG_DELAY,		TRUE    },
        {	"surr",			TRIG_SURR,		TRUE    },
        {	"leave",		TRIG_LEAVE,		TRUE	},
        {	"leall",		TRIG_LEALL,		TRUE	},
        {   "timer",		TRIG_TIMER,		TRUE	},
        {	"hour",			TRIG_HOUR,		TRUE	},  */
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
                                       {0, NULL}};

const struct weapon_type weapon_table[] = {{"sword", OBJ_VNUM_SCHOOL_SWORD, WEAPON_SWORD, &gsn_sword},
                                           {"mace", OBJ_VNUM_SCHOOL_MACE, WEAPON_MACE, &gsn_mace},
                                           {"dagger", OBJ_VNUM_SCHOOL_DAGGER, WEAPON_DAGGER, &gsn_dagger},
                                           {"axe", OBJ_VNUM_SCHOOL_AXE, WEAPON_AXE, &gsn_axe},

                                           {"exotic", OBJ_VNUM_SCHOOL_SWORD, WEAPON_EXOTIC, NULL},
                                           {"spear", OBJ_VNUM_SCHOOL_MACE, WEAPON_SPEAR, &gsn_spear},
                                           {"flail", OBJ_VNUM_SCHOOL_DAGGER, WEAPON_FLAIL, &gsn_flail},
                                           {"whip", OBJ_VNUM_SCHOOL_AXE, WEAPON_WHIP, &gsn_whip},
                                           {"polearm", OBJ_VNUM_SCHOOL_AXE, WEAPON_POLEARM, &gsn_polearm},
                                           {NULL, 0, 0, NULL}};
