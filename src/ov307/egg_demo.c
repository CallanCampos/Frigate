#include "types.h"
#include "app/name_entry.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/script_text_banks.h"
#include "constants/sound.h"
#include "demo/egg_demo.h"
#include "field/player_state.h"
#include "field/zone.h"
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
#include "pml/poke_party.h"
#include "save/pokedex.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/bmp_menu.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wordset.h"

// The egg hatching process: its steps, the music, the message and the question whether to give a nickname

#define ACTOR_COUNT 6

// The steps of the process
enum {
    EGG_DEMO_WAIT,
    EGG_DEMO_FADE_IN,
    EGG_DEMO_WAIT_ACTORS,
    EGG_DEMO_WAIT_BGM,
    EGG_DEMO_WAIT_KOUKAN,
    EGG_DEMO_HATCH,
    EGG_DEMO_HATCHED,
    EGG_DEMO_WAIT_WHITE,
    EGG_DEMO_REVEAL,
    EGG_DEMO_SHOW_WINDOW,
    EGG_DEMO_PRINT_HATCHED,
    EGG_DEMO_WAIT_HATCHED,
    EGG_DEMO_WAIT_QUESTION,
    EGG_DEMO_WAIT_ANSWER,
    EGG_DEMO_WAIT_FADE_NAME,
    EGG_DEMO_NAME_ENTRY,
    EGG_DEMO_WAIT_BGM_NAMED,
    EGG_DEMO_END_NAMED,
    EGG_DEMO_WAIT_BGM_END,
    EGG_DEMO_WAIT_FADE_END,
    EGG_DEMO_END,
};

// What the music is doing
enum {
    BGM_IDLE,
    BGM_FADE_OUT,
    BGM_PLAY,
    BGM_LOAD_KOUKAN,
    BGM_KOUKAN,
    BGM_LOAD_FANFARE,
    BGM_FANFARE,
    BGM_FADE_OUT_END,
};

// The answer to whether to give a nickname
enum {
    ANSWER_NO,
    ANSWER_YES,
    ANSWER_NONE,
};

typedef struct {
    HeapID heapId;
    EggDemoGraphic *graphic;
    Font *font;
    PrintQueue *printQueue;
    u32 state;
    u8 wait;
    u32 bgm;
    u32 unk1C;
    u32 dialogState;
    u32 bgmStep;
    TCB *vblankTask;
    u32 chars;
    u32 palette;
    u32 cellAnims;
    ClActor *actors[ACTOR_COUNT];
    PrintStream *printStream;
    TCBExManager *tcbManager;
    BmpWin *window;
    BmpWin *blankWindow;
    u32 frameChars;
    MsgData *msgData;
    StrBuf *message;
    // Enables BG 1 at the next VBlank
    BOOL showBG1;
    ConfirmDialogSetup dialogSetup;
    BmpMenu *dialog;
    u32 answer;
    EggDemoView *view;
    NameEntryParam *nameEntryParam;
    GameProcManager *procManager;
} EggDemoWork;

