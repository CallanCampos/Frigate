#include "types.h"
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

// The Key System: exchanging keys with another Black 2 or White 2 over infrared, and the settings that keys unlock,
// the difficulty, the city and the chamber behind the mystery door. The ROM doesn't name this file

// The index of this version in the tables that hold both versions' messages
#define VERSION_INDEX (GAME_VERSION - VERSION_WHITE2)

// The Key System's menu
enum {
    KEY_MENU_EXCHANGE,
    KEY_MENU_SETTINGS,
    KEY_MENU_BACK,
};

// The settings
enum {
    KEY_LIST_DIFFICULTY,
    KEY_LIST_CITY,
    KEY_LIST_CHAMBER,
    KEY_LIST_BACK,
};

// What a key list's select callback returns: the item is chosen already, its key isn't enabled, it was chosen, or
// the player is in the area it changes
enum {
    KEY_SELECT_NONE,
    KEY_SELECT_CURRENT,
    KEY_SELECT_LOCKED,
    KEY_SELECT_DONE,
    KEY_SELECT_IN_AREA,
};

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
static void KeySystemFlow_MenuSceneInit(void *work, HeapID heapId);
static BOOL KeySystemFlow_MenuSceneMain(void *work);
static void KeySystemFlow_MenuSceneExit(void *work);
static void KeySystemFlow_ConnectSceneInit(void *work, HeapID heapId);
static BOOL KeySystemFlow_ConnectSceneMain(void *work);
static void KeySystemFlow_ConnectSceneExit(void *work);
static void KeySystemFlow_SettingsSceneInit(void *work, HeapID heapId);
static BOOL KeySystemFlow_SettingsSceneMain(void *work);
static void KeySystemFlow_SettingsSceneExit(void *work);
static void KeySystemFlow_DifficultySceneInit(void *work, HeapID heapId);
static BOOL KeySystemFlow_DifficultySceneMain(void *work);
static void KeySystemFlow_DifficultySceneExit(void *work);
static void KeySystemFlow_CitySceneInit(void *work, HeapID heapId);
static BOOL KeySystemFlow_CitySceneMain(void *work);
static void KeySystemFlow_CitySceneExit(void *work);
static void KeySystemFlow_KeyAnimSceneInit(void *work, HeapID heapId);
static BOOL KeySystemFlow_KeyAnimSceneMain(void *work);
static void KeySystemFlow_KeyAnimSceneExit(void *work);
static void KeySystemFlow_SendSceneInit(void *work, HeapID heapId);
static BOOL KeySystemFlow_SendSceneMain(void *work);
static void KeySystemFlow_SendSceneExit(void *work);
static void KeySystemFlow_KeysHeldSceneInit(void *work, HeapID heapId);
static BOOL KeySystemFlow_KeysHeldSceneMain(void *work);
static void KeySystemFlow_KeysHeldSceneExit(void *work);
static void KeySystemFlow_ChamberSceneInit(void *work, HeapID heapId);
static BOOL KeySystemFlow_ChamberSceneMain(void *work);
static void KeySystemFlow_ChamberSceneExit(void *work);
static void KeySystemKeyState_Init(KeySystemKeyState *keys, KeyInfoSave *keyInfo);
static void KeySystemKeyState_Save(KeySystemKeyState *keys, KeyInfoSave *keyInfo);
static BOOL KeySystemKeyState_CanExchange(KeySystemKeyState *keys, KeySystemKeyState *partner, BOOL receive);
static void KeySystemKeyState_Receive(KeySystemKeyState *keys, KeySystemKeyState *partner);
static void KeySystemSaveInfo_Init(KeySystemSaveInfo *info, GameData *gameData);
static void KeySystemKeySelect_Init(KeySystemKeySelect *keySelect, u32 mode, KeySystemClAct *clact,
                                    KeySystemList **list, MsgData *msgData, Font *font, KeyInfoSave *keyInfo,
                                    GameData *gameData, HeapID heapId);
static void KeySystemKeySelect_Free(KeySystemKeySelect *keySelect);
static void KeySystemKeySelect_Update(KeySystemKeySelect *keySelect);
static BOOL KeySystemKeySelect_IsPrinted(KeySystemKeySelect *keySelect);
static u32 KeySystemKeySelect_GetCursor(KeySystemKeySelect *keySelect);
static BOOL KeySystemKeySelect_IsDecided(KeySystemKeySelect *keySelect);
static void KeySystemKeySelect_Reset(KeySystemKeySelect *keySelect);
static int KeySystemKeySelect_GetResult(KeySystemKeySelect *keySelect);
static int KeySystemKeySelect_GetIndex(KeySystemKeySelect *keySelect);
static int KeySystemKeySelect_Select(int index, void *arg);
static void KeySystemKeySelect_SetupDifficulty(KeySystemKeySelect *keySelect, KeySystemListSetup *setup);
static int KeySystemKeySelect_SelectDifficulty(int index, KeySystemKeySelect *keySelect);
static void KeySystemKeySelect_SetupCity(KeySystemKeySelect *keySelect, KeySystemListSetup *setup);
static int KeySystemKeySelect_SelectCity(int index, KeySystemKeySelect *keySelect);
static void KeySystemKeySelect_SetupChamber(KeySystemKeySelect *keySelect, KeySystemListSetup *setup);
static int KeySystemKeySelect_SelectChamber(int index, KeySystemKeySelect *keySelect);
static u16 KeySystemFlow_GetDifficultyMsg(KeyInfoSave *keyInfo);
static u16 KeySystemFlow_GetCityMsg(KeyInfoSave *keyInfo);
static u16 KeySystemFlow_GetChamberMsg(KeyInfoSave *keyInfo);
static u16 KeySystemFlow_GetSettingsMsg(KeyInfoSave *keyInfo, u16 item);
static BOOL KeySystemFlow_FormatKeyMsg(KeySystemWork *wk, int key, BOOL receive);
static void KeySystemKeyAnim_Init(KeySystemKeyAnim *anim, KeySystemClAct *clact, KeySystemMsgWin *msgWin, Font *font,
                                  HeapID heapId);
