#include "types.h"
#include "battle/btl_setup.h"
#include "battle/regulation.h"
#include "battle/trainer_data.h"
#include "constants/flags.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "field/field_event.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/box.h"
#include "save/config.h"
#include "save/event_work.h"
#include "save/key_info.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "save/trainer_data_savedata.h"
#include "system/game_data.h"
#include "system/pms_data.h"

// The setup of a battle: its kind, parties, trainers and surroundings. The file name is the ROM's string; the function
// names are swan's where they have one.

static void BtlFieldEnv_Init(BtlFieldEnv *env);
static void TrainerBattleSetup_Free(BtlSetupTrainer *trainer);
static void BtlSetup_LoadTrainer(BtlSetup *setup, GameData *gameData, u32 client, PokeParty **party, int trainerId,
                                 HeapID heapId);
static void BtlSetup_LoadPlayerParty(BtlSetup *setup, GameData *gameData, HeapID heapId);
static void BtlSetup_LoadGameData(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status);
static void BtlSetup_LoadTrainerBase(BtlSetup *setup, GameData *gameData, u32 style, BtlFieldStatus *status,
                                     HeapID heapId);
static void BtlSetup_SetNetImpl(BtlSetup *setup, GameData *gameData, u32 style, u8 multiMode, NetHandle *handle,
                                u8 a5, u8 a6, HeapID heapId);

// The clients whose parties the difficulty changes the levels of
static const u32 DIFFICULTY_ADJUST_PARTIES[] = { 1, 3 };

BtlSetup *BtlSetup_Create(HeapID heapId) {
    BtlSetup *setup = GFL_HeapAllocate(heapId, sizeof(BtlSetup), FALSE, "btl_setup.c", 52);

    BtlSetup_Reset(setup);
    return setup;
}

void BtlSetup_Free(BtlSetup *setup) {
    BtlSetup_FreeContents(setup);
    GFL_HeapFree(setup);
}

void BtlSetup_Reset(BtlSetup *setup) {
    sys_memset(setup, 0, sizeof(BtlSetup));
    BtlFieldEnv_Init(&setup->fieldSituation.env);
}

void BtlSetup_FreeContents(BtlSetup *setup) {
    int i;

    for (i = 0; i < 4; i++) {
        if (setup->party[i] != NULL) {
            GFL_HeapFree(setup->party[i]);
        }
        if (setup->trainers[i] != NULL) {
            TrainerBattleSetup_Free(setup->trainers[i]);
        }
        if (i != 0 && setup->unk34[i] != NULL) {
            GFL_HeapFree(setup->unk34[i]);
        }
    }
    if (setup->unkB0 != NULL) {
        GFL_HeapFree(setup->unkB0);
    }
    sys_memset(setup, 0, sizeof(BtlSetup));
}

void BtlSetup_SetParty(BtlSetup *setup, PokeParty *party, u32 a2) {
    PokeParty_Copy(party, setup->party[a2]);
}

PokeParty *BtlSetup_GetParty(BtlSetup *setup, u32 index) {
    return setup->party[index];
}

void BtlSetup_SetFlag(BtlSetup *setup, u32 flag) {
    setup->flags |= flag;
}

u32 BtlSetup_CheckFlag(BtlSetup *setup, u32 flag) {
    return setup->flags & flag;
}

void BtlSetup_ApplyRegulation(BtlSetup *setup, Regulation *regulation, HeapID heapId) {
    int i;

    setup->unk8E = Regulation_GetParam(regulation, REGULATION_PARAM_16) * 60;
    setup->unk8C = Regulation_GetParam(regulation, REGULATION_PARAM_15) * 60;
    func_0200b760(regulation, (RegulationUnkAC *)setup->unk90);
    switch (Regulation_GetParam(regulation, REGULATION_PARAM_18)) {
    case 0:
        break;
    case 1:
        setup->flags |= 0x800;
        break;
    case 2:
        setup->flags |= 0x400;
        break;
    case 3:
        setup->flags |= 0x400;
        setup->flags |= 0x800;
        break;
    }
    for (i = 0; i < 4; i++) {
        if (setup->party[i] != NULL) {
            PokeRegulation_ApplyNicknamesToParty(regulation, setup->party[i], heapId);
            PokeRegulation_ApplyLevelToParty(regulation, setup->party[i]);
        }
    }
}

