#include "types.h"
#include "app/t_download/t_download_local.h"
#include "app/t_download/t_download_save.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/time_icon.h"

// The Pokémon World Tournament's downloaded tournaments, as the save's extra blocks keep them and as the download
// leaves them, named after the ROM's "t_download_save.c". The names are ours

// Which version a tournament is for: only the one the game is, or both
#define T_DOWNLOAD_KIND_WHITE2 0
#define T_DOWNLOAD_KIND_BLACK2 1
#define T_DOWNLOAD_KIND_BOTH 2

// The bit of the work's saveFlags that says save slot i holds a tournament
#define T_DOWNLOAD_SAVE_FLAG_SLOT(i) (1 << ((i) + T_DOWNLOAD_SAVE_FLAG_SLOT_SHIFT))

// The save's extra block of each slot, and where the tournament is in it
typedef struct {
    u32 block;
    u32 offset;
} TDownloadSaveBlock;

static const TDownloadSaveBlock sTDownloadSaveBlocks[T_DOWNLOAD_SAVE_SLOTS] = {
    { 0x11, 0 },
    { 0x12, 0 },
    { 0x13, 0 },
};

static BOOL TDownloadSave_LoadSlot(TDownloadWork *wk, int slot);
static int TDownloadSave_LoadReceived(TDownloadWork *wk);
static BOOL TDownloadSave_IsCaught(TDownloadWork *wk, int species);
static void TDownloadSave_CopyToSlot(TDownloadWork *wk);

int TDownloadSave_LoadSaved(TDownloadWork *wk) {
    int i;
    int count = 0;

    for (i = 0; i < T_DOWNLOAD_SAVE_SLOTS; i++) {
        if (TDownloadSave_LoadSlot(wk, i) == TRUE) {
            wk->saveFlags |= T_DOWNLOAD_SAVE_FLAG_SLOT(i);
            wk->lastSlot = i;
            count++;
        }
    }
    return count;
}

static BOOL TDownloadSave_LoadSlot(TDownloadWork *wk, int slot) {
    SaveControl *save;
    BOOL loaded = FALSE;
    u32 block;
    void *src;

    wk->savedData[slot] = GFL_HeapAllocate(HEAPID_TAIL(wk->heapId), T_DOWNLOAD_TOURNAMENT_SIZE, FALSE,
                                           "t_download_save.c", 138);
    sys_memset32(0, wk->savedData[slot], T_DOWNLOAD_TOURNAMENT_SIZE);
    save = GameData_GetSaveControl(GSYS_GetGameData(wk->param->gsys));
    block = sTDownloadSaveBlocks[slot].block;
    if (func_020074ec(save, block, wk->heapId) == TRUE) {
        src = getAddressOfExtraSaveBlk(save, block, sTDownloadSaveBlocks[slot].offset);
        sys_memcpy32(src, wk->savedData[slot], T_DOWNLOAD_TOURNAMENT_SIZE);
        loaded = TRUE;
    }
    freeIntermediateSaveExtraBlksAfterLoad(save, block);
    return loaded;
}

void TDownloadSave_FreeSaved(TDownloadWork *wk) {
    int i;

    for (i = 0; i < T_DOWNLOAD_SAVE_SLOTS; i++) {
        if (wk->savedData[i] != NULL) {
            GFL_HeapFree(wk->savedData[i]);
            wk->savedData[i] = NULL;
        }
    }
}

int TDownloadSave_LoadDownloaded(TDownloadWork *wk) {
    return TDownloadSave_LoadReceived(wk);
}

void TDownloadSave_LoadString(TDownloadWork *wk, const TDownloadTournament *data, int kind, StrBuf *strbuf) {
    if (kind == 0) {
        GFL_StrBufLoadFixedString(strbuf, data->name, T_DOWNLOAD_NAME_LENGTH);
    } else if (kind == 1) {
        GFL_StrBufLoadFixedString(strbuf, data->author, T_DOWNLOAD_AUTHOR_LENGTH);
    } else if (kind == 2) {
        GFL_StrBufLoadFixedString(strbuf, data->text, T_DOWNLOAD_TEXT_LENGTH);
    }
}

int TDownloadSave_CountEntries(TDownloadWork *wk, const TDownloadTournament *data, int kind) {
    int i, j;
    int count = 0;
    const u8 *bits;

    if (kind == 0) {
        bits = data->species;
        for (i = 0; i < T_DOWNLOAD_SPECIES_BYTES; i++) {
            for (j = 0; j < 8; j++) {
                if (bits[i] & (1 << j)) {
                    if (TDownloadSave_IsCaught(wk, i * 8 + j) == TRUE) {
                        count++;
                    }
                }
            }
        }
    } else if (kind == 1) {
        bits = data->items;
        for (i = 0; i < T_DOWNLOAD_ITEM_BYTES; i++) {
            for (j = 0; j < 8; j++) {
                if (bits[i] & (1 << j)) {
                    count++;
                }
            }
        }
    } else if (kind == 2) {
        for (i = 1; i < 560; i++) {
            if ((u16)i == data->move) {
                count++;
            }
        }
        if (count >= 1) {
            count = 1;
        }
    }
    return count;
}

