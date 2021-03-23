#include "Configuration.hpp"

#include <fmt/format.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

Configuration::Configuration() {
    area_dir_ = make_path(require_path_env(MUD_AREA_DIR_ENV));
    html_dir_ = make_path(require_path_env(MUD_HTML_DIR_ENV));
    data_dir_ = make_path(require_path_env(MUD_DATA_DIR_ENV));
    player_dir_ = data_dir_ + "player/";
    system_dir_ = data_dir_ + "system/";
    gods_dir_ = data_dir_ + "gods/";
    area_file_ = area_dir_ + "area.lst";
    tip_file_ = area_dir_ + "tipfile.txt";
    chat_data_file_ = area_dir_ + "chat.data";
    ban_file_ = system_dir_ + "ban.lst";
    bug_file_ = system_dir_ + "bugs.txt";
    typo_file_ = system_dir_ + "typos.txt";
    ideas_file_ = system_dir_ + "ideas.txt";
    notes_file_ = system_dir_ + "notes.txt";
    port_ = int_env(MUD_PORT_ENV, 9000);
}

std::string Configuration::area_dir() const { return area_dir_; }
std::string Configuration::html_dir() const { return html_dir_; }
std::string Configuration::player_dir() const { return player_dir_; }
std::string Configuration::system_dir() const { return system_dir_; }
std::string Configuration::gods_dir() const { return gods_dir_; }
std::string Configuration::area_file() const { return area_file_; }
std::string Configuration::tip_file() const { return tip_file_; }
std::string Configuration::chat_data_file() const { return chat_data_file_; }
std::string Configuration::ban_file() const { return ban_file_; }
std::string Configuration::bug_file() const { return bug_file_; }
std::string Configuration::typo_file() const { return typo_file_; }
std::string Configuration::ideas_file() const { return ideas_file_; }
std::string Configuration::notes_file() const { return notes_file_; }
uint Configuration::port() const { return port_; }

bool Configuration::is_valid_dir(const char *dirname) const {
    if (!dirname) {
        return false;
    }
    struct stat dir;
    return !stat(dirname, &dir) && S_ISDIR(dir.st_mode);
}

std::string Configuration::require_path_env(const std::string &envkey) const {
    const auto value = std::getenv(envkey.c_str());
    if (!is_valid_dir(value)) {
        throw std::invalid_argument(
            fmt::format("An environment variable called {} must specify a directory path", envkey));
    }
    return value;
}

std::string Configuration::make_path(std::string dir) const {
    if (dir.empty()) {
        return "./";
    } else if (dir.back() != '/') {
        std::string path(dir);
        path.append("/");
        return path;
    } else {
        return dir;
    }
}

int Configuration::int_env(const std::string &envkey, const int default_value) const {
    const auto value = std::getenv(envkey.c_str());
    return value ? atoi(value) : default_value;
}

Configuration &Configuration::singleton() {
    static Configuration singleton;
    return singleton;
}
