/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

#include <string_view>

struct MobIndexData;

namespace MProg {

// Loads all mob progs found in a #MOBPROGS section of an area file.
void load_mobprogs(FILE *fp);
bool read_program(std::string_view file_name, FILE *prog_file, MobIndexData *mobIndex);

}
