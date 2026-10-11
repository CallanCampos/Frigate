#include "types.h"
#include "field/field.h"
#include "field/game_beacon_search.h"
#include "field/musical.h"
#include "field/union_comm.h"
#include "field/union_main.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/player_info.h"
#include "system/game_comm.h"
#include "system/game_data.h"

// The game's communication system: one at a time, it boots a communication (a GameCommNo), runs its callbacks through
// a sequence of states, and ends it on request. It also logs the last players met over a communication

enum {
    GAME_COMM_SEQ_INIT,
    GAME_COMM_SEQ_INIT_WAIT,
    GAME_COMM_SEQ_MAIN,
    GAME_COMM_SEQ_EXIT,
    GAME_COMM_SEQ_EXIT_WAIT,
    GAME_COMM_SEQ_END,
};

typedef struct {
    // Set to 0 at each change of seq; the callbacks get it as their sequence
    u32 subSeq;
    u8 seq;
    u8 changed;
} GameCommSeq;

// A player met over a communication
typedef struct {
    u16 beaconType;
    u8 netId;
    u8 valid : 1;
    u8 gender1 : 1;
    u8 gender2 : 1;
    StrBuf *name1;
    StrBuf *name2;
} GameCommLogEntry;

// A ring of the last players met
typedef struct {
    GameCommLogEntry entries[GAME_COMM_LOG_COUNT];
    u8 start;
    u8 end;
} GameCommLog;

struct GameCommSys {
    GameData *gameData;
    // An exit requested while booting
    u8 exitReq : 1;
    u8 exitReqAgain : 1;
    u8 unkFlag2 : 1;
    u8 unkFlag3 : 1;
    u8 commNo;
    u8 lastCommNo;
    u32 unk8;
    GameCommSeq seq;
    void *param;
    void *work;
    void *exitCallbackArg;
    GameCommExitCallback exitCallback;
    u8 unk24[0x18];
    GameCommLog log;
};

// A communication's callbacks. param is what it was booted with, and work is what init returned
typedef struct {
    void *(*init)(u32 *seq, void *param);
    BOOL (*initWait)(u32 *seq, void *param, void *work);
    void (*main)(u32 *seq, void *param, void *work);
    BOOL (*exit)(u32 *seq, void *param, void *work);
    BOOL (*exitWait)(u32 *seq, void *param, void *work);
    void (*fieldCreate)(void *param, void *work, Field *field);
    void (*fieldDelete)(void *param, void *work, Field *field);
} GameCommFuncs;

static void GameCommSys_ExitReqCallback(GameCommSys *comm, GameCommExitCallback callback, void *callbackArg);
static void GameComm_SetSeq(GameCommSeq *seq, u8 next);
static void GameCommSys_ClearUnk24(GameCommSys *comm);
static void GameCommSys_LogPlayers(GameCommSys *comm, u32 netId1, u32 netId2, u32 beaconType);

static const GameCommFuncs sGameCommFuncs[GAME_COMM_NO_MAX] = {
    [GAME_COMM_NO_NULL] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    [GAME_COMM_NO_BEACON_SEARCH] = {
        func_ov012_0215f55c,
        func_ov012_0215f5b4,
        func_ov012_0215f654,
        func_ov012_0215f610,
        func_ov012_0215f628,
        NULL,
        NULL,
    },
    [GAME_COMM_NO_UNK2] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    [GAME_COMM_NO_UNION] = {
        UnionComm_Init,
        UnionComm_InitWait,
        UnionComm_Main,
        UnionComm_Exit,
        UnionComm_ExitWait,
        UnionMain_OnFieldIn,
        UnionMain_OnFieldOut,
    },
    [GAME_COMM_NO_MUSICAL] = {
        func_ov211_021ef230,
        NULL,
        func_ov211_021ef394,
        func_ov211_021ef288,
        func_ov211_021ef378,
        NULL,
        NULL,
    },
    [GAME_COMM_NO_UNK5] = {
        func_ov012_0215f55c,
        func_ov012_0215f5b4,
        func_ov012_0215f654,
        func_ov012_0215f610,
        func_ov012_0215f628,
        NULL,
        NULL,
    },
};

GameCommSys *GameCommSys_Create(HeapID heapId, GameData *gameData) {
    GameCommSys *comm = GFL_HeapAllocate(heapId, sizeof(GameCommSys), TRUE, "game_comm.c", 236);
    int i;
    PlayerInfo *info;
    GameCommLog *log;
    GameCommLogEntry *entry;

    comm->gameData = gameData;
    info = GetGameDataPlayerInfo(gameData);
    log = &comm->log;
    for (i = 0; i < GAME_COMM_LOG_COUNT; i++) {
        entry = &log->entries[i];
        entry->name1 = GFL_StrBufCreate(8, heapId);
        entry->name2 = GFL_StrBufCreate(8, heapId);
        textCopy(info->name, entry->name1);
        textCopy(info->name, entry->name2);
    }
    return comm;
}