static void KeySystemKeyAnim_Free(KeySystemKeyAnim *anim);
static void KeySystemKeyAnim_Start(KeySystemKeyAnim *anim, u32 key, MsgData *msgData, u16 msgId, StrBuf *str,
                                   BOOL enabled);
static BOOL KeySystemKeyAnim_Update(KeySystemKeyAnim *anim);
static BOOL KeySystemKeyAnim_Unlock(KeySystemKeyAnim *anim);
static BOOL KeySystemKeyAnim_Show(KeySystemKeyAnim *anim);
static void KeySystemFlow_OnNetError(void *work);

// The frames of the key's moves, which the animations read as constants
const u32 data_ov332_021c8e68[3] = { 150, 80, 150 };
const u32 data_ov332_021c8e78[3] = { 40, 70, 40 };

static const ClActorPos sEffectPos = { 184, 114 };
static const u8 sKeyPalettes[KEY_SYSTEM_KEY_COUNT] = { 2, 1, 1, 1, 2 };

// Where the key, the plate and the key of an enabled setting move
static const KeySystemPos sPlateDownEnd = { 128, 208 };
static const KeySystemPos sKeyInEnd = { 184, 104 };
static const KeySystemPos sPlateDownStart = { 128, 114 };
static const KeySystemPos sPlateUpStart = { 128, 208 };
static const KeySystemPos sShowInEnd = { 128, 104 };
static const KeySystemPos sPlateUpEnd = { 128, 114 };
static const KeySystemPos sKeyInStart = { 184, -32 };
static const KeySystemPos sShowOutStart = { 128, 104 };
static const KeySystemPos sShowOutEnd = { 128, 288 };
static const KeySystemPos sKeyOutStart = { 184, 104 };
static const KeySystemPos sKeyOutEnd = { 184, 122 };
static const KeySystemPos sShowInStart = { 128, -32 };

// The names of the keys, for the key animations and the lists of keys held, in each version's order
static const u16 sKeyNameMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { 8, 10, 11, 101, 102 },
    { 8, 10, 12, 101, 102 },
};
static const u16 sReceivedKeyNameMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { 8, 10, 11, 101, 102 },
    { 8, 10, 12, 101, 102 },
};
// What a key that is already enabled says
static const u16 sKeyEnabledMsgs[KEY_SYSTEM_KEY_COUNT] = { 107, 108, 0, 109, 110 };
static const u16 sKeyUnlockedMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { 89, 90, 91, 103, 104 },
    { 89, 90, 92, 103, 104 },
};
static const u16 sSendKeyNameMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { 14, 15, 17, 105, 106 },
    { 14, 15, 16, 105, 106 },
};
static const u16 sHeldKeyNameMsgs[2][KEY_SYSTEM_KEY_COUNT] = {
    { 14, 15, 17, 105, 106 },
    { 14, 15, 16, 105, 106 },
};

