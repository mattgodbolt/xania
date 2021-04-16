#include "pfu.hpp"
#include "VnumRooms.hpp"
#include "WrappedFd.hpp"
#include "common/Configuration.hpp"
#include "common/Time.hpp"
#include "handler.hpp"
#include "merc.h"
#include "save.hpp"

#include <dirent.h>
#include <magic_enum.hpp>
#include <optional>
#include <range/v3/algorithm/fill.hpp>
#include <range/v3/view/filter.hpp>
#include <spdlog/spdlog.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace pfu {

// Register new tasks here. Ideally in ascending order of CharVersion. You can have multiple
// tasks for any given version.
Tasks register_tasks() {
    Tasks all_tasks;
    all_tasks.emplace_back(std::make_unique<ResetModifiableAttrs>(CharVersion::Five));
    return all_tasks;
}

std::vector<std::string> collect_player_names(const std::string &player_dir, Logger &logger) {
    DIR *dir = opendir(player_dir.c_str());
    if (!dir) {
        logger.critical("Couldn't open player directory: {}", strerror(errno));
        return {};
    }
    std::vector<std::string> names;
    struct dirent *entry;
    struct stat st;
    while ((entry = readdir(dir))) {
        if (entry->d_type == DT_REG) {
            std::string filename = player_dir + entry->d_name;
            if (!stat(filename.c_str(), &st) && st.st_size > 0) {
                names.emplace_back(entry->d_name);
            } else {
                logger.warn("Ignoring bad player file: {}", filename);
            }
        }
    }
    closedir(dir);
    return names;
}

void upgrade_players(const std::vector<std::string> &players, Logger &logger) {
    auto all_tasks = register_tasks();
    auto succeeded = 0;
    auto failed = 0;
    const CharSaver saver;
    for (auto &player : players) {
        if (upgrade_player(player, all_tasks, logger, [&saver](const Char &ch) { saver.save(ch); })) {
            succeeded++;
        } else {
            failed++;
        }
    }
    logger.info("Processed characters: {} succeeded, {} failed.", succeeded, failed);
}

std::optional<Time> try_parse_login_at(std::string &login_at) {
    if (login_at.empty()) {
        return std::nullopt;
    }
    std::tm tm_login;
    // Includes older formats used for login time.
    for (const auto format : {"%Y-%m-%d %H:%M:%SZ", "%Y-%m-%d %H:%M:%S", "%a %b %d %H:%M:%S %Y"}) {
        memset(&tm_login, 0, sizeof(std::tm));
        if (strptime(login_at.c_str(), format, &tm_login)) {
            return Clock::from_time_t(mktime(&tm_login));
        }
    }
    return std::nullopt;
}

// Reset/repair Char attributes.
ResetModifiableAttrs::ResetModifiableAttrs(CharVersion version) : UpgradeTask(version) {}

// This method replaces the old handler.cpp:reset_char(). Originally it was executed
// every time a char logged in and it reset the char's armour and attributes
// to a healthy starting value before applying modifiers from spell and object "affects". Unclear why
// this was introduced, but it was probably to fix "attribute drift" e.g. where the modifiers of a spell affect
// got changed in the code without retrospectively adjusting the player's stats.
void ResetModifiableAttrs::execute(Char &ch) const {
    OBJ_DATA *obj;
    // Restore the character to his/her true condition
    // All of the attributes reset here are ones that can be modified in AFFECT_DATA.cpp
    ranges::fill(ch.mod_stat, 0);
    ch.max_hit = ch.pcdata->perm_hit;
    ch.max_mana = ch.pcdata->perm_mana;
    ch.max_move = ch.pcdata->perm_move;
    // #216 -1 is initial value for armour now
    ranges::fill(ch.armor, -1);
    ch.hitroll = 0;
    ch.damroll = 0;
    ch.saving_throw = 0;
    ch.sex = ch.pcdata->true_sex;
    // add back object effects
    for (auto loc = 0; loc < MAX_WEAR; loc++) {
        obj = get_eq_char(&ch, loc);
        if (!obj)
            continue;
        for (size_t i = 0; i < ch.armor.size(); i++)
            ch.armor[i] -= apply_ac(obj, loc, i);

        if (!obj->enchanted)
            for (const auto &af : obj->pIndexData->affected)
                af.apply(ch);

        for (auto &af : obj->affected)
            af.apply(ch);
    }
    // add back spell effects
    for (auto &af : ch.affected)
        af.apply(ch);
}

CharUpgraderResult::CharUpgraderResult() : ch_(nullptr), upgraded_(false) {}

CharUpgraderResult::CharUpgraderResult(Char *ch, bool upgraded) : ch_(ch), upgraded_(upgraded) {}

CharUpgraderResult::~CharUpgraderResult() {
    if (ch_) {
        extract_char(ch_, true);
    }
}

CharUpgraderResult::operator bool() const noexcept { return ch_ && !ch_->name.empty(); }

CharUpgraderResult::operator Char &() const noexcept { return *ch_; }

CharUpgrader::CharUpgrader(std::string_view name, const Tasks &all_tasks, Logger &logger)
    : name_(name), desc_(Descriptor(0)), all_tasks_(all_tasks), logger_(logger) {}

auto CharUpgrader::required_tasks(const Char &ch) const {
    return all_tasks_ | ranges::views::filter([&ch](const auto &upgrade) { return upgrade->is_required(ch); });
}

CharUpgraderResult CharUpgrader::upgrade() {
    if (auto ch = simulate_login()) {
        bool upgraded{};
        logger_.debug("{}", name_);
        // Chars that have an malformed pfile will have been created and added
        // to the world, but no tasks should be run. Released when the result is destroyed.
        if (!ch->name.empty()) {
            for (auto &task : required_tasks(*ch)) {
                task->execute(*ch);
                upgraded = true;
            }
        } else
            logger_.warn("Invalid player file: {}", name_);
        return {ch, upgraded};
    }
    return {};
}

Char *CharUpgrader::simulate_login() {
    if (auto fp = WrappedFd::open(filename_for_player(name_))) {
        LastLoginInfo last_login;
        // This snippet is very similar to what's in nanny() and try_load_player().
        Char *ch = new Char();
        ch->pcdata = std::make_unique<PcData>();
        load_into_char(*ch, last_login, fp);
        char_list.add_front(ch);
        desc_.character(ch);
        ch->desc = &desc_;
        // Retain the original login from & time if possible.
        if (!last_login.login_from.empty()) {
            desc_.host(last_login.login_from);
        }
        auto parsed_time = try_parse_login_at(last_login.login_at);
        if (parsed_time) {
            desc_.login_time(*parsed_time);
        }
        if (ch->in_room) {
            char_to_room(ch, ch->in_room);
        } else {
            char_to_room(ch, get_room_index(rooms::MidgaardTemple));
        }
        if (ch->pet) {
            char_to_room(ch->pet, ch->in_room);
        }
        return ch;
    } else
        return nullptr;
}

}
