#include "app/comm_tvt/ctvt_talk.h"
#include "types.h"
#include "app/comm_tvt/comm_tvt_sys.h"
#include "app/comm_tvt/ctvt_camera.h"
#include "app/comm_tvt/ctvt_comm.h"
#include "app/comm_tvt/ctvt_game.h"
#include "app/comm_tvt/ctvt_mic.h"
#include "app/comm_tvt/ima_adpcm.h"
#include "constants/sound.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"

// The Xtransceiver's talk mode: the video chat itself. The member who holds the talk button records their voice,
// which is sent in chunks and played on every machine, with its waveform drawn on the bottom screen. The parent can
// also invite the others to a minigame, or end the call

#define CTVT_TALK_SPEEDS 9
#define CTVT_TALK_SPEED_DEFAULT 4
#define CTVT_TALK_COOLDOWN 8

// The keys that hold the talk button
#define CTVT_TALK_KEYS (PAD_BUTTON_A | PAD_BUTTON_R)

// The sizes of a voice in bytes: it is sent in chunks, and its first chunk, the click of the microphone starting, is
// left out of the wave drawn, which leaves at most VOICE_MAX_SIZE of a recording. A type of its own, so that MWCC
// keeps the skipped size as a variable at the wave's two DrawWave calls
typedef enum {
    VOICE_CHUNK_SIZE = 0x800,
    VOICE_MAX_SIZE = 0xc000,
} CtvtTalkVoiceSize;

// The steps of the talk mode
typedef enum {
    TALK_STATE_FADE_IN,
    TALK_STATE_WAIT_FADE_IN,
    TALK_STATE_FADE_OUT_TO_GAME,
    TALK_STATE_FADE_OUT,
    TALK_STATE_WAIT_FADE_OUT,
    TALK_STATE_MAIN,
    TALK_STATE_REQUEST_TALK,
    TALK_STATE_WAIT_TALKER,
    TALK_STATE_RECORD,
    TALK_STATE_ASK_EXIT,
    TALK_STATE_EXIT_MENU,
    TALK_STATE_INVITE,
    TALK_STATE_SEND_INVITE,
    TALK_STATE_WAIT_ANSWERS,
    TALK_STATE_SEND_START,
    TALK_STATE_START_GAME,
    TALK_STATE_CANCEL_GAME,
    TALK_STATE_INVITED,
    TALK_STATE_INVITED_MENU,
    TALK_STATE_INVITED_WAIT,
    TALK_STATE_JOIN_GAME,
    TALK_STATE_REFUSED,
    TALK_STATE_GAME_CANCELLED,
    TALK_STATE_GAME_CANCELLED_SYNC,
    TALK_STATE_SEND_EXIT,
    TALK_STATE_WAIT_EXIT,
    TALK_STATE_DISCONNECTED,
    TALK_STATE_WAIT_DISCONNECTED,
    TALK_STATE_SEND_PARENT_EXIT,
    TALK_STATE_PARENT_LEFT,
    TALK_STATE_WAIT_PARENT_LEFT,
    TALK_STATE_SYNC_EXIT,
    TALK_STATE_WAIT_SYNC_EXIT,
    TALK_STATE_ALONE,
    TALK_STATE_WAIT_ALONE,
} CtvtTalkState;

// What the talk mode goes on to, in next
enum {
    TALK_NEXT_DRAW = 6,
    TALK_NEXT_GAME,
    TALK_NEXT_EXIT,
};

// The steps of recording, in next while recording
enum {
    RECORD_START,
    RECORD_RECORDING,
    RECORD_SENDING,
    RECORD_SEND_DONE,
    RECORD_WAIT_PLAY,
    RECORD_WAIT_PLAY_END,
};

// What the voice icon shows: the talk button free, the member's own voice recording, or the button not usable (another
// member talking, no microphone, nobody to talk to); VOICE_ICON_MAX is none of them, so that the first is drawn
typedef enum {
    VOICE_ICON_READY,
    VOICE_ICON_TALKING,
    VOICE_ICON_BUSY,
    VOICE_ICON_MAX,
} CtvtTalkVoiceIcon;

// Which of the speed buttons is touched: the ones that speed the voice up, or the ones that slow it down
enum {
    SPEED_TOUCH_UP,
    SPEED_TOUCH_DOWN,
};

// speedTouch when no speed button is held
#define SPEED_TOUCH_NONE 0xff

struct CtvtTalk {
    CtvtTalkState state;
    int next;
    BOOL waitSound;
    u8 requestCooldown;
    u8 speed;
    u8 speedTouch;
    u8 speedRepeat;
    CtvtMic *mic;
    BOOL chunkPending;
    u8 chunk;
    void *voiceAlloc;
    CtvtVoicePacket *voicePacket;
    u8 *voiceData;
    BOOL messagePending;
    BmpWin *waveWindow;
    u8 waveX;
    u8 waveY;
    BOOL flashing;
    u8 flashFrame;
    u16 flashPalette[16];
    u16 normalPalette[16];
    BmpWin *messageWindow;
    AppTaskMenu *exitMenu;
    BOOL notifyExit;
    u16 waitFrames;
    BOOL waveShown;
    ClActor *speedCursor;
    ClActor *zoomButton;
    ClActor *talkButton;
    ClActor *exitButton;
    ClActor *voiceIcon;
    ClActor *gameButtons[2];
    CtvtTalkVoiceIcon voiceIconState;
    CtvtTalkVoiceIcon shownVoiceIconState;
    BmpWin *gameWindow;
    BmpWin *gameWindowSub;
    BOOL gameWindowPending;
    BOOL gameWindowSubPending;
    StrBuf *message;
    StrBuf *expanded;
    u16 messageFrames;
    AppTaskMenu *gameMenu;
    u16 countdown;
    u8 countdownFrames;
    u32 lastVBlank;
    // What the other members sent about a minigame
    BOOL gameInvited;
    BOOL gameCancelRequested;
    BOOL gameCancelled;
    BOOL gameStarting;
    BOOL gameStart;
    u8 joinedMask;
    u8 readyMask;
    BOOL notAlone;
    BOOL notAloneChanged;
    s8 newMembers;
};

static void CtvtTalk_UpdateMain(CommTvtWork *sys, CtvtTalk *talk);
static void CtvtTalk_UpdateRecord(CommTvtWork *sys, CtvtTalk *talk);
static void CtvtTalk_UpdateVoiceIcon(CommTvtWork *sys, CtvtTalk *talk);
static void CtvtTalk_UpdateSpeed(CommTvtWork *sys, CtvtTalk *talk);
static void CtvtTalk_DrawWaveLine(CommTvtWork *sys, CtvtTalk *talk, u8 *pixels, u8 x0, u8 y0, u8 x1, u8 y1, u8 color);
static void CtvtTalk_PutWavePixel(u8 *pixels, u8 x, u8 y, u8 color);
static void CtvtTalk_PrintMessage(CommTvtWork *sys, CtvtTalk *talk, u32 msgId);
static void CtvtTalk_PrintCountdown(CommTvtWork *sys, CtvtTalk *talk, u32 msgId, int count);
static void CtvtTalk_PrintGameMessage(CommTvtWork *sys, CtvtTalk *talk, u32 msgId, StrBuf *name, u16 frames);
static void CtvtTalk_AskExit(CommTvtWork *sys, CtvtTalk *talk);
static void CtvtTalk_UpdateExitMenu(CommTvtWork *sys, CtvtTalk *talk);
static BOOL CtvtTalk_CheckDisconnect(CommTvtWork *sys, CtvtTalk *talk);
static void CtvtTalk_ResetWave(CommTvtWork *sys, CtvtTalk *talk);
static void CtvtTalk_DrawWave(CommTvtWork *sys, CtvtTalk *talk, const s16 *samples, u8 color, s32 pos, u16 size);
static BOOL CtvtTalk_IsTalkButtonTouched(CommTvtWork *sys, CtvtTalk *talk, BOOL held);
static void CtvtTalk_OpenGameWindow(CommTvtWork *sys, CtvtTalk *talk);
static void CtvtTalk_CloseGameWindow(CommTvtWork *sys, CtvtTalk *talk);
static BOOL CtvtTalk_CheckGameCancel(CommTvtWork *sys, CtvtTalk *talk);

// The buttons of the bottom screen, declared in the order CtvtTalk_UpdateMain and CtvtTalk_UpdateSpeed test them,
// which is the order MWCC's size sort needs to lay them out as the ROM does
static const TouchRect sExitButtons[] = {
    { 0x08, 0x18, 0xb8, 0xe8 },
    { 0x18, 0x28, 0xb8, 0xf0 },
    { TOUCH_RECT_END },
};

static const TouchRect sGameButtons1[] = {
    { 0x28, 0x40, 0xc0, 0xff },
    { 0x40, 0x48, 0xc8, 0xff },
    { TOUCH_RECT_END },
};

static const TouchRect sGameButtons2[] = {
    { 0x48, 0x58, 0xc8, 0xff },
    { 0x58, 0x68, 0xd0, 0xff },
    { TOUCH_RECT_END },
};

