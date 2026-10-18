// The script commands of the Hidden Grottoes: rolling what a grotto holds, asking for it, and the events that take the
// player in and out of one. The ROM has no name for the file; scrcmd_hidden_hollow.c is descriptive. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/event_mapchange.h"
#include "field/fest_mission_hollow.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/hidden_hollow.h"
#include "field/rival_select.h"
#include "field/scrcmd_hidden_hollow.h"
#include "gfl/arc.h"
#include "gfl/random.h"
#include "save/event_work.h"
#include "save/high_link.h"
#include "save/misc_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/tpoke_data.h"

#define SLOT_ARC 0x111
#define HOLLOW_COUNT 20
#define SLOT_CHANCE_COUNT 10
#define HOLLOW_ZONE 0x206
#define FLAG_HOLLOW_EVENT 0x97f

// Which of a grotto file's two sets of Pokémon the version reads
#ifdef BLACK2
#define HOLLOW_FILE_VERSION 1
#else
#define HOLLOW_FILE_VERSION 0
#endif

// What a grotto holds in each slot, from the grotto's file in the archive
typedef struct {
    u16 species[4];
    u8 maxLevel[4];
    u8 minLevel[4];
    // The gender roll under which the Pokémon is female
    u8 femaleBelow[4];
    u8 form[4];
    u8 unk18[2];
} HollowSlot;

// A grotto's file: the Pokémon of each version's three slots, White 2's first, then the items of the other slots
typedef struct {
    HollowSlot slots[2][3];
    u16 itemsA[4][4];
    u16 itemsB[4][4];
} HollowFile;

// What the events that set a grotto up, and the warp into it, work on
typedef struct {
    GameSystem *gsys;
    Field *field;
    MMSys *actors;
    RivalDataSave *rival;
    EventWork *eventWork;
    u8 hollow;
} HollowEventWork;

typedef struct {
    GameSystem *gsys;
    Field *field;
    RivalDataSave *rival;
    u8 hollow;
} WarpOutWork;

// By grotto, how likely each of the ten slots is, out of 100
static const u8 HOLLOW_SLOT_CHANCES[HOLLOW_COUNT][SLOT_CHANCE_COUNT] = {
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
    { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 }, { 1, 4, 15, 1, 4, 10, 25, 1, 4, 10 },
};

// The level of a Pokémon that a grotto's mission gives, by grotto
static const u8 HOLLOW_MISSION_LEVELS[HOLLOW_COUNT] = { 15, 25, 25, 30, 30, 35, 40, 40, 40, 50,
                                                        45, 55, 40, 40, 60, 60, 60, 60, 60, 60 };

static void CreateHiddenHollowWarpPosVec(VecFx32 *pos, u32 x, u32 y, u32 z);
static void SetupHiddenHollowEvents(HollowEventWork *work);
static void CreateGenericHiddenHollowPokemonActor(HollowEventWork *work);
static void CreateHiddenHollowPokemonActor(MMSys *actors, u16 species, u16 sex, u16 form, u32 moves, u32 heapId);
static BOOL isFoongusAmoongus(u16 species);
static void CreateHiddenHollowItemActor(MMSys *actors);
static GameEventReturnCode EventHiddenHollowWarpIn_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventHiddenHollowWarpOut_Callback(GameEvent *event, u32 *state, void *data);
static HollowSlot *getSlotFilePtr(HollowFile *file, u32 slot);
static void func_ov036_021c9394(HollowEventWork *work);
static void func_ov036_021c93c0(HollowEventWork *work);
static u32 func_ov036_021c9440(u32 hollow);

void generateHollow(RivalDataSave *data, u32 hollow) {
    u8 slot = 10;
    u8 group = GFL_RandomLC(4);
    u32 volume = PassPower_ApplyGrottoVolume(1);
    u32 i = 0;
    int j;
    const u8 *chances;
    u8 roll;
    u8 sum;

    if (volume != 0) {
        do {
            roll = GFL_RandomLC(100) + 1;
            sum = 0;
            chances = HOLLOW_SLOT_CHANCES[hollow];

            for (j = 0; j < SLOT_CHANCE_COUNT; j++) {
                sum += chances[j];
                if (roll <= sum) {
                    slot = j;
                    break;
                }
            }
            if (slot <= 2) {
                break;
            }
        } while (++i < volume);
    }
    setSlot(data, hollow, slot);
    setGroup(data, hollow, group);
    setHollowGender(data, hollow, (u8)GFL_RandomLC(100));
    setHollowStatus(data, hollow, TRUE);
}

