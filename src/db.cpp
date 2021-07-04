/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  db.c: file in/out, area database handler                             */
/*                                                                       */
/*************************************************************************/

#include "db.h"
#include "AFFECT_DATA.hpp"
#include "AREA_DATA.hpp"
#include "BitsCharAct.hpp"
#include "BitsCharOffensive.hpp"
#include "Char.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "Exit.hpp"
#include "Help.hpp"
#include "MobIndexData.hpp"
#include "Note.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"
#include "ResetData.hpp"
#include "Shop.hpp"
#include "SkillNumbers.hpp"
#include "SkillTables.hpp"
#include "Socials.hpp"
#include "TimeInfoData.hpp"
#include "VnumRooms.hpp"
#include "WeatherData.hpp"
#include "buffer.h"
#include "common/Configuration.hpp"
#include "common/urange.hpp"
#include "handler.hpp"
#include "interp.h"
#include "lookup.h"
#include "magic.h"
#include "merc.h"
#include "mob_prog.hpp"
#include "string_utils.hpp"

#include <range/v3/algorithm/fill.hpp>
#include <range/v3/iterator/operations.hpp>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <range/v3/algorithm/find_if.hpp>
#include <sys/resource.h>

namespace {

static inline constexpr auto RoomResetAgeOccupiedArea = 15;
static inline constexpr auto RoomResetAgeUnoccupiedArea = 10;

Shop *shop_first;
Shop *shop_last;

std::map<int, MobIndexData> mob_indexes; // a map only so things like "vnum mob XXX" are ordered.
char *string_hash[MAX_KEY_HASH];

char *string_space;
char *top_string;

int top_exit;
int top_reset;
int top_room;
int top_shop;
// Count of object templates. TODO: Get rid of this.
int newobjs = 0;
// Index number of the latest affect on an object.
int top_obj_affect;

/**
 * Commands used in #RESETS section of area files
 */
constexpr auto ResetMobInRoom = 'M'; /* place mob in a room */
constexpr auto ResetEquipObjMob = 'E'; /* equip and item on a mobile */
constexpr auto ResetGiveObjMob = 'G'; /* give an item to a mobile's inventory */
constexpr auto ResetObjInRoom = 'O'; /* place a static object in a room */
constexpr auto ResetPutObjInObj = 'P'; /* place a static object in another object */
constexpr auto ResetExitFlags = 'D'; /* set exit closed/locked flags */
constexpr auto ResetRandomizeExits = 'R'; /* randomize room exits */
constexpr auto ResetComment = '*'; /* comment line */
constexpr auto ResetEndSection = 'S'; /* end of the resets section */

}

/* Externally referenced functions. */
void wiznet_initialise();
SpecialFunc spec_lookup(const char *name);

// Mutable global: modified whenever a new Char is loaded from the database or when a player Char logs in or out.
GenericList<Char *> char_list;
// Mutable global: modified whenever a new object is created or destroyed.
GenericList<Object *> object_list;
// Mutable global: object template pointers.
ObjectIndex *obj_index_hash[MAX_KEY_HASH];
// Mutable global: room template pointers.
Room *room_hash[MAX_KEY_HASH];
// Mutable global: index number of the latest object template to be created.
int top_obj_index;

char str_empty[1]; // TODO: Get rid of this and str_dup()

// Global skill numbers initialized once on startup.
sh_int gsn_backstab;
sh_int gsn_dodge;
sh_int gsn_hide;
sh_int gsn_peek;
sh_int gsn_pick_lock;
sh_int gsn_sneak;
sh_int gsn_steal;

sh_int gsn_disarm;
sh_int gsn_enhanced_damage;
sh_int gsn_kick;
sh_int gsn_parry;
sh_int gsn_rescue;
sh_int gsn_second_attack;
sh_int gsn_third_attack;

sh_int gsn_blindness;
sh_int gsn_charm_person;
sh_int gsn_curse;
sh_int gsn_invis;
sh_int gsn_mass_invis;
sh_int gsn_poison;
sh_int gsn_plague;
sh_int gsn_sleep;

sh_int gsn_axe;
sh_int gsn_dagger;
sh_int gsn_flail;
sh_int gsn_mace;
sh_int gsn_polearm;
sh_int gsn_shield_block;
sh_int gsn_spear;
sh_int gsn_sword;
sh_int gsn_whip;

sh_int gsn_bash;
sh_int gsn_berserk;
sh_int gsn_dirt;
sh_int gsn_hand_to_hand;
sh_int gsn_trip;

sh_int gsn_fast_healing;
sh_int gsn_haggle;
sh_int gsn_lore;
sh_int gsn_meditation;

sh_int gsn_scrolls;
sh_int gsn_staves;
sh_int gsn_wands;
sh_int gsn_recall;

sh_int gsn_headbutt;
sh_int gsn_ride;
sh_int gsn_throw;

sh_int gsn_sharpen;

sh_int gsn_raise_dead;
sh_int gsn_octarine_fire;
sh_int gsn_insanity;
sh_int gsn_bless;

/*
 * Merc-2.2 MOBprogram locals - Faramir 31/8/1998
 */

int mprog_name_to_type(char *name);
MPROG_DATA *mprog_file_read(char *file_name, MPROG_DATA *mprg, MobIndexData *pMobIndex);
void load_mobprogs(FILE *fp);
void mprog_read_programs(FILE *fp, MobIndexData *pMobIndex);

/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
#define MAX_STRING 2650976
#define MAX_PERM_BLOCK 131072
#define MAX_MEM_LIST 14

void *rgFreeList[MAX_MEM_LIST];
const int rgSizeList[MAX_MEM_LIST] = {
    /*   16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768-64 */
    16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144};

int nAllocString;
int sAllocString;
int nAllocPerm;
int sAllocPerm;

/* Semi-locals. */
bool fBootDb;
static bool area_header_found;

/* Local booting procedures. */
void init_mm();
void load_area(FILE *fp, const std::string &area_name);
void load_helps(FILE *fp);
void load_mobiles(FILE *fp);
void load_objects(FILE *fp);

void load_resets(FILE *fp);
void new_reset(Room *, ResetData *);

void load_rooms(FILE *fp);

void load_shops(FILE *fp);
void load_specials(FILE *fp);

void fix_exits();

void reset_area(AREA_DATA *pArea);

/* RT max open files fix */

void maxfilelimit() {
    struct rlimit r;

    getrlimit(RLIMIT_NOFILE, &r);
    r.rlim_cur = r.rlim_max;
    setrlimit(RLIMIT_NOFILE, &r);
}

/* Big mama top level function. */
void boot_db() {

    /* open file fix */
    maxfilelimit();

    /* Init some data space stuff. */
    if ((string_space = static_cast<char *>(calloc(1, MAX_STRING))) == nullptr) {
        bug("Boot_db: can't alloc {} string space.", MAX_STRING);
        exit(1);
    }
    top_string = string_space;
    fBootDb = true;

    /* Init random number generator. */
    init_mm();

    // Set time and weather.
    time_info = TimeInfoData(Clock::now());
    weather_info = WeatherData(Rng::global_rng(), time_info);

    /* Assign gsn's for skills which have them. */
    {
        int sn;

        for (sn = 0; sn < MAX_SKILL; sn++) {
            if (skill_table[sn].pgsn != nullptr)
                *skill_table[sn].pgsn = sn;
        }
    }

    /* Read in all the area files. */
    {
        FILE *fpList;
        const auto &config = Configuration::singleton();
        const auto area_file = config.area_file();
        if ((fpList = fopen(area_file.c_str(), "r")) == nullptr) {
            perror(area_file.c_str());
            exit(1);
        }

        for (;;) {
            std::string area_name = fread_word(fpList);
            if (area_name[0] == '$')
                break;

            FILE *area_fp{};
            if (area_name[0] == '-') {
                area_fp = stdin;
            } else {
                std::string area_file_path = fmt::format("{}{}", config.area_dir(), area_name);
                if ((area_fp = fopen(area_file_path.c_str(), "r")) == nullptr) {
                    perror(area_file_path.c_str());
                    exit(1);
                }
            }
            BugAreaFileContext context(area_name, area_fp);
            for (;;) {
                char *word;
                if (fread_letter(area_fp) != '#') {
                    bug("Boot_db: # not found.");
                    exit(1);
                }

                word = fread_word(area_fp);

                if (word[0] == '$')
                    break;
                else if (!str_cmp(word, "AREA"))
                    load_area(area_fp, area_name);
                else if (!str_cmp(word, "HELPS"))
                    load_helps(area_fp);
                else if (!str_cmp(word, "MOBILES"))
                    load_mobiles(area_fp);
                else if (!str_cmp(word, "OBJECTS"))
                    load_objects(area_fp);
                else if (!str_cmp(word, "RESETS"))
                    load_resets(area_fp);
                else if (!str_cmp(word, "ROOMS"))
                    load_rooms(area_fp);
                else if (!str_cmp(word, "SHOPS"))
                    load_shops(area_fp);
                else if (!str_cmp(word, "SOCIALS"))
                    load_socials(area_fp);
                else if (!str_cmp(word, "SPECIALS"))
                    load_specials(area_fp);
                else if (!str_cmp(word, "MOBPROGS"))
                    load_mobprogs(area_fp);
                else {
                    bug("Boot_db: bad section name.");
                    exit(1);
                }
            }

            if (area_fp != stdin)
                fclose(area_fp);
        }
        fclose(fpList);
    }
    /*
     * Fix up exits.
     * Declare db booting over.
     * Reset all areas once.
     * Load up the notes file.
     */
    fix_exits();
    fBootDb = false;
    AreaList::singleton().sort();
    area_update();
    note_initialise();
    wiznet_initialise();
    interp_initialise();
}