void ResetBtlFieldStatus(BtlFieldStatus *status) {
    status->bgId = 9;
    status->terrain = 0;
    status->weather = 0;
    status->zoneId = 0;
    status->hour = 0;
    status->minute = 0;
    status->season = 0;
}

static void BtlFieldEnv_Init(BtlFieldEnv *env) {
    env->bgType = 0;
    env->terrain = 0;
    env->season = 0;
    env->weather = 0;
    env->zoneId = 0;
    env->hour = 10;
    env->minute = 0;
}

BtlSetupTrainer *TrainerBattleSetup_Create(HeapID heapId) {
    BtlSetupTrainer *trainer = GFL_HeapAllocate(heapId, sizeof(BtlSetupTrainer), TRUE, "btl_setup.c", 242);

    trainer->name = GFL_StrBufCreate(16, heapId);
    trainer->trainerId = 0;
    PMSData_Clear(&trainer->unk18);
    PMSData_Clear(&trainer->unk20);
    return trainer;
}

static void TrainerBattleSetup_Free(BtlSetupTrainer *trainer) {
    GFL_StrBufFree(trainer->name);
    GFL_HeapFree(trainer);
}

// Loads a trainer and its party for a client, and picks the music of a win by the trainer's class
static void BtlSetup_LoadTrainer(BtlSetup *setup, GameData *gameData, u32 client, PokeParty **party, int trainerId,
                                 HeapID heapId) {
    if (*party == NULL) {
        *party = PokeParty_Create(heapId);
    }
    if (setup->trainers[client] == NULL) {
        setup->trainers[client] = TrainerBattleSetup_Create(heapId);
    }
    if (trainerId != 0) {
        TrainerUtil_LoadTrainer(gameData, trainerId, setup->trainers[client], heapId);
        TrainerUtil_LoadParty(trainerId, *party, heapId);
        TransformVsPokePartyBySeason(gameData, *party, GameData_GetSeason(gameData));
        switch (GetTrainerClassBGMGroupId(setup->trainers[client]->trainerClass)) {
        case 0:
        case 1:
            setup->fieldSituation.unk12 = SEQ_BGM_WIN3;
            break;
        case 5:
        case 9:
            setup->fieldSituation.unk12 = SEQ_BGM_WIN4;
            break;
        case 8:
        case 14:
            setup->fieldSituation.unk12 = SEQ_BGM_WIN5;
            break;
        case 7:
        case 10:
            setup->fieldSituation.unk12 = SEQ_BGM_WIN6;
            break;
        default:
            setup->fieldSituation.unk12 = SEQ_BGM_WIN2;
            break;
        }
    }
}

static void BtlSetup_LoadPlayerParty(BtlSetup *setup, GameData *gameData, HeapID heapId) {
    GameData_GetPlayerState(gameData);
    setup->party[0] = PokeParty_Create(heapId);
    PokeParty_Copy(GameData_GetParty(gameData), setup->party[0]);
}

// What every battle takes from the game data, with the music of a wild battle
static void BtlSetup_LoadGameData(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status) {
    u32 i;
    SaveControl *save;
    TrainerCardSave *card;

    setup->fieldSituation.netHandle = NULL;
    setup->unkA0 = 0xffff;
    setup->fieldSituation.unk18 = 0;
    setup->fieldSituation.unk19 = 0;
    setup->fieldSituation.unk1a = 0;
    setup->unkB0 = NULL;
    setup->fieldSituation.unk1b = 0;
    setup->unkDD_0 = FALSE;
    setup->gameData = gameData;
    setup->unkDD_1 = FALSE;
    for (i = 0; i < 4; i++) {
        setup->party[i] = NULL;
        setup->unk34[i] = NULL;
    }
    for (i = 0; i < 6; i++) {
        setup->unkE1[i] = 0;
    }
    setup->unk97 = 1;
    for (i = 0; i < 7; i++) {
        setup->unk90[i] = 0;
    }
    setup->unk34[0] = GetGameDataPlayerInfo(gameData);
    setup->bag = GameData_GetBag(gameData);
    setup->unk7C = func_0201734c(gameData);
    setup->pokedex = GameData_GetPokedex(gameData);
    setup->unk88 = GameData_GetCommPlayerSupport(gameData);
    setup->records = GameData_GetRecords(gameData);
    save = GameData_GetSaveControl(gameData);
    card = getTrainerCardData_wrapper(save);
    setup->config = &getTrainerDataBlkAddress(save)->config;
    setup->unk98 = getBadgeCount(card);
    sys_memcpy(status, &setup->fieldSituation.env, sizeof(BtlFieldEnv));
    setup->fieldSituation.bgm = SEQ_BGM_VS_NORAPOKE;
    setup->fieldSituation.unk12 = SEQ_BGM_WIN1;
    setup->unkA8 = 1;
    if (EventWork_FlagGet(GameData_GetEventWork(gameData), EVENT_FLAG_0x096b)) {
        BtlSetup_SetFlag(setup, 2);
    }
}

