#include "types.h"
#include "save/high_link.h"
#include "constants/pokemon.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "pml/waza.h"

// The pass powers in effect: one for each effect, with the time it has left, and what each does to the game's
// numbers. The file's name is a guess, from what it does: the ROM has no string for it. The functions' names are
// swan's

// The pass powers' table
#define ARCID_PASS_POWER 37

// A second of a pass power's time, and the HP that stands for all the party's HP and PP
#define PASS_POWER_TICKS_PER_SECOND 60
#define PASS_POWER_RESTORE_ALL 999

// The levels of the pass powers whose value is a level
#define PASS_POWER_EXPLORING_LEVELS 3
#define PASS_POWER_GROTTO_LEVELS 4
#define PASS_POWER_LUCKY_LEVELS 4

typedef struct {
    // The time left; at 1 the pass power is spent, and runs on until it is ended
    u32 timers[PASS_POWER_EFFECT_COUNT];
    u8 passPowers[PASS_POWER_EFFECT_COUNT];
    u16 values[PASS_POWER_EFFECT_COUNT];
    // The effect of the pass power bought last, or PASS_POWER_EFFECT_COUNT
    u8 paidEffect;
} PassPowerState;

static void PassPower_DeactivateAllEffects(void);
static void PassPower_DeactivateEffect(int effect);
static void PassPower_Dummy(void);

// What the levels of the exploring pass powers add to the chance of a phenomenon, and those of the Hidden Grotto
// pass powers to the chance that a grotto fills
static const u32 PASSPOWER_EXPLORING_CHANCES[PASS_POWER_EXPLORING_LEVELS] = { 10, 15, 20 };
static const u32 PASSPOWER_HOLLOW_CHANCES[PASS_POWER_GROTTO_LEVELS] = { 10, 20, 30, 50 };

static PassPowerState g_PassPowerState;

void PassPower_Init(void) {
    sys_memset(&g_PassPowerState, 0, sizeof(PassPowerState));
    PassPower_DeactivateAllEffects();
}

void PassPower_Update(void) {
    PassPowerEffect i;

    for (i = 0; i < PASS_POWER_EFFECT_COUNT; i++) {
        if (g_PassPowerState.timers[i] > 1) {
            g_PassPowerState.timers[i]--;
        }
    }
    PassPower_Dummy();
}

static void PassPower_DeactivateAllEffects(void) {
    PassPowerEffect i;

    for (i = 0; i < PASS_POWER_EFFECT_COUNT; i++) {
        PassPower_DeactivateEffect(i);
    }
    g_PassPowerState.paidEffect = PASS_POWER_EFFECT_COUNT;
}

BOOL PassPower_IsIDInvalid(int passPower) {
    if (passPower == PASS_POWER_NONE) {
        return FALSE;
    }
    if (passPower >= PASS_POWER_MAX || (passPower >= PASS_POWER_BW1_MAX && passPower < PASS_POWER_NONE)) {
        return TRUE;
    }
    return FALSE;
}

BOOL PassPower_IsIDValid(int passPower) {
    if (passPower >= PASS_POWER_MAX || (passPower >= PASS_POWER_BW1_MAX && passPower <= PASS_POWER_NONE)) {
        return FALSE;
    }
    return TRUE;
}

BOOL PassPower_IsBW1Compatible(int passPower) {
    if (!PassPower_IsIDValid(passPower)) {
        return FALSE;
    }
    if (passPower < PASS_POWER_BW1_MAX) {
        return TRUE;
    }
    return FALSE;
}

u8 PassPower_GetSPowerBaseID(int passPower) {
    if (passPower >= PASS_POWER_SPECIAL_FIRST && passPower <= PASS_POWER_SPECIAL_LAST) {
        return passPower - PASS_POWER_SPECIAL_FIRST;
    }
    return 0xff;
}

void PassPower_Activate(u32 passPower, const void *data, BOOL paid) {
    int effect;
    const PassPowerData *entry;
    if (!PassPower_IsIDValid(passPower)) {
        return;
    }
    entry = &((const PassPowerData *)data)[passPower];
    effect = entry->effect;
    g_PassPowerState.passPowers[effect] = passPower;
    g_PassPowerState.timers[effect] = entry->minutes * (60 * PASS_POWER_TICKS_PER_SECOND);
    g_PassPowerState.values[effect] = entry->value;
    if (g_PassPowerState.paidEffect == effect) {
        g_PassPowerState.paidEffect = PASS_POWER_EFFECT_COUNT;
    }
    if (paid == TRUE) {
        g_PassPowerState.paidEffect = effect;
    }
    switch (effect) {
    case PASS_POWER_EFFECT_ENCOUNTER_UP:
        PassPower_DeactivateEffect(PASS_POWER_EFFECT_ENCOUNTER_DOWN);
        break;
    case PASS_POWER_EFFECT_ENCOUNTER_DOWN:
        PassPower_DeactivateEffect(PASS_POWER_EFFECT_ENCOUNTER_UP);
        break;
    case PASS_POWER_EFFECT_EXP_UP:
        PassPower_DeactivateEffect(PASS_POWER_EFFECT_EXP_DOWN);
        break;
    case PASS_POWER_EFFECT_EXP_DOWN:
        PassPower_DeactivateEffect(PASS_POWER_EFFECT_EXP_UP);
        break;
    }
}

