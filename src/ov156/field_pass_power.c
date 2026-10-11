#include "types.h"
#include "constants/arc.h"
#include "field/field_pass_power.h"
#include "constants/items.h"
#include "constants/script_text_banks.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_script.h"
#include "field/field_script_event.h"
#include "field/game_beacon_set.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/high_link.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/str_tool.h"
#include "system/wordset.h"
#include "text/script/global_10435.h"

// The pass power whose script gets 16 in place of its effect
#define PASS_POWER_027 0x27
// The effects of the pass powers that act at once
#define PASS_POWER_EFFECT_HP_RESTORE 5
#define PASS_POWER_EFFECT_PP_RESTORE 6
// The pass powers in use have one of these effects each
#define PASS_POWER_EFFECT_COUNT 16

typedef struct {
    u32 unk00;
    GameSystem *gsys;
    GameData *gameData;
} PassPowerFlashWork;

typedef struct {
    u16 unk00;
    HeapID heapId;
    HeapID tailHeapId;
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    void *msgBGSys;
    void *listWindow;
    MMSys *actorSystem;
    void *msgWindow;
    u32 unk24;
    MsgData *msgData;
    WordSet *wordSet;
    void *passPowerData;
    StrBuf *strbuf;
    StrBuf *formatted;
} PassPowerListWork;

static void PassPowerList_Init(PassPowerListWork *work, GameSystem *gsys, Field *field);
static void PassPowerList_Exit(PassPowerListWork *work);
static GameEventReturnCode EventPassPowerFlash_Callback(GameEvent *event, u32 *state, void *data);
static void PassPower_ApplyInstant(GameSystem *gsys, WordSet *wordSet, u32 effect);
static GameEventReturnCode EventPassPowerList_Callback(GameEvent *event, u32 *state, void *data);
static void PassPowerList_PrintRow(PassPowerListWork *work, StrBuf *format, u8 passPower, u16 seconds, u8 row);
static void PassPowerList_Print(PassPowerListWork *work);

GameEvent *EventPassPowerActivate_Create(GameSystem *gsys, void *args) {
    u32 *params = args;
    GameRecords *records;
    GameEvent *event;
    ScriptWork *work;
    GameData *gameData;
    void *data;
    u32 passPower;
    u32 effect;

    gameData = GSYS_GetGameData(gsys);
    data = PassPowerData_Create(HEAPID_TAIL(HEAPID_FIELDMAP));
    passPower = params[0];
    effect = 0;
    event = EventScriptCall_Create(gsys, 0x28c3, NULL, HEAPID_FIELDMAP);
    work = EventScriptCall_GetWork(event);
    if (!PassPower_IsIDValid(passPower)) {
        passPower = 0;
    }
    records = getTrainerCardInfoBlkAddress(GameData_GetSaveControl(gameData));
    effect = PassPowerData_GetEffect(data, passPower);
    PassPower_Activate(passPower, data, params[1]);
    RecordAdd(records, 45, 1);
    if (passPower == PASS_POWER_027) {
        ScriptWork_SetParams(work, passPower, 16, params[1], 0);
    } else {
        // The script's parameters are u16s; the prototype's u32 needs the cast to narrow it as the original does
        ScriptWork_SetParams(work, passPower, (u16)effect, params[1], 0);
    }
    PassPower_ApplyInstant(gsys, ScriptWork_GetWordSet(work), effect);
    if (params[1]) {
        u16 price = PassPowerData_GetPrice(data, passPower);
        BagSave_SubItem(GameData_GetBag(gameData), ITEM_PASS_ORB, price, HEAPID_TAIL(HEAPID_FIELDMAP));
        func_ov012_0215fcd0(passPower);
    } else {
        func_ov012_02160340(passPower);
    }
    PassPowerData_Free(data);
    return event;
}

GameEvent *EventPassPowerDepleted_Create(GameSystem *gsys, void *args) {
    return EventScriptCall_Create(gsys, 0x28c4, NULL, HEAPID_FIELDMAP);
}

GameEvent *EventPassPowerFlash_Create(GameSystem *gsys) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventPassPowerFlash_Callback, sizeof(PassPowerFlashWork));
    PassPowerFlashWork *work = GameEvent_GetData(event);

    work->gameData = GSYS_GetGameData(gsys);
    work->gsys = gsys;
    return event;
}

GameEvent *EventPassPowerList_Create(GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventPassPowerList_Callback, sizeof(PassPowerListWork));

    PassPowerList_Init(GameEvent_GetData(event), gsys, field);
    return event;
}

static void PassPowerList_Init(PassPowerListWork *work, GameSystem *gsys, Field *field) {
    work->heapId = HEAPID_FIELDMAP;
    work->tailHeapId = HEAPID_TAIL(work->heapId);
    work->field = field;
    work->gsys = gsys;
    work->gameData = GSYS_GetGameData(gsys);
    work->msgBGSys = Field_GetMsgBGSys(work->field);
    work->actorSystem = Field_GetActorSystem(work->field);
    work->passPowerData = PassPowerData_Create(work->heapId);
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10435, work->heapId);
    work->wordSet = GFL_WordSetSystemCreateDefault(work->heapId);
    work->strbuf = GFL_StrBufCreate(129, work->heapId);
    work->formatted = GFL_StrBufCreate(129, work->heapId);
    DisableAllActorsMovement(work->actorSystem);
}

