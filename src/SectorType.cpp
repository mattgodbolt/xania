#include "SectorType.hpp"

std::optional<SectorType> try_get_sector_type(int value) {
    if (value >= 0 && value < static_cast<int>(SectorType_Max))
        return static_cast<SectorType>(value);
    return {};
}

std::string_view to_string(SectorType type) {
    switch (type) {
    case SectorType::Inside: return "inside";
    case SectorType::City: return "city";
    case SectorType::Field: return "field";
    case SectorType::Forest: return "forest";
    case SectorType::Hills: return "hills";
    case SectorType::Mountain: return "mountain";
    case SectorType::SwimmableWater: return "water (swimmable)";
    case SectorType::NonSwimmableWater: return "water (non-swimmable)";
    case SectorType::Air: return "air";
    case SectorType::Desert: return "desert";
    }
    return "(unknown)";
}
