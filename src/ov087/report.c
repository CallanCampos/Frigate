#include "types.h"
#include "field/report.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "pml/poke_party.h"
#include "save/event_work.h"
#include "save/join_avenue.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/poke_icon.h"
#include "system/printsys.h"
#include "system/rtc.h"
#include "system/str_tool.h"
#include "system/wordset.h"

// The windows, in the order of the lines of message file 359
enum {
    REPORT_WINDOW_NAME,
    REPORT_WINDOW_DATE,
    REPORT_WINDOW_TIME,
    REPORT_WINDOW_LOCATION,
    REPORT_WINDOW_BADGES,
    REPORT_WINDOW_POKEDEX,
    REPORT_WINDOW_PLAY_TIME,
    REPORT_WINDOW_LAST_SAVE,
    REPORT_WINDOW_MAX,
};

// The actors: the party's icons, then the bar's
#define REPORT_BAR_FIRST 6
#define REPORT_BAR_COUNT 10
#define REPORT_ACTOR_MAX (REPORT_BAR_FIRST + REPORT_BAR_COUNT)

// doneFlags
#define REPORT_DONE_ICONS (1 << 0)
#define REPORT_DONE_WINDOWS (1 << 1)

// Frames between two of the bar's icons, and frames without one before the bar is filled at once
#define REPORT_BAR_WAIT 8
#define REPORT_BAR_TIMEOUT 1200

struct ReportScreen {
    GameSystem *gsys;
    SaveControl *save;
    PrintQueue *printQueue;
    PrintWindow windows[REPORT_WINDOW_MAX];
    Font *font;
    ClActUnit *actUnit;
    ClActor *actors[REPORT_ACTOR_MAX];
    // The party's icons' characters, then the bar's at REPORT_BAR_FIRST
    u32 chars[REPORT_BAR_FIRST + 1];
    u8 unused_b0[0x20]; // Never read or written
    // The party's icons' palettes and the bar's
    u32 palettes[2];
    u8 unused_d8[0x2c]; // Never read or written
    u32 cellAnims[2];
    TCB *vblankTcb;
    // Whether the bar fills
    BOOL barActive;
    // What func_020074b8 gives: the size of the blocks that changed and of all of them
    u32 changedSize;
    u32 totalSize;
    // func_0200743c's count, times 256, at which the bar's next icon shows, and the step between two: a tenth of
    // the changed size times 512
    u32 barThreshold;
    u32 barStep;
    u16 barCount;
    u16 barWait;
    u32 lastBarCount;
    int stallFrames;
    u32 doneFlags;
    HeapID heapId;
};

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} ReportWindowSetup;

static void Report_SetupBGs(HeapID heapId);
static void Report_ReleaseBGs(void);
static void Report_LoadGraphics(HeapID heapId);
static void Report_PrintWindows(ReportScreen *wk);
static void Report_FreeWindows(ReportScreen *wk);
static void Report_CreateActors(ReportScreen *wk);
static void Report_FreeActors(ReportScreen *wk);
static void Report_ShowBarIcon(ReportScreen *wk, int index);
static void Report_FillBar(ReportScreen *wk);
static void Report_VBlank(TCB *tcb, void *data);

ReportScreen *Report_Create(GameSystem *gsys, HeapID heapId) {
    ReportScreen *wk = GFL_HeapAllocate(heapId, sizeof(ReportScreen), FALSE, "report.c", 288);

    wk->gsys = gsys;
    wk->heapId = heapId;
    wk->barActive = FALSE;
    wk->doneFlags = 0;
    Report_SetupBGs(wk->heapId);
    Report_LoadGraphics(wk->heapId);
    Report_CreateActors(wk);
    Report_PrintWindows(wk);
    func_02042ba8(FALSE, heapId);
    wk->vblankTcb = GFL_VBlankTCBAdd(Report_VBlank, wk, 0);
    return wk;
}

void Report_Free(ReportScreen *wk) {
    GFL_TCBRemove(wk->vblankTcb);
    Report_FreeWindows(wk);
    Report_FreeActors(wk);
    Report_ReleaseBGs();
    GFL_HeapFree(wk);
}

void Report_Update(ReportScreen *wk) {
    PokeParty *party;
    u32 i;

    if (!(wk->doneFlags & REPORT_DONE_ICONS) && func_02021c0c(wk->printQueue) == TRUE) {
        party = GameData_GetParty(GSYS_GetGameData(wk->gsys));
        for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
            func_0204c124(wk->actors[i], TRUE);
        }
        wk->doneFlags |= REPORT_DONE_ICONS;
    }
}

