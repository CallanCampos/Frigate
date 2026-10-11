// The Battle Recorder's resources: the font, the messages and the word set, the BGs and cell actor graphics its
// screens load by ID, and the palettes of the color the player chose for it. The name is the ROM's string, from
// GFL_HeapAllocate's asserts

#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "app/battle_recorder/br_fade.h"
#include "app/battle_recorder/br_res.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "system/gf_font.h"
#include "system/wordset.h"

#define BR_RES_COLOR_MAX 8

struct BrRes {
    // The palette of the buttons and other common cell actors, for each screen
    u32 common_obj_plt[2];
    BrResObjData obj[BR_RES_OBJ_MAX];
    BOOL obj_flag[BR_RES_OBJ_MAX];
    Font *font;
    MsgData *msg;
    WordSet *wordset;
    // Whether the player's color is used, or the Global Link's
    BOOL isUseColor;
    u32 color;
};

static u16 BrRes_GetCommonObjPltt(u32 color);
static u16 BrRes_GetCommonBgPltt(u32 color);
static u16 BrRes_GetCommonFontPltt(u32 color);
static u16 BrRes_GetFadeColorByColor(u32 color);

// The palettes and the fade color of each color the Battle Recorder can have
static const u32 sc_common_obj_plt[BR_RES_COLOR_MAX] = { 0x4f, 0x16, 0x50, 0x51, 0x52, 0x53, 0x5b, 0x60 };
static const u32 sc_common_bg_plt[BR_RES_COLOR_MAX] = { 0x4a, 0x18, 0x4b, 0x4c, 0x4d, 0x4e, 0x5a, 0x5f };
static const u32 sc_common_font_plt[BR_RES_COLOR_MAX] = { 0x40, 0x17, 0x41, 0x42, 0x43, 0x44, 0x58, 0x5d };
static const u32 sc_fade_color[BR_RES_COLOR_MAX] = { 0x1642, 0x7e05, 0x357f, 0x3def, 0x031f, 0x00bc, 0x0131, 0x023f };

BrRes *BrRes_Init(u32 color, BOOL isUseColor, HeapID heapId) {
    BrRes *p_wk = GFL_HeapAllocate(heapId, sizeof(BrRes), FALSE, "br_res.c", 96);

    sys_memset(p_wk, 0, sizeof(BrRes));
    p_wk->isUseColor = isUseColor;
    if (p_wk->isUseColor) {
        p_wk->color = color;
        // The Global Link's color isn't one the player can choose
        if (p_wk->color == BR_RES_COLOR_GLOBAL) {
            p_wk->color = 0;
        }
    } else {
        p_wk->color = BR_RES_COLOR_GLOBAL;
    }

    p_wk->font = GFL_FontCreate(23, 0, 0, FALSE, heapId);
    p_wk->msg = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0008, heapId);
    p_wk->wordset = GFL_WordSetSystemCreate(8, 64, heapId);
    BrRes_LoadCommonPltt(p_wk, CLACT_VRAM_MAIN, heapId);
    BrRes_LoadCommonPltt(p_wk, CLACT_VRAM_SUB, heapId);
    return p_wk;
}

void BrRes_Exit(BrRes *p_wk) {
    BrRes_UnloadCommonPltt(p_wk, CLACT_VRAM_MAIN);
    BrRes_UnloadCommonPltt(p_wk, CLACT_VRAM_SUB);
    GFL_WordSetSystemFree(p_wk->wordset);
    GFL_MsgDataFree(p_wk->msg);
    GFL_FontFree(p_wk->font);
    GFL_HeapFree(p_wk);
}

