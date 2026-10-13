#include "types.h"
#include "app/key_system_flow.h"
#include "app/unova_link.h"
#include "constants/sound.h"
#include "constants/version.h"
#include "field/zone.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "save/key_info.h"
#include "save/save_control.h"
#include "save/save_outside.h"
#include "system/bmp_menulist.h"
#include "system/bmp_oam.h"
#include "system/dsi.h"
#include "system/game_data.h"
#include "system/wordset.h"

// The Key System's sequences and scenes: exchanging keys with another Black 2 or White 2 over infrared, and the
// settings that keys unlock, the difficulty, the city and the chamber behind the mystery door. The ROM doesn't name
// this file; its helpers and the scene tables are in key_system_flow_sub.c (the ROM's .rodata order says so: the
// four message tables below pack as one section before the sub file's initializer and scene tables)

static void KeySystemFlow_SeqExchange(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqSettings(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqDifficulty(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqCity(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqChamber(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqReceive(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqSend(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqSave(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartMenuScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartConnectScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartSettingsScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartDifficultyScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartCityScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartKeyAnimScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartSendScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartKeysHeldScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemFlow_SeqStartChamberScene(KeySystemSeq *seq, int *state, void *work);

// What a key that is already enabled says, what an unlocked key says, and the keys' names in the lists of keys
// being sent and held, in each version's order. MWCC heapsorts a unit's data by size, which gives the ROM's
// order of the three equal-size tables (unlocked, send, held) only from this declaration order
static const u16 sKeyEnabledMsgs[KEY_SYSTEM_KEY_COUNT] = { 107, 108, 0, 109, 110 };
static const u16 sHeldKeyNameMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { 14, 15, 17, 105, 106 },
    { 14, 15, 16, 105, 106 },
};
static const u16 sSendKeyNameMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { 14, 15, 17, 105, 106 },
    { 14, 15, 16, 105, 106 },
};
static const u16 sKeyUnlockedMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { 89, 90, 91, 103, 104 },
    { 89, 90, 92, 103, 104 },
};

// The windows of the keys being sent, a title and then a key's name in each
static KeySystemMsgWinTemplate sSendTemplates[] = {
    { 1, 4, 18, 2, 13 }, { 1, 7, 18, 2, 0 }, { 1, 10, 18, 2, 0 }, { 1, 13, 18, 2, 0 }, { 1, 16, 18, 2, 0 },
};

// The windows of the keys held
static KeySystemMsgWinTemplate sKeysHeldTemplates[] = {
    { 1, 1, 18, 2, 13 }, { 1, 4, 18, 2, 0 },  { 1, 7, 18, 2, 0 },  { 1, 10, 18, 2, 0 },
    { 1, 13, 18, 2, 0 }, { 1, 16, 18, 2, 0 }, { 1, 19, 18, 2, 0 },
};

void KeySystemFlow_Init(KeySystemWork *wk, HeapID heapId) {
    KeySystemKeyState_Init(&wk->keys, wk->keyInfo);
    KeySystemSaveInfo_Init(&wk->saveInfo, wk->param->gameData);
    if (wk->param->mode == UNOVA_LINK_MODE_GAME_CLEAR) {
        GFL_SndBGMPush();
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_1_TO_0, 1);
        func_02042ba8(TRUE, heapId);
    }
}

void KeySystemFlow_Exit(KeySystemWork *wk) {
    UnovaLinkParam *param = wk->param;

    if (param->mode == UNOVA_LINK_MODE_GAME_CLEAR) {
        if (param->key <= KEY_SYSTEM_KEY_CHALLENGE) {
            func_0200ca78(getTrainerCardDataBlkAddress(param->gameData), 1);
        }
        GFL_SndBGMPop();
    }
}

void KeySystemFlow_SeqGameClear(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    StrBuf *str;
    BOOL enabled;
    u16 msgId;

    switch (*state) {
    case 0:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartKeyAnimScene);
        (*state)++;
        break;
    case 1:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeIn);
        (*state)++;
        break;
    case 2:
        enabled = FALSE;
        msgId = sKeyNameMsgs[VERSION_INDEX][wk->param->key];
        str = GFL_MsgDataLoadStrbufNew(wk->msgData, sKeyUnlockedMsgs[VERSION_INDEX][wk->param->key]);
        if (wk->param->key == KEY_SYSTEM_KEY_CITY) {
            enabled = TRUE;
        } else if (keyEnabler(wk->keyInfo, wk->param->key)) {
            enabled = TRUE;
            GFL_StrBufFree(str);
            str = GFL_MsgDataLoadStrbufNew(wk->msgData, sKeyEnabledMsgs[wk->param->key]);
        }
        KeySystemKeyAnim_Start(&wk->keyAnim, wk->param->key, wk->msgData, msgId, str, enabled);
        GFL_StrBufFree(str);
        (*state)++;
        break;
    case 3:
        if (KeySystemKeyAnim_Update(&wk->keyAnim)) {
            (*state)++;
        }
        break;
    case 4:
        func_020104b0(wk->keyInfo, wk->param->key);
        if (wk->param->key != KEY_SYSTEM_KEY_CITY) {
            func_020104e0(wk->keyInfo, wk->param->key);
        }
        (*state)++;
        break;
    case 5:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeOut);
        (*state)++;
        break;
    case 6:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        KeySystem_FreeMsgWin(wk);
        (*state)++;
        break;
    case 7:
        KeySystemSeq_Pop(seq);
        break;
    }
}

void KeySystemFlow_SeqMenu(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartMenuScene);
        (*state)++;
        break;
    case 1:
        KeySystemList_Update(wk->list);
        if (KeySystemList_IsChanged(wk->list) && KeySystemMsgWin_IsDone(wk->infoWin)) {
            KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemList_GetCursor(wk->list) + 18,
                                     KEY_SYSTEM_MSG_PRINT);
        }
        if (KeySystemList_IsDecided(wk->list)) {
            wk->choice = KeySystemList_GetCursor(wk->list);
            if (wk->choice != KEY_MENU_BACK) {
                KeySystemTags_Set(wk->tags, "KEM", KeySystemList_GetCursor(wk->list));
            }
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 3:
        switch (wk->choice) {
        case KEY_MENU_EXCHANGE:
            if (isWirelessEnabled()) {
                KeySystemSeq_Push(seq, KeySystemFlow_SeqExchange);
            } else {
                KeySystemSeq_Push(seq, KeySystem_SeqWirelessOff);
            }
            *state = 0;
            break;
        case KEY_MENU_SETTINGS:
            KeySystemSeq_Push(seq, KeySystemFlow_SeqSettings);
            *state = 0;
            break;
        case KEY_MENU_BACK:
            (*state)++;
            break;
        }
        break;
    case 4:
        KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_0_TO_1, 30);
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqExchange(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    s32 result;

    switch (*state) {
    case 0:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartKeysHeldScene);
        (*state)++;
        break;
    case 1:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 67, KEY_SYSTEM_MSG_STREAM);
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
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        KeySystem_FreeMenu(wk);
        switch (result) {
        case 0:
            *state = 7;
            break;
        case 1:
        default:
            KeySystemMsgWinGroup_Free(wk->msgWinGroup);
            wk->msgWinGroup = NULL;
            GFL_BGSysClearScr(6);
            GFL_BGSysQueueScrLoad(6);
            GFL_BGSysQueueScrLoad(4);
            *state = 25;
            break;
        }
        break;
    case 5:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartKeysHeldScene);
        (*state)++;
        break;
    case 6:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 111, KEY_SYSTEM_MSG_STREAM);
        *state = 2;
        break;
    case 7:
        wk->choice = 0;
        wk->noCancel = FALSE;
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartConnectScene);
        func_02042ba8(TRUE, HEAPID_KEY_SYSTEM);
        KeySystemNet_SetMode(wk->net, KEY_SYSTEM_NET_MODE_WIRELESS);
        GFL_SndSEPlay(SEQ_SE_SW_KEYSYS_01);
        (*state)++;
        break;
    case 8: {
        KeySystemNetCallback callback;

        sys_memset(&callback, 0, sizeof(KeySystemNetCallback));
        callback.arg = wk;
        callback.func = KeySystemFlow_OnNetError;
        wk->choice = 0;
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_CONNECT, &callback);
        (*state)++;
        break;
    }
    case 9:
        if (wk->choice) {
            ClActor *actor0 = KeySystemClAct_GetActor(&wk->clact, 0);
            ClActor *actor1 = KeySystemClAct_GetActor(&wk->clact, 1);
            ClActor *actor2 = KeySystemClAct_GetActor(&wk->clact, 2);
            ClActor *actor3 = KeySystemClAct_GetActor(&wk->clact, 3);

            func_0204c124(actor0, FALSE);
            func_0204c124(actor1, FALSE);
            func_0204c488(actor2, 4);
            func_0204c488(actor3, 5);
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 60, KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
            GFL_SndStop();
            GFL_SndSEPlay(SEQ_SE_SYS_12);
            wk->choice = 0;
        } else if (!wk->noCancel && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
            KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_CANCEL, NULL);
        }
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_CONNECTED) {
            wk->isParent = func_02042bc4();
            *state = 13;
        } else if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            GFL_SndStop();
            *state = 11;
        } else if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_CANCELING) {
            GFL_SndStop();
            *state = 10;
        }
        break;
    case 10:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            *state = 12;
        }
        break;
    case 11:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        *state = 5;
        break;
    case 12:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        *state = 25;
        break;
    case 13: {
        KeySystemNetSend send;

        send.size = sizeof(KeySystemSaveInfo);
        send.data = &wk->saveInfo;
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_SEND, &send);
        (*state)++;
        break;
    }
    case 14:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_CONNECTED) {
            wk->choice = KeySystemNet_GetReceived(wk->net, &wk->partnerSaveInfo, sizeof(KeySystemSaveInfo));
            (*state)++;
        }
        break;
    case 15: {
        KeySystemNetSend send;

        send.size = sizeof(KeySystemKeyState);
        send.data = &wk->keys;
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_SEND, &send);
        (*state)++;
        break;
    }
    case 16:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_CONNECTED) {
            wk->choice = KeySystemNet_GetReceived(wk->net, &wk->partnerKeys, sizeof(KeySystemKeyState));
            (*state)++;
        }
        break;
    case 17:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        if (wk->choice) {
            (*state)++;
        } else {
            *state = 20;
        }
        break;
    case 18:
        if (wk->isParent) {
            KeySystemSeq_Push(seq, KeySystemFlow_SeqReceive);
        } else {
            KeySystemSeq_Push(seq, KeySystemFlow_SeqSend);
        }
        (*state)++;
        break;
    case 19:
        if (!wk->isParent) {
            KeySystemSeq_Push(seq, KeySystemFlow_SeqReceive);
        } else {
            KeySystemSeq_Push(seq, KeySystemFlow_SeqSend);
        }
        (*state)++;
        break;
    case 20:
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_DISCONNECT, NULL);
        (*state)++;
        break;
    case 21:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_IDLE) {
            (*state)++;
        }
        break;
    case 22:
        if (KeySystemKeyState_CanExchange(&wk->keys, &wk->partnerKeys, TRUE)) {
            KeySystemKeyState_Receive(&wk->keys, &wk->partnerKeys);
            KeySystemKeyState_Save(&wk->keys, wk->keyInfo);
            KeySystemSeq_Push(seq, KeySystemFlow_SeqSave);
        }
        *state = 23;
        break;
    case 23:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 57, KEY_SYSTEM_MSG_STREAM_FAST);
        (*state)++;
        break;
    case 24:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case 25:
        KeySystemNet_SetMode(wk->net, KEY_SYSTEM_NET_MODE_NONE);
        KeySystem_FreeMsgWin(wk);
        GFL_BGSysQueueScrLoad(0);
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqSettings(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartSettingsScene);
        (*state)++;
        break;
    case 1:
        KeySystemList_Update(wk->list);
        if (KeySystemList_IsChanged(wk->list) && KeySystemMsgWin_IsDone(wk->infoWin)) {
            KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData,
                                     KeySystemFlow_GetSettingsMsg(wk->keyInfo, KeySystemList_GetCursor(wk->list)),
                                     KEY_SYSTEM_MSG_PRINT);
        }
        if (KeySystemList_IsDecided(wk->list)) {
            wk->choice = KeySystemList_GetCursor(wk->list);
            if (wk->choice != KEY_LIST_BACK) {
                KeySystemTags_Set(wk->tags, "KLL", KeySystemList_GetCursor(wk->list));
            }
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 3:
        switch (wk->choice) {
        case KEY_LIST_DIFFICULTY:
            KeySystemSeq_Push(seq, KeySystemFlow_SeqDifficulty);
            *state = 4;
            break;
        case KEY_LIST_CITY:
            KeySystemSeq_Push(seq, KeySystemFlow_SeqCity);
            *state = 4;
            break;
        case KEY_LIST_CHAMBER:
            KeySystemSeq_Push(seq, KeySystemFlow_SeqChamber);
            *state = 4;
            break;
        case KEY_LIST_BACK:
            KeySystemSeq_Pop(seq);
            break;
        }
        break;
    case 4:
        *state = 0;
        break;
    }
}

