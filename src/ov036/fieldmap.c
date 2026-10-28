// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/zones.h"
#include "field/encounter.h"
#include "field/encounter_effect.h"
#include "field/event_data.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_3dci.h"
#include "field/field_actor.h"
#include "field/field_async_proc.h"
#include "field/field_camera.h"
#include "field/field_controller.h"
#include "field/field_daycare.h"
#include "field/field_disp_control.h"
#include "field/field_effect.h"
#include "field/field_environment.h"
#include "field/field_event.h"
#include "field/field_exp_obj.h"
#include "field/field_fog.h"
#include "field/field_g3d_mapper.h"
#include "field/field_g3dobj.h"
#include "field/field_internal.h"
#include "field/field_lens_flare.h"
#include "field/field_map.h"
#include "field/field_nogrid_mapper.h"
#include "field/field_palace.h"
#include "field/field_player.h"
#include "field/field_prop.h"
#include "field/field_rail.h"
#include "field/field_scene_area.h"
#include "field/field_script.h"
#include "field/field_script_plugin.h"
#include "field/field_skill_map_eff.h"
#include "field/field_sound.h"
#include "field/field_status.h"
#include "field/field_task.h"
#include "field/field_weather.h"
#include "field/fieldmap_ctrl_hybrid.h"
#include "field/intrude_work.h"
#include "field/medal.h"
#include "field/player_state.h"
#include "field/pleasure_boat.h"
#include "field/resort_mapcreate.h"
#include "field/scrcmd_leaves.h"
#include "field/subscreen.h"
#include "field/union_eff.h"
#include "field/union_main.h"
#include "field/wbt.h"
#include "field/zone.h"
#include "field/zone_change.h"
#include "gfl/arc.h"
#include "gfl/billboard_actor.h"
#include "gfl/bmpwin.h"
#include "gfl/calctool.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/rtc_cache.h"
#include "gfl/sound.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "gfl/wm_icon.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "save/box.h"
#include "save/join_avenue.h"
#include "save/medal_box.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/area_data.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/iss_sys.h"
#include "system/main.h"
#include "system/resort_work.h"
#include "system/rtc.h"
#include "system/season.h"
#include "system/zone_weather.h"

// What a field routine returns: run on, with the other of its pair next frame; go on to the next routine; or end the
// field
enum {
    FIELD_ROUTINE_CONTINUE,
    FIELD_ROUTINE_NEXT,
    FIELD_ROUTINE_END,
};

// The phases of FieldRoutine_MapLoad, by what they make
enum {
    MAP_LOAD_CAMERA,
    MAP_LOAD_MAPPERS,
    MAP_LOAD_MAP,
    MAP_LOAD_ACTORS,
    MAP_LOAD_PLAYER,
    MAP_LOAD_SUBSCREEN,
    MAP_LOAD_SYSTEMS,
};

// The phases of FieldRoutine_MapSpawnFinish: the map's first frame, then the zone's scripts
enum {
    MAP_SPAWN_FIRST_FRAME,
    MAP_SPAWN_SCRIPTS,
};

static const u8 sResolvedControllerTypes[4] = { FIELDMAP_CTRL_TYPE_GRID, FIELDMAP_CTRL_TYPE_NOGRID,
                                                FIELDMAP_CTRL_TYPE_HYBRID, FIELDMAP_CTRL_TYPE_GRID };

// A kind of map: the setup of its mapper and its controller, whether it takes its chunks from the zone's map matrix,
// and the size of its heap
typedef struct {
    FieldG3DMapperConfig mapperConfig;
    const FieldmapCtrlVTable *ctrlVTable;
    BOOL useMapMatrix;
    u32 heapSize;
} FieldMapConfig;

// A step of the field: 0 to run again next frame, 1 to go on to the next routine, 2 when the field ends
typedef u32 (*FieldRoutine)(GameSystem *gsys, Field *field);
typedef void (*FieldRenderFunc)(Field *field);

// The edge colors of the area, loaded from archive 15
static GXRgb g_FieldEdgeColorTable[8];

u32 FieldRoutine_Open(GameSystem *gsys, Field *field);
u32 FieldRoutine_MapLoad(GameSystem *gsys, Field *field);
u32 FieldRoutine_MapSpawnFinish(GameSystem *gsys, Field *field);
u32 FieldRoutine_Run(GameSystem *gsys, Field *field);
u32 FieldRoutine_RunAltFrame(GameSystem *gsys, Field *field);
u32 FieldRoutine_MapUnload(GameSystem *gsys, Field *field);
u32 FieldRoutine_Close(GameSystem *gsys, Field *field);

// clang-format off
static const LightSetup sFieldLights[4] = {
    { 0, { { -0xfff, -0xfff, -0xfff }, 0x7fff } },
    { 1, { { 0xfff, -0xfff, -0xfff }, 0x7fff } },
    { 2, { { -0xfff, -0xfff, -0xfff }, 0x7fff } },
    { 3, { { -0xfff, -0xfff, -0xfff }, 0x7fff } },
};
// clang-format on

static const LightSetupList FIELD_LIGHT_SETUP = { sFieldLights, 4 };

// The tables of one size are declared in this order because MWCC lays out objects of the same size by where each was
// declared: it is one that gives the ROM's .rodata, not the original's, which is unknown
static const FieldRenderFunc FIELD_RENDER_FUNCS_PHASE1[] = {
    FieldRenderPhase1_Fieldmap,
    NULL,
    NULL,
};

static const VecFx32 DEFAULT_FIELDCAMERA_TGT = { 0, 0, 0 };

static const FieldRenderFunc FIELD_RENDER_FUNCS_PHASE2[] = {
    FieldRenderPhase2_Fieldmap,
    FieldRenderPhase2_FieldEffect,
    FieldRenderPhase2_EncountEffect,
};

static const VecFx32 DEFAULT_FIELDCAMERA_POS = { 0, 0x40000, 0x80000 };

static const GXRgb FIELD_DEFAULT_EDGE_COLOR_TABLE[8] = {
    0x294a, 0x294a, 0x294a, 0x294a, 0x294a, 0x294a, 0x294a, 0x294a
};

static const BGSysLCDConfig FIELD_LCD_CONFIG = { 1, 0, 0, TRUE };

static const ClActSysSetup FIELD_CLACT_SETUP = {
    0, 0, 0, 0x200, 4, 0x7c, 4, 0x7c, 0, 0x48, 0x40, 0x40, 0x40, 0x10, 0x10,
};

// clang-format off
static const u32 FIELD_DEFAULT_FOG_TABLE[8] = {
    0x0c080400, 0x1c181410, 0x2c282420, 0x3c383430, 0x4c484440, 0x5c585450, 0x6c686460, 0x7c787470,
};
// clang-format on

static const GXRgb FIELD_STD_PALETTE[16] = { 0x7ffe };

static const BGSysVRAMConfig FIELDMAP_VRAM_CONFIG = { 8, 0, 0x80, 0, 0x10, 0, 0x100, 0, 7, 0x40, 0x100010, 0x10 };

static const FieldRoutine FIELDMAP_ROUTINES[][2] = {
    { FieldRoutine_Open, FieldRoutine_Open },
    { FieldRoutine_MapLoad, FieldRoutine_MapLoad },
    { FieldRoutine_MapSpawnFinish, FieldRoutine_MapSpawnFinish },
    { FieldRoutine_Run, FieldRoutine_RunAltFrame },
    { FieldRoutine_MapUnload, FieldRoutine_MapUnload },
    { FieldRoutine_Close, FieldRoutine_Close },
};

