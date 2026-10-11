#include "types.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "field/event_fest_mission.h"
#include "constants/sound.h"
#include "field/encounter.h"
#include "field/encounter_effect.h"
#include "field/event_festival.h"
#include "field/event_sound.h"
#include "field/fest_mission_data.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/field_sound.h"
#include "field/townmap_util.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "save/high_link.h"
#include "save/join_avenue.h"
#include "save/medal_box.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/str_tool.h"
#include "system/wordset.h"
#include "text/script/0412.h"

// Overlay 21's event_festival.c
#define OVERLAY_EVENT_FESTIVAL OVERLAY_ID(21)

// The medals for hosting 10 and 50 missions, and for joining 1 and 10
#define MEDAL_MISSION_HOST_10 0xe1
#define MEDAL_MISSION_HOST_50 0xe2
#define MEDAL_MISSION_JOIN_1 0xe3
#define MEDAL_MISSION_JOIN_10 0xe4

typedef struct {
    u8 unk00;
    // Whether the player joins another's mission, rather than hosting it
    u8 joined;
    HeapID heapId;
    HeapID tailHeapId;
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    void *msgBGSys;
    void *window;
    u8 unk1C[8];
    MMSys *actorSystem;
    void *fesGimmick;
    u8 unk2C[8];
    MsgData *msgData;
    WordSet *wordSet;
    void *passPowerData;
    u32 unk40;
    LinkFestival *festival;
    // Copied from a FestMissionConfig, the same 0x2c bytes: FestMissionConfig and FestMission are two layouts of one
    // struct, so the copies cast until they are merged
    FestMission mission;
    FestivalText *festText;
    u8 unk78[0x14];
    // 0 or 1, which picks message 3 or 4 when the mission ends
    u32 endType;
    u32 messageType;
    StrBuf *strbuf;
    StrBuf *formatted;
} FestMissionEventWork;

typedef struct {
    u32 state;
    void *window;
    BOOL waitButton;
} FestMissionMsgWork;

static void FestMissionEvent_Init(FestMissionEventWork *work, GameSystem *gsys, Field *field);
static void FestMissionEvent_Exit(FestMissionEventWork *work);
static void FestMissionMsg_Call(GameSystem *gsys, GameEvent *parent, void *msgBGSys, MsgData *msgData, u16 messageId,
                                BOOL waitButton);
static void FestMissionMsg_CallNoWait(GameSystem *gsys, GameEvent *parent, void *msgBGSys, MsgData *msgData,
                                      u16 messageId);
static GameEventReturnCode FestMissionMsg_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFestMissionStart_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFestMissionMessage_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFestMissionMenu_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFestMissionInfo_Callback(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode EventFestMissionEnd_Callback(GameEvent *event, u32 *state, void *data);
static void FestMission_PrintLine(FestMissionEventWork *work, StrBuf *format, u16 x, u16 row);
static void FestMission_Print(FestMissionEventWork *work);
static void FestMission_PrintStart(FestMissionEventWork *work);
static void FestMission_PrintInfo(FestMissionEventWork *work);

GameEvent *EventFestMissionStart_Create(GameSystem *gsys, void *args) {
    FestMissionEventArgs *eventArgs = args;
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFestMissionStart_Callback, sizeof(FestMissionEventWork));
    FestMissionEventWork *work = GameEvent_GetData(event);

    FestMissionEvent_Init(work, gsys, field);
    work->mission = *(FestMission *)&eventArgs->config;
    work->joined = eventArgs->joined;
    return event;
}

GameEvent *EventFestMissionMessage_Create(GameSystem *gsys, void *args) {
    FestMissionEventArgs *eventArgs = args;
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFestMissionMessage_Callback, sizeof(FestMissionEventWork));
    FestMissionEventWork *work = GameEvent_GetData(event);

    FestMissionEvent_Init(work, gsys, field);
    work->mission = *(FestMission *)&eventArgs->config;
    work->messageType = eventArgs->messageType;
    func_020153b8(work->festival, eventArgs);
    return event;
}

GameEvent *EventFestMissionMenu_Create(GameSystem *gsys, void *args) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFestMissionMenu_Callback, sizeof(FestMissionEventWork));

    FestMissionEvent_Init(GameEvent_GetData(event), gsys, args);
    return event;
}

GameEvent *EventFestMissionInfo_Create(GameSystem *gsys, void *args) {
    FestMissionEventArgs *eventArgs = args;
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFestMissionInfo_Callback, sizeof(FestMissionEventWork));
    FestMissionEventWork *work = GameEvent_GetData(event);

    FestMissionEvent_Init(work, gsys, field);
    work->mission = *(FestMission *)&eventArgs->config;
    return event;
}