static void KeySystemFlow_SeqDifficulty(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartDifficultyScene);
        (*state)++;
        break;
    case 1:
        KeySystemKeySelect_Update(&wk->keySelect);
        if (KeySystemKeySelect_IsDecided(&wk->keySelect)) {
            if (KeySystemKeySelect_GetCursor(&wk->keySelect) == 3) {
                *state = 10;
            } else {
                *state = 2;
            }
        } else if (KeySystemKeySelect_GetResult(&wk->keySelect) == KEY_SELECT_LOCKED) {
            *state = 8;
        }
        break;
    case 2:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        KeySystemSeq_Push(seq, KeySystemFlow_SeqSave);
        (*state)++;
        break;
    case 3:
        KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetDifficultyMsg(wk->keyInfo),
                                 KEY_SYSTEM_MSG_PRINT);
        (*state)++;
        break;
    case 4:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case 5:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, GetGameDifficulty(wk->keyInfo) + 112, KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case 6:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = 7;
        }
        break;
    case 7:
        KeySystem_FreeMsgWin(wk);
        GFL_BGSysQueueScrLoad(0);
        KeySystemKeySelect_Reset(&wk->keySelect);
        *state = 1;
        break;
    case 8:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        switch (KeySystemKeySelect_GetIndex(&wk->keySelect)) {
        case GAME_DIFFICULTY_EASY:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 61, KEY_SYSTEM_MSG_PRINT);
            break;
        case GAME_DIFFICULTY_CHALLENGE:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 62, KEY_SYSTEM_MSG_PRINT);
            break;
        }
        (*state)++;
        break;
    case 9:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            GFL_BGSysQueueScrLoad(0);
            if (GCTX_HIDGetPressedKeys()) {
                *state = 7;
            }
        }
        break;
    case 10:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 11:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqCity(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartCityScene);
        (*state)++;
        break;
    case 1:
        KeySystemKeySelect_Update(&wk->keySelect);
        if (KeySystemKeySelect_IsDecided(&wk->keySelect)) {
            if (KeySystemKeySelect_GetCursor(&wk->keySelect) == 2) {
                *state = 10;
            } else {
                *state = 2;
            }
        } else if (KeySystemKeySelect_GetResult(&wk->keySelect) == KEY_SELECT_LOCKED ||
                   KeySystemKeySelect_GetResult(&wk->keySelect) == KEY_SELECT_IN_AREA) {
            *state = 8;
        }
        break;
    case 2:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        KeySystemSeq_Push(seq, KeySystemFlow_SeqSave);
        (*state)++;
        break;
    case 3:
        KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetCityMsg(wk->keyInfo), KEY_SYSTEM_MSG_PRINT);
        (*state)++;
        break;
    case 4:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case 5:
