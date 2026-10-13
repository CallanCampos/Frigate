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

// The Key System's helpers: the key state, the key lists, the key messages and the key animations, and the scene
// tables of key_system_flow.c. The ROM doesn't name this file

// The frames of the key's moves in and out of view, of the plate's moves down and up and of a setting's reveal and
// hide (key_system_key_frames.c and key_system_plate_frames.c hold the same values as tables nothing reads)
#define KEY_IN_FRAMES 150
#define KEY_OUT_FRAMES 80
#define SHOW_IN_FRAMES 150
#define PLATE_DOWN_FRAMES 40
#define PLATE_UP_FRAMES 70
#define SHOW_OUT_FRAMES 40

static int KeySystemKeySelect_Select(int index, void *arg);
static void KeySystemKeySelect_SetupDifficulty(KeySystemKeySelect *keySelect, KeySystemListSetup *setup);
static int KeySystemKeySelect_SelectDifficulty(int index, KeySystemKeySelect *keySelect);
static void KeySystemKeySelect_SetupCity(KeySystemKeySelect *keySelect, KeySystemListSetup *setup);
static int KeySystemKeySelect_SelectCity(int index, KeySystemKeySelect *keySelect);
static void KeySystemKeySelect_SetupChamber(KeySystemKeySelect *keySelect, KeySystemListSetup *setup);
static int KeySystemKeySelect_SelectChamber(int index, KeySystemKeySelect *keySelect);
static BOOL KeySystemKeyAnim_Unlock(KeySystemKeyAnim *anim);
static BOOL KeySystemKeyAnim_Show(KeySystemKeyAnim *anim);

// The scenes' functions, for key_system_flow.c. MWCC heapsorts a unit's data by size, which for these nine
// equal-size tables gives the ROM's order (menu, connect, settings, difficulty, city, key animation, send,
// keys held, chamber) only from this declaration order
const KeySystemSceneFuncs sKeysHeldSceneFuncs = {
    KeySystemFlow_KeysHeldSceneInit,
    KeySystemFlow_KeysHeldSceneMain,
    KeySystemFlow_KeysHeldSceneExit,
    NULL,
};

const KeySystemSceneFuncs sSendSceneFuncs = {
    KeySystemFlow_SendSceneInit,
    KeySystemFlow_SendSceneMain,
    KeySystemFlow_SendSceneExit,
    NULL,
};

const KeySystemSceneFuncs sConnectSceneFuncs = {
    KeySystemFlow_ConnectSceneInit,
    KeySystemFlow_ConnectSceneMain,
    KeySystemFlow_ConnectSceneExit,
    NULL,
};

const KeySystemSceneFuncs sCitySceneFuncs = {
    KeySystemFlow_CitySceneInit,
    KeySystemFlow_CitySceneMain,
    KeySystemFlow_CitySceneExit,
    NULL,
};

const KeySystemSceneFuncs sDifficultySceneFuncs = {
    KeySystemFlow_DifficultySceneInit,
    KeySystemFlow_DifficultySceneMain,
    KeySystemFlow_DifficultySceneExit,
    NULL,
};

const KeySystemSceneFuncs sSettingsSceneFuncs = {
    KeySystemFlow_SettingsSceneInit,
    KeySystemFlow_SettingsSceneMain,
    KeySystemFlow_SettingsSceneExit,
    NULL,
};

const KeySystemSceneFuncs sKeyAnimSceneFuncs = {
    KeySystemFlow_KeyAnimSceneInit,
    KeySystemFlow_KeyAnimSceneMain,
    KeySystemFlow_KeyAnimSceneExit,
    NULL,
};

const KeySystemSceneFuncs sMenuSceneFuncs = {
    KeySystemFlow_MenuSceneInit,
    KeySystemFlow_MenuSceneMain,
    KeySystemFlow_MenuSceneExit,
    NULL,
};

const KeySystemSceneFuncs sChamberSceneFuncs = {
    KeySystemFlow_ChamberSceneInit,
    KeySystemFlow_ChamberSceneMain,
    KeySystemFlow_ChamberSceneExit,
    NULL,
};

void KeySystemKeyState_Init(KeySystemKeyState *keys, KeyInfoSave *keyInfo) {
    int i;

    sys_memset(keys, 0, sizeof(KeySystemKeyState));
    keys->version = GAME_VERSION;
    for (i = 0; i < KEY_SYSTEM_KEY_COUNT; i++) {
        keys->unlocked[i] = func_020104c4(keyInfo, i);
        keys->enabled[i] = keyEnabler(keyInfo, i);
    }
}

