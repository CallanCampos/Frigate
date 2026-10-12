// The events that run battles from the field: wild, trainer, Trial House and capture demonstration battles, and what
// the game records after them. The name is the ROM's own, from GFL_HeapAllocate's file argument.
// Function and data names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "app/event_battle_return.h"
#include "app/ov337.h"
#include "battle/battle_proc.h"
#include "battle/btl_result.h"
#include "battle/btl_setup.h"
#include "battle/trainer_data.h"
#include "constants/moves.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "constants/version.h"
#include "field/badge_gate.h"
#include "field/infinity_dungeon.h"
#include "field/infinity_dungeon_gimmick.h"
#include "field/encounter.h"
#include "field/event_battle.h"
#include "field/event_battle_lose.h"
#include "field/event_sound.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_event.h"
#include "field/field_map.h"
#include "field/field_status.h"
#include "field/game_beacon_set.h"
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
#include "save/pokedex.h"
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

// The work of EventBattleCall_Callback, the battle itself
typedef struct {
    GameSystem *gsys;
    GameData *gameData;
    BtlSetup *setup;
    EventBattleReturnParam battleReturnParam;
    // Whether the caller returns to the field itself
    u32 unk14;
    // Whether losing doesn't black the player out
    u32 unk18;
    // Whether it is the Trial House's
    u32 unk1C;
    // Whether the caller frees the setup
    u32 unk20;
    u32 encEffect;
    u32 flags;
} EventBattleCallWork;

// The work of EventWildBattleCall_Callback
typedef struct {
    GameSystem *gsys;
    BtlSetup *setup;
    u8 unk08;
    u8 specialIndex;
    u8 unk0A;
    u32 unk0C;
} EventWildBattleCallWork;

static GameEventReturnCode EventBattleCall_Callback(GameEvent *event, u32 *state, void *work);
static void func_ov012_02168edc(u16 trainerId, u32 kind);
static void func_ov012_02168efc(BtlSetup *setup, u32 kind);
static void func_ov012_02168f38(EventBattleCallWork *work);
static void func_ov012_0216907c(EventBattleCallWork *work);
static BOOL EventBattleCall_IsResultDefeat(EventBattleCallWork *work);
static void EventBattleCall_SyncGameDataOnReturn(EventBattleCallWork *work, GameData *gameData);
static void EventBattleCall_Setup(EventBattleCallWork *work, GameSystem *gsys, BtlSetup *setup);
static void EventBattleCall_ReleaseBtlSetup(EventBattleCallWork *work);
static void *func_ov012_02169154(u32 *seed, void *work);
static void *func_ov012_02169170(u32 *seed, void *work);
static void *func_ov012_02169180(u32 *seed, void *work);
static void *func_ov012_02169198(u32 *seed, void *work);

// The result of a wild battle for each battle result
static const u8 WILD_BATTLE_RESULT[7] = {0xff, 2, 0xff, 1, 1, 0, 0};

// The beacons a trainer battle sends, for its kind of trainer and whether it starts or ends
static void (*const data_ov012_0216dd24[3][2])(u16 trainerId) = {
    {func_ov012_0215fa90, func_ov012_0215faf0},
    {func_ov012_0215fb50, func_ov012_0215fbb0},
    {func_ov012_0215fc10, func_ov012_0215fc70},
};

// Whether each battle result is not a defeat, for each battle type
static const u8 BTL_RESULT_LUT[7][5] = {
    {0, 0, 0, 0, 0},
    {1, 1, 1, 1, 0},
    {0, 0, 0, 0, 0},
    {1, 2, 2, 0, 0},
    {1, 2, 1, 1, 0},
    {1, 2, 2, 2, 0},
    {0, 0, 0, 0, 0},
};

