// The events that the field looks for on each frame, in the grid maps, the rail maps, the maps that have both and the
// Union Room: reserved and trigger scripts, warps, what counts the player's steps (eggs, friendship, repels, the
// Hidden Grottoes), wild encounters, what the A button talks to, surfing ashore and the menus. The ROM has no string
// for the file; the name is descriptive, after fieldmap.c, which comes before it. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the works' layouts, their field names and the constants are
// our own. The code was first written by mariano54 on their parallel-decomp branch.
#include "types.h"
#include "constants/abilities.h"
#include "constants/directions.h"
#include "constants/flags.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "field/c_gear_sw.h"
#include "field/castelia_rush.h"
#include "field/encounter.h"
#include "field/entree_forest.h"
#include "field/event_abyssal_ruins.h"
#include "field/event_data.h"
#include "field/event_dive.h"
#include "field/event_fest_mission.h"
#include "field/event_mapchange.h"
#include "field/event_trainer_eye.h"
#include "field/event_wild_battle.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_acmd.h"
#include "field/field_actor.h"
#include "field/field_daycare.h"
#include "field/field_effect.h"
#include "field/field_event.h"
#include "field/field_gimmick_d06.h"
#include "field/field_map.h"
#include "field/field_menu.h"
#include "field/field_pass_power.h"
#include "field/field_player.h"
#include "field/field_rail.h"
#include "field/field_script.h"
#include "field/field_script_event.h"
#include "field/field_slope_slip.h"
#include "field/field_sound.h"
#include "field/field_status.h"
#include "field/field_water_current.h"
#include "field/gimmick_roller_coaster.h"
#include "field/gym_driftveil.h"
#include "field/gym_gimmick.h"
#include "field/gym_humilau.h"
#include "field/gym_mistralton.h"
#include "field/height_ex.h"
#include "field/hidden_hollow_generate.h"
#include "field/pair_sys.h"
#include "field/player_state.h"
#include "field/pleasure_boat.h"
#include "field/shortcut_menu.h"
#include "field/subscreen.h"
#include "field/union_actor.h"
#include "field/union_main.h"
#include "field/union_oneself.h"
#include "field/zone.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "nitro/mi.h"
#include "pml/poke_friendship.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/encounter.h"
#include "save/event_work.h"
#include "save/high_link.h"
#include "save/misc_savedata.h"
#include "save/player_savedata.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/fest_mission.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/wipe.h"

// The scripts that these events call
#define SCRID_NONE 0xffff
#define SCRID_EGG_HATCH 0x8ca
#define SCRID_SURF 0x2712
#define SCRID_WATERFALL 0x2716
#define SCRID_DIVE 0x271b
#define SCRID_WATERFALL_CLIMB 0x271f
// A repel has run out, with another of the same in the bag, and without one
#define SCRID_REPEL_ANOTHER 0x27a0
#define SCRID_REPEL_WORN_OFF 0x2792
// What the player reads with A in front of a PC, a TV, the bookcases, the trash cans, the shelves of odds and ends
// and a vending machine
#define SCRID_PC 0x276a
#define SCRID_TV 0x2922
#define SCRID_BOOKCASE_A 0x9c4
#define SCRID_BOOKCASE_B 0x9c5
#define SCRID_BOOKCASE_C 0x9c6
#define SCRID_BOOKCASE_D 0x9c7
#define SCRID_TRASH_CAN 0x9c8
#define SCRID_ASSORTMENT_A 0x9c9
#define SCRID_ASSORTMENT_B 0x9ca
#define SCRID_ASSORTMENT_C 0x9cb
#define SCRID_VENDING_MACHINE 0x28a0

// A grid tile's side, in world units and in fx32
#define GRID_UNITS 16
#define GRID_SIZE FX32_CONST(GRID_UNITS)

// The steps that the step counter of the game data counts to
#define STEP_TOTAL_MAX 9999
// The friendship of the party grows every so many steps
#define HAPPINESS_STEPS 128

// Each step adds to the eggs' progress (more with the Hatching pass power), and past a cycle's worth the eggs are a
// cycle closer to hatching
#define EGG_STEP_SUBSTEPS 0x100
#define EGG_CYCLE_SUBSTEPS 0x10000

#define HIDDEN_HOLLOW_COUNT 20
// Every so many steps, each empty Hidden Grotto may fill, with this chance in percent (before the pass power)
#define HIDDEN_HOLLOW_STEPS 256
#define HIDDEN_HOLLOW_CHANCE 5

// What the field's checks read of the player and the keys on this frame
typedef struct FieldInteractWk {
    HeapID heapId;
    GameSystem *gsys;
    GameData *gameData;
    EventData *eventData;
    u16 zoneId;
    Field *field;
    FieldPlayer *player;
    u32 observeStatus;
    u32 moveStatus;
    u16 faceDir;
    u16 reserveScript;
    u32 pressed;
    u32 held;
    u16 moveDir;
    VecFx32 *pos;
    u32 tileType;
    u32 aPressed : 1;
    u32 xPressed : 1;
    u32 stepped : 1;
    u32 stepStarted : 1;
    u32 walking : 1;
    u32 turning : 1;
    u32 dirHeld : 1;
    u32 yPressed : 1;
    u32 idle : 1;
    u32 unused9 : 1;
    u32 verticalOnly : 1;
    u32 canAct : 1;
} FieldInteractWk;

// The tiles that call a script when the player faces them and presses A, in a direction or in any
// (PLAYER_DIR_NONE)
typedef struct {
    BOOL (*check)(u32 tileClass);
    u16 dir;
    u16 scrId;
} SpecialTileScript;

// The work of the event that jumps the player off the water
typedef struct {
    u32 unused;
    u16 dir;
    u16 overEdge;
    GameSystem *gsys;
    Field *field;
} SurfExitWork;

static GameEvent *FieldEventProvider_Grid_CheckForEvent(GameSystem *gsys, Field *field, BOOL *scriptCalled,
                                                        BOOL *menuOpened);
