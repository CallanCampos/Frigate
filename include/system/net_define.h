#ifndef POKEBW2_SYSTEM_NET_DEFINE_H
#define POKEBW2_SYSTEM_NET_DEFINE_H

#include "types.h"
#include "dwc/dwc.h"
#include "struct_decls.h"

// The game's side of the network library, in main; the functions that the library and the Wi-Fi icon call are
// declared in gfl/net.h and gfl/wm_icon.h, and func_02011b30 in save/wifi_list.h

// Steps the network each frame, every other frame while the game command base is 0x35
void func_02011818(void);

// What func_02011980, func_02011a28's check and func_02011b30's check say of a friend's data against a list
#define FRIEND_CHECK_SAME 0
// The same profile under other data
#define FRIEND_CHECK_SAME_PROFILE 1
#define FRIEND_CHECK_NEW 2
#define FRIEND_CHECK_INVALID 3

// Looks for a friend key in the Friend Roster: index gets the friend's place, or the first free place, or -1
int func_02011980(WifiList *wifiList, u64 friendKey, int *index);
// Whether the friend data is new to the network's friend list; data of a known profile is replaced by the list's
BOOL func_02011a28(DWCFriendData *friendData);

#endif // POKEBW2_SYSTEM_NET_DEFINE_H
