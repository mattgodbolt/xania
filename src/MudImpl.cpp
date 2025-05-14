/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2025 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#include "MudImpl.hpp"
#include "Act.hpp"
#include "Area.hpp"
#include "AreaList.hpp"
#include "Ban.hpp"
#include "CommFlag.hpp"
#include "Help.hpp"
#include "Logging.hpp"
#include "Note.hpp"
#include "Object.hpp"
#include "ObjectIndex.hpp"
#include "PlayerActFlag.hpp"
#include "PromptFormat.hpp"
#include "Races.hpp"
#include "SkillNumbers.hpp"
#include "Titles.hpp"
#include "VnumObjects.hpp"
#include "VnumRooms.hpp"
#include "common/BitOps.hpp"
#include "common/Configuration.hpp"
#include "common/Fd.hpp"
#include "common/doorman_protocol.h"
#include "db.h" // TODO remove this
#include "save.hpp"
#include "skills.hpp"
#include "string_utils.hpp"

#include <range/v3/algorithm/any_of.hpp>
#include <range/v3/algorithm/find_if.hpp>

#include <csignal>
#include <fstream>
#include <netinet/in.h>
#include <sys/signalfd.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <thread>

// TODO: move these?
extern void move_active_char_from_limbo(Char *ch);
extern void update_handler(Mud &mud);
extern int race_lookup(std::string_view name);
extern ObjectIndex *get_obj_index(int vnum, const Logger &logger);
extern void obj_to_char(Object *obj, Char *ch);
extern int get_weapon_sn(Char *ch);
extern Room *get_room(int vnum, const Logger &logger);
extern void char_to_room(Char *ch, Room *room);
extern void announce(std::string_view buf, const Char *ch);
extern void look_auto(Char *ch);
extern std::string describe_fight_condition(const Char &victim);

// TODO Move into a Universe class
extern std::vector<std::unique_ptr<Char>> char_list;
extern std::vector<std::unique_ptr<Object>> object_list;

namespace {

const auto GoldTaxThreshold = 250000u;
const auto NewbieWeaponSkillPct = 40u;
const auto NewbieNumTrains = 3u;
const auto NewbieNumPracs = 5u;

}

MudImpl::MudImpl()
    : config_{std::make_unique<Configuration>()}, logger_{std::make_unique<Logger>(descriptors_)},
      interpreter_{std::make_unique<Interpreter>()}, bans_{std::make_unique<Bans>(config_->ban_file())},
      areas_{std::make_unique<AreaList>()}, help_{std::make_unique<HelpList>()}, main_loop_running_(true),
      wizlock_(false), newlock_(false), control_fd_{std::nullopt}, boot_time_{std::chrono::system_clock::now()},
      current_time_{std::chrono::system_clock::now()}, current_tick_{TimeInfoData(Clock::now())},
      max_players_today_(0) {}

Configuration &MudImpl::config() const { return *config_; }

DescriptorList &MudImpl::descriptors() { return descriptors_; }

Logger &MudImpl::logger() const { return *logger_; }

Interpreter &MudImpl::interpreter() const { return *interpreter_; }

Bans &MudImpl::bans() const { return *bans_; }

AreaList &MudImpl::areas() const { return *areas_; }

HelpList &MudImpl::help() const { return *help_; }

/* Send a packet to doorman */
bool MudImpl::send_to_doorman(const Packet *p, const void *extra) const {
    // TODO: do something rather than return here if there's a failure
    if (!doorman_fd_.is_open())
        return false;
    if (p->nExtra > PacketMaxPayloadSize) {
        logger_->bug("MUD tried to send a doorman packet with payload size {} > {}! Dropping!", p->nExtra,
                     PacketMaxPayloadSize);
        return false;
    }
    try {
        if (p->nExtra)
            doorman_fd_.write_many(*p, gsl::span<const byte>(static_cast<const byte *>(extra), p->nExtra));
        else
            doorman_fd_.write(*p);
    } catch (const std::runtime_error &re) {
        logger_->bug("Unable to write to doorman: {}", re.what());
        return false;
    }
    return true;
}

TimeInfoData &MudImpl::current_tick() { return current_tick_; }

Time MudImpl::boot_time() const { return boot_time_; }

Time MudImpl::current_time() const { return current_time_; }

