/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  comm.c: the core of the MUD, handles initialisation and networking   */
/*                                                                       */
/*************************************************************************/

/*
 * Originally contained a ton of OS-specific stuff.
 * We only support linux these days...so we stopped pretending...
 */

#include "comm.hpp"
#include "Descriptor.hpp"
#include <arpa/telnet.h>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <string>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "challeng.h"
#include "doorman/doorman_protocol.h"
#include "interp.h"
#include "merc.h"
#include "news.h"
#include "note.h"
#include "string_utils.hpp"

/* Added by Rohan - extern to player list for adding new players to it */
extern KNOWN_PLAYERS *player_list;

/* extern void identd_lookup();  ......commented out due to sock leaks */

char str_boot_time[MAX_INPUT_LENGTH];

/*
 * Socket and TCP/IP stuff.
 */

const char echo_off_str[] = {char(IAC), char(WILL), char(TELOPT_ECHO), '\0'};
const char echo_on_str[] = {char(IAC), char(WONT), char(TELOPT_ECHO), '\0'};
const char go_ahead_str[] = {char(IAC), char(GA), '\0'};

/*
 * Global variables.
 */
Descriptor *descriptor_list; /* All open descriptors    */
Descriptor *d_next; /* Next descriptor in loop */
FILE *fpReserve; /* Reserved file handle    */
bool god; /* All new chars are gods! */
bool merc_down; /* Shutdown       */
bool wizlock; /* Game is wizlocked    */
bool newlock; /* Game is newlocked    */
time_t current_time; /* time of this pulse */
bool MOBtrigger;

void game_loop_unix(int control);
int init_socket(const char *file);
Descriptor *new_descriptor(int channel);
bool read_from_descriptor(Descriptor *d, char *text, int nRead);
void move_active_char_from_limbo(CHAR_DATA *ch);

/*
 * Other local functions (OS-independent).
 */
bool check_parse_name(const char *name);
bool check_reconnect(Descriptor *d, bool fConn);
bool check_playing(Descriptor *d, char *name);
void nanny(Descriptor *d, const char *argument);
bool process_output(Descriptor *d, bool fPrompt);
void show_prompt(Descriptor *d, char *prompt);

/* Handle to get to doorman */
int doormanDesc = 0;

/* Send a packet to doorman */
bool SendPacket(Packet *p, const void *extra) {
    // TODO: do something rather than return here if there's a failure
    if (!doormanDesc)
        return false;
    if (p->nExtra > PACKET_MAX_PAYLOAD_SIZE) {
        bug("MUD tried to send a doorman packet with payload size %d > %d! Dropping!", p->nExtra,
            PACKET_MAX_PAYLOAD_SIZE);
        return false;
    }
    int bytes_written = write(doormanDesc, (char *)p, sizeof(Packet));
    if (bytes_written != sizeof(Packet))
        return false;
    if (p->nExtra) {
        bytes_written = write(doormanDesc, extra, p->nExtra);
        if (bytes_written != (ssize_t)(p->nExtra))
            return false;
    }
    return true;
}

void SetEchoState(Descriptor *d, int on) {
    Packet p;
    p.type = on ? PACKET_ECHO_ON : PACKET_ECHO_OFF;
    p.channel = d->channel();
    p.nExtra = 0;
    SendPacket(&p, nullptr);
}

/* where we're asked nicely to quit from the outside (mudmgr or OS) */
void handle_signal_shutdown() {
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    Descriptor *d, *d_next;

    log_string("Signal shutdown received");

    /* ask everyone to save! */
    for (vch = char_list; vch != nullptr; vch = vch_next) {
        vch_next = vch->next;

        // vch->d->c check added by TM to avoid crashes when
        // someone hasn't logged in but the mud is shut down
        if (!IS_NPC(vch) && vch->desc && vch->desc->is_playing()) {
            /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
            MOBtrigger = false;
            do_save(vch, "");
            send_to_char("|RXania has been asked to shutdown by the operating system.|w\n\r", vch);
            if (vch->desc && vch->desc->has_buffered_output())
                process_output(vch->desc, false);
        }
    }
    merc_down = true;
    for (d = descriptor_list; d != nullptr; d = d_next) {
        d_next = d->next;
        close_socket(d);
    }
}