static GameEvent *FieldEventProvider_NoGrid_CheckForEvent(GameSystem *gsys, Field *field);
static void FieldEventProvider_InitCheckData(FieldInteractWk *wk, GameSystem *gsys, Field *field);
static void StopXtransceiverRinging(GameSystem *gsys, Field *field);
static void GameData_UpdateGlobalStepCounter(GameData *gameData);
static void GameData_UpdateUserStepCounter(GameData *gameData);
static GameEvent *CheckReserveEventScriptCall(FieldInteractWk *wk);
static GameEvent *CheckNoneDirTriggerScrEvent(FieldInteractWk *wk);
static GameEvent *CheckTriggerScrEvent(FieldInteractWk *wk, u16 dir);
static GameEvent *CheckMoveDirChangeTriggerScrEvent(FieldInteractWk *wk);
static GameEvent *CheckDirChangeTriggerScrEvent(FieldInteractWk *wk);
static GameEvent *CheckQuicksandEvent(FieldInteractWk *wk);
static GameEvent *CheckPerStepEvents(FieldInteractWk *wk, Field *field, RailPosition *railPos);
static u32 PokeParty_UtilGetEggHatchSpeed(PokeParty *party);
static void PokeParty_UtilUpdateHatchSteps(PokeParty *party);
static void GameData_UpdateEggCycles(GameData *gameData, PokeParty *party);
static BOOL isEggReadyToHatch(PokeParty *party);
static GameEvent *CheckEggHatchEvent(FieldInteractWk *wk, Field *field, GameSystem *gsys, GameData *gameData);
static void UpdatePerStepHappiness(GameData *gameData, Field *field);
static GameEvent *UpdateAbyssalRuinsSteps(GameSystem *gsys, Field *field, u16 zoneId);
static GameEvent *CheckRepelEvent(Field *field, GameSystem *gsys, GameData *gameData);
static void hollowSet(GameData *gameData);
static GameEvent *CheckPhenomenonEvent(Field *field, GameSystem *gsys, GameData *gameData);
static GameEvent *CheckPassPowerDepletedEvent(GameSystem *gsys);
static GameEvent *CheckFunfestMissionEvent(GameSystem *gsys, Field *field);
static GameEvent *CheckWarpMapChangeEvent(FieldInteractWk *wk, Field *field, VecFx32 *pos);
static GameEvent *CheckExternalPushMoveEvents(FieldInteractWk *wk, Field *field);
static GameEvent *CreateWarpMapChangeEvent(FieldInteractWk *wk, Field *field, u32 warpId, VecFx32 *pos);
static void RememberOutboundWarp(FieldInteractWk *wk, u32 warpId, VecFx32 *pos);
static u32 GetWarpForWalkIn(FieldInteractWk *wk, VecFx32 *pos);
static void FieldInteractWk_CalcPlayerFrontPos(FieldInteractWk *wk, VecFx32 *out);
static BOOL IsStepOnWarp(ZoneWarp *warp);
static BOOL CheckWarpDirectionMatch2(const ZoneWarp *warp, u16 dir);
static GameEvent *CheckWallWarpEvent(FieldInteractWk *wk, GameSystem *gsys, Field *field);
static GameEvent *CheckAutoTileEvents(FieldInteractWk *wk, GameSystem *gsys, Field *field);
static GameEvent *CheckRailWarpMapChangeEvent(FieldInteractWk *wk, GameSystem *gsys, Field *field);
static GameEvent *CheckRailWallWarpEvent(FieldInteractWk *wk, GameSystem *gsys, Field *field, RailPosition *front);
static GameEvent *CheckCallRandomEncounter(FieldInteractWk *wk, GameSystem *gsys, Field *field);
static GameEvent *CheckRailDiveEvent(FieldInteractWk *wk, GameSystem *gsys, Field *field);
static void RememberOutboundWarpRail(FieldInteractWk *wk, u32 warpId, RailPosition *pos);
static GameEvent *CreateWarpMapChangeEventRail(FieldInteractWk *wk, Field *field, u32 warpId, RailPosition *pos);
static BOOL CanMoveOnRail(FieldPlayer *player, FieldInteractWk *wk);
static GameEventReturnCode EventSurfExit_Callback(GameEvent *event, u32 *state, void *data);
static GameEvent *EventSurfExit_Create(FieldInteractWk *wk, GameSystem *gsys, Field *field, u16 dir, BOOL overEdge);
static GameEvent *CheckSurfExitEvent(FieldInteractWk *wk, GameSystem *gsys, Field *field);
static FieldActor *FindActorForInteraction(FieldInteractWk *wk, Field *field);
static FieldActor *FindActorForInteractionRail(FieldInteractWk *wk, Field *field);
static u16 CheckFieldEnvInteraction(FieldInteractWk *wk, Field *field);
static u32 GameData_FindPartyPkmByMove(GameData *gameData, u16 move);
static BOOL IsSurfExitPathClear(FieldInteractWk *wk, Field *field, int steps);

static const SpecialTileScript SPECIAL_TILE_SCRIPTS[] = {
    { MapTile_IsPC, DIR_UP, SCRID_PC },
    { MapTile_IsUnused0xD7, PLAYER_DIR_NONE, SCRID_NONE },
    { MapTile_IsTV, DIR_UP, SCRID_TV },
    { MapTile_IsBookcaseA, PLAYER_DIR_NONE, SCRID_BOOKCASE_A },
    { MapTile_IsBookcaseB, PLAYER_DIR_NONE, SCRID_BOOKCASE_B },
    { MapTile_IsBookcaseC, PLAYER_DIR_NONE, SCRID_BOOKCASE_C },
    { MapTile_IsBookcaseD, PLAYER_DIR_NONE, SCRID_BOOKCASE_D },
    { MapTile_IsTrashCanA, PLAYER_DIR_NONE, SCRID_TRASH_CAN },
    { MapTile_IsTrashCanB, PLAYER_DIR_NONE, SCRID_TRASH_CAN },
    { MapTile_IsAssortmentA, PLAYER_DIR_NONE, SCRID_ASSORTMENT_A },
    { MapTile_IsAssortmentB, PLAYER_DIR_NONE, SCRID_ASSORTMENT_B },
    { MapTile_IsAssortmentC, PLAYER_DIR_NONE, SCRID_ASSORTMENT_C },
    { MapTile_IsVendingMachine, DIR_UP, SCRID_VENDING_MACHINE },
};

static GameEvent *FieldEventProvider_Grid_CheckForEvent(GameSystem *gsys, Field *field, BOOL *scriptCalled,
                                                        BOOL *menuOpened) {
    FieldInteractWk wk;
    GameEvent *event;

    *scriptCalled = FALSE;
    *menuOpened = FALSE;
    FieldEventProvider_InitCheckData(&wk, gsys, field);
    event = CheckReserveEventScriptCall(&wk);
    if (event != NULL) {
        *scriptCalled = TRUE;
        return event;
    }
    event = PleasureBoat_Update(gsys, *GameData_GetPleasureBoatPtr(wk.gameData));
    if (event != NULL) {
        return event;
    }
    if (Field_CheckGimmickID(field, GIMMICK_MISTRALTON_GYM)) {
        event = GymMistralton_CheckStep(gsys);
        if (event != NULL) {
            return event;
        }
    }
    event = EventTrainerEye_CheckAll(field);
    if (event != NULL) {
        *scriptCalled = TRUE;
        return event;
    }
    if (wk.walking) {
        EncSys_Update(Field_GetEncountSystem(field), wk.stepped, wk.verticalOnly);
    }
    if (wk.stepped) {
        GameData_UpdateGlobalStepCounter(wk.gameData);
    }
    if (wk.stepped) {
        GameData_UpdateUserStepCounter(wk.gameData);
    }
    if (wk.stepped) {
        event = CheckNoneDirTriggerScrEvent(&wk);
        if (event != NULL) {
            *scriptCalled = TRUE;
            return event;
        }
        event = CheckWarpMapChangeEvent(&wk, field, wk.pos);
        if (event != NULL) {
            return event;
        }
        event = CheckExternalPushMoveEvents(&wk, field);
        if (event != NULL) {
            return event;
        }
    }
    event = CheckPerStepEvents(&wk, field, NULL);
    if (event != NULL) {
        return event;
    }
    if (wk.walking) {
        event = CheckMoveDirChangeTriggerScrEvent(&wk);
        if (event != NULL) {
            return event;
        }
    }
    event = CheckDirChangeTriggerScrEvent(&wk);
    if (event != NULL) {
        return event;
    }
    event = CheckCallRandomEncounter(&wk, gsys, field);
    if (event != NULL) {
        return event;
    }
    {
        u32 contactFlags = 0;

        if (EventWork_FlagGet(GameData_GetEventWork(wk.gameData), EVENT_FLAG_0x0964) == TRUE) {
            contactFlags |= OBJ_CONTACT_STRENGTH;
        }
        event = FieldPlayer_CheckObjContactEvent(wk.player, wk.pressed, wk.held, contactFlags);
        if (event != NULL) {
            return event;
        }
    }
    if (wk.aPressed) {
        u16 faceDir = FieldPlayer_GetFaceDir(wk.player);
        VecFx32 pos;
        FieldActor *actor;
        u16 diveZoneId;
        u16 scrId;

        FieldPlayer_GetWPos(wk.player, &pos);
        ExpandVecInGridDir(faceDir, &pos, GRID_SIZE);
        actor = FindActorForInteraction(&wk, field);
        if (actor != NULL) {
            u32 actorScrId = FldAct_GetSCRID(actor);

            if (FieldScript_CheckSCRID(actorScrId) == TRUE) {
                FieldActor *playerActor = FieldPlayer_GetActor(wk.player);

                FieldPlayer_ForceBrake(wk.player);
                *scriptCalled = TRUE;
                return CreatePlayerXActorScriptCall(gsys, field, actorScrId, playerActor, actor, wk.heapId);
            }
        }
        scrId = CheckProxyEntityEventGrid(wk.eventData, GameData_GetEventWork(wk.gameData), &pos, faceDir);
        if (scrId != SCRID_NONE) {
            return EventScriptCall_Create(gsys, scrId, NULL, wk.heapId);
        }
        scrId = CheckFieldEnvInteraction(&wk, field);
        if (scrId != SCRID_NONE) {
            return EventScriptCall_Create(gsys, scrId, NULL, wk.heapId);
        }
        if (GetAbyssalRuinsDiveZoneID(field, &diveZoneId) == TRUE) {
            return EventScriptCall_Create(gsys, SCRID_DIVE, NULL, wk.heapId);
        }
        if (IsZoneAbyssalRuinsInside(wk.zoneId)) {
            scrId = GetSCRIDOfCollidingTriggerAtLocation(wk.eventData, GameData_GetEventWork(wk.gameData), &pos);
            if (scrId != SCRID_NONE) {
                return EventScriptCall_Create(gsys, scrId, NULL, wk.heapId);
            }
        }
    }
    if (wk.observeStatus == PLAYER_OBSERVE_END || wk.observeStatus == PLAYER_OBSERVE_NONE) {
        event = CheckSurfExitEvent(&wk, gsys, field);
        if (event != NULL) {
            return event;
        }
    }
    if (wk.dirHeld) {
        event = CheckWallWarpEvent(&wk, gsys, field);
        if (event != NULL) {
            return event;
        }
        event = CheckAutoTileEvents(&wk, gsys, field);
        if (event != NULL) {
            return event;
        }
    }
    if (wk.yPressed && !IsZoneBlackTowerOrWhiteTreehollow(wk.zoneId) && GFL_WipeIsFinished()) {
        FieldSubscreen *subscreen = Field_GetSubscreen(field);

        if (IsExistAnyYShortcut(gsys) && FieldSubscreen_IsReady(subscreen)) {
            event = CallYButtonShortcutMenu(gsys, field, wk.heapId);
            if (event != NULL) {
                return event;
            }
        }
    }
    if (GSYS_IsNetErrorPending(gsys) == TRUE && GFL_WipeIsFinished() && wk.canAct) {
        return EventCommError_Create(gsys, field);
    }
    if (wk.xPressed && GFL_WipeIsFinished()) {
        if (FieldSubscreen_GetScreenID(Field_GetSubscreen(field)) == FLD_SUBSCREEN_ID_DOWSING) {
            return NULL;
        }
        *menuOpened = TRUE;
        return EventFieldMenu_Create(gsys, field, wk.heapId);
    }
    if (GFL_WipeIsFinished()) {
        event = FieldSubscreen_CheckGameEvents(Field_GetSubscreen(field), wk.idle);
        if (event != NULL) {
            return event;
        }
    }
    if (wk.idle && GFL_WipeIsFinished()) {
        event = CheckFunfestMissionEvent(gsys, field);
        if (event != NULL) {
            return event;
        }
        event = CheckPassPowerDepletedEvent(gsys);
        if (event != NULL) {
            return event;
        }
    }
    event = CheckEntralinkForestFirstWarpEvent(field, gsys, wk.player);
    if (event != NULL) {
        return event;
    }
    return NULL;
}