void BrRes_LoadBG(BrRes *p_wk, u32 bgID, HeapID heapId) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(137, heapId);

    switch (bgID) {
    case 0:
        GFL_BGSysLoadArcNCGRStatic(handle, 38, 3, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 40, 3, 0, 0, FALSE, heapId);
        break;
    case 1:
        GFL_BGSysLoadArcNCGRStatic(handle, 38, 6, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 40, 6, 0, 0, FALSE, heapId);
        break;
    case 2:
        GFL_BGSysLoadArcNCGRStatic(handle, 107, 0, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 2, 0, 0, 0, FALSE, heapId);
        break;
    case 3:
        GFL_BGSysLoadArcNCGRStatic(handle, 38, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 39, 2, 0, 0, FALSE, heapId);
        break;
    case 4:
        GFL_BGSysLoadArcNCGRStatic(handle, 38, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 26, 2, 0, 0, FALSE, heapId);
        break;
    case 5:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 5, 5, 0, 0, FALSE, heapId);
        break;
    case 6:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 45, 2, 0, 0, FALSE, heapId);
        break;
    case 7:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 44, 2, 0, 0, FALSE, heapId);
        break;
    case 8:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 43, 2, 0, 0, FALSE, heapId);
        break;
    case 9:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 97, 5, 0, 0, FALSE, heapId);
        break;
    case 10:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 99, 2, 0, 0, FALSE, heapId);
        break;
    case 11:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 100, 2, 0, 0, FALSE, heapId);
        break;
    case 12:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 101, 2, 0, 0, FALSE, heapId);
        break;
    case 13:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 102, 2, 0, 0, FALSE, heapId);
        break;
    case 19:
        GFL_BGSysLoadArcNCGRStatic(handle, 107, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 2, 2, 0, 0, FALSE, heapId);
        break;
    case 14:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 98, 2, 0, 0, FALSE, heapId);
        break;
    case 15:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 103, 5, 0, 0, FALSE, heapId);
        break;
    case 18:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 104, 2, 0, 0, FALSE, heapId);
        break;
    case 17:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 105, 2, 0, 0, FALSE, heapId);
        break;
    case 16:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 106, 2, 0, 0, FALSE, heapId);
        break;
    case 20:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 31, 5, 0, 0, FALSE, heapId);
        break;
    case 21:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 29, 5, 0, 0, FALSE, heapId);
        break;
    case 22:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 13, 5, 0, 0, FALSE, heapId);
        break;
    case 23:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 14, 5, 0, 0, FALSE, heapId);
        break;
    case 24:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 3, 2, 0, 0, FALSE, heapId);
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 6, 5, 0, 0, FALSE, heapId);
        break;
    case 25:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 2, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 27, 2, 0, 0, FALSE, heapId);
        break;
    case 26:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 30, 5, 0, 0, FALSE, heapId);
        break;
    case 28:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 28, 5, 0, 0, FALSE, heapId);
        break;
    case 27:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 32, 5, 0, 0, FALSE, heapId);
        break;
    case 29:
        GFL_BGSysLoadArcNCGRStatic(handle, 38, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 18, 5, 0, 0, FALSE, heapId);
        break;
    case 30:
        GFL_BGSysLoadArcNCGRStatic(handle, 33, 5, 0, 0, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(handle, 47, 5, 0, 0, FALSE, heapId);
        break;
    }
    GFL_ArcToolFree(handle);
}

void BrRes_UnloadBG(BrRes *p_wk, u32 bgID) {
    switch (bgID) {
    case 24:
        GFL_BGSysClearScr(2);
        GFL_BGSysLoadScr(2);
        GFL_BGSysClearScr(5);
        GFL_BGSysLoadScr(5);
        break;
    case 0:
        GFL_BGSysClearScr(3);
        GFL_BGSysLoadScr(3);
        break;
    case 1:
        GFL_BGSysClearScr(6);
        GFL_BGSysLoadScr(6);
        break;
    case 2:
        GFL_BGSysClearScr(0);
        GFL_BGSysLoadScr(0);
        break;
    case 5:
    case 9:
    case 15:
    case 20:
    case 21:
    case 22:
    case 23:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
        GFL_BGSysClearScr(5);
        GFL_BGSysLoadScr(5);
        break;
    case 3:
    case 4:
    case 6:
    case 7:
    case 8:
    case 10:
    case 11:
    case 12:
    case 13:
    case 16:
    case 17:
    case 18:
    case 19:
    case 25:
        GFL_BGSysClearScr(2);
        GFL_BGSysLoadScr(2);
        break;
    }
}

