#include "types.h"
#include "constants/arc.h"
#include "constants/flags.h"
#include "constants/sound.h"
#include "constants/vars.h"
#include "constants/zones.h"
#include "dsprot/dsprot.h"
#include "field/event_3d_demo.h"
#include "field/event_data.h"
#include "field/event_mapchange.h"
#include "field/event_season_banner.h"
#include "field/event_sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/field_lens_flare.h"
#include "field/field_map.h"
#include "field/field_script.h"
#include "field/field_sound.h"
#include "field/field_status.h"
#include "field/intrude_work.h"
#include "field/player_state.h"
#include "field/shaymin_form.h"
#include "field/union_main.h"
#include "field/zone.h"
#include "field/zone_change.h"
#include "gfl/arc.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_system.h"
#include "gfl/overlay.h"
#include "gfl/rtc_cache.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "pml/poke_party.h"
#include "save/adventure.h"
#include "save/dream_world.h"
#include "save/event_work.h"
#include "save/join_avenue.h"
#include "save/key_info.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "struct_decls.h"
#include "system/area_data.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/iss_switch_sys.h"
#include "system/iss_sys.h"
#include "system/new_game.h"
#include "system/playtime_ctrl.h"
#include "system/resort_work.h"
#include "system/season.h"
#include "system/zone_weather.h"

struct EventGameOpening {
    GameSystem *gsys;
    GameSystemProcData *procData;
};

struct EventFieldFirst {
    GameSystem *gsys;
    GameData *gameData;
    ZoneSpawnInfo spawn;
};

struct EventFieldContinue {
    GameSystem *gsys;
    GameData *gameData;
    u16 zoneId;
    BOOL continueFromSave;
};

struct EventMapChange {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    u16 zoneId;
    ZoneSpawnInfo spawn;
    u32 unk2C;
    u8 mode;
    VecFx32 unk34;
    BOOL unk40;
    BOOL seasonChanged;
    u16 prevSeason;
    u16 season;
    WarpSequence warp;
    u32 unk9C;
    BOOL lensFlareStarted;
};

struct EventMapChangeCore {
    EventMapChange *mapChange;
};

struct EventMapChangeBlackout {
    GameSystem *gsys;
    GameData *gameData;
    ZoneSpawnInfo spawn;
};

struct ZoneGimmick {
    u32 zoneId;
    u16 gimmickId;
};

struct EventEntralinkWarp {
    ZoneSpawnInfo spawn;
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    u32 unk28;
    u8 unk2C;
    u32 festMissionStatus;
};

GameEventReturnCode EventEntralinkWarp_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventEntralinkWarpIn_Callback(GameEvent *event, u32 *state, void *data);
void *EventMapChange_DSProtNop(void *arg0, void *arg1);
void *EventMapChange_DSProtTamper1(void *arg0, void *arg1);
void *EventMapChange_DSProtTamper2(void *arg0, void *arg1);

GameEventReturnCode EventGameOpening_Callback(GameEvent *event, u32 *state, void *data);
void EventFieldFirst_SetupCity(GameSystem *gsys);
GameEventReturnCode EventFieldFirst_Callback(GameEvent *event, u32 *state, void *data);
void EventFieldContinue_SetupCity(GameSystem *gsys);
GameEventReturnCode EventFieldContinue_Callback(GameEvent *event, u32 *state, void *data);
void EventMapChange_LoadSeasons(EventMapChange *wk);
void EventMapChange_SetupWarpSequenceOut(EventMapChange *wk, GameEvent *parent);
void EventMapChange_SetupWarpSequenceIn(EventMapChange *wk, GameEvent *parent);
GameEventReturnCode EventMapChangeCore_Callback(GameEvent *event, u32 *state, void *data);
GameEvent *EventMapChangeCore_Create(EventMapChange *wk, u8 mode);
GameEventReturnCode EventMapChangeWarp_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventMapChange_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventMapChangeEnding_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventMapChangeFakeWarp_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventMapChangeQuicksand_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventMapChangeEscapeRope_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventMapChangeDig_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventMapChangeTeleport_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventMapChangeDiveOut_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventMapChangeDiveIn_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventMapChangeWarpPad_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventMapChangeUnionRoomExit_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventUnionRoomWarp_Callback(GameEvent *event, u32 *state, void *data);
void InitMapChangeEvent(EventMapChange *wk, GameSystem *gsys);
GameEventReturnCode EventMapChangeBlackout_Callback(GameEvent *event, u32 *state, void *data);

static inline void ClearLCDCVram(void) {
    gfxSetLCDCBanks(0x1ff);
    sys_memset32_fast(0, (void *)0x06800000, 0xa4000);
    gfxDisableLCDCBanks();
}

GameEvent *CreateGameEntryPointEvent(GameSystem *gsys, GameSystemProcData *procData) {
    switch (procData->entryPoint) {
    case GAME_ENTRYPOINT_OPENING:
        return EventGameOpening_Create(gsys, procData);
    case GAME_ENTRYPOINT_FIELD_CONTINUE:
        return EventFieldContinue_Create(gsys, procData);
    case GAME_ENTRYPOINT_DEBUG:
        return EventFieldFirst_Create(gsys, procData);
    }
}

GameEventReturnCode EventGameOpening_Callback(GameEvent *event, u32 *state, void *data) {
    EventGameOpening *wk = data;
    GameSystem *gsys = wk->gsys;
    GameData *gameData = GSYS_GetGameData(gsys);
    u8 season;

    switch (*state) {
    case 0:
        ClearLCDCVram();
        (*state)++;
        break;
    case 1:
        Season_Set(gameData, Season_GetRealTime());
        season = GameData_GetSeason(gameData);
        GameEvent_ChainNext(event, EventSeasonBanner_CreateStandalone(gsys, season, season));
        (*state)++;
        break;
    case 2:
        GameEvent_ChainNext(event, Event3DDemo_Create(gsys, event, 5, 0, 1));
        (*state)++;
        break;
    case 3:
        GameEvent_Replace(event, EventFieldFirst_Create(gsys, wk->procData));
        break;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventGameOpening_Create(GameSystem *gsys, GameSystemProcData *procData) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameOpening_Callback, sizeof(EventGameOpening));
    EventGameOpening *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->procData = procData;
    return event;
}

void EventFieldFirst_SetupCity(GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    CityState *city = GameData_GetMyCityState(gameData);
    PlayerInfo *player = GetGameDataPlayerInfo(gameData);

    CityState_InitFromSave(city, player, GameData_GetSaveControl(gameData), 1);
    func_ov012_0215cd58(GameData_GetMyCityState(gameData));
}

GameEventReturnCode EventFieldFirst_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldFirst *wk = data;
    GameSystem *gsys = wk->gsys;
    GameData *gameData = GSYS_GetGameData(gsys);

    GameData_GetEventWork(gameData);
    switch (*state) {
    case 0:
        FieldScript_CallPlayerInitSetup(gsys, 1);
        FieldMapControl_LoadZone(gsys, wk->spawn.zoneId);
        FieldMapControl_InitSpawn(gsys, &wk->spawn);
        GameBeacon_SetZone(wk->spawn.zoneId, gameData);
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(
            event,
            EventBGMChange_Create(
                gsys, GetMapBGMIDByPlayerState2(gameData, wk->spawn.zoneId, GameData_GetSeason(gameData)), 0, 60));
        (*state)++;
        break;
    case 2:
        GameEvent_ChainNext(event, EventFieldOpen_Create(gsys));
        (*state)++;
        break;
    case 3:
        SetActorFlag(FieldPlayer_GetActor(Field_GetPlayer(GSYS_GetField(gsys))), 4);
        EventScriptCall_Start(event, 0x1d, NULL, 0, HEAPID_USER);
        (*state)++;
        break;
    case 4:
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, GSYS_GetField(gsys), 0, 0, 1, 0, 0));
        (*state)++;
        break;
    case 5:
        EventScriptCall_Start(event, 0x1e, NULL, 0, HEAPID_USER);
        (*state)++;
        break;
    case 6:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventFieldFirst_Create(GameSystem *gsys, GameSystemProcData *procData) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldFirst_Callback, sizeof(EventFieldFirst));
    EventFieldFirst *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->gameData = GSYS_GetGameData(gsys);
    LoadAspertiaCitySpawnInfo(&wk->spawn);

    GFL_OvlLoad(OVERLAY_NEW_GAME);
    InitDreamRadarFlagSave(GSYS_GetGameData(gsys), HEAPID_USER);
    GFL_OvlUnload(OVERLAY_NEW_GAME);

    if (procData->entryPoint == GAME_ENTRYPOINT_OPENING) {
        GFL_OvlLoad(OVERLAY_NEW_GAME);
        InitItemBag(GSYS_GetGameData(gsys), HEAPID_USER);
        GFL_OvlUnload(OVERLAY_NEW_GAME);
    }

    EventFieldFirst_SetupCity(gsys);
    setAdvTimeBlkRtcOffsetOwnerMacBdayMonthDay(getSaveAdventureDataBlk(GameData_GetSaveControl(wk->gameData)));
    GameSystemTimer_Start();
    ClearLCDCVram();
    return event;
}

