#pragma once

#include <memory>
#include <vector>

class AreaData;

class AreaList {
    std::vector<std::unique_ptr<AreaData>> areas_;

public:
    AreaList();
    ~AreaList();

    void add(std::unique_ptr<AreaData> area) { areas_.emplace_back(std::move(area)); }
    void sort();
    [[nodiscard]] auto begin() const { return areas_.begin(); }
    [[nodiscard]] auto end() const { return areas_.end(); }

    [[nodiscard]] AreaData *back() const {
        if (areas_.empty())
            return nullptr;
        return areas_.back().get();
    }

    [[nodiscard]] size_t count() const noexcept { return areas_.size(); }

    static AreaList &singleton();
};
