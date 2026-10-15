#include "types.h"
#include "app/badge_view.h"
#include "app/trainer_card.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/trainer_classes.h"
#include "field/trcard_sys.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/rtc_cache.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/rtc.h"
#include "nnsys/g2d.h"
#include "nnsys/gfd.h"
#include "pml/poke_graphic.h"
#include "save/timesig_savedata.h"
#include "save/trainer_card.h"
#include "system/app_menu_common.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"

// The Gym Badge case of the trainer card (badge_view.c, from the ROM's string)

#define BADGE_COUNT 8

// The actors
enum {
    BADGE_ACTOR_CARD,
    BADGE_ACTOR_RETURN,
    BADGE_ACTOR_EXIT,
    BADGE_ACTOR_SHORTCUT,
    BADGE_ACTOR_BADGE,
    BADGE_ACTOR_SPARKLE = BADGE_ACTOR_BADGE + BADGE_COUNT,
    BADGE_ACTOR_LEADER = BADGE_ACTOR_SPARKLE + BADGE_COUNT * 3,
    BADGE_ACTOR_COUNT = BADGE_ACTOR_LEADER + BADGE_COUNT,
};

// The actors' resources: the case's own and the touch bar's. The actor table's indices go on into the Gym Leaders'
enum {
    BADGE_RES_PALETTE,
    BADGE_RES_CHARS,
    BADGE_RES_CELLS,
    BADGE_RES_BAR_PALETTE,
    BADGE_RES_BAR_CHARS,
    BADGE_RES_BAR_CELLS,
    BADGE_RES_COUNT,
};

// What is touched: the badges, the Gym Leaders above them and the buttons
enum {
    BADGE_HIT_BADGE,
    BADGE_HIT_LEADER = BADGE_HIT_BADGE + BADGE_COUNT,
    BADGE_HIT_CARD = BADGE_HIT_LEADER + BADGE_COUNT,
    BADGE_HIT_SHORTCUT,
    BADGE_HIT_EXIT,
    BADGE_HIT_RETURN,
};

// The proc's states
enum {
    BADGE_SEQ_FADE_IN,
    BADGE_SEQ_MAIN,
    BADGE_SEQ_BUTTON,
    BADGE_SEQ_FADE_OUT_START,
    BADGE_SEQ_FADE_OUT,
};

// What the message window shows
enum {
    BADGE_MODE_NONE,
    BADGE_MODE_BADGE,
    BADGE_MODE_LEADER,
};

// The steps of the message window's slide: in from 100, out from 200
enum {
    BADGE_MOVE_NONE = 0,
    BADGE_MOVE_IN_START = 100,
    BADGE_MOVE_IN = 110,
    BADGE_MOVE_IN_END = 120,
    BADGE_MOVE_OUT_START = 200,
    BADGE_MOVE_OUT = 210,
    BADGE_MOVE_OUT_END = 220,
    BADGE_MOVE_DONE = 10000,
};

#define BADGE_FLAG_MOVING 0x1
#define BADGE_FLAG_MOVE_END 0x2
#define BADGE_FLAG_MODEL 0x4
#define BADGE_FLAG_PALETTE_ANIME 0x8
// From bit 4, the buttons that are off
#define BADGE_FLAG_BUTTON_OFF_SHIFT 4

#define BADGE_PRINT_COLOR 0x3c40

typedef struct {
    u32 palette;
    u32 chars;
    u32 cellAnims;
} BadgeViewLeaderRes;

// A value that moves from one end to the other in a number of frames, in hundredths
typedef struct {
    s16 from;
    s16 to;
    s16 flags;
    int step;
    int value;
} BadgeViewValue;

typedef struct {
    ArcTool *arc;
    Font *font;
    BmpWin *win;
    PrintWindow printWin;
    PrintQueue *queue;
    u8 unk18[8];
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *expandBuf;
    StrBuf *strs[7];
    ClActUnit *unit;
    ClActor *actors[BADGE_ACTOR_COUNT];
    u32 res[BADGE_RES_COUNT];
    BadgeViewLeaderRes leaderRes[BADGE_COUNT];
    TCB *vblankTask;
    G3DManager *g3dMgr;
    u8 unk17C[4];
    G3DLight *lights;
    G3DCamera *camera;
    TrainerCardParam *param;
    int seq;
    // The button that ends the screen, BADGE_HIT_*
    int result;
    u16 page;
    u16 pageMax;
    int badgeCount;
    int frame;
    u8 unk1A0[0x24];
    int unk1C4;
    u8 unk1C8[4];
    u32 mode;
    int moveSeq;
    u32 flags;
    // The badge whose message shows, and whether it is the badge's (0) or the Gym Leader's (1)
    int selected;
    int kind;
    s16 scrollY;
    s16 unk1E2;
    // Which of the Gym Leader's two messages shows
    s16 msgPage;
    int timers[2];
    BadgeViewValue values[4];
    void *palRes;
    NNSG2dPaletteData *palData;
    void *buffer;
    SRTMatrix srt;
    int angle;
    int unk27C;
    int unk280;
    u16 scenes[BADGE_COUNT];
    int palAngle;
    u16 palette[2][16];
    u16 paletteOut[16];
    u8 hasBadge[BADGE_COUNT];
    TimeSigSave *timeSig;
    int polish[BADGE_COUNT];
    u8 unk324[8];
    u8 unk32C[BADGE_COUNT];
    u8 unk334[0x10];
} BadgeViewWork;

// An actor: its position, its animation, its resources as indices into the work's, and its surface
typedef struct {
    int x;
    int y;
    int sequence;
    int charRes;
    int paletteRes;
    int cellRes;
    int bgPriority;
    int surface;
} BadgeViewActorDef;

// Where one of the message window's slides starts and ends, the step of BadgeView_UpdateMove that starts it, and
// how many frames it takes
typedef struct {
    s16 from;
    s16 to;
    s16 moveSeq;
    s16 frames;
} BadgeViewMove;

typedef struct {
    u32 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u32 palette;
} BadgeViewWinDef;

typedef BOOL (*BadgeViewSeqFunc)(BadgeViewWork *wk);

static BOOL BadgeView_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BadgeView_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL BadgeView_Exit(GameProc *proc, u32 *state, void *param, void *work);
static void BadgeView_VBlank(TCB *tcb, void *data);
static void BadgeView_InitWork(BadgeViewWork *wk, TrainerCardParam *param);
static void BadgeView_FreeWork(BadgeViewWork *wk);
static void BadgeView_InitVRAM(void);
static void BadgeView_InitBG(BadgeViewWork *wk);
static void BadgeView_ExitBG(BadgeViewWork *wk);
static void BadgeView_LoadBG(BadgeViewWork *wk);
static void BadgeView_FreePalette(BadgeViewWork *wk);
static void BadgeView_InitObj(BadgeViewWork *wk);
static void BadgeView_AddActors(BadgeViewWork *wk);
static void BadgeView_ExitObj(BadgeViewWork *wk);
static void BadgeView_Init3D(BadgeViewWork *wk);
static void BadgeView_Exit3D(BadgeViewWork *wk);
static void BadgeView_InitScenes(BadgeViewWork *wk);
static void BadgeView_ExitScenes(BadgeViewWork *wk);
static void BadgeView_Draw3D(BadgeViewWork *wk);
static void BadgeView_InitBmp(BadgeViewWork *wk);
static void BadgeView_ExitBmp(BadgeViewWork *wk);
static void BadgeView_SetMessageVisible(BadgeViewWork *wk, int visible);
static void BadgeView_FlushWin(BadgeViewWork *wk);
static void BadgeView_InitPrint(BadgeViewWork *wk);
static void BadgeView_ExitPrint(BadgeViewWork *wk);
static void BadgeView_UpdatePrint(BadgeViewWork *wk);
static BOOL BadgeView_RunSeq(BadgeViewWork *wk);
static BOOL BadgeView_SeqFadeIn(BadgeViewWork *wk);
static BOOL BadgeView_SeqMain(BadgeViewWork *wk);
static BOOL BadgeView_SeqButton(BadgeViewWork *wk);
static BOOL BadgeView_SeqFadeOutStart(BadgeViewWork *wk);
static BOOL BadgeView_SeqFadeOut(BadgeViewWork *wk);
static void BadgeView_PrintLeaderLine(BadgeViewWork *wk, int message, int line, int badge, StrBuf *strbuf);
static void BadgeView_DrawMessage(BadgeViewWork *wk, int kind, int badge, int page);
static int BadgeView_Input(BadgeViewWork *wk);
static void BadgeView_HandleHit(BadgeViewWork *wk, int hit, BOOL key);
static void BadgeView_ToggleShortcut(BadgeViewWork *wk);
static void BadgeView_InitPages(BadgeViewWork *wk);
static void BadgeView_SetSparklesVisible(BadgeViewWork *wk, int badge, BOOL visible);
static void BadgeView_InitBadges(BadgeViewWork *wk, int page, int pageMax);
static void BadgeView_SetPage(BadgeViewWork *wk, int page);
static void BadgeView_SavePolish(BadgeViewWork *wk);
static void BadgeView_LoadPolish(BadgeViewWork *wk);
static void BadgeView_StartValue(BadgeViewWork *wk, int index, s16 from, s16 to, s16 frames);
static int BadgeView_UpdateValue(BadgeViewWork *wk, int index);
static BOOL BadgeView_IsValueMoving(BadgeViewWork *wk, int index);
static void BadgeView_Update(BadgeViewWork *wk);
static void BadgeView_StartMove(BadgeViewWork *wk, int move);
static void BadgeView_UpdateMove(BadgeViewWork *wk);
static BOOL BadgeView_IsMoving(BadgeViewWork *wk);
static void BadgeView_SetActorVisible(BadgeViewWork *wk, int actor, BOOL visible);
static void BadgeView_SetButtonActive(BadgeViewWork *wk, int button, BOOL active);
static void BadgeView_SetTimer(BadgeViewWork *wk, int index, int frames);
static int BadgeView_TickTimer(BadgeViewWork *wk, int index);
static void BadgeView_UpdateLeader(BadgeViewWork *wk);
static void BadgeView_UpdateBadge(BadgeViewWork *wk);
static void BadgeView_PlayBadgeSE(BadgeViewWork *wk, int badge);
static void BadgeView_LoadBadgePalette(BadgeViewWork *wk, int badge);
static void BadgeView_LoadBadgePalettes(BadgeViewWork *wk);
static void BadgeView_Stub(BadgeViewWork *wk, int badge);
static void BadgeView_SelectLeader(BadgeViewWork *wk, int show, int badge);
static void BadgeView_UpdateRotation(BadgeViewWork *wk);
static void BadgeView_SetModelVisible(BadgeViewWork *wk, int visible);
static void BadgeView_StartPaletteAnime(BadgeViewWork *wk);
static void BadgeView_AnimatePalette(BadgeViewWork *wk);
static void BadgeView_SetPaletteAnime(BadgeViewWork *wk, int on);
static BOOL BadgeView_IsPaletteAnime(BadgeViewWork *wk);
static void BadgeView_LoadLeaders(BadgeViewWork *wk, u32 vramType);
static void BadgeView_FreeLeaders(BadgeViewWork *wk);
static void BadgeView_SetActorX(BadgeViewWork *wk, int actor, int x);
static void BadgeView_SetLeadersX(BadgeViewWork *wk, int x);

