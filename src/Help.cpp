#include "Help.hpp"

#include "AreaData.hpp"
#include "db.h"
#include "string_utils.hpp"

#include <range/v3/algorithm/find_if.hpp>

#include <vector>

using namespace std::literals;

std::optional<Help> Help::load(FILE *fp, const AreaData *area) {
    auto level = fread_number(fp);
    auto keyword = fread_stdstring(fp);
    if (matches_start("$", keyword))
        return {};
    auto text = fread_stdstring(fp);
    // Strip leading '.' to allow initial blanks.
    if (!text.empty() && text.front() == '.')
        text = text.substr(1);
    return Help(area, level, std::move(keyword), std::move(text));
}

bool Help::matches(sh_int level, std::string_view keyword) const noexcept {
    return level_ <= level && is_name(keyword, keyword_);
}
bool Help::operator==(const Help &rhs) const {
    return std::tie(area_, level_, keyword_, text_) == std::tie(rhs.area_, rhs.level_, rhs.keyword_, rhs.text_);
}
bool Help::operator!=(const Help &rhs) const { return !(rhs == *this); }

std::string_view Help::area_name() const noexcept { return area_ ? area_->short_name() : "(no area)"sv; }

HelpList &HelpList::singleton() {
    static HelpList singleton;
    return singleton;
}

const Help *HelpList::lookup(int level, std::string_view keyword) const noexcept {
    if (auto it = ranges::find_if(helps_, [&](const Help &h) { return h.matches(level, keyword); }); it != helps_.end())
        return &*it;
    return nullptr;
}
