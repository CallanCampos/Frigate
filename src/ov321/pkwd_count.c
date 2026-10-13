#include "types.h"
#include "app/pkwd_count.h"
#include "app/pkwd_count_graphic.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
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
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/pokewood_lines.h"

// The opening of a Pokéstar Studios movie: a film leader whose count shows on the screens, then the movie's title card
// and tagline. Named after the ROM's "pkwd_count_graphic.c" (the other file of the overlay is pkwd_count_graphic.c)

#define MAIN_ACTOR_COUNT 1
#define SUB_ACTOR_COUNT 6
#define RESOURCE_COUNT 6
#define WINDOW_COUNT 2
#define TIMER_COUNT 4
#define LOAD_COUNT_MAX 8

// The ways to align a text on its x
#define ALIGN_LEFT 0
#define ALIGN_RIGHT 1
#define ALIGN_CENTER 2

// The flags of the work
#define FLAG_WIPE_IN_DONE 0x1
#define FLAG_LEADER_DONE 0x2

// The steps of the opening, in order
enum {
    STEP_START = 0,
    STEP_WAIT_WIPE = 10,
    STEP_PLAY_LEADER = 100,
    STEP_SHOW_COUNT = 105,
    STEP_WAIT_COUNT = 110,
    STEP_SHOW_NUMBERS = 120,
    STEP_WAIT_NUMBERS = 200,
    STEP_WAIT_NUMBERS_FADE = 210,
    STEP_WAIT_BLANK = 220,
    STEP_WAIT_TITLE_FADE_IN = 300,
    STEP_WAIT_TITLE = 310,
    STEP_WAIT_TITLE_FADE_OUT = 320,
    STEP_END = 10000,
};

// How the scenes of the leader arrange the BG
enum {
    SCENE_LEADER,
    SCENE_COUNT,
    SCENE_NUMBERS,
};

// A file to load to a palette or BG: where, which file, the offset and, for the palettes and the screens, the size and
// the palette to add
typedef struct {
    u8 target;
    u8 fileId;
    u8 offset;
    u8 extra;
} PkwdCountLoad;

// The files of the cell actors' resources and where their loads go in the work
typedef struct {
    s16 paletteFile;
    s16 paletteSlot;
    s16 paletteOffset;
    s16 paletteCount;
    s16 charsFile;
    s16 charsSlot;
    s16 unk6;
    s16 unk7;
    s16 cellFile;
    s16 cellSlot;
    s16 unk10;
    s16 unk11;
    s16 animFile;
    s16 unk13;
    s16 unk14;
    s16 unk15;
} PkwdCountActorRes;

// The resources that a group of cell actors share, and the list that holds them
typedef struct {
    s16 paletteSlot;
    s16 charsSlot;
    s16 cellSlot;
    s16 count;
    s16 isSub;
} PkwdCountActorGroup;

typedef struct {
    u8 sequence;
    u8 priority;
    u8 bgPriority;
    u8 visible;
    u8 unk4;
    u8 flags;
    u8 positionIdx;
    u8 unk7;
} PkwdCountActorEntry;

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} PkwdCountWindowSetup;

typedef struct {
    BmpWin *window;
    u8 printing;
} PkwdCountWindow;

typedef struct {
    u16 heapId;
    u32 resources[RESOURCE_COUNT];
    ClActor *mainActors[MAIN_ACTOR_COUNT];
    ClActor *subActors[SUB_ACTOR_COUNT];
    PkwdCountGraphic *graphic;
    PokewoodTitleParam *param;
    Font *font;
    PrintQueue *printQueue;
    PkwdCountWindow windows[WINDOW_COUNT];
    MsgData *msgData[2];
    s32 step;
    s32 scene;
    u32 flags;
    s32 timers[TIMER_COUNT];
    s16 *positions;
} PkwdCountWork;

static BOOL PkwdCount_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PkwdCount_Exit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL PkwdCount_Main(GameProc *proc, u32 *state, void *param, void *work);
static void PkwdCount_InitScreen(PkwdCountWork *wk);
static void PkwdCount_Step(PkwdCountWork *wk);
static void PkwdCount_FreeScreen(PkwdCountWork *wk);
static void PkwdCount_StepSequence(PkwdCountWork *wk);
static void PkwdCount_RunSequence(PkwdCountWork *wk);
static s16 PkwdCount_GetPosition(PkwdCountWork *wk, u32 index);
static void PkwdCount_LoadGraphics(PkwdCountWork *wk);
static void PkwdCount_LoadTitleGraphics(PkwdCountWork *wk, u32 graphicIdx, BOOL loadScreen);
static void PkwdCount_CreateActors(PkwdCountWork *wk, u32 surface, const PkwdCountActorEntry *entries,
                                   const PkwdCountActorGroup *group);
