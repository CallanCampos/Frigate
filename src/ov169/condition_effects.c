#include "battle/condition_effects.h"
#include "types.h"
#include "battle/btl_calc.h"
#include "battle/btl_event.h"
#include "battle/btl_handler.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_server_flow.h"
#include "constants/battle.h"
#include "constants/moves.h"
#include "constants/types.h"

// The message of a condition that ended: the one by default and the one with a second form, as negative when the
// condition has none
typedef struct ConditionCureMessage {
    u32 condition;
    s16 message;
    s16 messageAlt;
} ConditionCureMessage;

static void ConditionEndTurnActive(BtlServerFlow *flow, BattleMon *mon, u8 monId, s32 condition);
static void ConditionEndTurnPerishSong(BtlServerFlow *flow, BattleMon *mon, u8 monId);
static void ConditionEndTurnLeechSeed(BtlServerFlow *flow, BattleMon *mon, u8 monId);
static void ConditionEndTurnIngrain(BtlServerFlow *flow, BattleMon *mon, u8 monId);
static void ConditionEndTurnBind(BtlServerFlow *flow, BattleMon *mon, u8 monId);
static void ConditionEndTurnAquaRing(BtlServerFlow *flow, BattleMon *mon, u8 monId);
static void ConditionEndTurnEncore(BtlServerFlow *flow, BattleMon *mon, u8 monId);
static void ConditionCured(BtlServerFlow *flow, BattleMon *mon, u8 monId, u32 condition, BattleCondition cont);
static void ConditionCuredYawn(BtlServerFlow *flow, BattleMon *mon);
static void ConditionCuredPerishSong(BtlServerFlow *flow, BattleMon *mon);
static void PerishSongMessage(BtlServerFlow *flow, BattleMon *mon, u8 turns);
static void ConditionCuredEmbargo(BtlServerFlow *flow, BattleMon *mon);
static void ConditionCuredHealBlock(BtlServerFlow *flow, BattleMon *mon);
static s32 GetConditionAddMessage(s32 condition, BattleCondition value);
static s32 GetConditionCureMessage(u32 condition, BOOL alt);
static s32 GetConditionDamageMessage(u32 condition);

void Condition_EndTurnEffect(BattleMon *mon, u32 condition, BattleCondition prev, BOOL cured, BtlServerFlow *flow) {
    u8 monId = GetMonID(mon);

    if (cured) {
        ConditionCured(flow, mon, monId, condition, prev);
    } else {
        ConditionEndTurnActive(flow, mon, monId, condition);
    }
}

static void ConditionEndTurnActive(BtlServerFlow *flow, BattleMon *mon, u8 monId, s32 condition) {
    u32 damage;

    if (IsFainted(mon)) {
        return;
    }
    damage = func_ov167_021bbb24(mon, condition);
    if (damage != 0) {
        func_ov167_021a8524(flow, mon, condition, damage);
    }
    switch (condition) {
    case CONDITION_PERISH_SONG:
        ConditionEndTurnPerishSong(flow, mon, monId);
        break;
    case CONDITION_LEECH_SEED:
        ConditionEndTurnLeechSeed(flow, mon, monId);
        break;
    case CONDITION_INGRAIN:
        ConditionEndTurnIngrain(flow, mon, monId);
        break;
    case CONDITION_BIND:
        ConditionEndTurnBind(flow, mon, monId);
        break;
    case 0x23:
        ConditionEndTurnAquaRing(flow, mon, monId);
        break;
    case 0x17:
        ConditionEndTurnEncore(flow, mon, monId);
        break;
    }
}

BOOL Condition_SetDamageMessage(BattleHandlerString *string, BattleMon *mon, u32 condition) {
    s32 message = GetConditionDamageMessage(condition);

    if (message > 0) {
        BattleHandler_StrSetup(string, 2, message);
        BattleHandler_AddArg(string, GetMonID(mon));
    }
    return FALSE;
}

static s32 GetConditionDamageMessage(u32 condition) {
    switch (condition) {
    case CONDITION_POISON:
        return 0xf3;
    case CONDITION_BURN:
        return 0x105;
    case CONDITION_NIGHTMARE:
        return 0x144;
    case CONDITION_CURSE:
        return 0x42f;
    }
    return -1;
}

