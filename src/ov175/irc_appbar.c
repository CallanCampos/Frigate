#include "types.h"
#include "app/irc_appbar.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/mi.h"
#include "system/app_taskmenu.h"
#include "text/system/btl_main_4.h"

// irc_appbar.c, a ROM name: the compatibility checker's CANCEL and BACK button. The names are descriptive guesses

#define ACTOR_COUNT 7

// The steps of the button
enum {
    STATE_WAIT_TOUCH,
    STATE_FLASH,
    STATE_PRESSED,
};

struct IrcAppBar {
    AppTaskMenuWin *win;
    AppTaskMenuRes *res;
    ClActor *actors[ACTOR_COUNT];
    u8 unk24[0xc];
    u32 palRes;
    u32 charRes;
    u32 cellRes;
    u32 bg;
    u32 unk40;
    int result;
    int unk48;
    u8 unk4C[4];
    int state;
    int label;
    u16 heapId;
    int active;
};

static void IrcAppBar_LoadObjRes(IrcAppBar *bar, u32 vramType, u32 unused2, u32 unused3, u32 unused4, u8 objPalette,
                                 HeapID heapId);
static void IrcAppBar_FreeObjRes(IrcAppBar *bar);
static void IrcAppBar_CreateActor(IrcAppBar *bar, ClActUnit *unit, u32 unused2, u32 surface, u8 priority,
                                  HeapID heapId);
static void IrcAppBar_FreeActors(IrcAppBar *bar);

IrcAppBar *IrcAppBar_Create(u32 unused0, ClActUnit *unit, u8 bg, u8 palette, u8 objPalette, u32 unused5, Font *font,
                            PrintQueue *queue, HeapID heapId) {
    IrcAppBar *bar = GFL_HeapAllocate(heapId, sizeof(IrcAppBar), FALSE, "irc_appbar.c", 125);
    MsgData *msgData;
    AppTaskMenuItem item;
    BOOL isSub = FALSE;
    u32 vramType;
    u32 surface;

    sys_memset(bar, 0, sizeof(IrcAppBar));
    bar->bg = bg;
    bar->result = -1;
    bar->unk48 = -1;
    bar->label = 0;
    bar->heapId = heapId;
    bar->active = TRUE;
    if (bg >= 4) {
        isSub = TRUE;
        vramType = 1;
        surface = 4;
    } else {
        vramType = 0;
        surface = 0;
    }
    IrcAppBar_LoadObjRes(bar, vramType, surface, unused5, palette, objPalette, heapId);
    bar->res = AppTaskMenuRes_Create(bg, palette, font, queue, heapId);
    IrcAppBar_CreateActor(bar, unit, unused0, isSub, GFL_BGSysGetBGPriority(bg), heapId);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_4, heapId);
    sys_memset(&item, 0, sizeof(AppTaskMenuItem));
    item.color = 0x39e3;
    item.str = GFL_MsgDataLoadStrbufNew(msgData, BtlMain4_Text_Cancel);
    item.type = APP_TASKMENU_ITEM_RETURN;
    bar->win = AppTaskMenuWin_Create(bar->res, &item, 19, 21, 13, heapId);
    GFL_StrBufFree(item.str);
    GFL_MsgDataFree(msgData);
    return bar;
}

void IrcAppBar_Free(IrcAppBar *bar) {
    AppTaskMenuWin_Free(bar->win);
    IrcAppBar_FreeActors(bar);
    AppTaskMenuRes_Free(bar->res);
    IrcAppBar_FreeObjRes(bar);
    GFL_HeapFree(bar);
}

