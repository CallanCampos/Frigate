// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/arc.h"
#include "constants/places.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_msgbg.h"
#include "field/place_name_letter.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "save/join_avenue.h"
#include "save/player_savedata.h"
#include "save/save_control.h"
#include "system/bmp_oam.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/printsys.h"
#include "system/version.h"

// The banner's modes: waiting for a request, loading the name and the window, sliding the letters in, showing them,
// and sliding them out
enum {
    PLACE_NAME_MODE_IDLE,
    PLACE_NAME_MODE_PREPARE,
    PLACE_NAME_MODE_FADE_IN,
    PLACE_NAME_MODE_FADE_IN_WAIT,
    PLACE_NAME_MODE_LETTERS_LAUNCH,
    PLACE_NAME_MODE_LETTERS_LAND,
    PLACE_NAME_MODE_FADE_OUT,
};

// The steps of PLACE_NAME_MODE_PREPARE, a frame each: the name, a frame's wait, the window's screen, palette, chars
// and screen, its BG and window planes, then the letters, which it waits for
enum {
    PLACE_NAME_PREPARE_LOAD_MESSAGE,
    PLACE_NAME_PREPARE_WAIT,
    PLACE_NAME_PREPARE_CLEAR_SCREEN,
    PLACE_NAME_PREPARE_PALETTE,
    PLACE_NAME_PREPARE_CHARS,
    PLACE_NAME_PREPARE_SCREEN,
    PLACE_NAME_PREPARE_WINDOW,
    PLACE_NAME_PREPARE_LETTERS,
    PLACE_NAME_PREPARE_WAIT_LETTERS,
};

// The steps of the modes that start something and then wait for it
enum {
    PLACE_NAME_STEP_START,
    PLACE_NAME_STEP_WAIT,
};

// The place names the message bank has: a zone's past them shows none
#define PLACE_NAME_COUNT 154

// The place name banner of the field
struct PlaceName {
    GameSystem *gsys;
    GameData *gameData;
    PrintQueue *printQueue;
    u16 heapId;
    ArcTool *arc;
    MsgData *msgData;
    Font *font;
    StrBuf *name;
    u8 charCount;
    u8 season;
    u16 shownZoneId;
    u32 charPos;
    u32 chars;
    u32 cellAnims;
    u32 palettes[2];
    BmpOamSys *bmpOam;
    ClActUnit *clactUnit;
    ClActRenderer *renderer;
    u32 mode;
    u8 state;
    u16 timer;
    u16 prevZoneId;
    u16 reqZoneId;
    u32 requested;
    u32 forced;
    PlaceNameLetter *objects[2];
    void *paletteFile;
    NNSG2dPaletteData *paletteData;
    void *screenFile;
    NNSG2dScreenData *screenData;
    void *charFile;
    NNSG2dCharacterData *charData;
};

