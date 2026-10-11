// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "battle/btl_action.h"
#include "battle/btl_calc.h"
#include "battle/btl_field.h"
#include "battle/btl_pokeparam.h"
#include "constants/abilities.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "pml/item.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "pml/waza.h"

BattleMon *BattleMon_Create(PartyPkm *src, u8 monId, HeapID heapId) {
    BattleMon *mon;
    u32 status;

    mon = GFL_HeapAllocate(heapId, sizeof(BattleMon), TRUE, "btl_pokeparam.c", 227);
    mon->core.monId = monId;
    mon->core.species = PokeParty_GetParam(src, PKM_PARAM_SPECIES, NULL);
    mon->core.src = src;
    mon->core.hp = PokeParty_GetParam(src, PKM_PARAM_HP, NULL);
    mon->core.maxHP = PokeParty_GetParam(src, PKM_PARAM_MAX_HP, NULL);
    mon->core.heldItem = PokeParty_GetParam(src, PKM_PARAM_ITEM, NULL);
    if (mon->core.heldItem > ITEM_LAST) {
        mon->core.heldItem = 0;
    }
    mon->core.consumedItem = 0;
    mon->core.transformed = 0;
    mon->core.illusionDisguise = NULL;
    mon->core.illusion = 0;
    mon->core.unk1b_7 = 0;
    mon->core.baseForm = PokeParty_GetParam(src, PKM_PARAM_FORM, NULL);
    mon->core.baseAbility = PokeParty_GetParam(src, PKM_PARAM_ABILITY, NULL);
    mon->core.level = PokeParty_GetParam(src, PKM_PARAM_LEVEL, NULL);
    mon->core.unk1a = PML_PersonalGetParamSingle(mon->core.species, mon->core.baseForm, 1);
    setupBySrcData(mon, src, TRUE, TRUE);
    mon->moveCount = GetNumMoves(mon, src, TRUE);
    mon->unk143 = 0;
    func_ov167_021babc0(&mon->statStages);
    ClearMoveStatusWork(mon, 1);
    status = GetStatusCond(src);
    if (status != 0) {
        mon->core.conditions[status] = func_ov167_021bd52c(status);
        mon->core.conditionCounters[status] = 0;
    }
    mon->unk148 = 10000;
    mon->unk146 = 0;
    mon->substituteHP = 0;
    mon->prevMoveUsed = 0;
    mon->prevMoveId = 0;
    mon->unk144 = 0x11;
    mon->consecutiveMoveCount = 0;
    mon->comboMove = 0;
    mon->comboMonId = 0x1f;
    mon->unk142 = 0;
    sys_memset(mon->turnFlags, 0, sizeof(mon->turnFlags));
    sys_memset(mon->conditionFlags, 0, sizeof(mon->conditionFlags));
    func_ov167_021bbff4(mon);
    func_ov167_021bc5c4(mon);
    return mon;
}

void setupBySrcData(BattleMon *mon, PartyPkm *src, BOOL readHP, BOOL readAbility) {
    if (readHP) {
        mon->core.hp = PokeParty_GetParam(src, PKM_PARAM_HP, NULL);
        mon->core.maxHP = PokeParty_GetParam(src, PKM_PARAM_MAX_HP, NULL);
    }
    mon->core.exp = PokeParty_GetParam(src, PKM_PARAM_EXP, NULL);
    setupBySrcDataBase(mon, src, readAbility);
    if (readAbility) {
        mon->ability = PokeParty_GetParam(src, PKM_PARAM_ABILITY, NULL);
    }
    mon->form = PokeParty_GetParam(src, PKM_PARAM_FORM, NULL);
    mon->weight = PML_PersonalGetParamSingle(mon->core.species, mon->form, 0x26);
    if (mon->weight < 1) {
        mon->weight = 1;
    }
}

u32 GetNumMoves(BattleMon *mon, PartyPkm *src, BOOL reset) {
    u8 wasEncrypted;
    u32 count;
    u32 i;

    wasEncrypted = PokeParty_DecryptPkm(src);
    count = 0;
    if (reset) {
        for (i = 0; i < 4; i++) {
            mon->moves[i].truth.flagsLow = 0;
            mon->moves[i].truth.flagsHigh = 0;
        }
    }
    for (i = 0; i < 4; i++) {
        if (func_ov167_021baa44(&mon->moves[i].truth, src, i)) {
            count++;
        }
    }
    if (reset) {
        for (i = 0; i < 4; i++) {
            mon->moves[i].surface.id = mon->moves[i].truth.id;
            mon->moves[i].surface.ppPair = mon->moves[i].truth.ppPair;
            mon->moves[i].surface.flagsPair = mon->moves[i].truth.flagsPair;
            mon->moves[i].linked = 1;
        }
    }
    PokeParty_EncryptPkm(src, wasEncrypted);
    return count;
}

void SetMovesAndPP(BattleMon *mon) {
    PartyPkm *src = mon->core.src;
    u32 i;

    for (i = 0; i < 4; i++) {
        PokeParty_SetMove(src, mon->moves[i].truth.id, i);
        PokeParty_SetParam(src, PKM_PARAM_MOVE1_PP + i, mon->moves[i].truth.pp);
        PokeParty_SetParam(src, PKM_PARAM_MOVE1_PP_UP + i, mon->moves[i].truth.ppUp);
    }
}

void func_ov167_021ba8b4(BattleMon *mon) {
    PartyPkm *src = mon->core.src;
    u32 i;

    mon->moveCount = 0;
    for (i = 0; i < 4; i++) {
        if (func_ov167_021baa44(&mon->moves[i].truth, src, i)) {
            mon->moveCount++;
        }
        if (mon->moves[i].linked) {
            mon->moves[i].surface.id = mon->moves[i].truth.id;
            mon->moves[i].surface.ppPair = mon->moves[i].truth.ppPair;
            mon->moves[i].surface.flagsPair = mon->moves[i].truth.flagsPair;
        }
    }
}

void MoveWork_ClearSurface(BattleMon *mon) {
    u32 i;

    mon->moveCount = 0;
    for (i = 0; i < 4; i++) {
        mon->moves[i].surface = mon->moves[i].truth;
        if (mon->moves[i].surface.id != 0) {
            mon->moveCount++;
        }
        mon->moves[i].linked = 1;
    }
}

void func_ov167_021ba9cc(BattleMoveWork *move) {
    move->surface.flagsLow = 0;
    move->truth.flagsLow = 0;
}

void MoveWork_UpdateNumber(BattleMoveWork *work, u16 move, u8 maxPP, BOOL updateCurrent) {
    if (updateCurrent) {
        MoveCore_UpdateNumber(&work->truth, move, maxPP);
        if (work->linked != 0) {
            work->surface.id = work->truth.id;
            work->surface.ppPair = work->truth.ppPair;
            work->surface.flagsPair = work->truth.flagsPair;
        }
    } else {
        MoveCore_UpdateNumber(&work->surface, move, maxPP);
        work->linked = 0;
    }
}

void MoveCore_UpdateNumber(BattleMoveCore *core, u16 move, u8 maxPP) {
    u8 pp;

    core->id = move;
    core->flagsLow = 0;
    core->flagsHigh = 0;
    if (move != 0) {
        pp = PML_MoveGetMaxPP(move, 0);
    } else {
        pp = 0;
    }
    core->maxPP = pp;
    if (maxPP != 0 && core->maxPP > maxPP) {
        core->maxPP = maxPP;
    }
    core->pp = core->maxPP;
}