void IrcAppBar_Update(IrcAppBar *bar) {
    if (bar->active) {
        switch (bar->state) {
        case STATE_WAIT_TOUCH:
            bar->result = -1;
            if (AppTaskMenuWin_IsTouched(bar->win)) {
                GFL_SndSEPlay(SEQ_SE_CANCEL1);
                AppTaskMenuWin_SetFlashing(bar->win, TRUE);
                bar->state = STATE_FLASH;
            }
            break;
        case STATE_FLASH:
            if (AppTaskMenuWin_IsFlashFinished(bar->win)) {
                AppTaskMenuWin_ResetFlash(bar->win);
                AppTaskMenuWin_SetActive(bar->win, FALSE);
                bar->state = STATE_PRESSED;
            }
            break;
        case STATE_PRESSED:
            bar->result = 1;
            break;
        }
        AppTaskMenuWin_Update(bar->win);
    }
}

int IrcAppBar_GetResult(IrcAppBar *bar) {
    return bar->result;
}

BOOL IrcAppBar_IsPressed(IrcAppBar *bar) {
    if (bar->state > 0) {
        return TRUE;
    }
    return FALSE;
}

void IrcAppBar_Reset(IrcAppBar *bar) {
    AppTaskMenuWin_ResetFlash(bar->win);
    AppTaskMenuWin_SetActive(bar->win, FALSE);
    bar->state = STATE_WAIT_TOUCH;
}

BOOL IrcAppBar_IsPressedOrDone(IrcAppBar *bar) {
    if (bar->state != 0) {
        return TRUE;
    }
    return FALSE;
}

void IrcAppBar_SetVisible(IrcAppBar *bar, BOOL visible) {
    bar->active = visible;
    GFL_BGSysSetBGEnabled(bar->bg, visible);
}

void IrcAppBar_SetActive(IrcAppBar *bar, BOOL active) {
    bar->active = active;
    bar->state = STATE_WAIT_TOUCH;
}

void IrcAppBar_SetLabel(IrcAppBar *bar, int label) {
    if (label != bar->label) {
        MsgData *msgData;
        AppTaskMenuItem item;

        AppTaskMenuWin_Free(bar->win);
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_4, bar->heapId);
        sys_memset(&item, 0, sizeof(AppTaskMenuItem));
        item.color = 0x39e3;
        item.str = GFL_MsgDataLoadStrbufNew(msgData, BtlMain4_Text_Cancel + label);
        item.type = APP_TASKMENU_ITEM_RETURN;
        bar->win = AppTaskMenuWin_Create(bar->res, &item, 19, 21, 13, bar->heapId);
        GFL_StrBufFree(item.str);
        GFL_MsgDataFree(msgData);
        bar->label = label;
        bar->state = STATE_WAIT_TOUCH;
        bar->result = -1;
    }
}

static void IrcAppBar_LoadObjRes(IrcAppBar *bar, u32 vramType, u32 unused2, u32 unused3, u32 unused4, u8 objPalette,
                                 HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(93, heapId);

    bar->palRes = func_0204bbb8(arc, 41, vramType, (u16)(objPalette << 5), 0, 1, heapId);
    bar->charRes = func_0204b81c(arc, 40, FALSE, vramType, heapId);
    bar->cellRes = func_0204bde0(arc, 39, 38, heapId);
    GFL_ArcToolFree(arc);
}

static void IrcAppBar_FreeObjRes(IrcAppBar *bar) {
    func_0204be64(bar->cellRes);
    func_0204b98c(bar->charRes);
    func_0204bcd0(bar->palRes);
}

static void IrcAppBar_CreateActor(IrcAppBar *bar, ClActUnit *unit, u32 unused2, u32 surface, u8 priority,
                                  HeapID heapId) {
    ClActorSetup setup;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 128;
    setup.y = 96;
    setup.sequence = 0;
    setup.bgPriority = priority + 1;
    bar->actors[0] = func_0204c040(unit, bar->charRes, bar->palRes, bar->cellRes, &setup, surface, heapId);
}

static void IrcAppBar_FreeActors(IrcAppBar *bar) {
    int i;

    for (i = 0; i < ACTOR_COUNT; i++) {
        if (bar->actors[i] != NULL) {
            func_0204c108(bar->actors[i]);
        }
    }
}
