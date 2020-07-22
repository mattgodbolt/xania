/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  comm.c: the core of the MUD, handles initialisation and networking   */
/*                                                                       */
/*************************************************************************/

/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

#if defined(macintosh)
#include <types.h>
#else
#if defined(riscos)
#include "sys/types.h"
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/un.h>

#include "challeng.h"
#include "doorman/doorman.h"
#include "merc.h"
#include "news.h"
#include "note.h"

/* command procedures needed */
DECLARE_DO_FUN(do_help);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_skills);
DECLARE_DO_FUN(do_outfit);
void announce(char *, CHAR_DATA *ch);

/* SIGTRAP on/off */
bool debug = FALSE;
/* check the mobs/objects */
bool printinfo = FALSE;

/* Added by Rohan - extern to player list for adding new players to it */
extern KNOWN_PLAYERS *player_list;

/* extern void identd_lookup(void);  ......commented out due to sock leaks */

/*
 * Malloc debugging stuff.
 */
#if defined(sun)
#undef MALLOC_DEBUG
#endif

#if defined(MALLOC_DEBUG)
#include <malloc.h>
extern int malloc_debug args((int));
extern int malloc_verify args((void));
#endif

/*
 * Signal handling.
 * Apollo has a problem with __attribute(atomic) in signal.h,
 *   I dance around it.
 */
#if defined(apollo)
#define __attribute(x)
#endif

#if defined(unix)
#include <signal.h>
#endif

#if defined(apollo)
#undef __attribute
#endif

/*
 * Socket and TCP/IP stuff.
 */
#if defined(macintosh) || defined(MSDOS) || defined(riscos)
const char echo_off_str[] = {'\0'};
const char echo_on_str[] = {'\0'};
const char go_ahead_str[] = {'\0'};
#endif

#if defined(unix)
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/telnet.h>
const char echo_off_str[] = {IAC, WILL, TELOPT_ECHO, '\0'};
const char echo_on_str[] = {IAC, WONT, TELOPT_ECHO, '\0'};
const char go_ahead_str[] = {IAC, GA, '\0'};
#endif

#if defined(riscos)
#include "sys/socket.h"
#include "arpa/inet.h"
#include "sys/sockio.h"
#include "netdb.h"
#include "netinet/in.h"
int close args((int fd));
int write args((int fd, char *buf, int nbyte));
#endif

/*
 * OS-dependent declarations.
 */
#if defined(_AIX)
#include <sys/select.h>
int accept args((int s, struct sockaddr *addr, int *addrlen));
int bind args((int s, struct sockaddr *name, int namelen));
void bzero args((char *b, int length));
int getpeername args((int s, struct sockaddr *name, int *namelen));
int getsockname args((int s, struct sockaddr *name, int *namelen));
int gettimeofday args((struct timeval * tp, struct timezone *tzp));

int listen args((int s, int backlog));

int setsockopt args((int s, int level, int optname, void *optval, int optlen));
int socket args((int domain, int type, int protocol));
#endif

#if defined(apollo)
#include <unistd.h>
void bzero args((char *b, int length));
#endif

#if defined(__hpux)
int accept args((int s, void *addr, int *addrlen));
int bind args((int s, const void *addr, int addrlen));
void bzero args((char *b, int length));
int getpeername args((int s, void *addr, int *addrlen));
int getsockname args((int s, void *name, int *addrlen));
int gettimeofday args((struct timeval * tp, struct timezone *tzp));
int listen args((int s, int backlog));
int setsockopt args((int s, int level, int optname, const void *optval, int optlen));
int socket args((int domain, int type, int protocol));
#endif

#if defined(interactive)
#include <net/errno.h>
#include <sys/fnctl.h>
#endif

#if defined(linux)
#include <unistd.h>
#endif

#if defined(macintosh) || defined(riscos)
#if defined(macintosh)
#include <console.h>
#include <fcntl.h>
#include <unix.h>
#endif
struct timeval {
    time_t tv_sec;
    time_t tv_usec;
};
#if !defined(isascii)
#define isascii(c) ((c) < 0200)
#endif

int gettimeofday args((struct timeval * tp, void *tzp));
#endif

#if defined(MIPS_OS)
extern int errno;
#endif

#if defined(MSDOS)
/*int gettimeofday   args( ( struct timeval *tp, void *tzp ) );*/
int kbhit args((void));
#endif

#if defined(NeXT)
int close args((int fd));
int fcntl args((int fd, int cmd, int arg));
#if !defined(htons)
u_short htons args((u_short hostshort));
#endif
#if !defined(ntohl)
u_long ntohl args((u_long hostlong));
#endif
int read args((int fd, char *buf, int nbyte));
int select args((int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout));
int write args((int fd, char *buf, int nbyte));
#endif

#if defined(sequent)
int accept args((int s, struct sockaddr *addr, int *addrlen));
int bind args((int s, struct sockaddr *name, int namelen));
int close args((int fd));
int fcntl args((int fd, int cmd, int arg));
int getpeername args((int s, struct sockaddr *name, int *namelen));
int getsockname args((int s, struct sockaddr *name, int *namelen));
int gettimeofday args((struct timeval * tp, struct timezone *tzp));
#if !defined(htons)
u_short htons args((u_short hostshort));
#endif
int listen args((int s, int backlog));
#if !defined(ntohl)
u_long ntohl args((u_long hostlong));
#endif
int read args((int fd, char *buf, int nbyte));
int select args((int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout));
int setsockopt args((int s, int level, int optname, caddr_t optval, int optlen));
int socket args((int domain, int type, int protocol));
int write args((int fd, char *buf, int nbyte));
#endif

/* This includes Solaris Sys V as well */
#if defined(sun)
int accept args((int s, struct sockaddr *addr, int *addrlen));
int bind args((int s, struct sockaddr *name, int namelen));
void bzero args((char *b, int length));
int close args((int fd));
int getpeername args((int s, struct sockaddr *name, int *namelen));
int getsockname args((int s, struct sockaddr *name, int *namelen));
int gettimeofday args((struct timeval * tp, struct timezone *tzp));
int listen args((int s, int backlog));
int read args((int fd, char *buf, int nbyte));
int select args((int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout));
#if defined(SYSV)
int setsockopt args((int s, int level, int optname, const char *optval, int optlen));
#else
int setsockopt args((int s, int level, int optname, void *optval, int optlen));
#endif
int socket args((int domain, int type, int protocol));
int write args((int fd, char *buf, int nbyte));
#endif

#if defined(ultrix)
int accept args((int s, struct sockaddr *addr, int *addrlen));
int bind args((int s, struct sockaddr *name, int namelen));
void bzero args((char *b, int length));
int close args((int fd));
int getpeername args((int s, struct sockaddr *name, int *namelen));
int getsockname args((int s, struct sockaddr *name, int *namelen));
int gettimeofday args((struct timeval * tp, struct timezone *tzp));
int listen args((int s, int backlog));
int read args((int fd, char *buf, int nbyte));
int select args((int width, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout));
int setsockopt args((int s, int level, int optname, void *optval, int optlen));
int socket args((int domain, int type, int protocol));
int write args((int fd, char *buf, int nbyte));
#endif

/*
 * Global variables.
 */
DESCRIPTOR_DATA *descriptor_free; /* Free list for descriptors  */
DESCRIPTOR_DATA *descriptor_list; /* All open descriptors    */
DESCRIPTOR_DATA *d_next;          /* Next descriptor in loop */
FILE *fpReserve;                  /* Reserved file handle    */
bool god;                         /* All new chars are gods! */
bool merc_down;                   /* Shutdown       */
bool wizlock;                     /* Game is wizlocked    */
bool newlock;                     /* Game is newlocked    */
char str_boot_time[MAX_INPUT_LENGTH];
time_t current_time; /* time of this pulse */

/*
 * OS-dependent local functions.
 */
#if defined(macintosh) || defined(MSDOS) || defined(riscos)
void game_loop_mac_msdos args((void));
bool read_from_descriptor args((DESCRIPTOR_DATA * d));
bool write_to_descriptor args((int desc, char *txt, int length));
#endif

#if defined(unix)
void game_loop_unix args((int control));
int init_socket(char *file);
DESCRIPTOR_DATA *new_descriptor args((int control));
bool read_from_descriptor args((DESCRIPTOR_DATA * d, char *text, int nRead));
bool write_to_descriptor args((int desc, char *txt, int length));
#endif

/*
 * Other local functions (OS-independent).
 */
bool check_parse_name args((char *name));
bool check_reconnect args((DESCRIPTOR_DATA * d, char *name, bool fConn));
bool check_playing args((DESCRIPTOR_DATA * d, char *name));
int main args((int argc, char **argv));
void nanny args((DESCRIPTOR_DATA * d, char *argument));
bool process_output args((DESCRIPTOR_DATA * d, bool fPrompt));
void read_from_buffer args((DESCRIPTOR_DATA * d));
void stop_idling args((CHAR_DATA * ch));
void show_prompt args((DESCRIPTOR_DATA * d, char *prompt));

DECLARE_DO_FUN(do_save);

/* Handle to get to doorman */
static int doormanDesc = 0;

/* Send a packet to doorman */
bool SendPacket(Packet *p, void *extra) {
    // TODO: do something rather than return here if there's a failure
    if (!doormanDesc)
        return false;
    int bytesRead = write(doormanDesc, (char *)p, sizeof(Packet));
    if (bytesRead != sizeof(Packet))
        return false;
    if (p->nExtra) {
        bytesRead = write(doormanDesc, extra, p->nExtra);
        if (bytesRead != sizeof(Packet))
            return false;
    }
    return true;
}
void SetEchoState(DESCRIPTOR_DATA *d, int on) {
    Packet p;
    p.type = on ? PACKET_ECHO_ON : PACKET_ECHO_OFF;
    p.channel = d->descriptor;
    p.nExtra = 0;
    SendPacket(&p, NULL);
}

/* This handler deals with catching SIGTERM calls - for instance if the
 * machine the MUD is running on is going down.
 * It tidies up and tells everyone what's going on
 */