BOOL func_ov167_021baa44(BattleMoveCore *core, PartyPkm *src, u8 index) {
    BOOL result = TRUE;
    u16 move = PokeParty_GetParam(src, PKM_PARAM_MOVE1 + index, NULL);

    if (core->id != move) {
        core->flagsLow = 0;
        core->flagsHigh = 0;
    }
    core->id = move;
    if (core->id != 0) {
        core->pp = PokeParty_GetParam(src, PKM_PARAM_MOVE1_PP + index, NULL);
        core->maxPP = PokeParty_GetParam(src, PKM_PARAM_MOVE1_MAX_PP + index, NULL);
        core->ppUp = PokeParty_GetParam(src, PKM_PARAM_MOVE1_PP_UP + index, NULL);
    } else {
        result = FALSE;
        core->pp = 0;
        core->maxPP = 0;
        core->ppUp = 0;
    }
    return result;
}

void setupBySrcDataBase(BattleMon *mon, PartyPkm *src, BOOL readTypes) {
    if (readTypes) {
        mon->type1 = PokeParty_GetParam(src, PKM_PARAM_TYPE1, NULL);
        mon->type2 = PokeParty_GetParam(src, PKM_PARAM_TYPE2, NULL);
    }
    mon->sex = PokeParty_GetSex(src);
    mon->attack = PokeParty_GetParam(src, PKM_PARAM_ATTACK, NULL);
    mon->defense = PokeParty_GetParam(src, PKM_PARAM_DEFENSE, NULL);
    mon->spAttack = PokeParty_GetParam(src, PKM_PARAM_SP_ATTACK, NULL);
    mon->spDefense = PokeParty_GetParam(src, PKM_PARAM_SP_DEFENSE, NULL);
    mon->speed = PokeParty_GetParam(src, PKM_PARAM_SPEED, NULL);
}

void ClearFormChange(BattleMon *mon) {
    if (mon->core.transformed) {
        setupBySrcData(mon, mon->core.src, 0, 1);
        MoveWork_ClearSurface(mon);
        mon->core.transformed = 0;
    }
}

void ClearUsedMoveFlag(BattleMon *mon) {
    u32 i;

    for (i = 0; i < 4; i++) {
        func_ov167_021ba9cc(&mon->moves[i]);
    }
    mon->prevMoveUsed = 0;
    mon->prevMoveId = 0;
    mon->unk144 = 0x11;
    mon->consecutiveMoveCount = 0;
}

void ClearCounter(BattleMon *mon) {
    u32 i;

    for (i = 0; i < 5; i++) {
        mon->counters[i] = 0;
    }
}

void func_ov167_021babb8(BattleMon *mon) {
    GFL_HeapFree(mon);
}

void func_ov167_021babc0(BattleMonStatStages *stages) {
    ResetStatStages(stages);
}

void ResetStatStages(BattleMonStatStages *stages) {
    stages->attack = 6;
    stages->defense = 6;
    stages->spAttack = 6;
    stages->spDefense = 6;
    stages->speed = 6;
    stages->accuracy = 6;
    stages->evasion = 6;
}

BOOL func_ov167_021babdc(BattleMonStatStages *stages) {
    BOOL result = FALSE;

    if (stages->attack < 6) {
        stages->attack = 6;
        result = TRUE;
    }
    if (stages->defense < 6) {
        stages->defense = 6;
        result = TRUE;
    }
    if (stages->spAttack < 6) {
        stages->spAttack = 6;
        result = TRUE;
    }
    if (stages->spDefense < 6) {
        stages->spDefense = 6;
        result = TRUE;
    }
    if (stages->speed < 6) {
        stages->speed = 6;
        result = TRUE;
    }
    if (stages->accuracy < 6) {
        stages->accuracy = 6;
        result = TRUE;
    }
    if (stages->evasion < 6) {
        stages->evasion = 6;
        result = TRUE;
    }
    return result;
}

u8 GetMonID(BattleMon *mon) {
    return mon->core.monId;
}

u16 GetBattleMonSpecies(const BattleMon *mon) {
    return mon->core.species;
}

u8 GetBattleMonMoveCount(BattleMon *mon) {
    return mon->moveCount;
}

u8 func_ov167_021bac50(BattleMon *mon) {
    u32 count;
    u32 i;

    for (i = 0, count = 0; i < 4; i++) {
        if (mon->moves[i].truth.id == 0) {
            break;
        }
        count++;
    }
    return count;
}

u8 CountUsedMoves(const BattleMon *mon) {
    u8 count;
    u8 i;

    for (i = 0, count = 0; i < mon->moveCount; i++) {
        if (mon->moves[i].surface.flagsLow) {
            count++;
        }
    }
    return count;
}

u8 func_ov167_021bacb4(BattleMon *mon) {
    return mon->unk143;
}

u16 MoveGetID(BattleMon *mon, u8 index) {
    return mon->moves[index].surface.id;
}

u16 func_ov167_021bacd0(BattleMon *mon, u8 index) {
    return mon->moves[index].truth.id;
}

u8 CheckIfMoveWasUsed(BattleMon *mon, u8 index) {
    return mon->moves[index].surface.flagsLow;
}

void func_ov167_021bacf4(BattleMon *mon, BattleMon *dest) {
    u32 i;

    for (i = 0; i < 4; i++) {
        dest->moves[i] = mon->moves[i];
    }
    dest->moveCount = mon->moveCount;
}

u16 func_ov167_021bad28(BattleMon *mon, u8 index, u8 *pp, u8 *maxPP) {
    *pp = mon->moves[index].surface.pp;
    *maxPP = mon->moves[index].surface.maxPP;
    return mon->moves[index].surface.id;
}

u8 GetMovePPUsed(BattleMon *mon, u8 index) {
    return mon->moves[index].surface.maxPP - mon->moves[index].surface.pp;
}

u8 func_ov167_021bad68(BattleMon *mon, u8 index) {
    return mon->moves[index].truth.maxPP - mon->moves[index].truth.pp;
}

u16 GetMovePP(BattleMon *mon, u8 index) {
    if (index < mon->moveCount) {
        return mon->moves[index].surface.pp;
    }
    return 0;
}

u8 func_ov167_021bada0(const BattleMon *mon, u16 move) {
    u32 i;

    for (i = 0; i < mon->moveCount; i++) {
        if (move == mon->moves[i].surface.id) {
            return mon->moves[i].surface.pp;
        }
    }
    return 0;
}

BOOL Move_IsPPFull(BattleMon *mon, u8 index, BOOL truth) {
    BattleMoveCore *move;

    if (truth != 0) {
        move = &mon->moves[index].truth;
    } else {
        move = &mon->moves[index].surface;
    }
    if (move->id != 0 && move->pp == move->maxPP) {
        return TRUE;
    }
    return FALSE;
}

void func_ov167_021bae08(BattleMon *mon, u8 index, u8 amount) {
    if (mon->moves[index].surface.pp >= amount) {
        mon->moves[index].surface.pp -= amount;
    } else {
        mon->moves[index].surface.pp = 0;
    }
    if (mon->moves[index].linked) {
        mon->moves[index].truth.pp = mon->moves[index].surface.pp;
        PokeParty_SetParam(mon->core.src, PKM_PARAM_MOVE1_PP + index, mon->moves[index].truth.pp);
    }
}