static const TouchRect sZoomButtons[] = {
    { 0xa8, 0xc0, 0xe8, 0x00 },
    { 0x68, 0xa0, 0xd8, 0xff },
    { TOUCH_RECT_END },
};

static const TouchRect sSpeedButtons[] = {
    { 0x08, 0x28, 0x10, 0x48 },
    { 0x20, 0x4d, 0x00, 0x40 },
    { 0x4d, 0xa0, 0x00, 0x30 },
    { TOUCH_RECT_END },
};

// The masks that keep the other pixels of a byte of the waveform's 4bpp bitmap, and the pixel of each position
static const u16 sPixelMasks[] = { 0xfff0, 0xff0f, 0xf0ff, 0x0fff };

// How far the waveform may swing at the window's ends, so that it fits the rounded corners
static const u8 sWaveEdges[] = { 3, 7, 11, 14, 17, 21, 24, 32, 32 };

CtvtTalk *CtvtTalk_Create(CommTvtWork *sys, HeapID heapId) {
    u8 i;
    CtvtTalk *talk = GFL_HeapAllocate(heapId, sizeof(CtvtTalk), TRUE, "ctvt_talk.c", 294);

    talk->mic = CtvtMic_Create(heapId);
    talk->voiceAlloc = GFL_HeapAllocate(heapId, 0x808, TRUE, "ctvt_talk.c", 297);
    talk->voicePacket = talk->voiceAlloc;
    talk->voiceData = (u8 *)talk->voiceAlloc + 8;
    talk->speed = CTVT_TALK_SPEED_DEFAULT;
    MIi_CpuCopy16((void *)(HW_DB_BG_PLTT + 0x20), talk->normalPalette, sizeof(talk->normalPalette));
    for (i = 0; i < 16; i++) {
        talk->flashPalette[i] = 0x7fff;
    }
    return talk;
}

void CtvtTalk_Delete(CommTvtWork *sys, CtvtTalk *talk) {
    GFL_HeapFree(talk->voicePacket);
    CtvtMic_Delete(talk->mic);
    GFL_HeapFree(talk);
}

// The resources of the actors are the system's: characters, palette and cells
static inline void CtvtTalk_CreateActor(CommTvtWork *sys, ClActor **actor, int chars, int palette, int cells,
                                        const ClActorSetup *setup, HeapID heapId) {
    *actor = ClActor_Create(CommTvt_GetClActUnit(sys), CommTvt_GetObjResource(sys, chars),
                            CommTvt_GetObjResource(sys, palette), CommTvt_GetObjResource(sys, cells), setup, 1, heapId);
}

void CtvtTalk_Enter(CommTvtWork *sys, CtvtTalk *talk) {
    HeapID heapId = CommTvt_GetHeapId(sys);
    ArcTool *arc = CommTvt_GetArc(sys);
    ClActorSetup setup;
    u8 talker;

    GFL_BGSysLoadArcNCGRStatic(arc, 12, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 17, 6, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 12, 5, 0, 0x5000, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 15, 5, 0, 0, FALSE, heapId);
    G2x_SetBlendAlpha_((u32)&reg_G2S_DB_BLDCNT, 2, 0x14, 5, 15);
    GFL_BGSysLoadScr(6);
    GFL_BGSysLoadScr(5);

    setup.x = 128;
    setup.y = 96;
    setup.sequence = talk->speed + 25;
    setup.priority = 0;
    setup.bgPriority = 2;
    CtvtTalk_CreateActor(sys, &talk->speedCursor, 5, 1, 9, &setup, heapId);
    ClActor_SetVisible(talk->speedCursor, TRUE);
    if (CommTvt_GetViewSwapped(sys) == TRUE) {
        setup.sequence = 21;
    } else {
        setup.sequence = 22;
    }
    CtvtTalk_CreateActor(sys, &talk->zoomButton, 5, 1, 9, &setup, heapId);
    ClActor_SetVisible(talk->zoomButton, TRUE);
    setup.sequence = 20;
    CtvtTalk_CreateActor(sys, &talk->exitButton, 5, 1, 9, &setup, heapId);
    ClActor_SetVisible(talk->exitButton, TRUE);
    setup.sequence = 36;
    CtvtTalk_CreateActor(sys, &talk->gameButtons[0], 5, 1, 9, &setup, heapId);
    ClActor_SetVisible(talk->gameButtons[0], TRUE);
    setup.sequence = 37;
    CtvtTalk_CreateActor(sys, &talk->gameButtons[1], 5, 1, 9, &setup, heapId);
    ClActor_SetVisible(talk->gameButtons[1], TRUE);
    setup.sequence = 24;
    CtvtTalk_CreateActor(sys, &talk->voiceIcon, 5, 1, 9, &setup, heapId);
    ClActor_SetVisible(talk->voiceIcon, TRUE);
    setup.x = 232;
    setup.y = 168;
    setup.sequence = 1;
    setup.priority = 0;
    setup.bgPriority = 0;
    CtvtTalk_CreateActor(sys, &talk->talkButton, 6, 2, 10, &setup, heapId);
    ClActor_SetVisible(talk->talkButton, TRUE);
    ClActor_SetAutoAnim(talk->talkButton, TRUE);

    talk->state = TALK_STATE_FADE_IN;
    talk->waitSound = FALSE;
    talk->voiceIconState = VOICE_ICON_READY;
    talk->shownVoiceIconState = VOICE_ICON_MAX;
    talk->waveWindow = BmpWin_CreateDynamic(4, 8, 3, 16, 8, 10, TRUE);
    talk->messageWindow = BmpWin_CreateDynamic(4, 1, 1, 30, 4, 10, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(talk->waveWindow), 0);
    BmpWin_FlushMap(talk->waveWindow);
    BmpWin_FlushChar(talk->waveWindow);
    talk->exitMenu = NULL;
    talk->notifyExit = FALSE;
    talk->messagePending = FALSE;
    GFL_BGSysLoadScr(4);
    talk->flashFrame = 0;
    talk->waveShown = FALSE;
    talk->flashing = FALSE;
    talk->requestCooldown = 0;
    talk->gameWindow = BmpWin_CreateDynamic(0, 1, 15, 30, 6, 10, TRUE);
    talk->gameWindowSub = BmpWin_CreateDynamic(4, 1, 8, 30, 6, 10, TRUE);
    talk->message = GFL_StrBufCreate(109, heapId);
    talk->expanded = GFL_StrBufCreate(109, heapId);
    talker = CtvtComm_GetTalker(sys, CommTvt_GetComm(sys));
    if (talker != CTVT_COMM_NONE) {
        CommTvt_PlaceSlotMarker(sys, talker);
        talk->voiceIconState = VOICE_ICON_BUSY;
    }
    if (!CtvtMic_IsReady(talk->mic)) {
        talk->voiceIconState = VOICE_ICON_BUSY;
    }
    if (CommTvt_GetSelfIndex(sys) != 0 || CommTvt_IsLaunchModeCall(sys) == TRUE || talk->notAlone == TRUE) {
        ClActor_SetAnimFrame(talk->gameButtons[0], 1);
        ClActor_SetAnimFrame(talk->gameButtons[1], 1);
    }
}