void sigterm(int signum) {
    (void)signum;
    CHAR_DATA *ch;

    bug("The MUD has caught a SIGTERM - gracefully shutting down\n\r");
    for (ch = char_list; ch; ch = ch->next) {
        if (!IS_NPC(ch)) {
            do_save(ch, "");
            send_to_char("|R******************************************\n\r"
                         "**  The machine up which |WXania|R runs is  **\n\r"
                         "**  being shut down for maintainance.   **\n\r"
                         "**  Your character has been saved, but  **\n\r"
                         "**  the MUD will be down for a short    **\n\r"
                         "**  time.  Sorry for any problems!      **\n\r"
                         "******************************************|W\n\r",
                         ch);
            if (ch->desc && ch->desc->outtop > 0)
                process_output(ch->desc, FALSE);
        }
    }
    sleep(2);
    exit(0);
}

/* where we're asked nicely to quit from the outside (mudmgr) */

void ext_shutdown(int signum) {
    (void)signum;
    char buf[MAX_STRING_LENGTH];
    extern bool merc_down;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    FILE *fp;
    DESCRIPTOR_DATA *d, *d_next;

    snprintf(buf, sizeof(buf), "Shutdown requested by operating system.");

    fclose(fpReserve);
    if ((fp = fopen(SHUTDOWN_FILE, "a")) == NULL) {
        perror(SHUTDOWN_FILE);
    } else {
        fputs(buf, fp);
        fclose(fp);
    }

    fpReserve = fopen(NULL_FILE, "r");
    log_string(buf);
    /* ask everyone to save! */
    for (vch = char_list; vch != NULL; vch = vch_next) {
        vch_next = vch->next;

        // vch->d->c check added by TM to avoid crashes when
        // someone hasn't logged in but the mud is shut down
        if (!IS_NPC(vch) && vch->desc && vch->desc->connected == CON_PLAYING) {
            /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
            MOBtrigger = FALSE;
            do_save(vch, "");
            send_to_char("|RXania has been asked to shutdown by the operating system.|w\n\r", vch);
            if (vch->desc && vch->desc->outtop > 0)
                process_output(vch->desc, FALSE);
        }
    }
    merc_down = TRUE;
    for (d = descriptor_list; d != NULL; d = d_next) {
        d_next = d->next;
        close_socket(d);
    }
    sleep(2);
    exit(0);
}

void panic(int signum) {

    CHAR_DATA *ch;

    /*  signal( signum, SIG_DFL ); restore  usual handler in case  we fall over
                    trying to do this now - hopefully this won't
                    happen!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    bug("OH SHIT!!!!!!!!!!!!!!!!!!  The MUD's crashed with sig %d\n\r", signum);
    for (ch = char_list; ch; ch = ch->next) {
        if (!IS_NPC(ch)) {
            do_save(ch, "");
            send_to_char("|R******************************************\n\r"
                         "**  Xania has crashed - your character  **\n\r"
                         "**  |WHAS|R been saved safely.  Xania       **\n\r"
                         "**  will reboot shortly, we hope!       **\n\r"
                         "**  |BThe X-Development Team|R apologizes   **\n\r"
                         "**  for any inconvience! |G---\\--/-|R@      **\n\r"
                         "******************************************|w\n\r",
                         ch);
            if (ch->desc && ch->desc->outtop > 0)
                process_output(ch->desc, FALSE);
        }
    }
    sleep(2);
    exit(0);
}

int main(int argc, char **argv) {
    struct timeval now_time;
    char file[256];
    int port;

#if defined(unix)
    int control;
#endif

    /*
     * Memory debugging if needed.
     */
#if defined(MALLOC_DEBUG)
    malloc_debug(2);
#endif

    /*
     * Init time.
     */
    gettimeofday(&now_time, NULL);
    current_time = (time_t)(now_time.tv_sec);
    strcpy(str_boot_time, ctime(&current_time));

    /*
     * Macintosh console initialization.
     */
#if defined(macintosh)
    console_options.nrows = 31;
    cshow(stdout);
    csetmode(C_RAW, stdin);
    cecho2file("log file", 1, stderr);
#endif

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
#if defined(macintosh) || defined(MSDOS) || defined(riscos)
    boot_db();
    log_string("Merc is ready to rock.");
    game_loop_mac_msdos();
#endif

#if defined(unix)
    control = init_socket(file);
    boot_db();
    load_bans();
    /* Rohan: Load player list into memory */
    load_player_list();
    startchat("chat.data");
    if (printinfo)
        check_xania();
    load_tipfile(); /* tip wizard - Faramir 21 Sep 1998 */
    snprintf(log_buf, LOG_BUF_SIZE, "Xania is ready to rock via %s.", file);
    log_string(log_buf);
    if (doormanDesc) {
        Packet pInit;
        pInit.nExtra = pInit.channel = 0;
        pInit.type = PACKET_INIT;
        SendPacket(&pInit, NULL);
    }
    if (!debug) {
        signal(SIGQUIT, ext_shutdown); /* external QUIT request (mudmgr)*/
        signal(SIGSEGV, panic);        /*     panic  handler    */
        signal(SIGBUS, panic);         /* another panic handler */
        signal(SIGTERM, sigterm);      /* catch terminate calls */
    } else {
        log_string("*** DEBUG MODE ON *** - no signal handlers installed.");
    }
    game_loop_unix(control);
    close(control);
    if (!debug) {
        signal(SIGQUIT, ext_shutdown); /* external QUIT request (mudmgr)*/
        signal(SIGSEGV, SIG_DFL);
        signal(SIGBUS, SIG_DFL);
        signal(SIGTERM, SIG_DFL);
    }
#endif

    /*
     * That's all, folks.
     */
    log_string("Normal termination of game.");
    exit(0);
    return 0;
}

#if defined(unix)
int init_socket(char *file) {
    static struct sockaddr_un sa_zero;
    struct sockaddr_un sa;
    int x = 1;
    int fd;
    socklen_t size;

    unlink(file);

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        perror("Init_socket: socket");
        exit(1);
    }

    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&x, sizeof(x)) < 0) {
        perror("Init_socket: SO_REUSEADDR");
        close(fd);
        exit(1);
    }

#if defined(SO_DONTLINGER) && !defined(SYSV)
    {
        struct linger ld;

        ld.l_onoff = 1;
        ld.l_linger = 1000;

        if (setsockopt(fd, SOL_SOCKET, SO_DONTLINGER, (char *)&ld, sizeof(ld)) < 0) {
            perror("Init_socket: SO_DONTLINGER");
            close(fd);
            exit(1);
        }
    }
#endif

    sa = sa_zero;
    sa.sun_family = AF_UNIX;
    strcpy(sa.sun_path, file);

    if (bind(fd, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("Init socket: bind");
        close(fd);
        exit(1);
    }

    if (listen(fd, 3) < 0) {
        perror("Init socket: listen");
        close(fd);
        exit(1);
    }

    log_string("Waiting for doorman...");
    size = sizeof(sa);
    if ((doormanDesc = accept(fd, (struct sockaddr *)&sa, &size)) < 0) {
        log_string("Unable to accept doorman...booting anyway");
        doormanDesc = 0;
    } else {
        log_string("Doorman has connected - proceeding with boot-up");
    }

    return fd;
}
#endif

#if defined(macintosh) || defined(MSDOS) || defined(riscos)
void game_loop_mac_msdos(void) {
    struct timeval last_time;
    struct timeval now_time;
    static DESCRIPTOR_DATA dcon;

    gettimeofday(&last_time, NULL);
    current_time = (time_t)last_time.tv_sec;

    /*
     * New_descriptor analogue.
     */
    dcon.descriptor = 0;
    dcon.connected = CON_GET_NAME;
    dcon.host = str_dup("localhost");
    dcon.outsize = 2000;
    dcon.outbuf = alloc_mem(dcon.outsize);
    dcon.next = descriptor_list;
    dcon.showstr_head = NULL;
    dcon.showstr_point = NULL;
    descriptor_list = &dcon;

    /*
     * Send the greeting.
     */
    {
        extern char *help_greeting;
        if (help_greeting[0] == '.')
            write_to_buffer(&dcon, help_greeting + 1, 0);
        else
            write_to_buffer(&dcon, help_greeting, 0);
    }

    /* Main loop */
    while (!merc_down) {
        DESCRIPTOR_DATA *d;

        /*
         * Process input.
         */
        for (d = descriptor_list; d != NULL; d = d_next) {
            d_next = d->next;
            d->fcommand = FALSE;

#if defined(MSDOS)
            if (kbhit())
#endif
            {
                if (d->character != NULL)
                    d->character->timer = 0;
                if (!read_from_descriptor(d)) {
                    if (d->character != NULL)
                        save_char_obj(d->character);
                    d->outtop = 0;
                    close_socket(d);
                    continue;
                }
            }

            if (d->character != NULL && d->character->wait > 0) {
                --d->character->wait;
                continue;
            }

            read_from_buffer(d);
            if (d->incomm[0] != '\0') {
                d->fcommand = TRUE;
                stop_idling(d->character);

                if (d->connected == CON_PLAYING)
                    interpret(d->character, d->incomm);
                else
                    nanny(d, d->incomm);

                d->incomm[0] = '\0';
            }
        }

        /*
         * Autonomous game motion.
         */
        update_handler();

        /*
         * Output.
         */
        for (d = descriptor_list; d != NULL; d = d_next) {
            d_next = d->next;

            if ((d->fcommand || d->outtop > 0)) {
                if (!process_output(d, TRUE)) {
                    if (d->character != NULL && d->character->level > 1)
                        save_char_obj(d->character);
                    d->outtop = 0;
                    close_socket(d);
                }
            }
        }

        /*
         * Synchronize to a clock.
         * Busy wait (blargh).
         */
        now_time = last_time;
        for (;;) {
            int delta;

#if defined(MSDOS)
            if (kbhit())
#endif
            {
                if (dcon.character != NULL)
                    dcon.character->timer = 0;
                if (!read_from_descriptor(&dcon)) {
                    if (dcon.character != NULL && d->character->level > 1)
                        save_char_obj(d->character);
                    dcon.outtop = 0;
                    close_socket(&dcon);
                }
#if defined(MSDOS)
                break;
#endif
            }

            gettimeofday(&now_time, NULL);
            delta = (now_time.tv_sec - last_time.tv_sec) * 1000 * 1000 + (now_time.tv_usec - last_time.tv_usec);
            if (delta >= 1000000 / PULSE_PER_SECOND)
                break;
        }
        last_time = now_time;
        current_time = (time_t)last_time.tv_sec;
    }

    return;
}
#endif