void func_ov167_021bae40(BattleMon *mon, u8 index, u8 amount) {
    s32 pp;

    if (mon->moves[index].linked == 0) {
        pp = mon->moves[index].truth.pp - amount;
        if (pp < 0) {
            pp = 0;
        }
        mon->moves[index].truth.pp = pp;
        PokeParty_SetParam(mon->core.src, PKM_PARAM_MOVE1_PP + index, mon->moves[index].truth.pp);
    }
}

u16 Move_IncrementPP(BattleMon *mon, u8 index, u8 amount) {
    BattleMoveWork *move;

    move = &mon->moves[index];
    move->surface.pp += amount;
    if (move->surface.pp > move->surface.maxPP) {
        move->surface.pp = move->surface.maxPP;
    }
    if (move->linked != 0) {
        move->truth.pp = move->surface.pp;
    }
    return move->surface.id;
}

u16 Move_IncrementPP_Org(BattleMon *mon, u8 index, u8 amount) {
    BattleMoveWork *move;

    move = &mon->moves[index];
    move->truth.pp += amount;
    if (move->truth.pp > move->truth.maxPP) {
        move->truth.pp = move->truth.maxPP;
    }
    if (move->linked != 0) {
        move->surface.pp = move->truth.pp;
    }
    return move->truth.id;
}

void func_ov167_021baecc(BattleMon *mon, u8 index) {
    BattleMoveWork *move = &mon->moves[index];

    if (move->surface.flagsHigh == 0) {
        move->surface.flagsHigh = 1;
        mon->unk143++;
    }
    move->surface.flagsLow = 1;
    if (move->linked) {
        move->truth.flagsLow = move->surface.flagsLow;
        move->truth.flagsHigh = move->surface.flagsHigh;
    }
}

void Move_UpdateID(BattleMon *mon, u8 index, u16 move, u8 maxPP, BOOL updateCurrent) {
    MoveWork_UpdateNumber(&mon->moves[index], move, maxPP, updateCurrent);
}

BOOL MoveIsUsable(const BattleMon *mon, u16 move) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (mon->moves[i].surface.id == move) {
            return TRUE;
        }
    }
    return FALSE;
}

u8 func_ov167_021baf78(BattleMon *mon, u16 move) {
    u32 i;

    if (move != 0) {
        for (i = 0; i < 4; i++) {
            if (move == mon->moves[i].surface.id) {
                return i;
            }
        }
    }
    return 4;
}

void splitTypeCore(BattleMon *mon, u8 *type1, u8 *type2) {
    BOOL condition;
    u8 type;

    condition = CheckCondition(mon, 0x18);
    type = mon->type1;
    if (type == 2 && condition) {
        *type1 = 0x11;
    } else {
        *type1 = type;
    }
    type = mon->type2;
    if (type == 2 && condition) {
        *type2 = 0x11;
    } else {
        *type2 = type;
    }
    if (*type1 == 0x11) {
        if (*type2 == 0x11) {
            *type2 = 0;
        }
        *type1 = *type2;
    } else if (*type2 == 0x11) {
        *type2 = *type1;
    }
}

PokeTypePair GetPokeType(BattleMon *mon) {
    u8 type1;
    u8 type2;

    splitTypeCore(mon, &type1, &type2);
    return PokeTypePair_Make(type1, type2);
}

BOOL DoesMonHaveType(BattleMon *mon, u32 type) {
    u8 type1;
    u8 type2;

    if (type != 0x11) {
        splitTypeCore(mon, &type1, &type2);
        if (type1 == type || type2 == type) {
            return TRUE;
        }
    }
    return FALSE;
}

PartyPkm *GetSrcData(const BattleMon *mon) {
    return mon->core.src;
}

void SetIllusionDisguise(BattleMon *mon, PartyPkm *disguise) {
    mon->core.illusionDisguise = disguise;
    mon->core.illusion = 1;
}

void func_ov167_021bb054(BattleMon *mon) {
    mon->core.illusionDisguise = NULL;
    mon->core.illusion = 0;
}

PartyPkm *func_ov167_021bb064(const BattleMon *mon) {
    PartyPkm *disguise;

    disguise = mon->core.illusionDisguise;
    if (disguise != NULL && mon->core.illusion) {
        return disguise;
    }
    return mon->core.src;
}

u32 func_ov167_021bb07c(const BattleMon *mon, u32 stat) {
    switch (stat) {
    case 9:
        if (IsFieldEffectActive(6)) {
            stat = 11;
        }
        break;
    case 11:
        if (IsFieldEffectActive(6)) {
            stat = 9;
        }
        break;
    }
    return stat;
}

s32 RawBattleMonStat(const BattleMon *mon, u32 stat) {
    stat = func_ov167_021bb07c(mon, stat);
    switch (stat) {
    case 8:
        return mon->attack;
    case 9:
        return mon->defense;
    case 10:
        return mon->spAttack;
    case 11:
        return mon->spDefense;
    case 12:
        return mon->speed;
    case 6:
        return 6;
    case 7:
        return 6;
    default:
        return GetBattleMonStat(mon, stat);
    }
}

void func_ov167_021bb10c(BattleMon *mon, BattleMonLevelUp *stats) {
    u8 wasEncrypted;

    wasEncrypted = PokeParty_DecryptPkm(mon->core.src);
    stats->hp = PokeParty_GetParam(mon->core.src, PKM_PARAM_MAX_HP, NULL);
    stats->attack = PokeParty_GetParam(mon->core.src, PKM_PARAM_ATTACK, NULL);
    stats->defense = PokeParty_GetParam(mon->core.src, PKM_PARAM_DEFENSE, NULL);
    stats->spAttack = PokeParty_GetParam(mon->core.src, PKM_PARAM_SP_ATTACK, NULL);
    stats->spDefense = PokeParty_GetParam(mon->core.src, PKM_PARAM_SP_DEFENSE, NULL);
    stats->speed = PokeParty_GetParam(mon->core.src, PKM_PARAM_SPEED, NULL);
    PokeParty_EncryptPkm(mon->core.src, wasEncrypted);
}

void SetBaseStatus(BattleMon *mon, u32 stat, u16 value) {
    switch (func_ov167_021bb07c(mon, stat)) {
    case 8:
        mon->attack = value;
        break;
    case 9:
        mon->defense = value;
        break;
    case 10:
        mon->spAttack = value;
        break;
    case 11:
        mon->spDefense = value;
        break;
    case 12:
        mon->speed = value;
        break;
    }
}