// The file's tables are declared where they are, here and between the functions below, for the order of its
// .rodata: MWCC's sort of them by size depends on where each one is declared

static const u32 sBadgeViewBGs[] = { 1, 2, 3, 4, 5, 6, 7 };

// The models of the badges, sBadgeViewModel0 to sBadgeViewModel7
static const G3DSceneResourceSetup sBadgeViewModel0 = { ARCID_TRAINER_CARD_GRA, 56, 0, G3D_SCENE_RES_ARCSYS };

// Where the Gym Leaders' pictures come from and go to as the message window slides in and out, an unused value, and
// in how many frames
static const s16 sBadgeViewLeaderMoves[2 * 4] = {
    256, 192, 0, 16, // in
    192, 256, 0, 16, // out
};

static const BGSysVRAMConfig sBadgeViewVRAMConfig = {
    GX_VRAM_BG_128_B, GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,      GX_VRAM_I,
    GX_VRAM_TEX_0_A,  GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

static const BadgeViewActorDef sBadgeViewActorDefs[BADGE_ACTOR_COUNT] = {
    { 4, 168, 11, 1, 0, 2, 0, 1 },    { 224, 168, 1, 4, 4, 4, 0, 1 },   { 192, 168, 0, 4, 4, 4, 0, 1 },
    { 160, 172, 7, 4, 4, 4, 0, 1 },   { 16, 128, 0, 1, 0, 2, 3, 1 },    { 48, 128, 1, 1, 0, 2, 3, 1 },
    { 80, 128, 2, 1, 0, 2, 3, 1 },    { 112, 128, 3, 1, 0, 2, 3, 1 },   { 144, 128, 4, 1, 0, 2, 3, 1 },
    { 176, 128, 5, 1, 0, 2, 3, 1 },   { 208, 128, 6, 1, 0, 2, 3, 1 },   { 240, 128, 7, 1, 0, 2, 3, 1 },
    { 16, 112, 9, 1, 0, 2, 2, 1 },    { 32, 136, 10, 1, 0, 2, 2, 1 },   { 24, 152, 8, 1, 0, 2, 2, 1 },
    { 48, 112, 9, 1, 0, 2, 2, 1 },    { 64, 136, 10, 1, 0, 2, 2, 1 },   { 56, 152, 8, 1, 0, 2, 2, 1 },
    { 80, 112, 9, 1, 0, 2, 2, 1 },    { 96, 136, 10, 1, 0, 2, 2, 1 },   { 88, 152, 8, 1, 0, 2, 2, 1 },
    { 112, 112, 9, 1, 0, 2, 2, 1 },   { 128, 136, 10, 1, 0, 2, 2, 1 },  { 120, 152, 8, 1, 0, 2, 2, 1 },
    { 144, 112, 9, 1, 0, 2, 2, 1 },   { 160, 136, 10, 1, 0, 2, 2, 1 },  { 152, 152, 8, 1, 0, 2, 2, 1 },
    { 176, 112, 9, 1, 0, 2, 2, 1 },   { 192, 136, 10, 1, 0, 2, 2, 1 },  { 184, 152, 8, 1, 0, 2, 2, 1 },
    { 208, 112, 9, 1, 0, 2, 2, 1 },   { 224, 136, 10, 1, 0, 2, 2, 1 },  { 216, 152, 8, 1, 0, 2, 2, 1 },
    { 240, 112, 9, 1, 0, 2, 2, 1 },   { 256, 136, 10, 1, 0, 2, 2, 1 },  { 248, 152, 8, 1, 0, 2, 2, 1 },
    { 192, 96, 0, 7, 6, 8, 1, 0 },    { 192, 96, 0, 10, 9, 11, 1, 0 },  { 192, 96, 0, 13, 12, 14, 1, 0 },
    { 192, 96, 0, 16, 15, 17, 1, 0 }, { 192, 96, 0, 19, 18, 20, 1, 0 }, { 192, 96, 0, 22, 21, 23, 1, 0 },
    { 192, 96, 0, 25, 24, 26, 1, 0 }, { 192, 96, 0, 28, 27, 29, 1, 0 },
};

static BOOL BadgeView_Init(GameProc *proc, u32 *state, void *param, void *work) {
    BadgeViewWork *wk;

    GFL_OvlLoad(OVERLAY_ID(147));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_TRAINER_CARD_SCREEN, 0x28000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(BadgeViewWork), HEAPID_TRAINER_CARD_SCREEN);
    sys_memset32_fast(0, wk, sizeof(BadgeViewWork));
    BadgeView_InitWork(wk, param);
    BadgeView_InitVRAM();
    BadgeView_InitBG(wk);
    BadgeView_LoadBG(wk);
    BadgeView_InitObj(wk);
    BadgeView_AddActors(wk);
    BadgeView_Init3D(wk);
    BadgeView_InitScenes(wk);
    BadgeView_InitBmp(wk);
    BadgeView_InitPrint(wk);
    BadgeView_SetPage(wk, wk->page);
    func_02042ba8(TRUE, HEAPID_TRAINER_CARD_SCREEN);
    GFL_WipeSet(0, 1, 1, 0, 6, 1, HEAPID_TRAINER_CARD_SCREEN);
    return TRUE;
}

static BOOL BadgeView_Main(GameProc *proc, u32 *state, void *param, void *work) {
    BadgeViewWork *wk = work;

    if (!BadgeView_RunSeq(wk)) {
        return TRUE;
    }
    BadgeView_Draw3D(wk);
    BadgeView_UpdatePrint(wk);
    func_0204b794();
    wk->frame++;
    if (wk->frame >= 0x100) {
        wk->frame = 0;
    }
    return FALSE;
}

static BOOL BadgeView_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    BadgeViewWork *wk = work;

    BadgeView_ExitPrint(wk);
    BadgeView_ExitBmp(wk);
    BadgeView_ExitScenes(wk);
    BadgeView_Exit3D(wk);
    BadgeView_ExitObj(wk);
    BadgeView_FreePalette(wk);
    BadgeView_ExitBG(wk);
    BadgeView_FreeWork(wk);
    BadgeView_SavePolish(wk);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapStatusValidate(HEAPID_TRAINER_CARD_SCREEN);
    GFL_HeapDelete(HEAPID_TRAINER_CARD_SCREEN);
    GFL_OvlUnload(OVERLAY_ID(147));
    return TRUE;
}

static void BadgeView_VBlank(TCB *tcb, void *data) {
    BadgeViewWork *wk = data;
    int y = -(wk->frame / 2);

    GFL_BGSysMoveBG(3, BG_MOVE_SET_X, y);
    GFL_BGSysMoveBG(3, BG_MOVE_SET_Y, y);
    if (wk->flags & BADGE_FLAG_MOVING) {
        GFL_BGSysMoveBG(7, BG_MOVE_SET_Y, wk->scrollY);
        GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, wk->scrollY);
        if (wk->flags & BADGE_FLAG_MOVE_END) {
            wk->flags &= ~BADGE_FLAG_MOVING;
            wk->flags &= ~BADGE_FLAG_MOVE_END;
        }
    }
    func_0204b7c8();
    GFL_BGSysUpdate();
}

