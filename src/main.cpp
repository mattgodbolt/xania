/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 2021 Xania Development Team                                      */
/*  See merc.h and README for original copyrights                        */
/*************************************************************************/
#include "Ban.hpp"
#include "Char.hpp"
#include "Logging.hpp"
#include "TimeInfoData.hpp"
#include "Tip.hpp"
#include "chat/chatlink.h"
#include "comm.hpp"
#include "common/Configuration.hpp"
#include "common/Time.hpp"
#include "common/doorman_protocol.h"
#include "db.h"
#include "string_utils.hpp"
#include "version.h"

#include <fmt/format.h>
#include <sys/time.h>
#include <unistd.h>

int main() {

    // Init time.
    current_time = Clock::now();
    log_string("Xania {} booting...", get_build_full_version());
    const auto &config = Configuration::singleton();
    const auto pipe_file = fmt::format(PIPE_FILE, config.port(), getenv("USER") ? getenv("USER") : "unknown");
    auto control = init_socket(pipe_file.c_str());
    boot_db();
    const auto ban_count = Bans::singleton().load();
    log_string("{} site bans loaded.", ban_count);
    startchat(config.chat_data_file());
    load_tipfile();
    log_string("Xania version {} is ready to rock via {}.", get_build_version(), pipe_file);

    Packet pInit;
    pInit.nExtra = pInit.channel = 0;
    pInit.type = PACKET_INIT;
    send_to_doorman(&pInit, nullptr);

    game_loop_unix(std::move(control));
    delete_globals_on_shutdown();

    /*
     * That's all, folks.
     */
    log_string("Normal termination of game.");
    exit(0);
}