void BtlSetup_SetWildNormal(BtlSetup *setup, GameData *gameData, PokeParty *party, BtlFieldStatus *status,
                            BOOL doubleBattle, HeapID heapId) {
    BtlSetup_Reset(setup);
    BtlSetup_LoadGameData(setup, gameData, status);
    BtlSetup_LoadPlayerParty(setup, gameData, heapId);
    setup->party[1] = PokeParty_Create(heapId);
    if (party != NULL) {
        PokeParty_Copy(party, setup->party[1]);
    }
    setup->battleStyle = doubleBattle;
    setup->battleType = 0;
    // The party and the boxes are full
    if (PokeParty_GetPkmCount(setup->party[0]) == PokeParty_GetCapacity(setup->party[0])) {
        BoxSaveAccessor *boxes = GameData_GetBoxSaveAccessor(gameData);
        int box = 0;
        int slot = 0;

        if (!BoxSaveAccessor_GetNextFreeBoxSlot(boxes, &box, &slot)) {
            BtlSetup_SetFlag(setup, 0x40);
        }
    }
}

void BtlSetup_SetWildMulti(BtlSetup *setup, GameData *gameData, PokeParty *party, u32 trainerId, BtlFieldStatus *status,
                           HeapID heapId) {
    BtlSetup_Reset(setup);
    BtlSetup_LoadGameData(setup, gameData, status);
    BtlSetup_LoadPlayerParty(setup, gameData, heapId);
    BtlSetup_LoadTrainer(setup, gameData, 2, &setup->party[2], trainerId, heapId);
    setup->party[1] = PokeParty_Create(heapId);
    if (party != NULL) {
        PokeParty_Copy(party, setup->party[1]);
    }
    setup->battleStyle = 1;
    setup->fieldSituation.unk1a = 5;
    setup->battleType = 0;
    if (PokeParty_GetPkmCount(setup->party[0]) == PokeParty_GetCapacity(setup->party[0])) {
        BoxSaveAccessor *boxes = GameData_GetBoxSaveAccessor(gameData);
        int box = 0;
        int slot = 0;

        if (!BoxSaveAccessor_GetNextFreeBoxSlot(boxes, &box, &slot)) {
            BtlSetup_SetFlag(setup, 0x40);
        }
    }
}

// What every battle against trainers starts from
static void BtlSetup_LoadTrainerBase(BtlSetup *setup, GameData *gameData, u32 style, BtlFieldStatus *status,
                                     HeapID heapId) {
    BtlSetup_Reset(setup);
    BtlSetup_LoadGameData(setup, gameData, status);
    BtlSetup_LoadPlayerParty(setup, gameData, heapId);
    setup->battleType = 1;
    setup->battleStyle = style;
    setup->fieldSituation.unk12 = SEQ_BGM_WIN2;
}

// What every link battle starts from, with a party and a player's info for each other client
static void BtlSetup_SetNetImpl(BtlSetup *setup, GameData *gameData, u32 style, u8 multiMode, NetHandle *handle,
                                u8 a5, u8 a6, HeapID heapId) {
    BtlFieldStatus status;
    u32 i;

    BtlSetup_Reset(setup);
    ResetBtlFieldStatus(&status);
    status.bgId = 9;
    status.terrain = 1;
    BtlSetup_LoadGameData(setup, gameData, &status);
    BtlSetup_LoadPlayerParty(setup, gameData, heapId);
    setup->battleType = 3;
    setup->battleStyle = style;
    setup->fieldSituation.netHandle = handle;
    setup->fieldSituation.unk18 = a5;
    setup->fieldSituation.unk1a = multiMode;
    setup->fieldSituation.unk19 = a6;
    setup->unkA0 = 0xff;
    setup->fieldSituation.unk12 = SEQ_BGM_WIN2;
    for (i = 0; i < 4; i++) {
        if (i != 0) {
            if (setup->unk34[i] == NULL) {
                setup->unk34[i] = func_02008b0c(heapId);
            }
            if (setup->party[i] == NULL) {
                setup->party[i] = PokeParty_Create(heapId);
            }
        }
    }
}

