#pragma once

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

    std::string areaname;
    std::string filename;
    int lvnum{};
    int uvnum{};
    int vnum{};
    unsigned int area_flags{};
};

class AreaList {
    std::vector<std::unique_ptr<AREA_DATA>> areas_;

public:
    void add(std::unique_ptr<AREA_DATA> area) { areas_.emplace_back(std::move(area)); }
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
