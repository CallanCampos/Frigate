#ifndef POKEBW2_FIELD_BSUBWAY_SCR_H
#define POKEBW2_FIELD_BSUBWAY_SCR_H

#include "types.h"
#include "app/ov306.h"
#include "battle/btl_setup.h"
#include "gfl/heap.h"
#include "app/ircbattlematch.h"
#include "gfl/proc.h"
#include "save/player_info.h"
#include "struct_decls.h"
#include "system/pms.h"

// The Battle Subway's work while the player is on the subway, which func_0201794c returns. Overlay 33's
// bsubway_scr.c and overlay 12 keep it, and script plugin 1 (overlay 50) drives it

// A Pokémon of a Battle Subway Trainer, which genSubwayBtlInstitutePoke makes a party Pokémon of. The fields are
// the PokeParty fields func_ov033_0217bf04 copies into it
struct BSubwayPokemon {
    u16 species : 11;
    u16 form : 5;
    u16 item;
    u16 moves[4];
    u32 id;
    u32 personality;
    union {
        u32 all;
        struct {
            u32 hp : 5;
            u32 attack : 5;
            u32 defense : 5;
            u32 speed : 5;
            u32 spAttack : 5;
            u32 spDefense : 5;
            u32 unk30 : 2;
        } stat;
    } ivs;
    u8 evs[6];
    // Two bits per move
    u8 ppUps;
    u8 region;
    u8 ability;
    u8 happiness;
    u16 nickname[11];
    u8 nature;
    u8 unk39[3];
};

// The first two Pokémon picked for a trainer, to make them again: the ID they were made with, their files of the
// Pokémon arc, personalities and natures
struct BSubwayTeamConfig {
    u32 id;
    u16 files[2];
    u32 pids[2];
    u8 natures[2];
};

// A Trainer met in the Battle Subway or the Trial House
struct BSubwayTrainer {
    // The number of the trainer's file plus 1
    u32 unk00;
    u16 trainerId;
    u8 unk06[2];
    u16 name[8];
    // What the Trainer says before the battle, or a sentence type of 0xffff and a message of file 0x178
    PMSData message;
    // What the Trainer says on winning and on losing
    u16 winWords[4];
    u16 loseWords[4];
    BSubwayPokemon pokemon[4];
};

// The commands the multi-battle partners send each other, from GFL net command 0x2c00
enum {
    BSUBWAY_COMM_MEMBERS = 0x2c00,
    BSUBWAY_COMM_TRAINERS,
    BSUBWAY_COMM_RETIRE,
    BSUBWAY_COMM_PLAYER_INFO,
    BSUBWAY_COMM_PLAY_MODE,
    BSUBWAY_COMM_5,
    BSUBWAY_COMM_6,
    BSUBWAY_COMM_7,
    BSUBWAY_COMM_MAX,
};

// The size of a command's data, in u16s
#define BSUBWAY_COMM_BUF_LEN 35

// The beacon of a Battle Subway multi battle
typedef struct {
    PlayerInfo playerInfo;
    u8 mac[6];
    // 0x3a0b
    u16 gameId;
} BSubwayBeacon;

