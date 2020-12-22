/*************************************************************************/
/*  Xania (M)ulti(U)ser(D)ungeon server source code                      */
/*  (C) 1995-2000 Xania Development Team                                    */
/*  See the header to file: merc.h for original code copyrights          */
/*                                                                       */
/*  doorman.h                                                            */
/*                                                                       */
/*************************************************************************/
/*
 * Doorman.h
 * Included by both doorman and the MUD, and acts as the link between
 * the two
 */

#pragma once

#include <stdint.h>

static inline constexpr auto PIPE_FILE = "/tmp/.xania-{}-{}";

/*
 * A packet of data passing between the MUD and doorman
 */

typedef enum {
    PACKET_INIT, /* Sent on initialisation of the MUD/doorman */
    PACKET_CONNECT, /* Initiate a new channel */
    PACKET_DISCONNECT, /* Disconnect a channel */
    PACKET_DISCONNECT_ACK, /* Confirm a channel is no longer in use */
    PACKET_RECONNECT, /* Sent on receipt of MUD init for each channel */
    PACKET_MESSAGE, /* Send a message to a channel */
    PACKET_INFO, /* Update socket information */
    PACKET_ECHO_ON, /* Turn echo on */
    PACKET_ECHO_OFF, /* Turn echo off */
    PACKET_AUTHORIZED, /* Character has been authorized */
} PacketType;

typedef struct tagInfoData {
    uint16_t port; /* Their port number */
    uint32_t netaddr; /* Their IP address */
    uint8_t ansi; /* ANSI-compliant terminal */
    /* Followed by hostname\0 */
    char data[0];
} InfoData;

#define PACKET_MAX_PAYLOAD_SIZE 4096
typedef struct tagPacket {
    PacketType type; /* Type of packet */

    uint32_t nExtra; /* Number of extra bytes after the packet */
    uint32_t channel; /* The channel number */
    char data[0]; /* nExtra bytes live here onwards */
} Packet;