// The trainer classes that send the third kind of beacon
static u16 data_ov012_0216e5cc[7] = {0x9b, 0x9c, 0xc0, 0xbe, 0x0a, 0x0c, 0x0b};

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
        battleWork->unk14 = TRUE;
        pkm = PokeParty_GetPkm(BtlSetup_GetParty(wk->setup, 1), 0);
        species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
        field = GSYS_GetField(gsys);
        EventBattleCall_DecideEnvWild(species, form, wk->unk0C, wk->setup->battleStyle == 1, field,
                                      &battleWork->encEffect, &battleWork->setup->fieldSituation.bgm);
        GameEvent_ChainNext(event, battle);
        (*state)++;
        break;
    }
    case 1: {
        u32 result = GameData_GetLastBtlResult(gameData);
        BOOL defeat = IsBattleResultDefeat(result, 0);

        if (result == 5 && wk->specialIndex != 0xff) {
            EncountSave *save = SaveControl_GetEncountSave(GameData_GetSaveControl(gameData));

            SetNPokeCaught(save, wk->specialIndex);
        }
        if (wk->unk0A) {
            FieldActor *player = FindPlayerFieldActor(GameData_GetMMSys(gameData));

            if (func_ov012_0216754c(player)) {
                func_ov012_02167564(player, 0);
            }
        }
        if (!defeat && GameData_CheckPairFlag(gameData)) {
            PokeParty_RecoverAll(GameData_GetParty(gameData));
        }
        if (wk->unk08 == 1) {
            CommPlayerSupport_Init(GameData_GetCommPlayerSupport(gameData));
            return GAMEEVENT_DONE;
        }
        if (defeat) {
            CommPlayerSupport_Init(GameData_GetCommPlayerSupport(gameData));
            GameEvent_Replace(event, EventBattleLose_Create(gsys));
        } else {
            GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, GSYS_GetField(gsys), 0, 0, 1, 0, 0));
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

GameEvent *EventWildBattleCall_CreateCore(GameSystem *gsys, Field *field, BtlSetup *setup, u8 a3, u32 a4,
                                          u8 specialIndex) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWildBattleCall_Callback, sizeof(EventWildBattleCallWork));
    EventWildBattleCallWork *work = GameEvent_GetData(event);

    work->gsys = gsys;
    work->setup = setup;
    work->unk08 = a3;
    work->unk0C = a4;
    work->specialIndex = specialIndex;
    work->unk0A = BtlSetup_CheckFlag(setup, 1);
    if (a4 == 1) {
        BtlSetup_SetFlag(setup, 1 << 14);
    }
    func_ov012_02168efc(setup, 0);
    RecordAddOne(GameData_GetRecords(GSYS_GetGameData(gsys)), 5);
    func_ov036_021a2364(Field_GetEncountSystem(field));
    return event;
}

GameEvent *EventWildBattleCall_Create(GameSystem *gsys, Field *field, BtlSetup *setup, u8 a3, u32 a4) {
    return EventWildBattleCall_CreateCore(gsys, field, setup, a3, a4, 0xff);
}

GameEvent *EventTrainerBattleCall_Create(GameSystem *gsys, Field *field, u32 a2, u32 a3, u32 trainerId, u32 a5,
                                         u32 flags) {
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
    BtlSetup_SetTrainerLocal(encountSystem, setup, a2, a3, trainerId, a5, 4);
    adjustPkmLvForChallengeKeys(setup, gameData, Field_GetPlayerStateZoneID(field));
    func_ov012_02168edc(trainerId, 0);
    RecordAddOne(records, 6);
    RecordAddOne(records, 0x56);
    EventBattleCall_Setup(work, gsys, setup);
    work->unk14 = TRUE;
    if (flags & 1) {
        BtlSetup_SetFlag(setup, 0x100);
        work->unk18 = TRUE;
    }
    if (flags & 4) {
        PokeParty *party = BtlSetup_GetParty(setup, 1);

        func_ov036_021b67d8(Field_GetFesGimmick(field), party);
    }
    if (flags & 2) {
        if (func_02018f60(ZoneData_GetAreaID(Field_GetPlayerStateZoneID(field)))) {
            u8 level = func_02010378(getKeyDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))));

            setup->fieldSituation.env.terrain = func_ov127_021f0dd8(InfinityDungeonGimmick_GetDungeon(field)) == 0x17 ? 0x12 : 0x13;
            setup->fieldSituation.env.bgType = level;
            setup->unkDD_5 = TRUE;
            setup->unkDE_0 = TRUE;
        }
    }
    EventBattleCall_DecideEnvTrainer(trainerId, field, &work->encEffect, &work->setup->fieldSituation.bgm);
    func_ov036_021a2364(Field_GetEncountSystem(field));
    return event;
}

