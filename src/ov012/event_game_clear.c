#include "types.h"
#include "app/unova_link.h"
#include "constants/pokemon.h"
#include "constants/version.h"
#include "demo/the_end.h"
#include "field/encounter.h"
#include "field/event_3d_demo.h"
#include "field/event_game_clear.h"
#include "field/event_mapchange.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/field_script_event.h"
#include "field/subscreen.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/config.h"
#include "save/event_work.h"
#include "save/medal_box.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/version.h"

// Where the player returns to after the credits, in each version
static const VecFx32 sEndingPositionA = { 31 * 16 * FX32_ONE, 47 * 16 * FX32_ONE, 16 * FX32_ONE };
static const VecFx32 sEndingPositionB = { 43 * 16 * FX32_ONE, 16 * FX32_ONE, 757 * 16 * FX32_ONE };

GameEventReturnCode EventGameClear_Callback(GameEvent *event, u32 *state, void *data) {
    GameClearWork *work = data;
    GameSystem *gsys = work->gameSystem;
    GameData *gameData = work->gameData;
    SaveControl *save = GameData_GetSaveControl(gameData);
    Field *field = GSYS_GetField(work->gameSystem);
    GameCommSys *comm = GSYS_GetGameCommSystem(work->gameSystem);
    VecFx32 positionA;
    VecFx32 positionB;
    u32 enabled;

    switch (work->current) {
    case 0:
        func_ov012_0215a670(work);
        EventWork_FlagSet(GameData_GetEventWork(gameData), 0x9f8);
        EventGameClear_GiveMonotypeMedals(work);
        GFL_SndBGMFadeOut(30);
        EventGameClear_NextState(work, state);
        break;
    case 1:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0));
        EventGameClear_NextState(work, state);
        break;
    case 2:
        if (GameCommSys_BootCheck(comm)) {
            GameCommSys_ExitReq(comm);
        }
        EventGameClear_NextState(work, state);
        break;
    case 3:
        if (!GameCommSys_BootCheck(comm)) {
            EventGameClear_NextState(work, state);
        }
        break;
    case 4:
        GameEvent_ChainNext(event, EventFieldCloseKeepSound_Create(gsys, field));
        EventGameClear_NextState(work, state);
        break;
    case 5:
        GSYS_QueueProc(gsys, OVERLAY_ID(265), &HALL_OF_FAME_DEMO_PROC_FUNCTIONS, &work->ov265Param);
        EventGameClear_NextState(work, state);
        break;
    case 6:
        if (!GSYS_GetProcMgrState(gsys)) {
            EventGameClear_NextState(work, state);
        }
        break;
    case 7:
        GSYS_QueueProc(gsys, OVERLAY_STAFF_ROLL, &STAFF_ROLL_PROC_FUNCTIONS, &work->staffRollParam);
        EventGameClear_NextState(work, state);
        break;
    case 8:
        if (!GSYS_GetProcMgrState(gsys)) {
            EventGameClear_NextState(work, state);
        }
        break;
    case 9:
        GameEvent_ChainNext(event, Event3DDemo_Create(gsys, event, EventGameClear_Get3DDemoID(), 0, 0));
        EventGameClear_NextState(work, state);
        break;
    case 10:
        FieldScript_CallPlayerPostHOFSetup(gsys, 4);
        EventGameClear_NextState(work, state);
        break;
    case 11:
        if (!KeyInfo_IsKeyUnlocked(getKeyInfoSaveBlk(save), work->unovaLinkParam.key)) {
            GSYS_QueueProcAsEvent(event, OVERLAY_ID(332), &UNOVA_LINK_PROC_FUNCTIONS, &work->unovaLinkParam);
        }
        EventGameClear_NextState(work, state);
        break;
    case 12:
        GSYS_QueueProcAsEvent(event, OVERLAY_ID(295), &GAME_CLEAR_SAVE_PROC_FUNCTIONS, &work->ov295Param);
        EventGameClear_NextState(work, state);
        break;
    case 13:
        if (work->counter > 60) {
            work->counter = 0;
            EventGameClear_NextState(work, state);
        }
        work->counter++;
        break;
    case 14:
        GSYS_QueueProcAsEvent(event, OVERLAY_THE_END, &THE_END_PROC_FUNCTIONS, &work->ov267Param);
        GSYS_SetCGearOn(gsys, 0);
        EventGameClear_NextState(work, state);
        break;
    case 15:
        positionA = sEndingPositionA;
        GameEvent_ChainNext(event, EventMapChangeEnding_Create(gsys, field, 0x8b, &positionA, 1));
        EventGameClear_NextState(work, state);
        break;
    case 16:
        positionB = sEndingPositionB;
        GameEvent_ChainNext(event, EventMapChangeEnding_Create(gsys, field, 0x1ab, &positionB, 1));
        EventGameClear_NextState(work, state);
        break;
    case 17:
        enabled = GFL_BGSysGetEnabledBGsA();
        FieldG2D_SetLCDConfig();
        GFL_BGSysSetEnabledBGsA(enabled);
        FieldG2D_Prepare3DSurface(field);
        FieldSubscreen_ChangeImm(Field_GetSubscreen(field), 5);
        EventScriptCall_Start(event, 1, NULL, 0, 4);
        EventGameClear_NextState(work, state);
        break;
    case 18:
        enabled = GFL_BGSysGetEnabledBGsA();
        FieldG2D_SetLCDConfig();
        GFL_BGSysSetEnabledBGsA(enabled);
        FieldG2D_Prepare3DSurface(field);
        FieldSubscreen_ChangeImm(Field_GetSubscreen(field), 5);
        EventScriptCall_Start(event, 0x20, NULL, 0, 4);
        EventGameClear_NextState(work, state);
        break;
    case 19:
        GameEvent_ChainNext(event, EventFieldOpen_Create(gsys));
        EventGameClear_NextState(work, state);
        break;
    case 20:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        EventGameClear_NextState(work, state);
        break;
    case 21:
        GFL_SndBGMStop(0x3f8);
        EventGameClear_NextState(work, state);
        break;
    case 22:
        GFL_SndBGMPlay(0x4f4, 0xffff);
        EventGameClear_NextState(work, state);
        break;
    case 23:
        if (GFL_SndBGMIsPlaying() == TRUE) {
            GFL_SndBGMFadeOut(64);
        }
        EventGameClear_NextState(work, state);
        break;
    case 24:
        if (GFL_SndBGMIsFading() == TRUE) {
            break;
        }
        if (GFL_SndBGMIsPlaying() == TRUE) {
            GFL_SndBGMStopImmediate();
        }
        EventGameClear_NextState(work, state);
        break;
    case 25:
        EventWork_FlagReset(GameData_GetEventWork(gameData), 0x9f8);
        return GAMEEVENT_DONE;
    case 26:
        return GAMEEVENT_CONTINUE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventGameClear_Create(GameSystem *gsys, u32 param) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameClear_Callback, sizeof(GameClearWork));
    GameClearWork *work = GameEvent_GetData(event);

    work->gameSystem = gsys;
    work->gameData = gameData;
    work->unk08 = param;
    work->unk10 = GetGameDataPlayerInfo(gameData);
    work->counter = 0;
    work->ov295Param.gsys = gsys;
    work->ov295Param.param = param;
    EncountSave_SpawnPendingEntries(gameData);
    SetGameClearGameData(work);
    SetGameClearStaffRollParam(work);
    work->unovaLinkParam.gameData = gameData;
    work->unovaLinkParam.mode = UNOVA_LINK_MODE_GAME_CLEAR;
