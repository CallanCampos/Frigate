#include "types.h"
#include "battle/btl_field.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_server_flow.h"
#include "battle/btlv_effect.h"
#include "battle/tr_ai.h"
#include "constants/abilities.h"
#include "constants/arc.h"
#include "constants/moves.h"
#include "constants/tr_ai.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "nitro/fx.h"
#include "nitro/os.h"
#include "pml/item.h"
#include "pml/personal.h"
#include "pml/waza.h"
#include "system/vm.h"

// The AI stops thinking for the frame once it has run for this many ticks
#define TRAI_TIME_LIMIT 500

#define TRAI_MOVE_MAX 4
#define TRAI_SCORE_INITIAL 100

// The scripts that are loaded at once, each for one AI flag
#define TRAI_SCRIPT_CACHE_SIZE 4
#define TRAI_SCRIPT_NONE 0xff

// TrAIContext.status
#define TRAI_STATUS_MOVE_DONE 0x1
#define TRAI_STATUS_FLEE 0x2
#define TRAI_STATUS_SKIP_MOVES 0x8
// The AI ran out of time, and continues where it stopped in the next call
#define TRAI_STATUS_SUSPENDED 0x10

// TrAIContext.seq
#define TRAI_SEQ_LOAD_SCRIPT 0
#define TRAI_SEQ_RUN_SCRIPT 1
#define TRAI_SEQ_DONE 2

// How AIConditionalJump compares a value with the script's value
enum {
    AI_CMP_LT,
    AI_CMP_GT,
    AI_CMP_EQ,
    AI_CMP_NE,
    AI_CMP_AND,
    AI_CMP_NAND,
    AI_CMP_LE,
    AI_CMP_GE,
};

typedef struct {
    u8 seq;
    u8 moveIdx;
    u16 moveId;
    s32 scores[TRAI_MOVE_MAX];
    s32 initialScores[TRAI_MOVE_MAX];
    // The best score and move against each position, when choosing a target
    s16 bestScores[BTL_POS_MAX];
    s8 bestMoves[BTL_POS_MAX];
    // The value that commands load and compare
    s32 result;
    // The AI flags whose scripts are still to run, shifted so that the next one is bit 0
    u32 flagsRemaining;
    u32 aiFlags;
    u8 status;
    // The AI flag whose script is running, which is also its file in the AI script archive
    u8 scriptIndex;
    u8 chosenMove;
    u8 chosenTarget;
    u8 damageRandom[TRAI_MOVE_MAX];
    // The moves that each position was seen to use
    u16 knownMoves[BTL_POS_MAX][TRAI_MOVE_MAX];
    u8 knownAbilities[BTL_POS_MAX];
    u8 unk82[0x12];
    u8 attackerPos;
    u8 defenderPos;
    u32 battleStyle;
    u32 battleType;
    HeapID heapId;
    ArcTool *scriptArc;
    ArcTool *itemArc;
    void *script;
    BtlMainModule *mainModule;
    BtlServerFlow *serverFlow;
    BtlPokeCon *pokeCon;
    BattleMon *attacker;
    BattleMon *defender;
    // What every command returns, so VM_Run returns after each command
    BOOL cmdReturn;
    u32 unkC8;
    // A random number for the turn
    u32 random;
    u64 startTick;
} TrAIContext;

typedef struct {
    u8 fileIds[TRAI_SCRIPT_CACHE_SIZE];
    u8 refCounts[TRAI_SCRIPT_CACHE_SIZE];
    void *scripts[TRAI_SCRIPT_CACHE_SIZE];
} TrAIScriptCache;

BOOL TrAI_ChooseMove(VM *vm, TrAIContext *wk);
BOOL TrAI_ChooseMoveAndTarget(VM *vm, TrAIContext *wk);
void TrAI_RunScripts(VM *vm, TrAIContext *wk);
BOOL AIGreaterThanRandom(VM *vm, void *work);
BOOL AILessThanRandom(VM *vm, void *work);
BOOL AIEqualToRandom(VM *vm, void *work);
BOOL AINotEqualToRandom(VM *vm, void *work);
void CompareToRandom(VM *vm, TrAIContext *wk, u32 op);
BOOL AIIncrementScore(VM *vm, void *work);
BOOL AIIsHPLessThan(VM *vm, void *work);
BOOL AIIsHPGreaterThan(VM *vm, void *work);
BOOL AIIsHPEqualTo(VM *vm, void *work);
BOOL AIIsHPNotEqualTo(VM *vm, void *work);
void CompareHP(VM *vm, TrAIContext *wk, u32 op);
BOOL AIHasStatus(VM *vm, void *work);
BOOL AIDoesNotHaveStatus(VM *vm, void *work);
void CheckStatusCondition(VM *vm, TrAIContext *wk, u32 op);
BOOL AIHasCondition(VM *vm, void *work);
BOOL AIDoesNotHaveCondition(VM *vm, void *work);
void CheckMonCondition(VM *vm, TrAIContext *wk, u32 op);
BOOL AIIsBadlyPoisoned(VM *vm, void *work);
BOOL AIIsNotBadlyPoisoned(VM *vm, void *work);
void CheckBadlyPoisoned(VM *vm, TrAIContext *wk, u32 op);
BOOL AIHasConditionFlag(VM *vm, void *work);
BOOL AIDoesNotHaveConditionFlag(VM *vm, void *work);
void CheckConditionFlag(VM *vm, TrAIContext *wk, u32 op);
BOOL AIIfSideEffect(VM *vm, void *work);
BOOL AIIfNotSideEffect(VM *vm, void *work);
void CheckSideEffect(VM *vm, TrAIContext *wk, u32 op);
BOOL AIIfLessThan(VM *vm, void *work);
BOOL AIIfGreaterThan(VM *vm, void *work);
BOOL AIIfEqual(VM *vm, void *work);
BOOL AIIfNotEqual(VM *vm, void *work);
BOOL AIIfBit(VM *vm, void *work);
BOOL AIIfNotBit(VM *vm, void *work);
void CompareResult(VM *vm, TrAIContext *wk, u32 op);
BOOL AIIfMove(VM *vm, void *work);
BOOL AIIfNotMove(VM *vm, void *work);
void CheckMove(VM *vm, TrAIContext *wk, u32 op);
BOOL AIIfResultInList(VM *vm, void *work);
BOOL AIIfResultNotInList(VM *vm, void *work);
BOOL AIHasDamagingMove(VM *vm, void *work);
BOOL AIDoesNotHaveDamagingMove(VM *vm, void *work);
BOOL AIGetTurnCount(VM *vm, void *work);
BOOL AIGetType(VM *vm, void *work);
BOOL AIGetBasePower(VM *vm, void *work);
BOOL AIGetHighestDamagingMove(VM *vm, void *work);
BOOL AIGetPreviousMove(VM *vm, void *work);
BOOL AICompareSpeed(VM *vm, void *work);
BOOL AICheckTeamCount(VM *vm, void *work);
BOOL AICheckMoveID(VM *vm, void *work);
BOOL AICheckMoveEffect(VM *vm, void *work);
BOOL AICheckAbility(VM *vm, void *work);
BOOL AINop43(VM *vm, void *work);
BOOL AIIfTypeEffectiveness(VM *vm, void *work);
BOOL AIIfStatusInParty(VM *vm, void *work);
BOOL AIIfStatusNotInParty(VM *vm, void *work);
void CheckStatusInParty(VM *vm, TrAIContext *wk, u32 op);
BOOL AIGetWeather(VM *vm, void *work);
BOOL AIHasMoveID(VM *vm, void *work);
BOOL AIDoesNotHaveMoveID(VM *vm, void *work);
void CompareMoveID(VM *vm, TrAIContext *wk, u32 op);
BOOL AIStatStageLessThan(VM *vm, void *work);
BOOL AIStatStageGreaterThan(VM *vm, void *work);
BOOL AIStatStageEqual(VM *vm, void *work);
BOOL AIStatStageNotEqual(VM *vm, void *work);
void CompareStatStage(VM *vm, TrAIContext *wk, u32 op);
BOOL AIIfCanFaint(VM *vm, void *work);
BOOL AIIfCannotFaint(VM *vm, void *work);
void CheckCanFaint(VM *vm, TrAIContext *wk, u32 op);
BOOL AIIfHasMove(VM *vm, void *work);
BOOL AIIfDoesNotHaveMove(VM *vm, void *work);
void CheckHasMove(VM *vm, TrAIContext *wk, u32 op);
BOOL AIIfHasMoveWithEffect(VM *vm, void *work);
BOOL AIIfDoesNotHaveMoveWithEffect(VM *vm, void *work);
void CheckHasMoveWithEffect(VM *vm, TrAIContext *wk, u32 op);
BOOL AINop60(VM *vm, void *work);
BOOL AIFlee(VM *vm, void *work);
BOOL AINop62(VM *vm, void *work);
BOOL AINop63(VM *vm, void *work);
BOOL AIGetHeldItem(VM *vm, void *work);
BOOL AIGetItemEffect(VM *vm, void *work);
BOOL AIGetGender(VM *vm, void *work);
BOOL AIIsFakeOutActive(VM *vm, void *work);
BOOL AIGetStockpileCount(VM *vm, void *work);
BOOL AIGetBattleStyle(VM *vm, void *work);
BOOL AIGetBattleType(VM *vm, void *work);
BOOL AIGetConsumedItem(VM *vm, void *work);
BOOL AINop72(VM *vm, void *work);
BOOL AIGetMovePower(VM *vm, void *work);
BOOL AIGetMoveID(VM *vm, void *work);
BOOL AIGetProtectCount(VM *vm, void *work);
BOOL AIJump(VM *vm, void *work);
BOOL AIEnd(VM *vm, void *work);
BOOL AICompareLevel(VM *vm, void *work);
BOOL AIIfTaunted(VM *vm, void *work);
BOOL AIIfNotTaunted(VM *vm, void *work);
void CheckTauntStatus(VM *vm, TrAIContext *wk, u32 op);
BOOL AIIfTargetIsAlly(VM *vm, void *work);
BOOL AIDoesMonHaveType(VM *vm, void *work);
BOOL AIGuessAbility(VM *vm, void *work);
BOOL AIIfFlashFireIsActive(VM *vm, void *work);
BOOL AIIfHasItem(VM *vm, void *work);
BOOL AIIfFieldEffect(VM *vm, void *work);
BOOL AIGetSideEffect(VM *vm, void *work);
BOOL AIIfPartyMemberDamaged(VM *vm, void *work);
BOOL AIIfPartyMemberUsedPP(VM *vm, void *work);
BOOL AIGetFlingPower(VM *vm, void *work);
BOOL AIGetMovePP(VM *vm, void *work);
BOOL AIIfCanUseLastResort(VM *vm, void *work);
BOOL AIGetMoveCategory(VM *vm, void *work);
BOOL AIGetLastMoveCategory(VM *vm, void *work);
BOOL AIGetOrderInTurn(VM *vm, void *work);
BOOL func_ov170_0218119c(VM *vm, void *work);
BOOL AIIfPartyMemberDealsMoreDamage(VM *vm, void *work);
BOOL AIIfHasSuperEffectiveMove(VM *vm, void *work);
BOOL AIIfLastMoveDealsMoreDamage(VM *vm, void *work);
BOOL AIGetPositiveStatStageTotal(VM *vm, void *work);
BOOL AIGetStatStageDifference(VM *vm, void *work);
BOOL AINop102(VM *vm, void *work);
BOOL AINop103(VM *vm, void *work);
BOOL AINop104(VM *vm, void *work);
BOOL AIGetHighestDamagingMoveWithPartners(VM *vm, void *work);
BOOL AIIsFainted(VM *vm, void *work);
BOOL AIIsNotFainted(VM *vm, void *work);
void CheckIfFainted(VM *vm, TrAIContext *wk, u32 op);
BOOL AIGetAbility(VM *vm, void *work);
BOOL AIIfSubstitute(VM *vm, void *work);
BOOL AIGetSpecies(VM *vm, void *work);
BOOL AIGreaterThanTurnRandom(VM *vm, void *work);
BOOL AILessThanTurnRandom(VM *vm, void *work);
BOOL AIEqualToTurnRandom(VM *vm, void *work);
BOOL AINotEqualToTurnRandom(VM *vm, void *work);
void CompareToTurnRandom(VM *vm, TrAIContext *wk, u32 op);
BOOL AIJumpByMoveEffect(VM *vm, void *work);
BOOL func_ov170_02181734(VM *vm, void *work);
BOOL AIIsAtkLessThanSpAtk(VM *vm, void *work);
BOOL AIIsAtkGreaterThanSpAtk(VM *vm, void *work);
BOOL AIIsAtkEqualToSpAtk(VM *vm, void *work);
void CompareOffensiveStats(VM *vm, TrAIContext *wk, u32 op);
BOOL AIConditionalJump(VM *vm, u32 op, s32 value, s32 ref, s32 offset);
u8 TrAI_GetTargetPos(TrAIContext *wk, u32 side);
u32 GuessAbility(TrAIContext *wk, u32 side, u8 pos);
BattleMon *TrAI_GetBattleMon(TrAIContext *wk, u8 pos);
BattleMon *TrAI_GetPartyMon(BattleParty *party, u8 index);
void TrAI_ResetScores(TrAIContext *wk);
void TrAI_RememberDefenderMove(TrAIContext *wk);
s32 GetMoveData(TrAIContext *wk, u16 move, u32 param);
BOOL GetMoveFlag(TrAIContext *wk, u16 move, u32 flag);
u32 TrAI_GetMaxDamage(TrAIContext *wk, BattleMon *attacker, BattleMon *defender, u32 damageRoll);
s32 GetItemData(TrAIContext *wk, u16 item, u32 param);
void TrAI_CreateScriptCache(HeapID heapId);
void TrAI_FreeScriptCache(void);
void *TrAI_LoadScript(TrAIContext *wk);
void TrAI_ReleaseScript(TrAIContext *wk);

