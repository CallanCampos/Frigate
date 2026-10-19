#include "types.h"
#include "app/beacon_detail.h"
#include "app/beacon_detail_gra.h"
#include "app/beacon_detail_local.h"
#include "app/townmap.h"
#include "app/townmap/townmap_data.h"
#include "app/ui/touchbar.h"
#include "app/ui/ui_scene.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "pml/poke_graphic.h"
#include "system/beacon_status.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The screen of a beacon's details: the application that the C-Gear's log of passersby opens for one of its
// beacons. The upper screen shows the sender's profile, the touch screen where the sender is on the map and what the
// sender was doing, and the touch bar turns to the log's other beacons. The ROM has no name for this file;
// "beacon_detail.c" is a guess from its graphic file's, "beacon_detail_gra.c"

// How many beacons the log's list shows at once
#define LOG_LIST_ROWS 4

#define TRAINER_PALETTE 8

// The files of a set of cell actor graphics, and where they go
typedef struct {
    u8 vramType;
    u8 paletteOffset;
    u8 paletteCount;
    u16 paletteFile;
    u16 charFile;
    // The animations follow it
    u16 cellFile;
} BeaconDetailObjResFiles;

static BOOL BeaconDetail_Init(GameProc *proc, u32 *state, void *param, void *data);
static BOOL BeaconDetail_Exit(GameProc *proc, u32 *state, void *param, void *data);
static BOOL BeaconDetail_Main(GameProc *proc, u32 *state, void *param, void *data);
static u32 BeaconDetail_SeqInput(BeaconDetailWork *work);
static u32 BeaconDetail_SeqWait(BeaconDetailWork *work);
static u32 BeaconDetail_SeqInit(BeaconDetailWork *work);
static u32 BeaconDetail_SeqFadeIn(BeaconDetailWork *work);
static u32 BeaconDetail_SeqFadeOut(BeaconDetailWork *work);
static void BeaconDetail_InitData(BeaconDetailWork *work);
static void BeaconDetail_FreeData(BeaconDetailWork *work);
static void BeaconDetail_InitSystems(BeaconDetailWork *work);
static void BeaconDetail_FreeSystems(BeaconDetailWork *work);
static void BeaconDetail_LoadBGs(BeaconDetailWork *work, HeapID heapId);
static void BeaconDetail_FreeBGs(BeaconDetailWork *work);
static TouchBar *BeaconDetail_CreateTouchBar(ClActUnit *unit, HeapID heapId);
static void BeaconDetail_FreeTouchBar(TouchBar *touchBar);
static void BeaconDetail_UpdateTouchBar(TouchBar *touchBar);
static void BeaconDetail_LoadObjRes(BeaconDetailWork *work, BeaconDetailObjRes *res,
                                    const BeaconDetailObjResFiles *files, ArcTool *arc);
static void BeaconDetail_FreeObjRes(BeaconDetailWork *work, BeaconDetailObjRes *res);
static void BeaconDetail_LoadObjs(BeaconDetailWork *work, ArcTool *arc);
static void BeaconDetail_FreeObjs(BeaconDetailWork *work);
static void BeaconDetail_InitWindow(BeaconDetailWindow *window, u8 bg, u8 palette, u8 x, u8 y, u8 width, u8 height);
static ClActor *BeaconDetail_AddActor(BeaconDetailWork *work, ClActUnit *unit, BeaconDetailObjRes *res, s16 x, s16 y,
                                      u8 anim, u8 priority, u8 bgPriority, u16 surface);
static void BeaconDetail_CreateMapActors(BeaconDetailWork *work);
static void BeaconDetail_FreeMapActors(BeaconDetailWork *work);
static void BeaconDetail_CreateInfoWindow(BeaconDetailWork *work);
static void BeaconDetail_FreeInfoWindow(BeaconDetailWork *work);
static void BeaconDetail_CreatePanels(BeaconDetailWork *work);
static void BeaconDetail_FreePanels(BeaconDetailWork *work);
static void BeaconDetail_UpdatePMSDraw(BeaconDetailWork *work);

// The widths of the windows of a panel's name, job, hobby and play time
static const u8 sBeaconDetailWindowWidths[] = { 19, 19, 19, 11 };

static const TouchBarItem sBeaconDetailTouchBarItems[] = {
    { BEACON_DETAIL_ICON_RETURN, { 232, 168 } },
    { BEACON_DETAIL_ICON_NEXT, { 200, 168 } },
    { BEACON_DETAIL_ICON_PREV, { 168, 168 } },
};