static void BadgeView_InitWork(BadgeViewWork *wk, TrainerCardParam *param) {
    int i;

    wk->param = param;
    wk->result = 0;
    wk->arc = GFL_ArcSysCreateFileHandle(ARCID_TRAINER_CARD_GRA, HEAPID_TRAINER_CARD_SCREEN);
    wk->msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_SERVER_FLOW_4, HEAPID_TRAINER_CARD_SCREEN);
    for (i = 0; i < 7; i++) {
        wk->strs[i] = GFL_MsgDataLoadStrbufNew(wk->msgData, i);
        GFL_StrBufGetStringPtr(wk->strs[i]);
    }
    wk->expandBuf = GFL_StrBufCreate(104, HEAPID_TRAINER_CARD_SCREEN);
    wk->wordSet = GFL_WordSetSystemCreate(8, 64, HEAPID_TRAINER_CARD_SCREEN);
    wk->buffer = GFL_HeapAllocate(HEAPID_TRAINER_CARD_SCREEN, 0x600, FALSE, "badge_view.c", 770);
    sys_memset(wk->buffer, 0, 0x600);
    wk->vblankTask = GFL_VBlankTCBAdd(BadgeView_VBlank, wk, 0);
    wk->badgeCount = BADGE_COUNT;
    wk->unk1C4 = -1;
    BadgeView_InitPages(wk);
    wk->timeSig = getTimeSigSaveBlock(wk->param->gameData);
    BadgeView_LoadPolish(wk);
    wk->mode = BADGE_MODE_NONE;
    wk->moveSeq = BADGE_MOVE_NONE;
    wk->flags = 0;
    wk->selected = 0;
    wk->kind = 0;
    wk->scrollY = 0;
    wk->unk1E2 = 0;
    wk->msgPage = 0;
    wk->palRes = NULL;
    wk->palData = NULL;
    wk->palAngle = 0;
    for (i = 0; i < 2; i++) {
        wk->timers[i] = 0;
    }
    for (i = 0; i < 4; i++) {
        wk->values[i].flags = 0;
    }
    wk->angle = 0;
    wk->unk27C = 0;
    wk->unk280 = 0;
    wk->srt.translation.x = -FX32_ONE;
    wk->srt.translation.y = 0;
    wk->srt.translation.z = 0;
    wk->srt.scale.x = FX32_ONE;
    wk->srt.scale.y = FX32_ONE;
    wk->srt.scale.z = FX32_ONE;
    MTX_Identity33_(&wk->srt.rotation);
}

static void BadgeView_FreeWork(BadgeViewWork *wk) {
    int i;

    for (i = 0; i < 7; i++) {
        GFL_StrBufFree(wk->strs[i]);
    }
    GFL_StrBufFree(wk->expandBuf);
    GFL_WordSetSystemFree(wk->wordSet);
    GFL_MsgDataFree(wk->msgData);
    GFL_TCBRemove(wk->vblankTask);
    GFL_ArcToolFree(wk->arc);
    GFL_HeapFree(wk->buffer);
}

static void BadgeView_InitVRAM(void) {
    GFL_BGSysInitVRAM(0);
    GFL_BGSysSetVRAMBanks(&sBadgeViewVRAMConfig);
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
}

static void BadgeView_InitBG(BadgeViewWork *wk) {
    u32 i;

    GFL_BGSysCreate(HEAPID_TRAINER_CARD_SCREEN);
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, TRUE };

        GFL_BGSysSetLCDConfig(&lcdConfig);
    }
    {
        BGSetup setups[] = {
            { 0, 0, 0x800, 0, 3, 0, 30, 0, 0x8000, 0, 0, 0, 0 }, { 0, 0, 0x800, 0, 1, 0, 29, 2, 0x4000, 0, 2, 0, 0 },
            { 0, 0, 0x800, 0, 1, 0, 28, 4, 0x4000, 0, 3, 0, 0 }, { 0, 0, 0x800, 0, 2, 0, 30, 0, 0x4000, 0, 1, 0, 0 },
            { 0, 0, 0x800, 0, 1, 0, 26, 4, 0x4000, 0, 0, 0, 0 }, { 0, 0, 0x800, 0, 1, 0, 27, 2, 0x4000, 0, 3, 0, 0 },
            { 0, 0, 0x800, 0, 2, 0, 28, 6, 0x4000, 0, 2, 0, 0 },
        };

        for (i = 0; i < NELEMS(sBadgeViewBGs); i++) {
            u32 bg = sBadgeViewBGs[i];

            GFL_BGSysCreateBG(bg, &setups[i], 0);
            GFL_BGSysClearScr(bg);
            GFL_BGSysClearCharCore(bg, 0x20, 0, HEAPID_TRAINER_CARD_SCREEN);
            GFL_BGSysSetBGEnabled(bg, TRUE);
        }
    }
    G2S_BlendNone();
    G2_BlendNone();
}

static void BadgeView_ExitBG(BadgeViewWork *wk) {
    u32 i;

    for (i = 0; i < NELEMS(sBadgeViewBGs); i++) {
        GFL_BGSysReleaseBG(sBadgeViewBGs[i]);
    }
    GFL_BGSysFree();
    G2_BlendNone();
    G2S_BlendNone();
}

static void BadgeView_LoadBG(BadgeViewWork *wk) {
    ArcTool *arc = wk->arc;
    ArcTool *barArc;
    NNSG2dPaletteData *palette;
    void *file;
    u16 *colors;

    GFL_BGSysLoadArcNCGRStatic(arc, 29, 3, 0, 0, FALSE, HEAPID_TRAINER_CARD_SCREEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 40, 3, 0, 0, FALSE, HEAPID_TRAINER_CARD_SCREEN);
    GFL_G2DIOLoadArcNCLRDefault(arc, 3, 0, 0, 0, HEAPID_TRAINER_CARD_SCREEN);
    GFL_BGSysLoadArcNCGRStatic(arc, 30, 2, 0, 0, FALSE, HEAPID_TRAINER_CARD_SCREEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 41, 2, 0, 0, FALSE, HEAPID_TRAINER_CARD_SCREEN);
    GFL_BGSysLoadArcNCGRStatic(arc, 28, 6, 0, 0, FALSE, HEAPID_TRAINER_CARD_SCREEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 38, 6, 0, 0, FALSE, HEAPID_TRAINER_CARD_SCREEN);
    GFL_G2DIOLoadArcNCLR(arc, 0, 4, 0x100, 0x100, 0x20, HEAPID_TRAINER_CARD_SCREEN);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 4, 0, 0x100, HEAPID_TRAINER_CARD_SCREEN);
    barArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_TRAINER_CARD_SCREEN);
    GFL_BGSysLoadArcNCGRStatic(barArc, func_0202d824(), 5, 0, 0, FALSE, HEAPID_TRAINER_CARD_SCREEN);
    loadBGScrToVramByFileNoReserveNegAlign(barArc, func_0202d828(), 5, 0, 0, FALSE, HEAPID_TRAINER_CARD_SCREEN);
    GFL_G2DIOLoadArcNCLR(barArc, func_0202d820(), 4, 0, 0x180, 0x20, HEAPID_TRAINER_CARD_SCREEN);
    GFL_BGSysSetScrPaletteNo(5, 0, 21, 32, 3, 12);
    GFL_BGSysLoadScr(5);
    GFL_ArcToolFree(barArc);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1e0, 0x20, HEAPID_TRAINER_CARD_SCREEN);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1e0, 0x20, HEAPID_TRAINER_CARD_SCREEN);
    GFL_BGSysLoadArcNCGRStatic(arc, 32, 7, 0, 0, FALSE, HEAPID_TRAINER_CARD_SCREEN);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 42, 7, 0, 0, FALSE, HEAPID_TRAINER_CARD_SCREEN);
    GFL_G2DIOLoadArcNCLRDefault(arc, 7, 4, 0x1a0, 0x40, HEAPID_TRAINER_CARD_SCREEN);
    GFL_BGSysSetScrPaletteNo(7, 0, 0, 32, 12, 13);
    GFL_BGSysLoadScr(7);
    wk->palRes = GFL_G2DIOReadNCLRArc(arc, 0, &wk->palData, HEAPID_TRAINER_CARD_SCREEN);
    file = GFL_G2DIOReadNCLR(ARCID_TRAINER_CARD_GRA, 7, &palette, HEAPID_TRAINER_CARD_SCREEN);
    colors = palette->rawData;
    sys_memcpy(colors, wk->palette[0], 0x20);
    sys_memcpy(colors + 16, wk->palette[1], 0x20);
    GFL_HeapFree(file);
}

static void BadgeView_FreePalette(BadgeViewWork *wk) {
    GFL_HeapFree(wk->palRes);
}

