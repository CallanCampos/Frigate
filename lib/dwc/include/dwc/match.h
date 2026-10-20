#ifndef POKEBW2_DWC_MATCH_H
#define POKEBW2_DWC_MATCH_H

#include "types.h"
#include "dwc/account.h"
#include "dwc/gp.h"
#include "dwc/gt2Main.h"

// DWC's matchmaking (dwc_match.c, a guessed name), in overlay 11: the friends match control that the game starts, logs
// in and runs every frame, the matching with anybody or with a friend, and the machines of a match, each with its ID
// in the match (its aid). The ROM names none of it; what the functions do is read off their callers

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
void func_ov011_0215fa0c(DWCUserData *userData, int productId, const char *secretKey, int sendBufSize, int recvBufSize,
                         DWCFriendData *friendList, int friendListLen);
// Ends it, freeing everything; and runs it, every frame
void func_ov011_0215fb78(void);
void func_ov011_0215fc50(void);
// Logs in with the player's name, which FALSE says could not start
BOOL func_ov011_0215fdf0(const u16 *name, const char *reserved, void (*callback)(int error), void *param);
// Updates the friend list with the servers
BOOL func_ov011_0215fe94(const char *name, void (*callback)(int error), void *userData,
                         void (*statusCallback)(int index, u8 status, const char *location, void *param),
                         void *statusParam, void (*deleteCallback)(int deleteIndex, int srcIndex, void *param),
                         void *deleteParam);

// Matching: with the key and number, anybody, or a friend, and the callbacks that tell how it went
typedef void (*DWCMatchedFunc)(int error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param);
typedef void (*DWCNewClientFunc)(int index, void *param);
typedef BOOL (*DWCAttemptFunc)(void *param);
BOOL func_ov011_0215fef8(int type, u8 numEntry, const char *query, DWCMatchedFunc matched, void *matchedParam,
                         DWCNewClientFunc newClient, void *newClientParam, DWCEvalFunc eval, void *evalParam,
                         DWCAttemptFunc attempt, int *attemptParam, void *param);
BOOL func_ov011_0215ff7c(int type, u8 numEntry, DWCMatchedFunc matched, void *matchedParam, DWCNewClientFunc newClient,
                         void *newClientParam, DWCAttemptFunc attempt, int *attemptParam, void *param);
BOOL func_ov011_0215fff4(int type, int friendIndex, DWCMatchedFunc matched, void *matchedParam,
                         DWCNewClientFunc newClient, void *newClientParam, DWCAttemptFunc attempt, int *attemptParam,
                         void *param);

// The function called when a connection to another machine has been closed, which FALSE says could not be set
typedef void (*DWCClosedFunc)(int error, BOOL a1, BOOL a2, u8 netId, int index, void *param);
BOOL func_ov011_02160150(DWCClosedFunc func, void *param);
// Closes every connection at once: 0 once closing, 1 when there were none (or matching was canceled), -1 when it can't
int func_ov011_02160170(void);
// Closes the connection to a machine at once: 0 once closed, -1 when it can't, -2 for an unknown machine and -3 when
// this machine isn't the server
int func_ov011_02160284(u8 netId);

// The machines: how many there are, this machine's aid, the list of the aids, the aids as a bitmap, and whether an
// aid is another machine of the match
BOOL func_ov011_021602a0(void);
u8 func_ov011_021602c0(void);
int func_ov011_021602f0(u8 **aids);
int func_ov011_02160344(void);
BOOL func_ov011_02160370(u8 netId);
// Whether this machine is the server of the match (its dwc_hoststate is 2)
BOOL func_ov011_02160408(void);
// Makes the GT2 socket, returning GT2's result
int func_ov011_0216043c(void);
// The GT2 connection to the machine with an aid, the aid of a connection (0xff if it has none) and whether there is a
// connection to an aid
GTI2Connection *func_ov011_021604b0(u8 aid);
u8 func_ov011_021604ec(GTI2Connection *connection);
BOOL func_ov011_02160588(u8 aid);
// Closes the connection to a machine; 0 once closed, -1 when it can't and -2 for an unknown machine
int func_ov011_02160710(u8 aid);
// The server's aid, or the first machine's
u8 func_ov011_02160d6c(void);

// Adds a key with a value that is read when the match runs, a number or a text, and reads a candidate's key
u8 func_ov011_02160e10(u8 keyId, const char *key, const int *valueSrc);
u8 func_ov011_02160ed4(u8 keyId, const char *key, const char *value);
int func_ov011_02160f98(int index, const char *key, int defaultValue);

// Takes a command that came from another machine of the match through the transport
void func_ov011_02162930(u8 aid, int type, const void *data);
// Sets the player's GameSpy Presence status with the match's numbers: its size (SCM), its machines (SCN), the version
// (VER) and whether it is closed (LCK)
int func_ov011_02162d80(int status);
// Takes a packet of the matchmaking that came in through the transport
void func_ov011_0216731c(const u8 *data, int size);
// Whether this machine is the first of the match, its server; and, on the server, sends the others the machines of the
// match every 15 seconds or at once when forced, returning -1 when that failed
BOOL func_ov011_021677e8(void);
int func_ov011_021679c8(BOOL force);
// Whether the match is in one of its states 9 to 11
BOOL func_ov011_02167f3c(void);
// The number of machines of the match, and the one at an index
int func_ov011_02168024(void);
DWCNodeInfo *func_ov011_02168250(int index);
// The LCK value of a friend's GameSpy Presence status: whether the friend's match is closed
u8 func_ov011_021686c0(const GPBuddyStatus *status);
// Sets the player's profile ID, once logged in; and registers the matchmaking's qr2 keys (dwc_pid, dwc_mtype,
// dwc_mver, dwc_eval, dwc_groupid, dwc_hoststate and dwc_suspend)
void func_ov011_02168780(int profileId);
void func_ov011_02168794(void);

#endif // POKEBW2_DWC_MATCH_H
