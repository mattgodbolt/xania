#include "merc.h"

#include "doorman/doorman.h"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

/* SIGTRAP on/off */
bool debug = FALSE;
/* check the mobs/objects */
bool printinfo = FALSE;

extern char str_boot_time[];

// TODO this pile of things should probably be moved out into a separate file
// and/or the guts of "main" below should be extracted
extern int init_socket(const char *file);
extern int doormanDesc;
extern bool SendPacket(Packet *p, void *extra);
extern void game_loop_unix(int control);

int main(int argc, char **argv) {
    struct timeval now_time;
    char file[256];
    int port;
    int control;

    /*
     * Init time.
     */
    gettimeofday(&now_time, NULL);
    current_time = (time_t)(now_time.tv_sec);
    strcpy(str_boot_time, ctime(&current_time));

    snprintf(log_buf, LOG_BUF_SIZE, "Xania %s booting...", BUILD_FULL_VERSION);
    log_string(log_buf);

    /*
     * Reserve one channel for our use.
     */
    if ((fpReserve = fopen(NULL_FILE, "r")) == NULL) {
        perror(NULL_FILE);
        exit(1);
    }

    /*
     * Get the UNIX domain file
     */
    port = 9000;
    if (argc > 1) {
        int num = 1;
        if (*argv[num] == '-') {
            num++;
            debug = TRUE;
        } else if (*argv[num] == 'L') {
            num++;
            printinfo = TRUE;
        } else if (is_number(argv[num])) {
            port = atoi(argv[num]);
        }
    }
    snprintf(file, sizeof(file), XANIA_FILE, port, getenv("USER") ? getenv("USER") : "unknown");

    /*
     * Run the game.
     */

    control = init_socket(file);
    boot_db();
    load_bans();
    /* Rohan: Load player list into memory */
    load_player_list();
    startchat("chat.data");
    if (printinfo)
        check_xania();
    load_tipfile(); /* tip wizard - Faramir 21 Sep 1998 */
    snprintf(log_buf, LOG_BUF_SIZE, "Xania version %s is ready to rock via %s.", BUILD_VERSION, file);
    log_string(log_buf);
    if (doormanDesc) {
        Packet pInit;
        pInit.nExtra = pInit.channel = 0;
        pInit.type = PACKET_INIT;
        SendPacket(&pInit, NULL);
    }
    game_loop_unix(control);
    close(control);

    /*
     * That's all, folks.
     */
    log_string("Normal termination of game.");
    exit(0);
    return 0;
}
