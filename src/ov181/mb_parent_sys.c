#include "types.h"
#include "app/mb_parent.h"
#include "app/mb_parent/mb_comm_sys.h"
#include "app/mb_parent/mb_util_msg.h"
#include "app/mb_parent/mbp.h"
#include "constants/arc.h"
#include "constants/field_script.h"
#include "constants/items.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/backup_system.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "gfl/wih.h"
#include "gfl/wm_icon.h"
#include "nitro/card.h"
#include "nitro/fs.h"
#include "nitro/gx.h"
#include "nitro/mb.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include "pml/met_data.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/box.h"
#include "save/event_work.h"
#include "save/join_avenue.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "system/app_menu_common.h"
#include "system/dsi.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/startmenu_gfl_net_err_disp_message.h"

// The DS Download Play parent (mb_parent_sys.c): sends a program to another system by DS Download Play and stores the
// Pokémon it sends back. The Poké Transfer Lab starts it to bring Pokémon over from the Gen 4 games, and the start
// menu's item starts it with a game data of its own; that this is the Pokémon Dream Radar's transfer is our inference,
// from the second child program, the Lock Capsule it gives and the Dream Radar flag it checks

// The game's GGID for DS Download Play
#define MB_PARENT_GGID 0x1380

// The states of MBParent_Main
enum {
    MB_PARENT_STATE_WIPE_IN,
    MB_PARENT_STATE_WAIT_WIPE_IN,
    MB_PARENT_STATE_WIPE_OUT,
    MB_PARENT_STATE_WAIT_WIPE_OUT,
    MB_PARENT_STATE_ASK_START,
    MB_PARENT_STATE_WAIT_ASK_MSG,
    MB_PARENT_STATE_ASK_ANSWER,
    MB_PARENT_STATE_SETUP_DOWNLOAD,
    MB_PARENT_STATE_DOWNLOAD,
    MB_PARENT_STATE_END_DOWNLOAD,
    MB_PARENT_STATE_WAIT_NET_START,
    MB_PARENT_STATE_WAIT_CHILD,
    MB_PARENT_STATE_LOAD_CHILD_PROGRAM,
    MB_PARENT_STATE_CHILD_PROGRAM_LOADED,
    MB_PARENT_STATE_WAIT_CHILD_REQUEST,
    MB_PARENT_STATE_SEND_PROGRAM,
    MB_PARENT_STATE_WAIT_PROGRAM_SENT,
    MB_PARENT_STATE_WAIT_POKEMON,
    MB_PARENT_STATE_ACK_POKEMON,
    MB_PARENT_STATE_WAIT_ACK,
    MB_PARENT_STATE_STORE_POKEMON,
    MB_PARENT_STATE_SAVE,
    MB_PARENT_STATE_SAVED,
    MB_PARENT_STATE_SEND_BOX_SPACE,
    MB_PARENT_STATE_WAIT_CHILD_NEXT,
    MB_PARENT_STATE_FINISH,
    MB_PARENT_STATE_WAIT_FINISH,
    MB_PARENT_STATE_SEND_END,
    MB_PARENT_STATE_WAIT_END,
    MB_PARENT_STATE_WAIT_NET_END,
    MB_PARENT_STATE_TIMEOUT,
    MB_PARENT_STATE_WAIT_TIMEOUT,
    MB_PARENT_STATE_RESULT,
    MB_PARENT_STATE_NO_WIRELESS,
};

// The steps of the distribution, MBParent_UpdateDistribution's (the wireless, the measure of the channels, DS Download
// Play's entry and the wireless's end), and then of the save that the Pokémon received are stored with, which
// MBParent_UpdateTransfer runs in step with the child, in the work's distributionSeq
enum {
    MB_PARENT_DIST_SEQ_START_WIRELESS,
    MB_PARENT_DIST_SEQ_MEASURE_CHANNEL,
    MB_PARENT_DIST_SEQ_START_MBP,
    MB_PARENT_DIST_SEQ_ENTRY,
    MB_PARENT_DIST_SEQ_COMPLETE,
    MB_PARENT_DIST_SEQ_DONE,
    MB_PARENT_DIST_SEQ_END_WIRELESS,
    MB_PARENT_DIST_SEQ_WAIT_WIRELESS_END,
    MB_PARENT_DIST_SEQ_STOPPED,
    MB_PARENT_SAVE_SEQ_SEND_SYNC_1,
    MB_PARENT_SAVE_SEQ_WAIT_SYNC_1,
    MB_PARENT_SAVE_SEQ_START,
    MB_PARENT_SAVE_SEQ_WAIT_LAST,
    MB_PARENT_SAVE_SEQ_WAIT_CHILD_START,
    MB_PARENT_SAVE_SEQ_SEND_SYNC_3 = 16,
    MB_PARENT_SAVE_SEQ_SEND_SYNC_4,
    MB_PARENT_SAVE_SEQ_WAIT_SYNC_4 = 19,
    MB_PARENT_SAVE_SEQ_WAIT_FRAMES,
    MB_PARENT_SAVE_SEQ_FINISH,
    MB_PARENT_SAVE_SEQ_SEND_SYNC_2,
};

// The steps of the cancel of the distribution, in MBParent_UpdateEntry: none, the question printed, the yes/no menu,
// its answer and the cancel
enum {
    MB_PARENT_CANCEL_SEQ_NONE,
    MB_PARENT_CANCEL_SEQ_PRINT,
    MB_PARENT_CANCEL_SEQ_ASK,
    MB_PARENT_CANCEL_SEQ_ANSWER,
    MB_PARENT_CANCEL_SEQ_CANCELLED,
};

// The steps of MBParent_UpdateResult, for the start menu's transfer: the result the child sends, the questions about
// it and their answers, the Pokémon received, and the item the child offers
enum {
    MB_PARENT_RESULT_SEQ_WAIT_RESULT,
    MB_PARENT_RESULT_SEQ_ASK_RECEIVE,
    MB_PARENT_RESULT_SEQ_RECEIVE_ANSWER,
    MB_PARENT_RESULT_SEQ_PRINT_MORE,
    MB_PARENT_RESULT_SEQ_WAIT_MORE_MSG,
    MB_PARENT_RESULT_SEQ_PRINT_FLAG1,
    MB_PARENT_RESULT_SEQ_ASK_FLAG1,
    MB_PARENT_RESULT_SEQ_FLAG1_ANSWER,
    MB_PARENT_RESULT_SEQ_ASK_FLAG2,
    MB_PARENT_RESULT_SEQ_FLAG2_ANSWER,
    MB_PARENT_RESULT_SEQ_PRINT_CONFIRM,
    MB_PARENT_RESULT_SEQ_ASK_CONFIRM,
    MB_PARENT_RESULT_SEQ_CONFIRM_ANSWER,
    MB_PARENT_RESULT_SEQ_SEND_ANSWER,
    MB_PARENT_RESULT_SEQ_RECEIVE_POKEMON,
    MB_PARENT_RESULT_SEQ_ACK_POKEMON,
    MB_PARENT_RESULT_SEQ_WAIT_RECEIVED_MSG,
    MB_PARENT_RESULT_SEQ_PRINT_BOX_FULL,
    MB_PARENT_RESULT_SEQ_WAIT_BOX_FULL_MSG,
    MB_PARENT_RESULT_SEQ_WAIT_ITEM_INFO,
    MB_PARENT_RESULT_SEQ_ASK_ITEM,
    MB_PARENT_RESULT_SEQ_ITEM_ANSWER,
    MB_PARENT_RESULT_SEQ_WAIT_ITEM_OWNED_MSG,
    MB_PARENT_RESULT_SEQ_SEND_ITEM_ANSWER,
    MB_PARENT_RESULT_SEQ_WAIT_ITEM_MSG,
    MB_PARENT_RESULT_SEQ_FINISH,
};

typedef struct {
    HeapID heapId;
    TCB *vblankTask;
    MBParentParam *param;
    MBCommSys *comm;
    BOOL netError;
    BOOL wirelessError;
    BOOL eventsPaused;
    BOOL wirelessStarted;
    u32 state;
    u8 distributionSeq;
    u8 resultSeq;
    u8 cancelSeq;
    u8 waitFrames;
    u8 startMenu;
    u16 waitConnectFrames;
    BOOL childRomSent;
    BOOL distributionDone;
    int answer;
    u16 highScore;
    u16 transferCount;
    u16 receivedCount;
    BOOL boxFull;
    BOOL receivedMore;
    BOOL receivedItem;
    TrainerGameInfoSave *trainerGameInfo;
    u16 *gameName;
    u16 *gameIntro;
    MBCommParentInfo parentInfo;
    void *childRom;
    u32 childRomSize;
    MBUtilMsg *msg;
    ClActUnit *actorUnit;
    u32 cancelButtonPalette;
    u32 cancelButtonChars;
    u32 cancelButtonCellAnims;
    ClActor *cancelButton;
    MBGameRegistry registry;
} MBParentWork;

static void MBParent_Init(MBParentWork *wk);
static void MBParent_Exit(MBParentWork *wk);
static BOOL MBParent_Main(MBParentWork *wk);
static void MBParent_VBlankTask(TCB *tcb, void *data);
static void MBParent_ScrollBGs(void *data);
static void MBParent_ScrollBGsSlow(void *data);
static void MBParent_InitGraphics(MBParentWork *wk);
static void MBParent_FreeGraphics(MBParentWork *wk);
static void MBParent_CreateBG(const BGSetup *setup, u8 bg, u8 mode);
static void MBParent_LoadGraphics(MBParentWork *wk);
static void MBParent_DrawTopScreen(MBParentWork *wk);
static void MBParent_SetupGameInfo(MBParentWork *wk);
static void MBParent_FreeGameInfo(MBParentWork *wk);
static BOOL MBParent_UpdateDistribution(MBParentWork *wk);
static void MBParent_StartMBP(MBParentWork *wk);
static void MBParent_UpdateEntry(MBParentWork *wk);
static BOOL MBParent_OnWirelessDone(BOOL success);
static void MBParent_SetPalParkResult(MBParentWork *wk, u8 result);
static void MBParent_SoftResetCallback(void *work);
static void MBParent_ReceivePokemon(MBParentWork *wk);
static void MBParent_StorePokemon(MBParentWork *wk);
static void MBParent_Dummy(MBParentWork *wk);
static void MBParent_UpdateTransfer(MBParentWork *wk);
static void MBParent_UpdateResult(MBParentWork *wk);
static BOOL MBParent_CardPulledOut(void);
static BOOL MBParentProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MBParentProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL MBParentProc_Main(GameProc *proc, u32 *state, void *param, void *work);

