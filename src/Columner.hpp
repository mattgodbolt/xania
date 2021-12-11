/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Char.hpp"
#include "string_utils.hpp"

class Columner {
    Char &ch_;
    std::string current_;
    int cur_col_{};
    int num_cols_{};
    int col_size_{};

    Columner &add_col(const std::string &column) {
        if (cur_col_ == 0) {
            current_ = column;
        } else {
            auto num_spaces = std::max(1, cur_col_ * col_size_ - static_cast<int>(mud_string_width(current_)));
            current_ += std::string(num_spaces, ' ') + column;
        }
        if (++cur_col_ == num_cols_)
            flush();
        return *this;
    }

public:
    explicit Columner(Char &ch, int num_cols, int col_size = 24) : ch_(ch), num_cols_(num_cols), col_size_(col_size) {}
    ~Columner() { flush(); }
    Columner(const Columner &) = delete;
    Columner &operator=(const Columner &) = delete;
    Columner(Columner &&) = delete;
    Columner &operator=(Columner &&) = delete;

    template <typename... Args>
    Columner &add(fmt::string_view txt, Args &&... args) {
        return add_col(fmt::vformat(txt, fmt::make_format_args(args...)));
    }
    template <typename... Args>
    Columner &kv(std::string_view key, fmt::string_view value_fmt, Args &&... args) {
        return add_col(fmt::format("|C{}|w: {}", key, fmt::vformat(value_fmt, fmt::make_format_args(args...))));
    }
    template <typename StatVal>
    Columner &stat(std::string_view stat, StatVal val) {
        return kv(stat, "|W{}|w", val);
    }
    template <typename StatVal, typename MaxVal>
    Columner &stat_of(std::string_view stat, StatVal val, MaxVal max) {
        return kv(stat, "|W{}|w / {}", val, max);
    }
    template <typename StatVal, typename MaxVal>
    Columner &stat_eff(std::string_view stat, StatVal val, MaxVal max) {
        return kv(stat, "{} (|W{}|w)", val, max);
    }
    void flush() {
        if (current_.empty())
            return;
        ch_.send_line(current_);
        current_.clear();
        cur_col_ = 0;
    }
};
