#ifndef POKEBW2_APP_LIVEBATTLEMATCH_FLOW_H
#define POKEBW2_APP_LIVEBATTLEMATCH_FLOW_H

#include "types.h"
#include "app/ov262_common.h"
#include "gfl/heap.h"
#include "save/player_info.h"
#include "struct_decls.h"
#include "system/pms_data.h"

// Overlay 261's livebattlematch_flow.c (named by its embedded string): the flow of the live competitions. Only what
// battle_championship_flow.c calls is declared so far, and none has a name yet

typedef struct LiveBattleMatchFlow LiveBattleMatchFlow;

// What the player sends the other player over the link: their trainer info, the last results of the live competition and
// the team picked, which comes after the LIVEBATTLEMATCH_PLAYER_HEADER_SIZE bytes of this
typedef struct {
    PlayerInfo info;
    u8 mac[6];
    u8 unk26[2];
    // The record's results 1, 2 and 0 (func_0200f1d8)
    u32 record1;
    u32 record2;
    u32 record0;
    u8 unk34[4];
    PMSData sentence;
    // Field 2 of the player's entry in the tournament block, which the two players compare
    u32 entryField2;
    u32 unk44;
    u8 unk48[0x8c];
} LiveBattleMatchPlayerData;

// What the flow leaves for battle_championship_flow.c's caller to read and the flow to show: how the last match
// ended, 0 to 4
typedef struct {
    u32 result;
    u8 unk04[4];
    u8 unk08;
    u8 unk09;
    u8 unk0A;
    // Set once the link failed, so that the flow keeps to the menus that need none
    BOOL disconnected;
} LiveBattleMatchResult;

// What battle_championship_flow.c gives the flow
typedef struct {
    // 0 to 3: the flow of a player who joins (0), of one who comes back to the menu (1) or after a match (2), or who
    // quits (3)
    u32 mode;
    Ov262MsgBox *msgBox;
    Font *font;
    PrintQueue *printQueue;
    Ov262Seq *seq;
    Ov262Net *net;
    GameData *gameData;
    // The player's data to send, where the other player's goes, and the result of the match (the last three of
    // overlay 290's parameters)
    LiveBattleMatchPlayerData *playerData;
    LiveBattleMatchPlayerData *partnerData;
    LiveBattleMatchResult *matchResult;
} LiveBattleMatchFlowParam;

LiveBattleMatchFlow *LiveBattleMatchFlow_Create(const LiveBattleMatchFlowParam *param, HeapID heapId);
void LiveBattleMatchFlow_Delete(LiveBattleMatchFlow *flow);
// Called every frame
void LiveBattleMatchFlow_Main(LiveBattleMatchFlow *flow);
// 0 while the flow runs, then why it ended
u32 LiveBattleMatchFlow_GetResult(LiveBattleMatchFlow *flow);

// Overlay 262's parts that only livebattlematch_flow.c uses (the digital card is in ov262_common.h): the card of the
// other player, the list of the team and the icon of the link
typedef struct Ov262PartnerCard Ov262PartnerCard;
typedef struct Ov262TeamList Ov262TeamList;
typedef struct Ov262LinkIcon Ov262LinkIcon;

Ov262PartnerCard *func_ov262_021c4644(const LiveBattleMatchPlayerData *data, Ov262NetLink *link, Ov262Seq *seq,
                                      Font *font, PrintQueue *printQueue, MsgData *msgData, WordSet *wordSet, u32 a7,
                                      u32 a8, u32 a9, HeapID heapId);
void func_ov262_021c4834(Ov262PartnerCard *card);
void func_ov262_021c4898(Ov262PartnerCard *card, PrintQueue *printQueue);
BOOL func_ov262_021c48a0(Ov262PartnerCard *card);
Ov262LinkIcon *func_ov262_021c4e44(Ov262NetLink *link, Ov262Seq *seq, HeapID heapId);
void func_ov262_021c4f14(Ov262LinkIcon *icon);
void func_ov262_021c4f2c(Ov262LinkIcon *icon);
void func_ov262_021c4f30(Ov262LinkIcon *icon, BOOL shown);
Ov262TeamList *func_ov262_021c4f60(BattleBoxSave *battleBox, Ov262NetLink *link, Ov262Seq *seq, Font *font,
                                   PrintQueue *printQueue, MsgData *msgData, HeapID heapId);
void func_ov262_021c53e0(Ov262TeamList *list);
BOOL func_ov262_021c545c(Ov262TeamList *list);
BOOL func_ov262_021c5544(Ov262TeamList *list);
void func_ov262_021c561c(Ov262TeamList *list, PrintQueue *printQueue);

// Functions of the save and battle video code that have no header yet
// Checks the CRC of a tournament entry
BOOL func_0200b8c8(void *entry);
// Copies an entry into the block
void func_0200b900(void *block, u32 index, const void *entry);
// Clears an entry of the block's other entries, which are 0x1e bytes
void func_0200b918(void *record);
// Fills an entry of the block's other entries with the team's species and moves
void func_0200b9c4(void *record, PokeParty *party);
// Whether the battle video that is loaded is whole
BOOL func_0200bb80(void);
void func_0200c360(BattleBoxSave *battleBox, u16 mask);
// The live competition's record: clears it and gives it this console's address, adds to a result by an amount, gets and
// sets the 6-byte address of a result by its place and a byte after it, and whether an address is among the results
void func_0200f1c4(void *record);
void func_0200f1e0(void *record, u32 index, int amount);
void func_0200f218(void *record, u32 place, const u8 *mac);
void func_0200f238(void *record, u32 place, u32 byte, u8 value);
BOOL func_0200f254(void *record, const u8 *mac);
// Whether a Pokémon of the party has its parameters 0xa9 and 0x4c both at 1 (the regulation is not used)
BOOL func_0201f6e8(Regulation *regulation, PokeParty *party);

#endif // POKEBW2_APP_LIVEBATTLEMATCH_FLOW_H
