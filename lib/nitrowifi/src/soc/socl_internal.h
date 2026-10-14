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

// The receive pipe, 0x114 bytes
typedef struct SOCLRecvPipe {
    SOCLCommandPipe base;
    u8 unk_f8[4];
    u16 rcvbuf_consume_min; // From SOCLSocketParam
    u8 unk_fe[0xc];
    u16 unk_10a;            // From SOCLSocketParam's rcvpipe_unk_e
    int unk_10c;
    int unk_110;
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
    u8 unk_b0[0xc];
} SOCLSocket;

// socl_startup.c: the socket that sends every UDP socket's datagrams
extern SOCLSocket *SOCLiUDPSendSocket;

// SOCLSocket.state
#define SOCLI_STATE_CREATED 1

static inline BOOL SOCLi_SocketIsCreated(const SOCLSocket *socket) {
    if (socket != NULL && (socket->state & SOCLI_STATE_CREATED)) {
        return TRUE;
    }
    return FALSE;
}

// SOCLSocket.flag: 0 and 4 are TCP
static inline BOOL SOCLi_SocketIsTCP(const SOCLSocket *socket) {
    if (socket->flag == 0 || socket->flag == 4) {
        return TRUE;
    }
    return FALSE;
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
    u8 args[0x1e];
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

// socl_close.c
void SOCLi_TrashSocket(void);

// socl_cleanup.c
// Shuts CPS down after a failed SOCL_Startup (no counterpart in pokediamond's NitroWiFi)
void func_ov011_02157688(void);

// socl_misc.c
u32 SOCLi_RoundUp4(u32 value);

// socl_read.c
BOOL SOCLi_UdpRecvCallback(u8 *data, u32 len, CPSSoc *soc);

// socl_list.c
// Two counters that SOCL_CreateSocket raises, for the TCP sockets and for the others (a later file's data, probably
// the socket list's, unnamed yet)
extern s32 data_ov011_0218dc4c[2];
// Puts a new socket in the list of sockets (SOCLi_SocketRegister in pokediamond's NitroWiFi)
void SOCLi_SocketRegister(SOCLSocket *socket);
// Prepares the socket lists before SOCL starts (no counterpart in pokediamond's NitroWiFi)
void func_ov011_02157980(void);
// CPS's address check callback (no counterpart in pokediamond's NitroWiFi)
BOOL func_ov011_02157a48(void);

#endif // POKEBW2_NITROWIFI_SOCL_INTERNAL_H
