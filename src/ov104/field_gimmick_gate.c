#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/encounter.h"
#include "field/field.h"
#include "field/field_environment.h"
#include "field/field_exp_obj.h"
#include "field/field_gimmick_gate.h"
#include "field/field_map.h"
#include "field/field_script.h"
#include "field/gimmick_obj_elboard.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/calctool.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/config.h"
#include "save/event_work.h"
#include "save/hall_of_fame.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/rtc.h"
#include "system/season.h"
#include "system/version.h"
#include "system/wordset.h"
#include "text/system/gym_insect.h"

struct GimmickGateMessageList {
    u8 count;
    u8 padding[3];
    u32 kinds[7];
    u32 flags[7];
};

struct GimmickGateSave {
    u32 value;
    u32 boardTime;
    u16 flag;
    u16 padding;
    GimmickGateMessageList messages;
};

struct GimmickGateZoneList {
    u16 zones[4];
    u8 weather[4];
};

struct GimmickGateWork {
    u16 heapId;
    u16 pad02;
    Field *field;
    GameData *gameData;
    GameSystem *gameSystem;
    u32 value;
    Elboard *board;
    u32 boardTime;
    GimmickGateZoneData *zoneData;
    GimmickGateBoardEntry *boardEntries;
    u8 boardEntryCount;
    u8 unk25[3];
    GimmickGateMessageList messages;
    u16 flag;
};

void GimmickGate_SaveWork(GimmickGateWork *work);
void GimmickGate_LoadWork(GimmickGateWork *work);
void GimmickGate_ApplyTrainerCardFlag(GimmickGateWork *work);
void GimmickGate_RestoreEventFlags(GimmickGateWork *work);
void GimmickGate_RestoreBoardTime(GimmickGateWork *work);
GimmickGateWork *GimmickGate_CreateWork(Field *field);
void GimmickGate_InitFlag(GimmickGateWork *work);
void GimmickGate_FreeZoneData(GimmickGateWork *work);
void GimmickGate_StartBoardAnimation(GimmickGateWork *work);
void GimmickGate_PlaceBoard(GimmickGateWork *work);
void GimmickGate_AddMessage(GimmickGateWork *work, u32 kind, u32 flag);
u32 GimmickGate_ShouldShowHallOfFame(GimmickGateWork *work);
u32 GimmickGate_IsHallOfFameFlagSet(GimmickGateWork *work);
void GimmickGate_GetHallOfFameSpecies(const GimmickGateWork *work, u32 *species, s32 *count);
void GimmickGate_LoadBoardEntries(GimmickGateWork *work);
void GimmickGate_FreeBoardEntries(GimmickGateWork *work);
GimmickGateBoardEntry *GimmickGate_FindBoardEntry(GimmickGateWork *work);
GimmickGateBoardEntry *GimmickGate_FindListEntry(GimmickGateWork *work);
u32 GimmickGate_HasBoardEntry(GimmickGateWork *work);
void GimmickGate_AddBoardMessage(GimmickGateWork *work, ElboardMessageArg *message, u32 kind,
                                 GimmickGateBoardEntry *entry);
void GimmickGate_SetupMessages(GimmickGateWork *work);
void GimmickGate_SetupDefaultMessages(GimmickGateWork *work);
void GimmickGate_SetupHallOfFameMessages(GimmickGateWork *work);
void GimmickGate_SetupEntryMessages(GimmickGateWork *work);
void GimmickGate_AddDateMessage(GimmickGateWork *work);
void GimmickGate_AddWeatherMessage(GimmickGateWork *work);
void GimmickGate_AddSwarmMessage(GimmickGateWork *work);
void GimmickGate_AddWeekdayMessage3(GimmickGateWork *work);
void GimmickGate_AddWeekdayMessage4(GimmickGateWork *work);
void GimmickGate_AddWeekdayMessage5(GimmickGateWork *work);
void GimmickGate_AddWeekdayMessage6(GimmickGateWork *work);
void GimmickGate_AddHallOfFameMessage(GimmickGateWork *work);
void GimmickGate_AddEntryMessages(GimmickGateWork *work);
void GimmickGate_AddEntryMessage(GimmickGateWork *work, GimmickGateBoardEntry *entry, u32 index);
void GimmickGate_AddListEntryMessages(GimmickGateWork *work);
s32 GimmickGateZoneList_Count(const GimmickGateZoneList *list);
void GimmickGateZoneList_Clear(GimmickGateZoneList *list);
void GimmickGate_GetWeatherZones(GimmickGateWork *work, GimmickGateZoneList *list);
void GimmickGate_GetReportedWeatherZones(GimmickGateWork *work, GimmickGateZoneList *list);
void GimmickGate_GetRoamerZones(GimmickGateWork *work, GimmickGateZoneList *list);
void GimmickGate_FreeWork(GimmickGateWork *work);
void GimmickGate_LoadZoneData(GimmickGateWork *work);
GimmickGateSave *GimmickGate_GetSave(Field *field);

// The board's scenes
typedef enum GimmickGateScene {
    GATE_SCENE_0,
    GATE_SCENE_1,
    GATE_SCENE_COUNT,
} GimmickGateScene;

