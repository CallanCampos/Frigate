#ifndef POKEBW2_DWC_FRIEND_H
#define POKEBW2_DWC_FRIEND_H

#include "types.h"
#include "dwc/account.h"
#include "dwc/error.h"
#include "dwc/gp.h"
#include "nitro/os.h"

// DWC's friend list (overlay 11's dwc_friend.c): the texts of the friends' statuses and the friend list that DWC keeps
// in step with GameSpy's buddy list. The functions are named after the SDK's (see dwc_friend.c), and so is
// DWCFriendControl, the SDK's layout with the profile IDs added; the other layouts' names are ours, from their use.
// dwc/dwc.h declares the rest.

// The callbacks of the friend list's update: its end, with whether the list changed; a friend's status; a friend
// that was a copy of another and was deleted; and a friend that became a buddy
typedef void (*DWCUpdateServersCallback)(DWCError error, BOOL isChanged, void *param);
typedef void (*DWCFriendStatusCallback)(int index, u8 status, const char *statusString, void *param);
typedef void (*DWCDeleteFriendListCallback)(int deletedIndex, int srcIndex, void *param);
typedef void (*DWCBuddyFriendCallback)(int index, void *param);
// The callbacks of the persistent storage server's login, saving and loading (NitroDWC's base/dwc_friend.h)
typedef void (*DWCStorageLoginCallback)(DWCError error, void *param);
typedef void (*DWCSaveToServerCallback)(BOOL success, BOOL isPublic, void *param);
typedef void (*DWCLoadFromServerCallback)(BOOL success, int index, char *data, int len, void *param);

// The friend list's states: the SDK's DWCFriendState
typedef enum DWCFriendState {
    DWC_FRIEND_STATE_INIT = 0,
    DWC_FRIEND_STATE_PERS_LOGIN,
    DWC_FRIEND_STATE_LOGON,
    DWC_FRIEND_STATE_NUM,
} DWCFriendState;

// The friend list's state, which dwc_match.c gives the memory for
typedef struct DWCFriendControl {
    // 0 until the update starts, 1 while it runs and 2 once it is over
    DWCFriendState state;
    GPConnection *pGpObj;
    // How many times GameSpy's GP has been run, and when it was last
    u32 gpProcessCount;
    OSTick lastGpProcess;
    int friendListLen;
    DWCFriendData *friendList;
    // The friend that the update is at, whether it changed the list, and its step: 0 to read the buddy list, 1 to go
    // through the friends, 2 while a profile search runs and 3 once the friends are done
    u8 buddyUpdateIdx;
    u8 friendListChanged;
    u8 buddyUpdateState;
    // How many parts of the update are left: the friends, and the reverse buddies of the profile IDs
    u8 updateSteps;
    // Nonzero while the persistent storage server is logged in to
    int persLogin;
    // The player's profile ID, which nothing sets
    int profileID;
    // The player's name
    const u16 *playerName;
    DWCUpdateServersCallback updateCallback;
    void *updateParam;
    DWCFriendStatusCallback statusCallback;
    void *statusParam;
    DWCDeleteFriendListCallback deleteCallback;
    void *deleteParam;
    DWCBuddyFriendCallback buddyCallback;
    void *buddyParam;
    // The callbacks of the persistent storage server's login, saving and loading, which nothing sets (the SDK's
    // persLoginCallback, persLoginParam, saveCallback and loadCallback, in its order)
    DWCStorageLoginCallback persLoginCallback;
    void *persLoginParam;
    DWCSaveToServerCallback saveCallback;
    DWCLoadFromServerCallback loadCallback;
    // The profile IDs whose reverse buddies are asked for
    int *profileIds;
} DWCFriendControl;

// The friend list: starting it with its memory and the player's name, running it for a frame, whether its update runs,
// starting the update with its callbacks, ending it with an error, and the callbacks of GP's buddy requests, buddy
// authorizations and buddy statuses
void DWCi_FriendInit(DWCFriendControl *control, GPConnection *connection, const u16 *playerName,
                     DWCFriendData *friendList, int friendListLen);
void DWCi_FriendProcess(void);
BOOL DWCi_IsServerUpdating(void);
void DWCi_UpdateServersAsync(const char *authToken, const char *partnerChallenge,
                             DWCUpdateServersCallback updateCallback, void *updateParam,
                             DWCFriendStatusCallback statusCallback, void *statusParam,
                             DWCDeleteFriendListCallback deleteCallback, void *deleteParam);
void DWCi_StopFriendProcess(DWCError error, int code);
void DWCi_GPRecvBuddyRequestCallback(GPConnection *connection, GPRecvBuddyRequestArg *arg, void *param);
BOOL DWCi_GPRecvBuddyAuthCallback(GPConnection *connection, GPRecvBuddyAuthArg *arg, void *param);
void DWCi_GPRecvBuddyStatusCallback(GPConnection *connection, GPRecvBuddyStatusArg *arg, void *param);
// A friend's profile ID (0 if it has none), the index of a profile ID in the friend list (-1 if it isn't), noting that
// GP was run, setting the player's status (-1 and NULL keep the last ones) and ending the friend list
int DWCi_GetProfileIDFromList(int index);
int DWCi_GetFriendListIndex(int profileId);
void DWCi_InitGPProcessCount(void);
int DWCi_SetGPStatus(int status, const char *statusString, const char *locationString);
void DWCi_ShutdownFriend(void);
void DWCi_BeginCloseStatsConnection(void);
void DWCi_EndCloseStatsConnection(void);

#endif // POKEBW2_DWC_FRIEND_H