GameEvent *FieldEventProvider_Grid(GameSystem *gsys, void *data) {
    BOOL scriptCalled;
    BOOL menuOpened;
    Field *field = GSYS_GetField(gsys);
    EncountSystem *encounter = Field_GetEncountSystem(field);
    GameEvent *event;

    EncountSystem_CancelPhenomenonIfActorHit(encounter);
    event = FieldEventProvider_Grid_CheckForEvent(gsys, data, &scriptCalled, &menuOpened);
    if (event != NULL) {
        CasteliaRush *rush;

        if (scriptCalled) {
            EncountSystem_CancelPhenomenon(encounter);
        }
        PlaceName_Cancel(Field_GetPlaceName(field));
        FieldPlayer_InterruptCatwalkBalance(Field_GetPlayer(field), menuOpened);
        FieldPlayer_StopGridForcedMove(Field_GetPlayer(field));
        rush = Field_GetCasteliaRush(field);
        if (rush != NULL) {
            CasteliaRush_ClearBalloons(rush);
        }
        StopXtransceiverRinging(gsys, field);
    }
    return event;
}

GameEvent *FieldEventProvider_UnionRoom(GameSystem *gsys, void *data) {
    Field *field = data;
    FieldInteractWk wk;
    GameCommSys *commSys = GSYS_GetGameCommSystem(gsys);
    UnionSystem *unisys = GameCommSys_GetWork(commSys);
    GameEvent *event;

    FieldEventProvider_InitCheckData(&wk, gsys, field);
    if (unisys == NULL) {
        if (GFL_NetErrCheck()) {
            if (GFL_WipeIsFinished()) {
                return EventCommError_Create(gsys, field);
            }
            return NULL;
        }
        return EventMapChangeUnionRoomExit_Create(gsys, field);
    }
    if (UnionMain_GetAtCounter(unisys) == TRUE) {
        if (UnionMain_GetExiting(unisys) == 0) {
            GameCommSys_ExitReq(commSys);
        }
        return NULL;
    }
    if (wk.stepped) {
        event = CheckTriggerScrEvent(&wk, PLAYER_DIR_NONE);
        if (event != NULL) {
            return event;
        }
    }
    event = UnionOneself_CreateCommandEvent(gsys, field);
    if (event != NULL) {
        return event;
    }
    if (wk.aPressed) {
        event = UnionOneself_ReturnFalse(gsys, field);
        if (event != NULL) {
            return event;
        }
    }
    if (wk.aPressed && UnionMain_IsIdle(gsys) == TRUE) {
        FieldActor *actor = FindActorForInteraction(&wk, field);

        if (actor != NULL) {
            u32 scrId = FldAct_GetSCRID(actor);

            if (FieldScript_CheckSCRID(scrId) == TRUE) {
                FieldActor *playerActor = FieldPlayer_GetActor(wk.player);

                if (!UnionActor_IsId(GetActorUID(actor))) {
                    FieldPlayer_ForceBrake(wk.player);
                    return CreatePlayerXActorScriptCall(gsys, field, scrId, playerActor, actor, wk.heapId);
                }
            }
        }
    }
    if (GSYS_IsNetErrorPending(gsys) == TRUE && GFL_WipeIsFinished()) {
        return EventCommError_Create(gsys, field);
    }
    if (UnionMain_IsIdle(gsys) == FALSE) {
        return NULL;
    }
    if (wk.xPressed && GFL_WipeIsFinished()) {
        return EventFieldMenu_CreateUnionRoom(gsys, field, wk.heapId);
    }
    return NULL;
}

GameEvent *FieldEventProvider_NoGrid(GameSystem *gsys, void *data) {
    GameEvent *event = FieldEventProvider_NoGrid_CheckForEvent(gsys, data);

    if (event != NULL) {
        CasteliaRush *rush;
        Field *field = GSYS_GetField(gsys);

        PlaceName_Cancel(Field_GetPlaceName(field));
        rush = Field_GetCasteliaRush(field);
        if (rush != NULL) {
            CasteliaRush_ClearBalloons(rush);
        }
        StopXtransceiverRinging(gsys, field);
    }
    return event;
}

