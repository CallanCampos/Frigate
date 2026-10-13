#include "battle/btl_pospoke.h"
#include "types.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"

static void BtlPosPoke_SetupPos(BtlPosPoke *posPoke, BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 pos);
static void BtlPosPoke_RecordOpponents(BtlPosPoke *posPoke, u8 pos, BtlPokeCon *pokeCon);
static void BtlPosPoke_UpdatePosOfMon(BtlPosPoke *posPoke, u8 pos);

void BtlPosPoke_Init(BtlPosPoke *posPoke, BtlMainModule *mainModule, BtlPokeCon *pokeCon, u32 battleStyle) {
    u32 i;
    u32 numPos;
    u32 pos;

    for (i = 0; i < BTL_POSPOKE_NUM_POS; i++) {
        posPoke->entries[i].exists = FALSE;
        posPoke->entries[i].monId = BTL_POSPOKE_NO_MON;
    }
    for (i = 0; i < BTL_POSPOKE_NUM_POS; i++) {
        posPoke->posOfMon[i] = BTL_POS_NONE;
    }
    numPos = GetValidPosMax(mainModule);
    for (pos = 0; pos <= numPos; pos++) {
        BtlPosPoke_SetupPos(posPoke, mainModule, pokeCon, pos);
    }
}

static void BtlPosPoke_SetupPos(BtlPosPoke *posPoke, BtlMainModule *mainModule, BtlPokeCon *pokeCon, u8 pos) {
    u8 clientId;
    u8 index;
    BattleMon *mon;

    func_ov167_0219c694(mainModule, pos, &clientId, &index);
    mon = GetClientMonData(pokeCon, clientId, index);
    if (mon != NULL && !IsFainted(mon)) {
        u8 monId = GetMonID(mon);

        posPoke->entries[pos].monId = monId;
        posPoke->posOfMon[monId] = pos;
    } else {
        posPoke->entries[pos].monId = BTL_POSPOKE_NO_MON;
    }
    posPoke->entries[pos].clientId = clientId;
    posPoke->entries[pos].exists = TRUE;
}

void BtlPosPoke_RemoveMon(BtlPosPoke *posPoke, u8 monId) {
    u32 i;

    for (i = 0; i < BTL_POSPOKE_NUM_POS; i++) {
        if (posPoke->entries[i].exists && posPoke->entries[i].monId == monId) {
            posPoke->entries[i].monId = BTL_POSPOKE_NO_MON;
            return;
        }
    }
}

void BtlPosPoke_SetMon(BtlPosPoke *posPoke, u8 pos, u8 monId, BtlPokeCon *pokeCon) {
    posPoke->entries[pos].monId = monId;
    posPoke->posOfMon[monId] = pos;
    BtlPosPoke_RecordOpponents(posPoke, pos, pokeCon);
}

void BtlPosPoke_Rotate(BtlPosPoke *posPoke, u8 slot, u8 clientId, BattleMon *mon, BtlPokeCon *pokeCon) {
    u8 positions[5];
    u32 i;
    u32 count;
    u32 from;
    u32 to;
    BtlPosPokeEntry first;

    if (slot > 1) {
        count = 0;
        for (i = 0; i < BTL_POSPOKE_NUM_POS; i++) {
            if (posPoke->entries[i].clientId == clientId) {
                positions[count++] = i;
            }
        }
        from = func_ov167_0219d38c(slot);
        to = func_ov167_0219d3a4(slot);
        first = posPoke->entries[positions[0]];
        posPoke->entries[positions[from]] = posPoke->entries[positions[to]];
        posPoke->entries[positions[to]] = first;
        if (!IsFainted(mon)) {
            BtlPosPoke_SetMon(posPoke, positions[0], GetMonID(mon), pokeCon);
        } else {
            // BUG: i is BTL_POSPOKE_NUM_POS here, so this writes past the entries, over posOfMon
#ifdef BUGFIX
            posPoke->entries[positions[0]].monId = BTL_POSPOKE_NO_MON;
#else
            posPoke->entries[i].monId = BTL_POSPOKE_NO_MON;
#endif
        }
        for (i = 0; i < count; i++) {
            BtlPosPoke_UpdatePosOfMon(posPoke, positions[i]);
        }
    }
}

static void BtlPosPoke_RecordOpponents(BtlPosPoke *posPoke, u8 pos, BtlPokeCon *pokeCon) {
    u32 i;
    u8 monId = posPoke->entries[pos].monId;

    if (monId != BTL_POSPOKE_NO_MON) {
        for (i = 0; i < BTL_POSPOKE_NUM_POS; i++) {
            BtlPosPokeEntry *other = &posPoke->entries[i];

            if (other->monId != BTL_POSPOKE_NO_MON && monId != other->monId && !IsAllyMonID(other->monId, monId)) {
                func_ov167_021bc5cc(GetPokeParam(pokeCon, other->monId), monId);
                func_ov167_021bc5cc(GetPokeParam(pokeCon, monId), other->monId);
            }
        }
    }
}

void BtlPosPoke_Swap(BtlPosPoke *posPoke, u8 firstPos, u8 secondPos) {
    BtlPosPokeEntry tmp = posPoke->entries[firstPos];

    posPoke->entries[firstPos] = posPoke->entries[secondPos];
    posPoke->entries[secondPos] = tmp;
    BtlPosPoke_UpdatePosOfMon(posPoke, firstPos);
    BtlPosPoke_UpdatePosOfMon(posPoke, secondPos);
}

static void BtlPosPoke_UpdatePosOfMon(BtlPosPoke *posPoke, u8 pos) {
    u8 monId = posPoke->entries[pos].monId;

    if (monId != BTL_POSPOKE_NO_MON) {
        posPoke->posOfMon[monId] = pos;
    }
}

u8 BtlPosPoke_GetEmptyPositions(BtlPosPoke *posPoke, u8 clientId, u8 *positions) {
    u32 count = 0;
    u32 i;

    for (i = 0; i < BTL_POSPOKE_NUM_POS; i++) {
        if (posPoke->entries[i].exists && posPoke->entries[i].clientId == clientId
            && posPoke->entries[i].monId == BTL_POSPOKE_NO_MON) {
            positions[count++] = i;
        }
    }
    return count;
}

BOOL DoesBattleMonExist(BtlPosPoke *posPoke, u8 monId) {
    if (GetBattlePos(posPoke, monId) != BTL_POS_NONE) {
        return TRUE;
    }
    return FALSE;
}

BOOL BtlPosPoke_IsMonAtActivePos(BtlPosPoke *posPoke, BtlMainModule *mainModule, u8 monId) {
    u8 pos = GetBattlePos(posPoke, monId);

    if (pos != BTL_POS_NONE) {
        return func_ov167_0219bebc(mainModule, pos);
    }
    return FALSE;
}

u8 GetBattlePos(BtlPosPoke *posPoke, u8 monId) {
    u32 i;

    for (i = 0; i < BTL_POSPOKE_NUM_POS; i++) {
        if (posPoke->entries[i].exists && posPoke->entries[i].monId == monId) {
            return i;
        }
    }
    return BTL_POS_NONE;
}

u8 BtlPosPoke_GetPosOfMon(BtlPosPoke *posPoke, u8 monId) {
    return posPoke->posOfMon[monId];
}

u8 GetExistPokeID(BtlPosPoke *posPoke, u8 pos) {
    return posPoke->entries[pos].monId;
}
