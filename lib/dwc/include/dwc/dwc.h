#ifndef POKEBW2_DWC_DWC_H
#define POKEBW2_DWC_DWC_H

#include "types.h"
#include "dwc/account.h"
#include "dwc/connectinet.h"
#include "dwc/error.h"
#include "dwc/friend.h"
#include "dwc/gp.h"
#include "dwc/gsMemory.h"
// GameSpy's stats (gstats.c): the game's name, which the server URLs are made from, the secret key and the stats
// server's host, which the start sets
#include "dwc/gstats.h"
#include "dwc/gt2Main.h"
#include "dwc/init.h"
#include "dwc/match.h"
#include "dwc/nasfunc.h"
#include "gfl/heap.h"
#include "nitro/rtc.h"

// Nintendo's Wi-Fi Connection library (NitroDWC), in main and overlay 11. The ROM names none of it; what these
// functions do is read off their callers. The account and friend data are in dwc/account.h, the error in dwc/error.h,
// the start in dwc/init.h and the matchmaking in dwc/match.h

// Starts the login to Nintendo's authentication server (FALSE if its memory can't be had), one of the calls that end
// the connection after a server error
BOOL DWC_NASLoginAsync(void);
// Asks the service locator for the token of a service (an empty one takes "0000") into the buffer, returning FALSE if
// it could not start
BOOL DWC_SVLGetTokenAsync(char *svc, DWCSvlResult *buffer);
// The states of the token's request (a DWCSvlState) and of the login, 3 once done; the request ends after a state from
// 3 to 5. The login's are the SDK's DWCNasLoginState (NitroDWC's base/dwc_nasfunc.h)
typedef enum DWCNasLoginState {
    DWC_NASLOGIN_STATE_DIRTY = 0,
    DWC_NASLOGIN_STATE_IDLE,
    DWC_NASLOGIN_STATE_HTTP,
    DWC_NASLOGIN_STATE_SUCCESS,
    DWC_NASLOGIN_STATE_ERROR,
    DWC_NASLOGIN_STATE_CANCELED,
    DWC_NASLOGIN_STATE_MAX,
} DWCNasLoginState;
DWCSvlState DWC_SVLProcess(void);
DWCNasLoginState DWC_NASLoginProcess(void);

// The server's date and time
BOOL DWC_GetDateTime(RTCDate *date, RTCTime *time);

// The states of the profanity check, and the regions whose word lists it checks: the SDK's DWCProfState and
// DWCProfRegion (NitroDWC's auth/dwc_prof.h)
typedef enum DWCProfState {
    DWC_PROF_STATE_NOT_INITIALIZED = 0,
    DWC_PROF_STATE_OPERATING,
    DWC_PROF_STATE_SUCCESS,
    DWC_PROF_STATE_FAIL,
} DWCProfState;
typedef enum DWCProfRegion {
    DWC_PROF_REGION_SELF = 0x00,
    DWC_PROF_REGION_ALL = 0xff,
} DWCProfRegion;
// Asks the server whether count words are bad, and runs the request, returning 2 once it succeeded
BOOL DWC_CheckProfanityExAsync(const u16 **words, int count, const char *reserved, int timeout, char *result,
                               int *badWordCount, DWCProfRegion region);
DWCProfState DWC_CheckProfanityProcess(void);

// Overlay 11's friend list (friend.c, whose other functions are in dwc/friend.h)
BOOL DWC_SetBuddyFriendCallback(DWCBuddyFriendCallback callback, void *param);
u8 DWC_GetFriendStatusData(const DWCFriendData *friendData, char *statusData, int *size);
BOOL DWC_SetOwnStatusData(const char *statusData, u32 size);

// Overlay 11's transport, which sends to the machines of a match (dwc_transport.c; the rest is in dwc/transport.h)
BOOL DWC_IsSendableReliable(u8 netId);
u32 DWC_SendReliableBitmap(u32 netIds, const void *data, int size);
BOOL DWC_SetRecvBuffer(u8 netId, void *buffer, int size);
BOOL DWC_SetRecvTimeoutTime(u8 netId, u32 timeout);
// The functions called when data has been sent, when it has been received and when a machine has not sent for too
// long, with the parameter they were set with; the setters of the transport's (dwc/transport.h) return FALSE while it
// isn't running
typedef void (*DWCUserSendCallback)(int size, u8 netId, void *param);
typedef void (*DWCUserRecvCallback)(u8 netId, u8 *data, int size, void *param);
typedef void (*DWCUserRecvTimeoutCallback)(u8 netId, void *param);
BOOL DWC_SetUserSendCallback(DWCUserSendCallback func, void *param);
BOOL DWC_SetUserRecvCallback(DWCUserRecvCallback func, void *param);
BOOL DWC_SetUserRecvTimeoutCallback(DWCUserRecvTimeoutCallback func, void *param);

// Overlay 11's, which the library's start in main (dwc_init.c) calls: chooses the servers that the library talks to
// by the kind of start, 0 for development, 1 for retail (it knows a 2, which the start in main never gives), returning
// FALSE for another. The start then gives GameSpy its memory functions (malloc, free, realloc and memalign), with
// dwc/gsMemory.h's gsiMemoryCallbacksSet
BOOL DWCi_Auth_InitInterface(int server);

// Overlay 11's dwc_nasfunc.c (see dwc/nasfunc.h): sets the URL of the Nintendo authentication server, and reads it.
// The settings utility (overlay 182) sets its own for as long as it runs
void DWC_Auth_SetCustomNas(const char *url);
const char *DWCi_Auth_GetCustomNas(void);

#endif // POKEBW2_DWC_DWC_H