static GameEvent *FieldEventProvider_NoGrid_CheckForEvent(GameSystem *gsys, Field *field) {
    FieldInteractWk wk;
    RailPosition frontPos;
    RailPosition stepPos;
    RailPosition proxyPos;
    GameEvent *event;
    FieldActor *actor;

    FieldEventProvider_InitCheckData(&wk, gsys, field);
    actor = FieldPlayer_GetActor(wk.player);
    RailActor_GetFrontLocation(actor, &frontPos);
    event = CheckReserveEventScriptCall(&wk);
    if (event != NULL) {
        return event;
    }
    if (wk.stepped) {
        GameData_UpdateGlobalStepCounter(wk.gameData);
    }
    if (wk.stepped) {
        FieldActor *stepActor = FieldPlayer_GetActor(wk.player);
        EventWork *eventWork = GameData_GetEventWork(wk.gameData);
        u16 scrId;

        GetActorRailPos(stepActor, &stepPos);
        scrId = GetTriggerSCRIDAtPosRail(wk.eventData, eventWork, &stepPos);
        if (scrId != SCRID_NONE) {
            return EventScriptCall_Create(gsys, scrId, NULL, wk.heapId);
        }
        event = CheckRailWarpMapChangeEvent(&wk, gsys, field);
        if (event != NULL) {
            return event;
        }
        event = CheckPerStepEvents(&wk, field, &stepPos);
        if (event != NULL) {
            return event;
        }
    }
    event = CheckCallRandomEncounter(&wk, gsys, field);
    if (event != NULL) {
        return event;
    }
    if (wk.aPressed) {
        FieldActor *found = FindActorForInteractionRail(&wk, field);
        EventWork *eventWork;
        u16 faceDir;
        u16 scrId;

        if (found != NULL) {
            u32 foundScrId = FldAct_GetSCRID(found);

            if (FieldScript_CheckSCRID(foundScrId) == TRUE) {
                FieldActor *playerActor = FieldPlayer_GetActor(wk.player);

                FieldPlayer_ForceBrake(wk.player);
                return CreatePlayerXActorScriptCall(gsys, field, foundScrId, playerActor, found, wk.heapId);
            }
        }
        eventWork = GameData_GetEventWork(wk.gameData);
        faceDir = GetActorFaceDir(actor);
        proxyPos = frontPos;
        scrId = CheckProxyEntityEventRail(wk.eventData, eventWork, &proxyPos, faceDir);
        if (scrId != SCRID_NONE) {
            return EventScriptCall_Create(gsys, scrId, NULL, wk.heapId);
        }
    }
    if (wk.dirHeld) {
        if (!CanMoveOnRail(Field_GetPlayer(field), &wk)) {
            event = CheckRailWallWarpEvent(&wk, gsys, field, &frontPos);
            if (event != NULL) {
                return event;
            }
        }
    }
    event = CheckRailDiveEvent(&wk, gsys, field);
    if (event != NULL) {
        return event;
    }
    if (wk.yPressed && GFL_WipeIsFinished()) {
        FieldSubscreen *subscreen = Field_GetSubscreen(field);

        if (IsExistAnyYShortcut(gsys) && FieldSubscreen_IsReady(subscreen)) {
            event = CallYButtonShortcutMenu(gsys, field, wk.heapId);
            if (event != NULL) {
                return event;
            }
        }
    }
    if (GSYS_IsNetErrorPending(gsys) == TRUE && GFL_WipeIsFinished() && wk.canAct) {
        return EventCommError_Create(gsys, field);
    }
    if (wk.xPressed && GFL_WipeIsFinished()) {
        if (FieldSubscreen_GetScreenID(Field_GetSubscreen(field)) == FLD_SUBSCREEN_ID_DOWSING) {
            return NULL;
        }
        return EventFieldMenu_Create(gsys, field, wk.heapId);
    }
    event = FieldSubscreen_CheckGameEvents(Field_GetSubscreen(field), wk.idle && GFL_WipeIsFinished());
    if (event != NULL) {
        return event;
    }
    if (wk.idle && GFL_WipeIsFinished()) {
        event = CheckFunfestMissionEvent(gsys, field);
        if (event != NULL) {
            return event;
        }
        event = CheckPassPowerDepletedEvent(gsys);
        if (event != NULL) {
            return event;
        }
    }
    return NULL;
}

GameEvent *FieldEventProvider_Hybrid(GameSystem *gsys, void *data) {
    if (Field_GetResolvedControllerTypeID(data) == 0) {
        return FieldEventProvider_Grid(gsys, data);
    }
    return FieldEventProvider_NoGrid(gsys, data);
}

static void FieldEventProvider_InitCheckData(FieldInteractWk *wk, GameSystem *gsys, Field *field) {
    FieldActor *actor;

    MI_CpuFill8(wk, 0, sizeof(FieldInteractWk));
    wk->heapId = Field_GetHeapID(field);
    wk->gsys = gsys;
    wk->gameData = GSYS_GetGameData(gsys);
    wk->eventData = GameData_GetEventData(wk->gameData);
    wk->zoneId = Field_GetPlayerStateZoneID(field);
    wk->pressed = GCTX_HIDGetPressedKeys();
    wk->held = GCTX_HIDGetHeldKeys();
    wk->moveDir = FieldPlayer_GetMoveDirByKey(Field_GetPlayer(field), wk->held);
    wk->field = field;
    wk->player = Field_GetPlayer(field);
    FieldPlayer_UpdateActionStatusObserver(wk->player);
    wk->moveStatus = FieldPlayer_GetMoveStatus(wk->player);
    wk->observeStatus = FieldPlayer_GetMoveObserveStatus(wk->player);
    actor = FieldPlayer_GetActor(wk->player);
    wk->faceDir = GetActorFaceDir(actor);
    wk->pos = GetMModelWPosPtr(actor);
    wk->reserveScript = FieldStatus_GetReserveScript(GameData_GetFieldStatus(wk->gameData));
    wk->tileType = FieldPlayer_GetTileTypeUnder(wk->player);
    if (wk->observeStatus == PLAYER_OBSERVE_END || wk->observeStatus == PLAYER_OBSERVE_NONE) {
        wk->canAct = TRUE;
        wk->aPressed = (wk->pressed & PAD_BUTTON_A) != 0;
        wk->xPressed = (wk->pressed & PAD_BUTTON_X) != 0;
        wk->yPressed = (wk->pressed & PAD_BUTTON_Y) != 0;
        {
            u8 count = 0;

            count += wk->aPressed;
            count += wk->xPressed;
            count += wk->yPressed;
            if (count == 0) {
                wk->idle = TRUE;
            }
        }
    }
    wk->stepped = FALSE;
    wk->verticalOnly = FALSE;
    wk->stepStarted = FALSE;
    if (wk->moveStatus == PLAYER_MOVE_STATUS_MOVE) {
        wk->stepped = wk->observeStatus == PLAYER_OBSERVE_END;
        wk->stepStarted = wk->observeStatus == PLAYER_OBSERVE_START;
    }
    if (FieldPlayer_GetVerticalMoveOnlyFlag(wk->player)) {
        wk->verticalOnly = TRUE;
    }
    wk->walking = wk->observeStatus == PLAYER_OBSERVE_END;
    wk->turning = FALSE;
    if (wk->observeStatus != PLAYER_OBSERVE_END && wk->moveStatus == PLAYER_MOVE_STATUS_TURN) {
        wk->turning = TRUE;
    }
    if ((wk->faceDir == DIR_UP && (wk->held & PAD_KEY_UP)) || (wk->faceDir == DIR_DOWN && (wk->held & PAD_KEY_DOWN)) ||
        (wk->faceDir == DIR_LEFT && (wk->held & PAD_KEY_LEFT)) ||
        (wk->faceDir == DIR_RIGHT && (wk->held & PAD_KEY_RIGHT))) {
        wk->dirHeld = TRUE;
    } else {
        wk->dirHeld = FALSE;
    }
}

// Stops the Xtransceiver's ringtone, and its ringing on the C-Gear unless a call is starting
static void StopXtransceiverRinging(GameSystem *gsys, Field *field) {
    CGearSw *cgear = FieldSubscreen_GetCGear(Field_GetSubscreen(field));

    if (cgear == NULL) {
        FieldSnd_StopRingtone(GameData_GetFieldSoundSystem(GSYS_GetGameData(gsys)));
    } else if (!CGearSw_IsStartingTvt(cgear)) {
        FieldSnd_StopRingtone(GameData_GetFieldSoundSystem(GSYS_GetGameData(gsys)));
        CGearSw_StopRinging(cgear);
    }
}

static void GameData_UpdateGlobalStepCounter(GameData *gameData) {
    int steps;

    RecordAddOne(GameData_GetRecords(gameData), 0);
    steps = getCurrentStepTotal(gameData) + 1;
    if (steps > STEP_TOTAL_MAX) {
        steps = 0;
    }
    setCurrentStepTotal(gameData, steps);
}

static void GameData_UpdateUserStepCounter(GameData *gameData) {
    PlayerSave *save = SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData));

    if (PlayerSave_HasStepCounter(save)) {
        PlayerSave_IncrementStepCounter(save);
    }
}

static GameEvent *CheckReserveEventScriptCall(FieldInteractWk *wk) {
    GameEvent *event;

    if (wk->reserveScript != 0) {
        FieldStatus_ReserveScript(GameData_GetFieldStatus(wk->gameData), 0);
        return EventScriptCall_Create(wk->gsys, wk->reserveScript, NULL, wk->heapId);
    }
    event = FieldScript_CheckSceneChangeEvent(wk->gsys, wk->heapId);
    if (event != NULL) {
        return event;
    }
    return NULL;
}

static GameEvent *CheckNoneDirTriggerScrEvent(FieldInteractWk *wk) {
    GameEvent *event = CheckQuicksandEvent(wk);

    if (event == NULL) {
        event = CheckTriggerScrEvent(wk, PLAYER_DIR_NONE);
    }
    return event;
}

