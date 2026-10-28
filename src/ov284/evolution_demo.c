// The ROM names this file shinka_demo.c.
#include "types.h"
#include "app/p_status.h"
#include "app/ui/ui_scene.h"
#include "app/ui/yesno_menu.h"
#include "battle/b_app_tool.h"
#include "battle/b_plist_main.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "demo/evolution_demo.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/hw.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "pml/species_names.h"
#include "save/bag.h"
#include "save/pokedex_savedata.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/app_keycursor.h"
#include "system/bmp_winframe.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wordset.h"

// The evolution process: its steps, the music, the messages, learning the new form's moves and making a Shedinja

#define ACTOR_COUNT 6

// The steps of the process
enum {
    EVOLUTION_DEMO_WAIT,
    EVOLUTION_DEMO_FADE_IN,
    EVOLUTION_DEMO_WAIT_ACTORS,
    EVOLUTION_DEMO_SHOW_WINDOW,
    EVOLUTION_DEMO_PRINT_EVOLVING,
    EVOLUTION_DEMO_WAIT_EVOLVING,
    EVOLUTION_DEMO_WAIT_CRY,
    EVOLUTION_DEMO_WAIT_BGM,
    EVOLUTION_DEMO_EVOLVE,
    EVOLUTION_DEMO_EVOLVED,
    EVOLUTION_DEMO_SHOW_WINDOW_EVOLVED,
    EVOLUTION_DEMO_PRINT_EVOLVED,
    EVOLUTION_DEMO_WAIT_EVOLVED,
    EVOLUTION_DEMO_CANCELLED,
    EVOLUTION_DEMO_SHOW_WINDOW_CANCELLED,
    EVOLUTION_DEMO_PRINT_CANCELLED,
    EVOLUTION_DEMO_WAIT_CANCELLED,
    EVOLUTION_DEMO_AFTER_EVOLVING,
    EVOLUTION_DEMO_LEARN_MOVE,
    EVOLUTION_DEMO_WAIT_LEARNED,
    EVOLUTION_DEMO_PRINT_WANTS_MOVE,
    EVOLUTION_DEMO_WAIT_WANTS_MOVE,
    EVOLUTION_DEMO_WAIT_FORGET_ANSWER,
    EVOLUTION_DEMO_WAIT_FADE_OV207,
    EVOLUTION_DEMO_OV207,
    EVOLUTION_DEMO_OV207_END,
    EVOLUTION_DEMO_WAIT_FADE_OV207_END,
    EVOLUTION_DEMO_WAIT_FADE_OV287,
    EVOLUTION_DEMO_OV287,
    EVOLUTION_DEMO_WAIT_FADE_OV287_END,
    EVOLUTION_DEMO_OV287_END,
    EVOLUTION_DEMO_FORGET,
    EVOLUTION_DEMO_SHOW_WINDOW_FORGET,
    EVOLUTION_DEMO_PRINT_FORGOT,
    EVOLUTION_DEMO_WAIT_FORGOT,
    EVOLUTION_DEMO_WAIT_LEARNED_NEW,
    EVOLUTION_DEMO_GIVE_UP,
    EVOLUTION_DEMO_SHOW_WINDOW_GIVE_UP,
    EVOLUTION_DEMO_PRINT_GIVE_UP,
    EVOLUTION_DEMO_WAIT_GIVE_UP,
    EVOLUTION_DEMO_WAIT_GIVE_UP_ANSWER,
    EVOLUTION_DEMO_WAIT_NOT_LEARNED,
    EVOLUTION_DEMO_END,
    EVOLUTION_DEMO_WAIT_BGM_END,
    EVOLUTION_DEMO_WAIT_FADE_END,
    EVOLUTION_DEMO_DONE,
};

// What the music is doing
enum {
    BGM_IDLE,
    BGM_FADE_OUT,
    BGM_PLAY,
    BGM_LOAD_TRADE,
    BGM_TRADE,
    BGM_PUSHED,
    BGM_FADE_OUT_END,
    BGM_LOAD_FANFARE,
    BGM_FANFARE,
    BGM_LEVEL_UP,
};

// What to do with BG 1, the message window's, at the next VBlank
enum {
    BG1_NONE,
    BG1_SHOW,
    BG1_HIDE,
};

// The kinds of the words in a message
enum {
    WORD_STRBUF,
    WORD_SPECIES,
    WORD_NICKNAME,
    // A pointer to a u16 move
    WORD_MOVE,
    WORD_NONE,
};

typedef struct {
    u32 type;
    void *value;
} MessageWord;

typedef struct {
    HeapID heapId;
    HeapID graphicHeapId;
    EvolutionDemoGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    PartyPkm *pkm;
    // The nickname before evolving
    StrBuf *nickname;
    u16 forgetMove;
    u8 slot;
    u16 move;
    u32 learnIndex;
    BOOL cancelled;
    u32 state;
    u32 wait;
    u32 bgm;
    u32 bgmStep;
    TCB *vblankTask;
    u32 chars;
    u32 palette;
    u32 cellAnims;
    ClActor *actors[ACTOR_COUNT];
    BOOL actorsCreated;
    PrintStream *printStream;
    TCBExManager *tcbExManager;
    BmpWin *window;
    BmpWin *blankWindow;
    u32 frameChars;
    MsgData *msgData;
    MsgData *moveMsgData;
    StrBuf *message;
    KeyCursor *keyCursor;
    u32 bg1Request;
    BOOL windowShown;
    MsgData *menuMsgData;
    StrBuf *choices[2];
    TwoChoiceMenu *menu;
    TCBManager *tcbManager;
    void *tcbBuffer;
    PaletteFade *paletteFade;
    // Whether the player was using the keys rather than the touch screen, for overlay 287's screen
    u8 usingKeys;
    BPlistParam ov287Param;
    PStatusParam *pstatusParam;
    EvolutionDemoView *view;
    EvolutionDemoEffect *effect;
    BOOL unk10C;
    BOOL unk110;
    BOOL unk114;
    BOOL unk118;
    GameProcManager *procManager;
} EvolutionDemoWork;