GameEvent *func_ov012_0216881c(GameSystem *gsys, Field *field, BtlSetup *setup) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBattleCall_Callback, sizeof(EventBattleCallWork));
    EventBattleCallWork *work = GameEvent_GetData(event);

    EventBattleCall_Setup(work, gsys, setup);
    work->unk14 = FALSE;
    work->unk18 = TRUE;
    work->unk20 = TRUE;
    work->encEffect = 9;
    func_ov036_021a2364(Field_GetEncountSystem(field));
    return event;
}

GameEvent *CreateTrialHouseBattleEvent(GameSystem *gsys, Field *field, BtlSetup *setup) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBattleCall_Callback, sizeof(EventBattleCallWork));
    EventBattleCallWork *work = GameEvent_GetData(event);

    EventBattleCall_Setup(work, gsys, setup);
    work->unk14 = FALSE;
    work->unk1C = TRUE;
    work->unk18 = TRUE;
    work->encEffect = 9;
    work->setup->fieldSituation.bgm = 0x46b;
    func_ov036_021a2364(Field_GetEncountSystem(field));
    return event;
}

GameEvent *func_ov012_021688ac(GameSystem *gsys, Field *field, BtlSetup *setup) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBattleCall_Callback, sizeof(EventBattleCallWork));
    EventBattleCallWork *work;

    setup->unkDD_5 = TRUE;
    setup->unkDE_0 = TRUE;
    work = GameEvent_GetData(event);
    EventBattleCall_Setup(work, gsys, setup);
    work->unk14 = TRUE;
    work->unk18 = FALSE;
    work->unk20 = FALSE;
    work->encEffect = 9;
    work->setup->fieldSituation.bgm = 0x46a;
    work->setup->fieldSituation.unk12 = 0x47d;
    func_ov036_021a2364(Field_GetEncountSystem(field));
    return event;
}

GameEvent *func_ov012_02168924(GameSystem *gsys, Field *field, BtlSetup *setup) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBattleCall_Callback, sizeof(EventBattleCallWork));
    EventBattleCallWork *work = GameEvent_GetData(event);

    EventBattleCall_Setup(work, gsys, setup);
    work->unk14 = TRUE;
    work->unk18 = TRUE;
    work->unk20 = TRUE;
    work->encEffect = 0x1c;
    func_ov036_021a2364(Field_GetEncountSystem(field));
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
    work->unk14 = TRUE;
    EventBattleCall_DecideEnvWild(SPECIES_PURRLOIN, 0, 5, FALSE, field, &work->encEffect,
                                  &work->setup->fieldSituation.bgm);
    work->encEffect = 0;
    return event;
}

