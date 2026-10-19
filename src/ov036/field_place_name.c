#include "types.h"
#include "constants/arc.h"
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
void func_ov036_021b5194(PlaceName *wk);
void func_ov036_021b51bc(PlaceName *wk);
void func_ov036_021b51d0(PlaceName *wk);
void func_ov036_021b51e0(PlaceName *wk, u32 fileId);
void func_ov036_021b5204(PlaceName *wk);
void func_ov036_021b521c(PlaceName *wk);
void func_ov036_021b5230(PlaceName *wk, u32 fileId);
void func_ov036_021b5258(PlaceName *wk);
void func_ov036_021b5270(PlaceName *wk);
void func_ov036_021b5284(PlaceName *wk, u32 fileId);
void func_ov036_021b52ac(PlaceName *wk);
void func_ov036_021b52c4(PlaceName *wk);
void func_ov036_021b52e0(PlaceName *wk);
void func_ov036_021b52fc(PlaceName *wk);
void func_ov036_021b5318(PlaceName *wk);
void func_ov036_021b5330(PlaceName *wk);
void func_ov036_021b5344(PlaceName *wk);
void func_ov036_021b53a8(PlaceName *wk);
void func_ov036_021b53cc(PlaceName *wk);
void func_ov036_021b53e8(PlaceName *wk);
void func_ov036_021b53f4(PlaceName *wk, u32 index);
void func_ov036_021b540c(PlaceName *wk);
void func_ov036_021b5420(PlaceName *wk);
void func_ov036_021b5434(PlaceName *wk);
void func_ov036_021b54e0(PlaceName *wk);
void func_ov036_021b5500(PlaceName *wk);
void func_ov036_021b5550(PlaceName *wk);
void func_ov036_021b5570(PlaceName *wk);
BOOL func_ov036_021b5588(PlaceName *wk);
void func_ov036_021b55ac(PlaceName *wk);
void func_ov036_021b55cc(PlaceName *wk);
void func_ov036_021b55ec(PlaceName *wk);
void func_ov036_021b5600(PlaceName *wk);
void func_ov036_021b5614(PlaceName *wk);
void func_ov036_021b561c(PlaceName *wk);
u32 func_ov036_021b5624(PlaceName *wk);
void func_ov036_021b5628(PlaceName *wk);
void func_ov036_021b5630(PlaceName *wk);
u32 func_ov036_021b5638(PlaceName *wk);
BOOL IsLocNameIdxChanged(PlaceName *wk);
void func_ov036_021b5660(PlaceName *wk);
void func_ov036_021b56d4(PlaceName *wk, u32 mode);
void func_ov036_021b5708(PlaceName *wk);
void func_ov036_021b5778(PlaceName *wk);
void func_ov036_021b57bc(PlaceName *wk);
void func_ov036_021b58b4(PlaceName *wk);
void func_ov036_021b5900(PlaceName *wk);
void func_ov036_021b5914(PlaceName *wk);
void func_ov036_021b5938(PlaceName *wk);
void func_ov036_021b5978(PlaceName *wk);
void func_ov036_021b5994(PlaceName *wk);
void func_ov036_021b5998(PlaceName *wk);
void __endColorBlending(PlaceName *wk);
void func_ov036_021b59a4(PlaceName *wk);
void func_ov036_021b59a8(PlaceName *wk);
void func_ov036_021b59ac(PlaceName *wk);
void _endColorBlending(PlaceName *wk);
PlaceName *FieldPlaceName_Alloc(HeapID heapId);
void func_ov036_021b59d4(PlaceName *wk, GameSystem *gsys);
void func_ov036_021b5a0c(PlaceName *wk);
void func_ov036_021b5a14(PlaceName *wk);
void func_ov036_021b5a2c(PlaceName *wk);
void func_ov036_021b5a44(PlaceName *wk);
void func_ov036_021b5a58(PlaceName *wk);
void func_ov036_021b5a6c(PlaceName *wk);
void func_ov036_021b5a80(PlaceName *wk);
HeapID FieldPlaceName_GetHeapID(PlaceName *wk);
ArcTool *FieldPlaceName_GetArc(PlaceName *wk);
Font *func_ov036_021b5a9c(PlaceName *wk);
void func_ov036_021b5aa0(PlaceName *wk, HeapID heapId);
void func_ov036_021b5aa4(PlaceName *wk, Font *font);
void func_ov036_021b5aa8(PlaceName *wk, u32 mode);
u32 func_ov036_021b5aac(PlaceName *wk);
void func_ov036_021b5ab0(PlaceName *wk);
void func_ov036_021b5ab8(PlaceName *wk);
u8 func_ov036_021b5ac8(PlaceName *wk);
void func_ov036_021b5ad0(PlaceName *wk);
void func_ov036_021b5ad8(PlaceName *wk);
void func_ov036_021b5ae8(PlaceName *wk);
void func_ov036_021b5b04(PlaceName *wk);
u16 PlaceName_GetReqZoneID(PlaceName *wk);
void SetPlaceNameDispZone(PlaceName *wk, u16 zoneId);
StrBuf *func_ov036_021b5b3c(PlaceName *wk);
BOOL CheckPopupPlaceNameDispAuto(u16 zoneId, BOOL a1);

