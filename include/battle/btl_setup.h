#ifndef POKEBW2_BATTLE_BTL_SETUP_H
#define POKEBW2_BATTLE_BTL_SETUP_H

#include "types.h"
#include "battle/regulation.h"
#include "gfl/heap.h"
#include "nitro/math.h"
#include "save/bag.h"
#include "save/config.h"
#include "struct_decls.h"
#include "system/pms_data.h"

// The background, place and time of a battle, which the battle view copies as one struct
typedef struct {
    u32 bgType;
    u32 terrain;
    u8 weather;
    u8 season;
    u16 zoneId;
    u8 hour;
    u8 minute;
} BtlFieldEnv;

// The battle's surroundings, which GetFieldEffectData returns
struct BtlFieldSituation {
    BtlFieldEnv env;
    // The battle's music, and the music of a win
    u16 bgm;
    u16 winBgm;
    NetHandle *netHandle;
    u8 unk18;
    u8 unk19;
    // The kind of multi battle, which GetMultiMode returns: 0 for a battle of one player a side
    u8 multiMode;
    u8 unk1b;
};

// A trainer the battle is set up with
struct BtlSetupTrainer {
    u32 trainerId;
    u32 trainerClass;
    u32 aiFlags;
    u16 items[4];
    StrBuf *name;
    // The phrases the trainer says on winning and on losing
    PMSData winWords;
    PMSData loseWords;
};

// A trainer as a link battle sends it, with the name as characters
typedef struct {
    u32 trainerId;
    u32 trainerClass;
    u32 aiFlags;
    u16 items[4];
    u8 unk14[4];
    PMSData winWords;
    PMSData loseWords;
    u16 name[0x20];
    u32 nameLength;
} BtlCommTrainerData;

struct BtlSetup {
    u32 battleType;
    u32 battleStyle;
    BtlFieldSituation fieldSituation;
    // The parties of the four clients
    PokeParty *party[4];
    // The players' infos, by client
    PlayerInfo *playerInfo[4];
    u8 unk44[4];
    // The trainers of the four clients
    BtlSetupTrainer *trainers[4];
    // The trainers' names, by client
    StrBuf *trainerNames[4];
    // The opponent's lines when it loses and when it wins, for a trainer ID from 0x800 (a Pokémon World Tournament
    // entrant's, wbt_battle.c), which BtlString_LoadTrainerWinLoseMsg shows
    StrBuf *trainerLoseMsg;
    StrBuf *trainerWinMsg;
    GameData *gameData;
    Config *config;
    BagSave *bag;
    BagCursor *unk7C;
    PokeDexSave *pokedex;
    GameRecords *records;
    CommPlayerSupport *unk88;
    u16 unk8C;
    u16 unk8E;
    // What Regulation_GetShooterSettings fills in from a regulation, a RegulationUnkAC
    u8 unk90[7];
    u8 unk97;
    // The player's badges
    u8 unk98;
    u8 unk99[3];
    // BtlSetup_SetFlag sets these
    u32 flags;
    u16 unkA0;
    u16 unkA2;
    // The money the player wins, or negative loses
    u32 moneyChange;
    // How the battle ended, a BTL_RESULT_*
    u32 result;
    // The slot of the caught Pokémon in the second party, or 6 if none was caught
    u8 capturedSlot;
    u8 unkAD;
    u8 unkAE;
    u8 unkAF;
    void *unkB0;
    u32 unkB4;
    // The battle's random state, which the main module copies back
    MATHRandContext32 rand;
    u16 unkD0;
    u8 unkD2;
    u8 unkD3;
    u8 unkD4;
    u8 unkD5;
    u8 unkD6;
    u8 unkD7;
    u8 unkD8;
    u8 unkD9;
    u8 unkDA;
    u8 unkDB;
    u8 unkDC;
    u8 unkDD_0 : 1;
    // Whether a played recording was read to its end on every client (BtlMain_IsRecReadComplete)
    u8 recReadComplete : 1;
    u8 unkDD_2 : 1;
    // 1 for a Pokéstar Studios movie's battle and 2 for one loaded from its record (GetStudioMode)
    u8 studioMode : 2;
    u8 unkDD_5 : 1;
    u8 unkDD_6 : 1;
    u8 unkDD_7 : 1;
    // Whether the battle doesn't register the foes in the Pokédex (RegisterDexMon)
    u8 noPokedexRegistration : 1;
    u8 unkDE_1 : 7;
    u8 unkDF;
    u8 unkE0;
    // Whether each party slot's Pokémon appeared in the battle
    u8 appeared[6];
    // The party slots each client sends out
    u8 unkE7[4][6];
    u8 unkFF;
    // Each client's remaining HP, in percent of its party's total
    u32 unk100[4];
    // The choice made in each scene of a Pokéstar Studios movie, which its playback shows again
    u8 studioChoices[0x14];
    // How a Pokéstar Studios movie ended, which picks its ending music
    u32 studioEnding;
    // The size of a Pokéstar Studios movie's audience, which event_pokewood.c draws from the grade
    u8 audienceCount;
    // The player's grade at the studio (PokewoodSave_GetLevelOfPoints)
    u8 studioGrade;
    // One bit per member of the audience still in it
    u32 audience;
    // The Pokéstar Studios movie's score
    s32 studioScore;
    u32 unk134;
    u16 unk138;
    u16 unk13a;
    // Added to the defeated mon's level for the experience it gives
    s8 levelDiff;
};

