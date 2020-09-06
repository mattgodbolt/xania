#include "merc.h"

#include "TimeInfoData.hpp"
#include "chat/chatlink.h"
#include "comm.hpp"
#include "common/Time.hpp"
#include "common/doorman_protocol.h"
#include "string_utils.hpp"

#include <cstdlib>
#include <cstring>
#include <sys/time.h>
#include <unistd.h>

/* SIGTRAP on/off */
bool debug = false;
/* check the mobs/objects */
bool printinfo = false;

int main(int argc, char **argv) {
    // Init time.
    current_time = Clock::now();

    log_string("Xania {} booting...", BUILD_FULL_VERSION);

    /*
     * Get the UNIX domain file
     */
    int port = 9000;
    if (argc > 1) {
        int num = 1;
        if (*argv[num] == '-') {
            num++;
            debug = true;
        } else if (*argv[num] == 'L') {
            num++;
            printinfo = true;
        } else if (is_number(argv[num])) {
            port = atoi(argv[num]);
        }
    }
    char file[256];
    snprintf(file, sizeof(file), XANIA_FILE, port, getenv("USER") ? getenv("USER") : "unknown");

    /*
     * Run the game.
     */

    auto control = init_socket(file);
    boot_db();
    load_bans();
    /* Rohan: Load player list into memory */
    load_player_list();
    startchat("chat.data");
    if (printinfo)
        check_xania();
    load_tipfile(); /* tip wizard - Faramir 21 Sep 1998 */
    log_string("Xania version {} is ready to rock via {}.", BUILD_VERSION, file);

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