static const ClActSurfaceSetup sPlaceNameSurfaceSetup = { 0, 0, 256, 192, 0, 1 };

static const BGSetup sPlaceNameBGSetup = { 0, 0, 0x800, 0, 1, 0, 2, 1, 0x8000, 0, 1, 0, 0 };

BOOL isGameVerWhite2(void) {
    if (getGameVersion() == 22) {
        return TRUE;
    }
    return FALSE;
}

PlaceName *FieldPlaceName_Create(GameSystem *gsys, HeapID heapId, void *msgBGSys) {
    PlaceName *wk = FieldPlaceName_Alloc(heapId);
    func_ov036_021b59d4(wk, gsys);
    func_ov036_021b5ae8(wk);
    func_ov036_021b540c(wk);
    func_ov036_021b5a14(wk);
    func_ov036_021b5a2c(wk);
    func_ov036_021b5a44(wk);
    func_ov036_021b5aa0(wk, heapId);
    func_ov036_021b5aa4(wk, func_ov036_0218799c(msgBGSys));
    func_ov036_021b5318(wk);
    func_ov036_021b5344(wk);
    func_ov036_021b53cc(wk);
    func_ov036_021b53f4(wk, 0);
    func_ov036_021b55ec(wk);
    func_ov036_021b5434(wk);
    GFL_BGSysCreateBG(3, &sPlaceNameBGSetup, 0);
    if (isGameVerWhite2()) {
        func_ov036_021b51e0(wk, 4);
        func_ov036_021b5230(wk, 3);
        func_ov036_021b5284(wk, 5);
    } else {
        func_ov036_021b51e0(wk, 1);
        func_ov036_021b5230(wk, 0);
        func_ov036_021b5284(wk, 2);
    }
    func_ov036_021b52e0(wk);
    func_ov036_021b56d4(wk, 0);
    return wk;
}

void FieldPlaceName_Free(PlaceName *wk) {
    func_ov036_021b5b04(wk);
    func_ov036_021b52ac(wk);
    func_ov036_021b5258(wk);
    func_ov036_021b5204(wk);
    func_ov036_021b54e0(wk);
    func_ov036_021b5a80(wk);
    func_ov036_021b5600(wk);
    func_ov036_021b53e8(wk);
    func_ov036_021b53a8(wk);
    func_ov036_021b5330(wk);
    func_ov036_021b5a58(wk);
    func_ov036_021b5a6c(wk);
    func_ov036_021b52fc(wk);
    func_ov036_021b5420(wk);
    GFL_BGSysReleaseBG(3);
    func_ov036_021b5a0c(wk);
}

void func_ov036_021b4ff8(PlaceName *wk) {
    func_ov036_021b5ab8(wk);
    switch (func_ov036_021b5aac(wk)) {
    case 0:
        func_ov036_021b5778(wk);
        break;
    case 1:
        func_ov036_021b57bc(wk);
        break;
    case 2:
        func_ov036_021b58b4(wk);
        break;
    case 3:
        func_ov036_021b5900(wk);
        break;
    case 4:
        func_ov036_021b5914(wk);
        break;
    case 5:
        func_ov036_021b5938(wk);
        break;
    case 6:
        func_ov036_021b5978(wk);
        break;
    }
    func_ov036_021b5570(wk);
}