u32 GetBattleMonStat(const BattleMon *mon, u32 stat) {
    stat = func_ov167_021bb07c(mon, stat);
    switch (stat) {
    case 8:
        return GetBoostFromStatStage(RawBattleMonStat(mon, stat), mon->statStages.attack);
    case 9:
        return GetBoostFromStatStage(RawBattleMonStat(mon, stat), mon->statStages.defense);
    case 10:
        return GetBoostFromStatStage(RawBattleMonStat(mon, stat), mon->statStages.spAttack);
    case 11:
        return GetBoostFromStatStage(RawBattleMonStat(mon, stat), mon->statStages.spDefense);
    case 12:
        return GetBoostFromStatStage(RawBattleMonStat(mon, stat), mon->statStages.speed);
    case 1:
        return mon->statStages.attack;
    case 2:
        return mon->statStages.defense;
    case 3:
        return mon->statStages.spAttack;
    case 4:
        return mon->statStages.spDefense;
    case 5:
        return mon->statStages.speed;
    case 6:
        return mon->statStages.accuracy;
    case 7:
        return mon->statStages.evasion;
    case 15:
        return mon->core.level;
    case 18:
        return mon->sex;
    case 13:
        return mon->core.hp;
    case 14:
        return mon->core.maxHP;
    case 21:
        return mon->core.unk1a;
    case 17:
        if (CheckCondition(mon, 0x10)) {
            return 0;
        }
    case 16:
        return mon->ability;
    case 19:
        return mon->form;
    case 20:
        return mon->core.exp;
    default:
        return 0;
    }
}

u32 CritAtkDefLevel(BattleMon *mon, u32 stat) {
    BOOL useRaw;

    useRaw = FALSE;
    switch (func_ov167_021bb07c(mon, stat)) {
    case 8:
        if (mon->statStages.attack < 6) {
            useRaw = TRUE;
        }
        break;
    case 10:
        if (mon->statStages.spAttack < 6) {
            useRaw = TRUE;
        }
        break;
    case 9:
        if (mon->statStages.defense > 6) {
            useRaw = TRUE;
        }
        break;
    case 11:
        if (mon->statStages.spDefense > 6) {
            useRaw = TRUE;
        }
        break;
    }
    if (useRaw) {
        return RawBattleMonStat(mon, stat);
    }
    return GetBattleMonStat(mon, stat);
}

u32 GetBattleMonHeldItem(BattleMon *mon) {
    return mon->core.heldItem;
}

void SetItem(BattleMon *mon, u16 item) {
    mon->core.heldItem = item;
}

BOOL IsMonFullHP(BattleMon *mon) {
    if (GetBattleMonStat(mon, 13) == GetBattleMonStat(mon, 14)) {
        return TRUE;
    }
    return FALSE;
}

BOOL IsFainted(const BattleMon *mon) {
    if (GetBattleMonStat(mon, 13) == 0) {
        return TRUE;
    }
    return FALSE;
}

BOOL CanPokemonBattle(BattleMon *mon) {
    BOOL result = FALSE;

    if (PokeParty_GetParam(mon->core.src, PKM_PARAM_IS_EGG, NULL)) {
        return result;
    }
    if (!IsFainted(mon)) {
        result = TRUE;
    }
    return result;
}

u16 func_ov167_021bb3a4(BattleMon *mon) {
    return mon->unk146;
}

u32 GetTurnFlag(BattleMon *mon, u32 flag) {
    u32 bit;
    u8 mask;
    u32 result;

    bit = flag & 7;
    mask = (u8)(1 << bit);
    flag = (flag << 21) >> 24;
    result = TRUE;
    if ((mon->turnFlags[flag] & mask) == 0) {
        result = FALSE;
    }
    return result;
}

u32 GetAdditionalConditionFlag(BattleMon *mon, u32 flag) {
    u32 bit;
    u8 mask;
    u32 result;

    bit = flag & 7;
    mask = (u8)(1 << bit);
    flag = (flag << 21) >> 24;
    result = TRUE;
    if ((mon->conditionFlags[flag] & mask) == 0) {
        result = FALSE;
    }
    return result;
}

u32 func_ov167_021bb408(BattleMon *mon) {
    u32 i;

    for (i = 0; i < 4; i++) {
        if (GetAdditionalConditionFlag(mon, data_ov167_021d7490[i])) {
            return data_ov167_021d7490[i];
        }
    }
    return 0x10;
}

BOOL IsSemiInvulnMove(BattleMon *mon) {
    if (func_ov167_021bb408(mon) != 0x10) {
        return TRUE;
    }
    return FALSE;
}

fx32 GetHPRatio(BattleMon *mon) {
    double ratio;
    double fixed;

    ratio = (double)(mon->core.hp * 100) / (double)mon->core.maxHP;
    if (ratio > 0.0) {
        fixed = ratio * 4096.0 + 0.5;
    } else {
        fixed = ratio * 4096.0 - 0.5;
    }
    return (fx32)fixed;
}

s8 *func_ov167_021bb4b4(BattleMon *mon, u32 stat, s8 *min, s8 *max) {
    *min = 0;
    *max = 12;
    switch (stat) {
    case 1:
        return &mon->statStages.attack;
    case 2:
        return &mon->statStages.defense;
    case 3:
        return &mon->statStages.spAttack;
    case 4:
        return &mon->statStages.spDefense;
    case 5:
        return &mon->statStages.speed;
    case 6:
        return &mon->statStages.accuracy;
    case 7:
        return &mon->statStages.evasion;
    default:
        return NULL;
    }
}

BOOL IsStatChangeValid(BattleMon *mon, u32 stat, s32 change) {
    s8 min;
    s8 max;
    s8 *stage;

    stage = func_ov167_021bb4b4(mon, stat, &min, &max);
    if (change > 0) {
        return *stage < max;
    }
    return *stage > min;
}

s32 func_ov167_021bb550(BattleMon *mon, u32 stat) {
    s8 min;
    s8 max;
    s8 *stage;

    stage = func_ov167_021bb4b4(mon, stat, &min, &max);
    return max - *stage;
}

BOOL AreStatsLowered(BattleMon *mon) {
    if (mon->statStages.attack < 6) {
        return TRUE;
    }
    if (mon->statStages.defense < 6) {
        return TRUE;
    }
    if (mon->statStages.spAttack < 6) {
        return TRUE;
    }
    if (mon->statStages.spDefense < 6) {
        return TRUE;
    }
    if (mon->statStages.speed < 6) {
        return TRUE;
    }
    if (mon->statStages.accuracy < 6) {
        return TRUE;
    }
    if (mon->statStages.evasion < 6) {
        return TRUE;
    }
    return FALSE;
}

u32 func_ov167_021bb5c0(BattleMon *mon, u32 stat, u8 amount) {
    s8 *stage;

    switch (stat) {
    case 1:
        stage = &mon->statStages.attack;
        break;
    case 2:
        stage = &mon->statStages.defense;
        break;
    case 3:
        stage = &mon->statStages.spAttack;
        break;
    case 4:
        stage = &mon->statStages.spDefense;
        break;
    case 5:
        stage = &mon->statStages.speed;
        break;
    case 6:
        stage = &mon->statStages.accuracy;
        break;
    case 7:
        stage = &mon->statStages.evasion;
        break;
    default:
        return 0;
    }
    if (*stage < 12) {
        if (*stage + amount > 12) {
            amount = 12 - *stage;
        }
        *stage += (s8)amount;
        return amount;
    }
    return 0;
}

