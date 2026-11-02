#ifndef POKEBW2_BATTLE_BTL_EVENT_H
#define POKEBW2_BATTLE_BTL_EVENT_H

#include "types.h"
#include "struct_decls.h"

// The kinds of effect a BattleEventItem's handlers belong to (BattleEvent_AddItem): a move, a position, a side and the
// field effect, an ability and a held item; an isolated item, whose effect is gone, and every kind, for the calls
enum {
    BTL_EVENT_FACTOR_MOVE,
    BTL_EVENT_FACTOR_POS,
    BTL_EVENT_FACTOR_SIDE,
    BTL_EVENT_FACTOR_FIELD,
    BTL_EVENT_FACTOR_ABILITY,
    BTL_EVENT_FACTOR_ITEM,
    BTL_EVENT_FACTOR_ISOLATED,
    BTL_EVENT_FACTOR_ALL,
};

// The events whose handlers the server flow calls (BattleEvent_CallHandlers), each named after the ServerEvent_ or
// ServerControl_ function of btl_server_flow.c that calls it and the handlers registered for it. The ones without a
// name stay numbers: 0 and 5, which nothing calls, 0x22 and 0x23, called around the move's message, 0x7d, which has no
// handlers, and 0x82 to 0x87, the steps of ServerControl_DamageProcessEnd
enum {
    BTL_EVENT_ACTION_START = 0x01,
    BTL_EVENT_ACTION_END = 0x02,
    BTL_EVENT_MOVE_SEQ_START = 0x03,
    BTL_EVENT_MOVE_SEQ_END = 0x04,
    BTL_EVENT_CHECK_DELAY_MOVE = 0x06,
    BTL_EVENT_DELAY_MOVE_DECIDE = 0x07,
    BTL_EVENT_STEAL_MOVE = 0x08,
    BTL_EVENT_REFLECT_MOVE = 0x09,
    BTL_EVENT_CHECK_SURE_ESCAPE = 0x0b,
    BTL_EVENT_CHECK_ESCAPE_BLOCKED = 0x0c,
    BTL_EVENT_ESCAPE_MESSAGE = 0x0d,
    BTL_EVENT_CHECK_SLEEP_PREVENTED = 0x0e,
    BTL_EVENT_CHECK_SPECIAL_PRIORITY = 0x0f,
    BTL_EVENT_GET_MOVE_PRIORITY = 0x11,
    BTL_EVENT_CHECK_FLOATING = 0x12,
    BTL_EVENT_CALC_SPEED = 0x13,
    BTL_EVENT_BEFORE_FIGHT = 0x15,
    BTL_EVENT_GET_CALLED_MOVE = 0x18,
    BTL_EVENT_SET_MOVE_MESSAGE = 0x19,
    BTL_EVENT_CHECK_MOVE_STEAL = 0x1a,
    BTL_EVENT_FLOATING_IMMUNE = 0x1b,
    BTL_EVENT_SKIP_ACCURACY_CHECK = 0x1c,
    BTL_EVENT_CAN_USE_MOVE_ASLEEP = 0x1d,
    BTL_EVENT_CHECK_MOVE_FAIL = 0x1e,
    BTL_EVENT_CHECK_MOVE_FAIL_LATE = 0x1f,
    BTL_EVENT_MOVE_EXECUTE_FAIL = 0x21,
    BTL_EVENT_MOVE_EXECUTE_START = 0x24,
    BTL_EVENT_MOVE_EXECUTE_EFFECTIVE = 0x25,
    BTL_EVENT_MOVE_EXECUTE_NO_EFFECT = 0x26,
    BTL_EVENT_MOVE_EXECUTE_END = 0x27,
    BTL_EVENT_GET_MOVE_PARAM = 0x28,
    BTL_EVENT_DECIDE_MOVE_TARGET = 0x29,
    BTL_EVENT_BAIT_MOVE_TARGET = 0x2a,
    // ServerControl_RemoveNoEffectTargets' checks: the first skips sure hits, the third comes after Protect
    BTL_EVENT_CHECK_NO_EFFECT_1 = 0x2b,
    BTL_EVENT_CHECK_NO_EFFECT_2 = 0x2c,
    BTL_EVENT_CHECK_NO_EFFECT_3 = 0x2d,
    BTL_EVENT_CHECK_PROTECT_BREAK = 0x2e,
    BTL_EVENT_CHECK_DAMAGE_TO_RECOVER = 0x30,
    BTL_EVENT_APPLY_DAMAGE_TO_RECOVER = 0x31,
    BTL_EVENT_CHECK_ALWAYS_HIT = 0x32,
    BTL_EVENT_CHECK_HIT = 0x33,
    BTL_EVENT_GET_MOVE_ACCURACY = 0x34,
    BTL_EVENT_CHECK_MULTIHIT_HITS = 0x35,
    BTL_EVENT_CHECK_CRITICAL = 0x36,
    BTL_EVENT_MOVE_BASE_POWER = 0x37,
    BTL_EVENT_MOVE_POWER = 0x38,
    BTL_EVENT_DECIDE_ATTACK_STAT = 0x39,
    BTL_EVENT_DECIDE_DEFENSE_STAT = 0x3a,
    BTL_EVENT_ATTACK_POWER = 0x3b,
    BTL_EVENT_TARGET_DEFENSE = 0x3c,
    BTL_EVENT_CHECK_TYPE_IMMUNITY_ENABLED = 0x3d,
    BTL_EVENT_CHECK_TYPE_EFFECTIVENESS = 0x3e,
    BTL_EVENT_CHECK_SAME_TYPE_BONUS = 0x40,
    BTL_EVENT_SAME_TYPE_BONUS = 0x41,
    BTL_EVENT_BEFORE_DAMAGE_DISPLAY = 0x44,
    BTL_EVENT_DAMAGE_DETERMINE = 0x45,
    BTL_EVENT_FIXED_DAMAGE = 0x46,
    BTL_EVENT_CALC_DAMAGE = 0x47,
    BTL_EVENT_CALC_DAMAGE_END = 0x48,
    // ServerEvent_DamageReaction's three, in order
    BTL_EVENT_DAMAGE_REACTION_PRE = 0x4a,
    BTL_EVENT_DAMAGE_REACTION = 0x4b,
    BTL_EVENT_DAMAGE_REACTION_POST = 0x4c,
    BTL_EVENT_AFTER_DAMAGE = 0x4d,
    BTL_EVENT_CALC_PP_DECREMENT = 0x4e,
    BTL_EVENT_DECREMENT_PP = 0x4f,
    BTL_EVENT_CALC_RECOIL = 0x50,
    BTL_EVENT_STAT_CHANGE_CHANCE = 0x51,
    BTL_EVENT_INTERRUPT_SWITCH = 0x53,
    BTL_EVENT_SWITCH_OUT = 0x54,
    BTL_EVENT_MEMBER_IN = 0x55,
    BTL_EVENT_MEMBER_IN_PREV = 0x56,
    BTL_EVENT_MEMBER_IN_COMPLETE = 0x57,
    BTL_EVENT_ROTATION_IN = 0x58,
    BTL_EVENT_MOVE_STAT_CHANGE_VALUE = 0x59,
    BTL_EVENT_STAT_CHANGE_VALUE = 0x5a,
    BTL_EVENT_CHECK_STAT_CHANGE = 0x5b,
    BTL_EVENT_STAT_CHANGE_FAILED = 0x5c,
    BTL_EVENT_STAT_CHANGE_APPLIED = 0x5d,
    BTL_EVENT_MOVE_STAT_CHANGE_APPLIED = 0x5e,
    BTL_EVENT_DECIDE_SPECIAL_MOVE_CONDITION = 0x60,
    BTL_EVENT_ADD_MOVE_CONDITION_STRING = 0x61,
    BTL_EVENT_MOVE_CONDITION_CONTINUE = 0x62,
    BTL_EVENT_CHECK_MOVE_ADD_CONDITION = 0x64,
    BTL_EVENT_MOVE_CONDITION_CHECK_FAIL = 0x65,
    BTL_EVENT_ADD_CONDITION_FAILED = 0x67,
    BTL_EVENT_CONDITION_CONFIRMED = 0x68,
    BTL_EVENT_MOVE_STATUS_CONFIRMED = 0x69,
    BTL_EVENT_GASTRO_ACID_CONFIRMED = 0x6a,
    BTL_EVENT_CONDITION_DAMAGE = 0x6b,
    BTL_EVENT_GET_FLINCH_CHANCE = 0x6c,
    BTL_EVENT_CHECK_FLINCH = 0x6d,
    BTL_EVENT_FLINCH_FAIL = 0x6e,
    BTL_EVENT_CHECK_OHKO_HIT = 0x70,
    BTL_EVENT_USE_ITEM = 0x72,
    BTL_EVENT_USE_TEMP_ITEM = 0x73,
    BTL_EVENT_CHECK_ENDURE = 0x74,
    BTL_EVENT_ENDURE_USED = 0x75,
    // The handlers' steps of the turn's end: after the weather, after the side and field effects, and last
    BTL_EVENT_TURN_CHECK_BEGIN = 0x76,
    BTL_EVENT_TURN_CHECK_END = 0x77,
    BTL_EVENT_TURN_CHECK_DONE = 0x78,
    BTL_EVENT_NOTIFY_AIR_LOCK = 0x79,
    BTL_EVENT_GET_WEATHER = 0x7a,
    BTL_EVENT_GET_WEIGHT_RATIO = 0x7b,
    BTL_EVENT_MOVE_WEATHER_TURNS = 0x7c,
    BTL_EVENT_AFTER_WEATHER_CHANGE = 0x7e,
    BTL_EVENT_CHECK_WEATHER_DAMAGE = 0x7f,
    BTL_EVENT_CHECK_SIMPLE_DAMAGE_ENABLED = 0x80,
    BTL_EVENT_DAMAGE_PROCESS_START = 0x81,
    BTL_EVENT_DAMAGE_PROCESS_END = 0x88,
    BTL_EVENT_CHANGE_ABILITY_BEFORE = 0x89,
    BTL_EVENT_CHANGE_ABILITY_AFTER = 0x8a,
    BTL_EVENT_CHECK_FORCE_SWITCH_BLOCKED = 0x8b,
    BTL_EVENT_CALC_DRAIN = 0x8c,
    BTL_EVENT_CALC_DRAIN_END = 0x8d,
    BTL_EVENT_CALC_MOVE_HEAL = 0x8f,
    BTL_EVENT_CHECK_HELD_ITEM_FAIL = 0x90,
    BTL_EVENT_CHECK_ITEM_REACTION = 0x91,
    BTL_EVENT_CHECK_CHARGE_FAIL = 0x93,
    BTL_EVENT_CHECK_CHARGE_SKIP = 0x94,
    BTL_EVENT_CHARGE_START = 0x95,
    BTL_EVENT_CHARGE_START_FIXED = 0x96,
    BTL_EVENT_CHARGE_SKIP_FIXED = 0x97,
    BTL_EVENT_CHARGE_RELEASE = 0x98,
    BTL_EVENT_CHECK_SEMI_INVULN_AVOID = 0x99,
    BTL_EVENT_CHECK_ITEM_SET = 0x9a,
    BTL_EVENT_ITEM_SET_FAILED = 0x9b,
    BTL_EVENT_ITEM_SET_DECIDE = 0x9c,
    BTL_EVENT_ITEM_SET_FIXED = 0x9d,
    BTL_EVENT_FIELD_EFFECT_MOVE = 0x9e,
    BTL_EVENT_CHECK_SIDE_EFFECT_PARAM = 0x9f,
    BTL_EVENT_UNCATEGORIZED_MOVE = 0xa0,
    BTL_EVENT_UNCATEGORIZED_MOVE_NO_TARGET = 0xa1,
    BTL_EVENT_REWRITE_MOVE_TYPE = 0xa2,
    BTL_EVENT_BEFORE_FAINT = 0xa3,
    BTL_EVENT_AFTER_MOVE = 0xa4,
};

