// The events that run battles from the field: wild, trainer, Trial House and capture demonstration battles, and what
// the game records after them. The name is the ROM's own, from GFL_HeapAllocate's file argument.
// Function and data names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "app/event_battle_return.h"
#include "battle/battle_proc.h"
#include "battle/btl_result.h"
#include "battle/btl_setup.h"
#include "battle/trainer_data.h"
#include "constants/battle.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/trainer_classes.h"
#include "constants/version.h"
#include "dsprot/dsprot.h"
#include "field/badge_gate.h"
#include "field/encounter.h"
#include "field/event_battle.h"
#include "field/event_battle_lose.h"
#include "field/event_sound.h"
#include "field/event_wild_battle.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/field_map.h"
#include "field/field_status.h"
#include "field/game_beacon_set.h"
#include "field/infinity_dungeon.h"
#include "field/infinity_dungeon_gimmick.h"
#include "field/infinity_dungeon_rooms.h"
#include "field/player_state.h"
#include "field/trial_house.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "save/encounter.h"
#include "save/pokedex_savedata.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/traded_pokemon.h"
#include "system/aeabi.h"
#include "system/area_data.h"
#include "system/comm_player_support.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/zone_weather.h"

// EventBattleCall_Callback's steps
enum {
    BATTLE_CALL_STEP_PLAY_BGM,
    BATTLE_CALL_STEP_ENC_EFFECT,
    BATTLE_CALL_STEP_CLOSE_FIELD,
    BATTLE_CALL_STEP_BATTLE,
    BATTLE_CALL_STEP_BATTLE_RETURN,
    BATTLE_CALL_STEP_END_BATTLE,
    BATTLE_CALL_STEP_SYNC_GAME_DATA,
    BATTLE_CALL_STEP_FADE_BGM,
    BATTLE_CALL_STEP_OPEN_FIELD,
    BATTLE_CALL_STEP_WAIT_BGM,
    BATTLE_CALL_STEP_FADE_IN,
    BATTLE_CALL_STEP_END,
};

// The kinds of trainer, by the beacons their battles send (TRAINER_BATTLE_BEACONS)
enum {
    TRAINER_BEACON_KIND_TRAINER,
    TRAINER_BEACON_KIND_LEADER,
    TRAINER_BEACON_KIND_SPECIAL,
};

// The beacon a battle sends: at its start, on a win, when the player flees, and when the foe flees (none)
enum {
    BATTLE_BEACON_START,
    BATTLE_BEACON_WIN,
    BATTLE_BEACON_ESCAPE,
    BATTLE_BEACON_FOE_FLED,
};

// The work of EventBattleCall_Callback, the battle itself
typedef struct {
    GameSystem *gsys;
    GameData *gameData;
    BtlSetup *setup;
    EventBattleReturnParam battleReturnParam;
    // Whether the caller returns to the field itself
    u32 callerReturns;
    // Whether losing doesn't black the player out
    u32 noBlackout;
    // Whether it is the Trial House's
    u32 isTrialHouse;
    // Whether the caller frees the setup
    u32 callerFreesSetup;
    u32 encEffect;
    u32 flags;
} EventBattleCallWork;

// The work of EventWildBattleCall_Callback
typedef struct {
    GameSystem *gsys;
    BtlSetup *setup;
    // Whether the caller returns to the field itself, and handles a loss
    u8 callerReturns;
    u8 specialIndex;
    u8 unk0A;
    // An ENCOUNT_MODE_*
    u32 encountMode;
} EventWildBattleCallWork;

