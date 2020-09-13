#include "Dice.hpp"

#include "Logging.hpp"
#include "db.h"

Dice Dice::from_file(FILE *fp) {
    auto number = fread_number(fp);
    auto d = fread_letter(fp);
    auto type = fread_number(fp);
    auto plus = fread_letter(fp);
    auto bonus = fread_number(fp);
    if (number <= 0)
        bug("Bad dice: number <= 0");
    if (type <= 0)
        bug("Bad dice: type <= 0");
    if (d != 'd' && d != 'D')
        bug("Bad dice: expected 'd' or 'D', got '%c'", d);
    if (plus != '+')
        bug("Bad dice: expected '+', got '%c'", plus);
    return Dice(number, type, bonus);
}

bool Dice::operator==(const Dice &rhs) const {
    return std::tie(number_, type_, bonus_) == std::tie(rhs.number_, rhs.type_, rhs.bonus_);
}

bool Dice::operator!=(const Dice &rhs) const { return !(rhs == *this); }