static void PkwdCount_LoadActorResources(PkwdCountWork *wk, u32 surface, const PkwdCountActorRes *res);
static void PkwdCount_InitActors(PkwdCountWork *wk);
static void PkwdCount_FreeActors(PkwdCountWork *wk);
static void PkwdCount_PlayAnim(PkwdCountWork *wk, int isSub, int index, u32 sequence);
static void PkwdCount_SetActorVisible(PkwdCountWork *wk, int isSub, int index, BOOL visible);
static BOOL PkwdCount_IsActorAnimating(PkwdCountWork *wk, int isSub, int index);
static BOOL PkwdCount_PlayLeader(PkwdCountWork *wk);
static void PkwdCount_ScrollBG(PkwdCountWork *wk, u32 scene);
static void PkwdCount_SetScene(PkwdCountWork *wk, u32 scene);
static void PkwdCount_InitPaletteFade(PkwdCountWork *wk);
static void PkwdCount_FreePaletteFade(PkwdCountWork *wk);
static void PkwdCount_StartFade(PkwdCountWork *wk, BOOL fadeIn);
static BOOL PkwdCount_IsFading(PkwdCountWork *wk);
static void PkwdCount_SetTimer(PkwdCountWork *wk, u32 index, s32 frames);
static s32 PkwdCount_TickTimer(PkwdCountWork *wk, u32 index);
static void PkwdCount_InitWindows(PkwdCountWork *wk);
static void PkwdCount_FreeWindows(PkwdCountWork *wk);
static void PkwdCount_UpdateText(PkwdCountWork *wk);
static void PkwdCount_PrintText(PkwdCountWork *wk, u32 window, s32 x, s32 y, const StrBuf *strbuf, u32 align,
                                u16 color);
static u16 PkwdCount_GetAlignOffset(const StrBuf *strbuf, Font *font, u32 align);
static void PkwdCount_PrintTitle(PkwdCountWork *wk);
static u32 PkwdCount_GetTitleGraphicIdx(PkwdCountWork *wk);

// The message of the first movie's title, which the others follow
static const u32 sPkwdCountTitleMessageBase[1] = { PokewoodLines_Text_BrycenMan };

static const PkwdCountActorGroup sPkwdCountMainActorGroup = { 0, 1, 2, MAIN_ACTOR_COUNT, FALSE };

static const PkwdCountActorGroup sPkwdCountSubActorGroup = { 3, 4, 5, SUB_ACTOR_COUNT, TRUE };

// Where the BG 2 goes for each scene of the leader
static const ClActorPos sPkwdCountSceneScrolls[3] = { { 0, 0 }, { 256, 0 }, { 0, 192 } };

static const PkwdCountLoad sPkwdCountPaletteLoads[3] = {
    { PALTYPE_MAIN_BG, 1, 0, 2 },
    { PALTYPE_MAIN_BG, 4, 8, 2 },
    { 0xff, 0, 0, 0 },
};

static const PkwdCountLoad sPkwdCountCharLoads[4] = {
    { 2, 10, 0, 0 },
    { 3, 9, 0, 0 },
    { 1, 13, 0, 0 },
    { 0xff, 0, 0, 0 },
};

static const PkwdCountLoad sPkwdCountScreenLoads[4] = {
    { 2, 21, 0, 0 },
    { 3, 20, 0, 0 },
    { 1, 28, 0, 8 },
    { 0xff, 0, 0, 0 },
};

