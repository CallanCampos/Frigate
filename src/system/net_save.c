#include "types.h"
#include "system/net_save.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/random.h"
#include "system/game_data.h"

// Saving the game on two machines at once, each waiting for the other between the steps so that both saves end
// together or not at all. The name is the ROM's string.

// The timing numbers of the steps
enum {
    NET_SAVE_TIMING_START = 0x71,
    NET_SAVE_TIMING_WRITTEN,
    NET_SAVE_TIMING_WAITED,
    NET_SAVE_TIMING_END
};

#define NET_SAVE_TIMING_GROUP 8
#define NET_SAVE_WAIT_MAX 60

typedef void (*NetSaveState)(NetSave *save);

struct NetSave {
    void (*func)(void *work);
    void *work;
    NetSaveState state;
    GameData *gameData;
    int wait;
};

static void setState(NetSave *save, NetSaveState state);
static void stateStart(NetSave *save);
static void stateWrite(NetSave *save);
static void stateWritten(NetSave *save);
static void stateWait(NetSave *save);
static void stateWaited(NetSave *save);
static void stateFinish(NetSave *save);
static void stateEnd(NetSave *save);

static void setState(NetSave *save, NetSaveState state) {
    save->state = state;
}

NetSave *func_02012ee4(HeapID heapId, GameData *gameData) {
    NetSave *save = GFL_HeapAllocate(heapId, sizeof(NetSave), TRUE, "net_save.c", 117);

    save->gameData = gameData;
    func_02040624(func_02040440(), NET_SAVE_TIMING_START, NET_SAVE_TIMING_GROUP);
    setState(save, stateStart);
    return save;
}

NetSave *func_02012f1c(HeapID heapId, GameData *gameData, void (*func)(void *work), void *work) {
    NetSave *save = GFL_HeapAllocate(heapId, sizeof(NetSave), TRUE, "net_save.c", 135);

    save->func = func;
    save->work = work;
    save->gameData = gameData;
    func_02040624(func_02040440(), NET_SAVE_TIMING_START, NET_SAVE_TIMING_GROUP);
    setState(save, stateStart);
    return save;
}

BOOL func_02012f5c(NetSave *save) {
    if (save->state != NULL) {
        save->state(save);
    }
    if (GFL_NetErrCheck()) {
        if (save->gameData != NULL) {
            func_02017884(save->gameData);
        }
        return TRUE;
    }
    if (save->state == NULL) {
        return TRUE;
    }
    return FALSE;
}

void func_02012f8c(NetSave *save) {
    GFL_HeapFree(save);
}

static void stateStart(NetSave *save) {
    if (func_02040664(func_02040440(), NET_SAVE_TIMING_START, NET_SAVE_TIMING_GROUP)) {
        func_0201782c(save->gameData);
        setState(save, stateWrite);
    }
}

// Writes all but the end of the save, then waits a random time, so that the two machines don't end together
static void stateWrite(NetSave *save) {
    if (func_02017850(save->gameData) == 1) {
        func_02040624(func_02040440(), NET_SAVE_TIMING_WRITTEN, NET_SAVE_TIMING_GROUP);
        save->wait = GFL_RandomLC(NET_SAVE_WAIT_MAX);
        setState(save, stateWritten);
    }
}

static void stateWritten(NetSave *save) {
    if (func_02040664(func_02040440(), NET_SAVE_TIMING_WRITTEN, NET_SAVE_TIMING_GROUP)) {
        setState(save, stateWait);
    }
}

static void stateWait(NetSave *save) {
    save->wait--;
    if (save->wait < 0) {
        func_02040624(func_02040440(), NET_SAVE_TIMING_WAITED, NET_SAVE_TIMING_GROUP);
        setState(save, stateWaited);
    }
}

static void stateWaited(NetSave *save) {
    if (func_02040664(func_02040440(), NET_SAVE_TIMING_WAITED, NET_SAVE_TIMING_GROUP)) {
        setState(save, stateFinish);
    }
}

static void stateFinish(NetSave *save) {
    if (save->func != NULL) {
        save->func(save->work);
    }
    if (func_02017850(save->gameData) == 2) {
        save->gameData = NULL;
        func_02040624(func_02040440(), NET_SAVE_TIMING_END, NET_SAVE_TIMING_GROUP);
        setState(save, stateEnd);
    }
}

static void stateEnd(NetSave *save) {
    if (func_02040664(func_02040440(), NET_SAVE_TIMING_END, NET_SAVE_TIMING_GROUP)) {
        setState(save, NULL);
    }
}