BOOL isGameVerWhite2(void);
void endColorBlending(PlaceName *wk);
void PlaceName_SetupWindow(PlaceName *wk);
void PlaceName_ReleaseWindow(PlaceName *wk);
void PlaceName_ResetWindow(PlaceName *wk);
void PlaceName_LoadPaletteFile(PlaceName *wk, u32 fileId);
void PlaceName_FreePaletteFile(PlaceName *wk);
void PlaceName_TransferPalette(PlaceName *wk);
void PlaceName_LoadCharFile(PlaceName *wk, u32 fileId);
void PlaceName_FreeCharFile(PlaceName *wk);
void PlaceName_TransferChars(PlaceName *wk);
void PlaceName_LoadScreenFile(PlaceName *wk, u32 fileId);
void PlaceName_FreeScreenFile(PlaceName *wk);
void PlaceName_TransferScreen(PlaceName *wk);
void PlaceName_AllocBgChars(PlaceName *wk);
void PlaceName_FreeBgChars(PlaceName *wk);
void PlaceName_CreateRenderer(PlaceName *wk);
void PlaceName_FreeRenderer(PlaceName *wk);
void PlaceName_LoadClActResources(PlaceName *wk);
void PlaceName_FreeClActResources(PlaceName *wk);
void PlaceName_CreateClActUnit(PlaceName *wk);
void PlaceName_FreeClActUnit(PlaceName *wk);
void PlaceName_SetClActUnitRenderer(PlaceName *wk, u32 index);
void PlaceName_CreateArchive(PlaceName *wk);
void PlaceName_FreeArchive(PlaceName *wk);
void PlaceName_CreateLetters(PlaceName *wk);
void PlaceName_FreeLetters(PlaceName *wk);
void PlaceName_SetupLetters(PlaceName *wk);
void PlaceName_HideLetters(PlaceName *wk);
void PlaceName_UpdateLetters(PlaceName *wk);
BOOL PlaceName_IsAllSpawned(PlaceName *wk);
void PlaceName_SlideInLetters(PlaceName *wk);
void PlaceName_SlideOutLetters(PlaceName *wk);
void PlaceName_CreateBmpOam(PlaceName *wk);
void PlaceName_FreeBmpOam(PlaceName *wk);
void PlaceName_RequestForceShow(PlaceName *wk);
void PlaceName_ClearForceShowRequest(PlaceName *wk);
u32 PlaceName_GetZoneChangeRequest(PlaceName *wk);
void PlaceName_SetZoneChangeRequest(PlaceName *wk);
void PlaceName_ClearZoneChangeRequest(PlaceName *wk);
u32 PlaceName_GetForceShowRequest(PlaceName *wk);
BOOL IsLocNameIdxChanged(PlaceName *wk);
void PlaceName_LoadMessage(PlaceName *wk);
void PlaceName_SetState(PlaceName *wk, u32 mode);
void PlaceName_Interrupt(PlaceName *wk);
void PlaceName_StateIdle(PlaceName *wk);
void PlaceName_StatePrepare(PlaceName *wk);
void PlaceName_StateFadeIn(PlaceName *wk);
void PlaceName_StateFadeInWait(PlaceName *wk);
void PlaceName_StateLettersLaunch(PlaceName *wk);
void PlaceName_StateLettersLand(PlaceName *wk);
void PlaceName_StateFadeOut(PlaceName *wk);
void PlaceName_DrawIdle(PlaceName *wk);
void PlaceName_DrawPrepare(PlaceName *wk);
void __endColorBlending(PlaceName *wk);
void PlaceName_DrawFadeInWait(PlaceName *wk);
void PlaceName_DrawLettersLaunch(PlaceName *wk);
void PlaceName_DrawLettersLand(PlaceName *wk);
void _endColorBlending(PlaceName *wk);
PlaceName *FieldPlaceName_Alloc(HeapID heapId);
void PlaceName_InitWork(PlaceName *wk, GameSystem *gsys);
void PlaceName_FreeWork(PlaceName *wk);
void PlaceName_CreatePrintQueue(PlaceName *wk);
void PlaceName_CreateMsgData(PlaceName *wk);
void PlaceName_CreateName(PlaceName *wk);
void PlaceName_FreePrintQueue(PlaceName *wk);
void PlaceName_FreeMsgData(PlaceName *wk);
void PlaceName_FreeName(PlaceName *wk);
HeapID FieldPlaceName_GetHeapID(PlaceName *wk);
ArcTool *FieldPlaceName_GetArc(PlaceName *wk);
Font *PlaceName_GetFont(PlaceName *wk);
void PlaceName_SetHeapID(PlaceName *wk, HeapID heapId);
void PlaceName_SetFont(PlaceName *wk, Font *font);
void PlaceName_SetMode(PlaceName *wk, u32 mode);
u32 PlaceName_GetMode(PlaceName *wk);
void PlaceName_ResetTimer(PlaceName *wk);
void PlaceName_IncrementTimer(PlaceName *wk);
u8 PlaceName_GetState(PlaceName *wk);
void PlaceName_ResetState(PlaceName *wk);
void PlaceName_NextState(PlaceName *wk);
void PlaceName_LoadZoneFromSave(PlaceName *wk);
void PlaceName_SaveZoneToSave(PlaceName *wk);
u16 PlaceName_GetReqZoneID(PlaceName *wk);
void SetPlaceNameDispZone(PlaceName *wk, u16 zoneId);
StrBuf *PlaceName_GetName(PlaceName *wk);
BOOL CheckPopupPlaceNameDispAuto(u16 zoneId, BOOL continued);

static const ClActSurfaceSetup sPlaceNameSurfaceSetup = { 0, 0, 256, 192, 0, 1 };

static const BGSetup sPlaceNameBGSetup = { 0, 0, 0x800, 0, 1, 0, 2, 1, 0x8000, 0, 1, 0, 0 };

BOOL isGameVerWhite2(void) {
    if (getGameVersion() == VERSION_WHITE2) {
        return TRUE;
    }
    return FALSE;
}