static BOOL EvolutionDemo_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL EvolutionDemo_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL EvolutionDemo_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void EvolutionDemo_InitGraphics(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_FreeGraphics(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_InitBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_FreeBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_UpdateBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static BOOL EvolutionDemo_IsBGMFadingOut(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_RestoreBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_PlayBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static BOOL EvolutionDemo_IsBGMPlaying(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_PlayTrade(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_PushBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_PopBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static BOOL EvolutionDemo_IsTradePlaying(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_FadeOutBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static BOOL EvolutionDemo_IsBGMFadingOutEnd(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_PlayFanfare(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static BOOL EvolutionDemo_IsFanfarePlaying(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_PlayLevelUp(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_DiscardBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_InitBG(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_FreeBG(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_InitActors(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_FreeActors(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_StartActors(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static BOOL EvolutionDemo_AreActorsDone(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_VBlank(TCB *tcb, void *data);
static void EvolutionDemo_InitMsg(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_FreeMsg(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_UpdateMsg(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static BOOL EvolutionDemo_OnPrintEvent(u32 event);
static void EvolutionDemo_Print(EvolutionDemoWork *wk, MsgData *msgData, u32 messageId, PrintStreamCallback callback,
                                u32 type0, void *word0, u32 type1, void *word1);
static BOOL EvolutionDemo_IsPrintDone(EvolutionDemoWork *wk);
static BOOL EvolutionDemo_WaitButton(EvolutionDemoWork *wk);
static void EvolutionDemo_ClearWindow(EvolutionDemoWork *wk, u8 color);
static void EvolutionDemo_ShowWindow(EvolutionDemoParam *param, EvolutionDemoWork *wk, BOOL show);
static void EvolutionDemo_InitMenu(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_FreeMenu(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_SetMenuChoices(EvolutionDemoWork *wk, MsgData *msgData0, u32 messageId0, u32 type00,
                                         void *word00, u32 type10, void *word10, MsgData *msgData1, u32 messageId1,
                                         u32 type01, void *word01, u32 type11, void *word11);
static StrBuf *EvolutionDemo_FormatMessage(HeapID heapId, MsgData *msgData, u32 messageId, u32 type0, void *word0,
                                           u32 type1, void *word1);
static void EvolutionDemo_MakeShedinja(EvolutionDemoParam *param, EvolutionDemoWork *wk);
static void EvolutionDemo_RemoveHeldItem(EvolutionDemoParam *param, EvolutionDemoWork *wk);

const GameProcFunctions EVOLUTION_DEMO_PROC_FUNCTIONS = { EvolutionDemo_Init, EvolutionDemo_Main, EvolutionDemo_Exit };

static const ClActorSetup sActorSetups[ACTOR_COUNT] = {
    { 128, 96, 0, 0, 1 }, { 128, 96, 1, 0, 1 }, { 128, 96, 2, 0, 1 },
    { 128, 96, 3, 0, 1 }, { 128, 96, 4, 0, 1 }, { 128, 96, 5, 0, 1 },
};

static BOOL EvolutionDemo_Init(GameProc *proc, u32 *state, void *param, void *work) {
    EvolutionDemoParam *demoParam = param;
    EvolutionDemoWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_EVOLUTION_DEMO, 0x10000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(EvolutionDemoWork), HEAPID_EVOLUTION_DEMO);
    MI_CpuFill8(wk, 0, sizeof(EvolutionDemoWork));
    wk->heapId = HEAPID_EVOLUTION_DEMO;
    wk->state = EVOLUTION_DEMO_WAIT;
    wk->wait = 5;
    wk->bgm = BGM_IDLE;
    wk->pkm = PokeParty_GetPkm(demoParam->party, demoParam->partyIndex);
    wk->nickname = GFL_StrBufCreate(256, wk->heapId);
    PokeParty_GetParam(wk->pkm, PKM_PARAM_NICKNAME, wk->nickname);
    wk->cancelled = FALSE;
    wk->tcbManager = NULL;
    wk->tcbBuffer = NULL;
    wk->paletteFade = NULL;
    wk->usingKeys = FALSE;
    wk->pstatusParam = GFL_HeapAllocate(wk->heapId, sizeof(PStatusParam), TRUE, "shinka_demo.c", 578);
    GFL_FadeSet(3, 16, 16, -16);
    EvolutionDemo_InitBGM(param, wk);
    wk->vblankTask = GFL_VBlankTCBAdd(EvolutionDemo_VBlank, wk, 1);
    wk->unk10C = FALSE;
    wk->unk110 = FALSE;
    wk->unk114 = FALSE;
    wk->unk118 = FALSE;
    wk->procManager = CreateGameProcManager(wk->heapId);
    EvolutionDemo_InitGraphics(param, wk);
    GFL_NetSetIconScreen(0, wk->heapId);
    return TRUE;
}

static BOOL EvolutionDemo_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    EvolutionDemoWork *wk = work;

    EvolutionDemo_FreeGraphics(param, wk);
    FreeGameProcManager(wk->procManager);
    GFL_TCBRemove(wk->vblankTask);
    EvolutionDemo_FreeBGM(param, wk);
    GFL_HeapFree(wk->pstatusParam);
    GFL_StrBufFree(wk->nickname);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_EVOLUTION_DEMO);
    return TRUE;
}

static BOOL EvolutionDemo_Main(GameProc *proc, u32 *state, void *param, void *work) {
    EvolutionDemoParam *demoParam = param;
    EvolutionDemoWork *wk = work;
    BOOL procRunning;
    u32 keys;
    u32 result;
    PokeDexSave *pokedex;
    GameRecords *records;
    SaveControl *save;

    procRunning = GFL_ProcMgrUpdate(wk->procManager);
    if (procRunning == TRUE) {
        return FALSE;
    }
    if (wk->state != EVOLUTION_DEMO_OV207 && wk->state != EVOLUTION_DEMO_OV207_END && wk->printStream != NULL) {
        KeyCursor_Update(wk->keyCursor, wk->printStream, wk->window);
    }
    switch (wk->state) {
    case EVOLUTION_DEMO_WAIT:
        if (wk->wait == 0) {
            wk->state = EVOLUTION_DEMO_FADE_IN;
            GFL_FadeSet(3, 16, 0, -16);
        } else {
            wk->wait--;
        }
        break;
    case EVOLUTION_DEMO_FADE_IN:
        if (GFL_FadeIsRunning() == FALSE) {
            wk->state = EVOLUTION_DEMO_WAIT_ACTORS;
            EvolutionDemo_StartActors(param, wk);
        }
        break;
    case EVOLUTION_DEMO_WAIT_ACTORS:
        if (EvolutionDemo_IsBGMFadingOut(param, wk) == FALSE && EvolutionDemo_AreActorsDone(param, wk)) {
            if (wk->windowShown) {
                wk->state = EVOLUTION_DEMO_PRINT_EVOLVING;
            } else {
                wk->state = EVOLUTION_DEMO_SHOW_WINDOW;
                EvolutionDemo_ShowWindow(param, wk, TRUE);
            }
        }
        break;
    case EVOLUTION_DEMO_SHOW_WINDOW:
        wk->bg1Request = BG1_SHOW;
        wk->state = EVOLUTION_DEMO_PRINT_EVOLVING;
        break;
    case EVOLUTION_DEMO_PRINT_EVOLVING:
        EvolutionDemo_Print(wk, wk->msgData, 0, NULL, WORD_NICKNAME, wk->pkm, WORD_NONE, NULL);
        wk->state = EVOLUTION_DEMO_WAIT_EVOLVING;
        break;
    case EVOLUTION_DEMO_WAIT_EVOLVING:
        if (EvolutionDemo_IsPrintDone(wk) && EvolutionDemo_WaitButton(wk)) {
            EvolutionDemo_ShowWindow(param, wk, FALSE);
            wk->state = EVOLUTION_DEMO_WAIT_CRY;
            EvolutionDemoView_Start(wk->view);
        }
        break;
    case EVOLUTION_DEMO_WAIT_CRY:
        if (EvolutionDemoView_IsCryDone(wk->view)) {
            wk->state = EVOLUTION_DEMO_WAIT_BGM;
            EvolutionDemo_PlayBGM(param, wk);
        }
        break;
    case EVOLUTION_DEMO_WAIT_BGM:
        if (EvolutionDemo_IsBGMPlaying(param, wk) == FALSE) {
            wk->state = EVOLUTION_DEMO_EVOLVE;
            EvolutionDemoView_Evolve(wk->view);
            EvolutionDemoEffect_Start(wk->effect);
            GFL_SndSEPlay(SEQ_SE_SHDEMO_01);
            wk->unk10C = TRUE;
            wk->wait = 0;
        }
        break;
    case EVOLUTION_DEMO_EVOLVE:
        keys = GCTX_HIDGetPressedKeys();
        if (EvolutionDemoView_IsDone(wk->view)) {
            if (wk->cancelled) {
                wk->state = EVOLUTION_DEMO_CANCELLED;
            } else {
                wk->state = EVOLUTION_DEMO_EVOLVED;
            }
            break;
        }
        if (EvolutionDemoView_HavePiecesStarted(wk->view)) {
            EvolutionDemo_PlayTrade(param, wk);
        }
        if (EvolutionDemoView_GetUnk3C(wk->view)) {
            EvolutionDemo_PushBGM(param, wk);
        }
        if (EvolutionDemoView_HavePiecesReturned(wk->view)) {
            EvolutionDemoEffect_Start3D(wk->effect);
            if (wk->unk110 == FALSE) {
                GFL_SndSEPlay(SEQ_SE_SHDEMO_04);
                wk->unk10C = FALSE;
                wk->unk110 = TRUE;
            }
        }
        if (EvolutionDemoEffect_Is3DHeld(wk->effect)) {
            EvolutionDemoView_Reveal(wk->view);
        }
        if (EvolutionDemoEffect_Is3DReversing(wk->effect)) {
            EvolutionDemoView_FadeIn(wk->view);
            if (wk->unk114 == FALSE) {
                if (wk->cancelled == FALSE) {
                    wk->unk10C = TRUE;
                }
                wk->unk114 = TRUE;
            }
        }
        if (wk->unk10C) {
            if (wk->unk118 == FALSE && wk->wait >= 90) {
                if (EvolutionDemo_IsTradePlaying(param, wk)) {
                    GFL_SndSEPlay(SEQ_SE_SHDEMO_02);
                    wk->unk118 = TRUE;
                }
            } else if (wk->wait == 530 || wk->wait == 585 || wk->wait == 640 || wk->wait == 695) {
                GFL_SndSEPlay(SEQ_SE_SHDEMO_03);
            } else if (wk->wait == 740) {
                GFL_SndSEPlay(SEQ_SE_SHDEMO_05);
            }
            wk->wait++;
        }
        if (demoParam->canCancel && EvolutionDemo_IsTradePlaying(param, wk) && wk->cancelled == FALSE &&
            (keys & PAD_BUTTON_B) && EvolutionDemoView_Cancel(wk->view)) {
            wk->cancelled = TRUE;
            GCTX_HIDSetUsingTouch(0);
            EvolutionDemoEffect_Cancel(wk->effect);
        }
        break;
    case EVOLUTION_DEMO_EVOLVED:
        setChangedPkmSpecies(wk->pkm, demoParam->species);
        if (demoParam->gameData != NULL) {
            pokedex = GameData_GetPokedex(demoParam->gameData);
            PokeDex_RegistPkm(pokedex, wk->pkm);
            addPkmToDex(pokedex, wk->pkm);
        }
        GameBeaconSys_SendEvolution(PokeParty_GetParam(wk->pkm, PKM_PARAM_SPECIES, NULL), wk->nickname);
        EvolutionDemo_PlayFanfare(param, wk);
        if (wk->windowShown) {
            wk->state = EVOLUTION_DEMO_PRINT_EVOLVED;
        } else {
            EvolutionDemo_ShowWindow(param, wk, TRUE);
            wk->state = EVOLUTION_DEMO_SHOW_WINDOW_EVOLVED;
        }
        break;
    case EVOLUTION_DEMO_SHOW_WINDOW_EVOLVED:
        wk->bg1Request = BG1_SHOW;
        wk->state = EVOLUTION_DEMO_PRINT_EVOLVED;
        break;
    case EVOLUTION_DEMO_PRINT_EVOLVED:
        EvolutionDemo_Print(wk, wk->msgData, 2, NULL, WORD_STRBUF, wk->nickname, WORD_SPECIES, wk->pkm);
        wk->state = EVOLUTION_DEMO_WAIT_EVOLVED;
        break;
    case EVOLUTION_DEMO_WAIT_EVOLVED:
        if (EvolutionDemo_IsPrintDone(wk) && EvolutionDemo_IsFanfarePlaying(param, wk) == FALSE &&
            EvolutionDemo_WaitButton(wk)) {
            records = GameData_GetRecords(demoParam->gameData);
            RecordAddOne(records, 10);
            RecordAddOne(records, 88);
            wk->state = EVOLUTION_DEMO_AFTER_EVOLVING;
        }
        break;
    case EVOLUTION_DEMO_CANCELLED:
        if (wk->cancelled) {
            EvolutionDemo_DiscardBGM(param, wk);
        } else {
            EvolutionDemo_PopBGM(param, wk);
        }
        if (wk->windowShown) {
            wk->state = EVOLUTION_DEMO_PRINT_CANCELLED;
        } else {
            EvolutionDemo_ShowWindow(param, wk, TRUE);
            wk->state = EVOLUTION_DEMO_SHOW_WINDOW_CANCELLED;
        }
        break;
    case EVOLUTION_DEMO_SHOW_WINDOW_CANCELLED:
        wk->bg1Request = BG1_SHOW;
        wk->state = EVOLUTION_DEMO_PRINT_CANCELLED;
        break;
    case EVOLUTION_DEMO_PRINT_CANCELLED:
        EvolutionDemo_Print(wk, wk->msgData, 1, NULL, WORD_NICKNAME, wk->pkm, WORD_NONE, NULL);
        wk->state = EVOLUTION_DEMO_WAIT_CANCELLED;
        break;
    case EVOLUTION_DEMO_WAIT_CANCELLED:
        if (EvolutionDemo_IsPrintDone(wk) && EvolutionDemo_WaitButton(wk)) {
            wk->state = EVOLUTION_DEMO_END;
        }
        break;
    case EVOLUTION_DEMO_AFTER_EVOLVING:
        switch (demoParam->method) {
        case EVO_METHOD_SHEDINJA:
            EvolutionDemo_MakeShedinja(param, wk);
            break;
        case EVO_METHOD_TRADE_WITH_ITEM:
        case EVO_METHOD_HELD_ITEM_DAY:
        case EVO_METHOD_HELD_ITEM_NIGHT:
            EvolutionDemo_RemoveHeldItem(param, wk);
            break;
        }
        wk->state = EVOLUTION_DEMO_LEARN_MOVE;
        wk->learnIndex = 0;
        break;
    case EVOLUTION_DEMO_LEARN_MOVE:
        wk->move = PokeParty_LearnNextLevelMove(wk->pkm, &wk->learnIndex, wk->heapId);
        if (wk->move == 0) {
            wk->state = EVOLUTION_DEMO_END;
        } else if (wk->move != 0xfffe) {
            if (wk->move & 0x8000) {
                wk->move &= 0x7fff;
                wk->state = EVOLUTION_DEMO_PRINT_WANTS_MOVE;
            } else {
                wk->state = EVOLUTION_DEMO_WAIT_LEARNED;
                EvolutionDemo_Print(wk, wk->moveMsgData, 3, NULL, WORD_NICKNAME, wk->pkm, WORD_MOVE, &wk->move);
                EvolutionDemo_PlayLevelUp(param, wk);
            }
        }
        break;
    case EVOLUTION_DEMO_WAIT_LEARNED:
        if (EvolutionDemo_IsPrintDone(wk) && EvolutionDemo_WaitButton(wk)) {
            wk->state = EVOLUTION_DEMO_LEARN_MOVE;
        }
        break;
    case EVOLUTION_DEMO_PRINT_WANTS_MOVE:
        wk->state = EVOLUTION_DEMO_WAIT_WANTS_MOVE;
        EvolutionDemo_Print(wk, wk->moveMsgData, 4, NULL, WORD_NICKNAME, wk->pkm, WORD_MOVE, &wk->move);
        break;
    case EVOLUTION_DEMO_WAIT_WANTS_MOVE:
        if (EvolutionDemo_IsPrintDone(wk)) {
            wk->state = EVOLUTION_DEMO_WAIT_FORGET_ANSWER;
            EvolutionDemo_SetMenuChoices(wk, wk->menuMsgData, 2, WORD_NONE, NULL, WORD_NONE, NULL, wk->menuMsgData, 3,
                                         WORD_NONE, NULL, WORD_NONE, NULL);
            TwoChoiceMenu_Open(wk->menu, wk->choices[0], wk->choices[1]);
        }
        break;
    case EVOLUTION_DEMO_WAIT_FORGET_ANSWER:
        result = TwoChoiceMenu_GetResult(wk->menu);
        if (result == TWO_CHOICE_MENU_NONE) {
            break;
        }
        if (result == TWO_CHOICE_MENU_FIRST) {
            if (demoParam->unkC) {
                wk->state = EVOLUTION_DEMO_WAIT_FADE_OV207;
                GFL_FadeSet(3, 0, 16, 0);
            } else {
                wk->state = EVOLUTION_DEMO_WAIT_FADE_OV287;
                GFL_FadeSet(2, 0, 16, 0);
            }
        } else if (result == TWO_CHOICE_MENU_SECOND) {
            wk->state = EVOLUTION_DEMO_GIVE_UP;
        }
        TwoChoiceMenu_Close(wk->menu);
        break;
    case EVOLUTION_DEMO_WAIT_FADE_OV207:
        if (GFL_FadeIsRunning() == FALSE) {
            EvolutionDemo_FreeGraphics(param, wk);
            wk->state = EVOLUTION_DEMO_OV207;
            save = GameData_GetSaveControl(demoParam->gameData);
            wk->pstatusParam->dataType = PSTATUS_DATA_PARTY;
            wk->pstatusParam->gameData = demoParam->gameData;
            wk->pstatusParam->forceExit = FALSE;
            wk->pstatusParam->party = demoParam->party;
            wk->pstatusParam->trainerData = getTrainerDataBlkAddress(save);
            wk->pstatusParam->partyCount = PokeParty_GetPkmCount(demoParam->party);
            wk->pstatusParam->partyIndex = demoParam->partyIndex;
            wk->pstatusParam->move = wk->move;
            wk->pstatusParam->mode = PSTATUS_MODE_FORGET_MOVE;
            wk->pstatusParam->page = PSTATUS_PAGE_SKILL;
            wk->pstatusParam->fromFieldMenu = FALSE;
            GFL_OvlLoad(OVERLAY_PSTATUS);
            QueueGameProc(wk->procManager, OVERLAY_NONE, &PSTATUS_PROC_FUNCTIONS, wk->pstatusParam);
        }
        break;
    case EVOLUTION_DEMO_OV207:
        if (procRunning != TRUE) {
            wk->state = EVOLUTION_DEMO_OV207_END;
            break;
        }
        return FALSE;
    case EVOLUTION_DEMO_OV207_END:
        GFL_OvlUnload(OVERLAY_PSTATUS);
        wk->state = EVOLUTION_DEMO_WAIT_FADE_OV207_END;
        EvolutionDemo_InitGraphics(param, wk);
        GFL_NetSetIconScreen(0, wk->heapId);
        GFL_FadeSet(3, 16, 0, 0);
        break;
    case EVOLUTION_DEMO_WAIT_FADE_OV207_END:
        if (GFL_FadeIsRunning() == FALSE) {
            switch (wk->pstatusParam->result) {
            case 0:
                wk->slot = wk->pstatusParam->slot;
                wk->forgetMove = PokeParty_GetParam(wk->pkm, PKM_PARAM_MOVE1 + wk->slot, NULL);
                wk->state = EVOLUTION_DEMO_FORGET;
                break;
            case 1:
            case 2:
            default:
                wk->state = EVOLUTION_DEMO_GIVE_UP;
                break;
            }
        }
        break;
    case EVOLUTION_DEMO_WAIT_FADE_OV287:
        if (GFL_FadeIsRunning() == FALSE) {
            wk->state = EVOLUTION_DEMO_OV287;
            TwoChoiceMenu_Free(wk->menu);
            wk->menu = NULL;
            EvolutionDemoGraphic_FreeSubBG(wk->graphic);
            wk->usingKeys = GCTX_HIDIsUsingTouch() == FALSE;
            wk->tcbBuffer = GFL_HeapAllocate(wk->heapId, GFL_TCBMgrCalcAllocSize(8), FALSE, "shinka_demo.c", 1360);
            MI_CpuFill8(wk->tcbBuffer, 0, GFL_TCBMgrCalcAllocSize(8));
            wk->tcbManager = GFL_TCBMgrCreate(8, wk->tcbBuffer);
            wk->paletteFade = PaletteFade_Create(wk->heapId);
            PaletteFade_SetTransferAll(wk->paletteFade, TRUE);
            PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_BG, 0x1e0, wk->heapId);
            PaletteFade_AllocBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_OBJ, 0x1e0, wk->heapId);
            wk->ov287Param.gameData = demoParam->gameData;
            wk->ov287Param.party = demoParam->party;
            wk->ov287Param.font = wk->font;
            wk->ov287Param.heapId = wk->heapId;
            wk->ov287Param.unk1F = 4;
            wk->ov287Param.done = FALSE;
            wk->ov287Param.partyIndex = demoParam->partyIndex;
            wk->ov287Param.move = wk->move;
            wk->ov287Param.unk14 = 0;
            wk->ov287Param.usingKeys = &wk->usingKeys;
            wk->ov287Param.tcbManager = wk->tcbManager;
            wk->ov287Param.paletteFade = wk->paletteFade;
            wk->ov287Param.unk40 = 1;
            GFL_OvlLoad(OVERLAY_OV285);
            GFL_OvlLoad(OVERLAY_OV287);
            BPlistMain_Start(&wk->ov287Param);
            GFL_FadeSet(2, 16, 0, 0);
        }
        break;
    case EVOLUTION_DEMO_OV287:
        GFL_TCBMgrUpdate(wk->tcbManager);
        if (wk->ov287Param.done) {
            wk->state = EVOLUTION_DEMO_WAIT_FADE_OV287_END;
            GFL_FadeSet(2, 0, 16, 0);
        }
        break;
    case EVOLUTION_DEMO_WAIT_FADE_OV287_END:
        if (GFL_FadeIsRunning() == FALSE) {
            wk->state = EVOLUTION_DEMO_OV287_END;
            GFL_OvlUnload(OVERLAY_OV285);
            GFL_OvlUnload(OVERLAY_OV287);
            PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_BG);
            PaletteFade_FreeBuffer(wk->paletteFade, PALFADE_BUFFER_SUB_OBJ);
            PaletteFade_Free(wk->paletteFade);
            GFL_TCBMgrDelete(wk->tcbManager);
            GFL_HeapFree(wk->tcbBuffer);
            wk->tcbManager = NULL;
            wk->tcbBuffer = NULL;
            wk->paletteFade = NULL;
            EvolutionDemoGraphic_InitSubBG(wk->graphic);
            wk->menu = TwoChoiceMenu_Create(wk->heapId, 5, 0, 1, 0, EvolutionDemoGraphic_GetClActUnit(wk->graphic),
                                            wk->font, wk->printQueue, 0);
            GFL_NetSetIconScreen(0, wk->heapId);
            GFL_FadeSet(2, 16, 0, 0);
        }
        break;
    case EVOLUTION_DEMO_OV287_END:
        if (GFL_FadeIsRunning() == FALSE) {
            wk->slot = wk->ov287Param.slot;
            if (wk->slot == 4) {
                wk->state = EVOLUTION_DEMO_GIVE_UP;
            } else {
                wk->forgetMove = PokeParty_GetParam(wk->pkm, PKM_PARAM_MOVE1 + wk->slot, NULL);
                wk->state = EVOLUTION_DEMO_FORGET;
            }
            GCTX_HIDSetUsingTouch(wk->usingKeys ? FALSE : TRUE);
        }
        break;
    case EVOLUTION_DEMO_FORGET:
        if (wk->windowShown) {
            wk->state = EVOLUTION_DEMO_PRINT_FORGOT;
        } else {
            wk->state = EVOLUTION_DEMO_SHOW_WINDOW_FORGET;
            EvolutionDemo_ShowWindow(param, wk, TRUE);
        }
        break;
    case EVOLUTION_DEMO_SHOW_WINDOW_FORGET:
        wk->bg1Request = BG1_SHOW;
        wk->state = EVOLUTION_DEMO_PRINT_FORGOT;
        break;
    case EVOLUTION_DEMO_PRINT_FORGOT:
        EvolutionDemo_Print(wk, wk->moveMsgData, 5, EvolutionDemo_OnPrintEvent, WORD_NICKNAME, wk->pkm, WORD_MOVE,
                            &wk->forgetMove);
        wk->state = EVOLUTION_DEMO_WAIT_FORGOT;
        break;
    case EVOLUTION_DEMO_WAIT_FORGOT:
        if (EvolutionDemo_IsPrintDone(wk)) {
            PokeParty_SetMove(wk->pkm, wk->move, wk->slot);
            wk->state = EVOLUTION_DEMO_WAIT_LEARNED_NEW;
            EvolutionDemo_Print(wk, wk->moveMsgData, 6, NULL, WORD_NICKNAME, wk->pkm, WORD_MOVE, &wk->move);
            EvolutionDemo_PlayLevelUp(param, wk);
        }
        break;
    case EVOLUTION_DEMO_WAIT_LEARNED_NEW:
        if (EvolutionDemo_IsPrintDone(wk)) {
            wk->state = EVOLUTION_DEMO_LEARN_MOVE;
        }
        break;
    case EVOLUTION_DEMO_GIVE_UP:
        if (wk->windowShown) {
            wk->state = EVOLUTION_DEMO_PRINT_GIVE_UP;
        } else {
            wk->state = EVOLUTION_DEMO_SHOW_WINDOW_GIVE_UP;
            EvolutionDemo_ShowWindow(param, wk, TRUE);
        }
        break;
    case EVOLUTION_DEMO_SHOW_WINDOW_GIVE_UP:
        wk->bg1Request = BG1_SHOW;
        wk->state = EVOLUTION_DEMO_PRINT_GIVE_UP;
        break;
    case EVOLUTION_DEMO_PRINT_GIVE_UP:
        EvolutionDemo_Print(wk, wk->moveMsgData, 7, NULL, WORD_NONE, NULL, WORD_MOVE, &wk->move);
        wk->state = EVOLUTION_DEMO_WAIT_GIVE_UP;
        break;
    case EVOLUTION_DEMO_WAIT_GIVE_UP:
        if (EvolutionDemo_IsPrintDone(wk)) {
            wk->state = EVOLUTION_DEMO_WAIT_GIVE_UP_ANSWER;
            EvolutionDemo_SetMenuChoices(wk, wk->menuMsgData, 4, WORD_MOVE, &wk->move, WORD_NONE, NULL, wk->menuMsgData,
                                         5, WORD_MOVE, &wk->move, WORD_NONE, NULL);
            TwoChoiceMenu_Open(wk->menu, wk->choices[0], wk->choices[1]);
        }
        break;
    case EVOLUTION_DEMO_WAIT_GIVE_UP_ANSWER:
        result = TwoChoiceMenu_GetResult(wk->menu);
        if (result == TWO_CHOICE_MENU_NONE) {
            break;
        }
        if (result == TWO_CHOICE_MENU_FIRST) {
            wk->state = EVOLUTION_DEMO_WAIT_NOT_LEARNED;
            EvolutionDemo_Print(wk, wk->moveMsgData, 8, NULL, WORD_NICKNAME, wk->pkm, WORD_MOVE, &wk->move);
        } else if (result == TWO_CHOICE_MENU_SECOND) {
            wk->state = EVOLUTION_DEMO_PRINT_WANTS_MOVE;
        }
        TwoChoiceMenu_Close(wk->menu);
        break;
    case EVOLUTION_DEMO_WAIT_NOT_LEARNED:
        if (EvolutionDemo_IsPrintDone(wk)) {
            wk->state = EVOLUTION_DEMO_LEARN_MOVE;
        }
        break;
    case EVOLUTION_DEMO_END:
        if (wk->cancelled) {
            GFL_FadeSet(3, 0, 16, 2);
            wk->state = EVOLUTION_DEMO_WAIT_BGM_END;
        } else if (EvolutionDemo_IsTradePlaying(param, wk)) {
            EvolutionDemo_FadeOutBGM(param, wk);
            GFL_FadeSet(3, 0, 16, 2);
            wk->state = EVOLUTION_DEMO_WAIT_BGM_END;
        }
        break;
    case EVOLUTION_DEMO_WAIT_BGM_END:
        if (EvolutionDemo_IsBGMFadingOutEnd(param, wk) == FALSE) {
            wk->state = EVOLUTION_DEMO_WAIT_FADE_END;
            EvolutionDemo_RestoreBGM(param, wk);
        }
        break;
    case EVOLUTION_DEMO_WAIT_FADE_END:
        if (GFL_FadeIsRunning() == FALSE) {
            wk->state = EVOLUTION_DEMO_DONE;
            return TRUE;
        }
        break;
    case EVOLUTION_DEMO_DONE:
        return TRUE;
    }
    EvolutionDemo_UpdateBGM(param, wk);
    if (wk->state != EVOLUTION_DEMO_OV207 && wk->state != EVOLUTION_DEMO_OV207_END) {
        if (wk->menu != NULL) {
            TwoChoiceMenu_Main(wk->menu);
        }
        EvolutionDemo_UpdateMsg(param, wk);
        PrintQueue_Main(wk->printQueue);
        EvolutionDemoView_Update(wk->view);
        EvolutionDemoEffect_Update(wk->effect);
        EvolutionDemoGraphic_Update(wk->graphic);
        EvolutionDemoGraphic_Begin3D(wk->graphic);
        EvolutionDemoEffect_Draw(wk->effect);
        EvolutionDemoView_Draw(wk->view);
        EvolutionDemoGraphic_End3D(wk->graphic);
    }
    return FALSE;
}

static void EvolutionDemo_InitGraphics(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    u8 i;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_EVOLUTION_DEMO_GRAPHIC, 0x70000);
    wk->graphicHeapId = HEAPID_EVOLUTION_DEMO_GRAPHIC;
    GFL_OvlLoad(OVERLAY_APP_UI);
    for (i = 0; i <= 7; i++) {
        GFL_BGSysSetBGEnabled(i, FALSE);
    }
    wk->graphic = EvolutionDemoGraphic_Create(1, wk->graphicHeapId);
    EvolutionDemoGraphic_InitSubBG(wk->graphic);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, wk->graphicHeapId);
    wk->printQueue = PrintQueue_CreateDefault(wk->graphicHeapId);
    EvolutionDemo_InitMsg(param, wk);
    wk->menu = NULL;
    EvolutionDemo_InitMenu(param, wk);
    EvolutionDemo_InitBG(param, wk);
    wk->actorsCreated = FALSE;
    if (wk->state == EVOLUTION_DEMO_WAIT) {
        EvolutionDemo_InitActors(param, wk);
    }
    GFL_BGSysSetBGPriority(0, 2);
    GFL_BGSysSetBGPriority(2, 1);
    GFL_BGSysSetBGPriority(1, 0);
    GFL_BGSysSetBGPriority(4, 2);
    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(4, TRUE);
    GFL_BGSysResetStdPalette(0, GX_RGB(0, 0, 0));
    GFL_BGSysResetStdPalette(4, GX_RGB(0, 0, 0));
    if (wk->state == EVOLUTION_DEMO_WAIT) {
        if (param->method == EVO_METHOD_ITEM || param->method == EVO_METHOD_ITEM_MALE ||
            param->method == EVO_METHOD_ITEM_FEMALE) {
            param->canCancel = FALSE;
        }
    }
    if (wk->state == EVOLUTION_DEMO_WAIT) {
        wk->view = EvolutionDemoView_Create(wk->graphicHeapId, FALSE, wk->pkm, param->species);
    } else {
        wk->view = EvolutionDemoView_Create(wk->graphicHeapId, TRUE, wk->pkm, param->species);
    }
    if (wk->state == EVOLUTION_DEMO_WAIT) {
        wk->effect = EvolutionDemoEffect_Create(wk->graphicHeapId, FALSE);
    } else {
        wk->effect = EvolutionDemoEffect_Create(wk->graphicHeapId, TRUE);
    }
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0,
                       GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 |
                           GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD,
                       0, 0);
}

static void EvolutionDemo_FreeGraphics(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    EvolutionDemoEffect_Free(wk->effect);
    EvolutionDemoView_Free(wk->view);
    EvolutionDemo_FreeActors(param, wk);
    EvolutionDemo_FreeBG(param, wk);
    EvolutionDemo_FreeMenu(param, wk);
    EvolutionDemo_FreeMsg(param, wk);
    PrintQueue_Clear(wk->printQueue);
    PrintQueue_Free(wk->printQueue);
    GFL_FontFree(wk->font);
    EvolutionDemoGraphic_FreeSubBG(wk->graphic);
    EvolutionDemoGraphic_Free(wk->graphic);
    GFL_OvlUnload(OVERLAY_APP_UI);
    GFL_HeapDelete(HEAPID_EVOLUTION_DEMO_GRAPHIC);
}

// Fades out the music that was playing, to push it once it is quiet
static void EvolutionDemo_InitBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    if (param->unkC) {
        GFL_SndBGMFadeOut(30);
    }
    wk->bgm = BGM_FADE_OUT;
}

static void EvolutionDemo_FreeBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
}

static void EvolutionDemo_UpdateBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    switch (wk->bgm) {
    case BGM_FADE_OUT:
        if (GFL_SndBGMIsFading() == FALSE) {
            if (param->unkC) {
                GFL_SndBGMSetPaused(TRUE);
                GFL_SndBGMPush();
            }
            wk->bgm = BGM_IDLE;
        }
        break;
    case BGM_PLAY:
        if (GFL_SndBGMIsPlaying() == FALSE) {
            wk->bgm = BGM_IDLE;
        }
        break;
    case BGM_LOAD_TRADE:
        if (GFL_SndBGMLoadStep(SEQ_BGM_KOUKAN, &wk->bgmStep, FALSE)) {
            wk->bgm = BGM_TRADE;
        }
        break;
    case BGM_FADE_OUT_END:
        if (GFL_SndBGMIsFading() == FALSE) {
            GFL_SndBGMStopImmediate();
            wk->bgm = BGM_IDLE;
        }
        break;
    case BGM_LOAD_FANFARE:
        if (GFL_SndBGMLoadStep(SEQ_ME_SHINKAOME, &wk->bgmStep, FALSE)) {
            wk->bgm = BGM_FANFARE;
        }
        break;
    case BGM_FANFARE:
        if (GFL_SndBGMIsPlaying() == FALSE) {
            EvolutionDemo_PopBGM(param, wk);
        }
        break;
    case BGM_LEVEL_UP:
        if (GFL_SndBGMIsPlaying() == FALSE) {
            EvolutionDemo_PopBGM(param, wk);
        }
        break;
    }
}

static BOOL EvolutionDemo_IsBGMFadingOut(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    return wk->bgm == BGM_FADE_OUT;
}

// Brings back the music that was playing
static void EvolutionDemo_RestoreBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    if (wk->bgm == BGM_IDLE && param->unkC) {
        GFL_SndBGMPop();
        GFL_SndBGMSetPaused(FALSE);
        GFL_SndBGMFadeIn(60);
    }
}

static void EvolutionDemo_PlayBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    if (wk->bgm == BGM_IDLE) {
        GFL_SndBGMPlay(SEQ_BGM_SHINKA, SND_CHANNEL_MASK_ALL);
        wk->bgm = BGM_PLAY;
    }
}

static BOOL EvolutionDemo_IsBGMPlaying(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    return wk->bgm == BGM_PLAY;
}

static void EvolutionDemo_PlayTrade(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    if (wk->bgm == BGM_IDLE) {
        wk->bgmStep = 0;
        GFL_SndBGMLoadStep(SEQ_BGM_KOUKAN, &wk->bgmStep, TRUE);
        wk->bgm = BGM_LOAD_TRADE;
    }
}

static void EvolutionDemo_PushBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    if (wk->bgm == BGM_TRADE) {
        GFL_SndBGMSetPaused(TRUE);
        GFL_SndBGMPush();
        wk->bgm = BGM_PUSHED;
    }
}

static void EvolutionDemo_PopBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    if (wk->bgm == BGM_PUSHED || wk->bgm == BGM_FANFARE || wk->bgm == BGM_LEVEL_UP) {
        GFL_SndBGMPop();
        GFL_SndBGMSetPaused(FALSE);
        GFL_SndBGMFadeIn(6);
        wk->bgm = BGM_TRADE;
    }
}

static BOOL EvolutionDemo_IsTradePlaying(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    return wk->bgm == BGM_TRADE;
}

static void EvolutionDemo_FadeOutBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    if (wk->bgm == BGM_TRADE) {
        GFL_SndBGMFadeOut(60);
        wk->bgm = BGM_FADE_OUT_END;
    }
}

static BOOL EvolutionDemo_IsBGMFadingOutEnd(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    return wk->bgm == BGM_FADE_OUT_END;
}

static void EvolutionDemo_PlayFanfare(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    if (wk->bgm == BGM_PUSHED) {
        wk->bgmStep = 0;
        GFL_SndBGMLoadStep(SEQ_ME_SHINKAOME, &wk->bgmStep, TRUE);
        wk->bgm = BGM_LOAD_FANFARE;
    }
}

static BOOL EvolutionDemo_IsFanfarePlaying(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    return wk->bgm == BGM_LOAD_FANFARE || wk->bgm == BGM_FANFARE;
}

static void EvolutionDemo_PlayLevelUp(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    if (wk->bgm == BGM_TRADE) {
        EvolutionDemo_PushBGM(param, wk);
        GFL_SndBGMPlay(SEQ_ME_LVUP, SND_CHANNEL_MASK_ALL);
        wk->bgm = BGM_LEVEL_UP;
    }
}

// Drops the pushed music, when the evolution has been stopped
static void EvolutionDemo_DiscardBGM(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    if (wk->bgm == BGM_PUSHED) {
        GFL_SndBGMPop();
        GFL_SndBGMStopImmediate();
        wk->bgm = BGM_IDLE;
    }
}

static void EvolutionDemo_InitBG(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_EVOLUTION_DEMO, wk->graphicHeapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 3, 0, 0, 0x20, wk->graphicHeapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 5, 2, 0, 0, FALSE, wk->graphicHeapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 2, 0, 0, FALSE, wk->graphicHeapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(2);
}

static void EvolutionDemo_FreeBG(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
}

static void EvolutionDemo_InitActors(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_EVOLUTION_DEMO, wk->graphicHeapId);
    u8 i;

    wk->palette = ClActSys_LoadPlttRange(arc, 3, 0, 0, 0, 1, wk->graphicHeapId);
    wk->chars = ClActSys_LoadChars(arc, 2, 0, 0, wk->graphicHeapId);
    wk->cellAnims = ClActSys_LoadCellAnims(arc, 1, 0, wk->graphicHeapId);
    GFL_ArcToolFree(arc);
    for (i = 0; i < ACTOR_COUNT; i++) {
        wk->actors[i] = ClActor_Create(EvolutionDemoGraphic_GetClActUnit(wk->graphic), wk->chars, wk->palette,
                                       wk->cellAnims, &sActorSetups[i], 0, wk->graphicHeapId);
        ClActor_SetAutoAnim(wk->actors[i], FALSE);
    }
    wk->actorsCreated = TRUE;
}

static void EvolutionDemo_FreeActors(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    u8 i;

    if (wk->actorsCreated) {
        for (i = 0; i < ACTOR_COUNT; i++) {
            ClActor_Delete(wk->actors[i]);
        }
        ClActSys_FreeCellAnims(wk->cellAnims);
        ClActSys_FreeChars(wk->chars);
        ClActSys_FreePltt(wk->palette);
        wk->actorsCreated = FALSE;
    }
}

static void EvolutionDemo_StartActors(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    u8 i;

    if (wk->actorsCreated) {
        for (i = 0; i < ACTOR_COUNT; i++) {
            ClActor_SetAnimFrame(wk->actors[i], 0);
            ClActor_SetAutoAnim(wk->actors[i], TRUE);
        }
    }
}

// Frees the actors once they are done
static BOOL EvolutionDemo_AreActorsDone(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    BOOL done;
    u8 i;

    if (wk->actorsCreated) {
        done = TRUE;
        for (i = 0; i < ACTOR_COUNT; i++) {
            if (ClActor_IsAnimActive(wk->actors[i])) {
                done = FALSE;
                break;
            }
        }
        if (done) {
            EvolutionDemo_FreeActors(param, wk);
        }
        return done;
    }
    return TRUE;
}

static void EvolutionDemo_VBlank(TCB *tcb, void *data) {
    EvolutionDemoWork *wk = data;

    switch (wk->bg1Request) {
    case BG1_SHOW:
        GFL_BGSysSetBGEnabled(1, TRUE);
        wk->windowShown = TRUE;
        wk->bg1Request = BG1_NONE;
        break;
    case BG1_HIDE:
        GFL_BGSysSetBGEnabled(1, FALSE);
        wk->windowShown = FALSE;
        wk->bg1Request = BG1_NONE;
        break;
    }
    if (wk->paletteFade != NULL) {
        PaletteFade_Transfer(wk->paletteFade);
    }
}

static void EvolutionDemo_InitMsg(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    BmpWin *window;

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x20, 0x20, wk->graphicHeapId);
    wk->blankWindow = BmpWin_CreateDynamic(1, 0, 0, 1, 1, 1, 0);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->blankWindow), 0);
    BmpWin_FlushChar(wk->blankWindow);
    wk->window = BmpWin_CreateDynamic(1, 1, 19, 30, 4, 1, 1);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->window), 0);
    window = wk->window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
    wk->frameChars = LoadCursorImageEndOfHeap(1, 2, 0, wk->graphicHeapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVOLUTION_DEMO, wk->graphicHeapId);
    wk->moveMsgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVOLUTION_DEMO_BTL_MAIN, wk->graphicHeapId);
    wk->tcbExManager = GFL_TCBExMgrCreate(wk->graphicHeapId, wk->graphicHeapId, 1, 0);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    wk->message = NULL;
    wk->printStream = NULL;
    wk->keyCursor = KeyCursor_Create(15, TRUE, TRUE, wk->graphicHeapId);
    wk->bg1Request = BG1_NONE;
    GFL_BGSysSetBGEnabled(1, FALSE);
    wk->windowShown = FALSE;
}