static inline void CtvtTalk_ClearWindow(BmpWin *window) {
    BmpWin_ClearScreen(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
}

void CtvtTalk_Leave(CommTvtWork *sys, CtvtTalk *talk) {
    PrintQueue_Clear(CommTvt_GetPrintQueue(sys));
    if (talk->exitMenu != NULL) {
        AppTaskMenu_Free(talk->exitMenu);
    }
    if (talk->gameMenu != NULL) {
        AppTaskMenu_Free(talk->gameMenu);
        talk->gameMenu = NULL;
    }
    CtvtTalk_ClearWindow(talk->waveWindow);
    CtvtTalk_ClearWindow(talk->messageWindow);
    BmpWin_Free(talk->waveWindow);
    BmpWin_Free(talk->messageWindow);
    GFL_StrBufFree(talk->expanded);
    GFL_StrBufFree(talk->message);
    CtvtTalk_ClearWindow(talk->gameWindow);
    CtvtTalk_ClearWindow(talk->gameWindowSub);
    BmpWin_Free(talk->gameWindow);
    BmpWin_Free(talk->gameWindowSub);
    ClActor_Delete(talk->talkButton);
    ClActor_Delete(talk->voiceIcon);
    ClActor_Delete(talk->exitButton);
    ClActor_Delete(talk->gameButtons[0]);
    ClActor_Delete(talk->gameButtons[1]);
    ClActor_Delete(talk->zoomButton);
    ClActor_Delete(talk->speedCursor);
    GFL_BGSysClearScr(6);
    GFL_BGSysLoadScr(6);
    reg_G2S_DB_BLDCNT = 0;
}

int CtvtTalk_Main(CommTvtWork *sys, CtvtTalk *talk) {
    // The fades' steps and frames per step, read as immediates. A function-local static const is emitted even when
    // its reads fold: these two are the bytes 06 01 at the head of the file's .rodata, which nothing points to. That
    // they are these calls' is inferred from their arguments
    static const u8 wipeDivision = 6;
    static const u8 wipeSync = 1;
    HeapID heapId = CommTvt_GetHeapId(sys);

    switch (talk->state) {
    case TALK_STATE_FADE_IN:
        if (func_ov257_021aaa74(sys) == TRUE) {
            GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, wipeDivision, wipeSync, heapId);
        } else {
            GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, 0, wipeDivision, wipeSync, heapId);
        }
        talk->state = TALK_STATE_WAIT_FADE_IN;
        break;
    case TALK_STATE_WAIT_FADE_IN:
        if (GFL_WipeIsFinished() == TRUE) {
            talk->state = TALK_STATE_MAIN;
        }
        break;
    case TALK_STATE_FADE_OUT_TO_GAME:
        GFL_WipeSet(WIPE_MODE_SUB, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, wipeDivision, wipeSync, heapId);
        func_ov257_021aaa78(sys, FALSE);
        talk->state = TALK_STATE_WAIT_FADE_OUT;
        break;
    case TALK_STATE_FADE_OUT: {
        CtvtCamera *camera;

        if (talk->next == TALK_NEXT_DRAW && ClActor_IsAnimActive(talk->exitButton) == TRUE) {
            break;
        }
        camera = CommTvt_GetCamera(sys);
        if (talk->waitSound && CtvtCamera_IsSoundDone(sys, camera) != TRUE) {
            break;
        }
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, 0, wipeDivision, wipeSync, heapId);
        func_ov257_021aaa78(sys, TRUE);
        talk->state = TALK_STATE_WAIT_FADE_OUT;
        break;
    }
    case TALK_STATE_WAIT_FADE_OUT:
        if (GFL_WipeIsFinished() == TRUE) {
            if (talk->next == TALK_NEXT_DRAW) {
                return COMM_TVT_MODE_DRAW;
            }
            if (talk->next == TALK_NEXT_GAME) {
                return COMM_TVT_MODE_GAME;
            }
            return COMM_TVT_MODE_EXIT;
        }
        break;
    case TALK_STATE_MAIN:
        CtvtTalk_UpdateMain(sys, talk);
        break;
    case TALK_STATE_REQUEST_TALK:
        if (CtvtTalk_CheckDisconnect(sys, talk)) {
            break;
        }
        if (CtvtTalk_IsTalkButtonTouched(sys, talk, TRUE) == TRUE || (GCTX_HIDGetHeldKeys() & CTVT_TALK_KEYS)) {
            if (CtvtComm_SendPacket(sys, CommTvt_GetComm(sys), CTVT_PACKET_REQUEST_TALK, 0) == TRUE) {
                talk->state = TALK_STATE_WAIT_TALKER;
            }
        } else {
            talk->state = TALK_STATE_MAIN;
        }
        break;
    case TALK_STATE_WAIT_TALKER: {
        CtvtComm *comm;
        u8 talker, self;

        if (CtvtTalk_CheckDisconnect(sys, talk)) {
            break;
        }
        comm = CommTvt_GetComm(sys);
        talker = CtvtComm_GetTalker(sys, comm);
        self = CtvtComm_GetSelfNetId(sys, comm);
        if (CtvtTalk_IsTalkButtonTouched(sys, talk, TRUE) == TRUE || (GCTX_HIDGetHeldKeys() & CTVT_TALK_KEYS)) {
            if (talker == CTVT_COMM_NONE) {
                break;
            }
            if (talker == self) {
                talk->state = TALK_STATE_RECORD;
                talk->next = RECORD_START;
            } else {
                talk->state = TALK_STATE_MAIN;
            }
        } else {
            talk->state = TALK_STATE_MAIN;
        }
        break;
    }
    case TALK_STATE_RECORD:
        CtvtTalk_UpdateRecord(sys, talk);
        break;
    case TALK_STATE_ASK_EXIT:
        CtvtTalk_AskExit(sys, talk);
        break;
    case TALK_STATE_EXIT_MENU:
        CtvtTalk_UpdateExitMenu(sys, talk);
        break;
    case TALK_STATE_INVITE:
        CtvtComm_ScanInvited(sys, CommTvt_GetComm(sys));
        talk->state = TALK_STATE_SEND_INVITE;
        break;
    case TALK_STATE_SEND_INVITE:
        if (!CtvtTalk_CheckGameCancel(sys, talk) &&
            CtvtComm_SendPacket(sys, CommTvt_GetComm(sys), CTVT_PACKET_UNK_B, CtvtGame_GetType(CommTvt_GetGame(sys))) ==
                TRUE) {
            talk->state = TALK_STATE_WAIT_ANSWERS;
            CtvtTalk_PrintGameMessage(sys, talk, 45, NULL, 0);
            talk->gameMenu = func_ov257_021aac98(sys, 24, 15);
        }
        break;
    case TALK_STATE_WAIT_ANSWERS: {
        CtvtComm *comm;

        if (CtvtTalk_CheckGameCancel(sys, talk)) {
            break;
        }
        comm = CommTvt_GetComm(sys);
        CtvtComm_SendPacketAll(sys, comm, CTVT_PACKET_UNK_B, CtvtGame_GetType(CommTvt_GetGame(sys)));
        AppTaskMenu_Update(talk->gameMenu);
        if (AppTaskMenu_IsFlashFinished(talk->gameMenu) == TRUE) {
            if (AppTaskMenu_GetCursorPos(talk->gameMenu) == 0) {
                talk->state = TALK_STATE_CANCEL_GAME;
            }
            AppTaskMenu_Free(talk->gameMenu);
            talk->gameMenu = NULL;
        } else if (AppTaskMenu_IsDecided(talk->gameMenu) == TRUE) {
            break;
        }
        if (CtvtTalk_AreAllJoined(sys, talk)) {
            u8 sent[4];
            u8 order[4];
            u8 i;

            for (i = 0; i < 4; i++) {
                order[i] = i;
            }
            for (i = 0; i < 10; i++) {
                u8 a = GFL_RandomMTRange(4);
                u8 b = GFL_RandomMTRange(4);
                u8 swap = order[a];

                order[a] = order[b];
                order[b] = swap;
            }
            for (i = 0; i < 4; i++) {
                sent[i] = order[i];
            }
            CtvtComm_ScanNone(sys, comm);
            if (CtvtComm_SendPacketData(sys, comm, CTVT_PACKET_UNK_E, sent) == TRUE) {
                talk->state = TALK_STATE_SEND_START;
            }
        }
        if (talk->countdown != 0) {
            u32 now = OS_GetVBlankCount();

            talk->countdownFrames += (u8)(now - talk->lastVBlank);
            talk->lastVBlank = now;
            if (talk->countdownFrames > 60) {
                talk->countdownFrames = 0;
                if (talk->gameWindowPending == FALSE && talk->gameWindowSubPending == FALSE) {
                    talk->countdown--;
                    CtvtTalk_PrintCountdown(sys, talk, 43, talk->countdown);
                }
                if (talk->countdown == 0) {
                    talk->state = TALK_STATE_CANCEL_GAME;
                }
            }
        }
        break;
    }
    case TALK_STATE_SEND_START: {
        CtvtComm *comm;

        if (CtvtTalk_CheckGameCancel(sys, talk)) {
            break;
        }
        comm = CommTvt_GetComm(sys);
        if (CtvtTalk_AreAllReady(sys, talk) && CtvtComm_SendPacket(sys, comm, CTVT_PACKET_UNK_11, 0) == TRUE) {
            talk->state = TALK_STATE_START_GAME;
        }
        break;
    }
    case TALK_STATE_START_GAME: {
        CtvtCommBeacon *beacon;
        u8 i, j;

        if (CtvtTalk_CheckGameCancel(sys, talk)) {
            break;
        }
        beacon = CtvtComm_GetBeacon(sys, CommTvt_GetComm(sys));
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 6; j++) {
                beacon->inviteMacs[i][j] = 0xff;
            }
        }
        beacon->inviteOnly = FALSE;
        talk->state = TALK_STATE_FADE_OUT;
        talk->next = TALK_NEXT_GAME;
        break;
    }
    case TALK_STATE_CANCEL_GAME:
        if (CtvtComm_SendPacket(sys, CommTvt_GetComm(sys), CTVT_PACKET_UNK_F, 0) == TRUE) {
            talk->state = TALK_STATE_GAME_CANCELLED;
            CtvtTalk_PrintGameMessage(sys, talk, 46, NULL, 120);
        }
        BmpWin_ClearFrame(talk->gameWindow, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(talk->gameWindow), 15);
        BmpWin_ClearScreen(talk->gameWindow);
        GFL_BGSysLoadScr(0);
        talk->gameWindowPending = FALSE;
        if (talk->gameMenu != NULL) {
            AppTaskMenu_Free(talk->gameMenu);
            talk->gameMenu = NULL;
        }
        break;
    case TALK_STATE_INVITED: {
        StrBuf *name;

        if (CtvtTalk_CheckGameCancel(sys, talk)) {
            break;
        }
        name = copyTrainerNameToNewStrbuf((const u16 *)CtvtComm_GetMemberInfo(sys, CommTvt_GetComm(sys), 0)->playerInfo,
                                          heapId);
        CtvtTalk_PrintGameMessage(sys, talk, 47, name, 0);
        GFL_StrBufFree(name);
        talk->exitMenu = func_ov257_021aac08(sys, 24, 15);
        talk->state = TALK_STATE_INVITED_MENU;
        break;
    }
    case TALK_STATE_INVITED_MENU:
        if (CtvtTalk_CheckGameCancel(sys, talk)) {
            break;
        }
        AppTaskMenu_Update(talk->exitMenu);
        if (AppTaskMenu_IsFlashFinished(talk->exitMenu) != TRUE) {
            break;
        }
        if (AppTaskMenu_GetCursorPos(talk->exitMenu) == 0) {
            talk->state = TALK_STATE_INVITED_WAIT;
            CtvtTalk_PrintGameMessage(sys, talk, 45, NULL, 0);
            talk->gameMenu = func_ov257_021aac98(sys, 24, 15);
        } else {
            talk->state = TALK_STATE_REFUSED;
            CtvtTalk_PrintGameMessage(sys, talk, 48, NULL, 120);
        }
        AppTaskMenu_Free(talk->exitMenu);
        talk->exitMenu = NULL;
        break;
    case TALK_STATE_INVITED_WAIT: {
        CtvtComm *comm;

        if (CtvtTalk_CheckGameCancel(sys, talk)) {
            break;
        }
        comm = CommTvt_GetComm(sys);
        AppTaskMenu_Update(talk->gameMenu);
        if (AppTaskMenu_IsFlashFinished(talk->gameMenu) == TRUE) {
            if (AppTaskMenu_GetCursorPos(talk->gameMenu) == 0) {
                talk->state = TALK_STATE_REFUSED;
                CtvtTalk_PrintGameMessage(sys, talk, 46, NULL, 120);
            }
            AppTaskMenu_Free(talk->gameMenu);
            talk->gameMenu = NULL;
        } else if (AppTaskMenu_IsDecided(talk->gameMenu) == FALSE) {
            CtvtComm_SendPacketAll(sys, comm, CTVT_PACKET_UNK_C, 0);
            if (talk->gameStarting == TRUE) {
                talk->gameStarting = FALSE;
                talk->state = TALK_STATE_JOIN_GAME;
            }
        }
        break;
    }
    case TALK_STATE_JOIN_GAME:
        if (CtvtTalk_CheckGameCancel(sys, talk)) {
            break;
        }
        CtvtComm_SendPacketAll(sys, CommTvt_GetComm(sys), CTVT_PACKET_UNK_10, 0);
        if (talk->gameStart == TRUE) {
            talk->gameStart = FALSE;
            talk->state = TALK_STATE_FADE_OUT;
            talk->next = TALK_NEXT_GAME;
        }
        break;
    case TALK_STATE_REFUSED:
        if (talk->messageFrames != 0) {
            talk->messageFrames--;
        }
        if (talk->messageFrames == 0 && !CtvtTalk_CheckGameCancel(sys, talk)) {
            CtvtComm_SendPacketAll(sys, CommTvt_GetComm(sys), CTVT_PACKET_UNK_D, 0);
        }
        break;
    case TALK_STATE_GAME_CANCELLED:
        if (talk->messageFrames != 0) {
            talk->messageFrames--;
        }
        if (talk->messageFrames == 0) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 10, 32);
            talk->state = TALK_STATE_GAME_CANCELLED_SYNC;
        }
        break;
    case TALK_STATE_GAME_CANCELLED_SYNC:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 10, 32) == TRUE) {
            CtvtTalk_CloseGameWindow(sys, talk);
        }
        break;
    case TALK_STATE_SEND_EXIT:
        if (CtvtComm_SendPacket(sys, CommTvt_GetComm(sys), CTVT_PACKET_UNK_A, 0) == TRUE) {
            talk->state = TALK_STATE_WAIT_EXIT;
        }
        // fallthrough
    case TALK_STATE_WAIT_EXIT:
        if (CommTvt_GetMemberCount(sys) <= 1) {
            CtvtCamera *camera = CommTvt_GetCamera(sys);

            talk->next = TALK_NEXT_EXIT;
            talk->state = TALK_STATE_FADE_OUT;
            CommTvt_SetMessageActive(sys, TRUE);
            CtvtCamera_StopCamera(sys, camera);
            CtvtCamera_EndRecording(sys, camera);
            talk->waitSound = TRUE;
        }
        break;
    case TALK_STATE_DISCONNECTED:
        CtvtComm_Disconnect(sys, CommTvt_GetComm(sys));
        CommTvt_SetMessageActive(sys, TRUE);
        CtvtTalk_PrintMessage(sys, talk, 36);
        talk->state = TALK_STATE_WAIT_DISCONNECTED;
        CtvtCamera_EndRecording(sys, CommTvt_GetCamera(sys));
        talk->waitSound = TRUE;
        break;
    case TALK_STATE_WAIT_DISCONNECTED:
        if (GFL_HIDIsTouchPressed() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            talk->next = TALK_NEXT_EXIT;
            talk->state = TALK_STATE_FADE_OUT;
            CtvtCamera_StopCamera(sys, CommTvt_GetCamera(sys));
            talk->waitSound = TRUE;
        }
        break;
    case TALK_STATE_SEND_PARENT_EXIT:
        if (CtvtComm_SendPacket(sys, CommTvt_GetComm(sys), CTVT_PACKET_UNK_A, 0) == TRUE) {
            CtvtCamera *camera = CommTvt_GetCamera(sys);

            CommTvt_SetMessageActive(sys, TRUE);
            CtvtCamera_EndRecording(sys, camera);
            talk->state = TALK_STATE_SYNC_EXIT;
        }
        break;
    case TALK_STATE_PARENT_LEFT:
        CtvtComm_Disconnect(sys, CommTvt_GetComm(sys));
        CtvtTalk_PrintMessage(sys, talk, 37);
        talk->state = TALK_STATE_WAIT_PARENT_LEFT;
        talk->waitFrames = 0;
        CommTvt_SetMessageActive(sys, TRUE);
        CtvtCamera_EndRecording(sys, CommTvt_GetCamera(sys));
        talk->waitSound = TRUE;
        break;
    case TALK_STATE_WAIT_PARENT_LEFT:
        talk->waitFrames++;
        if (GFL_HIDIsTouchPressed() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) ||
            talk->waitFrames >= 300) {
            talk->state = TALK_STATE_SYNC_EXIT;
        }
        break;
    case TALK_STATE_SYNC_EXIT:
        CtvtComm_StartSync(sys, CommTvt_GetComm(sys), 47);
        talk->state = TALK_STATE_WAIT_SYNC_EXIT;
        break;
    case TALK_STATE_WAIT_SYNC_EXIT:
        if (CtvtComm_IsSynced(sys, CommTvt_GetComm(sys), 47) == TRUE) {
            talk->next = TALK_NEXT_EXIT;
            talk->state = TALK_STATE_FADE_OUT;
            CommTvt_DeleteTimeIcon(sys);
            CommTvt_SetMessageActive(sys, TRUE);
            CtvtCamera_StopCamera(sys, CommTvt_GetCamera(sys));
            talk->waitSound = TRUE;
        }
        break;
    case TALK_STATE_ALONE:
        CtvtComm_Disconnect(sys, CommTvt_GetComm(sys));
        CtvtTalk_PrintMessage(sys, talk, 38);
        talk->state = TALK_STATE_WAIT_ALONE;
        CommTvt_SetMessageActive(sys, TRUE);
        CtvtCamera_EndRecording(sys, CommTvt_GetCamera(sys));
        talk->waitSound = TRUE;
        break;
    case TALK_STATE_WAIT_ALONE:
        if (GFL_HIDIsTouchPressed() == TRUE || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B))) {
            talk->next = TALK_NEXT_EXIT;
            talk->state = TALK_STATE_FADE_OUT;
            CommTvt_SetMessageActive(sys, TRUE);
            CtvtCamera_StopCamera(sys, CommTvt_GetCamera(sys));
            talk->waitSound = TRUE;
        }
        break;
    }

    CtvtTalk_UpdateVoiceIcon(sys, talk);
    CtvtMic_Update(talk->mic);
    if (talk->messagePending == TRUE) {
        PrintQueue *queue = CommTvt_GetPrintQueue(sys);

        if (!PrintQueue_IsBitmapPending(queue, BmpWin_GetBitmap(talk->messageWindow))) {
            BmpWin_FlushChar(talk->messageWindow);
            BmpWin_FlushMap(talk->messageWindow);
            GFL_BGSysLoadScr(4);
            talk->messagePending = FALSE;
            if (talk->notifyExit == TRUE) {
                talk->notifyExit = FALSE;
                CommTvt_CreateTimeIcon(sys, talk->messageWindow);
            }
        }
    }
    if (talk->gameWindowPending == TRUE) {
        PrintQueue *queue = CommTvt_GetPrintQueue(sys);

        if (!PrintQueue_IsBitmapPending(queue, BmpWin_GetBitmap(talk->gameWindow))) {
            BmpWin_DrawFrame(talk->gameWindow, 1, 0x200, 9);
            BmpWin_FlushChar(talk->gameWindow);
            BmpWin_FlushMap(talk->gameWindow);
            GFL_BGSysLoadScr(0);
            talk->gameWindowPending = FALSE;
        }
    }
    if (talk->gameWindowSubPending == TRUE) {
        PrintQueue *queue = CommTvt_GetPrintQueue(sys);

        if (!PrintQueue_IsBitmapPending(queue, BmpWin_GetBitmap(talk->gameWindowSub))) {
            BmpWin_FlushChar(talk->gameWindowSub);
            BmpWin_FlushMap(talk->gameWindowSub);
            GFL_BGSysLoadScr(4);
            talk->gameWindowSubPending = FALSE;
        }
    }
    if (talk->flashing == TRUE) {
        talk->flashFrame++;
        if (talk->flashFrame > 6) {
            talk->flashing = FALSE;
            NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, 32, talk->normalPalette, 32);
        } else if (talk->flashFrame % 12 < 6) {
            NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, 32, talk->flashPalette, 32);
        } else {
            NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, 32, talk->normalPalette, 32);
        }
    }
    if (talk->waveShown == FALSE && talk->state != TALK_STATE_RECORD && talk->state < TALK_STATE_SEND_EXIT &&
        CtvtMic_IsPlaying(talk->mic) == TRUE) {
        CtvtTalk_ResetWave(sys, talk);
        talk->waveShown = TRUE;
    }
    if (talk->waveShown == TRUE) {
        if (CtvtMic_IsPlaying(talk->mic) == FALSE) {
            talk->waveShown = FALSE;
            GFL_BitmapFill(BmpWin_GetBitmap(talk->waveWindow), 0);
            BmpWin_FlushChar(talk->waveWindow);
        } else {
            CtvtVoicePacket *packet = CtvtComm_GetVoicePacket(sys, CommTvt_GetComm(sys));
            int frames = CtvtMic_GetPlayFrames(talk->mic);
            u16 length = CtvtMic_GetPlayLength(talk->mic);
            u32 size = CtvtMic_GetPlaySize(talk->mic);
            CtvtTalkVoiceSize skipped = VOICE_CHUNK_SIZE;

            if (frames > length) {
                frames = length;
            }
            CtvtTalk_DrawWave(sys, talk, (const s16 *)((u8 *)packet + skipped), 3, size * frames / length, size);
        }
    }
    if (ClActor_IsAnimActive(talk->talkButton) == FALSE) {
        if (talk->state == TALK_STATE_EXIT_MENU) {
            ClActor_SetSequence(talk->talkButton, 15);
        } else {
            ClActor_SetSequence(talk->talkButton, 1);
        }
    }
    return COMM_TVT_MODE_TALK;
}

