#ifndef POKEBW2_BATTLE_BTL_HANDLER_H
#define POKEBW2_BATTLE_BTL_HANDLER_H

#include "types.h"
#include "battle/btl_action.h"
#include "battle/btl_action_order.h"
#include "battle/btl_faintrec.h"
#include "battle/btl_handler_work.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_pospoke.h"
#include "battle/btl_server.h"
#include "battle/btl_wazarec.h"
#include "struct_decls.h"

// The start of every handler command's parameters
typedef struct {
    u32 command : 8;
    u32 monId : 5;
    // Rounded up to 4 bytes
    u32 size : 10;
    u32 unk23 : 1;
    // Skips the command when the previous one failed
    u32 checkPrevResult : 1;
    // Skips the command when the mon has fainted
    u32 checkFainted : 1;
    u32 unk26 : 1;
    u32 unk27 : 5;
} BattleHandlerHeader;

struct BattleHandlerPopupParam {
    u32 unk00 : 8;
    u32 monId : 5;
    u32 unk13 : 19;
};

struct BattleHandlerString {
    u16 message;
    union {
        u16 flags;
        struct {
            u16 enabled : 8;
            u16 count : 7;
            u16 hasSound : 1;
        };
    };
    u32 args[8];
    u32 soundEffect;
};

// A move's parameters as the events changed them
typedef struct {
    u16 move;
    u16 originalMove;
    PokeTypePair userType;
    u8 type;
    u8 unk07;
    u32 category;
    // How the move picks its targets, the move data's target type
    u32 targetType;
    union {
        u32 raw;
        struct {
            u32 unk0 : 1;
            u32 unk1 : 31;
        };
    } flags;
} BtlFlowMoveParam;

// One target's result from the damage calculation of a move
typedef struct {
    u16 damage;
    u16 monId : 5;
    u16 effectiveness : 4;
    u16 unk9 : 4;
    u16 critical : 1;
    u16 fixedDamage : 1;
    u16 substitute : 1;
} BtlFlowDamageEntry;

typedef struct {
    u8 count;
    u8 substituteCount;
    u8 total;
    u8 unk03;
    BtlFlowDamageEntry entries[6];
} BtlFlowDamageList;

// Where the move goes, and its effect
struct BattleMoveEffectState {
    u16 unk00;
    u8 pos1;
    u8 pos2;
    u8 index;
    u8 enabled : 1;
    u8 unk05_1 : 1;
    u8 unk05_2 : 6;
};

// How many times a move hits, and how its hits are checked
typedef struct {
    u8 count;
    u8 unk01;
    u8 unk02;
    u8 unk03;
    u8 unk04;
    u8 unk05;
} BtlFlowHitWork;

// The experience one party mon gets for a faint
typedef struct {
    u32 exp;
    // Set when a traded mon or a Lucky Egg raised it
    u8 boosted;
    // The effort values it gained
    u8 hp;
    u8 attack;
    u8 defense;
    u8 speed;
    u8 spAttack;
    u8 spDefense;
    u8 unk0B;
} BtlFlowExpEntry;

typedef struct {
    u8 unk000[0x220];
    u32 unk220;
} BtlFlowUnk1B54;

// Mons that react to a move, with their targets
typedef struct {
    u8 count;
    u8 monIds[6];
    u8 targets[6];
    u8 unk0D[6];
} BtlFlowReactionList;

// Client IDs the flow collected, such as those that must switch in
typedef struct {
    u8 count;
    u8 clientIds[4];
    u8 unk05;
} BtlFlowClientList;

// One level of the flow's work, kept for each level of nested handler calls; the flow's pointers point into the
// current one
typedef struct {
    // Mon sets in overlay 169's format
    u8 monSets[7][0x48];
    BattleMoveEffectState moveEffect;
    BtlFlowMoveParam moveParams[2];
    BtlFlowHitWork hitWork;
    BtlFlowReactionList reactionLists[2];
    BtlFlowDamageList damageLists[2];
    u8 unk28C;
    u8 unk28D;
    u8 unk28E;
} BtlFlowWorkFrame;

