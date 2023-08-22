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
#include "Area.hpp"
#include "AreaList.hpp"
#include "Attacks.hpp"
#include "BodySize.hpp"
#include "Char.hpp"
#include "CharActFlag.hpp"
#include "CommFlag.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "Exit.hpp"
#include "ExitFlag.hpp"
#include "Help.hpp"
#include "Logging.hpp"
#include "MProgLoader.hpp"
#include "Materials.hpp"
#include "MobIndexData.hpp"
#include "Note.hpp"
#include "Object.hpp"
#include "ObjectExtraFlag.hpp"
#include "ObjectIndex.hpp"
#include "ObjectType.hpp"
#include "OffensiveFlag.hpp"
#include "ResetData.hpp"
#include "RoomFlag.hpp"
#include "ScrollTargetValidator.hpp"
#include "Shop.hpp"
#include "SkillNumbers.hpp"
#include "SkillTables.hpp"
#include "Socials.hpp"
#include "TimeInfoData.hpp"
#include "VnumRooms.hpp"
#include "Weapon.hpp"
#include "WeatherData.hpp"
#include "Worn.hpp"
#include "WrappedFd.hpp"
#include "common/BitOps.hpp"
#include "common/Configuration.hpp"
#include "handler.hpp"
#include "interp.h"
#include "lookup.h"
#include "magic.h"
#include "string_utils.hpp"

#include <fmt/format.h>
#include <range/v3/algorithm/fill.hpp>
#include <range/v3/iterator/operations.hpp>
#include <range/v3/numeric/accumulate.hpp>

#include <gsl/narrow>
#include <range/v3/algorithm/find_if.hpp>
#include <sys/resource.h>
#include <utility>

namespace {

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

// Mob templates. A map only so things like "vnum mob XXX" are ordered.
std::map<int, MobIndexData> mob_indexes;
// Object templates.
std::map<int, ObjectIndex> object_indexes;
// Unlike the Mob & Object index maps, Rooms are instances.
std::map<int, Room> rooms;
const auto InitialObjectListCapacity = 1400u;

}

/* Externally referenced functions. */
void wiznet_initialise();
SpecialFunc spec_lookup(std::string_view name);

// Mutable global: modified whenever a new Char is loaded from the database or when a player Char logs in or out.
GenericList<Char *> char_list;
// Mutable global: modified whenever a new object is created or destroyed.
std::vector<std::unique_ptr<Object>> object_list;

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

void load_rooms(FILE *fp);

void load_shops(FILE *fp);
void load_specials(FILE *fp);

void map_exit_destinations();

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

        object_list.reserve(InitialObjectListCapacity);

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
                if (fread_letter(area_fp) != '#') {
                    bug("Boot_db: # not found.");
                    exit(1);
                }
                const auto word = fread_word(area_fp);
                if (word[0] == '$')
                    break;
                else if (matches(word, "AREA"))
                    load_area(area_fp, area_name);
                else if (matches(word, "HELPS"))
                    load_helps(area_fp);
                else if (matches(word, "MOBILES"))
                    load_mobiles(area_fp);
                else if (matches(word, "OBJECTS"))
                    load_objects(area_fp);
                else if (matches(word, "RESETS"))
                    load_resets(area_fp);
                else if (matches(word, "ROOMS"))
                    load_rooms(area_fp);
                else if (matches(word, "SHOPS"))
                    load_shops(area_fp);
                else if (matches(word, "SOCIALS"))
                    Socials::singleton().load(area_fp);
                else if (matches(word, "SPECIALS"))
                    load_specials(area_fp);
                else if (matches(word, "MOBPROGS"))
                    MProg::load_mobprogs(area_fp);
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
    map_exit_destinations();
    fBootDb = false;
    AreaList::singleton().sort();
    area_update();
    note_initialise();
    wiznet_initialise();
    interp_initialise();
}

// On shutdown, deletes the Chars owned by char_list.
// Objects in object_list are deleted automatically.
// Note that this doesn't call extract_char(), so it relies on Char's destructor
// to release any pointers the Char owns.
void delete_globals_on_shutdown() {
    for (auto *ch : char_list) {
        delete ch;
    }
}

/* Snarf an 'area' header line. */
void load_area(FILE *fp, const std::string &area_name) {
    auto &area_list = AreaList::singleton();
    auto area_obj = Area::parse(gsl::narrow<int>(area_list.count()), fp, area_name);

    AreaList::singleton().add(std::make_unique<Area>(std::move(area_obj)));

    area_header_found = true;
}

/* Snarf a help section. */
void load_helps(FILE *fp) {
    while (auto help = Help::load(fp, area_header_found ? AreaList::singleton().back() : nullptr))
        HelpList::singleton().add(std::move(*help));
}

