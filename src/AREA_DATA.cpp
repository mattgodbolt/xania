#include "AREA_DATA.hpp"

AreaList &AreaList::singleton() {
    static AreaList singleton;
    return singleton;
}