/* Snarf an 'area' header line. */
void load_area(FILE *fp, const std::string &area_name) {
    auto pArea = std::make_unique<AREA_DATA>();
    auto &area_list = AreaList::singleton();
    fread_string(fp); /* filename */
    pArea->areaname = fread_stdstring(fp);
    pArea->name = fread_stdstring(fp);
    int scanRet = sscanf(pArea->name.c_str(), "{%d %d}", &pArea->min_level, &pArea->max_level);
    if (scanRet != 2) {
        pArea->all_levels = true;
    }
    pArea->level_difference = pArea->max_level - pArea->min_level;
    pArea->lvnum = fread_number(fp);
    pArea->uvnum = fread_number(fp);
    pArea->area_num = area_list.count();
    pArea->filename = area_name;

    pArea->age = RoomResetAgeOccupiedArea; // trigger an area reset when main game loop starts
    pArea->nplayer = 0;
    pArea->empty = false;

    AreaList::singleton().add(std::move(pArea));

    area_header_found = true;
}

/* Snarf a help section. */
void load_helps(FILE *fp) {
    while (auto help = Help::load(fp, area_header_found ? AreaList::singleton().back() : nullptr))
        HelpList::singleton().add(std::move(*help));
}

/*
 * Adds a reset to a room.
 */
void new_reset(Room *room, ResetData *reset) {
    ResetData *last_reset;

    if (!room) {
        return;
    }
    last_reset = room->reset_last;

    if (!last_reset) {
        room->reset_first = reset;
        room->reset_last = reset;
    } else {
        room->reset_last->next = reset;
        room->reset_last = reset;
        room->reset_last->next = nullptr;
    }

    top_reset++;
}

void load_resets(FILE *fp) {
    ResetData *pReset;
    Room *room;
    Exit *pexit;
    int iLastRoom = 0;
    int iLastObj = 0;

    auto area_last = AreaList::singleton().back();
    if (area_last == nullptr) {
        bug("Load_resets: no #AREA seen yet.");
        exit(1);
    }

    for (;;) {

        char letter;
        if ((letter = fread_letter(fp)) == ResetEndSection)
            break;

        if (letter == ResetComment) {
            fread_to_eol(fp);
            continue;
        }

        pReset = static_cast<ResetData *>(alloc_perm(sizeof(*pReset)));
        pReset->command = letter;
        /* if_flag */ fread_number(fp);
        pReset->arg1 = fread_number(fp);

        pReset->arg2 = fread_number(fp);

        pReset->arg3 = (letter == ResetGiveObjMob || letter == ResetRandomizeExits) ? 0 : fread_number(fp);

        if (letter == ResetPutObjInObj || letter == ResetMobInRoom) {
            pReset->arg4 = fread_number(fp);
        } else
            pReset->arg4 = 0;

        fread_to_eol(fp);

        /* Don't validate now, do it after all area's have been loaded */
        /* Stuff to add reset to the correct room */
        switch (letter) {
        default:
            bug("Load_resets: bad command '{}'.", letter);
            exit(1);
            break;
        case ResetMobInRoom:
            if ((room = get_room(pReset->arg3))) {
                new_reset(room, pReset);
                iLastRoom = pReset->arg3;
            }
            break;
        case ResetObjInRoom:
            if ((room = get_room(pReset->arg3))) {
                new_reset(room, pReset);
                iLastObj = pReset->arg3;
            }
            break;
        case ResetPutObjInObj:
            if ((room = get_room(iLastObj)))
                new_reset(room, pReset);
            break;
        case ResetGiveObjMob:
        case ResetEquipObjMob:
            if ((room = get_room(iLastRoom))) {
                new_reset(room, pReset);
                iLastObj = iLastRoom;
            }
            break;
        case ResetExitFlags: {
            room = get_room(pReset->arg1);
            auto opt_door = try_cast_direction(pReset->arg2);

            if (!opt_door || !room || (pexit = room->exit[*opt_door]) == nullptr
                || !check_bit(pexit->rs_flags, EX_ISDOOR)) {
                bug("Load_resets: 'D': exit {} not door.", pReset->arg2);
                exit(1);
            }

            switch (pReset->arg3) {
            default: bug("Load_resets: 'D': bad 'locks': {}.", pReset->arg3);
            case 0: break;
            case 1: set_bit(pexit->rs_flags, EX_CLOSED); break;
            case 2: set_bit(pexit->rs_flags, EX_CLOSED | EX_LOCKED); break;
            }
            break;
        }
        case ResetRandomizeExits:
            if (pReset->arg2 < 0 || pReset->arg2 > static_cast<int>(all_directions.size())) {
                bug("Load_resets: 'R': bad exit {}.", pReset->arg2);
                exit(1);
            }

            if ((room = get_room(pReset->arg1)))
                new_reset(room, pReset);

            break;
        }

        pReset->next = nullptr;
        top_reset++;
    }
}

/* Snarf a room section. */
void load_rooms(FILE *fp) {

    auto area_last = AreaList::singleton().back();
    if (area_last == nullptr) {
        bug("Load_resets: no #AREA seen yet.");
        exit(1);
    }

    for (;;) {
        sh_int vnum;
        char letter;
        int iHash;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_rooms: # not found.");
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0)
            break;

        fBootDb = false;
        if (get_room(vnum) != nullptr) {
            bug("Load_rooms: vnum {} duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        auto *room = new Room;
        room->area = area_last;
        room->vnum = vnum;
        room->name = fread_string(fp);
        room->description = fread_string(fp);
        room->room_flags = fread_flag(fp);
        /* horrible hack */
        if (3000 <= vnum && vnum < 3400)
            set_bit(room->room_flags, ROOM_LAW);
        int sector_value = fread_number(fp);
        if (auto sector_type = try_get_sector_type(sector_value)) {
            room->sector_type = *sector_type;
        } else {
            bug("Invalid sector type {}, defaulted to {}", sector_value, room->sector_type);
        }

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'S')
                break;

            if (letter == 'D') {
                Exit *pexit;
                int locks;

                auto opt_door = try_cast_direction(fread_number(fp));
                if (!opt_door) {
                    bug("Fread_rooms: vnum {} has bad door number.", vnum);
                    exit(1);
                }

                pexit = static_cast<Exit *>(alloc_perm(sizeof(*pexit)));
                pexit->description = fread_string(fp);
                pexit->keyword = fread_string(fp);
                pexit->exit_info = 0;
                locks = fread_number(fp);
                pexit->key = fread_number(fp);
                auto exit_vnum = fread_number(fp);
                /* If an exit's destination room vnum is negative, it can be a cosmetic-only
                 * exit (-1), otherwise it's a one-way exit.
                 * fix_exits() ignores one-way exits that don't have a return path.
                 */
                if (exit_vnum < EXIT_VNUM_COSMETIC) {
                    pexit->is_one_way = true;
                    exit_vnum = -exit_vnum;
                }
                pexit->u1.vnum = exit_vnum;

                pexit->rs_flags = 0;

                switch (locks) {

                    /* the following statements assign rs_flags, replacing
                            exit_info which is what used to get set. */
                case 1: pexit->rs_flags = EX_ISDOOR; break;
                case 2: pexit->rs_flags = EX_ISDOOR | EX_PICKPROOF; break;
                case 3: pexit->rs_flags = EX_ISDOOR | EX_PASSPROOF; break;
                case 4: pexit->rs_flags = EX_ISDOOR | EX_PASSPROOF | EX_PICKPROOF; break;
                }

                room->exit[*opt_door] = pexit;
                top_exit++;
            } else if (letter == 'E') {
                auto keyword = fread_stdstring(fp);
                auto description = fread_stdstring(fp);
                room->extra_descr.emplace_back(ExtraDescription{keyword, description});
            } else {
                bug("Load_rooms: vnum {} has flag not 'DES'.", vnum);
                exit(1);
            }
        }

        iHash = vnum % MAX_KEY_HASH;
        room->next = room_hash[iHash];
        room_hash[iHash] = room;
        top_room++;
    }
}

/* Snarf a shop section. */
void load_shops(FILE *fp) {
    Shop *shop;

    for (;;) {
        MobIndexData *pMobIndex;
        uint iTrade;
        auto shopkeeper_vnum = fread_number(fp);
        if (shopkeeper_vnum == 0)
            break;
        shop = static_cast<Shop *>(alloc_perm(sizeof(*shop)));
        shop->keeper = shopkeeper_vnum;
        for (iTrade = 0; iTrade < MaxTrade; iTrade++)
            shop->buy_type[iTrade] = fread_number(fp);
        shop->profit_buy = fread_number(fp);
        shop->profit_sell = fread_number(fp);
        shop->open_hour = fread_number(fp);
        shop->close_hour = fread_number(fp);
        fread_to_eol(fp);
        pMobIndex = get_mob_index(shop->keeper);
        pMobIndex->shop = shop;

        if (shop_first == nullptr)
            shop_first = shop;
        if (shop_last != nullptr)
            shop_last->next = shop;

        shop_last = shop;
        shop->next = nullptr;
        top_shop++;
    }
}

