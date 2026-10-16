#include "types.h"
#include "app/wbt_download.h"
#include "field/event_wbt_wifi.h"
#include "field/field_event.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "system/game_comm.h"
#include "system/game_event.h"
#include "system/game_system.h"

typedef struct {
    GameSystem *gsys;
    Field *field;
    WbtDownloadParam *param;
} EventWbtWifi;

GameEventReturnCode EventWbtWifi_Callback(GameEvent *event, u32 *state, void *data);

// Called through GameEvent_CreateOverlayDelegate, by a script command in overlay 56
GameEvent *EventWbtWifi_Create(GameSystem *gsys, void *args) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, EventWbtWifi_Callback, sizeof(EventWbtWifi));
    EventWbtWifi *wk = GameEvent_GetData(event);
    WbtDownloadParam *param;

    MI_CpuFill8(wk, 0, sizeof(EventWbtWifi));
    wk->gsys = gsys;
    wk->field = field;
    wk->param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(WbtDownloadParam), TRUE, "event_wbt_wifi.c", 105);
    param = wk->param;
    param->gsys = gsys;
    param->gameData = GSYS_GetGameData(gsys);
    if (GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
        GameCommSys_ExitReq(GSYS_GetGameCommSystem(gsys));
    }
    return event;
}

GameEventReturnCode EventWbtWifi_Callback(GameEvent *event, u32 *state, void *data) {
    EventWbtWifi *wk = data;

    switch (*state) {
    case 0:
        // Wait for the comm system to shut down
        if (!GameCommSys_BootCheck(GSYS_GetGameCommSystem(wk->gsys))) {
            *state = 1;
        }
        break;
    case 1:
        GameEvent_ChainNext(event, EventFieldSubprocessTransition_Create(wk->gsys, wk->field, OVERLAY_WBT_DOWNLOAD,
                                                                         &WBT_DOWNLOAD_PROC_FUNCTIONS, wk->param));
        *state = 2;
        break;
    case 2:
        GFL_HeapFree(wk->param);
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
