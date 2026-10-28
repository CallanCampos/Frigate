#include "types.h"
#include "app/name_entry.h"
#include "app/unova_link.h"
#include "app/wifi_login.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/gx.h"
#include "save/event_work.h"
#include "save/key_info.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/save_outside.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/dsi.h"
#include "system/game_data.h"
#include "system/wordset.h"

// Memory Link: receiving a Black or White save over DS Wireless Communications, where the other system runs the
// download play program, or by its Game Sync ID over Nintendo Wi-Fi Connection, and the list of the memories it
// unlocks. Named after the ROM's assertion string

// The Memory Link menu
enum {
    LINK_MENU_WIRELESS,
    LINK_MENU_WIFI,
    LINK_MENU_MEMORIES,
    LINK_MENU_BACK,
};

#define MEMORY_COUNT 8
// What the list of memories returns for its last item
#define MEMORY_LIST_BACK 0xff
// The characters of a Game Sync ID, which each give 5 bits
#define GAME_SYNC_ID_LENGTH 10
// A Black or White save as Game Sync downloads it: a header, and then the save
#define WB_SAVE_HEADER_SIZE 0x80
#define WB_SAVE_SIZE (WB_SAVE_HEADER_SIZE + 0x80000)

static void DataConvert_SeqWireless(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqWifi(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqSave(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqSaveOther(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqSaveGame(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqSaveBoth(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqMemoryList(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqMemoryInfo(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqStartMenuScene(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqStartLinkScene(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqStartWifiScene(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqStartMemoryListScene(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqStartMsgScene(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqWifiLogin(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqWifiLogout(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_SeqEnterId(KeySystemSeq *seq, int *state, void *work);
static void DataConvert_MenuSceneInit(void *work, HeapID heapId);
static BOOL DataConvert_MenuSceneMain(void *work);
static void DataConvert_MenuSceneExit(void *work);
static void DataConvert_LinkSceneInit(void *work, HeapID heapId);
static BOOL DataConvert_LinkSceneMain(void *work);
static void DataConvert_LinkSceneExit(void *work);
static void DataConvert_WifiSceneInit(void *work, HeapID heapId);
static BOOL DataConvert_WifiSceneMain(void *work);
static void DataConvert_WifiSceneExit(void *work);
static void DataConvert_MemoryListSceneInit(void *work, HeapID heapId);
static BOOL DataConvert_MemoryListSceneMain(void *work);
static void DataConvert_MemoryListSceneExit(void *work);
static void DataConvert_MsgSceneInit(void *work, HeapID heapId);
static BOOL DataConvert_MsgSceneMain(void *work);
static void DataConvert_MsgSceneExit(void *work);
static void DataConvert_OnNetError(void *work);
static BOOL DataConvert_NeedsGameSave(KeySystemWork *wk);
static BOOL DataConvert_IsMemoryOpen(KeySystemWork *wk, u32 memory);
static BOOL DataConvert_HasMemories(KeySystemWork *wk);
static BOOL DataConvert_IsMemorySeen(KeySystemWork *wk, u32 memory);

static const KeySystemSceneFuncs sDataConvertWifiSceneFuncs = {
    DataConvert_WifiSceneInit,
    DataConvert_WifiSceneMain,
    DataConvert_WifiSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sDataConvertMenuSceneFuncs = {
    DataConvert_MenuSceneInit,
    DataConvert_MenuSceneMain,
    DataConvert_MenuSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sDataConvertLinkSceneFuncs = {
    DataConvert_LinkSceneInit,
    DataConvert_LinkSceneMain,
    DataConvert_LinkSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sDataConvertMsgSceneFuncs = {
    DataConvert_MsgSceneInit,
    DataConvert_MsgSceneMain,
    DataConvert_MsgSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sDataConvertMemoryListSceneFuncs = {
    DataConvert_MemoryListSceneInit,
    DataConvert_MemoryListSceneMain,
    DataConvert_MemoryListSceneExit,
    NULL,
};

// A Game Sync ID's characters: the letters without I and O, and the digits from 2
static const u16 sIdChars[32] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'K', 'L', 'M', 'N', 'P', 'Q', 'R',
    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '2', '3', '4', '5', '6', '7', '8', '9',
};

void DataConvert_Init(KeySystemWork *wk, HeapID heapId) {
}

void DataConvert_SeqMenu(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    u32 item;

    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, DataConvert_SeqStartMenuScene);
        (*state)++;
        break;
    case 1:
        KeySystemList_Update(wk->list);
        if (KeySystemList_IsChanged(wk->list) && KeySystemMsgWin_IsDone(wk->infoWin)) {
            item = KeySystemList_GetCursor(wk->list);
            // Without the memory list, the third item is BACK
            if (!DataConvert_HasMemories(wk) && item == LINK_MENU_MEMORIES) {
                item = LINK_MENU_BACK;
            }
            KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, item + 80, KEY_SYSTEM_MSG_PRINT);
        }
        if (KeySystemList_IsDecided(wk->list)) {
            wk->choice = KeySystemList_GetCursor(wk->list);
            if (DataConvert_HasMemories(wk)) {
                if (wk->choice < LINK_MENU_BACK) {
                    KeySystemTags_Set(wk->tags, "DNM", KeySystemList_GetCursor(wk->list));
                }
            } else if (wk->choice < LINK_MENU_MEMORIES) {
                KeySystemTags_Set(wk->tags, "DNM", KeySystemList_GetCursor(wk->list));
            }
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 3:
        if (DataConvert_HasMemories(wk)) {
            switch (wk->choice) {
            case LINK_MENU_WIRELESS:
                KeySystemSeq_Push(seq, DataConvert_SeqWireless);
                *state = 0;
                break;
            case LINK_MENU_WIFI:
                KeySystemSeq_Push(seq, DataConvert_SeqWifi);
                *state = 0;
                break;
            case LINK_MENU_MEMORIES:
                KeySystemSeq_Push(seq, DataConvert_SeqMemoryList);
                *state = 0;
                break;
            case LINK_MENU_BACK:
                (*state)++;
                break;
            }
        } else {
            switch (wk->choice) {
            case LINK_MENU_WIRELESS:
                KeySystemSeq_Push(seq, DataConvert_SeqWireless);
                *state = 0;
                break;
            case LINK_MENU_WIFI:
                KeySystemSeq_Push(seq, DataConvert_SeqWifi);
                *state = 0;
                break;
            case LINK_MENU_MEMORIES:
                (*state)++;
                break;
            }
        }
        break;
    case 4:
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_0_TO_2, 30);
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqWireless(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    s32 result;
    BOOL hasSave;
    StrBuf *str;

    switch (*state) {
    case 0:
        if (isWirelessEnabled()) {
            *state = 1;
        } else {
            KeySystemSeq_Push(seq, KeySystem_SeqWirelessOff);
            *state = 23;
        }
        break;
    case 1:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        GFL_BGSysQueueScrLoad(0);
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 128, KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case 2:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            KeySystem_CreateYesNoMenu(wk, HEAPID_KEY_SYSTEM);
            (*state)++;
        }
        break;
    case 3:
        if (KeySystemMenu_UpdatePrint(wk->menu)) {
            GFL_BGSysQueueScrLoad(0);
            (*state)++;
        }
        break;
    case 4:
        result = KeySystemMenu_Update(wk->menu);
        if (result == BMPMENULIST_NULL) {
            break;
        }
        KeySystem_FreeMenu(wk);
        KeySystem_FreeMsgWin(wk);
        switch (result) {
        case 0:
            (*state)++;
            break;
        case 1:
        default:
            *state = 23;
            break;
        }
        break;
    case 5:
        KeySystemNet_SetMode(wk->net, KEY_SYSTEM_NET_MODE_OV181);
        wk->wbData = NULL;
        hasSave = SaveControl_IsDataAlreadyPresent(GameData_GetSaveControl(wk->param->gameData));
        if (wk->msgStrs[0] != NULL) {
            GFL_StrBufFree(wk->msgStrs[0]);
            wk->msgStrs[0] = NULL;
        }
        if (wk->msgStrs[1] != NULL) {
            GFL_StrBufFree(wk->msgStrs[1]);
            wk->msgStrs[1] = NULL;
        }
        wk->msgStrs[0] = GFL_StrBufCreate(128, HEAPID_KEY_SYSTEM);
#ifdef BLACK2
        wk->msgStrs[1] = GFL_MsgDataLoadStrbufNew(wk->msgData, 122);
#else
        wk->msgStrs[1] = GFL_MsgDataLoadStrbufNew(wk->msgData, 123);
#endif
        if (hasSave) {
            WordSetNumber(wk->wordSet, 0, getTrainerID(GetGameDataPlayerInfo(wk->param->gameData)), 5, 2, TRUE);
            str = GFL_MsgDataLoadStrbufNew(wk->msgData, 120);
        } else {
            str = GFL_MsgDataLoadStrbufNew(wk->msgData, 121);
        }
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->msgStrs[0], str);
        GFL_StrBufFree(str);
        KeySystemSeq_Push(seq, DataConvert_SeqStartLinkScene);
        (*state)++;
        break;
    case 6: {
        KeySystemNetOv181Start start = { NULL, NULL, 0 };

        start.text = wk->msgStrs[0];
        start.title = wk->msgStrs[1];
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_OV181_START, &start);
        (*state)++;
        break;
    }
    case 7:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            if (KeySystemNet_GetRequest(wk->net)->ov181.result == 1) {
                *state = 9;
            } else {
                *state = 8;
            }
        }
        break;
    case 8:
        *state = 22;
        break;
    case 9:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 126, KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        {
            KeySystemNetOv181End end = { FALSE, 0, NULL };

            KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_OV181_END, &end);
        }
        (*state)++;
        break;
    case 10:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            (*state)++;
        }
        break;
    case 11: {
        KeySystemNetRequest *request = KeySystemNet_GetRequest(wk->net);

        switch (request->ov181End.result) {
        case 0:
            wk->wbData = request->ov181End.data;
            *state = 12;
            break;
        case 1:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 127, KEY_SYSTEM_MSG_STREAM);
            *state = 21;
            break;
        case 3:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 130, KEY_SYSTEM_MSG_STREAM);
            *state = 21;
            break;
        case 2:
        case 4:
        default:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 129, KEY_SYSTEM_MSG_STREAM);
            *state = 21;
            break;
        }
        break;
    }
    case 12:
        if (MemoryLinkSave_CanMerge(wk->memoryLink, wk->wbData, GameData_GetSaveControl(wk->param->gameData))) {
            *state = 16;
        } else {
            *state = 13;
        }
        break;
    case 13:
        KeySystemMsgWin_Free(wk->msgWin);
        wk->msgWin = KeySystemMsgWin_Create(0, 2, 4, 28, 12, 14, wk->font, HEAPID_KEY_SYSTEM);
        KeySystemMsgWin_SetColor(wk->msgWin, PRINT_COLOR(1, 2, 15));
        KeySystemMsgWin_DrawFrame(wk->msgWin, 1, 15);
        GFL_BGSysQueueScrLoad(0);
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 132, KEY_SYSTEM_MSG_STREAM);
        *state = 14;
        break;
    case 14:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            KeySystemMenuSetup setup;

            MI_CpuFill8(&setup, 0, sizeof(KeySystemMenuSetup));
            setup.msgData = wk->msgData;
            setup.font = wk->font;
            setup.printQueue = wk->printQueue;
            setup.bg = 0;
            setup.palette = 14;
            setup.framePalette = 15;
            setup.frameChar = 1;
            setup.msgIds[0] = 65;
            setup.msgIds[1] = 66;
            setup.count = 2;
            setup.cancelable = TRUE;
            setup.cancelValue = 1;
            setup.cursor = 0;
            wk->menu = KeySystemMenu_CreateAt(&setup, 21, 19, 10, 4, HEAPID_KEY_SYSTEM);
            GFL_BGSysQueueScrLoad(0);
            (*state)++;
        }
        break;
    case 15:
        result = KeySystemMenu_Update(wk->menu);
        if (result == BMPMENULIST_NULL) {
            break;
        }
        KeySystemMenu_Free(wk->menu);
        wk->menu = NULL;
        switch (result) {
        case 0:
            (*state)++;
            break;
        case 1:
        default:
            *state = 19;
            break;
        }
        break;
    case 16:
        wk->transferResult =
            MemoryLinkSave_Merge(wk->memoryLink, wk->wbData, GameData_GetSaveControl(wk->param->gameData));
        (*state)++;
        break;
    case 17:
        KeySystemSeq_Push(seq, DataConvert_SeqSave);
        (*state)++;
        break;
    case 18:
        if (wk->transferResult) {
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 138, KEY_SYSTEM_MSG_STREAM_FAST);
        } else {
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 131, KEY_SYSTEM_MSG_STREAM_FAST);
        }
        *state = 20;
        break;
    case 19:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 133, KEY_SYSTEM_MSG_STREAM_FAST);
        *state = 20;
        break;
    case 20:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = 22;
        }
        break;
    case 21:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = 22;
        }
        break;
    case 22:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        KeySystemNet_SetMode(wk->net, KEY_SYSTEM_NET_MODE_NONE);
        (*state)++;
        break;
    case 23:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqWifi(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    s32 result;
    u64 value;
    const u16 *chars;

    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeOut);
        (*state)++;
        break;
    case 1:
        (*state)++;
        break;
    case 2:
        KeySystemSeq_Push(seq, DataConvert_SeqWifiLogin);
        (*state)++;
        break;
    case 3:
        if (wk->loggedIn) {
            wk->transferResult = FALSE;
            KeySystemNet_SetBuffer(wk->net, wk->loginBuffer);
            KeySystemNet_SetMode(wk->net, KEY_SYSTEM_NET_MODE_WIFI);
            KeySystemSeq_Push(seq, DataConvert_SeqStartWifiScene);
        } else {
            KeySystemSeq_Push(seq, DataConvert_SeqStartMenuScene);
        }
        (*state)++;
        break;
    case 4:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeIn);
        (*state)++;
        break;
    case 5:
        if (wk->loggedIn) {
            *state = 10;
        } else {
            *state = 40;
        }
        break;
    // Nothing goes to states 6 to 9
    case 6:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 141, KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        (*state)++;
        break;
    case 7:
        (*state)++;
        break;
    case 8: {
        KeySystemNetWifiGet get = { 0, 0 };

        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_WIFI_GET, &get);
        (*state)++;
        break;
    }
    case 9:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            KeySystemNetRequest *request = KeySystemNet_GetRequest(wk->net);

            switch (request->wifiGet.result) {
            case 2:
            case 3:
                GFL_NetErrShowScreen();
                *state = 40;
                break;
            default:
                if (request->wifiGet.value == 4) {
                    *state = 41;
                } else {
                    (*state)++;
                }
                break;
            }
        }
        break;
    case 10:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 134, KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case 11:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            KeySystemNet_SetNoErrorCheck(wk->net, TRUE);
            (*state)++;
        }
        break;
    case 12:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeOut);
        (*state)++;
        break;
    case 13:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 14:
        KeySystemSeq_Push(seq, DataConvert_SeqEnterId);
        (*state)++;
        break;
    case 15:
        KeySystemSeq_Push(seq, DataConvert_SeqStartWifiScene);
        (*state)++;
        break;
    case 16:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeIn);
        (*state)++;
        break;
    case 17:
        KeySystemNet_SetNoErrorCheck(wk->net, FALSE);
        if (wk->enteredCode == NULL) {
            *state = 30;
            break;
        }
        wk->wbSave = GFL_HeapAllocate(HEAPID_USER, WB_SAVE_SIZE, TRUE, "data_convert_flow.c", 849);
        *state = 18;
        break;
    case 18:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 137, KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        {
            KeySystemNetWifiPost post = { 0, 0, NULL, 0 };
            u64 id = 0;
            int i;
            int j;

            chars = GFL_StrBufGetStringPtr(wk->enteredCode);
            for (i = 0; i < GAME_SYNC_ID_LENGTH; i++) {
                value = 0xffffffff;
                for (j = 0; j < NELEMS(sIdChars); j++) {
                    if (chars[i] == sIdChars[j]) {
                        value = j;
                        break;
                    }
                }
                id += value << (i * 5);
            }
            post.id = id & 0xffffffff;
            post.buffer = wk->wbSave;
            post.size = WB_SAVE_SIZE;
            KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_WIFI_POST, &post);
            KeySystemNet_SetErrorCallback(wk->net, DataConvert_OnNetError, wk);
        }
        (*state)++;
        break;
    case 19:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            switch (KeySystemNet_GetRequest(wk->net)->wifi.result) {
            case 0:
            case 1:
                break;
            case 2:
            case 3:
                GFL_NetErrShowScreen();
                *state = 40;
                break;
            case 4:
                *state = 20;
                break;
            case 5:
                if (wk->wbSave != NULL) {
                    GFL_HeapFree(wk->wbSave);
                    wk->wbSave = NULL;
                }
                *state = 31;
                break;
            case 6:
                if (wk->wbSave != NULL) {
                    GFL_HeapFree(wk->wbSave);
                    wk->wbSave = NULL;
                }
                *state = 28;
                break;
            case 7:
                *state = 32;
                break;
            }
        }
        break;
    case 20:
        if (wk->wbData != NULL) {
            GFL_HeapFree(wk->wbData);
        }
        wk->wbData = WBSaveConvert_Create((u8 *)wk->wbSave + WB_SAVE_HEADER_SIZE, HEAPID_KEY_SYSTEM);
        GFL_HeapFree(wk->wbSave);
        wk->wbSave = NULL;
        (*state)++;
        break;
    case 21:
        if (MemoryLinkSave_CanMerge(wk->memoryLink, wk->wbData, GameData_GetSaveControl(wk->param->gameData))) {
            *state = 25;
        } else {
            *state = 22;
        }
        break;
    case 22:
        KeySystemMsgWin_Free(wk->msgWin);
        wk->msgWin = KeySystemMsgWin_Create(0, 2, 4, 28, 12, 14, wk->font, HEAPID_KEY_SYSTEM);
        KeySystemMsgWin_SetColor(wk->msgWin, PRINT_COLOR(1, 2, 15));
        KeySystemMsgWin_DrawFrame(wk->msgWin, 1, 15);
        GFL_BGSysQueueScrLoad(0);
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 132, KEY_SYSTEM_MSG_STREAM);
        *state = 23;
        break;
    case 23:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            KeySystemMenuSetup setup;

            MI_CpuFill8(&setup, 0, sizeof(KeySystemMenuSetup));
            setup.msgData = wk->msgData;
            setup.font = wk->font;
            setup.printQueue = wk->printQueue;
            setup.bg = 0;
            setup.palette = 14;
            setup.framePalette = 15;
            setup.frameChar = 1;
            setup.msgIds[0] = 65;
            setup.msgIds[1] = 66;
            setup.count = 2;
            setup.cancelable = TRUE;
            setup.cancelValue = 1;
            setup.cursor = 0;
            wk->menu = KeySystemMenu_CreateAt(&setup, 21, 19, 10, 4, HEAPID_KEY_SYSTEM);
            GFL_BGSysQueueScrLoad(0);
            (*state)++;
        }
        break;
    case 24:
        result = KeySystemMenu_Update(wk->menu);
        if (result == BMPMENULIST_NULL) {
            break;
        }
        KeySystemMenu_Free(wk->menu);
        wk->menu = NULL;
        switch (result) {
        case 0:
            (*state)++;
            break;
        case 1:
        default:
            *state = 30;
            break;
        }
        break;
    case 25:
        wk->transferResult =
            MemoryLinkSave_Merge(wk->memoryLink, wk->wbData, GameData_GetSaveControl(wk->param->gameData));
        (*state)++;
        break;
    case 26:
        KeySystemSeq_Push(seq, DataConvert_SeqSave);
        (*state)++;
        break;
    case 27:
        if (wk->transferResult) {
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 138, KEY_SYSTEM_MSG_STREAM_FAST);
        } else {
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 131, KEY_SYSTEM_MSG_STREAM_FAST);
        }
        *state = 33;
        break;
    case 28:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 135, KEY_SYSTEM_MSG_STREAM);
        *state = 29;
        break;
    case 29:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = 10;
        }
        break;
    case 30:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 133, KEY_SYSTEM_MSG_STREAM);
        *state = 33;
        break;
    case 31:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 143, KEY_SYSTEM_MSG_STREAM);
        *state = 29;
        break;
    case 32:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 142, KEY_SYSTEM_MSG_STREAM);
        *state = 33;
        break;
    case 33:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = 34;
        }
        break;
    case 34:
        KeySystemNet_SetMode(wk->net, KEY_SYSTEM_NET_MODE_NONE);
        if (wk->wbSave != NULL) {
            GFL_HeapFree(wk->wbSave);
            wk->wbSave = NULL;
        }
        KeySystemSeq_Push(seq, KeySystem_SeqFadeOut);
        (*state)++;
        break;
    case 35:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 36:
        KeySystemSeq_Push(seq, DataConvert_SeqWifiLogout);
        (*state)++;
        break;
    case 37:
        KeySystemSeq_Push(seq, DataConvert_SeqStartMenuScene);
        (*state)++;
        break;
    case 38:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeIn);
        (*state)++;
        break;
    case 39:
        (*state)++;
        break;
    case 40:
        KeySystemSeq_Pop(seq);
        break;
    case 41:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 140, KEY_SYSTEM_MSG_STREAM);
        // BUG: The message starts again every frame, and the next state is never reached. It never shows, since nothing
        // goes to states 6 to 9, which lead here
