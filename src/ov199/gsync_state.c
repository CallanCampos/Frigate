// Game Sync itself: it asks the server for the account's state, sends the Pokémon to the Dream World or gets it back
// with what it brought, downloads the C-Gear and Pokédex skins and the musical the Dream World sent, and saves. The
// name is the ROM's string, from GFL_HeapAllocate's calls and an assert. Function names are ours.

#include "types.h"
#include "app/gsync.h"
#include "app/gsync/gsync_disp.h"
#include "app/gsync/gsync_download.h"
#include "app/gsync/gsync_message.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "gfl/nhttp_rap.h"
#include "gfl/clact.h"
#include "gfl/dwc_rap.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/rtc_cache.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/touchpanel.h"
#include "nitro/hw.h"
#include "nitro/rtc.h"
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "save/adventure.h"
#include "save/box.h"
#include "save/dream_world.h"
#include "save/join_avenue.h"
#include "save/medal_box.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/app_taskmenu.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/tpoke_data.h"
#include "system/wipe.h"

#define HEAPID_GSYNC 0x62

// Errors of Game Sync's own, beside the server's
#define GSYNC_ERROR_DOWNLOAD 0xfff0
#define GSYNC_ERROR_SERVICE_UNAVAILABLE 0xfff1
#define GSYNC_ERROR_BAD_GATEWAY 0xfff2
#define GSYNC_ERROR_HTTP 0xfff3

// The kinds of file Game Sync downloads
enum {
    GSYNC_DOWNLOAD_CGEAR,
    GSYNC_DOWNLOAD_MUSICAL,
    GSYNC_DOWNLOAD_ZUKAN,
};

// The requests Game Sync makes of the server, by their URLs in overlay 189's table
enum {
    GSYNC_REQUEST_PLAY_STATUS,         // account.playstatus
    GSYNC_REQUEST_SPECIES_FLAGS,       // sleepily.bitlist, the species that can be sent
    GSYNC_REQUEST_DOWNLOAD,            // savedata.download
    GSYNC_REQUEST_UPLOAD,              // savedata.upload
    GSYNC_REQUEST_DOWNLOAD_FINISH = 8, // savedata.download.finish
    GSYNC_REQUEST_CREATE = 9,          // account.create.upload
};

// The largest item ID the Dream World sends
#define GSYNC_ITEM_MAX 638

// The sizes of the downloaded skins, before their CRC
#define GSYNC_CGEAR_SKIN_SIZE 0x2600
#define GSYNC_ZUKAN_SKIN_SIZE 0x6200
#define GSYNC_MUSICAL_SIZE_MAX 0x17bf0

typedef struct GSyncWork GSyncWork;
typedef void (*GSyncStateFunc)(GSyncWork *wk);

// A Pokémon the Dream World sends to the Entree Forest
typedef struct {
    u16 species;
    u16 unk2;
    u8 form;
    u8 sex;
    u8 slot;
    u8 unk7;
} GSyncResultPokemon;

// Someone the Dream World sends to the Join Avenue
typedef struct {
    u16 name[8];
    u32 id;
    s32 profileId;
    u8 country;
    u8 region;
    u8 unk1A;
    u8 unk1B;
    u8 gender;
    u8 unk1D;
    u16 species;
} GSyncResultPerson;

// What the Dream World sends back with the Pokémon
typedef struct {
    u32 id;
    GSyncResultPokemon pokemon[10];
    u16 levels;
    u8 unk56;
    u8 musical;
    u8 cgear;
    u8 zukan;
    u8 unk5A;
    u8 unk5B;
    u16 items[20];
    u8 itemCounts[20];
    u8 entries[5][26];
    u8 unk11A[2];
    GSyncResultPerson people[12];
    u8 unk29C[4];
} GSyncResult;

// The server's answer: its status, and after the header the body of the request's answer
typedef struct {
    u32 status;
    u8 unk4[0x7c];
    union {
        GSyncResult result;
        GSyncAccountInfo accountInfo;
        u8 sendableSpecies[0x80];
    } body;
} GSyncResponse;

struct GSyncWork {
    HeapID heapId;
    u8 request[4];
    GSyncDownload *download;
    PartyPkm *pkm;
    NHttpRap *http;
    GSyncDisp *disp;
    GSyncMessage *msg;
    AppTaskMenu *yesNo;
    EventGameSync *param;
    BoxSaveAccessor *boxes;
    SaveControl *save;
    GameData *gameData;
    void *musical;
    void *dreamWorldCopy;
    GSyncResult *result;
    void *zukanSkin;
    void *cgearSkin;
    void *unk44;
    int boxTray;
    int boxPos;
    GSyncStateFunc state;
    u8 unk54[0xc];
    int timer;
    int seTimer;
    int idleProgress;
    int levels;
    int progress;
    int frames;
    int wait;
    u32 resultId;
    int error;
    u8 unk84[0x20];
    u8 getMusical;
    u8 getCGear;
    u8 getZukan;
    u8 downloadKind;
    u8 downloadFlags;
    u8 hasItems;
    u8 pkmTaken;
    u8 receiving;
    // Whether the Game Sync ID was shown before, from the save, and in this session
    u8 idShownSaved;
    u8 saving;
    u8 busy;
    u8 idShown;
};

static BOOL GSync_CheckHttpStatus(GSyncWork *wk, int status);
static BOOL GSync_IsOnline(GSyncWork *wk);
static BOOL GSync_IsNewAccount(GSyncWork *wk);
static BOOL GSync_HasAccountPokemon(GSyncWork *wk);
static void GSync_SetState(GSyncWork *wk, GSyncStateFunc state);
static void GSync_ChangeState(GSyncWork *wk, GSyncStateFunc state, int line);
static void GSync_StateWaitFadeOut(GSyncWork *wk);
static void GSync_StateFadeOut(GSyncWork *wk);
static void GSync_StateWaitCleanup(GSyncWork *wk);
static void GSync_StateErrorWait(GSyncWork *wk);
static void GSync_StateError(GSyncWork *wk);
static void GSync_StateEnd(GSyncWork *wk);
static void GSync_StateWaitDownloadsShown(GSyncWork *wk);
static void GSync_StateShowDownloads(GSyncWork *wk);
static void GSync_StateWaitLevelUp(GSyncWork *wk);
static void GSync_StateWaitReceived(GSyncWork *wk);
static void GSync_OnPokemonAnimEnd(u32 param, fx32 frame);
static void GSync_ReceivePokemon(GSyncWork *wk);
static void GSync_StateWaitReceiveSave(GSyncWork *wk);
static void GSync_StateWaitFinishDownload(GSyncWork *wk);
static void GSync_StateFinishDownload(GSyncWork *wk);
static void GSync_StateWaitReceiveSaveStart(GSyncWork *wk);
static void GSync_StateReceive(GSyncWork *wk);
static void GSync_StateCleanupDownload(GSyncWork *wk);
static void GSync_StateCGearSaved(GSyncWork *wk);
static void GSync_StateWaitCGearSave(GSyncWork *wk);
static void GSync_StateCGearDownloaded(GSyncWork *wk);
static void GSync_StateZukanSaved(GSyncWork *wk);
static void GSync_StateWaitZukanSave(GSyncWork *wk);
static void GSync_StateZukanDownloaded(GSyncWork *wk);
static void GSync_StateWaitMusicalImport(GSyncWork *wk);
static void GSync_MusicalDownloaded(GSyncWork *wk, int size);
static void GSync_StateWaitDownloadCleanup(GSyncWork *wk);
static void GSync_StateDownloaded(GSyncWork *wk);
static void GSync_StateWaitFile(GSyncWork *wk);
static void GSync_StateGetFile(GSyncWork *wk);
static void GSync_StateWaitFileList(GSyncWork *wk);
static void GSync_StateGetFileList(GSyncWork *wk);
static void GSync_StateSetAttr(GSyncWork *wk);
static void GSync_StateStartDownload(GSyncWork *wk);
static void GSync_StateNextDownload(GSyncWork *wk);
static void GSync_SetEntries(DreamWorldSave *dreamWorld, GSyncResult *result);
static BOOL GSync_SetItems(DreamWorldSave *dreamWorld, GSyncResult *result);
static BOOL GSync_HasItems(DreamWorldSave *dreamWorld, GSyncResult *result);
static void GSync_AddForestPokemon(GSyncWork *wk, DreamWorldSave *dreamWorld, int species, int sex, int form, int unk2,
                                   int slot);