static void BadgeView_InitObj(BadgeViewWork *wk) {
    ArcTool *barArc = GFL_ArcSysCreateFileHandle(getUINarcIdx(), HEAPID_TRAINER_CARD_SCREEN);

    ClActSys_Create(&data_02093f08, &sBadgeViewVRAMConfig, HEAPID_TRAINER_CARD_SCREEN);
    wk->unit = func_0204bf1c(BADGE_ACTOR_COUNT, 1, HEAPID_TRAINER_CARD_SCREEN);
    GFL_BGSysSetBGEnabledA(0x10, TRUE);
    GFL_BGSysSetBGEnabledB(0x10, TRUE);
    wk->res[BADGE_RES_CHARS] = func_0204b81c(wk->arc, 31, FALSE, CLACT_VRAM_SUB, HEAPID_TRAINER_CARD_SCREEN);
    wk->res[BADGE_RES_PALETTE] = func_0204bba0(wk->arc, 4, CLACT_VRAM_SUB, 0, HEAPID_TRAINER_CARD_SCREEN);
    wk->res[BADGE_RES_CELLS] = func_0204bde0(wk->arc, 52, 54, HEAPID_TRAINER_CARD_SCREEN);
    wk->res[BADGE_RES_BAR_CHARS] =
        func_0204b81c(barArc, func_0202d814(), FALSE, CLACT_VRAM_SUB, HEAPID_TRAINER_CARD_SCREEN);
    wk->res[BADGE_RES_BAR_PALETTE] =
        func_0204bbb8(barArc, func_0202d810(), CLACT_VRAM_SUB, 0x160, 0, 3, HEAPID_TRAINER_CARD_SCREEN);
    wk->res[BADGE_RES_BAR_CELLS] =
        func_0204bde0(barArc, func_0202d818(0), func_0202d81c(0), HEAPID_TRAINER_CARD_SCREEN);
    GFL_ArcToolFree(barArc);
    BadgeView_LoadLeaders(wk, CLACT_VRAM_MAIN);
}

static void BadgeView_AddActors(BadgeViewWork *wk) {
    int i;
    ClActorSetup setup;

    for (i = 0; i < BADGE_ACTOR_COUNT; i++) {
        const BadgeViewActorDef *def = &sBadgeViewActorDefs[i];

        setup.x = def->x;
        setup.y = def->y;
        setup.sequence = def->sequence;
        setup.bgPriority = def->bgPriority;
        setup.priority = BADGE_ACTOR_COUNT - i;
        wk->actors[i] = func_0204c040(wk->unit, wk->res[def->charRes], wk->res[def->paletteRes], wk->res[def->cellRes],
                                      &setup, def->surface, HEAPID_TRAINER_CARD_SCREEN);
        func_0204c520(wk->actors[i], TRUE);
    }
}

static void BadgeView_ExitObj(BadgeViewWork *wk) {
    func_0204b98c(wk->res[BADGE_RES_BAR_CHARS]);
    func_0204bcd0(wk->res[BADGE_RES_BAR_PALETTE]);
    func_0204be64(wk->res[BADGE_RES_BAR_CELLS]);
    func_0204b98c(wk->res[BADGE_RES_CHARS]);
    func_0204bcd0(wk->res[BADGE_RES_PALETTE]);
    func_0204be64(wk->res[BADGE_RES_CELLS]);
    BadgeView_FreeLeaders(wk);
    func_0204bf98(wk->unit);
    func_0204b758();
}

static const LightSetup sBadgeViewLights[] = {
    { 0, { { -0xfff, -0xfff, -0xfff }, GX_RGB(31, 31, 31) } },
    { 1, { { 0xfff, -0xfff, -0xfff }, GX_RGB(31, 31, 31) } },
    { 2, { { -0xfff, -0xfff, -0xfff }, GX_RGB(31, 31, 31) } },
    { 3, { { -0xfff, -0xfff, -0xfff }, GX_RGB(31, 31, 31) } },
};

static const G3DSceneResourceSetup sBadgeViewModel1 = { ARCID_TRAINER_CARD_GRA, 57, 0, G3D_SCENE_RES_ARCSYS };

static const G3DSceneResourceSetup sBadgeViewModel2 = { ARCID_TRAINER_CARD_GRA, 58, 0, G3D_SCENE_RES_ARCSYS };

static const LightSetupList sBadgeViewLightList = { sBadgeViewLights, NELEMS(sBadgeViewLights) };

static const G3DSceneResourceSetup sBadgeViewModel3 = { ARCID_TRAINER_CARD_GRA, 59, 0, G3D_SCENE_RES_ARCSYS };

static void BadgeView_Init3D(BadgeViewWork *wk) {
    GFL_G3DSysCreate(FALSE, 1, FALSE, 1, 0x1000, HEAPID_TRAINER_CARD_SCREEN, NULL);
    wk->lights = GFL_G3DLightCreate(&sBadgeViewLightList, HEAPID_TRAINER_CARD_SCREEN);
    GFL_G3DLightFlush(wk->lights);
    {
        G3DCameraProjection projection = {
            G3DCAM_PROJECTION_PERSPECTIVE,
            FX_SinIdx(DEG_TO_IDX(16)),
            FX_CosIdx(DEG_TO_IDX(16)),
            FX32_ONE * 4 / 3,
            0,
            FX32_ONE,
            1024 * FX32_ONE,
            0,
        };

        GFL_G3DSysMtxSetProjection(&projection);
    }
    GFL_BGSysSetBGEnabled(0, TRUE);
    G2_SetBG0Priority(1);
    G3X_AlphaBlend(TRUE);
}

static void BadgeView_Exit3D(BadgeViewWork *wk) {
    GFL_G3DLightFree(wk->lights);
    GFL_G3DSysFree();
}

static const G3DSceneResourceSetup sBadgeViewModel4 = { ARCID_TRAINER_CARD_GRA, 60, 0, G3D_SCENE_RES_ARCSYS };

static const G3DSceneResourceSetup sBadgeViewModel5 = { ARCID_TRAINER_CARD_GRA, 61, 0, G3D_SCENE_RES_ARCSYS };

static const G3DSceneResourceSetup sBadgeViewModel6 = { ARCID_TRAINER_CARD_GRA, 62, 0, G3D_SCENE_RES_ARCSYS };

static const G3DSceneResourceSetup sBadgeViewModel7 = { ARCID_TRAINER_CARD_GRA, 63, 0, G3D_SCENE_RES_ARCSYS };

static const G3DSceneActorSetup sBadgeViewModelActor = { 0, 0, 0, 0, NULL, 0 };

