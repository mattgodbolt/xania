/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2025 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#include "Tip.hpp"

extern std::string fread_string(FILE *fp);

Tip Tip::from_file(FILE *fp) { return Tip(fread_string(fp)); }