// Counts down the turns of Perish Song
static void ConditionEndTurnPerishSong(BtlServerFlow *flow, BattleMon *mon, u8 monId) {
    BattleCondition cont;
    s32 turns;

    if (!IsFainted(mon)) {
        cont = GetConditionContinuationParam(mon, CONDITION_PERISH_SONG);
        turns = func_ov167_021ce33c(cont) - func_ov167_021bbb1c(mon, CONDITION_PERISH_SONG);
        if (turns > 0) {
            PerishSongMessage(flow, mon, turns);
        }
    }
}

static void ConditionEndTurnLeechSeed(BtlServerFlow *flow, BattleMon *mon, u8 monId) {
    BattleHandlerDamageParam *damage;
    BattleHandlerDrainParam *drain;
    BattleCondition cont;
    u8 pos;
    u8 sourceId;
    u16 amount;
    u16 hp;
    u32 message;

    cont = GetConditionContinuationParam(mon, CONDITION_LEECH_SEED);
    pos = Condition_GetParam(cont);
    sourceId = func_ov167_021ab884(flow, pos);
    if (sourceId != 0x1f) {
        amount = DivideMaxHPZeroCheck(mon, 8);
        hp = GetBattleMonStat(mon, 0xd);
        if (amount > hp) {
            amount = hp;
        }
        damage = BattleHandler_PushWork(flow, 7, monId);
        message = 0x262;
        damage->targetIndex = monId;
        damage->amount = amount;
        BattleHandler_StrSetup(&damage->string, 2, message);
        BattleHandler_AddArg(&damage->string, monId);
        damage->showViewEffect = TRUE;
        message += 0x13;
        damage->effect = message;
        damage->effectArg1 = pos;
        damage->effectArg2 = func_ov167_021abb50(flow, monId);
        BattleHandler_PopWork(flow, damage);
        if (sourceId != 0x1f) {
            drain = BattleHandler_PushWork(flow, 6, monId);
            drain->header.checkPrevResult = TRUE;
            drain->monIndex = sourceId;
            drain->sourceIndex = monId;
            drain->amount = amount;
            BattleHandler_PopWork(flow, drain);
        }
    }
}

// Recovers a sixteenth of the HP of a Pokemon with Ingrain
static void ConditionEndTurnIngrain(BtlServerFlow *flow, BattleMon *mon, u8 monId) {
    BattleHandlerDrainParam *drain;

    if (!IsMonFullHP(mon)) {
        drain = BattleHandler_PushWork(flow, 6, monId);
        drain->monIndex = monId;
        drain->sourceIndex = 0x1f;
        drain->amount = DivideMaxHPZeroCheck(mon, 16);
        BattleHandler_StrSetup(&drain->string, 2, 0x2e3);
        BattleHandler_AddArg(&drain->string, monId);
        BattleHandler_PopWork(flow, drain);
    }
}

// Damages a Pokemon that a move like Bind holds
static void ConditionEndTurnBind(BtlServerFlow *flow, BattleMon *mon, u8 monId) {
    BattleHandlerDamageParam *damage;
    BattleCondition cont;
    s32 move;
    s32 effect;

    if (!IsFainted(mon)) {
        cont = GetConditionContinuationParam(mon, CONDITION_BIND);
        move = Condition_GetParam(cont);
        effect = -1;
        switch (move) {
        case MOVE_WRAP:
            effect = 0x277;
            break;
        case MOVE_BIND:
            effect = 0x276;
            break;
        case MOVE_FIRE_SPIN:
            effect = 0x278;
            break;
        case MOVE_MAGMA_STORM:
            effect = 0x279;
            break;
        case MOVE_CLAMP:
            effect = 0x27a;
            break;
        case MOVE_WHIRLPOOL:
            effect = 0x27b;
            break;
        case MOVE_SAND_TOMB:
            effect = 0x27c;
            break;
        }
        damage = BattleHandler_PushWork(flow, 7, monId);
        damage->targetIndex = monId;
        if (func_ov167_021ce464(cont) != 0) {
            damage->amount = DivideMaxHPZeroCheck(mon, 8);
        } else {
            damage->amount = DivideMaxHPZeroCheck(mon, 16);
        }
        if (effect != -1) {
            damage->showViewEffect = TRUE;
            damage->effect = effect;
            damage->effectArg1 = func_ov167_021abb50(flow, monId);
            damage->effectArg2 = 6;
        }
        BattleHandler_StrSetup(&damage->string, 2, 0x174);
        BattleHandler_AddArg(&damage->string, monId);
        BattleHandler_AddArg(&damage->string, move);
        BattleHandler_PopWork(flow, damage);
    }
}

