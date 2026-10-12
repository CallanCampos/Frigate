#ifndef POKEBW2_SAVE_BATTLE_REC_H
#define POKEBW2_SAVE_BATTLE_REC_H

#include "types.h"
#include <stddef.h>
#include "battle/btl_setup.h"
#include "gfl/heap.h"
#include "nitro/math.h"
#include "save/gds_profile.h"
#include "save/player_info.h"
#include "struct_decls.h"
#include "system/pms_data.h"

// battle_rec.c: the battle video that is loaded, which the battle recorder plays and the extra save data keeps

// A Pokémon as a recording keeps it: the fields of its blocks that the battle uses, unencrypted
typedef struct {
    u32 pid;
    u16 partyDecrypted : 1;
    u16 boxDecrypted : 1;
    u16 badEgg : 1;
    u16 nature : 8;
    u16 nPoke : 1;
    u16 species;
    u16 item;
    u32 id;
    u32 exp;
    u8 friendship;
    u8 ability;
    u8 hpEV;
    u8 atkEV;
    u8 defEV;
    u8 speEV;
    u8 spaEV;
    u8 spdEV;
    u16 moves[4];
    u8 pp[4];
    u8 ppUps[4];
    u32 hpIV : 5;
    u32 atkIV : 5;
    u32 defIV : 5;
    u32 speIV : 5;
    u32 spaIV : 5;
    u32 spdIV : 5;
    u32 isEgg : 1;
    u32 isNicknamed : 1;
    u8 fatefulEncounter : 1;
    u8 gender : 2;
    u8 form : 5;
    u16 nickname[11];
    u16 otName[8];
    u8 ball;
    u8 language;
    u32 statusCond;
    u8 level;
    u8 pokestarFame;
    u16 nowHP;
    u16 maxHP;
    u16 atk;
    u16 def;
    u16 spe;
    u16 spa;
    u16 spd;
} BattleRecPkm;

// A party as a recording keeps it
typedef struct {
    u16 capacity;
    u16 count;
    BattleRecPkm pkm[6];
} BattleRecParty;

// A BtlSetupTrainer as a recording keeps it, with the name as characters
typedef struct {
    u32 aiFlags;
    u16 trainerId;
    u16 trainerClass;
    u16 items[4];
    u16 name[16];
    PMSData unk30;
    PMSData unk38;
} BattleRecTrainer;

enum {
    BATTLE_REC_CLIENT_NONE,
    BATTLE_REC_CLIENT_PLAYER,
    BATTLE_REC_CLIENT_TRAINER,
};

// A client of the battle: a player, whose info the setup has, or a trainer, BATTLE_REC_CLIENT_*
typedef struct {
    u16 type;
    u16 unk2;
    union {
        PlayerInfo player;
        BattleRecTrainer trainer;
    } info;
} BattleRecClient;

// The setup's parameters, which both forms keep at their start
typedef struct {
    MATHRandContext32 rand;
    BtlFieldEnv env;
    u8 config[4];
    u16 bgm;
    u16 unk2E;
    u16 unk30;
    u8 battleType : 5;
    u8 unk32_5 : 3;
    u8 battleStyle : 4;
    u8 unk33_4 : 3;
    u8 unk33_7 : 1;
} BattleRecSetup;

// The magic number of a video's header and body
#define BATTLE_REC_MAGIC 0xe281
// The size of the extra save block that holds a video, which is larger than the video
#define BATTLE_REC_BLOCK_SIZE 0x1a00
// The version that a video's header carries
#define BATTLE_REC_VERSION 100
// The battle modes that a header's mode can be, the size of data_0208f7b8
#define BATTLE_REC_MODE_MAX 41
// The Pokémon that a header lists
#define BATTLE_REC_HEADER_PKM_MAX 12

// The header of a battle video, which is what a list of videos shows
struct BattleRecHeader {
    u16 species[BATTLE_REC_HEADER_PKM_MAX];
    // The form in the low 7 bits and the gender in the top one
    u8 formGender[BATTLE_REC_HEADER_PKM_MAX];
    u16 counter;
    u16 mode;
    u16 magic;
    u8 unk2A;
    u8 version;
    u8 unk2C[0xc];
    // The video's number, which the server gives it and the CRC leaves out
    u64 dataNumber;
    // getCRC16 of everything before dataNumber
    u16 crc;
    u16 padding;
};