static const KeySystemSceneFuncs sMenuSceneFuncs = {
    KeySystemFlow_MenuSceneInit,
    KeySystemFlow_MenuSceneMain,
    KeySystemFlow_MenuSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sConnectSceneFuncs = {
    KeySystemFlow_ConnectSceneInit,
    KeySystemFlow_ConnectSceneMain,
    KeySystemFlow_ConnectSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sSettingsSceneFuncs = {
    KeySystemFlow_SettingsSceneInit,
    KeySystemFlow_SettingsSceneMain,
    KeySystemFlow_SettingsSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sDifficultySceneFuncs = {
    KeySystemFlow_DifficultySceneInit,
    KeySystemFlow_DifficultySceneMain,
    KeySystemFlow_DifficultySceneExit,
    NULL,
};

static const KeySystemSceneFuncs sCitySceneFuncs = {
    KeySystemFlow_CitySceneInit,
    KeySystemFlow_CitySceneMain,
    KeySystemFlow_CitySceneExit,
    NULL,
};

static const KeySystemSceneFuncs sKeyAnimSceneFuncs = {
    KeySystemFlow_KeyAnimSceneInit,
    KeySystemFlow_KeyAnimSceneMain,
    KeySystemFlow_KeyAnimSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sSendSceneFuncs = {
    KeySystemFlow_SendSceneInit,
    KeySystemFlow_SendSceneMain,
    KeySystemFlow_SendSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sKeysHeldSceneFuncs = {
    KeySystemFlow_KeysHeldSceneInit,
    KeySystemFlow_KeysHeldSceneMain,
    KeySystemFlow_KeysHeldSceneExit,
    NULL,
};

static const KeySystemSceneFuncs sChamberSceneFuncs = {
    KeySystemFlow_ChamberSceneInit,
    KeySystemFlow_ChamberSceneMain,
    KeySystemFlow_ChamberSceneExit,
    NULL,
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

static void KeySystemFlow_MenuSceneInit(void *work, HeapID heapId) {
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

static BOOL KeySystemFlow_MenuSceneMain(void *work) {
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

static void KeySystemFlow_MenuSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystem_FreeInfoWin(wk);
    KeySystemList_Free(wk->list);
    wk->list = NULL;
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

static void KeySystemFlow_ConnectSceneInit(void *work, HeapID heapId) {
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

static BOOL KeySystemFlow_ConnectSceneMain(void *work) {
    KeySystemWork *wk = work;

    if (KeySystemMsgWin_IsDone(wk->msgWin)) {
        GFL_BGSysQueueScrLoad(0);
        return TRUE;
    }
    return FALSE;
}

static void KeySystemFlow_ConnectSceneExit(void *work) {
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

static void KeySystemFlow_SettingsSceneInit(void *work, HeapID heapId) {
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

static BOOL KeySystemFlow_SettingsSceneMain(void *work) {
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

static void KeySystemFlow_SettingsSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemList_Free(wk->list);
    wk->list = NULL;
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

static void KeySystemFlow_DifficultySceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetDifficultyMsg(wk->keyInfo),
                             KEY_SYSTEM_MSG_PRINT);
    KeySystemKeySelect_Init(&wk->keySelect, KEY_LIST_DIFFICULTY, &wk->clact, &wk->list, wk->msgData, wk->font,
                            wk->keyInfo, wk->param->gameData, heapId);
}

static BOOL KeySystemFlow_DifficultySceneMain(void *work) {
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

static void KeySystemFlow_DifficultySceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemKeySelect_Free(&wk->keySelect);
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

static void KeySystemFlow_CitySceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetCityMsg(wk->keyInfo), KEY_SYSTEM_MSG_PRINT);
    KeySystemKeySelect_Init(&wk->keySelect, KEY_LIST_CITY, &wk->clact, &wk->list, wk->msgData, wk->font, wk->keyInfo,
                            wk->param->gameData, heapId);
}

static BOOL KeySystemFlow_CitySceneMain(void *work) {
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

static void KeySystemFlow_CitySceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemKeySelect_Free(&wk->keySelect);
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

static void KeySystemFlow_KeyAnimSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystemKeyAnim_Init(&wk->keyAnim, &wk->clact, wk->msgWin, wk->font, heapId);
}

static BOOL KeySystemFlow_KeyAnimSceneMain(void *work) {
    return TRUE;
}

static void KeySystemFlow_KeyAnimSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemKeyAnim_Free(&wk->keyAnim);
}

static void KeySystemFlow_SendSceneInit(void *work, HeapID heapId) {
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

static BOOL KeySystemFlow_SendSceneMain(void *work) {
    KeySystemWork *wk = work;

    if (KeySystemMsgWinGroup_IsDone(wk->msgWinGroup)) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(2);
        return TRUE;
    }
    return FALSE;
}

static void KeySystemFlow_SendSceneExit(void *work) {
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

static void KeySystemFlow_KeysHeldSceneInit(void *work, HeapID heapId) {
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

static BOOL KeySystemFlow_KeysHeldSceneMain(void *work) {
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

static void KeySystemFlow_KeysHeldSceneExit(void *work) {
    GFL_BGSysQueueScrLoad(0);
}

static void KeySystemFlow_ChamberSceneInit(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemFlow_GetChamberMsg(wk->keyInfo), KEY_SYSTEM_MSG_PRINT);
    KeySystemKeySelect_Init(&wk->keySelect, KEY_LIST_CHAMBER, &wk->clact, &wk->list, wk->msgData, wk->font, wk->keyInfo,
                            wk->param->gameData, heapId);
}

static BOOL KeySystemFlow_ChamberSceneMain(void *work) {
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

static void KeySystemFlow_ChamberSceneExit(void *work) {
    KeySystemWork *wk = work;

    KeySystemKeySelect_Free(&wk->keySelect);
    KeySystem_FreeInfoWin(wk);
    GFL_BGSysQueueScrLoad(0);
    GFL_BGSysQueueScrLoad(1);
}

static void KeySystemKeyState_Init(KeySystemKeyState *keys, KeyInfoSave *keyInfo) {
    int i;

    sys_memset(keys, 0, sizeof(KeySystemKeyState));
    keys->version = GAME_VERSION;
    for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
        keys->unlocked[i] = func_020104c4(keyInfo, i);
        keys->enabled[i] = keyEnabler(keyInfo, i);
    }
}

static void KeySystemKeyState_Save(KeySystemKeyState *keys, KeyInfoSave *keyInfo) {
    int i;

    for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
        if (keys->unlocked[i]) {
            func_020104b0(keyInfo, i);
        }
        if (keys->enabled[i]) {
            func_020104e0(keyInfo, i);
        }
    }
}

// Whether a key can be received from the other game, or sent to it. The city key only goes between the two versions
static BOOL KeySystemKeyState_CanExchange(KeySystemKeyState *keys, KeySystemKeyState *partner, BOOL receive) {
    int i;
    int count = 0;

    for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
        if (receive) {
            if (!keys->enabled[i] && partner->unlocked[i] == TRUE) {
                if (i == KEY_SYSTEM_KEY_CITY) {
                    if (keys->version != partner->version) {
                        count++;
                    }
                } else {
                    count++;
                }
            }
        } else {
            if (keys->unlocked[i] == TRUE && !partner->enabled[i]) {
                if (i == KEY_SYSTEM_KEY_CITY) {
                    if (keys->version != partner->version) {
                        count++;
                    }
                } else {
                    count++;
                }
            }
        }
    }
    return count != 0 ? TRUE : FALSE;
}

static void KeySystemKeyState_Receive(KeySystemKeyState *keys, KeySystemKeyState *partner) {
    int i;

    for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
        if (partner->unlocked[i]) {
            if (i == KEY_SYSTEM_KEY_CITY) {
                if (keys->version != partner->version) {
                    keys->enabled[i] = TRUE;
                }
            } else {
                keys->enabled[i] = TRUE;
            }
        }
    }
}

static void KeySystemSaveInfo_Init(KeySystemSaveInfo *info, GameData *gameData) {
    SaveControl *save = GameData_GetSaveControl(gameData);

    sys_memset(info, 0, sizeof(KeySystemSaveInfo));
    info->hasSave = SaveControl_IsDataAlreadyPresent(save);
    if (info->hasSave) {
        sys_memcpy(GetGameDataPlayerInfo(gameData), &info->player, sizeof(PlayerInfo));
    }
}

static void KeySystemKeySelect_Init(KeySystemKeySelect *keySelect, u32 mode, KeySystemClAct *clact,
                                    KeySystemList **list, MsgData *msgData, Font *font, KeyInfoSave *keyInfo,
                                    GameData *gameData, HeapID heapId) {
    KeySystemListSetup setup;
    ClActorPos pos;
    int i;

    sys_memset(keySelect, 0, sizeof(KeySystemKeySelect));
    keySelect->clact = clact;
    keySelect->keyInfo = keyInfo;
    keySelect->list = list;
    keySelect->gameData = gameData;
    switch (mode) {
    case KEY_LIST_DIFFICULTY:
        keySelect->setup = KeySystemKeySelect_SetupDifficulty;
        break;
    case KEY_LIST_CITY:
        keySelect->setup = KeySystemKeySelect_SetupCity;
        break;
    case KEY_LIST_CHAMBER:
        keySelect->setup = KeySystemKeySelect_SetupChamber;
        break;
    }
    // The locks of the items
    for (i = 4; i <= 6; i++) {
        func_0204c124(KeySystemClAct_Create(clact, i, heapId), FALSE);
    }
    // The cursor at the setting
    func_0204c124(KeySystemClAct_Create(clact, 7, heapId), FALSE);
    sys_memset(&setup, 0, sizeof(KeySystemListSetup));
    setup.bg = 1;
    setup.unk04 = 14;
    setup.frameChar = 10;
    setup.palette = 1;
    setup.msgData = msgData;
    setup.font = font;
    setup.cursor = 0;
    setup.arg = keySelect;
    setup.select = KeySystemKeySelect_Select;
    for (i = 0; i < KEY_SYSTEM_LIST_MAX; i++) {
        setup.items[i].width = 15;
        setup.items[i].height = 2;
        setup.items[i].x = 16;
        setup.items[i].y = i * (setup.items[0].height + 2) + 3;
        if (i + 4 <= 6) {
            ClActor *lock = KeySystemClAct_GetActor(clact, i + 4);

            pos.x = (setup.items[i].x + setup.items[i].width - 1) * 8;
            pos.y = (setup.items[i].y + 1) * 8;
            func_0204c140(lock, &pos, CLACT_SURFACE_MAIN);
        }
    }
    keySelect->setup(keySelect, &setup);
    (setup.items + setup.count - 1)->x += 3;
    (setup.items + setup.count - 1)->width -= 3;
    *keySelect->list = KeySystemList_Create(&setup, heapId);
}

static void KeySystemKeySelect_Free(KeySystemKeySelect *keySelect) {
    KeySystemList_Free(*keySelect->list);
    *keySelect->list = NULL;
    KeySystemClAct_Delete(keySelect->clact, 6);
    KeySystemClAct_Delete(keySelect->clact, 5);
    KeySystemClAct_Delete(keySelect->clact, 4);
    KeySystemClAct_Delete(keySelect->clact, 7);
}

static void KeySystemKeySelect_Update(KeySystemKeySelect *keySelect) {
    KeySystemList_Update(*keySelect->list);
}

static BOOL KeySystemKeySelect_IsPrinted(KeySystemKeySelect *keySelect) {
    return KeySystemList_IsPrinted(*keySelect->list);
}

static u32 KeySystemKeySelect_GetCursor(KeySystemKeySelect *keySelect) {
    return KeySystemList_GetCursor(*keySelect->list);
}

static BOOL KeySystemKeySelect_IsDecided(KeySystemKeySelect *keySelect) {
    return KeySystemList_IsDecided(*keySelect->list);
}

static void KeySystemKeySelect_Reset(KeySystemKeySelect *keySelect) {
    keySelect->result = KEY_SELECT_NONE;
    keySelect->index = 0;
    KeySystemList_Reset(*keySelect->list);
}

static int KeySystemKeySelect_GetResult(KeySystemKeySelect *keySelect) {
    return keySelect->result;
}

static int KeySystemKeySelect_GetIndex(KeySystemKeySelect *keySelect) {
    return keySelect->index;
}

static int KeySystemKeySelect_Select(int index, void *arg) {
    KeySystemKeySelect *keySelect = arg;
    int result = keySelect->select(index, keySelect);

    switch (result) {
    case KEY_SELECT_CURRENT:
        return FALSE;
    case KEY_SELECT_LOCKED:
    case KEY_SELECT_IN_AREA:
        keySelect->result = result;
        keySelect->index = index;
        return FALSE;
    case KEY_SELECT_NONE:
    case KEY_SELECT_DONE:
    default:
        return TRUE;
    }
}

// Moves the cursor of the list to the setting
static inline void KeySystemKeySelect_MoveCursor(ClActor *cursor, const KeySystemListSetup *setup, int index) {
    ClActorPos pos;

    pos.x = (setup->items[index].x - 2) * 8;
    pos.y = (setup->items[index].y + 1) * 8;
    func_0204c140(cursor, &pos, CLACT_SURFACE_MAIN);
    func_0204c124(cursor, TRUE);
}

static void KeySystemKeySelect_SetupDifficulty(KeySystemKeySelect *keySelect, KeySystemListSetup *setup) {
    ClActor *cursor;

    setup->count = 4;
    setup->items[0].msgId = 8;
    setup->items[1].msgId = 9;
    setup->items[2].msgId = 10;
    setup->items[3].msgId = 7;
    keySelect->select = KeySystemKeySelect_SelectDifficulty;
    if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_EASY)) {
        func_0204c124(KeySystemClAct_GetActor(keySelect->clact, 4), TRUE);
    }
    if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_CHALLENGE)) {
        func_0204c124(KeySystemClAct_GetActor(keySelect->clact, 6), TRUE);
    }
    cursor = KeySystemClAct_GetActor(keySelect->clact, 7);
    KeySystemKeySelect_MoveCursor(cursor, setup, GetGameDifficulty(keySelect->keyInfo));
}

static int KeySystemKeySelect_SelectDifficulty(int index, KeySystemKeySelect *keySelect) {
    const KeySystemListSetup *setup;

    if (index == GetGameDifficulty(keySelect->keyInfo)) {
        return KEY_SELECT_CURRENT;
    }
    if (index < 3) {
        switch (index) {
        case GAME_DIFFICULTY_EASY:
            if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_EASY)) {
                return KEY_SELECT_LOCKED;
            }
            break;
        case GAME_DIFFICULTY_CHALLENGE:
            if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_CHALLENGE)) {
                return KEY_SELECT_LOCKED;
            }
            break;
        }
        SetGameDifficulty(keySelect->keyInfo, index);
        setup = KeySystemList_GetSetup(*keySelect->list);
        KeySystemKeySelect_MoveCursor(KeySystemClAct_GetActor(keySelect->clact, 7), setup, index);
    }
    return KEY_SELECT_DONE;
}