u32 TDownloadSave_GetEntry(TDownloadWork *wk, const TDownloadTournament *data, int kind, int number) {
    int i, j;
    int count;
    const u8 *bits;
    const u8 *items;

    if (kind == 0) {
        bits = data->species;
        count = 0;
        for (i = 0; i < T_DOWNLOAD_SPECIES_BYTES; i++) {
            for (j = 0; j < 8; j++) {
                if (bits[i] & (1 << j)) {
                    if (TDownloadSave_IsCaught(wk, i * 8 + j) == TRUE) {
                        count++;
                        if (number == count) {
                            return i * 8 + j;
                        }
                    }
                }
            }
        }
    } else if (kind == 1) {
        items = data->items;
        count = 0;
        for (i = 0; i < T_DOWNLOAD_ITEM_BYTES; i++) {
            for (j = 0; j < 8; j++) {
                if (items[i] & (1 << j)) {
                    count++;
                    if (number == count) {
                        return i * 8 + j;
                    }
                }
            }
        }
    } else if (kind == 2) {
        for (i = 1; i < 560; i++) {
            if ((u16)i == data->move) {
                return i;
            }
        }
        return 0;
    }
    return 0;
}

void TDownloadSave_StartSave(TDownloadWork *wk, int src, int dest) {
    GameData *gameData;
    SaveControl *save;
    u32 block;
    TDownloadTournament *data;

    if (!(wk->saveFlags & T_DOWNLOAD_SAVE_FLAG_SAVING)) {
        data = wk->receivedData[src];
        wk->copySrc = src;
        wk->copyDest = dest;
        gameData = GSYS_GetGameData(wk->param->gsys);
        save = GameData_GetSaveControl(gameData);
        block = sTDownloadSaveBlocks[dest].block;
        func_020074ec(save, block, wk->heapId);
        sys_memcpy32(data, getAddressOfExtraSaveBlk(save, block, sTDownloadSaveBlocks[dest].offset),
                     T_DOWNLOAD_TOURNAMENT_SIZE);
        func_020178c4(gameData, block);
        wk->saveFlags |= T_DOWNLOAD_SAVE_FLAG_SAVING;
    }
}

BOOL TDownloadSave_UpdateSave(TDownloadWork *wk) {
    BOOL saving = TRUE;
    GameData *gameData;
    SaveControl *save;
    int dest;
    u32 block;
    u32 state;

    if (!(wk->saveFlags & T_DOWNLOAD_SAVE_FLAG_SAVING)) {
        return FALSE;
    }
    dest = wk->copyDest;
    gameData = GSYS_GetGameData(wk->param->gsys);
    save = GameData_GetSaveControl(gameData);
    block = sTDownloadSaveBlocks[dest].block;
    state = func_020178f4(gameData, block);
    if (state == 2 || state == 3) {
        freeIntermediateSaveExtraBlksAfterLoad(save, block);
        wk->saveFlags &= ~T_DOWNLOAD_SAVE_FLAG_SAVING;
        TDownloadSave_CopyToSlot(wk);
        saving = FALSE;
    }
    return saving;
}

void TDownloadSave_CreateWaitIcon(TDownloadWork *wk) {
    wk->waitIcon = WaitIcon_CreateTCBEx(wk->tcbEx, wk->windows[T_DOWNLOAD_WINDOW_MESSAGE].window, 15, 16, wk->heapId);
}

void TDownloadSave_UpdateWaitIcon(TDownloadWork *wk) {
    if (wk->waitIcon != NULL) {
        WaitIcon_Main(wk->waitIcon);
    }
}

void TDownloadSave_FreeWaitIcon(TDownloadWork *wk) {
    if (wk->waitIcon != NULL) {
        WaitIcon_Free(wk->waitIcon);
        wk->waitIcon = NULL;
    }
}

int TDownloadSave_FindFreeSlot(TDownloadWork *wk) {
    return TDownloadSave_FindSlot(wk, 1);
}

BOOL TDownloadSave_IsSaved(TDownloadWork *wk, int index) {
    BOOL found = FALSE;
    u16 id = wk->receivedData[index]->id;
    s16 i;

    for (i = 0; i < wk->lastSlot + 1; i++) {
        if (wk->saveFlags & T_DOWNLOAD_SAVE_FLAG_SLOT(i)) {
            if (id == wk->savedData[i]->id) {
                found = TRUE;
                break;
            }
        }
    }
    return found;
}