static const BGSysLCDConfig sLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

static const BGSysVRAMConfig sVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const TouchRect sCancelButtonRect[] = {
    { 168, 192, 232, 255 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

GameProcFunctions MB_PARENT_PROC_FUNCTIONS = { MBParentProc_Init, MBParentProc_Main, MBParentProc_Exit };

static u8 sScrollWait;
static u16 sScrollFrames;
// Set by the wireless helper once it has shut down
static BOOL sWirelessDone;
// Whether the title is on the sub engine's BG 4, for the Poké Transfer Lab, rather than BG 2
static BOOL sTitleOnSub;

static void MBParent_Init(MBParentWork *wk) {
    wk->resultSeq = MB_PARENT_RESULT_SEQ_WAIT_RESULT;
    wk->state = MB_PARENT_STATE_WIPE_IN;
    wk->startMenu = wk->param->startMenu;
    MBParent_InitGraphics(wk);
    MBParent_LoadGraphics(wk);
    if (wk->startMenu == FALSE) {
        wk->msg = MBUtilMsg_Create(wk->heapId, 5, 5, 0x5c, 0, 0);
    } else {
        wk->msg = MBUtilMsg_Create(wk->heapId, 1, 1, 0x5c, 0, 1);
    }
    wk->comm = MBComm_Create(wk->heapId);
    wk->childRomSent = FALSE;
    wk->childRom = NULL;
    MBParent_DrawTopScreen(wk);
    if (wk->startMenu == FALSE) {
        wk->trainerGameInfo = getTrainerGameInfoAddress(GameData_GetSaveControl(wk->param->gameData));
        TrainerGameInfo_SetPalParkResult(wk->trainerGameInfo, PAL_PARK_RESULT_CANCELED);
    } else {
        wk->trainerGameInfo = NULL;
        sScrollFrames = 0;
    }
    wk->vblankTask = GFL_VBlankTCBAdd(MBParent_VBlankTask, wk, 8);
    if (wk->startMenu == FALSE) {
        GFL_VBlankSetCallback(MBParent_ScrollBGs, NULL);
    } else {
        GFL_VBlankSetCallback(MBParent_ScrollBGsSlow, NULL);
        GFL_SndBGMPlay(SEQ_BGM_WIFI_PRESENT, SND_CHANNEL_MASK_ALL);
    }
    wk->wirelessStarted = FALSE;
    wk->receivedMore = FALSE;
    wk->receivedItem = FALSE;
    wk->highScore = 0;
    wk->transferCount = 0;
    wk->receivedCount = 0;
    GCTX_HIDBlockSleep(0x10);
}

static void MBParent_Exit(MBParentWork *wk) {
    GCTX_HIDUnblockSleep(0x10);
    GFL_TCBRemove(wk->vblankTask);
    GFL_VBlankResetCallback();
    WmIcon_Delete();
    if (wk->startMenu == TRUE) {
        GFL_SndBGMStopImmediate();
    } else {
        EventWork *eventWork = GameData_GetEventWork(wk->param->gameData);
        u16 *highScore = EventWork_GetWkPtr(eventWork, VARS_START);
        u16 *transferCount = EventWork_GetWkPtr(eventWork, VARS_START + 1);
        *highScore = wk->highScore;
        *transferCount = wk->transferCount;
    }
    if (wk->childRom != NULL) {
        GFL_HeapFree(wk->childRom);
    }
    MBComm_Delete(wk->comm);
    MBUtilMsg_Delete(wk->msg);
    MBParent_FreeGraphics(wk);
}

static BOOL MBParent_Main(MBParentWork *wk) {
    MBComm_Update(wk->comm);
    if (wk->netError == TRUE && wk->state != MB_PARENT_STATE_WIPE_OUT && wk->state != MB_PARENT_STATE_WAIT_WIPE_OUT) {
        wk->eventsPaused = GameData_CheckEventsPaused(wk->param->gameData);
        if (wk->eventsPaused == TRUE) {
            GameData_CancelSave(wk->param->gameData);
        }
        wk->state = MB_PARENT_STATE_WIPE_OUT;
        MBParent_SetPalParkResult(wk, PAL_PARK_RESULT_ERROR);
    }
    switch (wk->state) {
    case MB_PARENT_STATE_WIPE_IN:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, 6, 1, wk->heapId);
        wk->state = MB_PARENT_STATE_WAIT_WIPE_IN;
        break;
    case MB_PARENT_STATE_WAIT_WIPE_IN:
        if (GFL_WipeIsFinished() == TRUE) {
            if (isWirelessEnabled() == TRUE) {
                if (wk->startMenu == FALSE) {
                    wk->state = MB_PARENT_STATE_SETUP_DOWNLOAD;
                } else {
                    wk->state = MB_PARENT_STATE_ASK_START;
                }
            } else {
                MBUtilMsg_SetWindow(wk->msg, 5);
                MBUtilMsg_PrintNoWireless(wk->msg, TextSpeed_GetCurrentWait());
                MBUtilMsg_ShowWindow(wk->msg, 1);
                wk->state = MB_PARENT_STATE_NO_WIRELESS;
            }
        }
        break;
    case MB_PARENT_STATE_WIPE_OUT:
        if (wk->param->startMenu == FALSE) {
            GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, 6, 1, wk->heapId);
        } else {
            GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0x7fff, 6, 1, wk->heapId);
            GFL_SndBGMFadeOut(6);
        }
        wk->state = MB_PARENT_STATE_WAIT_WIPE_OUT;
        break;
    case MB_PARENT_STATE_WAIT_WIPE_OUT:
        if (GFL_WipeIsFinished() == TRUE) {
            return TRUE;
        }
        break;
    case MB_PARENT_STATE_ASK_START:
        MBUtilMsg_SetWindow(wk->msg, 5);
        MBUtilMsg_Print(wk->msg, 0x2b, TextSpeed_GetCurrentWait());
        wk->state = MB_PARENT_STATE_WAIT_ASK_MSG;
        break;
    case MB_PARENT_STATE_WAIT_ASK_MSG:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            MBUtilMsg_CreateConfirm(wk->msg, 0);
            wk->state = MB_PARENT_STATE_ASK_ANSWER;
        }
        break;
    case MB_PARENT_STATE_ASK_ANSWER: {
        int answer = MBUtilMsg_UpdateConfirm(wk->msg);
        if (answer == MB_UTIL_MSG_YES) {
            wk->state = MB_PARENT_STATE_SETUP_DOWNLOAD;
            MBUtilMsg_ClearWindow(wk->msg);
            MBUtilMsg_ForgetConfirm(wk->msg);
        } else if (answer == MB_UTIL_MSG_NO) {
            wk->state = MB_PARENT_STATE_WIPE_OUT;
        }
        break;
    }
    case MB_PARENT_STATE_SETUP_DOWNLOAD:
        MBParent_SetupGameInfo(wk);
        wk->state = MB_PARENT_STATE_DOWNLOAD;
        break;
    case MB_PARENT_STATE_DOWNLOAD:
        if (MBParent_UpdateDistribution(wk) == TRUE) {
            wk->state = MB_PARENT_STATE_END_DOWNLOAD;
        }
        break;
    case MB_PARENT_STATE_END_DOWNLOAD:
        MBParent_FreeGameInfo(wk);
        if (wk->distributionDone == TRUE) {
            wk->state = MB_PARENT_STATE_WAIT_NET_START;
            wk->waitConnectFrames = 0;
            MBComm_StartNet(wk->comm);
            if (wk->startMenu == FALSE) {
                GFL_NetSetIconScreen(FALSE, wk->heapId);
            } else {
                GFL_NetSetIconScreen(TRUE, wk->heapId);
            }
        } else {
            wk->state = MB_PARENT_STATE_WIPE_OUT;
        }
        break;
    case MB_PARENT_STATE_WAIT_NET_START:
        if (MBComm_IsNetReady(wk->comm) == TRUE) {
            wk->state = MB_PARENT_STATE_WAIT_CHILD;
            MBComm_Connect(wk->comm);
        }
        break;
    case MB_PARENT_STATE_WAIT_CHILD:
        wk->waitConnectFrames++;
        if (wk->waitConnectFrames >= 1800) {
            MBParent_SetPalParkResult(wk, PAL_PARK_RESULT_ERROR);
            wk->state = MB_PARENT_STATE_TIMEOUT;
        } else if (MBComm_IsConnected(wk->comm) == TRUE) {
            wk->parentInfo.textSpeed = TextSpeed_GetCurrentWait();
            wk->parentInfo.language = GFL_MsgDataGetDefaultLangID();
            if (wk->startMenu == FALSE) {
                wk->parentInfo.highScore = TrainerGameInfo_GetPalParkHighScore(wk->trainerGameInfo);
            }
            if (MBComm_SendParentInfo(wk->comm, &wk->parentInfo) == TRUE) {
                if (wk->startMenu == FALSE) {
                    wk->state = MB_PARENT_STATE_LOAD_CHILD_PROGRAM;
                } else {
                    wk->state = MB_PARENT_STATE_RESULT;
                }
            }
        }
        break;
    case MB_PARENT_STATE_LOAD_CHILD_PROGRAM: {
        FSFile file;
        const char *path = GFL_ArcSysGetResourcePath(142);
        BOOL result;

        FS_InitFile(&file);
        result = FS_OpenFile(&file, path);
        GFL_ASSERT(result);
        FS_SeekFile(&file, 0, FS_SEEK_SET);
        wk->childRomSize = FS_GetLength(&file);
        wk->childRom = GFL_HeapAllocate(wk->heapId, wk->childRomSize, TRUE, "mb_parent_sys.c", 665);
        FS_ReadFile(&file, wk->childRom, wk->childRomSize);
        result = FS_CloseFile(&file);
        GFL_ASSERT(result);
        wk->state = MB_PARENT_STATE_CHILD_PROGRAM_LOADED;
        break;
    }
    case MB_PARENT_STATE_CHILD_PROGRAM_LOADED:
        wk->state = MB_PARENT_STATE_WAIT_CHILD_REQUEST;
        break;
    case MB_PARENT_STATE_WAIT_CHILD_REQUEST:
        if (MBComm_GetState(wk->comm) == MB_COMM_STATE_ERROR_1) {
            MBParent_SetPalParkResult(wk, PAL_PARK_RESULT_ERROR);
            MBUtilMsg_Print(wk->msg, 8, TextSpeed_GetCurrentWait());
            MBComm_StartDisconnect(wk->comm);
            wk->state = MB_PARENT_STATE_WAIT_END;
        } else if (MBComm_GetState(wk->comm) == MB_COMM_STATE_REQUEST_PROGRAM) {
            if (wk->childRomSent == FALSE) {
                wk->childRomSent = TRUE;
                MBComm_SendProgram(wk->comm, wk->childRom, wk->childRomSize);
            }
            MBUtilMsg_Print(wk->msg, 2, TextSpeed_GetCurrentWait());
            wk->state = MB_PARENT_STATE_SEND_PROGRAM;
        }
        break;
    case MB_PARENT_STATE_SEND_PROGRAM:
        if (MBComm_GetState(wk->comm) == MB_COMM_STATE_SELECT_POKEMON) {
            MBUtilMsg_Print(wk->msg, 3, TextSpeed_GetCurrentWait());
            wk->state = MB_PARENT_STATE_WAIT_PROGRAM_SENT;
        } else if (MBComm_GetState(wk->comm) == MB_COMM_STATE_CANCELED) {
            MBParent_SetPalParkResult(wk, PAL_PARK_RESULT_CANCELED);
            MBUtilMsg_Print(wk->msg, 8, TextSpeed_GetCurrentWait());
            MBComm_StartDisconnect(wk->comm);
            wk->state = MB_PARENT_STATE_WAIT_END;
        }
        // fallthrough
    case MB_PARENT_STATE_WAIT_PROGRAM_SENT:
        if (MBComm_GetState(wk->comm) == MB_COMM_STATE_TRANSFER) {
            MBComm_ClearPokemon(wk->comm);
            MBUtilMsg_Print(wk->msg, 4, TextSpeed_GetCurrentWait());
            wk->state = MB_PARENT_STATE_WAIT_POKEMON;
        }
        break;
    case MB_PARENT_STATE_WAIT_POKEMON:
        if (MBComm_GetState(wk->comm) == MB_COMM_STATE_POKEMON_SENT) {
            RecordAddOne(GameData_GetRecords(wk->param->gameData), 0x78);
            wk->transferCount++;
            wk->state = MB_PARENT_STATE_ACK_POKEMON;
        } else if (MBComm_GetState(wk->comm) == MB_COMM_STATE_NO_POKEMON) {
            RecordAddOne(GameData_GetRecords(wk->param->gameData), 0x78);
            wk->transferCount++;
            MBParent_SetPalParkResult(wk, PAL_PARK_RESULT_NONE_TRANSFERRED);
            wk->state = MB_PARENT_STATE_SEND_BOX_SPACE;
        }
        break;
    case MB_PARENT_STATE_ACK_POKEMON:
        if (MBComm_IsPokemonReceived(wk->comm) == TRUE && MBComm_SendCommand(wk->comm, MB_COMM_CMD_ACK, 0) == TRUE) {
            wk->state = MB_PARENT_STATE_WAIT_ACK;
        }
        break;
    case MB_PARENT_STATE_WAIT_ACK:
        if (MBComm_IsAcked(wk->comm) == TRUE || wk->startMenu == TRUE) {
            if (MBComm_GetState(wk->comm) == MB_COMM_STATE_WAIT_SAVE) {
                wk->state = MB_PARENT_STATE_STORE_POKEMON;
            } else if (MBComm_GetState(wk->comm) == MB_COMM_STATE_ERROR_1) {
                if (wk->startMenu == FALSE) {
                    MBParent_SetPalParkResult(wk, PAL_PARK_RESULT_ERROR);
                    MBUtilMsg_Print(wk->msg, 8, TextSpeed_GetCurrentWait());
                    MBComm_StartDisconnect(wk->comm);
                    wk->state = MB_PARENT_STATE_WAIT_END;
                } else {
                    MBUtilMsg_Print(wk->msg, 0x26, TextSpeed_GetCurrentWait());
                    MBUtilMsg_ShowWindow(wk->msg, 1);
                    wk->state = MB_PARENT_STATE_WAIT_FINISH;
                }
            }
        }
        break;
    case MB_PARENT_STATE_STORE_POKEMON:
        MBParent_ReceivePokemon(wk);
        wk->state = MB_PARENT_STATE_SAVE;
        wk->distributionSeq = MB_PARENT_SAVE_SEQ_SEND_SYNC_1;
        break;
    case MB_PARENT_STATE_SAVE:
        MBParent_UpdateTransfer(wk);
        break;
    case MB_PARENT_STATE_SAVED:
        MBParent_Dummy(wk);
        if (wk->startMenu == FALSE) {
            MBUtilMsg_Print(wk->msg, 7, TextSpeed_GetCurrentWait());
            wk->state = MB_PARENT_STATE_SEND_BOX_SPACE;
        } else {
            MBUtilMsg_Print(wk->msg, 0x27, TextSpeed_GetCurrentWait());
            MBUtilMsg_ShowWindow(wk->msg, 1);
            wk->state = MB_PARENT_STATE_SEND_END;
        }
        break;
    case MB_PARENT_STATE_SEND_BOX_SPACE:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            u16 space = BoxSaveAccessor_CountEmptySlots(GameData_GetBoxSaveAccessor(wk->param->gameData));

            if (MBComm_SendCommand(wk->comm, MB_COMM_CMD_BOX_SPACE, space) == TRUE) {
                MBUtilMsg_Print(wk->msg, 0xb, TextSpeed_GetCurrentWait());
                wk->state = MB_PARENT_STATE_WAIT_CHILD_NEXT;
            }
        }
        break;
    case MB_PARENT_STATE_WAIT_CHILD_NEXT:
        if (MBComm_GetState(wk->comm) == MB_COMM_STATE_REQUEST_PROGRAM) {
            MBComm_ResetCommands(wk->comm);
            wk->state = MB_PARENT_STATE_WAIT_CHILD_REQUEST;
        } else if (MBComm_GetState(wk->comm) == MB_COMM_STATE_FINISHED ||
                   MBComm_GetState(wk->comm) == MB_COMM_STATE_ERROR_1) {
            if (MBComm_GetState(wk->comm) == MB_COMM_STATE_ERROR_1) {
                MBParent_SetPalParkResult(wk, PAL_PARK_RESULT_ERROR);
            }
            MBUtilMsg_Print(wk->msg, 8, TextSpeed_GetCurrentWait());
            wk->state = MB_PARENT_STATE_SEND_END;
        }
        break;
    case MB_PARENT_STATE_SEND_END:
        if (MBComm_SendCommand(wk->comm, MB_COMM_CMD_END, 0) == TRUE) {
            MBComm_StartDisconnect(wk->comm);
            wk->state = MB_PARENT_STATE_WAIT_END;
        }
        break;
    case MB_PARENT_STATE_WAIT_END:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE && MBComm_IsDisconnected(wk->comm) == TRUE) {
            MBComm_EndNet(wk->comm);
            wk->state = MB_PARENT_STATE_WAIT_NET_END;
        }
        break;
    case MB_PARENT_STATE_WAIT_NET_END:
        if (MBComm_IsNetEnded(wk->comm) == TRUE) {
            if (wk->startMenu == FALSE) {
                wk->state = MB_PARENT_STATE_WIPE_OUT;
            } else {
                wk->state = MB_PARENT_STATE_FINISH;
            }
        }
        break;
    case MB_PARENT_STATE_FINISH:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            MBUtilMsg_ClearWindow(wk->msg);
            MBUtilMsg_SetWindow(wk->msg, 1);
            MBUtilMsg_Print(wk->msg, 0x23, TextSpeed_GetCurrentWait());
            MBUtilMsg_ShowWindow(wk->msg, 1);
            wk->state = MB_PARENT_STATE_WAIT_FINISH;
        }
        break;
    case MB_PARENT_STATE_WAIT_FINISH:
        if (MBComm_IsNetEnded(wk->comm) == TRUE && MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            wk->state = MB_PARENT_STATE_WIPE_OUT;
        }
        break;
    case MB_PARENT_STATE_TIMEOUT:
        MBUtilMsg_Print(wk->msg, 0xa, TextSpeed_GetCurrentWait());
        MBUtilMsg_ShowWindow(wk->msg, 1);
        wk->state = MB_PARENT_STATE_WAIT_TIMEOUT;
        MBComm_EndNet(wk->comm);
        break;
    case MB_PARENT_STATE_WAIT_TIMEOUT:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            wk->state = MB_PARENT_STATE_WAIT_NET_END;
        }
        // fallthrough: the original has no break here
    case MB_PARENT_STATE_RESULT:
        MBParent_UpdateResult(wk);
        break;
    case MB_PARENT_STATE_NO_WIRELESS:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            wk->state = MB_PARENT_STATE_WIPE_OUT;
        }
        break;
    }
    MBUtilMsg_Update(wk->msg);
    ClActSys_Draw();
    return FALSE;
}