GameEvent *LoadTradedPokemonBattleStats(GameSystem *gsys, Field *field, u32 a2, u32 a3, u32 trainerId, u32 a5,
                                        u32 flags, u32 index) {
    void *block = GetTradedPokemonBlock(GSYS_GetGameData(gsys));
    GameEvent *event = EventTrainerBattleCall_Create(gsys, field, a2, a3, trainerId, a5, flags);

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
        PokeParty_SetParam(pkm, PKM_PARAM_NICKNAME_RAW, (u32)TradedPokemonSave_GetNickname(block, index));
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
    case 0:
        GameEvent_ChainNext(event, EventBattleBGMPlay_Create(gsys, wk->setup->fieldSituation.bgm));
        (*state)++;
        break;
    case 1:
        EncEff_StartEvent(Field_GetEncEff(field), event, wk->encEffect);
        (*state)++;
        break;
    case 2:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        (*state)++;
        break;
    case 3:
        FieldStatus_SetBusyFlag(GameData_GetFieldStatus(gameData), TRUE);
        CommPlayerSupport_Init(GameData_GetCommPlayerSupport(gameData));
        GSYS_QueueProcAsEvent(event, OVERLAY_ID(167), &data_ov167_021d6ce0, wk->setup);
        (*state)++;
        break;
    case 4:
        if (wk->setup->unkDD_3 == 0) {
            wk->battleReturnParam.setup = wk->setup;
            wk->battleReturnParam.gameData = gameData;
            GSYS_QueueProcAsEvent(event, OVERLAY_ID(166), &EVENT_BATTLE_RETURN_PROC_FUNCTIONS, &wk->battleReturnParam);
        }
        (*state)++;
        break;
    case 5: {
        u32 index;

        GFL_OvlLoad(OVERLAY_ID(337));
        DSProt_Init();
        FieldStatus_SetBusyFlag(GameData_GetFieldStatus(gameData), FALSE);
        CommPlayerSupport_EndBattle(GameData_GetCommPlayerSupport(gameData));
        index = OS_GetVBlankCount() & 1;
        data_ov337_02182440 = index;
        data_ov337_02182444[index] = func_ov012_02169170;
        data_ov337_02182444[index ^ 1] = func_ov012_02169154;
        (*state)++;
        break;
    }
    case 6: {
        u32 shift;
        u32 checksum;
        const u32 *code = (const u32 *)DSProt_CallRunChecks1;
        u32 index;

        for (shift = 0x25, checksum = 0; shift != 0; shift--) {
            checksum ^= (*code >> shift) | (*code << (32 - shift));
            code++;
        }
        if (checksum == 0x9f75a8d6) {
            DSProt_CallRunChecks1(&seed, wk);
        } else {
            data_ov337_02182444[data_ov337_02182440 ^ 1](&seed, wk);
        }
        EventBattleCall_SyncGameDataOnReturn(wk, gameData);
        if (wk->unk1C) {
            SyncTrialHouseWkStatsFromBattle(*GetTrialHouseWkPPtr(gameData), wk->setup);
        }
        (*state)++;
        index = OS_GetVBlankCount() & 1;
        data_ov337_02182440 = index;
        data_ov337_02182444[index] = func_ov012_02169198;
        data_ov337_02182444[index ^ 1] = func_ov012_02169180;
        break;
    }
    case 7: {
        u32 shift;
        u32 checksum;
        const u32 *code = (const u32 *)DSProt_CallRunChecks3;

        for (shift = 0x25, checksum = 0; shift != 0; shift--) {
            checksum ^= (*code >> shift) | (*code << (32 - shift));
            code++;
        }
        if (checksum == 0x9f75a8d6) {
            DSProt_CallRunChecks3(&seed, wk);
        } else {
            data_ov337_02182444[data_ov337_02182440 ^ 1](&seed, wk);
        }
        if (wk->unk18 == FALSE && EventBattleCall_IsResultDefeat(wk) == TRUE) {
            GameEvent_ChainNext(event, EventBGMFadeStop_Create(gsys, 30));
        } else {
            GameEvent_ChainNext(event, EventBGMFadePop_Create(gsys));
        }
        GFL_OvlUnload(OVERLAY_ID(337));
        (*state)++;
        break;
    }
    case 8:
        if (wk->unk18 == FALSE && EventBattleCall_IsResultDefeat(wk) == TRUE) {
            if (wk->unk14 == TRUE) {
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
    case 9:
        GameEvent_ChainNext(event, EventBGMFadeWait_Create(gsys));
        (*state)++;
        break;
    case 10:
        if (wk->unk14 == FALSE) {
            GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, field, 0, 0, 1, 0, 0));
        }
        (*state)++;
        break;
    case 11:
        EventBattleCall_ReleaseBtlSetup(wk);
        return GAMEEVENT_DONE;
    }
    if (seed % 0x1933 != 0) {
        GFL_VBlankTCBAdd(get_mt, NULL, 0x7f);
        g_AntiPiracyTaskCount++;
    }
    return GAMEEVENT_CONTINUE;
}

// The kind of beacon for the trainer: 0 for some classes, 1 for others and 2 for the rest
static u32 func_ov012_02168e98(u16 trainerId) {
    u16 trainerClass = TrainerData_GetParam(trainerId, 1);
    u32 group = GetTrainerClassBGMGroupId(trainerClass);
    u32 i;

    for (i = 0; i < 7; i++) {
        if (trainerClass == data_ov012_0216e5cc[i]) {
            return 2;
        }
    }
    if (group == 0xf || group == 7) {
        return 0;
    }
    if (group == 0) {
        return 1;
    }
    return 2;
}

static void func_ov012_02168edc(u16 trainerId, u32 kind) {
    data_ov012_0216dd24[func_ov012_02168e98(trainerId)][kind](trainerId);
}

