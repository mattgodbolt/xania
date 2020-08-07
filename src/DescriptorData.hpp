#pragma once

#include "merc.h"

// Connected state for a descriptor.
#define CON_PLAYING 0
#define CON_GET_NAME 1
#define CON_GET_OLD_PASSWORD 2
#define CON_CONFIRM_NEW_NAME 3
#define CON_GET_NEW_PASSWORD 4
#define CON_CONFIRM_NEW_PASSWORD 5
#define CON_GET_NEW_RACE 6
#define CON_GET_NEW_SEX 7
#define CON_GET_NEW_CLASS 8
#define CON_GET_ALIGNMENT 9
#define CON_DEFAULT_CHOICE 10
#define CON_GEN_GROUPS 11
#define CON_PICK_WEAPON 12
#define CON_READ_IMOTD 13
#define CON_READ_MOTD 14
#define CON_BREAK_CONNECT 15
#define CON_GET_ANSI 16
#define CON_CIRCUMVENT_PASSWORD 18 // used by doorman
#define CON_DISCONNECTING 254 // disconnecting having been playing
#define CON_DISCONNECTING_NP 255 // disconnecting before playing

/*
 * Descriptor (channel) structure.
 */
struct Descriptor {
    Descriptor *next;
    Descriptor *snoop_by;
    CHAR_DATA *character;
    CHAR_DATA *original;
    char *host;
    char *logintime;
    uint32_t descriptor;
    int netaddr;
    sh_int connected;
    sh_int localport;
    bool fcommand;
    char inbuf[4 * MAX_INPUT_LENGTH];
    char incomm[MAX_INPUT_LENGTH];
    char inlast[MAX_INPUT_LENGTH];
    int repeat;
    char *outbuf;
    int outsize;
    int outtop;
    char *showstr_head;
    char *showstr_point;

    [[nodiscard]] bool is_playing() const noexcept { return connected == CON_PLAYING; }
};
