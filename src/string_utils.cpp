#include "string_utils.hpp"

#include "ArgParser.hpp"

#include <fmt/format.h>
#include <gsl/gsl_util>
#include <range/v3/algorithm/all_of.hpp>
#include <range/v3/algorithm/search.hpp>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/drop.hpp>
#include <range/v3/view/take.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/zip.hpp>

#include <algorithm>
#include <array>
#include <range/v3/view/concat.hpp>

using namespace std::literals;

constexpr std::array Vowels{'A', 'E', 'I', 'O', 'U', 'a', 'e', 'i', 'o', 'u'};

int parse_number(std::string_view sv) {
    if (sv.empty())
        return 0;
    bool is_neg = sv.front() == '-';
    if (sv.front() == '-' || sv.front() == '+')
        sv.remove_prefix(1);
    if (sv.empty())
        return 0;
    unsigned long long intermediate = 0;
    for (auto c : sv) {
        if (!isdigit(c))
            return 0;
        intermediate = intermediate * 10u + (c - '0');
    }
    return static_cast<int>(intermediate) * (is_neg ? -1 : 1);
}

bool is_number(std::string_view sv) {
    if (sv.empty())
        return false;

    if (sv.front() == '-' || sv.front() == '+')
        sv.remove_prefix(1);

    return ranges::all_of(sv, isdigit);
}

std::pair<int, std::string_view> number_argument(std::string_view argument) {
    if (auto dot = argument.find_first_of('.'); dot != std::string_view::npos)
        return {parse_number(argument.substr(0, dot)), argument.substr(dot + 1)};
    return {1, argument};
}

bool is_vowel(const char c) { return std::find(Vowels.begin(), Vowels.end(), c) != Vowels.end(); }

std::string smash_tilde(std::string_view str) {
    return str | ranges::view::transform([](char c) { return c == '~' ? '-' : c; }) | ranges::to<std::string>;
}

std::string replace_strings(std::string message, std::string_view from_str, std::string_view to_str) {
    if (from_str.empty()) {
        return message;
    }
    size_t pos = 0;
    while ((pos = message.find(from_str, pos)) != std::string::npos) {
        message.replace(pos, from_str.length(), to_str);
        pos += to_str.length();
    }
    return message;
}

namespace {
const auto not_space = [](char ch) { return !std::isspace(ch); };
}

std::string_view ltrim(std::string_view str) {
    const auto begin = std::find_if(str.begin(), str.end(), not_space);
    return {begin, static_cast<size_t>(str.end() - begin)};
}

std::string_view trim(std::string_view str) {
    const auto begin = std::find_if(str.begin(), str.end(), not_space);
    const auto rit = std::find_if(str.rbegin(), std::make_reverse_iterator(begin), not_space);
    return {begin, static_cast<size_t>(rit.base() - begin)};
}

std::string reduce_spaces(std::string_view str) {
    bool previous_was_space{false};
    bool past_leading_spaces{false};
    std::string result;
    result.reserve(str.size());
    for (auto c : str) {
        if (std::isspace(c)) {
            previous_was_space = true;
        } else {
            if (std::exchange(previous_was_space, false) && past_leading_spaces) {
                result.push_back(' ');
            }
            result.push_back(c);
            past_leading_spaces = true;
        }
    }
    return result;
}

std::string remove_last_line(std::string_view str) {
    static constexpr auto line_ending = "\n\r"sv;
    // If long enough to have a crlf...
    if (str.length() > line_ending.size()) {
        // If it ends with CRLF, ignore that in looking for a previous line.
        if (str.substr(str.length() - line_ending.size()) == line_ending)
            str.remove_suffix(2);
        if (auto last_crlf = str.rfind(line_ending); last_crlf != std::string_view::npos)
            return std::string(str.substr(0, last_crlf + line_ending.size()));
    }
    // Else, no crlf at all, return empty.
    return "";
}

std::string sanitise_input(std::string_view str) {
    std::string result;
    result.reserve(str.size());
    // Copy to result, applying any backspaces (\b) as we go. We don't combine this with dropping non-printing else
    // users typing, then deleting bad chars get confusing results.
    for (auto c : str) {
        if (c == '\b') {
            if (!result.empty())
                result.pop_back();
        } else {
            result.push_back(c);
        }
    }

    // Remove non-printing, non-ascii characters. This awkward erase(remove_if,,,) idiom is a uniquely C++ mess.
    result.erase(std::remove_if(result.begin(), result.end(), [](char c) { return !isascii(c) || !isprint(c); }),
                 result.end());

    return result;
}

impl::LineSplitter::Iter::Iter(std::string_view text, bool first) {
    // Handles all cases of `\n`, `\r`, `\r\n` and `\n\r`.
    if (auto cr_or_lf = text.find_first_of("\r\n"); cr_or_lf != std::string_view::npos) {
        line_ = text.substr(0, cr_or_lf);
        rest_ = text.substr(cr_or_lf + 1);
        // Fix up the "other" pair of a crlf or lfcr pair.
        if (!rest_.empty() && rest_[0] != text[cr_or_lf] && (rest_[0] == '\n' || rest_[0] == '\r'))
            rest_.remove_prefix(1);
    } else {
        line_ = text;
    }
    at_end_ = line_.empty() && rest_.empty() && !first;
}

