#include "types.h"
#include "app/btl_rec_sel.h"
#include "app/btl_rec_sel_graphic.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "save/bag.h"
#include "save/battle_rec.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/bmp_menu.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/time_icon.h"
#include "system/wordset.h"
#include "text/system/title.h"

// The screen that asks whether to save the last battle as the Battle Video, named after the ROM's "btl_rec_sel.c".
// With a video saved already it shows that one's battle mode and parties before asking whether to overwrite it.
//
// The file ends before BtlRecSelGraphic_G3DInit: the three palette fade functions after BtlRecSel_InitPaletteFade,
// which the survey gave to btl_rec_sel_graphic.c, use this file's work, and that file's siblings of the same
// template all start at the 3D system's callback

#define TEXT_WINDOW_COUNT 4
#define SIDE_COUNT 2
#define PARTY_SIZE 6

// The time to choose in when the other player is choosing too, in seconds
#define TIME_LIMIT 60
// How long the result of the save stays, in frames
#define RESULT_WAIT 120

// The palettes of BG1's windows, and that of the window frame
#define PALETTE_TEXT 9
#define PALETTE_FRAME 10
#define PALETTE_YESNO 11
#define PALETTE_MESSAGE 12

#define COLOR_TEXT PRINT_COLOR(1, 2, 0)
#define COLOR_TIME_LOW PRINT_COLOR(3, 4, 0)

// The steps of BtlRecSelProc_Main
enum {
    SEQ_FADE,
    SEQ_FADE_WAIT,
    SEQ_QUESTION,
    SEQ_QUESTION_WAIT,
    SEQ_MESSAGE,
    SEQ_SAVE_YESNO,
    SEQ_NO_SAVE_YESNO,
    SEQ_OVERWRITE_YESNO,
    SEQ_SHOW_PREVIOUS,
    SEQ_HIDE_PREVIOUS,
    SEQ_SAVE_MESSAGE,
    SEQ_SAVE_MESSAGE_WAIT,
    SEQ_SAVE,
    SEQ_SAVED_WAIT,
    SEQ_SAVE_FAILED_WAIT,
    SEQ_END_MESSAGE,
    SEQ_END_MESSAGE_WAIT,
    SEQ_END_SYNC,
    SEQ_END,
};

// What BtlRecSel_UpdatePaletteFade is asked to do, and where it is
enum {
    FADE_REQ_NONE,
    FADE_REQ_IN,
    FADE_REQ_OUT,
};

enum {
    FADE_STATE_OUT,
    FADE_STATE_FADING_IN,
    FADE_STATE_IN,
    FADE_STATE_FADING_OUT,
};

// The text windows other than the message's: the title and the battle mode of the saved video, and the time limit
enum {
    TEXT_WINDOW_TITLE,
    TEXT_WINDOW_MODE,
    TEXT_WINDOW_TIME_LABEL,
    TEXT_WINDOW_TIME,
};

// A Pokémon of the saved video and its icon
typedef struct {
    u32 species;
    u32 form;
    u32 sex;
    BOOL egg;
    UIObjRes res;
    ClActor *actor;
    u8 x;
    u8 y;
    u8 anim;
    u8 priority;
} BtlRecSelIcon;

typedef struct {
    u16 heapId;
    BtlRecSelGraphic *graphic;
    Font *font;
    // The queues of the message window, the saved video's windows and the two windows of the time limit
    PrintQueue *messageQueue;
    PrintQueue *titleQueue;
    PrintQueue *timeLabelQueue;
    PrintQueue *timeQueue;
    // The fade SEQ_FADE starts and the step after it. With paletteFade the palettes fade instead of the brightness,
    // which leaves the sub screen as it is
    u32 fadeNextSeq;
    u32 fadeMode;
    s32 fadeStart;
    s32 fadeEnd;
    s32 fadeWait;
    BOOL paletteFade;
    // The question SEQ_QUESTION asks and the step that reads its answer
    u32 yesNoSeq;
    u32 questionMessage;
    // Whether stopping the time limit clears BG2
    BOOL clearBGOnStop;
    // The message SEQ_MESSAGE prints and the step after it
    u32 messageNextSeq;
    u32 message;
    // Where SEQ_HIDE_PREVIOUS goes on to
    u32 hideNextSeq;
    TCB *vblankTask;
    BOOL hasVsRecorder;
    // Set when there is nothing to show: no Vs. Recorder and no other player to wait for
    BOOL skip;
    MsgData *msgData;
    PrintStream *stream;
    // Whether the message waits for a button once it is printed
    BOOL waitKey;
    u32 waitTimer;
    TCBExManager *tcbExManager;
    BmpWin *messageWindow;
    BmpWin *blankWindow;
    u32 frameChars;
    StrBuf *messageStr;
    BOOL messagePrinted;
    ConfirmDialogSetup yesNoSetup;
    BmpMenu *yesNo;
    BmpWin *textWindows[TEXT_WINDOW_COUNT];
    BOOL textPrinted[TEXT_WINDOW_COUNT];
    // The time limit in frames, on top of a second that the last number stays for, and the frames left after it
    u32 timer;
    u32 timerEnd;
    BOOL timerStopped;
    BOOL timeUp;
    // The color and the seconds the time limit was last printed with
    u32 timeColor;
    u16 timeSeconds;
    BtlRecSelIcon icons[SIDE_COUNT][PARTY_SIZE];
    // The saved video's battle mode as a message, and whether each side is one Trainer's party
    u32 modeMessage;
    BOOL oneTrainerPerSide;
    BattleRecVideo *video;
    u32 mode;
    u32 unk2B4;
    u16 saveSeq;
    u16 saveSubSeq;
    u32 saveResult;
    u32 bgChars;
    // An icon that is loaded and freed again at the start
    UIObjRes dummyRes;
    ClActor *dummyActor;
    TCBManager *tcbManager;
    void *tcbManagerWork;
    PaletteFade *fade;
    u32 fadeRequest;
    u32 fadeState;
    WaitIcon *waitIcon;
} BtlRecSelWork;

typedef struct {
    u32 bg;
    BGSetup setup;
    u32 mode;
    u32 enabled;
} BtlRecSelBGSetup;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 fromEnd;
} BtlRecSelWindowSetup;

// A battle mode's name, and whether two Trainers share each side
typedef struct {
    u8 message;
    u8 multi;
} BtlRecSelMode;

static BOOL BtlRecSelProc_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BtlRecSelProc_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BtlRecSelProc_Main(GameProc *proc, u32 *state, void *param, void *work);
static void BtlRecSel_SetFade(u32 *state, BtlRecSelParam *param, BtlRecSelWork *work, u32 nextSeq, u32 mode, s32 start,
                              s32 end, s32 wait, BOOL paletteFade);
static void BtlRecSel_AskQuestion(u32 *state, BtlRecSelParam *param, BtlRecSelWork *work, u32 yesNoSeq, u32 message,
                                  BOOL clearBGOnStop);
static void BtlRecSel_SetQuestion(BtlRecSelParam *param, BtlRecSelWork *work, u32 yesNoSeq, u32 message,
                                  BOOL clearBGOnStop);
