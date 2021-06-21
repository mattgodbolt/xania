#include "merc.h"

#include "TimeInfoData.hpp"
#include "Tip.hpp"
#include "chat/chatlink.h"
#include "comm.hpp"
#include "common/Configuration.hpp"
#include "common/Time.hpp"
#include "common/doorman_protocol.h"
#include "db.h"
#include "string_utils.hpp"

#include <cstdlib>
#include <cstring>
#include <fmt/format.h>
#include <sys/time.h>
#include <unistd.h>

/* SIGTRAP on/off */
bool debug = false;
extern void report_entity_imbalance();

int main(int argc, char **argv) {
    // Init time.
    current_time = Clock::now();
    bool show_entity_imbalance = false;
    log_string("Xania {} booting...", BUILD_FULL_VERSION);
    const auto &config = Configuration::singleton();
    /*
     * Get the UNIX domain file
     */
    if (argc > 1) {
        int num = 1;
        if (*argv[num] == '-') {
            num++;
            debug = true;
        } else if (*argv[num] == 'L') {
            num++;
            show_entity_imbalance = true;
        }
    }
    const auto pipe_file = fmt::format(PIPE_FILE, config.port(), getenv("USER") ? getenv("USER") : "unknown");
    /*
     * Run the game.
     */

    auto control = init_socket(pipe_file.c_str());
    boot_db();
    load_bans();
    startchat(config.chat_data_file());
    if (show_entity_imbalance)
        report_entity_imbalance();
    load_tipfile();
    log_string("Xania version {} is ready to rock via {}.", BUILD_VERSION, pipe_file);

    Packet pInit;
    pInit.nExtra = pInit.channel = 0;
    pInit.type = PACKET_INIT;
    send_to_doorman(&pInit, nullptr);

    game_loop_unix(std::move(control));

    /*
     * That's all, folks.
     */
    log_string("Normal termination of game.");
    exit(0);
}
