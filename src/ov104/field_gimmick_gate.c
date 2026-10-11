#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/encounter.h"
#include "field/field.h"
#include "field/field_environment.h"
#include "field/field_exp_obj.h"
#include "field/field_gimmick_gate.h"
#include "field/gimmick_obj_elboard.h"
#include "field/field_map.h"
#include "field/field_script.h"
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

void func_ov104_021eed78(GimmickGateWork *work);
void func_ov104_021eedb0(GimmickGateWork *work);
void func_ov104_021eedcc(GimmickGateWork *work);
void func_ov104_021eede4(GimmickGateWork *work);
void func_ov104_021eee24(GimmickGateWork *work);
GimmickGateWork *func_ov104_021eee34(Field *field);
void func_ov104_021eeebc(GimmickGateWork *work);
void func_ov104_021eef84(GimmickGateWork *work);
void func_ov104_021eef98(GimmickGateWork *work);
void func_ov104_021eefc0(GimmickGateWork *work);
void func_ov104_021ef02c(GimmickGateWork *work, u32 kind, u32 flag);
u32 func_ov104_021ef04c(GimmickGateWork *work);
u32 func_ov104_021ef068(GimmickGateWork *work);
void func_ov104_021ef084(GimmickGateWork *work, u32 *species, s32 *count);
void func_ov104_021ef114(GimmickGateWork *work);
void func_ov104_021ef168(GimmickGateWork *work);
GimmickGateBoardEntry *func_ov104_021ef180(GimmickGateWork *work);
GimmickGateBoardEntry *func_ov104_021ef204(GimmickGateWork *work);
u32 func_ov104_021ef278(GimmickGateWork *work);
void func_ov104_021ef28c(GimmickGateWork *work, ElboardMessageArg *message, u32 kind, GimmickGateBoardEntry *entry);
void func_ov104_021ef2cc(GimmickGateWork *work);
void func_ov104_021ef2fc(GimmickGateWork *work);
void func_ov104_021ef344(GimmickGateWork *work);
void func_ov104_021ef380(GimmickGateWork *work);
void func_ov104_021ef3c0(GimmickGateWork *work);
void func_ov104_021ef43c(GimmickGateWork *work);
void func_ov104_021ef5ac(GimmickGateWork *work);
void func_ov104_021ef658(GimmickGateWork *work);
void func_ov104_021ef6dc(GimmickGateWork *work);
void func_ov104_021ef760(GimmickGateWork *work);
void func_ov104_021ef7e4(GimmickGateWork *work);
void func_ov104_021ef868(GimmickGateWork *work);
void func_ov104_021ef924(GimmickGateWork *work);
void func_ov104_021ef94c(GimmickGateWork *work, GimmickGateBoardEntry *entry, u32 index);
void func_ov104_021ef994(GimmickGateWork *work);
s32 func_ov104_021ef9c8(const GimmickGateZoneList *list);
void func_ov104_021ef9f8(GimmickGateZoneList *list);
void func_ov104_021efa18(GimmickGateWork *work, GimmickGateZoneList *list);
void func_ov104_021efad0(GimmickGateWork *work, GimmickGateZoneList *list);
void func_ov104_021efb30(GimmickGateWork *work, GimmickGateZoneList *list);
BOOL func_ov104_021efb90(VM *vm, FieldScriptEnv *env);
void func_ov104_021eeea0(GimmickGateWork *work);
void func_ov104_021eeee0(GimmickGateWork *work);
GimmickGateSave *func_ov104_021eed58(Field *field);

static const u32 sEntrySlots[3] = { 3, 4, 5 };

// The zones whose weather the boards report, and the weathers they report
extern const u16 data_ov104_021f039c[1];
extern const u8 data_ov104_021f039e[3];
// The board's scenes
extern const G3DSceneSetup data_ov104_021f03d0[2];

