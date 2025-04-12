#pragma once

#include <string>

/**
 * Environment variables required by Configuration.
 */
static inline constexpr auto MUD_DATA_DIR_ENV = "MUD_DATA_DIR";
static inline constexpr auto MUD_AREA_DIR_ENV = "MUD_AREA_DIR";
static inline constexpr auto MUD_HTML_DIR_ENV = "MUD_HTML_DIR";
static inline constexpr auto MUD_PORT_ENV = "MUD_PORT";

/**
 * Accessors for configuration settings. Client code should use
 * the static singleton.
 */
class Configuration {
public:
    Configuration();
    [[nodiscard]] std::string area_dir() const;
    [[nodiscard]] std::string html_dir() const;
    [[nodiscard]] std::string player_dir() const;
    [[nodiscard]] std::string system_dir() const;
    [[nodiscard]] std::string gods_dir() const;
    [[nodiscard]] std::string area_file() const;
    [[nodiscard]] std::string tip_file() const;
    [[nodiscard]] std::string ban_file() const;
    [[nodiscard]] std::string bug_file() const;
    [[nodiscard]] std::string typo_file() const;
    [[nodiscard]] std::string ideas_file() const;
    [[nodiscard]] std::string notes_file() const;
    [[nodiscard]] uint port() const;

    static Configuration &singleton();

private:
    [[nodiscard]] bool is_valid_dir(const char *dirname) const;
    [[nodiscard]] std::string require_path_env(const std::string &envkey) const;
    [[nodiscard]] std::string make_path(std::string dir) const;
    [[nodiscard]] int int_env(const std::string &envkey, const int default_value) const;
    std::string area_dir_;
    std::string html_dir_;
    std::string player_dir_;
    std::string system_dir_;
    std::string gods_dir_;
    std::string data_dir_;

    std::string area_file_;
    std::string tip_file_;
    std::string ban_file_;
    std::string bug_file_;
    std::string typo_file_;
    std::string ideas_file_;
    std::string notes_file_;

    uint port_;
};
