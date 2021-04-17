#include "Tip.hpp"
#include "db.h"

Tip Tip::from_file(FILE *fp) { return Tip(fread_stdstring(fp)); }