static const TouchRect sBadgeViewMessageRect[] = {
    { 32, 87, 40, 216 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const G3DSceneSetup sBadgeViewScenes[BADGE_COUNT] = {
    { &sBadgeViewModel0, 1, &sBadgeViewModelActor, 1 }, { &sBadgeViewModel1, 1, &sBadgeViewModelActor, 1 },
    { &sBadgeViewModel2, 1, &sBadgeViewModelActor, 1 }, { &sBadgeViewModel3, 1, &sBadgeViewModelActor, 1 },
    { &sBadgeViewModel4, 1, &sBadgeViewModelActor, 1 }, { &sBadgeViewModel5, 1, &sBadgeViewModelActor, 1 },
    { &sBadgeViewModel6, 1, &sBadgeViewModelActor, 1 }, { &sBadgeViewModel7, 1, &sBadgeViewModelActor, 1 },
};

// The message window. BadgeView_InitBmp's use of it compiles to its values
static const BadgeViewWinDef sBadgeViewWinDef = { 4, 1, 1, 30, 9, 15 };

const GameProcFunctions BADGE_VIEW_PROC_FUNCTIONS = {
    BadgeView_Init,
    BadgeView_Main,
    BadgeView_Exit,
};

// The first of each Gym Leader's three messages
static const u32 sBadgeViewLeaderMessages[BADGE_COUNT] = { 65, 68, 71, 74, 77, 80, 83, 86 };

static void BadgeView_InitScenes(BadgeViewWork *wk) {
    int i;

    wk->g3dMgr = GFL_G3DMgrCreate(10, 10, HEAPID_TRAINER_CARD_SCREEN);
    for (i = 0; i < BADGE_COUNT; i++) {
        wk->scenes[i] = GFL_G3DMgrNewScene(wk->g3dMgr, &sBadgeViewScenes[i]);
    }
    {
        VecFx32 position = { 0, 0, 110 * FX32_ONE };
        VecFx32 target = { 0, 0, 0 };
        VecFx32 up = { 0, FX32_ONE, 0 };

        wk->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                         FX_CosIdx(DEG_TO_IDX(20)), FX32_ONE * 4 / 3, 0, FX32_ONE, 1024 * FX32_ONE, 0,
                                         &position, &up, &target, HEAPID_TRAINER_CARD_SCREEN);
    }
}

// Nothing reads the second word but BadgeView_Draw3D, which adds the first to it
static struct {
    int step;
    int count;
    int unk8[6];
} sBadgeViewCounter;

static BadgeViewSeqFunc sBadgeViewSeqFuncs[] = {
    BadgeView_SeqFadeIn, BadgeView_SeqMain, BadgeView_SeqButton, BadgeView_SeqFadeOutStart, BadgeView_SeqFadeOut,
};

static const TouchRect sBadgeViewRects[] = {
    { 88, 168, 0, 32 },     { 88, 168, 32, 64 },    { 88, 168, 64, 96 },
    { 88, 168, 96, 128 },   { 88, 168, 128, 160 },  { 88, 168, 160, 192 },
    { 88, 168, 192, 224 },  { 88, 168, 224, 0 },    { 0, 80, 0, 32 },
    { 0, 80, 32, 64 },      { 0, 80, 64, 96 },      { 0, 80, 96, 128 },
    { 0, 80, 128, 160 },    { 0, 80, 160, 192 },    { 0, 80, 192, 224 },
    { 0, 80, 224, 0 },      { 168, 191, 4, 36 },    { 168, 191, 160, 191 },
    { 168, 191, 192, 223 }, { 168, 191, 224, 255 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static const BadgeViewMove sBadgeViewMoves[] = {
    { 128, 0, BADGE_MOVE_IN_START, 16 },
    { -208, -80, BADGE_MOVE_IN_START, 16 },
    { 0, 128, BADGE_MOVE_OUT_START, 16 },
    { -80, -208, BADGE_MOVE_OUT_START, 16 },
};

// How long each of a Gym Leader's two messages shows
static const s16 sBadgeViewMessageFrames[] = { 300, 480 };

// The actor of each button
static const s16 sBadgeViewButtonActors[] = {
    BADGE_ACTOR_CARD,
    BADGE_ACTOR_RETURN,
    BADGE_ACTOR_EXIT,
    BADGE_ACTOR_SHORTCUT,
};

// The same for the badge's model
static const s16 sBadgeViewModelMoves[2 * 4] = {
    -0x400, -0x140, 0, 16, // in
    -0x140, -0x400, 0, 16, // out
};

// Touching outside this closes a Gym Leader's message
static const TouchRect sBadgeViewLeaderArea[] = {
    { 88, 168, 0, 255 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const u16 sBadgeViewBadgeSEs[BADGE_COUNT] = {
    SEQ_SE_SW_CARD_14, SEQ_SE_SW_CARD_15, SEQ_SE_SW_CARD_16, SEQ_SE_SW_CARD_17,
    SEQ_SE_SW_CARD_18, SEQ_SE_SW_CARD_19, SEQ_SE_SW_CARD_20, SEQ_SE_SW_CARD_21,
};

// And outside this, a badge's
static const TouchRect sBadgeViewBadgeArea[] = {
    { 0, 80, 0, 255 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

static const s16 sBadgeViewLeaderClasses[BADGE_COUNT] = {
    TRAINER_CLASS_LEADER_CHEREN, TRAINER_CLASS_LEADER_ROXIE, TRAINER_CLASS_LEADER_BURGH,   TRAINER_CLASS_LEADER_ELESA,
    TRAINER_CLASS_LEADER_CLAY,   TRAINER_CLASS_LEADER_SKYLA, TRAINER_CLASS_LEADER_DRAYDEN, TRAINER_CLASS_LEADER_MARLON,
};

static void BadgeView_ExitScenes(BadgeViewWork *wk) {
    int i;

    GFL_G3DCameraFree(wk->camera);
    for (i = 0; i < BADGE_COUNT; i++) {
        GFL_G3DMgrDeleteScene(wk->g3dMgr, wk->scenes[i]);
    }
    GFL_G3DMgrFree(wk->g3dMgr);
}

// Draws the model of the badge chosen, while its message shows
static void BadgeView_Draw3D(BadgeViewWork *wk) {
    G3DActor *actor;

    BadgeView_UpdateRotation(wk);
    GFL_G3DCameraFlush(wk->camera);
    GFL_G3DSysReset();
    GFL_G3DSysMtxViewFlush();
    actor = GFL_G3DMgrGetActor(wk->g3dMgr, GFL_G3DMgrGetSceneFirstActorIdx(wk->g3dMgr, wk->scenes[wk->selected]));
    if (wk->flags & BADGE_FLAG_MODEL) {
        GFL_G3DSysDrawObj(actor, &wk->srt);
    }
    GFL_G3DSysReqSwapBuffers();
    sBadgeViewCounter.count += sBadgeViewCounter.step;
}

static void BadgeView_InitBmp(BadgeViewWork *wk) {
    const BadgeViewWinDef *def = &sBadgeViewWinDef;

    BmpWin_InitAllocator(HEAPID_TRAINER_CARD_SCREEN);
    wk->win = BmpWin_CreateDynamic(def->bg, def->x, def->y, def->width, def->height, def->palette, 1);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->win), 0);
}

static void BadgeView_ExitBmp(BadgeViewWork *wk) {
    BmpWin_Free(wk->win);
    BmpWin_FreeAllocator();
}

static void BadgeView_SetMessageVisible(BadgeViewWork *wk, int visible) {
    GFL_BGSysSetBGEnabled(7, visible);
    GFL_BGSysSetBGEnabled(4, visible);
}

static void BadgeView_FlushWin(BadgeViewWork *wk) {
    BmpWin *win = wk->win;

    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
}

static void BadgeView_InitPrint(BadgeViewWork *wk) {
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_TRAINER_CARD_SCREEN);
    wk->queue = func_02021998(HEAPID_TRAINER_CARD_SCREEN);
    PrintWindow_Init(&wk->printWin, wk->win);
}

static void BadgeView_ExitPrint(BadgeViewWork *wk) {
    func_02021a18(wk->queue);
    GFL_FontFree(wk->font);
}

static void BadgeView_UpdatePrint(BadgeViewWork *wk) {
    func_02021a3c(wk->queue);
    PrintWindow_Flush(&wk->printWin, wk->queue);
}

static BOOL BadgeView_RunSeq(BadgeViewWork *wk) {
    return sBadgeViewSeqFuncs[wk->seq](wk);
}

static BOOL BadgeView_SeqFadeIn(BadgeViewWork *wk) {
    if (GFL_WipeIsFinished()) {
        wk->seq = BADGE_SEQ_MAIN;
    }
    return TRUE;
}

static BOOL BadgeView_SeqMain(BadgeViewWork *wk) {
    if (!func_02021c0c(wk->queue)) {
        return TRUE;
    }
    BadgeView_Input(wk);
    return TRUE;
}

// Waits for the animation of the button touched. The cases fall through to each other's actors
static BOOL BadgeView_SeqButton(BadgeViewWork *wk) {
    switch (wk->result) {
    case BADGE_HIT_EXIT:
        if (!func_0204c560(wk->actors[BADGE_ACTOR_EXIT])) {
            wk->seq = BADGE_SEQ_FADE_OUT_START;
        }
    case BADGE_HIT_RETURN:
        if (!func_0204c560(wk->actors[BADGE_ACTOR_RETURN])) {
            wk->seq = BADGE_SEQ_FADE_OUT_START;
        }
    case BADGE_HIT_CARD:
        if (!func_0204c560(wk->actors[BADGE_ACTOR_CARD])) {
            wk->seq = BADGE_SEQ_FADE_OUT_START;
        }
        break;
    }
    return TRUE;
}

static BOOL BadgeView_SeqFadeOutStart(BadgeViewWork *wk) {
    GFL_WipeSet(0, 0, 0, 0, 6, 1, HEAPID_TRAINER_CARD_SCREEN);
    wk->seq = BADGE_SEQ_FADE_OUT;
    return TRUE;
}

// Tells the trainer card's caller where to go: out to the field, back to the menu, or back to the card
static BOOL BadgeView_SeqFadeOut(BadgeViewWork *wk) {
    if (GFL_WipeIsFinished()) {
        if (wk->result == BADGE_HIT_EXIT) {
            wk->param->next = 0;
            wk->param->result = 1;
            wk->seq = BADGE_SEQ_FADE_IN;
        } else if (wk->result == BADGE_HIT_RETURN) {
            wk->param->next = 0;
            wk->param->result = 0;
            wk->seq = BADGE_SEQ_FADE_IN;
        } else if (wk->result == BADGE_HIT_CARD) {
            wk->param->next = 3;
            wk->seq = BADGE_SEQ_FADE_IN;
        }
        return FALSE;
    }
    return TRUE;
}

static void BadgeView_PrintLeaderLine(BadgeViewWork *wk, int message, int line, int badge, StrBuf *strbuf) {
    GFL_MsgDataLoadStrbuf(wk->msgData, message + sBadgeViewLeaderMessages[badge], strbuf);
    PrintWindow_Print(&wk->printWin, wk->queue, 0, line * 40, strbuf, wk->font, BADGE_PRINT_COLOR);
}

// A badge's message, with the date it was obtained on, or a Gym Leader's: the name and one of two lines
static void BadgeView_DrawMessage(BadgeViewWork *wk, int kind, int badge, int page) {
    StrBuf *strbuf = GFL_StrBufCreate(104, HEAPID_TRAINER_CARD_SCREEN);
    int i;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->win), 0);
    switch (kind) {
    case 0: {
        TimeSigBadgeDate date = func_02009230(wk->timeSig, badge);

        WordSetNumber(wk->wordSet, 0, date.year + 2000, 4, 0, TRUE);
        WordSetNumber(wk->wordSet, 1, date.month, 2, 0, TRUE);
        WordSetNumber(wk->wordSet, 2, date.day, 2, 0, TRUE);
        GFL_MsgDataLoadStrbuf(wk->msgData, badge * 3 + 89, strbuf);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->expandBuf, strbuf);
        PrintWindow_Print(&wk->printWin, wk->queue, 0, 0, wk->expandBuf, wk->font, BADGE_PRINT_COLOR);
        for (i = 1; i < 3; i++) {
            GFL_MsgDataLoadStrbuf(wk->msgData, badge * 3 + 89 + i, strbuf);
            PrintWindow_Print(&wk->printWin, wk->queue, 0, i * 40, strbuf, wk->font, BADGE_PRINT_COLOR);
        }
        break;
    }
    case 1:
        BadgeView_PrintLeaderLine(wk, 0, 0, badge, strbuf);
        BadgeView_PrintLeaderLine(wk, page + 1, 1, badge, strbuf);
        break;
    }
    GFL_StrBufFree(strbuf);
}

static int BadgeView_Input(BadgeViewWork *wk) {
    u32 x = 0;
    u32 y = 0;
    BOOL key = FALSE;
    BOOL moving;
    int hit;

    hit = func_0203da0c(sBadgeViewRects);
    func_0203d9c8(sBadgeViewRects);
    func_0203da0c(sBadgeViewMessageRect);
    func_0203d9c8(sBadgeViewMessageRect);
    func_0203da84(&x, &y);
    BadgeView_Update(wk);
    moving = FALSE;
    if (BadgeView_IsMoving(wk) == TRUE) {
        moving = TRUE;
        hit = TOUCH_RECT_NONE;
    }
    if (!moving) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            if (!(wk->flags & (1 << (BADGE_FLAG_BUTTON_OFF_SHIFT + 1)))) {
                GFL_SndSEPlay(SEQ_SE_SW_CARD_04);
                key = TRUE;
                hit = BADGE_HIT_RETURN;
            }
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_X) {
            if (!(wk->flags & (1 << (BADGE_FLAG_BUTTON_OFF_SHIFT + 2)))) {
                GFL_SndSEPlay(SEQ_SE_SW_CARD_05);
                hit = BADGE_HIT_EXIT;
            }
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_Y) {
            if (!(wk->flags & (1 << (BADGE_FLAG_BUTTON_OFF_SHIFT + 3)))) {
                hit = BADGE_HIT_SHORTCUT;
            }
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            if (!(wk->flags & (1 << BADGE_FLAG_BUTTON_OFF_SHIFT))) {
                hit = BADGE_HIT_CARD;
            }
        }
    }
    BadgeView_HandleHit(wk, hit, key);
    return hit;
}

