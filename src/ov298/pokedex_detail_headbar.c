// The ROM names this file zukan_detail_headbar.c.
#include "types.h"
#include "app/pokedex_detail.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "system/printsys.h"
#include "text/system/btl_server_flow_title.h"

// The detail screen's head bar, on BG 5 at the top of the sub screen: the title of the page, which slides in and out

#define HEADBAR_BG 5
// How far the bar is hidden above the screen
#define HIDDEN_Y 24

enum {
    HEADBAR_HIDDEN,
    HEADBAR_APPEARING,
    HEADBAR_SHOWN,
    HEADBAR_DISAPPEARING,
};

struct PokedexDetailHeadbar {
    HeapID heapId;
    Font *font;
    int state;
    // The title shown, from POKEDEX_DETAIL_HEADBAR_TITLE_COUNT for none
    int title;
    u8 wait;
    BmpWin *window;
    // The bar's characters with the title printed on them, and where they are in the BG's characters
    GFLBitmap *bitmap;
    u32 charPos;
    u32 charSize;
    MsgData *msgData;
    TCB *vblankTcb;
    PrintQueue *printQueue;
    BOOL transferPending;
};

static void PokedexDetailHeadbar_VBlank(TCB *tcb, void *data);
static void PokedexDetailHeadbar_InitBG(PokedexDetailHeadbar *headbar);
static void PokedexDetailHeadbar_FreeBG(PokedexDetailHeadbar *headbar);
static void PokedexDetailHeadbar_PrintTitle(PokedexDetailHeadbar *headbar);
static void PokedexDetailHeadbar_ClearTitle(PokedexDetailHeadbar *headbar);
static void PokedexDetailHeadbar_TransferTitle(PokedexDetailHeadbar *headbar);

PokedexDetailHeadbar *PokedexDetailHeadbar_Create(HeapID heapId, Font *font) {
    PokedexDetailHeadbar *headbar =
        GFL_HeapAllocate(heapId, sizeof(PokedexDetailHeadbar), TRUE, "zukan_detail_headbar.c", 120);

    headbar->heapId = heapId;
    headbar->font = font;
    headbar->state = HEADBAR_HIDDEN;
    headbar->title = POKEDEX_DETAIL_HEADBAR_TITLE_COUNT;
    GFL_BGSysMoveBG(HEADBAR_BG, BG_MOVE_SET_Y, HIDDEN_Y);
    GFL_BGSysSetBGPriority(HEADBAR_BG, 0);
    PokedexDetailHeadbar_InitBG(headbar);
    PokedexDetailHeadbar_PrintTitle(headbar);
    headbar->vblankTcb = GFL_VBlankTCBAdd(PokedexDetailHeadbar_VBlank, headbar, 1);
    headbar->printQueue = PrintQueue_CreateDefault(headbar->heapId);
    headbar->transferPending = FALSE;
    return headbar;
}

void PokedexDetailHeadbar_Free(PokedexDetailHeadbar *headbar) {
    PrintQueue_Clear(headbar->printQueue);
    PrintQueue_Free(headbar->printQueue);
    GFL_TCBRemove(headbar->vblankTcb);
    PokedexDetailHeadbar_ClearTitle(headbar);
    PokedexDetailHeadbar_FreeBG(headbar);
    GFL_HeapFree(headbar);
}

void PokedexDetailHeadbar_Update(PokedexDetailHeadbar *headbar) {
    switch (headbar->state) {
    case HEADBAR_HIDDEN:
    case HEADBAR_SHOWN:
        break;
    case HEADBAR_APPEARING:
        if (headbar->wait == 0) {
            if (GFL_BGSysGetBGOffsetY(HEADBAR_BG) <= 0) {
                GFL_BGSysMoveBGReq(HEADBAR_BG, BG_MOVE_SET_Y, 0);
                headbar->state = HEADBAR_SHOWN;
            } else {
                GFL_BGSysMoveBGReq(HEADBAR_BG, BG_MOVE_UP, 3);
                headbar->wait = 0;
            }
        } else {
            headbar->wait--;
        }
        break;
    case HEADBAR_DISAPPEARING:
        if (headbar->wait == 0) {
            if (GFL_BGSysGetBGOffsetY(HEADBAR_BG) >= HIDDEN_Y) {
                GFL_BGSysMoveBGReq(HEADBAR_BG, BG_MOVE_SET_Y, HIDDEN_Y);
                headbar->state = HEADBAR_HIDDEN;
            } else {
                GFL_BGSysMoveBGReq(HEADBAR_BG, BG_MOVE_DOWN, 3);
                headbar->wait = 0;
            }
        } else {
            headbar->wait--;
        }
        break;
    }

    PokedexDetailHeadbar_TransferTitle(headbar);
    PrintQueue_Main(headbar->printQueue);
}