u32 func_ov167_021bb638(BattleMon *mon, u32 stat, u8 amount) {
    s8 *stage;

    switch (stat) {
    case 1:
        stage = &mon->statStages.attack;
        break;
    case 2:
        stage = &mon->statStages.defense;
        break;
    case 3:
        stage = &mon->statStages.spAttack;
        break;
    case 4:
        stage = &mon->statStages.spDefense;
        break;
    case 5:
        stage = &mon->statStages.speed;
        break;
    case 6:
        stage = &mon->statStages.accuracy;
        break;
    case 7:
        stage = &mon->statStages.evasion;
        break;
    default:
        return 0;
    }
    if (*stage > 0) {
        if (*stage - amount < 0) {
            amount = *stage;
        }
        *stage -= (s8)amount;
        return amount;
    }
    return 0;
}

void func_ov167_021bb6a8(BattleMon *mon, u32 stat, u8 value) {
    s8 *stage;

    switch (stat) {
    case 1:
        stage = &mon->statStages.attack;
        break;
    case 2:
        stage = &mon->statStages.defense;
        break;
    case 3:
        stage = &mon->statStages.spAttack;
        break;
    case 4:
        stage = &mon->statStages.spDefense;
        break;
    case 5:
        stage = &mon->statStages.speed;
        break;
    case 6:
        stage = &mon->statStages.accuracy;
        break;
    case 7:
        stage = &mon->statStages.evasion;
        break;
    default:
        return;
    }
    if (value <= 12) {
        *stage = value;
    }
}

BOOL StatStageRecover(BattleMon *mon) {
    return func_ov167_021babdc(&mon->statStages);
}

void StatStageReset(BattleMon *mon) {
    ResetStatStages(&mon->statStages);
}

u8 func_ov167_021bb714(BattleMon *mon) {
    u8 stage = mon->unk142;

    if (GetAdditionalConditionFlag(mon, 9)) {
        stage += 2;
        if (stage > 4) {
            stage = 4;
        }
    }
    return stage;
}

BOOL func_ov167_021bb738(BattleMon *mon, s32 amount) {
    s32 decrease;

    if (amount > 0) {
        if (mon->unk142 < 4) {
            mon->unk142 += amount;
            if (mon->unk142 > 4) {
                mon->unk142 = 4;
            }
            return TRUE;
        }
    } else {
        decrease = amount;
        decrease *= -1;
        if (mon->unk142 != 0) {
            if (mon->unk142 > decrease) {
                mon->unk142 -= decrease;
            } else {
                mon->unk142 = 0;
            }
            return TRUE;
        }
    }
    return FALSE;
}

void func_ov167_021bb790(BattleMon *mon, u16 amount) {
    if (mon->core.hp > amount) {
        mon->core.hp -= amount;
    } else {
        mon->core.hp = 0;
    }
}

void HPAdd(BattleMon *mon, u16 amount) {
    mon->core.hp += amount;
    if (mon->core.hp > mon->core.maxHP) {
        mon->core.hp = mon->core.maxHP;
    }
}

void HPZero(BattleMon *mon) {
    mon->core.hp = 0;
}

void func_ov167_021bb7c0(BattleMon *mon, u32 flag) {
    u8 *flags = mon->turnFlags;

    flags[(u8)(flag >> 3)] |= (u8)(1 << (flag & 7));
}

void func_ov167_021bb7e4(BattleMon *mon, u32 flag) {
    u8 *flags = mon->conditionFlags;

    flags[(u8)(flag >> 3)] |= (u8)(1 << (flag & 7));
}

void func_ov167_021bb808(BattleMon *mon, u32 flag) {
    u8 *flags = mon->conditionFlags;

    flags[(u8)(flag >> 3)] &= (u8) ~(u8)(1 << (flag & 7));
}

void SetMoveCondition(BattleMon *mon, u32 condition, BattleCondition value) {
    if (IsBasicStatus(condition)) {
        CureCondition(mon);
    }
    mon->core.conditions[condition] = value;
    mon->core.conditionCounters[condition] = 0;
}

// The Pokémon's condition of the given ID
static inline BattleCondition *GetConditionPtr(BattleMon *mon, BattleConditionID id) {
    return &mon->core.conditions[id];
}

BOOL func_ov167_021bb864(BattleMon *mon, u32 index, BattleCondition *prev, BOOL *cured) {
    u8 turns;
    BattleCondition cond;

    if (index == 2 || index == 6) {
        return FALSE;
    }
    if (GetConditionPtr(mon, index)->common.type != 0) {
        turns = func_ov167_021ce33c(*GetConditionPtr(mon, index));
        cond = *GetConditionPtr(mon, index);
        if (prev != NULL) {
            *prev = cond;
        }
        if (cured != NULL) {
            *cured = FALSE;
        }
        if (index == 0x1b && !MoveIsUsable(mon, Condition_GetParam(cond))) {
            *GetConditionPtr(mon, index) = ZeroConditionTurns();
            mon->core.conditionCounters[index] = 0;
            if (cured != NULL) {
                *cured = TRUE;
            }
        }
        if (turns != 0) {
            mon->core.conditionCounters[index]++;
            if (mon->core.conditionCounters[index] >= turns) {
                *GetConditionPtr(mon, index) = ZeroConditionTurns();
                mon->core.conditionCounters[index] = 0;
                if (cured != NULL) {
                    *cured = TRUE;
                }
            }
        } else if (GetConditionPtr(mon, index)->common.type == 1) {
            u32 max = GetConditionPtr(mon, index)->common.turns;

            if (max != 0 && mon->core.conditionCounters[index] < max) {
                mon->core.conditionCounters[index]++;
            }
        }
        return TRUE;
    }
    return FALSE;
}