static GameEventReturnCode EventBattleCall_Callback(GameEvent *event, u32 *state, void *work);
static void EventBattleCall_BroadcastTrainerBattle(u16 trainerId, u32 kind);
static void EventBattleCall_BroadcastWildBattle(BtlSetup *setup, u32 kind);
static void EventBattleCall_BroadcastBattleResult(EventBattleCallWork *work);
static void EventBattleCall_RecordEnemyFled(EventBattleCallWork *work);
static BOOL EventBattleCall_IsResultDefeat(EventBattleCallWork *work);
static void EventBattleCall_SyncGameDataOnReturn(EventBattleCallWork *work, GameData *gameData);
static void EventBattleCall_Setup(EventBattleCallWork *work, GameSystem *gsys, BtlSetup *setup);
static void EventBattleCall_ReleaseBtlSetup(EventBattleCallWork *work);
static void *EventBattleCall_DSProtTamper1(void *seed, void *work);
static void *EventBattleCall_DSProtNop1(void *seed, void *work);
static void *EventBattleCall_DSProtTamper3(void *seed, void *work);
static void *EventBattleCall_DSProtNop3(void *seed, void *work);

// The result of a wild battle for each battle result
static const u8 WILD_BATTLE_RESULT[7] = { 0xff, 2, 0xff, 1, 1, 0, 0 };

// The beacons a trainer battle sends, for its kind of trainer and whether it starts or ends
static void (*const TRAINER_BATTLE_BEACONS[3][2])(u16 trainerId) = {
    { GameBeacon_BroadcastTrainerBattleStart, GameBeacon_BroadcastTrainerBattleWin },
    { GameBeacon_BroadcastLeaderBattleStart, GameBeacon_BroadcastLeaderBattleWin },
    { GameBeacon_BroadcastSpecialTrainerBattleStart, GameBeacon_BroadcastSpecialTrainerBattleWin },
};

// Whether each battle result is not a defeat, for each battle type
// clang-format off
static const u8 BTL_RESULT_LUT[7][5] = {
    {0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0},
    {1, 2, 2, 0, 0},
    {1, 2, 1, 1, 0},
    {1, 2, 2, 2, 0},
    {0, 0, 0, 0, 0},
};

// clang-format on

// The trainer classes that send the third kind of beacon
static u16 sSpecialBeaconTrainerClasses[7] = {
    TRAINER_CLASS_SUBWAY_BOSS_INGO,   TRAINER_CLASS_NO_DATA_M_156,      TRAINER_CLASS_TEAM_PLASMA_SHADOW,
    TRAINER_CLASS_PKMN_TRAINER_ROOD,  TRAINER_CLASS_PKMN_TRAINER_CHILI, TRAINER_CLASS_PKMN_TRAINER_CRESS,
    TRAINER_CLASS_PKMN_TRAINER_CILAN,
};

