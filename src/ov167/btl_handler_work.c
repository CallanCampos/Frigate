#include "types.h"
#include "battle/btl_handler.h"
#include "battle/btl_handler_work.h"
#include "gfl/std.h"

void func_ov167_021b083c(BtlActionState *state) {
    state->raw = 0;
    sys_memset(state->work, 0, sizeof(state->work));
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

BOOL func_ov167_021b0918(BtlActionState *state) {
    return state->result;
}

// The size of each handler command's work
typedef struct {
    u8 command;
    u8 size;
} BtlHandlerWorkSize;

// clang-format off
static const BtlHandlerWorkSize data_ov167_021d6dd8[59] = {
    { 0x00, 0x08 }, { 0x01, 0x04 }, { 0x05, 0x30 }, { 0x06, 0x30 }, { 0x08, 0x28 }, { 0x09, 0x30 },
    { 0x0a, 0x30 }, { 0x0b, 0x40 }, { 0x0c, 0x3c }, { 0x0e, 0x40 }, { 0x0f, 0x0c }, { 0x12, 0x08 },
    { 0x10, 0x0c }, { 0x11, 0x38 }, { 0x07, 0x34 }, { 0x13, 0x30 }, { 0x14, 0x08 }, { 0x04, 0x2c },
    { 0x02, 0x04 }, { 0x03, 0x04 }, { 0x15, 0x0c }, { 0x16, 0x0c }, { 0x17, 0x0c }, { 0x18, 0x0c },
    { 0x19, 0x38 }, { 0x1a, 0x08 }, { 0x1b, 0x38 }, { 0x1d, 0x30 }, { 0x1c, 0x08 }, { 0x1e, 0x20 },
    { 0x1f, 0x34 }, { 0x20, 0x34 }, { 0x21, 0x0c }, { 0x22, 0x08 }, { 0x23, 0x30 }, { 0x24, 0x80 },
    { 0x25, 0x0c }, { 0x26, 0x08 }, { 0x27, 0x08 }, { 0x28, 0x04 }, { 0x29, 0x58 }, { 0x2a, 0x08 },
    { 0x2b, 0x08 }, { 0x2c, 0x30 }, { 0x2d, 0x30 }, { 0x2e, 0x30 }, { 0x2f, 0x30 }, { 0x30, 0x08 },
    { 0x31, 0x30 }, { 0x32, 0x30 }, { 0x33, 0x30 }, { 0x34, 0x30 }, { 0x35, 0x04 }, { 0x36, 0x34 },
    { 0x37, 0x34 }, { 0x38, 0x04 }, { 0x39, 0x30 }, { 0x3a, 0x08 }, { 0x3b, 0x04 },
};

// clang-format on

// Allocates a handler command's work on the stack, zeroed and with its header set up
void *func_ov167_021b0920(BtlActionState *state, u32 command, u32 monId) {
    u32 i;
    u8 *work;
    u32 pos;
    u32 size;
    BattleHandlerHeader *header;

    for (i = 0, size = 0; i < 59; i++) {
        if (command == data_ov167_021d6dd8[i].command) {
            size = data_ov167_021d6dd8[i].size;
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
