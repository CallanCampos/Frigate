#ifndef POKEBW2_FIELD_UNION_APP_H
#define POKEBW2_FIELD_UNION_APP_H

#include "types.h"
#include "gfl/heap.h"
#include "save/player_info.h"
#include "struct_decls.h"

// union_app.c, overlay 69: who has joined an activity in the Union Room, such as a trade or a battle. The parent
// accepts the children that ask to enter, up to a maximum and while entry is open, collects each member's profile,
// sends the members its status, and calls back when a member joins or leaves. Overlay 28 (union_main.c and
// union_comm.c) drives it and handles its commands. No names here are swan's

#define UNION_APP_MEMBER_MAX 5

// Whether others can still enter, set by UnionApp_CloseEntry, UnionApp_OpenEntry and UnionApp_LimitEntry
enum {
    UNION_APP_ENTRY_OPEN,
    UNION_APP_ENTRY_CLOSED,
    UNION_APP_ENTRY_LIMITED,
};

// The part of the work that the parent sends the members (UnionApp_GetStatusSize gives its size)
typedef struct {
    u8 memberMax;
    // Bit per net ID
    u8 joined;
    u8 unk2[2];
} UnionAppStatus;

// A member's profile
typedef struct {
    PlayerInfo info;
    u8 mac[6];
    u8 pad[2];
} UnionAppMember;

typedef void (*UnionAppMemberCallback)(u8 netId, UnionAppMember *member, void *work);

// Every set of members below is a byte with a bit per net ID
struct UnionApp {
    UnionAppStatus status;
    // Those that are allowed to enter and haven't joined yet
    u8 entering;
    u8 unk5[2];
    // Those whose entry is confirmed, to join next update
    u8 confirmed;
    u8 unk8;
    u8 entryMode;
    // Entries accepted while entry is limited, and the most that can be
    u8 entryCount;
    u8 entryLimit;
    // Those to send the status to
    u8 statusSendTo;
    // Those to send this machine's profile to
    u8 profileSendTo;
    // Those whose profile has arrived
    u8 hasProfile;
    // Those waiting for their turn to be handled at 0x10
    u8 queued;
    u8 current;
    u8 currentPending;
    u8 unk12;
    u8 unk13;
    u32 unk14;
    u8 unk18[8];
    BOOL startRequested;
    BOOL starting;
    BOOL started;
    UnionAppMember members[UNION_APP_MEMBER_MAX];
    UnionAppMemberCallback onJoin;
    UnionAppMemberCallback onLeave;
    void *callbackWork;
};

UnionApp *UnionApp_Create(UnionSystem *sys, HeapID heapId, u8 memberMax, const PlayerInfo *info);
void UnionApp_Free(UnionApp *app);
BOOL UnionApp_RequestEntry(UnionApp *app, u8 netId);
void UnionApp_Update(UnionApp *app, UnionSystem *sys);
void UnionApp_Enqueue(UnionApp *app, u8 netId);
void func_ov069_0217cd5c(UnionApp *app);
u8 func_ov069_0217cd64(UnionApp *app);
void UnionApp_RequestSendStatus(UnionApp *app, u8 netId);
void UnionApp_ReceiveStatus(UnionApp *app, const UnionAppStatus *status);
void UnionApp_RequestSendProfile(UnionApp *app, u8 netId);
void UnionApp_SetMemberProfile(UnionApp *app, UnionSystem *sys, u8 netId, UnionAppMember *member);
BOOL UnionApp_HasMembers(UnionApp *app);
BOOL UnionApp_AllProfilesReceived(UnionApp *app);
void UnionApp_ConfirmEntry(UnionApp *app, u8 netId);
void UnionApp_OnLeave(UnionApp *app, u8 netId);
u32 UnionApp_GetStatusSize(void);
void UnionApp_JoinSelf(UnionApp *app);
BOOL UnionApp_IsEntryClosed(UnionApp *app);
void UnionApp_SetCallbacks(UnionApp *app, UnionAppMemberCallback onJoin, UnionAppMemberCallback onLeave, void *work);
BOOL UnionApp_CloseEntry(UnionApp *app);
void UnionApp_OpenEntry(UnionApp *app);
void UnionApp_LimitEntry(UnionApp *app, u8 limit);
void UnionApp_RequestStart(UnionApp *app);
BOOL UnionApp_IsStarted(UnionApp *app);
UnionAppMember *UnionApp_GetMember(UnionApp *app, u8 netId);
u8 UnionApp_GetJoined(UnionApp *app);
u8 UnionApp_CountJoined(UnionApp *app);

#endif // POKEBW2_FIELD_UNION_APP_H
