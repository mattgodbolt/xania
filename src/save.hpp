#pragma once

#include <string>

// The base directory is the installation root. Defaults to "..".
std::string get_base_dir();
std::string get_player_dir();
std::string filename_for_player(std::string_view player_name);