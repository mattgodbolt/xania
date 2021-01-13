#pragma once

#include "merc.h"
#include <string_view>

void remove_info_for_player(std::string_view player_name);
void update_info_cache(Char *ch);