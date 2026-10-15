// GTS Negotiations' messages: the top screen's message window, the lower screen's menus with their labels and touch
// buttons, the names of the list of people of the Pal Pad, and the partner's name, trades, place and greeting. The
// name is the ROM's string, from GFL_HeapAllocate's call. Function names are ours.

#include "types.h"
#include "app/gtsnego_local.h"
#include "constants/arc.h"
#include "field/unity_tower.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/button_man.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "save/player_info.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/country_region.h"
#include "system/gf_font.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/0045.h"

#define GTSNEGO_LABEL_COUNT 12

// The colors of the buttons' text
#define GTSNEGO_MENU_COLOR PRINT_COLOR(14, 15, 0)
#define GTSNEGO_BUTTON_COLOR PRINT_COLOR(14, 15, 3)

struct GtsNegoMessage {
    // The frame characters, loaded at the end of BG 6 and of BG 1
    u32 subFrameChars;
    u32 mainFrameChars;
    // The touch buttons of the lower screen's menu
    ButtonMan *buttons;
    MsgData *msgData;
    WordSet *wordSet;
    Font *font;
    StrBuf *strbuf;
    StrBuf *messageStrbuf;
    StrBuf *formatStrbuf;
    // The labels of the search's settings
    int levelLabel;
    int myKindLabel;
    int wantKindLabel;
    BmpWin *messageWin;
    BmpWin *unk34;
    BmpWin *labels[GTSNEGO_LABEL_COUNT];
    BmpWin *titleWin;
    BmpWin *friendWins[GTSNEGO_LIST_ROWS];
    // The partner's windows
    BmpWin *locationWin;
    BmpWin *phraseWin;
    BmpWin *tradesLabelWin;
    BmpWin *tradesWin;
    BmpWin *headingWin;
    BmpWin *nameWin;
    PrintStream *stream;
    TCBExManager *tcbManager;
    PrintQueue *printQueue;
    AppTaskMenuItem items[2];
    AppTaskMenuRes *menuRes;
    AppTaskMenuWin *cancelButton;
    HeapID heapId;
    PMSDraw *pmsDraw;
};

static void GtsNegoMessage_FreeButtons(GtsNegoMessage *msg);

// The messages of the level ranges and of the kinds of Pokémon
static u16 sGtsNegoMessageLevels[] = {
    Bank0045_Text_AnyKind,
    Bank0045_Text_Lv1Through49,
    Bank0045_Text_Lv50,
    Bank0045_Text_Lv51Through100,
};

static u16 sGtsNegoMessageKinds[] = {
    Bank0045_Text_AnyKind_2, Bank0045_Text_Cool, Bank0045_Text_Cute, Bank0045_Text_Scary, Bank0045_Text_Strange,
};

// The main menu's buttons
static const TouchRect sGtsNegoMessageMainRects[] = {
    { 56, 80, 64, 192 },
    { 104, 128, 64, 192 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The arrows of the search's settings, left then right of each
static const TouchRect sGtsNegoMessageSearchRects[] = {
    { 43, 69, 83, 109 },    { 43, 69, 227, 253 },   { 83, 109, 107, 133 },       { 83, 109, 227, 253 },
    { 123, 149, 107, 133 }, { 123, 149, 227, 253 }, { TOUCH_RECT_END, 0, 0, 0 },
};

static void GtsNegoMessage_FreeButtons(GtsNegoMessage *msg) {
    if (msg->buttons != NULL) {
        GFL_BMN_Delete(msg->buttons);
        msg->buttons = NULL;
    }
}

GtsNegoMessage *GtsNegoMessage_Create(HeapID heapId, u32 bank) {
    GtsNegoMessage *msg = GFL_HeapAllocate(heapId, sizeof(GtsNegoMessage), TRUE, "gtsnego_message.c", 169);

    msg->heapId = heapId;
    BmpWin_InitAllocator(msg->heapId);
    func_020232d0();
    msg->tcbManager = GFL_TCBExMgrCreate(msg->heapId, msg->heapId, 1, 0);
    msg->printQueue = func_02021998(msg->heapId);
    msg->messageStrbuf = GFL_StrBufCreate(240, msg->heapId);
    msg->strbuf = GFL_StrBufCreate(240, msg->heapId);
    msg->formatStrbuf = GFL_StrBufCreate(240, msg->heapId);
    msg->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, msg->heapId);
    msg->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, bank, msg->heapId);
    msg->wordSet = GFL_WordSetSystemCreate(6, 64, msg->heapId);
    msg->menuRes = AppTaskMenuRes_Create(5, 9, msg->font, msg->printQueue, msg->heapId);
    msg->mainFrameChars = LoadCursorImageEndOfHeap(1, 12, 0, msg->heapId);
    msg->subFrameChars = LoadCursorImageEndOfHeap(6, 12, 0, msg->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 0x160, 0x20, msg->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_MAIN_BG, 0x160, 0x20, msg->heapId);
    return msg;
}

