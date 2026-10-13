#include "types.h"
#include "app/mailbox_local.h"
#include "app/pokelist.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "field/mailsys.h"
#include "field/mailtool.h"
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
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "pml/item.h"
#include "pml/mail.h"
#include "save/bag.h"
#include "system/app_menu_common.h"
#include "system/app_taskmenu.h"
#include "system/bgwinfrm.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/btl_server_flow_2.h"

// The mailbox screen's set-up: the display, the BGs and their graphics, the frames, the text, the menus, the mails of
// the list and the palette changes. The ROM doesn't name this file; the name is a guess, after box2_main.c

typedef struct {
    u32 messageId;
    u32 type;
} MailBoxMenuItem;

const BGSysVRAMConfig MailBox_VRAMConfig = {
    GX_VRAM_BG_128_B,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_64_E,  GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_A, GX_VRAM_TEXPLTT_01_FG,   GX_OBJVRAMMODE_CHAR_1D_32K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

void MailBox_VBlank(TCB *tcb, void *data) {
    GFL_BGSysUpdate();
    func_0204b7c8();
}

void MailBox_SetVRAMBanks(void) {
    GFL_BGSysSetVRAMBanks(&MailBox_VRAMConfig);
}

void MailBox_InitBGs(MailBoxWork *work) {
    GFL_BGSysCreate(HEAPID_MAILBOX_VIEW);
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

        GFL_BGSysSetLCDConfig(&lcdConfig);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf800), GX_BG_CHARBASE(0x18000),
                       0x4000, GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(4, &bg, BGMODE_TEXT);
        GFL_BGSysClearScr(4);
        GFL_BGSysClearCharCore(4, BGSYS_TILE_SIZE_16, 0, HEAPID_MAILBOX_VIEW);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf000), GX_BG_CHARBASE(0x10000),
                       0x4000, GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(5, &bg, BGMODE_TEXT);
        GFL_BGSysClearScr(5);
        GFL_BGSysClearCharCore(5, BGSYS_TILE_SIZE_16, 0, HEAPID_MAILBOX_VIEW);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe800), GX_BG_CHARBASE(0x0),
                       0x4000, GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(6, &bg, BGMODE_TEXT);
        GFL_BGSysClearScr(6);
        GFL_BGSysClearCharCore(6, BGSYS_TILE_SIZE_16, 0, HEAPID_MAILBOX_VIEW);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe000), GX_BG_CHARBASE(0x8000),
                       0x4000, GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(7, &bg, BGMODE_TEXT);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf800), GX_BG_CHARBASE(0x18000),
                       0x4000, GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(0, &bg, BGMODE_TEXT);
        GFL_BGSysClearScr(0);
        GFL_BGSysClearCharCore(0, BGSYS_TILE_SIZE_16, 0, HEAPID_MAILBOX_VIEW);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf000), GX_BG_CHARBASE(0x10000),
                       0x4000, GX_BG_EXTPLTT_01, 1, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(1, &bg, BGMODE_TEXT);
        GFL_BGSysClearScr(1);
        GFL_BGSysClearCharCore(1, BGSYS_TILE_SIZE_16, 0, HEAPID_MAILBOX_VIEW);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe800), GX_BG_CHARBASE(0x0),
                       0x4000, GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(2, &bg, BGMODE_TEXT);
        GFL_BGSysClearScr(2);
    }
    {
        BGSetup bg = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe000), GX_BG_CHARBASE(0x0),
                       0x4000, GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, 0 };

        GFL_BGSysCreateBG(3, &bg, BGMODE_TEXT);
    }
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG0, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG2, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG3, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void MailBox_FreeBGs(MailBoxWork *work) {
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysFree();
}

void MailBox_LoadBGGraphics(MailBoxWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_MAILBOX, HEAPID_MAILBOX_VIEW);
    NNSG2dCharacterData *character;
    void *file;
    u8 *chars;

    GFL_BGSysLoadArcNCGRStatic(arc, 42, 7, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 41, 7, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    GFL_G2DIOLoadArcNCLRDefault(arc, 43, PALTYPE_SUB_BG, 0, 0, HEAPID_MAILBOX_VIEW);
    GFL_BGSysLoadArcNCGRStatic(arc, 38, 3, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    GFL_BGSysLoadArcNCGRStatic(arc, 38, 1, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 37, 3, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    GFL_G2DIOLoadArcNCLRDefault(arc, 39, PALTYPE_MAIN_BG, 0, 0, HEAPID_MAILBOX_VIEW);
    file = GFL_G2DIOReadBGNCGRArc(arc, 38, TRUE, &character, HEAPID_MAILBOX_VIEW);
    chars = character->rawData;
    sys_memcpy32_fast(chars + 0x100, work->view->tileA, 0x20);
    sys_memcpy32_fast(chars + 0x100, work->view->tileB, 0x20);
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);
}

void MailBox_LoadSystemPalettes(MailBoxWork *work) {
    LoadSysMsgBox(0, 0x3e2, 13, 0, HEAPID_MAILBOX_VIEW);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 7 * 0x40, 0x20, HEAPID_MAILBOX_VIEW);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 7 * 0x40 + 0x20, 0x20, HEAPID_MAILBOX_VIEW);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 7 * 0x40 + 0x20, 0x20, HEAPID_MAILBOX_VIEW);
}