void Report_UpdateAltFrame(ReportScreen *wk) {
    u32 i;

    if (!(wk->doneFlags & REPORT_DONE_WINDOWS) && func_02021c0c(wk->printQueue) == TRUE) {
        GFL_BGSysSetBGEnabled(4, TRUE);
        GFL_BGSysSetBGEnabled(5, TRUE);
        GFL_BGSysSetBGEnabled(6, TRUE);
        for (i = 0; i < REPORT_WINDOW_MAX; i++) {
            BmpWin_FlushMap(wk->windows[i].window);
        }
        GFL_BGSysQueueScrLoad(5);
        wk->doneFlags |= REPORT_DONE_WINDOWS;
    }
    func_02021a3c(wk->printQueue);
    for (i = 0; i < REPORT_WINDOW_MAX; i++) {
        PrintWindow_Flush(&wk->windows[i], wk->printQueue);
    }
}

BOOL Report_IsReady(ReportScreen *wk) {
    if (wk->doneFlags == (REPORT_DONE_ICONS | REPORT_DONE_WINDOWS)) {
        return TRUE;
    }
    return FALSE;
}

void Report_InitSave(ReportScreen *wk) {
    wk->save = GameData_GetSaveControl(GSYS_GetGameData(wk->gsys));
    func_020074b8(wk->save, &wk->changedSize, &wk->totalSize);
    wk->barThreshold = wk->barStep = (wk->changedSize << 9) / REPORT_BAR_COUNT;
    wk->barCount = 0;
    wk->barWait = 0;
}

BOOL Report_IsLargeSave(ReportScreen *wk) {
    if (wk->changedSize >= wk->totalSize / 3) {
        return TRUE;
    }
    return FALSE;
}

void Report_StartBar(ReportScreen *wk) {
    BmpWin *window = wk->windows[REPORT_WINDOW_LAST_SAVE].window;
    u32 i;

    BmpWin_ClearScreen(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
    for (i = REPORT_BAR_FIRST; i <= REPORT_ACTOR_MAX - 1; i++) {
        func_0204c124(wk->actors[i], TRUE);
    }
    wk->barActive = TRUE;
}

BOOL Report_IsBarFull(ReportScreen *wk) {
    if (wk->barCount == REPORT_BAR_COUNT) {
        wk->barActive = FALSE;
        return TRUE;
    }
    return FALSE;
}

void Report_StopBar(ReportScreen *wk) {
    wk->barActive = FALSE;
}

static void Report_SetupBGs(HeapID heapId) {
    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysSetBGEnabled(5, FALSE);
    GFL_BGSysSetBGEnabled(6, FALSE);
    GFL_BGSysSetBGEnabled(7, FALSE);
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x3800),
            GX_BG_CHARBASE(0x00000),
            0x2800,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(4);
        GFL_BGSysClearCharCore(4, 0x20, 0, heapId);
        GFL_BGSysLoadScr(4);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x3000),
            GX_BG_CHARBASE(0x04000),
            0x4000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(5);
        GFL_BGSysClearCharCore(5, 0x20, 0, heapId);
        GFL_BGSysLoadScr(5);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x2800),
            GX_BG_CHARBASE(0x04000),
            0x4000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(6);
        GFL_BGSysClearCharCore(6, 0x20, 0, heapId);
        GFL_BGSysLoadScr(6);
    }
}

static void Report_ReleaseBGs(void) {
    GFL_BGSysSetBGEnabled(6, FALSE);
    GFL_BGSysSetBGEnabled(5, FALSE);
    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
}

static void Report_LoadGraphics(HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(193, heapId);

    // Each version has its own palette
#ifdef BLACK2
    GFL_G2DIOLoadArcNCLRDefault(arc, 3, PALTYPE_SUB_BG, 0, 0x60, heapId);
#else
    GFL_G2DIOLoadArcNCLRDefault(arc, 2, PALTYPE_SUB_BG, 0, 0x60, heapId);
#endif
    GFL_BGSysResetStdPalette(4, 0);
    GFL_BGSysLoadArcNCGRStatic(arc, 1, 4, 0, 0, TRUE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 0, 4, 0, 0, TRUE, heapId);
    GFL_ArcToolFree(arc);
}

static const ReportWindowSetup sWindowSetups[REPORT_WINDOW_MAX] = {
    { 5, 1, 0, 15, 3, 2 },  { 5, 3, 7, 10, 2, 2 },   { 5, 14, 7, 5, 2, 2 },  { 5, 3, 9, 15, 2, 2 },
    { 5, 3, 16, 12, 2, 2 }, { 5, 16, 16, 12, 2, 2 }, { 5, 3, 18, 16, 2, 2 }, { 5, 1, 21, 30, 3, 2 },
};