struct BtlServerFlow {
    BtlServer *server;
    BtlMainModule *mainModule;
    BtlPokeCon *pokeCon;
    BtlServerCmdQueue *queue;
    u32 unk10;
    u32 unk14;
    u32 unk18;
    BtlWazaRec wazaRec;
    BtlFaintRec faintRec;
    ArcTool *unk4A4;
    BattleMoveEffectState *moveEffect;
    BtlFlowReactionList *unk4AC;
    BtlFlowReactionList *unk4B0;
    BtlFlowHitWork *unk4B4;
    BtlClientIDList clientIdList;
    BattleMonLevelUp levelUp;
    BtlFlowClientList unk4CE;
    u8 unk4D4[0x2a0];
    u32 unk774;
    u32 unk778;
    u8 unk77C;
    u8 unk77D;
    u8 unk77E;
    u8 unk77F;
    HeapID heapId;
    u8 actionOrderCount;
    u8 unk783;
    u8 unk784;
    u8 unk785;
    u8 unk786;
    u8 unk787;
    // The mons that strike a mon switching out, such as with Pursuit
    u8 interruptCount;
    u8 unk789;
    u8 unk78A_0 : 1;
    u8 unk78A_1 : 1;
    u8 unk78A_2 : 1;
    u8 unk78A_3 : 1;
    u8 unk78A_4 : 1;
    u8 unk78A_5 : 1;
    u8 unk78A_6 : 1;
    u8 unk78A_7 : 1;
    u8 interruptMonIds[6];
    u8 unk791[0x18];
    // Per mon ID, cleared when the mon is revived
    u8 unk7A9[24];
    u8 unk7C1[24];
    u8 unk7D9[4];
    u8 unk7DD[3];
    ActionOrderEntry actionOrder[6];
    // Where an entry is kept while the order is reshuffled
    ActionOrderEntry tempEntry;
    // Two mon sets in overlay 169's format: the move's targets, and a copy
    void *unk850;
    void *unk854;
    void *unk858;
    void *unk85C;
    void *unk860;
    void *unk864;
    void *unk868;
    BtlFlowDamageList *unk86C;
    BtlFlowDamageList *unk870;
    BtlFlowWorkFrame frames[7];
    u32 frameDepth;
    // The mons that came in this turn, in overlay 169's format
    u8 unk1A68[0x48];
    BtlFlowMoveParam *unk1AB0;
    BtlFlowMoveParam *unk1AB4;
    BtlPosPoke posPoke;
    u8 unk1AD0[0x14];
    BattleHandlerString message;
    BtlFlowExpEntry expEntries[6];
    BtlFlowUnk1B54 unk1B54;
    BtlActionState actionState;
    u8 unk1F70[8];
    u16 unk1F78;
    u8 unk1F7A[2];
    u32 unk1F7C;
    // How often the player's mons and their opponents hit each other for no effect, super or not very effectively
    u16 unk1F80[6];
    u32 unk1F8C[24];
    u8 unk1FEC[4];
    u8 unk1FF0[0x140];
    u32 unk2130;
};

struct BattleHandlerAbilityChangeParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 10;
    u32 popup : 1;
    u32 unk24 : 8;
    u16 ability;
    u8 targetIndex;
    u8 force;
    u8 unk08;
    u8 unk09[3];
    BattleHandlerString string;
};

struct BattleHandlerAddSideEffectParam {
    BattleHandlerHeader header;
    u32 effect;
    BattleCondition cont;
    u8 side;
    BattleHandlerString string;
};

