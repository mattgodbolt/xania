/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include "common/StandardBits.hpp"

#include <magic_enum.hpp>
#include <memory>
#include <string>
#include <vector>

class Char;
class ArgParser;

enum class BanFlag { Permanent = A, Newbies = B, Permit = C, Prefix = D, Suffix = E, All = F };

[[nodiscard]] constexpr auto to_int(const BanFlag flag) noexcept { return magic_enum::enum_integer<BanFlag>(flag); }

// Stores the site ban settings for one site.
struct Ban {
    explicit Ban(const std::string site, const int level, const int ban_flags)
        : site_(site), level_(level), ban_flags_(ban_flags) {}
    const std::string site_{};
    const int level_{};
    const int ban_flags_{};
};

// Maintains the database of site bans.
class Bans {
public:
    // Ban file open/close operations sit behind this interface to facilitate testing.
    struct Dependencies {
        virtual ~Dependencies() = default;
        virtual FILE *open_read() = 0;
        virtual FILE *open_write() = 0;
        virtual void close(FILE *fp) = 0;
        virtual void unlink() = 0;
    };

    explicit Bans(Dependencies &dependencies);

    // Returns true if site has the specified ban flag.
    bool check_ban(std::string_view site, const BanFlag ban_flag) const;

    // Returns true if site has the specified ban flag bits.
    bool check_ban(std::string_view site, const int type_bits) const;

    // Sends a list of all banned sites to ch if no args are passed.
    // Otherwise adds a new entry to the list of site bans.
    // If there's an existing ban for the same site that was created by a Char
    // with greater privileges (trust level) the request is rejected.
    // If is_permanent is true, the ban will be saved in system/ban.lst.
    // Returns true if a ban was added.
    bool ban_site(Char *ch, ArgParser args, const bool is_permanent);

    // Removes an entry from the list of site bans based on its position in the list.
    // Returns true if the entry was removed.
    bool allow_site(Char *ch, ArgParser args);
    // Load all bans from system/ban.lst. Returns the number of bans loaded.
    size_t load();
    static Bans &singleton();

private:
    void save();
    void list(Char *ch);
    Dependencies &dependencies_;
    std::vector<std::unique_ptr<const Ban>> bans_;
};