static void Report_PrintWindows(ReportScreen *wk) {
    MsgData *msgData;
    WordSet *wordSet;
    StrBuf *strbuf;
    StrBuf *fmt;
    GameData *gameData;
    SaveControl *save;
    u32 i;

    wk->printQueue = func_02021998(wk->heapId);
    for (i = 0; i < REPORT_WINDOW_MAX; i++) {
        wk->windows[i].window = BmpWin_CreateDynamic(
            sWindowSetups[i].bg, sWindowSetups[i].x, sWindowSetups[i].y, sWindowSetups[i].width,
            sWindowSetups[i].height, sWindowSetups[i].palette, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[i].window), 0);
    }

    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, wk->heapId);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0359, wk->heapId);
    wordSet = GFL_WordSetSystemCreateDefault(wk->heapId);
    strbuf = GFL_StrBufCreate(0x100, wk->heapId);
    gameData = GSYS_GetGameData(wk->gsys);
    save = GameData_GetSaveControl(gameData);

    fmt = GFL_MsgDataLoadStrbufNew(msgData, REPORT_WINDOW_NAME);
    copyVarForText(wordSet, 0, GetGameDataPlayerInfo(gameData));
    GFL_WordSetFormatStrbuf(wordSet, strbuf, fmt);
    PrintWindow_Print(&wk->windows[REPORT_WINDOW_NAME], wk->printQueue, 0, 4, strbuf, wk->font, PRINT_COLOR(13, 12, 0));
    GFL_StrBufFree(fmt);

    {
        RTCDate date;

        fmt = GFL_MsgDataLoadStrbufNew(msgData, REPORT_WINDOW_DATE);
        func_0207cc10(&date);
        WordSetNumber(wordSet, 0, date.year, 2, NUM_PAD_ZERO, TRUE);
        WordSetNumber(wordSet, 1, date.month, 2, NUM_PAD_NONE, TRUE);
        WordSetNumber(wordSet, 2, date.day, 2, NUM_PAD_NONE, TRUE);
        GFL_WordSetFormatStrbuf(wordSet, strbuf, fmt);
        PrintWindow_Print(&wk->windows[REPORT_WINDOW_DATE], wk->printQueue, 0, 0, strbuf, wk->font,
                          PRINT_COLOR(15, 14, 0));
        GFL_StrBufFree(fmt);
    }

    {
        RTCTime time;

        fmt = GFL_MsgDataLoadStrbufNew(msgData, REPORT_WINDOW_TIME);
        func_0207cc80(&time);
        WordSetNumber(wordSet, 0, time.hour, 2, NUM_PAD_NONE, TRUE);
        WordSetNumber(wordSet, 1, time.minute, 2, NUM_PAD_ZERO, TRUE);
        GFL_WordSetFormatStrbuf(wordSet, strbuf, fmt);
        PrintWindow_Print(&wk->windows[REPORT_WINDOW_TIME], wk->printQueue, 0, 0, strbuf, wk->font,
                          PRINT_COLOR(15, 14, 0));
        GFL_StrBufFree(fmt);
    }

    {
        u16 zoneId;

        fmt = GFL_MsgDataLoadStrbufNew(msgData, REPORT_WINDOW_LOCATION);
        zoneId = PlayerState_GetZoneID(GSYS_GetPlayerState(wk->gsys));
        // The Join Avenue is shown by the name the player gave it
        if (IsZoneJoinAvenue(zoneId) || IsZoneJoinAvenueSubZone(zoneId)) {
            JoinAvenueInfo *joinAvenueInfo =
                JoinAvenue_GetInfo(SaveControl_GetJoinAvenue(GameData_GetSaveControl(GSYS_GetGameData(wk->gsys))));
            StrBuf *name = GFL_StrBufCreate(64, wk->heapId);

            GFL_StrBufLoadString(name, (const u16 *)JoinAvenue_GetParam(joinAvenueInfo, 0, 0));
            func_0202437c(wordSet, 0, name, 2, 1, 2);
            GFL_StrBufFree(name);
        } else {
            loadLocationNameToStrbuf(wordSet, 0, ZoneData_GetPlaceNameID(zoneId));
        }
        GFL_WordSetFormatStrbuf(wordSet, strbuf, fmt);
        PrintWindow_Print(&wk->windows[REPORT_WINDOW_LOCATION], wk->printQueue, 0, 0, strbuf, wk->font,
                          PRINT_COLOR(15, 14, 0));
        GFL_StrBufFree(fmt);
    }

    fmt = GFL_MsgDataLoadStrbufNew(msgData, REPORT_WINDOW_BADGES);
    WordSetNumber(wordSet, 0, getBadgeCount(getTrainerCardDataBlkAddress(gameData)), 2, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(wordSet, strbuf, fmt);
    PrintWindow_Print(&wk->windows[REPORT_WINDOW_BADGES], wk->printQueue, 0, 0, strbuf, wk->font,
                      PRINT_COLOR(15, 14, 0));
    GFL_StrBufFree(fmt);

    // The Pokédex after flag 0x962 is set
    if (EventWork_FlagGet(GameData_GetEventWork(GSYS_GetGameData(wk->gsys)), 0x962) == TRUE) {
        fmt = GFL_MsgDataLoadStrbufNew(msgData, REPORT_WINDOW_POKEDEX);
        WordSetNumber(wordSet, 0, countSeenDexPokes(GameData_GetPokedex(gameData), wk->heapId), 3, NUM_PAD_NONE, TRUE);
        GFL_WordSetFormatStrbuf(wordSet, strbuf, fmt);
        PrintWindow_Print(&wk->windows[REPORT_WINDOW_POKEDEX], wk->printQueue, 0, 0, strbuf, wk->font,
                          PRINT_COLOR(15, 14, 0));
        GFL_StrBufFree(fmt);
    }

    {
        PlayTime *playTime;

        fmt = GFL_MsgDataLoadStrbufNew(msgData, REPORT_WINDOW_PLAY_TIME);
        playTime = func_02017a40(gameData);
        WordSetNumber(wordSet, 0, func_02008cec(playTime), 3, NUM_PAD_NONE, TRUE);
        WordSetNumber(wordSet, 1, func_02008cf0(playTime), 2, NUM_PAD_ZERO, TRUE);
        GFL_WordSetFormatStrbuf(wordSet, strbuf, fmt);
        PrintWindow_Print(&wk->windows[REPORT_WINDOW_PLAY_TIME], wk->printQueue, 0, 0, strbuf, wk->font,
                          PRINT_COLOR(15, 14, 0));
        GFL_StrBufFree(fmt);
    }

    // The date and time of the last save, when there is one
    if (!func_0200746c(save) && SaveControl_IsDataAlreadyPresent(save) == TRUE) {
        PlayTime *saveTime = func_02008de8(save);

        fmt = GFL_MsgDataLoadStrbufNew(msgData, REPORT_WINDOW_LAST_SAVE);
        WordSetNumber(wordSet, 0, func_02008d68(saveTime), 2, NUM_PAD_ZERO, TRUE);
        WordSetNumber(wordSet, 1, func_02008d70(saveTime), 2, NUM_PAD_NONE, TRUE);
        WordSetNumber(wordSet, 2, func_02008d78(saveTime), 2, NUM_PAD_NONE, TRUE);
        WordSetNumber(wordSet, 3, func_02008d80(saveTime), 2, NUM_PAD_NONE, TRUE);
        WordSetNumber(wordSet, 4, func_02008d88(saveTime), 2, NUM_PAD_ZERO, TRUE);
        GFL_WordSetFormatStrbuf(wordSet, strbuf, fmt);
        PrintWindow_Print(&wk->windows[REPORT_WINDOW_LAST_SAVE], wk->printQueue, 0, 4, strbuf, wk->font,
                          PRINT_COLOR(13, 12, 0));
        GFL_StrBufFree(fmt);
    } else {
        BmpWin_FlushChar(wk->windows[REPORT_WINDOW_LAST_SAVE].window);
    }

    GFL_StrBufFree(strbuf);
    GFL_WordSetSystemFree(wordSet);
    GFL_MsgDataFree(msgData);
}

