#include "types.h"
#include "battle/battle_proc.h"
#include "battle/btl_setup.h"
#include "field/event_colosseum_battle.h"
#include "field/event_make.h"
#include "field/event_sound.h"
#include "field/field_event.h"
#include "gfl/heap.h"
#include "gfl/net_handle.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "system/game_event.h"
#include "system/game_system.h"

// No rule picked yet
#define RULE_NONE 41

typedef struct {
    GameSystem *gsys;
    Field *field;
    BtlSetup setup;
    BattlePlayers *players;
} EventColosseumBattleWork;

static GameEventReturnCode EventColosseumBattle_Callback(GameEvent *event, u32 *state, void *data);
static void SetRuleIfNone(u32 *rule, u32 value);

static GameEventReturnCode EventColosseumBattle_Callback(GameEvent *event, u32 *state, void *data) {
    EventColosseumBattleWork *work = data;
    GameSystem *gsys = work->gsys;
    EventMakeArgs args;

    switch (*state) {
    case 0:
        *state = 3;
        break;
    case 3:
        // The battle's music, a u16 at 0x18 of the setup, which battle/btl_setup.h doesn't have yet
        GameEvent_ChainNext(event, EventBattleBGMPlay_Create(gsys, work->setup.fieldSituation.bgm));
        (*state)++;
        break;
    case 4:
        args.setup = &work->setup;
        args.players = work->players;
        args.unk08 = 0;
        GameEvent_ChainNext(event, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(10), eventMakeFunc, &args));
        (*state)++;
        break;
    case 5:
        BtlSetup_FreeContents(&work->setup);
        GameEvent_ChainNext(event, EventBGMFadePop_Create(gsys));
        (*state)++;
        break;
    case 6:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case 7:
        GameEvent_ChainNext(
            event, CallFieldMapEntranceInTransition(gsys, work->field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case 8:
        GameEvent_ChainNext(event, EventBGMFadeWait_Create(gsys));
        (*state)++;
        break;
    case 9:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventColosseumBattle_Create(GameSystem *gsys, Field *field, u32 category, ColosseumBattleParam *param) {
    u32 rule = RULE_NONE;
    GameEvent *event;
    EventColosseumBattleWork *work;
    BtlSetup *setup;
    GameData *gameData;

    event = GameEvent_Create(gsys, NULL, EventColosseumBattle_Callback, sizeof(EventColosseumBattleWork));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = field;
    work->players = &param->players.battle;
    setup = &work->setup;
    switch (category) {
    case 5:
        SetRuleIfNone(&rule, 0);
    case 4:
        SetRuleIfNone(&rule, 2);
    case 7:
        SetRuleIfNone(&rule, 1);
    case 6:
        SetRuleIfNone(&rule, 3);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNet1v1Single(setup, gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
        break;
    case 9:
        SetRuleIfNone(&rule, 4);
    case 8:
        SetRuleIfNone(&rule, 6);
    case 11:
        SetRuleIfNone(&rule, 5);
    case 10:
        SetRuleIfNone(&rule, 7);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNet1v1Double(setup, gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
        break;
    case 13:
        SetRuleIfNone(&rule, 8);
    case 12:
        SetRuleIfNone(&rule, 10);
    case 15:
        SetRuleIfNone(&rule, 9);
    case 14:
        SetRuleIfNone(&rule, 11);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNetTriple(setup, gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
        break;
    case 17:
        SetRuleIfNone(&rule, 12);
    case 16:
        SetRuleIfNone(&rule, 14);
    case 19:
        SetRuleIfNone(&rule, 13);
    case 18:
        SetRuleIfNone(&rule, 15);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNetRotation(setup, gameData, NetHandle_GetOwn(), 1, HEAPID_GAMEEVENT);
        break;
    case 21:
        SetRuleIfNone(&rule, 16);
    case 20:
        SetRuleIfNone(&rule, 18);
    case 23:
        SetRuleIfNone(&rule, 17);
    case 22:
        SetRuleIfNone(&rule, 19);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNetMultiVsNet(setup, gameData, NetHandle_GetOwn(), 1, param->unk12, HEAPID_GAMEEVENT);
        break;
    default:
        GFL_ASSERT_MSG(FALSE, "play_category = %d\n", category);
        return NULL;
    }
    BtlSetup_SetParty(setup, param->party, 0);
    BtlSetup_ApplyRegulation(setup, param->regulation, HEAPID_GAMEEVENT);
    // The battle's music, as above
    setup->fieldSituation.bgm = param->bgm;
    work->players->rule = rule;
    work->players->unk4C = 0;
    BtlSetup_AllocRecBuffer(setup, 1);
    return event;
}

// Sets the rule, unless one is set
static void SetRuleIfNone(u32 *rule, u32 value) {
    if (*rule == RULE_NONE) {
        *rule = value;
    }
}