static const VMCommand sTrAICommands[] = {
    AIGreaterThanRandom,
    AILessThanRandom,
    AIEqualToRandom,
    AINotEqualToRandom,
    AIIncrementScore,
    AIIsHPLessThan,
    AIIsHPGreaterThan,
    AIIsHPEqualTo,
    AIIsHPNotEqualTo,
    AIHasStatus,
    AIDoesNotHaveStatus,
    AIHasCondition,
    AIDoesNotHaveCondition,
    AIIsBadlyPoisoned,
    AIIsNotBadlyPoisoned,
    AIHasConditionFlag,
    AIDoesNotHaveConditionFlag,
    AIIfSideEffect,
    AIIfNotSideEffect,
    AIIfLessThan,
    AIIfGreaterThan,
    AIIfEqual,
    AIIfNotEqual,
    AIIfBit,
    AIIfNotBit,
    AIIfMove,
    AIIfNotMove,
    AIIfResultInList,
    AIIfResultNotInList,
    AIHasDamagingMove,
    AIDoesNotHaveDamagingMove,
    AIGetTurnCount,
    AIGetType,
    AIGetBasePower,
    AIGetHighestDamagingMove,
    AIGetPreviousMove,
    AIIfEqual,
    AIIfNotEqual,
    AICompareSpeed,
    AICheckTeamCount,
    AICheckMoveID,
    AICheckMoveEffect,
    AICheckAbility,
    AINop43,
    AIIfTypeEffectiveness,
    AIIfStatusInParty,
    AIIfStatusNotInParty,
    AIGetWeather,
    AIHasMoveID,
    AIDoesNotHaveMoveID,
    AIStatStageLessThan,
    AIStatStageGreaterThan,
    AIStatStageEqual,
    AIStatStageNotEqual,
    AIIfCanFaint,
    AIIfCannotFaint,
    AIIfHasMove,
    AIIfDoesNotHaveMove,
    AIIfHasMoveWithEffect,
    AIIfDoesNotHaveMoveWithEffect,
    AINop60,
    AIFlee,
    AINop62,
    AINop63,
    AIGetHeldItem,
    AIGetItemEffect,
    AIGetGender,
    AIIsFakeOutActive,
    AIGetStockpileCount,
    AIGetBattleStyle,
    AIGetBattleType,
    AIGetConsumedItem,
    AINop72,
    AIGetMovePower,
    AIGetMoveID,
    AIGetProtectCount,
    AIJump,
    AIEnd,
    AICompareLevel,
    AIIfTaunted,
    AIIfNotTaunted,
    AIIfTargetIsAlly,
    AIDoesMonHaveType,
    AIGuessAbility,
    AIIfFlashFireIsActive,
    AIIfHasItem,
    AIIfFieldEffect,
    AIGetSideEffect,
    AIIfPartyMemberDamaged,
    AIIfPartyMemberUsedPP,
    AIGetFlingPower,
    AIGetMovePP,
    AIIfCanUseLastResort,
    AIGetMoveCategory,
    AIGetLastMoveCategory,
    AIGetOrderInTurn,
    func_ov170_0218119c,
    AIIfPartyMemberDealsMoreDamage,
    AIIfHasSuperEffectiveMove,
    AIIfLastMoveDealsMoreDamage,
    AIGetPositiveStatStageTotal,
    AIGetStatStageDifference,
    AINop102,
    AINop103,
    AINop104,
    AIGetHighestDamagingMoveWithPartners,
    AIIsFainted,
    AIIsNotFainted,
    AIGetAbility,
    AIIfSubstitute,
    AIGetSpecies,
    AIGreaterThanTurnRandom,
    AILessThanTurnRandom,
    AIEqualToTurnRandom,
    AINotEqualToTurnRandom,
    AIJumpByMoveEffect,
    func_ov170_02181734,
    AIIsAtkLessThanSpAtk,
    AIIsAtkGreaterThanSpAtk,
    AIIsAtkEqualToSpAtk,
};

static const VMInitParam sTrAIVMInitParam = {
    16, 8, sTrAICommands, NELEMS(sTrAICommands), NULL, 0, 0,
};

static TrAIScriptCache *sScriptCache;

VM *TrAI_CreateVM(BtlMainModule *mainModule, BtlServerFlow *serverFlow, BtlPokeCon *pokeCon, u32 aiFlags,
                  HeapID heapId) {
    TrAIContext *wk = GFL_HeapAllocate(heapId, sizeof(TrAIContext), TRUE, "tr_ai.c", 473);
    VM *vm;

    wk->heapId = heapId;
    wk->cmdReturn = TRUE;
    wk->aiFlags = aiFlags;
    wk->scriptArc = GFL_ArcSysCreateFileHandle(ARCID_TRAI_SCRIPT, heapId);
    wk->itemArc = PML_ItemArcHandleCreate(heapId);
    wk->mainModule = mainModule;
    wk->serverFlow = serverFlow;
    wk->pokeCon = pokeCon;
    wk->battleStyle = BtlSetup_GetBattleStyle(mainModule);
    wk->battleType = BtlSetup_GetBattleType(mainModule);
    vm = VM_Create(heapId, &sTrAIVMInitParam);
    VM_ChangeEnv(vm, wk);
    if (!PML_MoveIsDataCachePresent()) {
        PML_MoveInitDataCache(48, heapId);
    }
    TrAI_CreateScriptCache(wk->heapId);
    return vm;
}