static BOOL BeaconDetail_Init(GameProc *proc, u32 *state, void *param, void *data) {
    BeaconDetailWork *work;

    GFL_OvlLoad(OVERLAY_APP_UI);
    GFL_OvlLoad(OVERLAY_TOWNMAP);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_BEACON_DETAIL, 0x30000);
    work = GFL_ProcInitSubsystem(proc, sizeof(BeaconDetailWork), HEAPID_BEACON_DETAIL);
    MI_CpuFill8(work, 0, sizeof(BeaconDetailWork));
    work->param = param;
    work->heapId = HEAPID_BEACON_DETAIL;
    work->tailHeapId = HEAPID_TAIL(HEAPID_BEACON_DETAIL);
    work->graphic = BeaconDetailGraphic_Create(0, work->heapId);
    BeaconDetail_InitData(work);
    BeaconDetail_InitSystems(work);
    BeaconDetail_LoadBGs(work, work->heapId);
    BeaconDetail_CreateInfoWindow(work);
    BeaconDetail_LoadObjs(work, work->arc);
    BeaconDetail_CreateMapActors(work);
    BeaconDetail_CreatePanels(work);
    work->touchBar = BeaconDetail_CreateTouchBar(BeaconDetailGraphic_GetClActUnit(work->graphic), work->heapId);
    return TRUE;
}

static BOOL BeaconDetail_Exit(GameProc *proc, u32 *state, void *param, void *data) {
    BeaconDetailWork *work = data;

    BeaconDetail_FreeTouchBar(work->touchBar);
    BeaconDetail_FreePanels(work);
    BeaconDetail_FreeMapActors(work);
    BeaconDetail_FreeObjs(work);
    BeaconDetail_FreeInfoWindow(work);
    BeaconDetail_FreeBGs(work);
    BeaconDetail_FreeSystems(work);
    BeaconDetail_FreeData(work);
    BeaconDetailGraphic_Free(work->graphic);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_BEACON_DETAIL);
    GFL_OvlUnload(OVERLAY_TOWNMAP);
    GFL_OvlUnload(OVERLAY_APP_UI);
    return TRUE;
}

static BOOL BeaconDetail_Main(GameProc *proc, u32 *state, void *param, void *data) {
    BeaconDetailWork *work = data;

    if (work->empty) {
        return TRUE;
    }
    if ((int)*state >= BEACON_DETAIL_SEQ_INPUT && (int)*state <= BEACON_DETAIL_SEQ_WAIT) {
        BeaconDetail_UpdateTouchBar(work->touchBar);
    }
    switch (*state) {
    case BEACON_DETAIL_SEQ_INIT:
        *state = BeaconDetail_SeqInit(work);
        break;
    case BEACON_DETAIL_SEQ_FADE_IN:
        *state = BeaconDetail_SeqFadeIn(work);
        break;
    case BEACON_DETAIL_SEQ_INPUT:
        *state = BeaconDetail_SeqInput(work);
        break;
    case BEACON_DETAIL_SEQ_WAIT:
        *state = BeaconDetail_SeqWait(work);
        break;
    case BEACON_DETAIL_SEQ_FADE_OUT:
        *state = BeaconDetail_SeqFadeOut(work);
        break;
    case BEACON_DETAIL_SEQ_END:
        return TRUE;
    }
    GFL_TCBExMgrUpdate(work->tcbManager);
    func_02021a3c(work->printQueue);
    BeaconDetail_UpdatePMSDraw(work);
    BeaconDetailGraphic_Update(work->graphic);
    return FALSE;
}

static u32 BeaconDetail_SeqInput(BeaconDetailWork *work) {
    return BeaconDetailSub_Input(work);
}

static u32 BeaconDetail_SeqWait(BeaconDetailWork *work) {
    if (work->taskCount) {
        return BEACON_DETAIL_SEQ_WAIT;
    }
    BeaconDetailSub_EndWait(work);
    return BEACON_DETAIL_SEQ_INPUT;
}

static u32 BeaconDetail_SeqInit(BeaconDetailWork *work) {
    switch (work->seq) {
    case 0:
        BeaconDetailSub_Start(work);
        work->seq++;
        break;
    case 1:
        if (func_02021c0c(work->printQueue)) {
            work->seq = 0;
            return BEACON_DETAIL_SEQ_FADE_IN;
        }
        break;
    }
    return BEACON_DETAIL_SEQ_INIT;
}