#ifdef BLACK2
    work->unovaLinkParam.key = KEY_SYSTEM_KEY_CHALLENGE;
#else
    work->unovaLinkParam.key = KEY_SYSTEM_KEY_EASY;
#endif
    SetGameClearStatusSequence(work);
    return event;
}

void SetGameClearGameData(GameClearWork *work) {
    work->ov265Param.party = GameData_GetParty(work->gameData);
    work->ov265Param.playerInfo = GetGameDataPlayerInfo(work->gameData);
    work->ov265Param.playTime = GameData_GetPlayTime(work->gameData);
}

void SetGameClearStaffRollParam(GameClearWork *work) {
    u32 value;

    work->staffRollParam.skippable = work->unk08 == 1;
    work->staffRollParam.playerInfo = GetGameDataPlayerInfo(work->gameData);
    value = Config_GetKanji((Config *)getTrainerDataBlkAddress(GameData_GetSaveControl(work->gameData)));
    work->staffRollParam.unk04 = value;
    work->ov267Param.unk00 = value;
}

void SetGameClearStatusSequence(GameClearWork *work) {
    unsigned int index = 0;

    work->states[1] = 2;
    work->states[2] = 3;
    work->states[3] = 4;
    work->states[4] = 5;
    work->states[5] = 6;
    work->states[0] = index;
    index += 6;
    if (work->unk08 == 0) {
        work->states[index++] = 11;
        work->states[index++] = 13;
    }
    work->states[index + 0] = 10;
    work->states[index + 1] = 12;
    work->states[index + 2] = 13;
    work->states[index + 3] = 19;
    work->states[index + 4] = 15;
    work->states[index + 5] = 21;
    work->states[index + 6] = 17;
    work->states[index + 7] = 4;
    work->states[index + 8] = 7;
    work->states[index + 9] = 8;
    work->states[index + 10] = 19;
    work->states[index + 11] = 16;
    work->states[index + 12] = 18;
    work->states[index + 13] = 23;
    work->states[index + 14] = 24;
    work->states[index + 15] = 4;
    work->states[index + 16] = 22;
    work->states[index + 17] = 14;
    work->states[index + 18] = 25;
    work->current = work->states[0];
}

