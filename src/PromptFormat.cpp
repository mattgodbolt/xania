/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2025 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#include "Area.hpp"
#include "Char.hpp"
#include "Room.hpp"

#include <string>

extern unsigned int exp_per_level(const Char *ch, int points);

namespace {

std::string format_one_prompt_part(char c, const Char &ch) {
    switch (c) {
    case '%': return "%";
    case 'B': {
        std::string bar;
        constexpr auto NumGradations = 10u;
        auto num_full = static_cast<float>(NumGradations * ch.hit) / ch.max_hit;
        char prev_colour = 0;
        for (auto loop = 0u; loop < NumGradations; ++loop) {
            auto fullness = num_full - loop;
            if (fullness <= 0.25f)
                bar += ' ';
            else {
                char colour = 'g';
                if (loop < NumGradations / 3)
                    colour = 'r';
                else if (loop < (2 * NumGradations) / 3)
                    colour = 'y';
                if (colour != prev_colour) {
                    bar = bar + '|' + colour;
                    prev_colour = colour;
                }
                if (fullness <= 0.5f)
                    bar += ".";
                else if (fullness < 1.f)
                    bar += ":";
                else
                    bar += "||";
            }
        }
        return bar + "|p";
    }

    case 'c': return fmt::format("{}", ch.wait);
    case 'h': return fmt::format("{}", ch.hit);
    case 'H': return fmt::format("{}", ch.max_hit);
    case 'm': return fmt::format("{}", ch.mana);
    case 'M': return fmt::format("{}", ch.max_mana);
    case 'g': return fmt::format("{}", ch.gold);
    case 'x':
        return fmt::format("{}",
                           (long int)(((long int)(ch.level + 1) * (long int)(exp_per_level(&ch, ch.pcdata->points))
                                       - (long int)(ch.exp))));
    case 'v': return fmt::format("{}", ch.move);
    case 'V': return fmt::format("{}", ch.max_move);
    case 'a':
        if (ch.get_trust() > 10)
            return fmt::format("{}", ch.alignment);
        else
            return "??";
    case 'X': return fmt::format("{}", ch.exp);
    case 'p':
        if (ch.player())
            return ch.player()->pcdata->prefix;
        return "";
    case 'r':
        if (ch.is_immortal())
            return ch.in_room ? ch.in_room->name : "";
        break;
    case 'W':
        if (ch.is_immortal())
            return ch.is_wizinvis() ? "|R*W*|p" : "-w-";
        break;
        /* Can be changed easily for tribes/clans etc. */
    case 'P':
        if (ch.is_immortal())
            return ch.is_prowlinvis() ? "|R*P*|p" : "-p-";
        break;
    case 'R':
        if (ch.is_immortal())
            return fmt::format("{}", ch.in_room ? ch.in_room->vnum : 0);
        break;
    case 'z':
        if (ch.is_immortal())
            return ch.in_room ? ch.in_room->area->short_name() : "";
        break;
    case 'n': return "\n\r";
    case 't': {
        auto ch_timet = Clock::to_time_t(ch.mud_.current_time());
        char time_buf[MAX_STRING_LENGTH];
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", gmtime(&ch_timet));
        return time_buf;
    }
    default: break;
    }

    return "|W ?? |p";
}

}

std::string format_prompt(const Char &ch, std::string_view prompt) {
    if (prompt.empty()) {
        return fmt::format("|p<{}/{}hp {}/{}m {}mv {}cd> |w", ch.hit, ch.max_hit, ch.mana, ch.max_mana, ch.move,
                           ch.wait);
    }

    bool prev_was_escape = false;
    std::string buf = "|p";
    for (auto c : prompt) {
        if (std::exchange(prev_was_escape, false)) {
            buf += format_one_prompt_part(c, ch);
        } else if (c == '%')
            prev_was_escape = true;
        else
            buf += c;
    }
    return buf + "|w";
}