void EventFieldContinue_SetupCity(GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    CityState *city = GameData_GetMyCityState(gameData);
    PlayerInfo *player = GetGameDataPlayerInfo(gameData);

    CityState_InitFromSave(city, player, GameData_GetSaveControl(gameData), 1);
    func_ov012_0215cd58(GameData_GetMyCityState(gameData));
}

GameEventReturnCode EventFieldContinue_Callback(GameEvent *event, u32 *state, void *data) {
    EventFieldContinue *wk = data;
    GameSystem *gsys = wk->gsys;
    GameData *gameData = GSYS_GetGameData(gsys);
    EventWork *eventWork = GameData_GetEventWork(gameData);

    switch (*state) {
    case 0:
        FieldStatus_SetContinueFlag(GameData_GetFieldStatus(gameData), TRUE);
        if (wk->continueFromSave) {
            ZoneSpawnInfo *next = GameData_GetNextZone(gameData);

            wk->zoneId = next->zoneId;
            FieldMapControl_DeleteAllActors(gsys);
            FieldMapControl_LoadZone(gsys, wk->zoneId);
            GameData_UpdatePartyForTimeOfDay(gameData);
            FieldMapControl_InitSpawn(gsys, next);
            GameBeacon_SetZone(next->zoneId, gameData);
        } else {
            s32 cacheIdx;
            MMSys *mmSys;

            FieldMapControl_LoadZone(gsys, wk->zoneId);
            GameData_UpdatePartyForTimeOfDay(gameData);
            func_ov012_02162f44(gameData);
            func_ov012_0215ef24(gameData, wk->zoneId);
            UpdateWeatherToDefault(gameData, wk->zoneId);
            cacheIdx = GetZoneNPCInfoCacheIdx(wk->zoneId);
            mmSys = GameData_GetMMSys(gameData);
            if (cacheIdx < 24) {
                LoadMModelSystemInfoCache(mmSys, cacheIdx);
            } else {
                FldActSys_ClearCache(mmSys);
            }
        }
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(
            event, EventBGMChange_Create(
                       gsys, GetMapBGMIDByPlayerState2(gameData, wk->zoneId, GameData_GetSeason(gameData)), 0, 60));
        (*state)++;
        break;
    case 2:
        GameEvent_ChainNext(event, EventFieldOpen_Create(gsys));
        (*state)++;
        break;
    case 3:
        if (wk->continueFromSave && *EventWork_GetWkPtr(eventWork, EVENT_WORK_CONTINUE_SCRIPT) != 0) {
            EventScriptCall_Start(event, 0x83b, NULL, 0, HEAPID_FIELDMAP);
        } else {
            u8 season = GameData_GetSeason(gameData);
            GameEvent_ChainNext(event,
                                CallFieldMapEntranceInTransition(gsys, GSYS_GetField(gsys), 3, 0, 0, season, season));
        }
        (*state)++;
        break;
    case 4: {
        Field *field = GSYS_GetField(gsys);

        if (Field_GetPlaceName(field) != NULL) {
            BeginContinuePlaceNameDisp(Field_GetPlaceName(field), wk->zoneId);
        }
        return GAMEEVENT_DONE;
    }
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventFieldContinue_Create(GameSystem *gsys, GameSystemProcData *procData) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventFieldContinue_Callback, sizeof(EventFieldContinue));
    EventFieldContinue *wk = GameEvent_GetData(event);
    EventWork *eventWork;
    SaveControl *save;
    AdventureSave *adventure;
    PokeParty *party;
    TrainerCardSave *trainerCard;

    wk->gsys = gsys;
    wk->gameData = GSYS_GetGameData(gsys);
    wk->zoneId = procData->zoneId;
    eventWork = GameData_GetEventWork(wk->gameData);
    wk->continueFromSave = EventWork_FlagGet(eventWork, EVENT_FLAG_CONTINUE_SCRIPT);
    EventWork_FlagReset(eventWork, EVENT_FLAG_CONTINUE_SCRIPT);

    save = GameData_GetSaveControl(wk->gameData);
    adventure = getSaveAdventureDataBlk(save);
    party = SaveControl_GetPokePartySave(save);
    trainerCard = getTrainerCardDataBlkAddress(wk->gameData);
    if (!hasClockNotBeenTampered(adventure)) {
        setNewDayForCountdown(getSaveAdventureTimeBlock(save));
        func_ov012_02164428(wk->gameData, party);
        setSecondsCurrentTimeInTrainerCard(trainerCard, RTC_ConvertSecondsCached());
    }
    setAdvTimeBlkRtcOffsetOwnerMacBdayMonthDay(adventure);
    TransformVsPokePartyBySeason(wk->gameData, party, GameData_GetSeason(wk->gameData));

    EventFieldContinue_SetupCity(gsys);
    GameSystemTimer_Start();
    ClearLCDCVram();
    return event;
}

void EventMapChange_LoadSeasons(EventMapChange *wk) {
    u16 prevSeason;
    u16 season;
    HeapID heapId;
    AreaData *areaData;
    BOOL isExterior;

    GameData_GetSeasons(wk->gameData, &prevSeason, &season);
    heapId = Field_GetHeapID(wk->field);
    areaData = AreaData_Create(heapId, ZoneData_GetAreaID(wk->spawn.zoneId), 0);
    isExterior = FALSE;
    if (AreaData_IsExterior(areaData)) {
        isExterior = TRUE;
    }
    AreaData_Free(areaData);

    if (season != prevSeason && isExterior) {
        wk->seasonChanged = TRUE;
        wk->prevSeason = prevSeason;
        wk->season = season;
    }
}

void EventMapChange_SetupWarpSequenceOut(EventMapChange *wk, GameEvent *parent) {
    WarpSequence *warp = &wk->warp;

    warp->gsys = wk->gsys;
    warp->gameData = wk->gameData;
    warp->field = wk->field;
    warp->unk10 = wk->unk2C;
    warp->parent = parent;
    warp->zoneId = wk->zoneId;
    warp->spawn = wk->spawn;
    warp->outTransition = GetOutTransitionTypeBetweenZones(wk->zoneId, wk->spawn.zoneId);
    warp->inTransition = GetInTransitionTypeBetweenZones(wk->zoneId, wk->spawn.zoneId);
    warp->seasonChanged = (wk->unk40 && wk->seasonChanged) ? TRUE : FALSE;
    warp->startSeason = Season_GetNext(wk->prevSeason);
    warp->endSeason = wk->season;
    warp->unk48 = 0;
    warp->unk4C = 0;
}

void EventMapChange_SetupWarpSequenceIn(EventMapChange *wk, GameEvent *parent) {
    WarpSequence *warp = &wk->warp;

    warp->gsys = wk->gsys;
    warp->gameData = wk->gameData;
    warp->field = wk->field;
    warp->unk10 = wk->unk2C;
    warp->parent = parent;
    warp->zoneId = wk->zoneId;
    warp->spawn = wk->spawn;
    warp->outTransition = GetOutTransitionTypeBetweenZones(wk->zoneId, wk->spawn.zoneId);
    warp->inTransition = GetInTransitionTypeBetweenZones(wk->zoneId, wk->spawn.zoneId);
    warp->seasonChanged = (wk->unk40 && wk->seasonChanged) ? TRUE : FALSE;
    warp->startSeason = Season_GetNext(wk->prevSeason);
    warp->endSeason = wk->season;
    if (wk->spawn.changeType == ZONE_SPAWN_CHANGE_TYPE_POSITION) {
        warp->transitionType = 0;
    } else {
        warp->transitionType =
            GetWarpTransitionType(GetZoneWarpByID(GameData_GetEventData(wk->gameData), wk->spawn.warpId));
    }
}