// The board advances by this each frame
static fx32 sBoardStep = FX32_ONE;
static u16 sMessageKinds[8] = { 0, 1, 2, 3, 4, 5, 6 };
static const char *sBoardPlNames[7] = {
    "gelboard_1_pl", "gelboard_2_pl", "gelboard_3_pl", "gelboard_4_pl",
    "gelboard_5_pl", "gelboard_6_pl", "gelboard_7_pl",
};
static const char *sBoardNames[7] = {
    "gelboard_1", "gelboard_2", "gelboard_3", "gelboard_4", "gelboard_5", "gelboard_6", "gelboard_7",
};
static u16 sBoardAnimations[16] = { 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21 };
// Messages for the weathers, as GimmickGateZoneList gives them
static u32 sWeatherMessages[15] = {
    0xb5, 0xb6, 0xb7, 0xb8, 0xba, 0xbb, 0xbe, 0xbf, 0xbd, 0xbc, 0xbc, 0xbc, 0xb9, 0xbc, 0xbc,
};

void func_ov104_021eec80(Field *field) {
    FieldExpObjSystem *expObj = Field_GetExpObjSystem(field);
    GimmickGateWork *work;

    func_02008a84((Config *)getTrainerDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(Field_GetGameSystem(field)))));
    LoadFieldExpandObjData(expObj, &data_ov104_021f03d0[1], 1);
    work = func_ov104_021eee34(field);
    func_ov104_021eeee0(work);
    func_ov104_021ef114(work);
    func_ov104_021eedb0(work);
    if (work->value == 0) {
        func_ov104_021eeebc(work);
    } else {
        func_ov104_021eedcc(work);
        func_ov104_021eede4(work);
    }
    func_ov104_021ef2cc(work);
    func_ov104_021eee24(work);
    func_ov104_021eefc0(work);
    func_ov104_021eef98(work);
}

void func_ov104_021eed00(Field *field) {
    GimmickGateWork *work;

    work = Field_GetGimmickWorkBlock(field, 0);
    func_ov104_021eed78(work);
    if (work->board != 0) {
        func_ov104_021efc8c(work->board);
    }
    func_ov104_021eeea0(work);
}

void func_ov104_021eed20(Field *field) {
    GimmickGateWork *work;

    work = Field_GetGimmickWorkBlock(field, 0);
    func_ov104_021efcc4(work->board, sBoardStep);
    FieldExpObj_StepAllAnimations(Field_GetExpObjSystem(field));
}

u32 func_ov104_021eed44(Field *field) {
    GimmickGateWork *work;
    GimmickGateZoneData *zoneData;

    work = Field_GetGimmickWorkBlock(field, 0);
    zoneData = work->zoneData;
    return (u8)zoneData->direction;
}

GimmickGateSave *func_ov104_021eed58(Field *field) {
    GimmickState *state;
    u32 id;

    state = GameData_GetGimmickState(GSYS_GetGameData(Field_GetGameSystem(field)));
    id = GimmickState_GetID(state);
    return GimmickState_GetUserData(state, id);
}

void func_ov104_021eed78(GimmickGateWork *work) {
    GimmickGateSave *data;

    data = func_ov104_021eed58(work->field);
    data->value = work->value;
    data->boardTime = func_ov104_021efcf0(work->board) >> 12;
    data->flag = work->flag;
    data->messages = work->messages;
}

void func_ov104_021eedb0(GimmickGateWork *work) {
    GimmickGateSave *data;

    data = func_ov104_021eed58(work->field);
    work->value = data->value;
    work->boardTime = data->boardTime;
    work->flag = data->flag;
}

void func_ov104_021eedcc(GimmickGateWork *work) {
    SaveControl *save;
    TrainerCardSave *card;

    save = GameData_GetSaveControl(work->gameData);
    card = getTrainerGameInfoAddress(save);
    func_0200cb08(card, work->flag);
}

void func_ov104_021eede4(GimmickGateWork *work) {
    EventWork *eventWork;
    s32 i;
    GimmickGateMessageList *list;

    eventWork = GameData_GetEventWork(work->gameData);
    list = &func_ov104_021eed58(work->field)->messages;
    for (i = 0; i < list->count; i++) {
        if (list->kinds[i] == 8) {
            EventWork_FlagSet(eventWork, list->flags[i]);
        }
    }
}

void func_ov104_021eee24(GimmickGateWork *work) {
    func_ov104_021efcfc(work->board, work->boardTime << 12);
}