static void func_ov012_02168efc(BtlSetup *setup, u32 kind) {
    u16 species = PokeParty_GetParam(PokeParty_GetPkm(BtlSetup_GetParty(setup, 1), 0), PKM_PARAM_SPECIES, NULL);

    switch (kind) {
    case 0:
        func_ov012_0215f958(species);
        break;
    case 1:
        func_ov012_0215f994(species);
        break;
    case 2:
        func_ov012_0215fe6c(species);
        break;
    }
}

static void func_ov012_02168f38(EventBattleCallWork *work) {
    BtlSetup *setup = work->setup;
    PokeParty *party = GameData_GetParty(work->gameData);
    PartyPkm *pkm = PokeParty_GetPkm(party, isEggInParty(party));

    if (setup->unkA8 != 0 && setup->unkA8 != 2) {
        u16 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        StrBuf *name = GFL_StrBufCreate(12, HEAPID_TAIL(4));
        u16 hp, maxHp;
        int i;

        PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME, name);
        hp = PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL);
        maxHp = PokeParty_GetParam(pkm, PKM_PARAM_MAX_HP, NULL);
        if (hp == 0) {
            func_ov012_0215ff84(species, name);
        } else if (hp * 2 <= maxHp) {
            func_ov012_0215febc(species, name);
        }
        for (i = 0; i < 4; i++) {
            if (PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL) != 0 &&
                PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP + i, NULL) == 0) {
                func_ov012_0215ff20(species, name);
                break;
            }
        }
        if (PokeParty_GetParam(pkm, PKM_PARAM_STATUS, NULL)) {
            func_ov012_0215ffe8(name);
        }
        GFL_StrBufFree(name);
    }
    switch (setup->battleType) {
    case 0:
        if (setup->unkA8 == 3) {
            func_ov012_02168efc(setup, 2);
        } else if (setup->unkA8 == 4) {
            func_ov012_02168efc(setup, 3);
        } else if (setup->unkA8 == 1) {
            func_ov012_02168efc(setup, 1);
        }
        break;
    case 1:
        if (setup->unkA8 == 1 && !(work->flags & 4)) {
            func_ov012_02168edc(setup->trainers[1]->trainerId, 1);
        }
        break;
    }
}

static void func_ov012_0216907c(EventBattleCallWork *work) {
    BtlSetup *setup = work->setup;
    GameRecords *records = GameData_GetRecords(GSYS_GetGameData(work->gsys));

    if (setup->battleType == 0 && setup->unkA8 == 4) {
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
    return IsBattleResultDefeat(work->setup->unkA8, work->setup->battleType);
}

static void EventBattleCall_SyncGameDataOnReturn(EventBattleCallWork *work, GameData *gameData) {
    GameData_SetLastBtlResult(gameData, work->setup->unkA8);
    func_ov012_02168f38(work);
    func_ov012_0216907c(work);
    ResetWeather(work->gsys, PlayerState_GetZoneID(GameData_GetPlayerState(gameData)));
}

static void EventBattleCall_Setup(EventBattleCallWork *work, GameSystem *gsys, BtlSetup *setup) {
    sys_memset32(0, work, sizeof(EventBattleCallWork));
    work->gsys = gsys;
    work->gameData = GSYS_GetGameData(gsys);
    work->setup = setup;
    work->unk14 = FALSE;
    work->unk18 = FALSE;
    work->unk1C = FALSE;
}

static void EventBattleCall_ReleaseBtlSetup(EventBattleCallWork *work) {
    if (work->unk20 == FALSE) {
        BtlSetup_Free(work->setup);
    }
}

// The fallbacks of the anti-piracy checks
static void *func_ov012_02169154(u32 *seed, void *work) {
    GFL_RandomMT();
    GFL_RandomMT();
    *seed += 0x3d1;
    return seed;
}

static void *func_ov012_02169170(u32 *seed, void *work) {
    GFL_RandomMT();
    GFL_RandomMT();
    return work;
}

static void *func_ov012_02169180(u32 *seed, void *work) {
    GFL_RandomMT();
    GFL_RandomMT();
    *seed += 0x9d;
    return seed;
}

static void *func_ov012_02169198(u32 *seed, void *work) {
    return work;
}
