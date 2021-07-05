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
#include "AREA_DATA.hpp"
#include "Ban.hpp"
#include "BitsPlayerAct.hpp"
#include "Char.hpp"
#include "CharGeneration.hpp"
#include "Classes.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "Help.hpp"
#include "MobIndexData.hpp"
#include "Note.hpp"
#include "Object.hpp"
#include "Pronouns.hpp"
#include "Races.hpp"
#include "SkillNumbers.hpp"
#include "TimeInfoData.hpp"
#include "Titles.hpp"
#include "VnumObjects.hpp"
#include "VnumRooms.hpp"
#include "act_comm.hpp"
#include "challenge.hpp"
#include "common/Fd.hpp"
#include "common/doorman_protocol.h"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"
#include "merc.h"
#include "mob_prog.hpp"
#include "save.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "update.hpp"

#include <range/v3/algorithm/any_of.hpp>

#include <arpa/telnet.h>
#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fmt/format.h>
#include <fstream>
#include <netdb.h>
#include <netinet/in.h>
#include <streambuf>
#include <string>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/un.h>
#include <thread>
#include <unistd.h>

using namespace std::literals;

extern size_t max_on;

/*
 * Global variables.
 */
DescriptorList the_descriptors;
DescriptorList &descriptors() { return the_descriptors; }
bool god; /* All new chars are gods! */
bool merc_down; /* Shutdown       */
bool wizlock; /* Game is wizlocked    */
bool newlock; /* Game is newlocked    */
bool MOBtrigger;

bool read_from_descriptor(Descriptor *d, std::string_view data);
void move_active_char_from_limbo(Char *ch);

/*
 * Other local functions (OS-independent).
 */
bool check_parse_name(const char *name);
bool check_reconnect(Descriptor *d, bool fConn);
bool check_playing(Descriptor *d, std::string_view name);
void nanny(Descriptor *d, const char *argument);
bool process_output(Descriptor *d, bool fPrompt);

/* Handle to get to doorman */
Fd doormanDesc;

/* Send a packet to doorman */
bool send_to_doorman(const Packet *p, const void *extra) {
    // TODO: do something rather than return here if there's a failure
    if (!doormanDesc.is_open())
        return false;
    if (p->nExtra > PACKET_MAX_PAYLOAD_SIZE) {
        bug("MUD tried to send a doorman packet with payload size {} > {}! Dropping!", p->nExtra,
            PACKET_MAX_PAYLOAD_SIZE);
        return false;
    }
    try {
        if (p->nExtra)
            doormanDesc.write_many(*p, gsl::span<const byte>(static_cast<const byte *>(extra), p->nExtra));
        else
            doormanDesc.write(*p);
    } catch (const std::runtime_error &re) {
        bug("Unable to write to doorman: {}", re.what());
        return false;
    }
    return true;
}

void SetEchoState(Descriptor *d, int on) {
    Packet p;
    p.type = on ? PACKET_ECHO_ON : PACKET_ECHO_OFF;
    p.channel = d->channel();
    p.nExtra = 0;
    send_to_doorman(&p, nullptr);
}

void greet(Descriptor &d) {
    const auto *greeting = HelpList::singleton().lookup(0, "greeting");
    if (!greeting) {
        bug("Unable to look up greeting");
        return;
    }
    d.write(greeting->text());
}

namespace {
bool is_being_debugged() {
    std::ifstream t("/proc/self/status");
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    for (auto line : line_iter(str)) {
        const auto match = "TracerPid:"sv;
        if (matches_start(match, line))
            return parse_number(ltrim(line.substr(match.size()))) != 0;
    }

    return false;
}
}

/* where we're asked nicely to quit from the outside (mudmgr or OS) */
void handle_signal_shutdown() {
    log_string("Signal shutdown received");

    /* ask everyone to save! */
    for (auto *vch : char_list) {
        // vch->d->c check added by TM to avoid crashes when
        // someone hasn't logged in but the mud is shut down
        if (vch->is_pc() && vch->desc && vch->desc->is_playing()) {
            MOBtrigger = false;
            do_save(vch);
            vch->send_line("|RXania has been asked to shutdown by the operating system.|w");
            if (vch->desc && vch->desc->has_buffered_output())
                process_output(vch->desc, false);
        }
    }
    merc_down = true;
    for (auto &d : descriptors().all())
        d.close();
}

Fd init_socket(const char *pipe_file) {
    unlink(pipe_file);

    auto fd = Fd::socket(PF_UNIX, SOCK_STREAM, 0);
    sockaddr_un xaniaAddr{};
    strncpy(xaniaAddr.sun_path, pipe_file, sizeof(xaniaAddr.sun_path) - 1);
    xaniaAddr.sun_family = PF_UNIX;
    int enabled = 1;
    fd.setsockopt(SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)).bind(xaniaAddr).listen(3);

    log_string("Waiting for doorman...");
    try {
        sockaddr_un dont_care{};
        socklen_t dont_care_len{sizeof(sockaddr_in)};
        doormanDesc = fd.accept(reinterpret_cast<sockaddr *>(&dont_care), &dont_care_len);
        log_string("Doorman has connected - proceeding with boot-up");
    } catch (const std::system_error &e) {
        log_string("Unable to accept doorman...booting anyway");
    }

    return fd;
}