/**
 * Calculate a hash of a string.
 */
unsigned long djb2_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

/**
 * Writes into hostbuf the supplied hostname, masked for privacy
 * and with a hashcode of the full hostname. This can be used by admins
 * to spot users coming from the same IP.
 */
char *get_masked_hostname(char *hostbuf, const char *hostname) {
    snprintf(hostbuf, MAX_MASKED_HOSTNAME, "%.6s*** [#%ld]", hostname, djb2_hash(hostname));
    return hostbuf;
}

#if defined(unix)
void game_loop_unix(int control) {
    static struct timeval null_time;
    struct timeval last_time;

    signal(SIGPIPE, SIG_IGN);
    gettimeofday(&last_time, NULL);
    current_time = (time_t)last_time.tv_sec;

    /* Main loop */
    while (!merc_down) {
        fd_set in_set;
        fd_set out_set;
        fd_set exc_set;
        int maxdesc;
        DESCRIPTOR_DATA *d, *dNext;

#if defined(MALLOC_DEBUG)
        if (malloc_verify() != 1)
            abort();
#endif

        /*
         * Poll all active descriptors.
         */
        FD_ZERO(&in_set);
        FD_ZERO(&out_set);
        FD_ZERO(&exc_set);
        FD_SET(control, &in_set);
        if (doormanDesc)
            FD_SET(doormanDesc, &in_set);
        maxdesc = UMAX(control, doormanDesc);

        if (select(maxdesc + 1, &in_set, &out_set, &exc_set, &null_time) < 0) {
            perror("Game_loop: select: poll");
            exit(1);
        }

        /* If we don't have a connection from doorman, wait for one */
        if (doormanDesc == 0) {
            struct sockaddr_un sock;
            socklen_t size;

            log_string("Waiting for doorman...");
            size = sizeof(sock);
            getsockname(control, (struct sockaddr *)&sock, &size);
            if ((doormanDesc = accept(control, (struct sockaddr *)&sock, &size)) < 0) {
                perror("doorman: accept");
                doormanDesc = 0;
            } else {
                Packet pInit;
                log_string("Doorman has connected.");
                pInit.nExtra = pInit.channel = 0;
                pInit.type = PACKET_INIT;
                SendPacket(&pInit, NULL);
            }
        }

        /* Process doorman input, if any */
        if (doormanDesc && FD_ISSET(doormanDesc, &in_set)) {
            do {
                Packet p;
                char buffer[4096 * 4];
                int nBytes;
                int ok;

                nBytes = read(doormanDesc, (char *)&p, sizeof(p));
                if (nBytes == 0) {
                    log_string("Lost connection to doorman");
                    doormanDesc = 0;
                    /* Now to go through and disconnect all the characters */
                    while (descriptor_list)
                        close_socket(descriptor_list);
                } else {
                    char logMes[18 * 1024];
                    if (p.nExtra) {
                        nBytes += read(doormanDesc, buffer, p.nExtra);
                    }
                    // TODO check nBytes and blow up if we get partial messages from doorman
                    switch (p.type) {
                    case PACKET_CONNECT:
                        snprintf(logMes, sizeof(logMes), "Incoming connection on channel %d.", p.channel);
                        log_string(logMes);
                        new_descriptor(p.channel);
                        break;
                    case PACKET_RECONNECT: {
                        DESCRIPTOR_DATA *d;
                        snprintf(logMes, sizeof(logMes), "Incoming reconnection on channel %d for %s.", p.channel,
                                 buffer);
                        log_string(logMes);
                        d = new_descriptor(p.channel);
                        // Login name
                        write_to_buffer(d, buffer, 0);
                        write_to_buffer(d, "\n\r", 2);
                        nanny(d, buffer);
                        write_to_buffer(d, "\n\r", 2);

                        // Paranoid :
                        if (d->connected == CON_GET_OLD_PASSWORD) {
                            d->connected = CON_CIRCUMVENT_PASSWORD;
                            // log in
                            nanny(d, "");
                            // accept ANSIness
                            write_to_buffer(d, "\n\r", 2);
                            nanny(d, "");
                        }
                    } break;
                    case PACKET_DISCONNECT:
                        /*
                         * Find the descriptor associated with the channel and close it
                         */
                        for (d = descriptor_list; d; d = d->next) {
                            if (d->descriptor == p.channel) {
                                if (d->character && d->character->level > 1)
                                    save_char_obj(d->character);
                                d->outtop = 0;
                                if (d->connected == CON_PLAYING)
                                    d->connected = CON_DISCONNECTING;
                                else
                                    d->connected = CON_DISCONNECTING_NP;
                                close_socket(d);
                                break;
                            }
                        }
                        break;
                    case PACKET_INFO:
                        /*
                         * Find the descriptor associated with the channel
                         */
                        ok = 0;
                        for (d = descriptor_list; d; d = d->next) {
                            if (d->descriptor == p.channel) {
                                InfoData *data = (InfoData *)buffer;
                                ok = 1;
                                d->netaddr = data->netaddr;
                                d->localport = data->port;
                                free_string(d->host);
                                d->host = str_dup(data->data);
                                break;
                            }
                        }
                        if (!ok) {
                            snprintf(logMes, sizeof(logMes), "Unable to associate info with a descriptor (%d)",
                                     p.channel);
                        } else {
                            char hostbuf[MAX_MASKED_HOSTNAME];
                            snprintf(logMes, sizeof(logMes), "Info from doorman: %d is %s", p.channel,
                                     get_masked_hostname(hostbuf, d->host));
                        }
                        log_string(logMes);
                        break;
                    case PACKET_MESSAGE:
                        /*
                         * Find the descriptor associated with the channel
                         */
                        ok = 0;
                        for (d = descriptor_list; d; d = d->next) {
                            if (d->descriptor == p.channel) {
                                ok = 1;
                                if (d->character != NULL)
                                    d->character->timer = 0;
                                read_from_descriptor(d, buffer, p.nExtra);
                            }
                        }
                        if (!ok) {
                            snprintf(logMes, sizeof(logMes), "Unable to associate message with a descriptor (%d)",
                                     p.channel);
                            log_string(logMes);
                        }
                        break;
                    default: break;
                    }
                }
                /* Reselect to see if there is more data */
                select(maxdesc + 1, &in_set, &out_set, &exc_set, &null_time);
            } while FD_ISSET(doormanDesc, &in_set);
        }

        /*
         * De-waitstate characters and process pending input
         */
        for (d = descriptor_list; d; d = dNext) {
            dNext = d->next;
            d->fcommand = FALSE;
            if (d->character && d->character->wait > 0)
                --d->character->wait;
            /* Waitstate the character */
            if (d->character && d->character->wait)
                continue;
            read_from_buffer(d);
            if (d->incomm[0] != '\0') {
                d->fcommand = TRUE;
                stop_idling(d->character);

                if (d->showstr_point)
                    show_string(d, d->incomm);
                else if (d->connected == CON_PLAYING)
                    interpret(d->character, d->incomm);
                else
                    nanny(d, d->incomm);

                d->incomm[0] = '\0';
            }
        }

#if 0
	  /*
	   * Kick out the freaky folks.
	   */
	  for ( d = descriptor_list; d != NULL; d = d_next )
	  {
	       d_next = d->next;
	       if ( FD_ISSET( d->descriptor, &exc_set ) )
	       {
		    FD_CLR( d->descriptor, &in_set  );
		    FD_CLR( d->descriptor, &out_set );
		    if ( d->character && d->character->level > 1)
			 save_char_obj( d->character );
		    d->outtop   = 0;
		    close_socket( d );
	       }
	  }

	  /*
	   * Process input.
	   */
	  for ( d = descriptor_list; d != NULL; d = d_next )
	  {
	       d_next  = d->next;
	       d->fcommand   = FALSE;

	       if ( FD_ISSET( d->descriptor, &in_set ) )
	       {
		    if ( d->character != NULL )
			 d->character->timer = 0;
		    if ( !read_from_descriptor( d ) )
		    {
			 FD_CLR( d->descriptor, &out_set );
			 if ( d->character != NULL && d->character->level > 1)
			      save_char_obj( d->character );
			 d->outtop  = 0;
			 close_socket( d );
			 continue;
		    }
	       }

	       if ( d->character != NULL && d->character->wait > 0 )
	       {
		    --d->character->wait;
		    continue;
	       }

	       read_from_buffer( d );
	       if ( d->incomm[0] != '\0' )
	       {
		    d->fcommand = TRUE;
		    stop_idling( d->character );

		    if (d->showstr_point)
			 show_string(d,d->incomm);
		    else if ( d->connected == CON_PLAYING )
			 interpret( d->character, d->incomm );
		    else
			 nanny( d, d->incomm );

		    d->incomm[0]   = '\0';
	       }
	  }
#endif

        /*
         * Autonomous game motion.
         */
        update_handler();

        /*
         * Output.
         */
        if (doormanDesc) {
            DESCRIPTOR_DATA *d;
            for (d = descriptor_list; d != NULL; d = d_next) {
                d_next = d->next;

                if ((d->fcommand || d->outtop > 0)) {
                    if (!process_output(d, TRUE)) {
                        if (d->character != NULL && d->character->level > 1)
                            save_char_obj(d->character);
                        d->outtop = 0;
                        close_socket(d);
                    }
                }
            }
        }

        /*
         * Synchronize to a clock.
         * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
         * Careful here of signed versus unsigned arithmetic.
         */
        {
            struct timeval now_time;
            long secDelta;
            long usecDelta;

            gettimeofday(&now_time, NULL);
            usecDelta = ((int)last_time.tv_usec) - ((int)now_time.tv_usec) + 1000000 / PULSE_PER_SECOND;
            secDelta = ((int)last_time.tv_sec) - ((int)now_time.tv_sec);
            while (usecDelta < 0) {
                usecDelta += 1000000;
                secDelta -= 1;
            }

            while (usecDelta >= 1000000) {
                usecDelta -= 1000000;
                secDelta += 1;
            }

            if (secDelta > 0 || (secDelta == 0 && usecDelta > 0)) {
                struct timeval stall_time;

                stall_time.tv_usec = usecDelta;
                stall_time.tv_sec = secDelta;
                if (select(0, NULL, NULL, NULL, &stall_time) < 0) {
                    perror("Game_loop: select: stall");
                    exit(1);
                }
            }
        }

        gettimeofday(&last_time, NULL);
        current_time = (time_t)last_time.tv_sec;
    }

    return;
}
#endif