BOOL s0294_HiddenHollowGetParam(VM *vm, FieldScriptEnv *env) {
    void *gimmick;
    Field *field;
    u8 slot;
    RivalDataSave *rival;
    u32 param;
    RivalEntry *entry;
    GameData *gameData;
    u16 *result;
    BOOL mission;
    u8 group;
    u8 hollow;
    HollowFile file;

    gameData = FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    param = ScriptReadAny(vm, env);
    result = ScriptReadVar(vm, env);
    mission = FALSE;
    rival = getHollow_RivalData(GameData_GetSaveControl(gameData));
    hollow = getHollowNum(rival);
    slot = getSlotNumber(rival, hollow);
    group = getGroupNumber(rival, hollow);
    gimmick = Field_GetFesGimmick(field);
    if (hollowFesMissions(gimmick, hollow) && func_ov073_021e8cac(gimmick, hollow) == 1) {
        entry = func_ov073_021e8be0(gimmick, hollow);
        mission = TRUE;
    }
    switch (param) {
    case 0:
        if (!mission) {
            GFL_ArcSysRead(&file, SLOT_ARC, hollow);
            *result = getSlotFilePtr(&file, slot)->species[group];
        } else {
            *result = entry->unk4;
        }
        break;
    case 1:
        if (!mission) {
            HollowSlot *slotData;
            u8 min;

            GFL_ArcSysRead(&file, SLOT_ARC, hollow);
            slotData = getSlotFilePtr(&file, slot);
            min = slotData->minLevel[group];
            *result = min + GFL_RandomLC((u8)(slotData->maxLevel[group] - min));
        } else {
            *result = func_ov036_021c9440(hollow);
        }
        break;
    case 2:
        if (!mission) {
            GFL_ArcSysRead(&file, SLOT_ARC, hollow);
            *result = getSlotFilePtr(&file, slot)->form[group];
        } else {
            *result = entry->unk6;
        }
        break;
    case 3:
        if (!mission) {
            u32 gender = getHollowGender(rival, hollow);
            BOOL female = FALSE;

            GFL_ArcSysRead(&file, SLOT_ARC, hollow);
            if (gender < getSlotFilePtr(&file, slot)->femaleBelow[group]) {
                female = TRUE;
            }
            *result = female;
        } else {
            *result = entry->unk1;
        }
        break;
    case 4:
        GFL_ArcSysRead(&file, SLOT_ARC, hollow);
        *result = file.itemsA[(u8)(slot - 3)][group];
        break;
    case 5:
        GFL_ArcSysRead(&file, SLOT_ARC, hollow);
        *result = file.itemsB[(u8)(slot - 7)][group];
        break;
    case 6:
        if (!mission) {
            if (getTypeOfFilledSlot(rival, hollow) == 3) {
                *result = 1;
            } else {
                *result = 0;
            }
        } else {
            *result = 0;
        }
        break;
    case 7:
        *result = getHollowNum(rival);
        break;
    }
    return FALSE;
}

BOOL s0295_HiddenHollowReset(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    BOOL mission = FALSE;
    RivalDataSave *rival = getHollow_RivalData(GameData_GetSaveControl(gameData));
    u8 hollow = getHollowNum(rival);
    void *gimmick = Field_GetFesGimmick(field);

    if (hollowFesMissions(gimmick, hollow) && func_ov073_021e8cac(gimmick, hollow) == 1) {
        mission = TRUE;
    }
    if (mission) {
        func_ov073_021e8bfc(gimmick, hollow);
    } else {
        setHollowStatus(rival, hollow, FALSE);
    }
    return FALSE;
}

BOOL s0293_HiddenHollowCallWarpIn(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    ScriptWork *scriptWork = FieldScriptEnv_GetScriptWork(env);
    u8 hollow = ScriptReadAny(vm, env);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventHiddenHollowWarpIn_Callback, sizeof(HollowEventWork));
    HollowEventWork *work;
    SaveControl *save;

    ScriptWork_CallEvent(scriptWork, event);
    work = GameEvent_GetData(event);
    save = GameData_GetSaveControl(gameData);
    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->actors = Field_GetActorSystem(work->field);
    work->rival = getHollow_RivalData(save);
    work->hollow = hollow;
    work->eventWork = GameData_GetEventWork(gameData);
    return TRUE;
}

