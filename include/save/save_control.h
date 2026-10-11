#ifndef POKEBW2_SAVE_SAVE_CONTROL_H
#define POKEBW2_SAVE_SAVE_CONTROL_H

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/rtc.h"
#include "save/player_savedata.h"
#include "save/playtime.h"
#include "save/timesig_savedata.h"
#include "save/trainer_data_savedata.h"
#include "struct_decls.h"

SaveControl *SaveControl_GetInstance(void);
// Frees the save data and its control
void SaveControl_Free(void);
EncountSave *SaveControl_GetEncountSave(SaveControl *save);
u32 func_02007560(SaveControl *save, u32 block, u32 heapId, void *buffer, u32 size);
// The save data Game Sync uploads, and its size
void *func_02007454(SaveControl *save, u32 *size);
// The sizes of a downloaded C-Gear skin and Pokédex skin, and a Pokédex skin block's valid flag
u32 func_0200ce50(void);
u32 func_0200f164(void);
void func_0200f194(void *block, u32 valid);
// Reads the downloaded musical's save block (6) into a new work, its archive and the archive's size, and frees it
void *func_0200cca0(GameData *gameData, HeapID heapId);
void func_0200cd10(void *work);
void *func_0200ce44(void *work);
u32 func_0200ce48(void *work);
// Imports a downloaded musical, a step a frame until it returns TRUE
void *func_0200cd34(GameData *gameData, void *data, u32 size, HeapID heapId);
BOOL func_0200cd64(void *work);
void *getAddressOfExtraSaveBlk(SaveControl *save, u32 block, u32 arg2);
void freeIntermediateSaveExtraBlksAfterLoad2(SaveControl *save, u32 block);
u32 SaveControl_GetStatus(SaveControl *save);
void func_020074b8(SaveControl *save, u32 *a1, u32 *a2);
// The bytes written so far of the save in progress, func_0203b080 of the save's SaveData
u32 func_0200743c(SaveControl *save);
u32 func_02007464(SaveControl *save);
void func_0200749c(SaveControl *save);
void func_02007324(SaveControl *save);
TrainerGameInfoSave *getTrainerGameInfoAddress(SaveControl *save);
u16 func_0200c96c(TrainerGameInfoSave *info);
void func_0200c974(TrainerGameInfoSave *info, u32 value);
void *func_020114f0(SaveControl *save);
const u16 *func_0200c93c(TrainerGameInfoSave *info);
void func_0200c940(TrainerGameInfoSave *info, StrBuf *name);
const u16 *func_0200c954(TrainerGameInfoSave *info);
void func_0200c958(TrainerGameInfoSave *info, StrBuf *name);
void func_020114fc(void *saveBlock, StrBuf *name);
const u16 *func_0201150c(void *saveBlock);
u16 func_0200ca7c(TrainerGameInfoSave *info);
u16 func_0200ca8c(TrainerGameInfoSave *info, u8 index);
// Adds to a survey question's count, up to 0xffff
void func_0200cab4(TrainerGameInfoSave *info, u8 index, u16 count);
// Poké Transfer's high score, 28 bits
u32 TrainerGameInfo_GetPalParkHighScore(TrainerGameInfoSave *info);
void TrainerGameInfo_SetPalParkHighScore(TrainerGameInfoSave *info, u32 score);
// How the last Poké Transfer session ended, 4 bits, which the Poké Transfer Lab's scripts react to
u8 TrainerGameInfo_GetPalParkResult(TrainerGameInfoSave *info);
void TrainerGameInfo_SetPalParkResult(TrainerGameInfoSave *info, u8 result);
u32 getCash(TrainerGameInfoSave *info);
void addCashToTotal(TrainerGameInfoSave *info, u32 amount);
// Stops at 0
void subCashFromTotal(TrainerGameInfoSave *info, u32 amount);