static GameEventReturnCode EventWildBattleCall_Callback(GameEvent *event, u32 *state, void *work) {
    EventWildBattleCallWork *wk = work;
    GameSystem *gsys = wk->gsys;
    GameData *gameData = GSYS_GetGameData(gsys);

    switch (*state) {
    case 0: {
        GameEvent *battle = GameEvent_Create(gsys, NULL, EventBattleCall_Callback, sizeof(EventBattleCallWork));
        EventBattleCallWork *battleWork = GameEvent_GetData(battle);
        PartyPkm *pkm;
        u32 species, form;
        Field *field;

        EventBattleCall_Setup(battleWork, gsys, wk->setup);
        battleWork->callerReturns = TRUE;
        pkm = PokeParty_GetPkm(BtlSetup_GetParty(wk->setup, 1), 0);
        species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        field = GSYS_GetField(gsys);
        EventBattleCall_DecideEnvWild(species, form, wk->encountMode, wk->setup->battleStyle == BTL_STYLE_DOUBLE, field,
                                      &battleWork->encEffect, &battleWork->setup->fieldSituation.bgm);
        GameEvent_ChainNext(event, battle);
        (*state)++;
        break;
    }
    case 1: {
        u32 result = GameData_GetLastBtlResult(gameData);
        BOOL defeat = IsBattleResultDefeat(result, 0);

        if (result == BTL_RESULT_CAPTURE && wk->specialIndex != 0xff) {
            EncountSave *save = SaveControl_GetEncountSave(GameData_GetSaveControl(gameData));

            SetNPokeCaught(save, wk->specialIndex);
        }
        if (wk->unk0A) {
            FieldActor *player = FindPlayerFieldActor(GameData_GetMMSys(gameData));

            if (IsActorReflectionHidden(player)) {
                SetActorReflectionHidden(player, 0);
            }
        }
        if (!defeat && GameData_CheckPairFlag(gameData)) {
            PokeParty_RecoverAll(GameData_GetParty(gameData));
        }
        if (wk->callerReturns == 1) {
            CommPlayerSupport_Init(GameData_GetCommPlayerSupport(gameData));
            return GAMEEVENT_DONE;
        }
        if (defeat) {
            CommPlayerSupport_Init(GameData_GetCommPlayerSupport(gameData));
            GameEvent_Replace(event, EventBattleLose_Create(gsys));
        } else {
            GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, GSYS_GetField(gsys),
                                                                        FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        }
        (*state)++;
        break;
    }
    case 2:
        (*state)++;
        break;
    case 3:
        CommPlayerSupport_Init(GameData_GetCommPlayerSupport(gameData));
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventWildBattleCall_CreateCore(GameSystem *gsys, Field *field, BtlSetup *setup, u8 callerReturns,
                                          u32 encountMode, u8 specialIndex) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWildBattleCall_Callback, sizeof(EventWildBattleCallWork));
    EventWildBattleCallWork *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->setup = setup;
    work->callerReturns = callerReturns;
    work->encountMode = encountMode;
    work->specialIndex = specialIndex;
    work->unk0A = BtlSetup_CheckFlag(setup, 1);
    if (encountMode == ENCOUNT_MODE_EFFECT) {
        BtlSetup_SetFlag(setup, 1 << 14);
    }
    EventBattleCall_BroadcastWildBattle(setup, BATTLE_BEACON_START);
    RecordAddOne(GameData_GetRecords(GSYS_GetGameData(gsys)), 5);
    EffectEncount_ConsumeSpot(Field_GetEncountSystem(field));
    return event;
}

GameEvent *EventWildBattleCall_Create(GameSystem *gsys, Field *field, BtlSetup *setup, u8 callerReturns,
                                      u32 encountMode) {
    return EventWildBattleCall_CreateCore(gsys, field, setup, callerReturns, encountMode, 0xff);
}

GameEvent *EventTrainerBattleCall_Create(GameSystem *gsys, Field *field, u32 style, u32 partnerId, u32 trainerId,
                                         u32 trainerId2, u32 flags) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBattleCall_Callback, sizeof(EventBattleCallWork));
    EventBattleCallWork *work = GameEvent_GetData(event);
    EncountSystem *encountSystem;
    GameData *gameData;
    GameRecords *records;
    BtlSetup *setup;

    work->flags = flags;
    encountSystem = Field_GetEncountSystem(field);
    gameData = GSYS_GetGameData(gsys);
    records = GameData_GetRecords(gameData);
    setup = BtlSetup_Create(4);
    BtlSetup_SetTrainerLocal(encountSystem, setup, style, partnerId, trainerId, trainerId2, 4);
    adjustPkmLvForChallengeKeys(setup, gameData, Field_GetPlayerStateZoneID(field));
    EventBattleCall_BroadcastTrainerBattle(trainerId, BATTLE_BEACON_START);
    RecordAddOne(records, 6);
    RecordAddOne(records, 0x56);
    EventBattleCall_Setup(work, gsys, setup);
    work->callerReturns = TRUE;
    if (flags & 1) {
        BtlSetup_SetFlag(setup, 0x100);
        work->noBlackout = TRUE;
    }
    if (flags & 4) {
        PokeParty *party = BtlSetup_GetParty(setup, 1);

        FesGimmick_BoostPartyLevels(Field_GetFesGimmick(field), party);
    }
    if (flags & 2) {
        if (AreaData_IsInfinityDungeon(ZoneData_GetAreaID(Field_GetPlayerStateZoneID(field)))) {
            u8 level = KeyDataSave_GetArea(getKeyDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))));

            setup->fieldSituation.env.terrain =
                InfinityDungeon_GetVersion(InfinityDungeonGimmick_GetDungeon(field)) == VERSION_BLACK2 ? 0x12 : 0x13;
            setup->fieldSituation.env.bgType = level;
            setup->unkDD_5 = TRUE;
            setup->noPokedexRegistration = TRUE;
        }
    }
    EventBattleCall_DecideEnvTrainer(trainerId, field, &work->encEffect, &work->setup->fieldSituation.bgm);
    EffectEncount_ConsumeSpot(Field_GetEncountSystem(field));
    return event;
}

