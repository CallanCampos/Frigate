#ifndef POKEBW2_APP_WIFIBATTLEMATCH_VIEW_H
#define POKEBW2_APP_WIFIBATTLEMATCH_VIEW_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "save/player_info.h"
#include "struct_decls.h"
#include "system/printsys.h"

// Overlay 262's wifibattlematch_view.c, named after the ROM's string: the backdrop of the Wi-Fi and live competition
// screens and what they show on it, the digital cards, the other player's card, the link icon and the team list. See
// wifibattlematch_graphic.h for the overlay

// The BG graphics and OBJ resources that the screens share, and their animated palette
typedef struct WifiBattleMatchRes WifiBattleMatchRes;
// A player's digital card, of the Wi-Fi competition's or of the live one's, and the rank card
typedef struct WifiBattleMatchCard WifiBattleMatchCard;
typedef struct WifiBattleMatchPartnerCard WifiBattleMatchPartnerCard;
typedef struct WifiBattleMatchTeamList WifiBattleMatchTeamList;
typedef struct WifiBattleMatchLinkIcon WifiBattleMatchLinkIcon;

// What the digital card is made from, for the Wi-Fi competition and for the live one
typedef struct {
    // The entry's data, which starts with the player's name
    u8 data[0x92];
    u8 unk92[2];
    // Dates, as the year since 2000, the month and the day in the bytes from the top
    u32 startDate;
    u32 endDate;
    // The player's appearance (func_02008bf4)
    u32 look;
    u16 unkA0;
    u8 unkA2[2];
    u32 unkA4;
    u16 unkA8;
    // Field 11 of the entry, which picks the card's colors
    u16 variant;
} WifiBattleMatchWifiCardInfo;

typedef struct {
    u8 data[0x92];
    u8 unk92[2];
    u32 startDate;
    u32 endDate;
    u32 look;
    u16 unkA0;
    u16 unkA2;
    u16 unkA4;
    u16 unkA6;
    // Field 11 of the entry, which picks the card's colors
    u16 variant;
    u8 unkAA[2];
} WifiBattleMatchLiveCardInfo;

// What the rank card shows
typedef struct {
    // The message of the title is this plus 2
    u32 titleMsg;
    u16 unk4;
    u8 unk6[2];
    // The player's appearance
    u32 look;
    u32 unkC;
    u16 unk10;
    u16 unk12;
} WifiBattleMatchRankInfo;

// The backdrop. WifiBattleMatchRes_Create takes the unit of the cell actors and a kind of backdrop, 0 to 3
WifiBattleMatchRes *WifiBattleMatchRes_Create(ClActUnit *unit, u32 kind, HeapID heapId);
void WifiBattleMatchRes_Free(WifiBattleMatchRes *res);
// Loads the BG screens of a kind of backdrop
void WifiBattleMatchRes_LoadBG(WifiBattleMatchRes *res, u32 kind, HeapID heapId);
// Animates the backdrop's palette
void WifiBattleMatchRes_Update(WifiBattleMatchRes *res);

// The rank card
WifiBattleMatchCard *WifiBattleMatchRankCard_Create(const WifiBattleMatchRankInfo *info, u32 titleMsg, PlayerInfo *playerInfo,
                                         ClActUnit *unit, WifiBattleMatchRes *res, Font *font,
                                         PrintQueue *printQueue, MsgData *msgData, WordSet *wordSet, HeapID heapId);
void WifiBattleMatchRankCard_Free(WifiBattleMatchCard *card);

// The digital card. The first of each pair is for the Wi-Fi competition's, the second for the live one's. ratio says
// whether the card's results are printed as two numbers with a message between them; main picks the main screen, or the
// sub screen
WifiBattleMatchCard *WifiBattleMatchWifiCard_Create(const WifiBattleMatchWifiCardInfo *info, u32 ratio, PlayerInfo *playerInfo,
                                         ClActUnit *unit, WifiBattleMatchRes *res, Font *font,
                                         PrintQueue *printQueue, MsgData *msgData, WordSet *wordSet, void *records,
                                         BOOL main, HeapID heapId);
WifiBattleMatchCard *WifiBattleMatchLiveCard_Create(const WifiBattleMatchLiveCardInfo *info, PlayerInfo *playerInfo,
                                         ClActUnit *unit, WifiBattleMatchRes *res, Font *font,
                                         PrintQueue *printQueue, MsgData *msgData, WordSet *wordSet, void *records,
                                         BOOL main, HeapID heapId);
void WifiBattleMatchWifiCard_Free(WifiBattleMatchCard *card);
void WifiBattleMatchLiveCard_Free(WifiBattleMatchCard *card);
// Shows a page of the card: its Pokémon, then two other pages that hide them
void WifiBattleMatchWifiCard_ShowPage(WifiBattleMatchCard *card, u32 page, MsgData *msgData, PrintQueue *printQueue, Font *font,
                         HeapID heapId);
void WifiBattleMatchLiveCard_ShowPage(WifiBattleMatchCard *card, u32 page, MsgData *msgData, PrintQueue *printQueue, Font *font,
                         HeapID heapId);
// Is TRUE once everything of the card is printed
BOOL WifiBattleMatchCard_Flush(WifiBattleMatchCard *card, PrintQueue *printQueue);
// Slide the card in and out, each frame, and return TRUE once it is done
BOOL WifiBattleMatchCard_SlideIn(WifiBattleMatchCard *card);
BOOL WifiBattleMatchCard_SlideOut(WifiBattleMatchCard *card);

// The other player's card
WifiBattleMatchPartnerCard *WifiBattleMatchPartnerCard_Create(const LiveBattleMatchPlayerData *data, ClActUnit *unit,
                                                WifiBattleMatchRes *res, Font *font, PrintQueue *printQueue,
                                                MsgData *msgData, WordSet *wordSet, u32 kind, u32 shown, u32 variant,
                                                HeapID heapId);
void WifiBattleMatchPartnerCard_Free(WifiBattleMatchPartnerCard *card);
BOOL WifiBattleMatchPartnerCard_Flush(WifiBattleMatchPartnerCard *card, PrintQueue *printQueue);
BOOL WifiBattleMatchPartnerCard_SlideIn(WifiBattleMatchPartnerCard *card);

// The icon of the link
WifiBattleMatchLinkIcon *WifiBattleMatchLinkIcon_Create(ClActUnit *unit, WifiBattleMatchRes *res, HeapID heapId);
void WifiBattleMatchLinkIcon_Free(WifiBattleMatchLinkIcon *icon);
void WifiBattleMatchLinkIcon_Update(WifiBattleMatchLinkIcon *icon);
void WifiBattleMatchLinkIcon_SetShown(WifiBattleMatchLinkIcon *icon, BOOL shown);

// The list of the team
WifiBattleMatchTeamList *WifiBattleMatchTeamList_Create(BattleBoxSave *battleBox, ClActUnit *unit, WifiBattleMatchRes *res,
                                             Font *font, PrintQueue *printQueue, MsgData *msgData, HeapID heapId);
void WifiBattleMatchTeamList_Free(WifiBattleMatchTeamList *list);
BOOL WifiBattleMatchTeamList_SlideIn(WifiBattleMatchTeamList *list);
BOOL WifiBattleMatchTeamList_SlideOut(WifiBattleMatchTeamList *list);
BOOL WifiBattleMatchTeamList_Flush(WifiBattleMatchTeamList *list, PrintQueue *printQueue);

#endif // POKEBW2_APP_WIFIBATTLEMATCH_VIEW_H
