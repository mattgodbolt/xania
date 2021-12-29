/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

#include <string>
#include <vector>

namespace MProg {

enum class TypeFlag;

struct Program {
    const TypeFlag type;
    const std::string arglist;
    const std::vector<std::string> lines;
};

}