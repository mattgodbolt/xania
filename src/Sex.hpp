#pragma once
#include "Types.hpp"
#include <optional>
#include <string>

using namespace std::literals;

class Sex {
public:
    enum class Type : ush_int { neutral, male, female };
    Sex();
    explicit Sex(Type sex);
    Sex operator+=(int mod);
    Sex operator+(int mod);
    bool operator==(const Sex &rhs) const;
    bool operator!=(const Sex &rhs) const;
    [[nodiscard]] bool is_neutral() const;
    [[nodiscard]] bool is_male() const;
    [[nodiscard]] bool is_female() const;
    [[nodiscard]] std::string_view name() const;
    [[nodiscard]] ush_int integer() const;
    [[nodiscard]] Type type() const;

    [[nodiscard]] static Sex male();
    [[nodiscard]] static Sex female();
    [[nodiscard]] static Sex neutral();
    [[nodiscard]] static std::optional<Sex> try_from_name(std::string_view name);
    [[nodiscard]] static std::optional<Sex> try_from_integer(const int sex);
    [[nodiscard]] static std::optional<Sex> try_from_char(const char c);
    [[nodiscard]] static std::string names_csv();

private:
    static std::string_view name(Type sex);

    Type sex_;
};
