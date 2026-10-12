#include "field/union_app.h"
#include "field/union_comm.h"
#include "field/union_main.h"
#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/std.h"
#include "nitro/math.h"
#include "save/player_info.h"

static void UnionApp_CheckJoinedLeft(UnionApp *app);
static void UnionApp_SendStatus(UnionApp *app);
static void UnionApp_UpdateQueue(UnionApp *app);
static void UnionApp_SendCurrent(UnionApp *app);
static void UnionApp_SendProfile(UnionApp *app);
static void UnionApp_JoinConfirmed(UnionApp *app);
static void UnionApp_CheckProfileLeft(UnionApp *app, UnionSystem *sys);
static void UnionApp_CheckEnteringLeft(UnionApp *app, UnionSystem *sys);
static void func_ov069_0217cf4c(UnionApp *app);

UnionApp *UnionApp_Create(UnionSystem *sys, HeapID heapId, u8 memberMax, const PlayerInfo *info) {
    UnionAppMember member;
    UnionApp *app = GFL_HeapAllocate(heapId, sizeof(UnionApp), TRUE, "union_app.c", 77);

    if (func_02042bc4() == TRUE) {
        app->status.joined = 3;
    }
    app->status.memberMax = memberMax;
    app->current = 0xff;
    app->currentPending = 0xff;

    sys_memset(&member, 0, sizeof(UnionAppMember));
    member.info = *info;
    func_0207c33c(member.mac);
    UnionApp_SetMemberProfile(app, sys, func_02042a6c(func_02040440()), &member);
    UnionApp_CloseEntry(app);
    func_ov069_0217cf4c(app);
    return app;
}

void UnionApp_Free(UnionApp *app) {
    GFL_HeapFree(app);
}

BOOL UnionApp_RequestEntry(UnionApp *app, u8 netId) {
    u32 joined;

    if (app->entryMode == UNION_APP_ENTRY_CLOSED) {
        return FALSE;
    }
    joined = countOneBits(app->status.joined);
    if (app->entryMode == UNION_APP_ENTRY_LIMITED && app->entryCount + 1U > app->entryLimit) {
        return FALSE;
    }
    if (joined + 1 <= app->status.memberMax) {
        app->entering |= 1 << netId;
        if (app->entryMode == UNION_APP_ENTRY_LIMITED) {
            app->entryCount++;
        }
        return TRUE;
    }
    return FALSE;
}

void UnionApp_Update(UnionApp *app, UnionSystem *sys) {
    if (GFL_NetErrCheck()) {
        return;
    }
    if (app->started == TRUE) {
        return;
    }
    if (app->startRequested == TRUE) {
        UnionComm_RequestLink1(sys);
        app->startRequested = FALSE;
        app->starting = TRUE;
        return;
    }
    if (app->starting == TRUE) {
        if (!UnionComm_IsLinkRequested(sys)) {
            app->starting = FALSE;
            app->started = TRUE;
        }
        return;
    }
    if (func_02042bc4() == TRUE) {
        UnionApp_CheckEnteringLeft(app, sys);
        UnionApp_SendStatus(app);
        UnionApp_CheckJoinedLeft(app);
        UnionApp_UpdateQueue(app);
        UnionApp_SendCurrent(app);
    }
    UnionApp_SendProfile(app);
    UnionApp_JoinConfirmed(app);
    UnionApp_CheckProfileLeft(app, sys);
}

static void UnionApp_CheckJoinedLeft(UnionApp *app) {
    int i;
    int bit;
    u32 connected = app->status.joined;

    if (app->current != 0xff) {
        return;
    }
    for (i = 0; i < UNION_APP_MEMBER_MAX; i++) {
        bit = 1 << i;
        if ((app->status.joined & bit) && !func_02042a80((u8)i)) {
            connected ^= bit;
        }
    }
    if (connected == app->status.joined) {
        return;
    }
    for (i = 0; i < UNION_APP_MEMBER_MAX; i++) {
        bit = 1 << i;
        if ((app->status.joined & bit) && !(connected & bit) && func_ov028_0217196c(connected, i) == TRUE) {
            UnionApp_OnLeave(app, i);
            app->status.joined ^= bit;
        }
    }
}