// The area NPC data, which keeps the symbol encounters of the Entree Forest, encrypted
AreaNPCSave *getAreaNPCData(SaveControl *save);
void decryptNpcData(AreaNPCSave *npcData);
void func_0200e904(AreaNPCSave *npcData);
u16 func_0200e9fc(AreaNPCSave *npcData, u32 kind);
u32 func_0200ea1c(u32 index);
void func_0200ea24(AreaNPCSave *npcData, u32 index);
// Whether the Entree Forest Pokémon, packed as EntreeForestPokemon is, is valid, checking its form when check is set
BOOL func_0200eb54(SaveControl *save, u32 *pokemon, HeapID heapId, BOOL check);
void func_0200eb14(AreaNPCSave *npcData, u16 index);
// Puts a Pokémon from the Dream World in the Entree Forest: its species, a2, sex and form, a5 below 9, and size 2 for
// a big overworld sprite or 3
void func_0200ea40(AreaNPCSave *npcData, u16 species, u16 a2, u8 sex, u32 form, u32 a5, u32 size);

// Returns a pointer to one of the save's blocks
void *SaveControl_GetBlockPtr(SaveControl *save, u32 block);
u32 func_0200bcf8(SaveControl *save, u32 a1, void *a2, u32 a3);
u32 func_0200be50(GameData *gameData, u32 a1, u32 a2, u32 a3, u32 a4, u16 *a5, u16 *a6);
u32 func_0200c1d0(u8 a0);
void func_0200c1f0(void);
void func_0200c200(void);
BOOL func_0200ae58(MusicalSave *musical);
// The musical save's accessors, by what they return
// A prop a Pokémon wears in a musical shot, by the slot it is worn on
typedef struct {
    u16 itemId;
    s16 unk2;
    u8 unk4;
    u8 unk5[3];
} MusicalShotEquip;

typedef struct {
    u16 species;
    u16 sex : 2;
    u16 rare : 1;
    u16 form : 5;
    u32 personality;
    u16 name[8];
    MusicalShotEquip equips[8];
} MusicalShotPoke;

// The photo of a musical's finale, which the musical event fills in and the save keeps. A month of 0 means no photo
typedef struct {
    u32 unk0_0 : 5;
    // A bit for each Pokémon that got the most points
    u32 tops : 4;
    u32 year : 7;
    u32 month : 5;
    u32 day : 6;
    // The player's Pokémon
    u32 player : 2;
    u32 unk0_29 : 3;
    MusicalShotPoke pokes[4];
    u16 title[0x25];
    u8 unk1AE;
    u8 unk1AF;
} MusicalShot;

typedef struct {
    u8 unk0;
    u8 unk1;
    u16 unk2;
} MusicalSaveUnk1E0;

typedef struct {
    u8 unk0;
    u8 unk1[5];
} MusicalSaveUnk1B0;

MusicalShot *func_0200ad5c(MusicalSave *musical);
MusicalSaveUnk1B0 *func_0200ad44(MusicalSave *musical);
// Whether a musical photo is saved
BOOL func_0200ad4c(MusicalSave *musical);
void func_0200add8(MusicalSave *musical, u8 prop);
MusicalSaveUnk1E0 *func_0200ae6c(MusicalSave *musical, u8 index);
u16 func_0200ae78(MusicalSave *musical);
u16 func_0200ae9c(MusicalSave *musical);
u8 func_0200aebc(MusicalSave *musical, u8 index);
u8 func_0200aed4(MusicalSave *musical);
u8 func_0200aee4(MusicalSave *musical);
void func_0200ae84(MusicalSave *musical);
void func_0200aea4(MusicalSave *musical);
void func_0200aec8(MusicalSave *musical, u8 index, u8 value);
void func_0200aedc(MusicalSave *musical, u8 value);
void func_0200af1c(MusicalSave *musical, u16 value);
u16 func_0200af38(MusicalSave *musical);
// Whether the downloaded props were added, which func_0200af64 sets
u8 func_0200af5c(MusicalSave *musical);
void func_0200af64(MusicalSave *musical, u8 value);
void *func_0200afbc(SaveControl *save);
// The block of func_0200afbc: 30 entries of 0x1c bytes, each starting with a name. The number of entries in use, the
// entry of a birthday and date, whether an entry is flagged, and an entry
u32 func_0200afc8(void *data);
u32 func_0200b05c(void *data, u8 month, u8 day, RTCDate *date);
BOOL func_0200b014(void *data, u32 index);
const u16 *func_0200afe4(void *data, u32 index);
void func_0200b220(void *data);
MusicalSave *getAddressOfMusicalDataInfo(SaveControl *save);
void func_0200aef0(MusicalSave *musical, u8 value);
u8 func_0200aefc(MusicalSave *musical);
// A name of 0x26 characters
const u16 *func_0200af14(MusicalSave *musical);
BOOL func_0200ad60(MusicalSave *musical, u8 prop);

