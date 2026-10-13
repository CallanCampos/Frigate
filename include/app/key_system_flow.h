#ifndef APP_KEY_SYSTEM_FLOW_H
#define APP_KEY_SYSTEM_FLOW_H

#include "types.h"
#include "app/unova_link.h"
#include "gfl/clact.h"
#include "save/key_info.h"
#include "system/game_data.h"

// The Key System's flow, split as the ROM links it: three data-only units (key_system_key_frames.c,
// key_system_plate_frames.c, key_system_anim_data.c), the sequences and scenes (key_system_flow.c) and the helpers
// with the scene tables (key_system_flow_sub.c). The names are ours; the ROM names none of them.
// Exchanging keys with another Black 2 or White 2 over infrared, and the settings that keys unlock,
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

// key_system_key_frames.c: the frames of the key's moves, which the code has as constants
extern const u32 data_ov332_021c8e68[3];

// key_system_plate_frames.c
extern const ClActorPos sEffectPos;
extern const u32 data_ov332_021c8e78[3];

// key_system_anim_data.c
extern const u8 sKeyPalettes[KEY_SYSTEM_KEY_COUNT];
extern const KeySystemPos sPlateDownEnd;
extern const KeySystemPos sKeyInEnd;
extern const KeySystemPos sPlateDownStart;
extern const KeySystemPos sPlateUpStart;
extern const KeySystemPos sShowInEnd;
extern const KeySystemPos sPlateUpEnd;
extern const KeySystemPos sKeyInStart;
extern const KeySystemPos sShowOutStart;
extern const KeySystemPos sShowOutEnd;
extern const KeySystemPos sKeyOutStart;
extern const KeySystemPos sKeyOutEnd;
extern const KeySystemPos sShowInStart;
extern const u16 sKeyNameMsgs[2][KEY_SYSTEM_KEY_COUNT];
extern const u16 sReceivedKeyNameMsgs[2][KEY_SYSTEM_KEY_COUNT];

// key_system_flow_sub.c: the scenes' functions
extern const KeySystemSceneFuncs sMenuSceneFuncs;
extern const KeySystemSceneFuncs sConnectSceneFuncs;
extern const KeySystemSceneFuncs sSettingsSceneFuncs;
extern const KeySystemSceneFuncs sDifficultySceneFuncs;
extern const KeySystemSceneFuncs sCitySceneFuncs;
extern const KeySystemSceneFuncs sKeyAnimSceneFuncs;
extern const KeySystemSceneFuncs sSendSceneFuncs;
extern const KeySystemSceneFuncs sKeysHeldSceneFuncs;
extern const KeySystemSceneFuncs sChamberSceneFuncs;

// key_system_flow.c: the scenes, which the tables point to
void KeySystemFlow_MenuSceneInit(void *work, HeapID heapId);
BOOL KeySystemFlow_MenuSceneMain(void *work);
void KeySystemFlow_MenuSceneExit(void *work);
void KeySystemFlow_ConnectSceneInit(void *work, HeapID heapId);
BOOL KeySystemFlow_ConnectSceneMain(void *work);
void KeySystemFlow_ConnectSceneExit(void *work);
void KeySystemFlow_SettingsSceneInit(void *work, HeapID heapId);
BOOL KeySystemFlow_SettingsSceneMain(void *work);
void KeySystemFlow_SettingsSceneExit(void *work);
void KeySystemFlow_DifficultySceneInit(void *work, HeapID heapId);
BOOL KeySystemFlow_DifficultySceneMain(void *work);
void KeySystemFlow_DifficultySceneExit(void *work);
void KeySystemFlow_CitySceneInit(void *work, HeapID heapId);
BOOL KeySystemFlow_CitySceneMain(void *work);
void KeySystemFlow_CitySceneExit(void *work);
void KeySystemFlow_KeyAnimSceneInit(void *work, HeapID heapId);
BOOL KeySystemFlow_KeyAnimSceneMain(void *work);
void KeySystemFlow_KeyAnimSceneExit(void *work);
void KeySystemFlow_SendSceneInit(void *work, HeapID heapId);
BOOL KeySystemFlow_SendSceneMain(void *work);
void KeySystemFlow_SendSceneExit(void *work);
void KeySystemFlow_KeysHeldSceneInit(void *work, HeapID heapId);
BOOL KeySystemFlow_KeysHeldSceneMain(void *work);
void KeySystemFlow_KeysHeldSceneExit(void *work);
void KeySystemFlow_ChamberSceneInit(void *work, HeapID heapId);
BOOL KeySystemFlow_ChamberSceneMain(void *work);
void KeySystemFlow_ChamberSceneExit(void *work);

// key_system_flow_sub.c
void KeySystemKeyState_Init(KeySystemKeyState *keys, KeyInfoSave *keyInfo);
void KeySystemKeyState_Save(KeySystemKeyState *keys, KeyInfoSave *keyInfo);
BOOL KeySystemKeyState_CanExchange(KeySystemKeyState *keys, KeySystemKeyState *partner, BOOL receive);
void KeySystemKeyState_Receive(KeySystemKeyState *keys, KeySystemKeyState *partner);
void KeySystemSaveInfo_Init(KeySystemSaveInfo *info, GameData *gameData);
void KeySystemKeySelect_Init(KeySystemKeySelect *keySelect, u32 mode, KeySystemClAct *clact,
                                    KeySystemList **list, MsgData *msgData, Font *font, KeyInfoSave *keyInfo,
                                    GameData *gameData, HeapID heapId);
void KeySystemKeySelect_Free(KeySystemKeySelect *keySelect);
void KeySystemKeySelect_Update(KeySystemKeySelect *keySelect);
BOOL KeySystemKeySelect_IsPrinted(KeySystemKeySelect *keySelect);
u32 KeySystemKeySelect_GetCursor(KeySystemKeySelect *keySelect);
BOOL KeySystemKeySelect_IsDecided(KeySystemKeySelect *keySelect);
void KeySystemKeySelect_Reset(KeySystemKeySelect *keySelect);
int KeySystemKeySelect_GetResult(KeySystemKeySelect *keySelect);
int KeySystemKeySelect_GetIndex(KeySystemKeySelect *keySelect);
u16 KeySystemFlow_GetDifficultyMsg(KeyInfoSave *keyInfo);
u16 KeySystemFlow_GetCityMsg(KeyInfoSave *keyInfo);
u16 KeySystemFlow_GetChamberMsg(KeyInfoSave *keyInfo);
u16 KeySystemFlow_GetSettingsMsg(KeyInfoSave *keyInfo, u16 item);
BOOL KeySystemFlow_FormatKeyMsg(KeySystemWork *wk, int key, BOOL receive);
void KeySystemKeyAnim_Init(KeySystemKeyAnim *anim, KeySystemClAct *clact, KeySystemMsgWin *msgWin, Font *font,
                                  HeapID heapId);
void KeySystemKeyAnim_Free(KeySystemKeyAnim *anim);
void KeySystemKeyAnim_Start(KeySystemKeyAnim *anim, u32 key, MsgData *msgData, u16 msgId, StrBuf *str,
                                   BOOL enabled);
BOOL KeySystemKeyAnim_Update(KeySystemKeyAnim *anim);
void KeySystemFlow_OnNetError(void *work);

#endif