#ifdef BUGFIX
        *state = 42;
#else
        *state = 41;
#endif
        break;
    case 42:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = 40;
        }
        break;
    }
}

static void DataConvert_SeqSave(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 29, KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        (*state)++;
        break;
    case 1:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case 2:
        if (DataConvert_NeedsGameSave(wk)) {
            KeySystemSeq_Push(seq, DataConvert_SeqSaveBoth);
        } else {
            KeySystemSeq_Push(seq, DataConvert_SeqSaveOther);
        }
        (*state)++;
        break;
    case 3:
        GFL_SndSEPlay(SEQ_SE_SAVE);
        *state = 4;
        break;
    case 4:
        *state = 5;
        break;
    case 5:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 56, KEY_SYSTEM_MSG_STREAM_FAST);
        *state = 6;
        break;
    case 6:
        if (KeySystemMsgWin_IsDone(wk->msgWin) && !GFL_SndPlayerIsActiveAny()) {
            *state = 7;
        }
        break;
    case 7:
        KeySystemSeq_Pop(seq);
        break;
    }
}

// Writes ov331's save, which holds the Memory Link
static void DataConvert_SeqSaveOther(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        SaveOutside_StartSave(wk->ov331Work);
        wk->timer = 0;
        (*state)++;
        break;
    case 1:
        wk->timer++;
        if (SaveOutside_Save(wk->ov331Work)) {
            (*state)++;
        }
        break;
    case 2:
        if (wk->timer++ > 60) {
            wk->timer = 0;
            (*state)++;
        }
        break;
    case 3:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqSaveGame(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        GameData_StartSave(wk->param->gameData);
        (*state)++;
        break;
    case 1:
        if (GameData_UpdateSave(wk->param->gameData) == 2) {
            (*state)++;
        }
        break;
    case 2:
        (*state)++;
        break;
    case 3:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqSaveBoth(KeySystemSeq *seq, int *state, void *work) {
    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, DataConvert_SeqSaveGame);
        (*state)++;
        break;
    case 1:
        KeySystemSeq_Push(seq, DataConvert_SeqSaveOther);
        (*state)++;
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqMemoryList(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    u32 cursor;
    u32 top;

    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, DataConvert_SeqStartMemoryListScene);
        (*state)++;
        break;
    case 1:
        wk->choice = KeySystemScrollList_Update(wk->scrollList);
        if (wk->choice == (u32)-1) {
            break;
        }
        if (wk->choice != MEMORY_LIST_BACK) {
            KeySystemScrollList_GetPos(wk->scrollList, &cursor, &top);
            KeySystemTags_Set(wk->tags, "DI1", cursor);
            KeySystemTags_Set(wk->tags, "DI2", top);
        }
        (*state)++;
        break;
    case 2:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 3:
        if (wk->choice == MEMORY_LIST_BACK) {
            (*state)++;
        } else {
            KeySystemSeq_Push(seq, DataConvert_SeqMemoryInfo);
            *state = 0;
        }
        break;
    case 4:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqMemoryInfo(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, DataConvert_SeqStartMsgScene);
        (*state)++;
        break;
    case 1:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, wk->choice + 153, KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case 2:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case 3:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 4:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqStartMenuScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sDataConvertMenuSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case 1:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqStartLinkScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sDataConvertLinkSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case 1:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqStartWifiScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sDataConvertWifiSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case 1:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqStartMemoryListScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sDataConvertMemoryListSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case 1:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqStartMsgScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sDataConvertMsgSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case 1:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

// Unova Link frees its graphics while the login runs, and sets them up again after
static void DataConvert_SeqWifiLogin(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    WifiLoginParam *param;

    switch (*state) {
    case 0:
        KeySystem_Teardown(wk, FALSE);
        param = GFL_HeapAllocate(HEAPID_KEY_SYSTEM, sizeof(WifiLoginParam), TRUE, "data_convert_flow.c", 1705);
        param->gameData = wk->param->gameData;
        param->unk4 = 0;
        param->unk8 = 1;
        param->buffer = wk->loginBuffer;
        param->unk18 = 0;
        param->unkC = 0x38;
        if (SaveControl_IsDataAlreadyPresent(GameData_GetSaveControl(wk->param->gameData))) {
            param->unk14 = 0;
        } else {
            param->unk14 = 2;
        }
        wk->subProcParam = param;
        GCTX_ProcMgrQueueProc(OVERLAY_ID(190), &WIFILOGIN_PROC_FUNCTIONS, param);
        (*state)++;
        break;
    case 1:
        (*state)++;
        break;
    case 2:
        if (((WifiLoginParam *)wk->subProcParam)->result == 0) {
            wk->loggedIn = TRUE;
        } else {
            wk->loggedIn = FALSE;
        }
        GFL_HeapFree(wk->subProcParam);
        wk->subProcParam = NULL;
        KeySystem_Setup(wk, HEAPID_KEY_SYSTEM);
        KeySystemBG_LoadScreen(wk->bg, 2, 0);
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_2_TO_0, 1);
        GFL_NetSetIconScreen(TRUE, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case 3:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_SeqWifiLogout(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    WifiLogoutParam *param;

    switch (*state) {
    case 0:
        KeySystem_Teardown(wk, FALSE);
        param = GFL_HeapAllocate(HEAPID_KEY_SYSTEM, sizeof(WifiLogoutParam), TRUE, "data_convert_flow.c", 1789);
        param->gameData = wk->param->gameData;
        param->unk4 = 0;
        param->unk8 = 1;
        param->unkC = 0;
        param->unk10 = 0;
        wk->subProcParam = param;
        GCTX_ProcMgrQueueProc(OVERLAY_ID(190), &WIFILOGOUT_PROC_FUNCTIONS, param);
        (*state)++;
        break;
    case 1:
        (*state)++;
        break;
    case 2:
        GFL_HeapFree(wk->subProcParam);
        wk->subProcParam = NULL;
        KeySystem_Setup(wk, HEAPID_KEY_SYSTEM);
        KeySystemBG_LoadScreen(wk->bg, 2, 0);
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_2_TO_0, 1);
        (*state)++;
        break;
    case 3:
        KeySystemSeq_Pop(seq);
        break;
    }
}

// The player enters the Game Sync ID in the name entry
static void DataConvert_SeqEnterId(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    NameEntryParam *param;

    switch (*state) {
    case 0:
        KeySystem_Teardown(wk, TRUE);
        param = GFL_HeapAllocate(HEAPID_KEY_SYSTEM, sizeof(NameEntryParam), TRUE, "data_convert_flow.c", 1848);
        param->mode = NAME_ENTRY_GAME_SYNC_ID;
        param->maxLength = GAME_SYNC_ID_LENGTH;
        param->unk2C = 3;
        param->name = GFL_StrBufCreate(GAME_SYNC_ID_LENGTH + 1, HEAPID_KEY_SYSTEM);
        wk->subProcParam = param;
        GCTX_ProcMgrQueueProc(OVERLAY_ID(280), &NAME_ENTRY_PROC_FUNCTIONS, param);
        (*state)++;
        break;
    case 1:
        (*state)++;
        break;
    case 2:
        param = wk->subProcParam;
        if (param->canceled) {
            if (wk->enteredCode != NULL) {
                GFL_StrBufFree(wk->enteredCode);
                wk->enteredCode = NULL;
            }
        } else {
            if (wk->enteredCode == NULL) {
                wk->enteredCode = GFL_StrBufCreate(GAME_SYNC_ID_LENGTH + 1, HEAPID_KEY_SYSTEM);
            }
            GFL_StrBufCopy(wk->enteredCode, param->name);
        }
        GFL_StrBufFree(param->name);
        GFL_HeapFree(param);
        wk->subProcParam = NULL;
        KeySystem_Setup(wk, HEAPID_KEY_SYSTEM);
        KeySystemBG_LoadScreen(wk->bg, 2, 0);
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_2_TO_0, 1);
        GFL_NetSetIconScreen(TRUE, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case 3:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void DataConvert_MenuSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    KeySystemListSetup setup;
    int i;

    KeySystemMsgWin_PrintMsg(wk->titleWin, wk->msgData, 76, KEY_SYSTEM_MSG_PRINT);
    MI_CpuFill8(&setup, 0, sizeof(KeySystemListSetup));
    setup.bg = 1;
    setup.unk04 = 14;
    setup.frameChar = 10;
    setup.palette = 1;
    setup.msgData = wk->msgData;
    setup.font = wk->font;
    setup.count = 3;
    if (KeySystemTags_Has(wk->tags, "DNM")) {
        setup.cursor = KeySystemTags_Get(wk->tags, "DNM");
        KeySystemTags_Remove(wk->tags, "DNM");
    } else {
        setup.cursor = 0;
    }
    for (i = 0; i < KEY_SYSTEM_LIST_MAX; i++) {
        setup.items[i].width = 26;
        setup.items[i].height = 2;
        setup.items[i].x = 3;
        setup.items[i].y = (setup.items[i].height + 3) * i + 5;
    }
    setup.items[0].msgId = 77;
    setup.items[1].msgId = 78;
    if (DataConvert_HasMemories(wk)) {
        setup.items[2].msgId = 93;
        setup.items[3].msgId = 79;
        setup.count = 4;
    } else {
        setup.items[2].msgId = 79;
    }
    wk->list = KeySystemList_Create(&setup, heapId);
    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, setup.cursor + 80, KEY_SYSTEM_MSG_PRINT);
}

static BOOL DataConvert_MenuSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL infoDone;
    BOOL listDone;

    // The title isn't waited for
    KeySystemMsgWin_IsDone(wk->titleWin);
    infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    listDone = KeySystemList_IsPrinted(wk->list);
    if (infoDone && listDone) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

static void DataConvert_MenuSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemList_Free(wk->list);
    wk->list = NULL;
    KeySystemMsgWin_Clear(wk->titleWin);
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

// What to do on the other system, with the Memory Link ID, and the picture of two systems
static void DataConvert_LinkSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    ArcTool *arc;

    wk->msgWin = KeySystemMsgWin_Create(0, 2, 4, 28, 16, 14, wk->font, HEAPID_KEY_SYSTEM);
    KeySystemMsgWin_SetColor(wk->msgWin, PRINT_COLOR(1, 2, 15));
    KeySystemMsgWin_DrawFrame(wk->msgWin, 1, 15);
    if (!SaveControl_IsDataAlreadyPresent(GameData_GetSaveControl(wk->param->gameData))) {
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 125, KEY_SYSTEM_MSG_PRINT);
    } else {
        PlayerInfo *player = GetGameDataPlayerInfo(wk->param->gameData);
        StrBuf *src = GFL_MsgDataLoadStrbufNew(wk->msgData, 124);
        StrBuf *dest = GFL_StrBufCreate(337, heapId);

        WordSetNumber(wk->wordSet, 0, getTrainerID(player), 5, 2, TRUE);
        GFL_WordSetFormatStrbuf(wk->wordSet, dest, src);
        KeySystemMsgWin_PrintStr(wk->msgWin, dest, KEY_SYSTEM_MSG_PRINT);
        GFL_StrBufFree(dest);
        GFL_StrBufFree(src);
    }
    arc = GFL_ArcSysCreateFileHandle(ARCID_DATA_CONVERT, heapId);
    GFL_G2DIOLoadArcNCLR(arc, 2, 4, 0x120, 0x120, 0x60, heapId);
    GFL_G2DIOLoadArcNCLR(arc, 3, 4, 0x1a0, 0x1a0, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 7, 4, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 6, 6, 0, 0x6000, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 11, 4, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 10, 6, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
}

static BOOL DataConvert_LinkSceneMain(void *work) {
    KeySystemWork *wk = work;

    if (KeySystemMsgWin_IsDone(wk->msgWin)) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(4);
        GFL_BGSysQueueScrLoad(6);
        return TRUE;
    }
    return FALSE;
}

static void DataConvert_LinkSceneExit(void *work) {
    KeySystemWork *wk = work;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_KEY_SYSTEM, HEAPID_KEY_SYSTEM);

    GFL_BGSysLoadArcNCGRStatic(arc, 3, 6, 0, 0, FALSE, HEAPID_KEY_SYSTEM);
    GFL_ArcToolFree(arc);
    LoadSysMsgBox(4, 1, 15, 0, HEAPID_KEY_SYSTEM);
    KeySystem_FreeMsgWin(wk);
    GFL_BGSysClearScr(4);
    GFL_BGSysClearScr(6);
    GFL_BGSysQueueScrLoad(4);
    GFL_BGSysQueueScrLoad(6);
}

