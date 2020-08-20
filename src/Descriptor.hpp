#pragma once

#include "common/Time.hpp"

#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

struct CHAR_DATA;

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
    GenGroups = 11,
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
    std::list<std::string> pending_commands_;
    std::string last_command_;
    std::string raw_host_{"unknown"};
    std::string masked_host_{"unknown"};
    Time login_time_;
    std::string outbuf_;
    std::list<std::string> page_outbuf_;
    std::unordered_set<Descriptor *> snoop_by_;
    std::unordered_set<Descriptor *> snooping_;
    uint32_t netaddr_{};
    uint16_t port_{};
    bool processing_command_{};
    DescriptorState state_{DescriptorState::GetName};
    CHAR_DATA *character_{};
    CHAR_DATA *original_{};

    [[nodiscard]] std::optional<std::string> pop_raw();

public:
    explicit Descriptor(uint32_t descriptor);
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
    void clear_input() { pending_commands_.clear(); }
    void add_command(std::string_view command) { pending_commands_.emplace_back(command); }
    [[nodiscard]] std::optional<std::string> pop_incomm();

    // Writes directly to a socket. May fail
    bool write_direct(std::string_view text) const;

    static constexpr auto MaxOutputBufSize = 32000u;
    // Buffers output to be processed later. If the max output size is exceeded, the descriptor is closed.
    void write(std::string_view message) noexcept;
    [[nodiscard]] bool has_buffered_output() const noexcept { return !outbuf_.empty(); }
    void clear_output_buffer() noexcept { outbuf_.clear(); }

    // Send a page of text to a descriptor. Sending a page replaces any previous page.
    void page_to(std::string_view page) noexcept;
    [[nodiscard]] bool is_paging() const noexcept { return !page_outbuf_.empty(); }
    void show_next_page(std::string_view input) noexcept;

    // deliberately named long and annoying to dissuade use! Go use host to get the safe version.
    [[nodiscard]] const std::string &raw_full_hostname() const noexcept { return raw_host_; }
    void set_endpoint(uint32_t netaddr, uint16_t port, std::string_view raw_full_hostname);

    [[nodiscard]] const std::string &host() const noexcept { return masked_host_; }
    [[nodiscard]] std::string login_time() const noexcept;

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

    [[nodiscard]] CHAR_DATA *character() const noexcept { return character_; }
    void character(CHAR_DATA *character) noexcept { character_ = character; }
    [[nodiscard]] CHAR_DATA *original() const noexcept { return original_; }
    // do_ prefix because switch is a C keyword
    void do_switch(CHAR_DATA *victim);
    // do_ prefix because return is a C keyword
    void do_return();
    // Return the real-life "person" player character behind this descriptor.
    [[nodiscard]] CHAR_DATA *person() const noexcept { return original_ ? original_ : character_; }
    [[nodiscard]] bool is_switched() const noexcept { return original_ != nullptr; }
};