BOOL TrAI_Think(VM *vm) {
    TrAIContext *wk = VM_GetEnv(vm);
    s32 targets[3];
    s32 count;
    BOOL suspended;

    if (wk->battleStyle == BTL_STYLE_SINGLE) {
        wk->defenderPos = wk->attackerPos ^ 1;
        return TrAI_ChooseMove(vm, wk);
    }
    if (wk->battleStyle == BTL_STYLE_DOUBLE || wk->battleStyle == BTL_STYLE_TRIPLE) {
        return TrAI_ChooseMoveAndTarget(vm, wk);
    }

    // A rotation battle: think against a random opponent
    count = 0;
    for (wk->defenderPos = 0; wk->defenderPos < BTL_POS_MAX; wk->defenderPos++) {
        if (wk->defenderPos != wk->attackerPos && (wk->defenderPos & 1) != (wk->attackerPos & 1) &&
            IsFainted(TrAI_GetBattleMon(wk, wk->defenderPos)) != TRUE) {
            targets[count++] = wk->defenderPos;
        }
    }
    wk->defenderPos = targets[GFL_RandomMTRange(count)];
    suspended = TrAI_ChooseMove(vm, wk);
    if (!suspended) {
        wk->chosenTarget = 0;
    }
    return suspended;
}

void TrAI_DeleteVM(VM *vm) {
    TrAIContext *wk = VM_GetEnv(vm);

    GFL_ArcToolFree(wk->scriptArc);
    GFL_ArcToolFree(wk->itemArc);
    GFL_HeapFree(wk);
    VM_Free(vm);
    if (PML_MoveIsDataCachePresent() == TRUE) {
        PML_MoveFreeDataCache();
    }
    TrAI_FreeScriptCache();
}

void TrAI_Setup(VM *vm, const u8 *usableMoves, u8 attackerPos) {
    TrAIContext *wk = VM_GetEnv(vm);
    s32 i;

    wk->seq = TRAI_SEQ_LOAD_SCRIPT;
    wk->moveId = 0;
    wk->moveIdx = 0;
    wk->result = 0;
    wk->status = 0;
    wk->scriptIndex = 0;
    wk->attackerPos = attackerPos;
    wk->flagsRemaining = wk->aiFlags;
    wk->unkC8 = 4;
    for (i = 0; i < TRAI_MOVE_MAX; i++) {
        if (usableMoves[i] == 0) {
            wk->initialScores[i] = 0;
        } else {
            wk->initialScores[i] = TRAI_SCORE_INITIAL;
        }
        wk->damageRandom[i] = 100 - GFL_RandomMTRange(16);
    }
    wk->random = GFL_RandomMTRange(256);
}

u8 TrAI_GetChosenMove(VM *vm) {
    TrAIContext *wk = VM_GetEnv(vm);

    return wk->chosenMove;
}

u8 TrAI_GetChosenTarget(VM *vm) {
    TrAIContext *wk = VM_GetEnv(vm);

    return wk->chosenTarget;
}

BOOL TrAI_ChooseMove(VM *vm, TrAIContext *wk) {
    s32 bestScores[TRAI_MOVE_MAX];
    u8 bestMoves[TRAI_MOVE_MAX];
    s32 count;
    s32 i;
    u8 move;

    wk->attacker = TrAI_GetBattleMon(wk, wk->attackerPos);
    wk->defender = TrAI_GetBattleMon(wk, wk->defenderPos);
    if (!(wk->status & TRAI_STATUS_SUSPENDED)) {
        TrAI_ResetScores(wk);
        TrAI_RememberDefenderMove(wk);
    }
    while (wk->flagsRemaining != 0) {
        if (wk->flagsRemaining & 1) {
            if (!(wk->status & TRAI_STATUS_SUSPENDED)) {
                wk->seq = TRAI_SEQ_LOAD_SCRIPT;
            }
            wk->status &= (u8)~TRAI_STATUS_SUSPENDED;
            wk->startTick = OS_GetTick();
            TrAI_RunScripts(vm, wk);
            if (wk->status & TRAI_STATUS_SUSPENDED) {
                return TRUE;
            }
        }
        wk->flagsRemaining >>= 1;
        wk->scriptIndex++;
        wk->moveIdx = 0;
    }

    if (wk->status & TRAI_STATUS_FLEE) {
        move = TRAI_MOVE_FLEE;
    } else {
        bestScores[0] = wk->scores[0];
        count = 1;
        bestMoves[0] = 0;
        for (i = 1; i < TRAI_MOVE_MAX; i++) {
            if (i < GetBattleMonMoveCount(wk->attacker)) {
                if (bestScores[0] == wk->scores[i]) {
                    bestScores[count] = wk->scores[i];
                    bestMoves[count++] = i;
                }
                if (bestScores[0] < wk->scores[i]) {
                    bestScores[0] = wk->scores[i];
                    count = 1;
                    bestMoves[0] = i;
                }
            }
        }
        move = bestMoves[GFL_RandomMTRange(count)];
    }
    wk->chosenMove = move;
    wk->chosenTarget = wk->defenderPos;
    return FALSE;
}

BOOL TrAI_ChooseMoveAndTarget(VM *vm, TrAIContext *wk) {
    s32 bestScores[TRAI_MOVE_MAX];
    u8 bestMoves[TRAI_MOVE_MAX];
    u8 targets[BTL_POS_MAX];
    s32 posCount;
    s32 count;
    s32 i;
    u8 pos;
    s16 best;
    u16 move;

    if (wk->battleStyle == BTL_STYLE_DOUBLE) {
        posCount = 4;
    } else {
        posCount = 6;
    }
    if (!(wk->status & TRAI_STATUS_SUSPENDED)) {
        wk->defenderPos = 0;
    }
    for (; wk->defenderPos < posCount; wk->defenderPos++) {
        wk->attacker = TrAI_GetBattleMon(wk, wk->attackerPos);
        wk->defender = TrAI_GetBattleMon(wk, wk->defenderPos);
        if (!(wk->status & TRAI_STATUS_SUSPENDED)) {
            if (wk->defender == NULL) {
                wk->bestMoves[wk->defenderPos] = -1;
                wk->bestScores[wk->defenderPos] = -1;
                continue;
            }
            if (wk->defenderPos == wk->attackerPos || IsFainted(wk->defender) == TRUE) {
                wk->bestMoves[wk->defenderPos] = -1;
                wk->bestScores[wk->defenderPos] = -1;
                continue;
            }
            TrAI_ResetScores(wk);
            TrAI_RememberDefenderMove(wk);
            wk->scriptIndex = 0;
            wk->moveIdx = 0;
            wk->flagsRemaining = wk->aiFlags;
        }
        while (wk->flagsRemaining != 0) {
            if (wk->flagsRemaining & 1) {
                if (!(wk->status & TRAI_STATUS_SUSPENDED)) {
                    wk->seq = TRAI_SEQ_LOAD_SCRIPT;
                }
                wk->status &= (u8)~TRAI_STATUS_SUSPENDED;
                wk->startTick = OS_GetTick();
                TrAI_RunScripts(vm, wk);
                if (wk->status & TRAI_STATUS_SUSPENDED) {
                    return TRUE;
                }
            }
            wk->flagsRemaining >>= 1;
            wk->scriptIndex++;
            wk->moveIdx = 0;
        }

        if (wk->status & TRAI_STATUS_FLEE) {
            wk->chosenMove = TRAI_MOVE_FLEE;
            continue;
        }
        bestScores[0] = wk->scores[0];
        count = 1;
        bestMoves[0] = 0;
        for (i = 1; i < TRAI_MOVE_MAX; i++) {
            if (i < GetBattleMonMoveCount(wk->attacker)) {
                if (bestScores[0] == wk->scores[i]) {
                    bestScores[count] = wk->scores[i];
                    bestMoves[count++] = i;
                }
                if (bestScores[0] < wk->scores[i]) {
                    bestScores[0] = wk->scores[i];
                    count = 1;
                    bestMoves[0] = i;
                }
            }
        }
        wk->bestMoves[wk->defenderPos] = bestMoves[GFL_RandomMTRange(count)];
        wk->bestScores[wk->defenderPos] = bestScores[0];
        // Don't target a partner unless the move scored at least the initial score
        if (!AreClientsOnOppositeSides(wk->mainModule, GetClientIDFromPos(wk->mainModule, wk->attackerPos),
                                       GetClientIDFromPos(wk->mainModule, wk->defenderPos)) &&
            wk->bestScores[wk->defenderPos] < TRAI_SCORE_INITIAL) {
            wk->bestScores[wk->defenderPos] = -1;
        }
    }

    best = wk->bestScores[0];
    targets[0] = 0;
    count = 1;
    for (pos = 1; pos < posCount; pos++) {
        if (best == wk->bestScores[pos]) {
            targets[count++] = pos;
        }
        if (best < wk->bestScores[pos]) {
            best = wk->bestScores[pos];
            targets[0] = pos;
            count = 1;
        }
    }
    wk->chosenTarget = targets[GFL_RandomMTRange(count)];
    wk->chosenMove = wk->bestMoves[wk->chosenTarget];
    if (GetMoveData(wk, MoveGetID(wk->attacker, wk->chosenMove), 27) == 1 && !(wk->chosenTarget & 1)) {
        wk->chosenTarget = wk->attackerPos;
    }
    move = MoveGetID(wk->attacker, wk->chosenMove);
    if (wk->battleStyle == BTL_STYLE_TRIPLE && !IsAdjacentOpponent(wk->attackerPos, wk->chosenTarget) &&
        !GetMoveFlag(wk, move, 11)) {
        wk->chosenTarget = (wk->chosenTarget & 1) + 2;
    }
    return FALSE;
}

