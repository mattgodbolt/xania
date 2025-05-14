#pragma once

#include "Logging.hpp"
#include "Types.hpp"

#include <fmt/core.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

class Area;

class HelpList;

class Help {
    const Area *area_;
    sh_int level_;
    std::string keyword_;
    std::string text_;

public:
    Help(const Area *area, sh_int level, std::string keyword, std::string text)
        : area_(area), level_(level), keyword_(std::move(keyword)), text_(std::move(text)) {}

    [[nodiscard]] sh_int level() const noexcept { return level_; }
    [[nodiscard]] const std::string &keyword() const noexcept { return keyword_; }
    [[nodiscard]] const std::string &text() const noexcept { return text_; }
    [[nodiscard]] const Area *area() const noexcept { return area_; }
    // Returns "(no area)" if...no area
    [[nodiscard]] std::string_view area_name() const noexcept;
    [[nodiscard]] bool matches(sh_int level, std::string_view keyword) const noexcept;

    bool operator==(const Help &rhs) const;
    bool operator!=(const Help &rhs) const;

    static std::optional<Help> load(FILE *fp, const Area *area, const Logger &logger);
};

class HelpList {
    std::vector<Help> helps_;

public:
    void add(Help help) { helps_.emplace_back(std::move(help)); }
    [[nodiscard]] const Help *lookup(int level, std::string_view keyword) const noexcept;
    [[nodiscard]] size_t count() const noexcept { return helps_.size(); }
};

template <>
struct fmt::formatter<Help> {
    constexpr auto parse(format_parse_context &ctx) { return ctx.begin(); }
    template <typename FormatContext>
    auto format(const Help &help, FormatContext &ctx) {
        return fmt::format_to(ctx.out(), "Help({}, {}, {}, {})", help.area_name(), help.level(), help.keyword(),
                              help.text());
    }
};
