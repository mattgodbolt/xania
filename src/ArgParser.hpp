#pragma once

#include <optional>
#include <string>
#include <string_view>

class ArgParser {
    std::string storage_;
    std::string_view remaining_;

    void ltrim();

public:
    // Constructing an ArgParser from a string takes a _copy_ of the underlying string. Any string_views handed out by
    // the various parse functions will be references to the copy inside the parser, and will be valid so long as the
    // ArgParser itself is still alive.
    explicit ArgParser(std::string args) noexcept : storage_(std::move(args)), remaining_(storage_) {}
    // Constructing an ArgParser from a string_view or const char* will keep the "reference like" non-copy feel of the
    // string_view. The ArgParser is as valid as the string_view it was constructed from.
    explicit ArgParser(std::string_view args) noexcept : remaining_(args) {}
    explicit ArgParser(const char *args) noexcept : remaining_(args) {}

    // Return whether we're empty (i.e. no more arguments).
    [[nodiscard]] bool empty() const noexcept { return remaining_.empty(); }

    // Return the entirety of the unparsed arguments.
    [[nodiscard]] std::string_view remaining() const noexcept { return remaining_; }

    // Shift a single argument from the argument list. Returns an empty string if the parser is empty.
    // Arguments starting with ' or " are parsed as a single argument including any spaces, which is frequently
    // needed for things like spell names that consist of two words.
    [[nodiscard]] std::string_view shift() noexcept;

    // This is a special case of shift() where the initial character can be a non-alphanumeric character
    // for scenarios where it is valid for a non-alphanumeric to be something to be interpreted.
    [[nodiscard]] std::string_view commandline_shift() noexcept;

    struct NumberArg {
        int number{};
        std::string_view argument;
    };
    // Shift a single argument from the argument list, parsed as a possibly number-prefixed `3.thing`. Returns a {0,""}
    // result if the parser is empty.
    [[nodiscard]] NumberArg shift_numbered_arg() noexcept;

    // Try and shift a numeric argument from the argument list. Remains unshifted if failed, or empty.
    [[nodiscard]] std::optional<int> try_shift_number() noexcept;

    class Iter {
        ArgParser &parser_;
        std::string_view arg_;

    public:
        Iter(ArgParser &parser, std::string_view arg) noexcept : parser_(parser), arg_(arg){};
        std::string_view operator*() const noexcept { return arg_; }
        Iter &operator++() noexcept {
            arg_ = parser_.shift();
            return *this;
        }
        bool operator==(const Iter &rhs) const noexcept { return arg_ == rhs.arg_; }
        bool operator!=(const Iter &rhs) const noexcept { return !(rhs == *this); }

        using iterator_category = std::forward_iterator_tag;
        using value_type = std::string_view;
        using difference_type = ptrdiff_t;
        using pointer = std::string_view *;
        using reference = std::string_view &;
    };

    Iter begin() noexcept { return Iter(*this, shift()); }
    Iter end() noexcept { return Iter(*this, std::string_view()); }
};

inline auto begin(ArgParser &parser) { return parser.begin(); }
inline auto end(ArgParser &parser) { return parser.end(); }