GameEventReturnCode EventMapChangeCore_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChangeCore *core = data;
    EventMapChange *wk = core->mapChange;
    GameSystem *gsys = wk->gsys;
    GameData *gameData = wk->gameData;
    Field *field = wk->field;
    GameCommSys *comm = GSYS_GetGameCommSystem(gsys);

    switch (*state) {
    case 0: {
        FieldLensFlare *lensFlare;
        u32 fog;

        FieldStatus_SetBusyFlag(GameData_GetFieldStatus(gameData), 2);
        lensFlare = Field_GetLensFlare(field);
        fog = GetZoneFogIndexAll(field, wk->spawn.zoneId);
        FieldLensFlare_DecideForZoneTransit(lensFlare, wk->spawn.zoneId, Field_GetPlayerStateZoneID(field), fog);
        GameEvent_ChainNext(event, EventFieldCloseKeepSound_Create(gsys, field));
        (*state)++;
        break;
    }
    case 1:
        GFL_OvlLoad(OVERLAY_DSPROT);
        func_ov337_02180bdc();
        if (IsZoneGameCommDisabled(wk->spawn.zoneId) == TRUE) {
            u8 status = GameCommSys_BootCheck(comm);
            if (status == 1 || status == 2) {
                GameCommSys_ExitReq(comm);
                *state = 2;
                break;
            }
        }
        *state = 3;
        break;
    case 2:
        if (!GameCommSys_BootCheck(comm)) {
            *state = 3;
        }
        break;
    case 3:
        DSPROT_CHECKED_CALL(func_ov337_02180a84, EventMapChange_DSProtNop, EventMapChange_DSProtTamper1, wk, gsys);
        FieldMapControl_DeleteAllActors(gsys);
        if (wk->unk40 && wk->seasonChanged) {
            AdventureTime *adventureTime;
            u8 season;
            PokeParty *party;

            Season_Set(gameData, wk->season);
            adventureTime = getSaveAdventureTimeBlock(GameData_GetSaveControl(gameData));
            season = GameData_GetSeason(gameData);
            party = GameData_GetParty(gameData);
            TransformVsPokePartyBySeason(gameData, party, season);
            func_ov012_021643f0(gameData, party, &adventureTime->time, season);
        }
        DSPROT_CHECKED_CALL(func_ov337_02180b30, EventMapChange_DSProtNop, EventMapChange_DSProtTamper2, wk, gsys);
        FieldMapControl_LoadZone(gsys, wk->spawn.zoneId);
        GameData_UpdateZoneChangeFlag(gameData, wk->spawn.zoneId, wk->zoneId);
        FieldMapControl_InitSpawn(gsys, &wk->spawn);
        if (wk->mode != 4) {
            GameBeacon_SetZone(wk->spawn.zoneId, gameData);
        }
        switch (wk->mode) {
        case 1:
            func_ov012_0215ee94(gameData, wk->spawn.zoneId);
            break;
        case 2:
            func_ov012_0215eedc(gameData, wk->spawn.zoneId);
            break;
        case 3:
            func_ov012_0215eeb8(gameData, wk->spawn.zoneId);
            break;
        }
        if (wk->mode != 0) {
            ShutdownFollowWork(wk->gameData);
        }
        GFL_OvlUnload(OVERLAY_DSPROT);
        func_ov012_02153668(comm);
        (*state)++;
        break;
    case 4:
        GameEvent_ChainNext(event, EventFieldOpen_Create(gsys));
        (*state)++;
        break;
    case 5: {
        Field *currentField = GSYS_GetField(gsys);

        if (GameData_IsLensFlareRequested(gameData)) {
            wk->lensFlareStarted = FALSE;
            GameData_SetLensFlareRequested(gameData, FALSE);
        }
        if (!wk->lensFlareStarted) {
            FieldLensFlare_RequestStart(Field_GetLensFlare(currentField));
        }
        GameData_InitEncountTerrain(gameData, currentField);
        GameEvent_ChainNext(event, EventWaitFieldSound_Create(gsys));
        (*state)++;
        break;
    }
    case 6:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventMapChangeCore_Create(EventMapChange *wk, u8 mode) {
    GameEvent *event = GameEvent_Create(wk->gsys, NULL, EventMapChangeCore_Callback, sizeof(EventMapChangeCore));
    EventMapChangeCore *core = GameEvent_GetData(event);

    core->mapChange = wk;
    wk->mode = mode;
    return event;
}

GameEventReturnCode EventMapChangeWarp_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    Field *field = wk->field;

    switch (*state) {
    case 0:
        EventMapChange_LoadSeasons(wk);
        DisableAllActorsMovement(Field_GetActorSystem(field));
        EventMapChange_SetupWarpSequenceOut(wk, event);
        GameEvent_ChainNext(event, EventWarpSequence_CreateOut(&wk->warp));
        (*state)++;
        return GAMEEVENT_CONTINUE_DIRECT;
    case 1:
        GameEvent_ChainNext(event, EventMapChangeCore_Create(wk, 0));
        (*state)++;
        break;
    case 2:
        EventMapChange_SetupWarpSequenceIn(wk, event);
        GameEvent_ChainNext(event, EventWarpSequence_CreateIn(&wk->warp));
        (*state)++;
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventMapChange_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;
    GameData *gameData = wk->gameData;
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, CallEventPrepareResidentActorsForZoneChange(gsys, field));
        (*state)++;
        break;
    case 1:
        FieldSnd_SetZoneBGM(fieldSound, gameData, wk->spawn.zoneId, wk->season);
        FieldSnd_FadeInImmediate(fieldSound, gameData);
        (*state)++;
        break;
    case 2:
        GameEvent_ChainNext(event, EventMapChangeCore_Create(wk, wk->mode));
        (*state)++;
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventMapChangeEnding_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, CallEventPrepareResidentActorsForZoneChange(gsys, field));
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, EventMapChangeCore_Create(wk, wk->mode));
        (*state)++;
        break;
    case 2:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventMapChangeFakeWarp_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;

    switch (*state) {
    case 0:
        EventMapChange_LoadSeasons(wk);
        GameEvent_ChainNext(event, CallEventPrepareResidentActorsForZoneChange(gsys, field));
        (*state)++;
        break;
    case 1:
        if (wk->unk40 && wk->seasonChanged) {
            GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(gsys, field, 0, 0));
        } else {
            GameEvent_ChainNext(event,
                                CallFieldMapEntranceOutTransitionDefault(
                                    gsys, field, GetOutTransitionTypeBetweenZones(wk->zoneId, wk->spawn.zoneId), 0));
        }
        (*state)++;
        break;
    case 2:
        GameEvent_ChainNext(event, EventMapChangeCore_Create(wk, wk->mode));
        (*state)++;
        break;
    case 3:
        if (wk->unk40 && wk->seasonChanged) {
            GameEvent_ChainNext(event,
                                CallFieldMapEntranceInTransition(gsys, field, 3, 0, 0, wk->prevSeason, wk->season));
        } else {
            GameEvent_ChainNext(
                event, CallFieldMapEntranceInTransition(
                           gsys, field, GetInTransitionTypeBetweenZones(wk->zoneId, wk->spawn.zoneId), 0, 1, 0, 0));
        }
        (*state)++;
        break;
    case 4:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventMapChangeQuicksand_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    GameSystem *gsys = wk->gsys;
    GameData *gameData = wk->gameData;
    Field *field = wk->field;
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, CallEventPrepareResidentActorsForZoneChange(gsys, field));
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, EventQuicksandDrawIn_Create(event, gsys, field, &wk->unk34));
        (*state)++;
        break;
    case 2:
        FieldSnd_SetZoneBGM(fieldSound, gameData, wk->spawn.zoneId, wk->season);
        FieldSnd_FadeInImmediate(fieldSound, gameData);
        (*state)++;
        break;
    case 3:
        GameEvent_ChainNext(event, EventMapChangeCore_Create(wk, 0));
        (*state)++;
        break;
    case 4:
        GameEvent_ChainNext(event, EventQuicksandArrive_Create(event, gsys, field));
        (*state)++;
        break;
    case 5:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventMapChangeEscapeRope_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    GameSystem *gsys = wk->gsys;
    GameData *gameData = wk->gameData;
    Field *field = wk->field;
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);

    switch (*state) {
    case 0:
        EventMapChange_LoadSeasons(wk);
        GameEvent_ChainNext(event, CallEventPrepareResidentActorsForZoneChange(gsys, field));
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, EventEscapeRope_Create(event, gsys, field, wk->seasonChanged));
        (*state)++;
        break;
    case 2:
        GameData_AdjustPlayerStateOnDiveOut(gameData);
        FieldSnd_SetZoneBGM(fieldSound, gameData, wk->spawn.zoneId, wk->season);
        FieldSnd_FadeInImmediate(fieldSound, gameData);
        (*state)++;
        break;
    case 3:
        GameEvent_ChainNext(event, EventMapChangeCore_Create(wk, 2));
        (*state)++;
        break;
    case 4:
        GameEvent_ChainNext(event,
                            func_ov036_021b95ac(event, gsys, field, wk->seasonChanged, wk->prevSeason, wk->season));
        (*state)++;
        break;
    case 5:
        func_ov036_021b50c8(Field_GetPlaceName(field), wk->spawn.zoneId);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventMapChangeDig_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    GameSystem *gsys = wk->gsys;
    GameData *gameData = wk->gameData;
    Field *field = wk->field;
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);

    switch (*state) {
    case 0:
        EventMapChange_LoadSeasons(wk);
        GameEvent_ChainNext(event, CallEventPrepareResidentActorsForZoneChange(gsys, field));
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, EventDig_Create(event, gsys, field, wk->seasonChanged));
        (*state)++;
        break;
    case 2:
        GameData_AdjustPlayerStateOnDiveOut(gameData);
        FieldSnd_SetZoneBGM(fieldSound, gameData, wk->spawn.zoneId, wk->season);
        FieldSnd_FadeInImmediate(fieldSound, gameData);
        (*state)++;
        break;
    case 3:
        GameEvent_ChainNext(event, EventMapChangeCore_Create(wk, 2));
        (*state)++;
        break;
    case 4:
        GameEvent_ChainNext(event,
                            func_ov036_021b95e0(event, gsys, field, wk->seasonChanged, wk->prevSeason, wk->season));
        (*state)++;
        break;
    case 5:
        func_ov036_021b50c8(Field_GetPlaceName(field), wk->spawn.zoneId);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventMapChangeTeleport_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    GameSystem *gsys = wk->gsys;
    GameData *gameData = wk->gameData;
    Field *field = wk->field;
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, CallEventPrepareResidentActorsForZoneChange(gsys, field));
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, EventTeleportEffect_Create(event, gsys, field, TRUE));
        (*state)++;
        break;
    case 2:
        SetPlayerSpecialState(GameData_GetPlayerState(gameData), 0);
        FieldSnd_SetZoneBGM(fieldSound, gameData, wk->spawn.zoneId, wk->season);
        FieldSnd_FadeInImmediate(fieldSound, gameData);
        (*state)++;
        break;
    case 3:
        GameEvent_ChainNext(event, EventMapChangeCore_Create(wk, 3));
        (*state)++;
        break;
    case 4:
        GameEvent_ChainNext(event, func_ov036_021b9614(event, gsys, field));
        (*state)++;
        break;
    case 5:
        func_ov036_021b50c8(Field_GetPlaceName(field), wk->spawn.zoneId);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventMapChangeDiveOut_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;
    FieldTaskManager *taskManager = Field_GetTaskManager(field);

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(gsys, field, 1, 0));
        (*state)++;
        break;
    case 1:
        if (FieldTaskManager_IsIdle(taskManager)) {
            GameEvent_ChainNext(event, CallEventPrepareResidentActorsForZoneChange(gsys, field));
            (*state)++;
        }
        break;
    case 2:
        GameEvent_ChainNext(event, EventMapChangeCore_Create(wk, 0));
        (*state)++;
        break;
    case 3:
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, field, 1, 0, 1, 0, 0));
        (*state)++;
        break;
    case 4:
        func_ov036_021b50c8(Field_GetPlaceName(field), wk->spawn.zoneId);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventMapChangeDiveIn_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, CallEventPrepareResidentActorsForZoneChange(gsys, field));
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, CallFieldMapEntranceOutTransitionDefault(gsys, field, 0, 0));
        (*state)++;
        break;
    case 2: {
        GameEvent *mapChange = GameEvent_Create(gsys, NULL, EventMapChange_Callback, sizeof(EventMapChange));
        EventMapChange *mapChangeWk = GameEvent_GetData(mapChange);

        *mapChangeWk = *wk;
        GameEvent_ChainNext(event, mapChange);
        (*state)++;
        break;
    }
    case 3:
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, field, 0, 0, 1, 0, 0));
        (*state)++;
        break;
    case 4:
        func_ov036_021b50c8(Field_GetPlaceName(field), wk->spawn.zoneId);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventMapChangeWarpPad_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    GameSystem *gsys = wk->gsys;
    GameData *gameData = wk->gameData;
    Field *field = wk->field;
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, CallEventPrepareResidentActorsForZoneChange(gsys, field));
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, func_ov036_021b9df8(event, gsys, field));
        (*state)++;
        break;
    case 2:
        FieldSnd_SetZoneBGM(fieldSound, gameData, wk->spawn.zoneId, wk->season);
        FieldSnd_FadeInImmediate(fieldSound, gameData);
        GameEvent_ChainNext(event, EventMapChangeCore_Create(wk, 0));
        (*state)++;
        break;
    case 3:
        GameEvent_ChainNext(event, EventPlayerSpinDown_Create(event, gsys, field));
        (*state)++;
        break;
    case 4:
        func_ov036_021b50c8(Field_GetPlaceName(field), wk->spawn.zoneId);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventMapChangeUnionRoomExit_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;
    GameData *gameData = wk->gameData;

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event, EventTeleportEffect_Create(event, gsys, field, FALSE));
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, EventMapChangeCore_Create(wk, 0));
        (*state)++;
        break;
    case 2:
        GFL_OvlUnload(OVERLAY_ID(28));
        GFL_OvlLoad(OVERLAY_ID(27));
        GameData_RestoreCGearPowerRequest(gameData);
        FieldSubscreen_ChangeImm(Field_GetSubscreen(field), 0);
        EventScriptCall_Start(event, 0x83a, NULL, 0, HEAPID_FIELDMAP);
        (*state)++;
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventUnionRoomWarp_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChange *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;
    GameData *gameData = wk->gameData;

    switch (*state) {
    case 0:
        GameData_SaveCGearPowerRequest(gameData);
        FieldSubscreen_ChangeImm(Field_GetSubscreen(field), 0);
        GameEvent_ChainNext(event, CallEventPrepareResidentActorsForZoneChange(gsys, field));
        (*state)++;
        break;
    case 1:
        GFL_OvlUnload(OVERLAY_ID(27));
        GFL_OvlLoad(OVERLAY_ID(28));
        GameEvent_ChainNext(event, EventMapChangeCore_Create(wk, 4));
        (*state)++;
        break;
    case 2:
        UnionMain_Boot(gsys);
        (*state)++;
        break;
    case 3:
        GameEvent_ChainNext(event, func_ov036_021b9664(event, gsys, field));
        (*state)++;
        break;
    case 4:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

void InitMapChangeEvent(EventMapChange *wk, GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    u8 season = GameData_GetSeason(gameData);

    wk->gsys = gsys;
    wk->gameData = gameData;
    wk->field = field;
    wk->zoneId = Field_GetPlayerStateZoneID(field);
    wk->unk40 = FALSE;
    wk->seasonChanged = FALSE;
    wk->prevSeason = season;
    wk->season = season;
    func_ov036_021a2398(Field_GetEncountSystem(wk->field), 1);
}

GameEvent *EventMapChangeWarp_CreateGrid(GameSystem *gsys, Field *field, u16 zoneId, const VecFx32 *pos, u16 dir,
                                         BOOL unk40) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeWarp_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    dir = ConvDirToWarpDir(dir);
    CreateZoneChangeData(&wk->spawn, zoneId, dir, pos->x, pos->y, pos->z);
    wk->unk2C = 0;
    wk->unk40 = unk40;
    wk->lensFlareStarted = TRUE;
    return event;
}