static u32 BeaconDetail_SeqFadeIn(BeaconDetailWork *work) {
    switch (work->seq) {
    case 0:
        GFL_FadeSet(3, 16, 0, 0);
        work->seq++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            work->seq = 0;
            return BEACON_DETAIL_SEQ_INPUT;
        }
        break;
    }
    return BEACON_DETAIL_SEQ_FADE_IN;
}

static u32 BeaconDetail_SeqFadeOut(BeaconDetailWork *work) {
    switch (work->seq) {
    case 0:
        // The return button's animation ends first, unless the game itself ends the screen
        if (!work->param->unk0A) {
            BeaconDetail_UpdateTouchBar(work->touchBar);
            if (TouchBar_GetDecided(work->touchBar) != BEACON_DETAIL_ICON_RETURN) {
                break;
            }
        }
        GFL_FadeSet(3, 0, 16, 0);
        work->seq++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            work->seq = 0;
            return BEACON_DETAIL_SEQ_END;
        }
        break;
    }
    return BEACON_DETAIL_SEQ_FADE_OUT;
}

// Lists the log's beacons that the screen can show, and finds the one it was opened for
static void BeaconDetail_InitData(BeaconDetailWork *work) {
    int i;
    int count;

    work->beacon = GameBeacon_Alloc(work->heapId);
    work->beaconStatus = func_020174d4(work->param->gameData);
    work->infoTable = func_0202d7a8(work->beaconStatus);
    count = GameBeaconInfoTable_GetCount(work->infoTable);
    work->index = 0xff;
    for (i = 0; i < count; i++) {
        GameBeaconInfoTable_RingGet(work->infoTable, work->beacon, &work->time, i);
        if (GameBeacon_IsInvalid(work->beacon) || GameBeacon_IsGiftType(work->beacon)) {
            continue;
        }
        if (work->param->subscreen == i) {
            work->index = work->count;
        }
        work->logIndices[work->count++] = i;
    }
    if (work->index == 0xff || work->index >= work->count) {
        work->index = 0;
    }
    if (work->count == 0) {
        work->empty = TRUE;
    }
    work->startLogIndex = work->logIndices[work->index];
    work->townMapData = TownMapData_Load(work->heapId);
}

// Scrolls the log's list to the beacon shown last, when it is not among the rows that the list showed
static void BeaconDetail_FreeData(BeaconDetailWork *work) {
    int logIndex = work->logIndices[work->index];
    int count = GameBeaconInfoTable_GetCount(work->infoTable);
    int row = work->param->mode;
    int last = work->startLogIndex + (LOG_LIST_ROWS - 1 - row);
    int first = work->startLogIndex - row;

    if (logIndex < first || logIndex > last) {
        if (count - logIndex < LOG_LIST_ROWS) {
            logIndex = count - LOG_LIST_ROWS;
        }
        func_0202d7b8(work->beaconStatus, logIndex);
    }
    TownMapData_Free(work->townMapData);
    GFL_HeapFree(work->beacon);
}

static void BeaconDetail_InitSystems(BeaconDetailWork *work) {
    work->tcbManager = GFL_TCBExMgrCreate(work->heapId, work->heapId, 64, 128);
    work->arc = GFL_ArcSysCreateFileHandle(ARCID_BEACON, work->heapId);
    GFL_TextRndUpdateColorIndexLUT(7, 8, 15);
    work->printQueue = func_020219a8(0x800, work->heapId);
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, work->heapId);
    work->wordSet = GFL_WordSetSystemCreate(8, 64, work->heapId);
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_STARTMENU, work->heapId);
    work->countryMsgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_EVENT_MAPCHANGE_LOAD_COUNTRY_TO_STRBUF, work->heapId);
    work->formatBuf = GFL_StrBufCreate(145, work->heapId);
    work->textBuf = GFL_StrBufCreate(145, work->heapId);
    work->unk144 = GFL_StrBufCreate(145, work->heapId);
}

static void BeaconDetail_FreeSystems(BeaconDetailWork *work) {
    GFL_StrBufFree(work->unk144);
    GFL_StrBufFree(work->textBuf);
    GFL_StrBufFree(work->formatBuf);
    GFL_MsgDataFree(work->countryMsgData);
    GFL_MsgDataFree(work->msgData);
    GFL_WordSetSystemFree(work->wordSet);
    GFL_FontFree(work->font);
    func_02021c44(work->printQueue);
    func_02021a18(work->printQueue);
    func_020232d8();
    GFL_TCBExMgrFree(work->tcbManager);
    GFL_ArcToolFree(work->arc);
}