static GameEvent *CheckTriggerScrEvent(FieldInteractWk *wk, u16 dir) {
    VecFx32 pos;
    Field *field = GSYS_GetField(wk->gsys);
    EventWork *eventWork = GameData_GetEventWork(wk->gameData);
    u16 scrId;

    FieldPlayer_GetWPos(wk->player, &pos);
    if (MapTile_IsQuicksand(GetTileClass(GetTileTypeAtPos(Field_GetG3DMapper(field), &pos))) == TRUE) {
        return NULL;
    }
    scrId = GetTriggerSCRIDAtPosGrid(wk->eventData, eventWork, &pos, dir);
    if (scrId != SCRID_NONE) {
        return EventScriptCall_Create(wk->gsys, scrId, NULL, wk->heapId);
    }
    return NULL;
}

static GameEvent *CheckMoveDirChangeTriggerScrEvent(FieldInteractWk *wk) {
    GameEvent *event = CheckTriggerScrEvent(wk, wk->faceDir);
    u16 dir;

    if (event != NULL) {
        return event;
    }
    dir = FieldPlayer_GetMoveDirByKey(wk->player, wk->held);
    if (dir == PLAYER_DIR_NONE || dir == wk->faceDir) {
        return NULL;
    }
    event = CheckTriggerScrEvent(wk, dir);
    if (event != NULL) {
        return event;
    }
    return NULL;
}

static GameEvent *CheckDirChangeTriggerScrEvent(FieldInteractWk *wk) {
    u32 observeStatus = wk->observeStatus;
    u16 dir = PLAYER_DIR_NONE;

    if (wk->held & PAD_KEY_UP) {
        dir = DIR_UP;
    } else if (wk->held & PAD_KEY_DOWN) {
        dir = DIR_DOWN;
    } else if (wk->held & PAD_KEY_LEFT) {
        dir = DIR_LEFT;
    } else if (wk->held & PAD_KEY_RIGHT) {
        dir = DIR_RIGHT;
    }
    if (dir == wk->faceDir) {
        return NULL;
    }
    if (dir == PLAYER_DIR_NONE) {
        return NULL;
    }
    return (observeStatus == PLAYER_OBSERVE_START || observeStatus == PLAYER_OBSERVE_MOVING)
               ? NULL
               : CheckTriggerScrEvent(wk, dir);
}

static GameEvent *CheckQuicksandEvent(FieldInteractWk *wk) {
    VecFx32 pos;
    Field *field = GSYS_GetField(wk->gsys);
    FieldPlayer *player = Field_GetPlayer(field);
    ZoneTrigger *trigger;
    u32 exState;
    u16 acmd;
    u32 centerX;
    u32 centerZ;
    ZoneTriggerGridPosition *grid;

    FieldPlayer_GetWPos(player, &pos);
    if (MapTile_IsQuicksand(GetTileClass(GetTileTypeAtPos(Field_GetG3DMapper(field), &pos))) != TRUE) {
        return NULL;
    }
    trigger = FindQuicksandTrigger(wk->eventData, GameData_GetEventWork(wk->gameData), &pos, PLAYER_DIR_NONE);
    if (trigger == NULL) {
        return NULL;
    }
    if (trigger->isRail) {
        return NULL;
    }
    grid = &trigger->pos.grid;
    centerZ = grid->z + grid->height / 2;
    centerX = grid->x + grid->width / 2;
    if (centerX == (pos.x >> FX32_SHIFT) / GRID_UNITS && centerZ == (pos.z >> FX32_SHIFT) / GRID_UNITS) {
        return EventScriptCall_Create(wk->gsys, trigger->scrId, NULL, wk->heapId);
    }
    exState = FieldPlayer_GetExState(player);
    acmd = FldAct_GetAcmd(FieldPlayer_GetActor(player));
    if (exState != FLD_PLAYER_EXSTATE_NONE || acmd == ACMD_DASH + DIR_UP || acmd == ACMD_DASH + DIR_DOWN ||
        acmd == ACMD_DASH + DIR_LEFT || acmd == ACMD_DASH + DIR_RIGHT || acmd == ACMD_DASH_UPHILL + DIR_UP ||
        acmd == ACMD_DASH_UPHILL + DIR_DOWN || acmd == ACMD_DASH_UPHILL + DIR_LEFT ||
        acmd == ACMD_DASH_UPHILL + DIR_RIGHT) {
        return EventScriptCall_Create(wk->gsys, trigger->scrId, NULL, wk->heapId);
    }
    return NULL;
}

static GameEvent *CheckPerStepEvents(FieldInteractWk *wk, Field *field, RailPosition *railPos) {
    GameEvent *event;
    GameSystem *gsys;
    GameData *gameData;

    // The original reads the heap and drops it
    Field_GetHeapID(field);
    gsys = Field_GetGameSystem(field);
    gameData = GSYS_GetGameData(gsys);
    if (wk->stepStarted && GameData_CheckPairFlag(gameData)) {
        updateFollowerModel(gameData);
    }
    if (wk->stepped || wk->verticalOnly) {
        FesGimmick *gimmick;

        event = CheckPhenomenonEvent(field, gsys, gameData);
        if (event != NULL) {
            return event;
        }
        gimmick = Field_GetFesGimmick(field);
        if (railPos != NULL) {
            event = FesGimmick_CheckEventRail(gimmick, railPos);
        } else {
            event = FesGimmick_CheckEventGrid(gimmick, wk->pos);
        }
        if (event != NULL) {
            return event;
        }
    }
    if (!wk->stepped) {
        return NULL;
    }
    UpdatePerStepHappiness(wk->gameData, field);
    event = UpdateAbyssalRuinsSteps(gsys, field, wk->zoneId);
    if (event != NULL) {
        return event;
    }
    event = CheckEggHatchEvent(wk, field, gsys, gameData);
    if (event != NULL) {
        return event;
    }
    event = CheckRepelEvent(field, gsys, gameData);
    if (event != NULL) {
        return event;
    }
    hollowSet(gameData);
    return NULL;
}

// Flame Body and Magma Armor hatch eggs twice as fast
static u32 PokeParty_UtilGetEggHatchSpeed(PokeParty *party) {
    int count = PokeParty_GetPkmCount(party);
    int i;

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);
        int ability = PokeParty_GetParam(pkm, PKM_PARAM_ABILITY, NULL);

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == FALSE &&
            (ability == ABILITY_FLAME_BODY || ability == ABILITY_MAGMA_ARMOR)) {
            return 2;
        }
    }
    return 1;
}

// An egg's cycles to hatch are its friendship
static void PokeParty_UtilUpdateHatchSteps(PokeParty *party) {
    int count = PokeParty_GetPkmCount(party);
    u32 speed = PokeParty_UtilGetEggHatchSpeed(party);
    int i;

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);
        int isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
        u32 cycles = PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);

        if (isEgg == TRUE) {
            u32 remaining = 0;

            if (cycles >= speed) {
                remaining = cycles - speed;
            }
            PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, remaining);
        }
    }
}

static void GameData_UpdateEggCycles(GameData *gameData, PokeParty *party) {
    u32 substeps;
    PlayerSave *save = SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData));

    PlayerSave_GetEggCycleSubsteps(save, &substeps);
    substeps += PassPower_ApplyHatch(EGG_STEP_SUBSTEPS);
    if (substeps > EGG_CYCLE_SUBSTEPS) {
        PokeParty_UtilUpdateHatchSteps(party);
        substeps = 0;
    }
    PlayerSave_SetEggCycleSubsteps(save, substeps);
}

