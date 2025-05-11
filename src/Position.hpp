/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/

#pragma once

#include "Logging.hpp"
#include "Types.hpp"

#include <optional>
#include <string_view>

class Position {
public:
    enum class Type : ush_int {
        Dead = 0,
        Mortal = 1,
        Incap = 2,
        Stunned = 3,
        Sleeping = 4,
        Resting = 5,
        Sitting = 6,
        Fighting = 7,
        Standing = 8,
    };

    Position();
    explicit Position(const Type position);
    [[nodiscard]] std::string_view name() const;
    [[nodiscard]] std::string_view short_description() const;
    [[nodiscard]] ush_int integer() const;
    [[nodiscard]] Type type() const;
    [[nodiscard]] std::string_view present_progressive_verb() const;
    [[nodiscard]] std::string_view bad_position_msg() const;
    Position &operator=(const Type rhs);
    [[nodiscard]] bool operator<(Type &rhs) const;
    [[nodiscard]] bool operator<=(Type &rhs) const;
    [[nodiscard]] bool operator==(Type &rhs) const;
    [[nodiscard]] bool operator!=(Type &rhs) const;
    [[nodiscard]] operator Position::Type() const;

    [[nodiscard]] static std::optional<Position> try_from_name(std::string_view name);
    [[nodiscard]] static Position read_from_word(FILE *fp, const Logger &logger);
    [[nodiscard]] static Position read_from_number(FILE *fp, const Logger &logger);

private:
    static std::string_view name(const Type position);

    Type position_;
};