u32 EventGameClear_Get3DDemoID(void) {
    switch (getGameVersion()) {
    default:
    case VERSION_WHITE2:
        return 7;
    case VERSION_BLACK2:
        return 6;
    }
}

void EventGameClear_NextState(GameClearWork *work, u32 *state) {
    ++*state;
    work->current = work->states[*state];
}

void func_ov012_0215a670(GameClearWork *work) {
    TrainerCard_SetHallOfFameMinutes(getTrainerCardDataBlkAddress(work->gameData), 0x5a0);
}

void EventGameClear_GiveMonotypeMedals(GameClearWork *work) {
    GameData *gameData = GSYS_GetGameData(work->gameSystem);
    SaveControl *save = GameData_GetSaveControl(gameData);
    MedalBox *box = SaveControl_GetMedalBox(save);
    PokeParty *party;
    int count;
    int eligible;
    int i;
    u32 typeCounts[17];

    MedalBox_DiscoverMedal(box, 0x56);
    MedalBox_DiscoverMedal(box, 0x5a);
    party = GameData_GetParty(gameData);
    count = PokeParty_GetPkmCount(party);
    eligible = 0;
    u16 medalIds[17] = { 236, 242, 245, 243, 244, 248, 247, 249, 252, 237, 238, 240, 239, 246, 241, 250, 251 };
    MI_CpuFill8(typeCounts, 0, sizeof(typeCounts));
    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);
        u32 type1;
        u32 type2;
        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 1) {
            continue;
        }
        type1 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE1, NULL);
        typeCounts[type1]++;
        type2 = PokeParty_GetParam(pkm, PKM_PARAM_TYPE2, NULL);
        if (type1 != type2) {
            typeCounts[type2]++;
        }
        eligible++;
    }
    for (i = 0; i < 17; i++) {
        if (eligible == typeCounts[i]) {
            MedalBox_GiveMedal(box, medalIds[i]);
        }
    }
}

GameEvent *CallCreateGameEntryPointEvent(GameSystem *gsys, GameSystemProcData *procData) {
    GFL_OvlLoad(OVERLAY_ID(35));
    return CreateGameEntryPointEvent(gsys, procData);
}

GameEvent *EventMapChangeBlackout_CreateExternal(GameSystem *gsys) {
    GFL_OvlLoad(OVERLAY_ID(35));
    return EventMapChangeBlackout_Create(gsys);
}

void LoadFieldGlueOverlay(void) {
    GFL_OvlLoad(OVERLAY_ID(35));
}

void UnloadFieldGlueOverlay(void) {
    GFL_OvlUnload(OVERLAY_ID(35));
}