static void Report_FreeWindows(ReportScreen *wk) {
    u32 i;

    GFL_FontFree(wk->font);
    for (i = 0; i < REPORT_WINDOW_MAX; i++) {
        BmpWin_Free(wk->windows[i].window);
    }
    func_02021a18(wk->printQueue);
}

// The first of the party's icons, and the first of the bar's
static const ClActorSetup sReportActorSetups[2] = {
    { 40, 104, 0, 0, 0 },
    { 74, 180, 0, 0, 0 },
};

static void Report_CreateActors(ReportScreen *wk) {
    PokeParty *party = GameData_GetParty(GSYS_GetGameData(wk->gsys));
    u32 count = PokeParty_GetPkmCount(party);
    ArcTool *arc;
    ClActorSetup setup;
    u32 i;

    wk->actUnit = func_0204bf1c(REPORT_ACTOR_MAX, 0, wk->heapId);

    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEICON, wk->heapId);
    for (i = 0; i < count; i++) {
        wk->chars[i] = func_0204b81c(arc, func_02020f40(func_0201d624(PokeParty_GetPkm(party, i))), FALSE,
                                     CLACT_VRAM_SUB, wk->heapId);
    }
    wk->palettes[0] = func_0204bc48(arc, func_02021114(), CLACT_VRAM_SUB, 0, wk->heapId);
    wk->cellAnims[0] = func_0204bde0(arc, func_0202111c(), getOBJTileMapping_MainEng(), wk->heapId);
    GFL_ArcToolFree(arc);

    arc = GFL_ArcSysCreateFileHandle(193, wk->heapId);
    wk->chars[REPORT_BAR_FIRST] = func_0204b81c(arc, 4, TRUE, CLACT_VRAM_SUB, wk->heapId);
    wk->palettes[1] = func_0204bbb8(arc, 5, CLACT_VRAM_SUB, 0x60, 0, 1, wk->heapId);
    wk->cellAnims[1] = func_0204bde0(arc, 6, 7, wk->heapId);
    GFL_ArcToolFree(arc);

    setup = sReportActorSetups[0];
    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        wk->actors[i] = func_0204c040(wk->actUnit, wk->chars[i], wk->palettes[0], wk->cellAnims[0], &setup,
                                      CLACT_SURFACE_SUB, wk->heapId);
        func_0204c124(wk->actors[i], FALSE);
        setup.x += 32;
        func_0204c378(wk->actors[i], func_020210c0(func_0201d624(pkm)), TRUE);
    }

    setup = sReportActorSetups[1];
    for (i = REPORT_BAR_FIRST; i <= REPORT_ACTOR_MAX - 1; i++) {
        wk->actors[i] = func_0204c040(wk->actUnit, wk->chars[REPORT_BAR_FIRST], wk->palettes[1], wk->cellAnims[1],
                                      &setup, CLACT_SURFACE_SUB, wk->heapId);
        func_0204c124(wk->actors[i], FALSE);
        setup.x += 12;
    }

    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