static void DataConvert_WifiSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    wk->msgWin = KeySystemMsgWin_Create(0, 2, 4, 28, 16, 14, wk->font, HEAPID_KEY_SYSTEM);
    KeySystemMsgWin_SetColor(wk->msgWin, PRINT_COLOR(1, 2, 15));
    KeySystemMsgWin_DrawFrame(wk->msgWin, 1, 15);
    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 0, KEY_SYSTEM_MSG_PRINT);
    KeySystemMsgWin_PrintMsg(wk->titleWin, wk->msgData, 76, KEY_SYSTEM_MSG_PRINT);
}

static BOOL DataConvert_WifiSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL msgDone = KeySystemMsgWin_IsDone(wk->msgWin);
    BOOL titleDone = KeySystemMsgWin_IsDone(wk->titleWin);

    if (msgDone && titleDone) {
        GFL_BGSysQueueScrLoad(0);
        return TRUE;
    }
    return FALSE;
}

static void DataConvert_WifiSceneExit(void *work) {
}

static void DataConvert_MemoryListSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    KeySystemScrollListSetup setup;
    int i;

    KeySystemMsgWin_PrintMsg(wk->titleWin, wk->msgData, 139, KEY_SYSTEM_MSG_PRINT);
    MI_CpuFill8(&setup, 0, sizeof(KeySystemScrollListSetup));
    setup.msgData = wk->msgData;
    setup.font = wk->font;
    setup.arrowUp = KeySystemClAct_Create(&wk->clact, 22, heapId);
    setup.arrowDown = KeySystemClAct_Create(&wk->clact, 23, heapId);
    for (i = 0; i < MEMORY_COUNT; i++) {
        if (DataConvert_IsMemoryOpen(wk, i)) {
            setup.msgIds[setup.count] = i + 144;
            setup.values[setup.count] = i;
            if (DataConvert_IsMemorySeen(wk, i)) {
                setup.icons[setup.count] = KeySystemClAct_Create(&wk->clact, i + 13, heapId);
            }
            setup.count++;
        }
    }
    setup.msgIds[setup.count] = 152;
    setup.values[setup.count] = MEMORY_LIST_BACK;
    setup.icons[setup.count] = NULL;
    setup.count++;
    setup.bg = 1;
    setup.palette = 1;
    setup.unkA8 = 10;
    if (KeySystemTags_Has(wk->tags, "DI1")) {
        setup.cursor = KeySystemTags_Get(wk->tags, "DI1");
        setup.unkAC = KeySystemTags_Get(wk->tags, "DI2");
        KeySystemTags_Remove(wk->tags, "DI1");
        KeySystemTags_Remove(wk->tags, "DI2");
    }
    wk->scrollList = KeySystemScrollList_Create(&setup, heapId);
    GX_SetVisiblePlane(GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3);
    wk->timer = 0;
}

