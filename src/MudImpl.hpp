/************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                     */
/*  (C) 2025 Xania Development Team                                     */
/*  See the header to file: merc.h for original code copyrights         */
/************************************************************************/
#pragma once

#include "AreaList.hpp"
#include "DescriptorList.hpp"
#include "Help.hpp"
#include "Interpreter.hpp"
#include "Logging.hpp"
#include "Mud.hpp"
#include "TimeInfoData.hpp"
#include "Tips.hpp"
#include "common/Configuration.hpp"
#include "common/Fd.hpp"
#include "common/doorman_protocol.h"

/**
 * The standard implementation of the Mud interface. Most dependents should include Mud.hpp instead.
 */
class MudImpl : public Mud {
public:
    MudImpl();
    MudImpl(const MudImpl &) = delete;
    MudImpl(MudImpl &&) = delete;
    MudImpl &operator=(const MudImpl &) = delete;
    MudImpl &operator=(MudImpl &&) = delete;
    virtual ~MudImpl() = default;

    Configuration &config() override;
    DescriptorList &descriptors() override;
    Logger &logger() override;
    Interpreter &interpreter() override;
    Bans &bans() override;
    AreaList &areas() override;
    HelpList &help() override;
    bool send_to_doorman(const Packet *p, const void *extra) const override;
    TimeInfoData &current_tick() override;
    Time boot_time() const override;
    Time current_time() const override;
    void terminate(const bool close_sockets) override;
    bool toggle_wizlock() override;
    bool toggle_newlock() override;
    size_t max_players_today() const override;
    void reset_max_players_today() override;
    void send_tips() override;

    void init_socket();
    void boot();
    void game_loop();

private:
    void set_echo_state(Descriptor *d, int on);
    bool send_go_ahead(Descriptor *d);
    void greet(Descriptor &d);
    bool is_being_debugged();
    void lobby_maybe_enable_colour(Descriptor *desc);
    void lobby_prompt_for_race(Descriptor *desc);
    void lobby_prompt_for_class(Descriptor *desc);
    void lobby_prompt_for_ansi(Descriptor *desc);
    void lobby_show_motd(Descriptor *desc);
    bool lobby_char_was_created(Descriptor *desc);
    void handle_signal_shutdown();
    void doorman_init();
    void doorman_lost();
    void handle_doorman_packet(const Packet &p, std::string_view buffer);
    bool read_from_descriptor(Descriptor *d, std::string_view data);
    bool process_output(Descriptor *d, bool fPrompt);
    void nanny(Descriptor *d, std::string_view argument);
    bool validate_player_name(std::string_view name);
    Char *find_link_dead_player_by_name(std::string_view name);
    bool check_reconnect(Descriptor *d);
    bool check_playing(Descriptor *d, std::string_view name);

    Configuration config_;
    DescriptorList descriptors_;
    Logger logger_;
    Interpreter interpreter_;
    Bans bans_;
    AreaList areas_;
    HelpList help_;
    bool main_loop_running_;
    bool wizlock_;
    bool newlock_;
    std::string pipe_file_;
    std::optional<Fd> control_fd_;
    Fd doorman_fd_;
    const Time boot_time_;
    Time current_time_;
    TimeInfoData current_tick_;
    size_t max_players_today_;
    Tips tips_{};
};
