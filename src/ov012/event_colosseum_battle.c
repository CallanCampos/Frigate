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

static GameEventReturnCode func_ov012_0215264c(GameEvent *event, u32 *state, void *data);
static void func_ov012_02152910(u32 *rule, u32 value);

static GameEventReturnCode func_ov012_0215264c(GameEvent *event, u32 *state, void *data) {
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
        func_02017cac(&work->setup);
        GameEvent_ChainNext(event, EventBGMFadePop_Create(gsys));
        (*state)++;
        break;
    case 6:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case 7:
        GameEvent_ChainNext(event, CallFieldMapEntranceInTransition(gsys, work->field, 0, 0, 1, 0, 0));
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

GameEvent *func_ov012_02152704(GameSystem *gsys, Field *field, u32 category, ColosseumBattleParam *param) {
    u32 rule = RULE_NONE;
    GameEvent *event;
    EventColosseumBattleWork *work;
    BtlSetup *setup;
    GameData *gameData;

    event = GameEvent_Create(gsys, NULL, func_ov012_0215264c, sizeof(EventColosseumBattleWork));
    work = GameEvent_GetData(event);
    work->gsys = gsys;
    work->field = field;
    work->players = &param->players.battle;
    setup = &work->setup;
    switch (category) {
    case 5:
        func_ov012_02152910(&rule, 0);
    case 4:
        func_ov012_02152910(&rule, 2);
    case 7:
        func_ov012_02152910(&rule, 1);
    case 6:
        func_ov012_02152910(&rule, 3);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNet1v1Single(setup, gameData, func_02040440(), 1, HEAPID_GAMEEVENT);
        break;
    case 9:
        func_ov012_02152910(&rule, 4);
    case 8:
        func_ov012_02152910(&rule, 6);
    case 11:
        func_ov012_02152910(&rule, 5);
    case 10:
        func_ov012_02152910(&rule, 7);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNet1v1Double(setup, gameData, func_02040440(), 1, HEAPID_GAMEEVENT);
        break;
    case 13:
        func_ov012_02152910(&rule, 8);
    case 12:
        func_ov012_02152910(&rule, 10);
    case 15:
        func_ov012_02152910(&rule, 9);
    case 14:
        func_ov012_02152910(&rule, 11);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNetTriple(setup, gameData, func_02040440(), 1, HEAPID_GAMEEVENT);
        break;
    case 17:
        func_ov012_02152910(&rule, 12);
    case 16:
        func_ov012_02152910(&rule, 14);
    case 19:
        func_ov012_02152910(&rule, 13);
    case 18:
        func_ov012_02152910(&rule, 15);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNetRotation(setup, gameData, func_02040440(), 1, HEAPID_GAMEEVENT);
        break;
    case 21:
        func_ov012_02152910(&rule, 16);
    case 20:
        func_ov012_02152910(&rule, 18);
    case 23:
        func_ov012_02152910(&rule, 17);
    case 22:
        func_ov012_02152910(&rule, 19);
        gameData = GSYS_GetGameData(gsys);
        BtlSetup_SetNetMultiVsNet(setup, gameData, func_02040440(), 1, param->unk12, HEAPID_GAMEEVENT);
        break;
    default:
        GFL_ASSERT_MSG(FALSE, "play_category = %d\n", category);
        return NULL;
    }
    func_02017cfc(setup, param->party, 0);
    func_02017d30(setup, param->regulation, HEAPID_GAMEEVENT);
    // The battle's music, as above
    setup->fieldSituation.bgm = param->bgm;
    work->players->rule = rule;
    work->players->unk4C = 0;
    func_020186b0(setup, 1);
    return event;
}

// Sets the rule, unless one is set
static void func_ov012_02152910(u32 *rule, u32 value) {
    if (*rule == RULE_NONE) {
        *rule = value;
    }
}