#ifdef BLACK2
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, KeyInfo_GetCityKey(wk->keyInfo) + 115, KEY_SYSTEM_MSG_STREAM);
#else
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, (KeyInfo_GetCityKey(wk->keyInfo) == 0 ? 1 : 0) + 115,
                                 KEY_SYSTEM_MSG_STREAM);
#endif
        (*state)++;
        break;
    case 6:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = 7;
        }
        break;
    case 7:
        KeySystem_FreeMsgWin(wk);
        GFL_BGSysQueueScrLoad(0);
        KeySystemKeySelect_Reset(&wk->keySelect);
        *state = 1;
        break;
    case 8:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        if (KeySystemKeySelect_GetResult(&wk->keySelect) == KEY_SELECT_IN_AREA) {
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 68, KEY_SYSTEM_MSG_PRINT);
        } else if (KeySystemKeySelect_GetIndex(&wk->keySelect) == 1) {
#ifdef BLACK2
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 63, KEY_SYSTEM_MSG_PRINT);
#else
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 64, KEY_SYSTEM_MSG_PRINT);
#endif
        }
        (*state)++;
        break;
    case 9:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            GFL_BGSysQueueScrLoad(0);
            if (GCTX_HIDGetPressedKeys()) {
                *state = 7;
            }
        }
        break;
    case 10:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 11:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqChamber(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartChamberScene);
        (*state)++;
        break;
    case 1:
        KeySystemKeySelect_Update(&wk->keySelect);
        if (KeySystemKeySelect_IsDecided(&wk->keySelect)) {
            if (KeySystemKeySelect_GetCursor(&wk->keySelect) == 3) {
                *state = 10;
            } else {
                *state = 2;
            }
        } else if (KeySystemKeySelect_GetResult(&wk->keySelect) == KEY_SELECT_LOCKED) {
            *state = 8;
        }
        break;
    case 2:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        KeySystemSeq_Push(seq, KeySystemFlow_SeqSave);
        (*state)++;
        break;
    case 3:
        KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetChamberMsg(wk->keyInfo),
                                 KEY_SYSTEM_MSG_PRINT);
        (*state)++;
        break;
    case 4:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case 5:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, func_020105a0(wk->keyInfo) + 117, KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case 6:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = 7;
        }
        break;
    case 7:
        KeySystem_FreeMsgWin(wk);
        GFL_BGSysQueueScrLoad(0);
        KeySystemKeySelect_Reset(&wk->keySelect);
        *state = 1;
        break;
    case 8:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        switch (KeySystemKeySelect_GetIndex(&wk->keySelect)) {
        case 1:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 97, KEY_SYSTEM_MSG_PRINT);
            break;
        case 2:
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 98, KEY_SYSTEM_MSG_PRINT);
            break;
        }
        (*state)++;
        break;
    case 9:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            GFL_BGSysQueueScrLoad(0);
            if (GCTX_HIDGetPressedKeys()) {
                *state = 7;
            }
        }
        break;
    case 10:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 11:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqReceive(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        if (KeySystemKeyState_CanExchange(&wk->keys, &wk->partnerKeys, TRUE)) {
            *state = 3;
        } else {
            *state = 1;
        }
        break;
    case 1:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 59, KEY_SYSTEM_MSG_STREAM_FAST);
        (*state)++;
        break;
    case 2:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = 9;
        }
        break;
    case 3:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartKeyAnimScene);
        wk->timer = 0;
        wk->choice = 0;
        (*state)++;
        break;
    case 4:
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_SYNC, NULL);
        (*state)++;
        break;
    case 5:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_CONNECTED) {
            (*state)++;
        }
        break;
    case 6:
        // The key that is looked at
        while (TRUE) {
            if (wk->choice >= KEY_SYSTEM_KEY_COUNT) {
                *state = 8;
                return;
            }
            if (KeySystemFlow_FormatKeyMsg(wk, wk->choice, TRUE)) {
                KeySystemKeyAnim_Start(&wk->keyAnim, wk->choice, wk->msgData,
                                       sReceivedKeyNameMsgs[VERSION_INDEX][wk->choice], wk->strBuf, FALSE);
                KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 43, KEY_SYSTEM_MSG_STREAM_FAST);
                (*state)++;
                return;
            }
            wk->choice++;
        }
    case 7:
        if (KeySystemKeyAnim_Update(&wk->keyAnim)) {
            wk->timer = 0;
            wk->choice++;
            *state = 4;
        }
        break;
    case 8:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 9:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqSend(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        if (KeySystemKeyState_CanExchange(&wk->keys, &wk->partnerKeys, FALSE)) {
            *state = 3;
        } else {
            *state = 1;
        }
        break;
    case 1:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 58, KEY_SYSTEM_MSG_STREAM_FAST);
        (*state)++;
        break;
    case 2:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            *state = 10;
        }
        break;
    case 3:
        KeySystemSeq_Push(seq, KeySystemFlow_SeqStartSendScene);
        wk->timer = 0;
        wk->choice = 0;
        (*state)++;
        break;
    case 4:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            wk->timer = 0;
            (*state)++;
        }
        break;
    case 5:
        KeySystemNet_Request(wk->net, KEY_SYSTEM_NET_REQUEST_SYNC, NULL);
        (*state)++;
        break;
    case 6:
        if (KeySystemNet_GetState(wk->net) == KEY_SYSTEM_NET_STATE_CONNECTED) {
            (*state)++;
        }
        break;
    case 7:
        while (TRUE) {
            if (wk->choice >= KEY_SYSTEM_KEY_COUNT) {
                *state = 9;
                return;
            }
            if (KeySystemFlow_FormatKeyMsg(wk, wk->choice, FALSE)) {
                KeySystemMsgWin_PrintStr(wk->msgWin, wk->strBuf, KEY_SYSTEM_MSG_STREAM_FAST);
                (*state)++;
                return;
            }
            wk->choice++;
        }
    case 8:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            wk->timer = 0;
            wk->choice++;
            *state = 5;
        }
        break;
    case 9:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 10:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqSave(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    u32 mode;

    switch (*state) {
    case 0:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 29, KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
        (*state)++;
        break;
    case 1:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            GFL_BGSysQueueScrLoad(0);
            if (SaveControl_IsDataAlreadyPresent(GameData_GetSaveControl(wk->param->gameData))) {
                func_0201782c(wk->param->gameData);
            } else {
                SaveOutside_StartSave(wk->ov331Work);
            }
            wk->timer = 0;
            (*state)++;
        }
        break;
    case 2:
        wk->timer++;
        if (SaveControl_IsDataAlreadyPresent(GameData_GetSaveControl(wk->param->gameData))) {
            if (func_02017850(wk->param->gameData) == 2) {
                (*state)++;
            }
        } else if (SaveOutside_Save(wk->ov331Work)) {
            (*state)++;
        }
        break;
    case 3:
        GFL_SndSEPlay(SEQ_SE_SAVE);
        *state = 4;
        break;
    case 4:
        *state = 5;
        break;
    case 5:
        if (wk->timer++ > 60) {
            mode = KEY_SYSTEM_MSG_STREAM;
            if (func_02042788()) {
                mode = KEY_SYSTEM_MSG_STREAM_FAST;
            }
            wk->timer = 0;
            KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 56, mode);
            (*state)++;
        }
        break;
    case 6:
        if (KeySystemMsgWin_IsDone(wk->msgWin) && !GFL_SndPlayerIsActiveAny()) {
            (*state)++;
        }
        break;
    case 7:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemFlow_SeqStartMenuScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sMenuSceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartConnectScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sConnectSceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartSettingsScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sSettingsSceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartDifficultyScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sDifficultySceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartCityScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sCitySceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartKeyAnimScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sKeyAnimSceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartSendScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sSendSceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartKeysHeldScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sKeysHeldSceneFuncs, HEAPID_KEY_SYSTEM);
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