void load_resets(FILE *fp) {
    Room *room;
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

        const auto command = letter;
        /* if_flag */ fread_number(fp);
        const sh_int arg1 = fread_number(fp);
        const sh_int arg2 = fread_number(fp);
        const sh_int arg3 = (letter == ResetGiveObjMob || letter == ResetRandomizeExits) ? 0 : fread_number(fp);
        const sh_int arg4 = (letter == ResetPutObjInObj || letter == ResetMobInRoom) ? fread_number(fp) : 0;
        fread_to_eol(fp);
        const auto reset = ResetData{command, arg1, arg2, arg3, arg4};

        /* Don't validate now, do it after all area's have been loaded */
        /* Stuff to add reset to the correct room */
        switch (letter) {
        default: bug("Load_resets: bad command '{}'.", letter); exit(1);
        case ResetMobInRoom:
            if ((room = get_room(reset.arg3))) {
                room->resets.push_back(std::move(reset));
                iLastRoom = reset.arg3;
            }
            break;
        case ResetObjInRoom:
            if ((room = get_room(reset.arg3))) {
                room->resets.push_back(std::move(reset));
                iLastObj = reset.arg3;
            }
            break;
        case ResetPutObjInObj:
            if ((room = get_room(iLastObj)))
                room->resets.push_back(std::move(reset));
            break;
        case ResetGiveObjMob:
        case ResetEquipObjMob:
            if ((room = get_room(iLastRoom))) {
                room->resets.push_back(std::move(reset));
                iLastObj = iLastRoom;
            }
            break;
        case ResetExitFlags: {
            room = get_room(reset.arg1);
            auto opt_direction = try_cast_direction(reset.arg2);

            if (!opt_direction || !room || !room->exits[*opt_direction]
                || !check_enum_bit(room->exits[*opt_direction]->rs_flags, ExitFlag::IsDoor)) {
                bug("Load_resets: 'D': exit {} not door.", reset.arg2);
                exit(1);
            }
            auto &exit = room->exits[*opt_direction];
            switch (reset.arg3) {
            default: bug("Load_resets: 'D': bad 'locks': {}.", reset.arg3);
            case 0: break;
            case 1: set_enum_bit(exit->rs_flags, ExitFlag::Closed); break;
            case 2: {
                set_enum_bit(exit->rs_flags, ExitFlag::Closed);
                set_enum_bit(exit->rs_flags, ExitFlag::Locked);
                break;
            }
            }
            break;
        }
        case ResetRandomizeExits:
            if (reset.arg2 < 0 || reset.arg2 > static_cast<int>(all_directions.size())) {
                bug("Load_resets: 'R': bad exit {}.", reset.arg2);
                exit(1);
            }

            if ((room = get_room(reset.arg1)))
                room->resets.push_back(std::move(reset));

            break;
        }
    }
}

/* Snarf a room section. */
void load_rooms(FILE *fp) {

    auto area_last = AreaList::singleton().back();
    if (area_last == nullptr) {
        bug("load_rooms: no #AREA seen yet.");
        exit(1);
    }

    for (;;) {
        char letter = fread_letter(fp);
        if (letter != '#') {
            bug("load_rooms: # not found.");
            exit(1);
        }
        sh_int vnum = fread_number(fp);
        if (vnum == 0)
            break;
        Room room;
        room.area = area_last;
        room.vnum = vnum;
        room.name = fread_string(fp);
        room.description = fread_string(fp);
        room.room_flags = fread_flag(fp);
        /* horrible hack */
        if (3000 <= vnum && vnum < 3400)
            set_enum_bit(room.room_flags, RoomFlag::Law);
        int sector_value = fread_number(fp);
        if (auto sector_type = try_get_sector_type(sector_value)) {
            room.sector_type = *sector_type;
        } else {
            bug("Invalid sector type {}, defaulted to {}", sector_value, room.sector_type);
        }

        for (;;) {
            letter = fread_letter(fp);

            if (letter == 'S')
                break;

            if (letter == 'D') {
                Exit exit;
                int locks;
                auto opt_direction = try_cast_direction(fread_number(fp));
                if (!opt_direction) {
                    bug("load_rooms: vnum {} has bad door number.", vnum);
                    ::exit(1);
                }

                exit.description = fread_string(fp);
                exit.keyword = fread_string(fp);
                exit.exit_info = 0;
                locks = fread_number(fp);
                exit.key = fread_number(fp);
                auto exit_vnum = fread_number(fp);
                /* If an exit's destination room vnum is negative, it can be a cosmetic-only
                 * exit (-1), otherwise it's a one-way exit.
                 * fix_exits() ignores one-way exits that don't have a return path.
                 */
                if (exit_vnum < EXIT_VNUM_COSMETIC) {
                    exit.is_one_way = true;
                    exit_vnum = -exit_vnum;
                }
                exit.u1.vnum = exit_vnum;
                exit.rs_flags = 0;

                switch (locks) {

                    /* the following statements assign rs_flags, replacing
                            exit_info which is what used to get set. */
                case 1: exit.rs_flags = to_int(ExitFlag::IsDoor); break;
                case 2: exit.rs_flags = to_int(ExitFlag::IsDoor) | to_int(ExitFlag::PickProof); break;
                case 3: exit.rs_flags = to_int(ExitFlag::IsDoor) | to_int(ExitFlag::PassProof); break;
                case 4:
                    exit.rs_flags =
                        to_int(ExitFlag::IsDoor) | to_int(ExitFlag::PassProof) | to_int(ExitFlag::PickProof);
                    break;
                }

                room.exits[*opt_direction] = std::move(exit);
            } else if (letter == 'E') {
                auto keyword = fread_string(fp);
                auto description = fread_string(fp);
                room.extra_descr.emplace_back(ExtraDescription{keyword, description});
            } else {
                bug("load_rooms: vnum {} has unrecognized instruction '{}'.", vnum, letter);
                exit(1);
            }
        }
        if (!rooms.try_emplace(vnum, std::move(room)).second) {
            bug("load_rooms: vnum {} duplicated.", vnum);
            exit(1);
        }
    }
}