GimmickGateWork *func_ov104_021eee34(Field *field) {
    u16 heapId;
    FieldExpObjSystem *system;
    GimmickGateWork *work;
    ElboardInit init;

    heapId = Field_GetHeapID(field);
    system = Field_GetExpObjSystem(field);
    work = Field_AllocGimmickWorkBlock(field, 0, heapId, sizeof(GimmickGateWork));
    sys_memset(work, 0, sizeof(GimmickGateWork));
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
    work->board = func_ov104_021efbd8(&init);
    return work;
}

void func_ov104_021eeea0(GimmickGateWork *work) {
    if (work != 0) {
        func_ov104_021eef84(work);
        func_ov104_021ef168(work);
        Field_DeleteGimmickWorkBlock(work->field, 0);
    }
}

void func_ov104_021eeebc(GimmickGateWork *work) {
    SaveControl *save;
    TrainerCardSave *card;

    if (work->value == 0) {
        save = GameData_GetSaveControl_(work->gameData);
        card = getTrainerCardData_wrapper(save);
        work->flag = func_0200cb00(card);
        work->value = 1;
    }
}

void func_ov104_021eeee0(GimmickGateWork *work) {
    GimmickGateZoneData temp;
    u32 i;
    u32 count;
    u32 zone;

    if (work->zoneData != 0) {
        return;
    }
    zone = Field_GetPlayerStateZoneID(work->field);
    work->zoneData = GFL_HeapAllocate(work->heapId, sizeof(struct GimmickGateZoneData), FALSE,
                                      "field_gimmick_gate.c", 0x39b);
    count = GFL_ArcSysGetDataMax(0xac);
    for (i = 0; i < count; i++) {
        func_ov104_021f0150(&temp, 0xac, i);
        if (temp.zoneId != zone) {
            continue;
        }
        if (temp.version != 0 && temp.version != getGameVersion()) {
            continue;
        }
        if (zone == 0x177 || zone == 0x17b) {
            switch (func_02017220(work->gameData)) {
            case 0:
                func_ov104_021f0150(&temp, 0xac, i + 1);
                break;
            case 1:
                break;
            }
        }
        *work->zoneData = temp;
        return;
    }
}

void func_ov104_021eef84(GimmickGateWork *work) {
    if (work->zoneData != 0) {
        GFL_HeapFree(work->zoneData);
        work->zoneData = 0;
    }
}

void func_ov104_021eef98(GimmickGateWork *work) {
    FieldExpObjSystem *system;
    u32 anm;

    system = Field_GetExpObjSystem(work->field);
    anm = work->zoneData->anmIndex;
    FieldExpObj_SetAnm(system, 1, 0, sBoardAnimations[anm], TRUE);
}

