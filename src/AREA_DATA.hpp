#pragma once

#include "Constants.hpp"
#include "Types.hpp"

#include <memory>
#include <string>
#include <vector>

// Area definition.
struct AREA_DATA {
    std::string name;

    sh_int age{};
    sh_int nplayer{};
    bool empty{};
    bool all_levels{};
    int min_level{0};
    int max_level{MAX_LEVEL};
    int level_difference;
    std::string areaname;
    std::string filename;
    int lvnum{};
    int uvnum{};
    int area_num{};
    unsigned int area_flags{};
};

class AreaList {
    std::vector<std::unique_ptr<AREA_DATA>> areas_;

public:
    void add(std::unique_ptr<AREA_DATA> area) { areas_.emplace_back(std::move(area)); }
    void sort();
    [[nodiscard]] auto begin() const { return areas_.begin(); }
    [[nodiscard]] auto end() const { return areas_.end(); }

    [[nodiscard]] AREA_DATA *back() const {
        if (areas_.empty())
            return nullptr;
        return areas_.back().get();
    }

    [[nodiscard]] size_t count() const noexcept { return areas_.size(); }

    static AreaList &singleton();
};