PlaceName *FieldPlaceName_Create(GameSystem *gsys, HeapID heapId, FieldMsgBG *msgBGSys) {
    PlaceName *wk = FieldPlaceName_Alloc(heapId);
    PlaceName_InitWork(wk, gsys);
    PlaceName_LoadZoneFromSave(wk);
    PlaceName_CreateArchive(wk);
    PlaceName_CreatePrintQueue(wk);
    PlaceName_CreateMsgData(wk);
    PlaceName_CreateName(wk);
    PlaceName_SetHeapID(wk, heapId);
    PlaceName_SetFont(wk, MsgBGPrinter_GetFont(msgBGSys));
    PlaceName_CreateRenderer(wk);
    PlaceName_LoadClActResources(wk);
    PlaceName_CreateClActUnit(wk);
    PlaceName_SetClActUnitRenderer(wk, 0);
    PlaceName_CreateBmpOam(wk);
    PlaceName_CreateLetters(wk);
    GFL_BGSysCreateBG(3, &sPlaceNameBGSetup, BGMODE_TEXT);
    if (isGameVerWhite2()) {
        PlaceName_LoadPaletteFile(wk, 4);
        PlaceName_LoadCharFile(wk, 3);
        PlaceName_LoadScreenFile(wk, 5);
    } else {
        PlaceName_LoadPaletteFile(wk, 1);
        PlaceName_LoadCharFile(wk, 0);
        PlaceName_LoadScreenFile(wk, 2);
    }
    PlaceName_AllocBgChars(wk);
    PlaceName_SetState(wk, PLACE_NAME_MODE_IDLE);
    return wk;
}

void FieldPlaceName_Free(PlaceName *wk) {
    PlaceName_SaveZoneToSave(wk);
    PlaceName_FreeScreenFile(wk);
    PlaceName_FreeCharFile(wk);
    PlaceName_FreePaletteFile(wk);
    PlaceName_FreeLetters(wk);
    PlaceName_FreeName(wk);
    PlaceName_FreeBmpOam(wk);
    PlaceName_FreeClActUnit(wk);
    PlaceName_FreeClActResources(wk);
    PlaceName_FreeRenderer(wk);
    PlaceName_FreePrintQueue(wk);
    PlaceName_FreeMsgData(wk);
    PlaceName_FreeBgChars(wk);
    PlaceName_FreeArchive(wk);
    GFL_BGSysReleaseBG(3);
    PlaceName_FreeWork(wk);
}

void PlaceName_Main(PlaceName *wk) {
    PlaceName_IncrementTimer(wk);
    switch (PlaceName_GetMode(wk)) {
    case PLACE_NAME_MODE_IDLE:
        PlaceName_StateIdle(wk);
        break;
    case PLACE_NAME_MODE_PREPARE:
        PlaceName_StatePrepare(wk);
        break;
    case PLACE_NAME_MODE_FADE_IN:
        PlaceName_StateFadeIn(wk);
        break;
    case PLACE_NAME_MODE_FADE_IN_WAIT:
        PlaceName_StateFadeInWait(wk);
        break;
    case PLACE_NAME_MODE_LETTERS_LAUNCH:
        PlaceName_StateLettersLaunch(wk);
        break;
    case PLACE_NAME_MODE_LETTERS_LAND:
        PlaceName_StateLettersLand(wk);
        break;
    case PLACE_NAME_MODE_FADE_OUT:
        PlaceName_StateFadeOut(wk);
        break;
    }
    PlaceName_UpdateLetters(wk);
}

void PlaceName_Draw(PlaceName *wk) {
    switch (PlaceName_GetMode(wk)) {
    case PLACE_NAME_MODE_IDLE:
        PlaceName_DrawIdle(wk);
        break;
    case PLACE_NAME_MODE_PREPARE:
        PlaceName_DrawPrepare(wk);
        break;
    case PLACE_NAME_MODE_FADE_IN:
        __endColorBlending(wk);
        break;
    case PLACE_NAME_MODE_FADE_IN_WAIT:
        PlaceName_DrawFadeInWait(wk);
        break;
    case PLACE_NAME_MODE_LETTERS_LAUNCH:
        PlaceName_DrawLettersLaunch(wk);
        break;
    case PLACE_NAME_MODE_LETTERS_LAND:
        PlaceName_DrawLettersLand(wk);
        break;
    case PLACE_NAME_MODE_FADE_OUT:
        _endColorBlending(wk);
        break;
    }
    PrintQueue_Main(wk->printQueue);
}

void PlaceName_OnZoneChange(PlaceName *wk, s32 zoneId) {
    SetPlaceNameDispZone(wk, zoneId);
    if (CheckPopupPlaceNameDispAuto(zoneId, FALSE)) {
        PlaceName_Interrupt(wk);
        PlaceName_SetZoneChangeRequest(wk);
    }
}

void PlaceName_ShowForZone(PlaceName *wk, u32 zoneId) {
    SetPlaceNameDispZone(wk, zoneId);
    if (CheckPopupPlaceNameDispAuto(zoneId, FALSE)) {
        PlaceName_Interrupt(wk);
        PlaceName_RequestForceShow(wk);
    }
}