// The keys of the event variables (BattleEventVar_SetValue and the others), each named after what the ServerEvent_
// functions of btl_server_flow.c that set it put in it and what the handlers read from it or write to it. Mon IDs are
// 0x1f for none. Left as numbers: 0x36 (CalcRecoil's bonus, which no handler touches), 0x3e (MoveExecuteStart's
// result, 1 or 2 from Charge and Bide), 0x50 (DecideMoveTarget's, never read), and the flags that mean something else
// in each event: 0x42 (whether a semi-invulnerable target avoids the move, whether each hit of a multi-hit move is
// checked), 0x45 (a critical hit, a doubled flinch chance) and 0x4b (ignoring the evasion stage, making the move
// typeless, ignoring an immunity)
enum {
    // An empty key and the start of a frame (BattleEventVar_Push), the stack's own
    BTL_EVAR_NONE = 0x00,
    BTL_EVAR_FRAME_START = 0x01,
    // The mon the event is about, the move's user and its target
    BTL_EVAR_MON = 0x02,
    BTL_EVAR_ATTACKER = 0x03,
    BTL_EVAR_DEFENDER = 0x04,
    // How many targets follow, then each target's mon ID
    BTL_EVAR_TARGET_COUNT = 0x05,
    BTL_EVAR_TARGET_1 = 0x06,
    BTL_EVAR_TARGET_2 = 0x07,
    BTL_EVAR_TARGET_3 = 0x08,
    BTL_EVAR_TARGET_4 = 0x09,
    BTL_EVAR_TARGET_5 = 0x0a,
    BTL_EVAR_TARGET_6 = 0x0b,
    // The action started or ended (BATTLE_ACTION_*)
    BTL_EVAR_ACTION = 0x0c,
    // The position a delayed or called move aims at, and the one the calling move aimed at
    BTL_EVAR_TARGET_POS = 0x0d,
    BTL_EVAR_ORIGINAL_TARGET_POS = 0x0e,
    BTL_EVAR_OLD_ABILITY = 0x0f,
    BTL_EVAR_NEW_ABILITY = 0x10,
    // 1, or 2 to act first in the priority bracket (Quick Claw, Custap Berry), 0 to act last (Stall, Lagging Tail)
    BTL_EVAR_SPECIAL_PRIORITY = 0x11,
    BTL_EVAR_MOVE = 0x12,
    // The move whose animation plays instead (MoveExecuteStart), 0 for the move's own
    BTL_EVAR_ANIM_MOVE = 0x13,
    // The move that called the move (Metronome, Sleep Talk and the others), or the move itself
    BTL_EVAR_ORIGINAL_MOVE = 0x14,
    BTL_EVAR_DEFENDER_TYPE = 0x15,
    BTL_EVAR_MOVE_TYPE = 0x16,
    // The slot of the move whose PP is taken
    BTL_EVAR_MOVE_INDEX = 0x17,
    BTL_EVAR_MOVE_PRIORITY = 0x18,
    // The serial of the stat changes of one move (ServerControl_NextSerial), so that a guard shows its message once
    BTL_EVAR_STAT_CHANGE_SERIAL = 0x19,
    BTL_EVAR_MOVE_CATEGORY = 0x1a,
    // How the move picks its targets, the move data's target type
    BTL_EVAR_TARGET_TYPE = 0x1b,
    BTL_EVAR_USER_TYPE = 0x1c,
    BTL_EVAR_CONDITION = 0x1d,
    // A BattleCondition's raw value
    BTL_EVAR_CONDITION_PARAM = 0x1e,
    // A MoveStatChangeStat, and how many stages it changes by; also the PP taken and the HP drained
    BTL_EVAR_STAT = 0x1f,
    BTL_EVAR_AMOUNT = 0x20,
    // The target's semi-invulnerable state (BattleMon_GetSemiInvulnFlag)
    BTL_EVAR_SEMI_INVULN_STATE = 0x21,
    // Why a move fails, 0 when it doesn't
    BTL_EVAR_FAIL_CAUSE = 0x22,
    // The turns an item adds to a weather move's
    BTL_EVAR_WEATHER_TURNS = 0x24,
    // The move's own flinch chance, and the chance of an effect: of a condition, a stat drop or a flinch
    BTL_EVAR_MOVE_FLINCH_CHANCE = 0x25,
    BTL_EVAR_CHANCE = 0x26,
    BTL_EVAR_ACCURACY_STAGE = 0x27,
    BTL_EVAR_EVASION_STAGE = 0x28,
    BTL_EVAR_MAX_HITS = 0x29,
    BTL_EVAR_HIT_COUNT = 0x2a,
    BTL_EVAR_ACCURACY = 0x2b,
    BTL_EVAR_CRITICAL_STAGE = 0x2c,
    BTL_EVAR_ITEM = 0x2d,
    // What made the item check whether it reacts (ServerControl_CheckItemReaction)
    BTL_EVAR_ITEM_REACTION = 0x2e,
    BTL_EVAR_SPEED = 0x2f,
    BTL_EVAR_POWER = 0x30,
    BTL_EVAR_POWER_RATIO = 0x31,
    BTL_EVAR_DAMAGE = 0x32,
    BTL_EVAR_ATTACK_POWER = 0x33,
    BTL_EVAR_DEFENSE = 0x34,
    // A fixed-point multiplier of the value the event calculates
    BTL_EVAR_RATIO = 0x35,
    // The damage of a move that does a fixed amount, 0 for the calculated damage
    BTL_EVAR_FIXED_DAMAGE = 0x37,
    BTL_EVAR_EFFECTIVENESS = 0x38,
    BTL_EVAR_WEATHER = 0x39,
    // What makes the target hang on with 1 HP, 0 for nothing
    BTL_EVAR_ENDURE_CAUSE = 0x3a,
    // The mon whose attack stat is used (Foul Play's target), 0x1f for the attacker
    BTL_EVAR_ATTACK_STAT_MON = 0x3b,
    // The defense stat a move's category picks, and how many times it is swapped for the other (Psyshock, Wonder
    // Room): an odd count swaps it
    BTL_EVAR_DEFENSE_STAT = 0x3c,
    BTL_EVAR_DEFENSE_SWAP_COUNT = 0x3d,
    // A BattleHandlerString for the handlers' message
    BTL_EVAR_STRING = 0x3f,
    // Set when the move has no effect on the target (ServerEvent_CheckNoEffect)
    BTL_EVAR_NO_EFFECT_FLAG = 0x40,
    // Set to make what the event checks fail, be blocked or be cancelled; CAN_USE_MOVE_ASLEEP's yes
    BTL_EVAR_FAIL_FLAG = 0x41,
    BTL_EVAR_SAME_TYPE_BONUS_FLAG = 0x44,
    // Set when the move hit the target's substitute
    BTL_EVAR_SUBSTITUTE_FLAG = 0x46,
    // Set when the target blocks the move's added effects (Shield Dust)
    BTL_EVAR_EFFECT_BLOCKED_FLAG = 0x47,
    // Set to skip the event that follows (Sheer Force, ServerEvent_DamageProcessEndSub)
    BTL_EVAR_SKIP_EVENT_FLAG = 0x48,
    // Set to reverse the speed order (Trick Room)
    BTL_EVAR_TRICK_ROOM_FLAG = 0x4a,
    // Set to make the move's effectiveness normal whatever the types (ServerEvent_CheckTypeEffectiveness)
    BTL_EVAR_FORCE_NORMAL_FLAG = 0x4c,
    // Set for the damage of a delayed move (BattleHandler_DelayMoveDamage: Future Sight, Doom Desire), which Shell
    // Bell, Red Card and Eject Button ignore
    BTL_EVAR_DELAYED_FLAG = 0x4d,
    // Set when the move was reflected back by Magic Coat or Magic Bounce, which don't reflect it again
    BTL_EVAR_REFLECTED_FLAG = 0x4e,
    // Set when the handler showed why the charge failed, so that the move's fail message isn't shown (Sky Drop)
    BTL_EVAR_FAIL_MESSAGE_FLAG = 0x4f,
    // The event's own yes or no, which each ServerEvent_ function reads as it says
    BTL_EVAR_FLAG = 0x51,
    // A side effect's side and effect (ServerEvent_CheckSideEffectParam)
    BTL_EVAR_SIDE = 0x52,
    BTL_EVAR_SIDE_EFFECT = 0x53,
};

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
    // Set while its move's effect is paused (BattleEvent_PausePersistentMoveItems), until BattleEvent_ResumeMoveItems
    // or the turn's end (BattleEvent_ClearTurnPause)
    u32 paused : 1;
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