static void MBParent_VBlankTask(TCB *tcb, void *data) {
    ClActSys_VBlank();
}

static void MBParent_ScrollBGs(void *data) {
    if (sScrollWait > 1) {
        sScrollWait = 0;
        GFL_BGSysMoveBG(3, BG_MOVE_LEFT, 1);
        GFL_BGSysMoveBG(3, BG_MOVE_UP, 1);
        GFL_BGSysMoveBG(7, BG_MOVE_LEFT, 1);
        GFL_BGSysMoveBG(7, BG_MOVE_UP, 1);
    } else {
        sScrollWait++;
    }
}

static void MBParent_ScrollBGsSlow(void *data) {
    sScrollFrames++;
    if (sScrollFrames >= 5) {
        sScrollFrames = 0;
        GFL_BGSysMoveBG(3, BG_MOVE_UP, 1);
        GFL_BGSysMoveBG(7, BG_MOVE_UP, 1);
    }
}

static void MBParent_InitGraphics(MBParentWork *wk) {
    // BG 7's setup comes first, which gives the ROM's layout of them
    static const BGSetup sBG7Setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, 0xe, 6, 0x8000, 1, 3, 0, 0 };
    static const BGSetup sBG0Setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, 0xb, 0, 0x5800, 0, 0, 0, 0 };
    static const BGSetup sBG1Setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, 0xc, 6, 0x8000, 0, 1, 0, 0 };
    static const BGSetup sBG2Setup = { 0, 0, 0x1000, 0, BGRES_512x256, GX_BG_COLORMODE_16, 0xd, 2, 0x8000, 1, 2, 1, 0 };
    static const BGSetup sBG3Setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, 0xf, 4, 0x8000, 1, 3, 0, 0 };
    static const BGSetup sBG4Setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, 0xf, 0, 0x6000, 1, 0, 0, 0 };
    static const BGSetup sBG5Setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, 0xc, 4, 0x8000, 1, 1, 0, 0 };
    static const BGSetup sBG6Setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, 0xd, 2, 0x8000, 1, 2, 0, 0 };
    ClActSysSetup clactSetup;

    GFL_BGSysDisableAllA();
    GFL_BGSysDisableAllB();
    GX_SetVisiblePlane(0);
    GXS_SetVisiblePlane(0);
    Wipe_SetScreenCovered(0, 0);
    Wipe_SetScreenCovered(1, 0);
    Wipe_HideWindows(0);
    Wipe_HideWindows(1);
    G2_BlendNone();
    G2S_BlendNone();
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GFL_BGSysSetVRAMBanks(&sVRAMConfig);
    GFL_BGSysCreate(wk->heapId);
    BmpWin_InitAllocator(wk->heapId);
    GFL_BGSysSetLCDConfig(&sLCDConfig);
    MBParent_CreateBG(&sBG0Setup, 0, BGMODE_TEXT);
    MBParent_CreateBG(&sBG1Setup, 1, BGMODE_TEXT);
    MBParent_CreateBG(&sBG2Setup, 2, BGMODE_TEXT);
    MBParent_CreateBG(&sBG3Setup, 3, BGMODE_TEXT);
    MBParent_CreateBG(&sBG4Setup, 4, BGMODE_TEXT);
    MBParent_CreateBG(&sBG5Setup, 5, BGMODE_TEXT);
    MBParent_CreateBG(&sBG6Setup, 6, BGMODE_TEXT);
    MBParent_CreateBG(&sBG7Setup, 7, BGMODE_TEXT);
    if (wk->startMenu == FALSE) {
        GFL_BGSysSetBGEnabled(6, TRUE);
    } else {
        GFL_BGSysSetBGEnabled(6, FALSE);
    }
    clactSetup = data_02093f08;
    clactSetup.charCount = 64;
    ClActSys_Create(&clactSetup, &sVRAMConfig, wk->heapId);
    wk->actorUnit = ClActUnit_Create(8, 0, wk->heapId);
    ClActUnit_UseSystemRenderer(wk->actorUnit);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void MBParent_FreeGraphics(MBParentWork *wk) {
    ClActor_Delete(wk->cancelButton);
    ClActUnit_Delete(wk->actorUnit);
    ClActSys_Delete();
    GFL_BGSysReleaseBG(0);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    BmpWin_FreeAllocator();
    GFL_BGSysFree();
}