void BeginContinuePlaceNameDisp(PlaceName *wk, s32 zoneId) {
    SetPlaceNameDispZone(wk, zoneId);
    if (CheckPopupPlaceNameDispAuto(zoneId, TRUE)) {
        PlaceName_Interrupt(wk);
        PlaceName_RequestForceShow(wk);
    }
}

void BeginForcePlaceNameDisp(PlaceName *wk, s32 zoneId) {
    SetPlaceNameDispZone(wk, zoneId);
    PlaceName_Interrupt(wk);
    PlaceName_RequestForceShow(wk);
}

void PlaceName_Cancel(PlaceName *wk) {
    PlaceName_SetState(wk, PLACE_NAME_MODE_IDLE);
    PlaceName_ClearZoneChangeRequest(wk);
    PlaceName_ClearForceShowRequest(wk);
}

void PlaceName_RestoreWindow(PlaceName *wk) {
    PlaceName_ResetWindow(wk);
}

void endColorBlending(PlaceName *wk) {
    G2_BlendNone();
}

void PlaceName_SetupWindow(PlaceName *wk) {
    PlaceName_TransferPalette(wk);
    PlaceName_AllocBgChars(wk);
    GFL_BGSysSetBGPriority(3, 1);
    GFL_BGSysSetBGEnabled(3, 0);
    endColorBlending(wk);
}

void PlaceName_ReleaseWindow(PlaceName *wk) {
    GFL_BGSysClearBG(3);
    PlaceName_FreeBgChars(wk);
}

void PlaceName_ResetWindow(PlaceName *wk) {
    PlaceName_ReleaseWindow(wk);
    PlaceName_SetupWindow(wk);
}

void PlaceName_LoadPaletteFile(PlaceName *wk, u32 fileId) {
    HeapID heapId = FieldPlaceName_GetHeapID(wk);
    wk->paletteFile = GFL_G2DIOReadNCLRArc(FieldPlaceName_GetArc(wk), fileId, &wk->paletteData, heapId);
}

void PlaceName_FreePaletteFile(PlaceName *wk) {
    if (wk->paletteFile != NULL) {
        GFL_HeapFree(wk->paletteFile);
        wk->paletteFile = NULL;
        wk->paletteData = NULL;
    }
}

void PlaceName_TransferPalette(PlaceName *wk) {
    GFL_BGSysUploadStdPalette(3, wk->paletteData->pRawData, 0x20, 0);
}

void PlaceName_LoadCharFile(PlaceName *wk, u32 fileId) {
    HeapID heapId = FieldPlaceName_GetHeapID(wk);
    wk->charFile = GFL_G2DIOReadBGNCGRArc(FieldPlaceName_GetArc(wk), fileId, FALSE, &wk->charData, heapId);
}

void PlaceName_FreeCharFile(PlaceName *wk) {
    if (wk->charFile != NULL) {
        GFL_HeapFree(wk->charFile);
        wk->charFile = NULL;
        wk->charData = NULL;
    }
}

void PlaceName_TransferChars(PlaceName *wk) {
    GFL_BGSysLoadChar(3, wk->charData->pRawData, wk->charData->szByte, 0);
}

void PlaceName_LoadScreenFile(PlaceName *wk, u32 fileId) {
    HeapID heapId = FieldPlaceName_GetHeapID(wk);
    wk->screenFile = GFL_G2DIOReadNSCRArc(FieldPlaceName_GetArc(wk), fileId, FALSE, &wk->screenData, heapId);
}

void PlaceName_FreeScreenFile(PlaceName *wk) {
    if (wk->screenFile != NULL) {
        GFL_HeapFree(wk->screenFile);
        wk->screenFile = NULL;
        wk->screenData = NULL;
    }
}

void PlaceName_TransferScreen(PlaceName *wk) {
    GFL_BGSysBufferScr(3, wk->screenData->rawData, wk->screenData->szByte, 0);
    GFL_BGSysQueueScrLoad(3);
}

void PlaceName_AllocBgChars(PlaceName *wk) {
    if (wk->charPos == (u32)-1) {
        wk->charPos = GFL_BGSysAllocChar(3, 0x20, 0);
    }
}

void PlaceName_FreeBgChars(PlaceName *wk) {
    if (wk->charPos != (u32)-1) {
        GFL_BGSysFreeCharMemory(3, wk->charPos, 0x20);
        wk->charPos = -1;
    }
}

void PlaceName_CreateRenderer(PlaceName *wk) {
    wk->renderer = ClActRenderer_Create(&sPlaceNameSurfaceSetup, 1, wk->heapId);
}

void PlaceName_FreeRenderer(PlaceName *wk) {
    if (wk->renderer != NULL) {
        ClActRenderer_Delete(wk->renderer);
        wk->renderer = NULL;
    }
}