static const PkwdCountActorEntry sPkwdCountMainActors[2] = {
    { 0, 0x30, 2, 1, 0, 0, 0, 0 },
    { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

static const PkwdCountWindowSetup sPkwdCountWindowSetups[WINDOW_COUNT + 1] = {
    { 0, 3, 15, 26, 2, 10 },
    { 0, 3, 18, 26, 4, 10 },
    { 0xff, 0, 0, 0, 0, 0 },
};

static const PkwdCountActorRes sPkwdCountActorRes1 = { 5, 3, 8, 0, 14, 4, 0, 0, 18, 5, 0, 0, 33, -1, 0, 0 };

static const PkwdCountActorRes sPkwdCountActorRes0 = { 2, 0, 0, 0, 11, 1, 0, 0, 17, 2, 0, 0, 32, -1, 0, 0 };

// The graphics of each movie's title card
static const u8 sPkwdCountTitleGraphics[41] = {
    0, 11, 11, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3,  4,  4,  4,  5,  5, 5,
    6, 6,  6,  7, 7, 7, 7, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 12, 0,
};

static const PkwdCountLoad sPkwdCountTitlePalettes[14] = {
    { 0, 0x24, 4, 1 }, { 0, 0x25, 4, 1 }, { 0, 0x26, 4, 1 }, { 0, 0x27, 4, 1 }, { 0, 0x28, 4, 1 },
    { 0, 0x29, 4, 1 }, { 0, 0x2a, 4, 1 }, { 0, 0x2b, 4, 1 }, { 0, 0x2c, 4, 1 }, { 0, 0x2d, 4, 1 },
    { 0, 0x2e, 4, 1 }, { 0, 0x2f, 4, 1 }, { 0, 0x30, 4, 1 }, { 0xff, 0, 0, 0 },
};

static const PkwdCountLoad sPkwdCountTitleScreens[14] = {
    { 1, 0x40, 0, 4 }, { 1, 0x41, 0, 4 }, { 1, 0x43, 0, 4 }, { 1, 0x44, 0, 4 }, { 1, 0x45, 0, 4 },
    { 1, 0x46, 0, 4 }, { 1, 0x47, 0, 4 }, { 1, 0x48, 0, 4 }, { 1, 0x49, 0, 4 }, { 1, 0x4a, 0, 4 },
    { 1, 0x4b, 0, 4 }, { 1, 0x4c, 0, 4 }, { 1, 0x4d, 0, 4 }, { 0xff, 0, 0, 0 },
};

static const PkwdCountActorEntry sPkwdCountSubActors[7] = {
    { 0, 0x30, 1, 0, 0, 1, 1, 0 }, { 1, 0x30, 1, 0, 0, 1, 2, 0 }, { 2, 0x30, 1, 0, 0, 1, 2, 0 },
    { 3, 0x28, 1, 0, 0, 1, 2, 0 }, { 4, 0x20, 1, 0, 0, 1, 2, 0 }, { 5, 0x20, 1, 0, 0, 1, 2, 0 },
    { 0xff, 0, 0, 0, 0, 0, 0, 0 },
};

static const PkwdCountLoad sPkwdCountTitleChars[14] = {
    { 1, 0x32, 0, 0 }, { 1, 0x33, 0, 0 }, { 1, 0x34, 0, 0 }, { 1, 0x35, 0, 0 }, { 1, 0x36, 0, 0 },
    { 1, 0x37, 0, 0 }, { 1, 0x38, 0, 0 }, { 1, 0x39, 0, 0 }, { 1, 0x3a, 0, 0 }, { 1, 0x3b, 0, 0 },
    { 1, 0x3c, 0, 0 }, { 1, 0x3d, 0, 0 }, { 1, 0x3e, 0, 0 }, { 0xff, 0, 0, 0 },
};

const GameProcFunctions PKWD_COUNT_PROC_FUNCTIONS = { PkwdCount_Init, PkwdCount_Main, PkwdCount_Exit };

// The lines that each movie's title card shows for a boy and a girl
static const s16 sPkwdCountTaglines[41][2] = {
    { PokewoodLines_Text_KidVsBrycenMan, PokewoodLines_Text_GirlVsBrycenMan },
    { PokewoodLines_Text_KidFacesReturnBrycen, PokewoodLines_Text_GirlFacesReturnBrycen },
    { PokewoodLines_Text_KidEndBrycenMan, PokewoodLines_Text_GirlEndBrycenMan },
    { PokewoodLines_Text_EnterNinja, PokewoodLines_Text_EnterNinja_2 },
    { PokewoodLines_Text_NinjasTrap, PokewoodLines_Text_NinjasTrap_2 },
    { PokewoodLines_Text_LittleSisterVsNinja, PokewoodLines_Text_LittleSisterVsNinja_2 },
    { PokewoodLines_Text_NinjaFinalCase, PokewoodLines_Text_NinjaFinalCase_2 },
    { PokewoodLines_Text_TerrorFromSky, PokewoodLines_Text_TerrorFromSky_2 },
    { PokewoodLines_Text_UfoMysteriousWoman, PokewoodLines_Text_UfoMysteriousWoman_2 },
    { PokewoodLines_Text_DecoyPlan, PokewoodLines_Text_DecoyPlan_2 },
    { PokewoodLines_Text_HumanitysUltimateStruggle, PokewoodLines_Text_HumanitysUltimateStruggle_2 },
    { PokewoodLines_Text_TitanicTyranitar, PokewoodLines_Text_TitanicTyranitar_2 },
    { PokewoodLines_Text_MechanicalMenace, PokewoodLines_Text_MechanicalMenace_2 },
    { PokewoodLines_Text_MysteriousBeach, PokewoodLines_Text_MysteriousBeach_2 },
    { PokewoodLines_Text_ReturnMechaTyranitar, PokewoodLines_Text_ReturnMechaTyranitar_2 },
    { PokewoodLines_Text_GlassesDestiny, PokewoodLines_Text_GlassesDestiny_2 },
    { PokewoodLines_Text_PokemonPlanet, PokewoodLines_Text_PokemonPlanet_2 },
    { PokewoodLines_Text_EscapeFromPokemonPlanet, PokewoodLines_Text_EscapeFromPokemonPlanet_2 },
    { PokewoodLines_Text_PenelopesStory, PokewoodLines_Text_MiguelsStory },
    { PokewoodLines_Text_BarbarasStory, PokewoodLines_Text_CamilosStory },
    { PokewoodLines_Text_MariasStory, PokewoodLines_Text_AntoniosStory },
    { PokewoodLines_Text_BeyondDoor, PokewoodLines_Text_BeyondDoor_2 },
    { PokewoodLines_Text_DoorsSecrets, PokewoodLines_Text_DoorsSecrets_2 },
    { PokewoodLines_Text_OpenDoor, PokewoodLines_Text_OpenDoor_2 },
    { PokewoodLines_Text_QuarrelEndsTroubleBegins, PokewoodLines_Text_QuarrelEndsTroubleBegins_2 },
    { PokewoodLines_Text_MysteriousCaller, PokewoodLines_Text_MysteriousCaller_2 },
    { PokewoodLines_Text_TrialsGiantWoman, PokewoodLines_Text_TrialsGiantWoman_2 },
    { PokewoodLines_Text_FarewellDearGiant, PokewoodLines_Text_FarewellDearGiant_2 },
    { PokewoodLines_Text_HorrificMotel, PokewoodLines_Text_HorrificMotel_2 },
    { PokewoodLines_Text_WanderersPark, PokewoodLines_Text_WanderersPark_2 },
    { PokewoodLines_Text_CryFromBeyond, PokewoodLines_Text_CryFromBeyond_2 },
    { PokewoodLines_Text_Initiate, PokewoodLines_Text_Initiate_2 },
    { PokewoodLines_Text_Connect, PokewoodLines_Text_Connect_2 },
    { PokewoodLines_Text_RuntimeException, PokewoodLines_Text_RuntimeException_2 },
    { PokewoodLines_Text_Shutdown, PokewoodLines_Text_Shutdown_2 },
    { PokewoodLines_Text_File1VillageGhosts, PokewoodLines_Text_File1VillageGhosts_2 },
    { PokewoodLines_Text_File2CuriousPossession, PokewoodLines_Text_File2CuriousPossession_2 },
    { PokewoodLines_Text_File3PhantomTreasure, PokewoodLines_Text_File3PhantomTreasure_2 },
    { PokewoodLines_Text_File4MajinMayhem, PokewoodLines_Text_File4MajinMayhem_2 },
    { PokewoodLines_Text_FinalShowdown, PokewoodLines_Text_FinalShowdown_2 },
    { PokewoodLines_Text_MidlifeManJusticeVs, PokewoodLines_Text_MidlifeManJusticeVs_2 },
};

static BOOL PkwdCount_Init(GameProc *proc, u32 *state, void *param, void *work) {
    PkwdCountWork *wk;

    GFL_OvlLoad(OVERLAY_ID(139));
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_PKWD_COUNT, 0x30000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(PkwdCountWork), HEAPID_PKWD_COUNT);
    sys_memset(wk, 0, sizeof(PkwdCountWork));
    wk->heapId = HEAPID_PKWD_COUNT;
    wk->param = param;
    wk->graphic = PkwdCountGraphic_Create(1, wk->heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, wk->heapId);
    wk->printQueue = func_02021998(wk->heapId);
    PkwdCount_InitScreen(wk);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
    return TRUE;
}

static BOOL PkwdCount_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    PkwdCountWork *wk = work;
    HeapID heapId;

    if (!func_02021c0c(wk->printQueue)) {
        return FALSE;
    }
    PkwdCount_FreeScreen(wk);
    func_02021a18(wk->printQueue);
    GFL_FontFree(wk->font);
    PkwdCountGraphic_Free(wk->graphic);
    heapId = wk->heapId;
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(heapId);
    GFL_OvlUnload(OVERLAY_ID(139));
    return TRUE;
}

