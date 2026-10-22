#ifndef POKEBW2_DWC_FRIEND_H
#define POKEBW2_DWC_FRIEND_H

#include "types.h"
#include "dwc/account.h"
#include "dwc/error.h"
#include "dwc/gp.h"
#include "nitro/os.h"

// Overlay 11's friend list and login (friend.c, a guessed name: the ROM names none of DWC's files): the date of the
// authentication server, the text of the friends' statuses, the friend list that DWC keeps in step with GameSpy's
// buddy list, and the login to GameSpy's GP with the token of the authentication server. The ROM names none of it:
// the functions are named after the SDK's (see friend.c), and so is DWCFriendControl, the SDK's layout with the
// profile IDs added; the other layouts' names are ours, from their use. dwc/dwc.h and dwc/ext.h declare the rest.

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
    GPConnection *connection;
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

// The callback of the login, with its error and the profile ID that logged in
typedef void (*DWCLoginCallback)(DWCError error, int profileId, void *param);

// The login's states: the SDK's DWCLoginState
typedef enum DWCLoginState {
    DWC_LOGIN_STATE_INIT = 0,
    DWC_LOGIN_STATE_REMOTE_AUTH,
    DWC_LOGIN_STATE_CONNECTING,
    DWC_LOGIN_STATE_GPGETINFO,
    DWC_LOGIN_STATE_GPSETINFO,
    DWC_LOGIN_STATE_CONNECTED,
    DWC_LOGIN_STATE_NUM,
} DWCLoginState;

// The login's state, which dwc_match.c gives the memory for
typedef struct DWCLoginControl {
    GPConnection *connection;
    // 0 before the login, 1 while the authentication server is asked, 2 to 4 while GP logs in (with the user data's
    // profile, or looking for it with the login ID) and 5 once logged in
    DWCLoginState state;
    int productId;
    u32 gameCode;
    const u16 *playerName;
    DWCLoginCallback callback;
    void *param;
    DWCUserData *userData;
    // The SDK's work of the backup manager, HTTP client and start of the login, which this version leaves unused
    void *bmWork;
    void *http;
    OSTick startTick;
    // Whether GP is being connected to, and since when
    u32 connecting;
    OSTick connectTick;
    // The login ID that the login uses until the user data has an authenticated one
    DWCAccLoginId tempLoginId;
    // What the authentication server gave for GP's login
    char authToken[0x100];
    char partnerChallenge[0x100];
    // The login name: the user ID, the game code and the player ID as text
    char loginName[21];
} DWCLoginControl;

// The server's date and time, and the "Date" header of the authentication server's answer, such as
// "Fri, 03 Mar 2006 01:28:13 GMT", which it is kept from (FALSE if the header can't be read)
BOOL DWCi_SetNasTimeFromDateHeader(const char *dateHeader);

// The texts of the statuses: a text of sep, key, sep and value written to out, or appended to it, both returning the
// text's length, and the value of a key read to dest (only measured if dest is NULL), returning its length or -1
int DWC_SetCommonKeyValueString(const char *key, const char *value, char *out, char sep);
int DWC_AddCommonKeyValueString(const char *key, const char *value, char *out, char sep);
int DWC_GetCommonValueString(const char *key, char *dest, const char *text, char sep);

// A random number below max (all 32 bits if max is 0), from a generator seeded with the console's address and clock
u32 DWCi_GetMathRand32(u32 max);
// The length of a text of u16 characters
u32 DWCi_WStrLen(const u16 *str);

// The friend list: starting it with its memory and the player's name, running it for a frame, whether its update runs,
// starting the update with its callbacks, ending it with an error, and the callbacks of GP's buddy requests, buddy
// authorizations and buddy statuses
void DWCi_FriendInit(DWCFriendControl *control, GPConnection *connection, const u16 *playerName,
                     DWCFriendData *friendList, int friendListLen);
void DWCi_FriendProcess(void);
BOOL DWCi_IsUpdatingServers(void);
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
void func_ov011_0215f2c0(void);
void func_ov011_0215f2cc(void);

// The login: starting it with its memory, starting the login to the authentication server, running it for a frame, the
// user data, ending it with an error, and ending it
void DWCi_LoginInit(DWCLoginControl *control, DWCUserData *userData, GPConnection *connection, int productId,
                    u32 gameCode, const u16 *playerName, DWCLoginCallback callback, void *param);
BOOL DWCi_LoginAsync(void);
void DWCi_LoginProcess(void);
DWCUserData *DWCi_GetUserData(void);
void DWCi_StopLogin(DWCError error, int code);
void DWCi_ShutdownLogin(void);

#endif // POKEBW2_DWC_FRIEND_H
