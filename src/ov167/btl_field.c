#include "types.h"
#include "battle/btl_field.h"
#include "battle/btl_main.h"
#include "battle/btl_ov169.h"
#include "battle/btl_pokeparam.h"
#include "gfl/heap.h"

// Overlay 167's btl_field.c (named by its string): the weather and the effects of the whole field, such as Trick
// Room and Gravity. The battle server's flow uses the static copy here; the main module allocates another for the
// clients. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0) where it has them.

#define FIELD_EFFECT_COUNT 8
// Effect 3 is Imprison, whose user is the first of its depending Pokemon
#define FIELD_EFFECT_IMPRISON 3
#define FIELD_DEPEND_POKE_MAX 6
#define NO_MON 31
#define WEATHER_TURNS_PERMANENT 0xff

struct BtlField {
    u32 weather;
    u32 weatherTurns;
    // The effect's event handlers, from overlay 169
    void *events[FIELD_EFFECT_COUNT];
    BattleCondition conditions[FIELD_EFFECT_COUNT];
    u32 turnCounts[FIELD_EFFECT_COUNT];
    // The Pokemon the effect lasts for
    u32 dependPokes[FIELD_EFFECT_COUNT][FIELD_DEPEND_POKE_MAX];
    u32 dependPokeCounts[FIELD_EFFECT_COUNT];
    u32 active[FIELD_EFFECT_COUNT];
};

static void clearEffect(BtlField *field, u32 effect);
static void initField(BtlField *field, u32 weather);
static u32 getWeatherTurns(BtlField *field);
static u8 decWeatherTurns(BtlField *field);

static BtlField sField;

static void clearEffect(BtlField *field, u32 effect) {
    u32 i;

    field->events[effect] = NULL;
    field->conditions[effect] = ZeroConditionTurns();
    field->turnCounts[effect] = 0;
    field->dependPokeCounts[effect] = 0;
    field->active[effect] = 0;
    for (i = 0; i < FIELD_DEPEND_POKE_MAX; i++) {
        field->dependPokes[effect][i] = NO_MON;
    }
}

void func_ov167_021d59a0(u32 weather) {
    initField(&sField, weather);
}

// Function name from swan.
u8 GetFieldWeather(void) {
    return func_ov167_021d5ad4(&sField);
}

u32 func_ov167_021d59c0(void) {
    return getWeatherTurns(&sField);
}

// Function name from swan.
void FieldStatusSetWeather(u8 weather, u8 duration) {
    func_ov167_021d5aec(&sField, weather, duration);
}

u8 func_ov167_021d59e4(void) {
    return decWeatherTurns(&sField);
}

// Function name from swan.
BOOL FieldStatusAddEffect(u32 effect, BattleCondition value) {
    return FieldStatusaddEffectCore(&sField, effect, value, TRUE);
}

// Function name from swan.
BOOL FieldStatusRemoveEffect(u32 effect) {
    return func_ov167_021d5bc0(&sField, effect);
}

// Function name from swan.
BOOL FieldStatusAddDependPoke(u32 effect, u8 monId) {
    return func_ov167_021d5c04(&sField, effect, monId);
}

void func_ov167_021d5a38(u8 monId) {
    func_ov167_021d5c60(&sField, monId);
}

BOOL func_ov167_021d5a48(BtlPokeCon *pokeCon, BattleMon *mon, u16 move) {
    return CheckImprison(&sField, pokeCon, mon, move);
}

void func_ov167_021d5a60(void (*callback)(u32 effect, BtlServerFlow *flow), BtlServerFlow *flow) {
    func_ov167_021d5da4(&sField, callback, flow);
}

// Function name from swan.
u32 IsFieldEffectActive(u32 fieldEffect) {
    return CheckFieldEffect(&sField, fieldEffect);
}

BtlField *func_ov167_021d5a84(HeapID heapId) {
    BtlField *field = GFL_HeapAllocate(heapId, sizeof(BtlField), TRUE, "btl_field.c", 0x10c);

    initField(field, 0);
    return field;
}

void func_ov167_021d5aac(BtlField *field) {
    GFL_HeapFree(field);
}

static void initField(BtlField *field, u32 weather) {
    u32 i;

    for (i = 0; i < FIELD_EFFECT_COUNT; i++) {
        clearEffect(field, i);
    }
    field->weather = weather;
    field->weatherTurns = WEATHER_TURNS_PERMANENT;
}

u8 func_ov167_021d5ad4(BtlField *field) {
    return field->weather;
}

static u32 getWeatherTurns(BtlField *field) {
    if (field->weather != 0) {
        return field->weatherTurns;
    }
    return 0;
}

void func_ov167_021d5aec(BtlField *field, u8 weather, u16 turns) {
    field->weather = weather;
    field->weatherTurns = turns;
}

void func_ov167_021d5af4(BtlField *field) {
    field->weather = 0;
    field->weatherTurns = 0;
}