static void GSync_AddForestPokemonList(GSyncWork *wk, DreamWorldSave *dreamWorld, GSyncResult *result);
static void GSync_SetFloatItems(GSyncWork *wk, GSyncResult *result);
static void GSync_ReadResult(GSyncWork *wk, DreamWorldSave *dreamWorld, GSyncResult *result, GSyncResponse *response);
static BOOL GSync_AreAccountItemsUnique(GSyncWork *wk);
static void GSync_StateWaitResult(GSyncWork *wk);
static void GSync_StateRequestResult(GSyncWork *wk);
static void GSync_StateWaitWakeMessage(GSyncWork *wk);
static void GSync_StateWake(GSyncWork *wk);
static void GSync_StateWaitTooSoon(GSyncWork *wk);
static void GSync_StateTooSoon(GSyncWork *wk);
static void GSync_StateWaitNoRoomMessage(GSyncWork *wk);
static void GSync_StateNoRoom(GSyncWork *wk);
static void GSync_StateWakeAnswer(GSyncWork *wk);
static void GSync_StateWakeAsk(GSyncWork *wk);
static void GSync_StateWakeShow(GSyncWork *wk);
static void GSync_StateWaitAccountMessage(GSyncWork *wk);
static void GSync_StateShowIdNext(GSyncWork *wk);
static void GSync_StateShowIdWait(GSyncWork *wk);
static void GSync_StateShowId(GSyncWork *wk);
static void GSync_StateWaitCreate(GSyncWork *wk);
static void GSync_StateCreate(GSyncWork *wk);
static void GSync_StateWaitNotDreamingMessage(GSyncWork *wk);
static void GSync_StateNotDreaming(GSyncWork *wk);
static void GSync_StateWaitDreamingNowMessage(GSyncWork *wk);
static void GSync_StateDreamingNow(GSyncWork *wk);
static void GSync_HandleAccount(GSyncWork *wk, u16 status, GSyncResponse *response);
static void GSync_StateWaitAccount(GSyncWork *wk);
static void GSync_StateRequestAccount(GSyncWork *wk);
static void GSync_StateWaitNoPokemonMessage(GSyncWork *wk);
static void GSync_StateNoPokemon(GSyncWork *wk);
static void GSync_StateWaitSpeciesFlags(GSyncWork *wk);
static void GSync_StateCheckDate(GSyncWork *wk);
static void GSync_StateWaitSentMessage(GSyncWork *wk);
static void GSync_StateWaitSentText(GSyncWork *wk);
static void GSync_StateSent(GSyncWork *wk);
static void GSync_StateWaitSendSave(GSyncWork *wk);
static void GSync_StateSendSave(GSyncWork *wk);
static void GSync_StateWaitUpload(GSyncWork *wk);
static void GSync_StateUpload(GSyncWork *wk);
static void GSync_StateSendAnimText(GSyncWork *wk);
static void GSync_StateSendAnimLeave(GSyncWork *wk);
static void GSync_StateSendAnimJump(GSyncWork *wk);
static void GSync_StateSendAnimIcon(GSyncWork *wk);
static void GSync_StateSendShowIcon(GSyncWork *wk);
static void GSync_StateSend(GSyncWork *wk);
static void GSync_OnDisconnect(void *work, int a1, int code, int error);
static void GSync_RestoreDreamWorld(GSyncWork *wk);
static BOOL GSync_PersonExists(GSyncResultPerson *person);
static void GSync_PersonToPlayerInfo(GSyncResultPerson *person, PlayerInfo *info);
static void GSync_AddJoinAvenuePeople(GameData *gameData, GSyncResultPerson *people, HeapID heapId);
static BOOL GSync_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL GSync_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL GSync_ProcExit(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions GSYNC_PROC_FUNCTIONS = {
    GSync_ProcInit,
    GSync_ProcMain,
    GSync_ProcExit,
};

// Returns whether the HTTP status is an error, and shows it
static BOOL GSync_CheckHttpStatus(GSyncWork *wk, int status) {
    switch (status) {
    case 503:
        wk->error = GSYNC_ERROR_SERVICE_UNAVAILABLE;
        GSync_ChangeState(wk, GSync_StateError, 218);
        return TRUE;
    case 502:
        wk->error = GSYNC_ERROR_BAD_GATEWAY;
        GSync_ChangeState(wk, GSync_StateError, 222);
        return TRUE;
    default:
        if (status >= 400) {
            wk->error = GSYNC_ERROR_HTTP;
            GSync_ChangeState(wk, GSync_StateError, 228);
            return TRUE;
        }
        return FALSE;
    }
}

static BOOL GSync_IsOnline(GSyncWork *wk) {
    EventGameSync *param = wk->param;

    if (param->gsyncResult == GSYNC_RESULT_CONNECT) {
        return FALSE;
    }
    if (param->hasAccountInfo && param->accountInfo.status == 4) {
        return FALSE;
    }
    if (func_02042788()) {
        return TRUE;
    }
    return FALSE;
}

static BOOL GSync_IsNewAccount(GSyncWork *wk) {
    EventGameSync *param = wk->param;

    if (param->hasAccountInfo && param->accountInfo.status == 0) {
        return TRUE;
    }
    return FALSE;
}

static BOOL GSync_HasAccountPokemon(GSyncWork *wk) {
    EventGameSync *param = wk->param;

    if (param->hasAccountInfo) {
        if (param->accountInfo.status == 0) {
            return TRUE;
        }
        if (param->accountInfo.status == 4) {
            return TRUE;
        }
    }
    return FALSE;
}

static void GSync_SetState(GSyncWork *wk, GSyncStateFunc state) {
    wk->state = state;
}

// Changes the state, from a line that the debug build reported
static void GSync_ChangeState(GSyncWork *wk, GSyncStateFunc state, int line) {
    GSync_SetState(wk, state);
}

static void GSync_StateWaitFadeOut(GSyncWork *wk) {
    if (GFL_WipeIsFinished()) {
        GSync_ChangeState(wk, NULL, 350);
    }
}

static void GSync_StateFadeOut(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg)) {
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
        GSync_ChangeState(wk, GSync_StateWaitFadeOut, 370);
    }
}

static void GSync_StateWaitCleanup(GSyncWork *wk) {
    if (GSyncDownload_IsSucceeded(wk->download)) {
        GSyncDownload_Free(wk->download);
        wk->download = NULL;
        GSync_ChangeState(wk, GSync_StateEnd, 387);
    }
}

static void GSync_StateErrorWait(GSyncWork *wk) {
    if (!wk->receiving && ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48())) {
        if (wk->saving) {
            func_02017884(wk->gameData);
            wk->saving = FALSE;
        }
        GSync_RestoreDreamWorld(wk);
        if (wk->download != NULL) {
            // BUG: the end replaces the wait for the download's cleanup, so the proc's exit frees the download without
            // waiting for it
#ifdef BUGFIX
            if (GSyncDownload_Cleanup(wk->download)) {
                GSync_ChangeState(wk, GSync_StateWaitCleanup, 408);
            } else {
                GSync_ChangeState(wk, GSync_StateEnd, 410);
            }
#else
            if (GSyncDownload_Cleanup(wk->download)) {
                GSync_ChangeState(wk, GSync_StateWaitCleanup, 408);
            }
            GSync_ChangeState(wk, GSync_StateEnd, 410);
#endif
        } else {
            GSync_ChangeState(wk, GSync_StateEnd, 413);
        }
    }
}

static void GSync_StateError(GSyncWork *wk) {
    int msgId = wk->error + 29;

    if (wk->http != NULL) {
        NHttpRap_EndRequest(wk->http);
    }
    if (wk->error == GSYNC_ERROR_DOWNLOAD) {
        msgId = 39;
    } else if (wk->error == GSYNC_ERROR_SERVICE_UNAVAILABLE) {
        msgId = 40;
    } else if (wk->error == GSYNC_ERROR_BAD_GATEWAY) {
        msgId = 41;
    } else if (wk->error == GSYNC_ERROR_HTTP) {
        msgId = 38;
    } else if (wk->error == 10) {
        msgId = 38;
    } else if (wk->error <= 0 || wk->error >= 11) {
        msgId = 38;
    }
    GSyncMessage_ClearMessage(wk->msg);
    GSyncDisp_DeleteActor(wk->disp, 13);
    GSyncMessage_LoadString(wk->msg, msgId);
    GSyncMessage_PrintInfoAt(wk->msg, 1, 16);
    GSync_ChangeState(wk, GSync_StateErrorWait, 457);
}

static void GSync_StateEnd(GSyncWork *wk) {
    wk->param->gsyncResult = GSYNC_RESULT_DONE;
    GSync_ChangeState(wk, GSync_StateFadeOut, 473);
}

static void GSync_StateWaitDownloadsShown(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg) &&
        ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48())) {
        GSync_ChangeState(wk, GSync_StateEnd, 489);
    }
}

// Shows what was downloaded: the C-Gear skin, the musical and the Pokédex skin
static void GSync_StateShowDownloads(GSyncWork *wk) {
    u32 msgIds[] = { 0, 24, 25, 27, 26, 29, 28, 23 };

    if (wk->downloadFlags) {
        GSyncMessage_ClearMessage(wk->msg);
        GSyncMessage_LoadString(wk->msg, msgIds[wk->downloadFlags]);
        GSyncMessage_PrintInfoAt(wk->msg, 1, 8);
        GSync_ChangeState(wk, GSync_StateWaitDownloadsShown, 512);
    } else {
        GSync_ChangeState(wk, GSync_StateEnd, 515);
    }
}

static void GSync_StateWaitLevelUp(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg)) {
        wk->wait--;
        if (wk->wait <= 0) {
            if (wk->wait == 0) {
                GFL_SndBGMPop();
                GFL_SndBGMSetPaused(FALSE);
                GFL_SndBGMFadeIn(6);
            }
            if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
                GSync_ChangeState(wk, GSync_StateShowDownloads, 539);
            }
        }
    }
}

static void GSync_StateWaitReceived(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg) &&
        ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48())) {
        if (wk->levels == 0) {
            GSync_ChangeState(wk, GSync_StateShowDownloads, 551);
            return;
        }
        GSyncMessage_ClearInfo(wk->msg);
        GSyncMessage_FormatPokemonNumber(wk->msg, 22, wk->levels, wk->pkm);
        GSyncMessage_PrintLoaded(wk->msg, FALSE);
        GFL_SndBGMSetPaused(TRUE);
        GFL_SndBGMPush();
        GFL_SndBGMPlay(SEQ_ME_LVUP, SND_CHANNEL_MASK_ALL);
        wk->wait = 180;
        GSync_ChangeState(wk, GSync_StateWaitLevelUp, 562);
    }
}

