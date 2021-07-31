#pragma once

#include <string>

// tip wizard type - OG Faramir Sep 21 1998
class Tip {
    std::string tip_;

public:
    explicit Tip(std::string tip) : tip_(std::move(tip)) {}

    [[nodiscard]] const std::string &tip() const noexcept { return tip_; }

    [[nodiscard]] static Tip from_file(FILE *fp);
};

void load_tipfile();
void tip_players();