void CtvtTalk_Draw(CommTvtWork *sys, CtvtTalk *talk) {
    CtvtMic_Draw(talk->mic);
}

static void CtvtTalk_UpdateMain(CommTvtWork *sys, CtvtTalk *talk) {
    CtvtComm *comm = CommTvt_GetComm(sys);
    u8 memberCount = CommTvt_GetMemberCount(sys);
    s32 hit;

    if (CtvtTalk_CheckDisconnect(sys, talk) == TRUE) {
        return;
    }
    if (talk->state <= TALK_STATE_RECORD && CommTvt_GetViewSwapped(sys) == FALSE) {
        CtvtCamera *camera = CommTvt_GetCamera(sys);

        if (GCTX_HIDGetPressedKeys() & PAD_KEY_UP) {
            CtvtCamera_SetEffect(sys, camera, CAMERA_EFFECT_NONE);
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
            CtvtCamera_SetEffect(sys, camera, CAMERA_EFFECT_SEPIA01);
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_DOWN) {
            CtvtCamera_SetEffect(sys, camera, CAMERA_EFFECT_MONO);
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
            CtvtCamera_SetEffect(sys, camera, CAMERA_EFFECT_NEGATIVE);
        }
    }

    if (memberCount == 1 || !CtvtMic_IsReady(talk->mic)) {
        talk->voiceIconState = VOICE_ICON_BUSY;
    } else {
        u8 self = CtvtComm_GetSelfNetId(sys, comm);
        u8 talker = CtvtComm_GetTalker(sys, comm);

        if (talker != CTVT_COMM_NONE) {
            if (talker == self) {
                if (talk->requestCooldown != 0) {
                    talk->requestCooldown--;
                } else if (CtvtComm_SendPacket(sys, comm, CTVT_PACKET_TALK_DONE, 0) == TRUE) {
                    talk->requestCooldown = CTVT_TALK_COOLDOWN;
                }
            }
            talk->voiceIconState = VOICE_ICON_BUSY;
        } else {
            BOOL touched = CtvtTalk_IsTalkButtonTouched(sys, talk, FALSE);

            if ((GCTX_HIDGetPressedKeys() & CTVT_TALK_KEYS) || touched == TRUE) {
                talk->state = TALK_STATE_REQUEST_TALK;
                GFL_SndSEPlay(SEQ_SE_SYS_45);
                talk->flashing = TRUE;
                talk->flashFrame = 0;
            }
            talk->voiceIconState = VOICE_ICON_READY;
        }
    }

    if (talk->state == TALK_STATE_MAIN) {
        if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) ||
            GFL_HIDGetTouchPressedRect(sExitButtons) != TOUCH_RECT_NONE) {
            talk->state = TALK_STATE_FADE_OUT;
            talk->next = TALK_NEXT_DRAW;
            ClActor_SetSequence(talk->exitButton, 20);
            ClActor_SetAutoAnim(talk->exitButton, TRUE);
            GFL_SndSEPlay(SEQ_SE_SYS_45);
        }
    }
    if (talk->state == TALK_STATE_MAIN) {
        if (CommTvt_GetSelfIndex(sys) == 0 && CommTvt_IsLaunchModeCall(sys) == FALSE &&
            GFL_HIDGetTouchPressedRect(sGameButtons1) != TOUCH_RECT_NONE && talk->notAlone == FALSE) {
            CtvtGame_SetType(CommTvt_GetGame(sys), 0);
            CtvtTalk_OpenGameWindow(sys, talk);
            talk->state = TALK_STATE_INVITE;
            ClActor_SetSequence(talk->gameButtons[0], 36);
            ClActor_SetAutoAnim(talk->gameButtons[0], TRUE);
            GFL_SndSEPlay(SEQ_SE_SYS_45);
        }
        if (CommTvt_GetSelfIndex(sys) == 0 && CommTvt_IsLaunchModeCall(sys) == FALSE &&
            GFL_HIDGetTouchPressedRect(sGameButtons2) != TOUCH_RECT_NONE && talk->notAlone == FALSE) {
            CtvtGame_SetType(CommTvt_GetGame(sys), 1);
            CtvtTalk_OpenGameWindow(sys, talk);
            talk->state = TALK_STATE_INVITE;
            ClActor_SetSequence(talk->gameButtons[1], 37);
            ClActor_SetAutoAnim(talk->gameButtons[1], TRUE);
            GFL_SndSEPlay(SEQ_SE_SYS_45);
        }
        if (talk->gameInvited == TRUE) {
            CtvtTalk_OpenGameWindow(sys, talk);
            talk->state = TALK_STATE_INVITED;
        }
        if (talk->notAloneChanged == TRUE) {
            u16 frame = ClActor_GetAnimFrame(talk->gameButtons[0]);

            if ((frame == 0 || frame == 2) && talk->notAlone == TRUE) {
                ClActor_SetAnimFrame(talk->gameButtons[0], 1);
                ClActor_SetAnimFrame(talk->gameButtons[1], 1);
                ClActor_StopAnim(talk->gameButtons[0]);
                ClActor_StopAnim(talk->gameButtons[1]);
            } else if (frame == 1 && talk->notAlone == FALSE) {
                ClActor_SetAnimFrame(talk->gameButtons[0], 0);
                ClActor_SetAnimFrame(talk->gameButtons[1], 0);
                ClActor_StopAnim(talk->gameButtons[0]);
                ClActor_StopAnim(talk->gameButtons[1]);
            }
            talk->notAloneChanged = FALSE;
        }
    }
    if (talk->state == TALK_STATE_MAIN) {
        hit = GFL_HIDGetTouchPressedRect(sZoomButtons);
        if (hit == 0 || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
            if (hit == 0) {
                GCTX_HIDSetUsingTouch(TRUE);
            } else {
                GCTX_HIDSetUsingTouch(FALSE);
            }
            talk->state = TALK_STATE_ASK_EXIT;
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ClActor_SetSequence(talk->talkButton, 9);
        } else if (hit == 1 || (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_SELECT | PAD_BUTTON_START))) {
            CommTvt_ToggleViewSwapped(sys);
            GFL_SndSEPlay(SEQ_SE_SYS_48);
            if (CommTvt_GetViewSwapped(sys) == TRUE) {
                ClActor_SetSequence(talk->zoomButton, 21);
            } else {
                ClActor_SetSequence(talk->zoomButton, 22);
            }
        }
    }
    if (CommTvt_GetSelfIndex(sys) == 0 && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_L) &&
        CtvtCamera_IsRedrawing(sys, CommTvt_GetCamera(sys)) == FALSE) {
        CommTvt_ToggleZoom(sys);
        GFL_SndSEPlay(SEQ_SE_SYS_45);
    }
    CtvtTalk_UpdateSpeed(sys, talk);
}

