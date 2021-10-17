#pragma once

#include "Char.hpp"

#include <spdlog/spdlog.h>
#include <vector>

namespace pfu {

class UpgradeTask {
public:
    explicit UpgradeTask(const CharVersion upgrade_version) : upgrade_version_(upgrade_version) {}
    virtual ~UpgradeTask() = default;
    [[nodiscard]] bool is_required(const Char &ch) const { return ch.version < upgrade_version_; }
    virtual void execute(Char &ch) const = 0;

private:
    const CharVersion upgrade_version_;
};

// Reset/repair Char attributes that are modifiable via spell/item bonii.
// This task accepts a version because over time the expectation is that
// it will be reused for future CharVersions. It _could_ be configured to
// use CharVersion::Max and thus run every time, but instead register_tasks()
// will include it explicitly for versions where we need it to run.
class ResetModifiableAttrs : public UpgradeTask {
public:
    explicit ResetModifiableAttrs(CharVersion version);
    void execute(Char &ch) const override;
};

class AddUniqueItemFlags : public UpgradeTask {
public:
    explicit AddUniqueItemFlags(CharVersion version);
    void execute(Char &ch) const override;

private:
    void walk_inventory_set_unique_flag(const GenericList<Object *> &objects) const;
};

struct CharUpgraderResult {
    CharUpgraderResult() = default;
    CharUpgraderResult(Char *ch, bool upgraded);
    ~CharUpgraderResult(); // remove the upgraded Char from the world
    // returns true if the Char was logged in and is valid. false if it couldn't be loaded (e.g. empty file)
    // and it's unnecessary for any upgrade tasks to have been processed because save should be called for
    // all valid chars.
    operator bool() const noexcept;
    operator Char &() const noexcept;
    Char *ch_{}; // set if a Char file was found, but it may still be invalid.
    bool upgraded_{}; // true if at least one task was performed
};

using Tasks = std::vector<std::unique_ptr<UpgradeTask>>;
using Logger = spdlog::logger;

// Log in one character and apply any relevant upgrades. Doesn't save the character.
class CharUpgrader {
public:
    explicit CharUpgrader(std::string_view name, const Tasks &all_tasks, Logger &logger);

    CharUpgraderResult upgrade();

private:
    // Simulates logging in a player character using its name.
    // Returns a new Char on the heap that's released when CharUpgraderResult is destroyed,
    // or null if it can't be loaded.
    // Retains the Char's original last login from/time, where possible.
    Char *simulate_login();
    // Filter all_tasks_ to include those that apply to the Char being upgraded.
    auto required_tasks(const Char &ch) const;

    const std::string_view name_;
    // A temporary Descriptor required when shoe-horning the Char into the world, it has
    // a mutual reference with Char, setup in simulate_login().
    Descriptor desc_;
    const Tasks &all_tasks_;
    Logger &logger_;
};

Tasks register_tasks();
std::vector<std::string> collect_player_names(const std::string &dirname, Logger &logger);
void upgrade_players(const std::vector<std::string> &players, Logger &logger);

// Instantiate a player character, upgrade/tweak it, save it then extract it from the world.
// This saves the Char out to the player file even if no UpgradeTasks are executed.
// Largely because the mud has introduced several subtle changes to Chars that are
// implicitly unversioned (e.g. addition of the Pronouns fields) and we want those things
// to be saved when upgrading legacy files.
template <typename SaveChar>
bool upgrade_player(std::string_view player, const Tasks &all_tasks, Logger &logger, SaveChar saveChar) {
    CharUpgrader cup(player, all_tasks, logger);
    auto result = cup.upgrade();
    if (result) {
        saveChar(result);
    }
    return result;
}

std::optional<Time> try_parse_login_at(const std::string &login_at);

}