// The board advances by this each frame
static fx32 sBoardStep = FX32_ONE;

// clang-format off
static const G3DSceneAnimationSetup sScene0Animations[] = {
    { 2, 0 }, { 3, 0 }, { 4, 0 }, { 5, 0 }, { 6, 0 }, { 7, 0 }, { 8, 0 }, { 17, 0 },
    { 18, 0 }, { 19, 0 }, { 20, 0 }, { 21, 0 }, { 22, 0 }, { 23, 0 }, { 24, 0 }, { 25, 0 },
    { 26, 0 }, { 27, 0 }, { 28, 0 }, { 16, 0 }, { 29, 0 }, { 30, 0 },
};

// clang-format on

// clang-format off
static const G3DSceneAnimationSetup sScene1Animations[] = {
    { 9, 0 }, { 10, 0 }, { 11, 0 }, { 12, 0 }, { 13, 0 }, { 14, 0 }, { 15, 0 }, { 17, 0 },
    { 18, 0 }, { 19, 0 }, { 20, 0 }, { 21, 0 }, { 22, 0 }, { 23, 0 }, { 24, 0 }, { 25, 0 },
    { 26, 0 }, { 27, 0 }, { 28, 0 }, { 16, 0 }, { 29, 0 }, { 30, 0 },
};

// clang-format on

static const G3DSceneActorSetup sScene0Actors[] = {
    { 0, 0, 1, 0, sScene0Animations, NELEMS(sScene0Animations) },
};

static const G3DSceneActorSetup sScene1Actors[] = {
    { 0, 0, 1, 0, sScene1Animations, NELEMS(sScene1Animations) },
};

// clang-format off
static const G3DSceneResourceSetup sSceneResources[] = {
    { 0x83, 0, 0 }, { 0x83, 1, 0 }, { 0x83, 2, 0 }, { 0x83, 3, 0 },
    { 0x83, 4, 0 }, { 0x83, 5, 0 }, { 0x83, 6, 0 }, { 0x83, 7, 0 },
    { 0x83, 8, 0 }, { 0x83, 9, 0 }, { 0x83, 10, 0 }, { 0x83, 11, 0 },
    { 0x83, 12, 0 }, { 0x83, 13, 0 }, { 0x83, 14, 0 }, { 0x83, 15, 0 },
    { 0x83, 16, 0 }, { 0x83, 17, 0 }, { 0x83, 18, 0 }, { 0x83, 19, 0 },
    { 0x83, 20, 0 }, { 0x83, 21, 0 }, { 0x83, 24, 0 }, { 0x83, 25, 0 },
    { 0x83, 26, 0 }, { 0x83, 27, 0 }, { 0x83, 28, 0 }, { 0x83, 29, 0 },
    { 0x83, 30, 0 }, { 0x83, 31, 0 }, { 0x83, 32, 0 },
};

// clang-format on

static const G3DSceneSetup sSceneSetups[GATE_SCENE_COUNT] = {
    { sSceneResources, NELEMS(sSceneResources), sScene0Actors, NELEMS(sScene0Actors) },
    { sSceneResources, NELEMS(sSceneResources), sScene1Actors, NELEMS(sScene1Actors) },
};

static const char *sBoardNames[7] = {
    "gelboard_1", "gelboard_2", "gelboard_3", "gelboard_4", "gelboard_5", "gelboard_6", "gelboard_7",
};
static const char *sBoardPlNames[7] = {
    "gelboard_1_pl", "gelboard_2_pl", "gelboard_3_pl", "gelboard_4_pl",
    "gelboard_5_pl", "gelboard_6_pl", "gelboard_7_pl",
};

static u16 sMessageKinds[8] = { 0, 1, 2, 3, 4, 5, 6 };
static u16 sBoardAnimations[16] = { 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 };

// The zones whose weather the boards report, and the weathers they report
static const u16 sWeatherZones[] = { 0x71 };
static const u8 sReportedWeathers[] = { 6, 7, 8 };

static const u32 sEntrySlots[3] = { 3, 4, 5 };

// Messages for the weathers, as GimmickGateZoneList gives them
static u32 sWeatherMessages[15] = {
    0xb5, 0xb6, 0xb7, 0xb8, 0xba, 0xbb, 0xbe, 0xbf, 0xbd, 0xbc, 0xbc, 0xbc, 0xb9, 0xbc, 0xbc,
};

void GimmickGate_Setup(Field *field) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    GimmickGateWork *work;
    GimmickGateScene scene;

    Config_GetKanji(
        (Config *)getTrainerDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(Field_GetGameSystem(field)))));
    scene = GATE_SCENE_1;
    // Loaded as the expansion objects' scene 1, where the board's functions find it
    LoadFieldExpandObjData(expObj, &sSceneSetups[scene], 1);
    work = GimmickGate_CreateWork(field);
    GimmickGate_LoadZoneData(work);
    GimmickGate_LoadBoardEntries(work);
    GimmickGate_LoadWork(work);
    if (work->value == 0) {
        GimmickGate_InitFlag(work);
    } else {
        GimmickGate_ApplyTrainerCardFlag(work);
        GimmickGate_RestoreEventFlags(work);
    }
    GimmickGate_SetupMessages(work);
    GimmickGate_RestoreBoardTime(work);
    GimmickGate_PlaceBoard(work);
    GimmickGate_StartBoardAnimation(work);
}