// A set of flags whose first byte is its size in bytes
static inline BOOL BattleHandler_IsFlagSet(const u8 *flags, u32 index) {
    u8 byte = (index >> 3) + 1;
    u8 bit = index & 7;

    if (byte < flags[0]) {
        if (flags[byte] & (1 << bit)) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

// Clears a set of flags whose first byte is its size in bytes
static inline void BattleHandler_InitFlags(u8 *flags, u32 size) {
    u32 i;

    flags[0] = size;
    for (i = 1; i < size; i++) {
        flags[i] = 0;
    }
}

static inline void BattleHandler_SetFlag(u8 *flags, u32 index) {
    u8 byte = (index >> 3) + 1;
    u8 bit = index & 7;

    if (byte < flags[0]) {
        flags[byte] |= 1 << bit;
    }
}

struct BattleHandlerRemoveSideEffectParam {
    BattleHandlerHeader header;
    u8 effects[3];
    u8 side;
};

struct BattleHandlerPosEffectParam {
    BattleHandlerHeader header;
    u32 effect;
    u8 pos;
    u32 args[4];
    u8 argCount;
};

struct BattleHandlerUpdateMoveParam {
    BattleHandlerHeader header;
    u8 monIndex;
    u8 slot;
    u8 maxPP;
    u8 updateCurrent;
    u16 move;
};

struct BattleHandlerForceSwitchParam {
    BattleHandlerHeader header;
    u16 effect;
    u8 targetIndex;
    u8 forced : 4;
    u8 ignoreLevel : 4;
    BattleHandlerString string;
};

struct BattleHandlerEffectAtPosParam {
    BattleHandlerHeader header;
    u16 effect;
    u8 pos1;
    u8 pos2;
    u16 reserve;
    u8 reserved;
    u8 hideMessageWindow;
    BattleHandlerString string;
};

struct BattleHandlerDelayMoveDamageParam {
    BattleHandlerHeader header;
    u8 attackerIndex;
    u8 targetIndex;
    u16 move;
};

struct BattleHandlerSetStatStageParam {
    BattleHandlerHeader header;
    u8 monIndex;
    u8 attack;
    u8 defense;
    u8 spAttack;
    u8 spDefense;
    u8 speed;
    u8 accuracy;
    u8 evasion;
};

// The stats it sets, as with Power Split and Guard Split
struct BattleHandlerSetStatusParam {
    BattleHandlerHeader header;
    u16 attack;
    u16 defense;
    u16 spAttack;
    u16 spDefense;
    u16 speed;
    u8 monIndex;
    u8 setAttack : 1;
    u8 setDefense : 1;
    u8 setSpAttack : 1;
    u8 setSpDefense : 1;
    u8 setSpeed : 1;
    u8 unk0f_5 : 3;
    BattleHandlerString string;
};

struct BattleHandlerAddConditionParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 10;
    u32 popup : 1;
    u32 unk24 : 8;
    u32 condition;
    BattleCondition value;
    u8 showFail;
    // Shows no message of its own when there's no string
    u8 noMessage;
    u8 skipItemReaction;
    u8 targetIndex;
    u8 overwrite;
    BattleHandlerString string;
};
struct BattleHandlerAddFieldEffectParam {
    u32 unk00;
    u32 effect;
    BattleCondition value;
    u8 duration;
    u8 unk0d[3];
    BattleHandlerString string;
};

struct BattleHandlerBatonPassParam {
    u32 unk00;
    u8 sourceMonIndex;
    u8 targetMonIndex;
};

struct BattleHandlerChangeFormParam {
    u32 unk00 : 23;
    u32 popup : 1;
    u32 unk18 : 8;
    u8 monIndex;
    u8 form;
    u8 unk06[2];
    BattleHandlerString string;
};

struct BattleHandlerChangeHPParam {
    BattleHandlerHeader header;
    u8 count;
    u8 suppress;
    u8 skipReaction;
    u8 monIds[9];
    u32 hpChanges[6];
};

struct BattleHandlerChangeTypeParam {
    u32 unk00;
    u16 type;
    u8 monIndex;
    u8 suppressMessage;
};

struct BattleHandlerChangeWeatherParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 10;
    u32 popup : 1;
    u32 unk24 : 8;
    u8 weather;
    u8 duration;
    u8 notifyAirLock;
    u8 unk07;
    BattleHandlerString string;
};

struct BattleHandlerCheckHeldItemParam {
    BattleHandlerHeader header;
    u8 monIndex;
    u8 unk05[3];
    u32 reaction;
};

struct BattleHandlerConsumeItemParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 19;
    u32 skipDisplay;
    BattleHandlerString string;
};

struct BattleHandlerCureConditionParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 10;
    u32 popup : 1;
    u32 checkPrevResult : 1;
    u32 unk25 : 1;
    u32 unk26 : 6;
    u32 condition;
    u8 monIds[12];
    u8 count;
    u8 useString;
    u8 unk16[2];
    BattleHandlerString string;
};

struct BattleHandlerRecoverHPParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 10;
    u32 popup : 1;
    u32 checkPrevResult : 1;
    u32 unk25 : 7;
    u16 amount;
    u8 targetIndex;
    // Recovers without the checks for Heal Block and the like
    u8 skipCheck;
    BattleHandlerString string;
};

struct BattleHandlerDamageParam {
    u32 unk00 : 8;
    u32 sourceIndex : 5;
    u32 unk13 : 10;
    u32 popup : 1;
    u32 unk24 : 8;
    u16 amount;
    u8 targetIndex;
    u8 checkSemi : 1;
    u8 showViewEffect : 1;
    u8 unkFlags : 6;
    u16 effect;
    u8 effectArg1;
    u8 effectArg2;
    BattleHandlerString string;
};

