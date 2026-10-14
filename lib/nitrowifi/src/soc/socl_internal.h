#ifndef POKEBW2_NITROWIFI_SOCL_INTERNAL_H
#define POKEBW2_NITROWIFI_SOCL_INTERNAL_H

#include "nitrowifi/soc.h"

// SOCL's state, shared by its files

// socl_startup.c
extern SOCLConfig *SOCLiConfigPtr;     // The config SOCL_Startup was given, NULL while SOCL is down
extern u32 SOCLiDefaultLanBufferSize;  // The LAN buffer's size when the config gives none
extern CPSInAddr SOCLiRequestedIP;     // The address that DHCP asks for
extern u32 SOCLiDhcpState;             // SOCLI_DHCP_*
extern CPSConfig SOCLiCPSConfig;

// SOCLiDhcpState: DHCP is in use, and it has finished (or the address is static)
#define SOCLI_DHCP_REQUESTED 1
#define SOCLI_DHCP_DONE 2

// A command pipe: the queue of the packets that its thread runs (the socket's fields at 0xa0 and 0xa4 point to its
// receive and send pipes, and a socket without a receive pipe uses the send pipe for control). Its messages and the
// thread's stack follow the pipe in the socket's memory.
typedef struct SOCLCommandPipe {
    OSMessageQueue queue;
    OSThread thread;
    OSMutex mutex;
} SOCLCommandPipe;

// A datagram that SOCLi_UdpRecvCallback queued, followed by its data
typedef struct SOCLUdpPacket {
    struct SOCLUdpPacket *next;
    u16 len;
    u16 port;
    CPSInAddr ip;
    u8 data[];
} SOCLUdpPacket;

// The receive pipe, 0x114 bytes
typedef struct SOCLRecvPipe {
    SOCLCommandPipe base;
    int cps_read_len;       // What the reads have taken from CPS's receive buffer and not yet consumed
    u16 rcvbuf_consume_min; // From SOCLSocketParam: how much to take before CPS_SocConsume
    s8 peek;                // 1 while a peeking read runs: it takes the data without moving the position
    u8 unk_ff;
    SOCLUdpPacket *udp_tail;  // The datagrams that came in, oldest first
    SOCLUdpPacket *udp_head;
    u16 udp_queued;         // The bytes of datagram data they hold
    u16 udp_queued_max;     // From SOCLSocketParam's rcvpipe_unk_e
    OSThreadQueue udp_wait; // Where a blocking UDP read sleeps
} SOCLRecvPipe;

// A buffer in the socket's memory
typedef struct SOCLSocketBuffer {
    u32 size;
    u8 *data; // NULL when size is 0
} SOCLSocketBuffer;

struct SOCLSocket;

// The send pipe, 0x110 bytes
typedef struct SOCLSendPipe {
    SOCLCommandPipe base;
    SOCLSocketBuffer buffer;
    u8 unk_100[4];
    int unk_104;
    int unk_108;
    struct SOCLSocket *socket;
} SOCLSendPipe;

// A socket, 0xbc bytes, followed in memory by its pipes and buffers
typedef struct SOCLSocket {
    u8 unk_00[0x48];
    SOCLSocketBuffer rcvbuf;   // 0x48
    u8 unk_50[4];
    SOCLSocketBuffer sndbuf;   // 0x54
    SOCLSocketBuffer rcvbuf_aux;
    SOCLSocketBuffer sndbuf_aux;
    u8 unk_6c[0x34];
    SOCLCommandPipe *recv_pipe;
    SOCLCommandPipe *send_pipe;
    int result; // What the last command packet returned
    s16 state;  // SOCLI_STATE_*
    s8 flag_block;
    s8 flag;    // The mode (SOCLSocketParam's flag_mode), copied into each command packet
    u16 local_port;  // 0xb0
    u16 remote_port;
    CPSInAddr remote_ip;
    u8 unk_b8[4];
} SOCLSocket;

// The receive pipe of a socket that has one (every socket that reads)
static inline SOCLRecvPipe *SOCLi_GetRecvPipe(const SOCLSocket *socket) {
    return (SOCLRecvPipe *)socket->recv_pipe;
}

// socl_startup.c: the socket that sends every UDP socket's datagrams
extern SOCLSocket *SOCLiUDPSendSocket;

// SOCLi_ReadFrom's flags (BSD's MSG_PEEK and MSG_DONTWAIT)
#define SOCLI_READ_PEEK 2     // Leave the data to be read again
#define SOCLI_READ_NOBLOCK 4  // Do not wait for data

// SOCLSocket.state
#define SOCLI_STATE_CREATED 1
#define SOCLI_STATE_BINDING 2        // SOCLi_ExecBindCommand has queued the bind (and connect)
#define SOCLI_STATE_CONNECTED 4      // The bind (and for TCP the connect) succeeded
#define SOCLI_STATE_CLOSING 8
#define SOCLI_STATE_CONNECT_FAILED 0x40 // The connect failed, with the error in the socket's result
#define SOCLI_STATE_RESET 0x80       // The connection was reset