void TrAI_RunScripts(VM *vm, TrAIContext *wk) {
    while (wk->seq != TRAI_SEQ_DONE && !(wk->status & TRAI_STATUS_SUSPENDED)) {
        switch (wk->seq) {
        case TRAI_SEQ_LOAD_SCRIPT:
            if (GetMovePP(wk->attacker, wk->moveIdx) == 0) {
                wk->moveId = 0;
            } else {
                wk->moveId = MoveGetID(wk->attacker, wk->moveIdx);
                wk->script = TrAI_LoadScript(wk);
                VM_LoadScript(vm, wk->script);
            }
            wk->seq++;
            // fallthrough
        case TRAI_SEQ_RUN_SCRIPT:
            // In a triple battle, skip moves that can't reach the defender
            if (wk->battleStyle == BTL_STYLE_TRIPLE && wk->moveId != 0 &&
                !IsAdjacentOpponent(wk->attackerPos, wk->defenderPos) && !GetMoveFlag(wk, wk->moveId, 11)) {
                wk->moveId = 0;
                VM_Halt(vm);
                TrAI_ReleaseScript(wk);
                wk->script = NULL;
            }
            if (wk->moveId != 0) {
                VM_Run(vm);
                if (wk->status & TRAI_STATUS_MOVE_DONE) {
                    wk->status &= (u8)~TRAI_STATUS_SUSPENDED;
                    TrAI_ReleaseScript(wk);
                    wk->script = NULL;
                } else if (OS_GetTick() - wk->startTick > TRAI_TIME_LIMIT) {
                    wk->status |= TRAI_STATUS_SUSPENDED;
                    break;
                }
            } else {
                wk->scores[wk->moveIdx] = 0;
                wk->status |= TRAI_STATUS_MOVE_DONE;
            }
            if (wk->status & TRAI_STATUS_MOVE_DONE) {
                wk->moveIdx++;
                if (wk->moveIdx < GetBattleMonMoveCount(wk->attacker) && !(wk->status & TRAI_STATUS_SKIP_MOVES)) {
                    wk->seq = TRAI_SEQ_LOAD_SCRIPT;
                } else {
                    wk->seq++;
                }
                wk->status &= (u8)~TRAI_STATUS_MOVE_DONE;
            }
            break;
        case TRAI_SEQ_DONE:
        default:
            wk->seq = TRAI_SEQ_DONE;
            break;
        }
    }
}

BOOL AIGreaterThanRandom(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareToRandom(vm, wk, AI_CMP_LT);
    return wk->cmdReturn;
}

BOOL AILessThanRandom(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareToRandom(vm, wk, AI_CMP_GT);
    return wk->cmdReturn;
}