impl::LineSplitter::Iter impl::LineSplitter::Iter::operator++() noexcept {
    auto prev = *this;
    *this = Iter(rest_, false);
    return prev;
}

std::string lower_case(std::string_view str) {
    return str | ranges::view::transform(tolower) | ranges::to<std::string>;
}

bool has_prefix(std::string_view haystack, std::string_view needle) {
    if (needle.size() > haystack.size())
        return false;
    return needle == haystack.substr(0, needle.size());
}

std::string decode_colour(bool ansi_enabled, char char_code) {
    char send_colour;
    switch (char_code) {
    case 'r':
    case 'R': send_colour = '1'; break;
    case 'g':
    case 'G': send_colour = '2'; break;
    case 'y':
    case 'Y': send_colour = '3'; break;
    case 'b':
    case 'B': send_colour = '4'; break;
    case 'm':
    case 'p':
    case 'M':
    case 'P': send_colour = '5'; break;
    case 'c':
    case 'C': send_colour = '6'; break;
    case 'w':
    case 'W': send_colour = '7'; break;
    case '|': return "|";
    default: return std::string(1, char_code); // NOLINT(modernize-return-braced-init-list)
    }
    if (ansi_enabled)
        return fmt::format("\033[{};3{}m", char_code >= 'a' ? '0' : '1', send_colour);
    return "";
}

std::string colourise_mud_string(bool use_ansi, std::string_view text) {
    std::string buf;
    bool prev_was_pipe = false;
    for (auto c : text) {
        if (std::exchange(prev_was_pipe, false))
            buf += decode_colour(use_ansi, c);
        else if (c == '|')
            prev_was_pipe = true;
        else
            buf.push_back(c);
    }
    return buf;
}

size_t mud_string_width(std::string_view text) {
    size_t width = 0;
    bool prev_was_pipe = false;
    for (auto c : text) {
        if (std::exchange(prev_was_pipe, false))
            width += decode_colour(false, c).size();
        else if (c == '|')
            prev_was_pipe = true;
        else
            width++;
    }
    return width;
}

std::string upper_first_character(std::string_view sv) {
    std::string result(sv);
    // Uppercase the first non-colour-sequence letter.
    bool skip_next{false};
    for (auto &c : result) {
        if (std::exchange(skip_next, false))
            continue;
        if (c == '|')
            skip_next = true;
        else {
            c = gsl::narrow_cast<char>(toupper(c));
            break;
        }
    }
    return result;
}

std::string initial_caps_only(std::string_view text) {
    return ranges::view::concat(text | ranges::view::take(1) | ranges::view::transform(toupper),
                                text | ranges::view::drop(1) | ranges::view::transform(tolower))
           | ranges::to<std::string>;
}

bool matches(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() != rhs.size())
        return false;
    return ranges::all_of(ranges::zip_view(lhs, rhs), [](auto pr) { return tolower(pr.first) == tolower(pr.second); });
}

bool matches_start(std::string_view lhs, std::string_view rhs) {
    if (lhs.size() > rhs.size() || lhs.empty())
        return false;
    return matches(lhs, rhs.substr(0, lhs.size()));
}

bool matches_inside(std::string_view needle, std::string_view haystack) {
    auto needle_low = needle | ranges::views::transform(tolower);
    auto haystack_low = haystack | ranges::views::transform(tolower);
    return !ranges::search(haystack_low, needle_low).empty();
}

namespace {
enum class PartMatch { Complete, Partial, None };
PartMatch match_one_name_part(std::string_view entire, std::string_view part, std::string_view namelist) {
    for (auto name : ArgParser(namelist)) {
        // Is the entire input string a match for this name?
        if (matches(entire, name))
            return PartMatch::Complete;

        // if this part matches a prefix, then we have at least a partial match.
        if (matches_start(part, name))
            return PartMatch::Partial;
    }
    // Else, this part did not match anything in the namelist.
    return PartMatch::None;
}
}

bool is_name(std::string_view str, std::string_view namelist) {
    // We need ALL parts of string to match part of namelist, or for the string to match one of the names exactly.
    for (auto part : ArgParser(str)) {
        switch (match_one_name_part(str, part, namelist)) {
        case PartMatch::Complete:
            // A complete match on any one part means this is a match, regardless of the other parts.
            return true;
        case PartMatch::Partial:
            // A partial match means we keep going to check all the other parts match partially.
            break;
        case PartMatch::None:
            // No match at all means we did not match.
            return false;
        }
    }

    // If we got here, every part matched partially, so we consider this a name.
    return true;
}

std::string lower_case_articles(std::string_view text) {
    auto result = std::string(text);
    if (has_prefix(text, "The ") || has_prefix(text, "A ") || has_prefix(text, "An ")) {
        result[0] = gsl::narrow_cast<char>(tolower(result[0]));
    }
    return result;
}