void BtlSetup_SetCaptureDemo(BtlSetup *setup, GameData *gameData, PokeParty *party, PokeParty *enemyParty,
                             BtlFieldStatus *status, HeapID heapId) {
    BtlSetup_Reset(setup);
    BtlSetup_LoadGameData(setup, gameData, status);
    BtlSetup_LoadPlayerParty(setup, gameData, heapId);
    if (party != NULL) {
        PokeParty_Copy(party, setup->party[0]);
    }
    setup->party[1] = PokeParty_Create(heapId);
    if (enemyParty != NULL) {
        PokeParty_Copy(enemyParty, setup->party[1]);
    }
    setup->battleType = 4;
    setup->battleStyle = 0;
}

void BtlSetup_SetTrainer1v1Single(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 a3, HeapID heapId) {
    BtlSetup_LoadTrainerBase(setup, gameData, 0, status, heapId);
    BtlSetup_LoadTrainer(setup, gameData, 1, &setup->party[1], a3, heapId);
}

void BtlSetup_SetTrainer1v1Double(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 a3, HeapID heapId) {
    BtlSetup_LoadTrainerBase(setup, gameData, 1, status, heapId);
    BtlSetup_LoadTrainer(setup, gameData, 1, &setup->party[1], a3, heapId);
}

void BtlSetup_SetTrainer3v3(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 a3, HeapID heapId) {
    BtlSetup_LoadTrainerBase(setup, gameData, 2, status, heapId);
    BtlSetup_LoadTrainer(setup, gameData, 1, &setup->party[1], a3, heapId);
}

void BtlSetup_SetTrainerRotation(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 a3, HeapID heapId) {
    BtlSetup_LoadTrainerBase(setup, gameData, 3, status, heapId);
    BtlSetup_LoadTrainer(setup, gameData, 1, &setup->party[1], a3, heapId);
}

void BtlSetup_SetTrainer1v2(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 trainerId, u32 trainerId2,
                            HeapID heapId) {
    BtlSetup_LoadTrainerBase(setup, gameData, 1, status, heapId);
    BtlSetup_LoadTrainer(setup, gameData, 1, &setup->party[1], trainerId, heapId);
    BtlSetup_LoadTrainer(setup, gameData, 3, &setup->party[3], trainerId2, heapId);
    setup->fieldSituation.unk1a = 4;
}

// a3 is the partner's trainer, a4 and a5 the opponents'
void BtlSetup_SetTrainer2v2(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 a3, u32 a4, u32 a5,
                            HeapID heapId) {
    BtlSetup_LoadTrainerBase(setup, gameData, 1, status, heapId);
    BtlSetup_LoadTrainer(setup, gameData, 2, &setup->party[2], a3, heapId);
    BtlSetup_LoadTrainer(setup, gameData, 1, &setup->party[1], a4, heapId);
    BtlSetup_LoadTrainer(setup, gameData, 3, &setup->party[3], a5, heapId);
    setup->fieldSituation.unk1a = 3;
}

void BtlSetup_SetNet1v1Single(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 a3, HeapID heapId) {
    BtlSetup_SetNetImpl(setup, gameData, 0, 0, handle, a3, func_02042a6c(handle), heapId);
}

void BtlSetup_SetNet1v1Double(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 a3, HeapID heapId) {
    BtlSetup_SetNetImpl(setup, gameData, 1, 0, handle, a3, func_02042a6c(handle), heapId);
}

void BtlSetup_SetNetMultiVsNet(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 a3, u8 a4, HeapID heapId) {
    BtlSetup_SetNetImpl(setup, gameData, 1, 1, handle, a3, a4, heapId);
}

