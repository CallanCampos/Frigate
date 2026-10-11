#ifndef POKEBW2_SAVE_WIFI_LIST_H
#define POKEBW2_SAVE_WIFI_LIST_H

#include "types.h"
#include "struct_decls.h"

s32 WifiList_GetMyGSID(WifiList *wifiList);
// The player's DWC user data
void *func_02009f7c(WifiList *wifiList);
BOOL func_0200a150(WifiList *wifiList);
// Whether the player's DWC user data is valid and has a friend code
BOOL func_0200a400(WifiList *wifiList);
void func_0200a2d4(WifiList *wifiList, u32 friendIndex, u32 wins, u32 losses, u32 draws);
void func_0200a29c(WifiList *wifiList, u32 friendIndex);
// Finds the player among the friends
BOOL func_0200a438(WifiList *wifiList, PlayerInfo *info, u32 *friendIndex);
// Finds a friend by ID and gender; the name is not compared
BOOL func_0200a46c(WifiList *wifiList, const u16 *name, u32 id, u32 gender, u32 *friendIndex);
// A block after the friends, of the players met, with its size
u32 func_0200a4b8(void);
void func_0200a504(void *block, PlayerInfo *info);
void func_0200a5cc(void *block);

// Clears the list, with the player's DWC user data
void initWifi(WifiList *wifiList);
// A friend's data, of a block of 12 bytes from index 0
void *func_0200a0e4(WifiList *wifiList, u32 friendIndex);
// Adds the wins, losses and draws of friend b to friend a, and clears b's, each capped at 9999
void func_0200a340(WifiList *wifiList, int a, int b);

BOOL func_0200a138(WifiList *list, u32 index);
u32 func_02009f80(WifiList *list, u32 index, u32 a2);
void *getPalPadFriendListAddress(SaveControl *save);
// Records a trainer the player traded with
void func_0200a504(void *list, PlayerInfo *info);

#endif // POKEBW2_SAVE_WIFI_LIST_H