static BOOL PkwdCount_Main(GameProc *proc, u32 *state, void *param, void *work) {
    PkwdCountWork *wk = work;

    switch (*state) {
    case 0:
        if (GFL_WipeIsFinished()) {
            (*state)++;
            wk->flags |= FLAG_WIPE_IN_DONE;
        }
        break;
    case 1:
        if (wk->step == STEP_END) {
            (*state)++;
        }
        break;
    case 2:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, wk->heapId);
        (*state)++;
        break;
    case 3:
        if (GFL_WipeIsFinished()) {
            return TRUE;
        }
        break;
    }
    PkwdCount_Step(wk);
    PkwdCount_UpdateText(wk);
    PkwdCountGraphic_Update(wk->graphic);
    PkwdCountGraphic_Unused1(wk->graphic);
    PkwdCountGraphic_Unused2(wk->graphic);
    return FALSE;
}

static void PkwdCount_InitScreen(PkwdCountWork *wk) {
    int i;

    wk->step = STEP_START;
    wk->scene = 0;
    wk->flags = 0;
    for (i = 0; i < TIMER_COUNT; i++) {
        wk->timers[i] = 0;
    }
    wk->positions = GFL_ArcSysReadHeapNew(ARCID_PKWD_COUNT, 34, wk->heapId);
    wk->msgData[0] = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_POKEWOOD_LINES, wk->heapId);
    wk->msgData[1] = NULL;
    PkwdCount_LoadGraphics(wk);
    PkwdCount_LoadTitleGraphics(wk, PkwdCount_GetTitleGraphicIdx(wk), FALSE);
    PkwdCount_InitWindows(wk);
    PkwdCount_InitActors(wk);
    PkwdCount_InitPaletteFade(wk);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 0, 0x3f, 8, 6);
    GFL_BGSysSetBGEnabled(1, FALSE);
}

static void PkwdCount_Step(PkwdCountWork *wk) {
    PkwdCount_StepSequence(wk);
}

static void PkwdCount_FreeScreen(PkwdCountWork *wk) {
    int i;

    GFL_HeapFree(wk->positions);
    PkwdCount_FreeWindows(wk);
    for (i = 0; i < 2; i++) {
        if (wk->msgData[i] != NULL) {
            GFL_MsgDataFree(wk->msgData[i]);
            wk->msgData[i] = NULL;
        }
    }
    PkwdCount_FreeActors(wk);
    PkwdCount_FreePaletteFade(wk);
}

static void PkwdCount_StepSequence(PkwdCountWork *wk) {
    PkwdCount_RunSequence(wk);
}