GameEvent *EventMapChangeWarp_CreateRail(GameSystem *gsys, Field *field, u16 zoneId, const RailPosition *pos, u16 dir,
                                         BOOL unk40) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeWarp_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    dir = ConvDirToWarpDir(dir);
    CreateZoneChangeDataRail(&wk->spawn, zoneId, dir, pos->componentId, pos->posFront, pos->posSide);
    wk->unk2C = 0;
    wk->unk40 = unk40;
    wk->lensFlareStarted = TRUE;
    return event;
}

GameEvent *EventMapChange_CreateRail(GameSystem *gsys, Field *field, u16 zoneId, const RailPosition *pos, u16 dir,
                                     BOOL unk40) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChange_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    dir = ConvDirToWarpDir(dir);
    CreateZoneChangeDataRail(&wk->spawn, zoneId, dir, pos->componentId, pos->posFront, pos->posSide);
    wk->unk2C = 0;
    wk->unk40 = unk40;
    return event;
}

GameEvent *EventMapChangeQuicksand_Create(GameSystem *gsys, Field *field, const VecFx32 *effectPos, u16 zoneId,
                                          VecFx32 *pos) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeQuicksand_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    CreateZoneChangeData(&wk->spawn, zoneId, 1, pos->x, pos->y, pos->z);
    VEC_Set(&wk->unk34, effectPos->x, effectPos->y, effectPos->z);
    wk->unk2C = 0;
    return event;
}

