#pragma once

#include "merc.h"

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
const char *name_of(DescriptorState state);

/*
 * Descriptor (channel) structure.
 */
struct Descriptor {
    Descriptor *next{};
    Descriptor *snoop_by{};
    CHAR_DATA *character{};
    CHAR_DATA *original{};
    char *host{};
    char *logintime{};
    uint32_t descriptor{};
    uint32_t netaddr{};
    DescriptorState connected{DescriptorState::GetName};
    uint16_t localport{};
    bool fcommand{};
    char inbuf[4 * MAX_INPUT_LENGTH]{};
    char incomm[MAX_INPUT_LENGTH]{};
    char inlast[MAX_INPUT_LENGTH]{};
    int repeat{};
    char *outbuf{};
    int outsize{2000};
    int outtop{};
    char *showstr_head{};
    char *showstr_point{};

    explicit Descriptor(uint32_t descriptor);
    ~Descriptor();
    [[nodiscard]] bool is_playing() const noexcept { return connected == DescriptorState::Playing; }
};