static void KeySystemKeySelect_SetupCity(KeySystemKeySelect *keySelect, KeySystemListSetup *setup) {
    ClActor *cursor;

    setup->count = 3;
#ifdef BLACK2
    setup->items[0].msgId = 11;
    setup->items[1].msgId = 12;
#else
    setup->items[0].msgId = 12;
    setup->items[1].msgId = 11;
#endif
    setup->items[2].msgId = 7;
    keySelect->select = KeySystemKeySelect_SelectCity;
    if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_CITY)) {
        func_0204c124(KeySystemClAct_GetActor(keySelect->clact, 5), TRUE);
    }
    cursor = KeySystemClAct_GetActor(keySelect->clact, 7);
    KeySystemKeySelect_MoveCursor(cursor, setup, KeyInfo_GetCityKey(keySelect->keyInfo));
}

// The two zones after the city's
static inline BOOL KeySystemFlow_IsCityZone(s16 zoneId) {
    s16 offset = zoneId - 478;

    return (u16)offset <= 1;
}

static int KeySystemKeySelect_SelectCity(int index, KeySystemKeySelect *keySelect) {
    const KeySystemListSetup *setup;
    ZoneSpawnInfo *spawn;
    u32 city = KeyInfo_GetCityKey(keySelect->keyInfo);

    if (city == index) {
        return KEY_SELECT_CURRENT;
    }
    if (index < 2) {
        if (SaveControl_IsDataAlreadyPresent(GameData_GetSaveControl(keySelect->gameData))) {
            spawn = GetGameDataNowSpawnZone(keySelect->gameData);
            // The city, its tower or treehollow, or the zones next to them
            if (IsZoneBlackCityOrWhiteForestLobby(GetZoneParentZone(spawn->zoneId)) ||
                IsZoneBlackTowerOrWhiteTreehollow(spawn->zoneId) || KeySystemFlow_IsCityZone(spawn->zoneId)) {
                return KEY_SELECT_IN_AREA;
            }
        }
        if (index == 1 && !keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_CITY)) {
            return KEY_SELECT_LOCKED;
        }
        func_02010550(keySelect->keyInfo, index);
        setup = KeySystemList_GetSetup(*keySelect->list);
        KeySystemKeySelect_MoveCursor(KeySystemClAct_GetActor(keySelect->clact, 7), setup, index);
    }
    return KEY_SELECT_DONE;
}