static void Report_FreeActors(ReportScreen *wk) {
    PokeParty *party = GameData_GetParty(GSYS_GetGameData(wk->gsys));
    u32 i;

    for (i = 0; i < PokeParty_GetPkmCount(party); i++) {
        func_0204c108(wk->actors[i]);
        func_0204b98c(wk->chars[i]);
    }
    for (i = REPORT_BAR_FIRST; i <= REPORT_ACTOR_MAX - 1; i++) {
        func_0204c108(wk->actors[i]);
    }
    func_0204b98c(wk->chars[REPORT_BAR_FIRST]);
    func_0204bcd0(wk->palettes[0]);
    func_0204bcd0(wk->palettes[1]);
    func_0204be64(wk->cellAnims[0]);
    func_0204be64(wk->cellAnims[1]);
    func_0204bf98(wk->actUnit);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
}

// Shows one of the bar's icons, from the start of its animation
static void Report_ShowBarIcon(ReportScreen *wk, int index) {
    func_0204c4d4(wk->actors[REPORT_BAR_FIRST + index], 0);
    func_0204c488(wk->actors[REPORT_BAR_FIRST + index], 1);
    func_0204c520(wk->actors[REPORT_BAR_FIRST + index], TRUE);
}

static void Report_FillBar(ReportScreen *wk) {
    int i;

    for (i = 0; i < REPORT_BAR_COUNT; i++) {
        Report_ShowBarIcon(wk, i);
    }
}

// Fills the bar as the save is written, an icon at most every REPORT_BAR_WAIT frames, and all at once when it hasn't
// moved for REPORT_BAR_TIMEOUT frames
static void Report_VBlank(TCB *tcb, void *data) {
    ReportScreen *wk = data;
    u32 written;

    if (wk->barActive) {
        written = func_0200743c(wk->save) << 8;
        if (wk->barWait != REPORT_BAR_WAIT) {
            wk->barWait++;
        }
        if (written >= wk->barThreshold && wk->barWait == REPORT_BAR_WAIT && wk->barCount < REPORT_BAR_COUNT) {
            wk->barThreshold += wk->barStep;
            wk->barWait = 0;
            Report_ShowBarIcon(wk, wk->barCount);
            wk->barCount++;
        }
        if (wk->barCount == wk->lastBarCount) {
            wk->stallFrames++;
        } else {
            wk->lastBarCount = wk->barCount;
            wk->stallFrames = 0;
        }
        if (wk->stallFrames >= REPORT_BAR_TIMEOUT) {
            Report_FillBar(wk);
            wk->barCount = REPORT_BAR_COUNT;
        }
        func_0204b794();
        func_0204b7c8();
    }
}