// Recovers a sixteenth of the HP of a Pokemon with Aqua Ring
static void ConditionEndTurnAquaRing(BtlServerFlow *flow, BattleMon *mon, u8 monId) {
    BattleHandlerDrainParam *drain;

    if (!IsMonFullHP(mon) && !IsFainted(mon)) {
        drain = BattleHandler_PushWork(flow, 6, monId);
        drain->monIndex = monId;
        drain->sourceIndex = 0x1f;
        drain->amount = DivideMaxHPZeroCheck(mon, 16);
        BattleHandler_StrSetup(&drain->string, 2, 0x25c);
        BattleHandler_AddArg(&drain->string, monId);
        BattleHandler_PopWork(flow, drain);
    }
}

// Cures the condition of a Pokemon whose move is gone
static void ConditionEndTurnEncore(BtlServerFlow *flow, BattleMon *mon, u8 monId) {
    BattleHandlerCureConditionParam *cure;
    u32 condition = 0x17;

    if (!func_ov167_021bada0(mon, Condition_GetParam(GetConditionContinuationParam(mon, condition)))) {
        cure = BattleHandler_PushWork(flow, 0xb, monId);
        cure->condition = condition;
        cure->monIds[0] = monId;
        cure->count = 1;
        BattleHandler_PopWork(flow, cure);
    }
}

static void ConditionCured(BtlServerFlow *flow, BattleMon *mon, u8 monId, u32 condition, BattleCondition cont) {
    BattleHandlerMessageParam *message;
    BattleHandlerString string;

    if (Condition_SetCureMessage(condition, cont, mon, 0, &string)) {
        message = BattleHandler_PushWork(flow, 4, monId);
        message->string = string;
        BattleHandler_PopWork(flow, message);
    }
    switch (condition) {
    case CONDITION_YAWN:
        ConditionCuredYawn(flow, mon);
        break;
    case CONDITION_PERISH_SONG:
        ConditionCuredPerishSong(flow, mon);
        break;
    case CONDITION_EMBARGO:
        ConditionCuredEmbargo(flow, mon);
        break;
    case CONDITION_HEAL_BLOCK:
        ConditionCuredHealBlock(flow, mon);
        break;
    }
}

// Yawn puts the Pokemon to sleep for a few turns if it has no status condition
static void ConditionCuredYawn(BtlServerFlow *flow, BattleMon *mon) {
    BattleHandlerAddConditionParam *add;
    u8 monId;
    s32 turns;

    turns = func_ov167_021bd3a0(2, 4);
    if (turns > 0) {
        if (GetBattleMonStatus(mon) == 0) {
            monId = GetMonID(mon);
            add = BattleHandler_PushWork(flow, 0xc, monId);
            add->showFail = TRUE;
            add->condition = CONDITION_SLEEP;
            add->value = SetConditionTurns(turns);
            add->targetIndex = monId;
            BattleHandler_PopWork(flow, add);
        }
    }
}

// Faints the Pokemon when the Perish Song count reaches 0
static void ConditionCuredPerishSong(BtlServerFlow *flow, BattleMon *mon) {
    BattleHandlerFaintParam *faint;
    u8 monId;

    PerishSongMessage(flow, mon, 0);
    monId = GetMonID(mon);
    faint = BattleHandler_PushWork(flow, 0x13, monId);
    faint->monIndex = monId;
    BattleHandler_PopWork(flow, faint);
}

static void PerishSongMessage(BtlServerFlow *flow, BattleMon *mon, u8 turns) {
    BattleHandlerMessageParam *message;
    u8 monId;

    monId = GetMonID(mon);
    message = BattleHandler_PushWork(flow, 4, monId);
    BattleHandler_StrSetup(&message->string, 2, 0x35c);
    BattleHandler_AddArg(&message->string, monId);
    BattleHandler_AddArg(&message->string, turns);
    BattleHandler_PopWork(flow, message);
}