// Save block 0x2b, which swan calls ReshZek: the Reshiram or Zekrom fused with Kyurem
void *getReshZekBlkAddress(SaveControl *save);
PartyPkm *func_0200afa8(void *reshZek);
void func_0200afac(void *reshZek, PartyPkm *pkm);

// Save block 0x45, which swan calls the key data. It keeps the Black Tower's and White Treehollow's progress, and the
// Trainers there that have been defeated (CheckTrainerAlreadyDefeated)
KeyDataSave *getKeyDataBlkAddress(SaveControl *save);
void func_0201024c(KeyDataSave *keyData);
u8 func_02010274(KeyDataSave *keyData, u32 a1);
u32 func_02010284(KeyDataSave *keyData);
BOOL func_02010288(KeyDataSave *keyData, u32 a1, u32 bit);
u16 func_020102a4(KeyDataSave *keyData);
u8 func_020102d0(KeyDataSave *keyData);
u8 func_020102d4(KeyDataSave *keyData);
u8 func_020102ec(KeyDataSave *keyData);
void func_020102f0(KeyDataSave *keyData, u8 value, u32 a2);
void func_02010300(KeyDataSave *keyData, u32 value);
void func_02010304(KeyDataSave *keyData, u32 a1, u32 bit, BOOL set);
void func_02010334(KeyDataSave *keyData, u8 value);
void func_02010338(KeyDataSave *keyData, u8 value);
void func_0201033c(KeyDataSave *keyData, u8 value);
BOOL func_02010340(KeyDataSave *keyData, u32 bit);
void func_02010354(KeyDataSave *keyData, u32 bit, BOOL set);
u8 func_02010378(KeyDataSave *keyData);
u8 func_020103a0(int a0);
u8 func_020103c4(u8 a0);
u32 func_020103e8(KeyDataSave *keyData);
void func_020103ec(KeyDataSave *keyData, u32 value);

// Save block 0x42, which keeps the rival's name
RivalDataSave *getHollow_RivalData(SaveControl *save);
// Save block 0x42, and setting a byte of it
void *getHollow_RivalBlk(SaveControl *save);
void func_0200ff78(void *rival, u32 value);
// The rival's hollow data: a value of an index, a flag of an index, and the hollows visited
void func_0200ff50(void *block, u32 index, u16 value);
u16 func_0200ff54(void *block, u16 index);
void func_0200ff58(void *block, u16 index, u32 value);
void func_0200ff6c(void *block);
u16 func_0200ff74(void *block);
void func_0200ff94(void *block, u32 value);
void func_0200ffb0(void *block, u16 value);
void func_0200ffb8(void *block, u32 value);
BOOL func_0200ffd4(void *block, int index);
// The C-Gear's save block (getCGearDataBlkAddress)
// Copies the C-Gear's record at index, 4 u16
void func_0200f700(RivalDataSave *rivalData, u32 id);
void copyRivalNameIntoHollowBlock(RivalDataSave *data, const u16 *name);
const u16 *getPtrToRivalName(RivalDataSave *data);
u32 func_0200ca64(TrainerGameInfoSave *info);
void func_0200ca6c(TrainerGameInfoSave *info, u32 value);
u32 func_0200ca74(TrainerGameInfoSave *info);
void func_0200ca78(TrainerGameInfoSave *info, u32 value);
u32 getBadgeCount(TrainerGameInfoSave *info);
// Two bits for each of the start menu's items that are added later: func_0200ca38 sets bits, and func_0200ca50
// returns them. 1 marks the item as new
void func_0200ca38(TrainerGameInfoSave *info, u32 item, u32 bits);
u8 func_0200ca50(TrainerGameInfoSave *info, u32 item);
BOOL SaveControl_IsDataAlreadyPresent(SaveControl *save);
// Whether the save is of another game, which the report may not overwrite
BOOL func_0200746c(SaveControl *save);

