/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Act.hpp"
#include "Ban.hpp"
#include "Char.hpp"
#include "Logging.hpp"
#include "MudImpl.hpp"
#include "Tip.hpp"
#include "db.h"
#include "version.h"

#include <fmt/format.h>

int main() {

    auto mud = std::make_unique<MudImpl>();
    const auto &logger = mud->logger();
    logger.log_string("Xania {} booting...", get_build_full_version());
    mud->init_socket();
    boot_db(*mud.get()); // TODO Move into Mud properly
    mud->boot();
    // TODO Bans shouldn't be a singleton and should be a Mud member.
    const auto ban_count = Bans::singleton().load(logger);
    logger.log_string("{} site bans loaded.", ban_count);
    mud->game_loop();
    logger.log_string("Normal termination of game.");
    exit(0);
}
