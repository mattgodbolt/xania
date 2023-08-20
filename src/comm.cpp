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
#include "Area.hpp"
#include "Ban.hpp"
#include "Char.hpp"
#include "Classes.hpp"
#include "CommFlag.hpp"
#include "Descriptor.hpp"
#include "DescriptorList.hpp"
#include "Help.hpp"
#include "Logging.hpp"
#include "MProg.hpp"
#include "MobIndexData.hpp"
#include "Note.hpp"
#include "Object.hpp"
#include "PcCustomization.hpp"
#include "PlayerActFlag.hpp"
#include "Pronouns.hpp"
#include "Races.hpp"
#include "SkillNumbers.hpp"
#include "TimeInfoData.hpp"
#include "Titles.hpp"
#include "VnumObjects.hpp"
#include "VnumRooms.hpp"
#include "act_comm.hpp"
#include "act_info.hpp"
#include "challenge.hpp"
#include "common/BitOps.hpp"
#include "common/Fd.hpp"
#include "common/doorman_protocol.h"
#include "db.h"
#include "fight.hpp"
#include "handler.hpp"
#include "interp.h"
#include "save.hpp"
#include "skills.hpp"
#include "string_utils.hpp"
#include "update.hpp"

#include <range/v3/algorithm/any_of.hpp>

#include <csignal>
#include <fstream>
#include <netinet/in.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <thread>

using namespace std::literals;

extern size_t max_on;

/*
 * Global variables.
 */
DescriptorList the_descriptors;
DescriptorList &descriptors() { return the_descriptors; }
bool merc_down; /* Shutdown       */
bool wizlock; /* Game is wizlocked    */
bool newlock; /* Game is newlocked    */

bool read_from_descriptor(Descriptor *d, std::string_view data);
void move_active_char_from_limbo(Char *ch);

/*
 * Other local functions (OS-independent).
 */
bool validate_player_name(std::string_view name);
bool check_reconnect(Descriptor *d, bool fConn);
bool check_playing(Descriptor *d, std::string_view name);
void nanny(Descriptor *d, std::string_view argument);
bool process_output(Descriptor *d, bool fPrompt);

/* Handle to get to doorman */
Fd doormanDesc;

