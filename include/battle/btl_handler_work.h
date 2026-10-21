#ifndef POKEBW2_BATTLE_BTL_HANDLER_WORK_H
#define POKEBW2_BATTLE_BTL_HANDLER_WORK_H

// Overlay 167's btl_handler_work.c, named descriptively: the stack of the handler commands' work, with the state of
// the event that pushed them. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it
// has them

#include "types.h"
#include "struct_decls.h"

struct BtlActionState {
    union {
        u32 raw;
        struct {
            u32 useItemNo : 10;
            // The top of the work stack, and where PushState saved it
            u32 workPos : 9;
            u32 savedPos : 9;
            u32 prevResult : 1;
            u32 result : 1;
            u32 used : 1;
            u32 unk31 : 1;
        };
    };
    // The stack of the handler commands' work
    u8 work[500];
};

void InitState(BtlActionState *state);
u32 PushState(BtlActionState *state, u32 command);
u32 PushStateUseItem(BtlActionState *state, u16 item, u32 command);
void PopState(BtlActionState *state, u32 value, u32 command);
u16 GetUseItemNo(BtlActionState *state);
BOOL IsUsed(BtlActionState *state);
void SetResult(BtlActionState *state, BOOL result);
BOOL GetPrevResult(BtlActionState *state);
BOOL GetResult(BtlActionState *state);
void *PushWork(BtlActionState *state, u32 command, u32 monId);
void PopWork(BtlActionState *state, void *work);

#endif // POKEBW2_BATTLE_BTL_HANDLER_WORK_H