GameEvent *func_ov157_021f5ae4(GameSystem *gsys, void *args) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFestMissionEnd_Callback, sizeof(FestMissionEventWork));
    FestMissionEventWork *work = GameEvent_GetData(event);

    FestMissionEvent_Init(work, gsys, field);
    work->endType = 0;
    return event;
}

GameEvent *func_ov157_021f5b18(GameSystem *gsys, void *args) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFestMissionEnd_Callback, sizeof(FestMissionEventWork));
    FestMissionEventWork *work = GameEvent_GetData(event);

    FestMissionEvent_Init(work, gsys, field);
    work->endType = 1;
    return event;
}

static void FestMissionEvent_Init(FestMissionEventWork *work, GameSystem *gsys, Field *field) {
    work->heapId = HEAPID_GAMEEVENT;
    work->tailHeapId = HEAPID_TAIL(work->heapId);
    work->field = field;
    work->gsys = gsys;
    work->gameData = GSYS_GetGameData(gsys);
    work->msgBGSys = Field_GetMsgBGSys(work->field);
    work->actorSystem = Field_GetActorSystem(work->field);
    work->fesGimmick = Field_GetFesGimmick(work->field);
    work->festival = GSYS_GetLinkFestival(gsys);
    work->festText = getTextFileForFestMissions(work->heapId);
    work->passPowerData = PassPowerData_Create(work->heapId);
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_0412, work->heapId);
    work->wordSet = GFL_WordSetSystemCreate(6, 48, work->heapId);
    work->strbuf = GFL_StrBufCreate(161, work->heapId);
    work->formatted = GFL_StrBufCreate(161, work->heapId);
    DisableAllActorsMovement(work->actorSystem);
}

static void FestMissionEvent_Exit(FestMissionEventWork *work) {
    GFL_StrBufFree(work->formatted);
    GFL_StrBufFree(work->strbuf);
    GFL_WordSetSystemFree(work->wordSet);
    GFL_MsgDataFree(work->msgData);
    PassPowerData_Free(work->passPowerData);
    func_ov027_02170b00(work->festText);
    EnableAllActorsMovement(work->actorSystem);
}

// Shows a message in a chained event, which ends when it is printed or, with waitButton, when a button is pressed
static void FestMissionMsg_Call(GameSystem *gsys, GameEvent *parent, void *msgBGSys, MsgData *msgData, u16 messageId,
                                BOOL waitButton) {
    GameEvent *event = GameEvent_Create(gsys, NULL, FestMissionMsg_Callback, sizeof(FestMissionMsgWork));
    FestMissionMsgWork *work = GameEvent_GetData(event);

    work->waitButton = waitButton;
    work->window = func_ov036_02188498(msgBGSys, msgData, 19);
    func_ov036_02188538(work->window, 0, 0, messageId);
    GameEvent_ChainNext(parent, event);
}

static void FestMissionMsg_CallNoWait(GameSystem *gsys, GameEvent *parent, void *msgBGSys, MsgData *msgData,
                                      u16 messageId) {
    FestMissionMsg_Call(gsys, parent, msgBGSys, msgData, messageId, FALSE);
}

static GameEventReturnCode FestMissionMsg_Callback(GameEvent *event, u32 *state, void *data) {
    FestMissionMsgWork *work = data;

    switch (work->state) {
    case 0:
        if (func_ov036_021885bc(work->window)) {
            if (!work->waitButton) {
                break;
            }
            work->state++;
        }
        return GAMEEVENT_CONTINUE;
    case 1:
        if (!(GCTX_HIDGetPressedKeys() &
              (PAD_BUTTON_A | PAD_BUTTON_B | PAD_KEY_RIGHT | PAD_KEY_LEFT | PAD_KEY_UP | PAD_KEY_DOWN))) {
            return GAMEEVENT_CONTINUE;
        }
        break;
    }
    func_ov036_02188504(work->window);
    return GAMEEVENT_DONE;
}

