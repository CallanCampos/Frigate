#ifndef POKEBW2_DWC_MATCH_H
#define POKEBW2_DWC_MATCH_H

#include "types.h"
#include "dwc/account.h"
#include "dwc/error.h"
#include "dwc/friend.h"
#include "dwc/gp.h"
#include "dwc/gt2Main.h"

// DWC's matchmaking (dwc_match.c, a guessed name), in overlay 11: the friends match control that the game starts, logs
// in and runs every frame, the matching with anybody or with a friend, and the machines of a match, each with its ID
// in the match (its aid). The ROM names none of it: the functions take the SDK's names, from NitroDWC's and the Wii
// DWC's dwc_main.c and dwc_match.c and, for the API with a topology, the newer Wii DWC's headers; the ones that BW2's
// DWC added have names of ours in their style

// A machine of the match, as the match's list keeps it: its profile ID, its public and local addresses, the address
// of the host it was reached through, its aid, whether it is behind a NAT (it has a private address), how often
// connecting to it failed and when it is tried next, and its user data
typedef struct DWCNodeInfo {
    s32 profileId;
    u32 publicIp;
    u32 localIp;
    u16 publicPort;
    u16 localPort;
    u32 hostIp;
    u16 hostPort;
    u8 aid;
    u8 hasPrivateAddress;
    s32 connectFailureCount;
    u64 nextConnectTick;
    u32 userData;
} DWCNodeInfo;

// Asked how well another machine fits a random match, and the function's work
typedef int (*DWCEvalFunc)(int index, void *work);

// Starts the friends match control with the user data, the product ID, the secret key of the game, the sizes of the
// GT2 buffers (0 for 8 KB) and the friend list
void DWC_InitFriendsMatch(DWCUserData *userData, int productId, const char *secretKey, int sendBufSize, int recvBufSize,
                          DWCFriendData *friendList, int friendListLen);
// Ends it, freeing everything; and runs it, every frame
void DWC_ShutdownFriendsMatch(void);
void DWC_ProcessFriendsMatch(void);
// Logs in with the player's name, which FALSE says could not start
BOOL DWC_LoginAsync(const u16 *name, const char *reserved, DWCLoginCallback callback, void *param);
// Updates the friend list with the servers
BOOL DWC_UpdateServersAsync(const char *name, DWCUpdateServersCallback callback, void *userData,
                            DWCFriendStatusCallback statusCallback, void *statusParam,
                            DWCDeleteFriendListCallback deleteCallback, void *deleteParam);

// Matching: with the key and number, anybody, or a friend, and the callbacks that tell how it went
typedef void (*DWCMatchedFunc)(DWCError error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param);
typedef void (*DWCNewClientFunc)(int index, void *param);
typedef BOOL (*DWCAttemptFunc)(void *param);
BOOL DWC_ConnectToAnybodyAsync(int type, u8 numEntry, const char *query, DWCMatchedFunc matched, void *matchedParam,
                               DWCNewClientFunc newClient, void *newClientParam, DWCEvalFunc eval, void *evalParam,
                               DWCAttemptFunc attempt, int *attemptParam, void *param);
BOOL DWC_SetupGameServer(int type, u8 numEntry, DWCMatchedFunc matched, void *matchedParam, DWCNewClientFunc newClient,
                         void *newClientParam, DWCAttemptFunc attempt, int *attemptParam, void *param);
BOOL DWC_ConnectToGameServerAsync(int type, int friendIndex, DWCMatchedFunc matched, void *matchedParam,
                                  DWCNewClientFunc newClient, void *newClientParam, DWCAttemptFunc attempt,
                                  int *attemptParam, void *param);

// The function called when a connection to another machine has been closed, which FALSE says could not be set
typedef void (*DWCClosedFunc)(DWCError error, BOOL a1, BOOL a2, u8 netId, int index, void *param);
BOOL DWC_SetConnectionClosedCallback(DWCClosedFunc func, void *param);
// Closes every connection at once: 0 once closing, 1 when there were none (or matching was canceled), -1 when it can't
int DWC_CloseAllConnectionsHard(void);
// Closes the connection to a machine at once: 0 once closed, -1 when it can't, -2 for an unknown machine and -3 when
// this machine isn't the server
int DWC_CloseConnectionHard(u8 netId);

// The machines: how many there are, this machine's aid, the list of the aids, the aids as a bitmap, and whether an
// aid is another machine of the match
int DWC_GetNumConnectionHost(void);
u8 DWC_GetMyAID(void);
int DWC_GetAIDList(u8 **aids);
u32 DWC_GetAIDBitmap(void);
BOOL DWC_IsValidAID(u8 netId);
// Whether this machine is the server of the match (its dwc_hoststate is 2)
BOOL DWC_IsServerMyself(void);
// Makes the GT2 socket, returning GT2's result
int DWCi_GT2Startup(void);
// The GT2 connection to the machine with an aid, the aid of a connection (0xff if it has none) and whether there is a
// connection to an aid
GT2Connection DWCi_GetGT2Connection(u8 aid);
u8 DWCi_GetConnectionAID(GT2Connection connection);
BOOL DWCi_IsValidAID(u8 aid);
// Closes the connection to a machine; 0 once closed, -1 when it can't and -2 for an unknown machine
int DWCi_CloseConnectionHard(u8 aid);
// The server's aid, or the first machine's
u8 DWCi_GetServerOrSubServerAID(void);

// Adds a key with a value that is read when the match runs, a number or a text, and reads a candidate's key
u8 DWC_AddMatchKeyInt(u8 keyId, const char *key, const int *valueSrc);
u8 DWC_AddMatchKeyString(u8 keyId, const char *key, const char *value);
int DWC_GetMatchIntValue(int index, const char *key, int defaultValue);

// Takes a command that came from another machine of the match through the transport
void DWCi_ProcessMatchSynPacket(u8 aid, u16 type, const void *data);
// Sets the player's GameSpy Presence status with the match's numbers: its size (SCM), its machines (SCN), the version
// (VER) and whether it is closed (LCK)
int DWCi_GPSetServerStatus(int status);
// Takes a packet of the matchmaking that came in through the transport
void DWCi_ProcessGT2MatchCommand(const u8 *data, int size);
// Whether this machine is the first of the match, its server; and, on the server, sends the others the machines of the
// match every 15 seconds or at once when forced, returning -1 when that failed
BOOL DWCi_IsServerNodeMyself(void);
int DWCi_SendServerPoll(BOOL force);
// Whether the match is in one of its states 9 to 11
BOOL DWCi_IsSvDownState(void);
// The number of machines of the match, and the one at an index
int DWCi_GetNumNodeInfo(void);
DWCNodeInfo *DWCi_GetNodeInfoByIndex(int index);
// The LCK value of a friend's GameSpy Presence status: whether the friend's match is closed
u8 DWCi_GetBuddyStatusLCK(const GPBuddyStatus *status);
// Sets the player's profile ID, once logged in; and registers the matchmaking's qr2 keys (dwc_pid, dwc_mtype,
// dwc_mver, dwc_eval, dwc_groupid, dwc_hoststate and dwc_suspend)
void DWCi_SetMatchProfileID(int profileId);
void DWCi_RegisterMatchQR2Keys(void);

#endif // POKEBW2_DWC_MATCH_H