GameEvent *EventMapChangeEscapeRope_Create(Field *field, GameSystem *gsys) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeEscapeRope_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    wk->spawn = *GameData_GetEscapeRopeZone(wk->gameData);
    AdjustEscapeRopeSpawn(wk->gameData, &wk->spawn);
    wk->spawn.changeType = ZONE_SPAWN_CHANGE_TYPE_POSITION;
    wk->unk2C = 0;
    wk->unk40 = TRUE;
    return event;
}

GameEvent *EventMapChangeDig_Create(GameSystem *gsys) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeDig_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    wk->spawn = *GameData_GetEscapeRopeZone(wk->gameData);
    AdjustEscapeRopeSpawn(wk->gameData, &wk->spawn);
    wk->spawn.changeType = ZONE_SPAWN_CHANGE_TYPE_POSITION;
    wk->unk2C = 0;
    wk->unk40 = TRUE;
    return event;
}

GameEvent *EventMapChangeTeleport_Create(GameSystem *gsys) {
    u16 returnLocation = GetReturnLocationIdx(GSYS_GetGameData(gsys));
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeTeleport_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    LoadZoneSpawnInfoCheckRail(&wk->spawn, GetRespawnZoneMainZone(returnLocation));
    wk->spawn.changeType = ZONE_SPAWN_CHANGE_TYPE_POSITION;
    wk->unk2C = 0;
    return event;
}

GameEvent *EventMapChangeDiveOut_Create(GameSystem *gsys) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeDiveOut_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    wk->spawn = *GameData_GetNextZone(wk->gameData);
    wk->spawn.changeType = ZONE_SPAWN_CHANGE_TYPE_POSITION;
    wk->spawn.warpDir = WARP_DIR_DOWN;
    wk->unk2C = 0;
    return event;
}

GameEvent *EventMapChangeDiveIn_Create(GameSystem *gsys, u16 zoneId) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeDiveIn_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);
    ZoneSpawnInfo returnSpawn;
    PlayerState *playerState;
    VecFx32 *pos;
    u16 currentZoneId;

    InitMapChangeEvent(wk, gsys);
    LoadZoneSpawnInfoCheckRail(&wk->spawn, zoneId);
    wk->unk2C = 0;
    wk->spawn.warpDir = WARP_DIR_UP;

    // Resurface where the dive started
    playerState = GameData_GetPlayerState(wk->gameData);
    pos = PlayerState_GetWPos(playerState);
    currentZoneId = PlayerState_GetZoneID(playerState);
    CreateZoneChangeData(&returnSpawn, currentZoneId, PlayerState_CalcDirection(playerState), pos->x, pos->y, pos->z);
    GameData_SetNextZone(wk->gameData, &returnSpawn);
    return event;
}

GameEvent *EventMapChangeWarpPad_Create(GameSystem *gsys, Field *field, u16 zoneId, const VecFx32 *pos, u16 dir) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeWarpPad_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    dir = ConvDirToWarpDir(dir);
    CreateZoneChangeData(&wk->spawn, zoneId, dir, pos->x, pos->y, pos->z);
    wk->unk2C = 5;
    wk->unk40 = FALSE;
    wk->lensFlareStarted = TRUE;
    return event;
}

GameEvent *EventUnionRoomWarp_Create(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventUnionRoomWarp_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    CreateZoneChangeData(&wk->spawn, ZONE_UNION_ROOM, ConvDirToWarpDir(0), FX32_CONST(184), 0, FX32_CONST(248));
    wk->unk2C = 0;
    return event;
}

GameEvent *EventMapChangeUnionRoomExit_Create(GameSystem *gsys) {
    ZoneSpawnInfo *next = GameData_GetNextZone(GSYS_GetGameData(gsys));
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeUnionRoomExit_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    wk->spawn = *next;
    return event;
}

GameEvent *EventEntralinkWarpIn_Create(GameSystem *gsys, u16 zoneId, const VecFx32 *pos, u32 a3) {
    Field *field = GSYS_GetField(gsys);
    GameData *gameData = GSYS_GetGameData(gsys);
    VecFx32 spawnPos = *pos;
    ZoneSpawnInfo returnSpawn;
    ZoneSpawnInfo spawn;

    EventEntralinkWarp_CreateReturnLocation(&returnSpawn, field);
    GameData_SetEntralinkParentSpawnInfo(gameData, &returnSpawn);
    func_0200c6f0(getHighLinkBlockAddress(GameData_GetSaveControl(gameData)), func_02017a40(gameData), 0);
    CreateZoneChangeData(&spawn, zoneId, ConvDirToWarpDir(0), spawnPos.x, spawnPos.y, spawnPos.z);
    return EventEntralinkWarpIn_CreateCore(gsys, field, &spawn, a3, 0);
}

GameEvent *EventEntralinkWarp_CreateOut(GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    ZoneSpawnInfo spawn = *GameData_GetEntralinkParentSpawnInfo(gameData);
    GameEvent *event = EventEntralinkWarp_Create(gsys, field, &spawn);
    GameCommSys *comm = GSYS_GetGameCommSystem(gsys);

    if (!GameCommSys_BootCheck(comm)) {
        func_0202be00(comm);
    }
    GameData_SetForceSeasonSync(gameData, FALSE);
    func_020175d8(gameData, 0);
    func_02017608(gameData, 0);
    func_020175c4(gameData, 1);
    return event;
}

void EventEntralinkWarp_CreateReturnLocation(ZoneSpawnInfo *spawn, Field *field) {
    FieldPlayer *player = Field_GetPlayer(field);

    if (Field_GetResolvedControllerTypeID(field) == 0) {
        VecFx32 *pos = GetMModelWPosPtr(FieldPlayer_GetActor(player));

        CreateZoneChangeData(spawn, Field_GetPlayerStateZoneID(field), WARP_DIR_DOWN, pos->x, pos->y, pos->z);
    } else {
        RailPosition railPos;

        func_ov036_0219ad24(player, &railPos);
        CreateZoneChangeDataRail(spawn, Field_GetPlayerStateZoneID(field), WARP_DIR_DOWN, railPos.componentId,
                                 railPos.posFront, railPos.posSide);
    }
}