static void MBParent_CreateBG(const BGSetup *setup, u8 bg, u8 mode) {
    GFL_BGSysCreateBG(bg, setup, mode);
    GFL_BGSysSetBGEnabled(bg, TRUE);
    GFL_BGSysClearBG(bg);
    GFL_BGSysLoadScr(bg);
}

static void MBParent_LoadGraphics(MBParentWork *wk) {
    ArcTool *arc;

    if (wk->startMenu == FALSE) {
        arc = GFL_ArcSysCreateFileHandle(ARCID_DATA_CONVERT, wk->heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 1, 4, 0, 0, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 5, 7, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 9, 7, 0, 0, FALSE, wk->heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0, 0, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 4, 3, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 8, 3, 0, 0, FALSE, wk->heapId);
        GFL_G2DIOLoadArcNCLR(arc, 2, 0, 0x120, 0x120, 0x80, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 6, 2, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 10, 2, 0, 0, FALSE, wk->heapId);
        GFL_G2DIOLoadArcNCLR(arc, 3, 0, 0x1a0, 0x1a0, 0x20, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 7, 0, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 11, 0, 0, 0, FALSE, wk->heapId);
        GFL_BGSysLoadScr(2);
        GFL_ArcToolFree(arc);
    } else {
        arc = GFL_ArcSysCreateFileHandle(ARCID_MYSTERY, wk->heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 6, 4, 0, 0, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 15, 7, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 31, 7, 0, 0, FALSE, wk->heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, 6, 0, 0, 0, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 15, 3, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 31, 3, 0, 0, FALSE, wk->heapId);
        GFL_ArcToolFree(arc);
        arc = GFL_ArcSysCreateFileHandle(ARCID_DATA_CONVERT, wk->heapId);
        GFL_G2DIOLoadArcNCLR(arc, 2, 4, 0x120, 0x120, 0x80, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 6, 5, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 10, 5, 0, 0, FALSE, wk->heapId);
        GFL_G2DIOLoadArcNCLR(arc, 3, 4, 0x1a0, 0x1a0, 0x20, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, 7, 4, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 11, 4, 0, 0, FALSE, wk->heapId);
        GFL_BGSysLoadScr(5);
        GFL_BGSysSetBGEnabled(5, FALSE);
        GFL_BGSysSetBGEnabled(4, FALSE);
        GFL_ArcToolFree(arc);
    }
    {
        ClActorSetup setup;

        arc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), wk->heapId);
        GFL_G2DIOLoadArcNCLRDefault(arc, AppMenuCommon_GetBarBgPaletteFile(), 4, 0x100, 0x20, wk->heapId);
        GFL_BGSysLoadArcNCGRStatic(arc, AppMenuCommon_GetBarBgCharFile(), 6, 0, 0, FALSE, wk->heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, AppMenuCommon_GetBarScreenFile(), 6, 0, 0, FALSE, wk->heapId);
        GFL_BGSysSetScrPaletteNo(6, 0, 21, 32, 3, 8);
        GFL_BGSysLoadScr(6);
        wk->cancelButtonPalette =
            ClActSys_LoadPlttRange(arc, AppMenuCommon_GetTouchBarPaletteFile(), CLACT_VRAM_SUB, 0, 0, 3, wk->heapId);
        wk->cancelButtonChars =
            ClActSys_LoadChars(arc, AppMenuCommon_GetTouchBarCharFile(), FALSE, CLACT_VRAM_SUB, wk->heapId);
        wk->cancelButtonCellAnims = ClActSys_LoadCellAnims(arc, AppMenuCommon_GetTouchBarCellFile(2),
                                                           AppMenuCommon_GetTouchBarAnimFile(2), wk->heapId);
        GFL_ArcToolFree(arc);
        setup.x = 232;
        setup.y = 168;
        setup.sequence = 1;
        setup.bgPriority = 0;
        setup.priority = 0;
        wk->cancelButton = ClActor_Create(wk->actorUnit, wk->cancelButtonChars, wk->cancelButtonPalette,
                                          wk->cancelButtonCellAnims, &setup, CLACT_SURFACE_SUB, wk->heapId);
    }
    if (wk->startMenu == FALSE) {
        ClActor_SetVisible(wk->cancelButton, TRUE);
    } else {
        ClActor_SetVisible(wk->cancelButton, FALSE);
    }
}

static void MBParent_DrawTopScreen(MBParentWork *wk) {
    u8 bg;
    ArcTool *arc;
    BmpWin *window;
    MsgData *msgData;
    StrBuf *strbuf;

    if (wk->startMenu == FALSE) {
        sTitleOnSub = TRUE;
        bg = 4;
    } else {
        sTitleOnSub = FALSE;
        bg = 2;
    }
    GFL_BGSysSetBGEnabled(bg, FALSE);
    arc = GFL_ArcSysCreateFileHandle(ARCID_ERROR_WINDOW_GRA, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 4, 0xe0, 0x20, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0xe0, 0x20, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 1, bg, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 2, bg, 0, 0, FALSE, wk->heapId);
    GFL_BGSysSetScrPaletteNo(bg, 0, 0, 32, 24, 7);
    GFL_BGSysLoadScr(bg);
    GFL_ArcToolFree(arc);
    window = BmpWin_CreateDynamic(bg, 1, 4, 30, 8, 7, TRUE);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_STARTMENU_GFL_NET_ERR_DISP_MESSAGE, wk->heapId);
    strbuf = GFL_MsgDataLoadStrbufNew(msgData, StartmenuGflNetErrDispMessage_Text_CommunicationErrorPleaseTurn);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 7);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(window), 0, 0, strbuf, MBUtilMsg_GetFont(wk->msg), 0x1167);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    GFL_StrBufFree(strbuf);
    GFL_MsgDataFree(msgData);
    BmpWin_Free(window);
    ((u16 *)HW_BG_PLTT)[0] = GX_RGB(10, 23, 31);
    ((u16 *)HW_DB_BG_PLTT)[0] = GX_RGB(10, 23, 31);
}