// Checks the held item again when Embargo ends
static void ConditionCuredEmbargo(BtlServerFlow *flow, BattleMon *mon) {
    BattleHandlerCheckHeldItemParam *check;

    check = BattleHandler_PushWork(flow, 0x21, GetMonID(mon));
    check->monIndex = GetMonID(mon);
    check->reaction = 0;
    BattleHandler_PopWork(flow, check);
}

// Checks the held item again when Heal Block ends
static void ConditionCuredHealBlock(BtlServerFlow *flow, BattleMon *mon) {
    BattleHandlerCheckHeldItemParam *check;

    check = BattleHandler_PushWork(flow, 0x21, GetMonID(mon));
    check->monIndex = GetMonID(mon);
    check->reaction = 1;
    BattleHandler_PopWork(flow, check);
}

static s32 GetConditionAddMessage(s32 condition, BattleCondition value) {
    s32 message = -1;

    switch (condition) {
    case CONDITION_BURN:
        message = 0xff;
        break;
    case CONDITION_PARALYSIS:
        message = 0x111;
        break;
    case CONDITION_FREEZE:
        message = 0x120;
        break;
    case CONDITION_SLEEP:
        message = 0x132;
        break;
    case CONDITION_CONFUSION:
        message = 0x159;
        break;
    case CONDITION_ATTRACT:
        message = 0x147;
        break;
    case CONDITION_NIGHTMARE:
        message = 0x141;
        break;
    case CONDITION_TAUNT:
        message = 0x238;
        break;
    case CONDITION_TORMENT:
        message = 0x241;
        break;
    case CONDITION_YAWN:
        message = 0x29b;
        break;
    case CONDITION_LEECH_SEED:
        message = 0x25f;
        break;
    case CONDITION_FORESIGHT:
        message = 0x171;
        break;
    case CONDITION_INGRAIN:
        message = 0x2e0;
        break;
    case CONDITION_EMBARGO:
        message = 0x2d7;
        break;
    case CONDITION_DISABLE:
        message = 0x250;
        break;
    case CONDITION_HEAL_BLOCK:
        message = 0x371;
        break;
    case CONDITION_POISON:
        if (Condition_IsBadlyPoisoned(value)) {
            message = 0xed;
        } else {
            message = 0xea;
        }
        break;
    }
    return message;
}

void Condition_SetAddMessage(u32 condition, BattleCondition value, BattleMon *mon, BattleHandlerString *string) {
    s32 message = GetConditionAddMessage(condition, value);

    if (message >= 0) {
        BattleHandler_StrSetup(string, 2, message);
        BattleHandler_AddArg(string, GetMonID(mon));
        if (condition == CONDITION_BIND || condition == CONDITION_DISABLE) {
            BattleHandler_AddArg(string, Condition_GetParam(value));
        }
    } else {
        BattleHandler_StrClear(string);
    }
}

// The messages of conditions that ended
static const ConditionCureMessage sConditionCureMessages[] = {
    { CONDITION_POISON, 0xf6, 0x395 },
    { CONDITION_BURN, 0x108, 0x3a1 },
    { CONDITION_SLEEP, 0x138, 0x39b },
    { CONDITION_FREEZE, 0x126, 0x39e },
    { CONDITION_PARALYSIS, 0x117, 0x398 },
    { 0x17, 0x232, -1 },
    { CONDITION_DISABLE, 0x256, -1 },
    { CONDITION_EMBARGO, 0x2da, -1 },
    { CONDITION_BIND, 0x177, 0x177 },
    { CONDITION_LEECH_SEED, 0x177, 0x177 },
    { 0x20, 0x477, -1 },
    { CONDITION_TAUNT, 0x23e, -1 },
    { 0x1e, 0x295, -1 },
    { CONDITION_HEAL_BLOCK, 0x374, -1 },
    { CONDITION_TORMENT, 0x247, -1 },
    { CONDITION_CONFUSION, 0x15f, 0x3a4 },
    { CONDITION_ATTRACT, 0x153, 0x3a7 },
};

static s32 GetConditionCureMessage(u32 condition, BOOL alt) {
    u32 i;

    for (i = 0; i < NELEMS(sConditionCureMessages); i++) {
        if (condition == sConditionCureMessages[i].condition) {
            s32 message;

            if (alt) {
                message = sConditionCureMessages[i].messageAlt;
            } else {
                message = sConditionCureMessages[i].message;
            }
            if (message < 0) {
                return sConditionCureMessages[i].message;
            }
            return message;
        }
    }
    return -1;
}