static void GSync_OnPokemonAnimEnd(u32 param, fx32 frame) {
    GSyncWork *wk = (GSyncWork *)param;

    GSyncDisp_SetActorSequence(wk->disp, 0, 0);
    GSyncDisp_CreateActor(wk->disp, 12);
    GSyncDisp_PokeIconSequence2(wk->disp);
}

// Takes the Pokémon back from the Dream World into the first free box slot, with the levels it gained
static void GSync_ReceivePokemon(GSyncWork *wk) {
    DreamWorldSave *dreamWorld = getDreamWorldStuffAddress(wk->save);

    if (wk->pkm != NULL) {
        GFL_HeapFree(wk->pkm);
        wk->pkm = NULL;
    }
    if (BoxSaveAccessor_GetNextFreeBoxSlot(wk->boxes, &wk->boxTray, &wk->boxPos)) {
        PartyPkm *pkm = GFL_HeapAllocate(wk->heapId, PokeParty_GetPkmRawSize(), TRUE, "gsync_state.c", 597);
        u32 size = PokeParty_GetPkmRawSize();

        sys_memcpy(func_02009998(dreamWorld), pkm, size);
        if (wk->levels != 0) {
            int level = PokeParty_GetParam(pkm, PKM_PARAM_LEVEL, NULL);

            if (level == 100) {
                wk->levels = 0;
            }
            if (level + wk->levels > 100) {
                wk->levels = 100 - level;
                level = 100;
            } else {
                level = level + wk->levels;
            }
            setLevel(pkm, level);
        }
        BoxSaveAccessor_InsertPkm(wk->boxes, func_0201d624(pkm));
        wk->pkm = pkm;
        func_02009a00(dreamWorld, FALSE);
        func_02009adc(dreamWorld, wk->resultId);
        MedalBox_GiveMedal(SaveControl_GetMedalBox(wk->save), 232);
    }
}

static void GSync_StateWaitReceiveSave(GSyncWork *wk) {
    ClActorCallback callback;

    getDreamWorldStuffAddress(wk->save);
    if (func_02017850(wk->gameData) == 2) {
        wk->saving = FALSE;
        wk->receiving = FALSE;
        wk->busy = FALSE;
        func_02038bc8(28);
        GSyncMessage_ClearMessage(wk->msg);
        GSyncMessage_FormatPokemonNumber(wk->msg, 45, 0, wk->pkm);
        GSyncMessage_PrintInfoAt(wk->msg, 1, 4);
        GSyncDisp_CreateActor(wk->disp, 0);
        GSyncDisp_SetActorSequence(wk->disp, 6, 7);
        GSyncDisp_SetActorSequence(wk->disp, 0, 1);
        GSyncDisp_DeleteActor(wk->disp, 13);
        callback.type = CLACT_CALLBACK_LAST_FRAME;
        callback.param = (u32)wk;
        callback.func = GSync_OnPokemonAnimEnd;
        GSyncDisp_SetActorCallback(wk->disp, 0, &callback);
        GSyncDisp_StartFade(wk->disp, FALSE);
        GSyncDisp_CreatePokeIcon(wk->disp, func_0201d620(wk->pkm), CLACT_SURFACE_MAIN);
        GSyncDisp_PokeIconSequence1(wk->disp);
        GFL_SndSEPlay(SEQ_SE_SYS_25);
        GSync_ChangeState(wk, GSync_StateWaitReceived, 686);
    }
}

static void GSync_StateWaitFinishDownload(GSyncWork *wk) {
    int status = NHttpRap_GetStatus(wk->http);

    if (!GSync_CheckHttpStatus(wk, status) && NHttpRap_Poll(wk->http) == 0) {
        NHttpRap_GetAnswer(wk->http);
        getDreamWorldStuffAddress(wk->save);
        wk->busy = TRUE;
        GSync_ChangeState(wk, GSync_StateWaitReceiveSave, 711);
    }
}

static void GSync_StateFinishDownload(GSyncWork *wk) {
    if (wk->wait != 0) {
        wk->wait--;
        return;
    }
    if (func_02042788() && NHttpRap_SendRequest(GSYNC_REQUEST_DOWNLOAD_FINISH, wk->http)) {
        wk->request[0] = 1;
        wk->request[1] = 0;
        func_ov189_021a0854(NHttpRap_GetConnection(wk->http), wk->request, sizeof(wk->request));
        if (NHttpRap_StartRequest(wk->http) == 0) {
            GSync_ChangeState(wk, GSync_StateWaitFinishDownload, 743);
        }
    }
}

static void GSync_StateWaitReceiveSaveStart(GSyncWork *wk) {
    if (wk->progress < 100) {
        wk->progress++;
    }
    GSyncDisp_SetProgress(wk->disp, wk->progress);
    if (func_02017850(wk->gameData) == 1) {
        wk->wait = GFL_RandomLC(190);
        GSync_ChangeState(wk, GSync_StateFinishDownload, 768);
    }
}

// Takes the Pokémon back and stores what it brought, then saves
static void GSync_StateReceive(GSyncWork *wk) {
    DreamWorldSave *dreamWorld = getDreamWorldStuffAddress(wk->save);

    wk->receiving = TRUE;
    GSync_ReceivePokemon(wk);
    if (!GSync_HasAccountPokemon(wk)) {
        GSync_SetItems(dreamWorld, wk->result);
        GSync_AddForestPokemonList(wk, dreamWorld, wk->result);
        GSync_SetEntries(dreamWorld, wk->result);
        func_020099d8(dreamWorld, wk->result->unk5A);
        GSync_AddJoinAvenuePeople(wk->gameData, wk->result->people, wk->heapId);
    } else {
        int i;
        GSyncAccountInfo *info = &wk->param->accountInfo;

        GSync_AddForestPokemon(wk, dreamWorld, info->species, info->sex, info->form, info->unk4, GFL_RandomLC(8));
        for (i = 0; i < 20; i++) {
            func_02009a50(dreamWorld, i, info->items[i], info->itemCounts[i]);
        }
    }
    wk->saving = TRUE;
    func_0201782c(wk->gameData);
    GSync_ChangeState(wk, GSync_StateWaitReceiveSaveStart, 814);
}

static void GSync_StateCleanupDownload(GSyncWork *wk) {
    if (GSyncDownload_Cleanup(wk->download)) {
        GSync_ChangeState(wk, GSync_StateWaitDownloadCleanup, 822);
        return;
    }
    wk->error = GSYNC_ERROR_DOWNLOAD;
    GSync_ChangeState(wk, GSync_StateError, 826);
}

static void GSync_StateCGearSaved(GSyncWork *wk) {
    wk->getCGear = FALSE;
    GFL_HeapFree(wk->cgearSkin);
    wk->busy = FALSE;
    wk->cgearSkin = NULL;
    GSync_ChangeState(wk, GSync_StateCleanupDownload, 837);
}

static void GSync_StateWaitCGearSave(GSyncWork *wk) {
    SaveControl *save = GameData_GetSaveControl(wk->gameData);

    if (func_020178f4(wk->gameData, 4) == 2) {
        freeIntermediateSaveExtraBlksAfterLoad2(save, 4);
        func_020098cc(func_02009918(save), TRUE);
        GSync_ChangeState(wk, GSync_StateCGearSaved, 848);
    }
}

static void GSync_StateCGearDownloaded(GSyncWork *wk) {
    SaveControl *save = GameData_GetSaveControl(wk->gameData);
    u8 *buffer = GSyncDownload_GetBuffer(wk->download);
    u16 crc = getCRC16(buffer, GSYNC_CGEAR_SKIN_SIZE);
    u8 *skin;

    if (crc != *(u16 *)(buffer + GSYNC_CGEAR_SKIN_SIZE)) {
        wk->error = GSYNC_ERROR_DOWNLOAD;
        GSync_ChangeState(wk, GSync_StateError, 866);
        return;
    }
    wk->busy = TRUE;
    skin = GFL_HeapAllocate(wk->heapId, 0x2800, FALSE, "gsync_state.c", 871);
    wk->cgearSkin = skin;
    func_02007560(save, 4, wk->heapId, skin, 0x2800);
    sys_memcpy(buffer, skin, GSYNC_CGEAR_SKIN_SIZE);
    func_020098d4(func_02009918(save), crc);
    func_020178c4(wk->gameData, 4);
    GSync_ChangeState(wk, GSync_StateWaitCGearSave, 882);
}

static void GSync_StateZukanSaved(GSyncWork *wk) {
    wk->getZukan = FALSE;
    GFL_HeapFree(wk->zukanSkin);
    wk->busy = FALSE;
    wk->zukanSkin = NULL;
    GSync_ChangeState(wk, GSync_StateCleanupDownload, 893);
}

static void GSync_StateWaitZukanSave(GSyncWork *wk) {
    SaveControl *save = GameData_GetSaveControl(wk->gameData);

    if (func_020178f4(wk->gameData, 7) == 2) {
        freeIntermediateSaveExtraBlksAfterLoad2(save, 7);
        GSync_ChangeState(wk, GSync_StateZukanSaved, 903);
    }
}