static void EvolutionDemo_FreeMsg(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    KeyCursor_Free(wk->keyCursor);
    if (wk->printStream != NULL) {
        PrintStream_Delete(wk->printStream);
    }
    GFL_TCBExMgrFree(wk->tcbExManager);
    if (wk->message != NULL) {
        GFL_StrBufFree(wk->message);
    }
    GFL_MsgDataFree(wk->msgData);
    GFL_MsgDataFree(wk->moveMsgData);
    GFL_BGSysFreeCharMemory(1, CHAR_POS(wk->frameChars), CHAR_SIZE(wk->frameChars));
    BmpWin_Free(wk->window);
    BmpWin_Free(wk->blankWindow);
}

static void EvolutionDemo_UpdateMsg(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    GFL_TCBExMgrUpdate(wk->tcbExManager);
}

// Plays the sounds of the message that the Pokémon forgot a move
static BOOL EvolutionDemo_OnPrintEvent(u32 event) {
    switch (event) {
    case 3:
        GFL_SndSEPlay(SEQ_SE_KON);
        break;
    case 5:
        return GFL_SndPlayerIsActiveAny();
    }
    return FALSE;
}

// Prints a message with up to two words, WORD_*
static void EvolutionDemo_Print(EvolutionDemoWork *wk, MsgData *msgData, u32 messageId, PrintStreamCallback callback,
                                u32 type0, void *word0, u32 type1, void *word1) {
    EvolutionDemo_ClearWindow(wk, 15);
    wk->message = EvolutionDemo_FormatMessage(wk->graphicHeapId, msgData, messageId, type0, word0, type1, word1);
    wk->printStream =
        PrintStream_CreateWithCallback(wk->window, 0, 0, wk->message, wk->font, TextSpeed_GetCurrentWait(),
                                       wk->tcbExManager, 2, wk->graphicHeapId, 15, callback);
}

