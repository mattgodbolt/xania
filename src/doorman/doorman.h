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

#ifndef _DOORMAN_H
#define _DOORMAN_H

#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

#define XANIA_FILE "/tmp/.xania-%d-%s"
#define CONNECTION_WAIT_TIME 10

// typedef enum { false = 0, true = 1 } bool;
#ifndef true
#define true 1
#endif
#ifndef false
#define false 0
#endif

/*
 * A packet of data passing between the MUD and doorman
 */

typedef enum {
    PACKET_INIT, /* Sent on initialisation of the MUD/doorman */
    PACKET_CONNECT, /* Initiate a new channel */
    PACKET_DISCONNECT, /* Disconnect a channel */
    PACKET_RECONNECT, /* Sent on receipt of MUD init for each channel */
    PACKET_MESSAGE, /* Send a message to a channel */
    PACKET_INFO, /* Update socket information */
    PACKET_SHUTDOWN, /* Notify of a MUD shutdown */
    PACKET_ECHO_ON, /* Turn echo on */
    PACKET_ECHO_OFF, /* Turn echo off */
    PACKET_AUTHORIZED, /* Character has been authorized */
} PacketType;

typedef struct tagInfoData {
    short port; /* Their port number */
    int netaddr; /* Their IP address */
    char ansi; /* ANSI-compliant terminal */
    /* Followed by hostname\0 */
    char data[0];
} InfoData;

#define CHANNEL_BROADCAST (-1)
#define CHANNEL_LOG (-2)
#define CHANNEL_MAX (64)
typedef struct tagPacket {
    PacketType type; /* Type of packet */

    unsigned int nExtra; /* Number of extra bytes after the packet */
    int channel; /* The channel number */
    char data[0]; /* nExtra bytes live here onwards */
} Packet;

/*
 * Buffer up to 2k of incoming data
 */
#define INCOMING_BUFFER_SIZE (2048)

#endif
