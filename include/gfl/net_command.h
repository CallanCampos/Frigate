#ifndef POKEBW2_GFL_NET_COMMAND_H
#define POKEBW2_GFL_NET_COMMAND_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The commands machines send each other. The library's own are below 0x100; a game's table takes a base of 0x100 or
// more, the command's high byte, and its commands are the low byte. Data larger than a packet is sent in chunks

typedef struct {
    // Called when the command arrives, with the data and the table's work
    void (*callback)(int netId, int size, const void *data, void *work, NetHandle *handle);
    // Where data that arrives in chunks goes, or NULL to keep it out of the receive buffer
    void *(*getBuffer)(int netId, void *work, int size);
} NetCommand;

// The library's commands, below which a machine that has not negotiated may send
#define GFL_NET_CMD_BASE_COUNT 0x11

// The tables are NetCommand arrays
void func_02040b94(int base, const void *list, int listNum, void *work, HeapID unused);
void func_02040bfc(void);
void func_02040c20(int base, const void *list, int listNum, void *work);
void func_02040c64(int base);
// Whether data from these machines (bits, or GFL_NET_NETID_SERVER for all) is for this machine
BOOL func_02040c94(int netIds);
// Calls a command's handler
void func_02040d78(int netId, int netIds, int command, int size, void *data, NetHandle *handle);
BOOL func_02040dc0(int command);
BOOL func_02040dd4(int command);
void *func_02040de8(int command, int netId, int size);
void func_02040e0c(void);
// Handlers for data in chunks: a chunk, and the header before the chunks
void func_02040e10(int netId, int size, const void *data, void *work, NetHandle *handle);
void func_02040ebc(int netId, int size, const void *data, void *work, NetHandle *handle);
// Sends data in chunks, returning whether it could start
int func_02040f84(NetHandle *handle, u8 netIds, u16 command, u32 size, void *data);
// Sends the next chunk, each frame
void func_020410dc(void);

#endif // POKEBW2_GFL_NET_COMMAND_H