static void TDownloadSave_CopyToSlot(TDownloadWork *wk) {
    int i;
    int count;

    sys_memcpy32(wk->receivedData[wk->copySrc], wk->savedData[wk->copyDest], T_DOWNLOAD_TOURNAMENT_SIZE);
    wk->saveFlags |= T_DOWNLOAD_SAVE_FLAG_SLOT(wk->copyDest);
    count = 0;
    for (i = 0; i < T_DOWNLOAD_SAVE_SLOTS; i++) {
        if (wk->saveFlags & T_DOWNLOAD_SAVE_FLAG_SLOT(i)) {
            wk->lastSlot = i;
            count++;
        }
    }
    if (count > T_DOWNLOAD_SAVE_SLOTS) {
        count = T_DOWNLOAD_SAVE_SLOTS;
    }
    wk->slotCount = count;
}

static BOOL TDownloadSave_IsCaught(TDownloadWork *wk, int species) {
    return PokeDex_IsCaught(GameData_GetPokedex(GSYS_GetGameData(wk->param->gsys)), species);
}

static inline BOOL TDownloadSave_IsKindUsable(u32 kind) {
    switch (kind) {
    case T_DOWNLOAD_KIND_WHITE2:
#ifdef WHITE2
        return TRUE;
#else
        return FALSE;
#endif
    case T_DOWNLOAD_KIND_BLACK2:
#ifdef BLACK2
        return TRUE;
#else
        return FALSE;
#endif
    case T_DOWNLOAD_KIND_BOTH:
        return TRUE;
    }
    return FALSE;
}

static inline BOOL TDownloadSave_IsCRCValid(TDownloadTournament *data) {
    if (getCRC16(data, T_DOWNLOAD_TOURNAMENT_SIZE - 4) == data->crc) {
        return TRUE;
    }
    return FALSE;
}

static inline BOOL TDownloadSave_IsValid(TDownloadTournament *data) {
    if (data->id != 0 && TDownloadSave_IsKindUsable(data->kind) && data->state == 2
        && TDownloadSave_IsCRCValid(data)) {
        return TRUE;
    }
    return FALSE;
}

static int TDownloadSave_LoadReceived(TDownloadWork *wk) {
    int i;
    int count = 0;
    TDownloadTournament *data;

    for (i = 0; i < T_DOWNLOAD_RECEIVED_MAX; i++) {
        data = (TDownloadTournament *)wk->param->unk4 + i;
        if (TDownloadSave_IsValid(data) == TRUE) {
            wk->receivedData[count] = GFL_HeapAllocate(HEAPID_TAIL(wk->heapId), T_DOWNLOAD_TOURNAMENT_SIZE, TRUE,
                                                       "t_download_save.c", 695);
            sys_memcpy32(data, wk->receivedData[count], T_DOWNLOAD_TOURNAMENT_SIZE);
            count++;
        }
    }
    return count;
}

int TDownloadSave_FindSlot(TDownloadWork *wk, int free) {
    int i;

    if (free == 0) {
        for (i = 0; i < T_DOWNLOAD_SAVE_SLOTS; i++) {
            if (wk->saveFlags & T_DOWNLOAD_SAVE_FLAG_SLOT(i)) {
                return i;
            }
        }
    } else {
        for (i = 0; i < T_DOWNLOAD_SAVE_SLOTS; i++) {
            if (!(wk->saveFlags & T_DOWNLOAD_SAVE_FLAG_SLOT(i))) {
                return i;
            }
        }
    }
    return -1;
}

int TDownloadSave_NextSlot(TDownloadWork *wk, BOOL down, int pos) {
    if (down == FALSE) {
        if (pos == 1) {
            if (wk->saveFlags & T_DOWNLOAD_SAVE_FLAG_SLOT(0)) {
                pos = 0;
            }
        } else if (pos == 2) {
            if (wk->saveFlags & T_DOWNLOAD_SAVE_FLAG_SLOT(1)) {
                pos = 1;
            } else if (wk->saveFlags & T_DOWNLOAD_SAVE_FLAG_SLOT(0)) {
                pos = 0;
            }
        }
    } else {
        if (pos == 1) {
            if (wk->saveFlags & T_DOWNLOAD_SAVE_FLAG_SLOT(2)) {
                pos = 2;
            }
        } else if (pos == 0) {
            if (wk->saveFlags & T_DOWNLOAD_SAVE_FLAG_SLOT(1)) {
                pos = 1;
            } else if (wk->saveFlags & T_DOWNLOAD_SAVE_FLAG_SLOT(2)) {
                pos = 2;
            }
        }
    }
    return pos;
}