int init_socket(const char *file) {
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

void doorman_lost() {
    log_string("Lost connection to doorman");
    if (doormanDesc)
        close(doormanDesc);
    doormanDesc = 0;
    /* Now to go through and disconnect all the characters */
    while (descriptor_list)
        close_socket(descriptor_list);
}

void game_loop_unix(int control) {
    static struct timeval null_time;
    struct timeval last_time;

    signal(SIGPIPE, SIG_IGN);

    sigset_t signals;
    sigemptyset(&signals);
    sigaddset(&signals, SIGTERM);
    sigaddset(&signals, SIGINT);
    sigprocmask(SIG_BLOCK, &signals, nullptr);
    int signal_fd = signalfd(-1, &signals, SFD_NONBLOCK);
    if (signal_fd < 0) {
        perror("signal_fd");
        return;
    }

    gettimeofday(&last_time, nullptr);
    current_time = (time_t)last_time.tv_sec;

    /* Main loop */
    while (!merc_down) {
        fd_set in_set;
        fd_set out_set;
        fd_set exc_set;
        int maxdesc;
        Descriptor *d, *dNext;

        /*
         * Poll all active descriptors.
         */
        FD_ZERO(&in_set);
        FD_ZERO(&out_set);
        FD_ZERO(&exc_set);
        FD_SET(control, &in_set);
        FD_SET(signal_fd, &in_set);
        if (doormanDesc)
            FD_SET(doormanDesc, &in_set);
        maxdesc = UMAX(control, doormanDesc);
        maxdesc = UMAX(maxdesc, signal_fd);

        if (select(maxdesc + 1, &in_set, &out_set, &exc_set, &null_time) < 0) {
            perror("Game_loop: select: poll");
            exit(1);
        }

        if (FD_ISSET(signal_fd, &in_set)) {
            struct signalfd_siginfo info;
            if (read(signal_fd, &info, sizeof(info)) != sizeof(info)) {
                bug("Unable to read signal info - treating as term");
                info.ssi_signo = SIGTERM;
            }
            if (info.ssi_signo == SIGTERM || info.ssi_signo == SIGINT) {
                handle_signal_shutdown();
                return;
            } else {
                bug("Unexpected signal %d: ignoring", info.ssi_signo);
            }
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
                SendPacket(&pInit, nullptr);
            }
        }

        /* Process doorman input, if any */
        if (doormanDesc && FD_ISSET(doormanDesc, &in_set)) {
            do {
                Packet p;
                char buffer[UMAX(PACKET_MAX_PAYLOAD_SIZE, 4096 * 4)];
                int nBytes;
                int ok;

                nBytes = read(doormanDesc, (char *)&p, sizeof(p));
                if (nBytes <= 0) {
                    doorman_lost();
                    break;
                } else {
                    char logMes[18 * 1024];
                    if (p.nExtra > PACKET_MAX_PAYLOAD_SIZE) {
                        bug("Doorman sent a too big packet! %d > %d: dropping", p.nExtra, PACKET_MAX_PAYLOAD_SIZE);
                        return;
                    }
                    if (p.nExtra) {
                        ssize_t payload_read = read(doormanDesc, buffer, p.nExtra);
                        if (payload_read <= 0) {
                            doorman_lost();
                            break;
                        }
                        nBytes += payload_read;
                    }
                    if ((size_t)nBytes != sizeof(Packet) + p.nExtra) {
                        doorman_lost();
                        break;
                    }
                    switch (p.type) {
                    case PACKET_CONNECT:
                        snprintf(logMes, sizeof(logMes), "Incoming connection on channel %d.", p.channel);
                        log_string(logMes);
                        new_descriptor(p.channel);
                        break;
                    case PACKET_RECONNECT: {
                        Descriptor *d;
                        snprintf(logMes, sizeof(logMes), "Incoming reconnection on channel %d for %s.", p.channel,
                                 buffer);
                        log_string(logMes);
                        d = new_descriptor(p.channel);
                        // Login name
                        d->write(buffer);
                        d->write("\n\r");
                        nanny(d, buffer);
                        d->write("\n\r");

                        // Paranoid :
                        if (d->connected == DescriptorState::GetOldPassword) {
                            d->connected = DescriptorState::CircumventPassword;
                            // log in
                            nanny(d, "");
                            // accept ANSIness
                            d->write("\n\r");
                            nanny(d, "");
                        }
                    } break;
                    case PACKET_DISCONNECT:
                        /*
                         * Find the descriptor associated with the channel and close it
                         */
                        for (d = descriptor_list; d; d = d->next) {
                            if (d->channel() == p.channel) {
                                if (d->character && d->character->level > 1)
                                    save_char_obj(d->character);
                                d->clear_output_buffer();
                                if (d->is_playing())
                                    d->connected = DescriptorState::Disconnecting;
                                else
                                    d->connected = DescriptorState::DisconnectingNp;
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
                            if (d->channel() == p.channel) {
                                ok = 1;
                                auto *data = reinterpret_cast<const InfoData *>(buffer);
                                d->set_endpoint(data->netaddr, data->port, data->data);
                                break;
                            }
                        }
                        if (!ok) {
                            snprintf(logMes, sizeof(logMes), "Unable to associate info with a descriptor (%d)",
                                     p.channel);
                        } else {
                            snprintf(logMes, sizeof(logMes), "Info from doorman: %d is %s", p.channel,
                                     d->host().c_str());
                        }
                        log_string(logMes);
                        break;
                    case PACKET_MESSAGE:
                        /*
                         * Find the descriptor associated with the channel
                         */
                        ok = 0;
                        for (d = descriptor_list; d; d = d->next) {
                            if (d->channel() == p.channel) {
                                ok = 1;
                                if (d->character != nullptr)
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
        std::unordered_set<Descriptor *> handled_input;
        for (d = descriptor_list; d; d = dNext) {
            dNext = d->next;
            d->processing_command(false);
            if (d->character && d->character->wait > 0)
                --d->character->wait;
            /* Waitstate the character */
            if (d->character && d->character->wait)
                continue;

            if (auto incomm = d->pop_incomm()) {
                d->processing_command(true);
                move_active_char_from_limbo(d->character);

                // It's possible that 'd' will be deleted as a result of any of the following operations. Be very
                // careful.
                if (d->is_paging())
                    d->show_next_page(*incomm);
                else if (d->is_playing())
                    interpret(d->character, incomm->c_str());
                else
                    nanny(d, incomm->c_str());
                // 'd' may be invalid, do no further work on it.
            }
        }

        /*
         * Autonomous game motion.
         */
        update_handler();

        /*
         * Output.
         */
        if (doormanDesc) {
            Descriptor *d;
            for (d = descriptor_list; d != nullptr; d = d_next) {
                d_next = d->next;

                if (d->processing_command() || d->has_buffered_output()) {
                    if (!process_output(d, true)) {
                        if (d->character != nullptr && d->character->level > 1)
                            save_char_obj(d->character);
                        d->clear_output_buffer();
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

            gettimeofday(&now_time, nullptr);
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
                if (select(0, nullptr, nullptr, nullptr, &stall_time) < 0) {
                    perror("Game_loop: select: stall");
                    exit(1);
                }
            }
        }

        gettimeofday(&last_time, nullptr);
        current_time = (time_t)last_time.tv_sec;
    }
}

Descriptor *new_descriptor(int channel) {
    auto *dnew = new Descriptor(channel);

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
            dnew->write(help_greeting + 1);
        else
            dnew->write(help_greeting);
    }

    return dnew;
}

void close_socket(Descriptor *dclose) {
    CHAR_DATA *ch;
    Packet p;

    if (dclose->has_buffered_output())
        process_output(dclose, false);

    dclose->close();

    if ((ch = dclose->character) != nullptr) {
        do_chal_canc(ch);
        snprintf(log_buf, LOG_BUF_SIZE, "Closing link to %s.", ch->name);
        log_new(log_buf, EXTRA_WIZNET_DEBUG,
                (IS_SET(ch->act, PLR_WIZINVIS) || IS_SET(ch->act, PLR_PROWL)) ? get_trust(ch) : 0);
        if (dclose->is_playing() || dclose->connected == DescriptorState::Disconnecting) {
            act("$n has lost $s link.", ch, nullptr, nullptr, TO_ROOM);
            ch->desc = nullptr;
        } else {
            free_char(dclose->original ? dclose->original : dclose->character);
        }
    } else {
        /*
         * New code: debug connections that haven't logged in properly
         * ...at least for level 100s
         */
        snprintf(log_buf, LOG_BUF_SIZE, "Closing link to channel %d.", dclose->channel());
        log_new(log_buf, EXTRA_WIZNET_DEBUG, 100);
    }

    if (d_next == dclose)
        d_next = d_next->next;

    if (dclose == descriptor_list) {
        descriptor_list = descriptor_list->next;
    } else {
        Descriptor *d;

        for (d = descriptor_list; d && d->next != dclose; d = d->next)
            ;
        if (d != nullptr)
            d->next = dclose->next;
        else
            bug("Close_socket: dclose not found.");
    }

    // If doorman didn't tell us to disconnect them, then tell doorman to kill the connection, else ack the disconnect.
    if (dclose->connected != DescriptorState::Disconnecting && dclose->connected != DescriptorState::DisconnectingNp) {
        p.type = PACKET_DISCONNECT;
    } else {
        p.type = PACKET_DISCONNECT_ACK;
    }
    p.channel = dclose->channel();
    p.nExtra = 0;
    SendPacket(&p, nullptr);

    // TODO: ideally no naked news/deletes. One day.
    delete dclose;
}

bool read_from_descriptor(Descriptor *d, char *data, int nRead) {
    if (d->is_input_full()) {
        snprintf(log_buf, LOG_BUF_SIZE, "%s input overflow!", d->host().c_str());
        log_string(log_buf);
        d->write_direct("\n\r*** PUT A LID ON IT!!! ***\n\r");
        d->clear_input();
        d->add_command("quit");
        return false;
    }

    d->add_command(std::string_view(data, nRead));
    return true;
}

/*
 * Low level output function.
 */
bool process_output(Descriptor *d, bool fPrompt) {
    extern bool merc_down;

    /*
     * Bust a prompt.
     */
    if (!merc_down && d->is_paging())
        d->write("[Hit Return to continue]\n\r");
    else if (fPrompt && !merc_down && d->is_playing()) {
        CHAR_DATA *ch;
        CHAR_DATA *victim;

        ch = d->character;

        /* battle prompt */
        if ((victim = ch->fighting) != nullptr) {
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
            d->write(buf);
        }

        ch = d->original ? d->original : d->character;
        if (!IS_SET(ch->comm, COMM_COMPACT))
            d->write("\n\r");

        if (IS_SET(ch->comm, COMM_PROMPT)) {
            /* get the prompt for the character in question */

            show_prompt(d, ch->pcdata->prompt);
        }

        if (IS_SET(ch->comm, COMM_TELNET_GA))
            d->write(go_ahead_str);
    }

    return d->flush_output();
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny(Descriptor *d, const char *argument) {
    Descriptor *d_old, *d_next;
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
    Descriptor *de;

    while (isspace(*argument))
        argument++;

    ch = d->character;

    switch (d->connected) {

    default:
        bug("Nanny: bad d->connected %d.", static_cast<int>(d->connected));
        close_socket(d);
        return;

    case DescriptorState::GetName: {
        if (argument[0] == '\0') {
            close_socket(d);
            return;
        }
        // Take a copy of the character's proposed name, so we can forcibly upper-case its first letter.
        std::string char_name(argument);
        char_name[0] = toupper(char_name[0]);
        if (!check_parse_name(char_name.c_str())) {
            d->write("Illegal name, try another.\n\rName: ");
            return;
        }

        /* TM's attempt number 2 to prevent object cloning under the
              suspicion that the other wy was causing a hang on lagged entry */
        /*      for ( ch = char_list ; ch ; ch=ch->next ) {
                 if ( !IS_NPC(ch) &&
                 (!str_cmp(ch->name,argument)) &&
                 (ch->desc != nullptr) ) {
                    write_to_buffer( d, "Illegal name, try another.\n\rName: ", 0 );
                    return;
                 }
                 } */

        fOld = load_char_obj(d, char_name.c_str());
        ch = d->character;

        if (IS_SET(ch->act, PLR_DENY)) {
            snprintf(log_buf, LOG_BUF_SIZE, "Denying access to %s@%s.", char_name.c_str(), d->host().c_str());
            log_string(log_buf);
            d->write("You are denied access.\n\r");
            close_socket(d);
            return;
        }

        if (check_reconnect(d, false)) {
            fOld = true;
        } else {
            if (wizlock && !IS_IMMORTAL(ch)) {
                d->write("The game is wizlocked.  Try again later - a reboot may be imminent.\n\r");
                close_socket(d);
                return;
            }
        }

        if (fOld) {
            /* Old player */
            d->write("Password: ");
            SetEchoState(d, 0);
            d->connected = DescriptorState::GetOldPassword;
            return;
        } else {
            /* New player */
            if (newlock) {
                d->write("The game is newlocked.  Try again later - a reboot may be imminent.\n\r");
                close_socket(d);
                return;
            }

            // Check for a newban on player's site. This is the one time we use the full host name.
            if (check_ban(d->raw_full_hostname().c_str(), BAN_NEWBIES)
                || check_ban(d->raw_full_hostname().c_str(), BAN_PERMIT)) {
                d->write("Your site has been banned.  Only existing players from your site may connect.\n\r");
                close_socket(d);
                return;
            }

            snprintf(buf, sizeof(buf), "Did I hear that right -  '%s' (Y/N)? ", char_name.c_str());
            d->write(buf);
            d->connected = DescriptorState::ConfirmNewName;
            return;
        }
    } break;

    case DescriptorState::GetOldPassword:
        d->write("\n\r");

        // TODO crypt can return null if if fails (e.g. password is truncated).
        // for now we just pwd[0], which lets us reset passwords.
        if (ch->pcdata->pwd[0] && strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd)) {
            d->write("Our survey said <Crude buzzer noise>.\n\rWrong password.\n\r");
            close_socket(d);
            return;
        }
        // falls through
    case DescriptorState::CircumventPassword:
        if (ch->pcdata->pwd[0] == 0) {
            d->write("Oopsie! Null password!\n\r");
            d->write("Unless some IMM has been fiddling, then this is a bug!\n\r");
            d->write("Type 'password null <new password>' to fix.\n\r");
        }

        SetEchoState(d, 1);

        // This is the one time we use the full host name.
        if (check_ban(d->raw_full_hostname().c_str(), BAN_PERMIT) && (!is_set_extra(ch, EXTRA_PERMIT))) {
            d->write("Your site has been banned.  Sorry.\n\r");
            close_socket(d);
            return;
        }

        if (check_playing(d, ch->name))
            return;

        if (check_reconnect(d, true))
            return;

        snprintf(log_buf, LOG_BUF_SIZE, "%s@%s has connected.", ch->name, d->host().c_str());
        log_new(log_buf, EXTRA_WIZNET_DEBUG,
                ((IS_SET(ch->act, PLR_WIZINVIS) || IS_SET(ch->act, PLR_PROWL))) ? get_trust(ch) : 0);

        d->write("Does your terminal support ANSI colour (Y/N/Return = as saved)?");
        d->connected = (d->connected == DescriptorState::CircumventPassword) ? DescriptorState::ReadMotd
                                                                             : DescriptorState::GetAnsi;

        break;

        /* RT code for breaking link */

    case DescriptorState::BreakConnect:
        switch (*argument) {
        case 'y':
        case 'Y':
            for (d_old = descriptor_list; d_old != nullptr; d_old = d_next) {
                d_next = d_old->next;
                if (d_old == d || d_old->character == nullptr)
                    continue;

                if (str_cmp(ch->name, d_old->character->name))
                    continue;

                close_socket(d_old);
            }
            if (check_reconnect(d, true))
                return;
            d->write("Reconnect attempt failed.\n\rName: ");
            if (d->character != nullptr) {
                free_char(d->character);
                d->character = nullptr;
            }
            d->connected = DescriptorState::GetName;
            break;

        case 'n':
        case 'N':
            d->write("Name: ");
            if (d->character != nullptr) {
                free_char(d->character);
                d->character = nullptr;
            }
            d->connected = DescriptorState::GetName;
            break;

        default: d->write("Please type Y or N? "); break;
        }
        break;

    case DescriptorState::ConfirmNewName:
        switch (*argument) {
        case 'y':
        case 'Y':
            snprintf(buf, sizeof(buf),
                     "Welcome new character, to Xania.\n\rThink of a password for %s: ", (char *)ch->name);
            SetEchoState(d, 0);
            d->write(buf);
            d->connected = DescriptorState::GetNewPassword;
            break;

        case 'n':
        case 'N':
            d->write("Ack! Amateurs! Try typing it in properly: ");
            free_char(d->character);
            d->character = nullptr;
            d->connected = DescriptorState::GetName;
            break;

        default: d->write("It's quite simple - type Yes or No: "); break;
        }
        break;

    case DescriptorState::GetAnsi:
        if (argument[0] == '\0') {
            if (ch->pcdata->colour) {
                send_to_char("This is a |RC|GO|BL|rO|gU|bR|cF|YU|PL |RM|GU|BD|W!\n\r", ch);
            }
            if (IS_HERO(ch)) {
                do_help(ch, "imotd");
                d->connected = DescriptorState::ReadIMotd;
            } else {
                do_help(ch, "motd");
                d->connected = DescriptorState::ReadMotd;
            }
        } else {
            switch (*argument) {
            case 'y':
            case 'Y':
                ch->pcdata->colour = 1;
                send_to_char("This is a |RC|GO|BL|rO|gU|bR|cF|YU|PL |RM|GU|BD|W!\n\r", ch);
                if (IS_HERO(ch)) {
                    do_help(ch, "imotd");
                    d->connected = DescriptorState::ReadIMotd;
                } else {
                    do_help(ch, "motd");
                    d->connected = DescriptorState::ReadMotd;
                }
                break;

            case 'n':
            case 'N':
                ch->pcdata->colour = 0;
                if (IS_HERO(ch)) {
                    do_help(ch, "imotd");
                    d->connected = DescriptorState::ReadIMotd;
                } else {
                    do_help(ch, "motd");
                    d->connected = DescriptorState::ReadMotd;
                }
                break;

            default: d->write("Please type Yes or No press return: "); break;
            }
        }
        break;

    case DescriptorState::GetNewPassword:
        d->write("\n\r");

        if (strlen(argument) < 5) {
            d->write("Password must be at least five characters long.\n\rPassword: ");
            return;
        }

        pwdnew = crypt(argument, ch->name);
        for (p = pwdnew; *p != '\0'; p++) {
            if (*p == '~') {
                d->write("New password not acceptable, try again.\n\rPassword: ");
                return;
            }
        }

        free_string(ch->pcdata->pwd);
        ch->pcdata->pwd = str_dup(pwdnew);
        d->write("Please retype password: ");
        d->connected = DescriptorState::ConfirmNewPassword;
        break;

    case DescriptorState::ConfirmNewPassword:
        d->write("\n\r");

        if (strcmp(crypt(argument, ch->pcdata->pwd), ch->pcdata->pwd)) {
            d->write("You could try typing the same thing in twice...\n\rRetype password: ");
            d->connected = DescriptorState::GetNewPassword;
            return;
        }

        SetEchoState(d, 1);
        d->write("The following races are available:\n\r  ");
        for (race = 1; race_table[race].name != nullptr; race++) {
            if (!race_table[race].pc_race)
                break;
            d->write(race_table[race].name);
            d->write(" ");
        }
        d->write("\n\r");
        d->write("What is your race (help for more information)? ");
        d->connected = DescriptorState::GetNewRace;
        break;

    case DescriptorState::GetNewRace:
        one_argument(argument, arg);

        if (!strcmp(arg, "help")) {
            argument = one_argument(argument, arg);
            if (argument[0] == '\0')
                do_help(ch, "race help");
            else
                do_help(ch, argument);
            d->write("What is your race (help for more information)? ");
            break;
        }

        race = race_lookup(argument);

        if (race == 0 || !race_table[race].pc_race) {
            d->write("That is not a valid race.\n\r");
            d->write("The following races are available:\n\r  ");
            for (race = 1; race_table[race].name != nullptr; race++) {
                if (!race_table[race].pc_race)
                    break;
                d->write(race_table[race].name);
                d->write(" ");
            }
            d->write("\n\r");
            d->write("What is your race? (help for more information) ");
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
            if (pc_race_table[race].skills[i] == nullptr)
                break;
            group_add(ch, pc_race_table[race].skills[i], false);
        }
        /* add cost */
        ch->pcdata->points = pc_race_table[race].points;
        ch->size = pc_race_table[race].size;

        d->write("What is your sex (M/F)? ");
        d->connected = DescriptorState::GetNewSex;
        break;

    case DescriptorState::GetNewSex:
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
        default: d->write("That's not a sex.\n\rWhat IS your sex? "); return;
        }

        strcpy(buf, "The following classes are available: ");
        for (iClass = 0; iClass < MAX_CLASS; iClass++) {
            if (iClass > 0)
                strcat(buf, " ");
            strcat(buf, class_table[iClass].name);
        }
        strcat(buf, "\n\r");
        d->write(buf);
        d->write("What is your class (help for more information)? ");
        d->connected = DescriptorState::GetNewClass;
        break;

    case DescriptorState::GetNewClass:
        one_argument(argument, arg);
        if (!strcmp(arg, "help")) {
            argument = one_argument(argument, arg);
            if (argument[0] == '\0')
                do_help(ch, "classes");
            else
                do_help(ch, argument);
            d->write("What is your class (help for more information)? ");
            break;
        }
        iClass = class_lookup(argument);
        if (iClass == -1) {
            d->write("That's not a class.\n\rWhat IS your class? ");
            return;
        }
        ch->class_num = iClass;
        snprintf(log_buf, LOG_BUF_SIZE, "%s@%s new player.", ch->name, d->host().c_str());
        log_string(log_buf);
        d->write("\n\r");
        d->write("You may be good, neutral, or evil.\n\r");
        d->write("Which alignment (G/N/E)? ");
        d->connected = DescriptorState::GetAlignment;
        break;

    case DescriptorState::GetAlignment:
        switch (argument[0]) {
        case 'g':
        case 'G': ch->alignment = 750; break;
        case 'n':
        case 'N': ch->alignment = 0; break;
        case 'e':
        case 'E': ch->alignment = -750; break;
        default:
            d->write("That's not a valid alignment.\n\r");
            d->write("Which alignment (G/N/E)? ");
            return;
        }

        d->write("\n\r");

        group_add(ch, "rom basics", false);
        group_add(ch, class_table[ch->class_num].base_group, false);
        ch->pcdata->learned[gsn_recall] = 50;
        d->write("Do you wish to customize this character?\n\r");
        d->write("Customization takes time, but allows a wider range of skills and abilities.\n\r");
        d->write("Customize (Y/N)? ");
        d->connected = DescriptorState::DefaultChoice;
        break;

    case DescriptorState::DefaultChoice:
        d->write("\n\r");
        switch (argument[0]) {
        case 'y':
        case 'Y':
            ch->gen_data = (GEN_DATA *)alloc_perm(sizeof(*ch->gen_data));
            ch->gen_data->points_chosen = ch->pcdata->points;
            do_help(ch, "group header");
            list_group_costs(ch);
            d->write("You already have the following skills:\n\r");
            do_skills(ch, "");
            do_help(ch, "menu choice");
            d->connected = DescriptorState::GenGroups;
            break;
        case 'n':
        case 'N':
            group_add(ch, class_table[ch->class_num].default_group, true);
            d->write("\n\r");
            d->write("Does your terminal support ANSI colour (Y/N/Return = as saved)?");
            d->connected = DescriptorState::GetAnsi;

            break;
        default: d->write("Please answer (Y/N)? "); return;
        }
        break;

    case DescriptorState::GenGroups:
        send_to_char("\n\r", ch);
        if (!str_cmp(argument, "done")) {
            snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->pcdata->points);
            send_to_char(buf, ch);
            snprintf(buf, sizeof(buf), "Experience per level: %d\n\r", exp_per_level(ch, ch->gen_data->points_chosen));
            if (ch->pcdata->points < 40)
                ch->train = (40 - ch->pcdata->points + 1) / 2;
            send_to_char(buf, ch);
            d->write("\n\r");
            d->write("Does your terminal support ANSI colour (Y/N/Return = as saved)?");
            d->connected = DescriptorState::GetAnsi;

            break;
        }

        if (!parse_gen_groups(ch, argument))
            send_to_char("Choices are: list,learned,premise,add,drop,info,help, and done.\n\r", ch);

        do_help(ch, "menu choice");
        break;

    case DescriptorState::ReadIMotd:
        d->write("\n\r");
        do_help(ch, "motd");
        d->connected = DescriptorState::ReadMotd;
        break;

    case DescriptorState::ReadMotd:
        d->write("\n\rWelcome to Xania.  May your stay be eventful.\n\r");
        ch->next = char_list;
        char_list = ch;
        d->connected = DescriptorState::Playing;
        reset_char(ch);

        /* Moog: tell doorman we logged in OK */
        {
            Packet p;
            p.type = PACKET_AUTHORIZED;
            p.channel = d->channel();
            p.nExtra = strlen(ch->name) + 1;
            SendPacket(&p, ch->name);
        }

        if (ch->level == 0) {

            if (ch->race != race_lookup("dragon"))
                ch->perm_stat[class_table[ch->class_num].attr_prime] += 3;

            ch->level = 1;
            ch->exp = exp_per_level(ch, ch->pcdata->points);
            ch->hit = ch->max_hit;
            ch->mana = ch->max_mana;
            ch->move = ch->max_move;
            ch->train = 3;
            ch->practice = 5;
            snprintf(buf, sizeof(buf), "the %s", title_table[ch->class_num][ch->level][ch->sex == SEX_FEMALE ? 1 : 0]);
            set_title(ch, buf);

            do_outfit(ch, "");
            obj_to_char(create_object(get_obj_index(OBJ_VNUM_MAP), 0), ch);

            ch->pcdata->learned[get_weapon_sn(ch)] = 40;

            char_to_room(ch, get_room_index(ROOM_VNUM_SCHOOL));
            send_to_char("\n\r", ch);
            do_help(ch, "NEWBIE INFO");
            send_to_char("\n\r", ch);

            /* Rohan: New player logged in, need to add name to player list */
            temp_known_player = (KNOWN_PLAYERS *)alloc_mem(sizeof(KNOWN_PLAYERS));
            temp_known_player->name = str_dup(ch->name);
            temp_known_player->next = player_list;
            player_list = temp_known_player;
            /* hack to let the newbie know about the tipwizard */
            send_to_char("|WTip: this is Xania's tip wizard! Type 'tips' to turn this on or off.|w\n\r", ch);
            /* turn on the newbie's tips */
            set_extra(ch, EXTRA_TIP_WIZARD);

        } else if (ch->in_room != nullptr) {
            char_to_room(ch, ch->in_room);
        } else if (IS_IMMORTAL(ch)) {
            char_to_room(ch, get_room_index(ROOM_VNUM_CHAT));
        } else {
            char_to_room(ch, get_room_index(ROOM_VNUM_TEMPLE));
        }

        snprintf(log_buf, LOG_BUF_SIZE, "|W### |P%s|W has entered the game.|w", ch->name);
        announce(log_buf, ch);
        act("|P$n|W has entered the game.", ch, nullptr, nullptr, TO_ROOM);
        do_look(ch, "auto");

        /* Rohan: code to increase the player count if needed - it was only
           updated if a player did count */
        count = 0;
        for (de = descriptor_list; de != nullptr; de = de->next)
            if (de->is_playing())
                count++;
        max_on = UMAX(count, max_on);

        if (ch->gold > 250000 && !IS_IMMORTAL(ch)) {
            snprintf(buf, sizeof(buf), "You are taxed %ld gold to pay for the Mayor's bar.\n\r",
                     (ch->gold - 250000) / 2);
            send_to_char(buf, ch);
            ch->gold -= (ch->gold - 250000) / 2;
        }

        if (ch->pet != nullptr) {
            char_to_room(ch->pet, ch->in_room);
            act("|P$n|W has entered the game.", ch->pet, nullptr, nullptr, TO_ROOM);
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
}

/*
 * Parse a name for acceptability.
 */
bool check_parse_name(const char *name) {
    /*
     * Reserved words.
     */
    if (is_name(name, "all auto immortal self someone something the you"))
        return false;

    if (!str_cmp(capitalize(name), "DEATH"))
        return true;

    /*    if (str_cmp(capitalize(name),"DEATH") && (!str_prefix("death",name)
        || !str_suffix("Death",name)))
       return false;*/

    /*
     * Length restrictions.
     */

    if (strlen(name) < 2)
        return false;

    if (strlen(name) > 12)
        return false;

    /*
     * Alphanumerics only.
     * Lock out IllIll twits.
     */
    {
        bool fIll;

        fIll = true;
        for (const char *pc = name; *pc != '\0'; pc++) {
            if (!isalpha(*pc))
                return false;
            if (tolower(*pc) != 'i' && tolower(*pc) != 'l')
                fIll = false;
        }

        if (fIll)
            return false;
    }

    /*
     * Prevent players from naming themselves after mobs.
     */
    {
        extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
        MOB_INDEX_DATA *pMobIndex;
        int iHash;

        for (iHash = 0; iHash < MAX_KEY_HASH; iHash++) {
            for (pMobIndex = mob_index_hash[iHash]; pMobIndex != nullptr; pMobIndex = pMobIndex->next) {
                if (is_name(name, pMobIndex->player_name))
                    return false;
            }
        }
    }

    return true;
}

/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect(Descriptor *d, bool fConn) {
    CHAR_DATA *ch;

    for (ch = char_list; ch != nullptr; ch = ch->next) {
        if (!IS_NPC(ch) && (!fConn || ch->desc == nullptr) && !str_cmp(d->character->name, ch->name)) {
            if (fConn == false) {
                free_string(d->character->pcdata->pwd);
                d->character->pcdata->pwd = str_dup(ch->pcdata->pwd);
            } else {
                free_char(d->character);
                d->character = ch;
                ch->desc = d;
                ch->timer = 0;
                send_to_char("Reconnecting.\n\r", ch);
                act("$n has reconnected.", ch, nullptr, nullptr, TO_ROOM);
                snprintf(log_buf, LOG_BUF_SIZE, "%s@%s reconnected.", ch->name, d->host().c_str());
                log_new(log_buf, EXTRA_WIZNET_DEBUG,
                        ((IS_SET(ch->act, PLR_WIZINVIS) || IS_SET(ch->act, PLR_PROWL))) ? get_trust(ch) : 0);
                d->connected = DescriptorState::Playing;
            }
            return true;
        }
    }

    return false;
}

/*
 * Check if already playing.
 */
bool check_playing(Descriptor *d, char *name) {
    Descriptor *dold;

    for (dold = descriptor_list; dold; dold = dold->next) {
        if (dold != d && dold->character != nullptr && dold->connected != DescriptorState::GetName
            && dold->connected != DescriptorState::GetOldPassword
            && !str_cmp(name, dold->original ? dold->original->name : dold->character->name)) {
            d->write("That character is already playing.\n\r");
            d->write("Do you wish to connect anyway (Y/N)?");
            d->connected = DescriptorState::BreakConnect;
            return true;
        }
    }

    return false;
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

    if (txt != nullptr && ch->desc != nullptr) {
        if (!IS_NPC(ch) && (ch->pcdata->colour))
            colour = 1;
        if (IS_NPC(ch) && (ch->desc->original != nullptr) /* paranoia factor */
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
        ch->desc->write(buf);
    }
}

/*
 * Send a page to one char.
 */
void page_to_char(const char *txt, CHAR_DATA *ch) {
    if (txt == nullptr || ch->desc == nullptr)
        return;

    ch->desc->page_to(txt);
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
    static const char *const he_she[] = {"it", "he", "she"};
    static const char *const him_her[] = {"it", "him", "her"};
    static const char *const his_her[] = {"its", "his", "her"};

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
    if (format == nullptr || format[0] == '\0')
        return;

    /* discard null rooms and chars */
    if (ch == nullptr || ch->in_room == nullptr)
        return;
    if (type == TO_GIVENROOM && givenRoom == nullptr) {
        bug("Act: null givenRoom with TO_GIVENROOM.");
        return;
    }

    if (type == TO_GIVENROOM)
        to = givenRoom->people;
    else
        to = ch->in_room->people;

    if (type == TO_VICT) {
        if (vch == nullptr) {
            bug("Act: null vch with TO_VICT.");
            return;
        }

        if (vch->in_room == nullptr)
            return;

        to = vch->in_room->people;
    }

    for (; to != nullptr; to = to->next_in_room) {
        if (to->desc == nullptr || to->position < min_pos)
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

            if (arg2 == nullptr && *str >= 'A' && *str <= 'Z') {
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
                    if (arg2 == nullptr || ((char *)arg2)[0] == '\0') {
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

        if ((type == TO_ROOM && to == ch) || (type == TO_NOTVICT && (to == ch || to == vch))) {
            /* Ignore them */
        } else {
            send_to_char(buf, to);
            /* Merc-2.2 MOBProgs - Faramir 31/8/1998 */
            if (MOBtrigger)
                mprog_act_trigger(buf, to, ch, obj1, vch);
        }
    }
    MOBtrigger = true;

    if (ch->in_room != nullptr) {
        if ((type == TO_ROOM || type == TO_NOTVICT) && ch->in_room->vnum == CHAL_ROOM) {
            Descriptor *d;

            for (d = descriptor_list; d != nullptr; d = d->next) {
                if (d->character != nullptr && d->character->in_room != nullptr && d->character->in_room->vnum == 1222
                    && IS_AWAKE(d->character))
                    send_to_char(buf, d->character);
            }
        }
    }
}

void show_prompt(Descriptor *d, char *prompt) {
    char buf[256]; /* this is actually sent to the ch */
    char buf2[64];
    CHAR_DATA *ch;
    CHAR_DATA *ch_prefix = nullptr; /* Needed for prefix in prompt with switched MOB */
    char *point;
    char *str;
    const char *i;
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

    if (prompt == nullptr || prompt[0] == '\0') {
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
                i = ch->in_room->area->areaname; // print the area name short form
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

                strftime(buf2, 63, "%H:%M:%S", ch_time);
            } else
                strftime(buf2, 63, "%H:%M:%S", localtime(&ch_timet));
            i = buf2;
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
}
