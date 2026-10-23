#include "types.h"
#include "battle/btl_ability.h"
#include "battle/btl_event.h"
#include "battle/btl_faintrec.h"
#include "battle/btl_handler.h"
#include "battle/btl_handler_work.h"
#include "battle/btl_main.h"
#include "battle/btl_monset.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_pospoke.h"
#include "battle/btl_result.h"
#include "battle/btl_server.h"
#include "battle/btl_server_cmd.h"
#include "battle/btl_server_flow.h"
#include "battle/btl_server_flow_sub.h"
#include "battle/btl_tables.h"
#include "battle/handler_common.h"
#include "battle/side_handlers.h"
#include "constants/abilities.h"
#include "constants/items.h"
#include "constants/moves.h"
#include "constants/types.h"
#include "gfl/std.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "pml/waza.h"
#include "save/high_link.h"
#include "save/player_info.h"
#include "system/rtc.h"

static u8 CollectTargetsSingle(BtlServerFlow *flow, BattleMon *mon, u8 target, BtlFlowMoveParam *param, u8 redirect,
                               void *targets);
static u8 GetCurseTargetPos(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, u8 pos, u8 targetPos);
static u8 CollectTargetsDouble(BtlServerFlow *flow, BattleMon *mon, u8 target, BtlFlowMoveParam *param, u8 redirect,
                               void *targets);
static u8 CollectTargetsTriple(BtlServerFlow *flow, BattleMon *mon, u8 target, BtlFlowMoveParam *param, u8 redirect,
                               void *targets);
static BOOL RetargetFromFaintedFoe(BtlServerFlow *flow, u32 style, BattleMon *mon, BtlFlowMoveParam *param, u8 target,
                                   void *targets);
static BattleMon *GetOpposingMonAt(BtlServerFlow *flow, u8 pos, u8 index);
static BattleMon *GetAllyMonAt(BtlServerFlow *flow, u8 pos);
static u8 RedirectMoveTarget(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u8 targetId);
static u8 ServerEvent_DecideMoveTarget(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param);
static u8 ServerEvent_BaitMoveTarget(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u8 targetId);
static u32 ScaleExpGainedByLevel(BattleMon *mon, u32 exp, u16 level, u16 defeatedLevel);
static void AddEVs(BattleMon *mon, BattleMon *defeated, BtlFlowExpEntry *entry);
static BOOL ThrowBall(BtlServerFlow *flow, BattleMon *mon, u16 item);
static BOOL CheckBallCapture(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 item, u8 *shakes,
                             u8 *critical);