static void PkwdCount_RunSequence(PkwdCountWork *wk) {
    switch (wk->step) {
    case STEP_START:
        wk->step = STEP_WAIT_WIPE;
        break;
    case STEP_WAIT_WIPE:
        if (wk->flags & FLAG_WIPE_IN_DONE) {
            wk->step = STEP_PLAY_LEADER;
            PkwdCount_PlayAnim(wk, FALSE, 0, 0);
            GFL_SndSEPlay(SEQ_SE_SW_PKWOOD_01);
        }
        break;
    case STEP_PLAY_LEADER:
        if (PkwdCount_PlayLeader(wk)) {
            break;
        }
        PkwdCount_SetScene(wk, SCENE_LEADER);
        wk->step = STEP_SHOW_COUNT;
        break;
    case STEP_SHOW_COUNT:
        PkwdCount_SetScene(wk, SCENE_COUNT);
        PkwdCount_SetTimer(wk, 0, 60);
        wk->step = STEP_WAIT_COUNT;
        break;
    case STEP_WAIT_COUNT:
        if (PkwdCount_TickTimer(wk, 0) != 0) {
            break;
        }
        PkwdCount_StartFade(wk, FALSE);
        wk->step = STEP_SHOW_NUMBERS;
        break;
    case STEP_SHOW_NUMBERS:
        PkwdCount_SetScene(wk, SCENE_NUMBERS);
        PkwdCount_SetTimer(wk, 1, 180);
        wk->step = STEP_WAIT_NUMBERS;
        GFL_SndSEPlay(SEQ_SE_SW_PKWOOD_02);
        break;
    case STEP_WAIT_NUMBERS:
        if (PkwdCount_TickTimer(wk, 1) != 0) {
            break;
        }
        if (PkwdCount_IsFading(wk)) {
            break;
        }
        PkwdCount_StartFade(wk, TRUE);
        wk->step = STEP_WAIT_NUMBERS_FADE;
        break;
    case STEP_WAIT_NUMBERS_FADE:
        if (PkwdCount_IsFading(wk)) {
            break;
        }
        wk->step = STEP_WAIT_BLANK;
        GFL_BGSysSetBGEnabled(1, FALSE);
        GFL_BGSysSetBGEnabled(3, FALSE);
        GFL_BGSysSetBGEnabled(2, FALSE);
        PkwdCount_SetActorVisible(wk, TRUE, 0, FALSE);
        PkwdCount_SetActorVisible(wk, TRUE, 1, FALSE);
        PkwdCount_SetActorVisible(wk, TRUE, 2, FALSE);
        PkwdCount_SetActorVisible(wk, TRUE, 3, FALSE);
        PkwdCount_SetActorVisible(wk, TRUE, 4, FALSE);
        PkwdCount_SetActorVisible(wk, TRUE, 5, FALSE);
        PkwdCount_SetTimer(wk, 2, 60);
        break;
    case STEP_WAIT_BLANK:
        if (PkwdCount_TickTimer(wk, 2) != 0) {
            break;
        }
        PkwdCount_LoadTitleGraphics(wk, PkwdCount_GetTitleGraphicIdx(wk), TRUE);
        PkwdCount_PrintTitle(wk);
        GFL_BGSysSetBGEnabled(1, TRUE);
        GFL_BGSysSetBGEnabled(3, TRUE);
        PkwdCount_StartFade(wk, FALSE);
        wk->step = STEP_WAIT_TITLE_FADE_IN;
        break;
    case STEP_WAIT_TITLE_FADE_IN:
        if (PkwdCount_IsFading(wk)) {
            break;
        }
        wk->step = STEP_WAIT_TITLE;
        PkwdCount_SetTimer(wk, 2, 180);
        break;
    case STEP_WAIT_TITLE:
        if (PkwdCount_TickTimer(wk, 2) != 0) {
            break;
        }
        PkwdCount_StartFade(wk, TRUE);
        wk->step = STEP_WAIT_TITLE_FADE_OUT;
        break;
    case STEP_WAIT_TITLE_FADE_OUT:
        if (PkwdCount_IsFading(wk)) {
            break;
        }
        wk->step = STEP_END;
        break;
    case STEP_END:
        break;
    }
}

// The x or the y of a position that the opening's file holds, or 0 past its three positions
static s16 PkwdCount_GetPosition(PkwdCountWork *wk, u32 index) {
    if (index >= 6) {
        return 0;
    }
    return wk->positions[index];
}

static void PkwdCount_LoadGraphics(PkwdCountWork *wk) {
    const PkwdCountLoad *palette = sPkwdCountPaletteLoads;
    const PkwdCountLoad *chars = sPkwdCountCharLoads;
    const PkwdCountLoad *screen = sPkwdCountScreenLoads;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_COUNT, wk->heapId);
    int i;

    for (i = 0; i < LOAD_COUNT_MAX; i++, palette++) {
        if (palette->target == 0xff) {
            break;
        }
        GFL_G2DIOLoadArcNCLRDefault(arc, palette->fileId, palette->target, palette->offset << 5, palette->extra << 5,
                                    wk->heapId);
    }
    for (i = 0; i < LOAD_COUNT_MAX; i++, chars++) {
        if (chars->target == 0xff) {
            break;
        }
        GFL_BGSysLoadArcNCGRStatic(arc, chars->fileId, chars->target, chars->offset, 0, FALSE, wk->heapId);
    }
    for (i = 0; i < LOAD_COUNT_MAX; i++, screen++) {
        if (screen->target == 0xff) {
            break;
        }
        loadBGScrToVramByFileNoReserveNegAlign(arc, screen->fileId, screen->target, screen->offset, 0, FALSE,
                                               wk->heapId);
        if (screen->extra != 0) {
            u16 *screenBuf = GFL_BGSysIsScrHeapExists(screen->target);
            int row;
            int col;

            for (row = 0; row < 24; row++) {
                for (col = 0; col < 32; col++) {
                    screenBuf[row * 32 + col] += (screen->extra & 0xf) << 12;
                }
            }
            GFL_BGSysLoadScr(screen->target);
        }
    }
    GFL_ArcToolFree(arc);
    GFL_BGSysResetStdPalette(0, 0);
}