#if defined(unix)
DESCRIPTOR_DATA *new_descriptor(int channel) {
    static DESCRIPTOR_DATA d_zero;
    DESCRIPTOR_DATA *dnew;

    /*
     * Cons a new descriptor.
     */
    if (descriptor_free == NULL) {
        dnew = alloc_perm(sizeof(*dnew));
    } else {
        dnew = descriptor_free;
        descriptor_free = descriptor_free->next;
    }

    *dnew = d_zero;
    dnew->descriptor = channel;
    dnew->connected = CON_GET_NAME;
    dnew->showstr_head = NULL;
    dnew->showstr_point = NULL;
    dnew->outsize = 2000;
    dnew->outbuf = alloc_mem(dnew->outsize);
    dnew->logintime = str_dup((char *)ctime(&current_time));

    dnew->host = str_dup("(unknown)");

    /*
     * Init descriptor data.
     */
    dnew->next = descriptor_list;
    descriptor_list = dnew;

    /*
     * Send the greeting.
     */
    {
        extern char *help_greeting;
        if (help_greeting[0] == '.')
            write_to_buffer(dnew, help_greeting + 1, 0);
        else
            write_to_buffer(dnew, help_greeting, 0);
    }

    return dnew;
}
#endif

void close_socket(DESCRIPTOR_DATA *dclose) {
    CHAR_DATA *ch;
    Packet p;

    if (dclose->outtop > 0)
        process_output(dclose, FALSE);

    if (dclose->snoop_by != NULL) {
        write_to_buffer(dclose->snoop_by, "Your victim has left the game.\n\r", 0);
    }

    {
        DESCRIPTOR_DATA *d;

        for (d = descriptor_list; d != NULL; d = d->next) {
            if (d->snoop_by == dclose)
                d->snoop_by = NULL;
        }
    }

    if ((ch = dclose->character) != NULL) {
        do_chal_canc(ch);
        snprintf(log_buf, LOG_BUF_SIZE, "Closing link to %s.", ch->name);
        log_new(log_buf, EXTRA_WIZNET_DEBUG,
                (IS_SET(ch->act, PLR_WIZINVIS) || IS_SET(ch->act, PLR_PROWL)) ? get_trust(ch) : 0);
        if (dclose->connected == CON_PLAYING || dclose->connected == CON_DISCONNECTING) {
            act("$n has lost $s link.", ch, NULL, NULL, TO_ROOM);
            ch->desc = NULL;
        } else {
            free_char(dclose->original ? dclose->original : dclose->character);
        }
    } else {
        /*
         * New code: debug connections that haven't logged in properly
         * ...at least for level 100s
         */
        snprintf(log_buf, LOG_BUF_SIZE, "Closing link to descriptor %d.", dclose->descriptor);
        log_new(log_buf, EXTRA_WIZNET_DEBUG, 100);
    }

    if (d_next == dclose)
        d_next = d_next->next;

    if (dclose == descriptor_list) {
        descriptor_list = descriptor_list->next;
    } else {
        DESCRIPTOR_DATA *d;

        for (d = descriptor_list; d && d->next != dclose; d = d->next)
            ;
        if (d != NULL)
            d->next = dclose->next;
        else
            bug("Close_socket: dclose not found.", 0);
    }

    // If doorman didn't tell us to disconnect them,
    // then tell doorman to kill the connection
    if (dclose->connected != CON_DISCONNECTING && dclose->connected != CON_DISCONNECTING_NP) {
        p.type = PACKET_DISCONNECT;
        p.channel = dclose->descriptor;
        p.nExtra = 0;
        SendPacket(&p, NULL);
    }

    free_string(dclose->host);
    /* RT socket leak fix -- I hope */
    free_mem(dclose->outbuf, dclose->outsize);
    /*    free_string(dclose->showstr_head); */
    dclose->next = descriptor_free;
    descriptor_free = dclose;
#if defined(MSDOS) || defined(macintosh) || defined(riscos)
    exit(1);
#endif
    return;
}