static inline BOOL SOCLi_SocketIsCreated(const SOCLSocket *socket) {
    BOOL created = FALSE;
    if (socket != NULL) {
        if (socket->state & SOCLI_STATE_CREATED) {
            created = TRUE;
        } else {
            created = FALSE;
        }
    }
    return created;
}

// SOCLSocket.flag: 0 and 4 are TCP
static inline BOOL SOCLi_SocketIsTCP(const SOCLSocket *socket) {
    BOOL tcp = TRUE;
    if (socket->flag != 0 && socket->flag != 4) {
        tcp = FALSE;
    }
    return tcp;
}

struct SOCLCommandPacket;
typedef int (*SOCLCommandHandler)(struct SOCLCommandPacket *packet);

// A command for a socket's pipe, 0x2c bytes: the handler, run by the pipe's thread, with its arguments after the
// header
typedef struct SOCLCommandPacket {
    SOCLCommandHandler handler;
    SOCLSocket *socket;
    OSMessageQueue *reply_queue; // Where a blocking caller waits for the result, NULL otherwise
    s8 socket_flag;
    s8 flag_block;               // 1 when the caller waits for the result (bit 0 of it blocks the send)
    u8 unk_0e[2];
    union {
        struct {
            u16 local_port;
            u16 remote_port;
            CPSInAddr remote_ip;
        } bind;
        struct {
            u8 *buf;
            int len;
            u16 *remote_port; // Where the datagram's sender goes, or NULL
            CPSInAddr *remote_ip;
        } read;
        u8 raw[0x1c];
    } args;
} SOCLCommandPacket;

// socl_command.c
int SOCLi_StartupCommandPacketQueue(u32 max);
int SOCLi_CleanupCommandPacketQueue(void);
SOCLCommandPacket *SOCLi_AllocCommandPacket(s32 flags);
SOCLCommandPacket *SOCLi_CreateCommandPacket(SOCLCommandHandler handler, SOCLSocket *socket, s32 flag_block);
void SOCLi_FreeCommandPacket(SOCLCommandPacket *packet);
SOCLCommandPipe *SOCLi_GetCtrlPipe(SOCLSocket *socket);
int SOCLi_SendCommandPacket(SOCLCommandPipe *pipe, SOCLCommandPacket *packet);
int SOCLi_SendCommandPacketToCtrlPipe(SOCLSocket *socket, SOCLCommandPacket *packet);
int SOCLi_ExecCommandPacketInRecvPipe(SOCLSocket *socket, SOCLCommandPacket *packet);
int SOCLi_ExecCommandPacketInSendPipe(SOCLSocket *socket, SOCLCommandPacket *packet);
int SOCLi_ExecCommandPacketInCtrlPipe(SOCLSocket *socket, SOCLCommandPacket *packet);
void SOCLi_CommandPacketHandler(OSMessageQueue *queue);

// socl_bind.c
int SOCL_Bind(SOCLSocket *socket, u16 local_port);
int SOCL_Connect(SOCLSocket *socket, u16 remote_port, CPSInAddr remote_ip);

// socl_close.c
void SOCLi_TrashSocket(void);

// socl_cleanup.c
// Shuts CPS down after a failed SOCL_Startup (no counterpart in pokediamond's NitroWiFi)
void func_ov011_02157688(void);

// socl_misc.c
u32 SOCLi_RoundUp4(u32 value);

// socl_read.c
// Reads from a socket; flags is SOCLI_READ_*, remote_port and remote_ip (either may be NULL) get the sender's
int SOCLi_ReadFrom(SOCLSocket *socket, u8 *buf, int len, u16 *remote_port, CPSInAddr *remote_ip, int flags);
BOOL SOCLi_UdpRecvCallback(u8 *data, u32 len, CPSSoc *soc);

// socl_list.c
// Two counters that SOCL_CreateSocket raises, for the TCP sockets and for the others (a later file's data, probably
// the socket list's, unnamed yet)
extern s32 data_ov011_0218dc4c[2];
// Puts a new socket in the list of sockets (SOCLi_SocketRegister in pokediamond's NitroWiFi)
void SOCLi_SocketRegister(SOCLSocket *socket);
BOOL SOCL_SocketIsInvalid(const SOCLSocket *socket); // TRUE for a handle that is not a socket of the list
// Prepares the socket lists before SOCL starts (no counterpart in pokediamond's NitroWiFi)
void func_ov011_02157980(void);
// CPS's address check callback (no counterpart in pokediamond's NitroWiFi)
BOOL func_ov011_02157a48(void);

#endif // POKEBW2_NITROWIFI_SOCL_INTERNAL_H
