#ifndef POKEBW2_BATTLE_BTL_SERVER_CMD_H
#define POKEBW2_BATTLE_BTL_SERVER_CMD_H

// Overlay 167's btl_server_cmd.c, named after the BTL_SERVER_CMD_QUE_SIZE of its asserts: the queue of the commands
// the server sends its clients, which writes each command's arguments packed as the command's format says. Function
// names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them

#include "types.h"
#include "gfl/std.h"
#include "struct_decls.h"

#define BTL_SERVER_CMD_QUE_SIZE 3000

// The server's commands, each named after the client's handler that plays it (btl_client.c's sServerCmdTable)
enum {
    SC_NULL,
    SC_OP_SUBTRACT_HP,
    SC_OP_ADD_HP,
    SC_OP_ZERO_HP,
    SC_OP_SUBTRACT_PP,
    SC_OP_SUBTRACT_ORIGINAL_PP,
    SC_OP_SET_MOVE_USED,
    SC_OP_ADD_PP,
    SC_OP_ADD_ORIGINAL_PP,
    SC_OP_RAISE_STAT_STAGE,
    SC_OP_LOWER_STAT_STAGE,
    SC_OP_SET_STAT_STAGES,
    SC_OP_RECOVER_STAT_STAGES,
    SC_OP_RESET_STAT_STAGES,
    SC_OP_ADD_CRIT_STAGE,
    SC_OP_SET_CONDITION,
    SC_OP_CURE_CONDITION,
    SC_OP_CURE_MOVE_CONDITION,
    SC_OP_SEND_IN,
    SC_OP_SET_BASE_STAT,
    SC_OP_SET_WEIGHT,
    SC_OP_CONDITION_TURN_CHECK,
    SC_OP_CHANGE_TYPE,
    SC_OP_CONSUME_ITEM,
    SC_OP_UPDATE_USED_MOVE,
    SC_OP_SET_CONDITION_FLAG,
    SC_OP_CLEAR_CONDITION_FLAG,
    SC_OP_SET_TURN_FLAG,
    SC_OP_CLEAR_TURN_FLAG,
    SC_OP_CHANGE_ABILITY,
    SC_OP_SET_ITEM,
    SC_OP_UPDATE_MOVE,
    SC_OP_CLEAR_FOR_SWITCH,
    SC_OP_ADD_FIELD_EFFECT,
    SC_OP_ADD_FIELD_EFFECT_DEPEND_MON,
    SC_OP_REMOVE_FIELD_EFFECT_DEPEND_MON,
    SC_OP_REMOVE_FIELD_EFFECT,
    SC_OP_SET_COUNTER,
    SC_OP_BATON_PASS,
    SC_OP_CREATE_SUBSTITUTE,
    SC_OP_DELETE_SUBSTITUTE,
    SC_OP_ADD_SHOOTER_ENERGY,
    SC_OP_SETUP_ILLUSION,
    SC_OP_CLEAR_CONSUMED_ITEM,
    SC_OP_CURE_MON_DEPENDENT_CONDITIONS,
    SC_OP_ADD_DAMAGE_RECORD,
    SC_OP_TURN_CHECK,
    SC_OP_FIELD_TURN_CHECK,
    SC_MOVE_ANIM,
    SC_SET_MON_VISIBLE,
    SC_DAMAGE,
    SC_DAMAGE_PLURAL,
    SC_ONE_HIT_KO,
    SC_SET_GAUGE_STATUS,
    SC_CONFUSION_DAMAGE,
    SC_STAT_UP_EFFECT,
    SC_STAT_DOWN_EFFECT,
    SC_FAINT,
    SC_SHOW_MON,
    SC_RECALL_MSG,
    SC_RECALL,
    SC_SEND_OUT,
    SC_ADD_EVS,
    SC_WEATHER_START,
    SC_WEATHER_END,
    SC_UPDATE_HP_GAUGE,
    SC_USE_HELD_ITEM,
    SC_FAINT_HP_GAUGE,
    SC_SWAP_POS,
    SC_EXP_GAIN,
    SC_BALL_THROW,
    SC_BALL_THROW_TRAINER,
    SC_ROTATION,
    SC_CHANGE_ABILITY,
    SC_SWAP_ABILITY,
    SC_ILLUSION_BREAK,
    SC_EFFECT,
    SC_EFFECT_BY_POS,
    SC_EFFECT_BETWEEN_POS,
    SC_CHANGE_FORM,
    SC_MOVE_TO_CENTER,
    SC_SUBSTITUTE_CREATE,
    SC_SUBSTITUTE_DELETE,
    SC_TRANSFORM,
    SC_DAMAGE_EFFECT,
    SC_PLAY_BGM,
    SC_HIDE_MSG_WINDOW,
    SC_ABILITY_POPUP_IN,
    SC_ABILITY_POPUP_OUT,
    SC_MOVE_MSG,
    // The message commands, which BtlServerCmdQueue_PutMsg writes: a message and its arguments, the _SE ones with a
    // sound effect
    SC_MSG_STD,
    SC_MSG_SET,
    SC_MSG_STD_SE,
    SC_MSG_SET_SE,
    // What BtlServerCmdQueue_Read returns when no command is left
    SC_NONE,
    // The room SCQUE_RESERVE_Pos keeps for a command, with its size, which BtlServerCmdQueue_Read skips
    SC_RESERVE,
    SC_MAX,
};

// What ends a message command's arguments
#define SC_MSG_ARG_END 0xffff0000

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

void BtlServerCmdQueue_Put(BtlServerCmdQueue *que, u32 event, ...);
u16 SCQUE_RESERVE_Pos(BtlServerCmdQueue *que, u32 event);
void BtlServerCmdQueue_PutReserved(BtlServerCmdQueue *que, u32 reserve, u32 event, ...);
u16 BtlServerCmdQueue_Read(BtlServerCmdQueue *que, u32 *args);
void BtlServerCmdQueue_PutByte(BtlServerCmdQueue *que, u8 value);
u8 BtlServerCmdQueue_ReadByte(BtlServerCmdQueue *que);
void BtlServerCmdQueue_PutMsg(BtlServerCmdQueue *que, u8 event, ...);
void func_ov167_021b1670(void);

// SC_OP_UPDATE_USED_MOVE, with its arguments' types
static inline void BtlServerCmd_Put18(BtlServerCmdQueue *queue, u8 monId, u8 target, u8 result, u8 arg4, u16 move,
                                      u16 arg6) {
    BtlServerCmdQueue_Put(queue, SC_OP_UPDATE_USED_MOVE, monId, target, result, arg4, move, arg6);
}

// SC_DAMAGE_EFFECT, with its arguments' types
static inline void BtlServerCmd_Put54(BtlServerCmdQueue *queue, u8 monId, u8 effectiveness, u16 move) {
    BtlServerCmdQueue_Put(queue, SC_DAMAGE_EFFECT, monId, effectiveness, move);
}

#endif // POKEBW2_BATTLE_BTL_SERVER_CMD_H