static BOOL EvolutionDemo_IsPrintDone(EvolutionDemoWork *wk) {
    BOOL done = FALSE;

    switch (PrintStream_GetState(wk->printStream)) {
    case PRINT_STREAM_RUNNING:
        if ((GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchHeld()) {
            PrintStream_SkipWait(wk->printStream, 0);
            if (GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                GCTX_HIDSetUsingTouch(FALSE);
            } else {
                GCTX_HIDSetUsingTouch(TRUE);
            }
        }
        break;
    case PRINT_STREAM_PAUSED:
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed()) {
            GFL_SndSEPlay(SEQ_SE_MESSAGE);
            PrintStream_Continue(wk->printStream);
            if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
                GCTX_HIDSetUsingTouch(FALSE);
            } else {
                GCTX_HIDSetUsingTouch(TRUE);
            }
        }
        break;
    case PRINT_STREAM_DONE:
        done = TRUE;
        break;
    }
    return done;
}

// Whether A or B was pressed or the touch screen touched
static BOOL EvolutionDemo_WaitButton(EvolutionDemoWork *wk) {
    if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || GFL_HIDIsTouchPressed()) {
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            GCTX_HIDSetUsingTouch(FALSE);
        } else {
            GCTX_HIDSetUsingTouch(TRUE);
        }
        return TRUE;
    }
    return FALSE;
}