static BOOL DataConvert_MemoryListSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL started = KeySystemScrollList_Start(wk->scrollList);
    BOOL titleDone = KeySystemMsgWin_IsDone(wk->titleWin);

    // The list is shown two frames after it is drawn
    if (started && titleDone && wk->timer++ > 1) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(1);
        GX_SetVisiblePlane(GX_PLANEMASK_ALL);
        return TRUE;
    }
    return FALSE;
}

static void DataConvert_MemoryListSceneExit(void *work) {
    KeySystemWork *wk = work;
    int i;

    KeySystemScrollList_Free(wk->scrollList);
    wk->scrollList = NULL;
    for (i = 13; i < 13 + MEMORY_COUNT; i++) {
        KeySystemClAct_Delete(&wk->clact, i);
    }
    KeySystemClAct_Delete(&wk->clact, 22);
    KeySystemClAct_Delete(&wk->clact, 23);
    GFL_BGSysClearScr(1);
    GFL_BGSysQueueScrLoad(1);
}

static void DataConvert_MsgSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystem_CreateMsgWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 0, KEY_SYSTEM_MSG_PRINT);
}

static BOOL DataConvert_MsgSceneMain(void *work) {
    KeySystemWork *wk = work;

    if (KeySystemMsgWin_IsDone(wk->msgWin)) {
        GFL_BGSysQueueScrLoad(0);
        return TRUE;
    }
    return FALSE;
}