struct BattleHandlerPPParam {
    BattleHandlerHeader header;
    u8 amount;
    u8 monIndex;
    u8 moveIndex;
    // Changes the PP of the moves the mon has now rather than its original ones, as after Transform
    u8 currentMoves : 1;
    u8 allowFainted : 1;
    u8 unk09 : 6;
    BattleHandlerString string;
};

struct BattleHandlerDrainParam {
    BattleHandlerHeader header;
    u16 amount;
    u8 monIndex;
    u8 sourceIndex;
    BattleHandlerString string;
};

struct BattleHandlerFaintParam {
    u32 unk00;
    u8 monIndex;
    u8 force;
    u8 unk06[2];
    BattleHandlerString string;
};

struct BattleHandlerFlagParam {
    BattleHandlerHeader header;
    u32 flag;
    u8 monIndex;
};

struct BattleHandlerFlinchParam {
    u32 unk00;
    u8 monIndex;
    u8 flag;
};

struct BattleHandlerForceUseItemParam {
    u32 unk00 : 8;
    u32 monIndex2 : 5;
    u32 unk13 : 11;
    u32 checkPrevResult : 1;
    u32 unk25 : 7;
    u8 monIndex;
    u8 unk05;
    u16 item;
};

struct BattleHandlerGravityCheckParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 19;
};

struct BattleHandlerHideTurnParam {
    u32 unk00;
    u8 monIndex;
    u8 unk05[3];
    u32 flag;
    BattleHandlerString string;
};

struct BattleHandlerIllusionBreakParam {
    u32 unk00;
    u8 monIndex;
    u8 unk05[3];
    BattleHandlerString string;
};

struct BattleHandlerInterruptParam {
    u32 unk00;
    union {
        u8 monId;
        u16 moveId;
    };
    u16 unk06;
    BattleHandlerString string;
};

struct BattleHandlerMessageParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 10;
    u32 popup : 1;
    u32 checkPrevResult : 1;
    u32 unk25 : 7;
    BattleHandlerString string;
};

struct BattleHandlerMoveEffectParam {
    BattleHandlerHeader header;
    u8 index;
};

struct BattleHandlerQuitBattleParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 19;
};

struct BattleHandlerRecoverStatStageParam {
    u32 unk00;
    u8 monIndex;
};

struct BattleHandlerRemoveFieldEffectParam {
    u32 unk00;
    u32 effect;
};

struct BattleHandlerResetStatStageParam {
    u32 unk00;
    u8 count;
    u8 monIndices[6];
};

struct BattleHandlerReviveParam {
    u32 unk00;
    u8 monIndex;
    u8 unk05;
    u16 amount;
    BattleHandlerString string;
};

struct BattleHandlerSetCounterParam {
    BattleHandlerHeader header;
    u8 monIndex;
    u8 counter;
    u8 value;
};

struct BattleHandlerSetItemParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 10;
    u32 popup : 1;
    u32 unk24 : 8;
    u16 item;
    u8 targetIndex;
    u8 clearConsumed;
    u8 clearOtherConsumed;
    u8 otherIndex;
    u8 unk0a[2];
    BattleHandlerString string;
};

struct BattleHandlerSetWeightParam {
    u32 unk00;
    u8 monIndex;
    u8 unk05;
    u16 weight;
    BattleHandlerString string;
};

struct BattleHandlerStatChangeParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 10;
    u32 popup : 1;
    u32 unk24 : 8;
    u32 stat;
    u32 value;
    s8 change;
    u8 flag;
    u8 unk0e;
    u8 count;
    u8 monIds[8];
    BattleHandlerString string;
};

struct BattleHandlerSwapItemParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 10;
    u32 popup : 1;
    u32 unk24 : 8;
    u8 otherIndex;
    u8 unk05[3];
    BattleHandlerString firstString;
    BattleHandlerString secondString;
    BattleHandlerString thirdString;
};

struct BattleHandlerSwapPokeParam {
    u32 unk00;
    u8 firstMonIndex;
    u8 secondMonIndex;
    u8 unk06[2];
    BattleHandlerString string;
};

struct BattleHandlerSwitchParam {
    BattleHandlerHeader header;
    BattleHandlerString firstString;
    BattleHandlerString secondString;
    u8 monIndex;
    u8 flag;
};

struct BattleHandlerTransformParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 10;
    u32 popup : 1;
    u32 unk24 : 8;
    u8 targetIndex;
    u8 unk05[3];
    BattleHandlerString string;
};

