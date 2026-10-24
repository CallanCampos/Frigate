#ifndef POKEBW2_BATTLE_BTL_EVENT_H
#define POKEBW2_BATTLE_BTL_EVENT_H

#include "types.h"
#include "struct_decls.h"

// Checks whether an item's handlers are skipped for the item being run: the checking item, the server flow, the
// checked item's factor type, the event, and the checked item's sub ID and mon
typedef BOOL (*BattleEventSkipCheckFn)(BattleEventItem *item, BtlServerFlow *flow, u32 factorType, u32 event, u16 subId,
                                       u8 monId);
// Called with the item, the server flow, the item's mon and its work
typedef void (*BattleEventHandlerFn)(BattleEventItem *item, BtlServerFlow *flow, u8 monId, s32 *work);

struct BattleEventHandlerEntry {
    u32 event;
    BattleEventHandlerFn handler;
};

// A registered handler table for an ability, item, move or other effect, 0x3c bytes; 0x84 are pooled
struct BattleEventItem {
    BattleEventItem *prev;
    BattleEventItem *next;
    const BattleEventHandlerEntry *handlers;
    BattleEventSkipCheckFn skipCheck;
    u32 factorType;
    // The priority level (7 minus the one it was added with) in the top byte, then the sub priority
    u32 priority;
    // The event call depth it was added at, which it isn't called at; its handlers aren't called while this is set
    // and deeper than the call
    u32 callDepth : 16;
    u32 numHandlers : 8;
    // Set while one of its handlers runs
    u32 running : 1;
    // Set until the turn's end, which BattleEvent_ClearTurnPause clears
    u32 unk25 : 1;
    u32 tempItem : 1;
    // Set when it was removed while running, to remove it once its handler returns
    u32 removeReserved : 1;
    // Lets its handlers be called again while one runs
    u32 recallEnable : 1;
    u32 active : 1;
    // Set while its mon is rotated out
    u32 sleeping : 1;
    u32 unk31 : 1;
    u32 work[7];
    u16 subId;
    u8 monId;
    u8 dependMonId;
};

// The event variables that handlers read and rewrite, a stack of frames that BattleEventVar_Push starts and
// BattleEventVar_Pop ends; keys[] holds 1 at the start of each frame
typedef struct {
    u32 sp;
    u16 keys[0x60];
    // Numbers, but for variable 0x3f, which carries the address of the BattleHandlerString that the handlers write
    // their message to (BattleEventVar_SetConstValue)
    sptr values[0x60];
    s32 maxValues[0x60];
    s32 minValues[0x60];
    // 0, or 1 to be rewritten once (then 2), 3 to be multiplied and clamped, 4 constant
    u8 kinds[0x60];
} BattleEventVarStack;

void BattleEvent_Init(void);
void BattleEvent_ClearTurnPause(void);
void BattleEvent_DebugPrintFactorList(void);
BattleEventItem *BattleEvent_AddItem(u32 factorType, u16 subId, u32 priority, u32 subPriority, u8 monId,
                                     const BattleEventHandlerEntry *handlers, u16 numHandlers);
void BattleEvent_RemoveIsolatedItems(void);
BOOL BattleEvent_CheckSkip(BtlServerFlow *flow, BattleEventItem *item, u32 event);
BattleEventItem *BattleEvent_GetNextItem(BattleEventItem *item);
void BattleEvent_PausePersistentMoveItems(u8 monId);
void BattleEvent_ResumeMoveItems(u8 monId);
BattleEventItem *BattleEvent_AllocItem(void);
void BattleEvent_FreeItem(BattleEventItem *item);
void BattleEvent_ClearItem(BattleEventItem *item);
void BattleEventVar_Init(void);
void BattleEventVar_CheckReset(void);
// The line is the caller's source line, which they ignore
void BattleEventVar_Push(u32 line);
void BattleEventVar_Pop(u32 line);
void BattleEventVar_SetValue(u32 key, s32 value);
// A constant for the handlers: a number, or for variable 0x3f the address of the message's BattleHandlerString
void BattleEventVar_SetConstValue(u32 key, sptr value);
void BattleEventVar_SetRewriteOnceValue(u32 key, s32 value);
void BattleEventVar_SetMulValue(u32 key, s32 value, s32 minValue, s32 maxValue);
BOOL BattleEventVar_GetValueIfExist(u16 key, s32 *value);
u32 BattleEventVar_FindSetIndex(BattleEventVarStack *vars, u32 key);
s32 BattleEventVar_FindIndex(BattleEventVarStack *vars, u16 key);
s32 BattleEventVar_ClampValue(BattleEventVarStack *vars, u32 index, s32 value);
// Whether the item's handlers are skipped for now
BOOL MoveEventItem_IsPersistent(BattleEventItem *item, s32 *work);

void BattleEventItem_ConvertToIsolated(BattleEventItem *item);
BOOL BattleEventItem_IsIsolated(BattleEventItem *item);
u16 BattleEventItem_GetSubID(BattleEventItem *item);
u8 HandlerGetMainModule(BattleEventItem *handler);
u32 BattleEventItem_GetWorkValue(BattleEventItem *item, u32 index);
void BattleEventItem_SetTempItemFlag(BattleEventItem *item);
void BattleEventItem_SetRecallEnable(BattleEventItem *item);
void BattleEventItem_SetWorkValue(BattleEventItem *item, u8 index, u32 value);
void BattleEvent_CallHandlersCore(BtlServerFlow *flow, u32 event, u32 factorType, BOOL checkSkip);
void BattleEvent_CallFactorHandlers(BtlServerFlow *flow, u32 event, u32 factorType);
void BattleEvent_CallHandlersSub(BtlServerFlow *flow, u32 event, u32 factorType, BOOL checkSkip);
void BattleEvent_ForceCallHandlers(BtlServerFlow *flow, u32 event);
void BattleEvent_CallHandlers(BtlServerFlow *flow, u32 event);
void BattleEventItem_AttachSkipCheckHandler(BattleEventItem *item, BattleEventSkipCheckFn handler);
void BattleEventItem_DetachSkipCheckHandler(BattleEventItem *item);
BattleEventItem *BattleEvent_SeekItem(u32 type, u32 monId);
void BattleEventItem_Remove(BattleEventItem *item);
void BattleEvent_ItemRotationSleep(u8 monId, u32 factorType);
BOOL BattleEvent_ItemRotationWake(u8 monId, u32 factorType);
// The variable's value in the current frame, or 0 when the frame doesn't set it: a number, or for variable 0x3f the
// address of the message's BattleHandlerString
uptr BattleEventVar_GetValue(u32 key);
BOOL BattleEventVar_RewriteValue(u16 key, s32 value);
void BattleEventVar_MulValue(u16 key, s32 value);

#endif // POKEBW2_BATTLE_BTL_EVENT_H
