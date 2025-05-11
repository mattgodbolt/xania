/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Position.hpp"
#include "Logging.hpp"
#include "string_utils.hpp"

#include <magic_enum/magic_enum.hpp>
#include <optional>

using namespace std::literals;

std::string fread_word(FILE *fp);
extern int fread_number(FILE *fp, const Logger &logger);

namespace {

constexpr std::array bad_positions{"Lie still; you are DEAD."sv,
                                   "You are hurt far too bad for that."sv,
                                   "You are hurt far too bad for that."sv,
                                   "You are too stunned to do that."sv,
                                   "In your dreams, or what?"sv,
                                   "You're resting. Try standing up first!"sv,
                                   "Better stand up first."sv,
                                   "No way!  You are still fighting!"sv,
                                   "You're standing."sv};

constexpr std::array present_progressive_verbs{"|RDEAD|w!!"sv,
                                               "|Rmortally wounded|w."sv,
                                               "|rincapacitated|w."sv,
                                               "|rlying here stunned|w."sv,
                                               "sleeping here."sv,
                                               "resting here."sv,
                                               "sitting here."sv,
                                               "here, fighting"sv,
                                               "here"sv};

constexpr std::array short_descriptions{"dead"sv,    "mortally wounded"sv, "incapacitated"sv, "stunned"sv, "sleeping"sv,
                                        "resting"sv, "sitting"sv,          "fighting"sv,      "standing"sv};

}

Position::Position() : position_(Type::Standing) {}

Position::Position(const Type position) : position_(position) {}

std::string_view Position::name(const Type position) { return magic_enum::enum_name<Type>(position); }

std::string_view Position::name() const { return name(position_); }

std::string_view Position::short_description() const { return short_descriptions[integer()]; }

ush_int Position::integer() const { return magic_enum::enum_integer<Type>(position_); }

Position::Type Position::type() const { return position_; }

std::string_view Position::bad_position_msg() const { return bad_positions[integer()]; }

std::string_view Position::present_progressive_verb() const { return present_progressive_verbs[integer()]; }

Position &Position::operator=(const Type rhs) {
    position_ = rhs;
    return *this;
}

bool Position::operator<(Type &rhs) const { return position_ < rhs; }

bool Position::operator<=(Type &rhs) const { return position_ <= rhs; }

bool Position::operator==(Type &rhs) const { return position_ == rhs; }

bool Position::operator!=(Type &rhs) const { return position_ != rhs; }

Position::operator Position::Type() const { return position_; }

std::optional<Position> Position::try_from_name(std::string_view name) {
    for (const auto enum_name : magic_enum::enum_names<Position::Type>()) {
        // Uses a partial, case insensitive match in area files mobs typically use a short form of each
        // position e.g. "stand" --> Type::Standing.
        if (matches_start(name, enum_name)) {
            if (const auto val = magic_enum::enum_cast<Position::Type>(enum_name)) {
                return Position(*val);
            }
        }
    }
    return std::nullopt;
}

// Reads the next word from a file stream and converts it into a Position, or the default Position if the
// word can't be parsed.
Position Position::read_from_word(FILE *fp, const Logger &logger) {
    const auto raw_pos = fread_word(fp);
    if (const auto opt_pos = Position::try_from_name(raw_pos)) {
        return *opt_pos;
    } else {
        logger.bug("Unrecognized position: {}, using default.", raw_pos);
        return Position();
    }
}

// Reads the next number from a file stream and converts it into a Position, or the default Position if the
// word can't be parsed.
Position Position::read_from_number(FILE *fp, const Logger &logger) {
    const auto raw_pos = fread_number(fp, logger);
    if (const auto opt_pos = magic_enum::enum_cast<Position::Type>(raw_pos)) {
        return Position(*opt_pos);
    } else {
        logger.bug("Unrecognized position: {}, using default.", raw_pos);
        return Position();
    }
}