static void BadgeView_HandleHit(BadgeViewWork *wk, int hit, BOOL key) {
    BOOL slide;
    int leader;
    int badge;
    int move;

    if (hit == TOUCH_RECT_NONE) {
        return;
    }
    switch (hit) {
    case BADGE_HIT_BADGE:
    case BADGE_HIT_BADGE + 1:
    case BADGE_HIT_BADGE + 2:
    case BADGE_HIT_BADGE + 3:
    case BADGE_HIT_BADGE + 4:
    case BADGE_HIT_BADGE + 5:
    case BADGE_HIT_BADGE + 6:
    case BADGE_HIT_BADGE + 7:
        if (func_02021c0c(wk->queue) && wk->hasBadge[hit] && wk->mode <= BADGE_MODE_BADGE) {
            slide = FALSE;
            if (wk->mode != BADGE_MODE_BADGE) {
                slide = TRUE;
            }
            BadgeView_Stub(wk, hit);
            if (slide == TRUE) {
                BadgeView_StartMove(wk, 0);
            }
            BadgeView_SelectLeader(wk, TRUE, hit);
            BadgeView_SetModelVisible(wk, TRUE);
            BadgeView_SetMessageVisible(wk, TRUE);
            wk->selected = hit;
            wk->kind = 0;
            if (!slide) {
                BadgeView_DrawMessage(wk, 0, hit, 0);
                BadgeView_FlushWin(wk);
                BadgeView_PlayBadgeSE(wk, wk->selected);
            }
            wk->mode = BADGE_MODE_BADGE;
        }
        break;
    case BADGE_HIT_LEADER:
    case BADGE_HIT_LEADER + 1:
    case BADGE_HIT_LEADER + 2:
    case BADGE_HIT_LEADER + 3:
    case BADGE_HIT_LEADER + 4:
    case BADGE_HIT_LEADER + 5:
    case BADGE_HIT_LEADER + 6:
    case BADGE_HIT_LEADER + 7:
        if (func_02021c0c(wk->queue) && (wk->mode == BADGE_MODE_NONE || wk->mode == BADGE_MODE_LEADER)) {
            slide = FALSE;
            if (wk->mode != BADGE_MODE_LEADER) {
                slide = TRUE;
            }
            leader = hit - BADGE_HIT_LEADER;
            badge = leader % BADGE_COUNT;
            if (wk->hasBadge[badge]) {
                BadgeView_Stub(wk, leader);
                BadgeView_SelectLeader(wk, TRUE, leader);
                BadgeView_SetModelVisible(wk, TRUE);
                if (!BadgeView_IsPaletteAnime(wk)) {
                    BadgeView_StartPaletteAnime(wk);
                    BadgeView_SetPaletteAnime(wk, TRUE);
                }
            } else {
                BadgeView_SelectLeader(wk, FALSE, 0);
                BadgeView_SetModelVisible(wk, FALSE);
                BadgeView_StartPaletteAnime(wk);
                BadgeView_SetPaletteAnime(wk, FALSE);
            }
            if (slide == TRUE) {
                BadgeView_StartMove(wk, 1);
            }
            BadgeView_SetMessageVisible(wk, TRUE);
            wk->selected = badge;
            wk->kind = 1;
            if (!slide) {
                BadgeView_DrawMessage(wk, 1, badge, 0);
                BadgeView_FlushWin(wk);
                GFL_SndSEPlay(SEQ_SE_SW_CARD_22);
            }
            wk->msgPage = 0;
            BadgeView_SetTimer(wk, 0, sBadgeViewMessageFrames[wk->msgPage]);
            wk->mode = BADGE_MODE_LEADER;
        }
        break;
    case BADGE_HIT_CARD:
        if (!(wk->flags & (1 << BADGE_FLAG_BUTTON_OFF_SHIFT))) {
            wk->seq = BADGE_SEQ_BUTTON;
            wk->result = hit;
            GFL_SndSEPlay(SEQ_SE_SW_CARD_01);
            func_0204c488(wk->actors[BADGE_ACTOR_CARD], 12);
        }
        break;
    case BADGE_HIT_SHORTCUT:
        if (!(wk->flags & (1 << (BADGE_FLAG_BUTTON_OFF_SHIFT + 3)))) {
            BadgeView_ToggleShortcut(wk);
        }
        break;
    case BADGE_HIT_EXIT:
        if (!(wk->flags & (1 << (BADGE_FLAG_BUTTON_OFF_SHIFT + 2)))) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_05);
            wk->seq = BADGE_SEQ_BUTTON;
            wk->result = hit;
            func_0204c488(wk->actors[BADGE_ACTOR_EXIT], 8);
        }
        break;
    case BADGE_HIT_RETURN:
        if (wk->mode == BADGE_MODE_NONE) {
            GFL_SndSEPlay(SEQ_SE_SW_CARD_04);
            wk->seq = BADGE_SEQ_BUTTON;
            wk->result = hit;
            func_0204c488(wk->actors[BADGE_ACTOR_RETURN], 9);
            if (key) {
                func_0203d564(FALSE);
            } else {
                func_0203d564(TRUE);
            }
        } else if (!(wk->flags & (1 << (BADGE_FLAG_BUTTON_OFF_SHIFT + 1)))) {
            // With a message open, the button closes it
            func_0204c488(wk->actors[BADGE_ACTOR_RETURN], 9);
            move = 2;
            if (wk->mode == BADGE_MODE_LEADER) {
                move = 3;
            }
            BadgeView_StartMove(wk, move);
            wk->mode = BADGE_MODE_NONE;
        }
        break;
    }
}

static void BadgeView_ToggleShortcut(BadgeViewWork *wk) {
    BOOL registered = GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_TRAINER_CARD_BADGES) ^ 1;

    func_0204c488(wk->actors[BADGE_ACTOR_SHORTCUT], registered + 6);
    GameData_SetKeyItemRegistration(wk->param->gameData, SHORTCUT_TRAINER_CARD_BADGES, registered);
    GFL_SndSEPlay(SEQ_SE_SW_CARD_03);
}

static void BadgeView_InitPages(BadgeViewWork *wk) {
    wk->page = 0;
    wk->pageMax = wk->badgeCount / 10;
    if (wk->badgeCount % 10) {
        wk->pageMax++;
    }
}