void GtsNegoMessage_Update(GtsNegoMessage *msg) {
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    GFL_TCBExMgrUpdate(msg->tcbManager);
    func_02021a3c(msg->printQueue);
    if (msg->cancelButton != NULL) {
        AppTaskMenuWin_Update(msg->cancelButton);
    }
    if (msg->pmsDraw != NULL) {
        PMSDraw_Main(msg->pmsDraw);
    }
}

void GtsNegoMessage_Free(GtsNegoMessage *msg) {
    int i;

    GFL_BGSysFreeCharMemory(1, CHAR_POS(msg->mainFrameChars), CHAR_SIZE(msg->mainFrameChars));
    GFL_BGSysFreeCharMemory(6, CHAR_POS(msg->subFrameChars), CHAR_SIZE(msg->subFrameChars));
    GtsNegoMessage_FreeButtons(msg);
    func_020232d8();
    GFL_MsgDataFree(msg->msgData);
    GFL_FontFree(msg->font);
    GFL_StrBufFree(msg->strbuf);
    GFL_StrBufFree(msg->messageStrbuf);
    GFL_StrBufFree(msg->formatStrbuf);
    GtsNegoMessage_FreePMSDraw(msg);
    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
        msg->stream = NULL;
    }
    GFL_WordSetSystemFree(msg->wordSet);
    if (msg->cancelButton != NULL) {
        AppTaskMenuWin_Free(msg->cancelButton);
        msg->cancelButton = NULL;
    }
    AppTaskMenuRes_Free(msg->menuRes);
    GFL_TCBExMgrFree(msg->tcbManager);
    func_02021c44(msg->printQueue);
    func_02021a18(msg->printQueue);
    if (msg->messageWin != NULL) {
        BmpWin_Free(msg->messageWin);
    }
    if (msg->locationWin != NULL) {
        BmpWin_Free(msg->locationWin);
    }
    if (msg->nameWin != NULL) {
        BmpWin_Free(msg->nameWin);
    }
    if (msg->tradesWin != NULL) {
        BmpWin_Free(msg->tradesWin);
    }
    if (msg->tradesLabelWin != NULL) {
        BmpWin_Free(msg->tradesLabelWin);
    }
    if (msg->headingWin != NULL) {
        BmpWin_Free(msg->headingWin);
    }
    if (msg->phraseWin != NULL) {
        BmpWin_Free(msg->phraseWin);
    }
    if (msg->unk34 != NULL) {
        BmpWin_Free(msg->unk34);
    }
    if (msg->titleWin != NULL) {
        BmpWin_Free(msg->titleWin);
    }
    for (i = 0; i < GTSNEGO_LABEL_COUNT; i++) {
        if (msg->labels[i] != NULL) {
            BmpWin_Free(msg->labels[i]);
        }
    }
    for (i = 0; i < GTSNEGO_LIST_ROWS; i++) {
        if (msg->friendWins[i] != NULL) {
            BmpWin_Free(msg->friendWins[i]);
        }
    }
    BmpWin_FreeAllocator();
    GFL_HeapFree(msg);
}

void GtsNegoMessage_Print(GtsNegoMessage *msg, u32 msgNo) {
    BmpWin *window;

    GFL_MsgDataLoadStrbuf(msg->msgData, msgNo, msg->messageStrbuf);
    if (msg->messageWin == NULL) {
        msg->messageWin = BmpWin_CreateDynamic(1, 1, 19, 30, 4, 11, TRUE);
    }
    window = msg->messageWin;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, msg->messageStrbuf, msg->font);
    BmpWin_DrawFrame(window, WINFRAME_TRANSFER_VBLANK, msg->mainFrameChars, 12);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(1);
}