void func_ov036_021b5064(PlaceName *wk) {
    switch (func_ov036_021b5aac(wk)) {
    case 0:
        func_ov036_021b5994(wk);
        break;
    case 1:
        func_ov036_021b5998(wk);
        break;
    case 2:
        __endColorBlending(wk);
        break;
    case 3:
        func_ov036_021b59a4(wk);
        break;
    case 4:
        func_ov036_021b59a8(wk);
        break;
    case 5:
        func_ov036_021b59ac(wk);
        break;
    case 6:
        _endColorBlending(wk);
        break;
    }
    PrintQueue_Main(wk->printQueue);
}

void func_ov036_021b50c8(PlaceName *wk, s32 zoneId) {
    SetPlaceNameDispZone(wk, zoneId);
    if (CheckPopupPlaceNameDispAuto(zoneId, FALSE)) {
        func_ov036_021b5708(wk);
        func_ov036_021b5628(wk);
    }
}

void func_ov036_021b50f4(PlaceName *wk, u32 zoneId) {
    SetPlaceNameDispZone(wk, zoneId);
    if (CheckPopupPlaceNameDispAuto(zoneId, FALSE)) {
        func_ov036_021b5708(wk);
        func_ov036_021b5614(wk);
    }
}

void BeginContinuePlaceNameDisp(PlaceName *wk, s32 zoneId) {
    SetPlaceNameDispZone(wk, zoneId);
    if (CheckPopupPlaceNameDispAuto(zoneId, TRUE)) {
        func_ov036_021b5708(wk);
        func_ov036_021b5614(wk);
    }
}

void BeginForcePlaceNameDisp(PlaceName *wk, s32 zoneId) {
    SetPlaceNameDispZone(wk, zoneId);
    func_ov036_021b5708(wk);
    func_ov036_021b5614(wk);
}

void func_ov036_021b5168(PlaceName *wk) {
    func_ov036_021b56d4(wk, 0);
    func_ov036_021b5630(wk);
    func_ov036_021b561c(wk);
}

void func_ov036_021b5180(PlaceName *wk) {
    func_ov036_021b51d0(wk);
}

void endColorBlending(PlaceName *wk) {
    G2_BlendNone();
}

void func_ov036_021b5194(PlaceName *wk) {
    func_ov036_021b521c(wk);
    func_ov036_021b52e0(wk);
    GFL_BGSysSetBGPriority(3, 1);
    GFL_BGSysSetBGEnabled(3, 0);
    endColorBlending(wk);
}

void func_ov036_021b51bc(PlaceName *wk) {
    GFL_BGSysClearBG(3);
    func_ov036_021b52fc(wk);
}

void func_ov036_021b51d0(PlaceName *wk) {
    func_ov036_021b51bc(wk);
    func_ov036_021b5194(wk);
}

void func_ov036_021b51e0(PlaceName *wk, u32 fileId) {
    HeapID heapId = FieldPlaceName_GetHeapID(wk);
    wk->paletteFile = GFL_G2DIOReadNCLRArc(FieldPlaceName_GetArc(wk), fileId, &wk->paletteData, heapId);
}

void func_ov036_021b5204(PlaceName *wk) {
    if (wk->paletteFile != NULL) {
        GFL_HeapFree(wk->paletteFile);
        wk->paletteFile = NULL;
        wk->paletteData = NULL;
    }
}

void func_ov036_021b521c(PlaceName *wk) {
    GFL_BGSysUploadStdPalette(3, wk->paletteData->rawData, 0x20, 0);
}

void func_ov036_021b5230(PlaceName *wk, u32 fileId) {
    HeapID heapId = FieldPlaceName_GetHeapID(wk);
    wk->charFile = GFL_G2DIOReadBGNCGRArc(FieldPlaceName_GetArc(wk), fileId, FALSE, &wk->charData, heapId);
}

void func_ov036_021b5258(PlaceName *wk) {
    if (wk->charFile != NULL) {
        GFL_HeapFree(wk->charFile);
        wk->charFile = NULL;
        wk->charData = NULL;
    }
}