void MailBox_CreateFrames(MailBoxWork *work) {
    u32 i;
    u16 *screen;

    work->view->frames = BGWinFrame_Create(2, 11, HEAPID_MAILBOX_VIEW);
    for (i = 0; i < MAILBOX_PAGE_SIZE; i++) {
        BGWinFrame_InitFrame(work->view->frames, i, 2, 14, 4);
    }
    BGWinFrame_InitFrame(work->view->frames, MAILBOX_FRAME_SELECTED, 1, 14, 4);
    BGWinFrame_LoadScreen(work->view->frames, 0, ARCID_MAILBOX, 40, TRUE);
    screen = BGWinFrame_GetScreen(work->view->frames, 0);
    for (i = 1; i <= MAILBOX_FRAME_SELECTED; i++) {
        sys_memcpy(screen, BGWinFrame_GetScreen(work->view->frames, i), 14 * 4 * sizeof(u16));
    }
    for (i = 0; i < MAILBOX_PAGE_SIZE; i++) {
        BGWinFrame_Put(work->view->frames, i, (i % 2) * 16 + 1, (i / 2) * 4);
    }
}

void MailBox_FreeFrames(MailBoxWork *work) {
    BGWinFrame_Delete(work->view->frames);
}

void MailBox_LoadMessages(MailBoxView *view) {
    view->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, MAILBOX_MSG_FILE, HEAPID_MAILBOX_VIEW);
    view->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_MAILBOX_VIEW);
    view->strbuf = GFL_StrBufCreate(0x400, HEAPID_MAILBOX_VIEW);
}

void MailBox_FreeMessages(MailBoxView *view) {
    GFL_StrBufFree(view->strbuf);
    GFL_WordSetSystemFree(view->wordSet);
    GFL_MsgDataFree(view->msgData);
}

void MailBox_CreateYesNoMenu(MailBoxWork *work) {
    AppTaskMenuInit init;

    init.heapId = HEAPID_MAILBOX_VIEW;
    init.itemCount = 2;
    init.items = work->view->yesNoItems;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = 21;
    init.y = 8;
    init.width = 10;
    init.height = 3;
    work->view->menu = AppTaskMenu_Create(&init, work->view->menuRes);
}

void MailBox_CreateMenuResources(MailBoxWork *work) {
    static const MailBoxMenuItem menuItems[4] = {
        { BtlServerFlow2_Text_Read, 0 },
        { BtlServerFlow2_Text_Erase, 0 },
        { BtlServerFlow2_Text_Give, 0 },
        { BtlServerFlow2_Text_Cancel_2, APP_TASKMENU_ITEM_RETURN },
    };
    static const MailBoxMenuItem yesNoItems[2] = {
        { BtlServerFlow2_Text_Yes, 0 },
        { BtlServerFlow2_Text_No, 0 },
    };
    u16 i;

    work->view->menuRes = AppTaskMenuRes_Create(0, 5, work->font, work->view->printQueue, HEAPID_MAILBOX_VIEW);
    for (i = 0; i < 4; i++) {
        work->view->menuItems[i].str = GFL_MsgDataLoadStrbufNew(work->view->msgData, menuItems[i].messageId);
        work->view->menuItems[i].color = PRINT_COLOR(14, 15, 3);
        work->view->menuItems[i].type = menuItems[i].type;
    }
    for (i = 0; i < 2; i++) {
        work->view->yesNoItems[i].str = GFL_MsgDataLoadStrbufNew(work->view->msgData, yesNoItems[i].messageId);
        work->view->yesNoItems[i].color = PRINT_COLOR(14, 15, 3);
        work->view->yesNoItems[i].type = yesNoItems[i].type;
    }
}

void MailBox_FreeMenuResources(MailBoxWork *work) {
    int i;

    for (i = 0; i < 4; i++) {
        GFL_StrBufFree(work->view->menuItems[i].str);
    }
    GFL_StrBufFree(work->view->yesNoItems[0].str);
    GFL_StrBufFree(work->view->yesNoItems[1].str);
    AppTaskMenuRes_Free(work->view->menuRes);
}

void MailBox_CreateMainMenu(MailBoxWork *work) {
    AppTaskMenuInit init;

    init.heapId = HEAPID_MAILBOX_VIEW;
    init.itemCount = 4;
    init.items = work->view->menuItems;
    init.posType = APP_TASKMENU_POS_TOP_LEFT;
    init.x = 20;
    init.y = 12;
    init.width = 12;
    init.height = 3;
    work->view->menu = AppTaskMenu_Create(&init, work->view->menuRes);
}

