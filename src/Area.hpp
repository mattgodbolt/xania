#pragma once

#include "Constants.hpp"
#include "Types.hpp"

#include <memory>
#include <string>
#include <vector>

class Area {
    std::string short_name_;

    sh_int age_{};
    sh_int num_players_{};
    bool empty_since_last_reset_{};
    bool all_levels_{};
    int min_level_{0};
    int max_level_{MAX_LEVEL};
    std::string description_;
    std::string filename_;
    int lowest_vnum_{};
    int highest_vnum_{};
    int num_{};

    Area() = default;

    void reset();

public:
    static Area parse(int area_num, FILE *fp, std::string filename);

    void define_vnum(int vnum);

    void player_entered();
    void player_left() { --num_players_; }

    void update();

    // Short name like "Arachnos".
    [[nodiscard]] constexpr auto &short_name() const { return short_name_; }
    // Full description like "{ 5 20} Mahatma Arachnos".
    [[nodiscard]] constexpr auto &description() const { return description_; }
    [[nodiscard]] constexpr auto all_levels() const { return all_levels_; }
    [[nodiscard]] constexpr auto min_level() const { return min_level_; }
    [[nodiscard]] constexpr auto max_level() const { return max_level_; }
    [[nodiscard]] constexpr auto level_difference() const { return max_level_ - min_level_; }
    [[nodiscard]] constexpr auto num() const { return num_; }
    [[nodiscard]] constexpr auto lowest_vnum() const { return lowest_vnum_; }
    [[nodiscard]] constexpr auto highest_vnum() const { return highest_vnum_; }
    [[nodiscard]] constexpr auto &filename() const { return filename_; }

    // NB this is not the same as !occupied - empty is only cleared once a reset has happened with no players inside.
    [[nodiscard]] constexpr auto empty_since_last_reset() const { return empty_since_last_reset_; }
    [[nodiscard]] constexpr auto occupied() const { return num_players_ > 0; }
};