GameEvent *EventBSubwayBattleCall_Create(GameSystem *gsys, Field *field, BtlSetup *setup) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBattleCall_Callback, sizeof(EventBattleCallWork));
    EventBattleCallWork *work = GameEvent_GetData(event);

    EventBattleCall_Setup(work, gsys, setup);
    work->callerReturns = FALSE;
    work->noBlackout = TRUE;
    work->callerFreesSetup = TRUE;
    work->encEffect = 9;
    EffectEncount_ConsumeSpot(Field_GetEncountSystem(field));
    return event;
}

GameEvent *CreateTrialHouseBattleEvent(GameSystem *gsys, Field *field, BtlSetup *setup) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBattleCall_Callback, sizeof(EventBattleCallWork));
    EventBattleCallWork *work = GameEvent_GetData(event);

    EventBattleCall_Setup(work, gsys, setup);
    work->callerReturns = FALSE;
    work->isTrialHouse = TRUE;
    work->noBlackout = TRUE;
    work->encEffect = 9;
    work->setup->fieldSituation.bgm = SEQ_BGM_VS_SUBWAY_TRAINER;
    EffectEncount_ConsumeSpot(Field_GetEncountSystem(field));
    return event;
}

GameEvent *EventInfinityDungeonBattleCall_Create(GameSystem *gsys, Field *field, BtlSetup *setup) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBattleCall_Callback, sizeof(EventBattleCallWork));
    EventBattleCallWork *work;

    setup->unkDD_5 = TRUE;
    setup->noPokedexRegistration = TRUE;
    work = GameEvent_GetData(event);
    EventBattleCall_Setup(work, gsys, setup);
    work->callerReturns = TRUE;
    work->noBlackout = FALSE;
    work->callerFreesSetup = FALSE;
    work->encEffect = 9;
    work->setup->fieldSituation.bgm = SEQ_BGM_VS_TRAINER;
    work->setup->fieldSituation.winBgm = SEQ_BGM_WIN2;
    EffectEncount_ConsumeSpot(Field_GetEncountSystem(field));
    return event;
}

GameEvent *EventWbtBattleCall_Create(GameSystem *gsys, Field *field, BtlSetup *setup) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBattleCall_Callback, sizeof(EventBattleCallWork));
    EventBattleCallWork *work = GameEvent_GetData(event);

    EventBattleCall_Setup(work, gsys, setup);
    work->callerReturns = TRUE;
    work->noBlackout = TRUE;
    work->callerFreesSetup = TRUE;
    work->encEffect = 0x1c;
    EffectEncount_ConsumeSpot(Field_GetEncountSystem(field));
    return event;
}