void func_ov036_021b5270(PlaceName *wk) {
    GFL_BGSysLoadChar(3, wk->charData->rawData, wk->charData->size, 0);
}

void func_ov036_021b5284(PlaceName *wk, u32 fileId) {
    HeapID heapId = FieldPlaceName_GetHeapID(wk);
    wk->screenFile = GFL_G2DIOReadNSCRArc(FieldPlaceName_GetArc(wk), fileId, FALSE, &wk->screenData, heapId);
}

void func_ov036_021b52ac(PlaceName *wk) {
    if (wk->screenFile != NULL) {
        GFL_HeapFree(wk->screenFile);
        wk->screenFile = NULL;
        wk->screenData = NULL;
    }
}

void func_ov036_021b52c4(PlaceName *wk) {
    GFL_BGSysBufferScr(3, wk->screenData->rawData, wk->screenData->size, 0);
    GFL_BGSysQueueScrLoad(3);
}

void func_ov036_021b52e0(PlaceName *wk) {
    if (wk->charPos == (u32)-1) {
        wk->charPos = GFL_BGSysAllocChar(3, 0x20, 0);
    }
}

void func_ov036_021b52fc(PlaceName *wk) {
    if (wk->charPos != (u32)-1) {
        GFL_BGSysFreeCharMemory(3, wk->charPos, 0x20);
        wk->charPos = -1;
    }
}

void func_ov036_021b5318(PlaceName *wk) {
    wk->renderer = ClActRenderer_Create(&sPlaceNameSurfaceSetup, 1, wk->heapId);
}

void func_ov036_021b5330(PlaceName *wk) {
    if (wk->renderer != NULL) {
        ClActRenderer_Delete(wk->renderer);
        wk->renderer = NULL;
    }
}

void func_ov036_021b5344(PlaceName *wk) {
    HeapID heapId = FieldPlaceName_GetHeapID(wk);
    ArcTool *arc = FieldPlaceName_GetArc(wk);
    wk->palettes[0] = ClActSys_LoadPlttRange(arc, 10, 0, 0x20, 0, 1, heapId);
    wk->palettes[1] = ClActSys_LoadPlttRange(arc, 9, 0, 0x80, 0, 1, heapId);
    wk->chars = ClActSys_LoadChars(arc, 8, 0, 0, heapId);
    wk->cellAnims = ClActSys_LoadCellAnims(arc, 7, 6, heapId);
}

void func_ov036_021b53a8(PlaceName *wk) {
    int i;
    ClActSys_FreeChars(wk->chars);
    ClActSys_FreeCellAnims(wk->cellAnims);
    for (i = 0; i < 2; i++) {
        ClActSys_FreePltt(wk->palettes[i]);
    }
}

void func_ov036_021b53cc(PlaceName *wk) {
    wk->clactUnit = ClActUnit_Create(0x20, 1, FieldPlaceName_GetHeapID(wk));
    ClActUnit_SetActive(wk->clactUnit, TRUE);
}

void func_ov036_021b53e8(PlaceName *wk) {
    ClActUnit_Delete(wk->clactUnit);
}

void func_ov036_021b53f4(PlaceName *wk, u32 index) {
    if (wk->renderer != NULL) {
        ClActUnit_SetRenderer((&wk->clactUnit)[index], wk->renderer);
    }
}

void func_ov036_021b540c(PlaceName *wk) {
    wk->arc = GFL_ArcSysCreateFileHandle(ARCID_FIELD_PLACE_NAME, FieldPlaceName_GetHeapID(wk));
}

void func_ov036_021b5420(PlaceName *wk) {
    if (wk->arc != NULL) {
        GFL_ArcToolFree(wk->arc);
        wk->arc = NULL;
    }
}

void func_ov036_021b5434(PlaceName *wk) {
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
    wk->objects[0] =
        func_ov036_021b6824(heapId, 0, &text, wk->clactUnit, arc, 0, wk->palettes[0], wk->chars, wk->cellAnims);
    MI_CpuFill8(&frame, 0, sizeof(frame));
    frame.palette = wk->palettes[1];
    frame.chars = wk->chars;
    frame.cellAnims = wk->cellAnims;
    heapId = FieldPlaceName_GetHeapID(wk);
    arc = FieldPlaceName_GetArc(wk);
    wk->objects[1] =
        func_ov036_021b6824(heapId, 1, &frame, wk->clactUnit, arc, 0, wk->palettes[0], wk->chars, wk->cellAnims);
}