static void CtvtTalk_UpdateRecord(CommTvtWork *sys, CtvtTalk *talk) {
    BOOL touched = CtvtTalk_IsTalkButtonTouched(sys, talk, TRUE);
    CtvtComm *comm;

    if (CtvtTalk_CheckDisconnect(sys, talk) == TRUE) {
        return;
    }
    switch (talk->next) {
    case RECORD_START:
        if (CtvtMic_StartRecording(talk->mic) == TRUE) {
            talk->voiceIconState = VOICE_ICON_TALKING;
            talk->next = RECORD_RECORDING;
            talk->chunk = 0;
            talk->chunkPending = FALSE;
            Adpcm_ResetEncoder();
            CtvtTalk_ResetWave(sys, talk);
        }
        break;
    case RECORD_RECORDING:
        if (touched != TRUE && !(GCTX_HIDGetHeldKeys() & CTVT_TALK_KEYS)) {
            CtvtMic_StopRecording(talk->mic);
        }
        if (CtvtMic_IsRecording(talk->mic) == FALSE) {
            talk->next = RECORD_SENDING;
            talk->voiceIconState = VOICE_ICON_BUSY;
        }
        {
            u8 *buffer = CtvtMic_GetBuffer(talk->mic);
            u32 size = CtvtMic_GetRecordedSize(talk->mic);
            CtvtTalkVoiceSize skipped = VOICE_CHUNK_SIZE;

            CtvtTalk_DrawWave(sys, talk, (const s16 *)(buffer + skipped), 9, size, VOICE_MAX_SIZE);
        }
        // fallthrough
    case RECORD_SENDING: {
        BOOL busy;
        u32 size;

        comm = CommTvt_GetComm(sys);
        if (CtvtComm_GetUnk3f8(sys, comm) != TRUE) {
            break;
        }
        busy = CtvtComm_IsVoiceBusy(sys, comm);
        size = CtvtMic_GetRecordedSize(talk->mic);
        if (talk->chunkPending == FALSE && busy == FALSE &&
            (size >= (talk->chunk + 1) * VOICE_CHUNK_SIZE || talk->next == RECORD_SENDING)) {
            u8 *buffer = CtvtMic_GetBuffer(talk->mic);

            u32 encoded = CtvtMic_Encode(talk->mic, (const s16 *)(buffer + talk->chunk * VOICE_CHUNK_SIZE),
                                         talk->voiceData, VOICE_CHUNK_SIZE);

            talk->voicePacket->chunk = talk->chunk;
            talk->voicePacket->size = encoded;
            talk->voicePacket->playSize = size;
            talk->voicePacket->speed = talk->speed;
            if (talk->next == RECORD_SENDING && size <= (talk->chunk + 1) * VOICE_CHUNK_SIZE) {
                talk->voicePacket->isLast = TRUE;
            } else {
                talk->voicePacket->isLast = FALSE;
            }
            talk->chunkPending = TRUE;
        }
        if (talk->chunkPending == TRUE && CtvtComm_SendVoice(sys, comm, talk->voicePacket) == TRUE) {
            talk->chunk++;
            talk->chunkPending = FALSE;
            if (talk->voicePacket->isLast == TRUE) {
                talk->next = RECORD_SEND_DONE;
            }
        }
        break;
    }
    case RECORD_SEND_DONE:
        comm = CommTvt_GetComm(sys);
        if (CtvtComm_IsVoiceBusy(sys, comm) == FALSE &&
            CtvtComm_SendPacket(sys, comm, CTVT_PACKET_PLAY_VOICE, 0) == TRUE) {
            talk->next = RECORD_WAIT_PLAY;
            GFL_BitmapFill(BmpWin_GetBitmap(talk->waveWindow), 0);
            BmpWin_FlushChar(talk->waveWindow);
        }
        break;
    case RECORD_WAIT_PLAY:
        if (talk->voicePacket->playSize > 32) {
            if (CtvtMic_IsPlaying(CommTvt_GetMic(sys)) == TRUE) {
                talk->next = RECORD_WAIT_PLAY_END;
            }
        } else {
            talk->state = TALK_STATE_MAIN;
        }
        break;
    case RECORD_WAIT_PLAY_END:
        comm = CommTvt_GetComm(sys);
        if (CtvtMic_IsPlaying(CommTvt_GetMic(sys)) == FALSE &&
            CtvtComm_SendPacket(sys, comm, CTVT_PACKET_CANCEL_TALK, 0) == TRUE) {
            talk->state = TALK_STATE_MAIN;
        }
        break;
    }
}

