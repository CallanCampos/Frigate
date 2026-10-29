// The script commands of the Hidden Grottoes: asking what a grotto holds, and the events that take the player in
// and out of one. The ROM has no name for the file; scrcmd_hidden_hollow.c is descriptive. Rolling what a grotto
// holds is hidden_hollow_generate.c, before it. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/directions.h"
#include "constants/flags.h"
#include "constants/obj_codes.h"
#include "constants/species.h"
#include "constants/zones.h"
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
#include "save/misc_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/tpoke_data.h"

#define SLOT_ARC 0x111

// Which of a grotto file's two sets of Pokémon the version reads
#ifdef BLACK2
#define HOLLOW_FILE_VERSION 1
#else
#define HOLLOW_FILE_VERSION 0
#endif

// What Scrcmd_HiddenHollow_HiddenHollowGetParam gives of the grotto's slot: the Pokémon's species, level, form and
// whether it is female, the item in a ball or the hidden item, whether the slot is a hidden item, and the grotto
enum {
    HIDDEN_HOLLOW_PARAM_SPECIES,
    HIDDEN_HOLLOW_PARAM_LEVEL,
    HIDDEN_HOLLOW_PARAM_FORM,
    HIDDEN_HOLLOW_PARAM_FEMALE,
    HIDDEN_HOLLOW_PARAM_ITEM,
    HIDDEN_HOLLOW_PARAM_HIDDEN_ITEM,
    HIDDEN_HOLLOW_PARAM_HAS_HIDDEN_ITEM,
    HIDDEN_HOLLOW_PARAM_HOLLOW,
};

// The steps of the warps into and out of a grotto: fade out, change the map, fade in once its actors are loaded, end
enum {
    HOLLOW_WARP_STATE_FADE_OUT,
    HOLLOW_WARP_STATE_MAP_CHANGE,
    HOLLOW_WARP_STATE_FADE_IN,
    HOLLOW_WARP_STATE_END,
};

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