void func_ov036_021b54e0(PlaceName *wk) {
    int i;
    for (i = 0; i < 2; i++) {
        if (wk->objects[i] != NULL) {
            func_ov036_021b68bc(wk->objects[i]);
            wk->objects[i] = NULL;
        }
    }
}

void func_ov036_021b5500(PlaceName *wk) {
    PlaceNameTextParam text;
    u32 season;

    MI_CpuFill8(&text, 0, sizeof(text));
    text.font = func_ov036_021b5a9c(wk);
    text.zoneId = wk->shownZoneId;
    text.name = func_ov036_021b5b3c(wk);
    func_ov036_021b68d8(wk->objects[0], &text);
    MI_CpuFill8(&season, 0, sizeof(season));
    season = wk->season;
    func_ov036_021b68d8(wk->objects[1], &season);
}

void func_ov036_021b5550(PlaceName *wk) {
    int i;
    for (i = 0; i < 2; i++) {
        func_ov036_021b6920(wk->objects[i]);
    }
    GFL_BGSysSetBGEnabled(3, 0);
}

void func_ov036_021b5570(PlaceName *wk) {
    int i;
    for (i = 0; i < 2; i++) {
        func_ov036_021b68e8(wk->objects[i]);
    }
}

BOOL func_ov036_021b5588(PlaceName *wk) {
    BOOL done = TRUE;
    int i;
    for (i = 0; i < 2; i++) {
        if (!func_ov036_021b6940(wk->objects[i])) {
            done = FALSE;
            break;
        }
    }
    return done;
}

void func_ov036_021b55ac(PlaceName *wk) {
    int i;
    for (i = 0; i < 2; i++) {
        func_ov036_021b6900(wk->objects[i], wk->timer, 10);
    }
}

void func_ov036_021b55cc(PlaceName *wk) {
    int i;
    for (i = 0; i < 2; i++) {
        func_ov036_021b6910(wk->objects[i], 0, 12);
    }
}

void func_ov036_021b55ec(PlaceName *wk) {
    wk->bmpOam = BmpOam_Init(FieldPlaceName_GetHeapID(wk), wk->clactUnit);
}

void func_ov036_021b5600(PlaceName *wk) {
    if (wk->bmpOam != NULL) {
        BmpOam_Exit(wk->bmpOam);
        wk->bmpOam = NULL;
    }
}

void func_ov036_021b5614(PlaceName *wk) {
    wk->forced = TRUE;
}

void func_ov036_021b561c(PlaceName *wk) {
    wk->forced = FALSE;
}

u32 func_ov036_021b5624(PlaceName *wk) {
    return wk->requested;
}

void func_ov036_021b5628(PlaceName *wk) {
    wk->requested = TRUE;
}

void func_ov036_021b5630(PlaceName *wk) {
    wk->requested = FALSE;
}