struct BattleHandlerUseHeldItemParam {
    u32 unk00 : 8;
    u32 monIndex : 5;
    u32 unk13 : 19;
    u32 checkFullHp : 1;
    u32 allowFainted : 1;
    u32 unk22 : 30;
};

void BattleHandler_StrClear(BattleHandlerString *string);
BOOL BattleHandler_StrIsEnabled(const BattleHandlerString *string);
void BattleHandler_StrSetup(BattleHandlerString *string, u32 enabled, u16 message);
void BattleHandler_AddArg(BattleHandlerString *string, u32 arg);
void BattleHandler_AddSoundEffect(BattleHandlerString *string, u32 soundEffect);

void *BattleHandler_PushWork(BtlServerFlow *flow, u32 command, u32 monId);
void BattleHandler_PushRun(BtlServerFlow *flow, u32 command, u32 monId);
void BattleHandler_PopWork(BtlServerFlow *flow, void *work);
u32 BattleHandler_Result(BtlServerFlow *handler);
void BattleHandler_Execute(BtlServerFlow *flow, void *work);
u32 func_ov167_021ac450(BtlServerFlow *flow);
u8 func_ov167_021ac7ac(BtlServerFlow *handler, BattleHandlerHeader *param);
u8 BattleHandler_AbilityPopupAdd(BtlServerFlow *handler, BattleHandlerHeader *param);
u8 BattleHandler_RecoverHP(BtlServerFlow *handler, const BattleHandlerRecoverHPParam *param, u16 itemId);
u8 BattleHandler_RecoverPP(BtlServerFlow *handler, const BattleHandlerPPParam *param, u16 itemId);
u8 BattleHandler_CureCondition(BtlServerFlow *handler, const struct BattleHandlerCureConditionParam *param, u16 context);
u8 BattleHandler_AddCondition(BtlServerFlow *handler, const BattleHandlerAddConditionParam *param);
u8 BattleHandler_StatChange(BtlServerFlow *handler, const struct BattleHandlerStatChangeParam *param, u16 context);
u8 BattleHandler_SetStatStage(BtlServerFlow *handler, const BattleHandlerSetStatStageParam *param);
u8 BattleHandler_SetStatus(BtlServerFlow *handler, const BattleHandlerSetStatusParam *param);
u8 BattleHandler_AddSideEffect(BtlServerFlow *handler, const BattleHandlerAddSideEffectParam *param);
u8 BattleHandler_RemoveSideEffectCore(BtlServerFlow *handler, const BattleHandlerRemoveSideEffectParam *param);
u8 func_ov167_021ad564(BtlServerFlow *handler, const BattleHandlerPosEffectParam *param);
u8 BattleHandler_UpdateMove(BtlServerFlow *handler, const BattleHandlerUpdateMoveParam *param);
u8 BattleHandler_DelayMoveDamage(BtlServerFlow *handler, const BattleHandlerDelayMoveDamageParam *param);
u8 BattleHandler_ForceSwitch(BtlServerFlow *handler, const BattleHandlerForceSwitchParam *param);
u8 BattleHandler_EffectAtPos(BtlServerFlow *handler, const BattleHandlerEffectAtPosParam *param);