// Counts down a weather that lasts some turns, and returns the weather that ended, or 0
static u8 decWeatherTurns(BtlField *field) {
    if (field->weather != 0 && field->weatherTurns != WEATHER_TURNS_PERMANENT) {
        if (--field->weatherTurns == 0) {
            u8 weather = field->weather;

            field->weather = 0;
            return weather;
        }
    }
    return 0;
}

// Function name from swan.
BOOL FieldStatusaddEffectCore(BtlField *field, u32 effect, BattleCondition value, BOOL addEvent) {
    u32 i;
    u8 monId;

    if (field->active[effect] == 0) {
        if (addEvent) {
            field->events[effect] = FieldEffectEventAdd(effect, 0);
            if (field->events[effect] == NULL) {
                return FALSE;
            }
        }
        field->active[effect] = 1;
        field->conditions[effect] = value;
        field->turnCounts[effect] = 0;
        field->dependPokeCounts[effect] = 0;
        for (i = 0; i < FIELD_DEPEND_POKE_MAX; i++) {
            field->dependPokes[effect][i] = NO_MON;
        }
        monId = Condition_GetMonID(value);
        if (monId != NO_MON) {
            func_ov167_021d5c04(field, effect, monId);
        }
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov167_021d5bc0(BtlField *field, u32 effect) {
    if (CheckFieldEffect(field, effect)) {
        if (field->events[effect] != NULL) {
            func_ov169_06898080(field->events[effect]);
            field->events[effect] = NULL;
        }
        clearEffect(field, effect);
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov167_021d5c04(BtlField *field, u32 effect, u8 monId) {
    u32 count;
    u32 i;

    if (CheckFieldEffect(field, effect)) {
        count = field->dependPokeCounts[effect];
        if (count < FIELD_DEPEND_POKE_MAX) {
            for (i = 0; i < count; i++) {
                if (field->dependPokes[effect][i] == monId) {
                    return FALSE;
                }
            }
            field->dependPokes[effect][count] = monId;
            field->dependPokeCounts[effect] = count + 1;
            return TRUE;
        }
    }
    return FALSE;
}

// Removes a Pokemon from every effect that depends on it, ending the effects that no other Pokemon keeps up
void func_ov167_021d5c60(BtlField *field, u8 monId) {
    u32 effect;
    u32 i;
    BOOL found;

    for (effect = 0; effect < FIELD_EFFECT_COUNT; effect++) {
        if (CheckFieldEffect(field, effect) && field->dependPokeCounts[effect] != 0) {
            found = FALSE;
            for (i = 0; i < field->dependPokeCounts[effect]; i++) {
                if (field->dependPokes[effect][i] == monId) {
                    found = TRUE;
                    break;
                }
            }
            if (found) {
                for (; i < FIELD_DEPEND_POKE_MAX - 1; i++) {
                    field->dependPokes[effect][i] = field->dependPokes[effect][i + 1];
                }
                field->dependPokes[effect][i] = NO_MON;
                if (--field->dependPokeCounts[effect] == 0) {
                    if (field->events[effect] != NULL) {
                        func_ov169_06898080(field->events[effect]);
                        field->events[effect] = NULL;
                    }
                    clearEffect(field, effect);
                } else if (monId == Condition_GetMonID(field->conditions[effect])) {
                    func_ov167_021ce308(&field->conditions[effect], (u8)field->dependPokes[effect][0]);
                }
            }
        }
    }
}

// Function name from swan.
BOOL CheckImprison(BtlField *field, BtlPokeCon *pokeCon, BattleMon *mon, u16 move) {
    u8 attackerId = GetMonID(mon);
    u8 monId;
    u32 count = field->dependPokeCounts[FIELD_EFFECT_IMPRISON];
    u32 i;

    for (i = 0; i < count; i++) {
        monId = field->dependPokes[FIELD_EFFECT_IMPRISON][i];
        if (!IsAllyMonID(attackerId, monId) && MoveIsUsable(GetPokeParamConst(pokeCon, monId), move)) {
            return TRUE;
        }
    }
    return FALSE;
}

// Counts the turns of the timed effects, ending those whose turns ran out
void func_ov167_021d5da4(BtlField *field, void (*callback)(u32 effect, BtlServerFlow *flow), BtlServerFlow *flow) {
    u32 effect;
    u8 turns;

    for (effect = 1; effect < FIELD_EFFECT_COUNT; effect++) {
        if (CheckFieldEffect(field, effect)) {
            turns = func_ov167_021ce33c(field->conditions[effect]);
            if (turns != 0 && ++field->turnCounts[effect] >= turns) {
                if (field->events[effect] != NULL) {
                    func_ov169_06898080(field->events[effect]);
                    field->events[effect] = NULL;
                }
                clearEffect(field, effect);
                if (callback != NULL) {
                    callback(effect, flow);
                }
            }
        }
    }
}

// Function name from swan.
BOOL CheckFieldEffect(BtlField *field, u32 effect) {
    return field->active[effect];
}