void GimmickGate_Delete(Field *field) {
    GimmickGateWork *work;

    work = Field_GetGimmickWorkBlock(field, 0);
    GimmickGate_SaveWork(work);
    if (work->board != 0) {
        Elboard_Free(work->board);
    }
    GimmickGate_FreeWork(work);
}

void GimmickGate_Update(Field *field) {
    GimmickGateWork *work;

    work = Field_GetGimmickWorkBlock(field, 0);
    Elboard_Update(work->board, sBoardStep);
    FieldExpObj_StepAllAnimations(Field_GetExpObjSystem(field));
}

u32 GimmickGate_GetDirection(Field *field) {
    GimmickGateWork *work;
    GimmickGateZoneData *zoneData;

    work = Field_GetGimmickWorkBlock(field, 0);
    zoneData = work->zoneData;
    return (u8)zoneData->direction;
}

GimmickGateSave *GimmickGate_GetSave(Field *field) {
    GimmickState *state;
    u32 id;

    state = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    id = GimmickState_GetID(state);
    return GimmickState_GetUserData(state, id);
}

void GimmickGate_SaveWork(GimmickGateWork *work) {
    GimmickGateSave *data;

    data = GimmickGate_GetSave(work->field);
    data->value = work->value;
    data->boardTime = Elboard_GetTime(work->board) >> 12;
    data->flag = work->flag;
    data->messages = work->messages;
}

void GimmickGate_LoadWork(GimmickGateWork *work) {
    GimmickGateSave *data;

    data = GimmickGate_GetSave(work->field);
    work->value = data->value;
    work->boardTime = data->boardTime;
    work->flag = data->flag;
}

void GimmickGate_ApplyTrainerCardFlag(GimmickGateWork *work) {
    SaveControl *save;
    TrainerCardSave *card;

    save = GameData_GetSaveControl(work->gameData);
    card = getTrainerGameInfoAddress(save);
    TrainerCard_SetHallOfFameMinutes(card, work->flag);
}

void GimmickGate_RestoreEventFlags(GimmickGateWork *work) {
    EventWork *eventWork;
    s32 i;
    GimmickGateMessageList *list;

    eventWork = GameData_GetEventWork(work->gameData);
    list = &GimmickGate_GetSave(work->field)->messages;
    for (i = 0; i < list->count; i++) {
        if (list->kinds[i] == 8) {
            EventWork_FlagSet(eventWork, list->flags[i]);
        }
    }
}

void GimmickGate_RestoreBoardTime(GimmickGateWork *work) {
    Elboard_SetTime(work->board, work->boardTime << 12);
}

GimmickGateWork *GimmickGate_CreateWork(Field *field) {
    u16 heapId;
    FieldExpObjSystem *system;
    GimmickGateWork *work;
    ElboardInit init;

    heapId = Field_GetHeapID(field);
    system = Field_GetExpObjSystem(field);
    work = Field_AllocGimmickWorkBlock(field, 0, heapId, sizeof(GimmickGateWork));
    MI_CpuFill8(work, 0, sizeof(GimmickGateWork));
    work->heapId = heapId;
    work->field = field;
    work->gameSystem = Field_GetGameSystem(field);
    work->gameData = GSYS_GetGameData(work->gameSystem);
    work->value = 0;
    init.heapId = heapId;
    init.capacity = 7;
    init.unk03 = 8;
    init.unk04 = 3;
    init.actor = FieldExpObj_GetActor(system, 1, 0);
    work->board = Elboard_Create(&init);
    return work;
}

void GimmickGate_FreeWork(GimmickGateWork *work) {
    if (work != 0) {
        GimmickGate_FreeZoneData(work);
        GimmickGate_FreeBoardEntries(work);
        Field_DeleteGimmickWorkBlock(work->field, 0);
    }
}

void GimmickGate_InitFlag(GimmickGateWork *work) {
    SaveControl *save;
    TrainerCardSave *card;

    if (work->value == 0) {
        save = GameData_GetSaveControl_(work->gameData);
        card = getTrainerCardData_wrapper(save);
        work->flag = TrainerCard_GetHallOfFameMinutes(card);
        work->value = 1;
    }
}

void GimmickGate_LoadZoneData(GimmickGateWork *work) {
    GimmickGateZoneData temp;
    u32 i;
    u32 count;
    u32 zone;

    if (work->zoneData != 0) {
        return;
    }
    zone = Field_GetPlayerStateZoneID(work->field);
    work->zoneData =
        GFL_HeapAllocate(work->heapId, sizeof(struct GimmickGateZoneData), FALSE, "field_gimmick_gate.c", 0x39b);
    count = GFL_ArcSysGetDataMax(0xac);
    for (i = 0; i < count; i++) {
        GimmickGateZoneData_Load(&temp, 0xac, i);
        if (temp.zoneId != zone) {
            continue;
        }
        if (temp.version != 0 && temp.version != getGameVersion()) {
            continue;
        }
        if (zone == 0x177 || zone == 0x17b) {
            switch (GameData_GetMyCity(work->gameData)) {
            case 0:
                GimmickGateZoneData_Load(&temp, 0xac, i + 1);
                break;
            case 1:
                break;
            }
        }
        *work->zoneData = temp;
        return;
    }
}