static void PassPowerList_Exit(PassPowerListWork *work) {
    GFL_StrBufFree(work->formatted);
    GFL_StrBufFree(work->strbuf);
    GFL_WordSetSystemFree(work->wordSet);
    GFL_MsgDataFree(work->msgData);
    PassPowerData_Free(work->passPowerData);
    EnableAllActorsMovement(work->actorSystem);
}

static GameEventReturnCode EventPassPowerFlash_Callback(GameEvent *event, u32 *state, void *data) {
    switch (*state) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_WHITE, 10, 16, 4);
        GFL_SndSEPlay(SEQ_SE_W295_01);
        (*state)++;
        break;
    case 1:
        if (GFL_FadeIsRunning()) {
            break;
        }
        GFL_FadeSet(FADE_ENGINE_A_WHITE, 16, 0, 8);
        (*state)++;
        // fallthrough
    case 2:
        if (GFL_FadeIsRunning()) {
            break;
        }
        // fallthrough
    default:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Puts the first Pokémon able to battle in the word set, and applies the pass powers that act at once
static void PassPower_ApplyInstant(GameSystem *gsys, WordSet *wordSet, u32 effect) {
    PokeParty *party = GameData_GetParty(GSYS_GetGameData(gsys));
    u8 slot = PokeParty_GetFirstBattleReady(party);

    loadPokemonNicknameToStrbuf(wordSet, 1, PokeParty_GetPkm(party, slot));
    switch (effect) {
    case PASS_POWER_EFFECT_HP_RESTORE:
        PassPower_ApplyHPRestore(party);
        break;
    case PASS_POWER_EFFECT_PP_RESTORE:
        PassPower_ApplyPPRestore(party);
        break;
    }
}

static GameEventReturnCode EventPassPowerList_Callback(GameEvent *event, u32 *state, void *data) {
    PassPowerListWork *work = data;

    switch (*state) {
    case 0:
        work->msgWindow = func_ov036_02188498(work->msgBGSys, work->msgData, 19);
        func_ov036_02188538(work->msgWindow, 0, 0, 3);
        (*state)++;
        break;
    case 1:
        if (func_ov036_021885bc(work->msgWindow)) {
            func_ov036_02188504(work->msgWindow);
            (*state)++;
        }
        break;
    case 2:
        work->listWindow = func_ov036_02187ca0(work->msgBGSys, work->msgData, 4, 1, 24, 22);
        func_ov036_02187d38(work->listWindow);
        PassPowerList_Print(work);
        (*state)++;
        break;
    case 3:
        if (GCTX_HIDGetPressedKeys() & (PAD_BUTTON_A | PAD_BUTTON_B)) {
            (*state)++;
        }
        break;
    default:
        func_ov036_02187d38(work->listWindow);
        func_ov036_02187d10(work->listWindow);
        PassPowerList_Exit(work);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Prints a pass power's name and the minutes and seconds it has left on a row of the list
static void PassPowerList_PrintRow(PassPowerListWork *work, StrBuf *format, u8 passPower, u16 seconds, u8 row) {
    loadPassPowerToStrbuf(work->wordSet, 0, passPower);
    WordSetNumber(work->wordSet, 1, seconds / 60, 2, NUM_PAD_SPACE, TRUE);
    WordSetNumber(work->wordSet, 2, seconds % 60, 2, NUM_PAD_ZERO, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->formatted, format);
    func_ov036_02187d28(work->listWindow, 0, row * 16, work->formatted);
}

// Prints the pass powers in use, the one with the least time left first
static void PassPowerList_Print(PassPowerListWork *work) {
    u8 passPowers[PASS_POWER_EFFECT_COUNT];
    u8 order[PASS_POWER_EFFECT_COUNT];
    u16 seconds[PASS_POWER_EFFECT_COUNT];
    u8 count;
    u8 passPower;
    u8 effect;
    u8 i, j;
    u8 tmp;

    // The High Link save is fetched and not used
    getHighLinkBlockAddress(GameData_GetSaveControl(work->gameData));
    GFL_MsgDataLoadStrbuf(work->msgData, Global10435_Text_ActivePassPower, work->strbuf);
    func_ov036_02187d28(work->listWindow, 0, 0, work->strbuf);
    GFL_MsgDataLoadStrbuf(work->msgData, Global10435_Text_Empty_3, work->strbuf);

    for (effect = 0, count = 0; effect < PASS_POWER_EFFECT_COUNT; effect++) {
        order[effect] = effect;
        passPower = PassPower_GetUsedIDByEffect(effect);
        if (passPower != HIGH_LINK_POWER_NONE) {
            seconds[count] = PassPower_GetRemainingSeconds(effect);
            passPowers[count] = passPower;
            count++;
        }
    }

    for (i = 0; i < count - 1; i++) {
        for (j = count - 1; j > i; j--) {
            if (seconds[order[j - 1]] > seconds[order[j]]) {
                tmp = order[j];
                order[j] = order[j - 1];
                order[j - 1] = tmp;
            }
        }
    }

    for (i = 0; i < count; i++) {
        PassPowerList_PrintRow(work, work->strbuf, passPowers[order[i]], seconds[order[i]], i + 1);
    }
}