/* Snarf spec proc declarations. */
void load_specials(FILE *fp) {
    for (;;) {
        MobIndexData *pMobIndex;
        char letter;

        switch (letter = fread_letter(fp)) {
        default: bug("Load_specials: letter '{}' not *, M or S.", letter); exit(1);

        case 'S': return;

        case '*': break;

        case 'M':
            pMobIndex = get_mob_index(fread_number(fp));
            pMobIndex->spec_fun = spec_lookup(fread_word(fp));
            if (pMobIndex->spec_fun == 0) {
                bug("Load_specials: 'M': vnum {}.", pMobIndex->vnum);
                exit(1);
            }
            break;
        }

        fread_to_eol(fp);
    }
}

namespace {

/* Sets vnum range for area when loading its constituent mobs/objects/rooms */
void assign_area_vnum(int vnum) {
    auto area_last = AreaList::singleton().back();
    if (area_last->lvnum == 0 || area_last->uvnum == 0)
        area_last->lvnum = area_last->uvnum = vnum;
    if (vnum != urange(area_last->lvnum, vnum, area_last->uvnum)) {
        if (vnum < area_last->lvnum)
            area_last->lvnum = vnum;
        else
            area_last->uvnum = vnum;
    }
}

}

/*
 * Snarf a mob section.  new style
 */
void load_mobiles(FILE *fp) {
    auto area_last = AreaList::singleton().back();
    if (!area_last) {
        bug("Load_mobiles: no #AREA seen yet!");
        exit(1);
    }
    for (;;) {
        auto maybe_mob = MobIndexData::from_file(fp);
        if (!maybe_mob)
            break;

        assign_area_vnum(maybe_mob->vnum);
        add_mob_index(std::move(*maybe_mob));
    }
}

/*
 * Snarf an obj section. new style
 */
void load_objects(FILE *fp) {
    ObjectIndex *objIndex;
    char temp; /* Used for Death's Wear Strings bit */

    auto area_last = AreaList::singleton().back();
    if (!area_last) {
        bug("Load_objects: no #AREA section found yet!");
        exit(1);
    }

    for (;;) {
        sh_int vnum;
        char letter;
        int iHash;

        letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_objects: # not found.");
            exit(1);
        }

        vnum = fread_number(fp);
        if (vnum == 0)
            break;

        fBootDb = false;
        if (get_obj_index(vnum) != nullptr) {
            bug("Load_objects: vnum {} duplicated.", vnum);
            exit(1);
        }
        fBootDb = true;

        objIndex = new ObjectIndex;
        objIndex->vnum = vnum;
        objIndex->area = area_last;
        objIndex->reset_num = 0;
        newobjs++;
        objIndex->name = fread_stdstring(fp);
        /*
         * MG added - snarf short descrips to kill:
         * You hit The beastly fido
         */
        objIndex->short_descr = lower_case_articles(fread_stdstring(fp));
        objIndex->description = fread_stdstring(fp);
        if (objIndex->description.empty()) {
            bug("Load_objects: empty long description in object {}.", vnum);
        }
        objIndex->material = material_lookup(fread_string(fp));

        objIndex->item_type = item_lookup(fread_word(fp));

        objIndex->extra_flags = fread_flag(fp);

        if (IS_OBJ_STAT(objIndex, ITEM_NOREMOVE) && objIndex->item_type != ITEM_WEAPON) {
            bug("Only weapons are meant to have ITEM_NOREMOVE: {} {}", objIndex->vnum, objIndex->name);
            exit(1);
        }

        objIndex->wear_flags = fread_flag(fp);

        temp = fread_letter(fp);
        if (temp == ',') {
            objIndex->wear_string = fread_stdstring(fp);
        } else {
            ungetc(temp, fp);
        }

        switch (objIndex->item_type) {
        case ITEM_WEAPON:
            objIndex->value[0] = weapon_type(fread_word(fp));
            objIndex->value[1] = fread_number(fp);
            objIndex->value[2] = fread_number(fp);
            objIndex->value[3] = attack_lookup(fread_word(fp));
            objIndex->value[4] = fread_flag(fp);
            break;
        case ITEM_CONTAINER:
            objIndex->value[0] = fread_number(fp);
            objIndex->value[1] = fread_flag(fp);
            objIndex->value[2] = fread_number(fp);
            objIndex->value[3] = fread_number(fp);
            objIndex->value[4] = fread_number(fp);
            break;
        case ITEM_DRINK_CON:
        case ITEM_FOUNTAIN:
            objIndex->value[0] = fread_number(fp);
            objIndex->value[1] = fread_number(fp);
            objIndex->value[2] = liq_lookup(fread_word(fp));
            objIndex->value[3] = fread_number(fp);
            objIndex->value[4] = fread_number(fp);
            break;
        case ITEM_WAND:
        case ITEM_STAFF:
            objIndex->value[0] = fread_number(fp);
            objIndex->value[1] = fread_number(fp);
            objIndex->value[2] = fread_number(fp);
            objIndex->value[3] = fread_spnumber(fp);
            objIndex->value[4] = fread_number(fp);
            break;
        case ITEM_POTION:
        case ITEM_PILL:
        case ITEM_SCROLL:
        case ITEM_BOMB:
            objIndex->value[0] = fread_number(fp);
            objIndex->value[1] = fread_spnumber(fp);
            objIndex->value[2] = fread_spnumber(fp);
            objIndex->value[3] = fread_spnumber(fp);
            objIndex->value[4] = fread_spnumber(fp);
            break;
        default:
            objIndex->value[0] = fread_flag(fp);
            objIndex->value[1] = fread_flag(fp);
            objIndex->value[2] = fread_flag(fp);
            objIndex->value[3] = fread_flag(fp);
            objIndex->value[4] = fread_flag(fp);
            break;
        }

        objIndex->level = fread_number(fp);
        objIndex->weight = fread_number(fp);
        objIndex->cost = fread_number(fp);

        /* condition */
        letter = fread_letter(fp);
        switch (letter) {
        case ('P'): objIndex->condition = 100; break;
        case ('G'): objIndex->condition = 90; break;
        case ('A'): objIndex->condition = 75; break;
        case ('W'): objIndex->condition = 50; break;
        case ('D'): objIndex->condition = 25; break;
        case ('B'): objIndex->condition = 10; break;
        case ('R'): objIndex->condition = 0; break;
        default: objIndex->condition = 100; break;
        }

        for (;;) {
            char letter;

            letter = fread_letter(fp);

            if (letter == 'A') {
                AFFECT_DATA af;
                af.type = -1;
                af.level = objIndex->level;
                af.duration = -1;
                af.location = static_cast<AffectLocation>(fread_number(fp));
                af.modifier = fread_number(fp);
                objIndex->affected.add(af);
                top_obj_affect++;
            }

            else if (letter == 'E') {
                auto keyword = fread_stdstring(fp);
                auto description = fread_stdstring(fp);
                objIndex->extra_descr.emplace_back(ExtraDescription{keyword, description});
            }

            else {
                ungetc(letter, fp);
                break;
            }
        }

        /*
         * Translate spell "slot numbers" to internal "skill numbers."
         */
        switch (objIndex->item_type) {
        case ITEM_BOMB:
            objIndex->value[4] = slot_lookup(objIndex->value[4]);
            // fall through
        case ITEM_PILL:
        case ITEM_POTION:
        case ITEM_SCROLL:
            objIndex->value[1] = slot_lookup(objIndex->value[1]);
            objIndex->value[2] = slot_lookup(objIndex->value[2]);
            objIndex->value[3] = slot_lookup(objIndex->value[3]);
            break;

        case ITEM_STAFF:
        case ITEM_WAND: objIndex->value[3] = slot_lookup(objIndex->value[3]); break;
        }

        iHash = vnum % MAX_KEY_HASH;
        objIndex->next = obj_index_hash[iHash];
        obj_index_hash[iHash] = objIndex;
        top_obj_index++;
        assign_area_vnum(vnum);
    }
}

/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits() {
    Room *room;
    Room *to_room;
    Exit *pexit;
    Exit *pexit_rev;
    int iHash;

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++) {
        for (room = room_hash[iHash]; room != nullptr; room = room->next) {
            bool fexit;

            fexit = false;
            for (auto door : all_directions) {
                if ((pexit = room->exit[door]) != nullptr) {
                    if (pexit->u1.vnum <= 0 || get_room(pexit->u1.vnum) == nullptr)
                        pexit->u1.to_room = nullptr;
                    else {
                        fexit = true;
                        pexit->u1.to_room = get_room(pexit->u1.vnum);
                    }
                }
            }
            if (!fexit)
                set_bit(room->room_flags, ROOM_NO_MOB);
        }
    }

    for (iHash = 0; iHash < MAX_KEY_HASH; iHash++) {
        for (room = room_hash[iHash]; room != nullptr; room = room->next) {
            for (auto door : all_directions) {
                if ((pexit = room->exit[door]) != nullptr && (to_room = pexit->u1.to_room) != nullptr
                    && (pexit_rev = to_room->exit[reverse(door)]) != nullptr && pexit_rev->u1.to_room != room
                    && !pexit->is_one_way) {
                    bug("Fix_exits: {} -> {}:{} -> {}.", room->vnum, static_cast<int>(door), to_room->vnum,
                        static_cast<int>(reverse(door)),
                        (pexit_rev->u1.to_room == nullptr) ? 0 : pexit_rev->u1.to_room->vnum);
                }
            }
        }
    }
}