bool read_from_descriptor(DESCRIPTOR_DATA *d, char *data, int nRead) {
    int iStart;

#if 0
   /* Hold horses if pending command already. */
   if ( d->incomm[0] != '\0' )
      return TRUE;
#endif

    /* Check for overflow. */
    iStart = strlen(d->inbuf);
    if (iStart >= (int)sizeof(d->inbuf) - 10) {
        char hostbuf[MAX_MASKED_HOSTNAME];
        snprintf(log_buf, LOG_BUF_SIZE, "%s input overflow!", get_masked_hostname(hostbuf, d->host));
        log_string(log_buf);
        write_to_descriptor(d->descriptor, "\n\r*** PUT A LID ON IT!!! ***\n\r", 0);
        strcpy(d->incomm, "quit");
        return FALSE;
    }

    /* Snarf input. */
#if defined(macintosh)
    for (;;) {
        int c;
        c = getc(stdin);
        if (c == '\0' || c == EOF)
            break;
        putc(c, stdout);
        if (c == '\r')
            putc('\n', stdout);
        d->inbuf[iStart++] = c;
        if (iStart > sizeof(d->inbuf) - 10)
            break;
    }
#endif

    memcpy(d->inbuf + iStart, data, nRead);
    iStart += nRead;

    d->inbuf[iStart] = '\0';
    return TRUE;
}

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer(DESCRIPTOR_DATA *d) {
    int i, j, k;

    /*
     * Hold horses if pending command already.
     */
    if (d->incomm[0] != '\0')
        return;

    /*
     * Look for at least one new line.
     */
    for (i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++) {
        if (d->inbuf[i] == '\0')
            return;
    }

    /*
     * Canonical input processing.
     */
    for (i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++) {
        if (k >= MAX_INPUT_LENGTH - 2) {
            write_to_descriptor(d->descriptor, "Line too long.\n\r", 0);

            /* skip the rest of the line */
            for (; d->inbuf[i] != '\0'; i++) {
                if (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
                    break;
            }
            d->inbuf[i] = '\n';
            d->inbuf[i + 1] = '\0';
            break;
        }

        if (d->inbuf[i] == '\b' && k > 0)
            --k;
        else if (isascii(d->inbuf[i]) && isprint(d->inbuf[i]))
            d->incomm[k++] = d->inbuf[i];
    }

    /*
     * Finish off the line.
     */
    if (k == 0)
        d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    /*
     * Do '!' substitution.
     */
    if (d->incomm[0] == '!')
        strcpy(d->incomm, d->inlast);
    else
        strcpy(d->inlast, d->incomm);

    /*
     * Shift the input buffer.
     */
    while (d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
        i++;
    for (j = 0; (d->inbuf[j] = d->inbuf[i + j]) != '\0'; j++)
        ;
    return;
}

/*
 * Low level output function.
 */
bool process_output(DESCRIPTOR_DATA *d, bool fPrompt) {
    extern bool merc_down;

    /*
     * Bust a prompt.
     */
    if (!merc_down && d->showstr_point)
        write_to_buffer(d, "[Hit Return to continue]\n\r", 0);
    else if (fPrompt && !merc_down && d->connected == CON_PLAYING) {
        CHAR_DATA *ch;
        CHAR_DATA *victim;

        ch = d->character;

        /* battle prompt */
        if ((victim = ch->fighting) != NULL) {
            int percent;
            char wound[100];
            char buf[MAX_STRING_LENGTH];

            if (victim->max_hit > 0)
                percent = victim->hit * 100 / victim->max_hit;
            else
                percent = -1;

            if (percent >= 100)
                snprintf(wound, sizeof(wound), "is in excellent condition.");
            else if (percent >= 90)
                snprintf(wound, sizeof(wound), "has a few scratches.");
            else if (percent >= 75)
                snprintf(wound, sizeof(wound), "has some small wounds and bruises.");
            else if (percent >= 50)
                snprintf(wound, sizeof(wound), "has quite a few wounds.");
            else if (percent >= 30)
                snprintf(wound, sizeof(wound), "has some big nasty wounds and scratches.");
            else if (percent >= 15)
                snprintf(wound, sizeof(wound), "looks pretty hurt.");
            else if (percent >= 0)
                snprintf(wound, sizeof(wound), "is in awful condition.");
            else
                snprintf(wound, sizeof(wound), "is bleeding to death.");

            snprintf(buf, sizeof(buf), "%s %s \n\r", IS_NPC(victim) ? victim->short_descr : victim->name, wound);
            buf[0] = UPPER(buf[0]);
            write_to_buffer(d, buf, 0);
        }

        ch = d->original ? d->original : d->character;
        if (!IS_SET(ch->comm, COMM_COMPACT))
            write_to_buffer(d, "\n\r", 2);

        if (IS_SET(ch->comm, COMM_PROMPT)) {
            /* get the prompt for the character in question */

            show_prompt(d, ch->pcdata->prompt);
        }

        if (IS_SET(ch->comm, COMM_TELNET_GA))
            write_to_buffer(d, go_ahead_str, 0);
    }

    /*
     * Short-circuit if nothing to write.
     */
    if (d->outtop == 0)
        return TRUE;

    /*
     * Snoop-o-rama.
     */
    if (d->snoop_by != NULL) {
        if (d->character != NULL) {
            write_to_buffer(d->snoop_by, d->character->name, 0);

            write_to_buffer(d->snoop_by, "> ", 2);
            write_to_buffer(d->snoop_by, d->outbuf, d->outtop);
        }
    }

    /*
     * OS-dependent output.
     */
    if (!write_to_descriptor(d->descriptor, d->outbuf, d->outtop)) {
        d->outtop = 0;
        return FALSE;
    } else {
        d->outtop = 0;
        return TRUE;
    }
}

/*
 * Append onto an output buffer.
 */
void write_to_buffer(DESCRIPTOR_DATA *d, const char *txt, int length) {
    /*
     * Find length in case caller didn't.
     */
    if (length <= 0)
        length = strlen(txt);

    /*
     * Initial \n\r if needed.
     */
    if (d->outtop == 0 && !d->fcommand) {
        d->outbuf[0] = '\n';
        d->outbuf[1] = '\r';
        d->outtop = 2;
    }

    /*
     * Expand the buffer as needed.
     */
    while (d->outtop + length >= d->outsize) {
        char *outbuf;

        if (d->outsize > 32000) {
            bug("Buffer overflow. Closing.\n\r", 0);
            close_socket(d);
            return;
        }
        outbuf = alloc_mem(2 * d->outsize);
        strncpy(outbuf, d->outbuf, d->outtop);
        free_mem(d->outbuf, d->outsize);
        d->outbuf = outbuf;
        d->outsize *= 2;
    }

    /*
     * Copy.
     */
    strncpy(d->outbuf + d->outtop, txt, length);
    d->outtop += length;
    return;
}

/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
 * MG> desc == channel for doorman
 */
bool write_to_descriptor(int desc, char *txt, int length) {
    int iStart;
    int nWrite;
    int nBlock;
    Packet p;

#if defined(macintosh) || defined(MSDOS)
    if (desc == 0)
        desc = 1;
#endif

    if (length <= 0)
        length = strlen(txt);

    p.type = PACKET_MESSAGE;
    p.channel = desc;

    for (iStart = 0; iStart < length; iStart += nWrite) {
        nBlock = UMIN(length - iStart, 4096);

        p.nExtra = nBlock;
        SendPacket(&p, txt + iStart);
        nWrite = p.nExtra;
    }

    return TRUE;
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny(DESCRIPTOR_DATA *d, char *argument) {
    DESCRIPTOR_DATA *d_old, *d_next;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *ch;
    char *pwdnew;
    char *p;
    int iClass;
    int race;
    int i;
    int notes;
    bool fOld;
    KNOWN_PLAYERS *temp_known_player;
    /* Rohan: 2 variables needed to increase player count */
    int count;
    DESCRIPTOR_DATA *de;

    while (isspace(*argument))
        argument++;

    ch = d->character;

    switch (d->connected) {

    default:
        bug("Nanny: bad d->connected %d.", d->connected);
        close_socket(d);
        return;

    case CON_GET_NAME:
        if (argument[0] == '\0') {
            close_socket(d);
            return;
        }

        argument[0] = UPPER(argument[0]);
        if (!check_parse_name(argument)) {
            write_to_buffer(d, "Illegal name, try another.\n\rName: ", 0);
            return;
        }

        /* TM's attempt number 2 to prevent object cloning under the
              suspicion that the other wy was causing a hang on lagged entry */
        /*      for ( ch = char_list ; ch ; ch=ch->next ) {
                 if ( !IS_NPC(ch) &&
                 (!str_cmp(ch->name,argument)) &&
                 (ch->desc != NULL) ) {
                    write_to_buffer( d, "Illegal name, try another.\n\rName: ", 0 );
                    return;
                 }
                 } */

        fOld = load_char_obj(d, argument);
        ch = d->character;

        if (IS_SET(ch->act, PLR_DENY)) {
            char hostbuf[MAX_MASKED_HOSTNAME];
            snprintf(log_buf, LOG_BUF_SIZE, "Denying access to %s@%s.", argument,
                     get_masked_hostname(hostbuf, d->host));
            log_string(log_buf);
            write_to_buffer(d, "You are denied access.\n\r", 0);
            close_socket(d);
            return;
        }

        if (check_reconnect(d, argument, FALSE)) {
            fOld = TRUE;
        } else {
            if (wizlock && !IS_IMMORTAL(ch)) {
                write_to_buffer(d, "The game is wizlocked.  Try again later - a reboot may be imminent.\n\r", 0);
                close_socket(d);
                return;
            }
        }

        if (fOld) {
            /* Old player */
            write_to_buffer(d, "Password: ", 0);
            SetEchoState(d, 0);
            d->connected = CON_GET_OLD_PASSWORD;
            return;
        } else {
            /* New player */
            if (newlock) {
                write_to_buffer(d, "The game is newlocked.  Try again later - a reboot may be imminent.\n\r", 0);
                close_socket(d);
                return;
            }

            /* Check for a newban on player's site */
            if (check_ban(d->host, BAN_NEWBIES) || check_ban(d->host, BAN_PERMIT)) {
                write_to_buffer(d, "Your site has been banned.  Only existing players from your site may connect.\n\r",
                                0);
                close_socket(d);
                return;
            }

            snprintf(buf, sizeof(buf), "Did I hear that right -  '%s' (Y/N)? ", argument);
            write_to_buffer(d, buf, 0);
            d->connected = CON_CONFIRM_NEW_NAME;
            return;
        }
        break;

    case CON_GET_OLD_PASSWORD:
#if defined(unix)
        write_to_buffer(d, "\n\r", 2);
#endif

        // TODO crypt can return null if if fails (e.g. password is truncated).
        // for now we just pwd[0], which lets us reset passwords.
        if (ch->pcdata->pwd[0] && strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd)) {
            write_to_buffer(d, "Our survey said <Crude buzzer noise>.\n\rWrong password.\n\r", 0);
            close_socket(d);
            return;
        }
        // falls through
    case CON_CIRCUMVENT_PASSWORD:
        if (ch->pcdata->pwd[0] == 0) {
            write_to_buffer(d, "Oopsie! Null password!\n\r", 0);
            write_to_buffer(d, "Unless some IMM has been fiddling, then this is a bug!\n\r", 0);
            write_to_buffer(d, "Type 'password null <new password>' to fix.\n\r", 0);
        }

        SetEchoState(d, 1);

        if (check_ban(d->host, BAN_PERMIT) && (!is_set_extra(ch, EXTRA_PERMIT))) {
            write_to_buffer(d, "Your site has been banned.  Sorry.\n\r", 0);
            close_socket(d);
            return;
        }

        if (check_playing(d, ch->name))
            return;

        if (check_reconnect(d, ch->name, TRUE))
            return;

        char hostbuf[MAX_MASKED_HOSTNAME];
        snprintf(log_buf, LOG_BUF_SIZE, "%s@%s has connected.", ch->name, get_masked_hostname(hostbuf, d->host));
        log_new(log_buf, EXTRA_WIZNET_DEBUG,
                ((IS_SET(ch->act, PLR_WIZINVIS) || IS_SET(ch->act, PLR_PROWL))) ? get_trust(ch) : 0);

        write_to_buffer(d, "Does your terminal support ANSI colour (Y/N/Return = as saved)?", 0);
        d->connected = (d->connected == CON_CIRCUMVENT_PASSWORD) ? CON_READ_MOTD : CON_GET_ANSI;

        break;

        /* RT code for breaking link */

    case CON_BREAK_CONNECT:
        switch (*argument) {
        case 'y':
        case 'Y':
            for (d_old = descriptor_list; d_old != NULL; d_old = d_next) {
                d_next = d_old->next;
                if (d_old == d || d_old->character == NULL)
                    continue;

                if (str_cmp(ch->name, d_old->character->name))
                    continue;

                close_socket(d_old);
            }
            if (check_reconnect(d, ch->name, TRUE))
                return;
            write_to_buffer(d, "Reconnect attempt failed.\n\rName: ", 0);
            if (d->character != NULL) {
                free_char(d->character);
                d->character = NULL;
            }
            d->connected = CON_GET_NAME;
            break;

        case 'n':
        case 'N':
            write_to_buffer(d, "Name: ", 0);
            if (d->character != NULL) {
                free_char(d->character);
                d->character = NULL;
            }
            d->connected = CON_GET_NAME;
            break;

        default: write_to_buffer(d, "Please type Y or N? ", 0); break;
        }
        break;

    case CON_CONFIRM_NEW_NAME:
        switch (*argument) {
        case 'y':
        case 'Y':
            snprintf(buf, sizeof(buf),
                     "Welcome new character, to Xania.\n\rThink of a password for %s: ", (char *)ch->name);
            SetEchoState(d, 0);
            write_to_buffer(d, buf, 0);
            d->connected = CON_GET_NEW_PASSWORD;
            break;

        case 'n':
        case 'N':
            write_to_buffer(d, "Ack! Amateurs! Try typing it in properly: ", 0);
            free_char(d->character);
            d->character = NULL;
            d->connected = CON_GET_NAME;
            break;

        default: write_to_buffer(d, "It's quite simple - type Yes or No: ", 0); break;
        }
        break;

    case CON_GET_ANSI:
        if (argument[0] == '\0') {
            if (ch->pcdata->colour) {
                send_to_char("This is a |RC|GO|BL|rO|gU|bR|cF|YU|PL |RM|GU|BD|W!\n\r", ch);
            }
            if (IS_HERO(ch)) {
                do_help(ch, "imotd");
                d->connected = CON_READ_IMOTD;
            } else {
                do_help(ch, "motd");
                d->connected = CON_READ_MOTD;
            }
        } else {
            switch (*argument) {
            case 'y':
            case 'Y':
                ch->pcdata->colour = 1;
                send_to_char("This is a |RC|GO|BL|rO|gU|bR|cF|YU|PL |RM|GU|BD|W!\n\r", ch);
                if (IS_HERO(ch)) {
                    do_help(ch, "imotd");
                    d->connected = CON_READ_IMOTD;
                } else {
                    do_help(ch, "motd");
                    d->connected = CON_READ_MOTD;
                }
                break;

            case 'n':
            case 'N':
                ch->pcdata->colour = 0;
                if (IS_HERO(ch)) {
                    do_help(ch, "imotd");
                    d->connected = CON_READ_IMOTD;
                } else {
                    do_help(ch, "motd");
                    d->connected = CON_READ_MOTD;
                }
                break;

            default: write_to_buffer(d, "Please type Yes or No press return: ", 0); break;
            }
        }
        break;

    case CON_GET_NEW_PASSWORD:
#if defined(unix)
        write_to_buffer(d, "\n\r", 2);
#endif

        if (strlen(argument) < 5) {
            write_to_buffer(d, "Password must be at least five characters long.\n\rPassword: ", 0);
            return;
        }

        pwdnew = crypt(argument, ch->name);
        for (p = pwdnew; *p != '\0'; p++) {
            if (*p == '~') {
                write_to_buffer(d, "New password not acceptable, try again.\n\rPassword: ", 0);
                return;
            }
        }

        free_string(ch->pcdata->pwd);
        ch->pcdata->pwd = str_dup(pwdnew);
        write_to_buffer(d, "Please retype password: ", 0);
        d->connected = CON_CONFIRM_NEW_PASSWORD;
        break;

    case CON_CONFIRM_NEW_PASSWORD:
#if defined(unix)
        write_to_buffer(d, "\n\r", 2);
#endif

        if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd)) {
            write_to_buffer(d, "You could try typing the same thing in twice...\n\rRetype password: ", 0);
            d->connected = CON_GET_NEW_PASSWORD;
            return;
        }

        SetEchoState(d, 1);
        write_to_buffer(d, "The following races are available:\n\r  ", 0);
        for (race = 1; race_table[race].name != NULL; race++) {
            if (!race_table[race].pc_race)
                break;
            write_to_buffer(d, race_table[race].name, 0);
            write_to_buffer(d, " ", 1);
        }
        write_to_buffer(d, "\n\r", 0);
        write_to_buffer(d, "What is your race (help for more information)? ", 0);
        d->connected = CON_GET_NEW_RACE;
        break;

    case CON_GET_NEW_RACE:
        one_argument(argument, arg);

        if (!strcmp(arg, "help")) {
            argument = one_argument(argument, arg);
            if (argument[0] == '\0')
                do_help(ch, "race help");
            else
                do_help(ch, argument);
            write_to_buffer(d, "What is your race (help for more information)? ", 0);
            break;
        }

        race = race_lookup(argument);

        if (race == 0 || !race_table[race].pc_race) {
            write_to_buffer(d, "That is not a valid race.\n\r", 0);
            write_to_buffer(d, "The following races are available:\n\r  ", 0);
            for (race = 1; race_table[race].name != NULL; race++) {
                if (!race_table[race].pc_race)
                    break;
                write_to_buffer(d, race_table[race].name, 0);
                write_to_buffer(d, " ", 1);
            }
            write_to_buffer(d, "\n\r", 0);
            write_to_buffer(d, "What is your race? (help for more information) ", 0);
            break;
        }

        ch->race = race;
        /* initialize stats */
        for (i = 0; i < MAX_STATS; i++)
            ch->perm_stat[i] = pc_race_table[race].stats[i];
        ch->affected_by = (int)(ch->affected_by | race_table[race].aff);
        ch->imm_flags = ch->imm_flags | race_table[race].imm;
        ch->res_flags = ch->res_flags | race_table[race].res;
        ch->vuln_flags = ch->vuln_flags | race_table[race].vuln;
        ch->form = race_table[race].form;
        ch->parts = race_table[race].parts;

        /* add skills */
        for (i = 0; i < 5; i++) {
            if (pc_race_table[race].skills[i] == NULL)
                break;
            group_add(ch, pc_race_table[race].skills[i], FALSE);
        }
        /* add cost */
        ch->pcdata->points = pc_race_table[race].points;
        ch->size = pc_race_table[race].size;

        write_to_buffer(d, "What is your sex (M/F)? ", 0);
        d->connected = CON_GET_NEW_SEX;
        break;

    case CON_GET_NEW_SEX:
        switch (argument[0]) {
        case 'm':
        case 'M':
            ch->sex = SEX_MALE;
            ch->pcdata->true_sex = SEX_MALE;
            break;
        case 'f':
        case 'F':
            ch->sex = SEX_FEMALE;
            ch->pcdata->true_sex = SEX_FEMALE;
            break;
        default: write_to_buffer(d, "That's not a sex.\n\rWhat IS your sex? ", 0); return;
        }

        strcpy(buf, "Select a class [");
        for (iClass = 0; iClass < MAX_CLASS; iClass++) {
            if (iClass > 0)
                strcat(buf, " ");
            strcat(buf, class_table[iClass].name);
        }
        strcat(buf, "]: ");
        write_to_buffer(d, buf, 0);
        d->connected = CON_GET_NEW_CLASS;
        break;

    case CON_GET_NEW_CLASS:
        iClass = class_lookup(argument);

        if (iClass == -1) {
            write_to_buffer(d, "That's not a class.\n\rWhat IS your class? ", 0);
            return;
        }

        ch->class = iClass;
        snprintf(log_buf, LOG_BUF_SIZE, "%s@%s new player.", ch->name, get_masked_hostname(hostbuf, d->host));
        log_string(log_buf);
        write_to_buffer(d, "\n\r", 2);
        write_to_buffer(d, "You may be good, neutral, or evil.\n\r", 0);
        write_to_buffer(d, "Which alignment (G/N/E)? ", 0);
        d->connected = CON_GET_ALIGNMENT;
        break;

    case CON_GET_ALIGNMENT:
        switch (argument[0]) {
        case 'g':
        case 'G': ch->alignment = 750; break;
        case 'n':
        case 'N': ch->alignment = 0; break;
        case 'e':
        case 'E': ch->alignment = -750; break;
        default:
            write_to_buffer(d, "That's not a valid alignment.\n\r", 0);
            write_to_buffer(d, "Which alignment (G/N/E)? ", 0);
            return;
        }

        write_to_buffer(d, "\n\r", 0);

        group_add(ch, "rom basics", FALSE);
        group_add(ch, class_table[ch->class].base_group, FALSE);
        ch->pcdata->learned[gsn_recall] = 50;
        write_to_buffer(d, "Do you wish to customize this character?\n\r", 0);
        write_to_buffer(d, "Customization takes time, but allows a wider range of skills and abilities.\n\r", 0);
        write_to_buffer(d, "Customize (Y/N)? ", 0);
        d->connected = CON_DEFAULT_CHOICE;
        break;

    case CON_DEFAULT_CHOICE:
        write_to_buffer(d, "\n\r", 2);
        switch (argument[0]) {
        case 'y':
        case 'Y':
            ch->gen_data = alloc_perm(sizeof(*ch->gen_data));
            ch->gen_data->points_chosen = ch->pcdata->points;
            do_help(ch, "group header");
            list_group_costs(ch);
            write_to_buffer(d, "You already have the following skills:\n\r", 0);
            do_skills(ch, "");
            do_help(ch, "menu choice");
            d->connected = CON_GEN_GROUPS;
            break;
        case 'n':
        case 'N':
            group_add(ch, class_table[ch->class].default_group, TRUE);
            write_to_buffer(d, "\n\r", 2);
            write_to_buffer(d, "Does your terminal support ANSI colour (Y/N/Return = as saved)?", 0);
            d->connected = CON_GET_ANSI;

            break;
        default: write_to_buffer(d, "Please answer (Y/N)? ", 0); return;
        }
        break;

    case CON_GEN_GROUPS:
        send_to_char("\n\r", ch);
        if (!str_cmp(argument, "done")) {
            snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->pcdata->points);
            send_to_char(buf, ch);
            snprintf(buf, sizeof(buf), "Experience per level: %d\n\r", exp_per_level(ch, ch->gen_data->points_chosen));
            if (ch->pcdata->points < 40)
                ch->train = (40 - ch->pcdata->points + 1) / 2;
            send_to_char(buf, ch);
            write_to_buffer(d, "\n\r", 2);
            write_to_buffer(d, "Does your terminal support ANSI colour (Y/N/Return = as saved)?", 0);
            d->connected = CON_GET_ANSI;

            break;
        }

        if (!parse_gen_groups(ch, argument))
            send_to_char("Choices are: list,learned,premise,add,drop,info,help, and done.\n\r", ch);

        do_help(ch, "menu choice");
        break;

    case CON_READ_IMOTD:
        write_to_buffer(d, "\n\r", 2);
        do_help(ch, "motd");
        d->connected = CON_READ_MOTD;
        break;

    case CON_READ_MOTD:
        write_to_buffer(d, "\n\rWelcome to Xania.  May your stay be eventful.\n\r", 0);
        ch->next = char_list;
        char_list = ch;
        d->connected = CON_PLAYING;
        reset_char(ch);

        /* Moog: tell doorman we logged in OK */
        {
            Packet p;
            p.type = PACKET_AUTHORIZED;
            p.channel = d->descriptor;
            p.nExtra = strlen(ch->name) + 1;
            SendPacket(&p, ch->name);
        }

        if (ch->level == 0) {

            if (ch->race != race_lookup("dragon"))
                ch->perm_stat[class_table[ch->class].attr_prime] += 3;

            ch->level = 1;
            ch->exp = exp_per_level(ch, ch->pcdata->points);
            ch->hit = ch->max_hit;
            ch->mana = ch->max_mana;
            ch->move = ch->max_move;
            ch->train = 3;
            ch->practice = 5;
            snprintf(buf, sizeof(buf), "the %s", title_table[ch->class][ch->level][ch->sex == SEX_FEMALE ? 1 : 0]);
            set_title(ch, buf);

            do_outfit(ch, "");
            obj_to_char(create_object(get_obj_index(OBJ_VNUM_MAP), 0), ch);

            ch->pcdata->learned[get_weapon_sn(ch)] = 40;

            char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL));
            send_to_char("\n\r", ch);
            do_help(ch, "NEWBIE INFO");
            send_to_char("\n\r", ch);

            /* Rohan: New player logged in, need to add name to player list */
            temp_known_player = alloc_mem(sizeof(KNOWN_PLAYERS));
            temp_known_player->name = strdup(ch->name);
            temp_known_player->next = player_list;
            player_list = temp_known_player;
            /* hack to let the newbie know about the tipwizard */
            send_to_char("|WTip: this is Xania's tip wizard! Type 'tips' to turn this on or off.|w\n\r", ch);
            /* turn on the newbie's tips */
            set_extra(ch, EXTRA_TIP_WIZARD);

        } else if (ch->in_room != NULL) {
            char_to_room(ch, ch->in_room);
        } else if (IS_IMMORTAL(ch)) {
            char_to_room(ch, get_room_index(ROOM_VNUM_CHAT));
        } else {
            char_to_room(ch, get_room_index(ROOM_VNUM_TEMPLE));
        }

        snprintf(log_buf, LOG_BUF_SIZE, "|W### |P%s|W has entered the game.|w", ch->name);
        announce(log_buf, ch);
        act("|P$n|W has entered the game.", ch, NULL, NULL, TO_ROOM);
        do_look(ch, "auto");

        /* Rohan: code to increase the player count if needed - it was only
           updated if a player did count */
        count = 0;
        for (de = descriptor_list; de != NULL; de = de->next)
            if (de->connected == CON_PLAYING)
                count++;
        max_on = UMAX(count, max_on);

        if (ch->gold > 250000 && !IS_IMMORTAL(ch)) {
            snprintf(buf, sizeof(buf), "You are taxed %ld gold to pay for the Mayor's bar.\n\r",
                     (ch->gold - 250000) / 2);
            send_to_char(buf, ch);
            ch->gold -= (ch->gold - 250000) / 2;
        }

        if (ch->pet != NULL) {
            char_to_room(ch->pet, ch->in_room);
            act("|P$n|W has entered the game.", ch->pet, NULL, NULL, TO_ROOM);
        }

        /* check notes */
        notes = note_count(ch);

        if (notes > 0) {
            snprintf(buf, sizeof(buf), "\n\rYou have %d new note%s waiting.\n\r", notes, (notes == 1) ? "" : "s");
            send_to_char(buf, ch);
        }
        move_to_next_unread(ch);
        news_info(ch);
        break;
    }

    return;
}