static void BadgeView_SetSparklesVisible(BadgeViewWork *wk, int badge, BOOL visible) {
    int i;

    for (i = 0; i < 3; i++) {
        func_0204c124(wk->actors[BADGE_ACTOR_SPARKLE + badge * 3 + i], visible);
    }
}

static void BadgeView_InitBadges(BadgeViewWork *wk, int page, int pageMax) {
    TrainerCardSave *trainerCard = getTrainerCardDataBlkAddress(wk->param->gameData);
    int i;

    for (i = 0; i < BADGE_COUNT; i++) {
        if (isBadgeObtained(trainerCard, i)) {
            func_0204c124(wk->actors[BADGE_ACTOR_BADGE + i], TRUE);
            wk->hasBadge[i] = TRUE;
        } else {
            func_0204c124(wk->actors[BADGE_ACTOR_BADGE + i], FALSE);
            wk->hasBadge[i] = FALSE;
        }
        BadgeView_SetSparklesVisible(wk, i, FALSE);
    }
    func_0204c488(wk->actors[BADGE_ACTOR_SHORTCUT],
                  GameData_IsShortcutRegistered(wk->param->gameData, SHORTCUT_TRAINER_CARD_BADGES) + 6);
}

static void BadgeView_SetPage(BadgeViewWork *wk, int page) {
    wk->page = page;
    BadgeView_InitBadges(wk, page, wk->pageMax);
    BadgeView_FlushWin(wk);
    BadgeView_SetMessageVisible(wk, FALSE);
    BadgeView_SelectLeader(wk, FALSE, 0);
    BadgeView_SetModelVisible(wk, FALSE);
    BadgeView_LoadBadgePalettes(wk);
}

// Saves how polished the badges are, and when
static void BadgeView_SavePolish(BadgeViewWork *wk) {
    RTCDate date;
    RTCTime time;
    s64 seconds;
    int i;

    RTC_GetCachedDateTime(&date, &time);
    seconds = func_0207d12c(&date, &time);
    saveSecondsTime(wk->timeSig, seconds, seconds >> 32);
    for (i = 0; i < BADGE_COUNT; i++) {
        func_020092b8(wk->timeSig, i, wk->polish[i]);
    }
}

// Reads how polished the badges were, less 2 for each hour since
static void BadgeView_LoadPolish(BadgeViewWork *wk) {
    RTCDate nowDate;
    RTCDate savedDate;
    RTCTime nowTime;
    RTCTime savedTime;
    s64 saved = loadSecondsTime(wk->timeSig);
    s64 hours;
    s64 decay;
    int i;

    func_0207d244(&savedDate, &savedTime, saved);
    RTC_GetCachedDateTime(&nowDate, &nowTime);
    hours = (func_0207d12c(&nowDate, &nowTime) - saved) / 3600;
    if (hours < 0) {
        hours = 0;
    }
    decay = hours * 2;
    for (i = 0; i < BADGE_COUNT; i++) {
        wk->polish[i] = func_020092a8(wk->timeSig, i);
        wk->polish[i] -= decay;
        if (wk->polish[i] < 0) {
            wk->polish[i] = 0;
        }
    }
    for (i = 0; i < BADGE_COUNT; i++) {
        wk->unk32C[i] = 0xff;
    }
}

static void BadgeView_StartValue(BadgeViewWork *wk, int index, s16 from, s16 to, s16 frames) {
    if (frames != 0) {
        wk->values[index].from = from;
        wk->values[index].to = to;
        wk->values[index].step = (to - from) * 100 / frames;
        wk->values[index].value = from * 100;
        wk->values[index].flags = 1;
    }
}

// Steps a value and returns it, which is its end once it gets there
static int BadgeView_UpdateValue(BadgeViewWork *wk, int index) {
    s16 *flags = &wk->values[index].flags;
    int result = 0;

    if (*flags & 1) {
        if (!(*flags & 2)) {
            s16 now;
            BOOL down;

            wk->values[index].value += wk->values[index].step;
            now = wk->values[index].value / 100;
            down = FALSE;
            if (wk->values[index].step < 0) {
                down = TRUE;
            }
            if (!down) {
                if (now >= wk->values[index].to) {
                    *flags |= 2;
                }
            } else {
                if (now <= wk->values[index].to) {
                    *flags |= 2;
                }
            }
            result = now;
        } else {
            result = wk->values[index].to;
        }
    }
    return result;
}

static BOOL BadgeView_IsValueMoving(BadgeViewWork *wk, int index) {
    BOOL moving = FALSE;

    if ((wk->values[index].flags & 1) && !(wk->values[index].flags & 2)) {
        moving = TRUE;
    }
    return moving;
}

static void BadgeView_Update(BadgeViewWork *wk) {
    BadgeView_UpdateLeader(wk);
    BadgeView_UpdateBadge(wk);
    BadgeView_UpdateMove(wk);
    BadgeView_AnimatePalette(wk);
}

// Starts the message window's slide in for a badge (0) or a Gym Leader (1), or out (2 and 3), with the model's and
// the Gym Leader's picture's
static void BadgeView_StartMove(BadgeViewWork *wk, int move) {
    int from;
    s16 to;
    s16 frames;
    s16 half;

    from = sBadgeViewMoves[move].from;
    to = sBadgeViewMoves[move].to;
    frames = sBadgeViewMoves[move].frames;
    BadgeView_StartValue(wk, 0, from, to, frames);
    wk->scrollY = from;
    GFL_BGSysMoveBG(7, BG_MOVE_SET_Y, from);
    GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, wk->scrollY);
    wk->moveSeq = sBadgeViewMoves[move].moveSeq;
    half = move / 2;
    from = sBadgeViewLeaderMoves[half * 4];
    to = sBadgeViewLeaderMoves[half * 4 + 1];
    frames = sBadgeViewLeaderMoves[half * 4 + 3];
    BadgeView_StartValue(wk, 1, from, to, frames);
    BadgeView_SetLeadersX(wk, from);
    half = move / 2;
    from = sBadgeViewModelMoves[half * 4];
    to = sBadgeViewModelMoves[half * 4 + 1];
    frames = sBadgeViewModelMoves[half * 4 + 3];
    BadgeView_StartValue(wk, 2, from, to, frames);
    wk->srt.translation.x = from << 8;
}

static void BadgeView_UpdateMove(BadgeViewWork *wk) {
    BOOL noLeader;

    switch (wk->moveSeq) {
    case BADGE_MOVE_NONE:
        break;
    case BADGE_MOVE_IN_START:
        wk->moveSeq = BADGE_MOVE_IN;
        wk->flags |= BADGE_FLAG_MOVING;
        BadgeView_SetButtonActive(wk, 3, FALSE);
        BadgeView_StartPaletteAnime(wk);
        GFL_SndSEPlay(SEQ_SE_SW_CARD_12);
        break;
    case BADGE_MOVE_IN:
        wk->scrollY = BadgeView_UpdateValue(wk, 0);
        if (!BadgeView_IsValueMoving(wk, 0)) {
            wk->moveSeq = BADGE_MOVE_IN_END;
        }
        BadgeView_SetLeadersX(wk, BadgeView_UpdateValue(wk, 1));
        wk->srt.translation.x = BadgeView_UpdateValue(wk, 2) << 8;
        break;
    case BADGE_MOVE_IN_END:
        BadgeView_DrawMessage(wk, wk->kind, wk->selected, 0);
        BadgeView_FlushWin(wk);
        if (wk->mode == BADGE_MODE_LEADER && !wk->hasBadge[wk->selected]) {
            noLeader = TRUE;
        } else {
            noLeader = FALSE;
        }
        if (!noLeader) {
            BadgeView_SetPaletteAnime(wk, TRUE);
        }
        wk->flags |= BADGE_FLAG_MOVE_END;
        wk->moveSeq = BADGE_MOVE_DONE;
        break;
    case BADGE_MOVE_OUT_START:
        wk->moveSeq = BADGE_MOVE_OUT;
        wk->flags |= BADGE_FLAG_MOVING;
        GFL_SndSEPlay(SEQ_SE_SW_CARD_13);
        break;
    case BADGE_MOVE_OUT:
        wk->scrollY = BadgeView_UpdateValue(wk, 0);
        if (!BadgeView_IsValueMoving(wk, 0)) {
            wk->moveSeq = BADGE_MOVE_OUT_END;
        }
        BadgeView_SetLeadersX(wk, BadgeView_UpdateValue(wk, 1));
        wk->srt.translation.x = BadgeView_UpdateValue(wk, 2) << 8;
        break;
    case BADGE_MOVE_OUT_END:
        GFL_BitmapFill(BmpWin_GetBitmap(wk->win), 0);
        BadgeView_FlushWin(wk);
        BadgeView_SetMessageVisible(wk, FALSE);
        BadgeView_SelectLeader(wk, FALSE, 0);
        BadgeView_SetModelVisible(wk, FALSE);
        BadgeView_SetButtonActive(wk, 3, TRUE);
        BadgeView_SetPaletteAnime(wk, FALSE);
        wk->flags |= BADGE_FLAG_MOVE_END;
        wk->moveSeq = BADGE_MOVE_DONE;
        break;
    case BADGE_MOVE_DONE:
        break;
    }
}