static void BtlRecSel_ShowMessage(u32 *state, BtlRecSelParam *param, BtlRecSelWork *work, u32 nextSeq, u32 message);
static void BtlRecSel_SetMessage(BtlRecSelParam *param, BtlRecSelWork *work, u32 nextSeq, u32 message);
static void BtlRecSel_VBlank(TCB *tcb, void *data);
static void BtlRecSel_InitMessage(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_FreeMessage(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_UpdateMessage(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_ShowMessageWindow(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_HideMessageFrame(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_ClearMessageWindow(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_EraseMessageWindow(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_StartStream(BtlRecSelParam *param, BtlRecSelWork *work, u32 message, BOOL waitKey);
static BOOL BtlRecSel_WaitStream(BtlRecSelParam *param, BtlRecSelWork *work);
static BOOL BtlRecSel_UpdateStream(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_PrintMessage(BtlRecSelParam *param, BtlRecSelWork *work, u32 message);
static BOOL BtlRecSel_IsMessagePrinted(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_InitYesNo(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_FreeYesNo(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_CreateYesNo(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_InitTextWindows(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_FreeTextWindows(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_UpdateTimer(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_FlushTextWindows(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_PrintPrevious(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_ClearPrevious(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_StartTimer(BtlRecSelParam *param, BtlRecSelWork *work, u16 seconds);
static void BtlRecSel_StopTimer(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_PrintTimer(BtlRecSelParam *param, BtlRecSelWork *work, u16 seconds);
static void BtlRecSel_CreateIcons(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_FreeIcons(BtlRecSelParam *param, BtlRecSelWork *work);
static ClActor *BtlRecSel_CreateIcon(ClActUnit *unit, HeapID heapId, u32 vramType, u8 paletteOffset, u32 species,
                                     u32 form, u32 sex, BOOL egg, UIObjRes *res, u8 x, u8 y, u8 anim, u8 bgPriority,
                                     u8 priority);
static void BtlRecSel_FreeIcon(UIObjRes *res, ClActor *actor);
static void BtlRecSel_SetMode(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_LoadBG(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_FreeBG(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_ShowTimerBG(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_ShowPartiesBG(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_ShowMultiPartiesBG(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_ClearBG(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_InitSubBG(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_FreeSubBG(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_CreateDummyIcon(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_FreeDummyIcon(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_InitPaletteFade(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_FreePaletteFade(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_UpdatePaletteFade(BtlRecSelParam *param, BtlRecSelWork *work);
static void BtlRecSel_TransferPaletteFade(BtlRecSelWork *work);

const GameProcFunctions BtlRecSelProcFunctions = { BtlRecSelProc_Init, BtlRecSelProc_Main, BtlRecSelProc_Exit };

// BG0 of the sub screen
static const BtlRecSelBGSetup sBtlRecSelSubBGSetups[1] = {
    { 4,
      { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x8000), GX_BG_CHARBASE(0x0000), 0x8000,
        GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE },
      BGMODE_TEXT,
      TRUE },
};

// By battle mode, as the video's header has it
static const BtlRecSelMode sModes[] = {
    { Title_Text_ColosseumSingleNoRestrictions, FALSE },
    { Title_Text_ColosseumSingleFlat, FALSE },
    { Title_Text_ColosseumSingleNoRestrictions_2, FALSE },
    { Title_Text_ColosseumSingleFlatLauncher, FALSE },
    { Title_Text_ColosseumDoubleNoRestrictions, FALSE },
    { Title_Text_ColosseumDoubleFlat, FALSE },
    { Title_Text_ColosseumDoubleNoRestrictions_2, FALSE },
    { Title_Text_ColosseumDoubleFlatLauncher, FALSE },
    { Title_Text_ColosseumTripleNoRestrictions, FALSE },
    { Title_Text_ColosseumTripleFlat, FALSE },
    { Title_Text_ColosseumTripleNoRestrictions_2, FALSE },
    { Title_Text_ColosseumTripleFlatLauncher, FALSE },
    { Title_Text_ColosseumRotationNoRestrictions, FALSE },
    { Title_Text_ColosseumRotationFlat, FALSE },
    { Title_Text_ColosseumRotationNoRestrictions_2, FALSE },
    { Title_Text_ColosseumRotationFlatLauncher, FALSE },
    { Title_Text_ColosseumMultiNoRestrictions, TRUE },
    { Title_Text_ColosseumMultiFlat, TRUE },
    { Title_Text_ColosseumMultiNoRestrictions_2, TRUE },
    { Title_Text_ColosseumMultiFlatLauncher, TRUE },
    { Title_Text_BattleSubwaySingle, FALSE },
    { Title_Text_BattleSubwayDouble, FALSE },
    { Title_Text_BattleSubwayMulti, TRUE },
    { Title_Text_RandomMatchupFreeSingle, FALSE },
    { Title_Text_RandomMatchupFreeDouble, FALSE },
    { Title_Text_RandomMatchupFreeTriple, FALSE },
    { Title_Text_RandomMatchupFreeRotation, FALSE },
    { Title_Text_RandomMatchupFreeLauncher, FALSE },
    { Title_Text_RandomMatchupRatingSingle, FALSE },
    { Title_Text_RandomMatchupRatingDouble, FALSE },
    { Title_Text_RandomMatchupRatingTriple, FALSE },
    { Title_Text_RandomMatchupRatingRotation, FALSE },
    { Title_Text_RandomMatchupRatingLauncher, FALSE },
    { Title_Text_CompetitionSingleLauncherOff, FALSE },
    { Title_Text_CompetitionSingleLauncher, FALSE },
    { Title_Text_CompetitionDoubleLauncherOff, FALSE },
    { Title_Text_CompetitionDoubleLauncher, FALSE },
    { Title_Text_CompetitionTripleLauncherOff, FALSE },
    { Title_Text_CompetitionTripleLauncher, FALSE },
    { Title_Text_CompetitionRotationLauncherOff, FALSE },
    { Title_Text_CompetitionRotationLauncher, FALSE },
};

static BOOL BtlRecSelProc_Init(GameProc *proc, u32 *state, void *param, void *work) {
    BtlRecSelParam *prm = param;
    BtlRecSelWork *wk;
    u8 i;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_BTL_REC_SEL, 0x50000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(BtlRecSelWork), HEAPID_BTL_REC_SEL);
    sys_memset(wk, 0, sizeof(BtlRecSelWork));
    wk->heapId = HEAPID_BTL_REC_SEL;
    for (i = 0; i <= BGSYS_BG_COUNT - 1; i++) {
        GFL_BGSysSetBGEnabled(i, FALSE);
    }
    wk->graphic = BtlRecSelGraphic_Create(GX_DISP_SELECT_MAIN_SUB, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    wk->messageQueue = func_02021998(wk->heapId);
    wk->titleQueue = func_02021998(wk->heapId);
    wk->timeLabelQueue = func_02021998(wk->heapId);
    wk->timeQueue = func_02021998(wk->heapId);
    wk->hasVsRecorder = BagSave_CheckAmount(GameData_GetBag(prm->gameData), ITEM_VS_RECORDER, 1, wk->heapId);
    wk->skip = FALSE;
    wk->vblankTask = GFL_VBlankTCBAdd(BtlRecSel_VBlank, wk, 1);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TITLE, wk->heapId);
    BtlRecSel_LoadBG(prm, wk);
    BtlRecSel_ClearBG(prm, wk);
    BtlRecSel_InitSubBG(prm, wk);
    BtlRecSel_InitMessage(prm, wk);
    BtlRecSel_ShowMessageWindow(prm, wk);
    BtlRecSel_InitYesNo(prm, wk);
    BtlRecSel_InitTextWindows(prm, wk);
    GFL_BGSysSetBGPriority(2, 2);
    GFL_BGSysSetBGPriority(3, 1);
    GFL_BGSysSetBGPriority(1, 0);
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(3, TRUE);
    GFL_BGSysSetBGEnabled(1, TRUE);
    GFL_BGSysResetStdPalette(0, GX_RGB(0, 0, 0));
    GFL_BGSysResetStdPalette(4, GX_RGB(0, 0, 0));
    GFL_FadeSet(3, 16, 16, 0);
    if (wk->hasVsRecorder) {
        if (prm->recorded) {
            if (prm->compatible) {
                BtlRecSel_SetFade(state, prm, wk, SEQ_QUESTION, 3, 16, 0, 2, FALSE);
                BtlRecSel_SetQuestion(prm, wk, SEQ_SAVE_YESNO, Title_Text_WouldLikeSaveLast, TRUE);
                BtlRecSel_StartTimer(prm, wk, TIME_LIMIT);
                if (prm->comm) {
                    BtlRecSel_ShowTimerBG(prm, wk);
                }
                wk->timerStopped = TRUE;
            } else {
                BtlRecSel_SetFade(state, prm, wk, SEQ_END_MESSAGE, 3, 16, 0, 2, FALSE);
            }
        } else {
            BtlRecSel_SetFade(state, prm, wk, SEQ_END_MESSAGE, 3, 16, 0, 2, FALSE);
        }
    } else {
        if (prm->comm) {
            BtlRecSel_SetFade(state, prm, wk, SEQ_END_MESSAGE, 3, 16, 0, 2, FALSE);
        } else {
            wk->skip = TRUE;
        }
    }
    BtlRecSel_CreateDummyIcon(prm, wk);
    BtlRecSel_InitPaletteFade(prm, wk);
    BtlRecSel_FreeDummyIcon(prm, wk);
    func_02042ba8(TRUE, wk->heapId);
    return TRUE;
}

static BOOL BtlRecSelProc_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    BtlRecSelParam *prm = param;
    BtlRecSelWork *wk = work;

    BtlRecSel_FreePaletteFade(prm, wk);
    BtlRecSel_FreeTextWindows(prm, wk);
    BtlRecSel_FreeYesNo(prm, wk);
    BtlRecSel_FreeMessage(prm, wk);
    BtlRecSel_FreeSubBG(prm, wk);
    BtlRecSel_FreeBG(prm, wk);
    GFL_MsgDataFree(wk->msgData);
    GFL_TCBRemove(wk->vblankTask);
    func_02021c44(wk->timeQueue);
    func_02021a18(wk->timeQueue);
    func_02021c44(wk->timeLabelQueue);
    func_02021a18(wk->timeLabelQueue);
    func_02021c44(wk->titleQueue);
    func_02021a18(wk->titleQueue);
    func_02021c44(wk->messageQueue);
    func_02021a18(wk->messageQueue);
    GFL_FontFree(wk->font);
    BtlRecSelGraphic_Free(wk->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_BTL_REC_SEL);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL BtlRecSelProc_Main(GameProc *proc, u32 *state, void *param, void *work) {
    BtlRecSelParam *prm = param;
    BtlRecSelWork *wk = work;
    SaveControl *save = GameData_GetSaveControl(prm->gameData);
    u32 saved;
    u32 result;
    BOOL done;
    u32 message;

    GCTX_HIDGetPressedKeys();
    switch (*state) {
    case SEQ_FADE:
        if (wk->skip) {
            *state = SEQ_END;
            break;
        }
        if (wk->paletteFade == TRUE) {
            if (wk->fadeStart == 0) {
                wk->fadeRequest = FADE_REQ_OUT;
            } else {
                wk->fadeRequest = FADE_REQ_IN;
            }
        } else {
            GFL_FadeSet(wk->fadeMode, wk->fadeStart, wk->fadeEnd, wk->fadeWait);
        }
        *state = SEQ_FADE_WAIT;
        break;
    case SEQ_FADE_WAIT:
        done = FALSE;
        if (wk->paletteFade == TRUE) {
            if (wk->fadeState == FADE_STATE_OUT || wk->fadeState == FADE_STATE_IN) {
                done = TRUE;
            }
        } else if (!GFL_FadeIsRunning()) {
            done = TRUE;
        }
        if (done) {
            wk->timerStopped = FALSE;
            *state = wk->fadeNextSeq;
        }
        break;
    case SEQ_QUESTION:
        BtlRecSel_StartStream(prm, wk, wk->questionMessage, FALSE);
        *state = SEQ_QUESTION_WAIT;
        break;
    case SEQ_QUESTION_WAIT:
        if (BtlRecSel_WaitStream(prm, wk)) {
            BtlRecSel_CreateYesNo(prm, wk);
            *state = wk->yesNoSeq;
        }
        break;
    case SEQ_MESSAGE:
        BtlRecSel_StartStream(prm, wk, wk->message, FALSE);
        *state = wk->messageNextSeq;
        break;
    case SEQ_SAVE_YESNO: {
        u32 choice = ConfirmDialog_Update(wk->yesNo);

        if (choice != BMPMENU_NULL) {
            func_0203d564(FALSE);
            if (choice == 0) {
                // With a video saved already, show it and ask whether to overwrite it
                if (func_0200bcf8(save, wk->heapId, &saved, 0)) {
                    wk->timerStopped = TRUE;
                    BtlRecSel_SetFade(state, prm, wk, SEQ_SHOW_PREVIOUS, 1, 0, 16, -2, TRUE);
                } else {
                    BtlRecSel_StopTimer(prm, wk);
                    *state = SEQ_SAVE_MESSAGE;
                }
            } else {
                BtlRecSel_AskQuestion(state, prm, wk, SEQ_NO_SAVE_YESNO, Title_Text_LastBattleWillNot, TRUE);
            }
        } else if (wk->timeUp) {
            ConfirmDialog_Free(wk->yesNo);
            *state = SEQ_END_MESSAGE;
        }
        break;
    }
    case SEQ_NO_SAVE_YESNO: {
        u32 choice = ConfirmDialog_Update(wk->yesNo);

        if (choice != BMPMENU_NULL) {
            func_0203d564(FALSE);
            if (choice == 0) {
                BtlRecSel_StopTimer(prm, wk);
                *state = SEQ_END_MESSAGE;
            } else {
                BtlRecSel_AskQuestion(state, prm, wk, SEQ_SAVE_YESNO, Title_Text_WouldLikeSaveLast, TRUE);
            }
        } else if (wk->timeUp) {
            ConfirmDialog_Free(wk->yesNo);
            *state = SEQ_END_MESSAGE;
        }
        break;
    }
    case SEQ_OVERWRITE_YESNO: {
        u32 choice = ConfirmDialog_Update(wk->yesNo);

        if (choice != BMPMENU_NULL) {
            func_0203d564(FALSE);
            if (choice == 0) {
                wk->hideNextSeq = SEQ_SAVE_MESSAGE;
            } else {
                wk->hideNextSeq = SEQ_END_MESSAGE;
            }
            BtlRecSel_StopTimer(prm, wk);
            BtlRecSel_SetFade(state, prm, wk, SEQ_HIDE_PREVIOUS, 1, 0, 16, -2, TRUE);
        } else if (wk->timeUp) {
            ConfirmDialog_Free(wk->yesNo);
            wk->hideNextSeq = SEQ_END_MESSAGE;
            BtlRecSel_SetFade(state, prm, wk, SEQ_HIDE_PREVIOUS, 1, 0, 16, -2, TRUE);
        }
        break;
    }
    case SEQ_SHOW_PREVIOUS: {
        s32 i = 0;

        wk->video = func_0200bcd0(save, wk->heapId, &result, 0);
        BtlRecSel_ClearBG(prm, wk);
        BtlRecSel_ClearMessageWindow(prm, wk);
        if (result == 1) {
            BattleRecHeader *header = func_0200c0cc(wk->video);

            wk->mode = func_0200c124(header, BATTLE_REC_HEADER_MODE, 0);
            BtlRecSel_SetMode(prm, wk);
            for (i = 0; i < SIDE_COUNT * PARTY_SIZE; i++) {
                u32 species;
                u32 form = 0;
                u32 sex = 0;

                species = func_0200c124(header, BATTLE_REC_HEADER_SPECIES, i);
                if (species != 0) {
                    form = func_0200c124(header, BATTLE_REC_HEADER_FORM, i);
                    sex = func_0200c124(header, BATTLE_REC_HEADER_GENDER, i);
                }
                if (wk->oneTrainerPerSide) {
                    if (i < PARTY_SIZE) {
                        wk->icons[1][i % PARTY_SIZE].species = species;
                        wk->icons[1][i % PARTY_SIZE].form = form;
                        wk->icons[1][i % PARTY_SIZE].sex = sex;
                    } else {
                        wk->icons[0][i % PARTY_SIZE].species = species;
                        wk->icons[0][i % PARTY_SIZE].form = form;
                        wk->icons[0][i % PARTY_SIZE].sex = sex;
                    }
                } else {
                    // Three of each Trainer, the partners in the second half of a side
                    u32 side;
                    u32 index;

                    if (i < 3) {
                        side = 1;
                        index = i;
                    } else if (i < 6) {
                        side = 0;
                        index = i - 3;
                    } else if (i < 9) {
                        side = 1;
                        index = i - 3;
                    } else {
                        side = 0;
                        index = i - 6;
                    }
                    wk->icons[side][index].species = species;
                    wk->icons[side][index].form = form;
                    wk->icons[side][index].sex = sex;
                }
            }
            BtlRecSel_PrintPrevious(prm, wk);
            BtlRecSel_CreateIcons(prm, wk);
            if (wk->oneTrainerPerSide) {
                BtlRecSel_ShowPartiesBG(prm, wk);
            } else {
                BtlRecSel_ShowMultiPartiesBG(prm, wk);
            }
            BtlRecSel_SetFade(state, prm, wk, SEQ_QUESTION, 1, 16, 0, -2, TRUE);
            BtlRecSel_SetQuestion(prm, wk, SEQ_OVERWRITE_YESNO, Title_Text_OkOverwriteExistingBattle, FALSE);
        } else {
            BtlRecSel_SetFade(state, prm, wk, SEQ_SAVE_MESSAGE, 1, 16, 0, -2, TRUE);
        }
        func_0200bb5c(wk->video);
        break;
    }
    case SEQ_HIDE_PREVIOUS:
        BtlRecSel_ClearPrevious(prm, wk);
        BtlRecSel_FreeIcons(prm, wk);
        BtlRecSel_ClearBG(prm, wk);
        BtlRecSel_SetFade(state, prm, wk, wk->hideNextSeq, 1, 16, 0, -2, TRUE);
        BtlRecSel_ClearMessageWindow(prm, wk);
        if (wk->hideNextSeq == SEQ_END_MESSAGE && wk->hasVsRecorder && prm->compatible && !prm->comm) {
            BtlRecSel_EraseMessageWindow(prm, wk);
            BtlRecSel_HideMessageFrame(prm, wk);
        }
        BtlRecSel_StopTimer(prm, wk);
        break;
    case SEQ_SAVE_MESSAGE:
        BtlRecSel_PrintMessage(prm, wk, Title_Text_SavingBattleVideoDont);
        *state = SEQ_SAVE_MESSAGE_WAIT;
        break;
    case SEQ_SAVE_MESSAGE_WAIT:
        if (BtlRecSel_IsMessagePrinted(prm, wk)) {
            wk->waitIcon = WaitIcon_Create(GFL_VBlankGetTCBMgr(), wk->messageWindow, 15, 16, wk->heapId);
            wk->unk2B4 = 0;
            wk->saveSeq = 0;
            wk->saveSubSeq = 0;
            wk->saveResult = 0;
            *state = SEQ_SAVE;
        }
        break;
    case SEQ_SAVE:
        if (wk->saveResult != 2 && wk->saveResult != 3) {
            wk->saveResult =
                func_0200be50(prm->gameData, wk->heapId, prm->mode, prm->counter, 0, &wk->saveSeq, &wk->saveSubSeq);
        }
        if (wk->saveResult == 2 || wk->saveResult == 3) {
            func_0200bbb4();
            WaitIcon_Free(wk->waitIcon);
            if (wk->saveResult == 2) {
                GFL_SndSEPlay(SEQ_SE_SAVE);
                BtlRecSel_ShowMessage(state, prm, wk, SEQ_SAVED_WAIT, Title_Text_BattleVideoHasBeen);
                wk->waitTimer = RESULT_WAIT;
            } else if (wk->saveResult == 3) {
                BtlRecSel_ShowMessage(state, prm, wk, SEQ_SAVE_FAILED_WAIT, Title_Text_ThereErrorBattleVideo);
                wk->waitTimer = RESULT_WAIT;
            }
        }
        break;
    case SEQ_SAVED_WAIT:
        if (BtlRecSel_WaitStream(prm, wk)) {
            *state = SEQ_END_MESSAGE;
        }
        break;
    case SEQ_SAVE_FAILED_WAIT:
        if (BtlRecSel_WaitStream(prm, wk)) {
            *state = SEQ_END_MESSAGE;
        }
        break;
    case SEQ_END_MESSAGE:
        if (wk->hasVsRecorder) {
            if (prm->recorded) {
                if (prm->compatible) {
                    if (prm->comm) {
                        message = Title_Text_AwaitingFriendsDecisionPlease;
                    } else {
                        BtlRecSel_SetFade(state, prm, wk, SEQ_END, 3, 0, 16, 2, FALSE);
                        break;
                    }
                } else {
                    if (prm->comm) {
                        message = Title_Text_CannotRecordVideosTrainer;
                    } else {
                        message = Title_Text_CannotRecordVideosTrainer_2;
                    }
                }
            } else {
                message = Title_Text_LastBattleCannotRecorded;
            }
        } else {
            if (prm->comm) {
                message = Title_Text_AwaitingFriendsDecisionPlease;
            } else {
                *state = SEQ_END;
                break;
            }
        }
        if (prm->comm) {
            BtlRecSel_PrintMessage(prm, wk, message);
        } else {
            BtlRecSel_StartStream(prm, wk, message, TRUE);
        }
        *state = SEQ_END_MESSAGE_WAIT;
        break;
    case SEQ_END_MESSAGE_WAIT:
        done = FALSE;
        if (prm->comm) {
            if (BtlRecSel_IsMessagePrinted(prm, wk)) {
                done = TRUE;
            }
        } else if (BtlRecSel_WaitStream(prm, wk)) {
            done = TRUE;
        }
        if (done) {
            if (prm->comm) {
                func_02040624(func_02040440(), 150, 43);
                wk->waitIcon = WaitIcon_Create(GFL_VBlankGetTCBMgr(), wk->messageWindow, 15, 16, wk->heapId);
            }
            *state = SEQ_END_SYNC;
        }
        break;
    case SEQ_END_SYNC:
        // Until the other player is done too
        done = TRUE;
        if (prm->comm) {
            if (!GFL_NetErrCheck()) {
                done = func_02040664(func_02040440(), 150, 43);
            }
            if (done) {
                WaitIcon_Free(wk->waitIcon);
            }
        }
        if (done) {
            BtlRecSel_SetFade(state, prm, wk, SEQ_END, 3, 0, 16, 2, FALSE);
        }
        break;
    case SEQ_END:
        return TRUE;
    }
    BtlRecSel_UpdateMessage(prm, wk);
    BtlRecSel_UpdateTimer(prm, wk);
    BtlRecSel_FlushTextWindows(prm, wk);
    BtlRecSel_UpdatePaletteFade(prm, wk);
    func_02021a3c(wk->messageQueue);
    func_02021a3c(wk->titleQueue);
    func_02021a3c(wk->timeLabelQueue);
    func_02021a3c(wk->timeQueue);
    BtlRecSelGraphic_Update(wk->graphic);
    BtlRecSelGraphic_Begin3D(wk->graphic);
    BtlRecSelGraphic_End3D(wk->graphic);
    return FALSE;
}

// Sets the fade SEQ_FADE starts, and the step after it
static void BtlRecSel_SetFade(u32 *state, BtlRecSelParam *param, BtlRecSelWork *work, u32 nextSeq, u32 mode, s32 start,
                              s32 end, s32 wait, BOOL paletteFade) {
    *state = SEQ_FADE;
    work->fadeNextSeq = nextSeq;
    work->fadeMode = mode;
    work->fadeStart = start;
    work->fadeEnd = end;
    work->fadeWait = wait;
    work->paletteFade = paletteFade;
}

static void BtlRecSel_AskQuestion(u32 *state, BtlRecSelParam *param, BtlRecSelWork *work, u32 yesNoSeq, u32 message,
                                  BOOL clearBGOnStop) {
    *state = SEQ_QUESTION;
    BtlRecSel_SetQuestion(param, work, yesNoSeq, message, clearBGOnStop);
}

static void BtlRecSel_SetQuestion(BtlRecSelParam *param, BtlRecSelWork *work, u32 yesNoSeq, u32 message,
                                  BOOL clearBGOnStop) {
    work->yesNoSeq = yesNoSeq;
    work->questionMessage = message;
    work->clearBGOnStop = clearBGOnStop;
}

static void BtlRecSel_ShowMessage(u32 *state, BtlRecSelParam *param, BtlRecSelWork *work, u32 nextSeq, u32 message) {
    *state = SEQ_MESSAGE;
    BtlRecSel_SetMessage(param, work, nextSeq, message);
}

static void BtlRecSel_SetMessage(BtlRecSelParam *param, BtlRecSelWork *work, u32 nextSeq, u32 message) {
    work->messageNextSeq = nextSeq;
    work->message = message;
}

static void BtlRecSel_VBlank(TCB *tcb, void *data) {
    BtlRecSel_TransferPaletteFade(data);
}

static void BtlRecSel_InitMessage(BtlRecSelParam *param, BtlRecSelWork *work) {
    u16 *palette = GFL_HeapAllocate(work->heapId, 3 * sizeof(u16), TRUE, "btl_rec_sel.c", 1513);

    // The text colors of the windows' palette
    palette[0] = GX_RGB(11, 10, 10);
    palette[1] = GX_RGB(20, 20, 21);
    palette[2] = GX_RGB(31, 31, 31);
    GFL_BGSysUploadStdPalette(2, palette, 3 * sizeof(u16), (PALETTE_TEXT * 16 + 13) * sizeof(u16));
    GFL_HeapFree(palette);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, PALETTE_MESSAGE * 0x20, 0x20, work->heapId);
    work->blankWindow = BmpWin_CreateDynamic(1, 0, 0, 1, 1, PALETTE_TEXT, FALSE);
    GFL_BitmapFill(BmpWin_GetBitmap(work->blankWindow), 0);
    BmpWin_FlushChar(work->blankWindow);
    work->messageWindow = BmpWin_CreateDynamic(1, 1, 19, 30, 4, PALETTE_MESSAGE, FALSE);
    GFL_BitmapFill(BmpWin_GetBitmap(work->messageWindow), 15);
    BmpWin_FlushChar(work->messageWindow);
    work->frameChars = LoadCursorImageEndOfHeap(1, PALETTE_FRAME, 0, work->heapId);
    work->tcbExManager = GFL_TCBExMgrCreate(work->heapId, work->heapId, 1, 0);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    GFL_BGSysQueueScrLoad(1);
    work->stream = NULL;
    work->waitKey = FALSE;
    work->waitTimer = 0;
    work->messageStr = NULL;
}

static void BtlRecSel_FreeMessage(BtlRecSelParam *param, BtlRecSelWork *work) {
    if (work->stream != NULL) {
        func_020223cc(work->stream);
    }
    if (work->messageStr != NULL) {
        GFL_StrBufFree(work->messageStr);
    }
    GFL_TCBExMgrFree(work->tcbExManager);
    GFL_BGSysFreeCharMemory(1, CHAR_POS(work->frameChars), CHAR_SIZE(work->frameChars));
    BmpWin_Free(work->messageWindow);
    BmpWin_Free(work->blankWindow);
}

static void BtlRecSel_UpdateMessage(BtlRecSelParam *param, BtlRecSelWork *work) {
    GFL_TCBExMgrUpdate(work->tcbExManager);
    if (GCTX_HIDGetUpdateRate() == 30) {
        GFL_TCBExMgrUpdate(work->tcbExManager);
    }
}

static void BtlRecSel_ShowMessageWindow(BtlRecSelParam *param, BtlRecSelWork *work) {
    BmpWin_DrawFrame(work->messageWindow, TRUE, CHAR_POS(work->frameChars), PALETTE_FRAME);
    BmpWin_Transfer(work->messageWindow);
}

static void BtlRecSel_HideMessageFrame(BtlRecSelParam *param, BtlRecSelWork *work) {
    BmpWin_ClearFrame(work->messageWindow, TRUE);
}

static void BtlRecSel_ClearMessageWindow(BtlRecSelParam *param, BtlRecSelWork *work) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->messageWindow), 15);
    BmpWin_Transfer(work->messageWindow);
}

static void BtlRecSel_EraseMessageWindow(BtlRecSelParam *param, BtlRecSelWork *work) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->messageWindow), 0);
    BmpWin_Transfer(work->messageWindow);
}

// Prints a message a character at a time. With waitKey it stays until a button or the touch screen is pressed
static void BtlRecSel_StartStream(BtlRecSelParam *param, BtlRecSelWork *work, u32 message, BOOL waitKey) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->messageWindow), 15);
    if (work->stream != NULL) {
        func_020223cc(work->stream);
    }
    if (work->messageStr != NULL) {
        GFL_StrBufFree(work->messageStr);
    }
    work->messageStr = GFL_MsgDataLoadStrbufNew(work->msgData, message);
    work->stream = func_02022268(work->messageWindow, 0, 0, work->messageStr, work->font, func_02017bcc(),
                                 work->tcbExManager, 2, work->heapId, 15);
    work->waitKey = waitKey;
}

// Twice a frame at 30 frames a second
static BOOL BtlRecSel_WaitStream(BtlRecSelParam *param, BtlRecSelWork *work) {
    BOOL done = BtlRecSel_UpdateStream(param, work);

    if (!done && GCTX_HIDGetUpdateRate() == 30) {
        done = BtlRecSel_UpdateStream(param, work);
    }
    return done;
}

static BOOL BtlRecSel_UpdateStream(BtlRecSelParam *param, BtlRecSelWork *work) {
    BOOL done = FALSE;

    switch (func_020223b4(work->stream)) {
    case PRINT_STREAM_RUNNING:
        if ((GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da2c()) {
            func_020223e0(work->stream, 0);
            if (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                func_0203d564(FALSE);
            } else {
                func_0203d564(TRUE);
            }
        }
        break;
    case PRINT_STREAM_PAUSED:
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            func_020223bc(work->stream);
            if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                func_0203d564(FALSE);
            } else {
                func_0203d564(TRUE);
            }
        }
        break;
    case PRINT_STREAM_DONE:
        if (work->waitTimer != 0) {
            work->waitTimer--;
        } else {
            if (work->waitKey) {
                if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
                    if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                        func_0203d564(FALSE);
                    } else {
                        func_0203d564(TRUE);
                    }
                    work->waitKey = FALSE;
                    done = TRUE;
                }
            } else {
                done = TRUE;
            }
        }
        break;
    }
    return done;
}

// Prints a message at once, through the queue
static void BtlRecSel_PrintMessage(BtlRecSelParam *param, BtlRecSelWork *work, u32 message) {
    GFL_BitmapFill(BmpWin_GetBitmap(work->messageWindow), 15);
    if (work->stream != NULL) {
        func_020223cc(work->stream);
    }
    work->stream = NULL;
    if (work->messageStr != NULL) {
        GFL_StrBufFree(work->messageStr);
    }
    work->messageStr = GFL_MsgDataLoadStrbufNew(work->msgData, message);
    work->messagePrinted = FALSE;
    func_02021c54(work->messageQueue, BmpWin_GetBitmap(work->messageWindow), 0, 0, work->messageStr, work->font);
    BtlRecSel_IsMessagePrinted(param, work);
}

static BOOL BtlRecSel_IsMessagePrinted(BtlRecSelParam *param, BtlRecSelWork *work) {
    if (!work->messagePrinted && !func_02021c1c(work->messageQueue, BmpWin_GetBitmap(work->messageWindow))) {
        BmpWin_Transfer(work->messageWindow);
        work->messagePrinted = TRUE;
    }
    return work->messagePrinted;
}

static void BtlRecSel_InitYesNo(BtlRecSelParam *param, BtlRecSelWork *work) {
    // Where the menu's window will be, at the end of BG1's characters
    u32 chars = GFL_BGSysAllocChar(1, 0x500, TRUE);
    u16 *palette;

    GFL_BGSysFreeCharMemory(1, chars, 0x500);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, PALETTE_YESNO * 0x20, 0x20, work->heapId);
    palette = GFL_HeapAllocate(work->heapId, 3 * sizeof(u16), TRUE, "btl_rec_sel.c", 1785);
    palette[0] = GX_RGB(11, 10, 10);
    palette[1] = GX_RGB(20, 20, 21);
    palette[2] = GX_RGB(31, 31, 31);
    GFL_BGSysUploadStdPalette(2, palette, 3 * sizeof(u16), (PALETTE_YESNO * 16 + 13) * sizeof(u16));
    GFL_HeapFree(palette);
    work->yesNoSetup.bg = 1;
    work->yesNoSetup.x = 24;
    work->yesNoSetup.y = 13;
    work->yesNoSetup.palette = PALETTE_YESNO;
    work->yesNoSetup.unk4 = chars;
}

static void BtlRecSel_FreeYesNo(BtlRecSelParam *param, BtlRecSelWork *work) {
}

static void BtlRecSel_CreateYesNo(BtlRecSelParam *param, BtlRecSelWork *work) {
    work->yesNo =
        ShopUI_CreateConfirmDialog(&work->yesNoSetup, CHAR_POS(work->frameChars), PALETTE_FRAME, 0, work->heapId);
}

static void BtlRecSel_InitTextWindows(BtlRecSelParam *param, BtlRecSelWork *work) {
    // The title and the battle mode, and the time limit's label and number
    BtlRecSelWindowSetup setups[TEXT_WINDOW_COUNT] = {
        { 1, 2, 1, 13, 2, FALSE },
        { 1, 3, 3, 26, 4, FALSE },
        { 3, 2, 15, 11, 2, TRUE },
        { 3, 13, 15, 5, 3, TRUE },
    };
    u8 i;

    for (i = 0; i < TEXT_WINDOW_COUNT; i++) {
        work->textWindows[i] = BmpWin_CreateDynamic(setups[i].bg, setups[i].x, setups[i].y, setups[i].width,
                                                    setups[i].height, PALETTE_TEXT, setups[i].fromEnd);
        GFL_BitmapFill(BmpWin_GetBitmap(work->textWindows[i]), 0);
        work->textPrinted[i] = TRUE;
    }
    work->timeColor = 0;
    work->timeSeconds = 0;
}

static void BtlRecSel_FreeTextWindows(BtlRecSelParam *param, BtlRecSelWork *work) {
    u8 i;

    for (i = 0; i < TEXT_WINDOW_COUNT; i++) {
        BmpWin_Free(work->textWindows[i]);
    }
}

// Counts the time limit down while the other player is choosing too. timeUp is set when it reaches 0; the number
// stays for half a second more
static void BtlRecSel_UpdateTimer(BtlRecSelParam *param, BtlRecSelWork *work) {
    u32 fps;

    if (GCTX_HIDGetUpdateRate() == 30) {
        fps = 30;
    } else {
        fps = 60;
    }
    if (param->comm && !work->timerStopped) {
        if (work->timer >= fps || work->timerEnd != 0) {
            BtlRecSel_PrintTimer(param, work, work->timer / fps);
            if (work->timer >= fps) {
                work->timer--;
                if (work->timer == fps - 1) {
                    work->timeUp = TRUE;
                }
            } else {
                work->timerEnd--;
                if (work->timerEnd == 0) {
                    BtlRecSel_StopTimer(param, work);
                }
            }
        }
    }
}

static void BtlRecSel_FlushTextWindows(BtlRecSelParam *param, BtlRecSelWork *work) {
    u8 i;

    for (i = TEXT_WINDOW_TITLE; i <= TEXT_WINDOW_MODE; i++) {
        if (!work->textPrinted[i] && !func_02021c1c(work->titleQueue, BmpWin_GetBitmap(work->textWindows[i]))) {
            BmpWin_Transfer(work->textWindows[i]);
            work->textPrinted[i] = TRUE;
        }
    }
    if (!work->textPrinted[TEXT_WINDOW_TIME_LABEL] &&
        !func_02021c1c(work->timeLabelQueue, BmpWin_GetBitmap(work->textWindows[TEXT_WINDOW_TIME_LABEL]))) {
        BmpWin_Transfer(work->textWindows[TEXT_WINDOW_TIME_LABEL]);
        work->textPrinted[TEXT_WINDOW_TIME_LABEL] = TRUE;
    }
    if (!work->textPrinted[TEXT_WINDOW_TIME] &&
        !func_02021c1c(work->timeQueue, BmpWin_GetBitmap(work->textWindows[TEXT_WINDOW_TIME]))) {
        BmpWin_Transfer(work->textWindows[TEXT_WINDOW_TIME]);
        work->textPrinted[TEXT_WINDOW_TIME] = TRUE;
    }
}

// Prints "PREVIOUS RECORD" and the saved video's battle mode
static void BtlRecSel_PrintPrevious(BtlRecSelParam *param, BtlRecSelWork *work) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, Title_Text_PreviousRecord);

    func_02021c7c(work->titleQueue, BmpWin_GetBitmap(work->textWindows[TEXT_WINDOW_TITLE]), 4, 1, str, work->font,
                  COLOR_TEXT);
    GFL_StrBufFree(str);
    work->textPrinted[TEXT_WINDOW_TITLE] = FALSE;
    str = GFL_MsgDataLoadStrbufNew(work->msgData, work->modeMessage);
    func_02021c7c(work->titleQueue, BmpWin_GetBitmap(work->textWindows[TEXT_WINDOW_MODE]), 4, 1, str, work->font,
                  COLOR_TEXT);
    GFL_StrBufFree(str);
    work->textPrinted[TEXT_WINDOW_MODE] = FALSE;
    BtlRecSel_FlushTextWindows(param, work);
}

static void BtlRecSel_ClearPrevious(BtlRecSelParam *param, BtlRecSelWork *work) {
    u8 i;

    for (i = TEXT_WINDOW_TITLE; i <= TEXT_WINDOW_MODE; i++) {
        func_02021c44(work->titleQueue);
        GFL_BitmapFill(BmpWin_GetBitmap(work->textWindows[i]), 0);
        BmpWin_Transfer(work->textWindows[i]);
        work->textPrinted[i] = TRUE;
    }
}

static void BtlRecSel_StartTimer(BtlRecSelParam *param, BtlRecSelWork *work, u16 seconds) {
    u32 fps;

    if (GCTX_HIDGetUpdateRate() == 30) {
        fps = 30;
    } else {
        fps = 60;
    }
    work->timer = fps * (seconds + 1) - 1;
    work->timerEnd = fps / 2;
    work->timerStopped = FALSE;
    work->timeUp = FALSE;
    work->timeColor = 0;
    work->timeSeconds = 0;
    BtlRecSel_PrintTimer(param, work, seconds);
}

static void BtlRecSel_StopTimer(BtlRecSelParam *param, BtlRecSelWork *work) {
    work->timer = 0;
    work->timerEnd = 0;
    func_02021c44(work->timeLabelQueue);
    GFL_BitmapFill(BmpWin_GetBitmap(work->textWindows[TEXT_WINDOW_TIME_LABEL]), 0);
    BmpWin_Transfer(work->textWindows[TEXT_WINDOW_TIME_LABEL]);
    work->textPrinted[TEXT_WINDOW_TIME_LABEL] = TRUE;
    func_02021c44(work->timeQueue);
    GFL_BitmapFill(BmpWin_GetBitmap(work->textWindows[TEXT_WINDOW_TIME]), 0);
    BmpWin_Transfer(work->textWindows[TEXT_WINDOW_TIME]);
    work->textPrinted[TEXT_WINDOW_TIME] = TRUE;
    if (work->clearBGOnStop) {
        BtlRecSel_ClearBG(param, work);
    }
    work->timeColor = 0;
    work->timeSeconds = 0;
}

// Prints the time limit, in another color for the last 10 seconds
static void BtlRecSel_PrintTimer(BtlRecSelParam *param, BtlRecSelWork *work, u16 seconds) {
    u16 color;
    u32 colorType;

    if (!param->comm) {
        return;
    }
    if (seconds > 10) {
        color = COLOR_TEXT;
        colorType = 1;
    } else {
        color = COLOR_TIME_LOW;
        colorType = 2;
    }
    if (colorType != work->timeColor) {
        GFL_BitmapFill(BmpWin_GetBitmap(work->textWindows[TEXT_WINDOW_TIME_LABEL]), 0);
        GFL_BitmapFill(BmpWin_GetBitmap(work->textWindows[TEXT_WINDOW_TIME]), 0);
    } else if (seconds != work->timeSeconds) {
        GFL_BitmapFill(BmpWin_GetBitmap(work->textWindows[TEXT_WINDOW_TIME]), 0);
    }
    if (colorType != work->timeColor) {
        StrBuf *str;

        func_02021c44(work->timeLabelQueue);
        str = GFL_MsgDataLoadStrbufNew(work->msgData, Title_Text_TimeLimit);
        func_02021c7c(work->timeLabelQueue, BmpWin_GetBitmap(work->textWindows[TEXT_WINDOW_TIME_LABEL]), 4, 1, str,
                      work->font, color);
        work->textPrinted[TEXT_WINDOW_TIME_LABEL] = FALSE;
        GFL_StrBufFree(str);
    }
    if (colorType != work->timeColor || seconds != work->timeSeconds) {
        WordSet *wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
        StrBuf *format = GFL_MsgDataLoadStrbufNew(work->msgData, Title_Text_Empty_8);
        StrBuf *str = GFL_StrBufCreate(8, work->heapId);

        func_02021c44(work->timeQueue);
        WordSetNumber(wordSet, 1, seconds, 2, 2, TRUE);
        GFL_WordSetFormatStrbuf(wordSet, str, format);
        func_02021c7c(work->timeQueue, BmpWin_GetBitmap(work->textWindows[TEXT_WINDOW_TIME]), 2, 2, str, work->font,
                      color);
        work->textPrinted[TEXT_WINDOW_TIME] = FALSE;
        GFL_StrBufFree(str);
        GFL_StrBufFree(format);
        GFL_WordSetSystemFree(wordSet);
    }
    work->timeColor = colorType;
    work->timeSeconds = seconds;
    BtlRecSel_FlushTextWindows(param, work);
}

// Creates the icons of the saved video's Pokémon: a row for each side, or two rows of three with two Trainers a side
static void BtlRecSel_CreateIcons(BtlRecSelParam *param, BtlRecSelWork *work) {
    ClActUnit *unit = BtlRecSelGraphic_GetClActUnit(work->graphic);
    u8 side;
    u8 i;

    for (side = 0; side < SIDE_COUNT; side++) {
        for (i = 0; i < PARTY_SIZE; i++) {
            work->icons[side][i].egg = FALSE;
            work->icons[side][i].anim = 1;
        }
    }
    for (side = 0; side < SIDE_COUNT; side++) {
        for (i = 0; i < PARTY_SIZE; i++) {
            if (work->oneTrainerPerSide) {
                if (side == 0) {
                    work->icons[side][i].x = i * 24 + 100;
                    work->icons[side][i].y = 64;
                    work->icons[side][i].priority = PARTY_SIZE - i;
                } else {
                    work->icons[side][i].x = i * 24 + 36;
                    work->icons[side][i].y = 112;
                    work->icons[side][i].priority = PARTY_SIZE - i;
                }
            } else {
                if (side == 0) {
                    if (i < 3) {
                        work->icons[side][i].x = i * 24 + 172;
                        work->icons[side][i].y = 64;
                        work->icons[side][i].priority = PARTY_SIZE - i;
                    } else {
                        work->icons[side][i].x = (i - 3) * 24 + 172;
                        work->icons[side][i].y = 88;
                        work->icons[side][i].priority = PARTY_SIZE - i;
                    }
                } else {
                    if (i < 3) {
                        work->icons[side][i].x = i * 24 + 36;
                        work->icons[side][i].y = 88;
                        work->icons[side][i].priority = PARTY_SIZE - i;
                    } else {
                        work->icons[side][i].x = (i - 3) * 24 + 36;
                        work->icons[side][i].y = 112;
                        work->icons[side][i].priority = PARTY_SIZE - i;
                    }
                }
            }
        }
    }
    for (side = 0; side < SIDE_COUNT; side++) {
        for (i = 0; i < PARTY_SIZE; i++) {
            if (work->icons[side][i].species != 0) {
                work->icons[side][i].actor =
                    BtlRecSel_CreateIcon(unit, work->heapId, CLACT_VRAM_MAIN, 0, work->icons[side][i].species,
                                         work->icons[side][i].form, work->icons[side][i].sex, work->icons[side][i].egg,
                                         &work->icons[side][i].res, work->icons[side][i].x, work->icons[side][i].y,
                                         work->icons[side][i].anim, 1, work->icons[side][i].priority);
            }
        }
    }
}

static void BtlRecSel_FreeIcons(BtlRecSelParam *param, BtlRecSelWork *work) {
    u8 side;
    u8 i;

    for (side = 0; side < SIDE_COUNT; side++) {
        for (i = 0; i < PARTY_SIZE; i++) {
            if (work->icons[side][i].species != 0) {
                BtlRecSel_FreeIcon(&work->icons[side][i].res, work->icons[side][i].actor);
            }
        }
    }
}

// Loads a Pokémon's icon and creates its actor
static ClActor *BtlRecSel_CreateIcon(ClActUnit *unit, HeapID heapId, u32 vramType, u8 paletteOffset, u32 species,
                                     u32 form, u32 sex, BOOL egg, UIObjRes *res, u8 x, u8 y, u8 anim, u8 bgPriority,
                                     u8 priority) {
    UIObjResSetup setup;
    ClActor *actor;

    setup.vramType = vramType;
    setup.flags = 1;
    setup.arcId = ARCID_POKEICON;
    setup.paletteFile = PokeIcon_GetPaletteFile();
    setup.charFile = PokeParty_GetIconIndex(species, form, sex, egg);
    setup.cellFile = PokeIcon_GetCellFile_Main();
    setup.animFile = getOBJTileMapping_MainEng();
    setup.paletteOffset = paletteOffset;
    setup.paletteStart = 0;
    setup.paletteCount = 3;
    UIObjRes_Load(res, &setup, unit, heapId);
    actor = UIObjRes_CreateActor(res, unit, x, y, anim, heapId);
    func_0204c468(actor, bgPriority);
    func_0204c438(actor, priority);
    func_0204c520(actor, TRUE);
    func_0204c378(actor, PokeIcon_GetPalette(species, form, sex, egg), 0);
    return actor;
}

static void BtlRecSel_FreeIcon(UIObjRes *res, ClActor *actor) {
    func_0204c108(actor);
    UIObjRes_Free(res);
}

// Sets the saved video's battle mode message and party layout from its mode
static void BtlRecSel_SetMode(BtlRecSelParam *param, BtlRecSelWork *work) {
    u8 mode = work->mode;

    if (mode >= NELEMS(sModes)) {
        mode = 0;
    }
    work->modeMessage = sModes[mode].message;
    if (!sModes[mode].multi) {
        work->oneTrainerPerSide = TRUE;
    } else {
        work->oneTrainerPerSide = FALSE;
    }
}

static void BtlRecSel_LoadBG(BtlRecSelParam *param, BtlRecSelWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_BTL_REC_SEL, work->heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0, 0x140, work->heapId);
    work->bgChars = GFL_BGSysLoadArcNCGRDynamic(arc, 1, 2, 0, FALSE, work->heapId);
    GFL_ArcToolFree(arc);
}

static void BtlRecSel_FreeBG(BtlRecSelParam *param, BtlRecSelWork *work) {
    GFL_BGSysFreeCharMemory(2, CHAR_POS(work->bgChars), CHAR_SIZE(work->bgChars));
}

// BG2's screens: the time limit's frame, and the frames of the saved video's parties
static void BtlRecSel_ShowTimerBG(BtlRecSelParam *param, BtlRecSelWork *work) {
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_BTL_REC_SEL, 2, 2, CHAR_POS(work->bgChars), 0, FALSE, work->heapId);
    GFL_BGSysQueueScrLoad(2);
    GFL_BGSysMoveBGReq(3, BG_MOVE_SET_Y, 0);
}

static void BtlRecSel_ShowPartiesBG(BtlRecSelParam *param, BtlRecSelWork *work) {
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_BTL_REC_SEL, 3, 2, CHAR_POS(work->bgChars), 0, FALSE, work->heapId);
    GFL_BGSysQueueScrLoad(2);
    GFL_BGSysMoveBG(3, BG_MOVE_SET_Y, -8);
}

static void BtlRecSel_ShowMultiPartiesBG(BtlRecSelParam *param, BtlRecSelWork *work) {
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_BTL_REC_SEL, 4, 2, CHAR_POS(work->bgChars), 0, FALSE, work->heapId);
    GFL_BGSysQueueScrLoad(2);
    GFL_BGSysMoveBG(3, BG_MOVE_SET_Y, -8);
}

static void BtlRecSel_ClearBG(BtlRecSelParam *param, BtlRecSelWork *work) {
    GFL_BGSysFillScrAsync(2, 0);
}

// The sub screen's background, in the player's color
static void BtlRecSel_InitSubBG(BtlRecSelParam *param, BtlRecSelWork *work) {
    u8 gender = getTrainerGender(GetGameDataPlayerInfo(param->gameData));
    int i;
    HeapID heapId;
    u32 palette;
    ArcTool *arc;

    G2S_BlendNone();
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
    GXS_DispOn();
    GFL_BGSysDisableAllB();
    gfxSetBGModeB(0);
    GFL_BGSysDisableBGsB();
    for (i = 0; i < NELEMS(sBtlRecSelSubBGSetups); i++) {
        GFL_BGSysCreateBG(sBtlRecSelSubBGSetups[i].bg, &sBtlRecSelSubBGSetups[i].setup, sBtlRecSelSubBGSetups[i].mode);
        GFL_BGSysClearBG(sBtlRecSelSubBGSetups[i].bg);
        GFL_BGSysSetBGEnabled(sBtlRecSelSubBGSetups[i].bg, sBtlRecSelSubBGSetups[i].enabled);
    }
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    heapId = work->heapId;
    palette = 1;
    if (gender != 1) {
        palette = 0;
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_C_GEAR, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, palette, 4, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 2, 4, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 3, 4, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(4);
}

static void BtlRecSel_FreeSubBG(BtlRecSelParam *param, BtlRecSelWork *work) {
    int i;

    for (i = 0; i < NELEMS(sBtlRecSelSubBGSetups); i++) {
        GFL_BGSysReleaseBG(sBtlRecSelSubBGSetups[i].bg);
    }
    G2S_BlendNone();
    GXS_SetVisibleWnd(GX_WNDMASK_NONE);
}

// Loads an icon, which BtlRecSel_FreeDummyIcon frees once the palettes are copied for the fade
static void BtlRecSel_CreateDummyIcon(BtlRecSelParam *param, BtlRecSelWork *work) {
    ClActUnit *unit = BtlRecSelGraphic_GetClActUnit(work->graphic);

    work->dummyActor =
        BtlRecSel_CreateIcon(unit, work->heapId, CLACT_VRAM_MAIN, 0, 1, 0, 0, FALSE, &work->dummyRes, 0, 0, 0, 1, 1);
}

static void BtlRecSel_FreeDummyIcon(BtlRecSelParam *param, BtlRecSelWork *work) {
    BtlRecSel_FreeIcon(&work->dummyRes, work->dummyActor);
}

// The fade of the main screen's BG and OBJ palettes, other than the last two of each
static void BtlRecSel_InitPaletteFade(BtlRecSelParam *param, BtlRecSelWork *work) {
    work->tcbManagerWork = GFL_HeapAllocate(work->heapId, GFL_TCBMgrCalcAllocSize(2), TRUE, "btl_rec_sel.c", 2403);
    work->tcbManager = GFL_TCBMgrCreate(2, work->tcbManagerWork);
    work->fade = PaletteFade_Create(work->heapId);
    PaletteFade_SetTransferAll(work->fade, TRUE);
    PaletteFade_AllocBuffer(work->fade, 0, 14 * 0x20, work->heapId);
    PaletteFade_AllocBuffer(work->fade, 2, 14 * 0x20, work->heapId);
    PaletteFade_LoadFromVRAM(work->fade, 0, 0, 14 * 0x20);
    PaletteFade_LoadFromVRAM(work->fade, 2, 0, 14 * 0x20);
    work->fadeState = FADE_STATE_IN;
    work->fadeRequest = FADE_REQ_NONE;
}

static void BtlRecSel_FreePaletteFade(BtlRecSelParam *param, BtlRecSelWork *work) {
    PaletteFade_FreeBuffer(work->fade, 0);
    PaletteFade_FreeBuffer(work->fade, 2);
    PaletteFade_Free(work->fade);
    func_0203a610(work->tcbManager);
    GFL_HeapFree(work->tcbManagerWork);
}

static void BtlRecSel_UpdatePaletteFade(BtlRecSelParam *param, BtlRecSelWork *work) {
    u16 mask;
    u8 i;

    GFL_TCBMgrUpdate(work->tcbManager);
    if (PaletteFade_GetActiveMask(work->fade) == 0) {
        switch (work->fadeState) {
        case FADE_STATE_FADING_IN:
            work->fadeState = FADE_STATE_IN;
            break;
        case FADE_STATE_FADING_OUT:
            work->fadeState = FADE_STATE_OUT;
            break;
        }
        switch (work->fadeRequest) {
        case FADE_REQ_IN:
            if (work->fadeState != FADE_STATE_FADING_IN) {
                mask = 0;
                for (i = 0; i < 14; i++) {
                    mask |= 1 << i;
                }
                PaletteFade_StartFade(work->fade, 1 << 0, mask, -2, 16, 0, GX_RGB(0, 0, 0), work->tcbManager);
                mask = 0;
                for (i = 0; i < 14; i++) {
                    mask |= 1 << i;
                }
                PaletteFade_StartFade(work->fade, 1 << 2, mask, -2, 16, 0, GX_RGB(0, 0, 0), work->tcbManager);
                work->fadeState = FADE_STATE_FADING_IN;
                work->fadeRequest = FADE_REQ_NONE;
            }
            break;
        case FADE_REQ_OUT:
            if (work->fadeState != FADE_STATE_FADING_OUT) {
                mask = 0;
                for (i = 0; i < 14; i++) {
                    mask |= 1 << i;
                }
                PaletteFade_StartFade(work->fade, 1 << 0, mask, -2, 0, 16, GX_RGB(0, 0, 0), work->tcbManager);
                mask = 0;
                for (i = 0; i < 14; i++) {
                    mask |= 1 << i;
                }
                PaletteFade_StartFade(work->fade, 1 << 2, mask, -2, 0, 16, GX_RGB(0, 0, 0), work->tcbManager);
                work->fadeState = FADE_STATE_FADING_OUT;
                work->fadeRequest = FADE_REQ_NONE;
            }
            break;
        }
    }
}

static void BtlRecSel_TransferPaletteFade(BtlRecSelWork *work) {
    PaletteFade_Transfer(work->fade);
}