// Whether the Pokémon has an ability and it is not suppressed
static inline BOOL CheckEffectiveAbility(BattleMon *mon, u16 ability) {
    if (CheckCondition(mon, 0x10)) {
        return FALSE;
    }
    if (mon->ability == ability) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov167_021bb930(BattleMon *mon) {
    BattleCondition *condition = &mon->core.conditions[2];
    u8 turns;
    u8 amount;

    if (condition->common.type != 0) {
        turns = func_ov167_021ce33c(*condition);
        if (turns != 0) {
            amount = CheckEffectiveAbility(mon, ABILITY_EARLY_BIRD) ? 2 : 1;
            mon->core.conditionCounters[2] += amount;
            if (mon->core.conditionCounters[2] >= turns) {
                *condition = ZeroConditionTurns();
                CureMoveCondition(mon, 2);
                return TRUE;
            }
        }
    }
    return FALSE;
}
BOOL func_ov167_021bb9a8(BattleMon *mon) {
    BattleCondition *condition = &mon->core.conditions[6];
    u8 turns;

    if (condition->common.type != 0) {
        turns = func_ov167_021ce33c(*condition);
        if (turns != 0) {
            mon->core.conditionCounters[6]++;
            if (mon->core.conditionCounters[6] >= turns) {
                *condition = ZeroConditionTurns();
                CureMoveCondition(mon, 6);
                return TRUE;
            }
        }
    }
    return FALSE;
}

void CureCondition(BattleMon *mon) {
    u32 i;

    for (i = 1; i < 6; i++) {
        mon->core.conditions[i] = ZeroConditionTurns();
        mon->core.conditionCounters[i] = 0;
        CureDependentCondition(mon, i);
    }
}

void CureDependentCondition(BattleMon *mon, u32 condition) {
    if (condition == 2) {
        mon->core.conditions[9] = ZeroConditionTurns();
        mon->core.conditionCounters[9] = 0;
    }
}

void CureMoveCondition(BattleMon *mon, u32 condition) {
    if (IsBasicStatus(condition)) {
        CureCondition(mon);
    } else {
        mon->core.conditions[condition] = ZeroConditionTurns();
        mon->core.conditionCounters[condition] = 0;
    }
}

void func_ov167_021bba64(BattleMon *mon, u32 monId) {
    u32 i;

    if (monId != 0x1f) {
        for (i = 0; i < 0x24; i++) {
            if (!func_ov167_021ce168(mon->core.conditions[i])) {
                if (Condition_GetMonID(mon->core.conditions[i]) == monId) {
                    mon->core.conditions[i] = ZeroConditionTurns();
                    CureDependentCondition(mon, i);
                }
            }
        }
    }
}

u32 GetBattleMonStatus(BattleMon *mon) {
    u32 i;

    for (i = 1; i < 6; i++) {
        if (mon->core.conditions[i].common.type != 0) {
            return i;
        }
    }
    return 0;
}

BOOL CheckCondition(const BattleMon *mon, u32 index) {
    return mon->core.conditions[index].common.type != 0;
}

u16 GetDisabledMove(BattleMon *mon, u32 index) {
    switch (mon->core.conditions[index].common.type) {
    case 2:
        return (mon->core.conditions[index].raw << 7) >> 16;
    case 3:
        return (mon->core.conditions[index].raw << 23) >> 26;
    case 4:
        return (mon->core.conditions[index].raw << 17) >> 26;
    default:
        return 0;
    }
}

BattleConditionCont GetConditionContinuationParam(BattleMon *mon, u32 index) {
    return mon->core.conditions[index];
}

u8 func_ov167_021bbb1c(BattleMon *mon, u32 index) {
    return mon->core.conditionCounters[index];
}

u32 func_ov167_021bbb24(BattleMon *mon, u32 index) {
    if (CheckCondition(mon, index)) {
        switch (index) {
        case 5:
            if (Condition_IsBadlyPoisoned(mon->core.conditions[index])) {
                return DivideMaxHPZeroCheck(mon, 16) * mon->core.conditionCounters[index];
            }
            return DivideMaxHPZeroCheck(mon, 8);
        case 4:
            return DivideMaxHPZeroCheck(mon, 8);
        case 9:
            if (CheckCondition(mon, 2)) {
                return DivideMaxHPZeroCheck(mon, 4);
            }
            break;
        case 10:
            return DivideMaxHPZeroCheck(mon, 4);
        default:
            return 0;
        }
    }
    return 0;
}

void ClearMoveStatusWork(BattleMon *mon, u32 flag) {
    u32 i;

    i = 0;
    if (flag == 0) {
        i = 6;
    }
    for (; i < 0x24; i++) {
        mon->core.conditions[i].raw = 0;
        mon->core.conditions[i].common.type = 0;
    }
    sys_memset(mon->core.conditionCounters, 0, 0x24);
}

void func_ov167_021bbbec(BattleMon *mon, u16 turn) {
    mon->unk148 = turn;
    mon->unk146 = 0;
    mon->core.unk1b_7 = 1;
    func_ov167_021bbff4(mon);
}

void func_ov167_021bbc08(BattleMon *mon) {
    sys_memset(mon->turnFlags, 0, sizeof(mon->turnFlags));
    if (mon->unk146 < 9999) {
        mon->unk146++;
    }
    func_ov167_021bc024(mon);
}

void func_ov167_021bbc40(BattleMon *mon, u32 flag) {
    u8 *flags = mon->turnFlags;

    flags[(u8)(flag >> 3)] &= (u8) ~(u8)(1 << (flag & 7));
}

void Clear_ForFainted(BattleMon *mon) {
    sys_memset(mon->turnFlags, 0, sizeof(mon->turnFlags));
    sys_memset(mon->conditionFlags, 0, sizeof(mon->conditionFlags));
    ClearFormChange(mon);
    MoveWork_ClearSurface(mon);
    ClearUsedMoveFlag(mon);
    ClearCounter(mon);
    ComboMove_ClearParam(mon);
    IllusionBreak(mon);
    ResetSpActPriority(mon);
    ClearMoveStatusWork(mon, 1);
    func_ov167_021babc0(&mon->statStages);
    mon->form = mon->core.baseForm;
    mon->ability = mon->core.baseAbility;
    PokeParty_SetParam(mon->core.src, PKM_PARAM_FORM, mon->form);
    PokeParty_RecalcStats(mon->core.src);
}

void Clear_ForSwitch(BattleMon *mon) {
    sys_memset(mon->turnFlags, 0, sizeof(mon->turnFlags));
    ClearFormChange(mon);
    MoveWork_ClearSurface(mon);
    ClearUsedMoveFlag(mon);
    ClearCounter(mon);
    ComboMove_ClearParam(mon);
    IllusionBreak(mon);
    if (!GetAdditionalConditionFlag(mon, 0xe)) {
        ResetSpActPriority(mon);
        ClearMoveStatusWork(mon, 0);
        func_ov167_021babc0(&mon->statStages);
        sys_memset(mon->conditionFlags, 0, sizeof(mon->conditionFlags));
    }
    mon->form = mon->core.baseForm;
    mon->ability = mon->core.baseAbility;
    PokeParty_SetParam(mon->core.src, PKM_PARAM_FORM, mon->form);
    PokeParty_RecalcStats(mon->core.src);
}

void func_ov167_021bbd80(BattleMon *mon) {
    setupBySrcData(mon, mon->core.src, FALSE, TRUE);
    sys_memset(mon->conditionFlags, 0, sizeof(mon->conditionFlags));
    sys_memset(mon->turnFlags, 0, sizeof(mon->turnFlags));
    ResetSpActPriority(mon);
    ClearUsedMoveFlag(mon);
    ClearMoveStatusWork(mon, 0);
    func_ov167_021babc0(&mon->statStages);
}

void CopyBatonPassParams(BattleMon *target, BattleMon *source) {
    u32 i;
    u16 attack;
    u16 defense;

    target->statStages = source->statStages;
    target->substituteHP = source->substituteHP;
    for (i = 0; i < 36; i++) {
        if (GetConditionPtr(source, i)->common.type != 0 && func_ov169_0689c9f0(i)) {
            *GetConditionPtr(target, i) = *GetConditionPtr(source, i);
            target->core.conditionCounters[i] = source->core.conditionCounters[i];
        }
    }
    if (GetAdditionalConditionFlag(source, 10)) {
        func_ov167_021bb7e4(target, 10);
        attack = RawBattleMonStat(target, 8);
        defense = RawBattleMonStat(target, 9);
        SetBaseStatus(target, 8, defense);
        SetBaseStatus(target, 9, attack);
    }
    if (GetAdditionalConditionFlag(source, 9)) {
        func_ov167_021bb7e4(target, 9);
    }
    ResetSpActPriority(source);
    ClearMoveStatusWork(source, 0);
    func_ov167_021babc0(&source->statStages);
    sys_memset(source->conditionFlags, 0, sizeof(source->conditionFlags));
}

void ChangePokeType(BattleMon *mon, u16 type) {
    mon->type1 = PokeTypePair_GetType1(type);
    mon->type2 = PokeTypePair_GetType2(type);
}

void ChangeAbility(BattleMon *mon, u32 ability) {
    mon->ability = ability;
}

void ChangeForm(BattleMon *mon, u8 form) {
    BOOL wasEncrypted;

    mon->form = form;
    wasEncrypted = PokeParty_DecryptPkm(mon->core.src);
    PokeParty_ChangeForme(mon->core.src, form);
    setupBySrcDataBase(mon, mon->core.src, TRUE);
    if (mon->core.species == SPECIES_SHAYMIN && form == 0) {
        mon->core.baseForm = 0;
        mon->ability = PokeParty_GetParam(mon->core.src, PKM_PARAM_ABILITY, NULL);
    }
    PokeParty_EncryptPkm(mon->core.src, wasEncrypted);
}

void ConsumeItem(BattleMon *mon, u16 item) {
    mon->core.consumedItem = item;
    mon->core.heldItem = 0;
}

void ClearConsumedItem(BattleMon *mon) {
    mon->core.consumedItem = 0;
}

u16 GetConsumedItem(BattleMon *mon) {
    return mon->core.consumedItem;
}

void func_ov167_021bbf44(BattleMon *mon, u8 targetPos, BOOL success, u8 unk144, u16 moveId, u16 moveUsed) {
    u16 prevMoveId = mon->prevMoveId;

    mon->prevMoveUsed = moveUsed;
    mon->prevMoveId = moveId;
    mon->prevTargetPos = targetPos;
    mon->unk144 = unk144;
    if (prevMoveId == moveId) {
        if (success) {
            mon->consecutiveMoveCount++;
        } else {
            mon->consecutiveMoveCount = 0;
        }
    } else {
        mon->consecutiveMoveCount = success ? 1 : 0;
    }
}

u32 GetConsecutiveMoveCount(BattleMon *mon) {
    return mon->consecutiveMoveCount;
}

u16 GetPreviousMoveID(BattleMon *mon) {
    return mon->prevMoveId;
}

u8 func_ov167_021bbfb0(BattleMon *mon) {
    return mon->unk144;
}

u16 GetPreviousMoveUsed(BattleMon *mon) {
    return mon->prevMoveUsed;
}

u8 GetPrevTargetPos(BattleMon *mon) {
    return mon->prevTargetPos;
}

BOOL func_ov167_021bbfd0(BattleMon *mon) {
    return mon->core.unk1b_7;
}

void SetWeight(BattleMon *mon, u16 weight) {
    if (weight < 1) {
        weight = 1;
    }
    mon->weight = weight;
}

u16 GetBattleMonWeight(BattleMon *mon) {
    return mon->weight;
}

void func_ov167_021bbff4(BattleMon *mon) {
    sys_memset(mon->damageRecords, 0, sizeof(mon->damageRecords));
    sys_memset(mon->damageRecordCounts, 0, sizeof(mon->damageRecordCounts));
    mon->damageRecordTurn = 0;
    mon->unk1f0 = 0;
}

void func_ov167_021bc024(BattleMon *mon) {
    mon->damageRecordTurn++;
    if (mon->damageRecordTurn >= 3) {
        mon->damageRecordTurn = 0;
    }
    mon->damageRecordCounts[mon->damageRecordTurn] = 0;
}

void func_ov167_021bc048(BattleMon *mon, const BattleMonDamageRecord *record) {
    u8 turn;
    u8 count;
    u32 i;

    turn = mon->damageRecordTurn;
    count = mon->damageRecordCounts[turn];
    if (count == 6) {
        for (i = 0; i < 5; i++) {
            mon->damageRecords[turn][i] = mon->damageRecords[turn][i + 1];
        }
        count--;
    }
    mon->damageRecords[turn][count] = *record;
    if (mon->damageRecordCounts[turn] < 6) {
        mon->damageRecordCounts[turn]++;
    }
}

u8 func_ov167_021bc120(BattleMon *mon, u8 turnsAgo) {
    s32 turn;

    if (turnsAgo < 3) {
        turn = mon->damageRecordTurn - turnsAgo;
        if (turn < 0) {
            turn += 3;
        }
        return mon->damageRecordCounts[turn];
    }
    return 0;
}

BOOL GetDamageReceived(const BattleMon *mon, u8 turnsAgo, u8 index, BattleMonDamageRecord *record) {
    s32 turn;
    u8 count;

    if (turnsAgo < 3) {
        turn = mon->damageRecordTurn - turnsAgo;
        if (turn < 0) {
            turn += 3;
        }
        count = mon->damageRecordCounts[turn];
        if (index < count) {
            *record = mon->damageRecords[turn][(u8)(count - index - 1)];
            return TRUE;
        }
    }
    return FALSE;
}

void COUNTER_Set(BattleMon *mon, u32 index, u8 value) {
    mon->counters[index] = value;
}

u8 GetConditionCount(BattleMon *mon, u32 index) {
    return mon->counters[index];
}

BOOL func_ov167_021bc1b8(BattleMon *mon, u32 *exp, BattleMonLevelUp *levelUp) {
    u32 oldExp;
    u32 newExp;
    u32 nextExp;
    u32 needed;
    u16 maxHP;
    u16 attack;
    u16 defense;
    u16 spAttack;
    u16 spDefense;
    u16 speed;
    u8 wasEncrypted;

    if (mon->core.level < 100) {
        oldExp = mon->core.exp;
        newExp = oldExp + *exp;
        nextExp = PML_UtilGetPkmLvExp(mon->core.species, mon->form, mon->core.level + 1);
        if (newExp >= nextExp) {
            maxHP = mon->core.maxHP;
            needed = nextExp - oldExp;
            wasEncrypted = PokeParty_DecryptPkm(mon->core.src);
            attack = PokeParty_GetParam(mon->core.src, PKM_PARAM_ATTACK, NULL);
            defense = PokeParty_GetParam(mon->core.src, PKM_PARAM_DEFENSE, NULL);
            speed = PokeParty_GetParam(mon->core.src, PKM_PARAM_SPEED, NULL);
            spAttack = PokeParty_GetParam(mon->core.src, PKM_PARAM_SP_ATTACK, NULL);
            spDefense = PokeParty_GetParam(mon->core.src, PKM_PARAM_SP_DEFENSE, NULL);
            mon->core.exp = oldExp + needed;
            PokeParty_SetParam(mon->core.src, PKM_PARAM_EXP, mon->core.exp);
            PokeParty_RecalcStats(mon->core.src);
            mon->core.maxHP = PokeParty_GetParam(mon->core.src, PKM_PARAM_MAX_HP, NULL);
            mon->core.level = PokeParty_GetParam(mon->core.src, PKM_PARAM_LEVEL, NULL);
            maxHP = mon->core.maxHP - maxHP;
            levelUp->attack = PokeParty_GetParam(mon->core.src, PKM_PARAM_ATTACK, NULL);
            levelUp->defense = PokeParty_GetParam(mon->core.src, PKM_PARAM_DEFENSE, NULL);
            levelUp->spAttack = PokeParty_GetParam(mon->core.src, PKM_PARAM_SP_ATTACK, NULL);
            levelUp->spDefense = PokeParty_GetParam(mon->core.src, PKM_PARAM_SP_DEFENSE, NULL);
            levelUp->speed = PokeParty_GetParam(mon->core.src, PKM_PARAM_SPEED, NULL);
            if (!mon->core.transformed) {
                mon->attack = levelUp->attack;
                mon->defense = levelUp->defense;
                mon->spAttack = levelUp->spAttack;
                mon->spDefense = levelUp->spDefense;
                mon->speed = levelUp->speed;
            }
            levelUp->level = mon->core.level;
            levelUp->hp = maxHP;
            levelUp->attack -= attack;
            levelUp->defense -= defense;
            levelUp->spAttack -= spAttack;
            levelUp->spDefense -= spDefense;
            levelUp->speed -= speed;
            mon->core.hp += maxHP;
            PokeParty_SetParam(mon->core.src, PKM_PARAM_HP, mon->core.hp);
            SetMovesAndPP(mon);
            PokeParty_EncryptPkm(mon->core.src, wasEncrypted);
            *exp -= needed;
            return TRUE;
        }
        mon->core.exp = newExp;
        PokeParty_SetParam(mon->core.src, PKM_PARAM_EXP, newExp);
    }
    if (levelUp != NULL) {
        sys_memset(levelUp, 0, sizeof(BattleMonLevelUp));
    }
    return FALSE;
}

u32 GetExpForLv100(BattleMon *mon) {
    return PML_UtilGetPkmLvExp(mon->core.species, mon->form, 100);
}

void func_ov167_021bc384(BattleMon *mon, BOOL keepBaseForm) {
    PartyPkm *src = mon->core.src;
    u32 form;

    PokeParty_SetParam(src, PKM_PARAM_EXP, mon->core.exp);
    PokeParty_SetParam(src, PKM_PARAM_HP, mon->core.hp);
    if (mon->core.hp != 0) {
        PokeParty_SetStatusCond(src, GetBattleMonStatus(mon));
    } else {
        PokeParty_SetStatusCond(src, 0);
    }
    SetMovesAndPP(mon);
    if (mon->core.transformed || keepBaseForm) {
        form = mon->core.baseForm;
    } else {
        form = mon->form;
    }
    PokeParty_SetParam(src, PKM_PARAM_FORM, (u8)form);
    PokeParty_RecalcStats(src);
    PokeParty_SetParam(src, PKM_PARAM_ITEM, mon->core.heldItem);
}

void func_ov167_021bc3fc(BattleMon *mon) {
    if (!mon->core.transformed) {
        setupBySrcData(mon, mon->core.src, TRUE, FALSE);
        func_ov167_021ba8b4(mon);
    } else {
        GetNumMoves(mon, mon->core.src, FALSE);
    }
}

BOOL IsIllusionEnabled(BattleMon *mon) {
    return mon->core.illusion;
}

void IllusionBreak(BattleMon *mon) {
    mon->core.illusion = 0;
    mon->core.illusionDisguise = NULL;
}

void func_ov167_021bc43c(BattleMon *mon, PartyPkm *src) {
    PokeParty_GetParam(src, PKM_PARAM_SPECIES, NULL);
    mon->core.src = src;
}

static BattleMoveWork sTransformMoveBackup[4];
static BattleMonCore sTransformCoreBackup;

BOOL TransformSet(BattleMon *mon, BattleMon *target) {
    u16 substituteHP;
    s32 i;

    if (!mon->core.transformed && !target->core.transformed && target->substituteHP == 0) {
        sys_memcpy(mon->moves, sTransformMoveBackup, sizeof(mon->moves));
        sTransformCoreBackup = mon->core;
        substituteHP = mon->substituteHP;
        *mon = *target;
        mon->core = sTransformCoreBackup;
        mon->substituteHP = substituteHP;
        sys_memcpy(sTransformMoveBackup, mon->moves, sizeof(mon->moves));
        for (i = 0; i < 4; i++) {
            MoveWork_UpdateNumber(&mon->moves[i], target->moves[i].surface.id, 5, FALSE);
        }
        sys_memset(mon->turnFlags, 0, sizeof(mon->turnFlags));
        sys_memset(mon->conditionFlags, 0, sizeof(mon->conditionFlags));
        mon->unk148 = 10000;
        mon->unk146 = 0;
        mon->prevMoveUsed = 0;
        mon->prevMoveId = 0;
        mon->unk144 = 0x11;
        mon->consecutiveMoveCount = 0;
        mon->core.transformed = 1;
        return TRUE;
    }
    return FALSE;
}

BOOL TransformCheck(BattleMon *mon) {
    return mon->core.transformed;
}

void func_ov167_021bc55c(BattleMon *mon, u16 value) {
    mon->substituteHP = value;
    CureMoveCondition(mon, 8);
}

void ResetSpActPriority(BattleMon *mon) {
    mon->substituteHP = 0;
}

BOOL IsSubstituteActive(BattleMon *mon) {
    if (mon->substituteHP != 0) {
        return TRUE;
    }
    return FALSE;
}

u32 func_ov167_021bc590(BattleMon *mon) {
    return mon->substituteHP;
}

BOOL func_ov167_021bc59c(BattleMon *mon, u16 *damage) {
    if (mon->substituteHP <= *damage) {
        *damage = mon->substituteHP;
        mon->substituteHP = 0;
        return TRUE;
    }
    mon->substituteHP -= *damage;
    return FALSE;
}

void func_ov167_021bc5c4(BattleMon *mon) {
    mon->core.unkD0Count = 0;
}

void func_ov167_021bc5cc(BattleMon *mon, u8 monId) {
    u32 i;

    for (i = 0; i < mon->core.unkD0Count; i++) {
        if (monId == mon->core.unkD1[i]) {
            return;
        }
    }
    if (i < 24) {
        mon->core.unkD1[i] = monId;
        mon->core.unkD0Count++;
    }
}

u8 func_ov167_021bc604(BattleMon *mon) {
    return mon->core.unkD0Count;
}

u8 func_ov167_021bc60c(BattleMon *mon, u8 index) {
    if (index < mon->core.unkD0Count) {
        return mon->core.unkD1[index];
    }
    return 0x1f;
}

void func_ov167_021bc624(BattleMon *mon, u16 item) {
    u32 ball = PML_ItemGetMonsBallID(item);

    if (ball == 0) {
        ball = 4;
    }
    PokeParty_SetParam(mon->core.src, PKM_PARAM_POKEBALL, ball);
}

void func_ov167_021bc640(BattleMon *mon, u8 monId, u16 move) {
    mon->comboMonId = monId;
    mon->comboMove = move;
}

BOOL func_ov167_021bc650(BattleMon *mon, u8 *monId, u16 *move) {
    if (mon->comboMonId != 0x1f) {
        *monId = mon->comboMonId;
        *move = mon->comboMove;
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov167_021bc674(BattleMon *mon) {
    if (mon->comboMonId != 0x1f) {
        return TRUE;
    }
    return FALSE;
}

void ComboMove_ClearParam(BattleMon *mon) {
    if (mon->comboMonId != 0x1f) {
        mon->comboMonId = 0x1f;
        mon->comboMove = 0;
    }
}

void func_ov167_021bc6a0(BattleMon *mon) {
    PokeParty_RecalcStats(mon->core.src);
}

BOOL func_ov167_021bc6ac(BattleMon *mon) {
    if (mon->core.hp == 0x29) {
        return TRUE;
    }
    return FALSE;
}