/* Snarf a shop section. */
void load_shops(FILE *fp) {
    for (;;) {
        MobIndexData *mobIndex;
        uint iTrade;
        auto shopkeeper_vnum = fread_number(fp);
        if (shopkeeper_vnum == 0)
            break;
        Shop shop;
        shop.keeper = shopkeeper_vnum;
        for (iTrade = 0; iTrade < MaxTrade; iTrade++) {
            const auto raw_obj_type = fread_number(fp);
            if (const auto opt_obj_type = ObjectTypes::try_from_integer(raw_obj_type)) {
                shop.buy_type[iTrade] = *opt_obj_type;
            }
            // If the raw object type number is zero or unrecognized we silently ignore it.
            // The typical case is that the shopkeeper is configured to not buy anything and you can use zero to
            // state that.
        }
        shop.profit_buy = fread_number(fp);
        shop.profit_sell = fread_number(fp);
        shop.open_hour = fread_number(fp);
        shop.close_hour = fread_number(fp);
        fread_to_eol(fp);
        mobIndex = get_mob_index(shop.keeper);
        mobIndex->shop = std::move(shop);
    }
}

/* Snarf spec proc declarations. */
void load_specials(FILE *fp) {
    for (;;) {
        MobIndexData *mobIndex;
        char letter;

        switch (letter = fread_letter(fp)) {
        default: bug("Load_specials: letter '{}' not *, M or S.", letter); exit(1);

        case 'S': return;

        case '*': break;

        case 'M':
            mobIndex = get_mob_index(fread_number(fp));
            mobIndex->spec_fun = spec_lookup(fread_word(fp));
            if (mobIndex->spec_fun == 0) {
                bug("Load_specials: 'M': vnum {}.", mobIndex->vnum);
                exit(1);
            }
            break;
        }

        fread_to_eol(fp);
    }
}

namespace {

void assign_area_vnum(int vnum) { AreaList::singleton().back()->define_vnum(vnum); }

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
    char temp; /* Used for Death's Wear Strings bit */

