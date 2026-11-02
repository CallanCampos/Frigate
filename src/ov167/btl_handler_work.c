#include "types.h"
#include "battle/btl_handler.h"
#include "battle/btl_handler_work.h"
#include "gfl/std.h"

void InitState(BtlActionState *state) {
    state->raw = 0;
    MI_CpuFill8(state->work, 0, sizeof(state->work));
}

// Function name from swan.
u32 PushState(BtlActionState *state, u32 command) {
    u32 prev = state->raw;

    state->useItemNo = 0;
    state->savedPos = state->workPos;
    state->result = 0;
    state->prevResult = 0;
    state->used = 0;
    return prev;
}

// Function name from swan.
u32 PushStateUseItem(BtlActionState *state, u16 item, u32 command) {
    u32 prev = state->raw;

    state->useItemNo = item;
    state->savedPos = state->workPos;
    state->result = 0;
    state->prevResult = 0;
    state->used = 0;
    return prev;
}

// Function names from swan.
void PopState(BtlActionState *state, u32 value, u32 command) {
    state->raw = value;
}

u16 GetUseItemNo(BtlActionState *state) {
    return state->useItemNo;
}

BOOL IsUsed(BtlActionState *state) {
    return state->used;
}

void SetResult(BtlActionState *state, BOOL result) {
    if (result) {
        state->prevResult = 1;
        state->result = 1;
    } else {
        state->prevResult = 0;
    }
    state->used = 1;
}

BOOL GetPrevResult(BtlActionState *state) {
    return state->prevResult;
}

BOOL GetResult(BtlActionState *state) {
    return state->result;
}

// The size of each handler command's work
typedef struct {
    u8 command;
    u8 size;
} BtlHandlerWorkSize;

static const BtlHandlerWorkSize sHandlerWorkSizes[] = {
    { BTL_HANDLER_USE_HELD_ITEM, 0x08 },
    { BTL_HANDLER_USE_ITEM_ANIM, 0x04 },
    { BTL_HANDLER_RECOVER_HP, 0x30 },
    { BTL_HANDLER_DRAIN, 0x30 },
    { BTL_HANDLER_CHANGE_HP, 0x28 },
    { BTL_HANDLER_RECOVER_PP, 0x30 },
    { BTL_HANDLER_DECREMENT_PP, 0x30 },
    { BTL_HANDLER_CURE_CONDITION, 0x40 },
    { BTL_HANDLER_ADD_CONDITION, 0x3c },
    { BTL_HANDLER_STAT_CHANGE, 0x40 },
    { BTL_HANDLER_SET_STAT_STAGE, 0x0c },
    { BTL_HANDLER_RECOVER_STAT_STAGE, 0x08 },
    { BTL_HANDLER_RESET_STAT_STAGE, 0x0c },
    { BTL_HANDLER_SET_STATUS, 0x38 },
    { BTL_HANDLER_DAMAGE, 0x34 },
    { BTL_HANDLER_FAINT, 0x30 },
    { BTL_HANDLER_CHANGE_TYPE, 0x08 },
    { BTL_HANDLER_MESSAGE, 0x2c },
    { BTL_HANDLER_ABILITY_POPUP_ADD, 0x04 },
    { BTL_HANDLER_ABILITY_POPUP_REMOVE, 0x04 },
    { BTL_HANDLER_SET_TURN_FLAG, 0x0c },
    { BTL_HANDLER_RESET_TURN_FLAG, 0x0c },
    { BTL_HANDLER_SET_CONTINUE_FLAG, 0x0c },
    { BTL_HANDLER_RESET_CONTINUE_FLAG, 0x0c },
    { BTL_HANDLER_ADD_SIDE_EFFECT, 0x38 },
    { BTL_HANDLER_REMOVE_SIDE_EFFECT, 0x08 },
    { BTL_HANDLER_ADD_FIELD_EFFECT, 0x38 },
    { BTL_HANDLER_CHANGE_WEATHER, 0x30 },
    { BTL_HANDLER_REMOVE_FIELD_EFFECT, 0x08 },
    { BTL_HANDLER_ADD_POS_EFFECT, 0x20 },
    { BTL_HANDLER_ABILITY_CHANGE, 0x34 },
    { BTL_HANDLER_SET_ITEM, 0x34 },
    { BTL_HANDLER_CHECK_HELD_ITEM, 0x0c },
    { BTL_HANDLER_FORCE_USE_ITEM, 0x08 },
    { BTL_HANDLER_CONSUME_ITEM, 0x30 },
    { BTL_HANDLER_SWAP_ITEM, 0x80 },
    { BTL_HANDLER_UPDATE_MOVE, 0x0c },
    { BTL_HANDLER_SET_COUNTER, 0x08 },
    { BTL_HANDLER_DELAY_MOVE_DAMAGE, 0x08 },
    { BTL_HANDLER_QUIT_BATTLE, 0x04 },
    { BTL_HANDLER_SWITCH, 0x58 },
    { BTL_HANDLER_BATON_PASS, 0x08 },
    { BTL_HANDLER_FLINCH, 0x08 },
    { BTL_HANDLER_REVIVE, 0x30 },
    { BTL_HANDLER_SET_WEIGHT, 0x30 },
    { BTL_HANDLER_FORCE_SWITCH, 0x30 },
    { BTL_HANDLER_INTERRUPT_ACTION, 0x30 },
    { BTL_HANDLER_INTERRUPT_MOVE, 0x08 },
    { BTL_HANDLER_SEND_LAST, 0x30 },
    { BTL_HANDLER_SWAP_POKE, 0x30 },
    { BTL_HANDLER_TRANSFORM, 0x30 },
    { BTL_HANDLER_ILLUSION_BREAK, 0x30 },
    { BTL_HANDLER_GRAVITY_CHECK, 0x04 },
    { BTL_HANDLER_HIDE_TURN_CANCEL, 0x34 },
    { BTL_HANDLER_EFFECT_AT_POS, 0x34 },
    { BTL_HANDLER_REMOVE_MESSAGE_WINDOW, 0x04 },
    { BTL_HANDLER_CHANGE_FORM, 0x30 },
    { BTL_HANDLER_SET_MOVE_EFFECT_INDEX, 0x08 },
    { BTL_HANDLER_SET_MOVE_EFFECT_ENABLE, 0x04 },
};

// Allocates a handler command's work on the stack, zeroed and with its header set up
void *PushWork(BtlActionState *state, u32 command, u32 monId) {
    u32 i;
    u8 *work;
    u32 pos;
    u32 size;
    BattleHandlerHeader *header;

    for (i = 0, size = 0; i < NELEMS(sHandlerWorkSizes); i++) {
        if (command == sHandlerWorkSizes[i].command) {
            size = sHandlerWorkSizes[i].size;
            break;
        }
    }
    if (size != 0) {
        while (size & 3) {
            size++;
        }
        pos = state->workPos;
        if (pos + size <= sizeof(state->work)) {
            work = state->work;
            for (i = 0; i < size; i++) {
                state->work[state->workPos + i] = 0;
            }
            header = (BattleHandlerHeader *)&work[pos];
            header->command = command;
            header->size = size;
            header->monId = monId;
            header->unk23 = 0;
            header->unk26 = 1;
            state->workPos += size;
            return header;
        }
    }
    return NULL;
}

// Function name from swan.
void PopWork(BtlActionState *state, void *work) {
    BattleHandlerHeader *header = work;
    u32 pos = state->workPos;

    if (header->size <= pos) {
        u32 start = (u8 *)work - state->work;
        u32 end = start + header->size;
        if (end == pos) {
            state->workPos = pos - header->size;
        }
    }
}
