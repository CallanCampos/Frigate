#ifndef POKEBW2_SAVE_WIFI_LIST_H
#define POKEBW2_SAVE_WIFI_LIST_H

#include "types.h"
#include "dwc/dwc.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "struct_decls.h"

// Names from swan: getSizeofWifiData, populateWifiList, copyWifilist, initWifi, WifiList and
// getPalPadFriendListAddress. WifiList_GetMyGSID, WifiListFriend and PalPadFriendList are ours

#define WIFI_LIST_FRIEND_MAX 32
#define WIFI_LIST_RECORD_MAX 9999
#define PAL_PAD_PLAYER_MAX 100
#define PAL_PAD_TRADE_COUNT_MAX 99999

// What func_02009f80 gives and func_0200a034 sets of a friend
#define WIFI_LIST_FRIEND_ID 0
#define WIFI_LIST_FRIEND_PROFILE_ID 1
#define WIFI_LIST_FRIEND_WINS 2
#define WIFI_LIST_FRIEND_LOSSES 3
#define WIFI_LIST_FRIEND_DRAWS 4
#define WIFI_LIST_FRIEND_LAST_YEAR 5
#define WIFI_LIST_FRIEND_LAST_MONTH 6
#define WIFI_LIST_FRIEND_LAST_DAY 7
#define WIFI_LIST_FRIEND_TRAINER_VIEW 8
#define WIFI_LIST_FRIEND_GENDER 9

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
DWCUserData *func_02009f7c(WifiList *wifiList);
// A friend's WIFI_LIST_FRIEND_* field; the record fields can't be set
u32 func_02009f80(WifiList *wifiList, u32 index, u32 param);
void func_0200a034(WifiList *wifiList, u32 index, u32 param, u32 value);
DWCFriendData *func_0200a0e4(WifiList *wifiList, u32 index);
// A friend's name, and loading or storing it through a StrBuf
u16 *func_0200a0f0(WifiList *wifiList, u32 index);
void func_0200a100(WifiList *wifiList, u32 index, StrBuf *dest);
void func_0200a11c(WifiList *wifiList, u32 index, const StrBuf *src);
// Whether the slot holds a friend, how many there are and the last used slot plus one
BOOL func_0200a138(WifiList *wifiList, u32 index);
u32 func_0200a150(WifiList *wifiList);
u32 func_0200a170(WifiList *wifiList);
// Deletes a friend and moves the later ones down
void func_0200a190(WifiList *wifiList, u32 index);
// Moves the friends into the empty slots
void func_0200a258(WifiList *wifiList);
// Stamps today on a friend
void func_0200a29c(WifiList *wifiList, u32 index);
// Adds to a friend's record and stamps today; and adds one friend's record to another's, clearing the first
void func_0200a2d4(WifiList *wifiList, s32 index, u32 wins, u32 losses, u32 draws);
void func_0200a340(WifiList *wifiList, s32 srcIndex, s32 destIndex);
// A token for exchanging friend data, from user data or the player's
void func_0200a3c4(const DWCUserData *userData, DWCFriendData *token);
void func_0200a3cc(WifiList *wifiList, DWCFriendData *token);
s32 WifiList_GetMyGSID(WifiList *wifiList);
// Whether the player's DWC user data has a profile and is this console's
BOOL func_0200a400(WifiList *wifiList);
WifiList *func_0200a424(SaveControl *save);
// Finds the player among the friends
BOOL func_0200a438(WifiList *wifiList, PlayerInfo *info, u32 *friendIndex);
// Finds a friend by ID and gender; the name is not compared
BOOL func_0200a46c(WifiList *wifiList, const u16 *name, u32 id, u32 gender, u32 *friendIndex);

u32 func_0200a4b8(void);
void func_0200a4c0(PalPadFriendList *list);
// The slot of the player with the ID, or -1
s32 func_0200a4d4(PalPadFriendList *list, u32 id);
// Records a trainer the player traded with
void func_0200a504(PalPadFriendList *list, PlayerInfo *info);
u32 func_0200a55c(PalPadFriendList *list, u32 index);
PlayerInfo *func_0200a570(PalPadFriendList *list, u32 index);
BOOL func_0200a590(PalPadFriendList *list, u32 id);
u32 func_0200a5a8(PalPadFriendList *list);
u32 func_0200a5c4(PalPadFriendList *list);
void func_0200a5cc(PalPadFriendList *list);
PalPadFriendList *getPalPadFriendListAddress(SaveControl *save);

#endif // POKEBW2_SAVE_WIFI_LIST_H