void MudImpl::terminate(const bool close_sockets) {
    // This cause the main loop to end on next iteration.
    main_loop_running_ = false;
    if (close_sockets) {
        for (auto &d : descriptors().all())
            d.close();
    }
}

bool MudImpl::toggle_wizlock() {
    wizlock_ = !wizlock_;
    return wizlock_;
}

bool MudImpl::toggle_newlock() {
    newlock_ = !newlock_;
    return newlock_;
}

size_t MudImpl::max_players_today() const { return max_players_today_; }

void MudImpl::reset_max_players_today() { max_players_today_ = 0; }

void MudImpl::send_tips() { tips_.send_tips(descriptors_); }

void MudImpl::init_socket() {

    pipe_file_ = fmt::format(PIPE_FILE, config_->port(), getenv("USER") ? getenv("USER") : "unknown");
    unlink(pipe_file_.c_str());
    control_fd_.emplace(Fd::socket(PF_UNIX, SOCK_STREAM, 0));
    sockaddr_un xaniaAddr{};
    strncpy(xaniaAddr.sun_path, pipe_file_.c_str(), sizeof(xaniaAddr.sun_path) - 1);
    xaniaAddr.sun_family = PF_UNIX;
    int enabled = 1;
    control_fd_->setsockopt(SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled)).bind(xaniaAddr).listen(3);

    logger_->log_string("Waiting for doorman...");
    try {
        sockaddr_un dont_care{};
        socklen_t dont_care_len{sizeof(sockaddr_in)};
        doorman_fd_ = control_fd_->accept(reinterpret_cast<sockaddr *>(&dont_care), &dont_care_len);
        logger_->log_string("Doorman has connected - proceeding with boot-up");
    } catch (const std::system_error &e) {
        logger_->log_string("Unable to accept doorman...booting anyway");
    }
}

void MudImpl::boot() {
    if (const auto tip_count = tips_.load(config_->tip_file()); tip_count < 0)
        logger_->log_string("Unable to load tips from file {}", config_->tip_file());
    else
        logger_->log_string("Loaded {} tips from file {}", tip_count, config_->tip_file());
    const auto ban_count = bans_->load(*logger_);
    logger_->log_string("{} site bans loaded.", ban_count);
}

