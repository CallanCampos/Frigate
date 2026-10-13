#include "battle/btl_monset.h"
#include "types.h"
#include "battle/btl_calc.h"
#include "battle/btl_main.h"
#include "battle/btl_pokeparam.h"
#include "battle/btl_server_flow.h"
#include "gfl/std.h"

void BtlMonSet_Clear(BtlMonSet *set) {
    sys_memset(set, 0, sizeof(BtlMonSet));
}

void BtlMonSet_Add(BtlMonSet *set, BattleMon *mon) {
    u32 i;
    u8 count = set->count;

    if (count < BTL_MONSET_MAX) {
        for (i = 0; i < count; i++) {
            if (set->mons[i] == mon) {
                return;
            }
        }
        set->mons[set->count] = mon;
        set->damageKind[set->count] = BTL_MONSET_NO_DAMAGE;
        set->damage[set->count] = 0;
        set->substituteDamage[set->count] = 0;
        set->count++;
        if (set->count > set->countMax) {
            set->countMax = set->count;
        }
    }
}

void BtlMonSet_AddDamage(BtlMonSet *set, BattleMon *mon, u16 damage, BOOL substitute) {
    u32 i;

    BtlMonSet_Add(set, mon);
    for (i = 0; i < set->count; i++) {
        if (set->mons[i] == mon) {
            if (substitute) {
                set->substituteDamage[i] += damage;
                set->damageKind[i] = BTL_MONSET_SUBSTITUTE_DAMAGE;
            } else {
                set->damage[i] += damage;
                set->damageKind[i] = BTL_MONSET_HP_DAMAGE;
            }
        }
    }
}

void BtlMonSet_Remove(BtlMonSet *set, BattleMon *mon) {
    u32 count = set->count;
    u32 i;
    u32 j;

    for (i = 0; i < count; i++) {
        if (set->mons[i] == mon) {
            for (j = i + 1; j < set->count; j++) {
                set->mons[j - 1] = set->mons[j];
            }
            set->count--;
            set->getIndex--;
            return;
        }
    }
}

BattleMon *BtlMonSet_Get(BtlMonSet *set, u32 index) {
    if (index < set->count) {
        return set->mons[index];
    }
    return NULL;
}

void BtlMonSet_ResetIndex(BtlMonSet *set) {
    set->getIndex = 0;
}

BattleMon *BtlMonSet_GetNext(BtlMonSet *set) {
    u8 index = set->getIndex;

    if (index < set->count) {
        set->getIndex++;
        return set->mons[index];
    }
    return NULL;
}

BOOL BtlMonSet_GetTotalDamage(const BtlMonSet *set, BattleMon *mon, u32 *damage) {
    u32 i;

    for (i = 0; i < set->count; i++) {
        if (set->mons[i] == mon && set->damageKind[i] != BTL_MONSET_NO_DAMAGE) {
            *damage = set->damage[i] + set->substituteDamage[i];
            return TRUE;
        }
    }
    return FALSE;
}

BOOL BtlMonSet_GetHpDamage(const BtlMonSet *set, BattleMon *mon, u32 *damage) {
    u32 i;

    for (i = 0; i < set->count; i++) {
        if (set->mons[i] == mon && set->damageKind[i] == BTL_MONSET_HP_DAMAGE) {
            *damage = set->damage[i];
            return TRUE;
        }
    }
    return FALSE;
}

u32 BtlMonSet_GetCount(BtlMonSet *set) {
    return set->count;
}

u32 BtlMonSet_GetCountMax(BtlMonSet *set) {
    return set->countMax;
}

void BtlMonSet_SetNumTargets(BtlMonSet *set, u8 numTargets) {
    set->numTargets = numTargets;
}

BOOL BtlMonSet_IsEmptyAfterTargeting(BtlMonSet *set) {
    if (set->countMax != 0 || set->numTargets != 0) {
        if (set->count == 0) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

u32 BtlMonSet_CopyNotFainted(BtlMonSet *set, BtlMonSet *dest) {
    u32 count;
    u32 i;
    BattleMon *mon;

    BtlMonSet_Clear(dest);
    count = BtlMonSet_GetCount(set);
    for (i = 0; i < count; i++) {
        mon = BtlMonSet_Get(set, i);
        if (!IsFainted(mon)) {
            BtlMonSet_Add(dest, mon);
        }
    }
    return BtlMonSet_GetCount(dest);
}

u32 BtlMonSet_CopyAllies(BtlMonSet *set, BattleMon *mon, BtlMonSet *out) {
    u32 count;
    u32 i;
    BattleMon *other;
    u8 monId;

    BtlMonSet_Clear(out);
    monId = GetMonID(mon);
    count = BtlMonSet_GetCount(set);
    for (i = 0; i < count; i++) {
        other = BtlMonSet_Get(set, i);
        if (IsAllyMonID(monId, GetMonID(other))) {
            BtlMonSet_Add(out, other);
        }
    }
    return BtlMonSet_GetCount(out);
}

u32 BtlMonSet_CopyOpponents(BtlMonSet *set, BattleMon *mon, BtlMonSet *out) {
    u32 count;
    u32 i;
    BattleMon *other;
    u8 monId;

    BtlMonSet_Clear(out);
    monId = GetMonID(mon);
    count = BtlMonSet_GetCount(set);
    for (i = 0; i < count; i++) {
        other = BtlMonSet_Get(set, i);
        if (!IsAllyMonID(monId, GetMonID(other))) {
            BtlMonSet_Add(out, other);
        }
    }
    return BtlMonSet_GetCount(out);
}

void BtlMonSet_RemoveFainted(BtlMonSet *set) {
    BattleMon *mon;

    BtlMonSet_ResetIndex(set);
    while ((mon = BtlMonSet_GetNext(set)) != NULL) {
        if (IsFainted(mon)) {
            BtlMonSet_Remove(set, mon);
        }
    }
}

void SortBySpeed(BtlMonSet *set, BtlServerFlow *flow) {
    u32 i;
    u32 j;

    if (set->count > 1) {
        for (i = 0; i < set->count; i++) {
            set->speed[i] = func_ov167_021abd08(flow, set->mons[i], TRUE);
        }
        for (i = 0; i < set->count; i++) {
            for (j = i + 1; j < set->count; j++) {
                if (set->speed[j] > set->speed[i] || (set->speed[j] == set->speed[i] && BattleRandom(2))) {
                    u16 damage = set->damage[i];
                    BattleMon *mon = set->mons[i];
                    u16 speed = set->speed[i];

                    set->mons[i] = set->mons[j];
                    set->damage[i] = set->damage[j];
                    set->speed[i] = set->speed[j];
                    set->mons[j] = mon;
                    set->damage[j] = damage;
                    set->speed[j] = speed;
                }
            }
        }
    }
}