struct BSubwayScrWork {
    // 0x12345678
    u32 magic;
    u32 heapId;
    u8 memberCount;
    u8 playMode;
    u8 gender;
    u8 partnerGender;
    u16 unkC_0 : 1;
    u16 unkC_1 : 2;
    // Whether the partner retired
    u16 unkC_3 : 1;
    u16 unkC_4 : 1;
    u16 unkC_5 : 3;
    u16 unkC_8 : 1;
    u16 unkC_9 : 1;
    u16 unkC_10 : 2;
    u16 unkC_12 : 1;
    u16 unkC_13 : 3;
    u16 unkE;
    u16 unk10;
    // The BSUBWAY_COMM_* command to send
    u16 sendCommand;
    u8 unk14[4];
    u16 unk18;
    u16 partnerSpecies[2];
    // The party slots and species and items of the members entered
    u8 memberSlots[4];
    u16 memberSpecies[4];
    u16 memberItems[4];
    u16 unk32[0x1d];
    GameData *gameData;
    BSubwayPlayData *unk70;
    BSubwayScoreData *unk74;
    void *unk78;
    // Each member's chosen party slot plus 1
    u8 memberChoices[6];
    u16 unk82;
    u16 unk84;
    u8 unk86[2];
    BSubwayTrainer trainers[2];
    BSubwayTrainer unk2C8[3];
    BSubwayTeamConfig teamConfigs[3];
    u8 unk664[4];
    // The data of the command to send, and of the one received. Some commands write a PlayerInfo or a u32 over it,
    // which the array's 2-byte alignment and odd length rule out as a union member
    u16 sendBuf[BSUBWAY_COMM_BUF_LEN];
    u16 recvBuf[BSUBWAY_COMM_BUF_LEN];
    BSubwayBeacon beacon;
    void *unk71C;
    u16 *resultVar;
    // How many of the commands sent have arrived, counting this machine's own
    u8 recvCount;
    // 1 when only this machine's command is awaited
    u8 recvMode;
    u8 unk726;
    u8 unk727;
    // What the partner's command answered
    u16 recvResult;
    u16 unk72A;
    PlayerInfo partner;
    u8 unk74C[0x58];
    IrcBattleMatchParam ircMatchParam;
    void *allocatedBuffer;
    BtlSetup *btlSetup;
    Ov306Param ov306Param;
    u16 unk7EC;
    u16 unk7EE;
};

// Overlay 12's bsubway_comm.c
void func_ov012_02161844(BSubwayScrWork *bsw);
void func_ov012_02161894(BSubwayScrWork *bsw);
void func_ov012_021618ac(BSubwayScrWork *bsw);
void func_ov012_021618b8(u8 timing);
BOOL func_ov012_021618c8(u8 timing);
// Fills the send buffer for a command, mode being the command's number
void func_ov012_02161990(BSubwayScrWork *bsw, u16 mode, u16 value);
BOOL func_ov012_02161a48(BSubwayScrWork *bsw);
void func_ov012_02161a88(BSubwayScrWork *bsw, u8 mode);
BOOL func_ov012_02161a94(BSubwayScrWork *bsw, u16 *var);
// Overlay 12
// Makes a party of count Pokémon at the level
void func_ov012_021621d4(PokeParty *party, const BSubwayPokemon *pkms, u16 level, int count, HeapID heapId);
// Makes a Pokémon from the file of the Battle Subway's Pokémon arc, with the personality, or one made from id when it
// is 0, the IVs and, when rentalItem is set, the rental item of the index. Returns the personality
u32 func_ov012_02162490(BSubwayPokemon *pkm, u32 arcId, u16 file, u32 id, u32 pid, u8 iv, u8 index, BOOL rentalItem,
                        HeapID heapId);
void *func_ov012_021628c0(BSubwayTrainer *trainer, u32 arcId, u16 trainerId, u16 msgFile, HeapID heapId);
// Overlay 12's event_bsubway.c
// The party screen for picking the Pokémon to enter, from the rental party when rental is set
GameEvent *func_ov012_02165f70(BSubwayScrWork *bsw, GameSystem *gsys, u8 rental);
GameEvent *func_ov012_02166070(BSubwayScrWork *bsw, GameSystem *gsys, Field *field);
// The message of a trainer of the train in a balloon over the actor
GameEvent *func_ov012_02166118(BSubwayScrWork *bsw, GameSystem *gsys, int index, u16 actorId, u8 winPos);
GameEvent *func_ov012_02166294(GameSystem *gsys);
// The message of a saved leader in a balloon over the actor
GameEvent *func_ov012_0216657c(GameSystem *gsys, u16 index, u16 actorId);