void BtlSetup_SetNetMultiVsAI(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 a3, u8 a4, u32 a5, u32 a6,
                              HeapID heapId) {
    BtlSetup_SetNetImpl(setup, gameData, 1, 2, handle, a3, a4, heapId);
    setup->battleType = 1;
    BtlSetup_LoadTrainer(setup, gameData, 1, &setup->party[1], a5, heapId);
    BtlSetup_LoadTrainer(setup, gameData, 3, &setup->party[3], a6, heapId);
}

void BtlSetup_SetNetTriple(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 a3, HeapID heapId) {
    BtlSetup_SetNetImpl(setup, gameData, 2, 0, handle, a3, func_02042a6c(handle), heapId);
}

void BtlSetup_SetNetRotation(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 a3, HeapID heapId) {
    BtlSetup_SetNetImpl(setup, gameData, 3, 0, handle, a3, func_02042a6c(handle), heapId);
}

// A setup with a party, a player's info and a trainer for every client, and its own copy of the options
void BtlSetup_InitForRecord(BtlSetup *setup, GameData *gameData, HeapID heapId) {
    u32 i;
    TrainerDataSave *trainerData;

    sys_memset(setup, 0, sizeof(BtlSetup));
    for (i = 0; i < 4; i++) {
        setup->party[i] = PokeParty_Create(heapId);
        setup->unk34[i] = func_02008b0c(heapId);
        setup->trainers[i] = TrainerBattleSetup_Create(heapId);
    }
    setup->gameData = gameData;
    trainerData = getTrainerDataBlkAddress(GameData_GetSaveControl(gameData));
    setup->config = func_0200898c(heapId);
    initConfig(&trainerData->config, setup->config);
    setup->pokedex = GameData_GetPokedex(gameData);
    setup->fieldSituation.unk1b = 1;
    BtlSetup_AllocRecBuffer(setup, heapId);
}

void BtlSetup_FreeForRecord(BtlSetup *setup) {
    if (setup->config != NULL) {
        GFL_HeapFree(setup->config);
        setup->config = NULL;
    }
    BtlSetup_FreeContents(setup);
}

void adjustPkmLvForChallengeKeys(BtlSetup *setup, GameData *gameData, u16 zoneId) {
    u32 difficulty = GetGameDifficulty(getKeyInfoSaveBlk(GameData_GetSaveControl(gameData)));
    s16 adjust = GetZoneEasyChallengeModeLevelAdjust(zoneId);
    u32 i;
    int j;

    setup->levelDiff = 0;
    if (difficulty == GAME_DIFFICULTY_EASY) {
        adjust *= -1;
    }
    if (difficulty != GAME_DIFFICULTY_NORMAL) {
        setup->levelDiff = adjust;
        for (i = 0; i < NELEMS(DIFFICULTY_ADJUST_PARTIES); i++) {
            PokeParty *party = setup->party[DIFFICULTY_ADJUST_PARTIES[i]];

            if (party != NULL) {
                for (j = 0; j < PokeParty_GetPkmCount(party); j++) {
                    PartyPkm *pkm = PokeParty_GetPkm(party, j);
                    s16 level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);

                    level += adjust;

                    if (level <= 0) {
                        level = 1;
                    }
                    PokeParty_SetParam(pkm, PKM_PARAM_LEVEL, level);
                }
            }
        }
    }
}

// Makes a trainer battle a Battle Subway one, with its music
void BtlSetup_PostProcessTrialHouse(BtlSetup *setup) {
    if (setup->battleType == 1) {
        setup->battleType = 2;
        if (GetTrainerClassBGMGroupId(setup->trainers[1]->trainerClass) != 14) {
            setup->fieldSituation.bgm = SEQ_BGM_VS_SUBWAY_TRAINER;
            setup->fieldSituation.unk12 = SEQ_BGM_WIN2;
        } else {
            setup->fieldSituation.bgm = SEQ_BGM_VS_CHAMP;
            setup->fieldSituation.unk12 = SEQ_BGM_WIN5;
        }
    }
}

void BtlSetup_AllocRecBuffer(BtlSetup *setup, HeapID heapId) {
    if (setup->unkB0 == NULL) {
        setup->unkB0 = GFL_HeapAllocate(heapId, 0xc00, FALSE, "btl_setup.c", 1035);
    }
}