static void UnionApp_SendStatus(UnionApp *app) {
    if (app->statusSendTo != 0 && func_ov028_02171724(&app->status, app->statusSendTo) == TRUE) {
        app->statusSendTo = 0;
    }
}

static void UnionApp_UpdateQueue(UnionApp *app) {
    int i;
    int bit;
    u8 queued;

    if (app->current != 0xff) {
        if (func_02042a80(app->current)) {
            return;
        }
        app->current = 0xff;
        app->currentPending = 0xff;
    }
    queued = app->queued;
    if (queued == 0) {
        return;
    }
    for (i = 0; i < UNION_APP_MEMBER_MAX; i++) {
        bit = 1 << i;
        if (queued & bit) {
            app->current = i;
            app->currentPending = i;
            app->queued ^= bit;
            return;
        }
    }
}

static void UnionApp_SendCurrent(UnionApp *app) {
    if (app->currentPending != 0xff && func_ov028_02171664(app->currentPending) == TRUE) {
        app->currentPending = 0xff;
    }
}

static void UnionApp_SendProfile(UnionApp *app) {
    if (app->profileSendTo != 0 &&
        func_ov028_0217181c(app->profileSendTo, &app->members[func_02042a6c(func_02040440())]) == TRUE) {
        app->profileSendTo = 0;
    }
}

static void UnionApp_JoinConfirmed(UnionApp *app) {
    int i;
    int bit;

    if (app->confirmed == 0) {
        return;
    }
    for (i = 0; i < UNION_APP_MEMBER_MAX; i++) {
        bit = 1 << i;
        if (app->confirmed & bit) {
            if ((app->hasProfile & bit) && (app->entering & bit)) {
                app->entering ^= bit;
                app->status.joined |= bit;
                if (app->onJoin != NULL) {
                    app->onJoin(i, &app->members[i], app->callbackWork);
                }
                app->current = 0xff;
            }
            app->confirmed ^= bit;
        }
    }
}

static void UnionApp_CheckProfileLeft(UnionApp *app, UnionSystem *sys) {
    int i;
    int bit;

    for (i = 0; i < UNION_APP_MEMBER_MAX; i++) {
        bit = 1 << i;
        if ((app->hasProfile & bit) && !func_02042a80((u8)i)) {
            if (func_02042bc4() == TRUE || (app->status.joined & bit)) {
                if (app->onLeave != NULL) {
                    app->onLeave(i, &app->members[i], app->callbackWork);
                }
                app->status.joined &= 0xff ^ bit;
                app->hasProfile &= 0xff ^ bit;
            }
            UnionGroup_RemoveMember(&sys->situ.mycomm, app->members[i].mac);
        }
    }
}

static void UnionApp_CheckEnteringLeft(UnionApp *app, UnionSystem *sys) {
    u8 i;
    int bit;

    for (i = 0; i < UNION_APP_MEMBER_MAX; i++) {
        bit = 1 << i;
        if ((app->entering & bit) && !func_02042a80(i)) {
            app->entering ^= bit;
            if (app->hasProfile & bit) {
                app->hasProfile &= 0xff ^ bit;
                UnionGroup_RemoveMember(&sys->situ.mycomm, app->members[i].mac);
            }
            if (app->entryMode == UNION_APP_ENTRY_LIMITED && app->entryCount != 0) {
                app->entryCount--;
            }
        }
    }
}

void UnionApp_Enqueue(UnionApp *app, u8 netId) {
    app->queued |= 1 << netId;
}

void func_ov069_0217cd5c(UnionApp *app) {
    app->unk12 = TRUE;
}

u8 func_ov069_0217cd64(UnionApp *app) {
    return app->unk12;
}

void UnionApp_RequestSendStatus(UnionApp *app, u8 netId) {
    app->statusSendTo |= 1 << netId;
}

void UnionApp_ReceiveStatus(UnionApp *app, const UnionAppStatus *status) {
    sys_memcpy(status, &app->status, sizeof(UnionAppStatus));
}