// clang-format off
static const FieldMapConfig MAP_CONFIGS[] = {
    { { 0x200000, 0x400000, 2, 2, 1, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x1b008, 1 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x800000, 0x800000, 2, 3, 0, 8, 2, 3, 1, NULL, FALSE, { 0, 0 }, { 0xffffffff, 0xffffffff }, 0x15004, 1 }, &FIELDMAP_CTRL_NOGRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 2, 2, 0, 8, 2, 2, 1, NULL, FALSE, { 0, 0 }, { 0xffffffff, 0xffffffff }, 0x15004, 0 }, &FIELDMAP_CTRL_NOGRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 2, 2, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 0 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 2, 2, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 1 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x400000, 0x400000, 1, 1, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x54010, 0 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 1, 1, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 0 }, &FIELDMAP_CTRL_GRID, TRUE, 0x12a000 },
    { { 0x200000, 0x400000, 2, 3, 1, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 1 }, &FIELDMAP_CTRL_NOGRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 3, 2, 1, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 0 }, &FIELDMAP_CTRL_NOGRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 1, 1, 1, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 0 }, &FIELDMAP_CTRL_GRID, TRUE, 0xaa000 },
    { { 0x200000, 0x400000, 2, 2, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 0 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 3, 2, 1, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x1b008, 1 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 2, 2, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 1 }, &FIELDMAP_CTRL_NOGRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 1, 6, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 1 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 2, 3, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 1 }, &FIELDMAP_CTRL_HYBRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 1, 1, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x1b008, 0 }, &FIELDMAP_CTRL_HYBRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 2, 2, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x1b008, 1 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 5, 1, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 1 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 3, 2, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 2 }, &FIELDMAP_CTRL_HYBRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 2, 2, 1, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x1b008, 0 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 2, 3, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 1 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 1, 6, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0xd004, 1 }, &FIELDMAP_CTRL_NOGRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 2, 3, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0xd534, 1 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 1, 2, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0xa004, 1 }, &FIELDMAP_CTRL_HYBRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 3, 2, 1, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0xe004, 1 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 1, 3, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 1 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 1, 1, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x15004, 1 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 2, 2, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0x1b008, 1 }, &FIELDMAP_CTRL_HYBRID, TRUE, 0x136800 },
    { { 0x200000, 0x400000, 1, 2, 0, 8, 1, 1, 1, NULL, FALSE, { 0, 0 }, { 0, 2 }, 0xf000, 1 }, &FIELDMAP_CTRL_GRID, TRUE, 0x136800 },
};
// clang-format on

Field *Field_Create(GameSystem *gsys, HeapID heapId) {
    Field *field = GFL_HeapAllocate(heapId, sizeof(Field), TRUE, "fieldmap.c", 0x22b);
    u32 season = 0;
    u16 areaId;

    field->heapId = heapId;
    field->routineState = 0;
    field->gameSystem = gsys;
    field->gameData = GSYS_GetGameData(gsys);
    field->zoneId = PlayerState_GetZoneID(GSYS_GetPlayerState(gsys));
    areaId = ZoneData_GetAreaID(field->zoneId);
    if (AreaData_HasSeasons(areaId)) {
        season = GameSystem_GetSeason(gsys);
    } else if (AreaData_IsInfinityDungeon(areaId)) {
        if (!IsZoneFirstBlackTowerOrWhiteTreehollow(PlayerState_GetZoneID(GSYS_GetPlayerState(gsys)))) {
            season = KeyDataSave_GetArea(getKeyDataBlkAddress(GameData_GetSaveControl(field->gameData)));
        }
    } else if (AreaData_IsJoinAvenue(areaId)) {
        season = GameData_GetJoinAvenueSeason(field->gameData);
    } else if (AreaData_IsWbt(areaId)) {
        WbtSystem **wbt = GameData_GetWbtSystemPtr(field->gameData);

        if (wbt != NULL && *wbt != NULL) {
            season = GetPWTSeason(*wbt);
        }
    }
    field->areaData = AreaData_Create(heapId, areaId, season);
    ResortWork_UpdateRecords(GameData_GetResortWork(field->gameData), GameData_GetSaveControl(field->gameData));
    field->ctrlVTable = GetZoneFieldmapCtrlVTable(field->zoneId);
    return field;
}

void Field_Free(Field *field) {
    GameSystem *gsys = field->gameSystem;

    AreaData_Free(field->areaData);
    GFL_HeapFree(field);
    GSYS_SetField(gsys, NULL);
    WmIcon_SetPos(0, 0);
}

u32 FieldRoutine_Open(GameSystem *gsys, Field *field) {
    HeapID heapId = field->heapId;

    GFL_BGSysSetVRAMBanks(&FIELDMAP_VRAM_CONFIG);
    FieldG2D_Init(field);
    ClActSys_Create(&FIELD_CLACT_SETUP, &FIELDMAP_VRAM_CONFIG, HEAPID_FIELD_CLACT);
    GFL_BGSysSetBGEnabledA(0x10, TRUE);
    GFL_BGSysSetBGEnabledB(0x10, TRUE);
    BmpWin_InitAllocator(heapId);
    GFL_TextRndInitColorIndexLUT();
    GFL_G3DSysCreate(0, 3, 0, 1, FX32_ONE, field->heapId, FieldG3D_InitCallback);
    FieldG3D_Init(field);
    field->g3DMapper = FieldG3DMapper_Create(field->heapId, GameData_GetSeason(field->gameData));
    field->asyncActorMatLoadTCB = GFL_VBlankTCBAdd(FldActSys_AsyncMatLoadTCBFunc, field, 0);
    field->tcbManagerHeap = GFL_HeapAllocate(heapId, GFL_TCBMgrCalcAllocSize(0x20), FALSE, "fieldmap.c", 0x2bf);
    field->tcbManager = GFL_TCBMgrCreate(0x20, field->tcbManagerHeap);
    field->taskManager = FieldTaskManager_Create(10, field->heapId);
    field->dispControl = FieldDispControl_Create(field->heapId);
    if (GetZoneIsUnionRoom(field->zoneId) == TRUE || IsZone150Or151(field->zoneId) == TRUE) {
        GFL_OvlLoad(OVERLAY_ID(34));
    } else {
        GFL_OvlLoad(OVERLAY_ID(33));
    }
    GCTX_HIDChangeFPS(30);
    return FIELD_ROUTINE_NEXT;
}

