/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2022   Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*************************************************************************/
#pragma once

#include "Types.hpp"

namespace Alignment {

// Alignment boundaries used by alignment label text. See get_align_description().
const sh_int Angelic = 1000;
const sh_int Saintly = 750;
const sh_int Good = 500;
const sh_int Kind = 250;
const sh_int Neutral = 0;
const sh_int Mean = -250;
const sh_int Evil = -500;
const sh_int Demonic = -750;
const sh_int Satanic = -1000;

// Intermediate alignments.
// >= than this means the character is "good" for several features.
const sh_int Amiable = 350;
// <= than this means the character is "evil" for several features.
const sh_int Depraved = -350;

}