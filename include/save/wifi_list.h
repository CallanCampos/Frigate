#ifndef POKEBW2_SAVE_WIFI_LIST_H
#define POKEBW2_SAVE_WIFI_LIST_H

#include "types.h"
#include "dwc/dwc.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "struct_decls.h"

// Names from swan: getSizeofWifiData, populateWifiList, copyWifilist, initWifi, WifiList and
// getPalPadFriendListAddress. WifiList_GetMyGSID, WifiListFriend and PalPadFriendList are ours

#define WIFI_LIST_FRIEND_MAX 32
#define WIFI_LIST_RECORD_MAX 9999
#define PAL_PAD_PLAYER_MAX 100
#define PAL_PAD_TRADE_COUNT_MAX 99999

// What WifiList_GetFriendParam gives and WifiList_SetFriendParam sets of a friend
enum {
    WIFI_LIST_FRIEND_ID,
    WIFI_LIST_FRIEND_PROFILE_ID,
    WIFI_LIST_FRIEND_WINS,
    WIFI_LIST_FRIEND_LOSSES,
    WIFI_LIST_FRIEND_DRAWS,
    WIFI_LIST_FRIEND_LAST_YEAR,
    WIFI_LIST_FRIEND_LAST_MONTH,
    WIFI_LIST_FRIEND_LAST_DAY,
    WIFI_LIST_FRIEND_TRAINER_VIEW,
    WIFI_LIST_FRIEND_GENDER,
};

// A gender of a friend added by friend code, whose trainer the player hasn't met
#define WIFI_LIST_GENDER_UNKNOWN 2

// What the Pal Pad keeps of a friend besides their DWC friend data
struct WifiListFriend {
    u16 name[8];
    u8 unk10[0xc];
    s32 profileId;
    u32 id;
    u16 wins;
    u16 losses;
    u16 draws;
    // When the player last battled or traded with the friend
    u16 lastYear;
    u8 lastMonth;
    u8 lastDay;
    u8 trainerView;
    u8 gender : 2;
    u8 unk2F_2 : 6;
}; // size: 0x30

// Block 0x1e: the player's own DWC account and the Pal Pad's friends
struct WifiList {
    DWCUserData myUserData;
    DWCFriendData friendData[WIFI_LIST_FRIEND_MAX];
    WifiListFriend friends[WIFI_LIST_FRIEND_MAX];
    u8 unk7C0[4];
}; // size: 0x7c4

// Block 0x1f (swan's getPalPadFriendListAddress): the players traded with, the latest first, and how many trades
struct PalPadFriendList {
    PlayerInfo players[PAL_PAD_PLAYER_MAX];
    u8 unkC80[0xd0];
    u32 tradeCount;
}; // size: 0xd54

u32 getSizeofWifiData(void);
WifiList *populateWifiList(u32 heapId);
void copyWifilist(const WifiList *src, WifiList *dest);
void initWifi(WifiList *wifiList);
// The player's DWC user data
DWCUserData *WifiList_GetMyUserData(WifiList *wifiList);
// A friend's WIFI_LIST_FRIEND_* field; the record fields can't be set
u32 WifiList_GetFriendParam(WifiList *wifiList, u32 index, u32 param);
void WifiList_SetFriendParam(WifiList *wifiList, u32 index, u32 param, u32 value);
DWCFriendData *WifiList_GetFriendData(WifiList *wifiList, u32 index);
// A friend's name, and loading or storing it through a StrBuf
u16 *WifiList_GetFriendName(WifiList *wifiList, u32 index);
void WifiList_LoadFriendName(WifiList *wifiList, u32 index, StrBuf *dest);
void WifiList_StoreFriendName(WifiList *wifiList, u32 index, const StrBuf *src);
// Whether the slot holds a friend, how many there are and the last used slot plus one
BOOL WifiList_IsFriendValid(WifiList *wifiList, u32 index);
u32 WifiList_CountFriends(WifiList *wifiList);
u32 WifiList_GetFriendSlotCount(WifiList *wifiList);
// Deletes a friend and moves the later ones down
void WifiList_RemoveFriend(WifiList *wifiList, u32 index);
// Moves the friends into the empty slots
void WifiList_CompactFriends(WifiList *wifiList);
// Stamps today on a friend
void WifiList_SetFriendLastDateToNow(WifiList *wifiList, u32 index);
// Adds to a friend's record and stamps today; and adds one friend's record to another's, clearing the first
void WifiList_AddFriendRecord(WifiList *wifiList, s32 index, u32 wins, u32 losses, u32 draws);
void WifiList_MergeFriendRecord(WifiList *wifiList, s32 srcIndex, s32 destIndex);
// A token for exchanging friend data, from user data or the player's
void WifiList_CreateExchangeTokenFromUserData(const DWCUserData *userData, DWCFriendData *token);
void WifiList_CreateExchangeToken(WifiList *wifiList, DWCFriendData *token);
s32 WifiList_GetMyGSID(WifiList *wifiList);
// Adds the friend whose 12 bytes of friend data a player gave, with the player's info, to the first free place of the
// Friend Roster; TRUE if there was one, FALSE if the friend was in it already or it was full. heapId is a scratch heap
BOOL WifiList_AddFriend(GameData *gameData, PlayerInfo *info, const void *friendData, HeapID heapId);
// Whether the player's DWC user data has a profile and is this console's
BOOL WifiList_HasValidProfile(WifiList *wifiList);
WifiList *SaveControl_GetWifiList(SaveControl *save);
// Finds the player among the friends
BOOL WifiList_FindFriendByPlayerInfo(WifiList *wifiList, PlayerInfo *info, u32 *friendIndex);
// Finds a friend by ID and gender; the name is not compared
BOOL WifiList_FindFriend(WifiList *wifiList, const u16 *name, u32 id, u32 gender, u32 *friendIndex);

u32 PalPadFriendList_GetSize(void);
void PalPadFriendList_Init(PalPadFriendList *list);
// The slot of the player with the ID, or -1
s32 PalPadFriendList_FindPlayer(PalPadFriendList *list, u32 id);
// Records a trainer the player traded with
void PalPadFriendList_AddPlayer(PalPadFriendList *list, PlayerInfo *info);
u32 PalPadFriendList_GetPlayerId(PalPadFriendList *list, u32 index);
PlayerInfo *PalPadFriendList_GetPlayer(PalPadFriendList *list, u32 index);
BOOL PalPadFriendList_HasPlayer(PalPadFriendList *list, u32 id);
u32 PalPadFriendList_CountPlayers(PalPadFriendList *list);
u32 PalPadFriendList_GetTradeCount(PalPadFriendList *list);
void PalPadFriendList_IncrementTradeCount(PalPadFriendList *list);
PalPadFriendList *getPalPadFriendListAddress(SaveControl *save);

#endif // POKEBW2_SAVE_WIFI_LIST_H