void PlaceName_LoadClActResources(PlaceName *wk) {
    HeapID heapId = FieldPlaceName_GetHeapID(wk);
    ArcTool *arc = FieldPlaceName_GetArc(wk);
    wk->palettes[0] = ClActSys_LoadPlttRange(arc, 10, 0, 0x20, 0, 1, heapId);
    wk->palettes[1] = ClActSys_LoadPlttRange(arc, 9, 0, 0x80, 0, 1, heapId);
    wk->chars = ClActSys_LoadChars(arc, 8, 0, 0, heapId);
    wk->cellAnims = ClActSys_LoadCellAnims(arc, 7, 6, heapId);
}

void PlaceName_FreeClActResources(PlaceName *wk) {
    int i;
    ClActSys_FreeChars(wk->chars);
    ClActSys_FreeCellAnims(wk->cellAnims);
    for (i = 0; i < 2; i++) {
        ClActSys_FreePltt(wk->palettes[i]);
    }
}

void PlaceName_CreateClActUnit(PlaceName *wk) {
    wk->clactUnit = ClActUnit_Create(0x20, 1, FieldPlaceName_GetHeapID(wk));
    ClActUnit_SetActive(wk->clactUnit, TRUE);
}

void PlaceName_FreeClActUnit(PlaceName *wk) {
    ClActUnit_Delete(wk->clactUnit);
}

void PlaceName_SetClActUnitRenderer(PlaceName *wk, u32 index) {
    if (wk->renderer != NULL) {
        ClActUnit_SetRenderer((&wk->clactUnit)[index], wk->renderer);
    }
}

void PlaceName_CreateArchive(PlaceName *wk) {
    wk->arc = GFL_ArcSysCreateFileHandle(ARCID_FIELD_PLACE_NAME, FieldPlaceName_GetHeapID(wk));
}

void PlaceName_FreeArchive(PlaceName *wk) {
    if (wk->arc != NULL) {
        GFL_ArcToolFree(wk->arc);
        wk->arc = NULL;
    }
}

void PlaceName_CreateLetters(PlaceName *wk) {
    PlaceNameTextSetup text;
    PlaceNameFrameSetup frame;
    HeapID heapId;
    ArcTool *arc;

    MI_CpuFill8(&text, 0, sizeof(text));
    text.bmpOam = wk->bmpOam;
    text.palette = wk->palettes[0];
    text.printQueue = wk->printQueue;
    heapId = FieldPlaceName_GetHeapID(wk);
    arc = FieldPlaceName_GetArc(wk);
    wk->objects[0] = PlaceNameLetter_Create(heapId, PLACE_NAME_LETTER_TEXT, &text, wk->clactUnit, arc, 0,
                                            wk->palettes[0], wk->chars, wk->cellAnims);
    MI_CpuFill8(&frame, 0, sizeof(frame));
    frame.palette = wk->palettes[1];
    frame.chars = wk->chars;
    frame.cellAnims = wk->cellAnims;
    heapId = FieldPlaceName_GetHeapID(wk);
    arc = FieldPlaceName_GetArc(wk);
    wk->objects[1] = PlaceNameLetter_Create(heapId, PLACE_NAME_LETTER_FRAME, &frame, wk->clactUnit, arc, 0,
                                            wk->palettes[0], wk->chars, wk->cellAnims);
}

void PlaceName_FreeLetters(PlaceName *wk) {
    int i;
    for (i = 0; i < 2; i++) {
        if (wk->objects[i] != NULL) {
            PlaceNameLetter_Delete(wk->objects[i]);
            wk->objects[i] = NULL;
        }
    }
}

void PlaceName_SetupLetters(PlaceName *wk) {
    PlaceNameTextParam text;
    u32 season;

    MI_CpuFill8(&text, 0, sizeof(text));
    text.font = PlaceName_GetFont(wk);
    text.zoneId = wk->shownZoneId;
    text.name = PlaceName_GetName(wk);
    PlaceNameLetter_SetParam(wk->objects[0], &text);
    MI_CpuFill8(&season, 0, sizeof(season));
    season = wk->season;
    PlaceNameLetter_SetParam(wk->objects[1], &season);
}

void PlaceName_HideLetters(PlaceName *wk) {
    int i;
    for (i = 0; i < 2; i++) {
        PlaceNameLetter_Hide(wk->objects[i]);
    }
    GFL_BGSysSetBGEnabled(3, 0);
}

void PlaceName_UpdateLetters(PlaceName *wk) {
    int i;
    for (i = 0; i < 2; i++) {
        PlaceNameLetter_Update(wk->objects[i]);
    }
}

