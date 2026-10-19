#ifndef POKEBW2_FIELD_COLOSSEUM_H
#define POKEBW2_FIELD_COLOSSEUM_H

#include "types.h"
#include "battle/battle_proc.h"
#include "field/comm_player.h"
#include "field/event_colosseum_battle.h"
#include "field/trcard_sys.h"
#include "save/player_info.h"
#include "struct_decls.h"

// colosseum.c, overlay 28, which the ROM names: the Union Room's colosseum, where up to four players gather their
// parties for a battle. Its work is shared with union_main.c and overlay 34

#define COLOSSEUM_MEMBER_MAX 4

// What a player sends the others (0x14 bytes, command 0x1503)
typedef struct {
    u32 unk00[5];
} ColosseumChoice;

// A player of the colosseum
typedef struct {
    PlayerInfo info;
    u8 mac[6];
    u8 unk26;
    u8 unk27;
    u8 unk28;
} ColosseumMember;

struct UnionColosseum {
    CommPlayerSys *commPlayer; // from CommPlayer_Init
    u8 unk04[0x14];
    ColosseumMember members[COLOSSEUM_MEMBER_MAX];
    u8 unkC8_0 : 1;
    u8 unkC8_1 : 1;
    u8 unkC8_2 : 6;
    u8 unkC9;
    u8 unkCA;
    u8 unkCB;
    u8 unkCC[4];
    u8 unkD0[4];
    u8 unkD4[4];
    u8 unkD8[4];
    u8 unkDC;
    u8 unkDD;
    u8 unkDE;
    u8 unkDF;
    ColosseumChoice choice[COLOSSEUM_MEMBER_MAX];
    u8 choiceReady[4];
    TrainerCardData *card[COLOSSEUM_MEMBER_MAX];      // where a player's card arrives
    TrainerCardData *cardCheck[COLOSSEUM_MEMBER_MAX]; // the same cards, which the receive checks
    u8 cardReady[4];
    PokeParty *party[COLOSSEUM_MEMBER_MAX];
    u8 partyReady[4];
    u8 unk16C[4];
    u8 unk170;
    u8 unk171;
    u8 unk172[4];
    u8 unk176[2];
    void *unk178; // a table of the field's players (see func_ov036_021c3f34)
    u8 unk17C[3];
    u8 unk17F;
};

// Registers the colosseum's net commands, with the UnionSystem as their work
void Colosseum_RegisterCommands(UnionSystem *sys);
// Sends the player's info (to the parent only when toParent is TRUE, else to all), the card, and the card's first part
BOOL Colosseum_SendMember(const void *member, BOOL toParent);
BOOL Colosseum_SendCard(const TrainerCardData *card);
BOOL Colosseum_SendCardHead(const TrainerCardData *card);
BOOL Colosseum_SendChoice(const ColosseumChoice *choice);
BOOL Colosseum_SendUnk4(UnionColosseum *colosseum);
BOOL Colosseum_SendParty(const PokeParty *party);
BOOL Colosseum_SendResult(const void *result);
BOOL Colosseum_SendUnk8(void);
BOOL Colosseum_SendUnk9(void);
BOOL Colosseum_SendUnkD(void);

// The colosseum's work, made for the player and freed
UnionColosseum *Colosseum_Create(GameData *gameData, GameSystem *gsys, const PlayerInfo *info, u32 flag);
void Colosseum_Free(UnionColosseum *colosseum, GameSystem *gsys);
void Colosseum_SetUnkC8_0(UnionColosseum *colosseum, BOOL value);
void Colosseum_SetUnkDC(UnionColosseum *colosseum, u32 value);
u8 Colosseum_GetUnkDD(const UnionColosseum *colosseum);
void Colosseum_SendVoteReplies(UnionColosseum *colosseum);
void Colosseum_UpdateReadySync(UnionColosseum *colosseum);
u8 Colosseum_TakeChoice(UnionColosseum *colosseum, u32 netId, ColosseumChoice *choice);
void Colosseum_ResetUnk171(UnionColosseum *colosseum);
BOOL Colosseum_AreAllPartiesReceived(const UnionColosseum *colosseum);
void Colosseum_ClearParties(UnionColosseum *colosseum, BOOL keepOwn);
void Colosseum_BuildBattlePlayers(UnionColosseum *colosseum, GameSystem *gsys, ColosseumPlayers *players, u32 unused);
void Colosseum_ClearPlayers(ColosseumPlayers *players);

// What colosseum_comm.c and colosseum.c use of each other
BOOL Colosseum_SendUnk5(UnionColosseum *colosseum, u32 netId, u32 value);
BOOL Colosseum_SendUnkA(u32 netId);
BOOL Colosseum_SendUnkB(void);
u8 Colosseum_GetUnkDC(const UnionColosseum *colosseum);
void Colosseum_SetUnkDD(UnionColosseum *colosseum, u32 value);
BOOL Colosseum_Vote(UnionColosseum *colosseum, u32 netId, u8 value);
void Colosseum_StoreChoice(UnionColosseum *colosseum, u32 netId, const ColosseumChoice *choice);

#endif // POKEBW2_FIELD_COLOSSEUM_H