void GimmickGate_FreeZoneData(GimmickGateWork *work) {
    if (work->zoneData != 0) {
        GFL_HeapFree(work->zoneData);
        work->zoneData = 0;
    }
}

void GimmickGate_StartBoardAnimation(GimmickGateWork *work) {
    FieldExpObjSystem *system;
    u32 anm;

    system = Field_GetExpObjSystem(work->field);
    anm = work->zoneData->anmIndex;
    FieldExpObj_SetAnm(system, 1, 0, sBoardAnimations[anm], TRUE);
}

void GimmickGate_PlaceBoard(GimmickGateWork *work) {
    FieldExpObjSystem *system;
    SRTMatrix *matrix;
    GimmickGateZoneData *state;
    u32 angle;

    system = Field_GetExpObjSystem(work->field);
    matrix = FieldExpObj_GetActorMatrixPtr(system, 1, 0);
    state = work->zoneData;
    matrix->translation.x = state->x << 12;
    matrix->translation.y = state->y << 12;
    matrix->translation.z = state->z << 12;
    switch (state->direction) {
    case 1:
        angle = 0;
        break;
    case 3:
        angle = 90;
        break;
    case 0:
        angle = 180;
        break;
    case 2:
        angle = 270;
        break;
    default:
        angle = 0;
        break;
    }
    MAT3_RotationEulerZYX(0, (u16)(angle * 182), 0, &matrix->rotation);
}

void GimmickGate_AddMessage(GimmickGateWork *work, u32 kind, u32 flag) {
    GimmickGateMessageList *list;
    s32 count;

    list = &work->messages;
    count = list->count;
    if (count < 7) {
        list->kinds[count] = kind;
        list->flags[count] = flag;
        list->count = count + 1;
    }
}

u32 GimmickGate_ShouldShowHallOfFame(GimmickGateWork *work) {
    if (GimmickGate_IsHallOfFameFlagSet(work) == 1 && work->flag != 0) {
        return 1;
    }
    return 0;
}

u32 GimmickGate_IsHallOfFameFlagSet(GimmickGateWork *work) {
    EventWork *eventWork;

    eventWork = GameData_GetEventWork(work->gameData);
    if (EventWork_FlagGet(eventWork, 0x960) == 1) {
        return 1;
    }
    return 0;
}

void GimmickGate_GetHallOfFameSpecies(const GimmickGateWork *work, u32 *species, s32 *count) {
    GameData *gameData = work->gameData;
    SaveControl *save;
    HallOfFameSave *hallOfFame;
    HallOfFamePokemon pokemon;
    s32 i = 0;
    u32 result;

    *count = 0;
    save = GameData_GetSaveControl(gameData);
    result = SaveControl_OpenExtraBlock(save, 8, work->heapId);
    if (result == 1 || result == 2) {
        hallOfFame = getAddressOfExtraSaveBlk(save, 8, 0);
        if (HallOfFameSave_GetTeamCount(hallOfFame) != 0) {
            *count = HallOfFameSave_GetPokemonCount(hallOfFame, 0);
            for (i = 0; i < *count; i++) {
                pokemon.nickname = GFL_StrBufCreate(0x40, work->heapId);
                pokemon.trainerName = GFL_StrBufCreate(0x40, work->heapId);
                HallOfFameSave_GetPokemon(hallOfFame, 0, i, &pokemon);
                species[i] = pokemon.species;
                GFL_StrBufFree(pokemon.nickname);
                GFL_StrBufFree(pokemon.trainerName);
            }
        }
    }
    freeIntermediateSaveExtraBlksAfterLoad(save, 8);
}

void GimmickGate_LoadBoardEntries(GimmickGateWork *work) {
    s32 count;
    s32 i;

    if (work->boardEntries == 0) {
        count = GFL_ArcSysGetDataMax(0xa4);
        work->boardEntries = GFL_HeapAllocate(work->heapId, count * sizeof(struct GimmickGateBoardEntry), FALSE,
                                              "field_gimmick_gate.c", 0x4b5);
        for (i = 0; i < count; i++) {
            GimmickGateBoardEntry_Load(&work->boardEntries[i], 0xa4, i);
        }
        work->boardEntryCount = count;
    }
}

void GimmickGate_FreeBoardEntries(GimmickGateWork *work) {
    if (work->boardEntries != 0) {
        GFL_HeapFree(work->boardEntries);
        work->boardEntries = 0;
        work->boardEntryCount = 0;
    }
}

