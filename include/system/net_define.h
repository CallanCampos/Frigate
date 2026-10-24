#ifndef POKEBW2_SYSTEM_NET_DEFINE_H
#define POKEBW2_SYSTEM_NET_DEFINE_H

#include "types.h"
#include "dwc/dwc.h"
#include "struct_decls.h"

// The game's side of the network library, in main; the functions that the library and the Wi-Fi icon call are
// declared in gfl/net.h and gfl/wm_icon.h, and WifiList_AddFriend in save/wifi_list.h

// Steps the network each frame, every other frame while the game command base is 0x35
void NetDefine_Update(void);

// What WifiList_FindFriendKey, DWCFriend_UpdateFriendData's check and WifiList_AddFriend's check say of a friend's data
// against a list
enum {
    FRIEND_CHECK_SAME,
    // The same profile under other data
    FRIEND_CHECK_SAME_PROFILE,
    FRIEND_CHECK_NEW,
    FRIEND_CHECK_INVALID,
};

// Looks for a friend key in the Friend Roster: index gets the friend's place, or the first free place, or -1
int WifiList_FindFriendKey(WifiList *wifiList, u64 friendKey, int *index);
// Whether the friend data is new to the network's friend list; data of a known profile is replaced by the list's
BOOL DWCFriend_UpdateFriendData(DWCFriendData *friendData);

#endif // POKEBW2_SYSTEM_NET_DEFINE_H