void PokedexDetailHeadbar_SetTitle(PokedexDetailHeadbar *headbar, int title) {
    headbar->title = title;
    PokedexDetailHeadbar_ClearTitle(headbar);
    PokedexDetailHeadbar_PrintTitle(headbar);
}

int PokedexDetailHeadbar_GetState(PokedexDetailHeadbar *headbar) {
    return headbar->state;
}

void PokedexDetailHeadbar_Appear(PokedexDetailHeadbar *headbar) {
    if (headbar->state != HEADBAR_SHOWN) {
        headbar->state = HEADBAR_APPEARING;
        headbar->wait = 0;
    }
}

void PokedexDetailHeadbar_Disappear(PokedexDetailHeadbar *headbar) {
    if (headbar->state != HEADBAR_HIDDEN) {
        headbar->state = HEADBAR_DISAPPEARING;
        headbar->wait = 0;
    }
}

static void PokedexDetailHeadbar_VBlank(TCB *tcb, void *data) {
}

static void PokedexDetailHeadbar_InitBG(PokedexDetailHeadbar *headbar) {
    headbar->window = BmpWin_CreateDynamic(HEADBAR_BG, 0, 0, 16, 1, 13, FALSE);
    GFL_BitmapFill(BmpWin_GetBitmap(headbar->window), 0);
    BmpWin_FlushChar(headbar->window);
    headbar->bitmap = GFL_G2DIOLoadBitmap(ARCID_POKEDEX_GRA, 14, FALSE, headbar->heapId);
    headbar->charSize = GFL_BitmapCalcPixelDataSize(headbar->bitmap);
    headbar->charPos = GFL_BGSysLoadCharDynamic(HEADBAR_BG, GFL_BitmapGetPixelData(headbar->bitmap), headbar->charSize);
    loadBGScrToVramByNarcNoReserve(ARCID_POKEDEX_GRA, 38, HEADBAR_BG, 0, headbar->charPos, 0xc0, FALSE,
                                   headbar->heapId);
    GFL_BGSysSetScrPaletteNo(HEADBAR_BG, 0, 0, 32, 24, 13);
    headbar->msgData =
        GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_SERVER_FLOW_TITLE, headbar->heapId);
}

static void PokedexDetailHeadbar_FreeBG(PokedexDetailHeadbar *headbar) {
    GFL_MsgDataFree(headbar->msgData);
    GFL_BGSysFreeCharMemory(HEADBAR_BG, headbar->charPos, headbar->charSize);
    GFL_BitmapFree(headbar->bitmap);
    BmpWin_Free(headbar->window);
}

static void PokedexDetailHeadbar_PrintTitle(PokedexDetailHeadbar *headbar) {
    if (headbar->title >= 0 && headbar->title < POKEDEX_DETAIL_HEADBAR_TITLE_COUNT) {
        ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, headbar->heapId);
        // The palette of each title's bar
        u32 palettes[POKEDEX_DETAIL_HEADBAR_TITLE_COUNT] = { 0, 2, 1, 3, 3 };
        GFLBitmap *bitmap;
        StrBuf *strbuf;

        GFL_G2DIOLoadArcNCLR(arc, 4, PALTYPE_SUB_BG, palettes[headbar->title] * 32, 13 * 32, 32, headbar->heapId);
        GFL_ArcToolFree(arc);
        bitmap = GFL_G2DIOLoadBitmap(ARCID_POKEDEX_GRA, 14, FALSE, headbar->heapId);
        GFL_BitmapCopy(bitmap, headbar->bitmap);
        GFL_BitmapFree(bitmap);
        strbuf = GFL_MsgDataLoadStrbufNew(headbar->msgData, BtlServerFlowTitle_Text_Info + headbar->title);
        PrintQueue_PrintColor(headbar->printQueue, headbar->bitmap, 24, 5, strbuf, headbar->font,
                              PRINT_COLOR(15, 14, 0));
        GFL_StrBufFree(strbuf);
        headbar->transferPending = TRUE;
        PokedexDetailHeadbar_TransferTitle(headbar);
    }
}

static void PokedexDetailHeadbar_ClearTitle(PokedexDetailHeadbar *headbar) {
}

// Loads the bar's characters once the title has been printed on them
static void PokedexDetailHeadbar_TransferTitle(PokedexDetailHeadbar *headbar) {
    if (headbar->transferPending && !PrintQueue_IsBitmapPending(headbar->printQueue, headbar->bitmap)) {
        GFL_BGSysLoadChar(HEADBAR_BG, GFL_BitmapGetPixelData(headbar->bitmap), headbar->charSize, headbar->charPos);
        GFL_BGSysQueueScrLoad(HEADBAR_BG);
        headbar->transferPending = FALSE;
    }
}