GameEvent *EventCaptureDemo_Create(GameSystem *gsys, Field *field, HeapID heapId) {
    HeapID tempHeapId = HEAPID_TAIL(heapId);
    GameData *gameData = GSYS_GetGameData(gsys);
    BtlSetup *setup = BtlSetup_Create(heapId);
    PokeParty *party = PokeParty_Create(tempHeapId);
    PokeParty *enemyParty = PokeParty_Create(tempHeapId);
    PartyPkm *pkm = GFL_HeapAllocate(tempHeapId, PokeParty_GetPkmRawSize(), TRUE, "event_battle.c", 673);
    BtlFieldStatus status;
    GameEvent *event;
    EventBattleCallWork *work;

    PokeParty_CreateTempPkm(pkm, SPECIES_LILLIPUP, 5, 0);
    PokeParty_SetMove(pkm, MOVE_TACKLE, 0);
    PokeParty_SetMove(pkm, MOVE_LEER, 1);
    PokeParty_SetMove(pkm, MOVE_ODOR_SLEUTH, 2);
    PokeParty_SetMove(pkm, 0, 3);
    PokeParty_AddPkm(party, pkm);
    PokeParty_CreateTempPkm(pkm, SPECIES_PURRLOIN, 2, 0);
    PokeParty_SetMove(pkm, MOVE_SCRATCH, 0);
    PokeParty_SetMove(pkm, MOVE_GROWL, 1);
    PokeParty_SetMove(pkm, 0, 2);
    PokeParty_SetMove(pkm, 0, 3);
    PokeParty_AddPkm(enemyParty, pkm);
    SaveBtlFieldStatus(&status, gameData, field);
    status.terrain = 5;
    status.bgId = 1;
    BtlSetup_SetCaptureDemo(setup, gameData, party, enemyParty, &status, heapId);
    GFL_HeapFree(pkm);
    GFL_HeapFree(enemyParty);
    GFL_HeapFree(party);
    event = GameEvent_Create(gsys, NULL, EventBattleCall_Callback, sizeof(EventBattleCallWork));
    work = GameEvent_GetData(event);
    EventBattleCall_Setup(work, gsys, setup);
    work->callerReturns = TRUE;
    EventBattleCall_DecideEnvWild(SPECIES_PURRLOIN, 0, 5, FALSE, field, &work->encEffect,
                                  &work->setup->fieldSituation.bgm);
    work->encEffect = 0;
    return event;
}

GameEvent *LoadTradedPokemonBattleStats(GameSystem *gsys, Field *field, u32 style, u32 partnerId, u32 trainerId,
                                        u32 trainerId2, u32 flags, u32 index) {
    TradedPokemonSave *block = GetTradedPokemonBlock(GSYS_GetGameData(gsys));
    GameEvent *event = EventTrainerBattleCall_Create(gsys, field, style, partnerId, trainerId, trainerId2, flags);

    if (TradedPokemonSave_IsTraded(block, index)) {
        EventBattleCallWork *work = GameEvent_GetData(event);
        PartyPkm *pkm = PokeParty_GetPkm(work->setup->party[1], 0);
        u32 species = TradedPokemonSave_GetSpecies(block, index, GAME_VERSION);
        u8 level = TradedPokemonSave_GetBattleLevel(block, index);
        u32 id = TradedPokemonSave_GetId(block, index);

        PokeParty_CreatePkm(pkm, species, level, id, PKM_IVS_RANDOM, TradedPokemonSave_GetPersonality(block, index));
        if (TradedPokemonSave_HasAbility(block, index)) {
            PokeParty_SetParam(pkm, 0x71, TRUE);
            PokeParty_SetParam(pkm, PKM_PARAM_ABILITY, TradedPokemonSave_GetAbility(block, index));
        }
        PokeParty_SetParam(pkm, PKM_PARAM_NICKNAME_RAW, (uptr)TradedPokemonSave_GetNickname(block, index));
        setChangedPkmSpecies(pkm, (u16)species);
        PokeParty_SetDefaultMoves(pkm);
    }
    return event;
}