/*
 * Parse a name for acceptability.
 */
bool check_parse_name(char *name) {
    /*
     * Reserved words.
     */
    if (is_name(name, "all auto immortal self someone something the you"))
        return FALSE;

    if (!str_cmp(capitalize(name), "DEATH"))
        return TRUE;

    /*    if (str_cmp(capitalize(name),"DEATH") && (!str_prefix("death",name)
        || !str_suffix("Death",name)))
       return FALSE;*/

    /*
     * Length restrictions.
     */

    if (strlen(name) < 2)
        return FALSE;

#if defined(MSDOS)
    if (strlen(name) > 8)
        return FALSE;
#endif

#if defined(macintosh) || defined(unix)
    if (strlen(name) > 12)
        return FALSE;
#endif

    /*
     * Alphanumerics only.
     * Lock out IllIll twits.
     */
    {
        char *pc;
        bool fIll;

        fIll = TRUE;
        for (pc = name; *pc != '\0'; pc++) {
            if (!isalpha(*pc))
                return FALSE;
            if (LOWER(*pc) != 'i' && LOWER(*pc) != 'l')
                fIll = FALSE;
        }

        if (fIll)
            return FALSE;
    }

    /*
     * Prevent players from naming themselves after mobs.
     */
    {
        extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
        MOB_INDEX_DATA *pMobIndex;
        int iHash;

        for (iHash = 0; iHash < MAX_KEY_HASH; iHash++) {
            for (pMobIndex = mob_index_hash[iHash]; pMobIndex != NULL; pMobIndex = pMobIndex->next) {
                if (is_name(name, pMobIndex->player_name))
                    return FALSE;
            }
        }
    }

    return TRUE;
}