static void KeySystemKeySelect_SetupChamber(KeySystemKeySelect *keySelect, KeySystemListSetup *setup) {
    ClActor *cursor;

    setup->count = 4;
    setup->items[0].msgId = 100;
    setup->items[1].msgId = 101;
    setup->items[2].msgId = 102;
    setup->items[3].msgId = 7;
    keySelect->select = KeySystemKeySelect_SelectChamber;
    if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_IRON)) {
        func_0204c124(KeySystemClAct_GetActor(keySelect->clact, 5), TRUE);
    }
    if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_ICEBERG)) {
        func_0204c124(KeySystemClAct_GetActor(keySelect->clact, 6), TRUE);
    }
    cursor = KeySystemClAct_GetActor(keySelect->clact, 7);
    KeySystemKeySelect_MoveCursor(cursor, setup, func_020105a0(keySelect->keyInfo));
}

static int KeySystemKeySelect_SelectChamber(int index, KeySystemKeySelect *keySelect) {
    const KeySystemListSetup *setup;

    if (index == func_020105a0(keySelect->keyInfo)) {
        return KEY_SELECT_CURRENT;
    }
    if (index < 3) {
        switch (index) {
        case 1:
            if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_IRON)) {
                return KEY_SELECT_LOCKED;
            }
            break;
        case 2:
            if (!keyEnabler(keySelect->keyInfo, KEY_SYSTEM_KEY_ICEBERG)) {
                return KEY_SELECT_LOCKED;
            }
            break;
        }
        func_0201058c(keySelect->keyInfo, index);
        setup = KeySystemList_GetSetup(*keySelect->list);
        KeySystemKeySelect_MoveCursor(KeySystemClAct_GetActor(keySelect->clact, 7), setup, index);
    }
    return KEY_SELECT_DONE;
}