u32 FieldRoutine_MapLoad(GameSystem *gsys, Field *field) {
    GameData *gameData = GSYS_GetGameData(gsys);
    BOOL loading = TRUE;

    switch (field->subroutinePhase) {
    case MAP_LOAD_CAMERA:
        ZoneChange_OnFieldLoad(gameData, field->zoneId);
        field->msgBGSys = CreateFieldMsgBGSystem(field->heapId, field->g3dCamera);
        field->moneyWin = NULL;
        field->placeName = FieldPlaceName_Create(gsys, HEAPID_FIELD_PLACE_NAME, field->msgBGSys);
        field->fmissionInfo = FieldFMissionInfo_Create(gsys, field->heapId);
        field->skillMapEff =
            FieldSkillMapEff_Create(FieldStatus_GetFlashPerms(GameData_GetFieldStatus(field->gameData)), field->heapId);
        field->cameraSystem = FieldCamera_Create(GetZoneDefaultCameraIndex(field->zoneId), 0, field->g3dCamera,
                                                 &field->playerPos, field->heapId);
        field->sceneArea = FieldSceneArea_Create(field->heapId, field->cameraSystem, field);
        field->sceneAreaLoader = FieldSceneAreaLoader_Create(field->heapId);
        break;
    case MAP_LOAD_MAPPERS: {
        FieldPropSystem *propSystem;

        field->noGridMapper =
            FieldNoGridMapper_Create(field->heapId, field->cameraSystem, field->sceneArea, field->sceneAreaLoader);
        field->palaceSys = FieldPalaceSys_Create(field->heapId, gsys, field, field->zoneId);
        FieldColorPostFX_Set(&field->colorPostFX, FieldPalaceSys_GetLuminanceTable(field->palaceSys),
                             IsZoneFlashbackMemoryPostFX(field->zoneId));
        propSystem = FieldG3DMapper_GetBMSystem(field->g3DMapper);
        SetupLoadZoneMapTypeData(field->zoneId, field->areaData, &field->mapperConfig,
                                 GetMapMatrixSystem(field->gameData));
        FieldPropSystem_LoadArea(propSystem, field->zoneId, field->areaData, &field->colorPostFX);
        FieldG3DObjSystem_SetColorPostFX(field->g3DObjSystem, &field->colorPostFX);
        Field_LoadWFBC(gameData, field, field->zoneId);
        Field_LoadJoinAvenue(gameData, field, field->zoneId);
        break;
    }
    case MAP_LOAD_MAP: {
        u32 railId;

        SetupZoneWarpArrivalGrid(&field->playerZoneState, field->zoneId, 0, 0, 0, field->playerPos.x,
                                 field->playerPos.y, field->playerPos.z);
        FieldG3DMapper_CreateMap(field->g3DMapper, &field->mapperConfig, &field->colorPostFX);
        railId = GetRailIDForZone(field->zoneId);
        if (railId != ZONE_RAIL_NONE) {
            FieldNoGridMapper_LoadByHeader(field->noGridMapper, railId, field->heapId);
        }
        field->objectProjectionMatrixOffset = GetObjectProjectionMatrixOffset(field->zoneId);
        FieldCameraBoundary_ChangeZone(field, field->zoneId, field->heapId);
        field->fesGimmick = FesGimmick_Create(gsys, field, field->heapId);
        break;
    }
    case MAP_LOAD_ACTORS:
        Field_InitActorSystem(field);
        field->fieldEffects = FieldEffects_Create(field, 0x19, field->heapId);
        FieldEffects_TCBManagerInit(field->fieldEffects, 0x5e);
        FieldEffects_SetLuminanceTable(field->fieldEffects, FieldPalaceSys_GetLuminanceTable(field->palaceSys));
        FieldEffects_Load(field->fieldEffects, STATIC_LOADED_FIELD_EFFECT_IDS, data_ov036_021d0388);
        if (GetZoneIsUnionRoom(field->zoneId) == TRUE || IsZone150Or151(field->zoneId) == TRUE) {
            field->unionEffect = UnionEff_Create(field->heapId);
        }
        break;
    case MAP_LOAD_PLAYER: {
        PlayerState *playerState = GSYS_GetPlayerState(gsys);
        u16 dir = PlayerState_CalcDirection(playerState);
        VecFx32 *pos = &playerState->position;
        u8 season;

        if (FieldPlayerState_GetExState(playerState) == FLD_PLAYER_EXSTATE_CYCLING &&
            !GetZoneFlagsEnableCycling(field->zoneId)) {
            SetPlayerSpecialState(playerState, 0);
        }
        field->player = FieldPlayer_Create(playerState, field, pos, getTrainerGender(GetGameDataPlayerInfo(gameData)),
                                           field->heapId);
        field->playerPos = *pos;
        field->ctrlVTable->create(field, &field->playerPos, dir);
        FieldG3DMapper_SetPlayerPos(field->g3DMapper, &field->playerPos);
        Field_InitCommonWork(&field->gimmickWork);
        field->expObjSystem = FieldExpObj_Create(0x3c, 0x60, field->heapId);
        Field_LoadEdgeColorTable(field->areaData, Field_GetPlayerStateZoneID(field));
        field->fog = FieldFog_Create(field->heapId);
        field->fogCtrl = FieldFogCtrl_Create(field->heapId);
        FieldFogCtrl_RequestLoad(field->fogCtrl, GetZoneFogIndexAll(field, field->zoneId),
                                 GetZoneStaticLightDataIndex(field->zoneId), TRUE);
        {
            u32 lightsId = AreaData_GetLightsID(field->areaData);
            u32 daySeconds = RTC_ConvertDaySecondsCached();

            field->lightSystem = FieldLight_Create(lightsId, daySeconds, GameSystem_GetSeason(gsys), field->fog,
                                                   field->g3dLights, field->heapId);
        }
        field->weatherSystem = FieldWeather_Create(field->cameraSystem, field->lightSystem, field->fog, field->fogCtrl,
                                                   GameData_GetFieldSoundSystem(field->gameData),
                                                   GameSystem_GetSeason(gsys), HEAPID_FIELD_WEATHER);
        SetWeatherInit(field->weatherSystem, GetNowWeather(field->gameData), HEAPID_FIELD_WEATHER);
        Field_CheckGiveDiamondDustMedal(field);
        season = GameData_GetSeason(field->gameData);
        field->lensFlare = FieldLensFlare_Create(gsys, field->gameData, field->expObjSystem, season,
                                                 Field_GetDayPeriod(field), field->heapId);
        Field_InitGimmick(field);
        SetScrPluginByZone(field->gameData, Field_GetPlayerStateZoneID(field));
        LoadScrPluginOverlays(field->gameData);
        break;
    }
    case MAP_LOAD_SUBSCREEN:
        field->subscreen = FieldSubscreen_Create(field->heapId, field, GameData_GetLastSubscreen(gameData));
        break;
    case MAP_LOAD_SYSTEMS:
        field->encountSystem = EncSys_Create(field);
        GameCommSys_FieldCreate(GSYS_GetGameCommSystem(gsys), field);
        field->asyncProcManager = FieldAsyncProcManager_Create(field, field->heapId, 0x20);
        field->particleSystem = FieldParticle_Create(HEAPID_FIELD_PARTICLE);
        field->g3dCi = Fld3DCi_Create(HEAPID_FIELD_PARTICLE, field->particleSystem);
        field->encEff = EncEff_Create(field->heapId, field);
        FesGimmick_BindPlayer(field->fesGimmick, field->fieldEffects, field->player);
        field->dayCare =
            DayCare_Create(field->heapId, field, getDaycareBlockAddress(GameData_GetSaveControl(field->gameData)));
        Field_LoadSceneArea(field, field->zoneId);
        loading = FALSE;
        break;
    }
    field->subroutinePhase++;
    field->renderMode = 0;
    if (!loading) {
        return FIELD_ROUTINE_NEXT;
    }
    return FIELD_ROUTINE_CONTINUE;
}

u32 FieldRoutine_MapSpawnFinish(GameSystem *gsys, Field *field) {
    switch (field->subroutinePhase) {
    case MAP_SPAWN_FIRST_FRAME:
        FieldG3DMapper_UpdateSync(field->g3DMapper);
        if (field->actorSystem != NULL) {
            do {
                FldActSys_Update(field->actorSystem);
                FldActSys_FinishAsyncMatLoad(field->actorSystem);
            } while (FldActSys_IsAsyncLoadPending(field->actorSystem) == TRUE);
        }
        while (FieldWeather_IsTaskActive(field->weatherSystem)) {
            FieldWeather_Update(field->weatherSystem, HEAPID_FIELD_WEATHER);
        }
        FieldG3D_Update(field);
        FieldG3D_RenderPhase1(field);
        FieldG3D_RenderPhase2(field);
        ClActSys_Draw();
        if (field->msgBGSys != NULL) {
            MsgBGPrinter_Main(field->msgBGSys);
        }
        FieldEffects_Update(field->fieldEffects);
        FieldAsyncProcManager_Update(field->asyncProcManager);
        field->subroutinePhase++;
    case MAP_SPAWN_SCRIPTS:
        if (FieldStatus_CheckContinueFlag(GameData_GetFieldStatus(field->gameData)) == TRUE &&
            IsZoneTwoPassLoad(field->zoneId) == TRUE && GFL_SndIsLoadThreadRunning()) {
            return FIELD_ROUTINE_CONTINUE;
        }
        if (FieldStatus_GetNewLoadFlag(GameData_GetFieldStatus(field->gameData))) {
            FieldScript_CallOnZoneReload(field->gameSystem, field->heapId);
            FieldStatus_SetNewLoadFlag(GameData_GetFieldStatus(field->gameData), FALSE);
        } else {
            FieldScript_CallOnZoneNewLoad(field->gameSystem, field->heapId);
        }
        FieldStatus_SetContinueFlag(GameData_GetFieldStatus(field->gameData), FALSE);
        FieldG3DMapper_UpdateSync(field->g3DMapper);
        FesGimmick_OnZoneEnter(field->fesGimmick);
        if (GetZoneIsUnionRoom(field->zoneId) == TRUE || IsZone150Or151(field->zoneId) == TRUE) {
            GSYS_SetEventProvider(gsys, FieldEventProvider_UnionRoom, field);
        } else if (Field_GetControllerTypeID(field) == FIELDMAP_CTRL_TYPE_NOGRID) {
            GSYS_SetEventProvider(gsys, FieldEventProvider_NoGrid, field);
        } else if (Field_GetControllerTypeID(field) == FIELDMAP_CTRL_TYPE_HYBRID) {
            GSYS_SetEventProvider(gsys, FieldEventProvider_Hybrid, field);
        } else {
            GSYS_SetEventProvider(gsys, FieldEventProvider_Grid, field);
        }
        field->routineState = 1;
        GameData_Set30FPSMode(GSYS_GetGameData(gsys), TRUE);
        break;
    }
    return FIELD_ROUTINE_NEXT;
}