    auto area_last = AreaList::singleton().back();
    if (!area_last) {
        bug("Load_objects: no #AREA section found yet!");
        exit(1);
    }
    const auto validators = std::array<std::unique_ptr<const ObjectIndexValidator>, 1>{
        std::make_unique<ScrollTargetValidator>(skill_table)};
    for (;;) {
        char letter = fread_letter(fp);
        if (letter != '#') {
            bug("Load_objects: # not found.");
            exit(1);
        }
        sh_int vnum = fread_number(fp);
        if (vnum == 0)
            break;
        ObjectIndex obj_index;
        obj_index.vnum = vnum;
        obj_index.area = area_last;
        obj_index.reset_num = 0;
        obj_index.name = fread_string(fp);
        /*
         * MG added - snarf short descrips to kill:
         * You hit The beastly fido
         */
        obj_index.short_descr = lower_case_articles(fread_string(fp));
        obj_index.description = fread_string(fp);
        if (obj_index.description.empty()) {
            bug("Load_objects: empty long description in object {}.", vnum);
        }

        obj_index.material = Material::lookup_with_default(fread_string(fp))->material;
        obj_index.type = ObjectTypes::lookup_with_default(fread_word(fp));
        obj_index.extra_flags = fread_flag(fp);

        if (obj_index.is_no_remove() && obj_index.type != ObjectType::Weapon) {
            bug("Only weapons are meant to have ObjectExtraFlag::NoRemove: {} {}", obj_index.vnum, obj_index.name);
            exit(1);
        }

        obj_index.wear_flags = fread_flag(fp);

        temp = fread_letter(fp);
        if (temp == ',') {
            obj_index.wear_string = fread_string(fp);
        } else {
            ungetc(temp, fp);
        }

        switch (obj_index.type) {
        case ObjectType::Weapon: {
            const auto raw_weapon_type = fread_word(fp);
            if (const auto opt_weapon_type = Weapons::try_from_name(raw_weapon_type)) {
                obj_index.value[0] = magic_enum::enum_integer<Weapon>(*opt_weapon_type);
            } else {
                bug("Invalid weapon type {} in object: {} {}", raw_weapon_type, obj_index.vnum, obj_index.short_descr);
                obj_index.value[0] = 0;
            }
            obj_index.value[1] = fread_number(fp);
            obj_index.value[2] = fread_number(fp);
            const auto attack_name = fread_word(fp);
            const auto attack_index = Attacks::index_of(attack_name);
            if (attack_index < 0) {
                bug("Invalid attack type {} in object: {} {}, defaulting to hit", attack_name, obj_index.vnum,
                    obj_index.short_descr);
                obj_index.value[3] = Attacks::index_of("hit");
            } else {
                obj_index.value[3] = attack_index;
            }
            obj_index.value[4] = fread_flag(fp);
            break;
        }
        case ObjectType::Container:
            obj_index.value[0] = fread_number(fp);
            obj_index.value[1] = fread_flag(fp);
            obj_index.value[2] = fread_number(fp);
            obj_index.value[3] = fread_number(fp);
            obj_index.value[4] = fread_number(fp);
            break;
        case ObjectType::Drink:
        case ObjectType::Fountain: {
            obj_index.value[0] = fread_number(fp);
            obj_index.value[1] = fread_number(fp);
            const auto raw_liquid = fread_word(fp);
            if (const auto *liquid = Liquid::try_lookup(raw_liquid)) {
                obj_index.value[2] = magic_enum::enum_integer<Liquid::Type>(liquid->liquid);
            } else {
                bug("Invalid liquid {} in object: {} {}, defaulting.", raw_liquid, obj_index.vnum,
                    obj_index.short_descr);
                obj_index.value[2] = 0;
            }
            obj_index.value[3] = fread_number(fp);
            obj_index.value[4] = fread_number(fp);
            break;
        }
        case ObjectType::Wand:
        case ObjectType::Staff:
            obj_index.value[0] = fread_number(fp);
            obj_index.value[1] = fread_number(fp);
            obj_index.value[2] = fread_number(fp);
            obj_index.value[3] = fread_spnumber(fp);
            obj_index.value[4] = fread_number(fp);
            break;
        case ObjectType::Potion:
        case ObjectType::Pill:
        case ObjectType::Scroll:
        case ObjectType::Bomb:
            obj_index.value[0] = fread_number(fp);
            obj_index.value[1] = fread_spnumber(fp);
            obj_index.value[2] = fread_spnumber(fp);
            obj_index.value[3] = fread_spnumber(fp);
            obj_index.value[4] = fread_spnumber(fp);
            break;
        default:
            obj_index.value[0] = fread_flag(fp);
            obj_index.value[1] = fread_flag(fp);
            obj_index.value[2] = fread_flag(fp);
            obj_index.value[3] = fread_flag(fp);
            obj_index.value[4] = fread_flag(fp);
            break;
        }

        obj_index.level = fread_number(fp);
        obj_index.weight = fread_number(fp);
        obj_index.cost = fread_number(fp);

        /* condition */
        letter = fread_letter(fp);
        switch (letter) {
        case ('P'): obj_index.condition = 100; break;
        case ('G'): obj_index.condition = 90; break;
        case ('A'): obj_index.condition = 75; break;
        case ('W'): obj_index.condition = 50; break;
        case ('D'): obj_index.condition = 25; break;
        case ('B'): obj_index.condition = 10; break;
        case ('R'): obj_index.condition = 0; break;
        default: obj_index.condition = 100; break;
        }

        for (;;) {
            char letter;

            letter = fread_letter(fp);

            if (letter == 'A') {
                AFFECT_DATA af;
                af.type = -1;
                af.level = obj_index.level;
                af.duration = -1;
                af.location = static_cast<AffectLocation>(fread_number(fp));
                af.modifier = fread_number(fp);
                obj_index.affected.add(af);
            }

            else if (letter == 'E') {
                auto keyword = fread_string(fp);
                auto description = fread_string(fp);
                obj_index.extra_descr.emplace_back(ExtraDescription{keyword, description});
            }

            else {
                ungetc(letter, fp);
                break;
            }
        }

        /*
         * Translate spell "slot numbers" to internal "skill numbers."
         */
        switch (obj_index.type) {
        case ObjectType::Bomb:
            obj_index.value[4] = slot_lookup(obj_index.value[4]);
            // fall through
        case ObjectType::Pill:
        case ObjectType::Potion:
        case ObjectType::Scroll:
            obj_index.value[1] = slot_lookup(obj_index.value[1]);
            obj_index.value[2] = slot_lookup(obj_index.value[2]);
            obj_index.value[3] = slot_lookup(obj_index.value[3]);
            break;

        case ObjectType::Staff:
        case ObjectType::Wand: obj_index.value[3] = slot_lookup(obj_index.value[3]); break;
        default:;
        }
        for (auto &&validator : validators) {
            if (const auto result = validator->validate(obj_index); result.error_message) {
                bug(*result.error_message);
                exit(1);
            }
        }
        if (!object_indexes.try_emplace(vnum, std::move(obj_index)).second) {
            bug("load_objects: vnum {} duplicated.", vnum);
            exit(1);
        }
        assign_area_vnum(vnum);
    }
}

/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void map_exit_destinations() {

    const auto mutable_rooms = []() {
        return rooms | ranges::views::transform([](auto &p) -> Room & { return p.second; });
    };
    for (auto &room : mutable_rooms()) {
        bool fexit = false;
        for (auto direction : all_directions) {
            if (auto &exit = room.exits[direction]; exit) {
                if (exit->u1.vnum <= 0 || get_room(exit->u1.vnum) == nullptr)
                    exit->u1.to_room = nullptr;
                else {
                    fexit = true;
                    auto to_room = get_room(exit->u1.vnum);
                    exit->u1.to_room = to_room;
                }
            }
        }
        if (!fexit)
            set_enum_bit(room.room_flags, RoomFlag::NoMob);
    }
    for (auto &room : mutable_rooms()) {
        for (auto direction : all_directions) {
            auto &exit = room.exits[direction];
            if (!exit)
                continue;
            if (Room *to_room = exit->u1.to_room) {
                if (auto &exit_rev = to_room->exits[reverse(direction)];
                    exit_rev && exit_rev->u1.to_room->vnum != room.vnum && !exit->is_one_way) {
                    bug("Fix_exits: {} -> {}:{} -> {}.", room.vnum, static_cast<int>(direction), to_room->vnum,
                        static_cast<int>(reverse(direction)),
                        (exit_rev->u1.to_room == nullptr) ? 0 : exit_rev->u1.to_room->vnum);
                }
            }
        }
    }
}