void MailBox_FreeMenu(MailBoxWork *work) {
    AppTaskMenu_Free(work->view->menu);
}

void MailBox_LoadMails(MailBoxWork *work) {
    u32 i;

    work->view->mailCount = 0;
    for (i = 0; i < MAIL_COUNT; i++) {
        work->view->pageMails[i] = 0xff;
        work->view->mails[i] = func_020097f4(work->mails, 0, i, HEAPID_MAILBOX_VIEW);
        if (func_020096a8(work->view->mails[i]) == TRUE) {
            work->view->pageMails[work->view->mailCount] = i;
            work->view->mailCount++;
        }
    }
    if (work->view->mailCount == 0) {
        work->view->maxPage = 0;
    } else {
        work->view->maxPage = (work->view->mailCount - 1) / MAILBOX_PAGE_SIZE;
    }
}

void MailBox_FreeMails(MailBoxWork *work) {
    u32 i;

    for (i = 0; i < MAIL_COUNT; i++) {
        GFL_HeapFree(work->view->mails[i]);
    }
}

void MailBox_UpdateMailList(MailBoxWork *work) {
    u16 first = MAILBOX_PAGE_SIZE * work->page;
    u8 *mails = &work->view->pageMails[first];
    u16 window = first + MAILBOX_WIN_FIRST_MAIL;
    u16 frame = 0;
    u16 i;

    for (i = 0; i < MAILBOX_PAGE_SIZE; i++) {
        if (mails[i] == 0xff) {
            BGWinFrame_Hide(work->view->frames, frame);
        } else {
            MailBox_DrawMailFrame(work, frame, window, mails[i]);
            BGWinFrame_Show(work->view->frames, frame);
        }
        window++;
        frame++;
    }
}

void MailBox_ShowSelectedFrame(MailBoxWork *work) {
    MailBox_DrawMailFrame(work, MAILBOX_FRAME_SELECTED, MAILBOX_WIN_SELECTED_MAIL, work->mailIndex);
    BGWinFrame_Put(work->view->frames, MAILBOX_FRAME_SELECTED, 4, 7);
}

BOOL MailBox_UpdatePaletteFx(MailBoxWork *work) {
    MailBoxPaletteFx *fx = &work->view->paletteFx;

    switch (fx->phase) {
    case 0:
        if (!fx->onScreen) {
            func_0204c378(work->view->actors[fx->target], fx->firstPalette, TRUE);
        } else {
            GFL_BGSysSetScrPaletteNo(fx->target, fx->x, fx->y, fx->width, fx->height, fx->firstPalette);
            GFL_BGSysQueueScrLoad(fx->target);
        }
        fx->phase++;
        break;
    case 1:
        fx->timer++;
        if (fx->timer == 4) {
            if (!fx->onScreen) {
                func_0204c378(work->view->actors[fx->target], fx->secondPalette, TRUE);
            } else {
                GFL_BGSysSetScrPaletteNo(fx->target, fx->x, fx->y, fx->width, fx->height, fx->secondPalette);
                GFL_BGSysQueueScrLoad(fx->target);
            }
            fx->timer = 0;
            fx->phase++;
        }
        break;
    case 2:
        fx->timer++;
        if (fx->timer == 2) {
            return FALSE;
        }
        break;
    }
    return TRUE;
}

void MailBox_LoadMailGraphics(MailBoxWork *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_MAILBOX, HEAPID_MAILBOX_VIEW);
    u8 design = func_02009750(work->view->mails[work->mailIndex]);

    GFL_BGSysLoadArcNCGRStatic(arc, design + 12, 5, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    GFL_G2DIOLoadArcNCLRDefault(arc, design, PALTYPE_SUB_BG, 0, 0x40, HEAPID_MAILBOX_VIEW);
    loadBGScrToVramByFileNoReserveNegAlign(arc, design + 24, 5, 0, 0, TRUE, HEAPID_MAILBOX_VIEW);
    GFL_ArcToolFree(arc);
}

void MailBox_CreatePMSDraw(MailBoxWork *work) {
    work->view->pmsPrintQueue = func_02021998(HEAPID_MAILBOX_VIEW);
    work->view->pmsClActUnit = func_0204bf1c(8, 2, HEAPID_MAILBOX_VIEW);
    work->view->pmsDraw = PMSDraw_Create(work->view->pmsClActUnit, CLACT_VRAM_SUB, work->view->pmsPrintQueue, work->font,
                                         4, 4, HEAPID_MAILBOX_VIEW);
    PMSDraw_SetBackColor(work->view->pmsDraw, 0);
}

void MailBox_FreePMSDraw(MailBoxWork *work) {
    PMSDraw_Delete(work->view->pmsDraw);
    func_02021a18(work->view->pmsPrintQueue);
    func_0204bf98(work->view->pmsClActUnit);
}