u32 FieldRoutine_Run(GameSystem *gsys, Field *field) {
    if (GSYS_GetNowEvent(field->gameSystem) == NULL) {
        FieldStatus_SetBusyFlag(GameData_GetFieldStatus(field->gameData), FLD_STATUS_BUSY_NONE);
    }
    if (GameData_CheckEventsPaused(field->gameData)) {
        return FIELD_ROUTINE_CONTINUE;
    }
    if (field->effectRunningFlag) {
        Field_RenderStart(field);
        ClActSys_Draw();
        return FIELD_ROUTINE_CONTINUE;
    }
    GCTX_HIDResetFrameCount();
    Field_CheckDoRealTimeLoad(field);
    if (!GSYS_GetEventRunningFlag(gsys)) {
        field->ctrlVTable->update(field, &field->playerPos);
        EvTime_Update(field->gameData);
    }
    if (field->placeName != NULL) {
        PlaceName_Main(field->placeName);
    }
    if (field->fmissionInfo != NULL) {
        FieldFMissionInfo_Update(field->fmissionInfo);
    }
    FieldSkillMapEff_Update(field->skillMapEff);
    FieldPlayer_SyncState(field->player);
    if (GetZoneIsUnionRoom(field->zoneId) == TRUE || IsZone150Or151(field->zoneId) == TRUE) {
        UnionMain_Update(GSYS_GetGameCommSystem(gsys), field);
    }
    FieldSubscreen_Update(field->subscreen);
    if (field->actorSystem != NULL) {
        MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
        FldActSys_Update(field->actorSystem);
        MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    }
    Field_UpdateGimmick(field);
    FieldEffects_Update(field->fieldEffects);
    if (field->unionEffect != NULL) {
        UnionEff_Update(field->unionEffect);
    }
    FieldAsyncProcManager_Update(field->asyncProcManager);
    if (field->msgBGSys != NULL) {
        MsgBGPrinter_Main(field->msgBGSys);
    }
    GFL_TCBMgrUpdate(field->tcbManager);
    FieldTaskManager_Update(field->taskManager);
    if (field->playerPosPtr != NULL) {
        field->playerPos = *field->playerPosPtr;
    }
    FieldG3DMapper_SetPlayerPos(field->g3DMapper, &field->playerPos);
    FieldG3D_Update(field);
    PleasureBoat_AckEvent(*GameData_GetPleasureBoatPtr(field->gameData));
    ClActSys_Draw();
    Field_RenderStart(field);
    return FIELD_ROUTINE_CONTINUE;
}

void Field_RenderStart(Field *field) {
    GFL_G3DSysResetGeometryCounter();
    FieldCamera_CalcTransform(field->cameraSystem, GCTX_HIDGetHeldKeys());
    field->actorYOffset = Field_CalcActorYOffset(field);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    FieldG3D_RenderPhase1(field);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
}

u32 FieldRoutine_RunAltFrame(GameSystem *gsys, Field *field) {
    if (GameData_CheckEventsPaused(field->gameData)) {
        GameData_ResetSkipFrame(GSYS_GetGameData(gsys));
        return FIELD_ROUTINE_CONTINUE;
    }
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    FieldG3DMapper_CallAllLoaderUpdate(field->g3DMapper);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    FieldSubscreen_UpdateAltFrame(field->subscreen);
    if (field->msgBGSys != NULL) {
        MsgBGPrinter_Main(field->msgBGSys);
    }
    if (field->placeName != NULL) {
        PlaceName_Draw(field->placeName);
    }
    if (field->fmissionInfo != NULL) {
        FieldFMissionInfo_UpdateDisplay(field->fmissionInfo);
    }
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    FieldG3D_RenderPhase2(field);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    GameData_ResetSkipFrame(GSYS_GetGameData(gsys));
    return FIELD_ROUTINE_CONTINUE;
}

u32 FieldRoutine_MapUnload(GameSystem *gsys, Field *field) {
    GameData *gameData;

    FesGimmick_DeleteSparkles(field->fesGimmick);
    EncEff_Free(field->encEff);
    Fld3DCi_Free(field->g3dCi);
    FieldParticle_Free(field->particleSystem);
    GSYS_SetEventProvider(gsys, NULL, NULL);
    FieldPlayer_SyncState(field->player);
    FieldAsyncProcManager_Free(field->asyncProcManager);
    GameCommSys_FieldDelete(GSYS_GetGameCommSystem(gsys), field);
    if (field->fmissionInfo != NULL) {
        FieldFMissionInfo_Free(field->fmissionInfo);
    }
    if (field->placeName != NULL) {
        FieldPlaceName_Free(field->placeName);
    }
    UnloadScrPluginOverlays(field->gameData);
    Field_TerminateGimmick(field);
    FieldLensFlare_Free(field->lensFlare);
    FieldExpObj_Free(field->expObjSystem);
    EncSys_Free(field->encountSystem);
    gameData = GSYS_GetGameData(gsys);
    GameData_SetLastSubscreen(gameData, FieldSubscreen_Free(field->subscreen));
    FieldEffects_Free(field->fieldEffects);
    if (field->unionEffect != NULL) {
        UnionEff_Free(field->unionEffect);
        field->unionEffect = NULL;
    }
    FieldWeather_Delete(field->weatherSystem);
    field->weatherSystem = NULL;
    FieldLight_Free(field->lightSystem);
    FieldFogCtrl_Free(field->fogCtrl);
    FieldFog_Free(field->fog);
    FieldPalaceSys_Free(field->palaceSys);
    FieldNoGridMapper_Free(field->noGridMapper);
    FreeFieldSceneAreaLoader(field->sceneAreaLoader);
    FreeFieldSceneArea(field->sceneArea);
    FieldCamera_Free(field->cameraSystem);
    field->ctrlVTable->free(field);
    Field_ResetController(field);
    Field_SuspendActorSystem(field);
    FesGimmick_Free(field->fesGimmick);
    FieldPlayer_Free(field->player);
    FieldG3DMapper_FreeMap(field->g3DMapper);
    if (field->moneyWin != NULL) {
        MsgBGPrinter_DeleteMsgWin(field->moneyWin);
    }
    if (field->msgBGSys != NULL) {
        MsgBGPrinter_Delete(field->msgBGSys);
    }
    GameData_Set30FPSMode(GSYS_GetGameData(gsys), FALSE);
    GCTX_HIDChangeFPS(60);
    DayCare_Free(field->dayCare);
    FieldSkillMapEff_Free(field->skillMapEff);
    return FIELD_ROUTINE_NEXT;
}

u32 FieldRoutine_Close(GameSystem *gsys, Field *field) {
    GFL_TCBMgrDelete(field->tcbManager);
    GFL_HeapFree(field->tcbManagerHeap);
    FieldDispControl_Free(field->dispControl);
    field->dispControl = NULL;
    FieldTaskManager_Free(field->taskManager);
    GFL_TCBRemove(field->asyncActorMatLoadTCB);
    FieldG3DMapper_Free(field->g3DMapper);
    FieldG3D_Free(field);
    ClActSys_Delete();
    BmpWin_FreeAllocator();
    Field_FreeGraphicsSystems(field);
    if (GetZoneIsUnionRoom(field->zoneId) == TRUE || IsZone150Or151(field->zoneId) == TRUE) {
        GFL_OvlUnload(OVERLAY_ID(34));
    } else {
        GFL_OvlUnload(OVERLAY_ID(33));
    }
    return FIELD_ROUTINE_END;
}

BOOL Field_CallRoutines(GameSystem *gsys, Field *field) {
    u32 result = 0;
    FieldRoutine routine;

    field->framesSinceStart++;
    routine = FIELDMAP_ROUTINES[field->routineID][field->routineAlternator];
    if (routine != NULL) {
        result = routine(gsys, field);
    }
    switch (result) {
    case FIELD_ROUTINE_CONTINUE:
        field->routineAlternator = field->routineAlternator == 0 ? 1 : 0;
        break;
    case FIELD_ROUTINE_NEXT:
        field->routineID++;
        field->subroutinePhase = 0;
        field->routineAlternator = 0;
        break;
    case FIELD_ROUTINE_END:
        return TRUE;
    }
    return FALSE;
}

void Field_RequestClose(Field *field) {
    field->routineState = 2;
    field->routineID = 4;
}

BOOL Field_CheckMapLoadFinished(Field *field) {
    return field->routineState == 1;
}

BOOL Field_ToggleCycling(Field *field) {
    BOOL changed = FALSE;
    u32 exState = FieldPlayer_GetExState(field->player);

    if (exState == FLD_PLAYER_EXSTATE_CYCLING) {
        FieldPlayer_SetSpecialSeq(field->player, 1);
        changed = TRUE;
    } else if (exState == FLD_PLAYER_EXSTATE_NONE) {
        changed = TRUE;
        GFL_SndSEPlay(SEQ_SE_BICYCLE);
        FieldPlayer_SetSpecialSeq(field->player, 2);
        RecordAddOne(GameData_GetRecords(field->gameData), 3);
    }

    if (changed == TRUE) {
        FieldPlayer_StepFormChange(field->player);
    }
    return changed;
}

FieldMsgBG *Field_GetMsgBGSys(Field *field) {
    return field->msgBGSys;
}

FieldCamera *Field_GetCameraSystem(Field *field) {
    return field->cameraSystem;
}