static void KeySystemFlow_SeqStartChamberScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sChamberSceneFuncs, HEAPID_KEY_SYSTEM);
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

void KeySystemFlow_MenuSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    KeySystemListSetup setup;
    u32 i;

    KeySystemMsgWin_PrintMsg(wk->titleWin, wk->msgData, 1, KEY_SYSTEM_MSG_PRINT);
    sys_memset(&setup, 0, sizeof(KeySystemListSetup));
    setup.bg = 1;
    setup.unk04 = 14;
    setup.frameChar = 10;
    setup.palette = 1;
    setup.msgData = wk->msgData;
    setup.font = wk->font;
    setup.count = 3;
    if (KeySystemTags_Has(wk->tags, "KEM")) {
        setup.cursor = KeySystemTags_Get(wk->tags, "KEM");
        KeySystemTags_Remove(wk->tags, "KEM");
    } else {
        setup.cursor = 0;
    }
    for (i = 0; i < setup.count; i++) {
        setup.items[i].width = 26;
        setup.items[i].height = 2;
        setup.items[i].x = 3;
        setup.items[i].y = (setup.items[i].height + 3) * i + 5;
    }
    setup.items[0].msgId = 2;
    setup.items[1].msgId = 3;
    setup.items[2].msgId = 7;
    wk->list = KeySystemList_Create(&setup, heapId);
    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, setup.cursor + 18, KEY_SYSTEM_MSG_PRINT);
}