BOOL AIEqualToRandom(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareToRandom(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AINotEqualToRandom(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareToRandom(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CompareToRandom(VM *vm, TrAIContext *wk, u32 op) {
    s32 value = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op, GFL_RandomMTRange(256), value, offset);
}

BOOL AIIncrementScore(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->scores[wk->moveIdx] += (s32)VM_Read32(vm);
    if (wk->scores[wk->moveIdx] < 0) {
        wk->scores[wk->moveIdx] = 0;
    }
    return wk->cmdReturn;
}

BOOL AIIsHPLessThan(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareHP(vm, wk, AI_CMP_LT);
    return wk->cmdReturn;
}

BOOL AIIsHPGreaterThan(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareHP(vm, wk, AI_CMP_GT);
    return wk->cmdReturn;
}

BOOL AIIsHPEqualTo(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareHP(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIIsHPNotEqualTo(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareHP(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CompareHP(VM *vm, TrAIContext *wk, u32 op) {
    u32 side = VM_Read32(vm);
    s32 value = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op, FX_Whole(GetHPRatio(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side)))), value, offset);
}

BOOL AIHasStatus(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckStatusCondition(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

BOOL AIDoesNotHaveStatus(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckStatusCondition(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

void CheckStatusCondition(VM *vm, TrAIContext *wk, u32 op) {
    u32 side = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op, GetBattleMonStatus(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side))), 0, offset);
}

BOOL AIHasCondition(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckMonCondition(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIDoesNotHaveCondition(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckMonCondition(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CheckMonCondition(VM *vm, TrAIContext *wk, u32 op) {
    u32 side = VM_Read32(vm);
    u32 condition = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op, CheckCondition(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side)), condition), TRUE,
                      offset);
}

BOOL AIIsBadlyPoisoned(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckBadlyPoisoned(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIIsNotBadlyPoisoned(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckBadlyPoisoned(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CheckBadlyPoisoned(VM *vm, TrAIContext *wk, u32 op) {
    u32 side = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    BattleMon *mon = TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side));
    BOOL poisoned = FALSE;
    BattleConditionCont cont;

    if (CheckCondition(mon, CONDITION_POISON)) {
        cont = GetConditionContinuationParam(mon, CONDITION_POISON);
        if (Condition_IsBadlyPoisoned(cont)) {
            poisoned = TRUE;
        }
    }
    AIConditionalJump(vm, op, poisoned, TRUE, offset);
}

BOOL AIHasConditionFlag(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckConditionFlag(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIDoesNotHaveConditionFlag(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckConditionFlag(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CheckConditionFlag(VM *vm, TrAIContext *wk, u32 op) {
    u32 side = VM_Read32(vm);
    u32 flag = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op, GetAdditionalConditionFlag(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side)), flag),
                      TRUE, offset);
}

BOOL AIIfSideEffect(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckSideEffect(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIIfNotSideEffect(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckSideEffect(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CheckSideEffect(VM *vm, TrAIContext *wk, u32 op) {
    u32 side = VM_Read32(vm);
    u32 sideEffect = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op,
                      BtlServerFlow_GetSideEffectLayers(wk->serverFlow, TrAI_GetTargetPos(wk, side), sideEffect) != 0,
                      TRUE, offset);
}

BOOL AIIfLessThan(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareResult(vm, wk, AI_CMP_LT);
    return wk->cmdReturn;
}

BOOL AIIfGreaterThan(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareResult(vm, wk, AI_CMP_GT);
    return wk->cmdReturn;
}

BOOL AIIfEqual(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareResult(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIIfNotEqual(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareResult(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

BOOL AIIfBit(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareResult(vm, wk, AI_CMP_AND);
    return wk->cmdReturn;
}

BOOL AIIfNotBit(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareResult(vm, wk, AI_CMP_NAND);
    return wk->cmdReturn;
}

void CompareResult(VM *vm, TrAIContext *wk, u32 op) {
    s32 value = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op, wk->result, value, offset);
}

BOOL AIIfMove(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckMove(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIIfNotMove(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckMove(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CheckMove(VM *vm, TrAIContext *wk, u32 op) {
    s32 move = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op, wk->moveId, move, offset);
}

// The list is 32-bit values, stored as 16-bit halves, and ends with 0xffffffff
BOOL AIIfResultInList(VM *vm, void *work) {
    TrAIContext *wk = work;
    const u16 *list = (const u16 *)(vm->pc + (s32)VM_Read32(vm));
    s32 offset = VM_Read32(vm);
    s32 i = 0;
    u32 value;

    do {
        value = list[i] | (list[i + 1] << 16);
        if (wk->result == value) {
            VM_Jump(vm, vm->pc + offset);
            break;
        }
        i += 2;
    } while (value != 0xffffffff);
    return wk->cmdReturn;
}

BOOL AIIfResultNotInList(VM *vm, void *work) {
    TrAIContext *wk = work;
    const u16 *list = (const u16 *)(vm->pc + (s32)VM_Read32(vm));
    s32 offset = VM_Read32(vm);
    s32 i = 0;
    u32 value;

    do {
        value = list[i] | (list[i + 1] << 16);
        if (wk->result == value) {
            return wk->cmdReturn;
        }
        i += 2;
    } while (value != 0xffffffff);
    VM_Jump(vm, vm->pc + offset);
    return wk->cmdReturn;
}

BOOL AIHasDamagingMove(VM *vm, void *work) {
    TrAIContext *wk = work;
    s32 offset = VM_Read32(vm);
    s32 i;

    for (i = 0; i < GetBattleMonMoveCount(wk->attacker); i++) {
        if (GetMoveData(wk, MoveGetID(wk->attacker, i), MOVE_PARAM_POWER) != 0) {
            break;
        }
    }
    if (i < GetBattleMonMoveCount(wk->attacker)) {
        VM_Jump(vm, vm->pc + offset);
    }
    return wk->cmdReturn;
}

BOOL AIDoesNotHaveDamagingMove(VM *vm, void *work) {
    TrAIContext *wk = work;
    s32 offset = VM_Read32(vm);
    s32 i;

    for (i = 0; i < GetBattleMonMoveCount(wk->attacker); i++) {
        if (GetMoveData(wk, MoveGetID(wk->attacker, i), MOVE_PARAM_POWER) != 0) {
            break;
        }
    }
    if (i == GetBattleMonMoveCount(wk->attacker)) {
        VM_Jump(vm, vm->pc + offset);
    }
    return wk->cmdReturn;
}

BOOL AIGetTurnCount(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetTurnCounter(wk->serverFlow);
    return wk->cmdReturn;
}

BOOL AIGetType(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 which = VM_Read32(vm);
    PokeTypePair attackerTypes = GetPokeType(wk->attacker);
    PokeTypePair defenderTypes = GetPokeType(wk->defender);

    switch (which) {
    case LOAD_ATTACKER_TYPE_1:
        wk->result = PokeTypePair_GetType1(attackerTypes);
        break;
    case LOAD_DEFENDER_TYPE_1:
        wk->result = PokeTypePair_GetType1(defenderTypes);
        break;
    case LOAD_ATTACKER_TYPE_2:
        wk->result = PokeTypePair_GetType2(attackerTypes);
        break;
    case LOAD_DEFENDER_TYPE_2:
        wk->result = PokeTypePair_GetType2(defenderTypes);
        break;
    case LOAD_MOVE_TYPE:
        wk->result = GetMoveData(wk, wk->moveId, MOVE_PARAM_TYPE);
        break;
    case LOAD_ATTACKER_PARTNER_TYPE_1:
        wk->result = PokeTypePair_GetType1(
            GetPokeType(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, AI_BATTLER_ATTACKER_PARTNER))));
        break;
    case LOAD_DEFENDER_PARTNER_TYPE_1:
        wk->result = PokeTypePair_GetType1(
            GetPokeType(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, AI_BATTLER_DEFENDER_PARTNER))));
        break;
    case LOAD_ATTACKER_PARTNER_TYPE_2:
        wk->result = PokeTypePair_GetType2(
            GetPokeType(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, AI_BATTLER_ATTACKER_PARTNER))));
        break;
    case LOAD_DEFENDER_PARTNER_TYPE_2:
        wk->result = PokeTypePair_GetType2(
            GetPokeType(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, AI_BATTLER_DEFENDER_PARTNER))));
        break;
    }
    return wk->cmdReturn;
}

BOOL AIGetBasePower(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = PML_MoveGetBasePower(wk->moveId);
    return wk->cmdReturn;
}

// Sets the result to 0 if the move does no damage, 1 if another move does more damage, or 2 otherwise
BOOL AIGetHighestDamagingMove(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 damageRoll = VM_Read32(vm);
    u8 attackerId = GetMonID(wk->attacker);
    u8 defenderId = GetMonID(wk->defender);
    u32 damage = AICalcDamage(wk->serverFlow, attackerId, defenderId, wk->moveId, TRUE, damageRoll);
    s32 i;
    u16 move;

    if (damage == 0) {
        wk->result = 0;
    } else {
        wk->result = 2;
        for (i = 0; i < GetBattleMonMoveCount(wk->attacker); i++) {
            move = MoveGetID(wk->attacker, i);
            if (i != wk->moveIdx &&
                AICalcDamage(wk->serverFlow, attackerId, defenderId, move, TRUE, damageRoll) > damage) {
                wk->result = 1;
                break;
            }
        }
    }
    return wk->cmdReturn;
}

BOOL AIGetPreviousMove(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetPreviousMoveID(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, VM_Read32(vm))));
    return wk->cmdReturn;
}

BOOL AICompareSpeed(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 op = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    u16 attackerSpeed = BtlServerFlow_CalcSpeed(wk->serverFlow, wk->attacker, TRUE);
    u16 defenderSpeed = BtlServerFlow_CalcSpeed(wk->serverFlow, wk->defender, TRUE);

    switch (op) {
    case COMPARE_SPEED_FASTER:
        op = AI_CMP_GT;
        break;
    case COMPARE_SPEED_SLOWER:
        op = AI_CMP_LT;
        break;
    case COMPARE_SPEED_TIE:
        op = AI_CMP_EQ;
        break;
    }
    AIConditionalJump(vm, op, attackerSpeed, defenderSpeed, offset);
    return wk->cmdReturn;
}

BOOL AICheckTeamCount(VM *vm, void *work) {
    TrAIContext *wk = work;
    u8 clientId = GetClientIDFromPos(wk->mainModule, TrAI_GetTargetPos(wk, VM_Read32(vm)));
    BattleParty *party = GetClientParty(wk->pokeCon, clientId);
    s32 i = GetClientBattlerCount(wk->mainModule, clientId);

    wk->result = 0;
    for (; i < GetNumMonsInParty(party); i++) {
        if (CanPokemonBattle(TrAI_GetPartyMon(party, i))) {
            wk->result++;
        }
    }
    return wk->cmdReturn;
}

BOOL AICheckMoveID(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = wk->moveId;
    return wk->cmdReturn;
}

BOOL AICheckMoveEffect(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetMoveData(wk, wk->moveId, MOVE_PARAM_EFFECT);
    return wk->cmdReturn;
}

BOOL AICheckAbility(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);

    wk->result = GuessAbility(wk, side, TrAI_GetTargetPos(wk, side));
    return wk->cmdReturn;
}

BOOL AINop43(VM *vm, void *work) {
    TrAIContext *wk = work;

    return wk->cmdReturn;
}

BOOL AIIfTypeEffectiveness(VM *vm, void *work) {
    TrAIContext *wk = work;
    s32 value = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    u8 attackerId = GetMonID(wk->attacker);

    AIConditionalJump(vm, AI_CMP_EQ,
                      CalcMoveEffectiveness(wk->serverFlow, attackerId, GetMonID(wk->defender), wk->moveId), value,
                      offset);
    return wk->cmdReturn;
}

BOOL AIIfStatusInParty(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckStatusInParty(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIIfStatusNotInParty(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckStatusInParty(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CheckStatusInParty(VM *vm, TrAIContext *wk, u32 op) {
    u32 side = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    u8 clientId = GetClientIDFromPos(wk->mainModule, TrAI_GetTargetPos(wk, side));
    BattleParty *party = GetClientParty(wk->pokeCon, clientId);
    s32 i = GetClientBattlerCount(wk->mainModule, clientId);
    BattleMon *mon;

    for (; i < GetNumMonsInParty(party); i++) {
        mon = TrAI_GetPartyMon(party, i);
        if (!IsFainted(mon) && AIConditionalJump(vm, op, GetBattleMonStatus(mon), 0, offset)) {
            break;
        }
    }
}

BOOL AIGetWeather(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetFieldWeather();
    return wk->cmdReturn;
}

BOOL AIHasMoveID(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareMoveID(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIDoesNotHaveMoveID(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareMoveID(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

// Compares the move's effect, despite swan's name
void CompareMoveID(VM *vm, TrAIContext *wk, u32 op) {
    s32 effect = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op, GetMoveData(wk, wk->moveId, MOVE_PARAM_EFFECT), effect, offset);
}

BOOL AIStatStageLessThan(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareStatStage(vm, wk, AI_CMP_LT);
    return wk->cmdReturn;
}

BOOL AIStatStageGreaterThan(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareStatStage(vm, wk, AI_CMP_GT);
    return wk->cmdReturn;
}

BOOL AIStatStageEqual(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareStatStage(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIStatStageNotEqual(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareStatStage(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CompareStatStage(VM *vm, TrAIContext *wk, u32 op) {
    u32 side = VM_Read32(vm);
    u32 stat = VM_Read32(vm);
    s32 value = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op, GetBattleMonStat(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side)), stat), value,
                      offset);
}

BOOL AIIfCanFaint(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckCanFaint(vm, wk, AI_CMP_LE);
    return wk->cmdReturn;
}

BOOL AIIfCannotFaint(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckCanFaint(vm, wk, AI_CMP_GT);
    return wk->cmdReturn;
}

void CheckCanFaint(VM *vm, TrAIContext *wk, u32 op) {
    s32 offset;
    u32 hp;
    u8 attackerId;

    VM_Read32(vm);
    offset = VM_Read32(vm);
    hp = GetBattleMonStat(wk->defender, BATTLEMON_HP);
    attackerId = GetMonID(wk->attacker);
    AIConditionalJump(
        vm, op, hp, AICalcDamage(wk->serverFlow, attackerId, GetMonID(wk->defender), wk->moveId, TRUE, USE_MIN_DAMAGE),
        offset);
}

BOOL AIIfHasMove(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckHasMove(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIIfDoesNotHaveMove(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckHasMove(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CheckHasMove(VM *vm, TrAIContext *wk, u32 op) {
    u32 side = VM_Read32(vm);
    u16 move = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    u8 pos = TrAI_GetTargetPos(wk, side);
    BOOL found = FALSE;
    BattleMon *mon;
    s32 i;
    u16 other;

    switch (side) {
    case AI_BATTLER_ATTACKER:
        for (i = 0; i < GetBattleMonMoveCount(wk->attacker); i++) {
            other = MoveGetID(wk->attacker, i);
            if (other == move) {
                found = TRUE;
                break;
            }
        }
        break;
    case AI_BATTLER_ATTACKER_PARTNER:
        mon = TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, AI_BATTLER_ATTACKER_PARTNER));
        if (IsFainted(mon)) {
            break;
        }
        for (i = 0; i < GetBattleMonMoveCount(mon); i++) {
            other = MoveGetID(mon, i);
            if (other == move) {
                found = TRUE;
                break;
            }
        }
        break;
    case AI_BATTLER_DEFENDER:
        // Only the moves the defender was seen to use
        for (i = 0; i < TRAI_MOVE_MAX; i++) {
            if (move == wk->knownMoves[pos][i]) {
                found = TRUE;
                break;
            }
        }
        break;
    }
    AIConditionalJump(vm, op, found, TRUE, offset);
}

BOOL AIIfHasMoveWithEffect(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckHasMoveWithEffect(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIIfDoesNotHaveMoveWithEffect(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckHasMoveWithEffect(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CheckHasMoveWithEffect(VM *vm, TrAIContext *wk, u32 op) {
    u32 side = VM_Read32(vm);
    s32 effect = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    u8 pos = TrAI_GetTargetPos(wk, side);
    BOOL found = FALSE;
    s32 i;
    s32 moveEffect;

    switch (side) {
    case AI_BATTLER_ATTACKER:
        for (i = 0; i < GetBattleMonMoveCount(wk->attacker); i++) {
            moveEffect = GetMoveData(wk, MoveGetID(wk->attacker, i), MOVE_PARAM_EFFECT);
            if (moveEffect == effect) {
                found = TRUE;
                break;
            }
        }
        break;
    case AI_BATTLER_DEFENDER:
        for (i = 0; i < TRAI_MOVE_MAX; i++) {
            if (wk->knownMoves[pos][i] != 0) {
                moveEffect = GetMoveData(wk, wk->knownMoves[pos][i], MOVE_PARAM_EFFECT);
                if (moveEffect == effect) {
                    found = TRUE;
                    break;
                }
            }
        }
        break;
    }
    AIConditionalJump(vm, op, found, TRUE, offset);
}

BOOL AINop60(VM *vm, void *work) {
    TrAIContext *wk = work;

    return wk->cmdReturn;
}

BOOL AIFlee(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->status |= TRAI_STATUS_FLEE;
    return wk->cmdReturn;
}

BOOL AINop62(VM *vm, void *work) {
    TrAIContext *wk = work;

    return wk->cmdReturn;
}

BOOL AINop63(VM *vm, void *work) {
    TrAIContext *wk = work;

    return wk->cmdReturn;
}

BOOL AIGetHeldItem(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetBattleMonHeldItem(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, VM_Read32(vm))));
    return wk->cmdReturn;
}

BOOL AIGetItemEffect(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetItemData(wk, GetBattleMonHeldItem(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, VM_Read32(vm)))), 1);
    return wk->cmdReturn;
}

BOOL AIGetGender(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetBattleMonStat(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, VM_Read32(vm))), BATTLEMON_GENDER);
    return wk->cmdReturn;
}

BOOL AIIsFakeOutActive(VM *vm, void *work) {
    TrAIContext *wk = work;
    BattleMon *mon = TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, VM_Read32(vm)));
    BOOL active = FALSE;

    if (GetAdditionalConditionFlag(mon, 0) == 0) {
        active = TRUE;
    }
    wk->result = active;
    return wk->cmdReturn;
}

BOOL AIGetStockpileCount(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetConditionCount(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, VM_Read32(vm))), 0);
    return wk->cmdReturn;
}

BOOL AIGetBattleStyle(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = wk->battleStyle;
    return wk->cmdReturn;
}

BOOL AIGetBattleType(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = wk->battleType;
    return wk->cmdReturn;
}

BOOL AIGetConsumedItem(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetConsumedItem(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, VM_Read32(vm))));
    return wk->cmdReturn;
}

BOOL AINop72(VM *vm, void *work) {
    TrAIContext *wk = work;

    return wk->cmdReturn;
}

// Replaces the result, a move, with its power
BOOL AIGetMovePower(VM *vm, void *work) {
    TrAIContext *wk = work;

    if (wk->result != 0) {
        wk->result = GetMoveData(wk, wk->result, MOVE_PARAM_POWER);
    } else {
        wk->result = 0;
    }
    return wk->cmdReturn;
}

// Replaces the result, a move, with its effect
BOOL AIGetMoveID(VM *vm, void *work) {
    TrAIContext *wk = work;

    if (wk->result != 0) {
        wk->result = GetMoveData(wk, wk->result, MOVE_PARAM_EFFECT);
    } else {
        wk->result = -1;
    }
    return wk->cmdReturn;
}

BOOL AIGetProtectCount(VM *vm, void *work) {
    TrAIContext *wk = work;
    BattleMon *mon = TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, VM_Read32(vm)));

    if (GetPreviousMoveID(mon) != MOVE_PROTECT && GetPreviousMoveID(mon) != MOVE_DETECT &&
        GetPreviousMoveID(mon) != MOVE_ENDURE) {
        wk->result = 0;
    } else {
        wk->result = GetConsecutiveMoveCount(mon);
    }
    return wk->cmdReturn;
}

BOOL AIJump(VM *vm, void *work) {
    TrAIContext *wk = work;
    s32 offset = VM_Read32(vm);

    VM_Jump(vm, vm->pc + offset);
    return wk->cmdReturn;
}

BOOL AIEnd(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->status |= TRAI_STATUS_MOVE_DONE;
    VM_Halt(vm);
    return TRUE;
}

BOOL AICompareLevel(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 mode = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    u32 ops[3] = { AI_CMP_GT, AI_CMP_LT, AI_CMP_EQ };
    u32 attackerLevel = GetBattleMonStat(wk->attacker, BATTLEMON_LEVEL);

    AIConditionalJump(vm, ops[mode], attackerLevel, GetBattleMonStat(wk->defender, BATTLEMON_LEVEL), offset);
    return wk->cmdReturn;
}

BOOL AIIfTaunted(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckTauntStatus(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIIfNotTaunted(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckTauntStatus(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CheckTauntStatus(VM *vm, TrAIContext *wk, u32 op) {
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op, CheckCondition(wk->defender, CONDITION_TAUNT), TRUE, offset);
}

BOOL AIIfTargetIsAlly(VM *vm, void *work) {
    TrAIContext *wk = work;

    AIConditionalJump(vm, AI_CMP_EQ, wk->attackerPos & 1, wk->defenderPos & 1, VM_Read32(vm));
    return wk->cmdReturn;
}

BOOL AIDoesMonHaveType(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);
    u8 type = VM_Read32(vm);
    PokeTypePair types = GetPokeType(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side)));

    if (type == PokeTypePair_GetType1(types) || type == PokeTypePair_GetType2(types)) {
        wk->result = TRUE;
    } else {
        wk->result = FALSE;
    }
    return wk->cmdReturn;
}

BOOL AIGuessAbility(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);
    u32 ability = VM_Read32(vm);
    u32 guess = GuessAbility(wk, side, TrAI_GetTargetPos(wk, side));

    if (guess == ability) {
        wk->result = TRUE;
    } else {
        wk->result = FALSE;
    }
    return wk->cmdReturn;
}

BOOL AIIfFlashFireIsActive(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    if (GetAdditionalConditionFlag(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side)), 13)) {
        VM_Jump(vm, vm->pc + offset);
    }
    return wk->cmdReturn;
}

BOOL AIIfHasItem(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);
    u32 item = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    if (item == GetBattleMonHeldItem(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side)))) {
        VM_Jump(vm, vm->pc + offset);
    }
    return wk->cmdReturn;
}

BOOL AIIfFieldEffect(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 fieldEffect = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, AI_CMP_EQ, IsFieldEffectActive(fieldEffect), TRUE, offset);
    return wk->cmdReturn;
}