u8 BattleHandler_SetTurnFlag(BtlServerFlow *handler, BattleHandlerFlagParam *param);
u8 BattleHandler_ResetTurnFlag(BtlServerFlow *handler, BattleHandlerFlagParam *param);
u8 BattleHandler_SetContinueFlag(BtlServerFlow *handler, BattleHandlerFlagParam *param);
u8 BattleHandler_ResetContinueFlag(BtlServerFlow *handler, BattleHandlerFlagParam *param);
u8 BattleHandler_InterruptAction(BtlServerFlow *handler, BattleHandlerInterruptParam *param);
u8 BattleHandler_InterruptMove(BtlServerFlow *handler, BattleHandlerInterruptParam *param);
u8 BattleHandler_SendLast(BtlServerFlow *handler, BattleHandlerInterruptParam *param);
BOOL BattleHandler_SetString(BtlServerFlow *handler, const BattleHandlerString *string);
u8 BattleHandler_AbilityPopupRemove(BtlServerFlow *handler, BattleHandlerPopupParam *param);
u8 BattleHandler_HideTurnCancel(BtlServerFlow *handler, BattleHandlerHideTurnParam *param);
u8 BattleHandler_RemoveMessageWindow(BtlServerFlow *handler, BattleHandlerHeader *header);
u8 BattleHandler_ChangeForm(BtlServerFlow *handler, BattleHandlerChangeFormParam *param);
u8 BattleHandler_Flinch(BtlServerFlow *handler, BattleHandlerFlinchParam *param);
u8 BattleHandler_SetWeight(BtlServerFlow *handler, BattleHandlerSetWeightParam *param);
u8 BattleHandler_Revive(BtlServerFlow *handler, BattleHandlerReviveParam *param);
u8 BattleHandler_SetCounter(BtlServerFlow *handler, BattleHandlerSetCounterParam *param);
u8 BattleHandler_CheckHeldItem(BtlServerFlow *handler, BattleHandlerCheckHeldItemParam *param);
u8 BattleHandler_UseHeldItem(BtlServerFlow *handler, BattleHandlerUseHeldItemParam *param);
u8 BattleHandler_ConsumeItem(BtlServerFlow *handler, BattleHandlerConsumeItemParam *param);
u8 BattleHandler_QuitBattle(BtlServerFlow *handler, BattleHandlerQuitBattleParam *param);
u8 BattleHandler_Switch(BtlServerFlow *handler, BattleHandlerSwitchParam *param);
u8 BattleHandler_AddFieldEffect(BtlServerFlow *handler, BattleHandlerAddFieldEffectParam *param);
u8 BattleHandler_RemoveFieldEffect(BtlServerFlow *handler, BattleHandlerRemoveFieldEffectParam *param);
u8 BattleHandler_RecoverStatStage(BtlServerFlow *handler, BattleHandlerRecoverStatStageParam *param);
u8 BattleHandler_ResetStatStage(BtlServerFlow *handler, BattleHandlerResetStatStageParam *param);
u8 BattleHandler_Message(BtlServerFlow *handler, BattleHandlerMessageParam *param);
u8 BattleHandler_DecrementPP(BtlServerFlow *handler, BattleHandlerPPParam *param, u16 itemId);
u8 BattleHandler_ForceUseItem(BtlServerFlow *handler, BattleHandlerForceUseItemParam *param);
u8 BattleHandler_BatonPass(BtlServerFlow *handler, BattleHandlerBatonPassParam *param);
u8 BattleHandler_IllusionBreak(BtlServerFlow *handler, BattleHandlerIllusionBreakParam *param);
u8 BattleHandler_SwapPoke(BtlServerFlow *handler, BattleHandlerSwapPokeParam *param);
u8 BattleHandler_ChangeWeather(BtlServerFlow *handler, BattleHandlerChangeWeatherParam *param);
u8 BattleHandler_GravityCheck(BtlServerFlow *handler, BattleHandlerGravityCheckParam *param);
u8 BattleHandler_Transform(BtlServerFlow *handler, BattleHandlerTransformParam *param);
u8 BattleHandler_SetItem(BtlServerFlow *handler, BattleHandlerSetItemParam *param);
u8 BattleHandler_SwapItem(BtlServerFlow *handler, BattleHandlerSwapItemParam *param);
u8 BattleHandler_Drain(BtlServerFlow *handler, BattleHandlerDrainParam *param, u16 itemId);
u8 BattleHandler_Damage(BtlServerFlow *handler, BattleHandlerDamageParam *param);
u8 BattleHandler_ChangeHP(BtlServerFlow *handler, BattleHandlerChangeHPParam *param);
u8 BattleHandler_Faint(BtlServerFlow *handler, BattleHandlerFaintParam *param);
u8 BattleHandler_ChangeType(BtlServerFlow *handler, BattleHandlerChangeTypeParam *param);
u8 BattleHandler_AbilityChange(BtlServerFlow *handler, BattleHandlerAbilityChangeParam *param);
BOOL func_ov167_021ad204(u16 species);
u8 HandlerGetAlivePartyCount(BtlServerFlow *handler, u16 code, u8 *monIds);
u8 func_ov167_021ab840(BtlServerFlow *flow, u8 monId);
u8 *func_ov167_021abc60(BtlServerFlow *flow, u32 size);

u8 BattleHandler_SetMoveEffectIndex(BtlServerFlow *handler, BattleHandlerMoveEffectParam *param);
u8 BattleHandler_SetMoveEffectEnable(BtlServerFlow *handler, BattleHandlerHeader *header);

#endif // POKEBW2_BATTLE_BTL_HANDLER_H