static u16 KeySystemFlow_GetDifficultyMsg(KeyInfoSave *keyInfo) {
    return GetGameDifficulty(keyInfo) + 22;
}

static u16 KeySystemFlow_GetCityMsg(KeyInfoSave *keyInfo) {
#ifdef BLACK2
    return KeyInfo_GetCityKey(keyInfo) + 25;
#else
    return 26 - KeyInfo_GetCityKey(keyInfo);
#endif
}

static u16 KeySystemFlow_GetChamberMsg(KeyInfoSave *keyInfo) {
    return func_020105a0(keyInfo) + 94;
}

static u16 KeySystemFlow_GetSettingsMsg(KeyInfoSave *keyInfo, u16 item) {
    switch (item) {
    case KEY_LIST_DIFFICULTY:
        return KeySystemFlow_GetDifficultyMsg(keyInfo);
    case KEY_LIST_CITY:
        return KeySystemFlow_GetCityMsg(keyInfo);
    case KEY_LIST_CHAMBER:
        return KeySystemFlow_GetChamberMsg(keyInfo);
    case KEY_LIST_BACK:
        return 27;
    }
    return 0;
}

// Formats into wk->strBuf what receiving or sending a key says, and returns whether the key goes
static BOOL KeySystemFlow_FormatKeyMsg(KeySystemWork *wk, int key, BOOL receive) {
    BOOL exchange;
    u16 msgId;
    StrBuf *str;

    if (receive) {
        exchange = FALSE;
        if (!wk->keys.enabled[key] && wk->partnerKeys.unlocked[key] == TRUE) {
            exchange = TRUE;
        }
    } else {
        exchange = FALSE;
        if (wk->keys.unlocked[key] == TRUE && !wk->partnerKeys.enabled[key]) {
            exchange = TRUE;
        }
    }
    if (key == KEY_SYSTEM_KEY_CITY && wk->keys.version == wk->partnerKeys.version) {
        return FALSE;
    }
    if (exchange) {
        // With the other player's name or without it
        if (wk->partnerSaveInfo.hasSave) {
            msgId = receive ? 44 : 31;
        } else {
            msgId = receive ? 50 : 37;
        }
        // The city key has a message for each city
        if (key == KEY_SYSTEM_KEY_CITY) {
#ifdef BLACK2
            key = receive ? 2 : 3;
#else
            key = receive ? 3 : 2;
#endif
        } else if (key > KEY_SYSTEM_KEY_CITY) {
            key++;
        }
        msgId += (u16)key;
        str = GFL_MsgDataLoadStrbufNew(wk->msgData, msgId);
        if (wk->partnerSaveInfo.hasSave) {
            copyVarForText(wk->wordSet, 0, &wk->partnerSaveInfo.player);
        }
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->strBuf, str);
        GFL_StrBufFree(str);
        return TRUE;
    }
    return FALSE;
}

static void KeySystemKeyAnim_Init(KeySystemKeyAnim *anim, KeySystemClAct *clact, KeySystemMsgWin *msgWin, Font *font,
                                  HeapID heapId) {
    sys_memset(anim, 0, sizeof(KeySystemKeyAnim));
    anim->clact = clact;
    anim->msgWin = msgWin;
    anim->font = font;
    anim->oamSys = BmpOam_Init(heapId, KeySystemClAct_GetUnit(clact));
    anim->str = GFL_StrBufCreate(281, heapId);
    anim->keyActor = KeySystemClAct_Create(clact, 8, heapId);
    anim->effectActor = KeySystemClAct_Create(clact, 10, heapId);
    anim->lockActor = KeySystemClAct_Create(clact, 12, heapId);
    anim->plateActor = KeySystemClAct_Create(clact, 9, heapId);
    {
        ClActorSetup setup = { 192, 98, 0, 1, 1 };

        anim->text = KeySystemOamText_Create(&setup, 16, 2, KeySystemClAct_GetResource(clact, 0), 3, CLACT_SURFACE_MAIN,
                                             anim->oamSys, heapId);
        BmpOam_ActorSetDrawEnable(KeySystemOamText_GetActor(anim->text), FALSE);
    }
}