GimmickGateBoardEntry *GimmickGate_FindBoardEntry(GimmickGateWork *work) {
    u16 zone;
    EventWork *eventWork;
    s32 i;
    BOOL flag;
    BOOL zoneMatch;

    zone = Field_GetPlayerStateZoneID(work->field);
    eventWork = GameData_GetEventWork(work->gameData);
    if (work->boardEntries == 0) {
        return 0;
    }
    for (i = 0; i < work->boardEntryCount; i++) {
        flag = EventWork_FlagGet(eventWork, work->boardEntries[i].flagId);
        zoneMatch = GimmickGateBoardEntry_MatchesZone(&work->boardEntries[i], zone);
        if (GimmickGateBoardEntry_MatchesVersion(&work->boardEntries[i]) && flag && zoneMatch) {
            return &work->boardEntries[i];
        }
    }
    return 0;
}

GimmickGateBoardEntry *GimmickGate_FindListEntry(GimmickGateWork *work) {
    u16 zone;
    EventWork *eventWork;
    s32 i;
    GimmickGateBoardEntry *entry;
    BOOL flag;
    BOOL zoneMatch;

    zone = Field_GetPlayerStateZoneID(work->field);
    eventWork = GameData_GetEventWork(work->gameData);
    if (work->boardEntries == 0) {
        return 0;
    }
    for (i = 0; i < work->boardEntryCount; i++) {
        flag = EventWork_FlagGet(eventWork, work->boardEntries[i].flagId);
        zoneMatch = GimmickGateBoardEntry_MatchesZone(&work->boardEntries[i], zone);
        if (flag && zoneMatch) {
            entry = &work->boardEntries[i];
            if (entry->type == 2) {
                return entry;
            }
        }
    }
    return 0;
}

u32 GimmickGate_HasBoardEntry(GimmickGateWork *work) {
    if (GimmickGate_FindBoardEntry(work) != 0) {
        return 1;
    }
    return 0;
}

void GimmickGate_AddBoardMessage(GimmickGateWork *work, ElboardMessageArg *message, u32 kind,
                                 GimmickGateBoardEntry *entry) {
    Elboard_AddMessage(work->board, message);
    if (kind == 8) {
        GimmickGate_AddMessage(work, kind, entry->flagId);
    } else {
        GimmickGate_AddMessage(work, kind, 0);
    }
    if (kind == 8 && entry->unk08 == 1) {
        EventWork_FlagReset(GameData_GetEventWork(work->gameData), entry->flagId);
    }
}

void GimmickGate_SetupMessages(GimmickGateWork *work) {
    if (GimmickGate_ShouldShowHallOfFame(work) == 1) {
        GimmickGate_SetupHallOfFameMessages(work);
        return;
    }
    if (GimmickGate_HasBoardEntry(work) != 0) {
        GimmickGate_SetupEntryMessages(work);
        return;
    }
    GimmickGate_SetupDefaultMessages(work);
}

void GimmickGate_SetupDefaultMessages(GimmickGateWork *work) {
    if (!Elboard_GetMessageCount(work->board) && work->zoneData && work->zoneData->enabled) {
        GimmickGate_AddSwarmMessage(work);
        GimmickGate_AddDateMessage(work);
        GimmickGate_AddWeatherMessage(work);
        GimmickGate_AddWeekdayMessage3(work);
        GimmickGate_AddWeekdayMessage4(work);
        GimmickGate_AddWeekdayMessage5(work);
        GimmickGate_AddWeekdayMessage6(work);
    }
}

void GimmickGate_SetupHallOfFameMessages(GimmickGateWork *work) {
    if (!Elboard_GetMessageCount(work->board) && work->zoneData && work->zoneData->enabled) {
        GimmickGate_AddSwarmMessage(work);
        GimmickGate_AddDateMessage(work);
        GimmickGate_AddWeatherMessage(work);
        GimmickGate_AddHallOfFameMessage(work);
        GimmickGate_AddWeekdayMessage6(work);
    }
}

void GimmickGate_SetupEntryMessages(GimmickGateWork *work) {
    if (!Elboard_GetMessageCount(work->board) && work->boardEntries && work->zoneData && work->zoneData->enabled) {
        GimmickGate_AddEntryMessages(work);
        GimmickGate_AddSwarmMessage(work);
        GimmickGate_AddDateMessage(work);
        GimmickGate_AddWeatherMessage(work);
        GimmickGate_AddWeekdayMessage6(work);
    }
}

void GimmickGate_AddDateMessage(GimmickGateWork *work) {
    WordSet *wordSet;
    ElboardMessageArg arg;
    RTCDate date;

    wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    RTC_GetDate(&date);
    WordSetNumber(wordSet, 2, date.year, 2, 2, 1);
    loadMonthToStrbuf(wordSet, 0, date.month);
    WordSetNumber(wordSet, 1, date.day, 2, 0, 1);
    arg.kind = sMessageKinds[0];
    arg.name = sBoardNames[0];
    arg.plName = sBoardPlNames[0];
    arg.messageArc = 2;
    arg.messageFile = 0x2b;
    arg.messageId = work->zoneData->messageIds[0];
    arg.wordSet = wordSet;
    GimmickGate_AddBoardMessage(work, &arg, 1, 0);
    GFL_WordSetSystemFree(wordSet);
}