static void MBParent_SetupGameInfo(MBParentWork *wk) {
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(wk->param->gameData);
    StrBuf *src;
    StrBuf *name;
    StrBuf *intro;
    u16 nameLength;
    u16 introLength;
    int i;

    wk->distributionSeq = MB_PARENT_DIST_SEQ_START_WIRELESS;
    wk->cancelSeq = MB_PARENT_CANCEL_SEQ_NONE;
    wk->gameName = GFL_HeapAllocate(wk->heapId, 0x60, TRUE, "mb_parent_sys.c", 1368);
    wk->gameIntro = GFL_HeapAllocate(wk->heapId, 0xc0, TRUE, "mb_parent_sys.c", 1369);
    MBUtilMsg_SetWindow(wk->msg, 1);
    MBUtilMsg_CreateWordSet(wk->msg);
    MBUtilMsg_SetNumberZeroPadded(wk->msg, 0, getTrainerID(playerInfo), 5);
    if (wk->startMenu == FALSE) {
        src = GFL_MsgDataLoadStrbufNew(MBUtilMsg_GetMsgData(wk->msg), 0xc);
    } else {
        src = GFL_MsgDataLoadStrbufNew(MBUtilMsg_GetMsgData(wk->msg), 0xf);
    }
    name = GFL_StrBufCreate(0x100, wk->heapId);
    GFL_WordSetFormatStrbuf(MBUtilMsg_GetWordSet(wk->msg), name, src);
    GFL_StrBufFree(src);
#ifdef BLACK2
    if (wk->startMenu == FALSE) {
        intro = GFL_MsgDataLoadStrbufNew(MBUtilMsg_GetMsgData(wk->msg), 0xd);
    } else {
        intro = GFL_MsgDataLoadStrbufNew(MBUtilMsg_GetMsgData(wk->msg), 0x10);
    }
#else
    if (wk->startMenu == FALSE) {
        intro = GFL_MsgDataLoadStrbufNew(MBUtilMsg_GetMsgData(wk->msg), 0xe);
    } else {
        intro = GFL_MsgDataLoadStrbufNew(MBUtilMsg_GetMsgData(wk->msg), 0x11);
    }
#endif
    nameLength = GFL_StrBufGetCharCount(name);
    introLength = GFL_StrBufGetCharCount(intro);
    MI_CpuCopy16(GFL_StrBufGetStringPtr(name), wk->gameName, nameLength * 2);
    MI_CpuCopy16(GFL_StrBufGetStringPtr(intro), wk->gameIntro, introLength * 2);
    GFL_StrBufFree(name);
    GFL_StrBufFree(intro);
    // The child's font has the characters of the game's charset at other codes
    wk->gameName[nameLength] = 0;
    wk->gameIntro[introLength] = 0;
    for (i = 0; i < 0x60; i++) {
        if (wk->gameIntro[i] == 0xfffe) {
            wk->gameIntro[i] = 0xa;
        } else if (wk->gameIntro[i] >= 0xff10 && wk->gameIntro[i] <= 0xff19) {
            wk->gameIntro[i] -= 0xfee0;
        }
    }
    for (i = 0; i < 0x30; i++) {
        if (wk->gameName[i] == 0xff29) {
            wk->gameName[i] = 0x49;
        } else if (wk->gameName[i] == 0xff24) {
            wk->gameName[i] = 0x44;
        } else if (wk->gameName[i] >= 0xff10 && wk->gameName[i] <= 0xff19) {
            wk->gameName[i] -= 0xfee0;
        }
    }
    wk->distributionDone = FALSE;
    GFL_OvlLoad(OVERLAY_ID(30));
    if (wk->startMenu == FALSE) {
        GFL_BGSysSetBGEnabled(2, TRUE);
        GFL_BGSysSetBGEnabled(0, TRUE);
    } else {
        GFL_BGSysSetBGEnabled(5, TRUE);
        GFL_BGSysSetBGEnabled(4, TRUE);
    }
    CARD_SetPulledOutCallback(MBParent_CardPulledOut);
}

static void MBParent_FreeGameInfo(MBParentWork *wk) {
    GFL_OvlUnload(OVERLAY_ID(30));
    CARD_SetPulledOutCallback(NULL);
    GFL_HeapFree(wk->gameName);
    GFL_HeapFree(wk->gameIntro);
    if (wk->startMenu == FALSE) {
        GFL_BGSysSetBGEnabled(2, FALSE);
        GFL_BGSysSetBGEnabled(0, FALSE);
    } else {
        GFL_BGSysSetBGEnabled(5, FALSE);
        GFL_BGSysSetBGEnabled(4, FALSE);
    }
}

static BOOL MBParent_UpdateDistribution(MBParentWork *wk) {
    if (WH_GetSystemState() == WH_SYSSTATE_ERROR || WH_GetSystemState() == WH_SYSSTATE_FATAL) {
        wk->distributionDone = FALSE;
        wk->wirelessError = TRUE;
        if (WH_Finalize() == TRUE) {
            MBP_FreeBuffers();
            WH_Release();
            return TRUE;
        }
        return FALSE;
    }
    switch (wk->distributionSeq) {
    case MB_PARENT_DIST_SEQ_START_WIRELESS:
        sWirelessDone = FALSE;
        wk->wirelessStarted = TRUE;
        WH_Initialize(wk->heapId, MBParent_OnWirelessDone, 0);
        WmIcon_Create(240, 0, 0, wk->heapId);
        if (wk->startMenu == FALSE) {
            WmIcon_SetScreen(FALSE, wk->heapId);
        } else {
            WmIcon_SetScreen(TRUE, wk->heapId);
        }
        wk->distributionSeq = MB_PARENT_DIST_SEQ_MEASURE_CHANNEL;
        GCTX_HIDSetSoftResetCallback(MBParent_SoftResetCallback, wk);
        break;
    case MB_PARENT_DIST_SEQ_MEASURE_CHANNEL:
        if (WH_GetSystemState() == WH_SYSSTATE_IDLE && WH_StartMeasureChannel() == TRUE) {
            wk->distributionSeq = MB_PARENT_DIST_SEQ_START_MBP;
        }
        break;
    case MB_PARENT_DIST_SEQ_START_MBP:
        if (WH_GetSystemState() == WH_SYSSTATE_MEASURECHANNEL) {
            PlayerInfo *playerInfo = GetGameDataPlayerInfo(wk->param->gameData);

            MBParent_StartMBP(wk);
            MBUtilMsg_ClearWindow(wk->msg);
            if (wk->startMenu == FALSE) {
                MBUtilMsg_SetWindow(wk->msg, 2);
            } else {
                MBUtilMsg_SetWindow(wk->msg, 3);
            }
            MBUtilMsg_CreateWordSet(wk->msg);
            MBUtilMsg_SetNumberZeroPadded(wk->msg, 0, getTrainerID(playerInfo), 5);
            if (wk->startMenu == FALSE) {
                MBUtilMsg_Print(wk->msg, 0, TextSpeed_GetCurrentWait());
            } else {
                MBUtilMsg_Print(wk->msg, 0x14, TextSpeed_GetCurrentWait());
            }
            MBUtilMsg_FreeWordSet(wk->msg);
            wk->distributionSeq = MB_PARENT_DIST_SEQ_ENTRY;
        }
        break;
    case MB_PARENT_DIST_SEQ_ENTRY:
        MBParent_UpdateEntry(wk);
        break;
    case MB_PARENT_DIST_SEQ_COMPLETE:
        MBUtilMsg_ClearWindow(wk->msg);
        MBUtilMsg_SetWindow(wk->msg, 1);
        if (wk->startMenu == FALSE) {
            MBUtilMsg_PrintAtOnce(wk->msg, 1);
        } else {
            MBUtilMsg_PrintAtOnce(wk->msg, 0x15);
        }
        MBUtilMsg_SetShowWaitIcon(wk->msg, 1);
        ClActor_SetSequence(wk->cancelButton, 15);
        wk->distributionSeq = MB_PARENT_DIST_SEQ_DONE;
        break;
    case MB_PARENT_DIST_SEQ_DONE:
        wk->distributionDone = TRUE;
        wk->distributionSeq = MB_PARENT_DIST_SEQ_END_WIRELESS;
        break;
    case MB_PARENT_DIST_SEQ_STOPPED:
        wk->distributionSeq = MB_PARENT_DIST_SEQ_END_WIRELESS;
        break;
    case MB_PARENT_DIST_SEQ_END_WIRELESS:
        sWirelessDone = FALSE;
        WH_End(MBParent_OnWirelessDone);
        wk->wirelessStarted = FALSE;
        wk->distributionSeq = MB_PARENT_DIST_SEQ_WAIT_WIRELESS_END;
        break;
    case MB_PARENT_DIST_SEQ_WAIT_WIRELESS_END:
        if (sWirelessDone == TRUE) {
            GCTX_HIDSetSoftResetCallback(NULL, NULL);
            WH_Release();
            return TRUE;
        }
        break;
    }
    WmIcon_SetStrength(WM_LINK_LEVEL_3 - WM_GetLinkLevel());
    WmIcon_Update();
    return FALSE;
}

static void MBParent_StartMBP(MBParentWork *wk) {
    const char *romPaths[2] = { "/dl_rom/child_r_eng.srl", "/dl_rom/child2_r_eng.srl" };
    // White 2's icon, then Black 2's
    const char *iconCharPaths[2] = { "/dl_rom/icon_w.char", "/dl_rom/icon_b.char" };
    const char *iconPalettePaths[2] = { "/dl_rom/icon_w.plt", "/dl_rom/icon_b.plt" };
    MBGameRegistry registry = {
        NULL, NULL, NULL, NULL, NULL, MB_PARENT_GGID, 2,
    };
    u16 channel = WH_GetMeasureChannel();

    MI_CpuCopy8(&registry, &wk->registry, sizeof(MBGameRegistry));
    if (wk->startMenu == FALSE) {
        wk->registry.romFilePathp = romPaths[0];
    } else {
        wk->registry.romFilePathp = romPaths[1];
    }
    wk->registry.iconCharPathp = iconCharPaths[GAME_VERSION == VERSION_BLACK2];
    wk->registry.iconPalettePathp = iconPalettePaths[GAME_VERSION == VERSION_BLACK2];
    wk->registry.gameNamep = wk->gameName;
    wk->registry.gameIntroductionp = wk->gameIntro;
    MBP_Init(HEAPID_MB_PARENT, MB_PARENT_GGID, MB_TGID_AUTO);
    MBP_Start(&wk->registry, channel);
}