BOOL KeySystemFlow_MenuSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL titleDone = KeySystemMsgWin_IsDone(wk->titleWin);
    BOOL infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    BOOL listDone = KeySystemList_IsPrinted(wk->list);

    if (titleDone && infoDone && listDone) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_MenuSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystem_FreeInfoWin(wk);
    KeySystemList_Free(wk->list);
    wk->list = NULL;
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

void KeySystemFlow_ConnectSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 21, KEY_SYSTEM_MSG_PRINT_WAIT_ICON);
    KeySystemClAct_Create(&wk->clact, 0, heapId);
    KeySystemClAct_Create(&wk->clact, 1, heapId);
    KeySystemClAct_Create(&wk->clact, 2, heapId);
    KeySystemClAct_Create(&wk->clact, 3, heapId);
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_PLANEMASK_OBJ, GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3, 16,
                        6);
    GX_SetVisibleWnd(GX_WNDMASK_W0);
    G2_SetWnd0Position(224, 0, 0, 32);
    G2_SetWnd0InsidePlane(GX_PLANEMASK_ALL, FALSE);
    G2_SetWndOutsidePlane(GX_PLANEMASK_ALL, TRUE);
}

BOOL KeySystemFlow_ConnectSceneMain(void *work) {
    KeySystemWork *wk = work;

    if (KeySystemMsgWin_IsDone(wk->msgWin)) {
        GFL_BGSysQueueScrLoad(0);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_ConnectSceneExit(void *work) {
    KeySystemWork *wk = work;

    if (wk->msgWinGroup != NULL) {
        GFL_BGSysClearScr(6);
        KeySystemMsgWinGroup_Free(wk->msgWinGroup);
        wk->msgWinGroup = NULL;
    }
    G2_BlendNone();
    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    KeySystemClAct_Delete(&wk->clact, 3);
    KeySystemClAct_Delete(&wk->clact, 2);
    KeySystemClAct_Delete(&wk->clact, 1);
    KeySystemClAct_Delete(&wk->clact, 0);
    KeySystemMsgWin_StopWaitIcon(wk->msgWin);
    GFL_BGSysQueueScrLoad(4);
    GFL_BGSysQueueScrLoad(6);
}

void KeySystemFlow_SettingsSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    KeySystemListSetup setup;
    u32 i;

    sys_memset(&setup, 0, sizeof(KeySystemListSetup));
    setup.bg = 1;
    setup.unk04 = 14;
    setup.frameChar = 10;
    setup.palette = 1;
    setup.msgData = wk->msgData;
    setup.font = wk->font;
    setup.count = 4;
    if (KeySystemTags_Has(wk->tags, "KLL")) {
        setup.cursor = KeySystemTags_Get(wk->tags, "KLL");
        KeySystemTags_Remove(wk->tags, "KLL");
    } else {
        setup.cursor = 0;
    }
    for (i = 0; i < setup.count; i++) {
        setup.items[i].width = 26;
        setup.items[i].height = 2;
        setup.items[i].x = 3;
        setup.items[i].y = (setup.items[i].height + 3) * i + 5;
    }
    setup.items[0].msgId = 5;
    setup.items[1].msgId = 6;
    setup.items[2].msgId = 99;
    setup.items[3].msgId = 7;
    wk->list = KeySystemList_Create(&setup, heapId);
    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetSettingsMsg(wk->keyInfo, setup.cursor),
                             KEY_SYSTEM_MSG_PRINT);
}

