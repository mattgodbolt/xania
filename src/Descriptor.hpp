#pragma once

#include "Mud.hpp"
#include "common/Time.hpp"

#include <cstdint>
#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

struct Char;

// Connected state for a descriptor.
enum class DescriptorState {
    Playing = 0,
    GetName = 1,
    GetOldPassword = 2,
    ConfirmNewName = 3,
    GetNewPassword = 4,
    ConfirmNewPassword = 5,
    GetNewRace = 6,
    GetNewSex = 7,
    GetNewClass = 8,
    GetAlignment = 9,
    DefaultChoice = 10,
    Customize = 11,
    ReadIMotd = 13,
    ReadMotd = 14,
    BreakConnect = 15,
    GetAnsi = 16,
    CircumventPassword = 18, // used by doorman
    Disconnecting = 254, // disconnecting having been playing
    DisconnectingNp = 255, // disconnecting before playing
    Closed = 256 // closed
};

const char *short_name_of(DescriptorState state);
/*
 * Descriptor (channel) structure.
 */
class Descriptor {
    static constexpr size_t MaxInbufBacklog = 50u;
    uint32_t channel_{};
    const Mud &mud_;
    std::list<std::string> pending_commands_;
    bool is_spammer_warned_{};
    std::string last_command_;
    std::string raw_host_{"unknown"};
    std::string masked_host_{"unknown"};
    std::string outbuf_;
    std::list<std::string> page_outbuf_;
    std::unordered_set<Descriptor *> snoop_by_;
    std::unordered_set<Descriptor *> snooping_;
    uint32_t netaddr_{};
    uint16_t port_{};
    bool ansi_terminal_detected_{};
    bool processing_command_{};
    DescriptorState state_{DescriptorState::GetName};
    // While a Char is in the lobby phase, Descriptor owns the Char, and that's held here.
    // Once the Char enters DescriptorState::Playing,
    // the Char enters the mud world and ownership is handed off. See nanny() and proceed_from_lobby().
    std::unique_ptr<Char> guest_;
    Char *character_{};
    Char *original_{};
    Time login_time_;

    [[nodiscard]] std::optional<std::string> pop_raw();

public:
    explicit Descriptor(uint32_t descriptor, Mud &mud);
    ~Descriptor();

    // Descriptors are referenced everywhere; prevent accidental copying or moving that would invalidate others'
    // references.
    Descriptor(const Descriptor &) = delete;
    Descriptor &operator=(const Descriptor &) = delete;
    Descriptor(Descriptor &&) = delete;
    Descriptor &operator=(Descriptor &&) = delete;

    void state(DescriptorState state) noexcept { state_ = state; }
    [[nodiscard]] DescriptorState state() const noexcept { return state_; }
    [[nodiscard]] bool is_playing() const noexcept { return state_ == DescriptorState::Playing; }

    [[nodiscard]] bool is_input_full() const noexcept { return pending_commands_.size() >= MaxInbufBacklog; }
    [[nodiscard]] bool is_spammer_warned() const noexcept { return is_spammer_warned_; }
    void warn_spammer();
    [[nodiscard]] bool is_in_lobby() const noexcept;
    // Returns true if the descriptor has been connected in the lobby (user/pass prompt) too long.
    [[nodiscard]] bool is_lobby_timeout_exceeded() const noexcept;
    [[nodiscard]] bool ansi_terminal_detected() const noexcept { return ansi_terminal_detected_; }
    void ansi_terminal_detected(const bool detected) { ansi_terminal_detected_ = detected; }
    void clear_input() { pending_commands_.clear(); }
    void add_command(std::string_view command) { pending_commands_.emplace_back(command); }
    [[nodiscard]] std::optional<std::string> pop_incomm();

    // Writes directly to a socket. May fail
    bool write_direct(std::string_view text) const;

    static constexpr auto MaxOutputBufSize = 32000u;
    // Buffers output to be processed later. If the max output size is exceeded, the descriptor is closed.
    void write(std::string_view message) noexcept;
    [[nodiscard]] bool has_buffered_output() const noexcept { return !outbuf_.empty(); }
    [[nodiscard]] const std::string &buffered_output() const noexcept { return outbuf_; }
    void clear_output_buffer() noexcept { outbuf_.clear(); }

    // Send a page of text to a descriptor. Sending a page replaces any previous page.
    void page_to(std::string_view page) noexcept;
    [[nodiscard]] bool is_paging() const noexcept { return !page_outbuf_.empty(); }
    void show_next_page(std::string_view input) noexcept;

    // deliberately named long and annoying to dissuade use! Go use host to get the safe version.
    [[nodiscard]] const std::string &raw_full_hostname() const noexcept { return raw_host_; }
    void set_endpoint(uint32_t netaddr, uint16_t port, std::string_view raw_full_hostname);

    [[nodiscard]] const std::string &host() const noexcept { return masked_host_; }
    void host(std::string_view host) { masked_host_ = host; }
    [[nodiscard]] std::string login_time() const noexcept;
    void login_time(Time login_time) { login_time_ = login_time; }

    [[nodiscard]] bool flush_output() noexcept;

    // Fails for reasons of snoop loops.
    [[nodiscard]] bool try_start_snooping(Descriptor &other);
    void stop_snooping(Descriptor &other);
    void stop_snooping();

    [[nodiscard]] bool is_closed() const noexcept { return state_ == DescriptorState::Closed; }
    void close() noexcept;

    [[nodiscard]] uint32_t channel() const noexcept { return channel_; }
    void note_input(std::string_view char_name, std::string_view input);

    void processing_command(bool is_processing) noexcept { processing_command_ = is_processing; }
    [[nodiscard]] bool processing_command() const noexcept { return processing_command_; }

    // Take ownership of a Char that has connected and is going through the lobby phase, i.e. not yet in a playing
    // state.
    void enter_lobby(std::unique_ptr<Char> guest) noexcept;

    // Put the Descriptor back to the start of the lobby phase and release any previous Char it was associated with.
    void restart_lobby() noexcept;

    // During the lobby phase, if the Char is reconnecting and they have a Char in the world already,
    // reconnect that Char with this Descriptor and set them Playing.
    void reconnect_from_lobby(Char *existing_char) noexcept;

    // Advances the Descriptor from the lobby into Playing state, rescinding ownership of a Char.
    std::unique_ptr<Char> proceed_from_lobby() noexcept;

    [[nodiscard]] Char *character() const noexcept { return character_; }
    void character(Char *character) noexcept { character_ = character; }
    [[nodiscard]] Char *original() const noexcept { return original_; }

    // do_ prefix because switch is a C keyword
    void do_switch(Char *victim);
    // do_ prefix because return is a C keyword
    void do_return();
    // Return the real-life "person" player character behind this descriptor.
    [[nodiscard]] Char *person() const noexcept { return original_ ? original_ : character_; }
    [[nodiscard]] bool is_switched() const noexcept { return original_ != nullptr; }
};