void GtsNegoMessage_PrintShort(GtsNegoMessage *msg, u32 msgNo) {
    BmpWin *window;

    GFL_MsgDataLoadStrbuf(msg->msgData, msgNo, msg->strbuf);
    if (msg->messageWin == NULL) {
        msg->messageWin = BmpWin_CreateDynamic(1, 1, 21, 30, 2, 11, TRUE);
    }
    window = msg->messageWin;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, msg->strbuf, msg->font);
    BmpWin_DrawFrame(window, WINFRAME_TRANSFER_VBLANK, msg->mainFrameChars, 12);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(1);
}

// Whether no message is being printed a character at a time, which A continues
BOOL GtsNegoMessage_IsDone(GtsNegoMessage *msg) {
    if (msg->stream != NULL) {
        switch (func_020223b4(msg->stream)) {
        case PRINT_STREAM_DONE:
            func_020223cc(msg->stream);
            msg->stream = NULL;
            break;
        case PRINT_STREAM_PAUSED:
            if (GCTX_HIDGetPressedKeys() == PAD_BUTTON_A) {
                func_020223bc(msg->stream);
            }
            break;
        }
        return FALSE;
    }
    return TRUE;
}

void GtsNegoMessage_ClearWindow(GtsNegoMessage *msg) {
    if (msg->messageWin != NULL) {
        BmpWin_ClearFrame(msg->messageWin, WINFRAME_TRANSFER_NONE);
        BmpWin_ClearScreen(msg->messageWin);
        GFL_BGSysQueueScrLoad(1);
        BmpWin_Free(msg->messageWin);
        msg->messageWin = NULL;
    }
}

AppTaskMenu *GtsNegoMessage_CreateYesNo(GtsNegoMessage *msg, u32 layout) {
    AppTaskMenuInit init;
    AppTaskMenu *menu;

    init.heapId = msg->heapId;
    init.itemCount = NELEMS(msg->items);
    init.items = msg->items;
    switch (layout) {
    case 0:
        init.x = 32;
        init.y = 14;
        init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
        break;
    case 1:
        init.x = 32;
        init.y = 24;
        init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
        break;
    }
    init.width = 13;
    init.height = 3;
    msg->items[0].str = GFL_StrBufCreate(100, msg->heapId);
    GFL_MsgDataLoadStrbuf(msg->msgData, Bank0045_Text_Yes, msg->items[0].str);
    msg->items[0].color = GTSNEGO_MENU_COLOR;
    msg->items[1].str = GFL_StrBufCreate(100, msg->heapId);
    GFL_MsgDataLoadStrbuf(msg->msgData, Bank0045_Text_No, msg->items[1].str);
    msg->items[1].color = GTSNEGO_MENU_COLOR;
    menu = AppTaskMenu_Create(&init, msg->menuRes);
    GFL_StrBufFree(msg->items[0].str);
    GFL_StrBufFree(msg->items[1].str);
    return menu;
}

void GtsNegoMessage_UpdateButtons(GtsNegoMessage *msg) {
    GFL_BMN_Main(msg->buttons);
}

void GtsNegoMessage_FreeLabels(GtsNegoMessage *msg) {
    int i;

    for (i = 0; i < GTSNEGO_LABEL_COUNT; i++) {
        if (msg->labels[i] != NULL) {
            BmpWin_Free(msg->labels[i]);
            msg->labels[i] = NULL;
        }
    }
    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
}

// Adds a label of a line on a BG, returning which it is
int GtsNegoMessage_AddLabel(GtsNegoMessage *msg, u32 msgNo, int x, int y, int width, int bg) {
    int i;

    for (i = 0; i < GTSNEGO_LABEL_COUNT; i++) {
        if (msg->labels[i] == NULL) {
            BmpWin *window;

            msg->labels[i] = BmpWin_CreateDynamic(bg, x, y, width, 2, 11, TRUE);
            window = msg->labels[i];
            GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
            GFL_MsgDataLoadStrbuf(msg->msgData, msgNo, msg->strbuf);
            GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 2, 2, msg->strbuf, msg->font);
            BmpWin_FlushChar(window);
            BmpWin_FlushMap(window);
            return i;
        }
    }
    return 0;
}