BOOL Condition_SetCureMessage(u32 condition, BattleCondition cont, BattleMon *mon, u16 count, BattleHandlerString *string) {
    BOOL hasCount = count != 0;
    s32 message = GetConditionCureMessage(condition, hasCount);

    if (message >= 0) {
        BattleHandler_StrSetup(string, 2, message);
        BattleHandler_AddArg(string, GetMonID(mon));
        if (hasCount) {
            BattleHandler_AddArg(string, count);
        }
        switch (condition) {
        case CONDITION_BIND:
            BattleHandler_AddArg(string, Condition_GetParam(cont));
            break;
        case CONDITION_LEECH_SEED:
            BattleHandler_AddArg(string, 0x49);
            break;
        }
        return TRUE;
    }
    return FALSE;
}

void Condition_CheckTypeImmunity(BtlServerFlow *flow, BattleMon *target) {
    BattleCondition cont;
    u8 pos;

    if (CheckCondition(target, CONDITION_FORESIGHT)) {
        cont = GetConditionContinuationParam(target, CONDITION_FORESIGHT);
        pos = Condition_GetParam(cont);
        if (pos == (u8)BattleEventVar_GetValue(0x15)) {
            if (func_ov167_021ce464(cont) == 0) {
                BattleEventVar_RewriteValue(0x4b, TRUE);
            } else {
                BattleEventVar_RewriteValue(0x4c, TRUE);
            }
        }
    }
    if (CheckCondition(target, CONDITION_INGRAIN) && BattleEventVar_GetValue(0x16) == TYPE_GROUND) {
        BattleEventVar_RewriteValue(0x4b, TRUE);
    }
    if (CheckCondition(target, 0x1f) && BattleEventVar_GetValue(0x16) == TYPE_GROUND) {
        BattleEventVar_RewriteValue(0x4b, TRUE);
    }
}

void Condition_CheckTrapped(BtlServerFlow *flow, BattleMon *mon) {
    if (CheckCondition(mon, CONDITION_MEAN_LOOK) || CheckCondition(mon, CONDITION_BIND)) {
        BattleEventVar_RewriteValue(0x41, TRUE);
    }
}

void Condition_CheckFloating(BtlServerFlow *flow, BattleMon *mon) {
    if (CheckCondition(mon, CONDITION_INGRAIN)) {
        BattleEventVar_RewriteValue(0x41, TRUE);
    }
    if (CheckCondition(mon, 0x1f)) {
        BattleEventVar_RewriteValue(0x41, TRUE);
    }
    if (CheckCondition(mon, 0x20) || CheckCondition(mon, 0x1e)) {
        BattleEventVar_RewriteValue(0x51, TRUE);
    }
}

void Condition_CheckIngrainForceSwitch(BtlServerFlow *flow, BattleMon *mon) {
    BattleHandlerMessageParam *message;
    u8 monId;

    if (CheckCondition(mon, CONDITION_INGRAIN) && BattleEventVar_RewriteValue(0x41, TRUE)) {
        monId = GetMonID(mon);
        message = BattleHandler_PushWork(flow, 4, monId);
        BattleHandler_StrSetup(&message->string, 2, 0x2e6);
        BattleHandler_AddArg(&message->string, monId);
        BattleHandler_PopWork(flow, message);
    }
}

// The conditions that Baton Pass passes on
static const u16 sBatonPassConditions[14] = {
    CONDITION_CONFUSION,
    CONDITION_HEAL_BLOCK,
    CONDITION_GASTRO_ACID,
    CONDITION_LEECH_SEED,
    CONDITION_EMBARGO,
    CONDITION_PERISH_SONG,
    CONDITION_INGRAIN,
    CONDITION_MEAN_LOOK,
    0x1d,
    0x1e,
    CONDITION_MEAN_LOOK,
    CONDITION_CURSE,
    0x20,
    0x23,
};

BOOL Condition_IsPassedByBatonPass(u32 condition) {
    u32 i;

    for (i = 0; i < NELEMS(sBatonPassConditions); i++) {
        if (condition == sBatonPassConditions[i]) {
            return TRUE;
        }
    }
    return FALSE;
}