void KeySystemKeyState_Save(KeySystemKeyState *keys, KeyInfoSave *keyInfo) {
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
BOOL KeySystemKeyState_CanExchange(KeySystemKeyState *keys, KeySystemKeyState *partner, BOOL receive) {
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

void KeySystemKeyState_Receive(KeySystemKeyState *keys, KeySystemKeyState *partner) {
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

void KeySystemSaveInfo_Init(KeySystemSaveInfo *info, GameData *gameData) {
    SaveControl *save = GameData_GetSaveControl(gameData);

    sys_memset(info, 0, sizeof(KeySystemSaveInfo));
    info->hasSave = SaveControl_IsDataAlreadyPresent(save);
    if (info->hasSave) {
        sys_memcpy(GetGameDataPlayerInfo(gameData), &info->player, sizeof(PlayerInfo));
    }
}

void KeySystemKeySelect_Init(KeySystemKeySelect *keySelect, u32 mode, KeySystemClAct *clact,
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

void KeySystemKeySelect_Free(KeySystemKeySelect *keySelect) {
    KeySystemList_Free(*keySelect->list);
    *keySelect->list = NULL;
    KeySystemClAct_Delete(keySelect->clact, 6);
    KeySystemClAct_Delete(keySelect->clact, 5);
    KeySystemClAct_Delete(keySelect->clact, 4);
    KeySystemClAct_Delete(keySelect->clact, 7);
}

void KeySystemKeySelect_Update(KeySystemKeySelect *keySelect) {
    KeySystemList_Update(*keySelect->list);
}

BOOL KeySystemKeySelect_IsPrinted(KeySystemKeySelect *keySelect) {
    return KeySystemList_IsPrinted(*keySelect->list);
}

u32 KeySystemKeySelect_GetCursor(KeySystemKeySelect *keySelect) {
    return KeySystemList_GetCursor(*keySelect->list);
}

BOOL KeySystemKeySelect_IsDecided(KeySystemKeySelect *keySelect) {
    return KeySystemList_IsDecided(*keySelect->list);
}

void KeySystemKeySelect_Reset(KeySystemKeySelect *keySelect) {
    keySelect->result = KEY_SELECT_NONE;
    keySelect->index = 0;
    KeySystemList_Reset(*keySelect->list);
}

int KeySystemKeySelect_GetResult(KeySystemKeySelect *keySelect) {
    return keySelect->result;
}

int KeySystemKeySelect_GetIndex(KeySystemKeySelect *keySelect) {
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

u16 KeySystemFlow_GetDifficultyMsg(KeyInfoSave *keyInfo) {
    return GetGameDifficulty(keyInfo) + 22;
}

u16 KeySystemFlow_GetCityMsg(KeyInfoSave *keyInfo) {
#ifdef BLACK2
    return KeyInfo_GetCityKey(keyInfo) + 25;
#else
    return 26 - KeyInfo_GetCityKey(keyInfo);
#endif
}

u16 KeySystemFlow_GetChamberMsg(KeyInfoSave *keyInfo) {
    return func_020105a0(keyInfo) + 94;
}

u16 KeySystemFlow_GetSettingsMsg(KeyInfoSave *keyInfo, u16 item) {
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
BOOL KeySystemFlow_FormatKeyMsg(KeySystemWork *wk, int key, BOOL receive) {
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

void KeySystemKeyAnim_Init(KeySystemKeyAnim *anim, KeySystemClAct *clact, KeySystemMsgWin *msgWin, Font *font,
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

void KeySystemKeyAnim_Free(KeySystemKeyAnim *anim) {
    KeySystemOamText_Free(anim->text);
    KeySystemClAct_Delete(anim->clact, 9);
    KeySystemClAct_Delete(anim->clact, 12);
    KeySystemClAct_Delete(anim->clact, 10);
    KeySystemClAct_Delete(anim->clact, 8);
    GFL_StrBufFree(anim->str);
    BmpOam_Exit(anim->oamSys);
}

void KeySystemKeyAnim_Start(KeySystemKeyAnim *anim, u32 key, MsgData *msgData, u16 msgId, StrBuf *str,
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

BOOL KeySystemKeyAnim_Update(KeySystemKeyAnim *anim) {
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
        KeySystemAccelMove_Init(&anim->move, &sPlateUpStart, &sPlateUpEnd, FX32_CONST(-3), PLATE_UP_FRAMES);
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
        KeySystemTween_Init(&anim->tween, &sKeyInStart, &sKeyInEnd, KEY_IN_FRAMES);
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
        KeySystemTween_Init(&anim->tween, &sKeyOutStart, &sKeyOutEnd, KEY_OUT_FRAMES);
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
        KeySystemTween_Init(&anim->tween, &sPlateDownStart, &sPlateDownEnd, PLATE_DOWN_FRAMES);
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
        KeySystemTween_Init(&anim->tween, &sShowInStart, &sShowInEnd, SHOW_IN_FRAMES);
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
        KeySystemTween_Init(&anim->tween, &sShowOutStart, &sShowOutEnd, SHOW_OUT_FRAMES);
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
void KeySystemFlow_OnNetError(void *work) {
    KeySystemWork *wk = work;

    wk->choice = TRUE;
    wk->noCancel = TRUE;
}