GameEvent *EventMapChangeWarp_CreateFromEntity(GameSystem *gsys, Field *field, ZoneWarp *warp, u32 a3) {
    EventMapChange *wk;
    GameEvent *event;
    ZoneSpawnInfo *remember;
    GameData *gameData;

    gameData = GSYS_GetGameData(gsys);
    event = GameEvent_Create(gsys, NULL, EventMapChangeWarp_Callback, sizeof(EventMapChange));
    wk = GameEvent_GetData(event);
    InitMapChangeEvent(wk, gsys);
    wk->unk40 = TRUE;
    if (IsWarpDestId256(warp)) {
        wk->spawn = *GameData_GetNextZone(gameData);
    } else {
        SetupWarpParamByWarp(warp, &wk->spawn, a3);
    }
    wk->unk2C = GetWarpTransitionType(warp);

    // Remember the exit when going from the overworld into a building or cave
    remember = GetOutboundWarpRememberSpawnInfo(gameData);
    if (GetIsZoneMatrix0(remember->zoneId) == TRUE && GetIsZoneMatrix0(wk->spawn.zoneId) == FALSE) {
        GameData_SetEscapeRopeZone(gameData, remember);
    }
    GameData_UpdateEscapeRopeZone(gameData, &wk->spawn);
    FieldStatus_SetBusyFlag(GameData_GetFieldStatus(gameData), 2);
    return event;
}

GameEvent *EventMapChange_CreateGrid(GameSystem *gsys, Field *field, u8 mode, u16 zoneId, VecFx32 *pos, u16 dir) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChange_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    dir = ConvDirToWarpDir(dir);
    CreateZoneChangeData(&wk->spawn, zoneId, dir, pos->x, pos->y, pos->z);
    wk->unk2C = 0;
    wk->mode = mode;
    wk->lensFlareStarted = TRUE;
    return event;
}

GameEvent *EventMapChange_CreateGridDefault(GameSystem *gsys, Field *field, u16 zoneId, VecFx32 *pos, u16 dir) {
    return EventMapChange_CreateGrid(gsys, field, 0, zoneId, pos, dir);
}

GameEvent *EventMapChange_CreateForFly(GameSystem *gsys, Field *field, u32 unused, ZoneSpawnInfo *spawn, u16 warpDir) {
    GameData *gameData = GSYS_GetGameData(gsys);
    GameEvent *event;
    EventMapChange *wk;

    SetPlayerSpecialState(GameData_GetPlayerState(gameData), 0);
    event = GameEvent_Create(gsys, NULL, EventMapChange_Callback, sizeof(EventMapChange));
    wk = GameEvent_GetData(event);
    InitMapChangeEvent(wk, gsys);
    wk->spawn = *spawn;
    wk->spawn.warpDir = warpDir;
    wk->unk2C = 0;
    wk->mode = 1;
    GameData_UpdateEscapeRopeZone(gameData, &wk->spawn);
    return event;
}

GameEvent *EventMapChangeFakeWarp_Create(GameSystem *gsys, Field *field, u16 zoneId, const VecFx32 *pos, u16 dir) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeFakeWarp_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    dir = ConvDirToWarpDir(dir);
    CreateZoneChangeData(&wk->spawn, zoneId, dir, pos->x, pos->y, pos->z);
    wk->unk2C = 0;
    wk->lensFlareStarted = TRUE;
    return event;
}

GameEvent *EventMapChangeEnding_Create(GameSystem *gsys, Field *field, u16 zoneId, const VecFx32 *pos, u16 dir) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeEnding_Callback, sizeof(EventMapChange));
    EventMapChange *wk = GameEvent_GetData(event);

    InitMapChangeEvent(wk, gsys);
    dir = ConvDirToWarpDir(dir);
    CreateZoneChangeData(&wk->spawn, zoneId, dir, pos->x, pos->y, pos->z);
    wk->unk2C = 0;
    wk->mode = 0;
    wk->lensFlareStarted = TRUE;
    return event;
}

void FieldMapControl_LoadBlackoutZone(GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    ZoneSpawnInfo spawn;
    ZoneSpawnInfo escapeRopeSpawn;

    SetupTeleportZoneChange(GetReturnLocationIdx(gameData), &spawn);
    LoadZoneSpawnInfoCheckRail(&escapeRopeSpawn, GetRespawnZoneMainZone(GetReturnLocationIdx(gameData)));
    GameData_SetEscapeRopeZone(gameData, &escapeRopeSpawn);
    FieldMapControl_DeleteAllActors(gsys);
    FieldMapControl_LoadZone(gsys, spawn.zoneId);
    FieldMapControl_InitSpawn(gsys, &spawn);
    GameBeacon_SetZone(spawn.zoneId, gameData);
    func_ov012_02162f44(gameData);
    func_ov012_0215ef00(gameData, spawn.zoneId);
    ShutdownFollowWork(gameData);
}

GameEventReturnCode EventMapChangeBlackout_Callback(GameEvent *event, u32 *state, void *data) {
    EventMapChangeBlackout *wk = data;

    switch (*state) {
    case 0:
        FieldMapControl_LoadBlackoutZone(wk->gsys);
        (*state)++;
        break;
    case 1:
        SetPlayerSpecialState(GameData_GetPlayerState(GSYS_GetGameData(wk->gsys)), 0);
        ISSSwitchSys_ResetSwitches(ISS_GetSwitchSys(GameSystem_GetISS(wk->gsys)));
        GameEvent_ChainNext(event, EventBGMChange_Create(wk->gsys,
                                                         GetMapBGMIDByPlayerState2(wk->gameData, wk->spawn.zoneId,
                                                                                   GameData_GetSeason(wk->gameData)),
                                                         0, 60));
        (*state)++;
        break;
    case 2:
        GameEvent_ChainNext(event, EventFieldOpen_Create(wk->gsys));
        (*state)++;
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventMapChangeBlackout_Create(GameSystem *gsys) {
    GSYS_GetGameData(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventMapChangeBlackout_Callback, sizeof(EventMapChangeBlackout));
    EventMapChangeBlackout *wk = GameEvent_GetData(event);

    wk->gsys = gsys;
    wk->gameData = GSYS_GetGameData(gsys);
    SetupTeleportZoneChange(GetReturnLocationIdx(wk->gameData), &wk->spawn);
    return event;
}

u16 ConvWarpDirToAngle(int warpDir) {
    switch (warpDir) {
    default:
    case WARP_DIR_UP:
        return 0;
    case WARP_DIR_LEFT:
        return 0x4000;
    case WARP_DIR_DOWN:
        return 0x8000;
    case WARP_DIR_RIGHT:
        return 0xc000;
    }
}

void SetupZoneChangeSpawn(EventData *eventData, ZoneSpawnInfo *next, ZoneSpawnInfo *spawn) {
    if (next->changeType == ZONE_SPAWN_CHANGE_TYPE_POSITION) {
        *spawn = *next;
    } else if (!SetupZoneWarpArrival(eventData, spawn, next->warpId, next->posWeightBits)) {
        LoadZoneSpawnInfoCheckRail(spawn, next->zoneId);
    }
}

void FieldMapControl_InitSpawn(GameSystem *gsys, ZoneSpawnInfo *next) {
    GameData *gameData = GSYS_GetGameData(gsys);
    PlayerState *playerState = GameData_GetPlayerState(gameData);
    EventData *eventData = GameData_GetEventData(gameData);
    ZoneSpawnInfo spawn;
    u16 respawnLocation;
    s32 cacheIdx;
    MMSys *mmSys;

    FieldStatus_SetNewLoadFlag(GameData_GetFieldStatus(gameData), TRUE);
    SetupZoneChangeSpawn(eventData, next, &spawn);
    if (spawn.changeType == ZONE_SPAWN_CHANGE_TYPE_3) {
        GameData_SetNextZone(gameData, GetOutboundWarpRememberSpawnInfo(gameData));
    }

    PlayerState_SetZoneID(playerState, spawn.zoneId);
    PlayerState_SetRotation(playerState, ConvWarpDirToAngle(spawn.warpDir));
    if (!GetZoneSpawnInfoIsRail(&spawn)) {
        PlayerState_SetWPos(playerState, &spawn.pos.vec);
        PlayerState_SetIsRail(playerState, FALSE);
    } else {
        PlayerState_SetRailPos(playerState, &spawn.pos.rail);
        PlayerState_SetIsRail(playerState, TRUE);
    }

    ISS_ChangeZone(GameSystem_GetISS(gsys), spawn.zoneId);
    SetGameDataNowSpawnZone(gameData, &spawn);
    respawnLocation = GetRespawnLocationIndexForRespawnZone(spawn.zoneId);
    if (respawnLocation != 0) {
        SetCurrentTeleportOrDeathZone(gameData, respawnLocation);
    }
    GameData_SetGimmickByZone(gameData, spawn.zoneId);
    func_ov012_02162f44(gameData);
    func_ov012_0215ee40(gameData, spawn.zoneId);
    SetTeleportZoneDiscover(gameData, spawn.zoneId);
    FieldScript_CallOnZoneInit(gsys, 4);
    resetRebattleTrainers(GameData_GetEventWork(gameData));
    CallSpawnAllZoneNPCs(gameData, next);
    func_ov012_021683f4(gsys, spawn.zoneId);
    GameData_UpdateFlashStatus(gameData, spawn.zoneId);
    ResetWeather(gsys, spawn.zoneId);

    cacheIdx = GetZoneNPCInfoCacheIdx(spawn.zoneId);
    mmSys = GameData_GetMMSys(gameData);
    if (cacheIdx < 24) {
        LoadMModelSystemInfoCache(mmSys, cacheIdx);
    } else {
        FldActSys_ClearCache(mmSys);
    }
}

void CallSpawnAllZoneNPCs(GameData *gameData, const ZoneSpawnInfo *spawn) {
    EventData *eventData = GameData_GetEventData(gameData);
    u32 count = GetZoneNPCsCount(eventData);

    if (count != 0) {
        EventWork *eventWork = GameData_GetEventWork(gameData);
        MMSys *mmSys = GameData_GetMMSys(gameData);

        SpawnAllZoneNPCs(mmSys, GetZoneNPCs(eventData), spawn->zoneId, count, eventWork);
    }
}

void GameData_DeleteAllActors(GameData *gameData) {
    FldActSys_DeleteAllActors(GameData_GetMMSys(gameData));
}

// Allocates a list of people while in Join Avenue and frees it elsewhere, then resets the people in it and in the save
void GameData_UpdateJoinAvenueForZone(GameData *gameData, u16 zoneId) {
    ResortWork *unk = func_02017b84(gameData);
    JoinAvenuePersonList **personList;

    if (IsZoneJoinAvenue(zoneId) || IsZoneJoinAvenueSubZone(zoneId)) {
        personList = GameData_GetJoinAvenuePersonListPtr(gameData);
        if (*personList == NULL) {
            *personList = JoinAvenuePersonList_Create(4, 8);
        }
    } else {
        personList = GameData_GetJoinAvenuePersonListPtr(gameData);
        if (*personList != NULL) {
            JoinAvenuePersonList_Free(*personList);
            *personList = NULL;
        }
    }

    if (IsZoneJoinAvenue(zoneId)) {
        u16 param =
            JoinAvenue_GetParam(JoinAvenue_GetInfo(SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData))), 5, 0);
        func_02017b64(gameData, param);
        func_02038bc8(0x18);
    }

    ResortWork_Set(unk, 8, 0);
    {
        JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));
        JoinAvenuePersonList *lists[2] = { NULL, NULL };
        int i;

        personList = GameData_GetJoinAvenuePersonListPtr(gameData);
        lists[0] = JoinAvenue_GetPersonList(joinAvenue);
        if (*personList != NULL) {
            lists[1] = *personList;
        }
        for (i = 0; i < 2; i++) {
            if (lists[i] != NULL) {
                u32 j;

                for (j = 0; j < JoinAvenuePersonList_GetCount(lists[i]); j++) {
                    JoinAvenuePerson *person = JoinAvenuePersonList_Get(lists[i], j);

                    if (!JoinAvenuePerson_IsEmpty(person)) {
                        JoinAvenuePerson_SetParam(person, 0x26, 0);
                    }
                }
            }
        }
    }
}