static void GSync_StateZukanDownloaded(GSyncWork *wk) {
    SaveControl *save = GameData_GetSaveControl(wk->gameData);
    u8 *buffer = GSyncDownload_GetBuffer(wk->download);
    u16 crc = getCRC16(buffer, GSYNC_ZUKAN_SKIN_SIZE);

    if (crc != *(u16 *)(buffer + GSYNC_ZUKAN_SKIN_SIZE)) {
        wk->error = GSYNC_ERROR_DOWNLOAD;
        GSync_ChangeState(wk, GSync_StateError, 917);
        return;
    }
    wk->busy = TRUE;
    wk->zukanSkin = GFL_HeapAllocate(wk->heapId, 0x6800, FALSE, "gsync_state.c", 922);
    func_02007560(save, 7, wk->heapId, wk->zukanSkin, 0x6800);
    sys_memcpy(buffer, wk->zukanSkin, GSYNC_ZUKAN_SKIN_SIZE);
    func_0200f194(wk->zukanSkin, TRUE);
    func_020178c4(wk->gameData, 7);
    GSync_ChangeState(wk, GSync_StateWaitZukanSave, 928);
}

static void GSync_StateWaitMusicalImport(GSyncWork *wk) {
    if (func_0200cd64(wk->musical)) {
        wk->busy = FALSE;
        wk->musical = NULL;
        GSync_ChangeState(wk, GSync_StateCleanupDownload, 939);
    }
}

// Checks the musical's CRC, after it at the end of the file, and imports it
static void GSync_MusicalDownloaded(GSyncWork *wk, int size) {
    u16 *buffer;
    int crcIndex;
    u16 crc;

    wk->getMusical = FALSE;
    buffer = GSyncDownload_GetBuffer(wk->download);
    crcIndex = size / 2;
    if (size > GSYNC_MUSICAL_SIZE_MAX) {
        wk->error = GSYNC_ERROR_DOWNLOAD;
        GSync_ChangeState(wk, GSync_StateError, 956);
        return;
    }
    crc = getCRC16(buffer, size);
    if (crc != buffer[crcIndex]) {
        wk->error = GSYNC_ERROR_DOWNLOAD;
        GSync_ChangeState(wk, GSync_StateError, 962);
        return;
    }
    wk->busy = TRUE;
    wk->musical = func_0200cd34(wk->gameData, buffer, size, wk->heapId);
    GSync_ChangeState(wk, GSync_StateWaitMusicalImport, 968);
}

static void GSync_StateWaitDownloadCleanup(GSyncWork *wk) {
    if (GSyncDownload_IsSucceeded(wk->download)) {
        if (GSyncDownload_IsFailed(wk->download)) {
            wk->error = GSYNC_ERROR_DOWNLOAD;
            GSync_ChangeState(wk, GSync_StateError, 981);
            return;
        }
        GSyncDownload_Free(wk->download);
        wk->download = NULL;
        if (wk->progress < 100) {
            wk->progress += 10;
        }
        GSyncDisp_SetProgress(wk->disp, wk->progress);
        GSync_ChangeState(wk, GSync_StateNextDownload, 994);
    }
}

static void GSync_StateDownloaded(GSyncWork *wk) {
    switch (wk->downloadKind) {
    case GSYNC_DOWNLOAD_CGEAR:
        GSync_ChangeState(wk, GSync_StateCGearDownloaded, 1002);
        break;
    case GSYNC_DOWNLOAD_MUSICAL:
        GSync_MusicalDownloaded(wk, GSyncDownload_GetFileSize(wk->download) - 2);
        break;
    case GSYNC_DOWNLOAD_ZUKAN:
        GSync_ChangeState(wk, GSync_StateZukanDownloaded, 1010);
        break;
    }
}

static void GSync_StateWaitFile(GSyncWork *wk) {
    if (GSyncDownload_IsFailed(wk->download)) {
        wk->error = GSYNC_ERROR_DOWNLOAD;
        GSync_ChangeState(wk, GSync_StateError, 1022);
        return;
    }
    if (GSyncDownload_IsSucceeded(wk->download)) {
        GSyncDownload_ResetTimer(wk->download);
        GSync_ChangeState(wk, GSync_StateDownloaded, 1033);
    }
}

static void GSync_StateGetFile(GSyncWork *wk) {
    if (GSyncDownload_GetFile(wk->download)) {
        GSync_ChangeState(wk, GSync_StateWaitFile, 1042);
        return;
    }
    wk->error = GSYNC_ERROR_DOWNLOAD;
    GSync_ChangeState(wk, GSync_StateError, 1046);
}

static void GSync_StateWaitFileList(GSyncWork *wk) {
    if (GSyncDownload_IsFailed(wk->download)) {
        wk->error = GSYNC_ERROR_DOWNLOAD;
        GSync_ChangeState(wk, GSync_StateError, 1055);
        return;
    }
    if (GSyncDownload_IsSucceeded(wk->download)) {
        if (GSyncDownload_GetFileSize(wk->download) == 0) {
            wk->error = GSYNC_ERROR_DOWNLOAD;
            GSync_ChangeState(wk, GSync_StateError, 1064);
            return;
        }
        GSyncDownload_ResetTimer(wk->download);
        GSync_ChangeState(wk, GSync_StateGetFile, 1070);
    }
}

static void GSync_StateGetFileList(GSyncWork *wk) {
    if (GSyncDownload_GetFileList(wk->download)) {
        GSync_ChangeState(wk, GSync_StateWaitFileList, 1078);
        return;
    }
    wk->error = GSYNC_ERROR_DOWNLOAD;
    GSync_ChangeState(wk, GSync_StateError, 1082);
}

static void GSync_StateSetAttr(GSyncWork *wk) {
    if (GSyncDownload_IsSucceeded(wk->download)) {
        if (GSyncDownload_IsFailed(wk->download)) {
            wk->error = GSYNC_ERROR_DOWNLOAD;
            GSync_ChangeState(wk, GSync_StateError, 1093);
            return;
        }
        switch (wk->downloadKind) {
        case GSYNC_DOWNLOAD_CGEAR:
            if (GSyncDownload_SetAttr(wk->download, "CGEAR2_E", wk->getCGear)) {
                GSync_ChangeState(wk, GSync_StateGetFileList, 1100);
                return;
            }
            wk->error = GSYNC_ERROR_DOWNLOAD;
            GSync_ChangeState(wk, GSync_StateError, 1104);
            return;
        case GSYNC_DOWNLOAD_MUSICAL:
            if (GSyncDownload_SetAttr(wk->download, "MUSICAL_E", wk->getMusical)) {
                GSync_ChangeState(wk, GSync_StateGetFileList, 1109);
                return;
            }
            wk->error = GSYNC_ERROR_DOWNLOAD;
            GSync_ChangeState(wk, GSync_StateError, 1113);
            return;
        case GSYNC_DOWNLOAD_ZUKAN:
            if (GSyncDownload_SetAttr(wk->download, "ZUKAN_E", wk->getZukan)) {
                GSync_ChangeState(wk, GSync_StateGetFileList, 1118);
                return;
            }
            wk->error = GSYNC_ERROR_DOWNLOAD;
            GSync_ChangeState(wk, GSync_StateError, 1122);
            return;
        }
    }
}

static void GSync_StateStartDownload(GSyncWork *wk) {
    switch (wk->downloadKind) {
    case GSYNC_DOWNLOAD_CGEAR:
        wk->download = GSyncDownload_Create(wk->heapId, func_0200ce50() + 4);
        break;
    case GSYNC_DOWNLOAD_MUSICAL:
        wk->download = GSyncDownload_Create(wk->heapId, 0x20000);
        break;
    case GSYNC_DOWNLOAD_ZUKAN:
        wk->download = GSyncDownload_Create(wk->heapId, func_0200f164() + 4);
        break;
    }
    if (GSyncDownload_Init(wk->download)) {
        GSync_ChangeState(wk, GSync_StateSetAttr, 1149);
        return;
    }
    wk->error = GSYNC_ERROR_DOWNLOAD;
    GSync_ChangeState(wk, GSync_StateError, 1153);
}

// Downloads the next of the files the Dream World sent, or takes the Pokémon back once they are done
static void GSync_StateNextDownload(GSyncWork *wk) {
    if (wk->getCGear) {
        wk->downloadKind = GSYNC_DOWNLOAD_CGEAR;
        GSync_ChangeState(wk, GSync_StateStartDownload, 1162);
    } else if (wk->getMusical) {
        wk->downloadKind = GSYNC_DOWNLOAD_MUSICAL;
        GSync_ChangeState(wk, GSync_StateStartDownload, 1167);
    } else if (wk->getZukan) {
        wk->downloadKind = GSYNC_DOWNLOAD_ZUKAN;
        GSync_ChangeState(wk, GSync_StateStartDownload, 1172);
    } else {
        GSync_ChangeState(wk, GSync_StateReceive, 1175);
    }
}

// Stores the result's entries, if they changed
static void GSync_SetEntries(DreamWorldSave *dreamWorld, GSyncResult *result) {
    BOOL changed = FALSE;
    int i;

    for (i = 0; i < 5; i++) {
        if (GFL_STD_MemCmp(func_02009a98(dreamWorld, i), result->entries[i], sizeof(result->entries[i])) != 0) {
            changed = TRUE;
            break;
        }
    }
    if (changed) {
        for (i = 0; i < 5; i++) {
            func_02009ab0(dreamWorld, i, result->entries[i]);
        }
        func_02009af8(dreamWorld, 127);
    }
}

static BOOL GSync_SetItems(DreamWorldSave *dreamWorld, GSyncResult *result) {
    BOOL any = FALSE;
    int i;

    for (i = 0; i < 20; i++) {
        u16 item = result->items[i];

        if (item != ITEM_NONE && item <= GSYNC_ITEM_MAX) {
            func_02009a50(dreamWorld, i, item, result->itemCounts[i]);
            any = TRUE;
        }
    }
    return any;
}