void doorman_lost() {
    log_string("Lost connection to doorman");
    doormanDesc.close();
    /* Now to go through and disconnect all the characters */
    for (auto &d : descriptors().all())
        d.close();
}

void handle_doorman_packet(const Packet &p, std::string_view buffer) {
    switch (p.type) {
    case PACKET_CONNECT:
        log_string("Incoming connection on channel {}.", p.channel);
        if (auto *d = descriptors().create(p.channel)) {
            greet(*d);
        } else {
            log_string("Duplicate channel {} on connect!", p.channel);
        }
        break;
    case PACKET_RECONNECT:
        log_string("Incoming reconnection on channel {} for {}.", p.channel, buffer);
        if (auto *d = descriptors().create(p.channel)) {
            greet(*d);
            // Login name
            d->write(fmt::format("{}\n\r", buffer));
            std::string nannyable(buffer);
            nanny(d, nannyable.c_str()); // TODO one day string_viewify
            d->write("\n\r");

            // Paranoid :
            if (d->state() == DescriptorState::GetOldPassword) {
                d->state(DescriptorState::CircumventPassword);
                // log in
                nanny(d, "");
                // accept ANSIness
                d->write("\n\r");
                nanny(d, "");
            }
        } else {
            log_string("Duplicate channel {} on reconnect!", p.channel);
        }
        break;
    case PACKET_DISCONNECT:
        if (auto *d = descriptors().find_by_channel(p.channel)) {
            if (d->character() && d->character()->level > 1)
                save_char_obj(d->character());
            d->clear_output_buffer();
            if (d->is_playing())
                d->state(DescriptorState::Disconnecting);
            else
                d->state(DescriptorState::DisconnectingNp);
            d->close();
        } else {
            log_string("Unable to find channel to close ({})", p.channel);
        }
        break;
    case PACKET_INFO:
        if (auto *d = descriptors().find_by_channel(p.channel)) {
            auto *data = reinterpret_cast<const InfoData *>(buffer.data());
            d->set_endpoint(data->netaddr, data->port, data->data);
            log_string("Info from doorman: {} is {}", p.channel, d->host());
        } else {
            log_string("Unable to associate info with a descriptor ({})", p.channel);
        }
        break;

    case PACKET_MESSAGE:
        if (auto *d = descriptors().find_by_channel(p.channel)) {
            if (d->character() != nullptr)
                d->character()->timer = 0;
            read_from_descriptor(d, buffer);
        } else {
            log_string("Unable to associate message with a descriptor ({})", p.channel);
        }
        break;
    default: break;
    }
}