void MudImpl::game_loop() {
    static timeval null_time;
    if (is_being_debugged())
        logger_->log_string("Running under a debugger");
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
    if (!control_fd_) {
        logger_->log_string("Doorman socket not initialized, exiting!");
        return;
    }
    doorman_init();
    logger_->log_string("Xania is ready to rock via {}.", pipe_file_);

    /* Main loop */
    while (main_loop_running_) {
        current_time_ = Clock::now();

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
        FD_SET(control_fd_->number(), &in_set);
        FD_SET(signal_fd, &in_set);
        if (doorman_fd_.is_open()) {
            FD_SET(doorman_fd_.number(), &in_set);
            maxdesc = std::max(control_fd_->number(), doorman_fd_.number());
        } else
            maxdesc = control_fd_->number();
        maxdesc = std::max(maxdesc, signal_fd);

        if (select(maxdesc + 1, &in_set, &out_set, &exc_set, &null_time) < 0) {
            perror("Game_loop: select: poll");
            exit(1);
        }

        if (FD_ISSET(signal_fd, &in_set)) {
            struct signalfd_siginfo info;
            if (read(signal_fd, &info, sizeof(info)) != sizeof(info)) {
                logger_->bug("Unable to read signal info - treating as term");
                info.ssi_signo = SIGTERM;
            }
            if (info.ssi_signo == SIGINT) {
                if (is_being_debugged()) {
                    logger_->log_string("Caught SIGINT but detected a debugger, trapping instead");
                    raise(SIGTRAP);
                } else {
                    handle_signal_shutdown();
                    return;
                }
            } else if (info.ssi_signo == SIGTERM) {
                handle_signal_shutdown();
                return;
            } else {
                logger_->bug("Unexpected signal {}: ignoring", info.ssi_signo);
            }
        }

        /* If we don't have a connection from doorman, wait for one */
        if (!doorman_fd_.is_open()) {
            logger_->log_string("Waiting for doorman...");

            try {
                sockaddr_un dont_care{};
                socklen_t dont_care_len{sizeof(sockaddr_in)};
                doorman_fd_ = control_fd_->accept(reinterpret_cast<sockaddr *>(&dont_care), &dont_care_len);
                logger_->log_string("Doorman has connected.");
                doorman_init();
            } catch (const std::runtime_error &re) {
                logger_->bug("Unable to accept doorman connection: {}", re.what());
            }
        }

        /* Process doorman input, if any */
        if (doorman_fd_.is_open() && FD_ISSET(doorman_fd_.number(), &in_set)) {
            Packet p{};
            std::string payload;
            do {
                try {
                    p = doorman_fd_.read_all<Packet>();
                    if (p.nExtra) {
                        if (p.nExtra > PacketMaxPayloadSize) {
                            logger_->bug("Doorman sent a too big packet! {} > {}: dropping", p.nExtra,
                                         PacketMaxPayloadSize);
                            doorman_lost();
                            break;
                        }
                        payload.resize(p.nExtra);
                        doorman_fd_.read_all(gsl::span<char>(payload));
                    }
                } catch (const std::runtime_error &re) {
                    logger_->bug("Unable to read doorman packet: {}", re.what());
                    doorman_lost();
                    break;
                }
                handle_doorman_packet(p, payload);
                /* Reselect to see if there is more data */
                select(maxdesc + 1, &in_set, &out_set, &exc_set, &null_time);
            } while FD_ISSET(doorman_fd_.number(), &in_set);
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
                    interpreter_->interpret(character, *incomm);
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
        update_handler(*this);

        /*
         * Output.
         */
        if (doorman_fd_.is_open()) {
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

        // Synchronize to a clock.
        using namespace std::chrono;
        using namespace std::literals;
        constexpr auto nanos_per_pulse = duration_cast<nanoseconds>(1s) / PULSE_PER_SECOND;
        auto next_pulse = current_time_ + nanos_per_pulse;
        std::this_thread::sleep_until(next_pulse);
    }
}

void MudImpl::set_echo_state(Descriptor *d, int on) {
    Packet p;
    p.type = on ? PACKET_ECHO_ON : PACKET_ECHO_OFF;
    p.channel = d->channel();
    p.nExtra = 0;
    send_to_doorman(&p, nullptr);
}

bool MudImpl::send_go_ahead(Descriptor *d) {
    Packet p;
    p.type = PACKET_GO_AHEAD;
    p.channel = d->channel();
    p.nExtra = 0;
    return send_to_doorman(&p, nullptr);
}

void MudImpl::greet(Descriptor &d) {
    const auto *greeting = help_->lookup(0, "greeting");
    if (!greeting) {
        logger_->bug("Unable to look up greeting");
        return;
    }
    d.write(greeting->text());
}

bool MudImpl::is_being_debugged() {
    std::ifstream t("/proc/self/status");
    std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    for (auto line : line_iter(str)) {
        const auto match = "TracerPid:"sv;
        if (matches_start(match, line))
            return parse_number(ltrim(line.substr(match.size()))) != 0;
    }

    return false;
}

void MudImpl::lobby_maybe_enable_colour(Descriptor *desc) {
    if (desc->ansi_terminal_detected()) {
        desc->character()->pcdata->colour = true;
    }
}

void MudImpl::lobby_prompt_for_race(Descriptor *desc) {
    do_help(desc->character(), "selectrace");
    desc->state(DescriptorState::GetNewRace);
}

void MudImpl::lobby_prompt_for_class(Descriptor *desc) {
    do_help(desc->character(), "selectclass");
    desc->state(DescriptorState::GetNewClass);
}

// If Doorman didn't detect an ansi terminal ask the user.
void MudImpl::lobby_prompt_for_ansi(Descriptor *desc) {
    desc->write("Does your terminal support ANSI colour (Y/N/Return = as saved)?");
    desc->state(DescriptorState::GetAnsi);
}

void MudImpl::lobby_show_motd(Descriptor *desc) {
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
bool MudImpl::lobby_char_was_created(Descriptor *desc) {
    struct stat player_file;
    const auto player_dir = config_->player_dir();
    if (!stat(filename_for_player(desc->character()->name.c_str(), player_dir).c_str(), &player_file)) {
        desc->write("Your character could not be created, please try again.\n\r");
        desc->close();
        return false;
    } else {
        save_char_obj(desc->character());
        return true;
    }
}

/* where we're asked nicely to quit from the outside */
void MudImpl::handle_signal_shutdown() {
    logger_->log_string("Signal shutdown received");

    /* ask everyone to save! */
    for (auto &&uch : char_list) {
        // TODO remove global
        auto *vch = uch.get();
        // vch->d->c check added by TM to avoid crashes when
        // someone hasn't logged in but the mud is shut down
        if (vch->is_pc() && vch->desc && vch->desc->is_playing()) {
            do_save(vch);
            vch->send_line("|RXania has been asked to shutdown by the operating system.|w");
            if (vch->desc && vch->desc->has_buffered_output())
                process_output(vch->desc, false);
        }
    }
    terminate(true);
}

void MudImpl::doorman_init() {
    Packet pInit{.type{PACKET_INIT}, .nExtra{0}, .channel{0}, .data{}};
    send_to_doorman(&pInit, nullptr);
}

void MudImpl::doorman_lost() {
    logger_->log_string("Lost connection to doorman");
    doorman_fd_.close();
    /* Now to go through and disconnect all the characters */
    for (auto &d : descriptors().all())
        d.close();
}

void MudImpl::handle_doorman_packet(const Packet &p, std::string_view buffer) {
    switch (p.type) {
    case PACKET_CONNECT:
        logger_->log_string("Incoming connection on channel {}.", p.channel);
        if (auto *d = descriptors().create(p.channel, *this)) {
            greet(*d);
        } else {
            logger_->log_string("Duplicate channel {} on connect!", p.channel);
        }
        break;
    case PACKET_RECONNECT:
        logger_->log_string("Incoming reconnection on channel {} for {}.", p.channel, buffer);
        if (auto *d = descriptors().create(p.channel, *this)) {
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
            logger_->log_string("Duplicate channel {} on reconnect!", p.channel);
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
            logger_->log_string("Unable to find channel to close ({})", p.channel);
        }
        break;
    case PACKET_INFO:
        if (auto *d = descriptors().find_by_channel(p.channel)) {
            auto *data = reinterpret_cast<const InfoData *>(buffer.data());
            d->set_endpoint(data->netaddr, data->port, data->data);
            d->ansi_terminal_detected(data->ansi);
        } else {
            logger_->log_string("Unable to associate info with a descriptor ({})", p.channel);
        }
        break;

    case PACKET_MESSAGE:
        if (auto *d = descriptors().find_by_channel(p.channel)) {
            if (d->character() != nullptr)
                d->character()->idle_timer_ticks = 0;
            read_from_descriptor(d, buffer);
        } else {
            logger_->log_string("Unable to associate message with a descriptor ({})", p.channel);
        }
        break;
    default: break;
    }
}

bool MudImpl::read_from_descriptor(Descriptor *d, std::string_view data) {
    if (d->is_input_full()) {
        // Only log input for the first command exceeding the input limit
        // so that wiznet doesn't become unusable.
        if (!d->is_spammer_warned()) {
            logger_->log_string("{} input overflow!", d->host());
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
bool MudImpl::process_output(Descriptor *d, bool fPrompt) {
    if (main_loop_running_ && d->is_paging()) {
        d->write("[Hit Return to continue]\n\r");
        return d->flush_output();
    } else if (fPrompt && main_loop_running_ && d->is_playing()) {
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
void MudImpl::nanny(Descriptor *d, std::string_view argument) {
    argument = ltrim(argument);
    auto *ch = d->character();
    switch (d->state()) {
    default:
        logger_->bug("Nanny: bad d->state() {}.", static_cast<int>(d->state()));
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
        auto res = try_load_player(*this, player_name);
        ch = res.character.get();
        d->enter_lobby(std::move(res.character));
        auto existing_player = !res.newly_created;

        if (check_enum_bit(ch->act, PlayerActFlag::PlrDeny)) {
            logger_->log_string("Denying access to {}@{}.", player_name, d->host());
            d->write("You are denied access.\n\r");
            d->close();
            return;
        }
        if (wizlock_ && ch->is_mortal()) {
            d->write("The game is wizlocked.  Try again later - a reboot may be imminent.\n\r");
            d->close();
            return;
        }
        if (existing_player) {
            d->write("Password: ");
            set_echo_state(d, 0);
            d->state(DescriptorState::GetOldPassword);
            return;
        } else {
            if (newlock_) {
                d->write("The game is newlocked.  Try again later - a reboot may be imminent.\n\r");
                d->close();
                return;
            }
            if (bans_->check_ban(d->raw_full_hostname(), to_int(BanFlag::Newbies) | to_int(BanFlag::All))) {
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

        if ((!ch->is_set_extra(CharExtraFlag::Permit) && (bans_->check_ban(d->raw_full_hostname(), BanFlag::Permit)))
            || bans_->check_ban(d->raw_full_hostname(), BanFlag::All)) {
            d->write("Your site has been banned.  Sorry.\n\r");
            d->close();
            return;
        }

        if (check_playing(d, ch->name))
            return;

        if (check_reconnect(d))
            return;

        logger_->log_new(fmt::format("{}@{} has connected.", ch->name, d->host()), CharExtraFlag::WiznetDebug,
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
            if (check_reconnect(d))
                return;
            d->write("Reconnect attempt failed.\n\rName: ");
            d->restart_lobby();
            break;

        case 'n':
        case 'N':
            d->write("Name: ");
            d->restart_lobby();
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
            d->restart_lobby();
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
        if (argument.length() < MIN_PASSWORD_LEN) {
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
            if (pc_race_table[race].skills[i].empty())
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
        if (const auto *chosen_class = Class::by_name(choice); chosen_class) {
            ch->pcdata->class_type = chosen_class;
            logger_->log_string("{}@{} new player.", ch->name, d->host());
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
        group_add(ch, ch->pcdata->class_type->base_skill_group, false);
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
            group_add(ch, ch->pcdata->class_type->default_skill_group, true);
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
        // The lobby phase is complete. The "world" takes ownership of the Char. Being on this list and in the playing
        // state means the Char exists in the world and is processed by the main update loop.
        char_list.emplace_back(d->proceed_from_lobby());
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
                ch->perm_stat[ch->pcdata->class_type->primary_stat] += 3;

            ch->level = 1;
            ch->exp = exp_per_level(ch, ch->pcdata->points);
            ch->hit = ch->max_hit;
            ch->mana = ch->max_mana;
            ch->move = ch->max_move;
            ch->train = NewbieNumTrains;
            ch->practice = NewbieNumPracs;
            ch->set_title(fmt::format("the {}", Titles::default_title(*ch)));

            do_outfit(ch);
            auto map_uptr = get_obj_index(Objects::Map, *logger_)->create_object(*logger_);
            auto *map = map_uptr.get();
            object_list.push_back(std::move(map_uptr));
            obj_to_char(map, ch);

            ch->pcdata->learned[get_weapon_sn(ch)] = NewbieWeaponSkillPct;

            char_to_room(ch, get_room(Rooms::MudschoolEntrance, *logger_));
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
            char_to_room(ch, get_room(Rooms::Chat, *logger_));
        } else {
            char_to_room(ch, get_room(Rooms::MidgaardTemple, *logger_));
        }

        announce(fmt::format("|W### |P{}|W has entered the game.|w", ch->name), ch);
        act("|P$n|W has entered the game.", ch);
        look_auto(ch);

        /* Rohan: code to increase the player count if needed - it was only
           updated if a player did count */
        max_players_today_ = std::max(static_cast<size_t>(ranges::distance(descriptors().all())), max_players_today_);

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

bool MudImpl::validate_player_name(std::string_view name) {
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

Char *MudImpl::find_link_dead_player_by_name(std::string_view name) {
    const auto is_link_dead_with_name = [&name](auto &&ch) -> bool {
        return ch->is_link_dead_pc() && matches(name, ch->name);
    };
    if (auto it = ranges::find_if(char_list, is_link_dead_with_name); it != char_list.end()) {
        return it->get();
    } else {
        return nullptr;
    }
}

/*
 * Look for link-dead player to reconnect.
 */
bool MudImpl::check_reconnect(Descriptor *d) {
    if (Char *ch = find_link_dead_player_by_name(d->character()->name); ch) {
        d->reconnect_from_lobby(ch);
        ch->send_line("Reconnecting.");
        act("$n has reconnected.", ch);
        logger_->log_new(fmt::format("{}@{} reconnected.", ch->name, d->host()), CharExtraFlag::WiznetDebug,
                         (ch->is_wizinvis() || ch->is_prowlinvis()) ? ch->get_trust() : 0);
        return true;
    }
    return false;
}

bool MudImpl::check_playing(Descriptor *d, std::string_view name) {
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