BOOL KeySystemFlow_SettingsSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    BOOL listDone = KeySystemList_IsPrinted(wk->list);

    if (infoDone && listDone) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_SettingsSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemList_Free(wk->list);
    wk->list = NULL;
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

void KeySystemFlow_DifficultySceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetDifficultyMsg(wk->keyInfo),
                             KEY_SYSTEM_MSG_PRINT);
    KeySystemKeySelect_Init(&wk->keySelect, KEY_LIST_DIFFICULTY, &wk->clact, &wk->list, wk->msgData, wk->font,
                            wk->keyInfo, wk->param->gameData, heapId);
}

BOOL KeySystemFlow_DifficultySceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    BOOL listDone = KeySystemKeySelect_IsPrinted(&wk->keySelect);

    if (infoDone && listDone) {
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_DifficultySceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemKeySelect_Free(&wk->keySelect);
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

void KeySystemFlow_CitySceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetCityMsg(wk->keyInfo), KEY_SYSTEM_MSG_PRINT);
    KeySystemKeySelect_Init(&wk->keySelect, KEY_LIST_CITY, &wk->clact, &wk->list, wk->msgData, wk->font, wk->keyInfo,
                            wk->param->gameData, heapId);
}

BOOL KeySystemFlow_CitySceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    BOOL listDone = KeySystemKeySelect_IsPrinted(&wk->keySelect);

    if (infoDone && listDone) {
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_CitySceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemKeySelect_Free(&wk->keySelect);
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

void KeySystemFlow_KeyAnimSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystemKeyAnim_Init(&wk->keyAnim, &wk->clact, wk->msgWin, wk->font, heapId);
}

