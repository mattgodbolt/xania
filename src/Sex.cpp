#include "Sex.hpp"
#include "string_utils.hpp"

#include <fmt/ranges.h>
#include <magic_enum.hpp>
#include <optional>

namespace {

Sex::Type operator+=(Sex::Type &sex, const int mod) {
    if (mod == 0) {
        // nothing to do
        return sex;
    }
    auto val = magic_enum::enum_integer<Sex::Type>(sex);
    const ush_int count = magic_enum::enum_count<Sex::Type>();
    // As modifiers can be negative and taking the modulus of a negative
    // value can result in a negative, we have to flip the result around.
    auto result = (val + mod) % count;
    if (result < 0) {
        result += count;
    }
    sex = *magic_enum::enum_cast<Sex::Type>(result);
    return sex;
}

}

Sex::Sex() : sex_(Type::neutral) {}

Sex::Sex(Type sex) : sex_(sex) {}

// Apply a change to the sex attribute. This can be positive or negative. Either way
// it will result in the attribute being altered (if mod is non-zero).
Sex Sex::operator+=(const int mod) {
    sex_ += mod;
    return *this;
}

Sex Sex::operator+(const int mod) {
    sex_ += mod;
    return *this;
}

bool Sex::operator==(const Sex &rhs) const { return sex_ == rhs.sex_; }

bool Sex::operator!=(const Sex &rhs) const { return sex_ != rhs.sex_; }

bool Sex::is_neutral() const { return sex_ == Type::neutral; }

bool Sex::is_male() const { return sex_ == Type::male; }

bool Sex::is_female() const { return sex_ == Type::female; }

std::string_view Sex::name(const Type sex) { return magic_enum::enum_name<Type>(sex); }

std::string_view Sex::name() const { return name(sex_); }

ush_int Sex::integer() const { return magic_enum::enum_integer<Sex::Type>(sex_); }

Sex::Type Sex::type() const { return sex_; }

Sex Sex::male() { return Sex(Type::male); }

Sex Sex::female() { return Sex(Type::female); }

Sex Sex::neutral() { return Sex(Type::neutral); }

std::optional<Sex> Sex::try_from_name(std::string_view name) {
    if (matches_start(name, Sex::name(Type::neutral))) {
        return neutral();
    } else if (matches_start(name, Sex::name(Type::male))) {
        return male();
    } else if (matches_start(name, Sex::name(Type::female))) {
        return female();
    } else {
        return std::nullopt;
    }
}

std::optional<Sex> Sex::try_from_integer(const int sex) {
    if (auto enum_val = magic_enum::enum_cast<Type>(sex)) {
        return Sex(*enum_val);
    } else {
        return std::nullopt;
    }
}

std::optional<Sex> Sex::try_from_char(const char c) {
    switch (c) {
    case 'm':
    case 'M': return male();
    case 'f':
    case 'F': return female();
    case 'o':
    case 'O':
    case 'n':
    case 'N': return neutral();
    default: return std::nullopt;
    }
}

std::string Sex::names_csv() {
    constexpr auto &names = magic_enum::enum_names<Type>();
    return fmt::format("{}", fmt::join(names, ", "sv));
}