// The profile's panels on the upper screen's BGs, and the town map with the frame of the info window on the touch
// screen's
static void BeaconDetail_LoadBGs(BeaconDetailWork *work, HeapID heapId) {
    ArcTool *townMapArc = GFL_ArcSysCreateFileHandle(ARCID_TOWNMAP_GRA, heapId);

    GFL_G2DIOLoadArcNCLRDefault(townMapArc, 0, 0, 0, 0x200, heapId);
    GFL_G2DIOLoadArcNCLRDefault(work->arc, 28, 4, 0, 0x200, heapId);
    GFL_BGSysLoadArcNCGRStatic(work->arc, 30, 4, 0, 0, TRUE, heapId);
    GFL_BGSysLoadArcNCGRStatic(work->arc, 30, 5, 0, 0, TRUE, heapId);
    GFL_BGSysLoadArcNCGRStatic(work->arc, 30, 7, 0, 0, TRUE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(work->arc, 32, 4, 0, 0, TRUE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(work->arc, 32, 5, 0, 0, TRUE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(work->arc, 33, 7, 0, 0, TRUE, heapId);
    GFL_BGSysSetScrPaletteNo(5, 0, 0, 32, 24, 1);
    GFL_BGSysLoadScr(5);
    GFL_BGSysLoadArcNCGRStatic(work->arc, 31, 1, 0, 0, TRUE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(work->arc, 34, 1, 0, 0x800, TRUE, heapId);
    GFL_BGSysLoadArcNCGRStatic(townMapArc, 5, 2, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(townMapArc, 17, 2, 0, 0x800, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(townMapArc, 7, 3, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(townMapArc, 16, 3, 0, 0x800, FALSE, heapId);
    work->versionPalette.file = GFL_ArcToolReadHeapNew(work->arc, 29, work->heapId);
    NNS_G2dGetUnpackedPaletteData(work->versionPalette.file, &work->versionPalette.data);
    work->versionPalette.raw = work->versionPalette.data->rawData;
    GFL_ArcToolFree(townMapArc);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG1,
                       GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ, 11, 5);
    G2x_SetBlendAlpha_(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG1, GX_PLANEMASK_ALL, 13, 3);
}

static void BeaconDetail_FreeBGs(BeaconDetailWork *work) {
    GFL_HeapFree(work->versionPalette.file);
    G2S_BlendNone();
    G2_BlendNone();
}

static TouchBar *BeaconDetail_CreateTouchBar(ClActUnit *unit, HeapID heapId) {
    TouchBarSetup setup;

    MI_CpuFill8(&setup, 0, sizeof(TouchBarSetup));
    setup.items = (TouchBarItem *)sBeaconDetailTouchBarItems;
    setup.count = NELEMS(sBeaconDetailTouchBarItems);
    setup.unit = unit;
    setup.bg = 0;
    setup.bgPalette = 13;
    setup.objPalette = 11;
    setup.vramType = CLACT_VRAM_MAIN;
    return TouchBar_Create(&setup, heapId);
}

static void BeaconDetail_FreeTouchBar(TouchBar *touchBar) {
    TouchBar_Free(touchBar);
}

static void BeaconDetail_UpdateTouchBar(TouchBar *touchBar) {
    TouchBar_Main(touchBar);
}

static void BeaconDetail_LoadObjRes(BeaconDetailWork *work, BeaconDetailObjRes *res,
                                    const BeaconDetailObjResFiles *files, ArcTool *arc) {
    MI_CpuFill8(res, 0, sizeof(BeaconDetailObjRes));
    res->palette = func_0204bbb8(arc, files->paletteFile, files->vramType, files->paletteOffset * 0x20, 0,
                                 files->paletteCount, work->heapId);
    res->chars = func_0204b81c(arc, files->charFile, FALSE, files->vramType, work->heapId);
    res->cellAnims = func_0204bde0(arc, files->cellFile, files->cellFile + 1, work->heapId);
}

static void BeaconDetail_FreeObjRes(BeaconDetailWork *work, BeaconDetailObjRes *res) {
    func_0204be64(res->cellAnims);
    func_0204b98c(res->chars);
    func_0204bcd0(res->palette);
}

// The icons, the map's trainer with every view's characters and the palettes of the views, and a trainer picture
// for each panel
static void BeaconDetail_LoadObjs(BeaconDetailWork *work, ArcTool *arc) {
    ArcTool *unionArc;
    int i;

    work->trainerArc = MakeTrGraArcHandle(work->heapId);
    {
        BeaconDetailObjResFiles files = { CLACT_VRAM_BOTH, 0, 3, 24, 25, 26 };

        BeaconDetail_LoadObjRes(work, &work->iconRes, &files, arc);
    }
    {
        BeaconDetailObjResFiles files = { CLACT_VRAM_MAIN, 10, 1, 0, 49, 65 };

        unionArc = GFL_ArcSysCreateFileHandle(ARCID_WIFI_UNIONCHAR, work->tailHeapId);
        BeaconDetail_LoadObjRes(work, &work->unionRes, &files, unionArc);
    }
    work->unionPalette.file = GFL_ArcToolReadHeapNew(unionArc, 0, work->heapId);
    NNS_G2dGetUnpackedPaletteData(work->unionPalette.file, &work->unionPalette.data);
    work->unionPalette.raw = work->unionPalette.data->rawData;
    for (i = 0; i < BEACON_DETAIL_UNION_VIEW_COUNT; i++) {
        work->unionChars[i].file = GFL_ArcToolReadHeapNew(unionArc, i + 49, work->heapId);
        NNS_G2dGetUnpackedCharacterData(work->unionChars[i].file, &work->unionChars[i].data);
    }
    GFL_ArcToolFree(unionArc);
    for (i = 0; i < BEACON_DETAIL_PANEL_COUNT; i++) {
        work->trainerRes[i].chars = TrGra_LoadClActChars(work->trainerArc, i + 2, CLACT_VRAM_SUB, work->heapId);
        work->trainerRes[i].palette =
            TrGra_LoadClActPalette(work->trainerArc, i + 2, CLACT_VRAM_SUB, (i + TRAINER_PALETTE) * 0x20, work->heapId);
        work->trainerRes[i].cellAnims = TrGra_LoadClActCellAnims(i + 2, 0, CLACT_VRAM_SUB, work->heapId);
    }
}

static void BeaconDetail_FreeObjs(BeaconDetailWork *work) {
    int i;

    for (i = 0; i < BEACON_DETAIL_PANEL_COUNT; i++) {
        BeaconDetail_FreeObjRes(work, &work->trainerRes[i]);
    }
    for (i = 0; i < BEACON_DETAIL_UNION_VIEW_COUNT; i++) {
        GFL_HeapFree(work->unionChars[i].file);
    }
    GFL_HeapFree(work->unionPalette.file);
    BeaconDetail_FreeObjRes(work, &work->unionRes);
    BeaconDetail_FreeObjRes(work, &work->iconRes);
    GFL_ArcToolFree(work->trainerArc);
}

static void BeaconDetail_InitWindow(BeaconDetailWindow *window, u8 bg, u8 palette, u8 x, u8 y, u8 width, u8 height) {
    window->window = BmpWin_CreateDynamic(bg, x, y, width, height, palette, TRUE);
    window->bitmap = BmpWin_GetBitmap(window->window);
    PrintWindow_Init(&window->print, window->window);
    GFL_BitmapFill(window->bitmap, 3);
    BmpWin_TransferNow(window->window);
}

static ClActor *BeaconDetail_AddActor(BeaconDetailWork *work, ClActUnit *unit, BeaconDetailObjRes *res, s16 x, s16 y,
                                      u8 anim, u8 priority, u8 bgPriority, u16 surface) {
    ClActorSetup setup;

    setup.x = x;
    setup.y = y;
    setup.sequence = anim;
    setup.bgPriority = bgPriority;
    setup.priority = priority;
    return func_0204c040(unit, res->chars, res->palette, res->cellAnims, &setup, surface, work->heapId);
}

static void BeaconDetail_CreateMapActors(BeaconDetailWork *work) {
    ClActUnit *unit = BeaconDetailGraphic_GetClActUnit(work->graphic);

    work->actors[BEACON_DETAIL_ACTOR_TRAINER] =
        BeaconDetail_AddActor(work, unit, &work->unionRes, 128, 120, 0, 1, 2, CLACT_SURFACE_MAIN);
    func_0204c378(work->actors[BEACON_DETAIL_ACTOR_TRAINER], 0, 1);
    work->actors[BEACON_DETAIL_ACTOR_RECENT] =
        BeaconDetail_AddActor(work, unit, &work->iconRes, 96, 120, 0, 2, 2, CLACT_SURFACE_MAIN);
}

static void BeaconDetail_FreeMapActors(BeaconDetailWork *work) {
    int i;

    for (i = 0; i < BEACON_DETAIL_ACTOR_COUNT; i++) {
        func_0204c124(work->actors[i], FALSE);
        func_0204c108(work->actors[i]);
    }
}

static void BeaconDetail_CreateInfoWindow(BeaconDetailWork *work) {
    BeaconDetail_InitWindow(&work->infoWindow, 1, 11, 2, 22, 28, 6);
}

static void BeaconDetail_FreeInfoWindow(BeaconDetailWork *work) {
    BmpWin_Free(work->infoWindow.window);
}

static void BeaconDetail_CreatePanels(BeaconDetailWork *work) {
    ClActUnit *unit = BeaconDetailGraphic_GetClActUnit(work->graphic);
    int i;
    int j;

    work->pmsDraw =
        PMSDraw_Create(unit, CLACT_VRAM_SUB, work->printQueue, work->font, 10, BEACON_DETAIL_PANEL_COUNT, work->heapId);
    PMSDraw_SetBackColor(work->pmsDraw, 4);
    PMSDraw_SetColor(work->pmsDraw, PRINT_COLOR(15, 14, 4));
    PMSDraw_SetFollowScroll(work->pmsDraw, TRUE);
    for (i = 0; i < BEACON_DETAIL_PANEL_COUNT; i++) {
        int bg = BGSYS_BG_SUB + i;
        BeaconDetailPanel *panel = &work->panels[i];

        panel->bg = bg;
        panel->recordWindow = BmpWin_CreateDynamic(bg, 2, 15, 28, 4, i, TRUE);
        for (j = 0; j < 4; j++) {
            BeaconDetail_InitWindow(&panel->windows[j], bg, i, 2, j * 2 + 1, sBeaconDetailWindowWidths[j], 2);
        }
        BeaconDetail_InitWindow(&panel->windows[BEACON_DETAIL_WINDOW_FROM], bg, i, 2, 9, 19, 2);
        BeaconDetail_InitWindow(&panel->windows[BEACON_DETAIL_WINDOW_HOME], bg, i, 3, 11, 28, 4);
        BeaconDetail_InitWindow(&panel->windows[BEACON_DETAIL_WINDOW_COUNTS], bg, i, 4, 19, 27, 4);
        panel->trainerActor =
            BeaconDetail_AddActor(work, unit, &work->trainerRes[i], 220, 44, 0, 1, i, CLACT_SURFACE_SUB);
        func_0204c318(panel->trainerActor, GX_OAM_MODE_NORMAL);
        func_0204c124(panel->trainerActor, FALSE);
        panel->rankActor = BeaconDetail_AddActor(work, unit, &work->iconRes, 200, 80, 6, 0, i, CLACT_SURFACE_SUB);
        func_0204c318(panel->rankActor, GX_OAM_MODE_XLU);
        func_0204c124(panel->rankActor, FALSE);
    }
}

static void BeaconDetail_FreePanels(BeaconDetailWork *work) {
    int i;
    int j;

    for (i = 0; i < BEACON_DETAIL_PANEL_COUNT; i++) {
        BeaconDetailPanel *panel = &work->panels[i];

        func_0204c124(panel->trainerActor, FALSE);
        func_0204c108(panel->trainerActor);
        func_0204c124(panel->rankActor, FALSE);
        func_0204c108(panel->rankActor);
        BmpWin_Free(panel->windows[BEACON_DETAIL_WINDOW_COUNTS].window);
        for (j = 0; j < 2; j++) {
            BmpWin_Free(panel->windows[BEACON_DETAIL_WINDOW_FROM + j].window);
        }
        for (j = 0; j < 4; j++) {
            BmpWin_Free(panel->windows[j].window);
        }
        BmpWin_Free(panel->recordWindow);
    }
    PMSDraw_Delete(work->pmsDraw);
}

static void BeaconDetail_UpdatePMSDraw(BeaconDetailWork *work) {
    PMSDraw_Main(work->pmsDraw);
}

const GameProcFunctions BeaconDetail_ProcFunctions = {
    BeaconDetail_Init,
    BeaconDetail_Main,
    BeaconDetail_Exit,
};