static BOOL BadgeView_IsMoving(BadgeViewWork *wk) {
    BOOL moving = FALSE;

    if (wk->flags & BADGE_FLAG_MOVING) {
        moving = TRUE;
    }
    return moving;
}

static void BadgeView_SetActorVisible(BadgeViewWork *wk, int actor, BOOL visible) {
    func_0204c124(wk->actors[actor], visible);
}

// Shows or hides a button, and keeps whether it can be used
static void BadgeView_SetButtonActive(BadgeViewWork *wk, int button, BOOL active) {
    BadgeView_SetActorVisible(wk, sBadgeViewButtonActors[button], active);
    if (active == TRUE) {
        wk->flags &= ~(1 << (button + BADGE_FLAG_BUTTON_OFF_SHIFT));
    } else {
        wk->flags |= 1 << (button + BADGE_FLAG_BUTTON_OFF_SHIFT);
    }
}

static void BadgeView_SetTimer(BadgeViewWork *wk, int index, int frames) {
    wk->timers[index] = frames;
}

static int BadgeView_TickTimer(BadgeViewWork *wk, int index) {
    wk->timers[index]--;
    if (wk->timers[index] <= 0) {
        wk->timers[index] = 0;
    }
    return wk->timers[index];
}

// While a Gym Leader's message shows: it changes between its two lines in time, and a touch outside the Gym Leaders
// goes on to the second line and then closes it. That of a Gym Leader whose badge the player lacks only closes
static void BadgeView_UpdateLeader(BadgeViewWork *wk) {
    BOOL hasBadge = FALSE;
    BOOL changed = FALSE;

    if (wk->mode != BADGE_MODE_LEADER) {
        return;
    }
    if (wk->hasBadge[wk->selected]) {
        hasBadge = TRUE;
    }
    if (hasBadge == FALSE) {
        if (func_0203da0c(sBadgeViewLeaderArea) == 0) {
            BadgeView_StartMove(wk, 3);
            wk->mode = BADGE_MODE_NONE;
        }
    } else if (hasBadge == TRUE) {
        if (BadgeView_TickTimer(wk, 0) <= 0) {
            changed = TRUE;
            wk->msgPage = 1 - wk->msgPage;
            BadgeView_SetTimer(wk, 0, sBadgeViewMessageFrames[wk->msgPage]);
            BadgeView_DrawMessage(wk, wk->kind, wk->selected, wk->msgPage);
            BadgeView_FlushWin(wk);
        }
        if (!changed && func_0203da0c(sBadgeViewLeaderArea) == 0) {
            if (wk->msgPage == 0) {
                wk->msgPage = 1;
                BadgeView_SetTimer(wk, 0, sBadgeViewMessageFrames[wk->msgPage]);
                BadgeView_DrawMessage(wk, wk->kind, wk->selected, wk->msgPage);
                BadgeView_FlushWin(wk);
                GFL_SndSEPlay(SEQ_SE_SW_CARD_02);
            } else {
                BadgeView_StartMove(wk, 3);
                wk->mode = BADGE_MODE_NONE;
            }
        }
    }
}

// A touch outside the badges closes a badge's message
static void BadgeView_UpdateBadge(BadgeViewWork *wk) {
    if (wk->mode == BADGE_MODE_BADGE && func_0203da0c(sBadgeViewBadgeArea) == 0) {
        BadgeView_StartMove(wk, 2);
        wk->mode = BADGE_MODE_NONE;
    }
}

static void BadgeView_PlayBadgeSE(BadgeViewWork *wk, int badge) {
    GFL_SndSEPlay(sBadgeViewBadgeSEs[badge]);
}

static void BadgeView_LoadBadgePalette(BadgeViewWork *wk, int badge) {
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, badge * 0x20,
                                       (u8 *)wk->palData->rawData + badge * 0x20, 0x20);
}

static void BadgeView_LoadBadgePalettes(BadgeViewWork *wk) {
    int i;

    for (i = 0; i < BADGE_COUNT; i++) {
        if (wk->hasBadge[i] == TRUE) {
            BadgeView_LoadBadgePalette(wk, i);
        }
    }
}

static void BadgeView_Stub(BadgeViewWork *wk, int badge) {
}

// Hides the Gym Leaders' pictures but, with show, one
static void BadgeView_SelectLeader(BadgeViewWork *wk, int show, int badge) {
    int i;

    for (i = 0; i < BADGE_COUNT; i++) {
        BadgeView_SetActorVisible(wk, BADGE_ACTOR_LEADER + i, FALSE);
    }
    if (show == TRUE) {
        BadgeView_SetActorVisible(wk, BADGE_ACTOR_LEADER + badge, TRUE);
    }
}

static void BadgeView_UpdateRotation(BadgeViewWork *wk) {
    wk->angle = 0x4000;
    MTX_RotX33_(&wk->srt.rotation, FX_SinIdx(wk->angle), FX_CosIdx(wk->angle));
}

static void BadgeView_SetModelVisible(BadgeViewWork *wk, int visible) {
    if (visible == TRUE) {
        wk->flags |= BADGE_FLAG_MODEL;
    } else {
        wk->flags &= ~BADGE_FLAG_MODEL;
    }
}

static void BadgeView_StartPaletteAnime(BadgeViewWork *wk) {
    wk->palAngle = 0x8000;
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, 0x1a0, wk->palette[0], 0x20);
}

// Fades the message window's palette between its two versions
static void BadgeView_AnimatePalette(BadgeViewWork *wk) {
    int i;
    s16 ratio;

    if (wk->flags & BADGE_FLAG_PALETTE_ANIME) {
        wk->palAngle += 0x400;
        if (wk->palAngle >= 0x10000) {
            wk->palAngle -= 0x10000;
        }
        ratio = (FX_SinCosTable_[(wk->palAngle >> 4) * 2 + 1] + 0x1000) / 2;
        for (i = 0; i < 16; i++) {
            u16 from = wk->palette[0][i];
            u16 to = wk->palette[1][i];
            u8 fr, fg, fb, tr, tg, tb;
            u8 r, g, b;

            fg = (from & 0x3e0) >> 5;
            fr = from & 0x1f;
            fb = (from & 0x7c00) >> 10;
            tb = (to & 0x7c00) >> 10;
            b = fb + ((tb - fb) * ratio >> 12);
            tr = to & 0x1f;
            r = fr + ((tr - fr) * ratio >> 12);
            tg = (to & 0x3e0) >> 5;
            g = fg + ((tg - fg) * ratio >> 12);
            wk->paletteOut[i] = GX_RGB(r, g, b);
        }
        NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_2D_BG_PLTT_SUB, 0x1a0, wk->paletteOut, 0x20);
    }
}

static void BadgeView_SetPaletteAnime(BadgeViewWork *wk, int on) {
    if (on == TRUE) {
        wk->flags |= BADGE_FLAG_PALETTE_ANIME;
    } else {
        wk->flags &= ~BADGE_FLAG_PALETTE_ANIME;
    }
}

static BOOL BadgeView_IsPaletteAnime(BadgeViewWork *wk) {
    BOOL on = FALSE;

    if (wk->flags & BADGE_FLAG_PALETTE_ANIME) {
        on = TRUE;
    }
    return on;
}

// The Gym Leaders' pictures, each with its own palette
static void BadgeView_LoadLeaders(BadgeViewWork *wk, u32 vramType) {
    ArcTool *arc = MakeTrGraArcHandle(HEAPID_TRAINER_CARD_SCREEN);
    int i;

    for (i = 0; i < BADGE_COUNT; i++) {
        int trainerClass = sBadgeViewLeaderClasses[i];

        wk->leaderRes[i].palette =
            TrGra_LoadClActPalette(arc, trainerClass, vramType, i * 0x20, HEAPID_TRAINER_CARD_SCREEN);
        wk->leaderRes[i].chars = TrGra_LoadClActChars(arc, trainerClass, vramType, HEAPID_TRAINER_CARD_SCREEN);
        wk->leaderRes[i].cellAnims = TrGra_LoadClActCellAnims(trainerClass, 0, vramType, HEAPID_TRAINER_CARD_SCREEN);
    }
    GFL_ArcToolFree(arc);
}

static void BadgeView_FreeLeaders(BadgeViewWork *wk) {
    int i;

    for (i = 0; i < BADGE_COUNT; i++) {
        func_0204b98c(wk->leaderRes[i].chars);
        func_0204bcd0(wk->leaderRes[i].palette);
        func_0204be64(wk->leaderRes[i].cellAnims);
    }
}

static void BadgeView_SetActorX(BadgeViewWork *wk, int actor, int x) {
    ClActorPos pos;

    func_0204c178(wk->actors[actor], &pos, CLACT_SURFACE_MAIN);
    pos.x = x;
    func_0204c140(wk->actors[actor], &pos, CLACT_SURFACE_MAIN);
}

static void BadgeView_SetLeadersX(BadgeViewWork *wk, int x) {
    int i;

    for (i = 0; i < BADGE_COUNT; i++) {
        BadgeView_SetActorX(wk, BADGE_ACTOR_LEADER + i, x);
    }
}