static void PkwdCount_LoadTitleGraphics(PkwdCountWork *wk, u32 graphicIdx, BOOL loadScreen) {
    const PkwdCountLoad *palette = sPkwdCountTitlePalettes;
    const PkwdCountLoad *chars = sPkwdCountTitleChars;
    const PkwdCountLoad *screen = sPkwdCountTitleScreens;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_COUNT, wk->heapId);

    if (!loadScreen) {
        palette += graphicIdx;
        GFL_G2DIOLoadArcNCLRDefault(arc, palette->fileId, palette->target, palette->offset << 5, palette->extra << 5,
                                    wk->heapId);
    } else {
        chars += graphicIdx;
        GFL_BGSysLoadArcNCGRStatic(arc, chars->fileId, chars->target, chars->offset, 0, FALSE, wk->heapId);
        screen += graphicIdx;
        loadBGScrToVramByFileNoReserveNegAlign(arc, screen->fileId, screen->target, screen->offset, 0, FALSE,
                                               wk->heapId);
        if (screen->extra != 0) {
            u16 *screenBuf = GFL_BGSysIsScrHeapExists(screen->target);
            int row;
            int col;

            for (row = 0; row < 24; row++) {
                for (col = 0; col < 32; col++) {
                    screenBuf[row * 32 + col] += (screen->extra & 0xf) << 12;
                }
            }
            GFL_BGSysLoadScr(screen->target);
        }
    }
    GFL_ArcToolFree(arc);
    GFL_BGSysResetStdPalette(0, 0);
}

static void PkwdCount_CreateActors(PkwdCountWork *wk, u32 surface, const PkwdCountActorEntry *entries,
                                   const PkwdCountActorGroup *group) {
    int i;
    ClActUnit *unit = PkwdCountGraphic_GetClActUnit(wk->graphic);
    ClActorSetup setup;
    ClActor **actors;
    s16 count;
    s16 isSub;
    s16 paletteSlot;
    s16 charsSlot;
    s16 cellSlot;

    sys_memset(&setup, 0, sizeof(setup));
    isSub = group->isSub;
    paletteSlot = group->paletteSlot;
    charsSlot = group->charsSlot;
    cellSlot = group->cellSlot;
    count = group->count;
    if (isSub == 0) {
        actors = wk->mainActors;
    } else {
        actors = wk->subActors;
    }
    for (i = 0; i < count; entries++, i++) {
        u32 positionIdx;
        s16 x;
        s16 y;
        BOOL visible;
        BOOL animated;

        if (entries->sequence == 0xff) {
            break;
        }
        positionIdx = entries->positionIdx * 2;
        x = PkwdCount_GetPosition(wk, positionIdx);
        y = PkwdCount_GetPosition(wk, positionIdx + 1);
        setup.x = x;
        setup.y = y;
        setup.sequence = entries->sequence;
        setup.priority = entries->priority;
        setup.bgPriority = entries->bgPriority;
        actors[i] = func_0204c040(unit, wk->resources[charsSlot], wk->resources[paletteSlot], wk->resources[cellSlot],
                                  &setup, surface, wk->heapId);
        visible = FALSE;
        if (entries->visible == 1) {
            visible = TRUE;
        }
        func_0204c124(actors[i], visible);
        animated = FALSE;
        if (entries->flags & 1) {
            animated = TRUE;
        }
        func_0204c520(actors[i], animated);
        if (entries->flags & 2) {
            func_0204c2b0(actors[i], 1, TRUE);
        }
        if (entries->flags & 4) {
            func_0204c2b0(actors[i], 0, TRUE);
        }
    }
}