// A grotto's file: the Pokémon of each version's three slots, White 2's first, then the items in balls of the next
// four slots and the hidden items of the last four
typedef struct {
    HollowSlot slots[2][3];
    u16 items[4][4];
    u16 hiddenItems[4][4];
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

// The level of a Pokémon that a grotto's mission gives, by grotto
static const u8 HOLLOW_MISSION_LEVELS[HIDDEN_HOLLOW_COUNT] = { 15, 25, 25, 30, 30, 35, 40, 40, 40, 50,
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
static void CreateMissionHiddenHollowPokemonActor(HollowEventWork *work);
static void ActivateHiddenHollowMission(HollowEventWork *work);
static u32 GetHiddenHollowMissionLevel(u32 hollow);

BOOL Scrcmd_HiddenHollow_HiddenHollowGetParam(VM *vm, FieldScriptEnv *env) {
    FesGimmick *gimmick;
    Field *field;
    u8 slot;
    RivalDataSave *rival;
    u32 param;
    RivalEntry *entry;
    GameData *gameData;
    u16 *result;
    u32 hollow;
    u8 group;
    BOOL mission;
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
    if (hollowFesMissions(gimmick, hollow) && RivalSelect_IsSelectedAndActive(gimmick, hollow) == TRUE) {
        entry = RivalSelect_GetEntry(gimmick, hollow);
        mission = TRUE;
    }
    switch (param) {
    case HIDDEN_HOLLOW_PARAM_SPECIES:
        if (!mission) {
            GFL_ArcSysRead(&file, SLOT_ARC, hollow);
            *result = getSlotFilePtr(&file, slot)->species[group];
        } else {
            *result = entry->species;
        }
        break;
    case HIDDEN_HOLLOW_PARAM_LEVEL:
        if (!mission) {
            HollowSlot *slotData;
            u8 min;

            GFL_ArcSysRead(&file, SLOT_ARC, hollow);
            slotData = getSlotFilePtr(&file, slot);
            min = slotData->minLevel[group];
            *result = min + GFL_RandomLC((u8)(slotData->maxLevel[group] - min));
        } else {
            *result = GetHiddenHollowMissionLevel(hollow);
        }
        break;
    case HIDDEN_HOLLOW_PARAM_FORM:
        if (!mission) {
            GFL_ArcSysRead(&file, SLOT_ARC, hollow);
            *result = getSlotFilePtr(&file, slot)->form[group];
        } else {
            *result = entry->form;
        }
        break;
    case HIDDEN_HOLLOW_PARAM_FEMALE:
        if (!mission) {
            u32 gender = getHollowGender(rival, hollow);
            BOOL female = FALSE;

            GFL_ArcSysRead(&file, SLOT_ARC, hollow);
            if (gender < getSlotFilePtr(&file, slot)->femaleBelow[group]) {
                female = TRUE;
            }
            *result = female;
        } else {
            *result = entry->female;
        }
        break;
    case HIDDEN_HOLLOW_PARAM_ITEM:
        GFL_ArcSysRead(&file, SLOT_ARC, hollow);
        *result = file.items[(u8)(slot - 3)][group];
        break;
    case HIDDEN_HOLLOW_PARAM_HIDDEN_ITEM:
        GFL_ArcSysRead(&file, SLOT_ARC, hollow);
        *result = file.hiddenItems[(u8)(slot - 7)][group];
        break;
    case HIDDEN_HOLLOW_PARAM_HAS_HIDDEN_ITEM:
        if (!mission) {
            if (getTypeOfFilledSlot(rival, hollow) == HOLLOW_SLOT_TYPE_HIDDEN_ITEM) {
                *result = 1;
            } else {
                *result = 0;
            }
        } else {
            *result = 0;
        }
        break;
    case HIDDEN_HOLLOW_PARAM_HOLLOW:
        *result = getHollowNum(rival);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_HiddenHollow_HiddenHollowReset(VM *vm, FieldScriptEnv *env) {
    GameData *gameData;
    Field *field;
    RivalDataSave *rival;
    u32 hollow;
    BOOL mission;
    FesGimmick *gimmick;

    gameData = FieldScriptEnv_GetGameData(env);
    field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    mission = FALSE;
    rival = getHollow_RivalData(GameData_GetSaveControl(gameData));
    hollow = getHollowNum(rival);
    gimmick = Field_GetFesGimmick(field);
    if (hollowFesMissions(gimmick, hollow) && RivalSelect_IsSelectedAndActive(gimmick, hollow) == TRUE) {
        mission = TRUE;
    }
    if (mission) {
        RivalSelect_SelectOtherRandom(gimmick, hollow);
    } else {
        setHollowStatus(rival, hollow, FALSE);
    }
    return FALSE;
}

BOOL Scrcmd_HiddenHollow_HiddenHollowCallWarpIn(VM *vm, FieldScriptEnv *env) {
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
    case HOLLOW_WARP_STATE_FADE_OUT:
        setHollowNum(work->rival, work->hollow);
        ActivateHiddenHollowMission(work);
        DisableAllActorsMovement(Field_GetActorSystem(work->field));
        next = CallFieldMapEntranceOutTransitionDefault(work->gsys, work->field, FIELD_TRANSITION_WIPE, FALSE);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case HOLLOW_WARP_STATE_MAP_CHANGE:
        CreateHiddenHollowWarpPosVec(&pos, 15, 0, 27);
        next = EventMapChange_CreateGridDefault(work->gsys, work->field, ZONE_HIDDEN_GROTTO, &pos, DIR_UP);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case HOLLOW_WARP_STATE_FADE_IN:
        if (FldActSys_IsAsyncLoadPending(Field_GetActorSystem(work->field)) == TRUE) {
            break;
        }
        next = CallFieldMapEntranceInTransition(work->gsys, work->field, FIELD_TRANSITION_FADE_BLACK, TRUE, TRUE, 0, 0);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case HOLLOW_WARP_STATE_END:
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
    FesGimmick *gimmick = Field_GetFesGimmick(work->field);

    if (!hollowFesMissions(gimmick, work->hollow)) {
        switch (type) {
        case HOLLOW_SLOT_TYPE_POKEMON:
            CreateGenericHiddenHollowPokemonActor(work);
            EventWork_FlagReset(work->eventWork, EVENT_FLAG_0x097f);
            break;
        case HOLLOW_SLOT_TYPE_ITEM:
            CreateHiddenHollowItemActor(work->actors);
            EventWork_FlagReset(work->eventWork, EVENT_FLAG_0x097f);
            break;
        case HOLLOW_SLOT_TYPE_HIDDEN_ITEM:
            EventWork_FlagSet(work->eventWork, EVENT_FLAG_0x097f);
            break;
        }
    } else {
        if (RivalSelect_IsSelectedAndActive(gimmick, work->hollow)) {
            CreateMissionHiddenHollowPokemonActor(work);
        }
        EventWork_FlagReset(work->eventWork, EVENT_FLAG_0x097f);
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
    CreateHiddenHollowPokemonActor(work->actors, slotData->species[group], female, slotData->form[group], FALSE,
                                   HEAPID_FIELDMAP);
}

static void CreateHiddenHollowPokemonActor(MMSys *actors, u16 species, u16 sex, u16 form, u32 moves, u32 heapId) {
    FieldActor *actor;

    if (!isFoongusAmoongus(species)) {
        TPokeData *data = LoadTPokeData(HEAPID_FIELDMAP);
        u16 objCode = GetPokemonFieldOBJCODE(data, species, sex, form);
        FreeTPokeData(data);
        actor = CreateNewActorByParam(actors, 0xf, 0xc, 1, 0xe0, objCode, MOVE_CODE_NONE, ZONE_HIDDEN_GROTTO);
    } else {
        actor = CreateNewActorByParam(actors, 0xf, 0xc, 1, 0xe0, OBJ_CODE_110, MOVE_CODE_NONE, ZONE_HIDDEN_GROTTO);
    }
    SetActorSCRID(actor, 1);
    if (moves == TRUE) {
        SetActorMovementFlag(actor, 1 << 20);
    }
}

static BOOL isFoongusAmoongus(u16 species) {
    if (species != SPECIES_AMOONGUSS && species != SPECIES_FOONGUS) {
        return FALSE;
    }
    return TRUE;
}

static void CreateHiddenHollowItemActor(MMSys *actors) {
    FieldActor *actor =
        CreateNewActorByParam(actors, 0xf, 0xc, 1, 0xe0, OBJ_CODE_110, MOVE_CODE_NONE, ZONE_HIDDEN_GROTTO);

    SetActorSCRID(actor, 2);
}

BOOL Scrcmd_HiddenHollow_HiddenHollowCallWarpOut(VM *vm, FieldScriptEnv *env) {
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
    case HOLLOW_WARP_STATE_FADE_OUT:
        DisableAllActorsMovement(Field_GetActorSystem(work->field));
        next = CallFieldMapEntranceOutTransitionDefault(work->gsys, work->field, FIELD_TRANSITION_FADE_WHITE, TRUE);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case HOLLOW_WARP_STATE_MAP_CHANGE: {
        u16 zone = GetHiddenHollowEntranceParam(work->hollow, 0);
        u32 x = GetHiddenHollowEntranceParam(work->hollow, 1);
        u32 y = GetHiddenHollowEntranceParam(work->hollow, 2);
        u32 z = GetHiddenHollowEntranceParam(work->hollow, 3);

        CreateHiddenHollowWarpPosVec(&pos, x, y, z);
        next = EventMapChange_CreateGridDefault(work->gsys, work->field, zone, &pos, DIR_DOWN);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    }
    case HOLLOW_WARP_STATE_FADE_IN:
        if (FldActSys_IsAsyncLoadPending(Field_GetActorSystem(work->field)) == TRUE) {
            break;
        }
        next = CallFieldMapEntranceInTransition(work->gsys, work->field, FIELD_TRANSITION_FADE_WHITE, TRUE, TRUE, 0, 0);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case HOLLOW_WARP_STATE_END:
        EnableAllActorsMovement(Field_GetActorSystem(work->field));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static HollowSlot *getSlotFilePtr(HollowFile *file, u32 slot) {
    return &file->slots[HOLLOW_FILE_VERSION][slot];
}

BOOL Scrcmd_HiddenHollow_HiddenHollowSet(VM *vm, FieldScriptEnv *env) {
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

static void CreateMissionHiddenHollowPokemonActor(HollowEventWork *work) {
    FesGimmick *gimmick = Field_GetFesGimmick(work->field);
    RivalEntry *entry = RivalSelect_GetEntry(gimmick, work->hollow);

    CreateHiddenHollowPokemonActor(work->actors, entry->species, entry->female, entry->form, TRUE, HEAPID_FIELDMAP);
}

static void ActivateHiddenHollowMission(HollowEventWork *work) {
    FesGimmick *gimmick;

    getTypeOfFilledSlot(work->rival, work->hollow);
    gimmick = Field_GetFesGimmick(work->field);
    if (hollowFesMissions(gimmick, work->hollow)) {
        RivalSelect_SetActive(gimmick, work->hollow);
    }
}

BOOL Scrcmd_HiddenHollow_HiddenHollowCreateEvents(VM *vm, FieldScriptEnv *env) {
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

static u32 GetHiddenHollowMissionLevel(u32 hollow) {
    return HOLLOW_MISSION_LEVELS[hollow];
}