// Sets flag 8 when the zone changed, which GameData_UpdateJoinAvenueForZone clears
void GameData_UpdateZoneChangeFlag(GameData *gameData, u16 zoneId, u16 prevZoneId) {
    ResortWork *unk = func_02017b84(gameData);

    if (zoneId != prevZoneId) {
        ResortWork_Set(unk, 8, 1);
    }
}

void GameData_UpdateFlashStatus(GameData *gameData, u16 zoneId) {
    FieldStatus *status = GameData_GetFieldStatus(gameData);
    u32 flags = GetZoneFlashFlags(zoneId);

    if ((flags & 1) && FieldStatus_CheckFlashUsed(status)) {
        flags &= ~1;
        flags |= 2;
    }
    FieldStatus_SetFlashPerms(status, flags);
}

void FieldMapControl_LoadZone(GameSystem *gsys, u16 zoneId) {
    GameData *gameData = GSYS_GetGameData(gsys);
    EventData *eventData = GameData_GetEventData(gameData);
    MapMatrix *matrix;

    GSYS_GetField(gsys);

    if (GameData_IsForceSeasonSync(gameData) == TRUE && !IsZoneEntralinkHub(zoneId)) {
        FieldStatus_SetInLinkedWorld(GameData_GetFieldStatus(gameData), TRUE);
    } else {
        FieldStatus_SetInLinkedWorld(GameData_GetFieldStatus(gameData), FALSE);
    }

    if (GetZoneIsUnionRoom(zoneId)) {
        GameData_SetLastSubscreen(gameData, 2);
    } else if (IsZone150Or151(zoneId) || GetZoneIsMusicalTheater(zoneId) || IsZoneRoyalUnova(zoneId)) {
        GameData_SetLastSubscreen(gameData, 5);
    } else if (GetZoneIsPWTBattleStage(zoneId)) {
        GameData_SetLastSubscreen(gameData, 11);
    } else {
        u32 subscreen;

        if (GameData_IsForceSeasonSync(gameData) == TRUE && GameData_GetLastSubscreen(gameData) != 3) {
            GameData_SetLastSubscreen(gameData, 3);
        } else if (GameData_IsForceSeasonSync(gameData) == FALSE && GameData_GetLastSubscreen(gameData) == 3) {
            GameData_SetLastSubscreen(gameData, 0);
        }
        subscreen = GameData_GetLastSubscreen(gameData);
        if (subscreen != 3 && subscreen != 4 && subscreen != 10 && subscreen != 6) {
            GameData_SetLastSubscreen(gameData, 0);
        }
    }

    EventData_LoadZone(eventData, zoneId, GameData_GetSeason(gameData));
    matrix = GetMapMatrixSystem(gameData);
    MapMatrix_Load(matrix, GetZoneMatrixId(zoneId), zoneId, HEAPID_TAIL(HEAPID_USER));
    MapMatrix_Patch(matrix, gsys, HEAPID_TAIL(HEAPID_USER));
    GameData_UpdateFlashStatus(gameData, zoneId);
    GameData_UpdateJoinAvenueForZone(gameData, zoneId);

    SetAllowVersionSpecificArea(VERSION_AREA_BLACK2, FALSE);
    SetAllowVersionSpecificArea(VERSION_AREA_WHITE2, FALSE);
    SetAllowVersionSpecificArea(VERSION_AREA_2, FALSE);
    if (func_ov011_02154e70(gameData, 0)) {
        SetAllowVersionSpecificArea(VERSION_AREA_2, TRUE);
        SetAllowVersionSpecificArea(VERSION_AREA_OWN, TRUE);
    }
}

void FieldMapControl_DeleteAllActors(GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);

    GSYS_GetField(gsys);
    GameData_DeleteAllActors(gameData);
}

void Field_SetPlayerHidden(Field *field, BOOL hidden) {
    SetActorHidden(FieldPlayer_GetActor(Field_GetPlayer(field)), hidden);
}

void GameData_SetGimmickByZone(GameData *gameData, int zoneId) {
    GimmickState *gimmick = GameData_GetGimmickState(gameData);
    ArcTool *handle;
    u32 i;
    ZoneGimmick *gimmicks;
    u32 count;

    GimmickState_Reset(gimmick);
    handle = GFL_ArcSysCreateFileHandle(ARCID_GIMMICK_TBL, HEAPID_TAIL(HEAPID_USER));
    gimmicks = GFL_ArcToolReadHeapNew(handle, 0, HEAPID_TAIL(HEAPID_USER));
    count = GFL_ArcToolGetDataLength(handle, 0) / sizeof(ZoneGimmick);
    for (i = 0; i < count; i++) {
        if (zoneId == gimmicks[i].zoneId) {
            GimmickState_SetID(gimmick, gimmicks[i].gimmickId);
            break;
        }
    }
    GFL_HeapFree(gimmicks);
    GFL_ArcToolFree(handle);
}