/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect(DESCRIPTOR_DATA *d, char *name, bool fConn) {
    (void)name;
    CHAR_DATA *ch;

    for (ch = char_list; ch != NULL; ch = ch->next) {
        if (!IS_NPC(ch) && (!fConn || ch->desc == NULL) && !str_cmp(d->character->name, ch->name)) {
            if (fConn == FALSE) {
                free_string(d->character->pcdata->pwd);
                d->character->pcdata->pwd = str_dup(ch->pcdata->pwd);
            } else {
                char hostbuf[MAX_MASKED_HOSTNAME];
                free_char(d->character);
                d->character = ch;
                ch->desc = d;
                ch->timer = 0;
                send_to_char("Reconnecting.\n\r", ch);
                act("$n has reconnected.", ch, NULL, NULL, TO_ROOM);
                snprintf(log_buf, LOG_BUF_SIZE, "%s@%s reconnected.", ch->name, get_masked_hostname(hostbuf, d->host));
                log_new(log_buf, EXTRA_WIZNET_DEBUG,
                        ((IS_SET(ch->act, PLR_WIZINVIS) || IS_SET(ch->act, PLR_PROWL))) ? get_trust(ch) : 0);
                d->connected = CON_PLAYING;
            }
            return TRUE;
        }
    }

    return FALSE;
}

/*
 * Check if already playing.
 */
bool check_playing(DESCRIPTOR_DATA *d, char *name) {
    DESCRIPTOR_DATA *dold;

    for (dold = descriptor_list; dold; dold = dold->next) {
        if (dold != d && dold->character != NULL && dold->connected != CON_GET_NAME
            && dold->connected != CON_GET_OLD_PASSWORD
            && !str_cmp(name, dold->original ? dold->original->name : dold->character->name)) {
            write_to_buffer(d, "That character is already playing.\n\r", 0);
            write_to_buffer(d, "Do you wish to connect anyway (Y/N)?", 0);
            d->connected = CON_BREAK_CONNECT;
            return TRUE;
        }
    }

    return FALSE;
}