void func_ov104_021eefc0(GimmickGateWork *work) {
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

void func_ov104_021ef02c(GimmickGateWork *work, u32 kind, u32 flag) {
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

u32 func_ov104_021ef04c(GimmickGateWork *work) {
    if (func_ov104_021ef068(work) == 1 && work->flag != 0) {
        return 1;
    }
    return 0;
}

u32 func_ov104_021ef068(GimmickGateWork *work) {
    EventWork *eventWork;

    eventWork = GameData_GetEventWork(work->gameData);
    if (EventWork_FlagGet(eventWork, 0x960) == 1) {
        return 1;
    }
    return 0;
}

void func_ov104_021ef084(GimmickGateWork *work, u32 *species, s32 *count) {
    GameData *gameData = work->gameData;
    SaveControl *save;
    HallOfFameSave *hallOfFame;
    HallOfFamePokemon pokemon;
    s32 i = 0;
    u32 result;

    *count = 0;
    save = GameData_GetSaveControl(gameData);
    result = func_020074ec(save, 8, work->heapId);
    if (result == 1 || result == 2) {
        hallOfFame = getAddressOfExtraSaveBlk(save, 8, 0);
        if (func_0200f660(hallOfFame) != 0) {
            *count = func_0200f67c(hallOfFame, 0);
            for (i = 0; i < *count; i++) {
                pokemon.nickname = GFL_StrBufCreate(0x40, work->heapId);
                pokemon.trainerName = GFL_StrBufCreate(0x40, work->heapId);
                func_0200f69c(hallOfFame, 0, i, &pokemon);
                species[i] = pokemon.species;
                GFL_StrBufFree(pokemon.nickname);
                GFL_StrBufFree(pokemon.trainerName);
            }
        }
    }
    freeIntermediateSaveExtraBlksAfterLoad(save, 8);
}

void func_ov104_021ef114(GimmickGateWork *work) {
    s32 count;
    s32 i;

    if (work->boardEntries == 0) {
        count = GFL_ArcSysGetDataMax(0xa4);
        work->boardEntries = GFL_HeapAllocate(work->heapId, count * sizeof(struct GimmickGateBoardEntry), FALSE,
                                         "field_gimmick_gate.c", 0x4b5);
        for (i = 0; i < count; i++) {
            func_ov104_021f0324(&work->boardEntries[i], 0xa4, i);
        }
        work->boardEntryCount = count;
    }
}

void func_ov104_021ef168(GimmickGateWork *work) {
    if (work->boardEntries != 0) {
        GFL_HeapFree(work->boardEntries);
        work->boardEntries = 0;
        work->boardEntryCount = 0;
    }
}

GimmickGateBoardEntry *func_ov104_021ef180(GimmickGateWork *work) {
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
        zoneMatch = func_ov104_021f0334(&work->boardEntries[i], zone);
        if (func_ov104_021f037c(&work->boardEntries[i]) && flag &&
            zoneMatch) {
            return &work->boardEntries[i];
        }
    }
    return 0;
}

GimmickGateBoardEntry *func_ov104_021ef204(GimmickGateWork *work) {
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
        zoneMatch = func_ov104_021f0334(&work->boardEntries[i], zone);
        if (flag && zoneMatch) {
            entry = &work->boardEntries[i];
            if (entry->type == 2) {
                return entry;
            }
        }
    }
    return 0;
}

u32 func_ov104_021ef278(GimmickGateWork *work) {
    if (func_ov104_021ef180(work) != 0) {
        return 1;
    }
    return 0;
}

void func_ov104_021ef28c(GimmickGateWork *work, ElboardMessageArg *message, u32 kind, GimmickGateBoardEntry *entry) {
    func_ov104_021efc6c(work->board, message);
    if (kind == 8) {
        func_ov104_021ef02c(work, kind, entry->flagId);
    } else {
        func_ov104_021ef02c(work, kind, 0);
    }
    if (kind == 8 && entry->unk08 == 1) {
        EventWork_FlagReset(GameData_GetEventWork(work->gameData), entry->flagId);
    }
}

void func_ov104_021ef2cc(GimmickGateWork *work) {
    if (func_ov104_021ef04c(work) == 1) {
        func_ov104_021ef344(work);
        return;
    }
    if (func_ov104_021ef278(work) != 0) {
        func_ov104_021ef380(work);
        return;
    }
    func_ov104_021ef2fc(work);
}

void func_ov104_021ef2fc(GimmickGateWork *work) {
    if (!func_ov104_021efcf8(work->board) && work->zoneData && work->zoneData->enabled) {
        func_ov104_021ef5ac(work);
        func_ov104_021ef3c0(work);
        func_ov104_021ef43c(work);
        func_ov104_021ef658(work);
        func_ov104_021ef6dc(work);
        func_ov104_021ef760(work);
        func_ov104_021ef7e4(work);
    }
}

void func_ov104_021ef344(GimmickGateWork *work) {
    if (!func_ov104_021efcf8(work->board) && work->zoneData && work->zoneData->enabled) {
        func_ov104_021ef5ac(work);
        func_ov104_021ef3c0(work);
        func_ov104_021ef43c(work);
        func_ov104_021ef868(work);
        func_ov104_021ef7e4(work);
    }
}

void func_ov104_021ef380(GimmickGateWork *work) {
    if (!func_ov104_021efcf8(work->board) && work->boardEntries && work->zoneData && work->zoneData->enabled) {
        func_ov104_021ef924(work);
        func_ov104_021ef5ac(work);
        func_ov104_021ef3c0(work);
        func_ov104_021ef43c(work);
        func_ov104_021ef7e4(work);
    }
}

void func_ov104_021ef3c0(GimmickGateWork *work) {
    WordSet *wordSet;
    ElboardMessageArg arg;
    RTCDate date;

    wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    func_0207cc10(&date);
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
    func_ov104_021ef28c(work, &arg, 1, 0);
    GFL_WordSetSystemFree(wordSet);
}