u32 PassPower_GetUsedIDByPassPower(u32 passPower, void *data) {
    return g_PassPowerState.passPowers[((const PassPowerData *)data)[passPower].effect];
}

u32 PassPower_GetUsedIDByEffect(int effect) {
    return g_PassPowerState.passPowers[effect];
}

u32 PassPower_GetRemainingSeconds(int effect) {
    u32 seconds;

    if (g_PassPowerState.passPowers[effect] == PASS_POWER_NONE) {
        return 0;
    }
    seconds = g_PassPowerState.timers[effect] / PASS_POWER_TICKS_PER_SECOND;
    if (seconds == 0) {
        seconds = 1;
    }
    return seconds;
}

u32 PassPower_GetDepletedID(void) {
    PassPowerEffect i;

    for (i = 0; i < PASS_POWER_EFFECT_COUNT; i++) {
        if (g_PassPowerState.timers[i] == 1) {
            return g_PassPowerState.passPowers[i];
        }
    }
    return PASS_POWER_NONE;
}

void PassPower_Deactivate(u32 passPower, void *data) {
    PassPower_DeactivateEffect(((const PassPowerData *)data)[passPower].effect);
}

static void PassPower_DeactivateEffect(int effect) {
    if (effect < PASS_POWER_EFFECT_COUNT) {
        g_PassPowerState.passPowers[effect] = PASS_POWER_NONE;
        g_PassPowerState.timers[effect] = 0;
        g_PassPowerState.values[effect] = 0;
        if (g_PassPowerState.paidEffect == effect) {
            g_PassPowerState.paidEffect = PASS_POWER_EFFECT_COUNT;
        }
    }
}

void *PassPowerData_Create(HeapID heapId) {
    return GFL_ArcSysReadHeapNew(ARCID_PASS_POWER, 0, heapId);
}

void PassPowerData_Free(void *data) {
    GFL_HeapFree(data);
}

u8 PassPowerData_GetEffect(void *data, u32 passPower) {
    return ((const PassPowerData *)data)[passPower].effect;
}

u16 PassPowerData_GetPrice(void *data, u32 passPower) {
    return ((const PassPowerData *)data)[passPower].unk02;
}

static void PassPower_Dummy(void) {
}

u32 PassPower_ApplyEncounter(u32 rate) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_ENCOUNTER_UP] != PASS_POWER_NONE) {
        rate = g_PassPowerState.values[PASS_POWER_EFFECT_ENCOUNTER_UP] * rate / 100;
        if (rate > 100) {
            rate = 100;
        }
    } else if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_ENCOUNTER_DOWN] != PASS_POWER_NONE) {
        rate = g_PassPowerState.values[PASS_POWER_EFFECT_ENCOUNTER_DOWN] * rate / 100;
        if (rate > 100) {
            rate = 100;
        }
    }
    return rate;
}

u32 PassPower_ApplyHatch(u32 steps) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_HATCH] != PASS_POWER_NONE) {
        steps = g_PassPowerState.values[PASS_POWER_EFFECT_HATCH] * steps / 100;
    }
    return steps;
}

int PassPower_ApplyBefriend(int friendship) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_BEFRIEND] != PASS_POWER_NONE && friendship > 0) {
        friendship += g_PassPowerState.values[PASS_POWER_EFFECT_BEFRIEND];
    }
    return friendship;
}

u32 PassPower_ApplyBargain(u32 price) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_BARGAIN] != PASS_POWER_NONE) {
        price = g_PassPowerState.values[PASS_POWER_EFFECT_BARGAIN] * price / 100;
    }
    return price;
}

u32 PassPower_ApplyHPRestore(PokeParty *party) {
    PartyPkm *pkm;
    u32 index;
    BOOL wasEncrypted;
    int maxHP;
    int hp;

    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_HP_RESTORE] != PASS_POWER_NONE) {
        if (g_PassPowerState.values[PASS_POWER_EFFECT_HP_RESTORE] == PASS_POWER_RESTORE_ALL) {
            PokeParty_RecoverAll(party);
            index = 0;
        } else {
            index = PokeParty_GetFirstBattleReady(party);
            pkm = PokeParty_GetPkm(party, index);
            wasEncrypted = PokeParty_DecryptPkm(pkm);
            maxHP = PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL);
            hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);
            hp += g_PassPowerState.values[PASS_POWER_EFFECT_HP_RESTORE];
            if (hp > maxHP) {
                hp = maxHP;
            }
            PokeParty_SetParam(pkm, PKM_PARAM_HP, hp);
            PokeParty_EncryptPkm(pkm, wasEncrypted);
        }
        PassPower_DeactivateEffect(PASS_POWER_EFFECT_HP_RESTORE);
        return index;
    }
    return 0xff;
}