BOOL PlaceName_IsAllSpawned(PlaceName *wk) {
    BOOL done = TRUE;
    int i;
    for (i = 0; i < 2; i++) {
        if (!PlaceNameLetter_IsFinished(wk->objects[i])) {
            done = FALSE;
            break;
        }
    }
    return done;
}

void PlaceName_SlideInLetters(PlaceName *wk) {
    int i;
    for (i = 0; i < 2; i++) {
        PlaceNameLetter_SlideIn(wk->objects[i], wk->timer, 10);
    }
}

void PlaceName_SlideOutLetters(PlaceName *wk) {
    int i;
    for (i = 0; i < 2; i++) {
        PlaceNameLetter_SlideOut(wk->objects[i], 0, 12);
    }
}

void PlaceName_CreateBmpOam(PlaceName *wk) {
    wk->bmpOam = BmpOam_Init(FieldPlaceName_GetHeapID(wk), wk->clactUnit);
}

void PlaceName_FreeBmpOam(PlaceName *wk) {
    if (wk->bmpOam != NULL) {
        BmpOam_Exit(wk->bmpOam);
        wk->bmpOam = NULL;
    }
}

void PlaceName_RequestForceShow(PlaceName *wk) {
    wk->forced = TRUE;
}

void PlaceName_ClearForceShowRequest(PlaceName *wk) {
    wk->forced = FALSE;
}

u32 PlaceName_GetZoneChangeRequest(PlaceName *wk) {
    return wk->requested;
}

void PlaceName_SetZoneChangeRequest(PlaceName *wk) {
    wk->requested = TRUE;
}

void PlaceName_ClearZoneChangeRequest(PlaceName *wk) {
    wk->requested = FALSE;
}

u32 PlaceName_GetForceShowRequest(PlaceName *wk) {
    return wk->forced;
}

BOOL IsLocNameIdxChanged(PlaceName *wk) {
    u16 prev = ZoneData_GetPlaceNameID(wk->prevZoneId);
    u16 req = ZoneData_GetPlaceNameID(wk->reqZoneId);
    if (prev != req) {
        return TRUE;
    }
    return FALSE;
}

void PlaceName_LoadMessage(PlaceName *wk) {
    u16 zoneId = PlaceName_GetReqZoneID(wk);
    u16 nameId = ZoneData_GetPlaceNameID(zoneId);
    if (nameId >= PLACE_NAME_COUNT) {
        nameId = PLACE_NONE;
    }
    if (nameId == PLACE_NONE) {
        PlaceName_Cancel(wk);
        return;
    }
    if (IsZoneJoinAvenue(zoneId)) {
        GFL_StrBufLoadString(wk->name, (const u16 *)JoinAvenue_GetParam(JoinAvenue_GetInfo(SaveControl_GetJoinAvenue(
                                                                            GameData_GetSaveControl(wk->gameData))),
                                                                        JOIN_AVE_INFO_NAME, 0));
    } else {
        GFL_MsgDataLoadStrbuf(wk->msgData, nameId, wk->name);
    }
    wk->season = GameData_GetSeason(wk->gameData);
    wk->charCount = GFL_StrBufGetCharCount(wk->name);
    wk->shownZoneId = zoneId;
}

void PlaceName_SetState(PlaceName *wk, u32 mode) {
    PlaceName_SetMode(wk, mode);
    PlaceName_ResetTimer(wk);
    PlaceName_ResetState(wk);
    if (mode == PLACE_NAME_MODE_IDLE) {
        PlaceName_HideLetters(wk);
        GX_SetVisibleWnd(GX_WNDMASK_NONE);
    }
}

void PlaceName_Interrupt(PlaceName *wk) {
    u32 mode;
    s32 timer;
    switch (wk->mode) {
    case PLACE_NAME_MODE_IDLE:
    case PLACE_NAME_MODE_FADE_OUT:
        return;
    case PLACE_NAME_MODE_FADE_IN_WAIT:
    case PLACE_NAME_MODE_LETTERS_LAUNCH:
    case PLACE_NAME_MODE_LETTERS_LAND:
        mode = PLACE_NAME_MODE_LETTERS_LAND;
        timer = 0;
        break;
    case PLACE_NAME_MODE_FADE_IN:
        mode = PLACE_NAME_MODE_LETTERS_LAND;
        timer = 12.0f * (1.0f - wk->timer / 10.0f);
        break;
    case PLACE_NAME_MODE_PREPARE:
        mode = PLACE_NAME_MODE_IDLE;
        timer = 0;
        break;
    }
    PlaceName_SetState(wk, mode);
    wk->timer = timer;
}