static void DataConvert_MsgSceneExit(void *work) {
    KeySystem_FreeMsgWin(work);
    GFL_BGSysQueueScrLoad(0);
}

static void DataConvert_OnNetError(void *work) {
    KeySystemWork *wk = work;

    KeySystemMsgWin_ClearFrame(wk->msgWin);
    GFL_BGSysLoadScr(0);
}

// Always FALSE: only ov331's save is written
static BOOL DataConvert_NeedsGameSave(KeySystemWork *wk) {
    SaveControl *save = GameData_GetSaveControl(wk->param->gameData);

    if (SaveControl_IsDataAlreadyPresent(save)) {
        getConstDataBlock(save);
        return FALSE;
    }
    return FALSE;
}

static BOOL DataConvert_IsMemoryOpen(KeySystemWork *wk, u32 memory) {
    if (MemoryLinkSave_CheckMemoryCondition(wk->memoryLink, memory) || DataConvert_IsMemorySeen(wk, memory)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL DataConvert_HasMemories(KeySystemWork *wk) {
    int i;

    for (i = 0; i < MEMORY_COUNT; i++) {
        if (DataConvert_IsMemoryOpen(wk, i)) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL DataConvert_IsMemorySeen(KeySystemWork *wk, u32 memory) {
    EventWork *eventWork = getConstDataBlock(GameData_GetSaveControl(wk->param->gameData));

    if (*EventWork_GetWkPtr(eventWork, MemoryLink_GetMemoryEventWork(memory)) == 1) {
        return TRUE;
    }
    return FALSE;
}