NoGridMapper *Field_GetNoGridMapper(Field *field) {
    return field->noGridMapper;
}

FieldLight *Field_GetLightSystem(Field *field) {
    return field->lightSystem;
}

FieldFog *Field_GetFog(Field *field) {
    return field->fog;
}

FieldFogCtrl *Field_GetFogCtrl(Field *field) {
    return field->fogCtrl;
}

FieldWeather *Field_GetWeatherSystem(Field *field) {
    return field->weatherSystem;
}

u32 Field_GetWeatherForZone(Field *field, u16 zoneId) {
    return GetWeatherAll(field->gameSystem, zoneId);
}

MMSys *Field_GetActorSystem(Field *field) {
    return field->actorSystem;
}

GameSystem *Field_GetGameSystem(Field *field) {
    return field->gameSystem;
}

u16 Field_GetHeapID(Field *field) {
    return field->heapId;
}

BillboardActorSys *Field_GetEffectBillboardActor(Field *field) {
    return field->effectBillboardActor;
}

BillboardActorSys *Field_GetWildEffectBillboardActor(Field *field) {
    return field->wildEffectBillboardActor;
}

FieldG3DMapper *Field_GetG3DMapper(Field *field) {
    return field->g3DMapper;
}

u16 Field_GetPlayerStateZoneID(Field *field) {
    return field->playerZoneState.zoneId;
}

void *Field_GetController(Field *field) {
    return field->controller;
}

void Field_SetController(Field *field, void *controller) {
    field->controller = controller;
}

FieldPlayer *Field_GetPlayer(Field *field) {
    return field->player;
}

BOOL Field_HasPlayer(Field *field) {
    return field->player != NULL;
}

FieldSubscreen *Field_GetSubscreen(Field *field) {
    return field->subscreen;
}

FieldEffects *Field_GetFieldEffects(Field *field) {
    return field->fieldEffects;
}

FieldG3DObjSystem *Field_GetG3DObjSys(Field *field) {
    return field->g3DObjSystem;
}

UnionEffect *Field_GetUnionEff(Field *field) {
    return field->unionEffect;
}

EncountSystem *Field_GetEncountSystem(Field *field) {
    return field->encountSystem;
}

u32 Field_GetControllerTypeID(Field *field) {
    return field->ctrlVTable->typeId;
}

u32 Field_GetResolvedControllerTypeID(Field *field) {
    u32 type = sResolvedControllerTypes[field->ctrlVTable->typeId];
    if (type == FIELDMAP_CTRL_TYPE_HYBRID) {
        type = FieldmapCtrlHybrid_GetActiveTypeID(field->controller);
    }
    return type;
}

PlaceName *Field_GetPlaceName(Field *field) {
    return field->placeName;
}

FesGimmick *Field_GetFesGimmick(Field *field) {
    return field->fesGimmick;
}

FieldAsyncProcManager *Field_GetAsyncProcMgr(Field *field) {
    return field->asyncProcManager;
}

FieldExpObjSystem *Field_GetExpObjSystem(Field *field) {
    return field->expObjSystem;
}

TCBManager *Field_GetTCBMgr(Field *field) {
    return field->tcbManager;
}

FieldTaskManager *Field_GetTaskManager(Field *field) {
    return field->taskManager;
}

void Field_SetPlayerPosPtr(Field *field, VecFx32 *position) {
    field->playerPosPtr = position;
}

MoneyWin *Field_GetMoneyWin(Field *field) {
    return field->moneyWin;
}

void Field_SetMoneyWin(Field *field, MoneyWin *moneyWin) {
    field->moneyWin = moneyWin;
}

DayCare *Field_GetDayCare(Field *field) {
    return field->dayCare;
}

AreaData *Field_GetAreaData(Field *field) {
    return field->areaData;
}

void FieldG2D_Init(Field *field) {
    GFL_BGSysCreate(field->heapId);
    GFL_BGSysUploadStdPalette(0, FIELD_STD_PALETTE, 0x20, 0);
    GFL_BGSysUploadStdPalette(4, FIELD_STD_PALETTE, 0x20, 0);
    GFL_BGSysSetDisplayLayout(1);
    GFL_BGSysEnableEngines();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
}

void FieldG2D_SetLCDConfig(void) {
    GFL_BGSysSetLCDConfig(&FIELD_LCD_CONFIG);
}

void FieldG2D_Prepare3DSurface(Field *field) {
    MtxFx22 mtx;

    MAT2_SetScaleRot(&mtx, 0, FX32_ONE, FX32_ONE, MAT2_ROT_IDX);
    G2_SetBG2Affine(&mtx, 0, 0, 0, 0);
    GFL_BGSysSet3DBGPriority(3);
    MsgBGPrinter_SetupBg(field->msgBGSys);
}

void FieldG2D_Prepare3DSurfaceDelayed(Field *field) {
    MtxFx22 mtx;

    MAT2_SetScaleRot(&mtx, 0, FX32_ONE, FX32_ONE, MAT2_ROT_IDX);
    G2_SetBG2Affine(&mtx, 0, 0, 0, 0);
    GFL_BGSysSet3DBGPriority(3);
    MsgBGPrinter_SetupBgDelayed(field->msgBGSys);
}

FieldDispControl *FieldG2D_GetDispControl(Field *field) {
    return field->dispControl;
}

void Field_FreeGraphicsSystems(Field *field) {
    GFL_BGSysSetDisplayLayout(1);
    NNS_G3dGeFlushBuffer();
    GFL_G3DSysFree();
    GFL_BGSysFree();
}

void FieldG3D_InitCallback(void) {
    G3X_SetShading(GX_SHADING_TOON);
    G3X_AntiAlias(TRUE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
    G3X_SetFog(FALSE, GX_FOGBLEND_COLOR_ALPHA, GX_FOGSLOPE_0x0400, 0xe00);
    G3X_SetFogColor(GX_RGB(31, 31, 31), 0);
    G3X_SetFogTable(FIELD_DEFAULT_FOG_TABLE);
    G3X_SetClearColor(GX_RGB(0, 0, 0), 31, 0x7fff, 0, FALSE);
    G3X_SetEdgeColorTable(FIELD_DEFAULT_EDGE_COLOR_TABLE);
    G3X_EdgeMarking(FALSE);
    G3_ViewPort(0, 0, 255, 191);
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_MANUAL, GX_BUFFERMODE_Z);
}

// The field's camera at its default place: a 40-degree perspective at 4:3, from 1 to 1024 units
static inline G3DCamera *FieldG3D_CreateCamera(HeapID heapId) {
    VecFx32 up = { 0, FX32_ONE, 0 };

    return GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)),
                               FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024), 0, &DEFAULT_FIELDCAMERA_POS, &up,
                               &DEFAULT_FIELDCAMERA_TGT, heapId);
}

void FieldG3D_Init(Field *field) {
    BillboardActorScene *scene;

    field->actorBillboardActor = BillboardActorSys_Create(0x40, 0x80, FldActSys_VRAMUploadFunc, field->heapId);
    scene = BillboardActorSys_GetScene(field->actorBillboardActor);
    BillboardActorScene_SetGeomOrigin(scene, 2);
    BillboardActorScene_SetTexcoordOffset(scene, 0, -0xa00);
    field->effectBillboardActor = BillboardActorSys_Create(0x3a, 0x50, FldActSys_VRAMUploadFunc, field->heapId);
    field->wildEffectBillboardActor = BillboardActorSys_Create(0x1e, 0x28, FldActSys_VRAMUploadFunc, field->heapId);
    scene = BillboardActorSys_GetScene(field->wildEffectBillboardActor);
    {
        VecFx32 scale = { FX32_CONST(7), FX32_CONST(7), FX32_CONST(7) };
        u8 polyId;

        BillboardActorScene_SetScale(scene, &scale);
        polyId = 0;
        BillboardActorScene_SetBasePolyID(scene, &polyId);
    }
    Field_LoadActorMatColorPreset(field);
    field->g3DObjSystem = FieldG3DObjSystem_Create(field->heapId, 0x40, 0x40);
    field->g3dCamera = FieldG3D_CreateCamera(field->heapId);
    field->g3dLights = GFL_G3DLightCreate(&FIELD_LIGHT_SETUP, field->heapId);
    GFL_G3DCameraFlush(field->g3dCamera);
    GFL_G3DLightFlush(field->g3dLights);
}