/* Repopulate areas periodically. An area ticks every minute (Constants.hpp PULSE_AREA).
 * Regular areas reset every:
 * - ~15 minutes if occupied by players.
 * - ~10 minutes if unoccupied by players.
 * Mud School runs an accelerated schedule so that newbies don't get bored and confused:
 * - Every 2 minutes if occupied by players.
 * - Every minute if unoccupied by players.
 *
 * "pArea->empty" has subtley different meaning to "pArea->nplayer == 0". "empty" is set false
 * whenever a player enters any room in the area (see char_to_room()).
 * nplayer is decremented whenever a player leaves a room within the area, but the empty flag is only set true
 * if the area has aged sufficiently to be reset _and_ if there are still no players in the area.
 * This means that players can't force an area to reset more quickly simply by stepping out of it!
 *
 * Due to the special case code below, Mud School _never_ gets flagged as empty.
 */
void area_update() {
    for (auto &pArea : AreaList::singleton()) {
        ++pArea->age;
        if ((!pArea->empty && pArea->age >= RoomResetAgeOccupiedArea)
            || (pArea->empty && pArea->age >= RoomResetAgeUnoccupiedArea)) {
            Room *room;
            reset_area(pArea.get());
            pArea->age = number_range(0, 3);
            room = get_room(rooms::MudschoolEntrance);
            if (room != nullptr && pArea.get() == room->area)
                pArea->age = RoomResetAgeUnoccupiedArea;
            else if (pArea->nplayer == 0)
                pArea->empty = true;
        }
    }
}

/*
 * Reset one room.  Called by reset_area.
 */
void reset_room(Room *room) {
    ResetData *reset;
    Char *lastMob = nullptr;
    bool lastMobWasReset = false;
    if (!room)
        return;

    for (auto exit_dir : all_directions) {
        Exit *exit;
        if ((exit = room->exit[exit_dir])) {
            exit->exit_info = exit->rs_flags;
            if ((exit->u1.to_room != nullptr) && ((exit = exit->u1.to_room->exit[reverse(exit_dir)]))) {
                /* nail the other side */
                exit->exit_info = exit->rs_flags;
            }
        }
    }

    for (reset = room->reset_first; reset != nullptr; reset = reset->next) {
        switch (reset->command) {
        default: bug("Reset_room: bad command {}.", reset->command); break;

        case ResetMobInRoom: {
            MobIndexData *mobIndex;
            if (!(mobIndex = get_mob_index(reset->arg1))) {
                bug("Reset_room: 'M': bad vnum {}.", reset->arg1);
                continue;
            }
            if (mobIndex->count >= reset->arg2) {
                lastMobWasReset = false;
                continue;
            }
            int count = 0;
            for (auto *mch : room->people) {
                if (mch->pIndexData == mobIndex) {
                    count++;
                    if (count >= reset->arg4) {
                        lastMobWasReset = false;
                        break;
                    }
                }
            }

            if (count >= reset->arg4)
                continue;

            auto *mob = create_mobile(mobIndex);

            // Pet shop mobiles get ACT_PET set.
            Room *previousRoom;
            previousRoom = get_room(room->vnum - 1);
            if (previousRoom && check_bit(previousRoom->room_flags, ROOM_PET_SHOP))
                set_bit(mob->act, ACT_PET);

            char_to_room(mob, room);
            lastMob = mob;
            lastMobWasReset = true;
            break;
        }

        case ResetObjInRoom: {
            ObjectIndex *objIndex;
            Room *room;
            if (!(objIndex = get_obj_index(reset->arg1))) {
                bug("Reset_room: 'O': bad vnum {}.", reset->arg1);
                continue;
            }

            if (!(room = get_room(reset->arg3))) {
                bug("Reset_room: 'O': bad vnum {}.", reset->arg3);
                continue;
            }

            if (room->area->nplayer > 0 || count_obj_list(objIndex, room->contents) > 0) {
                continue;
            }

            auto object = create_object(objIndex);
            object->cost = 0;
            obj_to_room(object, room);
            break;
        }

        case ResetPutObjInObj: {
            int limit, count;
            ObjectIndex *containedObjIndex;
            ObjectIndex *containerObjIndex;
            Object *containerObj;
            if (!(containedObjIndex = get_obj_index(reset->arg1))) {
                bug("Reset_room: 'P': bad vnum {}.", reset->arg1);
                continue;
            }

            if (!(containerObjIndex = get_obj_index(reset->arg3))) {
                bug("Reset_room: 'P': bad vnum {}.", reset->arg3);
                continue;
            }

            if (reset->arg2 > 20) /* old format reduced from 50! */
                limit = 6;
            else if (reset->arg2 == -1) /* no limit */
                limit = 999;
            else
                limit = reset->arg2;

            // Don't create the contained object if:
            // The area has players right now, or if the containing object doesn't exist in the world,
            // or if already too many instances of contained object (and without 80% chance of "over spawning"),
            // or if count of contained object within the container's current room exceeds the item-in-room limit.
            if (room->area->nplayer > 0 || (containerObj = get_obj_type(containerObjIndex)) == nullptr
                || (containedObjIndex->count >= limit && number_range(0, 4) != 0)
                || (count = count_obj_list(containedObjIndex, containerObj->contains)) > reset->arg4) {
                continue;
            }

            while (count < reset->arg4) {
                auto object = create_object(containedObjIndex);
                obj_to_obj(object, containerObj);
                count++;
                if (containedObjIndex->count >= limit)
                    break;
            }

            // Close the container if required.
            if (containerObj->item_type == ITEM_CONTAINER) {
                containerObj->value[1] = containerObj->objIndex->value[1];
            }
            break;
        }

        case ResetGiveObjMob:
        case ResetEquipObjMob: {
            ObjectIndex *objIndex;
            Object *object;
            if (!(objIndex = get_obj_index(reset->arg1))) {
                bug("Reset_room: 'E' or 'G': bad vnum {}.", reset->arg1);
                continue;
            }

            if (!lastMobWasReset)
                continue;

            if (!lastMob) {
                bug("Reset_room: 'E' or 'G': null mob for vnum {}.", reset->arg1);
                lastMobWasReset = false;
                continue;
            }

            if (lastMob->pIndexData->shop) { /* Shop-keeper? */
                object = create_object(objIndex);
                set_bit(object->extra_flags, ITEM_INVENTORY);
            } else {
                const auto drop_rate = reset->arg2;
                if (drop_rate <= 0 || drop_rate > 100) {
                    bug("Invalid object drop rate: {} for object #{}", drop_rate, reset->arg1);
                    exit(1);
                }
                if (number_percent() <= drop_rate) {
                    object = create_object(objIndex);
                } else
                    continue;
            }

            obj_to_char(object, lastMob);
            if (reset->command == ResetEquipObjMob)
                equip_char(lastMob, object, reset->arg3);
            lastMobWasReset = true;
            break;
        }

        case ResetExitFlags: break;

        case ResetRandomizeExits: {
            Room *room;
            if (!(room = get_room(reset->arg1))) {
                bug("Reset_room: 'R': bad vnum {}.", reset->arg1);
                continue;
            }

            for (int d0 = 0; d0 < reset->arg2 - 1; d0++) {
                auto door0 = try_cast_direction(d0).value();
                auto door1 = try_cast_direction(number_range(d0, reset->arg2 - 1)).value();
                std::swap(room->exit[door0], room->exit[door1]);
            }
            break;
        }
        }
    }
}

/*
 * Reset one area.
 */
void reset_area(AREA_DATA *pArea) {
    Room *room;
    int vnum;
    for (vnum = pArea->lvnum; vnum <= pArea->uvnum; vnum++) {
        if ((room = get_room(vnum)))
            reset_room(room);
    }
}

/*
 * Create an instance of a mobile.
 */