static void MBParent_UpdateEntry(MBParentWork *wk) {
    // The state is read once for nothing, as in the original
    MBP_GetState();
    switch (MBP_GetState()) {
    case MBP_STATE_ENTRY:
        if (wk->cancelSeq == MB_PARENT_CANCEL_SEQ_NONE) {
            int hit = GFL_HIDGetTouchPressedRect(sCancelButtonRect);

            if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_B || (hit == 0 && wk->startMenu == FALSE)) {
                wk->cancelSeq = MB_PARENT_CANCEL_SEQ_PRINT;
                ClActor_SetSequence(wk->cancelButton, 9);
                ClActor_SetAutoAnim(wk->cancelButton, TRUE);
                GFL_SndSEPlay(SEQ_SE_CANCEL1);
            } else if (MBP_IsBootableAll()) {
                MBP_StartRebootAll();
            }
        }
        break;
    case MBP_STATE_COMPLETE:
        wk->distributionSeq = MB_PARENT_DIST_SEQ_COMPLETE;
        break;
    case MBP_STATE_ERROR:
        MBP_Cancel();
        break;
    case MBP_STATE_STOP:
        wk->distributionSeq = MB_PARENT_DIST_SEQ_STOPPED;
        break;
    }
    switch (wk->cancelSeq) {
    case MB_PARENT_CANCEL_SEQ_PRINT:
        MBUtilMsg_ClearWindow(wk->msg);
        ClActor_SetSequence(wk->cancelButton, 15);
        if (wk->startMenu == FALSE) {
            MBUtilMsg_SetWindow(wk->msg, 7);
            MBUtilMsg_Print(wk->msg, 9, TextSpeed_GetCurrentWait());
        } else {
            MBUtilMsg_SetWindow(wk->msg, 5);
            MBUtilMsg_Print(wk->msg, 0x2a, TextSpeed_GetCurrentWait());
        }
        wk->cancelSeq = MB_PARENT_CANCEL_SEQ_ASK;
        break;
    case MB_PARENT_CANCEL_SEQ_ASK:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            if (wk->startMenu == FALSE) {
                MBUtilMsg_CreateYesNoMenu(wk->msg, 0);
            } else {
                MBUtilMsg_CreateConfirm(wk->msg, 0);
            }
            wk->cancelSeq = MB_PARENT_CANCEL_SEQ_ANSWER;
        }
        break;
    case MB_PARENT_CANCEL_SEQ_ANSWER: {
        int answer;

        if (wk->startMenu == FALSE) {
            answer = MBUtilMsg_GetMenuResult(wk->msg);
        } else {
            answer = MBUtilMsg_UpdateConfirm(wk->msg);
        }
        if (answer == MB_UTIL_MSG_YES) {
            GFL_ASSERT_MSG(MBP_GetState() == MBP_STATE_ENTRY, "state is not[MBP_STATE_ENTRY][%d]!!!\n", MBP_GetState());
            MBParent_SetPalParkResult(wk, PAL_PARK_RESULT_CANCELED);
            MBP_Cancel();
            wk->cancelSeq = MB_PARENT_CANCEL_SEQ_CANCELLED;
        } else if (answer == MB_UTIL_MSG_NO) {
            PlayerInfo *playerInfo = GetGameDataPlayerInfo(wk->param->gameData);

            if (wk->startMenu == FALSE) {
                MBUtilMsg_FreeMenu(wk->msg);
            } else {
                MBUtilMsg_ForgetConfirm(wk->msg);
            }
            MBUtilMsg_ClearWindow(wk->msg);
            if (wk->startMenu == FALSE) {
                MBUtilMsg_SetWindow(wk->msg, 2);
            } else {
                MBUtilMsg_SetWindow(wk->msg, 3);
            }
            MBUtilMsg_CreateWordSet(wk->msg);
            MBUtilMsg_SetNumberZeroPadded(wk->msg, 0, getTrainerID(playerInfo), 5);
            ClActor_SetSequence(wk->cancelButton, 1);
            if (wk->startMenu == FALSE) {
                MBUtilMsg_Print(wk->msg, 0, TextSpeed_GetCurrentWait());
            } else {
                MBUtilMsg_Print(wk->msg, 0x14, TextSpeed_GetCurrentWait());
            }
            MBUtilMsg_FreeWordSet(wk->msg);
            wk->cancelSeq = MB_PARENT_CANCEL_SEQ_NONE;
        }
        break;
    }
    }
}

static BOOL MBParent_OnWirelessDone(BOOL success) {
    sWirelessDone = TRUE;
    return TRUE;
}

static void MBParent_SetPalParkResult(MBParentWork *wk, u8 result) {
    if (wk->startMenu == FALSE) {
        u8 current = TrainerGameInfo_GetPalParkResult(wk->trainerGameInfo);
        BOOL set = FALSE;

        switch (result) {
        case PAL_PARK_RESULT_TRANSFERRED:
            if (current != PAL_PARK_RESULT_HIGH_SCORE && current != PAL_PARK_RESULT_ERROR &&
                current != PAL_PARK_RESULT_ERROR_AFTER_TRANSFER) {
                set = TRUE;
            }
            break;
        case PAL_PARK_RESULT_HIGH_SCORE:
            if (current != PAL_PARK_RESULT_ERROR && current != PAL_PARK_RESULT_ERROR_AFTER_TRANSFER) {
                set = TRUE;
            }
            break;
        case PAL_PARK_RESULT_NONE_TRANSFERRED:
            if (current != PAL_PARK_RESULT_TRANSFERRED && current != PAL_PARK_RESULT_HIGH_SCORE &&
                current != PAL_PARK_RESULT_ERROR && current != PAL_PARK_RESULT_ERROR_AFTER_TRANSFER) {
                set = TRUE;
            }
            break;
        case PAL_PARK_RESULT_ERROR:
            set = TRUE;
            if (wk->receivedCount != 0) {
                result = PAL_PARK_RESULT_ERROR_AFTER_TRANSFER;
            }
            break;
        case PAL_PARK_RESULT_CANCELED:
            if (current != PAL_PARK_RESULT_NONE_TRANSFERRED && current != PAL_PARK_RESULT_TRANSFERRED &&
                current != PAL_PARK_RESULT_HIGH_SCORE && current != PAL_PARK_RESULT_ERROR &&
                current != PAL_PARK_RESULT_ERROR_AFTER_TRANSFER) {
                set = TRUE;
            }
            break;
        }
        if (set == TRUE) {
            TrainerGameInfo_SetPalParkResult(wk->trainerGameInfo, result);
        }
    }
}

static void MBParent_SoftResetCallback(void *work) {
    MBParentWork *wk = work;

    if (MBP_IsStarted() == TRUE) {
        MBP_Cancel();
    }
    while (MBP_GetState() != MBP_STATE_STOP && MBP_GetState() != MBP_STATE_COMPLETE) {
        OS_WaitIrq(TRUE, OS_IE_V_BLANK);
    }
    if (wk->wirelessStarted == TRUE) {
        sWirelessDone = FALSE;
        while (WH_End(MBParent_OnWirelessDone)) {
            OS_WaitIrq(TRUE, OS_IE_V_BLANK);
        }
        while (sWirelessDone == FALSE) {
            OS_WaitIrq(TRUE, OS_IE_V_BLANK);
        }
    }
}

static void MBParent_ReceivePokemon(MBParentWork *wk) {
    u8 count = MBComm_GetPokemonCount(wk->comm);

    MBUtilMsg_PrintAtOnce(wk->msg, 6);
    MBUtilMsg_SetShowWaitIcon(wk->msg, 1);
    MBParent_StorePokemon(wk);
    if (wk->startMenu == FALSE) {
        u16 highScore = TrainerGameInfo_GetPalParkHighScore(wk->trainerGameInfo);
        u16 score = MBComm_GetScore(wk->comm);

        if (highScore < score) {
            TrainerGameInfo_SetPalParkHighScore(wk->trainerGameInfo, score);
            MBParent_SetPalParkResult(wk, PAL_PARK_RESULT_HIGH_SCORE);
        } else if (count == 0) {
            MBParent_SetPalParkResult(wk, PAL_PARK_RESULT_NONE_TRANSFERRED);
        } else {
            MBParent_SetPalParkResult(wk, PAL_PARK_RESULT_TRANSFERRED);
        }
        if (wk->highScore < score) {
            wk->highScore = score;
        }
        wk->receivedCount += count;
    }
}

static void MBParent_StorePokemon(MBParentWork *wk) {
    BoxSaveAccessor *boxes = GameData_GetBoxSaveAccessor(wk->param->gameData);
    PokeDexSave *pokedex = GameData_GetPokedex(wk->param->gameData);
    u8 count = MBComm_GetPokemonCount(wk->comm);
    u8 i;

    if (wk->startMenu == FALSE) {
        GameRecords *records = GameData_GetRecords(wk->param->gameData);

        RecordAdd(records, 7, count);
        RecordAdd(records, 0x54, count);
        JoinAvenue_RecordActivity(0x1f);
    }
    for (i = 0; i < count; i++) {
        BoxPkm *pkm = MBComm_GetPokemon(wk->comm, i);
        BOOL result;
        PartyPkm *partyPkm;

        setMetCurrentDateTime(pkm);
        result = BoxSaveAccessor_InsertPkm(boxes, pkm);
        partyPkm = boxPkmRegenToPartyPkm(pkm, wk->heapId);
        addPkmToDex(pokedex, partyPkm);
        GFL_HeapFree(partyPkm);
        GFL_ASSERT_MSG(result == TRUE, "Multiboot parent Box is full!!\n");
    }
}

static void MBParent_Dummy(MBParentWork *wk) {
}

static void MBParent_UpdateTransfer(MBParentWork *wk) {
    switch (wk->distributionSeq) {
    case MB_PARENT_SAVE_SEQ_SEND_SYNC_1:
        if (MBUtilMsg_IsQueueDone(wk->msg) == TRUE && MBUtilMsg_IsPrintDone(wk->msg) == TRUE &&
            MBComm_IsSaveReady(wk->comm) == TRUE &&
            MBComm_SendCommand(wk->comm, MB_COMM_CMD_SAVE_SYNC_1, GFL_RandomLC(20) + 10) == TRUE) {
            wk->distributionSeq = MB_PARENT_SAVE_SEQ_WAIT_SYNC_1;
        }
        break;
    case MB_PARENT_SAVE_SEQ_WAIT_SYNC_1:
        if (MBComm_IsSaveSync1(wk->comm) == TRUE) {
            wk->distributionSeq = MB_PARENT_SAVE_SEQ_START;
        }
        break;
    case MB_PARENT_SAVE_SEQ_START:
        GameData_StartSave(wk->param->gameData);
        wk->distributionSeq = MB_PARENT_SAVE_SEQ_WAIT_LAST;
        break;
    case MB_PARENT_SAVE_SEQ_WAIT_LAST:
        if (GameData_UpdateSave(wk->param->gameData) == SAVE_RESULT_LAST) {
            wk->distributionSeq = MB_PARENT_SAVE_SEQ_WAIT_CHILD_START;
        }
        break;
    case MB_PARENT_SAVE_SEQ_WAIT_CHILD_START:
        if (MBComm_IsSaveStarted(wk->comm) == TRUE) {
            wk->waitFrames = GFL_RandomLC(20) + 10;
            wk->distributionSeq = MB_PARENT_SAVE_SEQ_WAIT_FRAMES;
        }
        break;
    case MB_PARENT_SAVE_SEQ_WAIT_FRAMES:
        wk->waitFrames--;
        if (wk->waitFrames == 0) {
            wk->distributionSeq = MB_PARENT_SAVE_SEQ_FINISH;
        }
        break;
    case MB_PARENT_SAVE_SEQ_FINISH:
        if (GameData_UpdateSave(wk->param->gameData) == SAVE_RESULT_OK) {
            wk->distributionSeq = MB_PARENT_SAVE_SEQ_SEND_SYNC_2;
        }
        break;
    case MB_PARENT_SAVE_SEQ_SEND_SYNC_2:
        if (MBComm_SendCommand(wk->comm, MB_COMM_CMD_SAVE_SYNC_2, 0) == TRUE) {
            wk->distributionSeq = MB_PARENT_SAVE_SEQ_SEND_SYNC_3;
        }
        // fallthrough
    case MB_PARENT_SAVE_SEQ_SEND_SYNC_3:
        if (MBComm_IsSaveMidReached(wk->comm) == TRUE &&
            MBComm_SendCommand(wk->comm, MB_COMM_CMD_SAVE_SYNC_3, 0) == TRUE) {
            wk->distributionSeq = MB_PARENT_SAVE_SEQ_SEND_SYNC_4;
            wk->waitFrames = 0;
        }
        break;
    case MB_PARENT_SAVE_SEQ_SEND_SYNC_4:
        if (MBComm_IsSaved(wk->comm) == TRUE && MBComm_SendCommand(wk->comm, MB_COMM_CMD_SAVE_SYNC_4, 0) == TRUE) {
            wk->distributionSeq = MB_PARENT_SAVE_SEQ_WAIT_SYNC_4;
        }
        break;
    case MB_PARENT_SAVE_SEQ_WAIT_SYNC_4:
        if (MBComm_IsSaveSync4(wk->comm) == TRUE) {
            wk->state = MB_PARENT_STATE_SAVED;
        }
        break;
    }
}