GameEvent *EventEntralinkWarpIn_CreateCore(GameSystem *gsys, Field *field, ZoneSpawnInfo *spawn, u32 a3, u32 a4) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventEntralinkWarpIn_Callback, sizeof(EventEntralinkWarp));
    EventEntralinkWarp *wk = GameEvent_GetData(event);

    wk->spawn = *spawn;
    wk->gsys = gsys;
    wk->gameData = GSYS_GetGameData(gsys);
    wk->field = field;
    wk->unk28 = a3;
    wk->unk2C = a4;
    return event;
}

GameEvent *EventEntralinkWarp_Create(GameSystem *gsys, Field *field, ZoneSpawnInfo *spawn) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventEntralinkWarp_Callback, sizeof(EventEntralinkWarp));
    EventEntralinkWarp *wk = GameEvent_GetData(event);

    wk->spawn = *spawn;
    wk->gsys = gsys;
    wk->gameData = GSYS_GetGameData(gsys);
    wk->field = field;
    wk->festMissionStatus = getStatusOfFesMission(GSYS_GetLinkFestival(gsys));
    return event;
}

GameEventReturnCode EventEntralinkWarp_Callback(GameEvent *event, u32 *state, void *data) {
    EventEntralinkWarp *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = wk->field;

    switch (*state) {
    case 0:
        func_ov036_021b5168(Field_GetPlaceName(field));
        if (wk->festMissionStatus != 0) {
            GFL_SndSEPlay(SEQ_SE_FLD_131);
            GameEvent_ChainNext(event, CallFieldMapEntranceOutTransition(gsys, field, 1, 0, 4));
        } else {
            GFL_SndSEPlay(SEQ_SE_FLD_131);
            EncEff_StartEvent(Field_GetEncEff(field), event, 0x25);
        }
        (*state)++;
        break;
    case 1: {
        GameEvent *mapChange = GameEvent_Create(gsys, NULL, EventMapChange_Callback, sizeof(EventMapChange));
        EventMapChange *mapChangeWk = GameEvent_GetData(mapChange);

        InitMapChangeEvent(mapChangeWk, gsys);
        mapChangeWk->spawn = wk->spawn;
        mapChangeWk->unk2C = 0;
        mapChangeWk->unk40 = FALSE;
        GameEvent_ChainNext(event, mapChange);
        (*state)++;
        break;
    }
    case 2:
        if (wk->festMissionStatus != 0) {
            GameEvent_ChainNext(event, func_ov036_021b8850(gsys, field, 1, 0, 2));
        } else {
            GameEvent_ChainNext(event, func_ov036_021b8850(gsys, field, 0, 0, 2));
        }
        (*state)++;
        break;
    case 3:
        BeginForcePlaceNameDisp(Field_GetPlaceName(field), wk->spawn.zoneId);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode EventEntralinkWarpIn_Callback(GameEvent *event, u32 *state, void *data) {
    EventEntralinkWarp *wk = data;
    GameCommSys *comm = GSYS_GetGameCommSystem(wk->gsys);

    switch (*state) {
    case 0: {
        u32 scriptId;

        if (!EventEntralinkWarpIn_CheckAllowed(wk->gsys)) {
            scriptId = 0x279c;
            *state = 2;
        } else if (GameData_CheckPairFlag(wk->gameData)) {
            scriptId = 0x27a1;
            *state = 2;
        } else {
            scriptId = 0x279a;
            GameCommSys_ExitReq(comm);
            *state = 1;
        }
        EventScriptCall_Start(event, scriptId, NULL, 0, Field_GetHeapID(wk->field));
        break;
    }
    case 1:
        if (!GameCommSys_BootCheck(comm)) {
            func_0202be00(comm);
            GameData_SetForceSeasonSync(wk->gameData, TRUE);
            func_020175d8(wk->gameData, func_0203ffc4());
            GameEvent_ChainNext(event, EventEntralinkWarp_Create(wk->gsys, wk->field, &wk->spawn));
            *state = 2;
        }
        break;
    case 2:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Sets where an Escape Rope leads when warping into the zones that need a special exit
void GameData_UpdateEscapeRopeZone(GameData *gameData, const ZoneSpawnInfo *spawn) {
    ZoneSpawnInfo *remember = GetOutboundWarpRememberSpawnInfo(gameData);
    ZoneSpawnInfo *now = GetGameDataNowSpawnZone(gameData);
    ZoneSpawnInfo escapeRopeSpawn;

    if (IsZoneInVictoryRoad(spawn->zoneId)) {
        if (now->zoneId == ZONE_POKEMON_LEAGUE) {
            GameData_SetEscapeRopeZone(gameData, remember);
        } else if (now->zoneId == ZONE_VICTORY_ROAD) {
            LoadZoneSpawnInfoCheckRail(&escapeRopeSpawn, ZONE_VICTORY_ROAD);
            GameData_SetEscapeRopeZone(gameData, &escapeRopeSpawn);
        }
    }
    if (now->zoneId == ZONE_DESERT_RESORT_2 && GetZoneFlagsEnableEscapeRope(spawn->zoneId)) {
        GameData_SetEscapeRopeZone(gameData, remember);
    }
    if (now->zoneId == ZONE_CASTELIA_CITY_12 && spawn->zoneId == ZONE_CASTELIA_SEWERS) {
        GameData_SetEscapeRopeZone(gameData, remember);
    }
}

// An Escape Rope used outside the Abyssal Ruins leads to the next zone instead
void AdjustEscapeRopeSpawn(GameData *gameData, ZoneSpawnInfo *spawn) {
    if (IsZoneAbyssalRuinsOutside(GetGameDataNowSpawnZone(gameData)->zoneId)) {
        *spawn = *GameData_GetNextZone(gameData);
    }
}

void GameData_AdjustPlayerStateOnDiveOut(GameData *gameData) {
    PlayerState *playerState = GameData_GetPlayerState(gameData);

    if (FieldPlayerState_GetExState(playerState) != 3) {
        SetPlayerSpecialState(playerState, 0);
    } else {
        SetPlayerSpecialState(playerState, 2);
    }
}

// Applies the form changes of party Pokemon that happen at night. In the base game, only Sky Forme Shaymin changes,
// reverting to Land Forme.
void GameData_UpdatePartyForTimeOfDay(GameData *gameData) {
    SaveControl *save = GameData_GetSaveControl(gameData);
    PokeParty *party = SaveControl_GetPokePartySave(save);
    u8 season = GameData_GetSeason(gameData);

    func_ov012_021643f0(gameData, party, &getSaveAdventureTimeBlock(save)->time, season);
}

// DS Protect tamper responses, which leak memory
void *EventMapChange_DSProtTamper1(void *arg0, void *arg1) {
    GFL_HeapAllocate(HEAPID_TAIL(HEAPID_GAMEEVENT), 0x1000, FALSE, "event_mapchange.c", 3931);
    return arg0;
}

void *EventMapChange_DSProtTamper2(void *arg0, void *arg1) {
    GFL_HeapAllocate(HEAPID_TAIL(HEAPID_GAMEEVENT), 0x1000, FALSE, "event_mapchange.c", 3937);
    return arg1;
}

void *EventMapChange_DSProtNop(void *arg0, void *arg1) {
    return arg0;
}

void CityState_InitFromSave(CityState *state, PlayerInfo *player, SaveControl *save, u32 unused) {
    u32 switched = KeyInfo_GetCityKey(getKeyInfoSaveBlk(save));

    state->initialized = TRUE;
#ifdef BLACK2
    if (!switched) {
        state->city = CITY_BLACK_CITY;
    } else {
        state->city = CITY_WHITE_FOREST;
    }
#else
    if (!switched) {
        state->city = CITY_WHITE_FOREST;
    } else {
        state->city = CITY_BLACK_CITY;
    }
#endif
}

BOOL CityState_IsCityValid(CityState *state) {
    if (state->initialized == TRUE && state->city >= 2) {
        return FALSE;
    }
    return TRUE;
}

BOOL CityState_IsSet(CityState *state) {
    if (!state->initialized) {
        return FALSE;
    }
    if (CityState_IsCityValid(state)) {
        return TRUE;
    }
    return FALSE;
}