// Overlay 33's bsubway_scr.c
void func_ov033_0217b468(GameSystem *gsys);
BSubwayScrWork *func_ov033_0217b478(GameSystem *gsys, u16 a1, u16 a2);
void func_ov033_0217b664(GameSystem *gsys, BSubwayScrWork *bsw);
void func_ov033_0217b6b4(BSubwayScrWork *bsw);
void func_ov033_0217b708(BSubwayScrWork *bsw);
void func_ov033_0217b790(BSubwayScrWork *bsw, GameSystem *gsys);
void func_ov033_0217b7e8(BSubwayScrWork *bsw);
u16 func_ov033_0217b86c(GameSystem *gsys);
void func_ov033_0217b8ac(GameSystem *gsys, BSubwayScrWork *bsw);
u16 func_ov033_0217b8ec(BSubwayScrWork *bsw);
void func_ov033_0217b9dc(BSubwayScrWork *bsw);
u16 func_ov033_0217ba94(BSubwayScrWork *bsw, GameSystem *gsys);
BOOL func_ov033_0217bb20(BSubwayScrWork *bsw);
void func_ov033_0217bb4c(BSubwayScrWork *bsw, GameSystem *gsys);
void func_ov033_0217bb98(BSubwayScrWork *bsw, GameSystem *gsys);
void func_ov033_0217bbac(BSubwayScrWork *bsw);
u32 func_ov033_0217bca0(BSubwayScrWork *bsw, u16 a1);
u16 func_ov033_0217bcb4(BSubwayScoreData *score, GameSystem *gsys, u32 a2);
void func_ov033_0217bd34(BSubwayScrWork *bsw);
PokeParty *func_ov033_0217bd60(BSubwayScrWork *bsw);
BOOL func_ov033_0217bdf4(const u16 *list, u16 value, u16 count);
u16 func_ov033_0217be1c(s32 value);
u16 func_ov033_0217bd84(BSubwayScrWork *bsw);
void func_ov033_0217bd8c(BSubwayScrWork *bsw);
void func_ov033_0217bd88(BSubwayScrWork *bsw, u32 score);
void func_ov033_0217be2c(BSubwayScrWork *bsw, SaveControl *save, u32 a2, u32 a3);
void func_ov033_0217c010(BSubwayScrWork *bsw, SaveControl *save, u32 value);
void func_ov033_0217bda0(BSubwayScrWork *bsw);
void func_ov033_0217bda8(BSubwayScrWork *bsw, u32 count, u32 extra);
u16 func_ov033_0217bdc0(u16 mode);
void func_ov033_0217be88(BSubwayScrWork *bsw, u8 a1);
void func_ov033_0217b9dc(BSubwayScrWork *bsw);
void func_ov033_0217bf04(BSubwayPokemon *dest, PartyPkm *pkm);
void *func_ov033_0217c110(BSubwayScrWork *bsw);
BtlSetup *func_ov033_0217c094(BSubwayScrWork *bsw, GameSystem *gsys);
BOOL func_ov033_0217c264(BSubwayScrWork *bsw, BSubwayTrainer *trainer, u16 trainerId, u32 count, const u16 *species,
                         const u16 *items, BSubwayTeamConfig *config, HeapID heapId);
u16 func_ov033_0217c11c(BSubwayScrWork *bsw, u16 level, u8 index, u32 mode, u8 side);
u8 func_ov033_0217c288(u32 value);
void func_ov033_0217c2c4(BSubwayScrWork *bsw, BSubwayTrainer *trainer, u16 trainerId, u32 count,
                         const BSubwayTeamConfig *config, HeapID heapId);
// Function name from swan
u16 randFFFFFFFFdivFFFF(BSubwayScrWork *bsw);

// The range of trainer IDs a train's trainers come from, by level
typedef struct {
    u16 min;
    u16 max;
} BSubwayTrainerRange;


#endif // POKEBW2_FIELD_BSUBWAY_SCR_H