void func_ov104_021ef43c(GimmickGateWork *work) {
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

    func_ov104_021efa18(work, &list);
    wordSet = GFL_WordSetSystemCreate(4, 0x100, work->heapId);
    placeMessages = GFL_MsgSysLoadData(0, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PLACE_NAMES, work->heapId);
    weatherMessages = GFL_MsgSysLoadData(0, ARCID_SYSTEM_MESSAGE, TEXT_BANK_GYM_INSECT, work->heapId);
    for (i = 0; i < 4; i++) {
        if (list.zones[i] == 0x267) {
            continue;
        }
        weather = list.weather[i];
        placeName = GFL_MsgDataLoadStrbufNew(placeMessages, ZoneData_GetPlaceNameID(list.zones[i]));
        weatherName = GFL_MsgDataLoadStrbufNew(weatherMessages, sWeatherMessages[weather]);
        formatSet = GFL_WordSetSystemCreate(2, 0x100, work->heapId);
        template = GFL_MsgDataLoadStrbufNew(weatherMessages, GymInsect_Text_Empty_57);
        formatted = GFL_StrBufCreate(0x40, work->heapId);
        func_0202437c(formatSet, 0, placeName, 0, 1, 0);
        func_0202437c(formatSet, 1, weatherName, 0, 1, 0);
        GFL_WordSetFormatStrbuf(formatSet, formatted, template);
        GFL_StrBufFree(template);
        GFL_WordSetSystemFree(formatSet);
        func_0202437c(wordSet, i, formatted, 0, 1, 0);
        GFL_StrBufFree(placeName);
        GFL_StrBufFree(weatherName);
        GFL_StrBufFree(formatted);
    }
    GFL_MsgDataFree(weatherMessages);
    GFL_MsgDataFree(placeMessages);
    switch (func_ov104_021ef9c8(&list)) {
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
    func_ov104_021ef28c(work, &arg, 2, 0);
    GFL_WordSetSystemFree(wordSet);
}

void func_ov104_021ef5ac(GimmickGateWork *work) {
    u16 zone;
    WordSet *wordSet;
    MsgData *msgData;
    StrBuf *zoneName;
    ElboardMessageArg arg;

    zone = getSwarmLevelRangeFromData(work->gameData);
    if (zone == 0xffff || (zone == 0x159 && GameData_GetSeason(work->gameData) == 3)) {
        return;
    }
    wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    msgData = GFL_MsgSysLoadData(0, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PLACE_NAMES, work->heapId);
    zoneName = GFL_MsgDataLoadStrbufNew(msgData, ZoneData_GetPlaceNameID(zone));
    func_0202437c(wordSet, 0, zoneName, 0, 1, 0);
    GFL_StrBufFree(zoneName);
    GFL_MsgDataFree(msgData);
    arg.kind = sMessageKinds[2];
    arg.name = sBoardNames[2];
    arg.plName = sBoardPlNames[2];
    arg.messageArc = 2;
    arg.messageFile = 0x2b;
    arg.messageId = work->zoneData->messageIds[2];
    arg.wordSet = wordSet;
    func_ov104_021ef28c(work, &arg, 3, 0);
    GFL_WordSetSystemFree(wordSet);
}

void func_ov104_021ef658(GimmickGateWork *work) {
    ElboardMessageArg arg;
    RTCDate date;

    func_0207cc10(&date);
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
    func_ov104_021ef28c(work, &arg, 4, 0);
}

void func_ov104_021ef6dc(GimmickGateWork *work) {
    ElboardMessageArg arg;
    RTCDate date;

    func_0207cc10(&date);
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
    func_ov104_021ef28c(work, &arg, 5, 0);
}

void func_ov104_021ef760(GimmickGateWork *work) {
    ElboardMessageArg arg;
    RTCDate date;

    func_0207cc10(&date);
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
    func_ov104_021ef28c(work, &arg, 6, 0);
}

void func_ov104_021ef7e4(GimmickGateWork *work) {
    ElboardMessageArg arg;
    RTCDate date;

    func_0207cc10(&date);
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
    func_ov104_021ef28c(work, &arg, 7, 0);
}

void func_ov104_021ef868(GimmickGateWork *work) {
    s32 count;
    ElboardMessageArg arg;
    u32 species[6];
    WordSet *wordSet;
    u32 message;
    s32 i;

    wordSet = GFL_WordSetSystemCreateDefault(func_ov104_021efcf4(work->board));
    func_ov104_021ef084(work, species, &count);
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
    func_ov104_021ef28c(work, &arg, 9, 0);
    GFL_WordSetSystemFree(wordSet);
}

void func_ov104_021ef924(GimmickGateWork *work) {
    GimmickGateBoardEntry *entry;

    entry = func_ov104_021ef180(work);
    switch (entry->type) {
    case 0:
        func_ov104_021ef94c(work, entry, 3);
        break;
    case 2:
        func_ov104_021ef994(work);
        break;
    }
}

void func_ov104_021ef94c(GimmickGateWork *work, GimmickGateBoardEntry *entry, u32 index) {
    ElboardMessageArg arg;

    arg.kind = sMessageKinds[index];
    arg.name = sBoardNames[index];
    arg.plName = sBoardPlNames[index];
    arg.messageArc = 2;
    arg.messageFile = 0x2b;
    arg.messageId = entry->unk0c;
    arg.wordSet = NULL;
    func_ov104_021ef28c(work, &arg, 8, entry);
}

void func_ov104_021ef994(GimmickGateWork *work) {
    s32 index;
    GimmickGateBoardEntry *entry;

    index = 0;
    entry = func_ov104_021ef204(work);
    while (entry && index < 3) {
        func_ov104_021ef94c(work, entry, sEntrySlots[index++]);
        entry = func_ov104_021ef204(work);
    }
}

s32 func_ov104_021ef9c8(const GimmickGateZoneList *list) {
    s32 index;
    s32 count;

    count = 0;
    for (index = 0; index < 4; index++) {
        if (list->zones[index] != 0x267 && list->weather[index] != 0xffff) {
            count++;
        }
    }
    return count;
}

void func_ov104_021ef9f8(GimmickGateZoneList *list) {
    s32 index;

    for (index = 0; index < 4; index++) {
        list->zones[index] = 0x267;
        list->weather[index] = 0xff;
    }
}

void func_ov104_021efa18(GimmickGateWork *work, GimmickGateZoneList *out) {
    GimmickGateZoneList second;
    GimmickGateZoneList first;
    s32 count;
    s32 i;
    GimmickGateZoneData *zoneData;

    count = 0;
    func_ov104_021ef9f8(out);
    func_ov104_021efad0(work, &second);
    func_ov104_021efb30(work, &first);
    for (i = 0; i < 4; i++) {
        if (first.zones[i] == 0x267 || first.weather[i] == 0xffff || count >= 2) {
            break;
        }
        out->zones[count] = first.zones[i];
        out->weather[count] = first.weather[i];
        count++;
    }
    for (i = 0; i < 4; i++) {
        if (second.zones[i] == 0x267 || second.weather[i] == 0xffff || count >= 2) {
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

void func_ov104_021efad0(GimmickGateWork *work, GimmickGateZoneList *list) {
    s32 count;
    u32 i;
    u32 j;
    u16 zone;
    u8 weather;

    func_ov104_021ef9f8(list);
    count = 0;
    for (i = 0; i < 1; i++) {
        zone = data_ov104_021f039c[i];
        weather = Field_GetWeatherForZone(work->field, zone);
        for (j = 0; j < 3; j++) {
            if (weather == data_ov104_021f039e[j]) {
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

void func_ov104_021efb30(GimmickGateWork *work, GimmickGateZoneList *list) {
    EncountSave *save;
    u32 version;
    u32 slot;
    u32 weather;
    u16 zone;

    save = SaveControl_GetEncountSave(GameData_GetSaveControl(work->gameData));
    func_ov104_021ef9f8(list);
    if (func_ov012_02159218(save)) {
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

BOOL func_ov104_021efb90(VM *vm, FieldScriptEnv *env) {
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
    event = func_ov104_021f02fc(gsys, field, id);
    ScriptWork_CallEvent(scriptWork, event);
    return TRUE;
}