void BrRes_LoadOBJ(BrRes *p_wk, u32 objID, HeapID heapId) {
    BrResObjData *p_data = &p_wk->obj[objID];
    ArcTool *handle;

    // clang-format off
    GFL_ASSERT(p_wk->obj_flag[ objID ] == FALSE);
    // clang-format on

    switch (objID) {
    case BR_RES_OBJ_SIDEBAR_M:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = p_wk->common_obj_plt[CLACT_VRAM_MAIN];
        p_data->chr = func_0204b81c(handle, 36, FALSE, CLACT_VRAM_MAIN, heapId);
        p_data->cell = func_0204bde0(handle, 21, 20, heapId);
        GFL_ArcToolFree(handle);
        break;
    case BR_RES_OBJ_SIDEBAR_S:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = p_wk->common_obj_plt[CLACT_VRAM_SUB];
        p_data->chr = func_0204b81c(handle, 36, FALSE, CLACT_VRAM_SUB, heapId);
        p_data->cell = func_0204bde0(handle, 21, 20, heapId);
        GFL_ArcToolFree(handle);
        break;
    case BR_RES_OBJ_BROWSE_BTN_M:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = p_wk->common_obj_plt[CLACT_VRAM_MAIN];
        p_data->chr = func_0204b81c(handle, 34, FALSE, CLACT_VRAM_MAIN, heapId);
        p_data->cell = func_0204bde0(handle, 9, 10, heapId);
        GFL_ArcToolFree(handle);
        break;
    case BR_RES_OBJ_BROWSE_BTN_S:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = p_wk->common_obj_plt[CLACT_VRAM_SUB];
        p_data->chr = func_0204b81c(handle, 34, FALSE, CLACT_VRAM_SUB, heapId);
        p_data->cell = func_0204bde0(handle, 9, 10, heapId);
        GFL_ArcToolFree(handle);
        break;
    case BR_RES_OBJ_MUSICAL_BTN_M:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = p_wk->common_obj_plt[CLACT_VRAM_MAIN];
        p_data->chr = func_0204b81c(handle, 63, FALSE, CLACT_VRAM_MAIN, heapId);
        p_data->cell = func_0204bde0(handle, 61, 62, heapId);
        GFL_ArcToolFree(handle);
        break;
    case BR_RES_OBJ_MUSICAL_BTN_S:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = p_wk->common_obj_plt[CLACT_VRAM_SUB];
        p_data->chr = func_0204b81c(handle, 63, FALSE, CLACT_VRAM_SUB, heapId);
        p_data->cell = func_0204bde0(handle, 61, 62, heapId);
        GFL_ArcToolFree(handle);
        break;
    case 6:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = p_wk->common_obj_plt[CLACT_VRAM_SUB];
        p_data->chr = func_0204b81c(handle, 35, FALSE, CLACT_VRAM_SUB, heapId);
        p_data->cell = func_0204bde0(handle, 7, 8, heapId);
        GFL_ArcToolFree(handle);
        break;
    case 8:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = p_wk->common_obj_plt[CLACT_VRAM_SUB];
        p_data->chr = func_0204b81c(handle, 86, FALSE, CLACT_VRAM_SUB, heapId);
        p_data->cell = func_0204bde0(handle, 85, 84, heapId);
        GFL_ArcToolFree(handle);
        break;
    case 7:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = p_wk->common_obj_plt[CLACT_VRAM_MAIN];
        p_data->chr = func_0204b81c(handle, 86, FALSE, CLACT_VRAM_MAIN, heapId);
        p_data->cell = func_0204bde0(handle, 85, 84, heapId);
        GFL_ArcToolFree(handle);
        break;
    case 11:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = func_0204bbb8(handle, 19, CLACT_VRAM_SUB, 0x80, 0, 1, heapId);
        p_data->chr = func_0204b81c(handle, 15, FALSE, CLACT_VRAM_SUB, heapId);
        p_data->cell = func_0204bde0(handle, 16, 17, heapId);
        GFL_ArcToolFree(handle);
        break;
    case 12:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = func_0204bbb8(handle, 51, CLACT_VRAM_SUB, 0xa0, 0, 1, heapId);
        p_data->chr = func_0204b81c(handle, 50, FALSE, CLACT_VRAM_SUB, heapId);
        p_data->cell = func_0204bde0(handle, 48, 49, heapId);
        GFL_ArcToolFree(handle);
        break;
    case 9:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = p_wk->common_obj_plt[CLACT_VRAM_MAIN];
        p_data->chr = func_0204b81c(handle, 54, FALSE, CLACT_VRAM_MAIN, heapId);
        p_data->cell = func_0204bde0(handle, 52, 53, heapId);
        GFL_ArcToolFree(handle);
        break;
    case 10:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = p_wk->common_obj_plt[CLACT_VRAM_SUB];
        p_data->chr = func_0204b81c(handle, 54, FALSE, CLACT_VRAM_SUB, heapId);
        p_data->cell = func_0204bde0(handle, 52, 53, heapId);
        GFL_ArcToolFree(handle);
        break;
    case 13:
        handle = GFL_ArcSysCreateFileHandle(137, heapId);
        p_data->plt = p_wk->common_obj_plt[CLACT_VRAM_MAIN];
        p_data->cell = func_0204bde0(handle, 41, 42, heapId);
        p_data->chr = func_0204b81c(handle, 37, FALSE, CLACT_VRAM_MAIN, heapId);
        GFL_ArcToolFree(handle);
        break;
    }
    p_wk->obj_flag[objID] = TRUE;
}