static void EvolutionDemo_ClearWindow(EvolutionDemoWork *wk, u8 color) {
    BmpWin *window;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->window), color);
    window = wk->window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
    if (wk->printStream != NULL) {
        PrintStream_Delete(wk->printStream);
    }
    if (wk->message != NULL) {
        GFL_StrBufFree(wk->message);
    }
    wk->printStream = NULL;
    wk->message = NULL;
}

// Draws the message window, or hides it at the next VBlank
static void EvolutionDemo_ShowWindow(EvolutionDemoParam *param, EvolutionDemoWork *wk, BOOL show) {
    BmpWin *window;

    if (show) {
        BmpWin_DrawFrame(wk->window, WINFRAME_TRANSFER_NOW, CHAR_POS(wk->frameChars), 2);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->window), 15);
        window = wk->window;
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    } else {
        wk->bg1Request = BG1_HIDE;
    }
}

static void EvolutionDemo_InitMenu(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    wk->menuMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_YES_NO, wk->graphicHeapId);
    wk->menu = TwoChoiceMenu_Create(wk->graphicHeapId, 5, 0, 1, 0, EvolutionDemoGraphic_GetClActUnit(wk->graphic),
                                    wk->font, wk->printQueue, 0);
    wk->choices[0] = NULL;
    wk->choices[1] = NULL;
}