int GtsNegoMessage_AddMenuLabel(GtsNegoMessage *msg, u32 msgNo, int x, int y) {
    return GtsNegoMessage_AddLabel(msg, msgNo, x, y, 20, 6);
}

void GtsNegoMessage_SetLabel(GtsNegoMessage *msg, u32 msgNo, int label) {
    BmpWin *window = msg->labels[label];

    if (window != NULL) {
        GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
        GFL_MsgDataLoadStrbuf(msg->msgData, msgNo, msg->strbuf);
        GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 2, 2, msg->strbuf, msg->font);
        BmpWin_FlushChar(window);
    }
}

void GtsNegoMessage_QueueLabelsLoad(GtsNegoMessage *msg) {
    GFL_BGSysQueueScrLoad(6);
}

void GtsNegoMessage_ClearLabels(GtsNegoMessage *msg) {
    int i;

    for (i = 0; i < GTSNEGO_LABEL_COUNT; i++) {
        if (msg->labels[i] != NULL) {
            GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(msg->labels[i]));
            BmpWin_ClearFrame(msg->labels[i], WINFRAME_TRANSFER_NONE);
            BmpWin_ClearScreen(msg->labels[i]);
        }
    }
}

void GtsNegoMessage_ShowMainMenu(GtsNegoMessage *msg, ButtonManCallback callback, void *work) {
    GtsNegoMessage_FreeLabels(msg);
    GtsNegoMessage_AddLabel(msg, Bank0045_Text_TradeAnyone, 8, 7, 16, 6);
    GtsNegoMessage_AddLabel(msg, Bank0045_Text_TradeRendezvous, 8, 13, 16, 6);
    GtsNegoMessage_QueueLabelsLoad(msg);
    GtsNegoMessage_FreeButtons(msg);
    msg->buttons = GFL_BMN_Create(sGtsNegoMessageMainRects, callback, work, msg->heapId);
}

void GtsNegoMessage_ShowSearchMenu(GtsNegoMessage *msg, ButtonManCallback callback, void *work, int level, int myKind,
                                   int wantKind) {
    GtsNegoMessage_FreeLabels(msg);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    GtsNegoMessage_AddMenuLabel(msg, Bank0045_Text_TradeWanted, 0, 0);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    GtsNegoMessage_AddMenuLabel(msg, Bank0045_Text_Level, 3, 6);
    GtsNegoMessage_AddMenuLabel(msg, Bank0045_Text_Want, 1, 11);
    GtsNegoMessage_AddMenuLabel(msg, Bank0045_Text_Have, 1, 16);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
    msg->levelLabel = GtsNegoMessage_AddMenuLabel(msg, sGtsNegoMessageLevels[level], 13, 6);
    msg->myKindLabel = GtsNegoMessage_AddMenuLabel(msg, sGtsNegoMessageKinds[myKind], 16, 11);
    msg->wantKindLabel = GtsNegoMessage_AddMenuLabel(msg, sGtsNegoMessageKinds[wantKind], 16, 16);
    GtsNegoMessage_QueueLabelsLoad(msg);
    GtsNegoMessage_FreeButtons(msg);
    msg->buttons = GFL_BMN_Create(sGtsNegoMessageSearchRects, callback, work, msg->heapId);
}

void GtsNegoMessage_SetLevel(GtsNegoMessage *msg, int level) {
    GtsNegoMessage_SetLabel(msg, sGtsNegoMessageLevels[level], msg->levelLabel);
}

void GtsNegoMessage_SetMyKind(GtsNegoMessage *msg, int kind) {
    GtsNegoMessage_SetLabel(msg, sGtsNegoMessageKinds[kind], msg->myKindLabel);
}

void GtsNegoMessage_SetWantKind(GtsNegoMessage *msg, int kind) {
    GtsNegoMessage_SetLabel(msg, sGtsNegoMessageKinds[kind], msg->wantKindLabel);
}

void GtsNegoMessage_HideMenu(GtsNegoMessage *msg) {
    GtsNegoMessage_FreeButtons(msg);
}

