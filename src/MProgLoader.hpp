/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

#include "Logging.hpp"

#include <string_view>

struct AreaList;
struct MobIndexData;

namespace MProg {

// Loads all mob progs found in a #MOBPROGS section of an area file.
void load_mobprogs(FILE *fp, const AreaList &areas, std::string_view area_dir, const Logger &logger);
bool read_program(std::string_view file_name, FILE *prog_file, MobIndexData *mobIndex, const Logger &logger);

}