static void CtvtTalk_UpdateVoiceIcon(CommTvtWork *sys, CtvtTalk *talk) {
    if (talk->voiceIconState != talk->shownVoiceIconState) {
        if (talk->voiceIconState == VOICE_ICON_TALKING) {
            ClActor_SetSequence(talk->voiceIcon, 23);
        } else if (talk->voiceIconState == VOICE_ICON_BUSY) {
            ClActor_SetSequence(talk->voiceIcon, 35);
        } else {
            ClActor_SetSequence(talk->voiceIcon, 24);
        }
    }
}

static void CtvtTalk_UpdateSpeed(CommTvtWork *sys, CtvtTalk *talk) {
    u32 trig = GFL_HIDGetTouchPressedRect(sSpeedButtons);
    u32 cont = GFL_HIDGetTouchHeldRect(sSpeedButtons);
    u32 dir = TOUCH_RECT_NONE;
    BOOL changed = FALSE;

    if (trig != TOUCH_RECT_NONE) {
        talk->speedRepeat = 0;
        if (trig <= 1) {
            talk->speedTouch = SPEED_TOUCH_UP;
            dir = SPEED_TOUCH_UP;
        } else {
            talk->speedTouch = SPEED_TOUCH_DOWN;
            dir = SPEED_TOUCH_DOWN;
        }
    } else if (cont != TOUCH_RECT_NONE) {
        BOOL same = FALSE;

        if (cont <= 1) {
            if (talk->speedTouch == SPEED_TOUCH_UP) {
                same = TRUE;
            }
        } else if (talk->speedTouch == SPEED_TOUCH_DOWN) {
            same = TRUE;
        }
        if (same == TRUE) {
            talk->speedRepeat++;
            if (talk->speedRepeat >= 25) {
                dir = talk->speedTouch;
                talk->speedRepeat = 10;
            }
        } else {
            talk->speedTouch = SPEED_TOUCH_NONE;
        }
    } else {
        talk->speedTouch = SPEED_TOUCH_NONE;
    }

    if (dir == SPEED_TOUCH_UP) {
        if (talk->speed < CTVT_TALK_SPEEDS - 1) {
            changed = TRUE;
            talk->speed++;
            ClActor_SetSequence(talk->speedCursor, talk->speed + 25);
        }
    } else if (dir == SPEED_TOUCH_DOWN && talk->speed != 0) {
        changed = TRUE;
        talk->speed--;
        ClActor_SetSequence(talk->speedCursor, talk->speed + 25);
    }
    if (changed == TRUE) {
        int pitch = (talk->speed - 4) * 64;

        GFL_SEPlayKeepVol(SEQ_SE_SYS_45, 1);
        GFL_SndPlayerSetParams(1, -1, pitch, -1);
    }
}

static void CtvtTalk_DrawWaveLine(CommTvtWork *sys, CtvtTalk *talk, u8 *pixels, u8 x0, u8 y0, u8 x1, u8 y1, u8 color) {
    int dx = MATH_ABS(x0 - x1);
    int dy = MATH_ABS(y0 - y1);
    int steps;
    fx32 stepX, stepY;
    fx32 offsetX, offsetY;
    s32 i;

    steps = (dx > dy ? dx : dy) + 1;
    stepX = FX32_CONST(x1 - x0) / steps;
    stepY = FX32_CONST(y1 - y0) / steps;
    offsetX = 0;
    offsetY = 0;
    for (i = 0; i < steps; i++) {
        CtvtTalk_PutWavePixel(pixels, x0 + (offsetX >> FX32_SHIFT), y0 + (offsetY >> FX32_SHIFT), color);
        offsetX += stepX;
        offsetY += stepY;
    }
}

