#include "types.h"
#include "app/battle_video.h"
#include "field/event_battle_video.h"
#include "field/field_event.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "system/game_comm.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The steps of the event
enum {
    STEP_WAIT_COMM_EXIT,
    STEP_CLOSE_FIELD,
    STEP_START_BATTLE_VIDEO,
    STEP_WAIT_BATTLE_VIDEO,
    STEP_EMPTY,
    STEP_OPEN_FIELD,
    STEP_END,
};

typedef struct {
    u32 bgm;
    GameSystem *gsys;
    Field *field;
    BattleVideoParam param;
    u32 mode;
    u32 unk18;
} EventBattleVideo;

GameEventReturnCode EventBattleVideo_Callback(GameEvent *event, u32 *state, void *data);

GameEventReturnCode EventBattleVideo_Callback(GameEvent *event, u32 *state, void *data) {
    EventBattleVideo *wk = data;
    GameSystem *gsys = wk->gsys;

    switch (*state) {
    case STEP_WAIT_COMM_EXIT:
        // Wait for the comm system to shut down
        if (!GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
            wk->bgm = GFL_SndBGMGetID();
            GFL_SndBGMFadeOut(6);
            (*state)++;
        }
        break;
    case STEP_CLOSE_FIELD:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, wk->field));
        *state = STEP_START_BATTLE_VIDEO;
        break;
    case STEP_START_BATTLE_VIDEO:
        wk->param.gameData = GSYS_GetGameData(gsys);
        wk->param.mode = wk->mode;
        GSYS_QueueProc(gsys, OVERLAY_NONE, &BATTLE_VIDEO_PROC_FUNCTIONS, &wk->param);
        (*state)++;
        break;
    case STEP_WAIT_BATTLE_VIDEO:
        if (!GSYS_GetProcMgrState(gsys)) {
            (*state)++;
        }
        break;
    case STEP_EMPTY:
        (*state)++;
        break;
    case STEP_OPEN_FIELD:
        GFL_SndBGMPlay(wk->bgm, SND_CHANNEL_MASK_ALL);
        GFL_SndBGMFadeIn(60);
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case STEP_END:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *EventBattleVideo_Create(GameSystem *gsys, Field *field, u32 mode) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventBattleVideo_Callback, sizeof(EventBattleVideo));
    EventBattleVideo *wk;

    if (GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
        GameCommSys_ExitReq(GSYS_GetGameCommSystem(gsys));
    }

    wk = GameEvent_GetData(event);
    wk->gsys = gsys;
    wk->field = field;
    wk->mode = mode;
    return event;
}

// Called through GameEvent_CreateOverlayDelegate, by the NetConnectBattleVideo script command
GameEvent *EventBattleVideo_CreateFromArgs(GameSystem *gsys, void *data) {
    EventBattleVideoArgs *args = data;

    return EventBattleVideo_Create(gsys, args->field, args->mode);
}
