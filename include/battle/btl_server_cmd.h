#ifndef POKEBW2_BATTLE_BTL_SERVER_CMD_H
#define POKEBW2_BATTLE_BTL_SERVER_CMD_H

// Overlay 167's btl_server_cmd.c, named after the BTL_SERVER_CMD_QUE_SIZE of its asserts: the queue of the commands
// the server sends its clients, which writes each command's arguments packed as the command's format says. Function
// names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "gfl/std.h"
#include "struct_decls.h"

#define BTL_SERVER_CMD_QUE_SIZE 3000

// The commands that the turn's flow writes, which the server then sends
struct BtlServerCmdQueue {
    u32 writePtr;
    u32 readPtr;
    u8 buffer[BTL_SERVER_CMD_QUE_SIZE];
};

static inline void BtlServerCmdQueue_Init(BtlServerCmdQueue *queue) {
    queue->writePtr = 0;
    queue->readPtr = 0;
}

// Fills the queue with a block of commands the server sent
static inline void BtlServerCmdQueue_Setup(BtlServerCmdQueue *queue, const void *data, u16 size) {
    MIi_CpuCopy32(data, queue->buffer, size);
    queue->writePtr = size;
    queue->readPtr = 0;
}

// Whether every command has been read
static inline BOOL BtlServerCmdQueue_IsEmpty(const BtlServerCmdQueue *queue) {
    return queue->readPtr == queue->writePtr;
}

void func_ov167_021b1434(BtlServerCmdQueue *que, u32 event, ...);
u16 SCQUE_RESERVE_Pos(BtlServerCmdQueue *que, u32 event);
void func_ov167_021b14ec(BtlServerCmdQueue *que, u32 reserve, u32 event, ...);
u16 func_ov167_021b1564(BtlServerCmdQueue *que, u32 *args);
void func_ov167_021b15c0(BtlServerCmdQueue *que, u8 value);
u8 func_ov167_021b15c8(BtlServerCmdQueue *que);
void func_ov167_021b15d0(BtlServerCmdQueue *que, u8 event, ...);
void func_ov167_021b1670(void);

// Command 0x18, with its arguments' types
static inline void BtlServerCmd_Put18(void *queue, u8 monId, u8 target, u8 result, u8 arg4, u16 move, u16 arg6) {
    func_ov167_021b1434(queue, 0x18, monId, target, result, arg4, move, arg6);
}

// Command 0x54, with its arguments' types
static inline void BtlServerCmd_Put54(void *queue, u8 monId, u8 effectiveness, u16 move) {
    func_ov167_021b1434(queue, 0x54, monId, effectiveness, move);
}

#endif // POKEBW2_BATTLE_BTL_SERVER_CMD_H