static GameEventReturnCode EventBattleCall_Callback(GameEvent *event, u32 *state, void *work) {
    EventBattleCallWork *wk = work;
    GameSystem *gsys = wk->gsys;
    GameData *gameData = wk->gameData;
    Field *field = GSYS_GetField(gsys);
    u32 seed = 0x13a1ab5;

    switch (*state) {
    case BATTLE_CALL_STEP_PLAY_BGM:
        GameEvent_ChainNext(event, EventBattleBGMPlay_Create(gsys, wk->setup->fieldSituation.bgm));
        (*state)++;
        break;
    case BATTLE_CALL_STEP_ENC_EFFECT:
        EncEff_StartEvent(Field_GetEncEff(field), event, wk->encEffect);
        (*state)++;
        break;
    case BATTLE_CALL_STEP_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        (*state)++;
        break;
    case BATTLE_CALL_STEP_BATTLE:
        FieldStatus_SetBusyFlag(GameData_GetFieldStatus(gameData), FLD_STATUS_BUSY_BATTLE);
        CommPlayerSupport_Init(GameData_GetCommPlayerSupport(gameData));
        GSYS_QueueProcAsEvent(event, OVERLAY_ID(167), &BATTLE_PROC_FUNCTIONS, wk->setup);
        (*state)++;
        break;
    case BATTLE_CALL_STEP_BATTLE_RETURN:
        if (wk->setup->studioMode == BTL_STUDIO_MODE_NONE) {
            wk->battleReturnParam.setup = wk->setup;
            wk->battleReturnParam.gameData = gameData;
            GSYS_QueueProcAsEvent(event, OVERLAY_ID(166), &EVENT_BATTLE_RETURN_PROC_FUNCTIONS, &wk->battleReturnParam);
        }
        (*state)++;
        break;
    case BATTLE_CALL_STEP_END_BATTLE: {
        u32 index;

        GFL_OvlLoad(OVERLAY_ID(337));
        DSProt_Init();
        FieldStatus_SetBusyFlag(GameData_GetFieldStatus(gameData), FLD_STATUS_BUSY_NONE);
        CommPlayerSupport_EndBattle(GameData_GetCommPlayerSupport(gameData));
        index = OS_GetVBlankCount() & 1;
        data_ov337_02182440 = index;
        data_ov337_02182444[index] = EventBattleCall_DSProtNop1;
        data_ov337_02182444[index ^ 1] = EventBattleCall_DSProtTamper1;
        (*state)++;
        break;
    }
    case BATTLE_CALL_STEP_SYNC_GAME_DATA: {
        u32 shift;
        u32 checksum;
        const u32 *code = (const u32 *)DSProt_CallRunChecks1;
        u32 index;

#ifdef PLATFORM_PC
        // A host's code is its own machine code, never DS Protect's ARM code, so the port skips its checksum
        // and takes a genuine cartridge's path (lib/dsprot/include/dsprot/dsprot.h)
        checksum = DSPROT_CHECKSUM;
#else
        for (shift = 0x25, checksum = 0; shift != 0; shift--) {
            checksum ^= (*code >> shift) | (*code << (32 - shift));
            code++;
        }
#endif
        if (checksum == DSPROT_CHECKSUM) {
            DSProt_CallRunChecks1(&seed, wk);
        } else {
            data_ov337_02182444[data_ov337_02182440 ^ 1](&seed, wk);
        }
        EventBattleCall_SyncGameDataOnReturn(wk, gameData);
        if (wk->isTrialHouse) {
            SyncTrialHouseWkStatsFromBattle(*GetTrialHouseWkPPtr(gameData), wk->setup);
        }
        (*state)++;
        index = OS_GetVBlankCount() & 1;
        data_ov337_02182440 = index;
        data_ov337_02182444[index] = EventBattleCall_DSProtNop3;
        data_ov337_02182444[index ^ 1] = EventBattleCall_DSProtTamper3;
        break;
    }
    case BATTLE_CALL_STEP_FADE_BGM: {
        u32 shift;
        u32 checksum;
        const u32 *code = (const u32 *)DSProt_CallRunChecks3;

#ifdef PLATFORM_PC
        // A host's code is its own machine code, never DS Protect's ARM code, so the port skips its checksum
        // and takes a genuine cartridge's path (lib/dsprot/include/dsprot/dsprot.h)
        checksum = DSPROT_CHECKSUM;
#else
        for (shift = 0x25, checksum = 0; shift != 0; shift--) {
            checksum ^= (*code >> shift) | (*code << (32 - shift));
            code++;
        }
#endif
        if (checksum == DSPROT_CHECKSUM) {
            DSProt_CallRunChecks3(&seed, wk);
        } else {
            data_ov337_02182444[data_ov337_02182440 ^ 1](&seed, wk);
        }
        if (wk->noBlackout == FALSE && EventBattleCall_IsResultDefeat(wk) == TRUE) {
            GameEvent_ChainNext(event, EventBGMFadeStop_Create(gsys, 30));
        } else {
            GameEvent_ChainNext(event, EventBGMFadePop_Create(gsys));
        }
        GFL_OvlUnload(OVERLAY_ID(337));
        (*state)++;
        break;
    }
    case BATTLE_CALL_STEP_OPEN_FIELD:
        if (wk->noBlackout == FALSE && EventBattleCall_IsResultDefeat(wk) == TRUE) {
            if (wk->callerReturns == TRUE) {
                EventBattleCall_ReleaseBtlSetup(wk);
                return GAMEEVENT_DONE;
            }
            EventBattleCall_ReleaseBtlSetup(wk);
            GameEvent_Replace(event, EventBattleLose_Create(gsys));
            return GAMEEVENT_CONTINUE;
        }
        if (GameData_CheckPairFlag(gameData)) {
            PokeParty_RecoverAll(GameData_GetParty(gameData));
        }
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case BATTLE_CALL_STEP_WAIT_BGM:
        GameEvent_ChainNext(event, EventBGMFadeWait_Create(gsys));
        (*state)++;
        break;
    case BATTLE_CALL_STEP_FADE_IN:
        if (wk->callerReturns == FALSE) {
            GameEvent_ChainNext(event,
                                CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        }
        (*state)++;
        break;
    case BATTLE_CALL_STEP_END:
        EventBattleCall_ReleaseBtlSetup(wk);
        return GAMEEVENT_DONE;
    }
    if (seed % 0x1933 != 0) {
        GFL_VBlankTCBAdd(get_mt, NULL, 0x7f);
        g_AntiPiracyTaskCount++;
    }
    return GAMEEVENT_CONTINUE;
}

// The kind of beacon for the trainer: special for the classes of sSpecialBeaconTrainerClasses, and otherwise by the
// class's music group
static u32 EventBattleCall_GetTrainerBeaconKind(u16 trainerId) {
    u16 trainerClass = TrainerData_GetParam(trainerId, TRAINER_PARAM_CLASS);
    u32 group = GetTrainerClassBGMGroupId(trainerClass);
    u32 i;

    for (i = 0; i < 7; i++) {
        if (trainerClass == sSpecialBeaconTrainerClasses[i]) {
            return TRAINER_BEACON_KIND_SPECIAL;
        }
    }
    if (group == 0xf || group == 7) {
        return TRAINER_BEACON_KIND_TRAINER;
    }
    if (group == 0) {
        return TRAINER_BEACON_KIND_LEADER;
    }
    return TRAINER_BEACON_KIND_SPECIAL;
}

static void EventBattleCall_BroadcastTrainerBattle(u16 trainerId, u32 kind) {
    TRAINER_BATTLE_BEACONS[EventBattleCall_GetTrainerBeaconKind(trainerId)][kind](trainerId);
}

static void EventBattleCall_BroadcastWildBattle(BtlSetup *setup, u32 kind) {
    u16 species = PokeParty_GetParam(PokeParty_GetPkm(BtlSetup_GetParty(setup, 1), 0), PKM_PARAM_SPECIES, NULL);

    switch (kind) {
    case BATTLE_BEACON_START:
        GameBeacon_BroadcastWildBattleStart(species);
        break;
    case BATTLE_BEACON_WIN:
        GameBeacon_BroadcastWildBattleWin(species);
        break;
    case BATTLE_BEACON_ESCAPE:
        GameBeacon_BroadcastBattleEscape(species);
        break;
    }
}

static void EventBattleCall_BroadcastBattleResult(EventBattleCallWork *work) {
    BtlSetup *setup = work->setup;
    PokeParty *party = GameData_GetParty(work->gameData);
    PartyPkm *pkm = PokeParty_GetPkm(party, isEggInParty(party));

    if (setup->result != BTL_RESULT_DEFEAT && setup->result != BTL_RESULT_DRAW) {
        u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        StrBuf *name = GFL_StrBufCreate(12, HEAPID_TAIL(4));
        u16 hp, maxHp;
        int i;

        PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME, name);
        hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);
        maxHp = PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL);
        if (hp == 0) {
            GameBeacon_BroadcastFainted(species, name);
        } else if (hp * 2 <= maxHp) {
            GameBeacon_BroadcastLowHP(species, name);
        }
        for (i = 0; i < 4; i++) {
            if (PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL) != 0 &&
                PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP + i, NULL) == 0) {
                GameBeacon_BroadcastLowPP(species, name);
                break;
            }
        }
        if (PokeParty_GetParam(pkm, PKM_PARAM_STATUS, NULL)) {
            GameBeacon_BroadcastStatusCondition(name);
        }
        GFL_StrBufFree(name);
    }
    switch (setup->battleType) {
    case 0:
        if (setup->result == BTL_RESULT_FLEE) {
            EventBattleCall_BroadcastWildBattle(setup, BATTLE_BEACON_ESCAPE);
        } else if (setup->result == BTL_RESULT_FLEE_FOE) {
            EventBattleCall_BroadcastWildBattle(setup, BATTLE_BEACON_FOE_FLED);
        } else if (setup->result == BTL_RESULT_VICTORY) {
            EventBattleCall_BroadcastWildBattle(setup, BATTLE_BEACON_WIN);
        }
        break;
    case 1:
        if (setup->result == BTL_RESULT_VICTORY && !(work->flags & 4)) {
            EventBattleCall_BroadcastTrainerBattle(setup->trainers[1]->trainerId, BATTLE_BEACON_WIN);
        }
        break;
    }
}