BOOL AIGetSideEffect(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);
    u32 sideEffect = VM_Read32(vm);

    wk->result = BtlServerFlow_GetSideEffectLayers(wk->serverFlow, TrAI_GetTargetPos(wk, side), sideEffect);
    return wk->cmdReturn;
}

// Checks the party's Pokemon that aren't in battle. For each one that hasn't fainted, the original checks the HP of
// the Pokemon at the position instead of the party member's
BOOL AIIfPartyMemberDamaged(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    u8 pos = TrAI_GetTargetPos(wk, side);
    u8 clientId = GetClientIDFromPos(wk->mainModule, pos);
    BattleParty *party = GetClientParty(wk->pokeCon, clientId);
    s32 i = GetClientBattlerCount(wk->mainModule, clientId);

    for (; i < GetNumMonsInParty(party); i++) {
        if (!IsFainted(TrAI_GetPartyMon(party, i)) &&
            AIConditionalJump(vm, AI_CMP_LT, GetHPRatio(TrAI_GetBattleMon(wk, pos)), 100, offset)) {
            break;
        }
    }
    return wk->cmdReturn;
}

// Checks the party's Pokemon that aren't in battle. For each one that hasn't fainted, the original checks the PP of
// the Pokemon at the position instead of the party member's
BOOL AIIfPartyMemberUsedPP(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    u8 pos = TrAI_GetTargetPos(wk, side);
    u8 clientId = GetClientIDFromPos(wk->mainModule, pos);
    BattleParty *party = GetClientParty(wk->pokeCon, clientId);
    s32 i = GetClientBattlerCount(wk->mainModule, clientId);
    s32 j;

    for (; i < GetNumMonsInParty(party); i++) {
        if (IsFainted(TrAI_GetPartyMon(party, i))) {
            continue;
        }
        for (j = 0; j < GetBattleMonMoveCount(TrAI_GetBattleMon(wk, pos)); j++) {
            if (AIConditionalJump(vm, AI_CMP_NE, GetMovePPUsed(TrAI_GetBattleMon(wk, pos), j), 0, offset)) {
                break;
            }
        }
        if (j != GetBattleMonMoveCount(TrAI_GetBattleMon(wk, pos))) {
            break;
        }
    }
    return wk->cmdReturn;
}

BOOL AIGetFlingPower(VM *vm, void *work) {
    TrAIContext *wk = work;
    u8 pos = TrAI_GetTargetPos(wk, VM_Read32(vm));

    wk->result = 0;
    if (!CheckCondition(TrAI_GetBattleMon(wk, pos), CONDITION_EMBARGO)) {
        wk->result = GetItemData(wk, GetBattleMonHeldItem(TrAI_GetBattleMon(wk, pos)), ITEM_PARAM_FLING_POWER);
    }
    return wk->cmdReturn;
}

BOOL AIGetMovePP(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetMovePP(wk->attacker, wk->moveIdx);
    return wk->cmdReturn;
}

BOOL AIIfCanUseLastResort(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    BattleMon *mon = TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side));
    u8 count = CountUsedMoves(mon);

    if (count >= GetBattleMonMoveCount(mon) && GetBattleMonMoveCount(mon) > 1) {
        VM_Jump(vm, vm->pc + offset);
    }
    return wk->cmdReturn;
}

BOOL AIGetMoveCategory(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetMoveData(wk, wk->moveId, MOVE_PARAM_CATEGORY);
    return wk->cmdReturn;
}

BOOL AIGetLastMoveCategory(VM *vm, void *work) {
    TrAIContext *wk = work;
    u16 move = GetPreviousMoveID(wk->defender);

    if (move != 0) {
        wk->result = GetMoveData(wk, move, MOVE_PARAM_CATEGORY);
    } else {
        wk->result = 0;
    }
    return wk->cmdReturn;
}