void area_update() {
    for (auto &pArea : AreaList::singleton())
        pArea->update();
}

/*
 * Reset one room.
 */
void reset_room(Room *room) {
    Char *lastMob = nullptr;
    bool lastMobWasReset = false;
    if (!room)
        return;

    for (auto exit_dir : all_directions) {
        if (auto &exit = room->exits[exit_dir]; exit) {
            exit->exit_info = exit->rs_flags;
            if (exit->u1.to_room) {
                if (auto &exit_rev = exit->u1.to_room->exits[reverse(exit_dir)]; exit_rev) {
                    exit_rev->exit_info = exit_rev->rs_flags;
                }
            }
        }
    }

    for (const auto &reset : room->resets) {
        switch (reset.command) {
        default: bug("Reset_room: bad command {}.", reset.command); break;

        case ResetMobInRoom: {
            MobIndexData *mobIndex;
            if (!(mobIndex = get_mob_index(reset.arg1))) {
                bug("Reset_room: 'M': bad vnum {}.", reset.arg1);
                continue;
            }
            if (mobIndex->count >= reset.arg2) {
                lastMobWasReset = false;
                continue;
            }
            int count = 0;
            for (auto *mch : room->people) {
                if (mch->mobIndex == mobIndex) {
                    count++;
                    if (count >= reset.arg4) {
                        lastMobWasReset = false;
                        break;
                    }
                }
            }

            if (count >= reset.arg4)
                continue;

            auto *mob = create_mobile(mobIndex);

            // Pet shop mobiles get CharActFlag::Pet set.
            Room *previousRoom;
            previousRoom = get_room(room->vnum - 1);
            if (previousRoom && check_enum_bit(previousRoom->room_flags, RoomFlag::PetShop))
                set_enum_bit(mob->act, CharActFlag::Pet);

            char_to_room(mob, room);
            lastMob = mob;
            lastMobWasReset = true;
            break;
        }

        case ResetObjInRoom: {
            ObjectIndex *objIndex;
            Room *obj_room;
            if (!(objIndex = get_obj_index(reset.arg1))) {
                bug("Reset_room: 'O': bad vnum {}.", reset.arg1);
                continue;
            }

            if (!(obj_room = get_room(reset.arg3))) {
                bug("Reset_room: 'O': bad vnum {}.", reset.arg3);
                continue;
            }

            if (obj_room->area->occupied() || count_obj_list(objIndex, obj_room->contents) > 0) {
                continue;
            }

            auto obj_uptr = objIndex->create_object();
            auto *object = obj_uptr.get();
            object_list.push_back(std::move(obj_uptr));
            object->cost = 0;
            obj_to_room(object, obj_room);
            break;
        }

        case ResetPutObjInObj: {
            int limit, count;
            ObjectIndex *contained_obj_idx;
            ObjectIndex *container_obj_idx;
            Object *container_obj;
            if (!(contained_obj_idx = get_obj_index(reset.arg1))) {
                bug("Reset_room: 'P': bad vnum {}.", reset.arg1);
                continue;
            }

            if (!(container_obj_idx = get_obj_index(reset.arg3))) {
                bug("Reset_room: 'P': bad vnum {}.", reset.arg3);
                continue;
            }

            if (reset.arg2 > 20) /* old format reduced from 50! */
                limit = 6;
            else if (reset.arg2 == -1) /* no limit */
                limit = 999;
            else
                limit = reset.arg2;

            // Don't create the contained object if:
            // The area has players right now, or if the containing object doesn't exist in the world,
            // or if already too many instances of contained object (and without 80% chance of "over spawning"),
            // or if count of contained object within the container's current room exceeds the item-in-room limit.
            if (room->area->occupied() || (container_obj = get_obj_type(container_obj_idx)) == nullptr
                || (contained_obj_idx->count >= limit && number_range(0, 4) != 0)
                || (count = count_obj_list(contained_obj_idx, container_obj->contains)) > reset.arg4) {
                continue;
            }

            while (count < reset.arg4) {
                auto obj_uptr = contained_obj_idx->create_object();
                auto *contained_obj = obj_uptr.get();
                object_list.push_back(std::move(obj_uptr));
                obj_to_obj(contained_obj, container_obj);
                count++;
                if (contained_obj_idx->count >= limit)
                    break;
            }

            // Close the container if required.
            if (container_obj->type == ObjectType::Container) {
                container_obj->value[1] = container_obj->objIndex->value[1];
            }
            break;
        }

        case ResetGiveObjMob:
        case ResetEquipObjMob: {
            ObjectIndex *obj_idx;
            Object *object;
            if (!(obj_idx = get_obj_index(reset.arg1))) {
                bug("Reset_room: 'E' or 'G': bad vnum {}.", reset.arg1);
                continue;
            }

            if (!lastMobWasReset)
                continue;

            if (!lastMob) {
                bug("Reset_room: 'E' or 'G': null mob for vnum {}.", reset.arg1);
                lastMobWasReset = false;
                continue;
            }

            if (lastMob->mobIndex->shop) { /* Shop-keeper? */
                auto obj_uptr = obj_idx->create_object();
                object = obj_uptr.get();
                object_list.push_back(std::move(obj_uptr));
                set_enum_bit(object->extra_flags, ObjectExtraFlag::Inventory);
            } else {
                const auto drop_rate = reset.arg2;
                if (drop_rate <= 0 || drop_rate > 100) {
                    bug("Invalid object drop rate: {} for object #{}", drop_rate, reset.arg1);
                    exit(1);
                }
                if (number_percent() <= drop_rate) {
                    auto obj_uptr = obj_idx->create_object();
                    object = obj_uptr.get();
                    object_list.push_back(std::move(obj_uptr));
                } else
                    continue;
            }

            obj_to_char(object, lastMob);
            if (reset.command == ResetEquipObjMob) {
                if (const auto opt_worn_loc = magic_enum::enum_cast<Worn>(reset.arg3)) {
                    equip_char(lastMob, object, *opt_worn_loc);
                } else {
                    bug("Invalid worn location: {} for object #{}", reset.arg3, reset.arg1);
                }
            }
            lastMobWasReset = true;
            break;
        }

        case ResetExitFlags: break;

        case ResetRandomizeExits: {
            Room *exit_room;
            if (!(exit_room = get_room(reset.arg1))) {
                bug("Reset_room: 'R': bad vnum {}.", reset.arg1);
                continue;
            }

            for (int d0 = 0; d0 < reset.arg2 - 1; d0++) {
                auto direction0 = try_cast_direction(d0).value();
                auto direction1 = try_cast_direction(number_range(d0, reset.arg2 - 1)).value();
                std::swap(exit_room->exits[direction0], exit_room->exits[direction1]);
            }
            break;
        }
        }
    }
}