static void PkwdCount_LoadActorResources(PkwdCountWork *wk, u32 surface, const PkwdCountActorRes *res) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_PKWD_COUNT, wk->heapId);

    PkwdCountGraphic_GetClActUnit(wk->graphic);
    wk->resources[res->paletteSlot] =
        func_0204bbb8(arc, res->paletteFile, surface, res->paletteOffset << 5, 0, res->paletteCount, wk->heapId);
    wk->resources[res->charsSlot] = func_0204b81c(arc, res->charsFile, FALSE, surface, wk->heapId);
    wk->resources[res->cellSlot] = func_0204bde0(arc, res->cellFile, res->animFile, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void PkwdCount_InitActors(PkwdCountWork *wk) {
    PkwdCountGraphic_GetClActUnit(wk->graphic);
    PkwdCount_LoadActorResources(wk, 0, &sPkwdCountActorRes0);
    PkwdCount_LoadActorResources(wk, 0, &sPkwdCountActorRes1);
    PkwdCount_CreateActors(wk, 0, sPkwdCountMainActors, &sPkwdCountMainActorGroup);
    PkwdCount_CreateActors(wk, 0, sPkwdCountSubActors, &sPkwdCountSubActorGroup);
    func_0204c318(wk->mainActors[0], 1);
    func_02042ba8(FALSE, wk->heapId);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void PkwdCount_FreeActors(PkwdCountWork *wk) {
    int i;

    func_0204c108(wk->mainActors[0]);
    for (i = 0; i < SUB_ACTOR_COUNT; i++) {
        func_0204c108(wk->subActors[i]);
    }
    func_0204bcd0(wk->resources[0]);
    func_0204b98c(wk->resources[1]);
    func_0204be64(wk->resources[2]);
    func_0204bcd0(wk->resources[3]);
    func_0204b98c(wk->resources[4]);
    func_0204be64(wk->resources[5]);
}

static void PkwdCount_PlayAnim(PkwdCountWork *wk, int isSub, int index, u32 sequence) {
    if (isSub == 0) {
        if (index < MAIN_ACTOR_COUNT) {
            func_0204c488(wk->mainActors[index], (u16)sequence);
            func_0204c520(wk->mainActors[index], TRUE);
        }
    } else if (index < SUB_ACTOR_COUNT) {
        func_0204c488(wk->subActors[index], (u16)sequence);
        func_0204c520(wk->subActors[index], TRUE);
    }
}

static void PkwdCount_SetActorVisible(PkwdCountWork *wk, int isSub, int index, BOOL visible) {
    if (isSub == 0) {
        if (index < MAIN_ACTOR_COUNT) {
            func_0204c124(wk->mainActors[index], visible);
        }
    } else if (index < SUB_ACTOR_COUNT) {
        func_0204c124(wk->subActors[index], visible);
    }
}

static BOOL PkwdCount_IsActorAnimating(PkwdCountWork *wk, int isSub, int index) {
    BOOL result = TRUE;

    if (isSub == 0) {
        if (index >= MAIN_ACTOR_COUNT) {
            return FALSE;
        }
        if (!func_0204c560(wk->mainActors[index])) {
            result = FALSE;
        }
    } else {
        if (index >= SUB_ACTOR_COUNT) {
            return FALSE;
        }
        if (!func_0204c560(wk->subActors[index])) {
            result = FALSE;
        }
    }
    return result;
}

// Runs the leader's animation, which has three parts. Returns whether it is still running
static BOOL PkwdCount_PlayLeader(PkwdCountWork *wk) {
    BOOL running = TRUE;

    if (!PkwdCount_IsActorAnimating(wk, FALSE, 0)) {
        PkwdCount_PlayAnim(wk, FALSE, 0, 0);
        wk->scene++;
        if (wk->scene > 2) {
            wk->scene = 2;
            wk->flags |= FLAG_LEADER_DONE;
            running = FALSE;
        }
        PkwdCount_ScrollBG(wk, wk->scene);
    }
    return running;
}

static void PkwdCount_ScrollBG(PkwdCountWork *wk, u32 scene) {
    GFL_BGSysMoveBG(2, BG_MOVE_SET_X, sPkwdCountSceneScrolls[scene].x);
    GFL_BGSysMoveBG(2, BG_MOVE_SET_Y, sPkwdCountSceneScrolls[scene].y);
}

static void PkwdCount_SetScene(PkwdCountWork *wk, u32 scene) {
    if (scene == SCENE_LEADER) {
        ClActorPos pos;

        PkwdCount_SetActorVisible(wk, FALSE, 0, FALSE);
        func_0204c178(wk->mainActors[0], &pos, 0);
        pos.y = 256;
        func_0204c140(wk->mainActors[0], &pos, 0);
    } else if (scene == SCENE_COUNT) {
        GFL_BGSysSetBGEnabled(3, FALSE);
        GFL_BGSysSetBGEnabled(2, FALSE);
    } else {
        GFL_BGSysSetBGEnabled(1, TRUE);
        PkwdCount_SetActorVisible(wk, TRUE, 0, TRUE);
        PkwdCount_SetActorVisible(wk, TRUE, 1, TRUE);
        PkwdCount_SetActorVisible(wk, TRUE, 2, TRUE);
        PkwdCount_SetActorVisible(wk, TRUE, 3, TRUE);
        PkwdCount_SetActorVisible(wk, TRUE, 4, TRUE);
        PkwdCount_SetActorVisible(wk, TRUE, 5, TRUE);
        PkwdCount_PlayAnim(wk, TRUE, 0, 0);
        PkwdCount_PlayAnim(wk, TRUE, 1, 1);
        PkwdCount_PlayAnim(wk, TRUE, 2, 2);
        PkwdCount_PlayAnim(wk, TRUE, 3, 3);
        PkwdCount_PlayAnim(wk, TRUE, 4, 4);
        PkwdCount_PlayAnim(wk, TRUE, 5, 5);
    }
}

static void PkwdCount_InitPaletteFade(PkwdCountWork *wk) {
    PaletteFade *fade = PkwdCountGraphic_GetPaletteFade(wk->graphic);

    PaletteFade_AllocBuffer(fade, 0, 0x180, wk->heapId);
    PaletteFade_AllocBuffer(fade, 2, 0x180, wk->heapId);
    PaletteFade_LoadFromVRAM(fade, 0, 0, 0x180);
    PaletteFade_LoadFromVRAM(fade, 2, 0, 0x180);
}

static void PkwdCount_FreePaletteFade(PkwdCountWork *wk) {
    PaletteFade *fade = PkwdCountGraphic_GetPaletteFade(wk->graphic);

    PaletteFade_FreeBuffer(fade, 0);
    PaletteFade_FreeBuffer(fade, 2);
}

static void PkwdCount_StartFade(PkwdCountWork *wk, BOOL fadeIn) {
    PaletteFade *fade = PkwdCountGraphic_GetPaletteFade(wk->graphic);
    u8 start;
    u8 end;
    s8 delay;

    if (!fadeIn) {
        start = 16;
        end = 0;
        delay = 4;
    } else {
        start = 0;
        end = 16;
        delay = 4;
    }
    PaletteFade_StartFade(fade, 1, 0xfff, delay, start, end, 0, GFL_VBlankGetTCBMgr());
    PaletteFade_StartFade(fade, 4, 0xfff, delay, start, end, 0, GFL_VBlankGetTCBMgr());
}

static BOOL PkwdCount_IsFading(PkwdCountWork *wk) {
    BOOL fading = TRUE;

    if (PaletteFade_GetActiveMask(PkwdCountGraphic_GetPaletteFade(wk->graphic)) == 0) {
        fading = FALSE;
    }
    return fading;
}

static void PkwdCount_SetTimer(PkwdCountWork *wk, u32 index, s32 frames) {
    wk->timers[index] = frames;
}

static s32 PkwdCount_TickTimer(PkwdCountWork *wk, u32 index) {
    wk->timers[index]--;
    if (wk->timers[index] <= 0) {
        wk->timers[index] = 0;
    }
    return wk->timers[index];
}

static void PkwdCount_InitWindows(PkwdCountWork *wk) {
    int i;
    const PkwdCountWindowSetup *setup = sPkwdCountWindowSetups;
    ArcTool *arc;

    for (i = 0; i < WINDOW_COUNT; i++, setup++) {
        if (setup->bg == 0xff) {
            break;
        }
        wk->windows[i].window =
            BmpWin_CreateDynamic(setup->bg, setup->x, setup->y, setup->width, setup->height, setup->palette, TRUE);
        BmpWin_FlushMap(wk->windows[i].window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(wk->windows[i].window));
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, wk->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, PALTYPE_MAIN_BG, 0x140, 0x20, wk->heapId);
    GFL_ArcToolFree(arc);
}

static void PkwdCount_FreeWindows(PkwdCountWork *wk) {
    int i;

    for (i = 0; i < WINDOW_COUNT; i++) {
        if (wk->windows[i].window != NULL) {
            BmpWin_Free(wk->windows[i].window);
            wk->windows[i].window = NULL;
        }
    }
}

static void PkwdCount_UpdateText(PkwdCountWork *wk) {
    int i;

    func_02021a3c(wk->printQueue);
    for (i = 0; i < WINDOW_COUNT; i++) {
        PrintQueue *queue = wk->printQueue;

        if (wk->windows[i].printing && !func_02021c1c(queue, BmpWin_GetBitmap(wk->windows[i].window))) {
            BmpWin_FlushChar(wk->windows[i].window);
            wk->windows[i].printing = FALSE;
        }
    }
}

// Prints a text so that its left, right or center is at x
static void PkwdCount_PrintText(PkwdCountWork *wk, u32 window, s32 x, s32 y, const StrBuf *strbuf, u32 align,
                                u16 color) {
    u16 offset = PkwdCount_GetAlignOffset(strbuf, wk->font, align);
    PrintQueue *queue = wk->printQueue;

    func_02021c7c(queue, BmpWin_GetBitmap(wk->windows[window].window), x - offset, y, strbuf, wk->font, color);
    wk->windows[window].printing = TRUE;
}

static u16 PkwdCount_GetAlignOffset(const StrBuf *strbuf, Font *font, u32 align) {
    if (align == ALIGN_RIGHT) {
        return GFL_FontGetBlockWidth(strbuf, font, 0);
    } else if (align == ALIGN_CENTER) {
        return GFL_FontGetBlockWidth(strbuf, font, 0) / 2;
    }
    return 0;
}

// Prints the movie's title and its tagline
static void PkwdCount_PrintTitle(PkwdCountWork *wk) {
    s32 lineId;
    WordSet *wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    StrBuf *title = GFL_MsgDataLoadStrbufNew(wk->msgData[0], sPkwdCountTitleMessageBase[0] + wk->param->movie);
    StrBuf *tagline;
    StrBuf *buf;

    PkwdCount_PrintText(wk, 0, 0x68, 0, title, ALIGN_CENTER, PRINT_COLOR(1, 2, 0));
    GFL_StrBufFree(title);
    lineId = sPkwdCountTaglines[wk->param->movie][wk->param->gender];
    if (wk->param->title != NULL) {
        buf = GFL_StrBufCreate(0x100, wk->heapId);
        func_0202437c(wordSet, 0, wk->param->title, 0, 1, 2);
        tagline = GFL_MsgDataLoadStrbufNew(wk->msgData[0], lineId);
        GFL_WordSetFormatStrbuf(wordSet, buf, tagline);
        PkwdCount_PrintText(wk, 1, 0x68, 0, buf, ALIGN_CENTER, PRINT_COLOR(1, 2, 0));
        GFL_StrBufFree(tagline);
    } else {
        buf = GFL_MsgDataLoadStrbufNew(wk->msgData[0], lineId);
        PkwdCount_PrintText(wk, 1, 0x68, 0, buf, ALIGN_CENTER, PRINT_COLOR(1, 2, 0));
    }
    GFL_StrBufFree(buf);
    GFL_WordSetSystemFree(wordSet);
}

static u32 PkwdCount_GetTitleGraphicIdx(PkwdCountWork *wk) {
    return sPkwdCountTitleGraphics[wk->param->movie];
}