static BOOL GSync_HasItems(DreamWorldSave *dreamWorld, GSyncResult *result) {
    BOOL any = FALSE;
    int i;

    for (i = 0; i < 20; i++) {
        u16 item = result->items[i];

        if (item != ITEM_NONE && item <= GSYNC_ITEM_MAX) {
            any = TRUE;
        }
    }
    return any;
}

// Puts a Pokémon from the Dream World in the Entree Forest, if it has an overworld model
static void GSync_AddForestPokemon(GSyncWork *wk, DreamWorldSave *dreamWorld, int species, int sex, int form, int unk2,
                                   int slot) {
    u8 forestSlot = slot;

    if (species != SPECIES_NONE && species <= SPECIES_GENESECT) {
        TPokeData *data = LoadTPokeData(wk->heapId);
        int size = 3;
        u8 sanitized = PML_PkmSanitizeForme(species, form);
        u32 sexRatio = PML_PersonalGetParamSingle(species, sanitized, PERSONAL_SEX_RATIO);
        u8 modelForm = func_0201efe4(species, sanitized);

        switch (sexRatio) {
        case 0:
            sex = 0;
            break;
        case 254:
            sex = 1;
            break;
        case 255:
            sex = 2;
            break;
        default:
            if (sex > 1) {
                sex = GFL_RandomLC(2);
            }
            break;
        }
        if (IsFieldPokemonSpriteHugeBillboard(wk->gameData, data, species, sex, modelForm)) {
            size = 2;
        }
        if (forestSlot > 8) {
            forestSlot = 0;
        }
        if (GetFieldPokemonMMdlLUTIndex_(data, species, sex, modelForm) != 0xffff) {
            func_0200ea40(getAreaNPCData(wk->save), species, unk2, sex, modelForm, forestSlot, size);
        }
        FreeTPokeData(data);
    }
}

static void GSync_AddForestPokemonList(GSyncWork *wk, DreamWorldSave *dreamWorld, GSyncResult *result) {
    int i;

    for (i = 0; i < 10; i++) {
        if (result->pokemon[i].species == SPECIES_NONE) {
            break;
        }
        GSync_AddForestPokemon(wk, dreamWorld, result->pokemon[i].species, result->pokemon[i].sex,
                               result->pokemon[i].form, result->pokemon[i].unk2, result->pokemon[i].slot);
    }
}

static void GSync_SetFloatItems(GSyncWork *wk, GSyncResult *result) {
    int i;

    for (i = 0; i < 20; i++) {
        u16 item = result->items[i];

        if (item != ITEM_NONE && result->itemCounts[i] != 0 && item <= GSYNC_ITEM_MAX) {
            GSyncDisp_SetFloatItem(wk->disp, i, item);
        }
    }
}

static void GSync_ReadResult(GSyncWork *wk, DreamWorldSave *dreamWorld, GSyncResult *result, GSyncResponse *response) {
    int i;

    if (response->status != 0) {
        wk->error = response->status;
        GSync_ChangeState(wk, GSync_StateError, 1323);
        return;
    }
    if (!result->unk5B && result->id != func_02009ad8(dreamWorld)) {
        wk->resultId = result->id;
        wk->getMusical = result->musical;
        wk->getCGear = result->cgear;
        wk->getZukan = result->zukan;
        wk->levels = result->levels;
        if (wk->getCGear) {
            wk->downloadFlags |= 1;
        }
        if (wk->getMusical) {
            wk->downloadFlags |= 2;
        }
        if (wk->getZukan) {
            wk->downloadFlags |= 4;
        }
        for (i = 0; i < 10; i++) {
            if (result->pokemon[i].species == SPECIES_NONE) {
                break;
            }
            GSyncDisp_SetFloatPokemon(wk->disp, i + GSYNC_DISP_FLOAT_ITEM_COUNT, result->pokemon[i].species,
                                      result->pokemon[i].form, result->pokemon[i].sex);
        }
        wk->hasItems = GSync_HasItems(dreamWorld, result);
        GSync_SetFloatItems(wk, result);
        GSyncDisp_LoadFloatGraphics(wk->disp);
    } else {
        wk->hasItems = FALSE;
    }
    GSync_StateNextDownload(wk);
}

static BOOL GSync_AreAccountItemsUnique(GSyncWork *wk) {
    int i;
    int j;
    GSyncAccountInfo *info;

    getDreamWorldStuffAddress(wk->save);
    info = &wk->param->accountInfo;
    for (i = 0; i < 20; i++) {
        for (j = i + 1; j < 20; j++) {
            if (info->items[i] != ITEM_NONE && info->items[j] == info->items[i]) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static void GSync_StateWaitResult(GSyncWork *wk) {
    if (GSync_IsOnline(wk) && !GSync_HasAccountPokemon(wk)) {
        if (wk->progress < 30) {
            wk->progress++;
        }
        GSyncDisp_SetProgress(wk->disp, wk->progress);
        if (!GSync_CheckHttpStatus(wk, NHttpRap_GetStatus(wk->http)) && NHttpRap_Poll(wk->http) == 0) {
            GSyncResponse *response = NHttpRap_GetAnswer(wk->http);
            DreamWorldSave *dreamWorld = getDreamWorldStuffAddress(wk->save);
            GSyncResult *result = &response->body.result;

            wk->result = GFL_HeapAllocate(wk->heapId, sizeof(GSyncResult), TRUE, "gsync_state.c", 1438);
            sys_memcpy(result, wk->result, sizeof(GSyncResult));
            GSync_ReadResult(wk, dreamWorld, result, response);
        }
        return;
    }
    wk->frames++;
    if (wk->progress < 30) {
        wk->progress++;
    }
    if (wk->frames == 360) {
        GSyncMessage_LoadString(wk->msg, 43);
        GSyncMessage_PrintInfo(wk->msg);
    } else if (wk->frames == 720) {
        GSyncMessage_LoadString(wk->msg, 44);
        GSyncMessage_PrintInfo(wk->msg);
    } else if (wk->frames == 1080) {
        if (GSync_IsNewAccount(wk)) {
            GSyncMessage_ClearInfo(wk->msg);
        }
        if (GSync_AreAccountItemsUnique(wk)) {
            GSync_ChangeState(wk, GSync_StateReceive, 1463);
        } else {
            wk->error = 9;
            GSync_ChangeState(wk, GSync_StateError, 1468);
        }
    }
    GSyncDisp_SetProgress(wk->disp, wk->progress);
}

static void GSync_StateRequestResult(GSyncWork *wk) {
    if (GSync_IsOnline(wk) && !GSync_HasAccountPokemon(wk)) {
        if (NHttpRap_SendRequest(GSYNC_REQUEST_DOWNLOAD, wk->http) && NHttpRap_StartRequest(wk->http) == 0) {
            GSync_ChangeState(wk, GSync_StateWaitResult, 1480);
        }
        return;
    }
    if (GSync_IsNewAccount(wk)) {
        wk->frames = 0;
        GSyncMessage_LoadString(wk->msg, 42);
        GSyncMessage_PrintInfo(wk->msg);
    } else {
        wk->frames = 1070;
    }
    GSync_ChangeState(wk, GSync_StateWaitResult, 1493);
}

static void GSync_StateWaitWakeMessage(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg)) {
        GSync_ChangeState(wk, GSync_StateRequestResult, 1502);
    }
}

static void GSync_StateWake(GSyncWork *wk) {
    GSyncMessage_PrintStream(wk->msg, 11);
    GSyncMessage_StartWaitIcon(wk->msg);
    GSyncDisp_DeleteActor(wk->disp, 8);
    GSyncDisp_StartWave(wk->disp);
    GSyncDisp_StartFade(wk->disp, TRUE);
    GSyncDisp_SetActorSequence(wk->disp, 6, 5);
    GSyncDisp_CreateActor(wk->disp, 0);
    GSyncDisp_CreateActor(wk->disp, 12);
    GSyncDisp_CreateActor(wk->disp, 13);
    GFL_SndSEPlay(SEQ_SE_SYS_24);
    GSync_ChangeState(wk, GSync_StateWaitWakeMessage, 1523);
}

static void GSync_StateWaitTooSoon(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg) &&
        ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48())) {
        GSync_ChangeState(wk, GSync_StateEnd, 1536);
    }
}

static void GSync_StateTooSoon(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg)) {
        GSyncMessage_PrintStream(wk->msg, 16);
        GSync_ChangeState(wk, GSync_StateWaitTooSoon, 1550);
    }
}

static void GSync_StateWaitNoRoomMessage(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg) &&
        ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48())) {
        GSync_ChangeState(wk, GSync_StateEnd, 1561);
    }
}

static void GSync_StateNoRoom(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg)) {
        GSyncMessage_PrintStream(wk->msg, 49);
        GSync_ChangeState(wk, GSync_StateWaitNoRoomMessage, 1574);
    }
}

// Whether to wake the Pokémon, with the sound of its sleep
static void GSync_StateWakeAnswer(GSyncWork *wk) {
    if (wk->seTimer % 110 == 0) {
        GFL_SndSEPlay(SEQ_SE_SYS_26);
    }
    wk->seTimer++;
    if (AppTaskMenu_IsFlashFinished(wk->yesNo)) {
        if (AppTaskMenu_GetCursorPos(wk->yesNo) == 0) {
            if (!BoxSaveAccessor_GetNextFreeBoxSlot(wk->boxes, &wk->boxTray, &wk->boxPos)) {
                GSync_ChangeState(wk, GSync_StateNoRoom, 1599);
            } else {
                GSync_ChangeState(wk, GSync_StateWake, 1602);
            }
        } else {
            GSync_ChangeState(wk, GSync_StateEnd, 1606);
        }
        GSyncMessage_ClearMessage(wk->msg);
        AppTaskMenu_Free(wk->yesNo);
        reg_G2S_DB_BLDCNT = 0;
        wk->yesNo = NULL;
    }
}