void GimmickGate_AddWeatherMessage(GimmickGateWork *work) {
    ElboardMessageArg arg;
    GimmickGateZoneList list;
    u32 message;
    WordSet *wordSet;
    WordSet *formatSet;
    MsgData *placeMessages;
    MsgData *weatherMessages;
    StrBuf *placeName;
    StrBuf *weatherName;
    StrBuf *template;
    StrBuf *formatted;
    s32 i;
    u8 weather;

    GimmickGate_GetWeatherZones(work, &list);
    wordSet = GFL_WordSetSystemCreate(4, 0x100, work->heapId);
    placeMessages = GFL_MsgSysLoadData(0, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PLACE_NAMES, work->heapId);
    weatherMessages = GFL_MsgSysLoadData(0, ARCID_SYSTEM_MESSAGE, TEXT_BANK_GYM_INSECT, work->heapId);
    for (i = 0; i < 4; i++) {
        if (list.zones[i] == ZONE_MAX) {
            continue;
        }
        weather = list.weather[i];
        placeName = GFL_MsgDataLoadStrbufNew(placeMessages, ZoneData_GetPlaceNameID(list.zones[i]));
        weatherName = GFL_MsgDataLoadStrbufNew(weatherMessages, sWeatherMessages[weather]);
        formatSet = GFL_WordSetSystemCreate(2, 0x100, work->heapId);
        template = GFL_MsgDataLoadStrbufNew(weatherMessages, GymInsect_Text_Empty_57);
        formatted = GFL_StrBufCreate(0x40, work->heapId);
        GFL_WordSetSetStrbuf(formatSet, 0, placeName, 0, 1, 0);
        GFL_WordSetSetStrbuf(formatSet, 1, weatherName, 0, 1, 0);
        GFL_WordSetFormatStrbuf(formatSet, formatted, template);
        GFL_StrBufFree(template);
        GFL_WordSetSystemFree(formatSet);
        GFL_WordSetSetStrbuf(wordSet, i, formatted, 0, 1, 0);
        GFL_StrBufFree(placeName);
        GFL_StrBufFree(weatherName);
        GFL_StrBufFree(formatted);
    }
    GFL_MsgDataFree(weatherMessages);
    GFL_MsgDataFree(placeMessages);
    switch (GimmickGateZoneList_Count(&list)) {
    case 1:
        message = 0xc1;
        break;
    case 2:
        message = 0xc2;
        break;
    case 3:
        message = 0xc3;
        break;
    case 4:
        message = 0xc4;
        break;
    }
    arg.kind = sMessageKinds[1];
    arg.name = sBoardNames[1];
    arg.plName = sBoardPlNames[1];
    arg.messageArc = 2;
    arg.messageFile = 0x2b;
    arg.messageId = message;
    arg.wordSet = wordSet;
    GimmickGate_AddBoardMessage(work, &arg, 2, 0);
    GFL_WordSetSystemFree(wordSet);
}

void GimmickGate_AddSwarmMessage(GimmickGateWork *work) {
    u16 zone;
    WordSet *wordSet;
    MsgData *msgData;
    StrBuf *zoneName;
    ElboardMessageArg arg;

    zone = getSwarmLevelRangeFromData(work->gameData);
    if (zone == 0xffff || (zone == 0x159 && GameData_GetSeason(work->gameData) == SEASON_WINTER)) {
        return;
    }
    wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    msgData = GFL_MsgSysLoadData(0, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PLACE_NAMES, work->heapId);
    zoneName = GFL_MsgDataLoadStrbufNew(msgData, ZoneData_GetPlaceNameID(zone));
    GFL_WordSetSetStrbuf(wordSet, 0, zoneName, 0, 1, 0);
    GFL_StrBufFree(zoneName);
    GFL_MsgDataFree(msgData);
    arg.kind = sMessageKinds[2];
    arg.name = sBoardNames[2];
    arg.plName = sBoardPlNames[2];
    arg.messageArc = 2;
    arg.messageFile = 0x2b;
    arg.messageId = work->zoneData->messageIds[2];
    arg.wordSet = wordSet;
    GimmickGate_AddBoardMessage(work, &arg, 3, 0);
    GFL_WordSetSystemFree(wordSet);
}

void GimmickGate_AddWeekdayMessage3(GimmickGateWork *work) {
    ElboardMessageArg arg;
    RTCDate date;

    RTC_GetDate(&date);
    switch (date.week) {
    case 0:
        arg.messageId = work->zoneData->messageIds[9];
        break;
    case 1:
        arg.messageId = work->zoneData->messageIds[6];
        break;
    case 2:
        arg.messageId = work->zoneData->messageIds[3];
        break;
    case 3:
        arg.messageId = work->zoneData->messageIds[7];
        break;
    case 4:
        arg.messageId = work->zoneData->messageIds[4];
        break;
    case 5:
        arg.messageId = work->zoneData->messageIds[8];
        break;
    case 6:
        arg.messageId = work->zoneData->messageIds[5];
        break;
    }
    arg.kind = sMessageKinds[3];
    arg.name = sBoardNames[3];
    arg.plName = sBoardPlNames[3];
    arg.messageArc = 2;
    arg.messageFile = 0x2b;
    arg.wordSet = 0;
    GimmickGate_AddBoardMessage(work, &arg, 4, 0);
}