BOOL AIGetOrderInTurn(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = BtlServerFlow_CountFasterMons(wk->serverFlow,
                                               TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, VM_Read32(vm))), TRUE);
    return wk->cmdReturn;
}

// Loads the 16-bit value at 0x146 in the Pokemon at the position. Gen 4's AI has its command that loads the turns
// since the Pokemon switched in at this place in the command table
BOOL func_ov170_0218119c(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = BattleMon_GetTurnsOnField(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, VM_Read32(vm))));
    return wk->cmdReturn;
}

// Jumps if one of the party's Pokemon that aren't in battle can deal more damage to the defender than the attacker
BOOL AIIfPartyMemberDealsMoreDamage(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 damageRoll = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    u8 clientId = GetClientIDFromPos(wk->mainModule, wk->attackerPos);
    BattleParty *party = GetClientParty(wk->pokeCon, clientId);
    s32 i = GetClientBattlerCount(wk->mainModule, clientId);
    u32 damage = TrAI_GetMaxDamage(wk, wk->attacker, wk->defender, damageRoll);
    BattleMon *mon;

    for (; i < GetNumMonsInParty(party); i++) {
        mon = TrAI_GetPartyMon(party, i);
        GetMonID(mon);
        if (!IsFainted(mon) && TrAI_GetMaxDamage(wk, mon, wk->defender, damageRoll) > damage) {
            VM_Jump(vm, vm->pc + offset);
            break;
        }
    }
    return wk->cmdReturn;
}

BOOL AIIfHasSuperEffectiveMove(VM *vm, void *work) {
    TrAIContext *wk = work;
    s32 offset = VM_Read32(vm);
    s32 i;
    u16 move;
    u8 attackerId;
    u32 effectiveness;

    for (i = 0; i < GetBattleMonMoveCount(wk->attacker); i++) {
        move = MoveGetID(wk->attacker, i);
        attackerId = GetMonID(wk->attacker);
        effectiveness = CalcMoveEffectiveness(wk->serverFlow, attackerId, GetMonID(wk->defender), move);
        if (effectiveness == TYPE_EFFECTIVENESS_DOUBLE || effectiveness == TYPE_EFFECTIVENESS_QUADRUPLE) {
            VM_Jump(vm, vm->pc + offset);
            break;
        }
    }
    return wk->cmdReturn;
}

// Jumps if the last move of the Pokemon at the position deals more damage to the defender than the attacker's moves
BOOL AIIfLastMoveDealsMoreDamage(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);
    u32 damageRoll = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    BattleMon *mon = TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side));
    u32 damage = TrAI_GetMaxDamage(wk, wk->attacker, wk->defender, damageRoll);
    u16 move = GetPreviousMoveID(mon);
    u8 monId = GetMonID(mon);

    if (damage < AICalcDamage(wk->serverFlow, monId, GetMonID(wk->defender), move, TRUE, damageRoll)) {
        VM_Jump(vm, vm->pc + offset);
    }
    return wk->cmdReturn;
}

BOOL AIGetPositiveStatStageTotal(VM *vm, void *work) {
    TrAIContext *wk = work;
    u8 pos = TrAI_GetTargetPos(wk, VM_Read32(vm));
    u32 stats[7] = { BATTLEMON_ATTACK_STAGE,     BATTLEMON_DEFENSE_STAGE, BATTLEMON_SP_ATTACK_STAGE,
                     BATTLEMON_SP_DEFENSE_STAGE, BATTLEMON_SPEED_STAGE,   BATTLEMON_ACCURACY_STAGE,
                     BATTLEMON_EVASION_STAGE };
    u32 i;
    s32 stage;

    wk->result = 0;
    for (i = 0; i < 7; i++) {
        stage = GetBattleMonStat(TrAI_GetBattleMon(wk, pos), stats[i]);
        if (stage > BATTLEMON_STAT_STAGE_NEUTRAL) {
            wk->result += stage - BATTLEMON_STAT_STAGE_NEUTRAL;
        }
    }
    return wk->cmdReturn;
}

BOOL AIGetStatStageDifference(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);
    u32 stat = VM_Read32(vm);

    wk->result = GetBattleMonStat(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side)), stat) -
                 GetBattleMonStat(wk->attacker, stat);
    return wk->cmdReturn;
}

BOOL AINop102(VM *vm, void *work) {
    TrAIContext *wk = work;

    return wk->cmdReturn;
}

BOOL AINop103(VM *vm, void *work) {
    TrAIContext *wk = work;

    return wk->cmdReturn;
}

BOOL AINop104(VM *vm, void *work) {
    TrAIContext *wk = work;

    return wk->cmdReturn;
}

// Like AIGetHighestDamagingMove, with the moves of the attacker's side that are in battle
BOOL AIGetHighestDamagingMoveWithPartners(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 damageRoll = VM_Read32(vm);
    u8 clientId = GetClientIDFromPos(wk->mainModule, wk->attackerPos);
    BattleParty *party = GetClientParty(wk->pokeCon, clientId);
    s32 i;
    s32 attackerIndex;
    u8 attackerId;
    u8 defenderId;
    u32 damage;
    s32 count;
    s32 j;
    BattleMon *mon;
    u8 monId;
    u16 move;

    count = GetClientBattlerCount(wk->mainModule, clientId);
    attackerIndex = FindPartyMon(party, wk->attacker);
    attackerId = GetMonID(wk->attacker);
    defenderId = GetMonID(wk->defender);
    damage = AICalcDamage(wk->serverFlow, attackerId, defenderId, wk->moveId, TRUE, damageRoll);

    if (attackerIndex < 0 || damage == 0) {
        wk->result = 0;
        return wk->cmdReturn;
    }
    for (i = 0; i < count; i++) {
        mon = TrAI_GetPartyMon(party, i);
        monId = GetMonID(mon);
        if (CanPokemonBattle(mon) && i != attackerIndex) {
            wk->result = 2;
            for (j = 0; j < GetBattleMonMoveCount(mon); j++) {
                move = MoveGetID(mon, j);
                if (j != wk->moveIdx &&
                    AICalcDamage(wk->serverFlow, monId, defenderId, move, TRUE, damageRoll) > damage) {
                    wk->result = 1;
                    break;
                }
            }
        }
    }
    return wk->cmdReturn;
}

BOOL AIIsFainted(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckIfFainted(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AIIsNotFainted(VM *vm, void *work) {
    TrAIContext *wk = work;

    CheckIfFainted(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CheckIfFainted(VM *vm, TrAIContext *wk, u32 op) {
    u32 side = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op, IsFainted(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side))), TRUE, offset);
}

BOOL AIGetAbility(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetBattleMonStat(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, VM_Read32(vm))), 17);
    return wk->cmdReturn;
}

BOOL AIIfSubstitute(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    if (IsSubstituteActive(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, side)))) {
        VM_Jump(vm, vm->pc + offset);
    }
    return wk->cmdReturn;
}

BOOL AIGetSpecies(VM *vm, void *work) {
    TrAIContext *wk = work;

    wk->result = GetBattleMonSpecies(TrAI_GetBattleMon(wk, TrAI_GetTargetPos(wk, VM_Read32(vm))));
    return wk->cmdReturn;
}

BOOL AIGreaterThanTurnRandom(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareToTurnRandom(vm, wk, AI_CMP_LT);
    return wk->cmdReturn;
}

BOOL AILessThanTurnRandom(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareToTurnRandom(vm, wk, AI_CMP_GT);
    return wk->cmdReturn;
}