// The waveform's window is 16 x 8 tiles of 4bpp, two pixels to a byte
static void CtvtTalk_PutWavePixel(u8 *pixels, u8 x, u8 y, u8 color) {
    u8 px = x % 8;
    u8 pos = px % 4;
    u16 *tile = (u16 *)(pixels + (u16)(x / 8 + (y / 8) * 16) * 32);
    u8 py = y % 8;
    u16 *pixel = tile + (px / 4 + py * 2);
    u16 colorMasks[4] = { 0x0001, 0x0010, 0x0100, 0x1000 };

    if (pixel < (u16 *)(pixels + 0x1000) && pixel >= (u16 *)pixels) {
        *pixel = (*pixel & sPixelMasks[pos]) + colorMasks[pos] * color;
    }
}

static void CtvtTalk_PrintMessage(CommTvtWork *sys, CtvtTalk *talk, u32 msgId) {
    Font *font = CommTvt_GetFont(sys);
    MsgData *msgData = CommTvt_GetMsgData(sys);
    PrintQueue *queue = CommTvt_GetPrintQueue(sys);
    StrBuf *str;

    PrintQueue_Clear(queue);
    GFL_BitmapFill(BmpWin_GetBitmap(talk->messageWindow), 15);
    str = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    PrintQueue_PrintColor(queue, BmpWin_GetBitmap(talk->messageWindow), 0, 0, str, font, 0x440);
    GFL_StrBufFree(str);
    BmpWin_FlushChar(talk->messageWindow);
    BmpWin_FlushMap(talk->messageWindow);
    BmpWin_DrawFrame(talk->messageWindow, 1, 0x140, 9);
    talk->messagePending = TRUE;
}

static void CtvtTalk_PrintCountdown(CommTvtWork *sys, CtvtTalk *talk, u32 msgId, int count) {
    HeapID heapId = CommTvt_GetHeapId(sys);
    CtvtGame *game = CommTvt_GetGame(sys);
    Font *font = CommTvt_GetFont(sys);
    MsgData *msgData = CommTvt_GetMsgData(sys);
    PrintQueue *queue = CommTvt_GetPrintQueue(sys);
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(heapId);
    StrBuf *gameName;
    u32 nameId;

    GFL_BitmapFill(BmpWin_GetBitmap(talk->gameWindow), 15);
    GFL_MsgDataLoadStrbuf(msgData, msgId, talk->message);
    nameId = CtvtGame_GetType(game) + 62;
    gameName = GFL_MsgDataLoadStrbufNew(msgData, nameId);
    GFL_WordSetSetStrbuf(wordSet, 0, gameName, 0, 1, 2);
    WordSetNumber(wordSet, 1, count, 2, 2, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, talk->expanded, talk->message);
    GFL_StrBufFree(gameName);
    PrintQueue_PrintColor(queue, BmpWin_GetBitmap(talk->gameWindow), 0, 0, talk->expanded, font, 0x440);
    GFL_WordSetSystemFree(wordSet);
    talk->gameWindowPending = TRUE;
}

static void CtvtTalk_PrintGameMessage(CommTvtWork *sys, CtvtTalk *talk, u32 msgId, StrBuf *name, u16 frames) {
    HeapID heapId = CommTvt_GetHeapId(sys);
    CtvtGame *game = CommTvt_GetGame(sys);
    Font *font = CommTvt_GetFont(sys);
    MsgData *msgData = CommTvt_GetMsgData(sys);
    PrintQueue *queue = CommTvt_GetPrintQueue(sys);
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(heapId);
    u32 index = 0;
    StrBuf *message;
    StrBuf *gameName;

    PrintQueue_Clear(queue);
    GFL_BitmapFill(BmpWin_GetBitmap(talk->gameWindowSub), 15);
    message = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    if (name != NULL) {
        GFL_WordSetSetStrbuf(wordSet, index, name, index, 1, 2);
        index++;
    }
    gameName = GFL_MsgDataLoadStrbufNew(msgData, CtvtGame_GetType(game) + 62);
    GFL_WordSetSetStrbuf(wordSet, index, gameName, 0, 1, 2);
    GFL_StrBufFree(gameName);
    GFL_WordSetFormatStrbuf(wordSet, talk->expanded, message);
    GFL_StrBufFree(message);
    PrintQueue_PrintColor(queue, BmpWin_GetBitmap(talk->gameWindowSub), 0, 0, talk->expanded, font, 0x440);
    GFL_WordSetSystemFree(wordSet);
    BmpWin_FlushChar(talk->gameWindowSub);
    BmpWin_FlushMap(talk->gameWindowSub);
    BmpWin_DrawFrame(talk->gameWindowSub, 1, 0x140, 9);
    talk->messageFrames = frames;
    talk->gameWindowSubPending = TRUE;
}

static void CtvtTalk_AskExit(CommTvtWork *sys, CtvtTalk *talk) {
    if (CtvtComm_GetConnectType(sys, CommTvt_GetComm(sys)) == CTVT_CONNECT_EXISTING) {
        CtvtTalk_PrintMessage(sys, talk, 32);
    } else if (CommTvt_GetSelfIndex(sys) == 0 && CommTvt_GetMemberCount(sys) > 1) {
        CtvtTalk_PrintMessage(sys, talk, 33);
    } else {
        CtvtTalk_PrintMessage(sys, talk, 32);
    }
    talk->exitMenu = CommTvt_OpenConfirmMenu(sys);
    talk->state = TALK_STATE_EXIT_MENU;
    if (ClActor_IsAnimActive(talk->talkButton) == FALSE) {
        ClActor_SetSequence(talk->talkButton, 15);
    }
}

static void CtvtTalk_UpdateExitMenu(CommTvtWork *sys, CtvtTalk *talk) {
    if (CtvtTalk_CheckDisconnect(sys, talk) == TRUE) {
        AppTaskMenu_Free(talk->exitMenu);
        talk->exitMenu = NULL;
        return;
    }
    AppTaskMenu_Update(talk->exitMenu);
    if (AppTaskMenu_IsFlashFinished(talk->exitMenu) != TRUE) {
        return;
    }
    if (AppTaskMenu_GetCursorPos(talk->exitMenu) == 0) {
        if (CtvtComm_GetConnectType(sys, CommTvt_GetComm(sys)) == CTVT_CONNECT_EXISTING) {
            talk->state = TALK_STATE_SEND_PARENT_EXIT;
            talk->notifyExit = TRUE;
            CtvtTalk_PrintMessage(sys, talk, 42);
        } else if (CommTvt_GetSelfIndex(sys) == 0 && CommTvt_GetMemberCount(sys) > 1) {
            talk->state = TALK_STATE_SEND_EXIT;
        } else {
            CtvtCamera *camera;

            talk->next = TALK_NEXT_EXIT;
            talk->state = TALK_STATE_FADE_OUT;
            CommTvt_SetMessageActive(sys, TRUE);
            camera = CommTvt_GetCamera(sys);
            CtvtCamera_StopCamera(sys, camera);
            CtvtCamera_EndRecording(sys, camera);
            talk->waitSound = TRUE;
        }
    } else {
        talk->state = TALK_STATE_MAIN;
        BmpWin_ClearFrame(talk->messageWindow, 1);
        BmpWin_ClearScreen(talk->messageWindow);
        BmpWin_FlushMap(talk->waveWindow);
        GFL_BGSysLoadScr(4);
        ClActor_SetSequence(talk->talkButton, 1);
    }
    AppTaskMenu_Free(talk->exitMenu);
    talk->exitMenu = NULL;
}

static BOOL CtvtTalk_CheckDisconnect(CommTvtWork *sys, CtvtTalk *talk) {
    if (CommTvt_GetEndRequested(sys) == TRUE) {
        if (CtvtComm_GetConnectType(sys, CommTvt_GetComm(sys)) == CTVT_CONNECT_EXISTING) {
            talk->state = TALK_STATE_PARENT_LEFT;
        } else {
            talk->state = TALK_STATE_DISCONNECTED;
        }
        return TRUE;
    }
    if (CommTvt_GetMemberCount(sys) <= 1) {
        talk->state = TALK_STATE_ALONE;
        return TRUE;
    }
    if (talk->gameCancelled == TRUE && CommTvt_GetSelfIndex(sys) != 0) {
        switch (talk->state) {
        case TALK_STATE_REQUEST_TALK:
        case TALK_STATE_WAIT_TALKER:
        case TALK_STATE_RECORD:
            GFL_BitmapFill(BmpWin_GetBitmap(talk->waveWindow), 0);
            BmpWin_FlushChar(talk->waveWindow);
            break;
        default:
            CtvtTalk_PrintGameMessage(sys, talk, 46, NULL, 120);
            break;
        }
        talk->gameCancelled = FALSE;
        talk->state = TALK_STATE_GAME_CANCELLED;
        return TRUE;
    }
    return FALSE;
}