static fx32 GetDarkGrassCaptureMod(BtlServerFlow *flow);
static fx32 GetBallCaptureMod(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 item);
static BOOL CheckCriticalCapture(BtlServerFlow *flow, fx32 value);
static BOOL BagItemCureSleep(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemCurePoison(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemCureBurn(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemCureFreeze(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemCureParalysis(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemCureConfusion(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemCureInfatuation(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemGuardSpec(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemRevive(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemRaiseAttack(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemRaiseDefense(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemRaiseSpAttack(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemRaiseSpDefense(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemRaiseSpeed(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemRaiseAccuracy(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemDireHit(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemRestorePP(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 slot);
static BOOL BagItemRestoreAllPP(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemRestoreHP(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemCureCondition(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u32 condition);
static BOOL BagItemCureMentalConditions(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemRaiseStat(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u32 stat);
static BOOL BagItemItemUrge(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemAbilityUrge(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemItemDrop(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL BagItemResetUrge(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
static BOOL HasFaintedMonOfSpecies(BattleParty *party, s16 species);
static BOOL HasFaintedMonOfOtherSpecies(BattleParty *party, s16 species);
static s32 CountFaintedMons(BattleParty *party);
static BOOL IsPartyAllFainted(BattleParty *party);
static s16 GetRulesTargetSpecies(BtlMainModule *mainModule, const BtlScriptedRules *rules);
static s32 GetLastRuleOutcome(const s32 *outcomes);
static s32 CheckRulesTurnLimit(BtlMainModule *mainModule, BOOL won);
static u32 JudgeByRemainingMons(BtlServerFlow *flow, const u8 *alive, const u8 *total, u8 side);
static void GetSideHP(BtlServerFlow *flow, s32 *hp, s32 *percent, u8 side);

// Fills targets with the mons a move hits, from its target type and the battle style
u8 CollectMoveTargets(BtlServerFlow *flow, BattleMon *mon, u8 target, BtlFlowMoveParam *param, void *targets) {
    u32 style;
    u8 redirect;
    u8 count;
    BattleMon *first;

    style = BtlSetup_GetBattleStyle(flow->mainModule);
    redirect = 0x1f;
    if (!param->flags.unk0) {
        redirect = ServerEvent_DecideMoveTarget(flow, mon, param);
    }
    BtlMonSet_Clear(targets);
    flow->unk77F = 6;
    switch (style) {
    case BTL_STYLE_SINGLE:
    default:
        count = CollectTargetsSingle(flow, mon, target, param, redirect, targets);
        break;
    case BTL_STYLE_DOUBLE:
        count = CollectTargetsDouble(flow, mon, target, param, redirect, targets);
        break;
    case BTL_STYLE_TRIPLE:
        count = CollectTargetsTriple(flow, mon, target, param, redirect, targets);
        break;
    case BTL_STYLE_ROTATION:
        count = CollectTargetsSingle(flow, mon, target, param, redirect, targets);
        break;
    }
    if (count == 1) {
        first = BtlMonSet_Get(targets, 0);
        if (first != NULL) {
            flow->unk77F = BtlPosPoke_GetPosOfMon(&flow->posPoke, GetMonID(first));
        }
        if (!RetargetFromFaintedFoe(flow, style, mon, param, target, targets)) {
            count = 0;
        }
    }
    return count;
}

// The targets in a single or rotation battle
static u8 CollectTargetsSingle(BtlServerFlow *flow, BattleMon *mon, u8 target, BtlFlowMoveParam *param, u8 redirect,
                               void *targets) {
    u8 pos;

    pos = MonIDToBattlePos(flow->mainModule, flow->pokeCon, GetMonID(mon));
    switch (param->targetType) {
    case 0:
    case 3:
    case 4:
    case 5:
    case 9:
        BtlMonSet_Add(targets,
                      redirect == 0x1f ? GetOpposingMonAt(flow, pos, 0) : GetPokeParam(flow->pokeCon, redirect));
        return 1;
    case 1:
    case 7:
        if (redirect == 0x1f) {
            BtlMonSet_Add(targets, mon);
        } else {
            BtlMonSet_Add(targets, GetPokeParam(flow->pokeCon, redirect));
        }
        return 1;
    case 8:
        BtlMonSet_Add(targets, mon);
        BtlMonSet_Add(targets, GetOpposingMonAt(flow, pos, 0));
        return 2;
    case 13:
        if (redirect != 0x1f) {
            BtlMonSet_Add(targets, GetPokeParam(flow->pokeCon, redirect));
            return 1;
        }
        return 0;
    }
    return 0;
}

// Curse picks its own target: the user itself, or a foe for a Ghost type
static u8 GetCurseTargetPos(BtlServerFlow *flow, BtlFlowMoveParam *param, BattleMon *mon, u8 pos, u8 targetPos) {
    u16 move = param->move;

    if (move == MOVE_CURSE) {
        if (param->targetType == 0) {
            if (pos == targetPos || targetPos == 6) {
                return DecideMoveTargetAuto(flow->mainModule, flow->pokeCon, mon, MOVE_CURSE);
            }
        } else if (param->targetType == 7 && pos != targetPos) {
            return pos;
        }
    }
    return targetPos;
}

// The targets in a double battle
static u8 CollectTargetsDouble(BtlServerFlow *flow, BattleMon *mon, u8 target, BtlFlowMoveParam *param, u8 redirect,
                               void *targets) {
    u8 pos;
    u8 targetPos;
    BattleMon *single;
    u8 newTarget;

    pos = MonIDToBattlePos(flow->mainModule, flow->pokeCon, GetMonID(mon));
    targetPos = GetCurseTargetPos(flow, param, mon, pos, target);
    switch (param->targetType) {
    case 0:
        single = PokeCon_GetPosMon(flow->pokeCon, targetPos);
        break;
    case 3:
        single = PokeCon_GetPosMon(flow->pokeCon, targetPos);
        break;
    case 9:
        single = GetOpposingMonAt(flow, pos, BattleRandom(2));
        break;
    case 5:
        BtlMonSet_Add(targets, GetOpposingMonAt(flow, pos, 0));
        BtlMonSet_Add(targets, GetOpposingMonAt(flow, pos, 1));
        return 2;
    case 4:
        BtlMonSet_Add(targets, GetAllyMonAt(flow, pos));
        BtlMonSet_Add(targets, GetOpposingMonAt(flow, pos, 0));
        BtlMonSet_Add(targets, GetOpposingMonAt(flow, pos, 1));
        return 3;
    case 8:
        BtlMonSet_Add(targets, mon);
        BtlMonSet_Add(targets, GetAllyMonAt(flow, pos));
        BtlMonSet_Add(targets, GetOpposingMonAt(flow, pos, 0));
        BtlMonSet_Add(targets, GetOpposingMonAt(flow, pos, 1));
        return 3;
    case 7:
        if (redirect == 0x1f) {
            BtlMonSet_Add(targets, mon);
        } else {
            BtlMonSet_Add(targets, GetPokeParam(flow->pokeCon, redirect));
        }
        return 1;
    case 1:
        BtlMonSet_Add(targets, PokeCon_GetPosMon(flow->pokeCon, targetPos));
        return 1;
    case 2:
        BtlMonSet_Add(targets, GetAllyMonAt(flow, pos));
        return 1;
    case 13:
        if (redirect != 0x1f) {
            single = GetPokeParam(flow->pokeCon, redirect);
            break;
        }
        return 0;
    default:
        return 0;
    }
    if (single != NULL) {
        newTarget = RedirectMoveTarget(flow, mon, param, GetMonID(single));
        if (newTarget != 0x1f) {
            single = GetPokeParam(flow->pokeCon, newTarget);
        }
        BtlMonSet_Add(targets, single);
        return 1;
    }
    return 0;
}

// The targets in a triple battle, where only the mons next to each other reach each other
static u8 CollectTargetsTriple(BtlServerFlow *flow, BattleMon *mon, u8 target, BtlFlowMoveParam *param, u8 redirect,
                               void *targets) {
    u8 pos;
    const AdjacentOpponentData *adjacent;
    u8 targetPos;
    BattleMon *single;
    u32 i;
    u32 j;
    u32 count;
    u8 newTarget;
    u8 index;

    pos = MonIDToBattlePos(flow->mainModule, flow->pokeCon, GetMonID(mon));
    adjacent = GetAdjacentOpponentData(pos);
    targetPos = GetCurseTargetPos(flow, param, mon, pos, target);
    switch (param->targetType) {
    case 0:
        single = PokeCon_GetPosMon(flow->pokeCon, targetPos);
        break;
    case 3:
        single = PokeCon_GetPosMon(flow->pokeCon, targetPos);
        break;
    case 9:
        index = BattleRandom(adjacent->count1);
        single = PokeCon_GetPosMon(flow->pokeCon, adjacent->list1[index]);
        break;
    case 5:
        for (i = 0; i < adjacent->count1; i++) {
            BtlMonSet_Add(targets, PokeCon_GetPosMon(flow->pokeCon, adjacent->list1[i]));
        }
        return adjacent->count1;
    case 4:
        count = 0;
        for (i = 0; i < adjacent->count1; i++) {
            BtlMonSet_Add(targets, PokeCon_GetPosMon(flow->pokeCon, adjacent->list1[i]));
            count++;
        }
        for (j = 0; j < adjacent->count2; j++) {
            if (pos != adjacent->list2[j]) {
                BtlMonSet_Add(targets, PokeCon_GetPosMon(flow->pokeCon, adjacent->list2[j]));
                count++;
            }
        }
        return count;
    case 8:
        BtlMonSet_Add(targets, mon);
        for (i = 0; i < 3; i++) {
            u8 allyPos = GetPosOnSameSide(pos, i);

            if (allyPos != pos) {
                BtlMonSet_Add(targets, PokeCon_GetPosMon(flow->pokeCon, allyPos));
            }
        }
        BtlMonSet_Add(targets, GetOpposingMonAt(flow, pos, 0));
        BtlMonSet_Add(targets, GetOpposingMonAt(flow, pos, 1));
        BtlMonSet_Add(targets, GetOpposingMonAt(flow, pos, 2));
        return 6;
    case 7:
        if (redirect == 0x1f) {
            BtlMonSet_Add(targets, mon);
        } else {
            BtlMonSet_Add(targets, GetPokeParam(flow->pokeCon, redirect));
        }
        return 1;
    case 1:
        BtlMonSet_Add(targets, PokeCon_GetPosMon(flow->pokeCon, targetPos));
        return 1;
    case 2:
        BtlMonSet_Add(targets, PokeCon_GetPosMon(flow->pokeCon, targetPos));
        return 1;
    case 13:
        if (redirect != 0x1f) {
            single = GetPokeParam(flow->pokeCon, redirect);
            break;
        }
        return 0;
    default:
        return 0;
    }
    if (single != NULL) {
        newTarget = RedirectMoveTarget(flow, mon, param, GetMonID(single));
        if (newTarget != 0x1f) {
            single = GetPokeParam(flow->pokeCon, newTarget);
        }
        BtlMonSet_Add(targets, single);
        return 1;
    }
    return 0;
}

// When a single target has fainted before the move, the move goes to the nearest foe still standing instead, or the
// one with more HP, or a random one of two
static BOOL RetargetFromFaintedFoe(BtlServerFlow *flow, u32 style, BattleMon *mon, BtlFlowMoveParam *param, u8 target,
                                   void *targets) {
    BattleMon *fainted;
    u8 pos;
    u8 numPositions;
    u8 positions[8];
    BattleMon *mons[3];
    u8 i;
    u8 count;
    s32 dist0;
    s32 dist1;
    u8 index;
    u16 code;

    if (BtlCalc_GetFrontPosCount(style) > 1 && BtlMonSet_GetCount(targets) == 1) {
        fainted = BtlMonSet_Get(targets, 0);
        if (IsFainted(fainted) && !IsAllyMonID(GetMonID(mon), GetMonID(fainted))) {
            pos = MonIDToBattlePos(flow->mainModule, flow->pokeCon, GetMonID(mon));
            BtlMonSet_Remove(targets, fainted);
            if (getMoveFlag(param->move, 0xb)) {
                code = 0x600 | pos;
            } else {
                code = 0x100 | pos;
            }
            numPositions = ExpandPosFlags(flow->mainModule, code, positions);
            for (i = 0; i < 3; i++) {
                mons[i] = NULL;
            }
            for (count = 0, i = 0; i < numPositions; i++) {
                mons[count] = PokeCon_GetPosMon(flow->pokeCon, positions[i]);
                if (!IsFainted(mons[count])) {
                    positions[count] = positions[i];
                    count++;
                }
            }
            if (count == 0) {
                return FALSE;
            }
            if (count != 2) {
                index = 0;
            } else {
                dist0 = positions[0] - target;
                if (dist0 < 0) {
                    dist0 *= -1;
                }
                dist1 = positions[1] - target;
                if (dist1 < 0) {
                    dist1 *= -1;
                }
                if (dist0 == dist1) {
                    dist0 = GetBattleMonStat(mons[0], 0xd);
                    dist1 = GetBattleMonStat(mons[1], 0xd);
                }
                if (dist0 < dist1) {
                    index = 0;
                } else if (dist1 < dist0) {
                    index = 1;
                } else {
                    index = BattleRandom(2);
                }
            }
            if (mons[index] != NULL) {
                BtlMonSet_Add(targets, mons[index]);
                return TRUE;
            }
        }
    }
    return FALSE;
}

// The mon on the other side at an index from a position
static BattleMon *GetOpposingMonAt(BtlServerFlow *flow, u8 pos, u8 index) {
    u8 clientId;
    u8 slot;

    GetPosClientAndIndex(flow->mainModule, GetOpposingPos(flow->mainModule, pos, index), &clientId, &slot);
    return BattleParty_GetMon(BattleServer_GetClient(flow->server, clientId)->party, slot);
}

// The mon beside a position
static BattleMon *GetAllyMonAt(BtlServerFlow *flow, u8 pos) {
    u8 clientId;
    u8 slot;

    GetPosClientAndIndex(flow->mainModule, GetPartnerPos(flow->mainModule, pos), &clientId, &slot);
    return BattleParty_GetMon(BattleServer_GetClient(flow->server, clientId)->party, slot);
}

static u8 RedirectMoveTarget(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u8 targetId) {
    u32 state;
    u8 result;

    state = PushState(&flow->actionState, 0x26b);
    result = ServerEvent_BaitMoveTarget(flow, mon, param, targetId);
    PopState(&flow->actionState, state, 0x26d);
    return result;
}

// The mon that draws a move to itself, as Follow Me and Lightning Rod do, or 0x1f for none
static u8 ServerEvent_DecideMoveTarget(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param) {
    u8 monId;

    BattleEventVar_Push(0x27f);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetRewriteOnceValue(4, 0x1f);
    BattleEventVar_SetRewriteOnceValue(0x50, FALSE);
    BattleEvent_CallHandlers(flow, 0x29);
    monId = BattleEventVar_GetValue(4);
    BattleEventVar_Pop(0x287);
    return monId;
}

// The mon that takes a move over from its target, as with Lightning Rod, or 0x1f for none
static u8 ServerEvent_BaitMoveTarget(BtlServerFlow *flow, BattleMon *mon, BtlFlowMoveParam *param, u8 targetId) {
    u8 newTarget;

    BattleEventVar_Push(0x298);
    BattleEventVar_SetConstValue(3, GetMonID(mon));
    BattleEventVar_SetConstValue(0x16, param->type);
    BattleEventVar_SetConstValue(0x12, param->move);
    BattleEventVar_SetRewriteOnceValue(4, targetId);
    BattleEvent_CallHandlers(flow, 0x2a);
    newTarget = BattleEventVar_GetValue(4);
    BattleEventVar_Pop(0x29f);
    if (newTarget == targetId) {
        newTarget = 0x1f;
    }
    return newTarget;
}

// Function name from swan.
void AddExpAndEVs(BtlServerFlow *flow, BattleParty *party, BattleMon *defeated, BtlFlowExpEntry *entries) {
    BtlSetup *setup;
    u32 baseExp;
    u16 numMons;
    u16 numExpShare;
    u16 i;
    BattleMon *mon;
    u32 shareExp;
    u8 numFaced;
    u8 numStanding;
    u32 exp;
    PlayerInfo *player;
    PartyPkm *pkm;
    u16 level;
    u16 defeatedLevel;
    BtlFlowExpEntry *entry;
    u16 j;
    u8 monId;
    u8 k;

    setup = GetBattleSetupParam(flow->mainModule);
    baseExp = CalcBaseExpGain(defeated, setup->levelDiff);
    numMons = GetNumMonsInParty(party);
    numExpShare = 0;
    if (BtlSetup_GetBattleType(flow->mainModule) == 1) {
        baseExp = baseExp * 15 / 10;
    }
    for (i = 0; i < 6; i++) {
        MI_CpuFill8(&entries[i], 0, sizeof(BtlFlowExpEntry));
    }
    // An Exp. Share takes half the experience, split among the mons that hold one
    for (i = 0; i < numMons; i++) {
        mon = GetBattleMonFromParty(party, i);
        if (!IsFainted(mon) && GetBattleMonHeldItem(mon) == ITEM_EXP_SHARE) {
            numExpShare++;
        }
    }
    if (numExpShare != 0) {
        shareExp = baseExp / 2;
        baseExp -= shareExp;
        shareExp /= numExpShare;
        if (shareExp == 0) {
            shareExp = 1;
        }
        for (i = 0; i < numMons; i++) {
            mon = GetBattleMonFromParty(party, i);
            if (!IsFainted(mon) && GetBattleMonHeldItem(mon) == ITEM_EXP_SHARE) {
                entries[i].exp = shareExp;
            }
        }
    }
    // The rest is split among the mons that faced the defeated one and are still standing
    numFaced = BattleMon_GetConfrontedMonCount(defeated);
    numStanding = 0;
    for (i = 0; i < numFaced; i++) {
        if (!IsFainted(GetPokeParam(flow->pokeCon, BattleMon_GetConfrontedMonID(defeated, i)))) {
            numStanding++;
        }
    }
    exp = baseExp / numStanding;
    if (exp == 0) {
        exp = 1;
    }
    for (j = 0; j < numMons; j++) {
        mon = GetBattleMonFromParty(party, j);
        if (!IsFainted(mon)) {
            monId = GetMonID(mon);
            for (k = 0; k < numFaced; k++) {
                if (monId == BattleMon_GetConfrontedMonID(defeated, k)) {
                    entries[j].exp += exp;
                }
            }
        }
    }
    for (i = 0; i < numMons; i++) {
        entry = &entries[i];
        if (entries[i].exp != 0) {
            player = GetPlayerInfo(flow->mainModule);
            mon = GetBattleMonFromParty(party, i);
            pkm = GetSrcData(mon);
            level = GetBattleMonStat(mon, 0xf);
            defeatedLevel = GetBattleMonStat(defeated, 0xf);
            if (setup->levelDiff < 0) {
                defeatedLevel += (u16)MATH_ABS(setup->levelDiff);
            }
            entry->exp = ScaleExpGainedByLevel(mon, entry->exp, level, defeatedLevel);
            // A traded mon gets half again as much, or 1.7 times from a game in another language
            if (!IsTrainerOT(pkm, player)) {
                entry->exp = fixed_round(
                    entry->exp, PokeParty_GetParam(pkm, 0xc, NULL) != TrainerInfo_GetRegion(player) ? 0x1b33 : 0x1800);
                entry->boosted = TRUE;
            }
            if (GetBattleMonHeldItem(mon) == ITEM_LUCKY_EGG) {
                entry->exp = fixed_round(entry->exp, 0x1800);
                entry->boosted = TRUE;
            }
            entry->exp = PassPower_ApplyEXP(entry->exp);
            if (entry->exp > 100000) {
                entry->exp = 100000;
            }
        }
    }
    for (i = 0; i < numMons; i++) {
        if (entries[i].exp != 0) {
            AddEVs(BattleParty_GetMon(party, i), defeated, &entries[i]);
        }
    }
}

// Function name from swan.
static u32 ScaleExpGainedByLevel(BattleMon *mon, u32 exp, u16 level, u16 defeatedLevel) {
    u32 num = defeatedLevel * 2 + 10;
    u32 den = defeatedLevel + level + 10;
    fx32 denSqrt;
    u32 result;
    u32 max;

    // Scaled by ((2 * defeated level + 10) / (defeated level + level + 10)) to the 2.5th power
    num = (FX_Sqrt(FX32_CONST(num)) * (num * num)) >> FX32_SHIFT;
    denSqrt = FX_Sqrt(FX32_CONST(den));
    den = den * den;
    result = (u64)exp * num / ((den * denSqrt) >> FX32_SHIFT) + 1;
    max = GetExpForLv100(mon);
    if (result > max) {
        result = max;
    }
    return result;
}

// For each stat: its effort value in the personal data, its field in the party data, and its Power item
typedef struct {
    u8 personalParam;
    u16 field;
    u16 powerItem;
} BtlFlowEVParam;

// clang-format off
static const BtlFlowEVParam data_ov167_021d6cfc[6] = {
    { 0x0a, 0x0d, 0x126 },
    { 0x0b, 0x0e, 0x121 },
    { 0x0c, 0x0f, 0x122 },
    { 0x0d, 0x10, 0x125 },
    { 0x0e, 0x11, 0x123 },
    { 0x0f, 0x12, 0x124 },
};

// clang-format on

// Function name from swan.
static void AddEVs(BattleMon *mon, BattleMon *defeated, BtlFlowExpEntry *entry) {
    u16 species;
    u16 form;
    u8 evs[6];
    u8 i;
    PartyPkm *pkm;
    BOOL wasEncrypted;
    u32 field;
    u32 total;

    species = GetBattleMonSpecies(defeated);
    form = GetBattleMonStat(defeated, 0x13);
    for (i = 0; i < 6; i++) {
        evs[i] = PML_PersonalGetParamSingle(species, form, data_ov167_021d6cfc[i].personalParam);
    }
    if (GetBattleMonHeldItem(mon) == ITEM_MACHO_BRACE) {
        for (i = 0; i < 6; i++) {
            evs[i] *= 2;
        }
    }
    for (i = 0; i < 6; i++) {
        u16 item = data_ov167_021d6cfc[i].powerItem;

        if (item == GetBattleMonHeldItem(mon)) {
            evs[i] += (u8)ItemGetParam(item, 2);
        }
    }
    pkm = GetSrcData(mon);
    wasEncrypted = PokeParty_DecryptPkm(pkm);
    if (PokeParty_GetParam(pkm, 0x97, NULL)) {
        for (i = 0; i < 6; i++) {
            evs[i] *= 2;
        }
    }
    for (i = 0; i < 6; i++) {
        if (evs[i] != 0) {
            field = data_ov167_021d6cfc[i].field;
            total = evs[i] + PokeParty_GetParam(pkm, field, NULL);
            if (total > 255) {
                total = 255;
            }
            PokeParty_SetParam(pkm, field, total);
            switch (i) {
            case 0:
                entry->hp = evs[i];
                break;
            case 1:
                entry->attack = evs[i];
                break;
            case 2:
                entry->defense = evs[i];
                break;
            case 3:
                entry->speed = evs[i];
                break;
            case 4:
                entry->spAttack = evs[i];
                break;
            case 5:
                entry->spDefense = evs[i];
                break;
            }
        }
    }
    PokeParty_EncryptPkm(pkm, wasEncrypted);
}

// An effect of the items a trainer uses from the bag, which applies to the items whose data has the parameter, or to
// the item itself when exact is set
typedef struct {
    u16 param;
    // 0 for any mon, 1 for one in battle, 2 only where any item can be used
    u8 place;
    u8 exact;
    BOOL (*func)(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param);
} BtlFlowItemEffect;

static const BtlFlowItemEffect data_ov167_021d6d20[23] = {
    { 0x12, 0, FALSE, BagItemCureSleep },
    { 0x13, 0, FALSE, BagItemCurePoison },
    { 0x14, 0, FALSE, BagItemCureBurn },
    { 0x15, 0, FALSE, BagItemCureFreeze },
    { 0x16, 0, FALSE, BagItemCureParalysis },
    { 0x17, 0, FALSE, BagItemCureConfusion },
    { 0x18, 0, FALSE, BagItemCureInfatuation },
    { 0x19, 1, FALSE, BagItemGuardSpec },
    { 0x1a, 0, FALSE, BagItemRevive },
    { 0x1e, 1, FALSE, BagItemRaiseAttack },
    { 0x1f, 1, FALSE, BagItemRaiseDefense },
    { 0x20, 1, FALSE, BagItemRaiseSpAttack },
    { 0x21, 1, FALSE, BagItemRaiseSpDefense },
    { 0x22, 1, FALSE, BagItemRaiseSpeed },
    { 0x23, 1, FALSE, BagItemRaiseAccuracy },
    { 0x24, 1, FALSE, BagItemDireHit },
    { 0x27, 0, FALSE, BagItemRestorePP },
    { 0x28, 0, FALSE, BagItemRestoreAllPP },
    { 0x29, 0, FALSE, BagItemRestoreHP },
    { ITEM_ITEM_URGE, 2, TRUE, BagItemItemUrge },
    { ITEM_ABILITY_URGE, 2, TRUE, BagItemAbilityUrge },
    { ITEM_ITEM_DROP, 2, TRUE, BagItemItemDrop },
    { ITEM_RESET_URGE, 2, TRUE, BagItemResetUrge },
};

// A trainer uses an item from the bag on a party mon
u8 UseBagItem(BtlServerFlow *flow, BattleMon *mon, u16 item, u8 param, u8 slot) {
    u8 numOut;
    BattleParty *party;
    u8 clientId;
    u8 targetId;
    u8 place;
    BattleMon *target;
    u32 state;
    u8 targetPos;
    BOOL used;
    u32 reserve;
    BOOL usable;
    u32 placed;
    u32 i;
    const BtlFlowItemEffect *effect;
    BOOL result;
    s32 value;
    u16 effectId;

    clientId = GetClientIDFromMonID(GetMonID(mon));
    target = NULL;
    placed = 0;
    if (slot != 6) {
        party = GetPartyData(flow->pokeCon, clientId);
        numOut = GetClientBattlerCount(flow->mainModule, clientId);
        target = BattleParty_GetMon(party, slot);
        targetId = GetMonID(target);
        targetPos = GetBattlePos(&flow->posPoke, targetId);
        // 0 for a mon in battle, 1 for one in a rotation battle's back slots, 2 for one in the party
        if (slot >= numOut) {
            placed = 2;
        }
        place = placed;
        if (BtlSetup_GetBattleStyle(flow->mainModule) == BTL_STYLE_ROTATION && place == 2 && slot < 3) {
            place = 1;
        }
    }
    if (flow->unk18 != 1) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x21, clientId, item, SC_MSG_ARG_END);
        if (ItemGetParam(item, 0xf) == 4) {
            if (ThrowBall(flow, mon, item)) {
                BattleClient_SubItem(flow->mainModule, clientId, item);
            }
            return FALSE;
        }
        if (ItemGetParam(item, 7) == 3) {
            BattleClient_SubItem(flow->mainModule, clientId, item);
            return TRUE;
        }
    } else {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x24, clientId, targetId, item, SC_MSG_ARG_END);
        if (clientId == GetPlayerClientID(flow->mainModule)) {
            IncRecord(flow->mainModule, 0x6f);
        }
    }
    usable = place == 0 && targetPos != 6 ? TRUE : FALSE;
    if (!usable) {
        for (i = 0; i < 23; i++) {
            effect = &data_ov167_021d6d20[i];
            if ((effect->exact && item == effect->param) || (!effect->exact && ItemGetParam(item, effect->param))) {
                if (effect->place == 0) {
                    usable = TRUE;
                    break;
                }
                if (effect->place == 1 && place < 2 && targetPos != 6) {
                    usable = TRUE;
                    break;
                }
            }
        }
    }
    if (!usable) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0xc4, SC_MSG_ARG_END);
        return FALSE;
    }
    if (flow->unk18 == 1) {
        if (targetPos != 6) {
            BtlServerCmdQueue_Put(flow->queue, SC_EFFECT_BY_POS, targetPos, 0x282);
        }
        for (i = 0; i < 23; i++) {
            if (data_ov167_021d6d20[i].exact && item == data_ov167_021d6d20[i].param) {
                result = FALSE;
                if (!BtlServerFlow_IsSemiInvuln(flow, targetId)) {
                    u32 reservePos = SCQUE_RESERVE_Pos(flow->queue, SC_EFFECT_BY_POS);

                    result = data_ov167_021d6d20[i].func(flow, target, item, 0, param);
                    if (result == TRUE) {
                        BtlServerCmdQueue_PutReserved(flow->queue, reservePos, SC_EFFECT_BY_POS, targetPos, 0x25e);
                        BtlServerCmdQueue_Put(flow->queue, SC_EFFECT, 0x236);
                        return FALSE;
                    }
                }
                if (result == FALSE) {
                    BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x44, SC_MSG_ARG_END);
                }
                BtlServerCmdQueue_Put(flow->queue, SC_EFFECT, 0x236);
                return FALSE;
            }
        }
    }
    used = FALSE;
    reserve = SCQUE_RESERVE_Pos(flow->queue, SC_EFFECT_BY_POS);
    state = PushState(&flow->actionState, 0x4d7);
    for (i = 0; i < 23; i++) {
        effect = &data_ov167_021d6d20[i];
        value = ItemGetParam(item, data_ov167_021d6d20[i].param);
        if (value != 0 && effect->func(flow, target, item, value, param)) {
            used = TRUE;
        }
    }
    if (used) {
        effectId = targetPos != 6 ? 0x25e : 0x292;
        BtlServerCmdQueue_PutReserved(flow->queue, reserve, SC_EFFECT_BY_POS, targetPos, effectId);
        if (flow->unk18 != 1) {
            BattleClient_SubItem(flow->mainModule, clientId, item);
            ChangeFriendshipByItem(flow->mainModule, target, item);
        }
    } else {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_STD, 0x44, SC_MSG_ARG_END);
    }
    PopState(&flow->actionState, state, 0x4f4);
    BtlServerCmdQueue_Put(flow->queue, SC_EFFECT, 0x236);
    return FALSE;
}

// Throws a ball at the first foe standing; only a wild one can be caught
static BOOL ThrowBall(BtlServerFlow *flow, BattleMon *mon, u16 item) {
    u8 positions[6];
    u8 shakes;
    u8 critical;
    BattleMon *target;
    u8 targetPos;
    u8 numPositions;
    u8 i;
    u8 caught;
    u8 isNew;

    target = NULL;
    targetPos = 6;
    numPositions = ExpandPosFlags(flow->mainModule,
                                  0x600 | MonIDToBattlePos(flow->mainModule, flow->pokeCon, GetMonID(mon)), positions);
    for (i = 0; i < numPositions; i++) {
        target = PokeCon_GetPosMon(flow->pokeCon, positions[i]);
        if (!IsFainted(target)) {
            targetPos = positions[i];
            break;
        }
    }
    if (BtlSetup_GetBattleType(flow->mainModule) == 0) {
        if (targetPos != 6) {
            caught = CheckBallCapture(flow, mon, target, item, &shakes, &critical);
            if (caught) {
                flow->unk14 = 6;
                flow->unk784 = targetPos;
                isNew = !IsMonCaught(flow->mainModule, target) ? TRUE : FALSE;
                BattleMon_SetCaughtBall(target, item);
            } else {
                isNew = FALSE;
            }
            BtlServerCmdQueue_Put(flow->queue, SC_BALL_THROW, targetPos, shakes, caught, isNew, critical, item);
        }
        return TRUE;
    }
    if (targetPos != 6) {
        BtlServerCmdQueue_Put(flow->queue, SC_BALL_THROW_TRAINER, targetPos, item);
    }
    return FALSE;
}

// Whether a ball catches its target, with how many times it shakes and whether it was a critical capture
static BOOL CheckBallCapture(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 item, u8 *shakes,
                             u8 *critical) {
    u32 maxHP;
    u32 base;
    fx32 rate;
    u16 species;
    u16 form;
    fx32 value;
    u32 numShakes;
    u32 i;

    *critical = FALSE;
    if (item == ITEM_MASTER_BALL) {
        *shakes = 3;
        return TRUE;
    }
    maxHP = GetBattleMonStat(target, 0xe) * 3;
    base = maxHP - GetBattleMonStat(target, 0xd) * 2;
    rate = FX32_CONST(base);
    if (BtlSetup_IsBattleType(flow->mainModule, 0x20)) {
        rate = FX_MUL(rate, GetDarkGrassCaptureMod(flow));
    }
    species = GetBattleMonSpecies(target);
    form = GetBattleMonStat(target, 0x13);
    rate *= (u16)PML_PersonalGetParamSingle(species, form, 8);
    rate = (u32)FX_MUL(rate, GetBallCaptureMod(flow, mon, target, item)) / maxHP;
    switch (GetBattleMonStatus(target)) {
    case 2:
    case 3:
        rate = FX_MUL(rate, FX32_CONST(2.5));
        break;
    case 1:
    case 4:
    case 5:
        rate = FX_MUL(rate, FX32_CONST(1.5));
        break;
    }
    value = PassPower_ApplyCapture(rate);
    *critical = CheckCriticalCapture(flow, value);
    if (value >= FX32_CONST(255)) {
        *shakes = *critical ? 1 : 3;
        return TRUE;
    }
    numShakes = *critical ? 1 : 3;
    value = FX_Div(FX32_CONST(65536), FX_Sqrt(FX_Sqrt(FX_Div(FX32_CONST(255), value)))) >> FX32_SHIFT;
    *shakes = 0;
    for (i = 0; i < numShakes; i++) {
        if (BattleRandom(65536) < value) {
            (*shakes)++;
        } else {
            return FALSE;
        }
    }
    return TRUE;
}

// In dark grass, catching gets harder the fewer species the player has caught
static fx32 GetDarkGrassCaptureMod(BtlServerFlow *flow) {
    u32 count = GetDexCaughtCount(flow->mainModule);

    if (count > 600) {
        return FX32_ONE;
    }
    if (count > 450) {
        return FX32_CONST(0.9);
    }
    if (count > 300) {
        return FX32_CONST(0.8);
    }
    if (count > 150) {
        return FX32_CONST(0.7);
    }
    if (count > 30) {
        return FX32_CONST(0.5);
    }
    return FX32_CONST(0.3);
}

// The catch rate multiplier of a ball
static fx32 GetBallCaptureMod(BtlServerFlow *flow, BattleMon *mon, BattleMon *target, u16 item) {
    switch (item) {
    case ITEM_GREAT_BALL:
        return FX32_CONST(1.5);
    case ITEM_ULTRA_BALL:
        return FX32_CONST(2);
    case ITEM_NET_BALL:
        if (DoesMonHaveType(target, TYPE_WATER) || DoesMonHaveType(target, TYPE_BUG)) {
            return FX32_CONST(3);
        }
        break;
    case ITEM_DIVE_BALL:
        if (BtlSetup_IsBattleType(flow->mainModule, 1)) {
            return FX32_CONST(3.5);
        }
        if (GetFieldEffectData(flow->mainModule)->env.terrain == 6) {
            return FX32_CONST(3.5);
        }
        break;
    case ITEM_NEST_BALL: {
        u16 level = GetBattleMonStat(target, 0xf);

        if (level < 30) {
            u16 bonus = 41 - level;

            if (bonus > 40) {
                bonus = 40;
            }
            return FX32_CONST(bonus) / 10;
        }
        break;
    }
    case ITEM_REPEAT_BALL:
        if (IsMonCaught(flow->mainModule, target)) {
            return FX32_CONST(3);
        }
        break;
    case ITEM_TIMER_BALL: {
        fx32 ratio = flow->unk10 * FX32_CONST(0.3) + FX32_ONE;

        if (ratio > FX32_CONST(4)) {
            ratio = FX32_CONST(4);
        }
        return ratio;
    }
    case ITEM_DUSK_BALL: {
        const BtlFieldSituation *field = GetFieldEffectData(flow->mainModule);
        u8 period = GetDayPeriod(field->env.season, field->env.hour);

        if (field->env.bgType == 4 || field->env.bgType == 5) {
            return FX32_CONST(3.5);
        }
        if (IsDuskBallBackground(field->env.bgType) && (period == 3 || period == 4)) {
            return FX32_CONST(3.5);
        }
        break;
    }
    case ITEM_QUICK_BALL:
        if (flow->unk10 == 0) {
            return FX32_CONST(5);
        }
        break;
    }
    return FX32_ONE;
}

// A critical capture, likelier the more species the player has caught
static BOOL CheckCriticalCapture(BtlServerFlow *flow, fx32 value) {
    u32 count = GetDexCaughtCount(flow->mainModule);
    fx32 ratio;

    if (count > 600) {
        ratio = FX32_CONST(2.5);
    } else if (count > 450) {
        ratio = FX32_CONST(2);
    } else if (count > 300) {
        ratio = FX32_CONST(1.5);
    } else if (count > 150) {
        ratio = FX32_CONST(1);
    } else if (count > 30) {
        ratio = FX32_CONST(0.5);
    } else {
        return FALSE;
    }
    if (value > FX32_CONST(255)) {
        value = FX32_CONST(255);
    }
    value = FX_MUL(value, ratio) / 6;
    if (BattleRandom(256) < (value >> FX32_SHIFT)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL BagItemCureSleep(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemCureCondition(flow, mon, item, value, CONDITION_SLEEP);
}

static BOOL BagItemCurePoison(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemCureCondition(flow, mon, item, value, CONDITION_POISON);
}

static BOOL BagItemCureBurn(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemCureCondition(flow, mon, item, value, CONDITION_BURN);
}

static BOOL BagItemCureFreeze(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemCureCondition(flow, mon, item, value, CONDITION_FREEZE);
}

static BOOL BagItemCureParalysis(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemCureCondition(flow, mon, item, value, CONDITION_PARALYSIS);
}

static BOOL BagItemCureConfusion(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemCureCondition(flow, mon, item, value, CONDITION_CONFUSION);
}

static BOOL BagItemCureInfatuation(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemCureMentalConditions(flow, mon, item, value, 7);
}

// Guard Spec. sets up Mist on the user's side
static BOOL BagItemGuardSpec(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    u8 side;
    BattleCondition cont;
    BattleHandlerMessageParam *message;

    side = GetSideFromMonID(GetMonID(mon));
    cont = SetConditionTurns(5);
    if (SideEffectEventAdd(side, 3, cont)) {
        message = BattleHandler_PushWork(flow, 4, 0x1f);
        BattleHandler_StrSetup(&message->string, 1, 0x88);
        BattleHandler_AddArg(&message->string, side);
        BattleHandler_PopWork(flow, message);
        return TRUE;
    }
    return FALSE;
}

// Revives a fainted mon with the HP the item gives
static BOOL BagItemRevive(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    u8 monId;
    BattleHandlerReviveParam *revive;

    if (IsFainted(mon)) {
        monId = GetMonID(mon);
        revive = BattleHandler_PushWork(flow, 0x2c, monId);
        revive->monIndex = monId;
        switch (ItemGetParam(item, 0x3a)) {
        case 0xff:
            revive->amount = GetBattleMonStat(mon, 0xe);
            break;
        case 0xfe:
            revive->amount = DivideMaxHPZeroCheck(mon, 2);
            break;
        case 0xfd:
            revive->amount = DivideMaxHPZeroCheck(mon, 4);
            break;
        default:
            revive->amount = ItemGetParam(item, 0x3a);
            break;
        }
        BattleHandler_StrSetup(&revive->string, 2, 3);
        BattleHandler_AddArg(&revive->string, monId);
        BattleHandler_PopWork(flow, revive);
        BtlServerFlow_NotifyRevive(flow, monId);
        return TRUE;
    }
    return FALSE;
}

static BOOL BagItemRaiseAttack(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemRaiseStat(flow, mon, item, value, 1);
}

static BOOL BagItemRaiseDefense(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemRaiseStat(flow, mon, item, value, 2);
}

static BOOL BagItemRaiseSpAttack(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemRaiseStat(flow, mon, item, value, 3);
}

static BOOL BagItemRaiseSpDefense(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemRaiseStat(flow, mon, item, value, 4);
}

static BOOL BagItemRaiseSpeed(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemRaiseStat(flow, mon, item, value, 5);
}

static BOOL BagItemRaiseAccuracy(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    return BagItemRaiseStat(flow, mon, item, value, 6);
}

// Dire Hit raises the critical hit stage
static BOOL BagItemDireHit(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    BOOL used = FALSE;

    if (!GetAdditionalConditionFlag(mon, 9)) {
        BattleMon_SetConditionFlag(mon, 9);
        BtlServerCmdQueue_Put(flow->queue, SC_OP_SET_CONDITION_FLAG, GetMonID(mon), 9);
        used = TRUE;
    }
    if (value > 1 && BattleMon_AddCritStage(mon, value - 1)) {
        BtlServerCmdQueue_Put(flow->queue, SC_OP_ADD_CRIT_STAGE, GetMonID(mon), value - 1);
        used = TRUE;
    }
    if (used) {
        BtlServerCmdQueue_PutMsg(flow->queue, SC_MSG_SET, 0x411, GetMonID(mon), SC_MSG_ARG_END);
        return TRUE;
    }
    return FALSE;
}

// An Ether restores the PP of one move
static BOOL BagItemRestorePP(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 slot) {
    u8 monId;
    u8 amount;
    u8 maxAmount;
    BattleHandlerPPParam *pp;

    if (BattleMon_GetOrgMoveCount(mon) > slot) {
        monId = GetMonID(mon);
        amount = BattleMon_GetOrgMovePPUsed(mon, slot);
        maxAmount = ItemGetParam(item, 0x3b);
        if (maxAmount != 0x7f && amount > maxAmount) {
            amount = maxAmount;
        }
        if (amount != 0) {
            pp = BattleHandler_PushWork(flow, 9, monId);
            pp->amount = amount;
            pp->monIndex = monId;
            pp->moveIndex = slot;
            pp->allowFainted = 1;
            BattleHandler_StrSetup(&pp->string, 2, 0x186);
            BattleHandler_AddArg(&pp->string, monId);
            BattleHandler_AddArg(&pp->string, BattleMon_GetOrgMoveID(mon, slot));
            BattleHandler_PopWork(flow, pp);
            return TRUE;
        }
    }
    return FALSE;
}

// An Elixir restores the PP of every move
static BOOL BagItemRestoreAllPP(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    u8 monId;
    u32 numMoves;
    BOOL used;
    u8 maxAmount;
    u8 amount;
    u32 i;
    BattleHandlerPPParam *pp;
    BattleHandlerMessageParam *message;

    monId = GetMonID(mon);
    maxAmount = ItemGetParam(item, 0x3b);
    used = FALSE;
    numMoves = BattleMon_GetOrgMoveCount(mon);
    for (i = 0; i < numMoves; i++) {
        amount = BattleMon_GetOrgMovePPUsed(mon, i);
        if (amount > maxAmount) {
            amount = maxAmount;
        }
        if (amount != 0) {
            pp = BattleHandler_PushWork(flow, 9, monId);
            pp->moveIndex = i;
            pp->monIndex = monId;
            pp->amount = amount;
            pp->allowFainted = 1;
            BattleHandler_PopWork(flow, pp);
            used = TRUE;
        }
    }
    if (used) {
        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 2, 0x189);
        BattleHandler_AddArg(&message->string, monId);
        BattleHandler_PopWork(flow, message);
    }
    return used;
}

// A Potion restores HP
static BOOL BagItemRestoreHP(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    u8 monId;
    BattleHandlerRecoverHPParam *recover;
    u32 amount;

    if (!IsMonFullHP(mon) && !IsFainted(mon)) {
        monId = GetMonID(mon);
        recover = BattleHandler_PushWork(flow, 5, monId);
        recover->targetIndex = monId;
        recover->skipCheck = TRUE;
        amount = ItemGetParam(item, 0x3a);
        switch (amount) {
        case 0xff:
            recover->amount = GetBattleMonStat(mon, 0xe);
            break;
        case 0xfe:
            recover->amount = DivideMaxHPZeroCheck(mon, 2);
            break;
        case 0xfd:
            recover->amount = DivideMaxHPZeroCheck(mon, 4);
            break;
        default:
            recover->amount = amount;
            break;
        }
        BattleHandler_StrSetup(&recover->string, 2, 0x183);
        BattleHandler_AddArg(&recover->string, monId);
        BattleHandler_PopWork(flow, recover);
        return TRUE;
    }
    return FALSE;
}

// Cures a condition of a mon
static BOOL BagItemCureCondition(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u32 condition) {
    u8 monId;
    BattleHandlerCureConditionParam *param;

    if (!IsFainted(mon) && CheckCondition(mon, condition)) {
        monId = GetMonID(mon);
        param = BattleHandler_PushWork(flow, 0xb, monId);
        param->count = 1;
        param->monIds[0] = monId;
        param->condition = condition;
        BattleHandler_PopWork(flow, param);
        return TRUE;
    }
    return FALSE;
}

// Cures every major condition of a mon that isn't in battle
static BOOL BagItemCureMentalConditions(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    BOOL cured;
    u32 i;
    u32 condition;
    u8 monId;
    BattleHandlerCureConditionParam *work;

    if (!IsFainted(mon) && !DoesBattleMonExist(&flow->posPoke, GetMonID(mon))) {
        cured = FALSE;
        i = 0;
        while (TRUE) {
            condition = GetMentalHerbCondition(i++);
            if (condition == CONDITION_NONE) {
                break;
            }
            if (CheckCondition(mon, condition)) {
                monId = GetMonID(mon);
                work = BattleHandler_PushWork(flow, 0xb, monId);
                work->count = 1;
                work->monIds[0] = monId;
                work->condition = condition;
                BattleHandler_PopWork(flow, work);
                cured = TRUE;
            }
        }
        return cured;
    }
    return FALSE;
}

// Raises a stat of a mon in battle
static BOOL BagItemRaiseStat(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u32 stat) {
    u8 monId;
    BattleHandlerStatChangeParam *param;

    monId = GetMonID(mon);
    if (GetBattlePos(&flow->posPoke, monId) != 6 && !IsFainted(mon) && IsStatChangeValid(mon, stat, value)) {
        param = BattleHandler_PushWork(flow, 0xe, monId);
        param->count = 1;
        param->monIds[0] = monId;
        param->stat = stat;
        param->change = value;
        param->unk0e = 1;
        BattleHandler_PopWork(flow, param);
        return TRUE;
    }
    return FALSE;
}

// Uses a mon's held item: 1 when it was used, 2 when it only set the flag, 0 when it can't be used
static BOOL BagItemItemUrge(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    u8 flag;
    u8 monId;
    u8 result;

    if (!IsReactiveHeldItem(GetBattleMonHeldItem(mon))) {
        monId = GetMonID(mon);
        result = BtlServerFlow_UseHeldItem(flow, mon, &flag);
        if (!result) {
            result = flag ? 2 : 0;
            return result;
        }
        return 1;
    }
    return 0;
}

static BOOL BagItemAbilityUrge(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    u32 state;
    u32 result;

    if (GetBattleMonStat(mon, 0x10) != ABILITY_IMPOSTER) {
        state = PushState(&flow->actionState, 0x797);
        BtlServerFlow_TriggerAbility(flow, mon);
        result = BtlServerFlow_GetHandlerResult(flow);
        PopState(&flow->actionState, state, 0x79c);
        if (result == 2) {
            return TRUE;
        }
    }
    return FALSE;
}

// Takes away a mon's held item
static BOOL BagItemItemDrop(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    u16 heldItem;
    u32 state;
    u8 monId;
    BattleHandlerSetItemParam *setItem;

    if (!IsItemUnremovable(flow, GetMonID(mon)) && (heldItem = GetBattleMonHeldItem(mon)) != 0) {
        state = PushState(&flow->actionState, 0x7ae);
        monId = GetMonID(mon);
        setItem = BattleHandler_PushWork(flow, 0x20, monId);
        setItem->targetIndex = monId;
        setItem->item = 0;
        BattleHandler_StrSetup(&setItem->string, 2, 0xe1);
        BattleHandler_AddArg(&setItem->string, monId);
        BattleHandler_AddArg(&setItem->string, heldItem);
        BattleHandler_PopWork(flow, setItem);
        PopState(&flow->actionState, state, 0x7bb);
        return TRUE;
    }
    return FALSE;
}

// Resets a mon's stat stages
static BOOL BagItemResetUrge(BtlServerFlow *flow, BattleMon *mon, u16 item, s32 value, u8 param) {
    u32 state;
    u8 monId;
    BattleHandlerResetStatStageParam *reset;
    BattleHandlerMessageParam *message;

    state = PushState(&flow->actionState, 0x7c6);
    monId = GetMonID(mon);
    reset = BattleHandler_PushWork(flow, 0x10, monId);
    reset->count = 1;
    reset->monIndices[0] = monId;
    BattleHandler_PopWork(flow, reset);
    message = BattleHandler_PushWork(flow, 4, monId);
    BattleHandler_StrSetup(&message->string, 2, 0xe4);
    BattleHandler_AddArg(&message->string, monId);
    BattleHandler_PopWork(flow, message);
    PopState(&flow->actionState, state, 0x7d6);
    return TRUE;
}

// Whether a party has a fainted mon of a species
static BOOL HasFaintedMonOfSpecies(BattleParty *party, s16 species) {
    s32 i;
    BattleMon *mon;

    for (i = 0; i < GetNumMonsInParty(party); i++) {
        mon = GetBattleMonFromParty(party, i);
        if (species == GetBattleMonSpecies(mon) && IsFainted(mon)) {
            return TRUE;
        }
    }
    return FALSE;
}

// Whether a party has a fainted mon of another species
static BOOL HasFaintedMonOfOtherSpecies(BattleParty *party, s16 species) {
    s32 i;
    BattleMon *mon;

    for (i = 0; i < GetNumMonsInParty(party); i++) {
        mon = GetBattleMonFromParty(party, i);
        if (species != GetBattleMonSpecies(mon) && IsFainted(mon)) {
            return TRUE;
        }
    }
    return FALSE;
}

// How many of a party's mons have fainted
static s32 CountFaintedMons(BattleParty *party) {
    s32 count = 0;
    s32 i;

    for (i = 0; i < GetNumMonsInParty(party); i++) {
        if (IsFainted(GetBattleMonFromParty(party, i))) {
            count++;
        }
    }
    return count;
}

// Whether all of a party's mons have fainted
static BOOL IsPartyAllFainted(BattleParty *party) {
    if (GetNumMonsInParty(party) == CountFaintedMons(party)) {
        return TRUE;
    }
    return FALSE;
}

static s16 GetRulesTargetSpecies(BtlMainModule *mainModule, const BtlScriptedRules *rules) {
    if (GetStudioMode(mainModule) == 1) {
        return rules->species1;
    }
    return rules->species2;
}

// The last outcome that was set
static s32 GetLastRuleOutcome(const s32 *outcomes) {
    s32 i;
    s32 outcome = 0;

    for (i = 0; i < 5; i++) {
        if (outcomes[i] != 0) {
            outcome = outcomes[i];
        }
    }
    return outcome;
}

// -2 when the turn limit is up without a win
static s32 CheckRulesTurnLimit(BtlMainModule *mainModule, BOOL won) {
    BtlServerFlow *flow = GetServerFlow(mainModule);
    const BtlScriptedRules *rules = GetStudioRules(mainModule);

    if (flow->unk10 >= rules->turnLimit && !won) {
        return -2;
    }
    return 0;
}

// Checks a scripted battle's rules at the end of a turn: 1 for a win, the outcome of a broken rule, or 0 to go on
s32 CheckScriptedBattleRules(BtlMainModule *mainModule, BtlPokeCon *pokeCon) {
    u8 unkDF = GetStudioMovieId(mainModule);
    BOOL won = FALSE;
    s32 outcomes[5] = { 0 };
    const BtlScriptedRules *rules;
    BOOL playerDown;
    s32 outcome;

    rules = GetStudioRules(mainModule);
    playerDown = IsPartyAllFainted(GetPartyData(pokeCon, 0));
    switch (rules->rule) {
    case 2: {
        BattleParty *party = GetPartyData(pokeCon, 1);
        s16 species = GetRulesTargetSpecies(mainModule, rules);
        BOOL otherDown = HasFaintedMonOfOtherSpecies(party, species);
        BOOL targetDown = HasFaintedMonOfSpecies(party, species);

        u32 value = func_ov167_0219e300(mainModule);

        if (targetDown) {
            if (value == rules->unk16) {
                won = TRUE;
            } else {
                outcomes[0] = -9;
            }
        } else if (otherDown) {
            outcomes[1] = -6;
        } else if (playerDown) {
            outcomes[2] = -3;
        }
        outcomes[3] = CheckRulesTurnLimit(mainModule, won);
        break;
    }
    case 4: {
        s16 species = GetRulesTargetSpecies(mainModule, rules);
        BattleParty *party = GetPartyData(pokeCon, 1);
        BOOL targetDown = HasFaintedMonOfSpecies(party, species);
        BOOL otherDown = HasFaintedMonOfOtherSpecies(party, species);

        if (targetDown) {
            won = TRUE;
        }
        if (otherDown) {
            outcomes[0] = -6;
        }
        if (playerDown) {
            outcomes[1] = -3;
        }
        outcomes[2] = CheckRulesTurnLimit(mainModule, won);
        break;
    }
    case 0: {
        BattleParty *party = GetPartyData(pokeCon, 1);
        s32 turn = GetServerTurnCount(mainModule);
        s32 numDown = CountFaintedMons(party);
        BOOL allDown = IsPartyAllFainted(party);

        if (turn > numDown || playerDown) {
            outcomes[0] = -8;
        }
        if (allDown) {
            if (numDown >= rules->turnLimit) {
                won = TRUE;
            } else {
                outcomes[1] = -8;
            }
        }
        if (playerDown) {
            outcomes[2] = -3;
        }
        outcomes[3] = CheckRulesTurnLimit(mainModule, won);
        break;
    }
    case 1: {
        BOOL allDown = IsPartyAllFainted(GetPartyData(pokeCon, 1));

        if (playerDown) {
            outcomes[0] = -3;
        }
        if (allDown) {
            won = TRUE;
        }
        outcomes[1] = CheckRulesTurnLimit(mainModule, won);
        break;
    }
    case 3: {
        s16 species = GetRulesTargetSpecies(mainModule, rules);

        if (HasFaintedMonOfSpecies(GetPartyData(pokeCon, 1), species)) {
            won = TRUE;
        }
        if (playerDown) {
            outcomes[0] = -3;
        }
        outcomes[1] = CheckRulesTurnLimit(mainModule, won);
        break;
    }
    case 5: {
        s16 species = GetRulesTargetSpecies(mainModule, rules);
        BattleParty *party = GetPartyData(pokeCon, 1);
        BOOL targetDown = HasFaintedMonOfSpecies(party, species);
        s32 numDown = CountFaintedMons(party);
        u8 numMons = GetNumMonsInParty(party);

        if (!targetDown && numDown == numMons - 1) {
            won = TRUE;
        }
        if (targetDown == TRUE) {
            outcomes[0] = -7;
        }
        if (playerDown) {
            outcomes[1] = -3;
        }
        outcomes[2] = CheckRulesTurnLimit(mainModule, won);
        break;
    }
    case 6: {
        BOOL allDown = IsPartyAllFainted(GetPartyData(pokeCon, 1));

        if (playerDown) {
            outcomes[0] = -3;
        }
        if (allDown) {
            outcomes[1] = -5;
        }
        won = CheckRulesTurnLimit(mainModule, FALSE);
        break;
    }
    case 7: {
        s32 numDown = CountFaintedMons(GetPartyData(pokeCon, 1));

        if (playerDown) {
            outcomes[0] = -3;
        }
        if (numDown) {
            outcomes[1] = -4;
        }
        won = CheckRulesTurnLimit(mainModule, FALSE);
        break;
    }
    case 8: {
        BOOL allDown = IsPartyAllFainted(GetPartyData(pokeCon, 1));

        if (playerDown) {
            won = TRUE;
        }
        if (allDown) {
            outcomes[0] = -5;
        }
        if (CheckRulesTurnLimit(mainModule, won)) {
            outcomes[1] = -2;
        }
        break;
    }
    }
    outcome = GetLastRuleOutcome(outcomes);
    if (won && outcome == 0) {
        return 1;
    }
    if (outcome != 0) {
        return outcome;
    }
    return 0;
}

// The battle's result for the player: BTL_RESULT_DEFEAT, VICTORY or DRAW
u32 CalcBattleResult(BtlServerFlow *flow) {
    u8 alive[2];
    u8 total[2];
    u32 i;
    BattleParty *party;
    u8 side;
    u8 otherSide;
    u8 monId;
    s32 outcome;
    u8 monSide;

    alive[0] = alive[1] = 0;
    total[0] = total[1] = 0;
    for (i = 0; i < 4; i++) {
        if (DoesClientExist(flow->mainModule, i)) {
            party = GetPartyData(flow->pokeCon, i);
            if (GetNumMonsInParty(party)) {
                side = GetClientSide(flow->mainModule, i);
                alive[side] += GetAlivePartyCount(party);
                total[side] += GetNumMonsInParty(party);
            }
        }
    }
    if (GetStudioMode(flow->mainModule) == 0) {
        side = GetClientSide(flow->mainModule, GetPlayerClientID(flow->mainModule));
        otherSide = side ^ 1;
        if (alive[side] == 0 && alive[otherSide] != 0) {
            return BTL_RESULT_DEFEAT;
        }
        if (alive[otherSide] == 0 && alive[side] != 0) {
            return BTL_RESULT_VICTORY;
        }
        if (!func_ov167_0219de6c(flow->mainModule)) {
            return BTL_RESULT_DRAW;
        }
        if (alive[0] == 0) {
            monId = BtlFaintRec_GetLastFainted(&flow->faintRec);
            if (monId != 0x1f) {
                monSide = GetSideFromMonID(monId);
                if (monSide == side) {
                    return BTL_RESULT_VICTORY;
                }
                return BTL_RESULT_DEFEAT;
            }
            return BTL_RESULT_DRAW;
        }
        return JudgeByRemainingMons(flow, alive, total, side);
    }
    outcome = CheckScriptedBattleRules(flow->mainModule, flow->pokeCon);
    flow->unk2130 = -outcome;
    if (outcome > 0) {
        return BTL_RESULT_VICTORY;
    }
    return BTL_RESULT_DEFEAT;
}

// When both sides are still standing: the side with more mons left wins, then the one with more HP left
static u32 JudgeByRemainingMons(BtlServerFlow *flow, const u8 *alive, const u8 *total, u8 side) {
    s32 down0;
    s32 down1;
    s32 hp[2];
    s32 percent[2];
    u8 winner;

    down0 = total[0] - alive[0];
    down1 = total[1] - alive[1];
    if (down0 != down1) {
        winner = down0 >= down1 ? 1 : 0;
        if (winner == side) {
            return BTL_RESULT_VICTORY;
        }
        return BTL_RESULT_DEFEAT;
    }
    GetSideHP(flow, &hp[0], &percent[0], 0);
    GetSideHP(flow, &hp[1], &percent[1], 1);
    if (percent[0] != percent[1]) {
        winner = percent[0] > percent[1] ? 0 : 1;
        if (winner == side) {
            return BTL_RESULT_VICTORY;
        }
        return BTL_RESULT_DEFEAT;
    }
    if (hp[0] != hp[1]) {
        winner = hp[0] > hp[1] ? 0 : 1;
        if (winner == side) {
            return BTL_RESULT_VICTORY;
        }
        return BTL_RESULT_DEFEAT;
    }
    return BTL_RESULT_DRAW;
}

// The HP a side has left, in total and in percent of its max
static void GetSideHP(BtlServerFlow *flow, s32 *hp, s32 *percent, u8 side) {
    u32 maxHP;
    u32 i;
    BattleParty *party;
    u8 numMons;
    u32 j;
    BattleMon *mon;

    maxHP = 0;
    *hp = 0;
    *percent = 0;
    for (i = 0; i < 4; i++) {
        if (DoesClientExist(flow->mainModule, i) && side == GetClientSide(flow->mainModule, i)) {
            party = GetPartyData(flow->pokeCon, i);
            numMons = GetNumMonsInParty(party);
            for (j = 0; j < numMons; j++) {
                mon = GetBattleMonFromParty(party, j);
                maxHP += GetBattleMonStat(mon, 0xe);
                *hp += GetBattleMonStat(mon, 0xd);
            }
        }
    }
    *percent = *hp * 100 / maxHP;
}

void EffectivenessTable_Init(u32 *table) {
    u32 i;

    for (i = 0; i < 24; i++) {
        table[i] = 6;
    }
}

void EffectivenessTable_Set(u32 *table, u8 monId, u32 value) {
    table[monId] = value;
}

u32 EffectivenessTable_Get(u32 *table, u8 monId) {
    return table[monId];
}

u32 EffectivenessTable_GetValue(u32 *table, u8 monId) {
    return table[monId];
}