void GimmickGate_AddWeekdayMessage4(GimmickGateWork *work) {
    ElboardMessageArg arg;
    RTCDate date;

    RTC_GetDate(&date);
    switch (date.week) {
    case 0:
        arg.messageId = work->zoneData->messageIds[10];
        break;
    case 1:
        arg.messageId = work->zoneData->messageIds[7];
        break;
    case 2:
        arg.messageId = work->zoneData->messageIds[4];
        break;
    case 3:
        arg.messageId = work->zoneData->messageIds[8];
        break;
    case 4:
        arg.messageId = work->zoneData->messageIds[5];
        break;
    case 5:
        arg.messageId = work->zoneData->messageIds[9];
        break;
    case 6:
        arg.messageId = work->zoneData->messageIds[6];
        break;
    }
    arg.kind = sMessageKinds[4];
    arg.name = sBoardNames[4];
    arg.plName = sBoardPlNames[4];
    arg.messageArc = 2;
    arg.messageFile = 0x2b;
    arg.wordSet = 0;
    GimmickGate_AddBoardMessage(work, &arg, 5, 0);
}

void GimmickGate_AddWeekdayMessage5(GimmickGateWork *work) {
    ElboardMessageArg arg;
    RTCDate date;

    RTC_GetDate(&date);
    switch (date.week) {
    case 0:
        arg.messageId = work->zoneData->messageIds[11];
        break;
    case 1:
        arg.messageId = work->zoneData->messageIds[8];
        break;
    case 2:
        arg.messageId = work->zoneData->messageIds[5];
        break;
    case 3:
        arg.messageId = work->zoneData->messageIds[9];
        break;
    case 4:
        arg.messageId = work->zoneData->messageIds[6];
        break;
    case 5:
        arg.messageId = work->zoneData->messageIds[10];
        break;
    case 6:
        arg.messageId = work->zoneData->messageIds[7];
        break;
    }
    arg.kind = sMessageKinds[5];
    arg.name = sBoardNames[5];
    arg.plName = sBoardPlNames[5];
    arg.messageArc = 2;
    arg.messageFile = 0x2b;
    arg.wordSet = 0;
    GimmickGate_AddBoardMessage(work, &arg, 6, 0);
}

void GimmickGate_AddWeekdayMessage6(GimmickGateWork *work) {
    ElboardMessageArg arg;
    RTCDate date;

    RTC_GetDate(&date);
    switch (date.week) {
    case 0:
        arg.messageId = work->zoneData->messageIds[18];
        break;
    case 1:
        arg.messageId = work->zoneData->messageIds[12];
        break;
    case 2:
        arg.messageId = work->zoneData->messageIds[13];
        break;
    case 3:
        arg.messageId = work->zoneData->messageIds[14];
        break;
    case 4:
        arg.messageId = work->zoneData->messageIds[15];
        break;
    case 5:
        arg.messageId = work->zoneData->messageIds[16];
        break;
    case 6:
        arg.messageId = work->zoneData->messageIds[17];
        break;
    }
    arg.kind = sMessageKinds[6];
    arg.name = sBoardNames[6];
    arg.plName = sBoardPlNames[6];
    arg.messageArc = 2;
    arg.messageFile = 0x2b;
    arg.wordSet = 0;
    GimmickGate_AddBoardMessage(work, &arg, 7, 0);
}

void GimmickGate_AddHallOfFameMessage(GimmickGateWork *work) {
    s32 count;
    ElboardMessageArg arg;
    u32 species[6];
    WordSet *wordSet;
    u32 message;
    s32 i;

    wordSet = GFL_WordSetSystemCreateDefault(Elboard_GetHeapID(work->board));
    GimmickGate_GetHallOfFameSpecies(work, species, &count);
    switch (count) {
    case 1:
        message = 0xcd;
        break;
    case 2:
        message = 0xce;
        break;
    case 3:
        message = 0xcf;
        break;
    case 4:
        message = 0xd0;
        break;
    case 5:
        message = 0xd1;
        break;
    case 6:
        message = 0xd2;
        break;
    }
    for (i = 0; i < count; i++) {
        WordSet_LoadSpeciesName(wordSet, i, (u16)species[i]);
    }
    copyVarForText(wordSet, 6, GetGameDataPlayerInfo(work->gameData));
    arg.kind = sMessageKinds[3];
    arg.name = sBoardNames[3];
    arg.plName = sBoardPlNames[3];
    arg.messageArc = 2;
    arg.messageFile = 0x2b;
    arg.messageId = message;
    arg.wordSet = wordSet;
    GimmickGate_AddBoardMessage(work, &arg, 9, 0);
    GFL_WordSetSystemFree(wordSet);
}

void GimmickGate_AddEntryMessages(GimmickGateWork *work) {
    GimmickGateBoardEntry *entry;

    entry = GimmickGate_FindBoardEntry(work);
    switch (entry->type) {
    case 0:
        GimmickGate_AddEntryMessage(work, entry, 3);
        break;
    case 2:
        GimmickGate_AddListEntryMessages(work);
        break;
    }
}