Char *create_mobile(MobIndexData *pMobIndex) {
    if (pMobIndex == nullptr) {
        bug("Create_mobile: nullptr pMobIndex.");
        exit(1);
    }

    auto *mob = new Char;
    mob->pIndexData = pMobIndex;

    mob->name = pMobIndex->player_name;
    mob->short_descr = pMobIndex->short_descr;
    mob->long_descr = pMobIndex->long_descr;
    mob->description = pMobIndex->description;
    mob->spec_fun = pMobIndex->spec_fun;

    /* read from prototype */
    mob->act = pMobIndex->act;
    mob->comm = COMM_NOCHANNELS | COMM_NOSHOUT | COMM_NOTELL;
    mob->affected_by = pMobIndex->affected_by;
    mob->alignment = pMobIndex->alignment;
    mob->level = pMobIndex->level;
    mob->hitroll = pMobIndex->hitroll;
    mob->max_hit = pMobIndex->hit.roll();
    mob->hit = mob->max_hit;
    mob->max_mana = pMobIndex->mana.roll();
    mob->mana = mob->max_mana;
    mob->damage = pMobIndex->damage;
    mob->damage.bonus(0); // clear the bonus; it's accounted for in the damroll
    mob->damroll = pMobIndex->damage.bonus();
    mob->dam_type = pMobIndex->dam_type;
    for (int i = 0; i < 4; i++)
        mob->armor[i] = pMobIndex->ac[i];
    mob->off_flags = pMobIndex->off_flags;
    mob->imm_flags = pMobIndex->imm_flags;
    mob->res_flags = pMobIndex->res_flags;
    mob->vuln_flags = pMobIndex->vuln_flags;
    mob->start_pos = pMobIndex->start_pos;
    mob->default_pos = pMobIndex->default_pos;
    mob->sex = pMobIndex->sex;
    mob->race = pMobIndex->race;
    if (pMobIndex->gold == 0)
        mob->gold = 0;
    else
        mob->gold = number_range(pMobIndex->gold / 2, pMobIndex->gold * 3 / 2);
    mob->form = pMobIndex->form;
    mob->parts = pMobIndex->parts;
    mob->size = pMobIndex->size;
    mob->material = pMobIndex->material;

    /* computed on the spot */

    ranges::fill(mob->perm_stat, std::min(25, 11 + mob->level / 4));

    if (check_bit(mob->act, ACT_WARRIOR)) {
        mob->perm_stat[Stat::Str] += 3;
        mob->perm_stat[Stat::Int] -= 1;
        mob->perm_stat[Stat::Con] += 2;
    }

    if (check_bit(mob->act, ACT_THIEF)) {
        mob->perm_stat[Stat::Dex] += 3;
        mob->perm_stat[Stat::Int] += 1;
        mob->perm_stat[Stat::Wis] -= 1;
    }

    if (check_bit(mob->act, ACT_CLERIC)) {
        mob->perm_stat[Stat::Wis] += 3;
        mob->perm_stat[Stat::Dex] -= 1;
        mob->perm_stat[Stat::Str] += 1;
    }

    if (check_bit(mob->act, ACT_MAGE)) {
        mob->perm_stat[Stat::Int] += 3;
        mob->perm_stat[Stat::Str] -= 1;
        mob->perm_stat[Stat::Dex] += 1;
    }

    if (check_bit(mob->off_flags, OFF_FAST))
        mob->perm_stat[Stat::Dex] += 2;

    mob->perm_stat[Stat::Str] += mob->size - SIZE_MEDIUM;
    mob->perm_stat[Stat::Con] += (mob->size - SIZE_MEDIUM) / 2;

    mob->position = mob->start_pos;

    /* link the mob to the world list */
    char_list.add_front(mob);
    pMobIndex->count++;
    return mob;
}

/* duplicate a mobile exactly -- except inventory */
void clone_mobile(Char *parent, Char *clone) {
    if (parent == nullptr || clone == nullptr || parent->is_pc())
        return;

    /* start fixing values */
    clone->name = parent->name;
    clone->version = parent->version;
    clone->short_descr = parent->short_descr;
    clone->long_descr = parent->long_descr;
    clone->description = parent->description;
    clone->sex = parent->sex;
    clone->class_num = parent->class_num;
    clone->race = parent->race;
    clone->level = parent->level;
    clone->trust = 0;
    clone->timer = parent->timer;
    clone->wait = parent->wait;
    clone->hit = parent->hit;
    clone->max_hit = parent->max_hit;
    clone->mana = parent->mana;
    clone->max_mana = parent->max_mana;
    clone->move = parent->move;
    clone->max_move = parent->max_move;
    clone->gold = parent->gold;
    clone->exp = parent->exp;
    clone->act = parent->act;
    clone->comm = parent->comm;
    clone->imm_flags = parent->imm_flags;
    clone->res_flags = parent->res_flags;
    clone->vuln_flags = parent->vuln_flags;
    clone->invis_level = parent->invis_level;
    clone->affected_by = parent->affected_by;
    clone->position = parent->position;
    clone->practice = parent->practice;
    clone->train = parent->train;
    clone->saving_throw = parent->saving_throw;
    clone->alignment = parent->alignment;
    clone->hitroll = parent->hitroll;
    clone->damroll = parent->damroll;
    clone->damage = parent->damage;
    clone->wimpy = parent->wimpy;
    clone->form = parent->form;
    clone->parts = parent->parts;
    clone->size = parent->size;
    clone->material = parent->material;
    clone->off_flags = parent->off_flags;
    clone->dam_type = parent->dam_type;
    clone->start_pos = parent->start_pos;
    clone->default_pos = parent->default_pos;
    clone->spec_fun = parent->spec_fun;

    for (int i = 0; i < 4; i++)
        clone->armor[i] = parent->armor[i];

    clone->perm_stat = parent->perm_stat;
    clone->mod_stat = parent->mod_stat;

    /* now add the affects */
    for (const auto &af : parent->affected)
        affect_to_char(clone, af);
}

/*
 * Create an instance of an object.
 * TheMoog 1/10/2k : fixes up portal objects - value[0] of a portal
 * if non-zero is looked up and then destination set accordingly.
 */
Object *create_object(ObjectIndex *objIndex) {
    Object *obj;

    if (objIndex == nullptr) {
        bug("Create_object: nullptr objIndex.");
        exit(1);
    }

    obj = new Object; // TODO! make an actual constructor for this!
    obj->objIndex = objIndex;
    obj->in_room = nullptr;
    obj->enchanted = false;
    obj->level = objIndex->level;
    obj->wear_loc = -1;

    obj->name = objIndex->name;
    obj->short_descr = objIndex->short_descr;
    obj->description = objIndex->description;
    obj->material = objIndex->material;
    obj->item_type = objIndex->item_type;
    obj->extra_flags = objIndex->extra_flags;
    obj->wear_flags = objIndex->wear_flags;
    obj->wear_string = objIndex->wear_string;
    obj->value[0] = objIndex->value[0];
    obj->value[1] = objIndex->value[1];
    obj->value[2] = objIndex->value[2];
    obj->value[3] = objIndex->value[3];
    obj->value[4] = objIndex->value[4];
    obj->weight = objIndex->weight;
    obj->cost = objIndex->cost;

    /*
     * Mess with object properties.
     */
    switch (obj->item_type) {
    default: bug("Read_object: vnum {} bad type.", objIndex->vnum); break;

    case ITEM_LIGHT:
        if (obj->value[2] == 999)
            obj->value[2] = -1;
        break;
    case ITEM_TREASURE:
    case ITEM_FURNITURE:
    case ITEM_TRASH:
    case ITEM_CONTAINER:
    case ITEM_DRINK_CON:
    case ITEM_KEY:
    case ITEM_FOOD:
    case ITEM_BOAT:
    case ITEM_CORPSE_NPC:
    case ITEM_CORPSE_PC:
    case ITEM_FOUNTAIN:
    case ITEM_MAP:
    case ITEM_CLOTHING:
    case ITEM_BOMB: break;

    case ITEM_PORTAL:
        if (obj->value[0] != 0) {
            obj->destination = get_room(obj->value[0]);
            if (!obj->destination)
                bug("Couldn't find room index {} for a portal (vnum {})", obj->value[0], objIndex->vnum);
            obj->value[0] = 0; // Prevents ppl ever finding the vnum in the obj
        }
        break;

    case ITEM_SCROLL:
    case ITEM_WAND:
    case ITEM_STAFF:
    case ITEM_WEAPON:
    case ITEM_ARMOR:
    case ITEM_POTION:
    case ITEM_PILL:
    case ITEM_MONEY: break;
    }

    object_list.add_front(obj);
    objIndex->count++;

    return obj;
}

/* duplicate an object exactly -- except contents */
void clone_object(Object *parent, Object *clone) {
    if (parent == nullptr || clone == nullptr)
        return;

    /* start fixing the object */
    clone->name = parent->name;
    clone->short_descr = parent->short_descr;
    clone->description = parent->description;
    clone->item_type = parent->item_type;
    clone->extra_flags = parent->extra_flags;
    clone->wear_flags = parent->wear_flags;
    clone->weight = parent->weight;
    clone->cost = parent->cost;
    clone->level = parent->level;
    clone->condition = parent->condition;
    clone->material = parent->material;
    clone->timer = parent->timer;

    for (int i = 0; i < 5; i++)
        clone->value[i] = parent->value[i];

    /* affects */
    clone->enchanted = parent->enchanted;

    for (auto &af : parent->affected)
        affect_to_obj(clone, af);

    /* extended desc */
    clone->extra_descr = parent->extra_descr;
}

/*
 * Get an extra description from a list.
 */
const char *get_extra_descr(std::string_view name, const std::vector<ExtraDescription> &ed) {
    if (auto it = ranges::find_if(ed, [&name](const auto &ed) { return is_name(name, ed.keyword); }); it != ed.end())
        return it->description.c_str();
    return nullptr;
}

// Translates mob virtual number to its mob index struct.
MobIndexData *get_mob_index(int vnum) {
    if (auto it = mob_indexes.find(vnum); it != mob_indexes.end())
        return &it->second;
    if (fBootDb) {
        bug("Get_mob_index: bad vnum {}.", vnum);
        exit(1);
    }
    return nullptr;
}

// Adds a new mob.
void add_mob_index(MobIndexData mob_index_data) {
    auto vnum = mob_index_data.vnum;
    if (!mob_indexes.try_emplace(vnum, std::move(mob_index_data)).second) {
        bug("Load_mobiles: vnum {} duplicated.", vnum);
        exit(1);
    }
}

const std::map<int, MobIndexData> &all_mob_index_pairs() { return mob_indexes; }