/* Send a packet to doorman */
bool send_to_doorman(const Packet *p, const void *extra) {
    // TODO: do something rather than return here if there's a failure
    if (!doormanDesc.is_open())
        return false;
    if (p->nExtra > PacketMaxPayloadSize) {
        bug("MUD tried to send a doorman packet with payload size {} > {}! Dropping!", p->nExtra, PacketMaxPayloadSize);
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

namespace {

const auto GoldTaxThreshold = 250000u;
const auto NewbieWeaponSkillPct = 40u;
const auto NewbieNumTrains = 3u;
const auto NewbieNumPracs = 5u;

void set_echo_state(Descriptor *d, int on) {
    Packet p;
    p.type = on ? PACKET_ECHO_ON : PACKET_ECHO_OFF;
    p.channel = d->channel();
    p.nExtra = 0;
    send_to_doorman(&p, nullptr);
}

bool send_go_ahead(Descriptor *d) {
    Packet p;
    p.type = PACKET_GO_AHEAD;
    p.channel = d->channel();
    p.nExtra = 0;
    return send_to_doorman(&p, nullptr);
}

void greet(Descriptor &d) {
    const auto *greeting = HelpList::singleton().lookup(0, "greeting");
    if (!greeting) {
        bug("Unable to look up greeting");
        return;
    }
    d.write(greeting->text());
}

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

void lobby_maybe_enable_colour(Descriptor *desc) {
    if (desc->ansi_terminal_detected()) {
        desc->character()->pcdata->colour = true;
    }
}

void lobby_prompt_for_race(Descriptor *desc) {
    do_help(desc->character(), "selectrace");
    desc->state(DescriptorState::GetNewRace);
}

void lobby_prompt_for_class(Descriptor *desc) {
    do_help(desc->character(), "selectclass");
    desc->state(DescriptorState::GetNewClass);
}

// If Doorman didn't detect an ansi terminal ask the user.
void lobby_prompt_for_ansi(Descriptor *desc) {
    desc->write("Does your terminal support ANSI colour (Y/N/Return = as saved)?");
    desc->state(DescriptorState::GetAnsi);
}

void lobby_show_motd(Descriptor *desc) {
    auto *ch = desc->character();
    if (ch->pcdata->colour) {
        ch->send_line("This is a |RC|GO|BL|rO|gU|bR|cF|YU|PL |RM|GU|BD|W!");
    }
    if (ch->is_immortal()) {
        do_help(ch, "imotd");
        desc->state(DescriptorState::ReadIMotd);
    } else {
        do_help(ch, "motd");
        desc->state(DescriptorState::ReadMotd);
    }
}

// The final step in the lobby is to save the character. In the meantime, if another new character
// with the same name was created and saved, this prevents duplication/overwriting.
bool lobby_char_was_created(Descriptor *desc) {
    struct stat player_file;
    if (!stat(filename_for_player(desc->character()->name.c_str()).c_str(), &player_file)) {
        desc->write("Your character could not be created, please try again.\n\r");
        desc->close();
        return false;
    } else {
        save_char_obj(desc->character());
        return true;
    }
}

}

/* where we're asked nicely to quit from the outside */
void handle_signal_shutdown() {
    log_string("Signal shutdown received");

    /* ask everyone to save! */
    for (auto *vch : char_list) {
        // vch->d->c check added by TM to avoid crashes when
        // someone hasn't logged in but the mud is shut down
        if (vch->is_pc() && vch->desc && vch->desc->is_playing()) {
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
            nanny(d, buffer);
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
            d->ansi_terminal_detected(data->ansi);
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
                        if (p.nExtra > PacketMaxPayloadSize) {
                            bug("Doorman sent a too big packet! {} > {}: dropping", p.nExtra, PacketMaxPayloadSize);
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
                    interpret(character, *incomm);
                else
                    nanny(&d, *incomm);
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
            log_string("{} input overflow!", d->host());
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
    if (!merc_down && d->is_paging()) {
        d->write("[Hit Return to continue]\n\r");
        return d->flush_output();
    } else if (fPrompt && !merc_down && d->is_playing()) {
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

        if (!check_enum_bit(person->comm, CommFlag::Compact))
            d->write("\n\r");

        if (check_enum_bit(person->comm, CommFlag::Prompt))
            d->write(colourise_mud_string(ansi, format_prompt(*character, person->pcdata->prompt)));
        return d->flush_output() && send_go_ahead(d);
    } else {
        return d->flush_output();
    }
}

/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny(Descriptor *d, std::string_view argument) {
    const auto &bans = Bans::singleton();
    argument = ltrim(argument);
    auto *ch = d->character();
    switch (d->state()) {
    default:
        bug("Nanny: bad d->state() {}.", static_cast<int>(d->state()));
        d->close();
        return;

    case DescriptorState::Closed:
        // Do nothing if the descriptor is already closed.
        return;

    case DescriptorState::GetName: {
        if (argument.empty()) {
            d->close();
            return;
        }
        // Take a copy of the character's proposed name, so we can forcibly upper-case its first letter.
        const auto player_name = upper_first_character(argument);
        if (!validate_player_name(player_name)) {
            d->write("Illegal name, try another.\n\rName: ");
            return;
        }
        auto res = try_load_player(player_name);
        ch = res.character.release(); // this is where we take ownership of the char
        auto existing_player = !res.newly_created;
        d->character(ch);
        ch->desc = d;

        if (check_enum_bit(ch->act, PlayerActFlag::PlrDeny)) {
            log_string("Denying access to {}@{}.", player_name, d->host());
            d->write("You are denied access.\n\r");
            d->close();
            return;
        }

        if (check_reconnect(d, false)) {
            existing_player = true;
        } else {
            if (wizlock && ch->is_mortal()) {
                d->write("The game is wizlocked.  Try again later - a reboot may be imminent.\n\r");
                d->close();
                return;
            }
        }
        if (existing_player) {
            d->write("Password: ");
            set_echo_state(d, 0);
            d->state(DescriptorState::GetOldPassword);
            return;
        } else {
            if (newlock) {
                d->write("The game is newlocked.  Try again later - a reboot may be imminent.\n\r");
                d->close();
                return;
            }
            if (bans.check_ban(d->raw_full_hostname(), to_int(BanFlag::Newbies) | to_int(BanFlag::All))) {
                d->write("Your site has been banned.\n\r");
                d->close();
                return;
            }
            d->write(fmt::format("Did I hear that right - '{}' (Y/N)? ", player_name));
            d->state(DescriptorState::ConfirmNewName);
            return;
        }
    }

    case DescriptorState::GetOldPassword:
        d->write("\n\r");

        // TODO crypt can return null if if fails (e.g. password is truncated).
        // for now we just pwd[0], which lets us reset passwords.
        if (!ch->pcdata->pwd.empty()
            && strcmp(crypt(std::string(argument).c_str(), ch->pcdata->pwd.c_str()), ch->pcdata->pwd.c_str())) {
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

        set_echo_state(d, 1);

        if ((!ch->is_set_extra(CharExtraFlag::Permit) && (bans.check_ban(d->raw_full_hostname(), BanFlag::Permit)))
            || bans.check_ban(d->raw_full_hostname(), BanFlag::All)) {
            d->write("Your site has been banned.  Sorry.\n\r");
            d->close();
            return;
        }

        if (check_playing(d, ch->name))
            return;

        if (check_reconnect(d, true))
            return;

        log_new(fmt::format("{}@{} has connected.", ch->name, d->host()), CharExtraFlag::WiznetDebug,
                ch->is_wizinvis() || ch->is_prowlinvis() ? ch->get_trust() : 0);

        lobby_maybe_enable_colour(d);
        if (ch->pcdata->colour) {
            lobby_show_motd(d);
        } else {
            lobby_prompt_for_ansi(d);
        }
        break;

    case DescriptorState::BreakConnect:
        switch (argument[0]) {
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
        switch (argument[0]) {
        case 'y':
        case 'Y':
            set_echo_state(d, 0);
            d->write(fmt::format("Welcome new character, to Xania.\n\rThink of a password for {}: ", ch->name));
            d->state(DescriptorState::GetNewPassword);
            break;

        case 'n':
        case 'N':
            d->write("OK, try again: ");
            delete d->character();
            d->character(nullptr);
            d->state(DescriptorState::GetName);
            break;

        default: d->write("It's quite simple - type Yes or No: "); break;
        }
        break;

    case DescriptorState::GetAnsi:
        if (argument.empty()) {
            lobby_show_motd(d);
        } else {
            switch (argument[0]) {
            case 'y':
            case 'Y':
                ch->pcdata->colour = true;
                lobby_show_motd(d);
                break;
            case 'n':
            case 'N':
                ch->pcdata->colour = false;
                lobby_show_motd(d);
                break;
            default: d->write("Please type Yes or No press return: "); break;
            }
        }
        break;

    case DescriptorState::GetNewPassword: {
        d->write("\n\r");
        if (argument.length() < MinPasswordLen) {
            d->write("Password must be at least five characters long.\n\rPassword: ");
            return;
        }
        auto *new_password = crypt(std::string(argument).c_str(), ch->name.c_str());
        for (auto *p = new_password; *p != '\0'; p++) {
            if (*p == '~') {
                d->write("New password not acceptable, try again.\n\rPassword: ");
                return;
            }
        }
        ch->pcdata->pwd = new_password;
        d->write("Please retype password: ");
        d->state(DescriptorState::ConfirmNewPassword);
        break;
    }
    case DescriptorState::ConfirmNewPassword:
        d->write("\n\r");

        if (strcmp(crypt(std::string(argument).c_str(), ch->pcdata->pwd.c_str()), ch->pcdata->pwd.c_str())) {
            d->write("You could try typing the same thing in twice...\n\rRetype password: ");
            d->state(DescriptorState::GetNewPassword);
            return;
        }

        set_echo_state(d, 1);
        lobby_maybe_enable_colour(d);
        lobby_prompt_for_race(d);
        break;

    case DescriptorState::GetNewRace: {
        auto parser = ArgParser(argument);
        auto choice = parser.shift();
        if (matches(choice, "help")) {
            if (parser.remaining().empty()) {
                do_help(ch, "race help");
            } else {
                do_help(ch, parser.remaining());
            }
            d->write("What is your race (help for more information)? ");
            break;
        }
        auto race = race_lookup(choice);
        if (race == 0 || !race_table[race].pc_race) {
            ch->send_line("That is not a valid race.");
            lobby_prompt_for_race(d);
            break;
        }

        ch->race = race;
        /* initialize stats */
        ch->perm_stat = pc_race_table[race].stats;
        ch->affected_by = (int)(ch->affected_by | race_table[race].aff);
        ch->imm_flags = ch->imm_flags | race_table[race].imm;
        ch->res_flags = ch->res_flags | race_table[race].res;
        ch->vuln_flags = ch->vuln_flags | race_table[race].vuln;
        ch->morphology = race_table[race].morphology;
        ch->parts = race_table[race].parts;

        /* add skills */
        for (auto i = 0; i < MAX_PC_RACE_BONUS_SKILLS; i++) {
            if (pc_race_table[race].skills[i] == nullptr)
                break;
            group_add(ch, pc_race_table[race].skills[i], false);
        }
        /* add cost */
        ch->pcdata->points = pc_race_table[race].points;
        ch->body_size = pc_race_table[race].body_size;

        ch->send_to("Are you male, female or other (M/F/O)? ");
        d->state(DescriptorState::GetNewSex);
        break;
    }
    case DescriptorState::GetNewSex: {
        if (auto sex = Sex::try_from_char(argument[0])) {
            ch->sex = *sex;
            ch->pcdata->true_sex = *sex;
        } else {
            ch->send_to("Please specify (M)ale, (F)emale or (O)ther. ");
            return;
        }
        ch->send_line("OK. Personal pronouns can be set later using the |ypronouns|w command.");
        lobby_prompt_for_class(d);
        break;
    }
    case DescriptorState::GetNewClass: {
        auto parser = ArgParser(argument);
        auto choice = parser.shift();
        if (matches(choice, "help")) {
            if (parser.remaining().empty()) {
                do_help(ch, "classes");
            } else {
                do_help(ch, parser.remaining());
            }
            d->write("What is your class (help for more information)? ");
            break;
        }
        if (const auto chosen_class = class_lookup(choice); chosen_class >= 0) {
            ch->class_num = chosen_class;
            log_string("{}@{} new player.", ch->name, d->host());
            ch->send_line("\n\rYou may be good, neutral, or evil.");
            ch->send_to("Which alignment (G/N/E)? ");
            d->state(DescriptorState::GetAlignment);
        } else {
            ch->send_to("That's not a class.\n\rWhat IS your class? ");
            return;
        }
        break;
    }
    case DescriptorState::GetAlignment:
        switch (argument[0]) {
        case 'g':
        case 'G': ch->alignment = Alignment::Saintly; break;
        case 'n':
        case 'N': ch->alignment = Alignment::Neutral; break;
        case 'e':
        case 'E': ch->alignment = Alignment::Demonic; break;
        default:
            ch->send_line("That's not a valid alignment.\n\r");
            ch->send_to("Which alignment (G/N/E)? ");
            return;
        }
        ch->send_line("");
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
            ch->pcdata->customization = PcCustomization{};
            ch->pcdata->customization->points_chosen = ch->pcdata->points;
            do_help(ch, "group header");
            list_available_group_costs(ch);
            ch->send_line("You already have the following skills:");
            do_skills(ch);
            do_help(ch, "menu choice");
            d->state(DescriptorState::Customize);
            break;
        case 'n':
        case 'N':
            group_add(ch, class_table[ch->class_num].default_group, true);
            if (!lobby_char_was_created(d)) {
                break;
            }
            d->write("\n\r");
            if (ch->pcdata->colour) {
                lobby_show_motd(d);
            } else {
                lobby_prompt_for_ansi(d);
            }
            break;
        default: d->write("Please answer (Y/N)? "); return;
        }
        break;

    case DescriptorState::Customize:
        ch->send_line("");
        if (matches(argument, "done")) {
            ch->send_line("|WCreation points|w: {}", ch->pcdata->points);
            ch->send_line("|GExperience per level|w: {}", exp_per_level(ch, ch->pcdata->customization->points_chosen));
            if (ch->pcdata->points < 40)
                ch->train = (40 - ch->pcdata->points + 1) / 2;
            if (!lobby_char_was_created(d)) {
                break;
            }
            if (ch->pcdata->colour) {
                lobby_show_motd(d);
            } else {
                lobby_prompt_for_ansi(d);
            }
            break;
        }
        parse_customizations(ch, ArgParser(argument));
        do_help(ch, "menu choice");
        break;

    case DescriptorState::ReadIMotd:
        d->write("\n\r");
        do_help(ch, "motd");
        d->state(DescriptorState::ReadMotd);
        break;

    case DescriptorState::ReadMotd:
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
            ch->train = NewbieNumTrains;
            ch->practice = NewbieNumPracs;
            ch->send_to("the {}", title_table[ch->class_num][ch->level][ch->sex.is_male() ? 0 : 1]);

            do_outfit(ch);
            auto map_uptr = get_obj_index(Objects::Map)->create_object();
            auto *map = map_uptr.get();
            object_list.push_back(std::move(map_uptr));
            obj_to_char(map, ch);

            ch->pcdata->learned[get_weapon_sn(ch)] = NewbieWeaponSkillPct;

            char_to_room(ch, get_room(Rooms::MudschoolEntrance));
            ch->send_line("");
            do_help(ch, "NEWBIE INFO");
            ch->send_line("");

            /* hack to let the newbie know about the tipwizard */
            ch->send_line("|WTip: this is Xania's tip wizard! Type 'tips' to turn this on or off.|w");
            /* turn on the newbie's tips */
            ch->set_extra(CharExtraFlag::TipWizard);

        } else if (ch->in_room) {
            char_to_room(ch, ch->in_room);
        } else if (ch->is_immortal()) {
            char_to_room(ch, get_room(Rooms::Chat));
        } else {
            char_to_room(ch, get_room(Rooms::MidgaardTemple));
        }

        announce(fmt::format("|W### |P{}|W has entered the game.|w", ch->name), ch);
        act("|P$n|W has entered the game.", ch);
        look_auto(ch);

        /* Rohan: code to increase the player count if needed - it was only
           updated if a player did count */
        max_on = std::max(static_cast<size_t>(ranges::distance(descriptors().all())), max_on);

        if (ch->gold > GoldTaxThreshold && ch->is_mortal()) {
            ch->send_line("You are taxed {} gold to pay for the Mayor's bar.", (ch->gold - 250000) / 2);
            ch->gold -= (ch->gold - GoldTaxThreshold) / 2;
        }

        if (ch->pet) {
            char_to_room(ch->pet, ch->in_room);
            act("|P$n|W has entered the game.", ch->pet);
        }
        if (const auto num_unread = NoteHandler::singleton().num_unread(*ch); num_unread > 0) {
            ch->send_line("\n\rYou have {} new note{} waiting.", num_unread, (num_unread == 1) ? "" : "s");
        }
        break;
    }
}

bool validate_player_name(std::string_view name) {
    if (is_name(name, "all auto immortal self someone something the you"))
        return false;
    if (matches(name, "DEATH"))
        return true;
    if (name.length() < 2 || name.length() > 12)
        return false;
    // Block non-alpha, and anyone who's creating a char with a name that only contains
    // 'i' or 'l' (unclear why, but it may have been a spamming incident).
    bool only_il = true;
    for (const char c : name) {
        // Doorman shouldn't have sent a null char in the player name, even as a null terminator in PACKET_RECONNECT,
        // but drop out as a precaution.
        if (!c)
            break;
        if (!isalpha(c))
            return false;
        if (tolower(c) != 'i' && tolower(c) != 'l')
            only_il = false;
    }
    if (only_il)
        return false;
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
                log_new(fmt::format("{}@{} reconnected.", ch->name, d->host()), CharExtraFlag::WiznetDebug,
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

namespace {

std::string_view he_she(const Char *ch) { return subjective(*ch); }
std::string_view him_her(const Char *ch) { return objective(*ch); }
std::string_view his_her(const Char *ch) { return possessive(*ch); }
std::string_view himself_herself(const Char *ch) { return reflexive(*ch); }

std::string format_act(std::string_view format, const Char *ch, Act1Arg arg1, Act2Arg arg2, const Char *to,
                       const Char *targ_ch) {
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

        if (std::holds_alternative<std::nullptr_t>(arg2) && c >= 'A' && c <= 'Z') {
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
        case 'n': {
            buf += to->describe(*ch);
        } break;
        case 'N': {
            buf += to->describe(*targ_ch);
        } break;
        case 'e': buf += he_she(ch); break;
        case 'E': buf += he_she(targ_ch); break;
        case 'm': buf += him_her(ch); break;
        case 'M': buf += him_her(targ_ch); break;
        case 's': buf += his_her(ch); break;
        case 'S': buf += his_her(targ_ch); break;
        case 'r': buf += himself_herself(ch); break;
        case 'R': buf += himself_herself(targ_ch); break;

        case 'p':
            if (auto arg1_as_obj_ptr = std::get_if<const Object *>(&arg1)) {
                auto &obj1 = *arg1_as_obj_ptr;
                buf += to->can_see(*obj1) ? obj1->short_descr : "something";
            } else {
                bug("$p passed but arg1 was not an object in '{}'", format);
                buf += "something";
            }
            break;

        case 'P':
            if (auto arg2_as_obj_ptr = std::get_if<const Object *>(&arg2)) {
                auto &obj2 = *arg2_as_obj_ptr;
                buf += to->can_see(*obj2) ? obj2->short_descr : "something";
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
    // Ignore folks below minimum position.
    // Allow NPCs so that act-driven triggers can fire.
    return person->position >= min_position;
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

std::vector<const Char *> collect_folks(const Char *ch, const Char *targ_ch, Act2Arg arg2, To type,
                                        const Position::Type min_position) {
    const Room *room{};

    switch (type) {
    case To::Char:
        if (act_to_person(ch, min_position))
            return {ch};
        else
            return {};

    case To::Vict:
        if (!targ_ch) {
            bug("Act: null or incorrect type of target ch");
            return {};
        }
        if (!targ_ch->in_room || ch == targ_ch || !act_to_person(targ_ch, min_position))
            return {};

        return {targ_ch};

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

    auto result = folks_in_room(room, ch, targ_ch, type, min_position);

    // If we're sending messages to the challenge arena...
    if (room->vnum == Rooms::ChallengeArena) {
        // also include all the folks in the viewing gallery with the appropriate position. We assume the victim
        // is not somehow in the viewing gallery.
        auto viewing = folks_in_room(get_room(Rooms::ChallengeGallery), ch, targ_ch, type, min_position);
        result.insert(result.end(), viewing.begin(), viewing.end());
    }

    return result;
}

}

void act(std::string_view format, const Char *ch, Act1Arg arg1, Act2Arg arg2, const To type, const MobTrig mob_trig,
         const Position::Type min_position) {
    if (format.empty() || !ch || !ch->in_room)
        return;

    const auto arg1_as_obj_ptr =
        std::holds_alternative<const Object *>(arg1) ? std::get<const Object *>(arg1) : nullptr;
    const auto *targ_ch = std::holds_alternative<const Char *>(arg2) ? std::get<const Char *>(arg2) : nullptr;
    const auto *targ_obj = std::holds_alternative<const Object *>(arg2) ? std::get<const Object *>(arg2) : nullptr;
    const auto mprog_target = MProg::to_target(targ_ch, targ_obj);

    for (auto *to : collect_folks(ch, targ_ch, arg2, type, min_position)) {
        auto formatted = format_act(format, ch, arg1, arg2, to, targ_ch);
        to->send_to(formatted);
        if (mob_trig == MobTrig::Yes) {
            // TODO: heinous const_cast here. Safe, but annoying and worth unpicking deeper down.
            MProg::act_trigger(formatted, const_cast<Char *>(to), ch, arg1_as_obj_ptr, mprog_target);
        }
    }
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
            return ch.in_room->area->short_name();
        break;
    case 'n': return "\n\r";
    case 't': {
        auto ch_timet = Clock::to_time_t(current_time);
        char time_buf[MAX_STRING_LENGTH];
        strftime(time_buf, sizeof(time_buf), "%H:%M:%S", gmtime(&ch_timet));
        return time_buf;
    }
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