BOOL KeySystemFlow_KeyAnimSceneMain(void *work) {
    return TRUE;
}

void KeySystemFlow_KeyAnimSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemKeyAnim_Free(&wk->keyAnim);
}

void KeySystemFlow_SendSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    u32 count = 0;
    int i;

    for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
        if (wk->keys.unlocked[i]) {
            // BUG: The fifth key's name goes past the table, into sKeysHeldTemplates' title
            sSendTemplates[1 + count++].msgId = sSendKeyNameMsgs[VERSION_INDEX][i];
        }
    }
    if (count + 1 > NELEMS(sSendTemplates)) {
        count = NELEMS(sSendTemplates) - 1;
    }
    wk->msgWinGroup = KeySystemMsgWinGroup_Create(sSendTemplates, count + 1, 0, 14, wk->font, wk->msgData, heapId);
    KeySystemBG_LoadScreen(wk->bg, 2, 1);
    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 30, KEY_SYSTEM_MSG_STREAM_FAST);
}

BOOL KeySystemFlow_SendSceneMain(void *work) {
    KeySystemWork *wk = work;

    if (KeySystemMsgWinGroup_IsDone(wk->msgWinGroup)) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(2);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_SendSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemMsgWinGroup_Free(wk->msgWinGroup);
    wk->msgWinGroup = NULL;
    if (wk->msgWin != NULL) {
        KeySystemMsgWin_DrawFrame(wk->msgWin, 1, 15);
        KeySystemMsgWin_StopWaitIcon(wk->msgWin);
    }
    KeySystemBG_LoadScreen(wk->bg, 2, 0);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(2);
}