static void GSync_StateWakeAsk(GSyncWork *wk) {
    if (wk->seTimer % 110 == 0) {
        GFL_SndSEPlay(SEQ_SE_SYS_26);
    }
    wk->seTimer++;
    if (GSyncMessage_IsPrintFinished(wk->msg)) {
        wk->yesNo = GSyncMessage_CreateYesNo(wk->msg, GSYNC_YESNO_POS_UPPER);
        GSync_ChangeState(wk, GSync_StateWakeAnswer, 1634);
    }
}

static void GSync_StateWakeShow(GSyncWork *wk) {
    GSyncMessage_ClearMessage(wk->msg);
    GSyncDisp_CreateActor(wk->disp, 0);
    GSyncDisp_CreateActor(wk->disp, 12);
    GSyncDisp_CreateActor(wk->disp, 6);
    GSyncDisp_CreateActor(wk->disp, 8);
    if (func_02009a78(getDreamWorldStuffAddress(wk->save)) != 0) {
        GSyncMessage_PrintStream(wk->msg, 21);
    } else {
        GSyncMessage_PrintStream(wk->msg, 10);
    }
    GSync_ChangeState(wk, GSync_StateWakeAsk, 1660);
}

static void GSync_StateWaitAccountMessage(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg)) {
        GSync_ChangeState(wk, GSync_StateWakeShow, 1725);
    }
}

static void GSync_StateShowIdNext(GSyncWork *wk) {
    if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
        GSyncMessage_ClearInfo(wk->msg);
        func_020099e8(getDreamWorldStuffAddress(wk->save), TRUE);
        wk->idShown = TRUE;
        GSync_ChangeState(wk, GSync_StateCheckDate, 1746);
    }
}

static void GSync_StateShowIdWait(GSyncWork *wk) {
    if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
        GSyncMessage_LoadString(wk->msg, 48);
        GSyncMessage_PrintInfoAt(wk->msg, 7, 14);
        GSync_ChangeState(wk, GSync_StateShowIdNext, 1755);
    }
}

// Shows the Game Sync ID
static void GSync_StateShowId(GSyncWork *wk) {
    GSyncMessage_FormatGSyncId(wk->msg, func_02008bdc(GetGameDataPlayerInfo(wk->param->gameData)));
    GSyncMessage_PrintLoaded(wk->msg, TRUE);
    GSyncMessage_LoadString(wk->msg, 47);
    GSyncMessage_PrintInfoAt(wk->msg, 7, 14);
    GSync_ChangeState(wk, GSync_StateShowIdWait, 1767);
}

static void GSync_StateWaitCreate(GSyncWork *wk) {
    if (func_02042788()) {
        if (!GSync_CheckHttpStatus(wk, NHttpRap_GetStatus(wk->http)) && NHttpRap_Poll(wk->http) == 0) {
            GSyncResponse *response;

            GSyncDisp_DeleteActor(wk->disp, 13);
            response = NHttpRap_GetAnswer(wk->http);
            if (response->status == 2) {
                GSync_ChangeState(wk, GSync_StateCheckDate, 1799);
            } else if (response->status == 0) {
                GSyncMessage_ClearMessage(wk->msg);
                GSync_ChangeState(wk, GSync_StateShowId, 1803);
            } else {
                wk->error = response->status;
                GSync_ChangeState(wk, GSync_StateError, 1807);
            }
        }
    } else {
        GSync_ChangeState(wk, GSync_StateCheckDate, 1813);
    }
}

// Creates the account, sending the save data
static void GSync_StateCreate(GSyncWork *wk) {
    u32 size;
    void *data;

    if (func_02042788()) {
        if (NHttpRap_SendRequest(GSYNC_REQUEST_CREATE, wk->http)) {
            data = func_02007454(wk->save, &size);
            func_ov189_021a0854(NHttpRap_GetConnection(wk->http), data, 0x80000);
            GSyncDisp_CreateActor(wk->disp, 13);
            if (NHttpRap_StartRequest(wk->http) == 0) {
                GSync_ChangeState(wk, GSync_StateWaitCreate, 1841);
            }
        }
    } else if (GCTX_HIDGetPressedKeys() || func_0203da48()) {
        GSync_ChangeState(wk, GSync_StateWaitCreate, 1847);
    }
}

static void GSync_StateWaitNotDreamingMessage(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg) &&
        ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48())) {
        GSync_ChangeState(wk, GSync_StateWaitAccountMessage, 1890);
    }
}

static void GSync_StateNotDreaming(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg)) {
        GSyncMessage_PrintStream(wk->msg, 17);
        GSync_ChangeState(wk, GSync_StateWaitNotDreamingMessage, 1908);
    }
}

static void GSync_StateWaitDreamingNowMessage(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg) &&
        ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48())) {
        GSync_ChangeState(wk, GSync_StateEnd, 1921);
    }
}

static void GSync_StateDreamingNow(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg)) {
        GSyncMessage_PrintStream(wk->msg, 20);
        GSync_ChangeState(wk, GSync_StateWaitDreamingNowMessage, 1939);
    }
}

// Goes on by the account's state and whether this proc sends or receives
static void GSync_HandleAccount(GSyncWork *wk, u16 status, GSyncResponse *response) {
    switch (response->status) {
    case 0:
        switch (wk->param->gsyncResult) {
        case GSYNC_RESULT_NONE:
            if (status == 0) {
                GSync_ChangeState(wk, GSync_StateWaitAccountMessage, 1967);
            } else if (status == 3) {
                GSync_ChangeState(wk, GSync_StateWaitAccountMessage, 1970);
            } else if (status == 1) {
                GSync_ChangeState(wk, GSync_StateNotDreaming, 1974);
            } else if (status == 2) {
                GSync_ChangeState(wk, GSync_StateDreamingNow, 1978);
            } else if (status == 4) {
                GSync_ChangeState(wk, GSync_StateWaitAccountMessage, 1981);
            } else {
                GSync_ChangeState(wk, GSync_StateWaitAccountMessage, 1986);
            }
            break;
        case GSYNC_RESULT_WIFI_SETTINGS:
            if (!wk->idShown) {
                GSync_ChangeState(wk, GSync_StateShowId, 1991);
            } else {
                GSync_ChangeState(wk, GSync_StateCheckDate, 1994);
            }
            break;
        }
        break;
    case 5:
        if (wk->param->gsyncResult == GSYNC_RESULT_WIFI_SETTINGS) {
            GSync_ChangeState(wk, GSync_StateCheckDate, 2002);
        } else {
            wk->error = response->status;
            GSync_ChangeState(wk, GSync_StateError, 2006);
        }
        break;
    case 8:
        switch (wk->param->gsyncResult) {
        case GSYNC_RESULT_NONE:
            GSync_ChangeState(wk, GSync_StateWaitAccountMessage, 2013);
            break;
        case GSYNC_RESULT_WIFI_SETTINGS:
            GSync_ChangeState(wk, GSync_StateCreate, 2017);
            break;
        }
        break;
    default:
        wk->error = response->status;
        GSync_ChangeState(wk, GSync_StateError, 2023);
        break;
    }
}

static void GSync_StateWaitAccount(GSyncWork *wk) {
    if (func_02042788()) {
        if (!GSync_CheckHttpStatus(wk, NHttpRap_GetStatus(wk->http)) && NHttpRap_Poll(wk->http) == 0) {
            GSyncResponse *response = NHttpRap_GetAnswer(wk->http);

            sys_memcpy(&response->body.accountInfo, &wk->param->accountInfo, sizeof(GSyncAccountInfo));
            wk->param->hasAccountInfo = TRUE;
            GSync_HandleAccount(wk, response->body.accountInfo.status, response);
        }
    } else {
        GSync_ChangeState(wk, GSync_StateWaitAccountMessage, 2063);
    }
}

static void GSync_StateRequestAccount(GSyncWork *wk) {
    GSyncMessage_Print(wk->msg, 13);
    GSyncMessage_StartWaitIcon(wk->msg);
    if (func_02042788()) {
        if (NHttpRap_SendRequest(GSYNC_REQUEST_PLAY_STATUS, wk->http) && NHttpRap_StartRequest(wk->http) == 0) {
            GSync_ChangeState(wk, GSync_StateWaitAccount, 2085);
        }
    } else if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
        GSync_ChangeState(wk, GSync_StateWaitAccount, 2092);
    }
}

static void GSync_StateWaitNoPokemonMessage(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg) &&
        ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48())) {
        wk->param->gsyncResult = GSYNC_RESULT_ACCOUNT;
        GSyncMessage_ClearMessage(wk->msg);
        GSync_ChangeState(wk, GSync_StateFadeOut, 2115);
    }
}

static void GSync_StateNoPokemon(GSyncWork *wk) {
    GSyncMessage_PrintStream(wk->msg, 19);
    GSync_ChangeState(wk, GSync_StateWaitNoPokemonMessage, 2130);
}