/*
 * Create an instance of a mobile.
 */
Char *create_mobile(MobIndexData *mobIndex) {
    if (mobIndex == nullptr) {
        bug("Create_mobile: nullptr mobIndex.");
        exit(1);
    }

    auto *mob = new Char;
    mob->mobIndex = mobIndex;

    mob->name = mobIndex->player_name;
    mob->short_descr = mobIndex->short_descr;
    mob->long_descr = mobIndex->long_descr;
    mob->description = mobIndex->description;
    mob->spec_fun = mobIndex->spec_fun;

    /* read from prototype */
    mob->act = mobIndex->act;
    mob->comm = to_int(CommFlag::NoChannels) | to_int(CommFlag::NoYell) | to_int(CommFlag::NoTell);
    mob->affected_by = mobIndex->affected_by;
    mob->alignment = mobIndex->alignment;
    mob->level = mobIndex->level;
    mob->hitroll = mobIndex->hitroll;
    mob->max_hit = mobIndex->hit.roll();
    mob->hit = mob->max_hit;
    mob->max_mana = mobIndex->mana.roll();
    mob->mana = mob->max_mana;
    mob->damage = mobIndex->damage;
    mob->damage.bonus(0); // clear the bonus; it's accounted for in the damroll
    mob->damroll = mobIndex->damage.bonus();
    mob->attack_type = mobIndex->attack_type;
    for (int i = 0; i < 4; i++)
        mob->armor[i] = mobIndex->ac[i];
    mob->off_flags = mobIndex->off_flags;
    mob->imm_flags = mobIndex->imm_flags;
    mob->res_flags = mobIndex->res_flags;
    mob->vuln_flags = mobIndex->vuln_flags;
    mob->start_pos = mobIndex->start_pos;
    mob->default_pos = mobIndex->default_pos;
    mob->sex = mobIndex->sex;
    mob->race = mobIndex->race;
    if (mobIndex->gold == 0)
        mob->gold = 0;
    else
        mob->gold = number_range(mobIndex->gold / 2, mobIndex->gold * 3 / 2);
    mob->morphology = mobIndex->morphology;
    mob->parts = mobIndex->parts;
    mob->body_size = mobIndex->body_size;
    mob->material = mobIndex->material;

    /* computed on the spot */

    ranges::fill(mob->perm_stat, std::min(25, 11 + mob->level / 4));

    if (check_enum_bit(mob->act, CharActFlag::Warrior)) {
        mob->perm_stat[Stat::Str] += 3;
        mob->perm_stat[Stat::Int] -= 1;
        mob->perm_stat[Stat::Con] += 2;
    }

    if (check_enum_bit(mob->act, CharActFlag::Thief)) {
        mob->perm_stat[Stat::Dex] += 3;
        mob->perm_stat[Stat::Int] += 1;
        mob->perm_stat[Stat::Wis] -= 1;
    }

    if (check_enum_bit(mob->act, CharActFlag::Cleric)) {
        mob->perm_stat[Stat::Wis] += 3;
        mob->perm_stat[Stat::Dex] -= 1;
        mob->perm_stat[Stat::Str] += 1;
    }

    if (check_enum_bit(mob->act, CharActFlag::Mage)) {
        mob->perm_stat[Stat::Int] += 3;
        mob->perm_stat[Stat::Str] -= 1;
        mob->perm_stat[Stat::Dex] += 1;
    }

    if (check_enum_bit(mob->off_flags, OffensiveFlag::Fast))
        mob->perm_stat[Stat::Dex] += 2;

    mob->perm_stat[Stat::Str] += BodySizes::get_mob_str_bonus(mob->body_size);
    mob->perm_stat[Stat::Con] += BodySizes::get_mob_con_bonus(mob->body_size);

    mob->position = mob->start_pos;

    /* link the mob to the world list */
    char_list.add_front(mob);
    mobIndex->count++;
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
    clone->morphology = parent->morphology;
    clone->parts = parent->parts;
    clone->body_size = parent->body_size;
    clone->material = parent->material;
    clone->off_flags = parent->off_flags;
    clone->attack_type = parent->attack_type;
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

const std::map<int, ObjectIndex> &all_object_index_pairs() { return object_indexes; }

const std::map<int, Room> &all_room_pairs() { return rooms; }

/*
 * Translates mob virtual number to its obj index struct.
 */
ObjectIndex *get_obj_index(int vnum) {
    const auto it = object_indexes.find(vnum);
    if (it != object_indexes.end()) {
        return &it->second;
    } else if (fBootDb) {
        bug("get_obj_index: bad vnum {}.", vnum);
        exit(1);
    }
    return nullptr;
}

/*
 * Translates room virtual number to its Room.
 */
Room *get_room(int vnum) {
    const auto it = rooms.find(vnum);
    if (it != rooms.end()) {
        return &it->second;
    } else if (fBootDb) {
        bug("get_room: bad vnum {}.", vnum);
        exit(1);
    }
    return nullptr;
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
 * Parse a skill/spell slot number, or its tilde terminated name, from a file.
 */
std::optional<int> try_fread_spnumber(FILE *fp) {
    int number = 0;
    skip_ws(fp);
    char c = getc(fp);
    if (!isdigit(c)) {
        if (c == '\'' || c == '"') {
            bug("fread_spnumber: quoted spell names not allowed, use spellname~ instead");
            return std::nullopt;
        }
        ungetc(c, fp);
        const auto spell_name = fread_string(fp);
        if ((number = skill_lookup(spell_name)) <= 0) {
            bug("fread_spnumber: bad spell.");
            return std::nullopt;
        }
        return skill_table[number].slot;
    }

    while (isdigit(c)) {
        number = number * 10 + c - '0';
        c = getc(fp);
    }
    if ((c != ' ') && (c != '~'))
        ungetc(c, fp);
    return number;
}

int fread_spnumber(FILE *fp) {
    if (const auto opt_spnumber = try_fread_spnumber(fp)) {
        return *opt_spnumber;
    } else {
        exit(1);
    }
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

std::string fread_string(FILE *fp) {
    skip_ws(fp);
    std::string result;
    for (;;) {
        auto c = fgetc(fp);
        switch (c) {
        default: result += static_cast<char>(c); break;
        case EOF: bug("fread_string: EOF"); return result;

        case '\n': result += "\n\r"; break;
        case '\r': break;

        case '~': return result;
        }
    }
}

std::string fread_string_eol(FILE *fp) {
    skip_ws(fp);
    std::string result;
    for (;;) {
        auto c = fgetc(fp);
        switch (c) {
        default: result += static_cast<char>(c); break;
        case EOF: bug("fread_string_eol: EOF"); return result;

        case '\n': return result;
        }
    }
}
/*
 * Read to end of line (for comments).
 */
void fread_to_eol(FILE *fp) {
    int c;

    do {
        c = getc(fp);
    } while (c != '\n' && c != '\r' && c != EOF);

    do {
        c = getc(fp);
    } while (c == '\n' || c == '\r');

    ungetc(c, fp);
}

// Read one word which can optionally begin with a single or double quote
// and must be terminated either with a whitespace, or with the same quote.
std::optional<std::string> try_fread_word(FILE *fp) {
    std::string word;
    int letter;
    char got_quote{'\0'};
    do {
        letter = fgetc(fp);
    } while (std::isspace(letter));
    for (;; letter = fgetc(fp)) {
        if (letter == EOF)
            break;
        if (!got_quote && (letter == '\'' || letter == '"')) {
            got_quote = letter;
            continue;
        }
        if (got_quote ? letter == got_quote : std::isspace(letter)) {
            if (std::isspace(letter))
                ungetc(letter, fp);
            return word;
        }
        word.push_back(static_cast<char>(letter));
    }
    bug("fread_word: unterminated word.");
    return std::nullopt;
}

// Read one word which can optionally begin with a single or double quote
// and must be terminated either with a whitespace, or with the same quote.
std::string fread_word(FILE *fp) {
    if (const auto opt_word = try_fread_word(fp)) {
        return *opt_word;
    } else {
        exit(1);
    }
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
    const Area *area_column1{};
    std::string_view colour_column1;
    const Area *area_column2{};
    std::string_view colour_column2;
    int num_found = 0;
    for (auto &area : AreaList::singleton()) {
        std::string_view colour = "|w";
        // Is it outside the requested range?
        if (area->min_level() > maxLevel || area->max_level() < minLevel)
            continue;

        // Work out colour code
        if (area->all_levels()) {
            colour = "|C";
        } else if (area->min_level() > (charLevel + 3) || area->max_level() < (charLevel - 3)) {
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
            ch->send_line("{}{:<39}{}{:<39}", colour_column1, area_column1->description(), colour_column2,
                          area_column2->description());
            area_column1 = area_column2 = nullptr;
        }
    }
    // Check for any straggling lines
    if (area_column1)
        ch->send_line("{}{:<39}", colour_column1, area_column1->description());
    if (num_found) {
        ch->send_line("");
        ch->send_line("Areas found: {}", num_found);
    } else {
        ch->send_line("No areas found.");
    }
}

void do_memory(Char *ch) {
    ch->send_line("Areas   {:5}", AreaList::singleton().count());
    ch->send_line("Helps   {:5}", HelpList::singleton().count());
    ch->send_line("Socials {:5}", Socials::singleton().count());
    ch->send_line("Mobs    {:5}", mob_indexes.size());
    ch->send_line("Chars   {:5}", Char::num_active());
    ch->send_line("Objs    {:5}", object_indexes.size());
    ch->send_line("Rooms   {:5}", rooms.size());
}

namespace {

bool dump_memory_stats(Char *ch) {
    if (auto fp = WrappedFd::open_write("mem.dmp")) {
        constexpr auto mem_format = "{:<9}{:>4} ({:>8} bytes)\n"sv;
        auto num_pcs = 0;
        auto aff_count = 0;
        auto count = 0;
        fmt::print(fp, mem_format, "MobProt"sv, mob_indexes.size(), mob_indexes.size() * (sizeof(MobIndexData)));
        for (auto *fch : char_list) {
            count++;
            if (fch->pcdata != nullptr)
                num_pcs++;
            aff_count += fch->affected.size();
        }

        fmt::print(fp, mem_format, "Mobs"sv, count, count * (sizeof(MobIndexData)));
        fmt::print(fp, mem_format, "PcData"sv, num_pcs, num_pcs * (sizeof(PcData)));
        count = static_cast<int>(ranges::distance(descriptors().all()));
        fmt::print(fp, mem_format, "Descs"sv, count, count * (sizeof(Descriptor)));
        aff_count = ranges::accumulate(all_object_indexes() | ranges::views::transform([](const auto &obj_index) {
                                           return obj_index.affected.size();
                                       }),
                                       aff_count);
        fmt::print(fp, mem_format, "ObjProt"sv, object_indexes.size(), object_indexes.size() * (sizeof(ObjectIndex)));
        count = 0;
        for (auto &&obj : object_list) {
            count++;
            aff_count += obj->affected.size();
        }

        fmt::print(fp, mem_format, "Objs"sv, count, count * (sizeof(Object)));
        fmt::print(fp, mem_format, "Affects"sv, aff_count, aff_count * (sizeof(AFFECT_DATA)));
        fmt::print(fp, mem_format, "Rooms"sv, rooms.size(), rooms.size() * (sizeof(Room)));
        return true;
    } else {
        bug("Unable to open mem.dmp for write, char: {}.", ch->name);
        return false;
    }
}

bool dump_mobile_stats(Char *ch) {
    if (auto fp = WrappedFd::open_write("mob.dmp")) {
        fmt::print(fp, "\nMobile Analysis\n");
        fmt::print(fp, "---------------\n");
        for (const auto &mob : all_mob_indexes())
            fmt::print(fp, "#{:<4} {:>3} active {:>3} killed     {}\n", mob.vnum, mob.count, mob.killed,
                       mob.short_descr);
        return true;
    } else {
        bug("Unable to open mob.dmp for write, char: {}.", ch->name);
        return false;
    }
}

bool dump_object_stats(Char *ch) {
    if (auto fp = WrappedFd::open_write("obj.dmp")) {
        fmt::print(fp, "\nObject Analysis\n");
        fmt::print(fp, "---------------\n");
        for (const auto &obj_index : all_object_indexes()) {
            fmt::print(fp, "#{:<4} {:>3} active {:>3} reset      {}\n", obj_index.vnum, obj_index.count,
                       obj_index.reset_num, obj_index.short_descr);
        }
        return true;
    } else {
        bug("Unable to open obj.dmp for write, char: {}.", ch->name);
        return false;
    }
}

}

void do_dump(Char *ch) {
    if (dump_memory_stats(ch) && dump_mobile_stats(ch) && dump_object_stats(ch)) {
        ch->send_line("Dump complete.");
    } else {
        ch->send_line("Unable to complete the dump.");
    }
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
 * Appends text to a system file.
 */
bool append_file(std::string file, std::string_view text) {
    if (auto fp = WrappedFd::open_append(file)) {
        fmt::print(fp, "{}", text);
        return true;
    } else {
        perror(file.c_str());
        return false;
    }
}