static void EventBattleCall_RecordEnemyFled(EventBattleCallWork *work) {
    BtlSetup *setup = work->setup;
    GameRecords *records = GameData_GetRecords(GSYS_GetGameData(work->gsys));

    if (setup->battleType == 0 && setup->result == BTL_RESULT_FLEE_FOE) {
        RecordAddOne(records, 0x4f);
    }
}

BOOL IsBattleResultDefeat(u32 result, u32 battleType) {
    if (battleType == 4) {
        return FALSE;
    }
    if (BTL_RESULT_LUT[result][battleType] == 0) {
        return TRUE;
    }
    return FALSE;
}

u32 GetWildBattleResultByCombined(u32 result) {
    u8 wildResult = WILD_BATTLE_RESULT[result];

    if (wildResult == 0xff) {
        wildResult = 1;
    }
    return wildResult;
}

static BOOL EventBattleCall_IsResultDefeat(EventBattleCallWork *work) {
    return IsBattleResultDefeat(work->setup->result, work->setup->battleType);
}

static void EventBattleCall_SyncGameDataOnReturn(EventBattleCallWork *work, GameData *gameData) {
    GameData_SetLastBtlResult(gameData, work->setup->result);
    EventBattleCall_BroadcastBattleResult(work);
    EventBattleCall_RecordEnemyFled(work);
    ResetWeather(work->gsys, PlayerState_GetZoneID(GameData_GetPlayerState(gameData)));
}