// A button as wide as its text, centered
AppTaskMenuWin *GtsNegoMessage_CreateButton(GtsNegoMessage *msg, u32 msgNo) {
    AppTaskMenuWin *button;
    int width;

    msg->items[0].str = GFL_StrBufCreate(100, msg->heapId);
    GFL_MsgDataLoadStrbuf(msg->msgData, msgNo, msg->items[0].str);
    msg->items[0].color = GTSNEGO_BUTTON_COLOR;
    msg->items[0].type = 0;
    width = ((int)GFL_FontGetBlockWidth(msg->items[0].str, msg->font, 0) + 32) / 16;
    button =
        AppTaskMenuWin_CreateEx(msg->menuRes, &msg->items[0], 16 - width, 21, width * 2, 3, FALSE, TRUE, msg->heapId);
    GFL_StrBufFree(msg->items[0].str);
    return button;
}

void GtsNegoMessage_PrintHeading(GtsNegoMessage *msg, u32 msgNo) {
    BmpWin *window;

    GFL_MsgDataLoadStrbuf(msg->msgData, msgNo, msg->strbuf);
    if (msg->headingWin == NULL) {
        msg->headingWin = BmpWin_CreateDynamic(1, 1, 13, 14, 3, 4, TRUE);
    }
    window = msg->headingWin;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    GFL_TextRndUpdateColorIndexLUT(15, 14, 0);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 2, 3, msg->strbuf, msg->font);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
}

// Prints where the partner lives, or nowhere when the place is not a valid one
void GtsNegoMessage_PrintLocation(GtsNegoMessage *msg, PlayerInfo *partner) {
    BmpWin *window;
    u8 country = UnityTowerVisitor_GetCountry(partner);
    u8 province = UnityTowerVisitor_GetProvince(partner);
    u8 validCountry = Country_GetValidCountry(country, province, TrainerInfo_GetRegion(partner));
    u8 validProvince = Country_GetValidRegion(country, province, TrainerInfo_GetRegion(partner));

    if (country == validCountry && province == validProvince) {
        loadCountryToStrbuf(msg->wordSet, 0, UnityTowerVisitor_GetCountry(partner));
        loadCountryAreaToStrbuf(msg->wordSet, 1, UnityTowerVisitor_GetCountry(partner),
                                UnityTowerVisitor_GetProvince(partner));
    } else {
        loadCountryToStrbuf(msg->wordSet, 0, 0);
        loadCountryAreaToStrbuf(msg->wordSet, 1, 0, 0);
    }
    GFL_MsgDataLoadStrbuf(msg->msgData, Bank0045_Text_Empty_3, msg->formatStrbuf);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->strbuf, msg->formatStrbuf);
    if (msg->locationWin == NULL) {
        msg->locationWin = BmpWin_CreateDynamic(1, 1, 15, 30, 5, 11, TRUE);
    }
    window = msg->locationWin;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 5, 5, msg->strbuf, msg->font);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(1);
}

void GtsNegoMessage_ClearLocation(GtsNegoMessage *msg) {
    if (msg->locationWin != NULL) {
        GFL_BitmapFill(BmpWin_GetBitmap(msg->locationWin), 0);
        BmpWin_FlushChar(msg->locationWin);
        BmpWin_ClearScreen(msg->locationWin);
        GFL_BGSysQueueScrLoad(1);
        BmpWin_Free(msg->locationWin);
        msg->locationWin = NULL;
    }
}

// Prints a person's name, ID and country in a row of the list
int GtsNegoMessage_PrintFriend(GtsNegoMessage *msg, PlayerInfo *friend, int row) {
    BmpWin *window;

    if (msg->friendWins[row] == NULL) {
        msg->friendWins[row] = BmpWin_CreateDynamic(6, 6, row * 6 + 1, 24, 4, 11, TRUE);
    }
    window = msg->friendWins[row];
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    GFL_StrBufClear(msg->formatStrbuf);
    GFL_StrBufClear(msg->strbuf);
    copyVarForText(msg->wordSet, 0, friend);
    WordSetNumber(msg->wordSet, 1, getTrainerID(friend), 5, 2, TRUE);
    loadCountryToStrbuf(msg->wordSet, 2, UnityTowerVisitor_GetCountry(friend));
    GFL_MsgDataLoadStrbuf(msg->msgData, Bank0045_Text_Id, msg->formatStrbuf);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->strbuf, msg->formatStrbuf);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, msg->strbuf, msg->font);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    return 0;
}