static void EvolutionDemo_FreeMenu(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    TwoChoiceMenu_Free(wk->menu);
    wk->menu = NULL;
    if (wk->choices[0] != NULL) {
        GFL_StrBufFree(wk->choices[0]);
    }
    if (wk->choices[1] != NULL) {
        GFL_StrBufFree(wk->choices[1]);
    }
    GFL_MsgDataFree(wk->menuMsgData);
}

static void EvolutionDemo_SetMenuChoices(EvolutionDemoWork *wk, MsgData *msgData0, u32 messageId0, u32 type00,
                                         void *word00, u32 type10, void *word10, MsgData *msgData1, u32 messageId1,
                                         u32 type01, void *word01, u32 type11, void *word11) {
    if (wk->choices[0] != NULL) {
        GFL_StrBufFree(wk->choices[0]);
    }
    if (wk->choices[1] != NULL) {
        GFL_StrBufFree(wk->choices[1]);
    }
    wk->choices[0] =
        EvolutionDemo_FormatMessage(wk->graphicHeapId, msgData0, messageId0, type00, word00, type10, word10);
    wk->choices[1] =
        EvolutionDemo_FormatMessage(wk->graphicHeapId, msgData1, messageId1, type01, word01, type11, word11);
}

static StrBuf *EvolutionDemo_FormatMessage(HeapID heapId, MsgData *msgData, u32 messageId, u32 type0, void *word0,
                                           u32 type1, void *word1) {
    StrBuf *message;
    StrBuf *format;
    WordSet *wordSet;
    MessageWord words[2];
    u32 i;

    if (type0 == WORD_NONE && type1 == WORD_NONE) {
        message = GFL_MsgDataLoadStrbufNew(msgData, messageId);
    } else {
        format = GFL_MsgDataLoadStrbufNew(msgData, messageId);
        wordSet = GFL_WordSetSystemCreateDefault(heapId);
        words[0].type = type0;
        words[0].value = word0;
        words[1].type = type1;
        words[1].value = word1;
        for (i = 0; i < NELEMS(words); i++) {
            switch (words[i].type) {
            case WORD_STRBUF:
                GFL_WordSetSetStrbuf(wordSet, i, words[i].value, 0, 1, 0);
                break;
            case WORD_SPECIES:
                setPartyPokemonSpeciesNameToStrbuf(wordSet, i, words[i].value);
                break;
            case WORD_NICKNAME:
                loadPokemonNicknameToStrbuf(wordSet, i, words[i].value);
                break;
            case WORD_MOVE:
                loadMoveNameToStrbuf(wordSet, i, *(u16 *)words[i].value);
                break;
            }
        }
        message = GFL_StrBufCreate(256, heapId);
        GFL_WordSetFormatStrbuf(wordSet, message, format);
        GFL_StrBufFree(format);
        GFL_WordSetSystemFree(wordSet);
    }
    return message;
}