static void GSync_StateWaitSpeciesFlags(GSyncWork *wk) {
    if (func_02042788()) {
        if (!GSync_CheckHttpStatus(wk, NHttpRap_GetStatus(wk->http)) && NHttpRap_Poll(wk->http) == 0) {
            GSyncResponse *response = NHttpRap_GetAnswer(wk->http);
            u32 status = response->status;

            if (status == 0 || status == 8) {
                sys_memcpy(response->body.sendableSpecies, wk->param->sendableSpecies,
                           sizeof(wk->param->sendableSpecies));
                wk->param->gsyncResult = GSYNC_RESULT_SELECT_POKEMON;
                GSync_ChangeState(wk, GSync_StateFadeOut, 2162);
            } else {
                wk->error = status;
                GSync_ChangeState(wk, GSync_StateError, 2173);
            }
        }
    } else {
        wk->param->gsyncResult = GSYNC_RESULT_SELECT_POKEMON;
        GSync_ChangeState(wk, GSync_StateFadeOut, 2180);
    }
}

// Game Sync runs once a day, and not while the adventure save's countdown runs once the ID was shown
static void GSync_StateCheckDate(GSyncWork *wk) {
    RTCDate last;
    RTCDate today;
    u32 date;

    if (hasFullDayMinutesPassed(getSaveAdventureTimeBlock(wk->save)) && wk->idShownSaved) {
        GSync_ChangeState(wk, GSync_StateTooSoon, 2199);
        return;
    }
    date = func_02009ad0(getDreamWorldStuffAddress(wk->save));
    last.year = (u8)(date >> 24);
    last.month = (u8)(date >> 16);
    last.day = (u8)(date >> 8);
    last.week = (u8)date;
    RTC_GetCachedDate(&today);
    if (func_02044298(&last) == func_02044298(&today)) {
        GSync_ChangeState(wk, GSync_StateTooSoon, 2212);
        return;
    }
    if (func_02042788()) {
        if (NHttpRap_SendRequest(GSYNC_REQUEST_SPECIES_FLAGS, wk->http) && NHttpRap_StartRequest(wk->http) == 0) {
            GSync_ChangeState(wk, GSync_StateWaitSpeciesFlags, 2220);
        }
    } else if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
        GSync_ChangeState(wk, GSync_StateWaitSpeciesFlags, 2227);
    }
}

static void GSync_StateWaitSentMessage(GSyncWork *wk) {
    if (wk->seTimer % 110 == 0) {
        GFL_SndSEPlay(SEQ_SE_SYS_26);
    }
    wk->seTimer++;
    if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
        GSync_ChangeState(wk, GSync_StateEnd, 2249);
    }
}

static void GSync_StateWaitSentText(GSyncWork *wk) {
    if (GSyncMessage_IsPrintFinished(wk->msg)) {
        GSync_ChangeState(wk, GSync_StateWaitSentMessage, 2267);
    }
}

static void GSync_StateSent(GSyncWork *wk) {
    if (wk->unk44 != NULL) {
        GFL_HeapFree(wk->unk44);
        wk->unk44 = NULL;
    }
    wk->progress = 100;
    GSyncDisp_SetProgress(wk->disp, 100);
    GSyncMessage_FormatPokemonNumber(wk->msg, 14, 0, wk->pkm);
    GSyncMessage_PrintLoaded(wk->msg, FALSE);
    GSyncDisp_CreateActor(wk->disp, 8);
    GSyncDisp_StartFade(wk->disp, FALSE);
    GSyncDisp_CreateActor(wk->disp, 0);
    GSyncDisp_SetActorSequence(wk->disp, 4, 6);
    GSyncDisp_DeleteActor(wk->disp, 13);
    GFL_SndSEPlay(SEQ_SE_SYS_25);
    GSync_ChangeState(wk, GSync_StateWaitSentText, 2301);
}

static void GSync_StateWaitSendSave(GSyncWork *wk) {
    if (wk->progress < 100) {
        wk->progress++;
    }
    GSyncDisp_SetProgress(wk->disp, wk->progress);
    switch (func_02017850(wk->gameData)) {
    case 1:
        if (wk->dreamWorldCopy != NULL) {
            GFL_HeapFree(wk->dreamWorldCopy);
            wk->dreamWorldCopy = NULL;
        }
        break;
    case 3:
        func_020424ac(0, 0, 0, 1011);
        func_02042454(0);
        break;
    case 2:
        wk->saving = FALSE;
        GSync_ChangeState(wk, GSync_StateSent, 2342);
        break;
    }
}

// Marks the Pokémon as sent and saves
static void GSync_StateSendSave(GSyncWork *wk) {
    DreamWorldSave *dreamWorld = getDreamWorldStuffAddress(wk->save);
    RTCDate date;

    wk->pkmTaken = FALSE;
    RTC_GetCachedDate(&date);
    func_02009ad4(dreamWorld, (date.year << 24) | ((date.month & 0xff) << 16) | ((date.day & 0xff) << 8) | date.week);
    if (func_02009ae0(dreamWorld) != 127) {
        func_02009b30(dreamWorld, 1);
        func_02009af8(dreamWorld, 126);
    }
    wk->progress = 50;
    RecordAddOne(GameData_GetRecords(wk->gameData), 119);
    func_02038bc8(28);
    wk->saving = TRUE;
    func_0201782c(wk->gameData);
    GSync_ChangeState(wk, GSync_StateWaitSendSave, 2391);
}

static void GSync_StateWaitUpload(GSyncWork *wk) {
    getDreamWorldStuffAddress(wk->save);
    GSyncDisp_UpdateFloats(wk->disp);
    if (GSync_IsNewAccount(wk)) {
        if (wk->frames == 0) {
            GSyncMessage_LoadString(wk->msg, 42);
            GSyncMessage_PrintInfo(wk->msg);
        } else if (wk->frames == 360) {
            GSyncMessage_LoadString(wk->msg, 43);
            GSyncMessage_PrintInfo(wk->msg);
        } else if (wk->frames == 720) {
            GSyncMessage_LoadString(wk->msg, 44);
            GSyncMessage_PrintInfo(wk->msg);
        }
        wk->frames++;
    }
    if (GSync_IsOnline(wk)) {
        if (wk->progress < 50) {
            wk->progress++;
        }
        GSyncDisp_SetProgress(wk->disp, wk->progress);
        if (!GSync_CheckHttpStatus(wk, NHttpRap_GetStatus(wk->http)) && NHttpRap_Poll(wk->http) == 0) {
            GSyncResponse *response = NHttpRap_GetAnswer(wk->http);

            if (GSync_IsNewAccount(wk)) {
                GSyncMessage_ClearInfo(wk->msg);
            }
            if (response->status == 0) {
                GSync_ChangeState(wk, GSync_StateSendSave, 2456);
                return;
            }
            wk->error = response->status;
            GSync_ChangeState(wk, GSync_StateError, 2460);
        }
        return;
    }
    GSyncDisp_SetProgress(wk->disp, wk->idleProgress);
    if (++wk->idleProgress == 100) {
        if (GSync_IsNewAccount(wk)) {
            GSyncMessage_ClearInfo(wk->msg);
        }
        GSync_ChangeState(wk, GSync_StateSendSave, 2474);
    }
}

static void GSync_StateUpload(GSyncWork *wk) {
    u32 size;

    getDreamWorldStuffAddress(wk->save);
    if (GSyncMessage_IsPrintFinished(wk->msg)) {
        wk->frames = 0;
        if (GSync_IsOnline(wk)) {
            if (NHttpRap_SendRequest(GSYNC_REQUEST_UPLOAD, wk->http)) {
                void *data = func_02007454(wk->save, &size);

                func_ov189_021a0854(NHttpRap_GetConnection(wk->http), data, 0x80000);
                GSyncDisp_SetProgress(wk->disp, 0);
                wk->progress = 0;
                if (NHttpRap_StartRequest(wk->http) == 0) {
                    GSync_ChangeState(wk, GSync_StateWaitUpload, 2505);
                }
            }
        } else {
            GSyncDisp_SetProgress(wk->disp, 0);
            GSync_ChangeState(wk, GSync_StateWaitUpload, 2511);
        }
    }
}

static void GSync_StateSendAnimText(GSyncWork *wk) {
    if (--wk->timer == 0) {
        GSyncDisp_StartWave(wk->disp);
        GSyncMessage_PrintStream(wk->msg, 8);
        GSyncMessage_StartWaitIcon(wk->msg);
        GSyncDisp_StartFade(wk->disp, TRUE);
        GSyncDisp_SetActorSequence(wk->disp, 4, 5);
        GSyncDisp_SetActorSequence(wk->disp, 0, 0);
        GSyncDisp_CreateActor(wk->disp, 12);
        GSyncDisp_CreateActor(wk->disp, 13);
        GFL_SndSEPlay(SEQ_SE_SYS_24);
        GSync_ChangeState(wk, GSync_StateUpload, 2543);
    }
}

static void GSync_StateSendAnimLeave(GSyncWork *wk) {
    if (--wk->timer == 0) {
        GSyncDisp_CreateActor(wk->disp, 4);
        wk->timer = 60;
        GSync_ChangeState(wk, GSync_StateSendAnimText, 2560);
    }
}

static void GSync_StateSendAnimJump(GSyncWork *wk) {
    if (--wk->timer == 0) {
        GSyncDisp_SetActorSequence(wk->disp, 0, 1);
        GSyncDisp_CreateActor(wk->disp, 2);
        GSyncDisp_CreateActor(wk->disp, 3);
        wk->timer = 15;
        GSync_ChangeState(wk, GSync_StateSendAnimLeave, 2582);
    }
}