void FieldG3D_Update(Field *field) {
    BOOL loading;

    BillboardActorSys_UpdateActors(field->actorBillboardActor);
    BillboardActorSys_UpdateActors(field->effectBillboardActor);
    BillboardActorSys_UpdateActors(field->wildEffectBillboardActor);
    FieldNoGridMapper_UpdateCamera(field->noGridMapper);
    FieldCameraArea_Update(field->sceneArea, &field->playerPos);
    FieldFogCtrl_Update(field->fogCtrl, field->heapId);
    FieldWeather_Update(field->weatherSystem, HEAPID_FIELD_WEATHER);
    FieldLensFlare_Update(field->lensFlare, field->cameraSystem);
    FieldFog_Update(field->fog);
    FieldLight_Update(field->lightSystem, RTC_ConvertDaySecondsCached());
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    loading = FieldG3DMapper_Update(field->g3DMapper);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    if (loading) {
        GFL_NetHoldScan(2);
    }
}

void FieldG3D_RenderPhase1(Field *field) {
    FieldRenderFunc render;

    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
    render = FIELD_RENDER_FUNCS_PHASE1[field->renderMode];
    if (render != NULL) {
        render(field);
    }
}

void FieldG3D_RenderPhase2(Field *field) {
    FIELD_RENDER_FUNCS_PHASE2[field->renderMode](field);
    GFL_G3DSysReqSwapBuffers();
}

void FieldG3D_Free(Field *field) {
    GFL_G3DLightFree(field->g3dLights);
    GFL_G3DCameraFree(field->g3dCamera);
    BillboardActorSys_Free(field->actorBillboardActor);
    BillboardActorSys_Free(field->effectBillboardActor);
    BillboardActorSys_Free(field->wildEffectBillboardActor);
    FieldG3DObjSystem_Free(field->g3DObjSystem);
}

void FldActSys_AsyncMatLoadTCBFunc(TCB *tcb, void *data) {
    Field *field = data;

    if (!GFL_IRQCartDataTransferIsEnabled()) {
        if (field->actorSystem != NULL) {
            FldActSys_FinishAsyncMatLoadSafe(field->actorSystem);
        }
        if (field->g3DObjSystem != NULL) {
            FieldG3DObjSystem_SetupResources(field->g3DObjSystem);
        }
    }
    ClActSys_VBlank();
    if (field->dispControl != NULL) {
        FieldDispControl_Update(field->dispControl);
    }
}

void FldActSys_VRAMUploadFunc(BOOL type, u32 dest, void *src, u32 size) {
    NNS_GfdRegisterNewVramTransferTask(!type ? NNS_GFD_DST_3D_TEX_VRAM : NNS_GFD_DST_3D_TEX_PLTT, dest, src, size);
}

void Field_LoadEdgeColorTable(AreaData *area, u16 zoneId) {
    u32 tableId = AreaData_GetEdgeColorTableID(area);

    if (tableId == AREA_EDGE_COLOR_TABLE_NONE) {
        G3X_EdgeMarking(FALSE);
        return;
    }
    if (IsZoneEntralinkEdgeColorTable(zoneId)) {
        G3X_EdgeMarking(FALSE);
    } else {
        G3X_EdgeMarking(TRUE);
    }
    GFL_ArcSysRead(g_FieldEdgeColorTable, ARCID_AREA_EDGE_COLOR_TABLES, tableId);
    g_FieldEdgeColorTable[7] = 0x4210;
    G3X_SetEdgeColorTable(g_FieldEdgeColorTable);
}

void Field_LoadActorMatColorPreset(Field *field) {
    HeapID heapId = HEAPID_TAIL(field->heapId);
    u32 presetId = AreaData_GetActorMatColorID(field->areaData);
    FldActMatColorPreset *preset = FldActMatColorPreset_Create(heapId);

    FldActMatColorPreset_Load(preset, presetId);
    FldActMatColorPreset_Apply(preset, BillboardActorSys_GetScene(field->actorBillboardActor));
    FldActMatColorPreset_Free(preset);
}

void Field_InitActorSystem(Field *field) {
    GameData *gameData = GSYS_GetGameData(field->gameSystem);
    MMSys *actorSystem = GameData_GetMMSys(gameData);

    field->actorSystem = actorSystem;
    FldActSys_AttachField(actorSystem, field->heapId, gameData, field, field->g3DMapper, field->noGridMapper,
                          &field->colorPostFX);
    FldActSys_InitBillboardActor(field->actorSystem, field->actorBillboardActor, 0x20);
    FldActSys_LoadCachedBillboardActor(field->actorSystem);
    FldActSys_LoadStaticBillboardActor(
        actorSystem,
        FieldPlayer_GetObjCodeByExState(getTrainerGender(GetGameDataPlayerInfo(gameData)), FLD_PLAYER_EXSTATE_NONE));
    FesGimmick_BindActorSystem(field->fesGimmick, actorSystem);
    FieldActorG3DSystem_Create(field->actorSystem, field->g3DObjSystem);
    FldActSys_InitDraw(field->actorSystem, FieldCamera_GetViewYawPtr(field->cameraSystem));
    FldActSys_WakeUpAllActors(field->actorSystem);
    FldActSys_SetShadowsEnabled(actorSystem, IsZoneShadowsEnabled(field->zoneId));
}

void Field_SuspendActorSystem(Field *field) {
    FldActSys_SuspendAllActors(field->actorSystem);
    FldActSys_DetachField(field->actorSystem);
    field->actorSystem = NULL;
}

BOOL Field_CheckDoRealTimeLoad(Field *field) {
    EventData *eventData;

    if (Field_UpdateZoneStatePos(field) == TRUE && Field_ShouldSwapZone(field) == TRUE) {
        Field_SwapZoneByMatrix(field);
        return TRUE;
    }
    eventData = GameData_GetEventData(field->gameData);
    if (!IsEncountDataLoaded(eventData)) {
        MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
        EventData_LoadEncData(eventData, field->playerZoneState.zoneId, GameData_GetSeason(field->gameData));
        MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    }
    return FALSE;
}

BOOL Field_UpdateZoneStatePos(Field *field) {
    ZoneSpawnInfo *zoneState = &field->playerZoneState;
    VecFx32 *playerPos = PlayerState_GetWPos(GameData_GetPlayerState(GSYS_GetGameData(field->gameSystem)));
    VecFx32 statePos;

    GetPlayerZoneStateWPos(zoneState, &statePos);
    if (playerPos->x != statePos.x || playerPos->z != statePos.z) {
        SetPlayerZoneStateWPos(zoneState, playerPos);
        return TRUE;
    }
    return FALSE;
}

BOOL Field_ShouldSwapZone(Field *field) {
    ZoneSpawnInfo *zoneState = &field->playerZoneState;
    MapMatrix *matrix = GetMapMatrixSystem(field->gameData);
    VecFx32 pos;

    GetPlayerZoneStateWPos(zoneState, &pos);
    if (RangeCheckChunkCoordinateWorld(matrix, pos.x, pos.z) == TRUE) {
        u16 zoneId = GetZoneIDAtMatrixXZWorld(matrix, pos.x, pos.z);

        if (zoneId != MATRIX_ZONE_NONE && zoneId != zoneState->zoneId) {
            return TRUE;
        }
    }
    return FALSE;
}

void Field_SwapZoneByMatrix(Field *field) {
    ZoneSpawnInfo *zoneState = &field->playerZoneState;
    GameData *gameData = GSYS_GetGameData(field->gameSystem);
    EventData *eventData = GameData_GetEventData(gameData);
    MMSys *actorSystem = field->actorSystem;
    MapMatrix *matrix = GetMapMatrixSystem(field->gameData);
    VecFx32 pos;
    u32 zoneId;

    GFL_NetHoldScan(2);
    EncountSystem_CancelPhenomenon(field->encountSystem);
    GetPlayerZoneStateWPos(zoneState, &pos);
    zoneId = GetZoneIDAtMatrixXZWorld(matrix, pos.x, pos.z);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    DeleteAllActors(actorSystem);
    EventData_Reset(eventData);
    EventData_LoadEntities(eventData, zoneId, GameData_GetSeason(gameData));
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    ZoneChange_OnSeamless(gameData, field);
    Field_SwapZoneBGM(field, zoneId);
    Field_SwapFog(field, zoneId);
    Field_SwapWeather(field, zoneId);
    Field_CheckGiveDiamondDustMedal(field);
    Field_SwapCameraBoundaries(field, zoneId);
    Field_UpdatePlayerStateZoneID(gameData, zoneId);
    Field_LoadSceneArea(field, zoneId);
    if (field->placeName != NULL && !GSYS_GetEventRunningFlag(field->gameSystem)) {
        PlaceName_OnZoneChange(field->placeName, zoneId);
    }
    zoneState->zoneId = zoneId;
    GameBeacon_SetZone(zoneId, gameData);
    SetTeleportZoneDiscover(gameData, zoneId);
    FieldScript_CallOnZoneInit(field->gameSystem, 4);
    resetRebattleTrainers(GameData_GetEventWork(gameData));
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM9);
    Field_HotswapSpawnActors(field, gameData, actorSystem, eventData, zoneId);
    FestMission_OnZoneEnter(field->gameSystem, zoneId);
    FesGimmick_OnZoneEnter(field->fesGimmick);
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    func_ov012_02153668(GSYS_GetGameCommSystem(field->gameSystem));
}