/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
ObjectIndex *get_obj_index(int vnum) {
    ObjectIndex *objIndex;

    for (objIndex = obj_index_hash[vnum % MAX_KEY_HASH]; objIndex != nullptr; objIndex = objIndex->next) {
        if (objIndex->vnum == vnum)
            return objIndex;
    }

    if (fBootDb) {
        bug("Get_obj_index: bad vnum {}.", vnum);
        exit(1);
    }

    return nullptr;
}

/*
 * Translates mob virtual number to its Room.
 * Hash table lookup.
 */
Room *get_room(int vnum) {
    Room *room;

    for (room = room_hash[vnum % MAX_KEY_HASH]; room != nullptr; room = room->next) {
        if (room->vnum == vnum)
            return room;
    }

    if (fBootDb) {
        bug("get_room: bad vnum {}.", vnum);
        exit(1);
    }

    return nullptr;
}

/*
 * Read a letter from a file.
 */
char fread_letter(FILE *fp) {
    char c;

    do {
        c = getc(fp);
    } while (isspace(c));

    return c;
}

/*
 * Read a number from a file.
 */
int fread_number(FILE *fp) {
    int number;
    bool sign;
    char c;

    do {
        c = getc(fp);
    } while (isspace(c));

    number = 0;

    sign = false;
    if (c == '+') {
        c = getc(fp);
    } else if (c == '-') {
        sign = true;
        c = getc(fp);
    }

    if (!isdigit(c)) {
        bug("Fread_number: bad format.");
        exit(1);
    }

    while (isdigit(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (sign)
        number = 0 - number;

    if (c == '|')
        number += fread_number(fp);
    else if (c != ' ')
        ungetc(c, fp);

    return number;
}
/*
 * Read a number from a file.
 */
int fread_spnumber(FILE *fp) {
    int number;
    bool sign;
    char c;
    char *spell_name;
    char long_spell_name[64];

    do {
        c = getc(fp);
    } while (isspace(c));

    number = 0;

    sign = false;
    if (c == '+') {
        c = getc(fp);
    } else if (c == '-') {
        sign = true;
        c = getc(fp);
    }

    if (!isdigit(c)) {
        if (c == '\'' || c == '"') {
            char endquote = c;
            /* MG> Replaced the elided C with something which works reliably! */
            for (spell_name = long_spell_name; (c = getc(fp)) != endquote; ++spell_name)
                *spell_name = c;
            *spell_name = '\0';
            spell_name = long_spell_name;
        } else {
            ungetc(c, fp);
            spell_name = fread_string(fp);
        }
        if ((number = skill_lookup(spell_name)) <= 0) {
            bug("Fread_spnumber: bad spell.");
            exit(1);
        }
        return skill_table[number].slot;
    }

    while (isdigit(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (sign)
        number = 0 - number;

    if (c == '|')
        number += fread_number(fp);
    else if ((c != ' ') && (c != '~'))
        ungetc(c, fp);

    return number;
}

// Note: not all flags are possible here - we return a 'long' (32 bits), but
// allow to decode up to 52 bits ('z'). This makes no sense.
long flag_convert(char letter) {
    if ('A' <= letter && letter <= 'Z') {
        return 1 << int(letter - 'A');
    } else if ('a' <= letter && letter <= 'z') {
        return 1 << (26 + int(letter - 'a'));
    }
    bug("illegal char '{}' in flag_convert", letter);
    return 0;
}

long fread_flag(FILE *fp) {
    char c;

    do {
        c = getc(fp);
    } while (isspace(c));

    int number = 0;

    if (!isdigit(c)) {
        while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z')) {
            const char lastc = c;
            number += flag_convert(c);
            do {
                c = getc(fp);
            } while (c == lastc); /* bug fix for Ozydoc crap by TheMoog */
        }
    }

    while (isdigit(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }

    if (c == '|') {
        number += fread_flag(fp);
    } else {
        if (c != ' ')
            ungetc(c, fp);
    }
    return number;
}

/* Reads a ~-terminated string from a file into a new buffer. */
BUFFER *fread_string_tobuffer(FILE *fp) {
    char buf[MAX_STRING_LENGTH];
    char c;
    int index = 0;
    BUFFER *buffer = buffer_create();

    if (buffer == nullptr) {
        bug("fread_string_tobuffer: Failed to create new buffer.");
        return nullptr;
    }
    do {
        c = getc(fp);
    } while (isspace(c));
    ungetc(c, fp);
    while (index < MAX_STRING_LENGTH - 2) {
        switch (buf[index] = getc(fp)) {
        default: index++; break;

        case EOF:
            bug("fread_string_tobuffer: EOF found.");
            buffer_shrink(buffer);
            return buffer;

        case '\r': break;

        case '\n':
            buf[index + 1] = '\r';
            buf[index + 2] = '\0';
            buffer_addline(buffer, buf);
            index = 0;
            break;

        case '~':
            buf[index] = '\0';
            buffer_addline(buffer, buf);
            buffer_shrink(buffer);
            return buffer;
        }
    }
    bug("fread_string_tobuffer: String overflow - aborting read.");
    buffer_shrink(buffer);
    return buffer;
}

void skip_ws(FILE *fp) {
    for (;;) {
        auto c = getc(fp);
        if (!isspace(c)) {
            ungetc(c, fp);
            break;
        }
    }
}

std::string fread_stdstring(FILE *fp) {
    skip_ws(fp);
    std::string result;
    for (;;) {
        auto c = fgetc(fp);
        switch (c) {
        default: result += static_cast<char>(c); break;
        case EOF: bug("fread_stdstring: EOF"); return result;

        case '\n': result += "\n\r"; break;
        case '\r': break;

        case '~': return result;
        }
    }
}

std::string fread_stdstring_eol(FILE *fp) {
    skip_ws(fp);
    std::string result;
    for (;;) {
        auto c = fgetc(fp);
        switch (c) {
        default: result += static_cast<char>(c); break;
        case EOF: bug("fread_stdstring_eol: EOF"); return result;

        case '\n': return result;
        }
    }
}

namespace {
/*
 * Read and allocate space for a string from a file.
 * These strings are read-only and shared.
 * Strings are hashed:
 *   each string prepended with hash pointer to prev string,
 *   hash code is simply the string length.
 *   this function takes 40% to 50% of boot-up time.
 */
char *do_horrible_boot_strdup_thing(const std::string &str) {
    char *plast = top_string + sizeof(char *);
    if (plast > &string_space[MAX_STRING - MAX_STRING_LENGTH]) {
        bug("Fread_string: MAX_STRING {} exceeded.", MAX_STRING);
        exit(1);
    }
    union {
        char *pc;
        char rgc[sizeof(char *)];
    } u1;
    int ic;
    int iHash;
    char *pHash;
    char *pHashPrev;
    char *pString;

    strcpy(plast, str.c_str());
    plast += str.size() + 1;
    const int diff = plast - 1 - top_string;
    iHash = std::min(MAX_KEY_HASH - 1, diff);
    for (pHash = string_hash[iHash]; pHash; pHash = pHashPrev) {
        for (ic = 0; ic < (int)sizeof(char *); ic++)
            u1.rgc[ic] = pHash[ic];
        pHashPrev = u1.pc;
        pHash += sizeof(char *);

        if (top_string[sizeof(char *)] == pHash[0] && !strcmp(top_string + sizeof(char *) + 1, pHash + 1))
            return pHash;
    }

    if (fBootDb) {
        pString = top_string;
        top_string = plast;
        u1.pc = string_hash[iHash];
        for (ic = 0; ic < (int)sizeof(char *); ic++)
            pString[ic] = u1.rgc[ic];
        string_hash[iHash] = pString;

        nAllocString += 1;
        sAllocString += top_string - pString;
        return pString + sizeof(char *);
    } else {
        return str_dup(top_string + sizeof(char *));
    }
}

}

char *fread_string(FILE *fp) { return do_horrible_boot_strdup_thing(fread_stdstring(fp)); }

char *fread_string_eol(FILE *fp) { return do_horrible_boot_strdup_thing(fread_stdstring_eol(fp)); }

/*
 * Read to end of line (for comments).
 */
void fread_to_eol(FILE *fp) {
    int c;

    do {
        c = getc(fp);
    } while (c != '\n' && c != '\r');

    do {
        c = getc(fp);
    } while (c == '\n' || c == '\r');

    ungetc(c, fp);
}

/*
 * Read one word (into static buffer).
 */
char *fread_word(FILE *fp) {
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    char cEnd;

    do {
        cEnd = getc(fp);
    } while (isspace(cEnd));

    if (cEnd == '\'' || cEnd == '"') {
        pword = word;
    } else {
        word[0] = cEnd;
        pword = word + 1;
        cEnd = ' ';
    }

    for (; pword < word + MAX_INPUT_LENGTH; pword++) {
        *pword = getc(fp);
        if (cEnd == ' ' ? isspace(*pword) : *pword == cEnd) {
            if (cEnd == ' ')
                ungetc(*pword, fp);
            *pword = '\0';
            return word;
        }
    }

    bug("Fread_word: word too long.");
    exit(1);
    return nullptr;
}

/*
 * Allocate some ordinary memory,
 *   with the expectation of freeing it someday.
 */
void *alloc_mem(int sMem) {
    void *pMem;
    int iList;

    for (iList = 0; iList < MAX_MEM_LIST; iList++) {
        if (sMem <= rgSizeList[iList])
            break;
    }

    if (iList == MAX_MEM_LIST) {
        bug("Alloc_mem: size {} too large.", sMem);
        exit(1);
    }

    if (rgFreeList[iList] == nullptr) {
        pMem = alloc_perm(rgSizeList[iList]);
    } else {
        pMem = rgFreeList[iList];
        rgFreeList[iList] = *((void **)rgFreeList[iList]);
    }

    return pMem;
}

/*
 * Free some memory.
 * Recycle it back onto the free list for blocks of that size.
 */
void free_mem(void *pMem, int sMem) {
    int iList;

    for (iList = 0; iList < MAX_MEM_LIST; iList++) {
        if (sMem <= rgSizeList[iList])
            break;
    }

    if (iList == MAX_MEM_LIST) {
        bug("Free_mem: size {} too large.", sMem);
        exit(1);
    }

    *((void **)pMem) = rgFreeList[iList];
    rgFreeList[iList] = pMem;
}

/*
 * Allocate some permanent memory.
 * Permanent memory is never freed,
 *   pointers into it may be copied safely.
 */
void *alloc_perm(int sMem) {
    static char *pMemPerm;
    static int iMemPerm;
    void *pMem;

    while (sMem % sizeof(long) != 0)
        sMem++;
    if (sMem > MAX_PERM_BLOCK) {
        bug("Alloc_perm: {} too large.", sMem);
        exit(1);
    }

    if (pMemPerm == nullptr || iMemPerm + sMem > MAX_PERM_BLOCK) {
        iMemPerm = 0;
        if ((pMemPerm = static_cast<char *>(calloc(1, MAX_PERM_BLOCK))) == nullptr) {
            perror("Alloc_perm");
            exit(1);
        }
    }

    pMem = pMemPerm + iMemPerm;
    iMemPerm += sMem;
    nAllocPerm += 1;
    sAllocPerm += sMem;
    return pMem;
}

/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char *str_dup(const char *str) {
    char *str_new;

    if (str[0] == '\0')
        return &str_empty[0];

    if (str >= string_space && str < top_string)
        return (char *)str;

    str_new = static_cast<char *>(alloc_mem(strlen(str) + 1));
    strcpy(str_new, str);
    return str_new;
}

/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */
void free_string(char *pstr) {
    if (pstr == nullptr || pstr == &str_empty[0] || (pstr >= string_space && pstr < top_string))
        return;

    free_mem(pstr, strlen(pstr) + 1);
}

// Now takes parameters (TM was 'ere 10/00)
void do_areas(Char *ch, ArgParser args) {
    int minLevel = 0;
    int maxLevel = MAX_LEVEL;
    if (!args.empty()) {
        auto min_level_str = args.shift();
        if (!is_number(min_level_str)) {
            ch->send_line("You must specify a number for the minimum level.");
            return;
        }
        minLevel = parse_number(min_level_str);

        if (!args.empty()) {
            auto max_level_str = args.shift();
            if (!is_number(max_level_str)) {
                ch->send_line("You must specify a number for the maximum level.");
                return;
            }
            maxLevel = parse_number(max_level_str);
        }
    }

    if (minLevel > maxLevel) {
        ch->send_line("The minimum level must be below the maximum level.");
        return;
    }

    const int charLevel = ch->level;
    const AREA_DATA *area_column1{};
    std::string_view colour_column1;
    const AREA_DATA *area_column2{};
    std::string_view colour_column2;
    int num_found = 0;
    for (auto &area : AreaList::singleton()) {
        std::string_view colour = "|w";
        // Is it outside the requested range?
        if (area->min_level > maxLevel || area->max_level < minLevel)
            continue;

        // Work out colour code
        if (area->all_levels) {
            colour = "|C";
        } else if (area->min_level > (charLevel + 3) || area->max_level < (charLevel - 3)) {
            colour = "|w";
        } else {
            colour = "|W";
        }

        if (area_column1 == nullptr) {
            area_column1 = area.get();
            colour_column1 = colour;
            num_found++;
        } else {
            area_column2 = area.get();
            colour_column2 = colour;
            num_found++;
            // And shift out
            ch->send_line("{}{:<39}{}{:<39}", colour_column1, area_column1->name, colour_column2, area_column2->name);
            area_column1 = area_column2 = nullptr;
        }
    }
    // Check for any straggling lines
    if (area_column1)
        ch->send_line("{}{:<39}", colour_column1, area_column1->name);
    if (num_found) {
        ch->send_line("");
        ch->send_line("Areas found: {}", num_found);
    } else {
        ch->send_line("No areas found.");
    }
}

void do_memory(Char *ch) {
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "Affects %5d\n\r", top_obj_affect);
    ch->send_to(buf);
    ch->send_line("Areas   {:5}", AreaList::singleton().count());
    snprintf(buf, sizeof(buf), "Exits   %5d\n\r", top_exit);
    ch->send_to(buf);
    ch->send_line("Helps   {:5}", HelpList::singleton().count());
    snprintf(buf, sizeof(buf), "Socials %5d\n\r", social_count);
    ch->send_to(buf);
    snprintf(buf, sizeof(buf), "Mobs    %5lu\n\r", mob_indexes.size());
    ch->send_to(buf);
    snprintf(buf, sizeof(buf), "(in use)%5d\n\r", Char::num_active());
    ch->send_to(buf);
    snprintf(buf, sizeof(buf), "Objs    %5d(%d new format)\n\r", top_obj_index, newobjs);
    ch->send_to(buf);
    snprintf(buf, sizeof(buf), "Resets  %5d\n\r", top_reset);
    ch->send_to(buf);
    snprintf(buf, sizeof(buf), "Rooms   %5d\n\r", top_room);
    ch->send_to(buf);
    snprintf(buf, sizeof(buf), "Shops   %5d\n\r", top_shop);
    ch->send_to(buf);

    snprintf(buf, sizeof(buf), "Strings %5d strings of %7d bytes (max %d).\n\r", nAllocString, sAllocString,
             MAX_STRING);
    ch->send_to(buf);

    snprintf(buf, sizeof(buf), "Perms   %5d blocks  of %7d bytes.\n\r", nAllocPerm, sAllocPerm);
    ch->send_to(buf);
}

void do_dump(Char *ch) {
    int count, num_pcs, aff_count;
    MobIndexData *pMobIndex;
    PcData *pc;
    ObjectIndex *objIndex;
    Room *room;
    Exit *exit;
    Descriptor *d;
    AFFECT_DATA *af;
    FILE *fp;
    int vnum, nMatch = 0;

    /* open file */
    fp = fopen("mem.dmp", "w");

    /* report use of data structures */

    num_pcs = 0;
    aff_count = 0;

    /* mobile prototypes */
    fprintf(fp, "MobProt	%4lu (%8ld bytes)\n", mob_indexes.size(), mob_indexes.size() * (sizeof(*pMobIndex)));

    /* mobs */
    count = 0;
    for (auto *fch : char_list) {
        count++;
        if (fch->pcdata != nullptr)
            num_pcs++;
        aff_count += fch->affected.size();
    }

    fprintf(fp, "Mobs	%4d (%8ld bytes)\n", count, count * (sizeof(MobIndexData)));

    /* pcdata */
    fprintf(fp, "Pcdata	%4d (%8ld bytes)\n", num_pcs, num_pcs * (sizeof(*pc)));

    /* descriptors */
    count = static_cast<int>(ranges::distance(descriptors().all()));

    fprintf(fp, "Descs	%4d (%8ld bytes)\n", count, count * (sizeof(*d)));

    /* object prototypes */
    for (vnum = 0; nMatch < top_obj_index; vnum++)
        if ((objIndex = get_obj_index(vnum)) != nullptr) {
            aff_count += objIndex->affected.size();
            nMatch++;
        }

    fprintf(fp, "ObjProt	%4d (%8ld bytes)\n", top_obj_index, top_obj_index * (sizeof(*objIndex)));

    /* objects */
    count = 0;
    for (auto *obj : object_list) {
        count++;
        aff_count += obj->affected.size();
    }

    fprintf(fp, "Objs	%4d (%8ld bytes)\n", count, count * (sizeof(Object)));

    /* affects */
    fprintf(fp, "Affects	%4d (%8ld bytes)\n", aff_count, aff_count * (sizeof(*af)));

    /* rooms */
    fprintf(fp, "Rooms	%4d (%8ld bytes)\n", top_room, top_room * (sizeof(*room)));

    /* exits */
    fprintf(fp, "Exits	%4d (%8ld bytes)\n", top_exit, top_exit * (sizeof(*exit)));

    fclose(fp);

    /* start printing out mobile data */
    fp = fopen("mob.dmp", "w");

    fprintf(fp, "\nMobile Analysis\n");
    fprintf(fp, "---------------\n");
    for (const auto &mob : all_mob_indexes())
        fprintf(fp, "#%-4d %3d active %3d killed     %s\n", mob.vnum, mob.count, mob.killed, mob.short_descr.c_str());
    fclose(fp);

    /* start printing out object data */
    fp = fopen("obj.dmp", "w");

    fprintf(fp, "\nObject Analysis\n");
    fprintf(fp, "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_obj_index; vnum++)
        if ((objIndex = get_obj_index(vnum)) != nullptr) {
            nMatch++;
            fmt::print(fp, "#{:<4} {:3} active {:3} reset      {}\n", objIndex->vnum, objIndex->count,
                       objIndex->reset_num, objIndex->short_descr);
        }

    /* close file */
    fclose(fp);

    ch->send_line("Dump complete");
}

KnuthRng knuth_rng(0);

/*
 * Stick a little fuzz on a number.
 */
int number_fuzzy(int number) {
    switch (knuth_rng.number_bits(2)) {
    case 0: number -= 1; break;
    case 3: number += 1; break;
    }

    return std::max(1, number);
}

int number_range(int from, int to) { return knuth_rng.number_range(from, to); }
/*
 * Generate a percentile roll.
 */
int number_percent() { return knuth_rng.number_percent(); }

int number_bits(int width) { return knuth_rng.number_bits(width); }

void init_mm() {
    knuth_rng = KnuthRng((int)Clock::to_time_t(current_time));
    Rng::set_global_rng(knuth_rng);
}

int number_mm() { return static_cast<int>(knuth_rng.number_mm()); }

/*
 * Roll some dice.
 */
int dice(int number, int size) { return knuth_rng.dice(number, size); }

/*
 * Simple linear interpolation.
 */
int interpolate(int level, int value_00, int value_32) { return value_00 + level * (value_32 - value_00) / 32; }

/*
 * Compare strings, case insensitive.
 * Return true if different
 *   (compatibility with historical functions).
 */
bool str_cmp(const char *astr, const char *bstr) {
    if (astr == nullptr) {
        bug("Str_cmp: null astr.");
        return true;
    }

    if (bstr == nullptr) {
        bug("Str_cmp: null bstr.");
        return true;
    }

    for (; *astr || *bstr; astr++, bstr++) {
        if (tolower(*astr) != tolower(*bstr))
            return true;
    }
    return false;
}

/*
 * Compare strings, case insensitive, for prefix matching.
 * Return true if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix(const char *astr, const char *bstr) {
    if (astr == nullptr) {
        bug("Strn_cmp: null astr.");
        return true;
    }

    if (bstr == nullptr) {
        bug("Strn_cmp: null bstr.");
        return true;
    }

    for (; *astr; astr++, bstr++) {
        if (tolower(*astr) != tolower(*bstr))
            return true;
    }

    return false;
}

/*
 * Compare strings, case insensitive, for suffix matching.
 * Return true if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix(const char *astr, const char *bstr) {
    int sstr1;
    int sstr2;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);
    if (sstr1 <= sstr2 && !str_cmp(astr, bstr + sstr2 - sstr1))
        return false;
    else
        return true;
}

/*
 * Append a string to a file.
 */
void append_file(Char *ch, const char *file, const char *str) {
    FILE *fp;

    if (ch->is_npc() || str[0] == '\0')
        return;

    if ((fp = fopen(file, "a")) == nullptr) {
        perror(file);
        ch->send_line("Could not open the file!");
    } else {
        fprintf(fp, "[%5d] %s: %s\n", ch->in_room ? ch->in_room->vnum : 0, ch->name.c_str(), str);
        fclose(fp);
    }
}

/* This routine transfers between alpha and numeric forms of the
 *  mob_prog bitvector types. This allows the use of the words in the
 *  mob/script files.
 */
int mprog_name_to_type(char *name) {
    if (!str_cmp(name, "in_file_prog"))
        return IN_FILE_PROG;
    if (!str_cmp(name, "act_prog"))
        return ACT_PROG;
    if (!str_cmp(name, "speech_prog"))
        return SPEECH_PROG;
    if (!str_cmp(name, "rand_prog"))
        return RAND_PROG;
    if (!str_cmp(name, "fight_prog"))
        return FIGHT_PROG;
    if (!str_cmp(name, "hitprcnt_prog"))
        return HITPRCNT_PROG;
    if (!str_cmp(name, "death_prog"))
        return DEATH_PROG;
    if (!str_cmp(name, "entry_prog"))
        return ENTRY_PROG;
    if (!str_cmp(name, "greet_prog"))
        return GREET_PROG;
    if (!str_cmp(name, "all_greet_prog"))
        return ALL_GREET_PROG;
    if (!str_cmp(name, "give_prog"))
        return GIVE_PROG;
    if (!str_cmp(name, "bribe_prog"))
        return BRIBE_PROG;
    return (ERROR_PROG);
}

/* This routine reads in scripts of MOBprograms from a file */
MPROG_DATA *mprog_file_read(char *file_name, MPROG_DATA *mprg, MobIndexData *pMobIndex) {
    MPROG_DATA *mprg2;
    FILE *progfile;
    char letter;
    bool done = false;
    std::string file_path = Configuration::singleton().area_dir() + file_name;
    progfile = fopen(file_path.c_str(), "r");
    if (!progfile) {
        bug("Mob:{} couldnt open mobprog file {}", pMobIndex->vnum, file_path);
        exit(1);
    }
    mprg2 = mprg;
    switch (letter = fread_letter(progfile)) {
    case '>': break;
    case '|':
        bug("empty mobprog file.");
        exit(1);
        break;
    default:
        bug("in mobprog file syntax error.");
        exit(1);
        break;
    }
    while (!done) {
        mprg2->type = mprog_name_to_type(fread_word(progfile));
        switch (mprg2->type) {
        case ERROR_PROG:
            bug("mobprog file type error");
            exit(1);
            break;
        case IN_FILE_PROG:
            bug("mprog file contains a call to file.");
            exit(1);
            break;
        default:
            pMobIndex->progtypes = pMobIndex->progtypes | mprg2->type;
            mprg2->arglist = fread_string(progfile);
            mprg2->comlist = fread_string(progfile);
            switch (letter = fread_letter(progfile)) {
            case '>':
                mprg2->next = (MPROG_DATA *)alloc_perm(sizeof(MPROG_DATA));
                mprg2 = mprg2->next;
                mprg2->next = nullptr;
                break;
            case '|': done = true; break;
            default:
                bug("in mobprog file syntax error.");
                exit(1);
                break;
            }
            break;
        }
    }
    fclose(progfile);
    return mprg2;
}

/* Snarf a MOBprogram section from the area file.
 */
void load_mobprogs(FILE *fp) {
    char letter;
    MobIndexData *iMob;
    int value;
    MPROG_DATA *original;
    MPROG_DATA *working;

    auto area_last = AreaList::singleton().back();
    if (area_last == nullptr) {
        bug("Load_mobprogs: no #AREA seen yet!");
        exit(1);
    }

    for (;;) {
        switch (letter = fread_letter(fp)) {
        default:
            bug("Load_mobprogs: bad command '{}'.", letter);
            exit(1);
            break;
        case 'S':
        case 's': fread_to_eol(fp); return;
        case '*': fread_to_eol(fp); break;
        case 'M':
        case 'm':
            value = fread_number(fp);
            if ((iMob = get_mob_index(value)) == nullptr) {
                bug("Load_mobprogs: vnum {} doesnt exist", value);
                exit(1);
            }

            original = iMob->mobprogs;
            if (original != nullptr)
                for (; original->next != nullptr; original = original->next)
                    ;
            working = (MPROG_DATA *)alloc_perm(sizeof(MPROG_DATA));
            if (original)
                original->next = working;
            else
                iMob->mobprogs = working;
            working = mprog_file_read(fread_word(fp), working, iMob);
            working->next = nullptr;
            fread_to_eol(fp);
            break;
        }
    }
}

/* This procedure is responsible for reading any in_file MOBprograms.
 */
void mprog_read_programs(FILE *fp, MobIndexData *pMobIndex) {
    MPROG_DATA *mprg;
    bool done = false;
    char letter;
    if ((letter = fread_letter(fp)) != '>') {
        bug("Load_mobiles: vnum {} MOBPROG char", pMobIndex->vnum);
        exit(1);
    }
    pMobIndex->mobprogs = (MPROG_DATA *)alloc_perm(sizeof(MPROG_DATA));
    mprg = pMobIndex->mobprogs;
    while (!done) {
        mprg->type = mprog_name_to_type(fread_word(fp));
        switch (mprg->type) {
        case ERROR_PROG:
            bug("Load_mobiles: vnum {} MOBPROG type.", pMobIndex->vnum);
            exit(1);
            break;
        case IN_FILE_PROG:
            mprg = mprog_file_read(fread_string(fp), mprg, pMobIndex);
            fread_to_eol(fp);
            switch (letter = fread_letter(fp)) {
            case '>':
                mprg->next = (MPROG_DATA *)alloc_perm(sizeof(MPROG_DATA));
                mprg = mprg->next;
                mprg->next = nullptr;
                break;
            case '|':
                mprg->next = nullptr;
                fread_to_eol(fp);
                done = true;
                break;
            default:
                bug("Load_mobiles: vnum {} bad MOBPROG.", pMobIndex->vnum);
                exit(1);
                break;
            }
            break;
        default:
            pMobIndex->progtypes = pMobIndex->progtypes | mprg->type;
            mprg->arglist = fread_string(fp);
            fread_to_eol(fp);
            mprg->comlist = fread_string(fp);
            fread_to_eol(fp);
            switch (letter = fread_letter(fp)) {
            case '>':
                mprg->next = (MPROG_DATA *)alloc_perm(sizeof(MPROG_DATA));
                mprg = mprg->next;
                mprg->next = nullptr;
                break;
            case '|':
                mprg->next = nullptr;
                fread_to_eol(fp);
                done = true;
                break;
            default:
                bug("Load_mobiles: vnum {} bad MOBPROG.", pMobIndex->vnum);
                exit(1);
                break;
            }
            break;
        }
    }
}