// The list's title, and the first people in the rows on the screen
void GtsNegoMessage_ShowFriendList(GtsNegoMessage *msg, GameData *gameData) {
    int i = 0;

    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    GtsNegoMessage_AddLabel(msg, Bank0045_Text_SelectRendezvous, 0, 0, 20, 5);
    for (; i < GTSNEGO_LIST_SHOWN; i++) {
        PlayerInfo *friend = GtsNego_GetFriend(gameData, i);

        if (friend != NULL) {
            GtsNegoMessage_PrintFriend(msg, friend, i + 2);
        }
    }
    GFL_BGSysQueueScrLoad(5);
}

void GtsNegoMessage_PrintLastFriend(GtsNegoMessage *msg, GameData *gameData, int index) {
    GtsNegoMessage_PrintFriend(msg, GtsNego_GetFriend(gameData, index), GTSNEGO_LIST_ROWS - 1);
    GFL_BGSysQueueScrLoad(6);
}

// Moves the names a row up, the first row's going away
void GtsNegoMessage_ShiftFriendsUp(GtsNegoMessage *msg) {
    int i;

    if (msg->friendWins[0] != NULL) {
        BmpWin_Free(msg->friendWins[0]);
        msg->friendWins[0] = NULL;
    }
    for (i = 1; i < GTSNEGO_LIST_ROWS; i++) {
        if (msg->friendWins[i] != NULL) {
            BmpWin_SetPosY(msg->friendWins[i], BmpWin_GetPosY(msg->friendWins[i]) - 6);
            BmpWin_FlushMap(msg->friendWins[i]);
        }
    }
    GFL_BGSysQueueScrLoad(6);
    for (i = 0; i < GTSNEGO_LIST_ROWS - 1; i++) {
        msg->friendWins[i] = msg->friendWins[i + 1];
    }
    msg->friendWins[GTSNEGO_LIST_ROWS - 1] = NULL;
}

void GtsNegoMessage_PrintFriends(GtsNegoMessage *msg, GameData *gameData, int top) {
    int i;

    for (i = 0; i < GTSNEGO_LIST_ROWS; i++) {
        if (i + top >= 0) {
            PlayerInfo *friend = GtsNego_GetFriend(gameData, i + top);

            if (friend != NULL) {
                GtsNegoMessage_PrintFriend(msg, friend, i);
            }
        }
    }
    GFL_BGSysQueueScrLoad(6);
}

void GtsNegoMessage_PrintFirstFriend(GtsNegoMessage *msg, GameData *gameData, int index) {
    GtsNegoMessage_PrintFriend(msg, GtsNego_GetFriend(gameData, index), 0);
    GFL_BGSysQueueScrLoad(6);
}

// Moves the names a row down, the last row's going away
void GtsNegoMessage_ShiftFriendsDown(GtsNegoMessage *msg) {
    int i;

    if (msg->friendWins[GTSNEGO_LIST_ROWS - 1] != NULL) {
        BmpWin_Free(msg->friendWins[GTSNEGO_LIST_ROWS - 1]);
        msg->friendWins[GTSNEGO_LIST_ROWS - 1] = NULL;
    }
    for (i = 0; i < GTSNEGO_LIST_ROWS; i++) {
        if (msg->friendWins[i] != NULL) {
            BmpWin_SetPosY(msg->friendWins[i], BmpWin_GetPosY(msg->friendWins[i]) + 6);
            BmpWin_FlushMap(msg->friendWins[i]);
        }
    }
    GFL_BGSysQueueScrLoad(6);
    for (i = GTSNEGO_LIST_ROWS - 1; i > 0; i--) {
        msg->friendWins[i] = msg->friendWins[i - 1];
    }
    msg->friendWins[0] = NULL;
}