void PlaceName_StateIdle(PlaceName *wk) {
    BOOL start = FALSE;
    if (PlaceName_GetForceShowRequest(wk)) {
        start = TRUE;
        PlaceName_ClearForceShowRequest(wk);
    } else if (PlaceName_GetZoneChangeRequest(wk)) {
        if (IsLocNameIdxChanged(wk)) {
            start = TRUE;
        }
        PlaceName_ClearZoneChangeRequest(wk);
    }
    if (start) {
        PlaceName_SetState(wk, PLACE_NAME_MODE_PREPARE);
    }
}

void PlaceName_StatePrepare(PlaceName *wk) {
    switch (PlaceName_GetState(wk)) {
    case PLACE_NAME_PREPARE_LOAD_MESSAGE:
        PlaceName_LoadMessage(wk);
        PlaceName_NextState(wk);
        break;
    case PLACE_NAME_PREPARE_WAIT:
        PlaceName_NextState(wk);
        break;
    case PLACE_NAME_PREPARE_CLEAR_SCREEN:
        GFL_BGSysClearScr(3);
        PlaceName_NextState(wk);
        break;
    case PLACE_NAME_PREPARE_PALETTE:
        PlaceName_TransferPalette(wk);
        PlaceName_NextState(wk);
        break;
    case PLACE_NAME_PREPARE_CHARS:
        PlaceName_TransferChars(wk);
        PlaceName_NextState(wk);
        break;
    case PLACE_NAME_PREPARE_SCREEN:
        PlaceName_TransferScreen(wk);
        PlaceName_NextState(wk);
        break;
    case PLACE_NAME_PREPARE_WINDOW:
        GFL_BGSysSetBGPriority(3, 1);
        GFL_BGSysSetBGEnabled(3, 0);
        endColorBlending(wk);
        GX_SetVisibleWnd(GX_WNDMASK_OW);
        G2_SetWndOBJInsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 |
                                    GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ,
                                TRUE);
        G2_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_OBJ,
                              TRUE);
        PlaceName_NextState(wk);
        break;
    case PLACE_NAME_PREPARE_LETTERS:
        PlaceName_SetupLetters(wk);
        PlaceName_NextState(wk);
        break;
    case PLACE_NAME_PREPARE_WAIT_LETTERS:
        if (PlaceName_IsAllSpawned(wk)) {
            PlaceName_SetState(wk, PLACE_NAME_MODE_FADE_IN);
        }
        break;
    }
}

void PlaceName_StateFadeIn(PlaceName *wk) {
    switch (PlaceName_GetState(wk)) {
    case PLACE_NAME_STEP_START:
        __endColorBlending(wk);
        GFL_BGSysSetBGEnabled(3, 1);
        PlaceName_SlideInLetters(wk);
        PlaceName_NextState(wk);
        break;
    case PLACE_NAME_STEP_WAIT:
        if (wk->timer > 10 && PlaceName_IsAllSpawned(wk)) {
            PlaceName_SetState(wk, PLACE_NAME_MODE_FADE_IN_WAIT);
        }
        break;
    }
}

void PlaceName_StateFadeInWait(PlaceName *wk) {
    if (wk->timer > 30) {
        PlaceName_SetState(wk, PLACE_NAME_MODE_LETTERS_LAUNCH);
    }
}

void PlaceName_StateLettersLaunch(PlaceName *wk) {
    switch (PlaceName_GetState(wk)) {
    case PLACE_NAME_STEP_START:
        PlaceName_NextState(wk);
        break;
    case PLACE_NAME_STEP_WAIT:
        PlaceName_SetState(wk, PLACE_NAME_MODE_LETTERS_LAND);
        break;
    }
}

void PlaceName_StateLettersLand(PlaceName *wk) {
    switch (PlaceName_GetState(wk)) {
    case PLACE_NAME_STEP_START:
        PlaceName_SlideOutLetters(wk);
        PlaceName_NextState(wk);
        break;
    case PLACE_NAME_STEP_WAIT:
        if (wk->timer > 12 && PlaceName_IsAllSpawned(wk)) {
            PlaceName_SetState(wk, PLACE_NAME_MODE_FADE_OUT);
        }
        break;
    }
}

void PlaceName_StateFadeOut(PlaceName *wk) {
    if (wk->timer > 5) {
        PlaceName_SetState(wk, PLACE_NAME_MODE_IDLE);
        PlaceName_HideLetters(wk);
    }
}

void PlaceName_DrawIdle(PlaceName *wk) {
}

void PlaceName_DrawPrepare(PlaceName *wk) {
}

void __endColorBlending(PlaceName *wk) {
    endColorBlending(wk);
}

void PlaceName_DrawFadeInWait(PlaceName *wk) {
}

void PlaceName_DrawLettersLaunch(PlaceName *wk) {
}

void PlaceName_DrawLettersLand(PlaceName *wk) {
}

