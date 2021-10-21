#pragma once

#include <memory>
#include <vector>

class Area;

class AreaList {
    std::vector<std::unique_ptr<Area>> areas_;

public:
    AreaList();
    ~AreaList();

    void add(std::unique_ptr<Area> area) { areas_.emplace_back(std::move(area)); }
    void sort();
    [[nodiscard]] auto begin() const { return areas_.begin(); }
    [[nodiscard]] auto end() const { return areas_.end(); }

    [[nodiscard]] Area *back() const {
        if (areas_.empty())
            return nullptr;
        return areas_.back().get();
    }

    [[nodiscard]] size_t count() const noexcept { return areas_.size(); }

    static AreaList &singleton();
};