void GimmickGate_AddEntryMessage(GimmickGateWork *work, GimmickGateBoardEntry *entry, u32 index) {
    ElboardMessageArg arg;

    arg.kind = sMessageKinds[index];
    arg.name = sBoardNames[index];
    arg.plName = sBoardPlNames[index];
    arg.messageArc = 2;
    arg.messageFile = 0x2b;
    arg.messageId = entry->unk0c;
    arg.wordSet = NULL;
    GimmickGate_AddBoardMessage(work, &arg, 8, entry);
}

void GimmickGate_AddListEntryMessages(GimmickGateWork *work) {
    s32 index;
    GimmickGateBoardEntry *entry;

    index = 0;
    entry = GimmickGate_FindListEntry(work);
    while (entry && index < 3) {
        GimmickGate_AddEntryMessage(work, entry, sEntrySlots[index++]);
        entry = GimmickGate_FindListEntry(work);
    }
}

s32 GimmickGateZoneList_Count(const GimmickGateZoneList *list) {
    s32 index;
    s32 count;

    count = 0;
    for (index = 0; index < 4; index++) {
        if (list->zones[index] != ZONE_MAX && list->weather[index] != 0xffff) {
            count++;
        }
    }
    return count;
}

void GimmickGateZoneList_Clear(GimmickGateZoneList *list) {
    s32 index;

    for (index = 0; index < 4; index++) {
        list->zones[index] = ZONE_MAX;
        list->weather[index] = 0xff;
    }
}

void GimmickGate_GetWeatherZones(GimmickGateWork *work, GimmickGateZoneList *out) {
    GimmickGateZoneList second;
    GimmickGateZoneList first;
    s32 count;
    s32 i;
    GimmickGateZoneData *zoneData;

    count = 0;
    GimmickGateZoneList_Clear(out);
    GimmickGate_GetReportedWeatherZones(work, &second);
    GimmickGate_GetRoamerZones(work, &first);
    for (i = 0; i < 4; i++) {
        if (first.zones[i] == ZONE_MAX || first.weather[i] == 0xffff || count >= 2) {
            break;
        }
        out->zones[count] = first.zones[i];
        out->weather[count] = first.weather[i];
        count++;
    }
    for (i = 0; i < 4; i++) {
        if (second.zones[i] == ZONE_MAX || second.weather[i] == 0xffff || count >= 2) {
            break;
        }
        out->zones[count] = second.zones[i];
        out->weather[count] = second.weather[i];
        count++;
    }
    if (count == 0) {
        zoneData = work->zoneData;
        out->zones[0] = zoneData->fallbackZones[0];
        out->zones[1] = zoneData->fallbackZones[1];
        out->zones[2] = zoneData->fallbackZones[2];
        out->zones[3] = zoneData->fallbackZones[3];
    }
    for (i = 0; i < 4; i++) {
        out->weather[i] = Field_GetWeatherForZone(work->field, out->zones[i]);
    }
}

void GimmickGate_GetReportedWeatherZones(GimmickGateWork *work, GimmickGateZoneList *list) {
    s32 count;
    int i;
    int j;
    u16 zone;
    u8 weather;

    GimmickGateZoneList_Clear(list);
    count = 0;
    for (i = 0; i < NELEMS(sWeatherZones); i++) {
        zone = sWeatherZones[i];
        weather = Field_GetWeatherForZone(work->field, zone);
        for (j = 0; j < NELEMS(sReportedWeathers); j++) {
            if (weather == sReportedWeathers[j]) {
                if (count >= 4) {
                    break;
                }
                list->zones[count] = zone;
                list->weather[count] = weather;
                count++;
            }
        }
    }
}

void GimmickGate_GetRoamerZones(GimmickGateWork *work, GimmickGateZoneList *list) {
    EncountSave *save;
    u32 version;
    u32 slot;
    u32 weather;
    u16 zone;

    save = SaveControl_GetEncountSave(GameData_GetSaveControl(work->gameData));
    GimmickGateZoneList_Clear(list);
    if (EncountSave_HasActiveEntry(save)) {
        version = getGameVersion();
        switch (version) {
        case 0x16:
            slot = 0;
            break;
        case 0x17:
            slot = 1;
            break;
        }
        switch (slot) {
        case 0:
            weather = 6;
            break;
        case 1:
            weather = 7;
            break;
        }
        zone = EncountSave_GetRoamingPkmZone(save, (u8)slot);
        if (zone != 0xffff) {
            list->zones[0] = zone;
            list->weather[0] = weather;
        }
    }
}

BOOL Scrcmd_FieldGimmickGate_GimmickGateViewBoard(VM *vm, FieldScriptEnv *env) {
    ScriptWork *scriptWork;
    GameSystem *gsys;
    Field *field;
    u32 id;
    GameEvent *event;

    scriptWork = FieldScriptEnv_GetScriptWork(env);
    gsys = FieldScriptEnv_GetGameSystem(env);
    field = GSYS_GetField(gsys);
    id = ScriptReadAny(vm, env);
    GFL_SndSEPlay(0x547);
    event = GimmickGateEvent_Create(gsys, field, id);
    ScriptWork_CallEvent(scriptWork, event);
    return TRUE;
}