static BOOL isEggReadyToHatch(PokeParty *party) {
    int count = PokeParty_GetPkmCount(party);
    int i;

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);
        int isEgg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);
        int cycles = PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);

        if (isEgg == TRUE && cycles == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

static GameEvent *CheckEggHatchEvent(FieldInteractWk *wk, Field *field, GameSystem *gsys, GameData *gameData) {
    HeapID heapId = Field_GetHeapID(field);
    PokeParty *party = GameData_GetParty(gameData);

    DayCare_Update(Field_GetDayCare(field));
    GameData_UpdateEggCycles(gameData, party);
    if (isEggReadyToHatch(party)) {
        return EventScriptCall_Create(gsys, SCRID_EGG_HATCH, NULL, heapId);
    }
    return NULL;
}

static void UpdatePerStepHappiness(GameData *gameData, Field *field) {
    PlayerSave *save = SaveControl_GetPlayerSave(GameData_GetSaveControl(gameData));
    BOOL reset = FALSE;
    u16 steps = PlayerSave_GetHappinesStepCounter(save) + 1;

    if (steps >= HAPPINESS_STEPS) {
        steps = 0;
        reset = TRUE;
    }
    PlayerSave_SetHappinessStepCounter(save, steps);
    if (reset) {
        PokeParty *party = GameData_GetParty(gameData);
        u16 zoneId = Field_GetPlayerStateZoneID(field);
        HeapID heapId = Field_GetHeapID(field);
        int count = PokeParty_GetPkmCount(party);
        int i;

        for (i = 0; i < count; i++) {
            happinessAdd(PokeParty_GetPkm(party, i), zoneId, heapId);
        }
    }
}

static GameEvent *UpdateAbyssalRuinsSteps(GameSystem *gsys, Field *field, u16 zoneId) {
    PlayerSave *save = SaveControl_GetPlayerSave(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    GameEvent *event = NULL;

    if (IsZoneAbyssalRuinsInside(zoneId)) {
        event = CheckAbyssalRuinsStepEvent(gsys, field);
        PlayerSave_SetAbyssalRuinsStepCounter(save, PlayerSave_GetAbyssalRuinsStepCounter(save) + 1);
    }
    return event;
}

static GameEvent *CheckRepelEvent(Field *field, GameSystem *gsys, GameData *gameData) {
    EncountSave *save = SaveControl_GetEncountSave(GameData_GetSaveControl(gameData));
    u8 wornOff = EncountSave_DecrementRepelSteps(save);

    if (wornOff) {
        u16 item = EncountSave_GetUsedRepelItemID(save);
        BagSave *bag = GameData_GetBag(gameData);

        if (BagSave_GetItemCountByID(bag, item, Field_GetHeapID(field)) != 0) {
            return EventScriptCall_Create(gsys, SCRID_REPEL_ANOTHER, NULL, Field_GetHeapID(field));
        }
        return EventScriptCall_Create(gsys, SCRID_REPEL_WORN_OFF, NULL, Field_GetHeapID(field));
    }
    return NULL;
}

// Every HIDDEN_HOLLOW_STEPS steps, each empty Hidden Grotto may fill
static void hollowSet(GameData *gameData) {
    SaveControl *saveControl = GameData_GetSaveControl(gameData);
    int steps = getCurrentStepTotal(gameData);
    RivalDataSave *rivalData = getHollow_RivalData(saveControl);
    int i;

    if (steps % HIDDEN_HOLLOW_STEPS == 0) {
        for (i = 0; i < HIDDEN_HOLLOW_COUNT; i++) {
            if (!isHollowFilled(rivalData, i)) {
                u32 chance = PassPower_ApplyGrotto(HIDDEN_HOLLOW_CHANCE);

                if (GFL_RandomLC(100) < chance) {
                    generateHollow(rivalData, i);
                }
            }
        }
    }
}

static GameEvent *CheckPhenomenonEvent(Field *field, GameSystem *gsys, GameData *gameData) {
    EncountSystem *system;
    GameEvent *event;

    // The original reads the save and drops it
    GameData_GetSaveControl(gameData);
    system = Field_GetEncountSystem(field);
    event = CreateRandomPhenomenonEvent(system);
    if (event != NULL) {
        return event;
    }
    UpdatePhenomenon(system);
    return NULL;
}

// The script that says a pass power has run out
static GameEvent *CheckPassPowerDepletedEvent(GameSystem *gsys) {
    if (PassPower_GetDepletedID() == PASS_POWER_NONE) {
        return NULL;
    }
    return GameEvent_CreateOverlayDelegate(gsys, OVERLAY_FIELD_PASS_POWER, EventPassPowerDepleted_Create, NULL);
}

// The Funfest mission's events: its start, and its end once it is cleared or out of time
static GameEvent *CheckFunfestMissionEvent(GameSystem *gsys, Field *field) {
    LinkFestival *festival = GSYS_GetLinkFestival(gsys);

    if (!isFesMissionAvailable(GetFestMissionCfg(festival))) {
        return NULL;
    }
    if (GSYS_IsCGearOn(gsys) == 0) {
        return GameEvent_CreateOverlayDelegate(gsys, OVERLAY_FUNFEST_MISSION, EventFestMissionDisconnect_Create, field);
    }
    if (LinkFestival_IsMissionOver(festival)) {
        return GameEvent_CreateOverlayDelegate(gsys, OVERLAY_FUNFEST_MISSION, EventFestMissionMenu_Create, field);
    }
    return NULL;
}

static GameEvent *CheckWarpMapChangeEvent(FieldInteractWk *wk, Field *field, VecFx32 *pos) {
    ZoneWarp *warp;
    u32 warpId = GetWarpForWalkIn(wk, pos);

    if (warpId == WARP_ID_NONE) {
        return NULL;
    }
    warp = GetZoneWarpByID(wk->eventData, warpId);
    if (IsWarpZoneOrWarpID0xFFFF(warp) == TRUE || !IsStepOnWarp(warp)) {
        return NULL;
    }
    RememberOutboundWarp(wk, warpId, pos);
    return CreateWarpMapChangeEvent(wk, field, warpId, pos);
}

static GameEvent *CheckExternalPushMoveEvents(FieldInteractWk *wk, Field *field) {
    VecFx32 pos;
    u16 tileClass;

    FieldPlayer_GetWPos(wk->player, &pos);
    tileClass = GetTileClass(GetTileTypeAtPos(Field_GetG3DMapper(field), &pos));
    if (IsTileSlopeSlip(tileClass)) {
        return EventSlopeSlip_Create(NULL, wk->gsys, field);
    }
    if (IsTileWaterCurrent(tileClass)) {
        return EventWaterCurrentMove_Create(NULL, wk->gsys, field);
    }
    return NULL;
}

static GameEvent *CreateWarpMapChangeEvent(FieldInteractWk *wk, Field *field, u32 warpId, VecFx32 *pos) {
    ZoneWarp *warp = GetZoneWarpByID(wk->eventData, warpId);
    u16 posWeightBits;

    if (pos != NULL) {
        posWeightBits = ZoneWarp_CalcPosWeightBitsGrid(warp, pos);
    } else {
        posWeightBits = 0;
    }
    return EventMapChangeWarp_CreateFromEntity(wk->gsys, field, warp, posWeightBits);
}

// Remembers the warp that the player leaves by
static void RememberOutboundWarp(FieldInteractWk *wk, u32 warpId, VecFx32 *pos) {
    ZoneSpawnInfo spawn;

    SetupZoneWarpArrivalGrid(&spawn, Field_GetPlayerStateZoneID(wk->field), warpId, 0, 0, pos->x, pos->y, pos->z);
    SetGameDataOutboundWarpRememberInfo(wk->gameData, &spawn);
}

static u32 GetWarpForWalkIn(FieldInteractWk *wk, VecFx32 *pos) {
    u32 warpId = GetWarpAtPosition(wk->eventData, pos);
    ZoneWarp *warp;

    if (warpId == WARP_ID_NONE) {
        return WARP_ID_NONE;
    }
    warp = GetZoneWarpByID(wk->eventData, warpId);
    if (!IsStepOnWarp(warp) && !CheckWarpDirectionMatch2(warp, wk->faceDir)) {
        return WARP_ID_NONE;
    }
    return warpId;
}

static void FieldInteractWk_CalcPlayerFrontPos(FieldInteractWk *wk, VecFx32 *out) {
    *out = *wk->pos;
    switch (wk->faceDir) {
    case DIR_UP:
        out->z -= GRID_SIZE;
        break;
    case DIR_DOWN:
        out->z += GRID_SIZE;
        break;
    case DIR_LEFT:
        out->x -= GRID_SIZE;
        break;
    case DIR_RIGHT:
        out->x += GRID_SIZE;
        break;
    }
}

// Whether the warp is taken by stepping on it, whatever the direction
static BOOL IsStepOnWarp(ZoneWarp *warp) {
    u32 type = GetWarpTransitionType(warp);

    return type == WARP_TRANSITION_NONE || type == WARP_TRANSITION_WARP_PAD || type == WARP_TRANSITION_GATE;
}

static BOOL CheckWarpDirectionMatch2(const ZoneWarp *warp, u16 dir) {
    return CheckWarpDirectionMatch(warp, dir);
}

// The warp of a wall that the player walks into: the mat under the player, or the warp in front
static GameEvent *CheckWallWarpEvent(FieldInteractWk *wk, GameSystem *gsys, Field *field) {
    VecFx32 front;
    u32 warpId;
    ZoneWarp *warp;

    FieldInteractWk_CalcPlayerFrontPos(wk, &front);
    if (!(GetTileFlags(GetTileTypeAtPos(Field_GetG3DMapper(field), &front)) & TILE_FLAG_COLLISION)) {
        return NULL;
    }
    warpId = GetWarpForWalkIn(wk, wk->pos);
    if (warpId != WARP_ID_NONE) {
        warp = GetZoneWarpByID(wk->eventData, warpId);
        if (!IsWarpZoneOrWarpID0xFFFF(warp) && GetWarpTransitionType(warp) == WARP_TRANSITION_MAT) {
            RememberOutboundWarp(wk, warpId, wk->pos);
            return CreateWarpMapChangeEvent(wk, field, warpId, wk->pos);
        }
    }
    warpId = GetWarpForWalkIn(wk, &front);
    if (warpId != WARP_ID_NONE) {
        if (!IsWarpZoneOrWarpID0xFFFF(GetZoneWarpByID(wk->eventData, warpId))) {
            RememberOutboundWarp(wk, warpId, wk->pos);
            return CreateWarpMapChangeEvent(wk, field, warpId, &front);
        }
    }
    return NULL;
}

static GameEvent *CheckAutoTileEvents(FieldInteractWk *wk, GameSystem *gsys, Field *field) {
    u32 tileType;
    fx32 height;
    VecFx32 front;
    VecFx32 above;

    FieldInteractWk_CalcPlayerFrontPos(wk, &front);
    if (Field_CheckGimmickID(field, GIMMICK_DRIFTVEIL_GYM)) {
        return DriftveilGym_CheckPadEvent(gsys);
    }
    if (Field_CheckGimmickID(field, GIMMICK_CHARGESTONE_CAVE)) {
        return GimmickD06_CheckPush(field, wk->faceDir);
    }
    tileType = GetTileTypeAtPos(Field_GetG3DMapper(field), &front);
    if (!(GetTileFlags(tileType) & TILE_FLAG_COLLISION)) {
        return NULL;
    }
    if (FieldPlayer_GetExState(wk->player) == FLD_PLAYER_EXSTATE_SURF &&
        GameData_FindPartyPkmByMove(wk->gameData, MOVE_WATERFALL) != PARTY_SIZE &&
        IsTileWaterfall(GetTileClass(tileType)) == TRUE) {
        FieldActor *actor;

        above = front;
        actor = FieldPlayer_GetActor(wk->player);
        ExpandVecInGridDir(wk->faceDir, &above, GRID_SIZE);
        if (GetHeightFromMap(actor, &above, &height) == TRUE && front.y > height) {
            return EventScriptCall_Create(wk->gsys, SCRID_WATERFALL_CLIMB, NULL, wk->heapId);
        }
    }
    if (GetWarpForWalkIn(wk, &front) == WARP_ID_NONE) {
        if (Field_CheckGimmickID(field, GIMMICK_CASTELIA_GYM)) {
            return GymInsect_CheckRide(gsys, wk->faceDir);
        }
        if (Field_CheckGimmickID(field, GIMMICK_HUMILAU_GYM)) {
            return GymHumilau_CheckRide(gsys);
        }
        if (Field_CheckGimmickID(field, GIMMICK_ROLLER_COASTER)) {
            return RollerCoaster_CheckRide(gsys);
        }
    }
    return NULL;
}

// The warp under the player on a rail
static GameEvent *CheckRailWarpMapChangeEvent(FieldInteractWk *wk, GameSystem *gsys, Field *field) {
    RailPosition pos;
    u32 warpId;
    ZoneWarp *warp;

    GetActorRailPos(FieldPlayer_GetActor(Field_GetPlayer(field)), &pos);
    warpId = GetWarpIDByPlayerPosRail(wk->eventData, &pos);
    if (warpId == WARP_ID_NONE) {
        return NULL;
    }
    warp = GetZoneWarpByID(wk->eventData, warpId);
    if (IsWarpZoneOrWarpID0xFFFF(warp) == TRUE || !IsStepOnWarp(warp)) {
        return NULL;
    }
    RememberOutboundWarpRail(wk, warpId, &pos);
    return CreateWarpMapChangeEventRail(wk, field, warpId, &pos);
}

// The warp that the player walks into on a rail: the mat under the player, or the warp in front
static GameEvent *CheckRailWallWarpEvent(FieldInteractWk *wk, GameSystem *gsys, Field *field, RailPosition *front) {
    RailPosition pos;
    u32 warpId;
    ZoneWarp *warp;

    GetActorRailPos(FieldPlayer_GetActor(Field_GetPlayer(field)), &pos);
    warpId = GetWarpIDByPlayerPosRail(wk->eventData, &pos);
    if (warpId != WARP_ID_NONE) {
        warp = GetZoneWarpByID(wk->eventData, warpId);
        if (!IsWarpZoneOrWarpID0xFFFF(warp) && CheckWarpDirectionMatch2(warp, wk->faceDir) &&
            GetWarpTransitionType(warp) == WARP_TRANSITION_MAT) {
            RememberOutboundWarpRail(wk, warpId, &pos);
            return CreateWarpMapChangeEventRail(wk, field, warpId, &pos);
        }
    }
    warpId = GetWarpIDByPlayerPosRail(wk->eventData, front);
    if (warpId == WARP_ID_NONE) {
        return NULL;
    }
    warp = GetZoneWarpByID(wk->eventData, warpId);
    if (!IsWarpZoneOrWarpID0xFFFF(warp) && CheckWarpDirectionMatch2(warp, wk->faceDir)) {
        // Unlike the grid's twin, which enters the warp in front at the front position, this enters it at the
        // player's own
        RememberOutboundWarpRail(wk, warpId, &pos);
        return CreateWarpMapChangeEventRail(wk, field, warpId, &pos);
    }
    return NULL;
}

static GameEvent *CheckCallRandomEncounter(FieldInteractWk *wk, GameSystem *gsys, Field *field) {
    EncountSystem *encounter = Field_GetEncountSystem(field);

    if (wk->walking) {
        return EventWildBattleCall_CreateRandom(encounter, ENCOUNT_MODE_NORMAL);
    }
    return NULL;
}

// The dive that the player starts by walking down off a rail's edge into deep water
static GameEvent *CheckRailDiveEvent(FieldInteractWk *wk, GameSystem *gsys, Field *field) {
    FieldPlayer *player = Field_GetPlayer(field);
    FieldActor *actor = FieldPlayer_GetActor(player);
    u16 dir = FieldPlayer_GetMoveDirByKey(player, wk->held);

    if (dir == DIR_DOWN && (wk->observeStatus == PLAYER_OBSERVE_NONE || wk->observeStatus == PLAYER_OBSERVE_END)) {
        if (FieldRailTilemap_IsSignBoard(GetTileClass(wk->tileType)) &&
            (RailActor_GetHitBitsDirNoAttr(actor, dir) & 3) != 0) {
            return func_ov033_0217a0f4(gsys, field, dir);
        }
    }
    return NULL;
}

static void RememberOutboundWarpRail(FieldInteractWk *wk, u32 warpId, RailPosition *pos) {
    ZoneSpawnInfo spawn;

    SetupZoneWarpArrivalRail(&spawn, Field_GetPlayerStateZoneID(wk->field), warpId, 0, 0, pos->componentId,
                             pos->posFront, pos->posSide);
    SetGameDataOutboundWarpRememberInfo(wk->gameData, &spawn);
}

static GameEvent *CreateWarpMapChangeEventRail(FieldInteractWk *wk, Field *field, u32 warpId, RailPosition *pos) {
    ZoneWarp *warp = GetZoneWarpByID(wk->eventData, warpId);

    return EventMapChangeWarp_CreateFromEntity(wk->gsys, field, warp, ZoneWarp_CalcPosWeightBitsRail(warp, pos));
}

// Whether the rail lets the player move on in the direction faced
static BOOL CanMoveOnRail(FieldPlayer *player, FieldInteractWk *wk) {
    if ((RailActor_GetHitBitsDirNoAttr(FieldPlayer_GetActor(player), wk->faceDir) & 3) == 0) {
        return TRUE;
    }
    return FALSE;
}

// The steps of the surf exit: face the shore, jump to it, land and change back, and let the actors move again
enum {
    SURF_EXIT_FACE,
    SURF_EXIT_JUMP,
    SURF_EXIT_LAND,
    SURF_EXIT_END,
};

static GameEventReturnCode EventSurfExit_Callback(GameEvent *event, u32 *state, void *data) {
    SurfExitWork *work = data;
    FieldPlayer *player = Field_GetPlayer(work->field);
    FieldActor *actor;

    // The original reads the player's state and drops it
    FieldPlayer_GetExState(player);
    actor = FieldPlayer_GetActor(player);
    switch (*state) {
    case SURF_EXIT_FACE:
        DisableAllActorsMovement(GetActorMModelSystem(actor));
        CheckSetActorFaceDir(actor, work->dir);
        (*state)++;
        break;
    case SURF_EXIT_JUMP: {
        u16 acmd;

        FldeffSurfPokemon_SetFollowMode(FieldPlayer_GetRideEffect(player), SURF_POKEMON_FOLLOW_NONE);
        FieldPlayer_SetSpecialSeq(player, 1);
        FieldPlayer_FinishFormChange(player);
        acmd = GetAcmdForDir(work->dir, ACMD_JUMP_1);
        if (work->overEdge == TRUE) {
            acmd = GetAcmdForDir(work->dir, ACMD_JUMP_2);
        }
        FldAct_SetAcmd(actor, acmd);
        (*state)++;
        break;
    }
    case SURF_EXIT_LAND:
        if (FldAct_CheckAcmdEnd(actor) == TRUE) {
            FieldPlayer_ChangeToNormalForm(player);
            (*state)++;
        }
        break;
    case SURF_EXIT_END:
        EnableAllActorsMovement(GetActorMModelSystem(actor));
        FieldPlayer_SetGridHopLanded(player);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEvent *EventSurfExit_Create(FieldInteractWk *wk, GameSystem *gsys, Field *field, u16 dir, BOOL overEdge) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventSurfExit_Callback, sizeof(SurfExitWork));
    SurfExitWork *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->field = field;
    work->dir = dir;
    work->overEdge = overEdge;
    return event;
}

static GameEvent *CheckSurfExitEvent(FieldInteractWk *wk, GameSystem *gsys, Field *field) {
    FieldPlayer *player = Field_GetPlayer(field);

    if (FieldPlayer_GetExState(player) == FLD_PLAYER_EXSTATE_SURF) {
        BOOL overEdge;
        FieldG3DMapper *mapper = Field_GetG3DMapper(field);
        u16 dir = FieldPlayer_GetMoveDirByKey(wk->player, wk->held);
        u32 steps = 1;

        if (dir != PLAYER_DIR_NONE) {
            VecFx32 pos;
            u32 tileType;
            u32 tileFlags;
            u16 tileClass;

            FieldPlayer_GetWPosInDir(player, dir, &pos);
            tileType = GetTileTypeAtPos(mapper, &pos);
            tileFlags = GetTileFlags(tileType);
            tileClass = GetTileClass(tileType);
            overEdge = FALSE;
            if (MapTile_IsSurfEdge(tileClass) == TRUE) {
                ExpandVecInGridDir(dir, &pos, GRID_SIZE);
                tileFlags = GetTileFlags(GetTileTypeAtPos(mapper, &pos));
                overEdge = TRUE;
                steps++;
            }
            if (IsSurfExitPathClear(wk, field, steps) == TRUE && !(tileFlags & TILE_FLAG_COLLISION) &&
                !(tileFlags & TILE_FLAG_WATER)) {
                return EventSurfExit_Create(wk, gsys, field, dir, overEdge);
            }
        }
    }
    return NULL;
}

static FieldActor *FindActorForInteraction(FieldInteractWk *wk, Field *field) {
    FieldActor *actor = FieldPlayer_GetActor(wk->player);
    VecFx32 pos;
    s16 x;
    s16 y;
    s16 z;
    FieldActor *found;

    FieldPlayer_GetGPosInFront(wk->player, &x, &y, &z);
    VecGPosToWPos(x, y, z, &pos);
    if (MapTile_IsReceptionCounter(GetTileClass(GetTileTypeAtPos(Field_GetG3DMapper(field), &pos))) == TRUE) {
        AdjusGridXZByDir(GetActorFaceDir(actor), &x, &z, 1);
    }
    found = FindActorByGPos(Field_GetActorSystem(field), x, z, GetActorPosY(actor), GRID_SIZE, FALSE);
    if (found != NULL && func_ov012_021675b4(found)) {
        return found;
    }
    return NULL;
}

// The actor in front of the player on a rail
static FieldActor *FindActorForInteractionRail(FieldInteractWk *wk, Field *field) {
    RailPosition pos;

    if (RailActor_GetFrontLocation(FieldPlayer_GetActor(wk->player), &pos)) {
        return FindActorByRailPos(Field_GetActorSystem(field), &pos, FALSE);
    }
    return NULL;
}

// The script of the tile in front of the player, or of the water to surf on or the waterfall
static u16 CheckFieldEnvInteraction(FieldInteractWk *wk, Field *field) {
    u32 under;
    u32 front;
    u16 tileClass;
    u32 i;

    if (GameData_IsForceSeasonSync(wk->gameData) == TRUE) {
        return SCRID_NONE;
    }
    under = FieldPlayer_GetTileTypeUnder(wk->player);
    front = FieldPlayer_GetTileTypeInDir(wk->player, wk->faceDir);
    tileClass = GetTileClass(front);
    for (i = 0; i < NELEMS(SPECIAL_TILE_SCRIPTS); i++) {
        if (wk->faceDir == SPECIAL_TILE_SCRIPTS[i].dir || SPECIAL_TILE_SCRIPTS[i].dir == PLAYER_DIR_NONE) {
            if (SPECIAL_TILE_SCRIPTS[i].check(tileClass)) {
                return SPECIAL_TILE_SCRIPTS[i].scrId;
            }
        }
    }
    if (!IsZoneNormal(wk->zoneId) || GameData_IsForceSeasonSync(wk->gameData) == TRUE) {
        return SCRID_NONE;
    }
    if (CheckSurfHeightAllow(wk->player, wk->faceDir) &&
        GameData_FindPartyPkmByMove(wk->gameData, MOVE_SURF) != PARTY_SIZE) {
        return SCRID_SURF;
    }
    if (CheckCanInteractWaterfall(wk->player, under, front)) {
        return SCRID_WATERFALL;
    }
    return SCRID_NONE;
}

// The first Pokémon of the party, not an egg, that knows the move, or PARTY_SIZE
static u32 GameData_FindPartyPkmByMove(GameData *gameData, u16 move) {
    int count;
    PokeParty *party;
    int i;

    party = GameData_GetParty(gameData);
    count = PokeParty_GetPkmCount(party);
    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == FALSE &&
            (move == PokeParty_GetParam(pkm, PKM_PARAM_MOVE1, NULL) ||
             move == PokeParty_GetParam(pkm, PKM_PARAM_MOVE2, NULL) ||
             move == PokeParty_GetParam(pkm, PKM_PARAM_MOVE3, NULL) ||
             move == PokeParty_GetParam(pkm, PKM_PARAM_MOVE4, NULL))) {
            return i;
        }
    }
    return PARTY_SIZE;
}

// The number of the party's Pokémon that can battle
u32 FieldEventProvider_CountAlivePartyMembers(GameSystem *gsys) {
    PokeParty *party = GameData_GetParty(GSYS_GetGameData(gsys));
    int count = PokeParty_GetPkmCount(party);
    int i = 0;
    u32 result = 0;

    for (; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == FALSE &&
            PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL) != 0) {
            result++;
        }
    }
    return result;
}

// Whether no actor stands on the tiles the player would jump over to leave the water
static BOOL IsSurfExitPathClear(FieldInteractWk *wk, Field *field, int steps) {
    FieldActor *actor = FieldPlayer_GetActor(wk->player);
    u16 dir = wk->moveDir;

    s16 x = GetGPosX(actor);
    s16 z = GetGPosZ(actor);
    int i;

    if (steps == 0) {
        steps++;
    }
    for (i = 0; i < steps; i++) {
        x += GetDirectionVectorCompX(dir);
        z += GetDirectionVectorCompZ(dir);
        if (GetFirstActorOnGPos(Field_GetActorSystem(field), x, z, TRUE) != NULL) {
            return FALSE;
        }
    }
    return TRUE;
}