void stop_idling(CHAR_DATA *ch) {
    if (ch == NULL || ch->desc == NULL || ch->desc->connected != CON_PLAYING || ch->was_in_room == NULL
        || ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
        return;

    ch->timer = 0;
    char_from_room(ch);
    char_to_room(ch, ch->was_in_room);
    ch->was_in_room = NULL;
    act("$n has returned from the void.", ch, NULL, NULL, TO_ROOM);
    return;
}

/*
 * Write to one char.
 */
void send_to_char(const char *txt, CHAR_DATA *ch) {
    int colour = 0;
    char sendcolour, temp;
    char buf[MAX_STRING_LENGTH];
    const char *p;
    char *bufptr;

    if (txt != NULL && ch->desc != NULL) {
        if (!IS_NPC(ch) && (ch->pcdata->colour))
            colour = 1;
        if (IS_NPC(ch) && (ch->desc->original != NULL) /* paranoia factor */
            && (ch->desc->original->pcdata->colour))
            colour = 1;

        for (p = txt, bufptr = buf; *p != 0;) {
            if (*p == '|') {
                p++;
                switch (*p) {
                case 'r':
                case 'R': sendcolour = '1'; break;
                case 'g':
                case 'G': sendcolour = '2'; break;
                case 'y':
                case 'Y': sendcolour = '3'; break;
                case 'b':
                case 'B': sendcolour = '4'; break;
                case 'm':
                case 'p':
                case 'M':
                case 'P': sendcolour = '5'; break;
                case 'c':
                case 'C': sendcolour = '6'; break;
                default: sendcolour = 'z'; break;
                case 'w':
                case 'W': sendcolour = '7'; break;
                case '|':
                    sendcolour = 'z';
                    p++; /* to move pointer on */
                    break;
                }
                if (sendcolour == 'z') {
                    *bufptr++ = '|';
                    continue;
                }
                temp = *p;
                p++;
                if (colour) {
                    *bufptr++ = 27;
                    *bufptr++ = '[';
                    if (temp >= 'a') {
                        *bufptr++ = '0';
                    } else {
                        *bufptr++ = '1';
                    }
                    *bufptr++ = ';';
                    *bufptr++ = '3';
                    *bufptr++ = sendcolour;
                    *bufptr++ = 'm';
                }

            } else {
                *bufptr++ = *p++;
            }
        }
        *bufptr = '\0';
        write_to_buffer(ch->desc, buf, strlen(buf));
    }
    return;
}

/*
 * Send a page to one char.
 */
void page_to_char(const char *txt, CHAR_DATA *ch) {
    if (txt == NULL || ch->desc == NULL)
        return;

#if defined(macintosh)
    send_to_char(txt, ch);
#else
    ch->desc->showstr_head = alloc_mem(strlen(txt) + 1);
    strcpy(ch->desc->showstr_head, txt);
    ch->desc->showstr_point = ch->desc->showstr_head;
    show_string(ch->desc, "");
#endif
}

/* string pager */
void show_string(struct descriptor_data *d, char *input) {
    char buffer[4 * MAX_STRING_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    register char *scan, *chk;
    int lines = 0, toggle = 1;
    int show_lines;

    one_argument(input, buf);
    if (buf[0] != '\0') {
        if (d->showstr_head) {
            free_string(d->showstr_head);
            d->showstr_head = 0;
        }
        d->showstr_point = 0;
        return;
    }

    if (d->character)
        show_lines = d->character->lines;
    else
        show_lines = 0;

    for (scan = buffer;; scan++, d->showstr_point++) {
        if (((*scan = *d->showstr_point) == '\n' || *scan == '\r') && (toggle = -toggle) < 0)
            lines++;

        else if (!*scan || (show_lines > 0 && lines >= show_lines)) {
            *scan = '\0';
            /*     write_to_buffer(d,buffer,strlen(buffer));*/
            send_to_char(buffer, d->character);
            for (chk = d->showstr_point; isspace(*chk); chk++)
                /* nothing */;
            if (!*chk) {
                if (d->showstr_head) {
                    free_string(d->showstr_head);
                    d->showstr_head = 0;
                }
                d->showstr_point = 0;
            }
            return;
        }
    }
    return;
}

/* quick sex fixer */
void fix_sex(CHAR_DATA *ch) {
    if (ch->sex < 0 || ch->sex > 2)
        ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
}

void act(const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type) {
    /* to be compatible with older code */
    act_new(format, ch, arg1, arg2, type, POS_RESTING);
}

void act_new(const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2, int type, int min_pos) {
    static char *const he_she[] = {"it", "he", "she"};
    static char *const him_her[] = {"it", "him", "her"};
    static char *const his_her[] = {"its", "his", "her"};

    char buf[MAX_STRING_LENGTH];
    char fname[MAX_INPUT_LENGTH];
    CHAR_DATA *to;
    CHAR_DATA *vch = (CHAR_DATA *)arg2;
    OBJ_DATA *obj1 = (OBJ_DATA *)arg1;
    OBJ_DATA *obj2 = (OBJ_DATA *)arg2;
    ROOM_INDEX_DATA *givenRoom = (ROOM_INDEX_DATA *)arg2; /* in case TO_GIVENROOM is used */
    const char *str;
    const char *i;
    char *point;
    buf[0] = '\0';

    /* Discard null and zero-length messages. */
    if (format == NULL || format[0] == '\0')
        return;

    /* discard null rooms and chars */
    if (ch == NULL || ch->in_room == NULL)
        return;
    if (type == TO_GIVENROOM && givenRoom == NULL) {
        bug("Act: null givenRoom with TO_GIVENROOM.", 0);
        return;
    }

    if (type == TO_GIVENROOM)
        to = givenRoom->people;
    else
        to = ch->in_room->people;

    if (type == TO_VICT) {
        if (vch == NULL) {
            bug("Act: null vch with TO_VICT.", 0);
            return;
        }

        if (vch->in_room == NULL)
            return;

        to = vch->in_room->people;
    }

    for (; to != NULL; to = to->next_in_room) {
        if (to->desc == NULL || to->position < min_pos)
            continue;

        if (type == TO_CHAR && to != ch)
            continue;
        if (type == TO_VICT && (to != vch || to == ch))
            continue;
        if (type == TO_ROOM && to == ch && ch->in_room->vnum != CHAL_ROOM)
            continue;
        if (type == TO_GIVENROOM && to == ch && ch->in_room->vnum != CHAL_ROOM)
            continue;
        if (type == TO_NOTVICT && (to == ch || to == vch) && ch->in_room->vnum != CHAL_ROOM)
            continue;

        point = buf;
        str = format;
        while (*str != '\0') {
            if (*str != '$') {
                *point++ = *str++;
                continue;
            }
            ++str;

            if (arg2 == NULL && *str >= 'A' && *str <= 'Z') {
                bug("Act: missing arg2 for code %d.", *str);
                i = " <@@@> ";
            } else {
                switch (*str) {
                default:
                    bug("Act: bad code %d.", *str);
                    i = " <@@@> ";
                    break;
                    /* Thx alex for 't' idea */
                case 't': i = (char *)arg1; break;
                case 'T': i = (char *)arg2; break;
                case 'n': i = PERS(ch, to); break;
                case 'N': i = PERS(vch, to); break;
                case 'e': i = he_she[URANGE(0, ch->sex, 2)]; break;
                case 'E': i = he_she[URANGE(0, vch->sex, 2)]; break;
                case 'm': i = him_her[URANGE(0, ch->sex, 2)]; break;
                case 'M': i = him_her[URANGE(0, vch->sex, 2)]; break;
                case 's': i = his_her[URANGE(0, ch->sex, 2)]; break;
                case 'S': i = his_her[URANGE(0, vch->sex, 2)]; break;

                case 'p': i = can_see_obj(to, obj1) ? obj1->short_descr : "something"; break;

                case 'P': i = can_see_obj(to, obj2) ? obj2->short_descr : "something"; break;

                case 'd':
                    if (arg2 == NULL || ((char *)arg2)[0] == '\0') {
                        i = "door";
                    } else {
                        one_argument((char *)arg2, fname);
                        i = fname;
                    }
                    break;
                }
            }

            ++str;
            while ((*point = *i) != '\0')
                ++point, ++i;
        }

        *point++ = '\n';
        *point++ = '\r';
        *point++ = '\0'; /* Added by TM */
        for (point = buf; *point != 0; point++) {
            if (*point == '|') {
                point++;
            } else {
                *point = UPPER(*point);
                break;
            }
        }

        /*        buf[0]   = UPPER(buf[0]); */

        /*      write_to_buffer( to->desc, buf, point - buf ); used to be this */
        if ((type == TO_ROOM && to == ch) || (type == TO_NOTVICT && (to == ch || to == vch))) {
            /* Ignore them */
        } else {
            send_to_char(buf, to);
            /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
            if (MOBtrigger)
                mprog_act_trigger(buf, to, ch, obj1, vch);
        }
    }
    MOBtrigger = TRUE;

    if (ch->in_room != NULL) {
        if ((type == TO_ROOM || type == TO_NOTVICT) && ch->in_room->vnum == CHAL_ROOM) {
            DESCRIPTOR_DATA *d;

            for (d = descriptor_list; d != NULL; d = d->next) {
                if (d->character != NULL && d->character->in_room != NULL && d->character->in_room->vnum == 1222
                    && IS_AWAKE(d->character))
                    send_to_char(buf, d->character);
            }
        }
    }

    return;
}

void show_prompt(DESCRIPTOR_DATA *d, char *prompt) {
    char buf[256]; /* this is actually sent to the ch */
    char buf2[64];
    CHAR_DATA *ch;
    CHAR_DATA *ch_prefix = NULL; /* Needed for prefix in prompt with switched MOB */
    char *point;
    char *str;
    char *i;
    int bufspace = 255; /* Counter of space left in buf[] */

    ch = d->character;
    /*
     * Discard null and zero-length prompts.
     */

    if (!IS_NPC(ch) && (ch->pcdata->colour))
        send_to_char("|p", ch);

    if (IS_NPC(ch)) {
        if (ch->desc->original)
            ch_prefix = ch->desc->original;
    } else
        ch_prefix = ch;

    if (prompt == NULL || prompt[0] == '\0') {
        snprintf(buf, sizeof(buf), "<%d/%dhp %d/%dm %dmv> |w", ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move);
        *(buf + strlen(buf)) = '\0';
        send_to_char(buf, ch);
        return;
    }

    point = buf;
    buf[0] = '\0';
    str = prompt; /* zero-terminated raw prompt macro */
    while ((*str != '\0') && (bufspace >= 0)) {
        float tmp, loop;
        char bar[256];

        if (*str != '%') {
            if (bufspace-- > 0) {
                *point++ = *str;
                *point = '\0';
            }
            str++;
            continue;
        }
        ++str;

        switch (*str) {
        default: i = "|W ?? |p"; break;

        case 'B':
            buf2[0] = '\0';
            bar[0] = '\0';
            strcpy(bar, "|g||");

            tmp = (float)ch->hit / (float)(ch->max_hit / 10.0);
            for (loop = 0.0; loop < 10.0; loop++) {
                if (loop > tmp) {
                    bar[2] = ' ';
                    bar[3] = '\0';
                }
                if (loop <= 10.0)
                    bar[1] = 'g';
                if (loop < 6.0)
                    bar[1] = 'y';
                if (loop < 3.0)
                    bar[1] = 'r';
                i = buf2;
                strcat(buf2, bar);
            }
            strcat(buf2, "|p");

            break;
        case 'h':
            i = buf2;
            snprintf(buf2, sizeof(buf2), "%d", ch->hit);
            break;
        case 'H':
            i = buf2;
            snprintf(buf2, sizeof(buf2), "%d", ch->max_hit);
            break;
        case 'm':
            i = buf2;
            snprintf(buf2, sizeof(buf2), "%d", ch->mana);
            break;
        case 'M':
            i = buf2;
            snprintf(buf2, sizeof(buf2), "%d", ch->max_mana);
            break;
        case 'g':
            i = buf2;
            snprintf(buf2, sizeof(buf2), "%ld", ch->gold);
            break;
        case 'x':
            i = buf2;
            snprintf(buf2, sizeof(buf2), "%ld",
                     (long int)(((long int)(ch->level + 1) * (long int)(exp_per_level(ch, ch->pcdata->points))
                                 - (long int)(ch->exp))));
            break;
        case 'v':
            i = buf2;
            snprintf(buf2, sizeof(buf2), "%d", ch->move);
            break;
        case 'V':
            i = buf2;
            snprintf(buf2, sizeof(buf2), "%d", ch->max_move);
            break;
        case 'a':
            i = buf2;
            if (get_trust(ch) >= 10) {
                snprintf(buf2, sizeof(buf2), "%d", ch->alignment);
            } else {
                snprintf(buf2, sizeof(buf2), "??");
            }
            break;
        case 'X':
            i = buf2;
            snprintf(buf2, sizeof(buf2), "%ld", ch->exp);
            break;
            /* PCFN 20-05-97 prefix */
        case 'p':
            if (ch_prefix) {
                i = ch_prefix->pcdata->prefix;
            } else {
                i = "\0";
            }
            break;
            /* end */
        case 'r':
            if (IS_IMMORTAL(ch)) {
                i = ch->in_room->name;
            } else {
                i = "|W ?? |p";
            }

            break;
        case 'W':
            if IS_IMMORTAL (ch) {
                i = (IS_SET(ch->act, PLR_WIZINVIS)) ? "|R*W*|p" : "-w-";
            } else {
                i = "|W ?? |p";
            }
            break;
            /* Can be changed easily for tribes/clans etc. */
        case 'P':
            if (IS_IMMORTAL(ch)) {
                i = (IS_SET(ch->act, PLR_PROWL)) ? "|R*P*|p" : "-p-";
            } else {
                i = "|W ?? |p";
            }
            break;
        case 'R':
            if IS_IMMORTAL (ch) {
                i = buf2;
                snprintf(buf2, sizeof(buf2), "%d", ch->in_room->vnum);
            } else {
                i = "|W ?? |p";
            }
            break;

        case 'z':
            if IS_IMMORTAL (ch) {
                i = ch->in_room->area->name;
            } else {
                i = "|W ?? |p";
            }
            break;

        case 'n': i = "\n\r"; break;

        case 't': {
            time_t ch_timet;

            ch_timet = time(0);

            if (ch_prefix->pcdata->houroffset || ch_prefix->pcdata->minoffset) {
                struct tm *ch_time;

                ch_time = gmtime(&ch_timet);
                ch_time->tm_min += ch_prefix->pcdata->minoffset;
                ch_time->tm_hour += ch_prefix->pcdata->houroffset;

                ch_time->tm_hour -= (ch_time->tm_min / 60);
                ch_time->tm_min = (ch_time->tm_min % 60);
                if (ch_time->tm_min < 0) {
                    ch_time->tm_min += 60;
                    ch_time->tm_hour -= 1;
                }
                ch_time->tm_hour = (ch_time->tm_hour % 24);
                if (ch_time->tm_hour < 0)
                    ch_time->tm_hour += 24;

                strftime(i = buf2, 63, "%H:%M:%S", ch_time);
            } else
                strftime(i = buf2, 63, "%H:%M:%S", localtime(&ch_timet));
        } break;
        }

        ++str;
        strncpy(point, i, bufspace - 1);
        if ((int)strlen(i) > (bufspace - 1))
            point[bufspace - 1] = '\0';
        point += strlen(i);
        if ((bufspace -= strlen(i)) <= 0)
            break;

        /*      while ( ( *point = *i ) != '\0' )
                 ++point, ++i;*/
    }

    if (bufspace > 0) {
        strncpy(point, "|w", bufspace - 1);
        point[bufspace - 1] = '\0';
    }

    send_to_char(buf, ch);

    return;
}

/*
 * Macintosh support functions.
 */
#if defined(macintosh)
int gettimeofday(struct timeval *tp, void *tzp) {
    tp->tv_sec = time(NULL);
    tp->tv_usec = 0;
}
#endif
