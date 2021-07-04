/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#pragma once

#include <algorithm>

template <typename T>
inline T urange(T a, T b, T c) {
    return std::min(std::max(a, b), c);
}