static void MBParent_UpdateResult(MBParentWork *wk) {
    switch (wk->resultSeq) {
    case MB_PARENT_RESULT_SEQ_WAIT_RESULT:
        if (MBComm_GetState(wk->comm) == MB_COMM_STATE_ERROR_1 || MBComm_GetState(wk->comm) == MB_COMM_STATE_ERROR_3) {
            MBUtilMsg_Print(wk->msg, 0x29, TextSpeed_GetCurrentWait());
            MBUtilMsg_ShowWindow(wk->msg, 1);
            MBComm_StartDisconnect(wk->comm);
            wk->state = MB_PARENT_STATE_SEND_END;
        }
        if (MBComm_GetState(wk->comm) == MB_COMM_STATE_ERROR_2) {
            MBUtilMsg_Print(wk->msg, 0x2c, TextSpeed_GetCurrentWait());
            MBUtilMsg_ShowWindow(wk->msg, 1);
            MBComm_StartDisconnect(wk->comm);
            wk->state = MB_PARENT_STATE_SEND_END;
        }
        if (MBComm_HasResult(wk->comm) == TRUE) {
            u16 count = MBComm_GetResultCount(wk->comm);
            u16 more = MBComm_GetResultMoreCount(wk->comm);

            if (count != 0) {
                MBUtilMsg_ClearWindow(wk->msg);
                MBUtilMsg_SetWindow(wk->msg, 5);
                MBUtilMsg_CreateWordSet(wk->msg);
                MBUtilMsg_SetNumber(wk->msg, 0, count + more, 3);
#ifdef BLACK2
                MBUtilMsg_Print(wk->msg, 0x16, TextSpeed_GetCurrentWait());
#else
                MBUtilMsg_Print(wk->msg, 0x17, TextSpeed_GetCurrentWait());
#endif
                MBUtilMsg_ShowWindow(wk->msg, 1);
                MBUtilMsg_FreeWordSet(wk->msg);
                wk->resultSeq = MB_PARENT_RESULT_SEQ_ASK_RECEIVE;
            } else {
                wk->resultSeq = MB_PARENT_RESULT_SEQ_WAIT_ITEM_INFO;
            }
        }
        break;
    case MB_PARENT_RESULT_SEQ_ASK_RECEIVE:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            MBUtilMsg_CreateConfirm(wk->msg, 1);
            wk->resultSeq = MB_PARENT_RESULT_SEQ_RECEIVE_ANSWER;
        }
        break;
    case MB_PARENT_RESULT_SEQ_RECEIVE_ANSWER:
        wk->answer = MBUtilMsg_UpdateConfirm(wk->msg);
        if (wk->answer == MB_UTIL_MSG_YES) {
            u16 count = MBComm_GetResultCount(wk->comm);
            u16 space = BoxSaveAccessor_CountEmptySlots(GameData_GetBoxSaveAccessor(wk->param->gameData));

            MBUtilMsg_ForgetConfirm(wk->msg);
            if (space < count) {
                wk->boxFull = TRUE;
                wk->resultSeq = MB_PARENT_RESULT_SEQ_SEND_ANSWER;
            } else {
                wk->boxFull = FALSE;
                wk->resultSeq = MB_PARENT_RESULT_SEQ_PRINT_MORE;
            }
        } else if (wk->answer == MB_UTIL_MSG_NO) {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_SEND_ANSWER;
        }
        break;
    case MB_PARENT_RESULT_SEQ_PRINT_MORE:
        if (MBComm_GetResultMoreCount(wk->comm) != 0) {
            u16 count = MBComm_GetResultCount(wk->comm);

            MBUtilMsg_ClearWindow(wk->msg);
            MBUtilMsg_SetWindow(wk->msg, 1);
            MBUtilMsg_CreateWordSet(wk->msg);
            MBUtilMsg_SetNumber(wk->msg, 0, count, 3);
            MBUtilMsg_Print(wk->msg, 0x24, TextSpeed_GetCurrentWait());
            MBUtilMsg_ShowWindow(wk->msg, 1);
            MBUtilMsg_FreeWordSet(wk->msg);
            wk->resultSeq = MB_PARENT_RESULT_SEQ_WAIT_MORE_MSG;
        } else {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_PRINT_FLAG1;
        }
        break;
    case MB_PARENT_RESULT_SEQ_WAIT_MORE_MSG:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_PRINT_FLAG1;
        }
        break;
    case MB_PARENT_RESULT_SEQ_PRINT_FLAG1:
        if (MBComm_GetResultFlag1(wk->comm) == TRUE) {
            MBUtilMsg_ClearWindow(wk->msg);
            MBUtilMsg_SetWindow(wk->msg, 5);
            MBUtilMsg_Print(wk->msg, 0x1a, TextSpeed_GetCurrentWait());
            wk->resultSeq = MB_PARENT_RESULT_SEQ_ASK_FLAG1;
        } else {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_SEND_ANSWER;
        }
        break;
    case MB_PARENT_RESULT_SEQ_ASK_FLAG1:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            MBUtilMsg_CreateConfirm(wk->msg, 1);
            wk->resultSeq = MB_PARENT_RESULT_SEQ_FLAG1_ANSWER;
        }
        break;
    case MB_PARENT_RESULT_SEQ_FLAG1_ANSWER: {
        int answer = MBUtilMsg_UpdateConfirm(wk->msg);

        if (answer == MB_UTIL_MSG_YES) {
            if (MBComm_GetResultFlag2(wk->comm) == TRUE) {
                MBUtilMsg_Print(wk->msg, 0x1b, TextSpeed_GetCurrentWait());
                wk->resultSeq = MB_PARENT_RESULT_SEQ_ASK_FLAG2;
            } else {
                wk->resultSeq = MB_PARENT_RESULT_SEQ_SEND_ANSWER;
            }
        } else if (answer == MB_UTIL_MSG_NO) {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_PRINT_CONFIRM;
        }
        break;
    }
    case MB_PARENT_RESULT_SEQ_ASK_FLAG2:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            MBUtilMsg_CreateConfirm(wk->msg, 1);
            wk->resultSeq = MB_PARENT_RESULT_SEQ_FLAG2_ANSWER;
        }
        break;
    case MB_PARENT_RESULT_SEQ_FLAG2_ANSWER: {
        int answer = MBUtilMsg_UpdateConfirm(wk->msg);

        if (answer == MB_UTIL_MSG_YES) {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_SEND_ANSWER;
        } else if (answer == MB_UTIL_MSG_NO) {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_PRINT_CONFIRM;
        }
        break;
    }
    case MB_PARENT_RESULT_SEQ_PRINT_CONFIRM:
        MBUtilMsg_Print(wk->msg, 0x1c, TextSpeed_GetCurrentWait());
        wk->resultSeq = MB_PARENT_RESULT_SEQ_ASK_CONFIRM;
        break;
    case MB_PARENT_RESULT_SEQ_ASK_CONFIRM:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            MBUtilMsg_CreateConfirm(wk->msg, 1);
            wk->resultSeq = MB_PARENT_RESULT_SEQ_CONFIRM_ANSWER;
        }
        break;
    case MB_PARENT_RESULT_SEQ_CONFIRM_ANSWER: {
        int answer = MBUtilMsg_UpdateConfirm(wk->msg);

        if (answer == MB_UTIL_MSG_YES) {
            wk->answer = MB_UTIL_MSG_NO;
            wk->resultSeq = MB_PARENT_RESULT_SEQ_SEND_ANSWER;
        } else if (answer == MB_UTIL_MSG_NO) {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_PRINT_FLAG1;
        }
        break;
    }
    case MB_PARENT_RESULT_SEQ_SEND_ANSWER: {
        u32 reply;

        if (wk->answer == MB_UTIL_MSG_YES) {
            if (wk->boxFull == TRUE) {
                reply = MB_COMM_ANSWER_BOX_FULL;
            } else {
                reply = MB_COMM_ANSWER_ACCEPT;
            }
        } else {
            reply = MB_COMM_ANSWER_DECLINE;
        }
        if (MBComm_SendCommand(wk->comm, MB_COMM_CMD_ANSWER, reply) == TRUE) {
            if (reply == MB_COMM_ANSWER_ACCEPT) {
                wk->resultSeq = MB_PARENT_RESULT_SEQ_RECEIVE_POKEMON;
                MBUtilMsg_ClearWindow(wk->msg);
                MBUtilMsg_SetWindow(wk->msg, 1);
                MBUtilMsg_PrintAtOnce(wk->msg, 0x19);
                MBUtilMsg_SetShowWaitIcon(wk->msg, 1);
            } else if (reply == MB_COMM_ANSWER_BOX_FULL) {
                wk->resultSeq = MB_PARENT_RESULT_SEQ_PRINT_BOX_FULL;
            } else {
                wk->resultSeq = MB_PARENT_RESULT_SEQ_WAIT_ITEM_INFO;
            }
        }
        break;
    }
    case MB_PARENT_RESULT_SEQ_RECEIVE_POKEMON:
        if (MBComm_IsPokemonReceived(wk->comm) == TRUE) {
            MBParent_StorePokemon(wk);
            wk->resultSeq = MB_PARENT_RESULT_SEQ_ACK_POKEMON;
        }
        if (MBComm_HasMore(wk->comm) == TRUE) {
            u16 count = MBComm_GetResultCount(wk->comm);

            MBUtilMsg_ClearWindow(wk->msg);
            MBUtilMsg_SetWindow(wk->msg, 1);
            MBUtilMsg_CreateWordSet(wk->msg);
            MBUtilMsg_SetNumber(wk->msg, 0, count, 3);
            MBUtilMsg_Print(wk->msg, 0x1e, TextSpeed_GetCurrentWait());
            MBUtilMsg_ShowWindow(wk->msg, 1);
            MBUtilMsg_FreeWordSet(wk->msg);
            wk->resultSeq = MB_PARENT_RESULT_SEQ_WAIT_RECEIVED_MSG;
            wk->receivedMore = TRUE;
        }
        break;
    case MB_PARENT_RESULT_SEQ_ACK_POKEMON:
        if (MBComm_SendCommand(wk->comm, MB_COMM_CMD_ACK, 0) == TRUE) {
            MBComm_ClearPokemon(wk->comm);
            wk->resultSeq = MB_PARENT_RESULT_SEQ_RECEIVE_POKEMON;
        }
        break;
    case MB_PARENT_RESULT_SEQ_WAIT_RECEIVED_MSG:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_WAIT_ITEM_INFO;
        }
        break;
    case MB_PARENT_RESULT_SEQ_PRINT_BOX_FULL:
        MBUtilMsg_ClearWindow(wk->msg);
        MBUtilMsg_SetWindow(wk->msg, 5);
        MBUtilMsg_Print(wk->msg, 0x18, TextSpeed_GetCurrentWait());
        wk->resultSeq = MB_PARENT_RESULT_SEQ_WAIT_BOX_FULL_MSG;
        break;
    case MB_PARENT_RESULT_SEQ_WAIT_BOX_FULL_MSG:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_WAIT_ITEM_INFO;
        }
        break;
    case MB_PARENT_RESULT_SEQ_WAIT_ITEM_INFO:
        if (MBComm_HasItemInfo(wk->comm) == TRUE) {
            if (MBComm_HasItem(wk->comm) == TRUE) {
                MBUtilMsg_ClearWindow(wk->msg);
                MBUtilMsg_SetWindow(wk->msg, 5);
                MBUtilMsg_Print(wk->msg, 0x1f, TextSpeed_GetCurrentWait());
                wk->resultSeq = MB_PARENT_RESULT_SEQ_ASK_ITEM;
            } else {
                wk->resultSeq = MB_PARENT_RESULT_SEQ_FINISH;
            }
        }
        break;
    case MB_PARENT_RESULT_SEQ_ASK_ITEM:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_ITEM_ANSWER;
            MBUtilMsg_CreateConfirm(wk->msg, 1);
        }
        break;
    case MB_PARENT_RESULT_SEQ_ITEM_ANSWER: {
        int answer = MBUtilMsg_UpdateConfirm(wk->msg);

        if (answer == MB_UTIL_MSG_YES || answer == MB_UTIL_MSG_NO) {
            // The bag is fetched here as for the gift below, but not used
            BagSave *bag = GameData_GetBag(wk->param->gameData);
            PlayerInfo *playerInfo = GetGameDataPlayerInfo(wk->param->gameData);
            TrainerCardSave *trainerCard = getTrainerCardDataBlkAddress(wk->param->gameData);

            if (answer == MB_UTIL_MSG_YES && isOneShotDRObtained(trainerCard, 0, playerInfo) == TRUE) {
                wk->answer = MB_UTIL_MSG_NO;
                wk->resultSeq = MB_PARENT_RESULT_SEQ_WAIT_ITEM_OWNED_MSG;
                MBUtilMsg_ClearWindow(wk->msg);
                MBUtilMsg_SetWindow(wk->msg, 1);
                MBUtilMsg_Print(wk->msg, 0x25, TextSpeed_GetCurrentWait());
                MBUtilMsg_ShowWindow(wk->msg, 1);
            } else {
                wk->answer = answer;
                wk->resultSeq = MB_PARENT_RESULT_SEQ_SEND_ITEM_ANSWER;
            }
        }
        break;
    }
    case MB_PARENT_RESULT_SEQ_WAIT_ITEM_OWNED_MSG:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_SEND_ITEM_ANSWER;
        }
        break;
    case MB_PARENT_RESULT_SEQ_SEND_ITEM_ANSWER: {
        BOOL accept = TRUE;

        if (wk->answer != MB_UTIL_MSG_YES) {
            accept = FALSE;
        }
        if (MBComm_SendCommand(wk->comm, MB_COMM_CMD_ITEM_ANSWER, accept) == TRUE) {
            if (accept == TRUE) {
                BagSave *bag = GameData_GetBag(wk->param->gameData);
                PlayerInfo *playerInfo = GetGameDataPlayerInfo(wk->param->gameData);
                TrainerCardSave *trainerCard = getTrainerCardDataBlkAddress(wk->param->gameData);

                MBUtilMsg_ClearWindow(wk->msg);
                MBUtilMsg_SetWindow(wk->msg, 1);
                MBUtilMsg_Print(wk->msg, 0x21, TextSpeed_GetCurrentWait());
                MBUtilMsg_ShowWindow(wk->msg, 1);
                setOneShotDRObtained(trainerCard, 0, playerInfo);
                BagSave_AddItem(bag, ITEM_LOCK_CAPSULE, 1, wk->heapId);
                wk->receivedItem = TRUE;
                wk->resultSeq = MB_PARENT_RESULT_SEQ_WAIT_ITEM_MSG;
            } else {
                wk->resultSeq = MB_PARENT_RESULT_SEQ_FINISH;
            }
        }
        break;
    }
    case MB_PARENT_RESULT_SEQ_WAIT_ITEM_MSG:
        if (MBUtilMsg_IsPrintDone(wk->msg) == TRUE) {
            wk->resultSeq = MB_PARENT_RESULT_SEQ_FINISH;
        }
        break;
    case MB_PARENT_RESULT_SEQ_FINISH:
        if (wk->receivedMore == TRUE || wk->receivedItem == TRUE) {
            if (MBComm_SendCommand(wk->comm, MB_COMM_CMD_FINISH, 0) == TRUE) {
                wk->state = MB_PARENT_STATE_WAIT_ACK;
            }
        } else {
            if (MBComm_GetResultCount(wk->comm) == 0 && MBComm_HasItem(wk->comm) == FALSE) {
                MBUtilMsg_ClearWindow(wk->msg);
                MBUtilMsg_SetWindow(wk->msg, 1);
                MBUtilMsg_Print(wk->msg, 0x28, TextSpeed_GetCurrentWait());
                MBUtilMsg_ShowWindow(wk->msg, 1);
            }
            wk->state = MB_PARENT_STATE_SEND_END;
        }
        break;
    }
}