static BOOL EggDemo_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL EggDemo_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL EggDemo_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void EggDemo_VBlank(TCB *tcb, void *data);
static void EggDemo_InitGraphics(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_FreeGraphics(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_InitBG(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_FreeBG(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_InitActors(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_FreeActors(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_StartActors(EggDemoParam *param, EggDemoWork *wk);
static BOOL EggDemo_AreActorsDone(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_InitBGM(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_FreeBGM(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_UpdateBGM(EggDemoParam *param, EggDemoWork *wk);
static BOOL EggDemo_IsBGMFadingOut(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_PlayBGM(EggDemoParam *param, EggDemoWork *wk);
static BOOL EggDemo_IsBGMPlaying(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_PlayKoukan(EggDemoParam *param, EggDemoWork *wk);
static BOOL EggDemo_IsKoukanPlaying(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_PushBGM(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_PopBGM(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_PlayFanfare(EggDemoParam *param, EggDemoWork *wk);
static BOOL EggDemo_IsFanfarePlaying(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_FadeOutBGM(EggDemoParam *param, EggDemoWork *wk);
static BOOL EggDemo_IsBGMFadingOutEnd(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_RestoreBGM(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_InitMsg(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_FreeMsg(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_UpdateMsg(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_ShowWindow(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_Print(EggDemoParam *param, EggDemoWork *wk, u32 messageId);
static BOOL EggDemo_IsPrintDone(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_InitDialog(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_FreeDialog(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_UpdateDialog(EggDemoParam *param, EggDemoWork *wk);
static void EggDemo_OpenDialog(EggDemoParam *param, EggDemoWork *wk);
static u32 EggDemo_GetAnswer(EggDemoParam *param, EggDemoWork *wk);

const GameProcFunctions EGG_DEMO_PROC_FUNCTIONS = { EggDemo_Init, EggDemo_Main, EggDemo_Exit };

static const ClActorSetup sActorSetups[ACTOR_COUNT] = {
    { 128, 96, 0, 0, 1 }, { 128, 96, 1, 0, 1 }, { 128, 96, 2, 0, 1 },
    { 128, 96, 3, 0, 1 }, { 128, 96, 4, 0, 1 }, { 128, 96, 5, 0, 1 },
};

static BOOL EggDemo_Init(GameProc *proc, u32 *state, void *param, void *work) {
    EggDemoWork *wk;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_EGG_DEMO, 0x50000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(EggDemoWork), HEAPID_EGG_DEMO);
    sys_memset(wk, 0, sizeof(EggDemoWork));
    wk->heapId = HEAPID_EGG_DEMO;
    wk->state = EGG_DEMO_WAIT;
    wk->wait = 5;
    wk->bgm = BGM_IDLE;
    wk->unk1C = 0;
    wk->dialogState = 0;
    EggDemo_InitGraphics(param, wk);
    EggDemo_InitBGM(param, wk);
    GFL_FadeSet(3, 16, 16, 0);
    wk->procManager = CreateGameProcManager(wk->heapId);
    func_02042ba8(0, wk->heapId);
    return TRUE;
}

static BOOL EggDemo_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    EggDemoWork *wk = work;

    FreeGameProcManager(wk->procManager);
    EggDemo_FreeBGM(param, wk);
    if (wk->state != EGG_DEMO_END_NAMED) {
        EggDemo_FreeGraphics(param, wk);
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_EGG_DEMO);
    return TRUE;
}

static BOOL EggDemo_Main(GameProc *proc, u32 *state, void *param, void *work) {
    EggDemoParam *demoParam = param;
    EggDemoWork *wk = work;
    PlayerState *playerState;
    PlayerInfo *playerInfo;
    PokeDexSave *pokedex;
    TrainerGameInfoSave *gameInfo;
    StrBuf *name;
    StrBuf *oldName;
    BOOL procRunning;
    u32 answer;

    GCTX_HIDGetPressedKeys();
    procRunning = GFL_ProcMgrUpdate(wk->procManager);
    if (procRunning == TRUE) {
        return FALSE;
    }
    switch (wk->state) {
    case EGG_DEMO_WAIT:
        if (wk->wait == 0) {
            wk->state = EGG_DEMO_FADE_IN;
            GFL_FadeSet(3, 16, 0, -16);
        } else {
            wk->wait--;
        }
        break;
    case EGG_DEMO_FADE_IN:
        if (GFL_FadeIsRunning() == FALSE) {
            wk->state = EGG_DEMO_WAIT_ACTORS;
            EggDemo_StartActors(param, wk);
        }
        break;
    case EGG_DEMO_WAIT_ACTORS:
        if (EggDemo_IsBGMFadingOut(param, wk) == FALSE && EggDemo_AreActorsDone(param, wk)) {
            wk->state = EGG_DEMO_WAIT_BGM;
            EggDemo_PlayBGM(param, wk);
        }
        break;
    case EGG_DEMO_WAIT_BGM:
        if (EggDemo_IsBGMPlaying(param, wk) == FALSE) {
            wk->state = EGG_DEMO_WAIT_KOUKAN;
            EggDemo_PlayKoukan(param, wk);
        }
        break;
    case EGG_DEMO_WAIT_KOUKAN:
        if (EggDemo_IsKoukanPlaying(param, wk)) {
            wk->state = EGG_DEMO_HATCH;
            EggDemoView_Start(wk->view);
        }
        break;
    case EGG_DEMO_HATCH:
        if (EggDemoView_IsHatched(wk->view)) {
            wk->state = EGG_DEMO_HATCHED;
            EggDemo_PushBGM(param, wk);
        }
        break;
    case EGG_DEMO_HATCHED:
        wk->state = EGG_DEMO_WAIT_WHITE;
        playerState = GameData_GetPlayerState(demoParam->gameData);
        playerInfo = GetGameDataPlayerInfo(demoParam->gameData);
        hatchEgg(demoParam->pkm, playerInfo, ZoneData_GetPlaceNameID(PlayerState_GetZoneID(playerState)), wk->heapId);
        pokedex = GameData_GetPokedex(demoParam->gameData);
        PokeDex_RegistPkm(pokedex, demoParam->pkm);
        addPkmToDex(pokedex, demoParam->pkm);
        EggDemoView_ShowPokemon(wk->view, demoParam->pkm);
        break;
    case EGG_DEMO_WAIT_WHITE:
        if (EggDemoView_IsWhite(wk->view)) {
            wk->state = EGG_DEMO_REVEAL;
            EggDemoView_Reveal(wk->view);
        }
        break;
    case EGG_DEMO_REVEAL:
        if (EggDemoView_IsDone(wk->view)) {
            wk->state = EGG_DEMO_SHOW_WINDOW;
            EggDemo_ShowWindow(param, wk);
            EggDemo_PlayFanfare(param, wk);
        }
        break;
    case EGG_DEMO_SHOW_WINDOW:
        wk->showBG1 = TRUE;
        wk->state = EGG_DEMO_PRINT_HATCHED;
        break;
    case EGG_DEMO_PRINT_HATCHED:
        EggDemo_Print(param, wk, 1);
        wk->state = EGG_DEMO_WAIT_HATCHED;
        break;
    case EGG_DEMO_WAIT_HATCHED:
        if (EggDemo_IsPrintDone(param, wk) && EggDemo_IsFanfarePlaying(param, wk) == FALSE) {
            wk->state = EGG_DEMO_WAIT_QUESTION;
            EggDemo_Print(param, wk, 2);
        }
        break;
    case EGG_DEMO_WAIT_QUESTION:
        if (EggDemo_IsPrintDone(param, wk)) {
            wk->state = EGG_DEMO_WAIT_ANSWER;
            EggDemo_OpenDialog(param, wk);
        }
        break;
    case EGG_DEMO_WAIT_ANSWER:
        answer = EggDemo_GetAnswer(param, wk);
        if (answer == ANSWER_NO) {
            wk->state = EGG_DEMO_WAIT_BGM_END;
            GFL_FadeSet(3, 0, 16, 2);
            EggDemo_FadeOutBGM(param, wk);
            func_0203d564(0);
        } else if (answer == ANSWER_YES) {
            wk->state = EGG_DEMO_WAIT_FADE_NAME;
            GFL_FadeSet(3, 0, 16, 0);
            func_0203d564(0);
        }
        break;
    case EGG_DEMO_WAIT_FADE_NAME:
        if (GFL_FadeIsRunning() == FALSE) {
            gameInfo = getTrainerGameInfoAddress(GameData_GetSaveControl(demoParam->gameData));
            EggDemo_FreeGraphics(param, wk);
            wk->state = EGG_DEMO_NAME_ENTRY;
            wk->nameEntryParam = setupPokemonNameEntry(wk->heapId, demoParam->pkm, 10, 0, gameInfo);
            QueueGameProc(wk->procManager, OVERLAY_ID(280), &NAME_ENTRY_PROC_FUNCTIONS, wk->nameEntryParam);
        }
        break;
    case EGG_DEMO_NAME_ENTRY:
        if (procRunning != TRUE) {
            if (func_ov012_02165b0c(wk->nameEntryParam) == FALSE) {
                name = GFL_StrBufCreate(32, wk->heapId);
                oldName = GFL_StrBufCreate(32, wk->heapId);
                func_ov012_02165afc(wk->nameEntryParam, name);
                PokeParty_GetParam(demoParam->pkm, PKM_PARAM_NICKNAME, oldName);
                PokeParty_SetParam(demoParam->pkm, PKM_PARAM_NICKNAME, (u32)name);
                if (func_ov012_02165b10(wk->nameEntryParam, oldName) == FALSE) {
                    RecordAddOne(GameData_GetRecords(demoParam->gameData), 30);
                }
                GFL_StrBufFree(oldName);
                GFL_StrBufFree(name);
            }
            func_ov012_02165ae8(wk->nameEntryParam);
            wk->state = EGG_DEMO_WAIT_BGM_NAMED;
            EggDemo_FadeOutBGM(param, wk);
            break;
        }
        return FALSE;
    case EGG_DEMO_WAIT_BGM_NAMED:
        if (EggDemo_IsBGMFadingOutEnd(param, wk) == FALSE) {
            wk->state = EGG_DEMO_END_NAMED;
            EggDemo_RestoreBGM(param, wk);
            return TRUE;
        }
        break;
    case EGG_DEMO_END_NAMED:
        break;
    case EGG_DEMO_WAIT_BGM_END:
        if (EggDemo_IsBGMFadingOutEnd(param, wk) == FALSE) {
            wk->state = EGG_DEMO_WAIT_FADE_END;
            EggDemo_RestoreBGM(param, wk);
        }
        break;
    case EGG_DEMO_WAIT_FADE_END:
        if (GFL_FadeIsRunning() == FALSE) {
            wk->state = EGG_DEMO_END;
            return TRUE;
        }
        break;
    }
    EggDemo_UpdateBGM(param, wk);
    if (wk->state != EGG_DEMO_NAME_ENTRY && wk->state != EGG_DEMO_WAIT_BGM_NAMED && wk->state != EGG_DEMO_END_NAMED &&
        wk->state != EGG_DEMO_END) {
        EggDemo_UpdateMsg(param, wk);
        EggDemo_UpdateDialog(param, wk);
        EggDemoView_Update(wk->view);
        func_02021a3c(wk->printQueue);
        EggDemoGraphic_Update(wk->graphic);
        EggDemoGraphic_Begin3D(wk->graphic);
        EggDemoView_Draw(wk->view);
        EggDemoGraphic_End3D(wk->graphic);
    }
    return FALSE;
}

static void EggDemo_VBlank(TCB *tcb, void *data) {
    EggDemoWork *wk = data;

    if (wk->showBG1) {
        GFL_BGSysSetBGEnabled(1, TRUE);
        wk->showBG1 = FALSE;
    }
}

static void EggDemo_InitGraphics(EggDemoParam *param, EggDemoWork *wk) {
    u8 i;

    for (i = 0; i <= 7; i++) {
        GFL_BGSysSetBGEnabled(i, FALSE);
    }
    wk->graphic = EggDemoGraphic_Create(1, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, wk->heapId);
    wk->printQueue = func_02021998(wk->heapId);
    wk->vblankTask = GFL_VBlankTCBAdd(EggDemo_VBlank, wk, 1);
    EggDemo_InitBG(param, wk);
    EggDemo_InitActors(param, wk);
    EggDemo_InitMsg(param, wk);
    EggDemo_InitDialog(param, wk);
    GFL_BGSysSetBGPriority(0, 2);
    GFL_BGSysSetBGPriority(2, 1);
    GFL_BGSysSetBGPriority(1, 0);
    GFL_BGSysSetBGPriority(4, 0);
    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(4, TRUE);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 |
                        GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_BD, 0, 0);
    GFL_BGSysResetStdPalette(0, GX_RGB(16, 16, 16));
    GFL_BGSysResetStdPalette(4, GX_RGB(0, 0, 0));
    wk->view = EggDemoView_Create(wk->heapId, param->pkm);
}

static void EggDemo_FreeGraphics(EggDemoParam *param, EggDemoWork *wk) {
    EggDemoView_Free(wk->view);
    EggDemo_FreeDialog(param, wk);
    EggDemo_FreeMsg(param, wk);
    EggDemo_FreeActors(param, wk);
    EggDemo_FreeBG(param, wk);
    GFL_TCBRemove(wk->vblankTask);
    func_02021c44(wk->printQueue);
    func_02021a18(wk->printQueue);
    GFL_FontFree(wk->font);
    EggDemoGraphic_Free(wk->graphic);
}

static void EggDemo_InitBG(EggDemoParam *param, EggDemoWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_EGG_DEMO, wk->heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 3, 0, 0, 0x20, wk->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 5, 2, 0, 0, FALSE, wk->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 2, 0, 0, FALSE, wk->heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysLoadScr(2);
}

static void EggDemo_FreeBG(EggDemoParam *param, EggDemoWork *wk) {
}

static void EggDemo_InitActors(EggDemoParam *param, EggDemoWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_EGG_DEMO, wk->heapId);
    u8 i;

    wk->palette = func_0204bbb8(arc, 3, 0, 0, 0, 1, wk->heapId);
    wk->chars = func_0204b81c(arc, 2, 0, 0, wk->heapId);
    wk->cellAnims = func_0204bde0(arc, 1, 0, wk->heapId);
    GFL_ArcToolFree(arc);
    for (i = 0; i < ACTOR_COUNT; i++) {
        wk->actors[i] = func_0204c040(EggDemoGraphic_GetClActUnit(wk->graphic), wk->chars, wk->palette, wk->cellAnims,
                                      &sActorSetups[i], 0, wk->heapId);
        func_0204c520(wk->actors[i], FALSE);
    }
}

static void EggDemo_FreeActors(EggDemoParam *param, EggDemoWork *wk) {
    u8 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        func_0204c108(wk->actors[i]);
    }
    func_0204be64(wk->cellAnims);
    func_0204b98c(wk->chars);
    func_0204bcd0(wk->palette);
}

static void EggDemo_StartActors(EggDemoParam *param, EggDemoWork *wk) {
    u8 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        func_0204c504(wk->actors[i], 0);
        func_0204c520(wk->actors[i], TRUE);
    }
}

static BOOL EggDemo_AreActorsDone(EggDemoParam *param, EggDemoWork *wk) {
    BOOL done = TRUE;
    u8 i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        if (func_0204c560(wk->actors[i])) {
            done = FALSE;
            break;
        }
    }
    return done;
}

// Fades out the field's music, to push it once it is quiet
static void EggDemo_InitBGM(EggDemoParam *param, EggDemoWork *wk) {
    GFL_SndBGMFadeOut(30);
    wk->bgm = BGM_FADE_OUT;
}

static void EggDemo_FreeBGM(EggDemoParam *param, EggDemoWork *wk) {
}

static void EggDemo_UpdateBGM(EggDemoParam *param, EggDemoWork *wk) {
    switch (wk->bgm) {
    case BGM_FADE_OUT:
        if (GFL_SndBGMIsFading() == FALSE) {
            GFL_SndBGMSetPaused(TRUE);
            GFL_SndBGMPush();
            wk->bgm = BGM_IDLE;
        }
        break;
    case BGM_PLAY:
        if (GFL_SndBGMIsPlaying() == FALSE) {
            wk->bgm = BGM_IDLE;
        }
        break;
    case BGM_LOAD_KOUKAN:
        if (func_02006424(SEQ_BGM_KOUKAN, &wk->bgmStep, FALSE)) {
            wk->bgm = BGM_KOUKAN;
        }
        break;
    case BGM_LOAD_FANFARE:
        if (func_02006424(SEQ_ME_SHINKAOME, &wk->bgmStep, FALSE)) {
            wk->bgm = BGM_FANFARE;
        }
        break;
    case BGM_FANFARE:
        if (GFL_SndBGMIsPlaying() == FALSE) {
            EggDemo_PopBGM(param, wk);
            wk->bgm = BGM_KOUKAN;
        }
        break;
    case BGM_FADE_OUT_END:
        if (GFL_SndBGMIsFading() == FALSE) {
            func_02005d8c();
            wk->bgm = BGM_IDLE;
        }
        break;
    }
}

static BOOL EggDemo_IsBGMFadingOut(EggDemoParam *param, EggDemoWork *wk) {
    return wk->bgm == BGM_FADE_OUT;
}

static void EggDemo_PlayBGM(EggDemoParam *param, EggDemoWork *wk) {
    GFL_SndBGMPlay(SEQ_BGM_SHINKA, SND_CHANNEL_MASK_ALL);
    wk->bgm = BGM_PLAY;
}

static BOOL EggDemo_IsBGMPlaying(EggDemoParam *param, EggDemoWork *wk) {
    return wk->bgm == BGM_PLAY;
}

static void EggDemo_PlayKoukan(EggDemoParam *param, EggDemoWork *wk) {
    wk->bgmStep = 0;
    func_02006424(SEQ_BGM_KOUKAN, &wk->bgmStep, TRUE);
    wk->bgm = BGM_LOAD_KOUKAN;
}

static BOOL EggDemo_IsKoukanPlaying(EggDemoParam *param, EggDemoWork *wk) {
    return wk->bgm == BGM_KOUKAN;
}

static void EggDemo_PushBGM(EggDemoParam *param, EggDemoWork *wk) {
    GFL_SndBGMSetPaused(TRUE);
    GFL_SndBGMPush();
}

static void EggDemo_PopBGM(EggDemoParam *param, EggDemoWork *wk) {
    GFL_SndBGMPop();
    GFL_SndBGMSetPaused(FALSE);
    GFL_SndBGMFadeIn(6);
}

static void EggDemo_PlayFanfare(EggDemoParam *param, EggDemoWork *wk) {
    wk->bgmStep = 0;
    func_02006424(SEQ_ME_SHINKAOME, &wk->bgmStep, TRUE);
    wk->bgm = BGM_LOAD_FANFARE;
}

static BOOL EggDemo_IsFanfarePlaying(EggDemoParam *param, EggDemoWork *wk) {
    return wk->bgm == BGM_LOAD_FANFARE || wk->bgm == BGM_FANFARE;
}

static void EggDemo_FadeOutBGM(EggDemoParam *param, EggDemoWork *wk) {
    GFL_SndBGMFadeOut(60);
    wk->bgm = BGM_FADE_OUT_END;
}

static BOOL EggDemo_IsBGMFadingOutEnd(EggDemoParam *param, EggDemoWork *wk) {
    return wk->bgm == BGM_FADE_OUT_END;
}

// Brings back the field's music
static void EggDemo_RestoreBGM(EggDemoParam *param, EggDemoWork *wk) {
    GFL_SndBGMPop();
    GFL_SndBGMSetPaused(FALSE);
    GFL_SndBGMFadeIn(60);
}

static void EggDemo_InitMsg(EggDemoParam *param, EggDemoWork *wk) {
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x20, 0x20, wk->heapId);
    wk->blankWindow = BmpWin_CreateDynamic(1, 0, 0, 1, 1, 1, 0);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->blankWindow), 0);
    BmpWin_FlushChar(wk->blankWindow);
    wk->window = BmpWin_CreateDynamic(1, 1, 19, 30, 4, 1, 0);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->window), 15);
    BmpWin_FlushChar(wk->window);
    wk->frameChars = LoadCursorImageEndOfHeap(1, 2, 0, wk->heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_2250, wk->heapId);
    wk->tcbManager = GFL_TCBExMgrCreate(wk->heapId, wk->heapId, 1, 0);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    GFL_BGSysQueueScrLoad(1);
    wk->printStream = NULL;
    wk->message = NULL;
    wk->showBG1 = FALSE;
    GFL_BGSysSetBGEnabled(1, FALSE);
}

static void EggDemo_FreeMsg(EggDemoParam *param, EggDemoWork *wk) {
    if (wk->printStream != NULL) {
        func_020223cc(wk->printStream);
    }
    if (wk->message != NULL) {
        GFL_StrBufFree(wk->message);
    }
    GFL_TCBExMgrFree(wk->tcbManager);
    GFL_MsgDataFree(wk->msgData);
    GFL_BGSysFreeCharMemory(1, CHAR_POS(wk->frameChars), CHAR_SIZE(wk->frameChars));
    BmpWin_Free(wk->window);
    BmpWin_Free(wk->blankWindow);
}

static void EggDemo_UpdateMsg(EggDemoParam *param, EggDemoWork *wk) {
    GFL_TCBExMgrUpdate(wk->tcbManager);
}

static void EggDemo_ShowWindow(EggDemoParam *param, EggDemoWork *wk) {
    BmpWin *window;

    BmpWin_DrawFrame(wk->window, WINFRAME_TRANSFER_NOW, CHAR_POS(wk->frameChars), 2);
    window = wk->window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
}

// Prints a message with the Pokémon's species name
static void EggDemo_Print(EggDemoParam *param, EggDemoWork *wk, u32 messageId) {
    StrBuf *format;
    WordSet *wordSet;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->window), 15);
    if (wk->printStream != NULL) {
        func_020223cc(wk->printStream);
    }
    if (wk->message != NULL) {
        GFL_StrBufFree(wk->message);
    }
    format = GFL_MsgDataLoadStrbufNew(wk->msgData, messageId);
    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    setPartyPokemonSpeciesNameToStrbuf(wordSet, 0, param->pkm);
    wk->message = GFL_StrBufCreate(256, wk->heapId);
    GFL_WordSetFormatStrbuf(wordSet, wk->message, format);
    GFL_WordSetSystemFree(wordSet);
    GFL_StrBufFree(format);
    wk->printStream = func_02022268(wk->window, 0, 0, wk->message, wk->font, func_02017bcc(), wk->tcbManager, 2,
                                    wk->heapId, 15);
}

static BOOL EggDemo_IsPrintDone(EggDemoParam *param, EggDemoWork *wk) {
    BOOL done = FALSE;

    switch (func_020223b4(wk->printStream)) {
    case PRINT_STREAM_RUNNING:
        if ((GCTX_HIDGetHeldKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da2c()) {
            func_020223e0(wk->printStream, 0);
        }
        break;
    case PRINT_STREAM_PAUSED:
        if ((GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) || func_0203da48()) {
            func_020223bc(wk->printStream);
        }
        break;
    case PRINT_STREAM_DONE:
        done = TRUE;
        break;
    }
    return done;
}

// The yes/no dialog goes where some characters are free
static void EggDemo_InitDialog(EggDemoParam *param, EggDemoWork *wk) {
    u32 pos = GFL_BGSysAllocChar(1, 0x500, 1);

    GFL_BGSysFreeCharMemory(1, pos, 0x500);
    wk->dialogSetup.bg = 1;
    wk->dialogSetup.x = 24;
    wk->dialogSetup.y = 13;
    wk->dialogSetup.palette = 1;
    wk->dialogSetup.unk4 = pos;
    wk->answer = ANSWER_NONE;
}

static void EggDemo_FreeDialog(EggDemoParam *param, EggDemoWork *wk) {
}

static void EggDemo_UpdateDialog(EggDemoParam *param, EggDemoWork *wk) {
    u32 result;

    switch (wk->dialogState) {
    case 0:
        break;
    case 1:
        result = ConfirmDialog_Update(wk->dialog);
        if (result != BMPMENU_NULL) {
            wk->answer = result == 0 ? ANSWER_YES : ANSWER_NO;
            wk->dialogState = 2;
        }
        break;
    }
}

static void EggDemo_OpenDialog(EggDemoParam *param, EggDemoWork *wk) {
    wk->dialog = ShopUI_CreateConfirmDialog(&wk->dialogSetup, CHAR_POS(wk->frameChars), 2, 0, wk->heapId);
    wk->answer = ANSWER_NONE;
    wk->dialogState = 1;
}

static u32 EggDemo_GetAnswer(EggDemoParam *param, EggDemoWork *wk) {
    return wk->answer;
}