BOOL AIEqualToTurnRandom(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareToTurnRandom(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

BOOL AINotEqualToTurnRandom(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareToTurnRandom(vm, wk, AI_CMP_NE);
    return wk->cmdReturn;
}

void CompareToTurnRandom(VM *vm, TrAIContext *wk, u32 op) {
    s32 value = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    AIConditionalJump(vm, op, wk->random, value, offset);
}

// Jumps to the entry for the move's effect in a table of offsets, or ends the script if the table has no entry.
// Modes other than 0 end the script
BOOL AIJumpByMoveEffect(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 mode = VM_Read32(vm);
    s32 max = VM_Read32(vm);
    const u16 *table = (const u16 *)(vm->pc + (s32)VM_Read32(vm));
    s32 index;
    BOOL end = FALSE;

    if (mode == 0) {
        index = GetMoveData(wk, wk->moveId, MOVE_PARAM_EFFECT) * 2;
        if (index / 2 > max) {
            end = TRUE;
        }
    } else {
        end = TRUE;
    }
    if (end == TRUE) {
        return AIEnd(vm, wk);
    }
    VM_Jump(vm, (const u8 *)table + (table[index] | (table[index + 1] << 16)));
    return wk->cmdReturn;
}

// Jumps if overlay 169's check for position effect 3 at the position returns TRUE
BOOL func_ov170_02181734(VM *vm, void *work) {
    TrAIContext *wk = work;
    u32 side = VM_Read32(vm);
    s32 offset = VM_Read32(vm);

    if (BtlServerFlow_IsPosEffectFree(wk->serverFlow, TrAI_GetTargetPos(wk, side), 3)) {
        VM_Jump(vm, vm->pc + offset);
    }
    return wk->cmdReturn;
}

BOOL AIIsAtkLessThanSpAtk(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareOffensiveStats(vm, wk, AI_CMP_LT);
    return wk->cmdReturn;
}

BOOL AIIsAtkGreaterThanSpAtk(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareOffensiveStats(vm, wk, AI_CMP_GT);
    return wk->cmdReturn;
}

BOOL AIIsAtkEqualToSpAtk(VM *vm, void *work) {
    TrAIContext *wk = work;

    CompareOffensiveStats(vm, wk, AI_CMP_EQ);
    return wk->cmdReturn;
}

// The original compares the position with the special attack, and discards the attack
void CompareOffensiveStats(VM *vm, TrAIContext *wk, u32 op) {
    u32 side = VM_Read32(vm);
    s32 offset = VM_Read32(vm);
    u8 pos = TrAI_GetTargetPos(wk, side);

    GetBattleMonStat(TrAI_GetBattleMon(wk, pos), BATTLEMON_ATTACK);
    AIConditionalJump(vm, op, pos, GetBattleMonStat(TrAI_GetBattleMon(wk, pos), BATTLEMON_SP_ATTACK), offset);
}

BOOL AIConditionalJump(VM *vm, u32 op, s32 value, s32 ref, s32 offset) {
    BOOL jump = FALSE;

    switch (op) {
    case AI_CMP_LT:
        if (value < ref) {
            jump = TRUE;
        }
        break;
    case AI_CMP_GT:
        if (value > ref) {
            jump = TRUE;
        }
        break;
    case AI_CMP_EQ:
        if (value == ref) {
            jump = TRUE;
        }
        break;
    case AI_CMP_NE:
        if (value != ref) {
            jump = TRUE;
        }
        break;
    case AI_CMP_AND:
        if (value & ref) {
            jump = TRUE;
        }
        break;
    case AI_CMP_NAND:
        if (!(value & ref)) {
            jump = TRUE;
        }
        break;
    case AI_CMP_LE:
        if (value <= ref) {
            jump = TRUE;
        }
        break;
    case AI_CMP_GE:
        if (value >= ref) {
            jump = TRUE;
        }
        break;
    }
    if (jump == TRUE) {
        VM_Jump(vm, vm->pc + offset);
    }
    return jump;
}

u8 TrAI_GetTargetPos(TrAIContext *wk, u32 side) {
    u8 pos;
    u32 slot;

    switch (side) {
    case AI_BATTLER_ATTACKER:
        pos = wk->attackerPos;
        break;
    case AI_BATTLER_DEFENDER:
    default:
        pos = wk->defenderPos;
        break;
    case AI_BATTLER_ATTACKER_PARTNER:
        switch (wk->battleStyle) {
        case BTL_STYLE_SINGLE:
        case BTL_STYLE_ROTATION:
            pos = wk->attackerPos;
            break;
        case BTL_STYLE_DOUBLE:
            pos = wk->attackerPos ^ 2;
            break;
        case BTL_STYLE_TRIPLE:
            slot = wk->attackerPos / 2;
            if (slot != 2) {
                slot++;
            } else {
                slot = 1;
            }
            pos = GetPosOnSameSide(wk->attackerPos, slot);
            break;
        }
        break;
    case AI_BATTLER_DEFENDER_PARTNER:
        switch (wk->battleStyle) {
        case BTL_STYLE_SINGLE:
        case BTL_STYLE_ROTATION:
            pos = wk->defenderPos;
            break;
        case BTL_STYLE_DOUBLE:
            pos = wk->defenderPos ^ 2;
            break;
        case BTL_STYLE_TRIPLE:
            slot = wk->defenderPos / 2;
            if (slot != 2) {
                slot++;
            } else {
                slot = 1;
            }
            pos = GetPosOnSameSide(wk->defenderPos, slot);
            break;
        }
        break;
    }
    return pos;
}

// The AI knows the abilities of its own side. For the other side, it knows the abilities that were shown, and the
// trapping abilities, and otherwise guesses one of the species' abilities
u32 GuessAbility(TrAIContext *wk, u32 side, u8 pos) {
    BattleMon *mon = TrAI_GetBattleMon(wk, pos);
    u32 abilities[3];
    s32 count;
    u32 ability;
    u32 species;
    u32 form;
    u32 ability1;
    u32 ability2;
    u32 abilityHidden;

    if (CheckCondition(mon, CONDITION_GASTRO_ACID)) {
        return 0;
    }
    if (side == AI_BATTLER_DEFENDER || side == AI_BATTLER_DEFENDER_PARTNER) {
        wk->knownAbilities[pos] = BtlvEffect_GetAbility(pos);
        if (wk->knownAbilities[pos] != 0) {
            return wk->knownAbilities[pos];
        }
        ability = GetBattleMonStat(mon, BATTLEMON_ABILITY);
        if (ability == ABILITY_SHADOW_TAG || ability == ABILITY_MAGNET_PULL || ability == ABILITY_ARENA_TRAP) {
            return ability;
        }
        species = GetBattleMonSpecies(mon);
        form = GetBattleMonStat(mon, BATTLEMON_FORM);
        count = 0;
        ability1 = PML_PersonalGetParamSingle(species, form, PERSONAL_ABILITY_1);
        ability2 = PML_PersonalGetParamSingle(species, form, PERSONAL_ABILITY_2);
        abilityHidden = PML_PersonalGetParamSingle(species, form, PERSONAL_ABILITY_HIDDEN);
        if (ability1 != 0) {
            abilities[count++] = ability1;
        }
        if (ability2 != 0) {
            abilities[count++] = ability2;
        }
        if (abilityHidden != 0) {
            abilities[count++] = abilityHidden;
        }
        return abilities[GFL_RandomMTRange(count)];
    }
    return GetBattleMonStat(mon, BATTLEMON_ABILITY);
}

BattleMon *TrAI_GetBattleMon(TrAIContext *wk, u8 pos) {
    return PokeCon_GetPosMonInner(wk->pokeCon, pos);
}

BattleMon *TrAI_GetPartyMon(BattleParty *party, u8 index) {
    return GetBattleMonFromParty(party, index);
}

void TrAI_ResetScores(TrAIContext *wk) {
    s32 i;

    for (i = 0; i < TRAI_MOVE_MAX; i++) {
        wk->scores[i] = wk->initialScores[i];
    }
}

void TrAI_RememberDefenderMove(TrAIContext *wk) {
    BattleMon *defender = TrAI_GetBattleMon(wk, wk->defenderPos);
    s32 i;
    u16 move;

    for (i = 0; i < TRAI_MOVE_MAX; i++) {
        move = wk->knownMoves[wk->defenderPos][i];
        if (move == GetPreviousMoveID(defender)) {
            break;
        }
        if (move == 0) {
            wk->knownMoves[wk->defenderPos][i] = GetPreviousMoveID(defender);
            break;
        }
    }
}

s32 GetMoveData(TrAIContext *wk, u16 move, u32 param) {
    return PML_MoveGetParam(move, param);
}

BOOL GetMoveFlag(TrAIContext *wk, u16 move, u32 flag) {
    return getMoveFlag(move, flag);
}

u32 TrAI_GetMaxDamage(TrAIContext *wk, BattleMon *attacker, BattleMon *defender, u32 damageRoll) {
    u32 max = 0;
    u8 attackerId = GetMonID(attacker);
    u8 defenderId = GetMonID(defender);
    s32 i;
    u32 damage;

    for (i = 0; i < GetBattleMonMoveCount(attacker); i++) {
        damage = AICalcDamage(wk->serverFlow, attackerId, defenderId, MoveGetID(attacker, i), TRUE, damageRoll);
        if (damage > max) {
            max = damage;
        }
    }
    return max;
}

s32 GetItemData(TrAIContext *wk, u16 item, u32 param) {
    void *data = PML_ItemArcHandleReadFile(wk->itemArc, item, wk->heapId);
    s32 value = PML_ItemGetParam(data, param);

    GFL_HeapFree(data);
    return value;
}

void TrAI_CreateScriptCache(HeapID heapId) {
    s32 i;

    if (sScriptCache == NULL) {
        sScriptCache = GFL_HeapAllocate(heapId, sizeof(TrAIScriptCache), TRUE, "tr_ai.c", 3999);
        for (i = 0; i < TRAI_SCRIPT_CACHE_SIZE; i++) {
            sScriptCache->fileIds[i] = TRAI_SCRIPT_NONE;
        }
    }
}

void TrAI_FreeScriptCache(void) {
    s32 i;

    if (sScriptCache != NULL) {
        for (i = 0; i < TRAI_SCRIPT_CACHE_SIZE; i++) {
            if (sScriptCache->scripts[i] != NULL) {
                GFL_HeapFree(sScriptCache->scripts[i]);
                sScriptCache->scripts[i] = NULL;
            }
        }
        GFL_HeapFree(sScriptCache);
        sScriptCache = NULL;
    }
}

void *TrAI_LoadScript(TrAIContext *wk) {
    s32 i;
    u32 heapId;

    for (i = 0; i < TRAI_SCRIPT_CACHE_SIZE; i++) {
        if (wk->scriptIndex == sScriptCache->fileIds[i]) {
            sScriptCache->refCounts[i]++;
            return sScriptCache->scripts[i];
        }
    }
    for (i = 0; i < TRAI_SCRIPT_CACHE_SIZE; i++) {
        if (sScriptCache->fileIds[i] == TRAI_SCRIPT_NONE) {
            break;
        }
    }
    if (wk->scriptIndex == 2) {
        heapId = HEAPID_TAIL(wk->heapId);
    } else {
        heapId = wk->heapId;
    }
    sScriptCache->fileIds[i] = wk->scriptIndex;
    sScriptCache->refCounts[i]++;
    sScriptCache->scripts[i] = GFL_ArcToolReadHeapNew(wk->scriptArc, wk->scriptIndex, heapId);
    return sScriptCache->scripts[i];
}

void TrAI_ReleaseScript(TrAIContext *wk) {
    s32 i;

    for (i = 0; i < TRAI_SCRIPT_CACHE_SIZE; i++) {
        if (wk->scriptIndex == sScriptCache->fileIds[i]) {
            sScriptCache->refCounts[i]--;
            if (sScriptCache->refCounts[i] == 0) {
                sScriptCache->fileIds[i] = TRAI_SCRIPT_NONE;
                GFL_HeapFree(sScriptCache->scripts[i]);
                sScriptCache->scripts[i] = NULL;
            }
            break;
        }
    }
}