void Field_HotswapSpawnActors(Field *field, GameData *gameData, MMSys *actorSystem, EventData *eventData, u32 zoneId) {
    u32 count = GetZoneNPCsCount(eventData);

    if (count != 0) {
        EventWork *eventWork = GameData_GetEventWork(gameData);

        SpawnAllZoneNPCs(actorSystem, GetZoneNPCs(eventData), zoneId, count, eventWork);
    }
}

void Field_SwapZoneBGM(Field *field, u32 zoneId) {
    GameData *gameData = GSYS_GetGameData(field->gameSystem);
    ISS *iss = GameSystem_GetISS(field->gameSystem);
    FieldSound *fieldSound = GameData_GetFieldSoundSystem(gameData);
    u32 exState = FieldPlayerState_GetExState(GameData_GetPlayerState(gameData));

    ISS_ChangeZone(iss, zoneId);
    if (exState != FLD_PLAYER_EXSTATE_SURF) {
        FieldSnd_ChangeZoneBGM(fieldSound, gameData, zoneId);
    }
}

void Field_SwapWeather(Field *field, u32 zoneId) {
    FieldWeather *weather = Field_GetWeatherSystem(field);
    u16 weatherId;

    ResetWeather(field->gameSystem, zoneId);
    weatherId = GetNowWeather(field->gameData);
    if (weatherId != WEATHER_MAX) {
        FieldWeather_RequestChange(weather, weatherId);
    }
}

void Field_CheckGiveDiamondDustMedal(Field *field) {
    if (GetNowWeather(field->gameData) == WEATHER_DIAMOND_DUST) {
        MedalBox_GiveMedal(SaveControl_GetMedalBox(GameData_GetSaveControl(field->gameData)), 0x65);
    }
}

void Field_SwapFog(Field *field, u32 zoneId) {
    u32 fogIndex;

    FieldFogCtrl_Reset(Field_GetFogCtrl(field));
    fogIndex = GetZoneFogIndexAll(field, zoneId);
    FieldFogCtrl_RequestLoad(field->fogCtrl, fogIndex, GetZoneStaticLightDataIndex(zoneId), TRUE);
}

void Field_UpdatePlayerStateZoneID(GameData *gameData, u32 zoneId) {
    PlayerState_SetZoneID(GameData_GetPlayerState(gameData), zoneId);
}

void Field_SwapCameraBoundaries(Field *field, u32 zoneId) {
    FieldCameraBoundary_ChangeZone(field, zoneId, field->heapId);
}

void Field_ResetController(Field *field) {
    field->controller = NULL;
}

void FieldCameraBoundary_ChangeZone(Field *field, u32 zoneId, HeapID heapId) {
    u16 boundaryId = GetZoneMatrixCamBoundIdx(zoneId);

    if (boundaryId != ZONE_CAMERA_BOUNDARY_NONE) {
        FieldCameraBoundary_ChangeID(field->cameraSystem, boundaryId, heapId);
    } else {
        FieldCameraBoundary_LoadDummy(field->cameraSystem);
    }
}

void Field_LoadWFBC(GameData *gameData, Field *field, u32 zoneId) {
    FieldStatus *status = GameData_GetFieldStatus(gameData);

    if (IsZoneBlackCityOrWhiteForestLobby(field->zoneId)) {
        BOOL isOther = FieldStatus_GetInLinkedWorld(status);
        CityState *city;

        if (isOther == FALSE) {
            city = GameData_GetMyCityState(field->gameData);
        } else {
            city = GameData_GetCityState(field->gameData, 0);
        }
        FieldG3DMapper_SetupWfbc(field->g3DMapper, city, isOther);
    }
}

void Field_LoadJoinAvenue(GameData *gameData, Field *field, u32 zoneId) {
    ResortMapCreateWork *resortMap = FieldG3DMapper_GetResortMap(field->g3DMapper);
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));
    JoinAvenueInfo *info = JoinAvenue_GetInfo(joinAvenue);
    JoinAvenueOccupants *occupants = getAddressOfBeginningOfOccupants(joinAvenue);

    if (IsZoneJoinAvenue(zoneId)) {
        ResortMapCreate_Setup(resortMap, zoneId, info, occupants);
        ResortMapCreate_MoveShopEntities(resortMap, GameData_GetEventData(gameData));
    }
}

void Field_LoadSceneArea(Field *field, u32 zoneId) {
    u16 cameraId = GetCameraIDForZone(zoneId);

    if (cameraId != ZONE_CAMERA_NONE) {
        CameraArea *cameraData;
        u32 count;

        ResetSceneArea(field->sceneArea);
        ResetSceneAreaLoader(field->sceneAreaLoader);
        LoadCameraDataToSceneAreaLoader(field->sceneAreaLoader, 1, cameraId, HEAPID_FIELD_SCENEAREA);
        cameraData = GetFldSceneAreaLoaderCameraData(field->sceneAreaLoader);
        count = GetFldSceneAreaLoaderCamCount(field->sceneAreaLoader);
        BuildSceneArea(field->sceneArea, cameraData, count,
                       GetFldSceneAreaLoaderCamFuncsStaticOffs(field->sceneAreaLoader));
        return;
    }
    if (!GetZoneHasRailSystem(zoneId)) {
        ResetSceneArea(field->sceneArea);
        ResetSceneAreaLoader(field->sceneAreaLoader);
    }
}

u32 Field_GetCommKind(GameCommSys *comm) {
    u32 state = GameCommSys_BootCheck(comm);

    switch (state) {
    case GAME_COMM_NO_NULL:
        if (GFL_NetErrCheck()) {
            state = GameCommSys_GetLastCommNo(comm);
        }
        return state;
    case GAME_COMM_NO_BEACON_SEARCH:
    case GAME_COMM_NO_INTRUDE:
    case GAME_COMM_NO_UNK5:
        return state;
    }
    return GAME_COMM_NO_INTRUDE;
}

BOOL Field_CheckCommExit(GameCommSys *comm) {
    if (GameCommSys_BootCheck(comm) == GAME_COMM_NO_NULL) {
        return TRUE;
    }
    if (!GameCommSys_IsTransitioning(comm)) {
        GameCommSys_ExitReq(comm);
    }
    return FALSE;
}

void Field_InitCommonWork(FieldGimmickWorkBlock *block) {
    block->password = FIELD_GIMMICK_PASSWORD_NONE;
    block->work = NULL;
}

void *Field_AllocGimmickWorkBlock(Field *field, u32 password, HeapID heapId, u32 size) {
    FieldGimmickWorkBlock *block = &field->gimmickWork;

    if (password == FIELD_GIMMICK_PASSWORD_NONE) {
        return NULL;
    }
    if (block->password != FIELD_GIMMICK_PASSWORD_NONE) {
        return NULL;
    }
    block->password = password;
    block->work = GFL_HeapAllocate(heapId, size, TRUE, "fieldmap.c", 0xe76);
    return block->work;
}

void Field_DeleteGimmickWorkBlock(Field *field, u32 password) {
    FieldGimmickWorkBlock *block = &field->gimmickWork;

    if (password != FIELD_GIMMICK_PASSWORD_NONE) {
        GFL_HeapFree(block->work);
        block->password = FIELD_GIMMICK_PASSWORD_NONE;
    }
}

BOOL Field_CheckGimmickWorkPassword(Field *field, u32 password) {
    if (field->gimmickWork.password == password) {
        return TRUE;
    }
    return FALSE;
}

void *Field_GetGimmickWorkBlock(Field *field, u32 password) {
    FieldGimmickWorkBlock *block = &field->gimmickWork;

    if (password == FIELD_GIMMICK_PASSWORD_NONE) {
        return NULL;
    }
    if (password != block->password) {
        return NULL;
    }
    return block->work;
}

void FieldRenderPhase1_Fieldmap(Field *field) {
    FieldFog_Flush(field->fog);
    FieldLight_Flush(field->lightSystem, FALSE);
    GFL_G3DCameraFlush(field->g3dCamera);
    GFL_G3DLightFlush(field->g3dLights);
    FieldG3D_RenderFieldmap(field->g3DMapper, field->g3dCamera, 0);
}