void game_loop_unix(Fd control) {
    static timeval null_time;

    if (is_being_debugged())
        log_string("Running under a debugger");
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

    /* Main loop */
    while (!merc_down) {
        current_time = Clock::now();

        fd_set in_set;
        fd_set out_set;
        fd_set exc_set;
        int maxdesc;

        /*
         * Poll all active descriptors.
         */
        FD_ZERO(&in_set);
        FD_ZERO(&out_set);
        FD_ZERO(&exc_set);
        FD_SET(control.number(), &in_set);
        FD_SET(signal_fd, &in_set);
        if (doormanDesc.is_open()) {
            FD_SET(doormanDesc.number(), &in_set);
            maxdesc = std::max(control.number(), doormanDesc.number());
        } else
            maxdesc = control.number();
        maxdesc = std::max(maxdesc, signal_fd);

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
            if (info.ssi_signo == SIGINT) {
                if (is_being_debugged()) {
                    log_string("Caught SIGINT but detected a debugger, trapping instead");
                    raise(SIGTRAP);
                } else {
                    handle_signal_shutdown();
                    return;
                }
            } else if (info.ssi_signo == SIGTERM) {
                handle_signal_shutdown();
                return;
            } else {
                bug("Unexpected signal {}: ignoring", info.ssi_signo);
            }
        }

        /* If we don't have a connection from doorman, wait for one */
        if (!doormanDesc.is_open()) {
            log_string("Waiting for doorman...");

            try {
                sockaddr_un dont_care{};
                socklen_t dont_care_len{sizeof(sockaddr_in)};
                doormanDesc = control.accept(reinterpret_cast<sockaddr *>(&dont_care), &dont_care_len);
                log_string("Doorman has connected.");
                Packet pInit;
                pInit.nExtra = pInit.channel = 0;
                pInit.type = PACKET_INIT;
                send_to_doorman(&pInit, nullptr);
            } catch (const std::runtime_error &re) {
                bug("Unable to accept doorman connection: {}", re.what());
            }
        }

        /* Process doorman input, if any */
        if (doormanDesc.is_open() && FD_ISSET(doormanDesc.number(), &in_set)) {
            Packet p{};
            std::string payload;
            do {
                try {
                    p = doormanDesc.read_all<Packet>();
                    if (p.nExtra) {
                        if (p.nExtra > PACKET_MAX_PAYLOAD_SIZE) {
                            bug("Doorman sent a too big packet! {} > {}: dropping", p.nExtra, PACKET_MAX_PAYLOAD_SIZE);
                            doorman_lost();
                            break;
                        }
                        payload.resize(p.nExtra);
                        doormanDesc.read_all(gsl::span<char>(payload));
                    }
                } catch (const std::runtime_error &re) {
                    bug("Unable to read doorman packet: {}", re.what());
                    doorman_lost();
                    break;
                }
                handle_doorman_packet(p, payload);
                /* Reselect to see if there is more data */
                select(maxdesc + 1, &in_set, &out_set, &exc_set, &null_time);
            } while FD_ISSET(doormanDesc.number(), &in_set);
        }

        /*
         * De-waitstate characters and process pending input
         */
        for (auto &d : descriptors().all()) {
            d.processing_command(false);
            auto *character = d.character();
            if (character && character->wait > 0)
                --character->wait;
            /* Waitstate the character */
            if (character && character->wait)
                continue;

            if (auto incomm = d.pop_incomm()) {
                d.processing_command(true);
                move_active_char_from_limbo(character);

                // It's possible that 'd' will be deleted as a result of any of the following operations. Be very
                // careful.
                if (d.is_paging())
                    d.show_next_page(*incomm);
                else if (d.is_playing())
                    interpret(character, incomm->c_str());
                else
                    nanny(&d, incomm->c_str());
            } else if (d.is_lobby_timeout_exceeded()) {
                d.close();
            }
        }

        // Reap any sockets that got closed as a result of input processing or nannying.
        descriptors().reap_closed();

        /*
         * Autonomous game motion.
         */
        update_handler();

        /*
         * Output.
         */
        if (doormanDesc.is_open()) {
            for (auto &d : descriptors().all()) {
                if (d.processing_command() || d.has_buffered_output()) {
                    if (!process_output(&d, true)) {
                        if (d.character() != nullptr && d.character()->level > 1)
                            save_char_obj(d.character());
                        d.clear_output_buffer();
                        d.close();
                    }
                }
            }
            // Reap any sockets that got closed as a result of output handling.
            descriptors().reap_closed();
        }

        // Now we know all game code has run, reap any dead characters.
        reap_old_chars();

        // Synchronize to a clock.
        using namespace std::chrono;
        using namespace std::literals;
        constexpr auto nanos_per_pulse = duration_cast<nanoseconds>(1s) / PULSE_PER_SECOND;
        auto next_pulse = current_time + nanos_per_pulse;
        std::this_thread::sleep_until(next_pulse);
    }
}