void FreeGameComm(GameCommSys *comm) {
    int i;
    GameCommLog *log = &comm->log;
    GameCommLogEntry *entry;

    for (i = 0; i < GAME_COMM_LOG_COUNT; i++) {
        entry = &log->entries[i];
        GFL_StrBufFree(entry->name1);
        GFL_StrBufFree(entry->name2);
    }
    GFL_HeapFree(comm);
}

void GameCommSys_Main(GameCommSys *comm) {
    GameCommSeq *seq;
    const GameCommFuncs *funcs;
    GameCommExitCallback callback;
    void *callbackArg;
    BOOL exitReqAgain;

    if (comm == NULL || comm->commNo == GAME_COMM_NO_NULL) {
        return;
    }
    seq = &comm->seq;
    funcs = &sGameCommFuncs[comm->commNo];
    if (seq->changed == TRUE) {
        seq->subSeq = 0;
        seq->changed = FALSE;
    }
    switch (seq->seq) {
    case GAME_COMM_SEQ_INIT:
        if (funcs->init != NULL) {
            comm->work = funcs->init(&seq->subSeq, comm->param);
        }
        comm->lastCommNo = comm->commNo;
        comm->unk8 = 0;
        GameComm_SetSeq(seq, GAME_COMM_SEQ_INIT_WAIT);
        break;
    case GAME_COMM_SEQ_INIT_WAIT:
        if (funcs->initWait == NULL || funcs->initWait(&seq->subSeq, comm->param, comm->work) == TRUE) {
            GameComm_SetSeq(seq, GAME_COMM_SEQ_MAIN);
        }
        break;
    case GAME_COMM_SEQ_MAIN:
        if (comm->exitReq == TRUE) {
            comm->exitReq = FALSE;
            GameComm_SetSeq(&comm->seq, GAME_COMM_SEQ_EXIT);
        } else {
            funcs->main(&seq->subSeq, comm->param, comm->work);
        }
        break;
    case GAME_COMM_SEQ_EXIT:
        if (funcs->exit != NULL) {
            if (funcs->exit(&seq->subSeq, comm->param, comm->work) == TRUE) {
                GameComm_SetSeq(seq, GAME_COMM_SEQ_EXIT_WAIT);
            }
        } else {
            GameComm_SetSeq(seq, GAME_COMM_SEQ_EXIT_WAIT);
        }
        break;
    case GAME_COMM_SEQ_EXIT_WAIT:
        if (funcs->exitWait != NULL && funcs->exitWait(&seq->subSeq, comm->param, comm->work) != TRUE) {
            break;
        }
        GameComm_SetSeq(seq, GAME_COMM_SEQ_END);
        // fallthrough
    case GAME_COMM_SEQ_END:
        comm->work = NULL;
        comm->commNo = GAME_COMM_NO_NULL;
        callback = comm->exitCallback;
        callbackArg = comm->exitCallbackArg;
        exitReqAgain = comm->exitReqAgain;
        comm->exitCallback = NULL;
        comm->exitCallbackArg = NULL;
        comm->exitReqAgain = FALSE;
        if (callback != NULL) {
            callback(callbackArg, exitReqAgain);
        }
        break;
    }
}

void GameCommSys_FieldCreate(GameCommSys *comm, Field *field) {
    void (*fieldCreate)(void *param, void *work, Field *field);

    if (comm->seq.seq != GAME_COMM_SEQ_END) {
        fieldCreate = sGameCommFuncs[comm->commNo].fieldCreate;
        if (fieldCreate != NULL) {
            fieldCreate(comm->param, comm->work, field);
        }
    }
}

void GameCommSys_FieldDelete(GameCommSys *comm, Field *field) {
    void (*fieldDelete)(void *param, void *work, Field *field);

    if (comm->seq.seq != GAME_COMM_SEQ_END) {
        fieldDelete = sGameCommFuncs[comm->commNo].fieldDelete;
        if (fieldDelete != NULL) {
            fieldDelete(comm->param, comm->work, field);
        }
    }
}

void GameCommSys_Boot(GameCommSys *comm, u8 commNo, void *param) {
    comm->param = param;
    comm->exitReqAgain = FALSE;
    comm->commNo = commNo;
    comm->exitCallback = NULL;
    comm->exitCallbackArg = NULL;
    sys_memset(&comm->seq, 0, sizeof(GameCommSeq));
    GameCommSys_ClearUnk24(comm);
}