// The sidebars' objects are only marked unloaded; their graphics stay in VRAM
void BrRes_UnloadOBJ(BrRes *p_wk, u32 objID) {
    BrResObjData *p_data = &p_wk->obj[objID];

    switch (objID) {
    case 11:
    case 12:
        func_0204bcd0(p_data->plt);
        func_0204b98c(p_data->chr);
        func_0204be64(p_data->cell);
        break;
    case BR_RES_OBJ_BROWSE_BTN_M:
    case BR_RES_OBJ_BROWSE_BTN_S:
    case BR_RES_OBJ_MUSICAL_BTN_M:
    case BR_RES_OBJ_MUSICAL_BTN_S:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 13:
        func_0204b98c(p_data->chr);
        func_0204be64(p_data->cell);
        break;
    }
    p_wk->obj_flag[objID] = FALSE;
}

BOOL BrRes_GetObjData(const BrRes *p_wk, u32 objID, BrResObjData *p_data) {
    if (p_wk->obj_flag[objID]) {
        *p_data = p_wk->obj[objID];
    }
    return p_wk->obj_flag[objID];
}

void BrRes_LoadCommonPltt(BrRes *p_wk, u32 vramType, HeapID heapId) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(137, heapId);

    p_wk->common_obj_plt[vramType] =
        func_0204bbb8(handle, BrRes_GetCommonObjPltt(p_wk->color), vramType, 0, 0, 13, heapId);
    if (vramType == CLACT_VRAM_MAIN) {
        GFL_G2DIOLoadArcNCLRDefault(handle, BrRes_GetCommonBgPltt(p_wk->color), 0, 0, 0, heapId);
        GFL_G2DIOLoadArcNCLRDefault(handle, BrRes_GetCommonFontPltt(p_wk->color), 0, 0x1c0, 0x20, heapId);
    } else {
        GFL_G2DIOLoadArcNCLRDefault(handle, BrRes_GetCommonBgPltt(p_wk->color), 4, 0, 0, heapId);
        GFL_G2DIOLoadArcNCLRDefault(handle, BrRes_GetCommonFontPltt(p_wk->color), 4, 0x1c0, 0x20, heapId);
    }
    GFL_ArcToolFree(handle);
}

void BrRes_UnloadCommonPltt(BrRes *p_wk, u32 vramType) {
    func_0204bcd0(p_wk->common_obj_plt[vramType]);
    p_wk->common_obj_plt[vramType] = 0xffffffff;
}

Font *BrRes_GetFont(const BrRes *p_wk) {
    return p_wk->font;
}

MsgData *BrRes_GetMsgData(const BrRes *p_wk) {
    return p_wk->msg;
}

WordSet *BrRes_GetWordSet(const BrRes *p_wk) {
    return p_wk->wordset;
}

u16 BrRes_GetFadeColor(const BrRes *p_wk) {
    return BrRes_GetFadeColorByColor(p_wk->color);
}

void BrRes_SetColor(BrRes *p_wk, u32 color) {
    p_wk->color = color;
}

u32 BrRes_GetColor(const BrRes *p_wk) {
    return p_wk->color;
}

// Loads the palettes of the color into the fade's buffers, to change the color with a fade
void BrRes_LoadColorPlttToFade(BrRes *p_wk, BrFade *fade, HeapID heapId) {
    ArcTool *handle = GFL_ArcSysCreateFileHandle(137, heapId);

    BrFade_LoadPlttArc(fade, handle, BrRes_GetCommonObjPltt(p_wk->color), 2, 0, 0x1c0, heapId);
    BrFade_LoadPlttArc(fade, handle, BrRes_GetCommonObjPltt(p_wk->color), 3, 0, 0x1c0, heapId);
    BrFade_LoadPlttArc(fade, handle, BrRes_GetCommonBgPltt(p_wk->color), 0, 0, 0, heapId);
    BrFade_LoadPlttArc(fade, handle, BrRes_GetCommonBgPltt(p_wk->color), 1, 0, 0, heapId);
    BrFade_LoadPlttArc(fade, handle, BrRes_GetCommonFontPltt(p_wk->color), 0, 0xe0, 0x20, heapId);
    BrFade_LoadPlttArc(fade, handle, BrRes_GetCommonFontPltt(p_wk->color), 1, 0xe0, 0x20, heapId);
    GFL_ArcToolFree(handle);
}

static u16 BrRes_GetCommonObjPltt(u32 color) {
    GFL_ASSERT(color < NELEMS(sc_common_obj_plt));
    return sc_common_obj_plt[color];
}

static u16 BrRes_GetCommonBgPltt(u32 color) {
    GFL_ASSERT(color < NELEMS(sc_common_bg_plt));
    return sc_common_bg_plt[color];
}

static u16 BrRes_GetCommonFontPltt(u32 color) {
    GFL_ASSERT(color < NELEMS(sc_common_font_plt));
    return sc_common_font_plt[color];
}

static u16 BrRes_GetFadeColorByColor(u32 color) {
    GFL_ASSERT(color < NELEMS(sc_fade_color));
    return sc_fade_color[color];
}
