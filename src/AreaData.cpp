#include "AreaData.hpp"
#include "Room.hpp"
#include "VnumRooms.hpp"
#include "db.h"

#include <algorithm>
#include <gsl/gsl_util>

AreaData AreaData::parse(int area_num, FILE *fp, std::string filename) {
    fread_string(fp); /* filename */

    AreaData result;
    result.description_ = fread_stdstring(fp);
    result.short_name_ = fread_stdstring(fp);
    int scanRet = sscanf(result.short_name_.c_str(), "{%d %d}", &result.min_level_, &result.max_level_);
    if (scanRet != 2) {
        result.all_levels_ = true;
    }
    result.lowest_vnum_ = fread_number(fp);
    result.highest_vnum_ = fread_number(fp);
    result.num_ = area_num;
    result.filename_ = std::move(filename);

    result.age_ = std::numeric_limits<decltype(result.age_)>::max(); // trigger an area reset when main game loop starts

    return result;
}

void AreaData::inc_player_count() {
    if (empty_) {
        empty_ = false;
        age_ = 0;
    }
    ++nplayer_;
}

// Sets vnum range for area when loading its constituent mobs/objects/rooms.
void AreaData::define_vnum(int vnum) {
    if (lowest_vnum_ == 0 || highest_vnum_ == 0)
        lowest_vnum_ = highest_vnum_ = vnum;
    if (vnum != std::clamp(vnum, lowest_vnum_, highest_vnum_)) {
        if (vnum < lowest_vnum_)
            lowest_vnum_ = vnum;
        else
            highest_vnum_ = vnum;
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
 * "empty" has subtley different meaning to "nplayer == 0". "empty" is set false
 * whenever a player enters any room in the area (see char_to_room()).
 * nplayer is decremented whenever a player leaves a room within the area, but the empty flag is only set true
 * if the area has aged sufficiently to be reset _and_ if there are still no players in the area.
 * This means that players can't force an area to reset more quickly simply by stepping out of it!
 *
 * Due to the special case code below, Mud School _never_ gets flagged as empty.
 */
static inline constexpr auto RoomResetAgeOccupiedArea = 15;
static inline constexpr auto RoomResetAgeUnoccupiedArea = 10;
void AreaData::update() {
    ++age_;
    const auto reset_age = empty_ ? RoomResetAgeUnoccupiedArea : RoomResetAgeOccupiedArea;
    if (age_ >= reset_age)
        reset();
}

void AreaData::reset() {
    for (auto vnum = lowest_vnum_; vnum <= highest_vnum_; vnum++) {
        if (auto *room = get_room(vnum))
            reset_room(room);
    }
    age_ = gsl::narrow_cast<sh_int>(number_range(0, 3));

    if (auto room = get_room(rooms::MudschoolEntrance); room != nullptr && room->area == this)
        age_ = RoomResetAgeUnoccupiedArea;
    else if (nplayer_ == 0)
        empty_ = true;
}

AreaList &AreaList::singleton() {
    static AreaList singleton;
    return singleton;
}

// Sort the areas so that:
// - the lowest level ones come first
// - those with same minimum level,  those with narrowest level range come first
// Note: areas applicable to ALL levels always have a
// min_level of 0, so are always at the front.
void AreaList::sort() {
    std::sort(areas_.begin(), areas_.end(), [](const auto &a, const auto &b) {
        return a->min_level() < b->min_level()
               || (a->min_level() == b->min_level() && a->level_difference() < b->level_difference());
    });
}