// Saving a step at a time: func_020073ac starts, func_020073c4 continues and returns the status, and func_02007424
// cancels
void func_020073ac(SaveControl *save);
u32 func_020073c4(SaveControl *save);
void func_02007424(SaveControl *save);
// Four flags of the save control, at 4 to 7
u8 func_0200748c(SaveControl *save);
void func_02007490(SaveControl *save, u32 value);
u8 func_02007494(SaveControl *save);
void func_02007498(SaveControl *save, u32 value);
u8 func_020074dc(SaveControl *save);
void func_020074e0(SaveControl *save, u32 value);
u8 func_020074e4(SaveControl *save);
void func_020074e8(SaveControl *save, u32 value);
// Used to delete the save data: func_020074ec tells whether a block is in the save, and func_020076a4 clears it
void func_020074ac(SaveControl *save);
BOOL func_020074ec(SaveControl *save, u32 block, HeapID heapId);
void func_020076a4(SaveControl *save, u32 block, HeapID heapId);
void freeIntermediateSaveExtraBlksAfterLoad(SaveControl *save, u32 block);
void func_02011558(HeapID heapId);

DreamRadarSave *GetDreamRadarSaveBlock(SaveControl *save);
JoinAvenueSave *SaveControl_GetJoinAvenue(SaveControl *save);
EventWork *getConstDataBlock(SaveControl *save);
PokeDexSave *getPokedexSaveAddress(SaveControl *save);
// A byte of this block, at 7, tells the start menu whether to ask about the C-Gear
void *func_02009918(SaveControl *save);
// Mark the downloaded C-Gear skin as there, and keep its CRC
void func_020098cc(void *block, u8 valid);
void func_020098d4(void *block, u16 crc);
void func_020098bc(void *cgear, u8 value);
// The same block, from the game data
void *func_02009924(GameData *gameData);
u8 func_020098c0(void *a0);
PokeParty *SaveControl_GetPokePartySave(SaveControl *save);
WorldTradeData *SaveControl_GetWorldTradeData(SaveControl *save);
DreamWorldSave *getDreamWorldStuffAddress(SaveControl *save);
HighLinkSave *getHighLinkBlockAddress(SaveControl *save);
void *func_02010dec(SaveControl *save);
// The Funfest mission records of func_02010dec's block: missions hosted, joined and completed, the most participants
// and the best score
u16 func_02010df8(void *a0);
u16 func_02010e24(void *a0);
// Count one more mission hosted and joined, up to 9999, and return the new count
u16 FestRecords_AddHosted(void *records);
u16 FestRecords_AddJoined(void *records);
u16 func_02010e50(void *a0);
u8 func_02010e78(void *a0);
u16 func_02010e94(void *a0);
KeyInfoSave *getKeyInfoSaveBlk(SaveControl *save);
// Whether a key system key is unlocked
BOOL func_020104c4(KeyInfoSave *keyInfo, u32 key);
RecordSave *getRecordBlkAddress(SaveControl *save);
AdventureSave *getSaveAdventureDataBlk(SaveControl *save);
AdventureTime *getSaveAdventureTimeBlock(SaveControl *save);
TrainerCardSave *getTrainerCardDataBlkAddress(GameData *gameData);
// The same block as GameData_GetRecords
GameRecords *getTrainerCardInfoBlkAddress(SaveControl *save);
UnityTowerSurveySave *getUnityTower_SurveySaveBlkAddrress(SaveControl *save);

#endif // POKEBW2_SAVE_SAVE_CONTROL_H
