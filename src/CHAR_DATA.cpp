#include "CHAR_DATA.hpp"

#include "TimeInfoData.hpp"

Seconds CHAR_DATA::total_played() const { return std::chrono::duration_cast<Seconds>(current_time - logon + played); }