// Counts the mission hosted or joined and gives its medals, plays the mission's fanfare and shows it, then, for a
// joined mission, takes the player back into the field
static GameEventReturnCode EventFestMissionStart_Callback(GameEvent *event, u32 *state, void *data) {
    FestMissionEventWork *work = data;
    SaveControl *save;
    void *records;
    MedalBox *medalBox;
    u16 joinedCount;
    u16 hostedCount;
    u32 zoneId;
    u32 bgm;
    GameEvent *next;

    switch (*state) {
    case 0:
        save = GameData_GetSaveControl(work->gameData);
        records = func_02010dec(save);
        medalBox = SaveControl_GetMedalBox(save);
        if (work->joined) {
            joinedCount = FestRecords_AddJoined(records);
            func_0201472c(work->festival, &work->mission, work->joined);
            work->mission = *(FestMission *)GetFestMissionCfg(work->festival);
            func_020153b8(work->festival, &work->mission);
            if (joinedCount >= 1) {
                MedalBox_GiveMedal(medalBox, MEDAL_MISSION_JOIN_1);
                if (joinedCount >= 10) {
                    MedalBox_GiveMedal(medalBox, MEDAL_MISSION_JOIN_10);
                }
            }
            next = EventBGMPlay_Create(work->gsys, SEQ_ME_MISSION_START);
        } else {
            hostedCount = FestRecords_AddHosted(records);
            if (hostedCount >= 10) {
                MedalBox_GiveMedal(medalBox, MEDAL_MISSION_HOST_10);
                if (hostedCount >= 50) {
                    MedalBox_GiveMedal(medalBox, MEDAL_MISSION_HOST_50);
                }
            }
            next = EventMEPlay_Create(work->gsys, SEQ_ME_MISSION_START);
        }
        GameEvent_ChainNext(event, next);
        func_020150dc(func_020146fc(work->festival), work->gameData, work->joined);
        func_02038bc8(7);
        func_02014844(work->festival, func_ov012_02160eb4(work->gameData, Field_GetPlayerStateZoneID(work->field)));
        work->window = func_ov036_02187ca0(work->msgBGSys, work->msgData, 1, 1, 30, 20);
        func_ov036_02187d38(work->window);
        FestMission_PrintStart(work);
        (*state)++;
        break;
    case 1:
        if (GFL_SndBGMIsPlaying()) {
            break;
        }
        zoneId = Field_GetPlayerStateZoneID(work->field);
        bgm = GetMapBGMIDByPlayerState2(work->gameData, zoneId, GameData_GetSeason(work->gameData));
        if (work->joined) {
            GameEvent_ChainNext(event, EventBGMPlay_Create(work->gsys, bgm));
        } else {
            GameEvent_ChainNext(event, EventPushBGMFinish_Create(work->gsys, 0, 6));
        }
        (*state)++;
        break;
    case 2:
        func_ov036_02187d38(work->window);
        func_ov036_02187d10(work->window);
        func_ov036_021b65e8(Field_GetFesGimmick(work->field));
        func_ov012_021683f4(work->gsys, Field_GetPlayerStateZoneID(work->field));
        if (!work->joined) {
            *state = 6;
            break;
        }
        GFL_SndSEPlay(SEQ_SE_W295_01);
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(work->gsys, work->field, 1, 0));
        (*state)++;
        break;
    case 3:
        EncountSystem_CancelPhenomenon(Field_GetEncountSystem(work->field));
        FieldSubscreen_ChangeImm(Field_GetSubscreen(work->field), 10);
        GameEvent_ChainNext(event, CreateFieldCloseEvent(work->gsys, work->field));
        (*state)++;
        break;
    case 4:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(work->gsys));
        (*state)++;
        break;
    case 5:
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(work->gsys, work->field, 1, 0, 1, 0, 0));
        (*state)++;
        break;
    case 6:
        if (work->joined) {
            func_02014774(work->festival, work->joined);
        }
        FestMissionEvent_Exit(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode EventFestMissionMessage_Callback(GameEvent *event, u32 *state, void *data) {
    FestMissionEventWork *work = data;

    switch (*state) {
    case 0:
        FestMissionMsg_Call(work->gsys, event, work->msgBGSys, work->msgData, work->messageType + 4, TRUE);
        (*state)++;
        break;
    case 1:
        func_020153b8(work->festival, &work->mission);
        func_02014964(work->festival, &work->mission);
        FestMissionEvent_Exit(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode EventFestMissionMenu_Callback(GameEvent *event, u32 *state, void *data) {
    FestMissionEventWork *work = data;
    u32 menuArgs;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, EventFieldEffect_Create(work->gsys, Field_Get3DCi(work->field), 0x69));
        (*state)++;
        break;
    case 1:
        FestMissionMsg_CallNoWait(work->gsys, event, work->msgBGSys, work->msgData, 1);
        (*state)++;
        break;
    case 2:
        menuArgs = 1;
        GameEvent_ChainNext(
            event, GameEvent_CreateOverlayDelegate(work->gsys, OVERLAY_EVENT_FESTIVAL, func_ov021_0216e80c, &menuArgs));
        (*state)++;
        break;
    default:
        FestMissionEvent_Exit(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode EventFestMissionInfo_Callback(GameEvent *event, u32 *state, void *data) {
    FestMissionEventWork *work = data;

    switch (*state) {
    case 0:
        FestMissionMsg_CallNoWait(work->gsys, event, work->msgBGSys, work->msgData, 2);
        (*state)++;
        break;
    case 1:
        work->window = func_ov036_02187ca0(work->msgBGSys, work->msgData, 1, 1, 30, 20);
        func_ov036_02187d38(work->window);
        FestMission_PrintInfo(work);
        (*state)++;
        break;
    case 2:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            (*state)++;
        }
        break;
    default:
        func_ov036_02187d38(work->window);
        func_ov036_02187d10(work->window);
        FestMissionEvent_Exit(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Says the mission is over, puts the Funfest's music back and reloads the field
static GameEventReturnCode EventFestMissionEnd_Callback(GameEvent *event, u32 *state, void *data) {
    FestMissionEventWork *work = data;
    u32 bgm;

    switch (*state) {
    case 0:
        FestMissionMsg_CallNoWait(work->gsys, event, work->msgBGSys, work->msgData, work->endType + 3);
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(work->gsys, work->field, 1, 0));
        (*state)++;
        break;
    case 2:
        func_ov036_021b6690(Field_GetFesGimmick(work->field));
        bgm = LinkFestival_GetNormalChangeBGMID(work->festival);
        if (bgm != (u32)-1) {
            GameEvent_ChainNext(event, EventBGMChange_Create(work->gsys, bgm, 60, 60));
        }
        (*state)++;
        break;
    case 3:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(work->gsys, work->field));
        (*state)++;
        break;
    case 4:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(work->gsys));
        (*state)++;
        break;
    case 5:
        FieldSubscreen_ChangeImm(Field_GetSubscreen(work->field), 0);
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(work->gsys, work->field, 1, 0, 1, 0, 0));
        (*state)++;
        break;
    default:
        if (FieldSnd_IsBusy(GameData_GetFieldSoundSystem(work->gameData))) {
            break;
        }
        FestMissionEvent_Exit(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void FestMission_PrintLine(FestMissionEventWork *work, StrBuf *format, u16 x, u16 row) {
    GFL_WordSetFormatStrbuf(work->wordSet, work->formatted, format);
    func_ov036_02187d28(work->window, x, row * 16 + 8, work->formatted);
}

// Prints the mission's name and description, the time it has left, how many it wants and its kind
static void FestMission_Print(FestMissionEventWork *work) {
    s32 seconds = func_02014dcc(&work->mission);
    s32 minutes;

    GFL_MsgDataLoadStrbuf(work->msgData, ScriptBank0412_Text_Empty, work->strbuf);
    func_ov027_02170d04(work->festText, work->formatted, &work->mission, work->tailHeapId);
    func_0202437c(work->wordSet, 0, work->formatted, 0, 1, 2);
    FestMission_PrintLine(work, work->strbuf, 0, 0);
    func_ov027_02170d90(work->festText, work->strbuf, &work->mission, work->heapId);
    FestMission_PrintLine(work, work->strbuf, 0, 2);
    minutes = seconds / 60;
    WordSetNumber(work->wordSet, 2, minutes, 4, NUM_PAD_NONE, TRUE);
    WordSetNumber(work->wordSet, 3, seconds % 60, 2, NUM_PAD_NONE, TRUE);
    WordSetNumber(work->wordSet, 4, work->mission.count, 4, NUM_PAD_NONE, TRUE);
    if (minutes == 0) {
        GFL_MsgDataLoadStrbuf(work->msgData, ScriptBank0412_Text_TimeRemainingSec, work->strbuf);
    } else {
        GFL_MsgDataLoadStrbuf(work->msgData, ScriptBank0412_Text_TimeRemainingMinSec, work->strbuf);
    }
    FestMission_PrintLine(work, work->strbuf, 8, 7);
    GFL_MsgDataLoadStrbuf(work->msgData, work->mission.unk10_6 + 13, work->strbuf);
    FestMission_PrintLine(work, work->strbuf, 8, 8);
}

static void FestMission_PrintStart(FestMissionEventWork *work) {
    FestMission_Print(work);
}

static void FestMission_PrintInfo(FestMissionEventWork *work) {
    FestMission_Print(work);
}