static BOOL MBParent_CardPulledOut(void) {
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    if (sTitleOnSub == TRUE) {
        GFL_BGSysSetBGEnabled(4, TRUE);
    } else {
        GFL_BGSysSetBGEnabled(2, TRUE);
    }
    return TRUE;
}

static BOOL MBParentProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    MBParentParam *mbParam = param;
    MBParentWork *wk;

    if (GFL_NetIsEnded() == FALSE) {
        return FALSE;
    }
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_MB_PARENT, 0x130000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(MBParentWork), HEAPID_MB_PARENT);
    if (mbParam == NULL) {
        mbParam = GFL_HeapAllocate(HEAPID_MB_PARENT, sizeof(MBParentParam), FALSE, "mb_parent_sys.c", 2843);
        if (GCTX_HIDGetHeldKeys() & PAD_BUTTON_R) {
            mbParam->startMenu = TRUE;
        } else {
            mbParam->startMenu = FALSE;
            mbParam->gameData = GameData_Create(HEAPID_MB_PARENT);
        }
    }
    if (mbParam->startMenu == TRUE) {
        mbParam->gameData = GameData_Create(HEAPID_MB_PARENT);
    }
    wk->heapId = HEAPID_MB_PARENT;
    wk->param = mbParam;
    wk->netError = FALSE;
    wk->wirelessError = FALSE;
    wk->eventsPaused = FALSE;
    MBParent_Init(wk);
    return TRUE;
}

static BOOL MBParentProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    MBParentWork *wk = work;
    BOOL netError = wk->netError;
    BOOL startMenu = FALSE;
    BOOL eventsPaused = wk->eventsPaused;
    BOOL wirelessError = wk->wirelessError;

    if (wk->param->startMenu == TRUE) {
        startMenu = TRUE;
    }
    MBParent_Exit(wk);
    if (param == NULL) {
        GameData_Free(wk->param->gameData);
        GFL_HeapFree(wk->param);
    } else if (wk->param->startMenu == TRUE) {
        GameData_Free(wk->param->gameData);
        GFL_HeapFree(wk->param);
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_MB_PARENT);
    if (netError == TRUE) {
        if (startMenu) {
            if (eventsPaused == TRUE || GFL_NetErrCheck() == TRUE) {
                GFL_NetErrShutdown();
                GFL_NetErrShow(TRUE);
            } else {
                GFL_NetErrShow(FALSE);
            }
        } else if (eventsPaused == TRUE) {
            GFL_NetErrShutdown();
            GFL_NetErrShow(TRUE);
        }
    }
    if (wirelessError == TRUE) {
        GFL_NetErrShowScreen();
    }
    if (startMenu == TRUE) {
        GFL_HIDDoSoftReset(0);
    }
    return TRUE;
}

static BOOL MBParentProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    MBParentWork *wk = work;

    if (MBParent_Main(wk) == TRUE) {
        return TRUE;
    }
    if (GFL_NetErrCheck() && wk->netError == FALSE) {
        GFL_NetErrMarkShown();
        wk->netError = TRUE;
    }
    return FALSE;
}