void UnionApp_RequestSendProfile(UnionApp *app, u8 netId) {
    app->profileSendTo |= 1 << netId;
}

void UnionApp_SetMemberProfile(UnionApp *app, UnionSystem *sys, u8 netId, UnionAppMember *member) {
    UnionSelf *self = &sys->situ;

    app->members[netId] = *member;
    app->hasProfile |= 1 << netId;
    if (netId != func_02042a6c(func_02040440())) {
        UnionGroup_AddMember(&self->mycomm, member->mac, func_02008bf4(&member->info), getTrainerGender(&member->info));
    }
}

BOOL UnionApp_HasMembers(UnionApp *app) {
    if (app->status.joined != 0) {
        return TRUE;
    }
    return FALSE;
}

BOOL UnionApp_AllProfilesReceived(UnionApp *app) {
    int i;
    int bit;

    for (i = 0; i < UNION_APP_MEMBER_MAX; i++) {
        bit = 1 << i;
        if ((app->status.joined & bit) && func_02042a80((u8)i) == TRUE && !(app->hasProfile & bit)) {
            return FALSE;
        }
    }
    return TRUE;
}

void UnionApp_ConfirmEntry(UnionApp *app, u8 netId) {
    if (func_02042bc4() == TRUE) {
        int bit = 1 << netId;

        if ((app->hasProfile & bit) && (app->entering & bit)) {
            app->confirmed |= bit;
        }
    } else {
        app->entering |= 1 << netId;
        app->confirmed |= 1 << netId;
    }
}

void UnionApp_OnLeave(UnionApp *app, u8 netId) {
}

u32 UnionApp_GetStatusSize(void) {
    return sizeof(UnionAppStatus);
}

void UnionApp_JoinSelf(UnionApp *app) {
    app->status.joined |= 1 << func_02042a6c(func_02040440());
}

BOOL UnionApp_IsEntryClosed(UnionApp *app) {
    if (app == NULL) {
        return FALSE;
    }
    switch (app->entryMode) {
    case UNION_APP_ENTRY_OPEN:
        return FALSE;
    case UNION_APP_ENTRY_CLOSED:
        return TRUE;
    case UNION_APP_ENTRY_LIMITED:
        if (app->entryCount >= app->entryLimit) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

void UnionApp_SetCallbacks(UnionApp *app, UnionAppMemberCallback onJoin, UnionAppMemberCallback onLeave, void *work) {
    app->onJoin = onJoin;
    app->onLeave = onLeave;
    app->callbackWork = work;
}

BOOL UnionApp_CloseEntry(UnionApp *app) {
    if (app->entering == 0) {
        func_02042a9c(func_02040440(), FALSE);
        app->entryMode = UNION_APP_ENTRY_CLOSED;
        return TRUE;
    }
    return FALSE;
}

void UnionApp_OpenEntry(UnionApp *app) {
    func_02042a9c(func_02040440(), TRUE);
    app->entryMode = UNION_APP_ENTRY_OPEN;
}

void UnionApp_LimitEntry(UnionApp *app, u8 limit) {
    if (func_02042788()) {
        func_02042a9c(func_02040440(), TRUE);
        app->entryMode = UNION_APP_ENTRY_LIMITED;
        app->entryLimit = limit;
        app->entryCount = 0;
    }
}

static void func_ov069_0217cf4c(UnionApp *app) {
    app->unk14 = TRUE;
}

void UnionApp_RequestStart(UnionApp *app) {
    app->startRequested = TRUE;
}

BOOL UnionApp_IsStarted(UnionApp *app) {
    return app->started;
}

UnionAppMember *UnionApp_GetMember(UnionApp *app, u8 netId) {
    int bit = 1 << netId;

    if (!(app->hasProfile & bit) || !(app->status.joined & bit)) {
        return NULL;
    }
    return &app->members[netId];
}

u32 UnionApp_GetJoined(UnionApp *app) {
    return app->status.joined;
}

u8 UnionApp_CountJoined(UnionApp *app) {
    return countOneBits(app->status.joined);
}