AppTaskMenu *GtsNegoMessage_CreateConfirm(GtsNegoMessage *msg, u32 layout) {
    AppTaskMenuInit init;
    AppTaskMenu *menu;

    init.heapId = msg->heapId;
    init.itemCount = NELEMS(msg->items);
    init.items = msg->items;
    switch (layout) {
    case 0:
        init.x = 32;
        init.y = 14;
        init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
        break;
    case 1:
        init.x = 32;
        init.y = 24;
        init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
        break;
    }
    init.width = 13;
    init.height = 3;
    msg->items[0].str = GFL_StrBufCreate(100, msg->heapId);
    GFL_MsgDataLoadStrbuf(msg->msgData, Bank0045_Text_Yes_2, msg->items[0].str);
    msg->items[0].color = GTSNEGO_MENU_COLOR;
    msg->items[0].type = 0;
    msg->items[1].str = GFL_StrBufCreate(100, msg->heapId);
    GFL_MsgDataLoadStrbuf(msg->msgData, Bank0045_Text_Cancel_2, msg->items[1].str);
    msg->items[1].color = GTSNEGO_MENU_COLOR;
    msg->items[1].type = 0;
    menu = AppTaskMenu_Create(&init, msg->menuRes);
    GFL_StrBufFree(msg->items[0].str);
    GFL_StrBufFree(msg->items[1].str);
    return menu;
}

void GtsNegoMessage_FreeMenu(AppTaskMenu *menu) {
    if (menu != NULL) {
        G2_BlendNone();
        AppTaskMenu_Free(menu);
    }
}

void GtsNegoMessage_CreatePMSDraw(GtsNegoMessage *msg, GtsNegoDisp *disp) {
    if (msg->pmsDraw == NULL) {
        msg->pmsDraw = PMSDraw_Create(GtsNegoDisp_GetClActUnit(disp), CLACT_VRAM_MAIN, msg->printQueue, msg->font, 8, 4,
                                      msg->heapId);
    }
}

void GtsNegoMessage_FreePMSDraw(GtsNegoMessage *msg) {
    if (msg->pmsDraw != NULL) {
        PMSDraw_Delete(msg->pmsDraw);
        msg->pmsDraw = NULL;
    }
}

void GtsNegoMessage_PrintPhrase(GtsNegoMessage *msg, PMSData *phrase) {
    PMSDrawPos pos = { 5, 5 };
    BmpWin *window;

    if (msg->phraseWin == NULL) {
        msg->phraseWin = BmpWin_CreateDynamic(1, 1, 15, 30, 5, 11, TRUE);
    }
    window = msg->phraseWin;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    PMSDraw_SetBackColor(msg->pmsDraw, 0);
    PMSDraw_SetColor(msg->pmsDraw, PRINT_COLOR(15, 2, 0));
    PMSDraw_PrintEx(msg->pmsDraw, window, phrase, 0, &pos);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(1);
}

void GtsNegoMessage_PrintTitle(GtsNegoMessage *msg, u32 msgNo) {
    BmpWin *window;

    GFL_MsgDataLoadStrbuf(msg->msgData, msgNo, msg->strbuf);
    if (msg->titleWin == NULL) {
        msg->titleWin = BmpWin_CreateDynamic(1, 0, 0, 14, 2, 11, TRUE);
    }
    window = msg->titleWin;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 2, 2, msg->strbuf, msg->font);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(1);
}