void KeySystemFlow_KeysHeldSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    u32 count = 0;
    int i;

    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 0, KEY_SYSTEM_MSG_PRINT);
    if (wk->msgWinGroup == NULL) {
        for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
            if (wk->keys.unlocked[i]) {
                sKeysHeldTemplates[1 + count++].msgId = sHeldKeyNameMsgs[VERSION_INDEX][i];
            }
        }
        wk->msgWinGroup =
            KeySystemMsgWinGroup_Create(sKeysHeldTemplates, count + 1, 4, 14, wk->font, wk->msgData, heapId);
    }
    KeySystemBG_LoadScreen(wk->bg, 6, 2);
}

BOOL KeySystemFlow_KeysHeldSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL groupDone = KeySystemMsgWinGroup_IsDone(wk->msgWinGroup);
    BOOL msgDone = KeySystemMsgWin_IsDone(wk->msgWin);

    if (groupDone && msgDone) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(4);
        GFL_BGSysQueueScrLoad(6);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_KeysHeldSceneExit(void *work) {
    GFL_BGSysQueueScrLoad(0);
}

void KeySystemFlow_ChamberSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetChamberMsg(wk->keyInfo), KEY_SYSTEM_MSG_PRINT);
    KeySystemKeySelect_Init(&wk->keySelect, KEY_LIST_CHAMBER, &wk->clact, &wk->list, wk->msgData, wk->font, wk->keyInfo,
                            wk->param->gameData, heapId);
}

BOOL KeySystemFlow_ChamberSceneMain(void *work) {
    KeySystemWork *wk = work;
    BOOL infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    BOOL listDone = KeySystemKeySelect_IsPrinted(&wk->keySelect);

    if (infoDone && listDone) {
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

void KeySystemFlow_ChamberSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemKeySelect_Free(&wk->keySelect);
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}