void FieldRenderPhase2_Fieldmap(Field *field) {
    MtxFx44 saved;
    MtxFx44 mtx;

    if (field->msgBGSys != NULL) {
        MsgBGPrinter_DrawTalkSys(field->msgBGSys);
    }
    FieldG3D_RenderFieldmap(field->g3DMapper, field->g3dCamera, 1);
    FieldSkillMapEff_Draw(field->skillMapEff);
    saved = NNS_G3dGlb.projMtx;
    mtx = saved;
    mtx.m[3][2] += FX_Mul(mtx.m[2][2], field->objectProjectionMatrixOffset);
    NNS_G3dGlbSetProjectionMtx(&mtx);
    NNS_G3dGlbFlushP();
    NNS_G3dGeFlushBuffer();
    FieldEffects_Draw(field->fieldEffects);
    FieldG3DObjSystem_Draw(field->g3DObjSystem);
    BillboardActorSys_Draw(field->actorBillboardActor, field->g3dCamera, field->g3dLights);
    BillboardActorSys_Draw(field->wildEffectBillboardActor, field->g3dCamera, field->g3dLights);
    if (field->unionEffect != NULL) {
        UnionEff_Draw(field->unionEffect);
    }
    mtx = saved;
    mtx.m[3][2] += FX_Mul(mtx.m[2][2], field->objectProjectionMatrixOffset / 2);
    NNS_G3dGlbSetProjectionMtx(&mtx);
    NNS_G3dGlbFlushP();
    NNS_G3dGeFlushBuffer();
    BillboardActorSys_Draw(field->effectBillboardActor, field->g3dCamera, field->g3dLights);
    NNS_G3dGlbSetProjectionMtx(&saved);
    NNS_G3dGlbFlushP();
    FieldAsyncProcManager_Draw(field->asyncProcManager);
    FieldWeather_Draw(field->weatherSystem);
    FieldExpObj_Draw(field->expObjSystem);
    FieldParticle_DrawAndUpdateAll();
    Fld3DCi_Draw(field->g3dCi);
}

void FieldRenderPhase2_FieldEffect(Field *field) {
    G3X_SetClearColor(0, 0, 0x7fff, 0, FALSE);
    FieldParticle_DrawAndUpdateAll();
    Fld3DCi_Draw(field->g3dCi);
}

void FieldRenderPhase2_EncountEffect(Field *field) {
    G3X_SetClearColor(0x4210, 31, 0x7fff, 0, FALSE);
    EncEff_CallRenderFunc(field->encEff);
}

u32 Field_GetRenderMode(Field *field) {
    return field->renderMode;
}

void Field_SetRenderMode(Field *field, u32 mode) {
    field->renderMode = mode;
}

Fld3DCi *Field_Get3DCi(Field *field) {
    return field->g3dCi;
}

EncEff *Field_GetEncEff(Field *field) {
    return field->encEff;
}

FieldSkillMapEff *Field_GetSkillMapEff(Field *field) {
    return field->skillMapEff;
}

FieldSceneArea *Field_GetSceneArea(Field *field) {
    return field->sceneArea;
}

void Field_SetFadeFlag(Field *field, BOOL flag) {
    field->fadeFlag = flag;
}

BOOL Field_GetFadeFlag(Field *field) {
    return field->fadeFlag;
}

BOOL Field_IsEventRunning(Field *field) {
    return GSYS_GetEventRunningFlag(field->gameSystem);
}

u16 Field_GetDayPeriod(Field *field) {
    return GetRealTimeDayPeriod(GameData_GetSeason(field->gameData));
}

BOOL Field_GetSeasonBannerOverdrawFlag(Field *field) {
    return field->seasonBannerOverdrawFlag;
}

void Field_SetSeasonBannerOverdrawFlag(Field *field, BOOL flag) {
    field->seasonBannerOverdrawFlag = flag;
}

void Field_SetEffectRunningFlag(Field *field, BOOL flag) {
    field->effectRunningFlag = flag;
}

FaceUpWork **Field_GetNDemoDataHandle(Field *field) {
    return &field->nDemoData;
}

void Field_SetCasteliaRush(Field *field, CasteliaRush *rush) {
    field->casteliaRush = rush;
}

CasteliaRush *Field_GetCasteliaRush(Field *field) {
    return field->casteliaRush;
}

FieldLensFlare *Field_GetLensFlare(Field *field) {
    return field->lensFlare;
}

FieldColorPostFX *Field_GetColorPostFX(Field *field) {
    return &field->colorPostFX;
}

fx32 Field_GetActorYOffset(Field *field) {
    return field->actorYOffset;
}

// How far below the camera the actors are drawn, from the camera's pitch
fx32 Field_CalcActorYOffset(Field *field) {
    u16 *pitch = FieldCamera_GetViewPitchPtr(field->cameraSystem);
    fx32 cos = FX_CosIdx(*pitch);
    fx32 offset;

    if (cos < 0) {
        cos = -cos;
    }
    offset = FX_Mul(cos, FX32_CONST(4)) - FX32_CONST(4);
    if (offset < FX32_CONST(-2)) {
        offset = FX32_CONST(-2);
    }
    return offset;
}

u32 GetZoneFogIndexAll(Field *field, u16 zoneId) {
    if (zoneId == ZONE_OPELUCID_CITY && !GameData_IsMapReplaceEventSet(field->gameData, 0)) {
        return 0xfffffff;
    }
    return GetZoneFogIndex(zoneId);
}

u32 GetObjectProjectionMatrixOffset(u16 zoneId) {
    if (ZoneData_GetObjectProjectionMatrixType(zoneId) == 1) {
        return 0x1ee;
    }
    return 0x136;
}

BOOL IsZoneShadowsEnabled(u16 zoneId) {
    if (zoneId == ZONE_NIMBASA_CITY_17) {
        return FALSE;
    }
    if (IsZone150Or151(zoneId) == TRUE) {
        return FALSE;
    }
    if (zoneId == ZONE_ABYSSAL_RUINS) {
        return FALSE;
    }
    if (zoneId == ZONE_ABYSSAL_RUINS_2) {
        return FALSE;
    }
    if (zoneId == ZONE_ABYSSAL_RUINS_3) {
        return FALSE;
    }
    if (zoneId == ZONE_ABYSSAL_RUINS_4) {
        return FALSE;
    }
    return TRUE;
}

BOOL IsZoneTwoPassLoad(u16 zoneId) {
    if (zoneId == ZONE_MISTRALTON_CITY_GYM)
        return TRUE;
    if (zoneId == ZONE_NIMBASA_CITY_17)
        return TRUE;
    if (zoneId == ZONE_POKEMON_LEAGUE_8)
        return TRUE;
    return FALSE;
}

BOOL IsZoneFirstBlackTowerOrWhiteTreehollow(u16 zoneId) {
    switch (zoneId) {
    case ZONE_WHITE_TREEHOLLOW:
    case ZONE_BLACK_TOWER:
        return TRUE;
    }
    return FALSE;
}

u32 GetZoneMapType2(u16 zoneId) {
    return GetZoneMapType(zoneId);
}

void SetupLoadZoneMapTypeData(u16 zoneId, AreaData *area, FieldG3DMapperConfig *config, MapMatrix *matrix) {
    u32 type = GetZoneMapType2(zoneId);

    *config = MAP_CONFIGS[type].mapperConfig;
    if (MAP_CONFIGS[type].useMapMatrix) {
        config->matrixWidth = GetMapMatrixWidth(matrix);
        config->matrixHeight = GetMapMatrixHeight(matrix);
        config->chunkIdCount = GetMapMatrixChunkIDCount(matrix);
        config->chunkDatIds = GetMapMatrixChunkIDs(matrix);
    }
    config->isSharedTexResource = TRUE;
    config->textures.arcId = ARCID_AREA_MAP_TEX;
    config->textures.fileId = AreaData_GetTexSetID(area);
    config->terrainAnmInfo.srtAnimeId = AreaData_GetSRTAnmID(area);
    config->terrainAnmInfo.patAnimeId = AreaData_GetPatAnmID(area);
}

const FieldmapCtrlVTable *GetZoneFieldmapCtrlVTable(u16 zoneId) {
    return MAP_CONFIGS[GetZoneMapType2(zoneId)].ctrlVTable;
}

u32 GetFieldmapZoneHeapSize(u16 zoneId) {
    return MAP_CONFIGS[GetZoneMapType2(zoneId)].heapSize;
}