u32 PassPower_ApplyPPRestore(PokeParty *party) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_PP_RESTORE] != PASS_POWER_NONE) {
        u32 index = PokeParty_GetFirstBattleReady(party);
        PartyPkm *pkm = PokeParty_GetPkm(party, index);
        BOOL wasEncrypted = PokeParty_DecryptPkm(pkm);
        u32 i;

        for (i = 0; i < 4; i++) {
            u32 move = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);

            if (move != 0) {
                u32 maxPP = PML_MoveGetMaxPP(move, PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i, NULL));
                u32 pp = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP + i, NULL);

                pp += g_PassPowerState.values[PASS_POWER_EFFECT_PP_RESTORE];
                if (pp > maxPP) {
                    pp = maxPP;
                }
                PokeParty_SetParam(pkm, PKM_PARAM_MOVE1_PP + i, pp);
            }
        }
        PokeParty_EncryptPkm(pkm, wasEncrypted);
        PassPower_DeactivateEffect(PASS_POWER_EFFECT_PP_RESTORE);
        return index;
    }
    return 0xff;
}

u32 PassPower_ApplyEXP(u32 exp) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_EXP_UP] != PASS_POWER_NONE) {
        return g_PassPowerState.values[PASS_POWER_EFFECT_EXP_UP] * exp / 100;
    }
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_EXP_DOWN] != PASS_POWER_NONE) {
        return g_PassPowerState.values[PASS_POWER_EFFECT_EXP_DOWN] * exp / 100;
    }
    return exp;
}

u32 PassPower_ApplyPrizeMoney(u32 money) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_PRIZE_MONEY] != PASS_POWER_NONE) {
        money = g_PassPowerState.values[PASS_POWER_EFFECT_PRIZE_MONEY] * money / 100;
    }
    return money;
}

u32 PassPower_ApplyCapture(s32 rate) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_CAPTURE] != PASS_POWER_NONE) {
        s64 value = g_PassPowerState.values[PASS_POWER_EFFECT_CAPTURE];

        value = value * rate / 100;
        return value;
    }
    return rate;
}

// Only the third level halves the steps
u32 PassPower_ApplyExploring(u32 steps) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_EXPLORING] != PASS_POWER_NONE) {
        int level = g_PassPowerState.values[PASS_POWER_EFFECT_EXPLORING];

        if (level < PASS_POWER_EXPLORING_LEVELS) {
            if (level >= 2) {
                steps /= 2;
            }
        }
    }
    return steps;
}

u32 PassPower_ApplyExploringChance(u32 chance) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_EXPLORING] != PASS_POWER_NONE) {
        int level = g_PassPowerState.values[PASS_POWER_EFFECT_EXPLORING];

        if (level < PASS_POWER_EXPLORING_LEVELS) {
            chance += PASSPOWER_EXPLORING_CHANCES[level];
            if (chance > 100) {
                chance = 100;
            }
        }
    }
    return chance;
}

u32 PassPower_ApplyGrotto(u32 chance) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_GROTTO] != PASS_POWER_NONE) {
        int level = g_PassPowerState.values[PASS_POWER_EFFECT_GROTTO];

        if (level < PASS_POWER_GROTTO_LEVELS) {
            chance += PASSPOWER_HOLLOW_CHANCES[level];
            if (chance > 100) {
                chance = 100;
            }
        }
    }
    return chance;
}

u32 PassPower_ApplyGrottoVolume(u32 volume) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_GROTTO] != PASS_POWER_NONE) {
        int level = g_PassPowerState.values[PASS_POWER_EFFECT_GROTTO];

        if (level < PASS_POWER_GROTTO_LEVELS) {
            volume += 2;
        }
    }
    return volume;
}

// The level is the row of the encounter slots' odds
u32 PassPower_ApplyLuckyEncProb(u32 row) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_LUCKY] != PASS_POWER_NONE) {
        int level = g_PassPowerState.values[PASS_POWER_EFFECT_LUCKY];

        if (level < PASS_POWER_LUCKY_LEVELS) {
            row = level;
        }
    }
    return row;
}

// Only the fourth level adds a roll
u32 PassPower_ApplyLuckyShiny(u32 rolls) {
    if (g_PassPowerState.passPowers[PASS_POWER_EFFECT_LUCKY] != PASS_POWER_NONE) {
        int level = g_PassPowerState.values[PASS_POWER_EFFECT_LUCKY];

        if (level < PASS_POWER_LUCKY_LEVELS) {
            if (level == 3) {
                rolls++;
            }
        }
    }
    return rolls;
}
