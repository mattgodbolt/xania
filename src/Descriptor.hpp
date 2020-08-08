#pragma once

#include "merc.h"

#include <list>
#include <optional>
#include <string>
#include <string_view>

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
};

const char *short_name_of(DescriptorState state);

/*
 * Descriptor (channel) structure.
 */
class Descriptor {
    static constexpr size_t MaxInbufBacklog = 50u;
    std::list<std::string> pending_commands_;
    std::string last_command_;
    std::string raw_host_{"unknown"};
    std::string masked_host_{"unknown"};
    std::string login_time_;
    std::string outbuf_;
    std::list<std::string> page_outbuf_;

    [[nodiscard]] std::optional<std::string> pop_raw();

public:
    Descriptor *next{};
    Descriptor *snoop_by{};
    CHAR_DATA *character{};
    CHAR_DATA *original{};
    uint32_t descriptor{};
    uint32_t netaddr{};
    DescriptorState connected{DescriptorState::GetName};
    uint16_t localport{};
    bool fcommand{};

    explicit Descriptor(uint32_t descriptor);

    [[nodiscard]] bool is_playing() const noexcept { return connected == DescriptorState::Playing; }
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
    void raw_full_hostname(std::string_view raw_full_hostname);

    [[nodiscard]] const std::string &host() const noexcept { return masked_host_; }
    [[nodiscard]] const std::string &login_time() const noexcept { return login_time_; }

    [[nodiscard]] bool flush_output() noexcept;
};