bool read_from_descriptor(Descriptor *d, std::string_view data) {
    if (d->is_input_full()) {
        // Only log input for the first command exceeding the input limit
        // so that wiznet doesn't become unusable.
        if (!d->is_spammer_warned()) {
            log_string("{} input overflow!", d->host().c_str());
            d->warn_spammer();
            d->add_command("quit");
        }
        d->clear_input();
        return false;
    }

    d->add_command(data);
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
        // Retrieve the pc data for the person associated with this descriptor. The person contains user preferences
        // like prompt, colourisation and comm settings.
        const auto *person = d->person();
        // Character is used for the actual character status. This is very commonly this is the same as person, but if
        // an IMM is switched, then the person will be the IMM, and character will be the monster they're switched to.
        const auto *character = d->character();
        bool ansi = person->pcdata->colour;

        // Battle prompt.
        if (auto *fighting = character->fighting)
            d->write(colourise_mud_string(ansi, describe_fight_condition(*fighting)));

        if (!check_bit(person->comm, COMM_COMPACT))
            d->write("\n\r");

        if (check_bit(person->comm, COMM_PROMPT))
            d->write(colourise_mud_string(ansi, format_prompt(*character, person->pcdata->prompt)));
    }

    return d->flush_output();
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny(Descriptor *d, const char *argument) {
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    Char *ch;
    char *pwdnew;
    char *p;
    int iClass;
    int race;
    int i;
    int notes;
    bool fOld;

    while (isspace(*argument))
        argument++;

    ch = d->character();

    switch (d->state()) {

    default:
        bug("Nanny: bad d->state() {}.", static_cast<int>(d->state()));
        d->close();
        return;

    case DescriptorState::Closed:
        // Do nothing if the descriptor is already closed.
        return;

    case DescriptorState::GetName: {
        if (argument[0] == '\0') {
            d->close();
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

        auto res = try_load_player(char_name);
        ch = res.character.release(); // this is where we take ownership of the char
        fOld = !res.newly_created;
        d->character(ch);
        ch->desc = d;

        if (check_bit(ch->act, PLR_DENY)) {
            log_string("Denying access to {}@{}.", char_name.c_str(), d->host().c_str());
            d->write("You are denied access.\n\r");
            d->close();
            return;
        }

        if (check_reconnect(d, false)) {
            fOld = true;
        } else {
            if (wizlock && ch->is_mortal()) {
                d->write("The game is wizlocked.  Try again later - a reboot may be imminent.\n\r");
                d->close();
                return;
            }
        }

        if (fOld) {
            /* Old player */
            d->write("Password: ");
            SetEchoState(d, 0);
            d->state(DescriptorState::GetOldPassword);
            return;
        } else {
            /* New player */
            if (newlock) {
                d->write("The game is newlocked.  Try again later - a reboot may be imminent.\n\r");
                d->close();
                return;
            }

            // Check for a newban on player's site. This is the one time we use the full host name.
            if (check_ban(d->raw_full_hostname().c_str(), BAN_NEWBIES)
                || check_ban(d->raw_full_hostname().c_str(), BAN_PERMIT)) {
                d->write("Your site has been banned.  Only existing players from your site may connect.\n\r");
                d->close();
                return;
            }

            snprintf(buf, sizeof(buf), "Did I hear that right -  '%s' (Y/N)? ", char_name.c_str());
            d->write(buf);
            d->state(DescriptorState::ConfirmNewName);
            return;
        }
    } break;

    case DescriptorState::GetOldPassword:
        d->write("\n\r");

        // TODO crypt can return null if if fails (e.g. password is truncated).
        // for now we just pwd[0], which lets us reset passwords.
        if (!ch->pcdata->pwd.empty() && strcmp(crypt(argument, ch->pcdata->pwd.c_str()), ch->pcdata->pwd.c_str())) {
            d->write("Our survey said <Crude buzzer noise>.\n\rWrong password.\n\r");
            d->close();
            return;
        }
        // falls through
    case DescriptorState::CircumventPassword:
        if (ch->pcdata->pwd.empty()) {
            d->write("Oopsie! Null password!\n\r");
            d->write("Unless some IMM has been fiddling, then this is a bug!\n\r");
            d->write("Type 'password null <new password>' to fix.\n\r");
        }

        SetEchoState(d, 1);

        // This is the one time we use the full host name.
        if (check_ban(d->raw_full_hostname().c_str(), BAN_PERMIT) && (!ch->is_set_extra(EXTRA_PERMIT))) {
            d->write("Your site has been banned.  Sorry.\n\r");
            d->close();
            return;
        }

        if (check_playing(d, ch->name))
            return;

        if (check_reconnect(d, true))
            return;

        log_new(fmt::format("{}@{} has connected.", ch->name, d->host()), EXTRA_WIZNET_DEBUG,
                ch->is_wizinvis() || ch->is_prowlinvis() ? ch->get_trust() : 0);

        d->write("Does your terminal support ANSI colour (Y/N/Return = as saved)?");
        d->state((d->state() == DescriptorState::CircumventPassword) ? DescriptorState::ReadMotd
                                                                     : DescriptorState::GetAnsi);

        break;

        /* RT code for breaking link */

    case DescriptorState::BreakConnect:
        switch (*argument) {
        case 'y':
        case 'Y':
            for (auto &d_old : descriptors().all()) {
                if (&d_old == d || d_old.character() == nullptr)
                    continue;

                if (!matches(ch->name, d_old.character()->name))
                    continue;

                d_old.close();
            }
            if (check_reconnect(d, true))
                return;
            d->write("Reconnect attempt failed.\n\rName: ");
            if (d->character()) {
                delete d->character();
                d->character(nullptr);
            }
            d->state(DescriptorState::GetName);
            break;

        case 'n':
        case 'N':
            d->write("Name: ");
            if (d->character()) {
                delete d->character();
                d->character(nullptr);
            }
            d->state(DescriptorState::GetName);
            break;

        default: d->write("Please type Y or N? "); break;
        }
        break;

    case DescriptorState::ConfirmNewName:
        switch (*argument) {
        case 'y':
        case 'Y':
            SetEchoState(d, 0);
            d->write(fmt::format("Welcome new character, to Xania.\n\rThink of a password for {}: ", ch->name));
            d->state(DescriptorState::GetNewPassword);
            break;

        case 'n':
        case 'N':
            d->write("Ack! Amateurs! Try typing it in properly: ");
            delete d->character();
            d->character(nullptr);
            d->state(DescriptorState::GetName);
            break;

        default: d->write("It's quite simple - type Yes or No: "); break;
        }
        break;

    case DescriptorState::GetAnsi:
        if (argument[0] == '\0') {
            if (ch->pcdata->colour) {
                ch->send_line("This is a |RC|GO|BL|rO|gU|bR|cF|YU|PL |RM|GU|BD|W!");
            }
            if (ch->is_immortal()) {
                do_help(ch, "imotd");
                d->state(DescriptorState::ReadIMotd);
            } else {
                do_help(ch, "motd");
                d->state(DescriptorState::ReadMotd);
            }
        } else {
            switch (*argument) {
            case 'y':
            case 'Y':
                ch->pcdata->colour = true;
                ch->send_line("This is a |RC|GO|BL|rO|gU|bR|cF|YU|PL |RM|GU|BD|W!");
                if (ch->is_immortal()) {
                    do_help(ch, "imotd");
                    d->state(DescriptorState::ReadIMotd);
                } else {
                    do_help(ch, "motd");
                    d->state(DescriptorState::ReadMotd);
                }
                break;

            case 'n':
            case 'N':
                ch->pcdata->colour = false;
                if (ch->is_immortal()) {
                    do_help(ch, "imotd");
                    d->state(DescriptorState::ReadIMotd);
                } else {
                    do_help(ch, "motd");
                    d->state(DescriptorState::ReadMotd);
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

        pwdnew = crypt(argument, ch->name.c_str());
        for (p = pwdnew; *p != '\0'; p++) {
            if (*p == '~') {
                d->write("New password not acceptable, try again.\n\rPassword: ");
                return;
            }
        }

        ch->pcdata->pwd = pwdnew;
        d->write("Please retype password: ");
        d->state(DescriptorState::ConfirmNewPassword);
        break;

    case DescriptorState::ConfirmNewPassword:
        d->write("\n\r");

        if (strcmp(crypt(argument, ch->pcdata->pwd.c_str()), ch->pcdata->pwd.c_str())) {
            d->write("You could try typing the same thing in twice...\n\rRetype password: ");
            d->state(DescriptorState::GetNewPassword);
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
        d->state(DescriptorState::GetNewRace);
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
        ch->perm_stat = pc_race_table[race].stats;
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

        d->write("Are you male, female or other (M/F/O)? ");
        d->state(DescriptorState::GetNewSex);
        break;

    case DescriptorState::GetNewSex:
        if (auto sex = Sex::try_from_char(argument[0])) {
            ch->sex = *sex;
            ch->pcdata->true_sex = *sex;
        } else {
            d->write("Please specify (M)ale, (F)emale or (O)ther. ");
            return;
        }
        d->write("Thanks. Personal pronouns can be set using the 'pronouns' command later on.\n");
        strcpy(buf, "The following classes are available: ");
        for (iClass = 0; iClass < MAX_CLASS; iClass++) {
            if (iClass > 0)
                strcat(buf, " ");
            strcat(buf, class_table[iClass].name);
        }
        strcat(buf, "\n\r");
        d->write(buf);
        d->write("What is your class (help for more information)? ");
        d->state(DescriptorState::GetNewClass);
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
        log_string("{}@{} new player.", ch->name, d->host());
        d->write("\n\r");
        d->write("You may be good, neutral, or evil.\n\r");
        d->write("Which alignment (G/N/E)? ");
        d->state(DescriptorState::GetAlignment);
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
        ch->send_line("Customizing your character allows for a wider range of skills,  If you're new");
        ch->send_line("to the game we recommend skipping customization for now.");
        ch->send_line("At level 60+ characters can learn skills they didn't start with.");
        ch->send_line("Customize (Y/N)? ");
        d->state(DescriptorState::DefaultChoice);
        break;

    case DescriptorState::DefaultChoice:
        d->write("\n\r");
        switch (argument[0]) {
        case 'y':
        case 'Y':
            ch->generation = (CharGeneration *)alloc_perm(sizeof(*ch->generation));
            ch->generation->points_chosen = ch->pcdata->points;
            do_help(ch, "group header");
            list_group_costs(ch);
            ch->send_line("You already have the following skills:");
            do_skills(ch);
            do_help(ch, "menu choice");
            d->state(DescriptorState::GenGroups);
            break;
        case 'n':
        case 'N':
            group_add(ch, class_table[ch->class_num].default_group, true);
            d->write("\n\r");
            d->write("Does your terminal support ANSI colour (Y/N/Return = as saved)?");
            d->state(DescriptorState::GetAnsi);

            break;
        default: d->write("Please answer (Y/N)? "); return;
        }
        break;

    case DescriptorState::GenGroups:
        ch->send_line("");
        if (!str_cmp(argument, "done")) {
            snprintf(buf, sizeof(buf), "Creation points: %d\n\r", ch->pcdata->points);
            ch->send_to(buf);
            snprintf(buf, sizeof(buf), "Experience per level: %d\n\r",
                     exp_per_level(ch, ch->generation->points_chosen));
            if (ch->pcdata->points < 40)
                ch->train = (40 - ch->pcdata->points + 1) / 2;
            ch->send_to(buf);
            d->write("\n\r");
            d->write("Does your terminal support ANSI colour (Y/N/Return = as saved)?");
            d->state(DescriptorState::GetAnsi);

            break;
        }

        if (!parse_gen_groups(ch, argument))
            ch->send_line("Choice (add, drop, help, info, learned, list, done)?");

        do_help(ch, "menu choice");
        break;

    case DescriptorState::ReadIMotd:
        d->write("\n\r");
        do_help(ch, "motd");
        d->state(DescriptorState::ReadMotd);
        break;

    case DescriptorState::ReadMotd:
        d->write("\n\rWelcome to Xania.  May your stay be eventful.\n\r");
        char_list.add_front(ch);
        d->state(DescriptorState::Playing);
        /* Moog: tell doorman we logged in OK */
        {
            Packet p;
            p.type = PACKET_AUTHORIZED;
            p.channel = d->channel();
            p.nExtra = ch->name.size();
            send_to_doorman(&p, ch->name.c_str());
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
            ch->send_to("the {}", title_table[ch->class_num][ch->level][ch->sex.is_male() ? 0 : 1]);

            do_outfit(ch);
            obj_to_char(create_object(get_obj_index(objects::Map)), ch);

            ch->pcdata->learned[get_weapon_sn(ch)] = 40;

            char_to_room(ch, get_room(rooms::MudschoolEntrance));
            ch->send_line("");
            do_help(ch, "NEWBIE INFO");
            ch->send_line("");

            /* hack to let the newbie know about the tipwizard */
            ch->send_line("|WTip: this is Xania's tip wizard! Type 'tips' to turn this on or off.|w");
            /* turn on the newbie's tips */
            ch->set_extra(EXTRA_TIP_WIZARD);

        } else if (ch->in_room) {
            char_to_room(ch, ch->in_room);
        } else if (ch->is_immortal()) {
            char_to_room(ch, get_room(rooms::Chat));
        } else {
            char_to_room(ch, get_room(rooms::MidgaardTemple));
        }

        announce(fmt::format("|W### |P{}|W has entered the game.|w", ch->name), ch);
        act("|P$n|W has entered the game.", ch);
        look_auto(ch);

        /* Rohan: code to increase the player count if needed - it was only
           updated if a player did count */
        max_on = std::max(static_cast<size_t>(ranges::distance(descriptors().all())), max_on);

        if (ch->gold > 250000 && ch->is_mortal()) {
            snprintf(buf, sizeof(buf), "You are taxed %ld gold to pay for the Mayor's bar.\n\r",
                     (ch->gold - 250000) / 2);
            ch->send_to(buf);
            ch->gold -= (ch->gold - 250000) / 2;
        }

        if (ch->pet) {
            char_to_room(ch->pet, ch->in_room);
            act("|P$n|W has entered the game.", ch->pet);
        }

        /* check notes */
        notes = NoteHandler::singleton().num_unread(*ch);

        if (notes > 0) {
            snprintf(buf, sizeof(buf), "\n\rYou have %d new note%s waiting.\n\r", notes, (notes == 1) ? "" : "s");
            ch->send_to(buf);
        }
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

    if (matches(name, "DEATH"))
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

    // Prevent players from naming themselves after mobs.
    if (ranges::any_of(all_mob_indexes(), [&](const auto &p) { return is_name(name, p.player_name); }))
        return false;

    return true;
}

/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect(Descriptor *d, bool fConn) {
    for (auto ch : char_list) {
        if (ch->is_pc() && (!fConn || ch->desc == nullptr) && matches(d->character()->name, ch->name)) {
            if (fConn == false) {
                d->character()->pcdata->pwd = ch->pcdata->pwd;
            } else {
                delete d->character();
                d->character(ch);
                ch->desc = d;
                ch->timer = 0;
                ch->send_line("Reconnecting.");
                act("$n has reconnected.", ch);
                log_new(fmt::format("{}@{} reconnected.", ch->name, d->host().c_str()), EXTRA_WIZNET_DEBUG,
                        (ch->is_wizinvis() || ch->is_prowlinvis()) ? ch->get_trust() : 0);
                d->state(DescriptorState::Playing);
            }
            return true;
        }
    }

    return false;
}

/*
 * Check if already playing.
 */
bool check_playing(Descriptor *d, std::string_view name) {
    for (auto &dold : descriptors().all()) {
        if (&dold != d && dold.character() != nullptr && dold.state() != DescriptorState::GetName
            && dold.state() != DescriptorState::GetOldPassword && matches(name, dold.person()->name)) {
            d->write("That character is already playing.\n\r");
            d->write("Do you wish to connect anyway (Y/N)?");
            d->state(DescriptorState::BreakConnect);
            return true;
        }
    }

    return false;
}

/*
 * Send a page to one char.
 */
void page_to_char(const char *txt, Char *ch) {
    if (txt)
        ch->page_to(txt);
}

void act(std::string_view format, const Char *ch, Act1Arg arg1, Act2Arg arg2, To type) {
    act(format, ch, arg1, arg2, type, Position::Type::Resting);
}

namespace {

std::string_view he_she(const Char *ch) { return subjective(*ch); }
std::string_view him_her(const Char *ch) { return objective(*ch); }
std::string_view his_her(const Char *ch) { return possessive(*ch); }
std::string_view himself_herself(const Char *ch) { return reflexive(*ch); }

std::string format_act(std::string_view format, const Char *ch, Act1Arg arg1, Act2Arg arg2, const Char *to,
                       const Char *vch) {
    std::string buf;

    bool prev_was_dollar = false;
    for (auto c : format) {
        if (!std::exchange(prev_was_dollar, false)) {
            if (c != '$') {
                prev_was_dollar = false;
                buf.push_back(c);
            } else {
                prev_was_dollar = true;
            }
            continue;
        }

        if (std::holds_alternative<nullptr_t>(arg2) && c >= 'A' && c <= 'Z') {
            bug("Act: missing arg2 for code {} in format '{}'", c, format);
            continue;
        }

        switch (c) {
        default:
            bug("Act: bad code {} in format '{}'", c, format);
            break;
            /* Thx alex for 't' idea */
        case 't':
            if (auto arg1_as_string_ptr = std::get_if<std::string_view>(&arg1)) {
                buf += *arg1_as_string_ptr;
            } else {
                bug("$t passed but arg1 was not a string in '{}'", format);
            }
            break;
        case 'T':
            if (auto arg2_as_string_ptr = std::get_if<std::string_view>(&arg2)) {
                buf += *arg2_as_string_ptr;
            } else {
                bug("$T passed but arg2 was not a string in '{}'", format);
            }
            break;
        case 'n': buf += pers(ch, to); break;
        case 'N': buf += pers(vch, to); break;
        case 'e': buf += he_she(ch); break;
        case 'E': buf += he_she(vch); break;
        case 'm': buf += him_her(ch); break;
        case 'M': buf += him_her(vch); break;
        case 's': buf += his_her(ch); break;
        case 'S': buf += his_her(vch); break;
        case 'r': buf += himself_herself(ch); break;
        case 'R': buf += himself_herself(vch); break;

        case 'p':
            if (auto arg1_as_obj_ptr = std::get_if<const Object *>(&arg1)) {
                auto &obj1 = *arg1_as_obj_ptr;
                buf += can_see_obj(to, obj1) ? obj1->short_descr : "something";
            } else {
                bug("$p passed but arg1 was not an object in '{}'", format);
                buf += "something";
            }
            break;

        case 'P':
            if (auto arg2_as_obj_ptr = std::get_if<const Object *>(&arg2)) {
                auto &obj2 = *arg2_as_obj_ptr;
                buf += can_see_obj(to, obj2) ? obj2->short_descr : "something";
            } else {
                bug("$p passed but arg2 was not an object in '{}'", format);
                buf += "something";
            }
            break;

        case 'd':
            if (auto arg2_as_string_ptr = std::get_if<std::string_view>(&arg2);
                arg2_as_string_ptr != nullptr && !arg2_as_string_ptr->empty()) {
                buf += ArgParser(*arg2_as_string_ptr).shift();
            } else {
                buf += "door";
            }
            break;
        }
    }

    return upper_first_character(buf) + "\n\r";
}

bool act_to_person(const Char *person, const Position::Type min_position) {
    // Ignore folks with no descriptor, or below minimum position.
    return person->desc != nullptr && person->position >= min_position;
}

std::vector<const Char *> folks_in_room(const Room *room, const Char *ch, const Char *vch, const To &type,
                                        const Position::Type min_position) {
    std::vector<const Char *> result;
    for (auto *person : room->people) {
        if (!act_to_person(person, min_position))
            continue;
        // Never consider the character themselves (they're handled explicitly elsewhere).
        if (person == ch)
            continue;
        // Ignore the victim if necessary.
        if (type == To::NotVict && person == vch)
            continue;
        result.emplace_back(person);
    }
    return result;
}

std::vector<const Char *> collect_folks(const Char *ch, const Char *vch, Act2Arg arg2, To type,
                                        const Position::Type min_position) {
    const Room *room{};

    switch (type) {
    case To::Char:
        if (act_to_person(ch, min_position))
            return {ch};
        else
            return {};

    case To::Vict:
        if (vch == nullptr) {
            bug("Act: null or incorrect type of vch");
            return {};
        }
        if (vch->in_room == nullptr || ch == vch || !act_to_person(vch, min_position))
            return {};

        return {vch};

    case To::GivenRoom:
        if (auto arg2_as_room_ptr = std::get_if<const Room *>(&arg2)) {
            room = *arg2_as_room_ptr;
        } else {
            bug("Act: null room with To::GivenRoom.");
            return {};
        }
        break;

    case To::Room:
    case To::NotVict: room = ch->in_room; break;
    }

    auto result = folks_in_room(room, ch, vch, type, min_position);

    // If we're sending messages to the challenge arena...
    if (room->vnum == rooms::ChallengeArena) {
        // also include all the folks in the viewing gallery with the appropriate position. We assume the victim
        // is not somehow in the viewing gallery.
        auto viewing = folks_in_room(get_room(rooms::ChallengeGallery), ch, vch, type, min_position);
        result.insert(result.end(), viewing.begin(), viewing.end());
    }

    return result;
}

}

void act(std::string_view format, const Char *ch, Act1Arg arg1, Act2Arg arg2, To type,
         const Position::Type min_position) {
    if (format.empty() || !ch || !ch->in_room)
        return;

    const Char *vch = std::get_if<const Char *>(&arg2) ? *std::get_if<const Char *>(&arg2) : nullptr;

    for (auto *to : collect_folks(ch, vch, arg2, type, min_position)) {
        auto formatted = format_act(format, ch, arg1, arg2, to, vch);
        to->send_to(formatted);
        if (MOBtrigger) {
            auto arg1_as_obj_ptr = std::get_if<const Object *>(&arg1);
            // TODO: heinous const_cast here. Safe, but annoying and worth unpicking deeper down.
            mprog_act_trigger(formatted.c_str(), const_cast<Char *>(to), ch,
                              arg1_as_obj_ptr ? *arg1_as_obj_ptr : nullptr, vch);
        }
    }
    MOBtrigger = true;
}

namespace {

std::string format_one_prompt_part(char c, const Char &ch) {
    switch (c) {
    case '%': return "%";
    case 'B': {
        std::string bar;
        constexpr auto NumGradations = 10u;
        auto num_full = static_cast<float>(NumGradations * ch.hit) / ch.max_hit;
        char prev_colour = 0;
        for (auto loop = 0u; loop < NumGradations; ++loop) {
            auto fullness = num_full - loop;
            if (fullness <= 0.25f)
                bar += ' ';
            else {
                char colour = 'g';
                if (loop < NumGradations / 3)
                    colour = 'r';
                else if (loop < (2 * NumGradations) / 3)
                    colour = 'y';
                if (colour != prev_colour) {
                    bar = bar + '|' + colour;
                    prev_colour = colour;
                }
                if (fullness <= 0.5f)
                    bar += ".";
                else if (fullness < 1.f)
                    bar += ":";
                else
                    bar += "||";
            }
        }
        return bar + "|p";
    }

    case 'c': return fmt::format("{}", ch.wait);
    case 'h': return fmt::format("{}", ch.hit);
    case 'H': return fmt::format("{}", ch.max_hit);
    case 'm': return fmt::format("{}", ch.mana);
    case 'M': return fmt::format("{}", ch.max_mana);
    case 'g': return fmt::format("{}", ch.gold);
    case 'x':
        return fmt::format("{}",
                           (long int)(((long int)(ch.level + 1) * (long int)(exp_per_level(&ch, ch.pcdata->points))
                                       - (long int)(ch.exp))));
    case 'v': return fmt::format("{}", ch.move);
    case 'V': return fmt::format("{}", ch.max_move);
    case 'a':
        if (ch.get_trust() > 10)
            return fmt::format("{}", ch.alignment);
        else
            return "??";
    case 'X': return fmt::format("{}", ch.exp);
    case 'p':
        if (ch.player())
            return ch.player()->pcdata->prefix;
        return "";
    case 'r':
        if (ch.is_immortal())
            return ch.in_room->name;
        break;
    case 'W':
        if (ch.is_immortal())
            return ch.is_wizinvis() ? "|R*W*|p" : "-w-";
        break;
        /* Can be changed easily for tribes/clans etc. */
    case 'P':
        if (ch.is_immortal())
            return ch.is_prowlinvis() ? "|R*P*|p" : "-p-";
        break;
    case 'R':
        if (ch.is_immortal())
            return fmt::format("{}", ch.in_room->vnum);
        break;
    case 'z':
        if (ch.is_immortal())
            return ch.in_room->area->areaname;
        break;
    case 'n': return "\n\r";
    case 't': {
        auto ch_timet = Clock::to_time_t(current_time);
        char time_buf[MAX_STRING_LENGTH];
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", gmtime(&ch_timet));
        return time_buf;
    } break;
    default: break;
    }

    return "|W ?? |p";
}

}

std::string format_prompt(const Char &ch, std::string_view prompt) {
    if (prompt.empty()) {
        return fmt::format("|p<{}/{}hp {}/{}m {}mv {}cd> |w", ch.hit, ch.max_hit, ch.mana, ch.max_mana, ch.move,
                           ch.wait);
    }

    bool prev_was_escape = false;
    std::string buf = "|p";
    for (auto c : prompt) {
        if (std::exchange(prev_was_escape, false)) {
            buf += format_one_prompt_part(c, ch);
        } else if (c == '%')
            prev_was_escape = true;
        else
            buf += c;
    }
    return buf + "|w";
}