void _endColorBlending(PlaceName *wk) {
    endColorBlending(wk);
}

PlaceName *FieldPlaceName_Alloc(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(PlaceName), TRUE, "field_place_name.c", 1927);
}

void PlaceName_InitWork(PlaceName *wk, GameSystem *gsys) {
    MI_CpuFill8(wk, 0, sizeof(PlaceName));
    wk->gsys = gsys;
    wk->gameData = GSYS_GetGameData(gsys);
    wk->charPos = -1;
    wk->forced = 0;
    wk->requested = 0;
    // The banner starts from no zone, before the save's is loaded
    wk->prevZoneId = ZONE_MAX;
    wk->reqZoneId = ZONE_MAX;
}

void PlaceName_FreeWork(PlaceName *wk) {
    GFL_HeapFree(wk);
}

void PlaceName_CreatePrintQueue(PlaceName *wk) {
    wk->printQueue = PrintQueue_CreateDefault(FieldPlaceName_GetHeapID(wk));
    PrintQueue_SetQueueAlways(wk->printQueue, TRUE);
}

void PlaceName_CreateMsgData(PlaceName *wk) {
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_PLACE_NAMES, FieldPlaceName_GetHeapID(wk));
}

void PlaceName_CreateName(PlaceName *wk) {
    wk->name = GFL_StrBufCreate(0x1d, FieldPlaceName_GetHeapID(wk));
}

void PlaceName_FreePrintQueue(PlaceName *wk) {
    if (wk->printQueue != NULL) {
        PrintQueue_Free(wk->printQueue);
        wk->printQueue = NULL;
    }
}

void PlaceName_FreeMsgData(PlaceName *wk) {
    if (wk->msgData != NULL) {
        GFL_MsgDataFree(wk->msgData);
        wk->msgData = NULL;
    }
}

void PlaceName_FreeName(PlaceName *wk) {
    if (wk->name != NULL) {
        GFL_StrBufFree(wk->name);
        wk->name = NULL;
    }
}

HeapID FieldPlaceName_GetHeapID(PlaceName *wk) {
    return wk->heapId;
}

ArcTool *FieldPlaceName_GetArc(PlaceName *wk) {
    return wk->arc;
}

Font *PlaceName_GetFont(PlaceName *wk) {
    return wk->font;
}

void PlaceName_SetHeapID(PlaceName *wk, HeapID heapId) {
    wk->heapId = heapId;
}

void PlaceName_SetFont(PlaceName *wk, Font *font) {
    wk->font = font;
}

void PlaceName_SetMode(PlaceName *wk, u32 mode) {
    wk->mode = mode;
}

u32 PlaceName_GetMode(PlaceName *wk) {
    return wk->mode;
}

void PlaceName_ResetTimer(PlaceName *wk) {
    wk->timer = 0;
}

void PlaceName_IncrementTimer(PlaceName *wk) {
    wk->timer++;
}

u8 PlaceName_GetState(PlaceName *wk) {
    return wk->state;
}

void PlaceName_ResetState(PlaceName *wk) {
    wk->state = 0;
}

void PlaceName_NextState(PlaceName *wk) {
    wk->state++;
}

void PlaceName_LoadZoneFromSave(PlaceName *wk) {
    wk->reqZoneId =
        PlayerSave_GetPlaceNameZoneID(SaveControl_GetPlayerSave(GameData_GetSaveControl(GSYS_GetGameData(wk->gsys))));
}

void PlaceName_SaveZoneToSave(PlaceName *wk) {
    PlayerSave_SetPlaceNameZoneID(SaveControl_GetPlayerSave(GameData_GetSaveControl(GSYS_GetGameData(wk->gsys))),
                                  wk->reqZoneId);
}

u16 PlaceName_GetReqZoneID(PlaceName *wk) {
    return wk->reqZoneId;
}

void SetPlaceNameDispZone(PlaceName *wk, u16 zoneId) {
    wk->prevZoneId = wk->reqZoneId;
    wk->reqZoneId = zoneId;
}

StrBuf *PlaceName_GetName(PlaceName *wk) {
    return wk->name;
}

BOOL CheckPopupPlaceNameDispAuto(u16 zoneId, BOOL continued) {
    switch (GetZoneLocNameDispType(zoneId)) {
    case PLACE_NAME_DISP_NEVER:
        return FALSE;
    case PLACE_NAME_DISP_ALWAYS:
        return TRUE;
    case PLACE_NAME_DISP_ON_CONTINUE:
        return continued;
    case PLACE_NAME_DISP_NOT_ON_CONTINUE:
        if (continued) {
            return FALSE;
        }
        return TRUE;
    default:
        return FALSE;
    }
}