u32 func_ov036_021b5638(PlaceName *wk) {
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

void func_ov036_021b5660(PlaceName *wk) {
    u16 zoneId = PlaceName_GetReqZoneID(wk);
    u16 nameId = ZoneData_GetPlaceNameID(zoneId);
    if (nameId >= 0x9a) {
        nameId = 0;
    }
    if (nameId == 0) {
        func_ov036_021b5168(wk);
        return;
    }
    if (IsZoneJoinAvenue(zoneId)) {
        GFL_StrBufLoadString(
            wk->name, (const u16 *)JoinAvenue_GetParam(
                          JoinAvenue_GetInfo(SaveControl_GetJoinAvenue(GameData_GetSaveControl(wk->gameData))), 0, 0));
    } else {
        GFL_MsgDataLoadStrbuf(wk->msgData, nameId, wk->name);
    }
    wk->season = GameData_GetSeason(wk->gameData);
    wk->charCount = GFL_StrBufGetCharCount(wk->name);
    wk->shownZoneId = zoneId;
}

void func_ov036_021b56d4(PlaceName *wk, u32 mode) {
    func_ov036_021b5aa8(wk, mode);
    func_ov036_021b5ab0(wk);
    func_ov036_021b5ad0(wk);
    if (mode == 0) {
        func_ov036_021b5550(wk);
        GX_SetVisibleWnd(GX_WNDMASK_NONE);
    }
}

void func_ov036_021b5708(PlaceName *wk) {
    u32 mode;
    s32 timer;
    switch (wk->mode) {
    case 0:
    case 6:
        return;
    case 3:
    case 4:
    case 5:
        mode = 5;
        timer = 0;
        break;
    case 2:
        mode = 5;
        timer = 12.0f * (1.0f - wk->timer / 10.0f);
        break;
    case 1:
        mode = 0;
        timer = 0;
        break;
    }
    func_ov036_021b56d4(wk, mode);
    wk->timer = timer;
}

void func_ov036_021b5778(PlaceName *wk) {
    BOOL start = FALSE;
    if (func_ov036_021b5638(wk)) {
        start = TRUE;
        func_ov036_021b561c(wk);
    } else if (func_ov036_021b5624(wk)) {
        if (IsLocNameIdxChanged(wk)) {
            start = TRUE;
        }
        func_ov036_021b5630(wk);
    }
    if (start) {
        func_ov036_021b56d4(wk, 1);
    }
}

void func_ov036_021b57bc(PlaceName *wk) {
    switch (func_ov036_021b5ac8(wk)) {
    case 0:
        func_ov036_021b5660(wk);
        func_ov036_021b5ad8(wk);
        break;
    case 1:
        func_ov036_021b5ad8(wk);
        break;
    case 2:
        GFL_BGSysClearScr(3);
        func_ov036_021b5ad8(wk);
        break;
    case 3:
        func_ov036_021b521c(wk);
        func_ov036_021b5ad8(wk);
        break;
    case 4:
        func_ov036_021b5270(wk);
        func_ov036_021b5ad8(wk);
        break;
    case 5:
        func_ov036_021b52c4(wk);
        func_ov036_021b5ad8(wk);
        break;
    case 6:
        GFL_BGSysSetBGPriority(3, 1);
        GFL_BGSysSetBGEnabled(3, 0);
        endColorBlending(wk);
        GX_SetVisibleWnd(GX_WNDMASK_OW);
        G2_SetWndOBJInsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 |
                                    GX_WND_PLANEMASK_BG3 | GX_WND_PLANEMASK_OBJ,
                                TRUE);
        G2_SetWndOutsidePlane(GX_WND_PLANEMASK_BG0 | GX_WND_PLANEMASK_BG1 | GX_WND_PLANEMASK_BG2 | GX_WND_PLANEMASK_OBJ,
                              TRUE);
        func_ov036_021b5ad8(wk);
        break;
    case 7:
        func_ov036_021b5500(wk);
        func_ov036_021b5ad8(wk);
        break;
    case 8:
        if (func_ov036_021b5588(wk)) {
            func_ov036_021b56d4(wk, 2);
        }
        break;
    }
}

void func_ov036_021b58b4(PlaceName *wk) {
    switch (func_ov036_021b5ac8(wk)) {
    case 0:
        __endColorBlending(wk);
        GFL_BGSysSetBGEnabled(3, 1);
        func_ov036_021b55ac(wk);
        func_ov036_021b5ad8(wk);
        break;
    case 1:
        if (wk->timer > 10 && func_ov036_021b5588(wk)) {
            func_ov036_021b56d4(wk, 3);
        }
        break;
    }
}

void func_ov036_021b5900(PlaceName *wk) {
    if (wk->timer > 30) {
        func_ov036_021b56d4(wk, 4);
    }
}

void func_ov036_021b5914(PlaceName *wk) {
    switch (func_ov036_021b5ac8(wk)) {
    case 0:
        func_ov036_021b5ad8(wk);
        break;
    case 1:
        func_ov036_021b56d4(wk, 5);
        break;
    }
}

void func_ov036_021b5938(PlaceName *wk) {
    switch (func_ov036_021b5ac8(wk)) {
    case 0:
        func_ov036_021b55cc(wk);
        func_ov036_021b5ad8(wk);
        break;
    case 1:
        if (wk->timer > 12 && func_ov036_021b5588(wk)) {
            func_ov036_021b56d4(wk, 6);
        }
        break;
    }
}