static void KeySystemKeyAnim_Free(KeySystemKeyAnim *anim) {
    KeySystemOamText_Free(anim->text);
    KeySystemClAct_Delete(anim->clact, 9);
    KeySystemClAct_Delete(anim->clact, 12);
    KeySystemClAct_Delete(anim->clact, 10);
    KeySystemClAct_Delete(anim->clact, 8);
    GFL_StrBufFree(anim->str);
    BmpOam_Exit(anim->oamSys);
}

static void KeySystemKeyAnim_Start(KeySystemKeyAnim *anim, u32 key, MsgData *msgData, u16 msgId, StrBuf *str,
                                   BOOL enabled) {
    u32 palette;

    anim->frame = 0;
    anim->state = 0;
    anim->timer = 0;
    anim->key = key;
    anim->msgId = msgId;
    anim->active = TRUE;
    anim->msgData = msgData;
    GFL_StrBufCopy(anim->str, str);
    palette = sKeyPalettes[key];
    if (key == KEY_SYSTEM_KEY_CITY) {
#ifdef BLACK2
        palette = 1;
        if (!enabled) {
            palette = 2;
        }
#else
        palette = 2;
        if (!enabled) {
            palette = 1;
        }
#endif
    }
    func_0204c378(anim->keyActor, palette, 1);
    if (enabled) {
        anim->func = KeySystemKeyAnim_Show;
    } else {
        anim->func = KeySystemKeyAnim_Unlock;
    }
}

static BOOL KeySystemKeyAnim_Update(KeySystemKeyAnim *anim) {
    return anim->func(anim);
}

// The key comes down, the plate with its name rises, and the lock opens
static BOOL KeySystemKeyAnim_Unlock(KeySystemKeyAnim *anim) {
    if (!anim->active) {
        return TRUE;
    }
    anim->frame++;
    switch (anim->state) {
    case 0:
        KeySystemOamText_SetColor(anim->text, PRINT_COLOR(1, 2, 0));
        KeySystemOamText_Print(anim->text, anim->msgData, anim->msgId, anim->font);
        anim->state++;
    case 1:
        KeySystemOamText_Update(anim->text);
        if (KeySystemOamText_IsDone(anim->text)) {
            anim->state++;
        }
        break;
    case 2:
        KeySystemAccelMove_Init(&anim->move, &sPlateUpStart, &sPlateUpEnd, FX32_CONST(-3), data_ov332_021c8e78[1]);
        func_0204c124(anim->plateActor, TRUE);
        func_0204c124(anim->lockActor, TRUE);
        func_0204c488(anim->lockActor, 7);
        BmpOam_ActorSetDrawEnable(KeySystemOamText_GetActor(anim->text), TRUE);
        anim->state++;
    case 3: {
        ClActorPos pos;
        ClActorPos actorPos;
        BmpOamActor *actor;

        if (KeySystemAccelMove_Update(&anim->move)) {
            anim->state++;
        }
        KeySystemAccelMove_GetPos(&anim->move, &pos);
        actorPos = pos;
        func_0204c140(anim->plateActor, &actorPos, CLACT_SURFACE_MAIN);
        actorPos = pos;
        actorPos.x += 56;
        func_0204c140(anim->lockActor, &actorPos, CLACT_SURFACE_MAIN);
        actor = KeySystemOamText_GetActor(anim->text);
        actorPos = pos;
        actorPos.x -= 64;
        actorPos.y -= 8;
        BmpOam_ActorSetPos(actor, actorPos.x, actorPos.y);
        break;
    }
    case 4:
        if (anim->timer++ > 60) {
            anim->timer = 0;
            anim->state++;
        }
        break;
    case 5:
        KeySystemTween_Init(&anim->tween, &sKeyInStart, &sKeyInEnd, data_ov332_021c8e68[0]);
        func_0204c124(anim->keyActor, TRUE);
        func_0204c488(anim->keyActor, 8);
        func_0204c520(anim->keyActor, TRUE);
        GFL_SndSEPlay(SEQ_SE_SW_KEYSYS_02);
        anim->state++;
    case 6: {
        ClActorPos pos;

        if (KeySystemTween_Update(&anim->tween) && func_0204c510(anim->keyActor) == 0) {
            func_0204c550(anim->keyActor);
            anim->state++;
        }
        KeySystemTween_GetPos(&anim->tween, &pos);
        func_0204c140(anim->keyActor, &pos, CLACT_SURFACE_MAIN);
        break;
    }
    case 7:
        if (anim->timer++ > 60) {
            anim->timer = 0;
            anim->state++;
        }
        break;
    case 8:
        KeySystemTween_Init(&anim->tween, &sKeyOutStart, &sKeyOutEnd, data_ov332_021c8e68[1]);
        func_0204c488(anim->keyActor, 9);
        anim->state++;
    case 9: {
        ClActorPos pos;

        if (KeySystemTween_Update(&anim->tween) && !func_0204c560(anim->keyActor)) {
            GFL_SndSEPlay(SEQ_SE_SW_KEYSYS_03);
            func_0204c124(anim->keyActor, FALSE);
            anim->state++;
        }
        KeySystemTween_GetPos(&anim->tween, &pos);
        func_0204c140(anim->keyActor, &pos, CLACT_SURFACE_MAIN);
        break;
    }
    case 10:
        func_0204c488(anim->lockActor, 12);
        func_0204c520(anim->lockActor, TRUE);
        anim->state++;
        break;
    case 11:
        if (!func_0204c560(anim->lockActor)) {
            func_0204c124(anim->lockActor, FALSE);
            anim->state++;
        }
        break;
    case 12:
        func_0204c140(anim->effectActor, &sEffectPos, CLACT_SURFACE_MAIN);
        func_0204c124(anim->effectActor, TRUE);
        func_0204c488(anim->effectActor, 11);
        func_0204c520(anim->effectActor, TRUE);
        GFL_SndSEPlay(SEQ_SE_SW_KEYSYS_04);
        anim->state++;
        break;
    case 13:
        if (!func_0204c560(anim->effectActor)) {
            func_0204c124(anim->effectActor, FALSE);
            anim->state++;
        }
        break;
    case 14:
        if (anim->timer++ > 120) {
            anim->timer = 0;
            anim->state++;
        }
        break;
    case 15:
        KeySystemMsgWin_PrintStr(anim->msgWin, anim->str, KEY_SYSTEM_MSG_STREAM_FAST);
        GFL_BGSysQueueScrLoad(0);
        anim->state++;
        break;
    case 16:
        if (KeySystemMsgWin_IsDone(anim->msgWin)) {
            anim->state++;
        }
        break;
    case 17:
        KeySystemTween_Init(&anim->tween, &sPlateDownStart, &sPlateDownEnd, data_ov332_021c8e78[0]);
        anim->state++;
    case 18: {
        ClActorPos pos;
        ClActorPos actorPos;
        BmpOamActor *actor;

        if (KeySystemTween_Update(&anim->tween)) {
            func_0204c124(anim->plateActor, FALSE);
            func_0204c124(anim->lockActor, FALSE);
            BmpOam_ActorSetDrawEnable(KeySystemOamText_GetActor(anim->text), FALSE);
            anim->state++;
        }
        KeySystemTween_GetPos(&anim->tween, &pos);
        actorPos = pos;
        func_0204c140(anim->plateActor, &actorPos, CLACT_SURFACE_MAIN);
        actorPos = pos;
        actorPos.x += 56;
        func_0204c140(anim->lockActor, &actorPos, CLACT_SURFACE_MAIN);
        actor = KeySystemOamText_GetActor(anim->text);
        actorPos = pos;
        actorPos.x -= 64;
        actorPos.y -= 8;
        BmpOam_ActorSetPos(actor, actorPos.x, actorPos.y);
        break;
    }
    case 19:
        anim->active = FALSE;
        return TRUE;
    }
    return FALSE;
}

