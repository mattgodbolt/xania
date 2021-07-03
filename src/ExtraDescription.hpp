/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once
#include <string>

// Extra description data for a room or object.
struct ExtraDescription {
    std::string keyword; // Keyword in look/examine
    std::string description; // What to see
};