// The size that the header's CRC covers
#define BATTLE_REC_HEADER_CRC_SIZE offsetof(BattleRecHeader, dataNumber)

// The body of a battle video, after its profile and header
typedef struct {
    BattleRecSetup setup;
    u32 dataSize;
    u8 data[0xc00];
    BattleRecParty parties[4];
    BattleRecClient clients[4];
    u16 magic;
    u16 padding;
    // getCRC16 of everything before it, and the key that the body is encrypted with
    u16 crc;
    u16 padding2;
} BattleRecBody;

// The size that the body's CRC covers, and that is encrypted
#define BATTLE_REC_BODY_CRC_SIZE offsetof(BattleRecBody, crc)

// The battle video that is loaded, data_02140f64 (battle_rec.c).
typedef struct {
    u8 profile[0x80];
    BattleRecHeader header;
    BattleRecBody body;
} BattleRecVideo;

extern BattleRecVideo *data_02140f64;

// The values of a header that func_0200c124 gives
enum {
    BATTLE_REC_HEADER_SPECIES,
    BATTLE_REC_HEADER_FORM,
    BATTLE_REC_HEADER_COUNTER,
    BATTLE_REC_HEADER_MODE,
    BATTLE_REC_HEADER_DATA_NUMBER,
    BATTLE_REC_HEADER_UNK2A,
    BATTLE_REC_HEADER_GENDER,
    BATTLE_REC_HEADER_VERSION,
};

// Allocates the loaded video, freeing the one there is, and frees it
void func_0200bb24(HeapID heapId);
void freeVSPlayerBlkClearPtr(void);
// Frees a video of func_0200bcd0
void func_0200bb5c(BattleRecVideo *video);
u32 getSizeOfVSPlayer(void);
void clearVSPlayerBlk(BattleRecVideo *video);
// Whether the loaded video's body has anything in it
BOOL func_0200bb80(void);
// Decrypt and encrypt the loaded video's body
void func_0200bbb4(void);
void func_0200bbdc(void);
// Loads the saved video of a slot (0 is the player's own, 1 to 3 the downloaded ones). result is 1 if it loaded,
// 0 if there is none, 2 if its CRCs are wrong and 3 if the load failed
void func_0200bc9c(SaveControl *save, HeapID heapId, u32 *result, u32 slot);
// The same into a video of its own, which func_0200bb5c frees
BattleRecVideo *func_0200bcd0(SaveControl *save, HeapID heapId, u32 *result, u32 slot);
// Saves the loaded video to a slot as it is, a step each call: 0 while it runs, then the save's result (2 or 3)
u32 func_0200bdd4(GameData *gameData, BattleRecVideo *video, u32 slot, u16 *seq, HeapID heapId);
// The clients of a battle mode and the Pokémon of each
void func_0200bef4(u16 mode, s32 *clientCount, s32 *pkmCount);
// decryptData, which encrypts as well
void decryptVSPlayer(void *data, u32 size, u32 seed);
// The profile of the player who recorded the loaded video. The name is swan's
GdsProfile *getVSPlayerAllocation(void);
// The body and the header of the loaded video
BattleRecBody *func_0200c0b4(void);
BattleRecHeader *func_0200c0c0(void);
// The header of a video
BattleRecHeader *func_0200c0cc(BattleRecVideo *video);
// Makes the loaded video of a received profile, header and body, and decrypts its body
void func_0200c0d0(const GdsProfile *profile, const BattleRecHeader *header, const BattleRecBody *body);
// A value from the header, BATTLE_REC_HEADER_*, such as the video's number
u64 func_0200c124(BattleRecHeader *header, u32 id, u32 index);
// Sets the CRC of a body
void func_0200c1dc(BattleRecBody *body);
// func_0200bcf8, func_0200be50, func_0200c1d0, func_0200c1f0 and func_0200c200 of this file are declared in
// save/save_control.h

#endif // POKEBW2_SAVE_BATTLE_REC_H