void GameCommSys_ExitReq(GameCommSys *comm) {
    GameCommSys_ExitReqCallback(comm, NULL, NULL);
}

static void GameCommSys_ExitReqCallback(GameCommSys *comm, GameCommExitCallback callback, void *callbackArg) {
    if (comm->seq.seq == GAME_COMM_SEQ_MAIN) {
        GameComm_SetSeq(&comm->seq, GAME_COMM_SEQ_EXIT);
        comm->exitCallback = callback;
        comm->exitCallbackArg = callbackArg;
    } else if (comm->seq.seq < GAME_COMM_SEQ_MAIN) {
        // Still booting: GameCommSys_Main exits when it reaches GAME_COMM_SEQ_MAIN
        comm->exitReq = TRUE;
        comm->exitCallback = callback;
        comm->exitCallbackArg = callbackArg;
    } else {
        comm->exitReqAgain = TRUE;
        if (comm->exitCallback == NULL) {
            comm->exitCallback = callback;
            comm->exitCallbackArg = callbackArg;
        }
    }
}

u8 GameCommSys_BootCheck(GameCommSys *comm) {
    if (comm->commNo != GAME_COMM_NO_NULL) {
        return comm->commNo;
    }
    return GAME_COMM_NO_NULL;
}

BOOL GameCommSys_IsTransitioning(GameCommSys *comm) {
    if (comm->commNo != GAME_COMM_NO_NULL && comm->seq.seq != GAME_COMM_SEQ_MAIN) {
        return TRUE;
    }
    return FALSE;
}

void *GameCommSys_GetWork(GameCommSys *comm) {
    return comm->work;
}

GameData *getBasePlayerBlk(GameCommSys *comm) {
    return comm->gameData;
}

u8 GameCommSys_GetLastCommNo(GameCommSys *comm) {
    return comm->lastCommNo;
}

void func_0202be00(GameCommSys *comm) {
    comm->unk8 = 0;
}

u32 func_0202be08(GameCommSys *comm) {
    return comm->unk8;
}

static void GameComm_SetSeq(GameCommSeq *seq, u8 next) {
    seq->seq = next;
    seq->changed = TRUE;
}

void func_0202be14(GameCommSys *comm, BOOL flag) {
    comm->unkFlag2 = flag;
}

void func_0202be28(GameCommSys *comm, BOOL flag) {
    comm->unkFlag3 = flag;
}

BOOL func_0202be3c(GameCommSys *comm) {
    if (comm->unkFlag2 || comm->unkFlag3) {
        return TRUE;
    }
    return FALSE;
}

static void GameCommSys_ClearUnk24(GameCommSys *comm) {
    sys_memset(comm->unk24, 0, sizeof(comm->unk24));
}

static void GameCommSys_LogPlayers(GameCommSys *comm, u32 netId1, u32 netId2, u32 beaconType) {
    GameCommLog *log = &comm->log;
    PlayerInfo *info1;
    PlayerInfo *info2;
    GameCommLogEntry *entry;

    if (netId1 == func_020175cc(comm->gameData)) {
        info1 = GetGameDataPlayerInfo(comm->gameData);
    } else {
        info1 = func_02017378(comm->gameData, netId1);
    }
    if (netId2 == func_020175cc(comm->gameData)) {
        info2 = GetGameDataPlayerInfo(comm->gameData);
    } else {
        info2 = func_02017378(comm->gameData, netId2);
    }
    if (func_02008b5c(info1) == TRUE || func_02008b5c(info2) == TRUE) {
        return;
    }

    entry = &log->entries[log->end];
    if (entry->valid == TRUE) {
        // The log is full: the oldest entry is overwritten
        log->start++;
    }
    entry->netId = netId1;
    entry->beaconType = beaconType;
    entry->valid = TRUE;
    textCopy(info1->name, entry->name1);
    entry->gender1 = getTrainerGender(info1);
    textCopy(info2->name, entry->name2);
    entry->gender2 = getTrainerGender(info2);
    log->end++;
    if (log->end >= GAME_COMM_LOG_COUNT) {
        log->end = 0;
    }
    if (log->start >= GAME_COMM_LOG_COUNT) {
        log->start = 0;
    }
}

void func_0202bf68(GameCommSys *comm, u32 netId1, u32 netId2) {
    u16 beaconType = netId1 == netId2 ? 0x27 : 0x26;

    GameCommSys_LogPlayers(comm, netId1, netId2, beaconType);
}

void func_0202bf7c(GameCommSys *comm) {
    u32 netId = func_020175cc(comm->gameData);

    GameCommSys_LogPlayers(comm, netId, netId, 0x37);
}