static void CtvtTalk_ResetWave(CommTvtWork *sys, CtvtTalk *talk) {
    talk->waveX = 3;
    talk->waveY = 32;
    GFL_BitmapFill(BmpWin_GetBitmap(talk->waveWindow), 0);
}

// `pos` is a long, the SDK's s32, which leaves `size` in its stack slot and `color` in a saved register
static void CtvtTalk_DrawWave(CommTvtWork *sys, CtvtTalk *talk, const s16 *samples, u8 color, s32 pos, u16 size) {
    u32 end;
    u8 *pixels;
    BOOL drawn;
    u32 y;

    if (pos < 0) {
        return;
    }
    end = 123 * pos / size;
    pixels = GFL_BitmapGetPixelData(BmpWin_GetBitmap(talk->waveWindow));
    drawn = FALSE;
    while (talk->waveX < end) {
        y = (samples[((talk->waveX + 1) * size / 128) / 2] + 0x8000) * 64 / 0x10000;
        if (talk->waveX < 11 && y < 32 - sWaveEdges[talk->waveX - 3]) {
            y = 32 - sWaveEdges[talk->waveX - 3];
        }
        if (talk->waveX > 114 && y < 32 - sWaveEdges[(u8)(122 - talk->waveX)]) {
            y = 32 - sWaveEdges[(u8)(122 - talk->waveX)];
        }
        CtvtTalk_DrawWaveLine(sys, talk, pixels, talk->waveX, talk->waveY, talk->waveX + 1, y, color);
        talk->waveX++;
        talk->waveY = y;
        drawn = TRUE;
        // Only one segment a call, though the loop reads as if it caught up to end
        break;
    }
    if (drawn == TRUE) {
        BmpWin_FlushChar(talk->waveWindow);
    }
}

static BOOL CtvtTalk_IsTalkButtonTouched(CommTvtWork *sys, CtvtTalk *talk, BOOL held) {
    u32 x, y;

    if (held == FALSE) {
        GFL_HIDGetTouchPressedPoint(&x, &y);
    } else {
        GFL_HIDGetTouchHeldPoint(&x, &y);
    }
    if (y >= 104) {
        int dx = 128 - x;
        int dy = 104 - y;
        int dist = dx * dx + dy * dy;

        if (dist > 0x100 && dist < 0x1440) {
            return TRUE;
        }
    }
    return FALSE;
}

static void CtvtTalk_OpenGameWindow(CommTvtWork *sys, CtvtTalk *talk) {
    talk->gameWindowPending = FALSE;
    talk->gameWindowSubPending = FALSE;
    talk->messageFrames = 0;
    talk->gameMenu = NULL;
    talk->countdown = 31;
    talk->countdownFrames = 0;
    talk->lastVBlank = 0;
    talk->gameInvited = FALSE;
    talk->gameCancelRequested = FALSE;
    talk->gameCancelled = FALSE;
    talk->gameStarting = FALSE;
    talk->gameStart = FALSE;
    talk->joinedMask = 0;
    talk->readyMask = 0;
    talk->newMembers = 0;
    GFL_BGSysSetBGEnabled(5, FALSE);
    G2x_SetBlendBrightness_((u32)&reg_G2S_DB_BLDCNT, 0x1c, -8);
    G2S_SetWnd0Position(240, 0, 0, 16);
    G2S_SetWnd0InsidePlane(0x10, FALSE);
    G2S_SetWndOutsidePlane(0x1d, TRUE);
    GXS_SetVisibleWnd(1);
}

static void CtvtTalk_CloseGameWindow(CommTvtWork *sys, CtvtTalk *talk) {
    talk->state = TALK_STATE_MAIN;
    BmpWin_ClearFrame(talk->gameWindow, 1);
    BmpWin_ClearFrame(talk->gameWindowSub, 1);
    GFL_BitmapFill(BmpWin_GetBitmap(talk->gameWindow), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(talk->gameWindowSub), 0);
    BmpWin_ClearScreen(talk->gameWindow);
    BmpWin_ClearScreen(talk->gameWindowSub);
    GFL_BGSysLoadScr(0);
    GFL_BGSysLoadScr(4);
    GFL_BitmapFill(BmpWin_GetBitmap(talk->waveWindow), 0);
    BmpWin_FlushMap(talk->waveWindow);
    if (talk->exitMenu != NULL) {
        AppTaskMenu_Free(talk->exitMenu);
        talk->exitMenu = NULL;
    }
    if (talk->gameMenu != NULL) {
        AppTaskMenu_Free(talk->gameMenu);
        talk->gameMenu = NULL;
    }
    GFL_BGSysSetBGEnabled(5, TRUE);
    G2x_SetBlendAlpha_((u32)&reg_G2S_DB_BLDCNT, 2, 0x14, 5, 15);
    GXS_SetVisibleWnd(0);
    CtvtComm_ScanAll(sys, CommTvt_GetComm(sys));
}

static BOOL CtvtTalk_CheckGameCancel(CommTvtWork *sys, CtvtTalk *talk) {
    BOOL cancelled = FALSE;

    if (talk->gameCancelRequested == TRUE && CommTvt_GetSelfIndex(sys) == 0) {
        talk->gameInvited = FALSE;
        talk->gameCancelRequested = FALSE;
        talk->state = TALK_STATE_CANCEL_GAME;
        cancelled = TRUE;
    }
    if (talk->gameCancelled == TRUE && CommTvt_GetSelfIndex(sys) != 0) {
        talk->gameCancelled = FALSE;
        talk->state = TALK_STATE_GAME_CANCELLED;
        CtvtTalk_PrintGameMessage(sys, talk, 46, NULL, 120);
        cancelled = TRUE;
    }
    if (cancelled == TRUE) {
        if (talk->exitMenu != NULL) {
            AppTaskMenu_Free(talk->exitMenu);
            talk->exitMenu = NULL;
        }
        if (talk->gameMenu != NULL) {
            AppTaskMenu_Free(talk->gameMenu);
            talk->gameMenu = NULL;
        }
    }
    if (CtvtTalk_CheckDisconnect(sys, talk) == TRUE) {
        CtvtTalk_CloseGameWindow(sys, talk);
        return TRUE;
    }
    return cancelled;
}

CtvtMic *CtvtTalk_GetMic(CommTvtWork *sys, CtvtTalk *talk) {
    return talk->mic;
}

void CtvtTalk_SetGameInvited(CtvtTalk *talk, BOOL invited) {
    if (talk->state == TALK_STATE_MAIN) {
        talk->gameInvited = invited;
    }
}

void CtvtTalk_SetGameCancelRequested(CtvtTalk *talk, BOOL value) {
    talk->gameCancelRequested = value;
}

void CtvtTalk_SetGameCancelled(CtvtTalk *talk, BOOL value) {
    talk->gameCancelled = value;
}

BOOL CtvtTalk_IsGameCancelled(CtvtTalk *talk) {
    return talk->gameCancelled;
}

void CtvtTalk_SetGameStarting(CtvtTalk *talk, BOOL value) {
    talk->gameStarting = value;
}

void CtvtTalk_SetGameStart(CtvtTalk *talk, BOOL value) {
    talk->gameStart = value;
}

void CtvtTalk_SetJoined(CtvtTalk *talk, u8 member) {
    talk->joinedMask |= 1 << member;
}

BOOL CtvtTalk_AreAllJoined(CommTvtWork *sys, CtvtTalk *talk) {
    u8 total = CommTvt_GetMemberCount(sys);
    u8 mask;
    u8 i;
    u8 count = 0;

    total += talk->newMembers;
    mask = talk->joinedMask;
    for (i = 1; i < 4; i++) {
        if ((1 << i) & mask) {
            count++;
        }
    }
    if (total == (u8)(count + 1)) {
        return TRUE;
    }
    return FALSE;
}

void CtvtTalk_SetReady(CtvtTalk *talk, u8 member) {
    talk->readyMask |= 1 << member;
}

BOOL CtvtTalk_AreAllReady(CommTvtWork *sys, CtvtTalk *talk) {
    u8 total = CommTvt_GetMemberCount(sys);
    u8 mask = talk->readyMask;
    u8 i;
    u8 count = 0;

    for (i = 1; i < 4; i++) {
        if ((1 << i) & mask) {
            count++;
        }
    }
    if (total == (u8)(count + 1)) {
        return TRUE;
    }
    return FALSE;
}

void CtvtTalk_SetNotAlone(CtvtTalk *talk, BOOL value) {
    talk->notAlone = value;
}

BOOL CtvtTalk_IsNotAlone(CtvtTalk *talk) {
    return talk->notAlone;
}

void CtvtTalk_SetNotAloneChanged(CtvtTalk *talk, BOOL value) {
    talk->notAloneChanged = value;
}

void CtvtTalk_AddNewMember(CtvtTalk *talk) {
    talk->newMembers++;
}

void CtvtTalk_ClearNewMembers(CtvtTalk *talk) {
    talk->newMembers = 0;
}
