#ifndef POKEBW2_NITROWIFI_SOCL_INTERNAL_H
#define POKEBW2_NITROWIFI_SOCL_INTERNAL_H

#include "nitrowifi/soc.h"

// SOCL's state, shared by its files

// socl_startup.c
extern SOCLConfig *SOCLiConfigPtr;     // The config SOCL_Startup was given, NULL while SOCL is down
extern u32 SOCLiDefaultLanBufferSize;  // The LAN buffer's size when the config gives none
extern CPSConfig SOCLiCPSConfig;

// A command pipe: the queue of the packets that its thread runs (the socket's fields at 0xa0 and 0xa4 point to its
// receive and send pipes, and a socket without a receive pipe uses the send pipe for control)
typedef struct SOCLCommandPipe {
    OSMessageQueue queue;
    u8 unk_20[0x28]; // (not read by socl_command.c)
} SOCLCommandPipe;

// The part of a socket that socl_command.c reads
typedef struct SOCLSocket {
    u8 unk_00[0xa0];
    SOCLCommandPipe *recv_pipe;
    SOCLCommandPipe *send_pipe;
    int result; // What the last command packet returned
    u8 unk_ac[3];
    s8 flag;    // Copied into each command packet
} SOCLSocket;

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

// socl_list.c
// Prepares the socket lists before SOCL starts (no counterpart in pokediamond's NitroWiFi)
void func_ov011_02157980(void);
// CPS's address check callback (no counterpart in pokediamond's NitroWiFi)
BOOL func_ov011_02157a48(void);

#endif // POKEBW2_NITROWIFI_SOCL_INTERNAL_H