static void EventBattleCall_Setup(EventBattleCallWork *work, GameSystem *gsys, BtlSetup *setup) {
    MIi_CpuClear32(0, work, sizeof(EventBattleCallWork));
    work->gsys = gsys;
    work->gameData = GSYS_GetGameData(gsys);
    work->setup = setup;
    work->callerReturns = FALSE;
    work->noBlackout = FALSE;
    work->isTrialHouse = FALSE;
}

static void EventBattleCall_ReleaseBtlSetup(EventBattleCallWork *work) {
    if (work->callerFreesSetup == FALSE) {
        BtlSetup_Free(work->setup);
    }
}

// The fallbacks of the anti-piracy checks
static void *EventBattleCall_DSProtTamper1(void *seed, void *work) {
    GFL_RandomMT();
    GFL_RandomMT();
    *(u32 *)seed += 0x3d1;
    return seed;
}

static void *EventBattleCall_DSProtNop1(void *seed, void *work) {
    GFL_RandomMT();
    GFL_RandomMT();
    return work;
}

static void *EventBattleCall_DSProtTamper3(void *seed, void *work) {
    GFL_RandomMT();
    GFL_RandomMT();
    *(u32 *)seed += 0x9d;
    return seed;
}

static void *EventBattleCall_DSProtNop3(void *seed, void *work) {
    return work;
}