static void GSync_StateSendAnimIcon(GSyncWork *wk) {
    if (--wk->timer == 0) {
        GSyncDisp_CreateSubActor(wk->disp);
        GSyncDisp_StartPokeIcon(wk->disp);
        wk->timer = 70;
        GSync_ChangeState(wk, GSync_StateSendAnimJump, 2603);
    }
}

static void GSync_StateSendShowIcon(GSyncWork *wk) {
    GSyncDisp_CreatePokeIcon(wk->disp, func_0201d620(wk->pkm), CLACT_SURFACE_SUB);
    wk->timer = 10;
    GSync_ChangeState(wk, GSync_StateSendAnimIcon, 2621);
}

// Takes the Pokémon chosen out of its box into the Dream World save
static void GSync_StateSend(GSyncWork *wk) {
    DreamWorldSave *dreamWorld = getDreamWorldStuffAddress(wk->save);
    BoxPkm *boxPkm;

    if (wk->pkm != NULL) {
        GFL_HeapFree(wk->pkm);
        wk->pkm = NULL;
    }
    boxPkm = BoxSaveAccessor_GetPkm(wk->boxes, wk->boxTray, wk->boxPos);
    if (boxPkm != NULL) {
        PartyPkm *pkm = boxPkmRegenToPartyPkm(boxPkm, wk->heapId);

        func_0200999c(dreamWorld, pkm);
        wk->pkm = pkm;
        BoxSaveAccessor_ClearPkm(wk->boxes, wk->boxTray, wk->boxPos);
        func_02009a00(dreamWorld, TRUE);
        wk->pkmTaken = TRUE;
    }
    GSyncDisp_CreateActor(wk->disp, 0);
    GSyncDisp_CreateActor(wk->disp, 12);
    GSync_ChangeState(wk, GSync_StateSendShowIcon, 2673);
}

static void GSync_OnDisconnect(void *work, int a1, int code, int error) {
    GSyncWork *wk = work;

    if (wk->http != NULL) {
        NHttpRap_EndRequest(wk->http);
        NHttpRap_Destroy(wk->http);
        wk->http = NULL;
    }
}

// Undoes what Game Sync changed in the Dream World save, after an error
static void GSync_RestoreDreamWorld(GSyncWork *wk) {
    DreamWorldSave *dreamWorld = getDreamWorldStuffAddress(wk->save);

    if (wk->dreamWorldCopy != NULL) {
        sys_memcpy(wk->dreamWorldCopy, dreamWorld, func_02009930());
        GFL_HeapFree(wk->dreamWorldCopy);
        wk->dreamWorldCopy = NULL;
    }
    if (wk->pkmTaken) {
        PartyPkm *pkm = func_02009998(dreamWorld);

        BoxSaveAccessor_SetPkm(wk->boxes, wk->boxTray, wk->boxPos, func_0201d624(pkm));
        PokeParty_ClearPkm(pkm);
        func_02009a00(dreamWorld, FALSE);
        wk->pkmTaken = FALSE;
    }
}

static BOOL GSync_PersonExists(GSyncResultPerson *person) {
    if (person->profileId != 0) {
        return TRUE;
    }
    return FALSE;
}

static void GSync_PersonToPlayerInfo(GSyncResultPerson *person, PlayerInfo *info) {
    int offset = 0;
    u16 style;

    sys_memset(info, 0, 0x20);
    copyTrainerName(info, person->name);
    func_02008be0(info, person->profileId);
    func_02008c14(info, person->country, person->region);
    setTrainerGender(info, person->gender);
    setIDAsUInt(info, person->id);
    func_02008c00(info, person->unk1B);
    func_02008c08(info, person->unk1A);
    // The Union Room look: the ID picks one of 8, and girls have the other 8
    style = person->id & 7;
    if (person->gender) {
        offset = 8;
    }
    style = style + (u16)offset;
    func_02008bf8(info, style);
}

static void GSync_AddJoinAvenuePeople(GameData *gameData, GSyncResultPerson *people, HeapID heapId) {
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));
    JoinAvenueEntry *entry = func_02037a40(heapId);
    PlayerInfo *info = func_02008b0c(heapId);
    int i;

    for (i = 0; i < 12; i++) {
        GSyncResultPerson *person = &people[i];

        if (GSync_PersonExists(person)) {
            GSync_PersonToPlayerInfo(person, info);
            func_02037ab4(entry, info, person->species, 7);
            func_02010078(joinAvenue, gameData, entry, 2);
        }
    }
    GFL_HeapFree(info);
    func_02037a68(entry);
}

static BOOL GSync_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    EventGameSync *pParent = param;
    GSyncWork *wk;
    s32 profileID;

    GFL_OvlLoad(OVERLAY_ID(189));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_GSYNC, 0x88000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(GSyncWork), HEAPID_GSYNC);
    sys_memset(wk, 0, sizeof(GSyncWork));
    wk->heapId = HEAPID_GSYNC;
    GFL_ASSERT(pParent);
    if (pParent != NULL) {
        wk->param = pParent;
        wk->gameData = pParent->gameData;
        wk->save = GameData_GetSaveControl(pParent->gameData);
        profileID = func_02008bdc(GetGameDataPlayerInfo(pParent->gameData));
        wk->boxes = GameData_GetBoxSaveAccessor(pParent->gameData);
        wk->boxTray = pParent->boxTray;
        wk->boxPos = pParent->boxPosition;
        wk->idShownSaved = func_020099f4(getDreamWorldStuffAddress(wk->save));
        wk->idShown = wk->idShownSaved;
        getDreamWorldStuffAddress(wk->save);
        switch (pParent->gsyncResult) {
        case GSYNC_RESULT_NONE:
            GFL_ASSERT(profileID);
            GSync_ChangeState(wk, GSync_StateRequestAccount, 2869);
            break;
        case GSYNC_RESULT_ACCOUNT:
            GFL_ASSERT(profileID);
            GSync_ChangeState(wk, GSync_StateSend, 2873);
            break;
        case GSYNC_RESULT_CONNECT:
            GFL_ASSERT(profileID);
            GSync_ChangeState(wk, GSync_StateSend, 2877);
            break;
        case GSYNC_RESULT_WIFI_SETTINGS:
            GFL_ASSERT(profileID);
            GSync_ChangeState(wk, GSync_StateRequestAccount, 2881);
            break;
        case GSYNC_RESULT_NO_POKEMON:
            GSync_ChangeState(wk, GSync_StateNoPokemon, 2884);
            break;
        }
        if (pParent->gsyncResult != GSYNC_RESULT_NO_POKEMON) {
            wk->http = NHttpRap_Create(HEAPID_GSYNC, profileID, pParent->loginBuffer);
            DWCRap_SetErrorFunc(GSync_OnDisconnect, wk);
        }
    }
    wk->disp = GSyncDisp_Create(wk->heapId);
    wk->msg = GSyncMessage_Create(wk->heapId, 44);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
    func_02042ba8(FALSE, wk->heapId);
    return TRUE;
}

static BOOL GSync_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    GSyncWork *wk = work;
    GSyncStateFunc func = wk->state;
    BOOL done = TRUE;

    if (GFL_WipeIsFinished() && !wk->busy && GFL_NetErrCheck()) {
        if (wk->download != NULL) {
            GSyncDownload_Cleanup(wk->download);
        }
        NHttpRap_EndRequest(wk->http);
        if (wk->saving) {
            func_02017884(wk->gameData);
            wk->saving = FALSE;
        }
        GSync_RestoreDreamWorld(wk);
        if (wk->receiving) {
            func_02011d04(41);
        }
        Wipe_SetScreenCovered(0, WIPE_COLOR_BLACK);
        Wipe_SetScreenCovered(1, WIPE_COLOR_BLACK);
        DWCRapCommon_CheckError(1, 0);
        wk->param->gsyncResult = GSYNC_RESULT_RETRY_LOGIN;
        return TRUE;
    }
    if (func != NULL) {
        func(wk);
        done = FALSE;
    }
    if (wk->yesNo != NULL) {
        AppTaskMenu_Update(wk->yesNo);
    }
    if (wk->download != NULL) {
        GSyncDownload_Main(wk->download);
    }
    GSyncDisp_Main(wk->disp);
    GSyncMessage_Main(wk->msg);
    return done;
}

static BOOL GSync_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    GSyncWork *wk = work;

    if (!GFL_WipeIsFinished()) {
        return FALSE;
    }
    if (wk->yesNo != NULL) {
        AppTaskMenu_Free(wk->yesNo);
    }
    if (wk->result != NULL) {
        GFL_HeapFree(wk->result);
        wk->result = NULL;
    }
    GSyncMessage_Free(wk->msg);
    GSyncDisp_Free(wk->disp);
    GSyncDownload_Free(wk->download);
    if (wk->musical != NULL) {
        GFL_HeapFree(wk->musical);
    }
    if (wk->zukanSkin != NULL) {
        GFL_HeapFree(wk->zukanSkin);
    }
    if (wk->cgearSkin != NULL) {
        GFL_HeapFree(wk->cgearSkin);
    }
    if (wk->pkm != NULL) {
        GFL_HeapFree(wk->pkm);
    }
    if (wk->unk44 != NULL) {
        GFL_HeapFree(wk->unk44);
    }
    if (func_02042788()) {
        DWCRap_SetEventFunc(NULL, NULL);
    }
    if (wk->http != NULL) {
        NHttpRap_Destroy(wk->http);
        DWCRap_SetErrorFunc(NULL, NULL);
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_GSYNC);
    GFL_OvlUnload(OVERLAY_ID(189));
    return TRUE;
}