void func_ov036_021b5978(PlaceName *wk) {
    if (wk->timer > 5) {
        func_ov036_021b56d4(wk, 0);
        func_ov036_021b5550(wk);
    }
}

void func_ov036_021b5994(PlaceName *wk) {
}

void func_ov036_021b5998(PlaceName *wk) {
}

void __endColorBlending(PlaceName *wk) {
    endColorBlending(wk);
}

void func_ov036_021b59a4(PlaceName *wk) {
}

void func_ov036_021b59a8(PlaceName *wk) {
}

void func_ov036_021b59ac(PlaceName *wk) {
}

void _endColorBlending(PlaceName *wk) {
    endColorBlending(wk);
}

PlaceName *FieldPlaceName_Alloc(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(PlaceName), TRUE, "field_place_name.c", 1927);
}

void func_ov036_021b59d4(PlaceName *wk, GameSystem *gsys) {
    MI_CpuFill8(wk, 0, sizeof(PlaceName));
    wk->gsys = gsys;
    wk->gameData = GSYS_GetGameData(gsys);
    wk->charPos = -1;
    wk->forced = 0;
    wk->requested = 0;
    wk->prevZoneId = 0x267;
    wk->reqZoneId = 0x267;
}

void func_ov036_021b5a0c(PlaceName *wk) {
    GFL_HeapFree(wk);
}

void func_ov036_021b5a14(PlaceName *wk) {
    wk->printQueue = PrintQueue_CreateDefault(FieldPlaceName_GetHeapID(wk));
    PrintQueue_SetQueueAlways(wk->printQueue, TRUE);
}

void func_ov036_021b5a2c(PlaceName *wk) {
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, 0x6d, FieldPlaceName_GetHeapID(wk));
}

void func_ov036_021b5a44(PlaceName *wk) {
    wk->name = GFL_StrBufCreate(0x1d, FieldPlaceName_GetHeapID(wk));
}

void func_ov036_021b5a58(PlaceName *wk) {
    if (wk->printQueue != NULL) {
        PrintQueue_Free(wk->printQueue);
        wk->printQueue = NULL;
    }
}

void func_ov036_021b5a6c(PlaceName *wk) {
    if (wk->msgData != NULL) {
        GFL_MsgDataFree(wk->msgData);
        wk->msgData = NULL;
    }
}

void func_ov036_021b5a80(PlaceName *wk) {
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

Font *func_ov036_021b5a9c(PlaceName *wk) {
    return wk->font;
}

void func_ov036_021b5aa0(PlaceName *wk, HeapID heapId) {
    wk->heapId = heapId;
}

void func_ov036_021b5aa4(PlaceName *wk, Font *font) {
    wk->font = font;
}

void func_ov036_021b5aa8(PlaceName *wk, u32 mode) {
    wk->mode = mode;
}

u32 func_ov036_021b5aac(PlaceName *wk) {
    return wk->mode;
}

void func_ov036_021b5ab0(PlaceName *wk) {
    wk->timer = 0;
}

void func_ov036_021b5ab8(PlaceName *wk) {
    wk->timer++;
}

u8 func_ov036_021b5ac8(PlaceName *wk) {
    return wk->state;
}

void func_ov036_021b5ad0(PlaceName *wk) {
    wk->state = 0;
}

void func_ov036_021b5ad8(PlaceName *wk) {
    wk->state++;
}

void func_ov036_021b5ae8(PlaceName *wk) {
    wk->reqZoneId =
        PlayerSave_GetPlaceNameZoneID(SaveControl_GetPlayerSave(GameData_GetSaveControl(GSYS_GetGameData(wk->gsys))));
}

void func_ov036_021b5b04(PlaceName *wk) {
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

StrBuf *func_ov036_021b5b3c(PlaceName *wk) {
    return wk->name;
}

BOOL CheckPopupPlaceNameDispAuto(u16 zoneId, BOOL a1) {
    switch (GetZoneLocNameDispType(zoneId)) {
    case 0:
        return FALSE;
    case 1:
        return TRUE;
    case 2:
        return a1;
    case 3:
        if (a1) {
            return FALSE;
        }
        return TRUE;
    default:
        return FALSE;
    }
}