// The key comes down and goes, for a key whose setting is enabled already
static BOOL KeySystemKeyAnim_Show(KeySystemKeyAnim *anim) {
    if (!anim->active) {
        return TRUE;
    }
    anim->frame++;
    switch (anim->state) {
    case 0:
        KeySystemTween_Init(&anim->tween, &sShowInStart, &sShowInEnd, data_ov332_021c8e68[2]);
        func_0204c124(anim->keyActor, TRUE);
        func_0204c520(anim->keyActor, TRUE);
        GFL_SndSEPlay(SEQ_SE_SW_KEYSYS_02);
        anim->state++;
    case 1: {
        ClActorPos pos;

        if (KeySystemTween_Update(&anim->tween) && func_0204c510(anim->keyActor) == 0) {
            func_0204c550(anim->keyActor);
            anim->state++;
        }
        KeySystemTween_GetPos(&anim->tween, &pos);
        func_0204c140(anim->keyActor, &pos, CLACT_SURFACE_MAIN);
        break;
    }
    case 2:
        if (anim->timer++ > 60) {
            anim->timer = 0;
            anim->state++;
        }
        break;
    case 3:
        KeySystemMsgWin_PrintStr(anim->msgWin, anim->str, KEY_SYSTEM_MSG_STREAM_FAST);
        GFL_BGSysQueueScrLoad(0);
        anim->state++;
        break;
    case 4:
        if (KeySystemMsgWin_IsDone(anim->msgWin)) {
            anim->state++;
        }
        break;
    case 5:
        KeySystemTween_Init(&anim->tween, &sShowOutStart, &sShowOutEnd, data_ov332_021c8e78[2]);
        anim->state++;
    case 6: {
        ClActorPos pos;

        if (KeySystemTween_Update(&anim->tween)) {
            anim->state++;
        }
        KeySystemTween_GetPos(&anim->tween, &pos);
        func_0204c140(anim->keyActor, &pos, CLACT_SURFACE_MAIN);
        break;
    }
    case 7:
        anim->active = FALSE;
        return TRUE;
    }
    return FALSE;
}

// Called when the wireless connection ends: the exchange stops and B can't cancel any more
static void KeySystemFlow_OnNetError(void *work) {
    KeySystemWork *wk = work;

    wk->choice = TRUE;
    wk->noCancel = TRUE;
}