BtlSetup *BtlSetup_Create(HeapID heapId);
u32 BtlSetup_CheckFlag(BtlSetup *setup, u32 flag);
void BtlSetup_Reset(BtlSetup *setup);
// Wild battles, against the party, alone or beside the trainer
void BtlSetup_SetWildNormal(BtlSetup *setup, GameData *gameData, PokeParty *party, BtlFieldStatus *status,
                            BOOL doubleBattle, HeapID heapId);
void BtlSetup_SetWildMulti(BtlSetup *setup, GameData *gameData, PokeParty *party, u32 trainerId, BtlFieldStatus *status,
                           HeapID heapId);
void BtlSetup_SetFlag(BtlSetup *setup, u32 flag);
void BtlSetup_Free(BtlSetup *setup);
PokeParty *BtlSetup_GetParty(BtlSetup *setup, u32 index);
void BtlSetup_SetNet1v1Double(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 a3, HeapID heapId);
void BtlSetup_SetNet1v1Single(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 a3, HeapID heapId);
void BtlSetup_SetNetRotation(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 a3, HeapID heapId);
void BtlSetup_SetNetTriple(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 a3, HeapID heapId);
void BtlSetup_SetNetMultiVsNet(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 a3, u8 a4, HeapID heapId);
// Battles against trainers, in the surroundings that SaveBtlFieldStatus saved
void BtlSetup_SetTrainer1v1Single(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 a3, HeapID heapId);
void BtlSetup_SetTrainer1v1Double(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 a3, HeapID heapId);
void BtlSetup_SetTrainer2v2(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 a3, u32 a4, u32 a5,
                            HeapID heapId);
void BtlSetup_SetTrainer1v2(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 trainerId, u32 trainerId2,
                            HeapID heapId);
void BtlSetup_SetTrainer3v3(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 a3, HeapID heapId);
void BtlSetup_SetTrainerRotation(BtlSetup *setup, GameData *gameData, BtlFieldStatus *status, u32 a3, HeapID heapId);
// A multi battle of two linked players against trainers
void BtlSetup_SetNetMultiVsAI(BtlSetup *setup, GameData *gameData, NetHandle *handle, u8 a3, u8 a4, u32 a5, u32 a6,
                              HeapID heapId);
void BtlSetup_PostProcessTrialHouse(BtlSetup *setup);
// The capture demonstration, between the two parties
void BtlSetup_SetCaptureDemo(BtlSetup *setup, GameData *gameData, PokeParty *party, PokeParty *enemyParty,
                             BtlFieldStatus *status, HeapID heapId);
// Changes the levels of the parties for the challenge mode of the zone, which the keys of Unova Link unlock
void adjustPkmLvForChallengeKeys(BtlSetup *setup, GameData *gameData, u16 zoneId);
// Frees what the setup holds and clears it
void BtlSetup_FreeContents(BtlSetup *setup);
void BtlSetup_SetParty(BtlSetup *setup, PokeParty *party, u32 a2);
void BtlSetup_ApplyRegulation(BtlSetup *setup, Regulation *regulation, HeapID heapId);
// Gives a battle's trainer a name buffer and empty words
BtlSetupTrainer *TrainerBattleSetup_Create(HeapID heapId);
// A field status with the default background
void ResetBtlFieldStatus(BtlFieldStatus *status);
void BtlSetup_InitForRecord(BtlSetup *setup, GameData *gameData, HeapID heapId);
void BtlSetup_FreeForRecord(BtlSetup *setup);
void BtlSetup_AllocRecBuffer(BtlSetup *setup, HeapID heapId);
void BattleRec_Init(HeapID heapId);
void freeVSPlayerBlkClearPtr(void);

#endif // POKEBW2_BATTLE_BTL_SETUP_H