// Adds a Shedinja to the party, using up a Poké Ball, if there is room in the party and a Poké Ball in the bag
static void EvolutionDemo_MakeShedinja(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    BagSave *bag = GameData_GetBag(param->gameData);
    PartyPkm *shedinja;
    StrBuf *name;
    MailData *mail;
    PokeDexSave *pokedex;
    GameRecords *records;
    int i;

    if (PokeParty_GetPkmCount(param->party) < PokeParty_GetCapacity(param->party) &&
        BagSave_CheckAmount(bag, ITEM_POKE_BALL, 1, wk->heapId)) {
        shedinja = GFL_HeapAllocate(wk->heapId, PokeParty_GetPkmRawSize(), TRUE, "shinka_demo.c", 2815);
        copyPartyPkm(wk->pkm, shedinja);
        setChangedPkmSpecies(shedinja, SPECIES_SHEDINJA);
        PokeParty_SetParam(shedinja, PKM_PARAM_POKEBALL, ITEM_POKE_BALL);
        PokeParty_SetParam(shedinja, PKM_PARAM_ITEM, ITEM_NONE);
        PokeParty_SetParam(shedinja, PKM_PARAM_MARKINGS, 0);
        for (i = PKM_PARAM_RIBBON_CHAMPION_SINNOH; i < PKM_PARAM_MOVE1; i++) {
            PokeParty_SetParam(shedinja, i, 0);
        }
        for (i = PKM_PARAM_RIBBON_G3_COOL; i < PKM_PARAM_FATEFUL_ENCOUNTER; i++) {
            PokeParty_SetParam(shedinja, i, 0);
        }
        for (i = PKM_PARAM_RIBBON_G4_COOL; i < PKM_PARAM_OT_NAME; i++) {
            PokeParty_SetParam(shedinja, i, 0);
        }
        name =
            GFL_MsgDataLoadStrbufNew(g_PMLSpeciesNamesResident, PokeParty_GetParam(shedinja, PKM_PARAM_SPECIES, NULL));
        PokeParty_SetParam(shedinja, PKM_PARAM_NICKNAME, (u32)name);
        GFL_StrBufFree(name);
        PokeParty_SetParam(shedinja, PKM_PARAM_STATUS, 0);
        mail = CreateMailData(wk->heapId);
        PokeParty_SetParam(shedinja, PKM_PARAM_MAIL, (u32)mail);
        GFL_HeapFree(mail);
        PokeParty_SetParam(shedinja, 0x9f, 0);
        PokeParty_SetParam(shedinja, PKM_PARAM_POKESTAR_FAME, 0);
        PokeParty_AddPkm(param->party, shedinja);
        BagSave_SubItem(bag, ITEM_POKE_BALL, 1, wk->heapId);
        GFL_HeapFree(shedinja);
        // Registers the Shedinja after freeing it
        pokedex = GameData_GetPokedex(param->gameData);
        PokeDex_RegistPkm(pokedex, shedinja);
        addPkmToDex(pokedex, shedinja);
        records = GameData_GetRecords(param->gameData);
        RecordAddOne(records, 10);
        RecordAddOne(records, 88);
    }
}

// The held item that the Pokémon evolved with is used up
static void EvolutionDemo_RemoveHeldItem(EvolutionDemoParam *param, EvolutionDemoWork *wk) {
    PokeParty_SetParam(wk->pkm, PKM_PARAM_ITEM, ITEM_NONE);
}