static GameEventReturnCode EventHiddenHollowWarpIn_Callback(GameEvent *event, u32 *state, void *data) {
    HollowEventWork *work = data;
    GameEvent *next;
    VecFx32 pos;

    switch (*state) {
    case 0:
        setHollowNum(work->rival, work->hollow);
        func_ov036_021c93c0(work);
        DisableAllActorsMovement(Field_GetActorSystem(work->field));
        next = CallFieldMapEntranceOutTransitionDefault(work->gsys, work->field, 4, 0);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case 1:
        CreateHiddenHollowWarpPosVec(&pos, 15, 0, 27);
        next = EventMapChange_CreateGridDefault(work->gsys, work->field, HOLLOW_ZONE, &pos, 0);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case 2:
        if (FldActSys_IsAsyncLoadPending(Field_GetActorSystem(work->field)) == 1) {
            break;
        }
        next = CallFieldMapEntranceInTransition(work->gsys, work->field, 0, 1, 1, 0, 0);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case 3:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void CreateHiddenHollowWarpPosVec(VecFx32 *pos, u32 x, u32 y, u32 z) {
    pos->x = (x * 16 + 8) << FX32_SHIFT;
    pos->y = (y * 16 + 8) << FX32_SHIFT;
    pos->z = (z * 16 + 8) << FX32_SHIFT;
}

static void SetupHiddenHollowEvents(HollowEventWork *work) {
    u32 type = getTypeOfFilledSlot(work->rival, work->hollow);
    void *gimmick = Field_GetFesGimmick(work->field);

    if (!hollowFesMissions(gimmick, work->hollow)) {
        switch (type) {
        case 1:
            CreateGenericHiddenHollowPokemonActor(work);
            EventWork_FlagReset(work->eventWork, FLAG_HOLLOW_EVENT);
            break;
        case 2:
            CreateHiddenHollowItemActor(work->actors);
            EventWork_FlagReset(work->eventWork, FLAG_HOLLOW_EVENT);
            break;
        case 3:
            EventWork_FlagSet(work->eventWork, FLAG_HOLLOW_EVENT);
            break;
        }
    } else {
        if (func_ov073_021e8cac(gimmick, work->hollow)) {
            func_ov036_021c9394(work);
        }
        EventWork_FlagReset(work->eventWork, FLAG_HOLLOW_EVENT);
    }
}

static void CreateGenericHiddenHollowPokemonActor(HollowEventWork *work) {
    u8 slot = getSlotNumber(work->rival, work->hollow);
    u8 group = getGroupNumber(work->rival, work->hollow);
    u32 gender = getHollowGender(work->rival, work->hollow);
    HollowFile file;
    HollowSlot *slotData;
    u16 female = FALSE;

    GFL_ArcSysRead(&file, SLOT_ARC, work->hollow);
    slotData = getSlotFilePtr(&file, slot);
    if (gender < slotData->femaleBelow[group]) {
        female = TRUE;
    }
    CreateHiddenHollowPokemonActor(work->actors, slotData->species[group], female, slotData->form[group], 0, 0x15);
}

static void CreateHiddenHollowPokemonActor(MMSys *actors, u16 species, u16 sex, u16 form, u32 moves, u32 heapId) {
    FieldActor *actor;

    if (!isFoongusAmoongus(species)) {
        TPokeData *data = LoadTPokeData(0x15);
        u16 objCode = GetPokemonFieldOBJCODE(data, species, sex, form);
        FreeTPokeData(data);
        actor = CreateNewActorByParam(actors, 0xf, 0xc, 1, 0xe0, objCode, 0, HOLLOW_ZONE);
    } else {
        actor = CreateNewActorByParam(actors, 0xf, 0xc, 1, 0xe0, 0x6e, 0, HOLLOW_ZONE);
    }
    SetActorSCRID(actor, 1);
    if (moves == 1) {
        SetActorMovementFlag(actor, 1 << 20);
    }
}

static BOOL isFoongusAmoongus(u16 species) {
    if (species != 0x24f && species != 0x24e) {
        return FALSE;
    }
    return TRUE;
}

static void CreateHiddenHollowItemActor(MMSys *actors) {
    FieldActor *actor = CreateNewActorByParam(actors, 0xf, 0xc, 1, 0xe0, 0x6e, 0, HOLLOW_ZONE);

    SetActorSCRID(actor, 2);
}

BOOL s0296_HiddenHollowCallWarpOut(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    ScriptWork *scriptWork = FieldScriptEnv_GetScriptWork(env);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventHiddenHollowWarpOut_Callback, sizeof(WarpOutWork));
    WarpOutWork *work;
    SaveControl *save;

    ScriptWork_CallEvent(scriptWork, event);
    work = GameEvent_GetData(event);
    save = GameData_GetSaveControl(gameData);
    work->gsys = gsys;
    work->field = GSYS_GetField(gsys);
    work->rival = getHollow_RivalData(save);
    work->hollow = getHollowNum(work->rival);
    return TRUE;
}

static GameEventReturnCode EventHiddenHollowWarpOut_Callback(GameEvent *event, u32 *state, void *data) {
    WarpOutWork *work = data;
    GameEvent *next;
    VecFx32 pos;

    switch (*state) {
    case 0:
        DisableAllActorsMovement(Field_GetActorSystem(work->field));
        next = CallFieldMapEntranceOutTransitionDefault(work->gsys, work->field, 1, 1);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case 1: {
        u16 zone = GetHiddenHollowEntranceParam(work->hollow, 0);
        u32 x = GetHiddenHollowEntranceParam(work->hollow, 1);
        u32 y = GetHiddenHollowEntranceParam(work->hollow, 2);
        u32 z = GetHiddenHollowEntranceParam(work->hollow, 3);

        CreateHiddenHollowWarpPosVec(&pos, x, y, z);
        next = EventMapChange_CreateGridDefault(work->gsys, work->field, zone, &pos, 1);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    }
    case 2:
        if (FldActSys_IsAsyncLoadPending(Field_GetActorSystem(work->field)) == 1) {
            break;
        }
        next = CallFieldMapEntranceInTransition(work->gsys, work->field, 1, 1, 1, 0, 0);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case 3:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static HollowSlot *getSlotFilePtr(HollowFile *file, u32 slot) {
    return &file->slots[HOLLOW_FILE_VERSION][slot];
}

BOOL s02E5_HiddenHollowSet(VM *vm, FieldScriptEnv *env) {
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u8 hollow = ScriptReadAny(vm, env);
    u16 slot = ScriptReadAny(vm, env);
    u16 group = ScriptReadAny(vm, env);
    u8 male = ScriptReadAny(vm, env);
    RivalDataSave *rival = getHollow_RivalData(GameData_GetSaveControl(gameData));

    setSlot(rival, hollow, (u8)slot);
    setGroup(rival, hollow, (u8)group);
    setHollowStatus(rival, hollow, TRUE);
    if (male == 1) {
        setHollowGender(rival, hollow, 0);
    } else {
        setHollowGender(rival, hollow, 100);
    }
    return FALSE;
}

static void func_ov036_021c9394(HollowEventWork *work) {
    void *gimmick = Field_GetFesGimmick(work->field);
    RivalEntry *entry = func_ov073_021e8be0(gimmick, work->hollow);

    CreateHiddenHollowPokemonActor(work->actors, entry->unk4, entry->unk1, entry->unk6, 1, 0x15);
}

static void func_ov036_021c93c0(HollowEventWork *work) {
    void *gimmick;

    getTypeOfFilledSlot(work->rival, work->hollow);
    gimmick = Field_GetFesGimmick(work->field);
    if (hollowFesMissions(gimmick, work->hollow)) {
        func_ov073_021e8cdc(gimmick, work->hollow);
    }
}

BOOL s02E6_HiddenHollowCreateEvents(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    RivalDataSave *rival;
    u8 hollow;
    HollowEventWork work;

    FieldScriptEnv_GetScriptWork(env);
    rival = getHollow_RivalData(GameData_GetSaveControl(gameData));
    hollow = getHollowNum(rival);
    work.gsys = gsys;
    work.field = GSYS_GetField(gsys);
    work.actors = Field_GetActorSystem(work.field);
    work.rival = rival;
    work.hollow = hollow;
    work.eventWork = GameData_GetEventWork(gameData);
    SetupHiddenHollowEvents(&work);
    return FALSE;
}

static u32 func_ov036_021c9440(u32 hollow) {
    return HOLLOW_MISSION_LEVELS[hollow];
}
