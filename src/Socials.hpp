/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "string_utils.hpp"
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

class Char;
/*
 * Structure for a social.
 * Every social can have up to 7 different contextual phrases. There's an 8th (char_not_found) but
 * that one is ignored on load at the moment. In the socials area file format, each social starts
 * with a single word that's the name of the social. Followed by 8 phrases then a terminating # that
 * is required. The end of the socials section is indicated by #0.
 */
class Social {
public:
    Social(std::string name, std::string char_no_arg, std::string others_no_arg, std::string char_found,
           std::string others_found, std::string vict_found, std::string char_auto, std::string others_auto)
        : name_(std::move(name)), char_no_arg_(std::move(char_no_arg)), others_no_arg_(std::move(others_no_arg)),
          char_found_(std::move(char_found)), others_found_(std::move(others_found)),
          vict_found_(std::move(vict_found)), char_auto_(std::move(char_auto)), others_auto_(std::move(others_auto)) {}
    Social(std::string_view name) : name_(name) {}

    [[nodiscard]] static std::optional<Social> load(FILE *fp);

    [[nodiscard]] std::string_view name() const noexcept { return name_; }
    [[nodiscard]] std::string_view char_no_arg() const noexcept { return char_no_arg_; }
    [[nodiscard]] std::string_view others_no_arg() const noexcept { return others_no_arg_; }
    [[nodiscard]] std::string_view char_found() const noexcept { return char_found_; }
    [[nodiscard]] std::string_view others_found() const noexcept { return others_found_; }
    [[nodiscard]] std::string_view vict_found() const noexcept { return vict_found_; }
    [[nodiscard]] std::string_view char_auto() const noexcept { return char_auto_; }
    [[nodiscard]] std::string_view others_auto() const noexcept { return others_auto_; }

private:
    std::string name_;
    std::string char_no_arg_;
    std::string others_no_arg_;
    std::string char_found_;
    std::string others_found_;
    std::string vict_found_;
    std::string char_auto_;
    std::string others_auto_;
};

class Socials {
public:
    void load(FILE *fp);
    [[nodiscard]] const Social *find(std::string_view name) const noexcept;
    [[nodiscard]] size_t count() const noexcept;
    [[nodiscard]] const Social &random() const noexcept;
    void show_table(Char *ch) const noexcept;

    static Socials &singleton();

private:
    std::vector<Social> socials_;
};