// The search's cancel button
static const TouchRect sGtsNegoMessageCancelRect[] = {
    { 160, 192, 80, 176 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

void GtsNegoMessage_CreateCancelButton(GtsNegoMessage *msg, ButtonManCallback callback, void *work) {
    msg->items[0].str = GFL_StrBufCreate(100, msg->heapId);
    GFL_MsgDataLoadStrbuf(msg->msgData, Bank0045_Text_Cancel, msg->items[0].str);
    msg->items[0].color = GTSNEGO_BUTTON_COLOR;
    msg->items[0].type = 0;
    msg->cancelButton = AppTaskMenuWin_CreateEx(msg->menuRes, &msg->items[0], 10, 21, 12, 3, FALSE, TRUE, msg->heapId);
    if (!func_0203d554()) {
        AppTaskMenuWin_SetActive(msg->cancelButton, TRUE);
    }
    GFL_StrBufFree(msg->items[0].str);
    GtsNegoMessage_FreeButtons(msg);
    msg->buttons = GFL_BMN_Create(sGtsNegoMessageCancelRect, callback, work, msg->heapId);
}

void GtsNegoMessage_PressCancelButton(GtsNegoMessage *msg) {
    AppTaskMenuWin_SetFlashing(msg->cancelButton, TRUE);
}

void GtsNegoMessage_FreeCancelButton(GtsNegoMessage *msg) {
    AppTaskMenuWin_Free(msg->cancelButton);
    GFL_BGSysQueueScrLoad(5);
    msg->cancelButton = NULL;
}

BOOL GtsNegoMessage_IsCancelPressDone(GtsNegoMessage *msg) {
    if (AppTaskMenuWin_IsFlashFinished(msg->cancelButton)) {
        GtsNegoMessage_FreeCancelButton(msg);
        return TRUE;
    }
    return FALSE;
}

// Prints how many trades the partner has made, under its label. msgNo is not used
void GtsNegoMessage_PrintTradeCount(GtsNegoMessage *msg, u32 msgNo, int count) {
    BmpWin *window;

    GFL_MsgDataLoadStrbuf(msg->msgData, Bank0045_Text_NoTrades, msg->strbuf);
    if (msg->tradesLabelWin == NULL) {
        msg->tradesLabelWin = BmpWin_CreateDynamic(1, 21, 4, 10, 2, 4, TRUE);
    }
    window = msg->tradesLabelWin;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    GFL_TextRndUpdateColorIndexLUT(15, 14, 0);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 3, msg->strbuf, msg->font);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    WordSetNumber(msg->wordSet, 0, count, 5, 1, TRUE);
    GFL_MsgDataLoadStrbuf(msg->msgData, Bank0045_Text_Empty_4, msg->formatStrbuf);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->strbuf, msg->formatStrbuf);
    if (msg->tradesWin == NULL) {
        msg->tradesWin = BmpWin_CreateDynamic(1, 21, 6, 10, 3, 11, TRUE);
    }
    window = msg->tradesWin;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    GFL_TextRndUpdateColorIndexLUT(15, 2, 0);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 5, msg->strbuf, msg->font);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
}

void GtsNegoMessage_PrintPartnerName(GtsNegoMessage *msg, PlayerInfo *partner) {
    BmpWin *window;

    copyVarForText(msg->wordSet, 0, partner);
    GFL_MsgDataLoadStrbuf(msg->msgData, Bank0045_Text_Empty_5, msg->formatStrbuf);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->strbuf, msg->formatStrbuf);
    if (msg->nameWin == NULL) {
        msg->nameWin = BmpWin_CreateDynamic(1, 11, 10, 10, 3, 11, TRUE);
    }
    window = msg->nameWin;
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(window), 0, 0, msg->strbuf, msg->font);
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
}

void GtsNegoMessage_ShowPartner(GtsNegoMessage *msg, PlayerInfo *partner, int count) {
    GtsNegoMessage_PrintPartnerName(msg, partner);
    GtsNegoMessage_PrintTradeCount(msg, Bank0045_Text_Empty_4, count);
    GtsNegoMessage_PrintHeading(msg, Bank0045_Text_Location);
    GtsNegoMessage_PrintLocation(msg, partner);
    GtsNegoMessage_PrintShort(msg, Bank0045_Text_TradePartnerHasBeen);
}

// Clears the windows but the message's, back for the main menu
void GtsNegoMessage_Reset(GtsNegoMessage *msg) {
    int i;

    if (msg->titleWin != NULL) {
        BmpWin_ClearFrame(msg->titleWin, WINFRAME_TRANSFER_NONE);
        BmpWin_ClearScreen(msg->titleWin);
        GFL_BGSysQueueScrLoad(1);
        BmpWin_Free(msg->titleWin);
        msg->titleWin = NULL;
    }
    if (msg->cancelButton != NULL) {
        AppTaskMenuWin_Free(msg->cancelButton);
        msg->cancelButton = NULL;
    }
    for (i = 0; i < GTSNEGO_LIST_ROWS; i++) {
        if (msg->friendWins[i] != NULL) {
            BmpWin_ClearScreen(msg->friendWins[i]);
            BmpWin_Free(msg->friendWins[i]);
            msg->friendWins[i] = NULL;
        }
    }
    GtsNegoMessage_ClearLabels(msg);
    GtsNegoMessage_FreeLabels(msg);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, PALTYPE_SUB_BG, 0x160, 0x20, msg->heapId);
    GFL_BGSysQueueScrLoad(5);
}
