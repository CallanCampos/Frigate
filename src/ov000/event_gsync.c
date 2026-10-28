#include "types.h"
#include "app/box2.h"
#include "app/dwc_utility.h"
#include "app/gsync.h"
#include "app/wifi_login.h"
#include "field/event_gsync.h"
#include "field/field_event.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/hw.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/dream_world.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

GameEventReturnCode EventGameSync_Callback(GameEvent *event, u32 *state, void *data);

GameEventReturnCode EventGameSync_Callback(GameEvent *event, u32 *state, void *data) {
    EventGameSync *wk = data;
    GameSystem *gsys = wk->gsys;
    Field *field = GSYS_GetField(gsys);

    switch (*state) {
    case 0:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceOutTransitionDefault(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0));
        (*state)++;
        break;
    case 1:
        GameEvent_ChainNext(event, CreateFieldCloseEvent(gsys, field));
        wk->fromWifiSettings = FALSE;
        *state = 2;
        break;
    case 2:
        MI_CpuFill8(&wk->accountInfo, 0, sizeof(wk->accountInfo));
        wk->hasAccountInfo = FALSE;
        GSYS_QueueProc(gsys, OVERLAY_GSYNC, &GSYNC_MENU_PROC_FUNCTIONS, wk);
        (*state)++;
        break;
    case 3:
        if (!GSYS_GetProcMgrState(gsys)) {
            if (wk->gsyncResult == GSYNC_RESULT_CONNECT) {
                *state = 16;
            } else if (wk->gsyncResult == GSYNC_RESULT_WIFI_SETTINGS) {
                *state = 5;
            } else {
                *state = 10;
            }
        }
        break;
    case 5:
        (*state)++;
        break;
    case 6:
        // Nintendo Wi-Fi Connection settings, which need the sound heap's memory
        wk->bgm = GFL_SndBGMGetID();
        GFL_SndDestroyHeap();
        GSYS_QueueProc(gsys, OVERLAY_DWC_UTILITY, &DWC_UTILITY_PROC_FUNCTIONS, wk);
        (*state)++;
        break;
    case 7:
        if (!GSYS_GetProcMgrState(gsys)) {
            *state = 14;
        }
        break;
    case 14:
        // Back from the Wi-Fi settings
        OS_EnableIrq();
        GFL_SndInit();
        GFL_SndBGMPlay(wk->bgm, SND_CHANNEL_MASK_ALL);
        GFL_SndBGMFadeIn(60);
        *state = 15;
        break;
    case 15:
        wk->fromWifiSettings = TRUE;
        *state = 2;
        break;
    case 10:
        GameEvent_ChainNext(event, EventFieldOpen_CreateHeadless(gsys));
        (*state)++;
        break;
    case 11:
        GameEvent_ChainNext(event,
                            CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_FADE_BLACK, 0, 1, 0, 0));
        (*state)++;
        break;
    case 12:
        if (wk->bgmPushed) {
            GFL_SndBGMPop();
            GFL_SndBGMSetPaused(FALSE);
            GFL_SndBGMFadeIn(60);
            wk->bgmPushed = FALSE;
        }
        (*state)++;
        break;
    case 13:
        return GAMEEVENT_DONE;
    case 16: {
        BOOL asleep =
            DreamWorldSave_IsPokemonAsleep(getDreamWorldStuffAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))));

        if (howManyNormalPokesAreInAllBoxes(GameData_GetBoxSaveAccessor(GSYS_GetGameData(gsys))) == 0 && !asleep) {
            wk->gsyncResult = GSYNC_RESULT_NO_POKEMON;
            *state = 19;
        } else {
            wk->login.unk14 = 0;
            GFL_SndBGMSetPaused(TRUE);
            GFL_SndBGMPush();
            wk->bgmPushed = TRUE;
            (*state)++;
        }
        break;
    }
    case 17:
        wk->login.buffer = wk->loginBuffer;
        GSYS_QueueProc(gsys, OVERLAY_WIFILOGIN, &WIFILOGIN_PROC_FUNCTIONS, &wk->login);
        (*state)++;
        break;
    case 18:
        if (!GSYS_GetProcMgrState(gsys)) {
            if (wk->login.result == 1) {
                *state = 10;
            } else if (!DreamWorldSave_IsPokemonAsleep(getDreamWorldStuffAddress(wk->save))) {
                wk->gsyncResult = GSYNC_RESULT_WIFI_SETTINGS;
                *state = 19;
            } else {
                wk->gsyncResult = GSYNC_RESULT_NONE;
                *state = 19;
            }
        }
        break;
    case 19:
        GSYS_QueueProc(gsys, OVERLAY_GSYNC, &GSYNC_PROC_FUNCTIONS, wk);
        (*state)++;
        break;
    case 20:
        if (!GSYS_GetProcMgrState(gsys)) {
            if (wk->gsyncResult == GSYNC_RESULT_ACCOUNT) {
                wk->fromWifiSettings = FALSE;
                *state = 2;
            } else if (wk->gsyncResult == GSYNC_RESULT_SELECT_POKEMON) {
                *state = 21;
            } else if (wk->gsyncResult == GSYNC_RESULT_NO_POKEMON) {
                *state = 21;
            } else if (wk->gsyncResult == GSYNC_RESULT_RETRY_LOGIN) {
                wk->login.unk14 = 1;
                *state = 17;
            } else {
                *state = 23;
            }
        }
        break;
    case 21:
        // Choose the Pokemon to send to the Dream World
        wk->box.gameData = GSYS_GetGameData(gsys);
        wk->box.boxes = GameData_GetBoxSaveAccessor(wk->box.gameData);
        wk->box.party = GameData_GetParty(wk->box.gameData);
        wk->box.bag = GameData_GetBag(wk->box.gameData);
        wk->box.playerInfo = GetGameDataPlayerInfo(wk->box.gameData);
        wk->box.trainerData = getTrainerDataBlkAddress(GameData_GetSaveControl(wk->box.gameData));
        wk->box.unk20 = wk->sendableSpecies;
        wk->box.isNationalDex = FALSE;
        wk->box.mode = BOX2_MODE_DREAM_WORLD;
        GSYS_QueueProcAsEvent(event, OVERLAY_BOX2, &BOX2_PROC_FUNCTIONS, &wk->box);
        (*state)++;
        break;
    case 22:
        if (GFL_NetErrCheck()) {
            GFL_NetErrMarkShown();
            GFL_NetErrShow(0);
            wk->login.unk14 = 1;
            *state = 17;
        } else {
            wk->boxTray = wk->box.tray;
            wk->boxPosition = wk->box.position;
            if (wk->box.tray == 0xff && wk->box.position == 0xff) {
                *state = 23;
            } else {
                if (wk->gsyncResult == GSYNC_RESULT_NO_POKEMON) {
                    wk->gsyncResult = GSYNC_RESULT_CONNECT;
                } else {
                    wk->gsyncResult = GSYNC_RESULT_ACCOUNT;
                }
                *state = 19;
            }
        }
        break;
    case 23:
        wk->logout.gameData = GSYS_GetGameData(gsys);
        wk->logout.unk4 = 1;
        wk->logout.unk8 = 0;
        GSYS_QueueProc(gsys, OVERLAY_WIFILOGIN, &WIFILOGOUT_PROC_FUNCTIONS, &wk->logout);
        (*state)++;
        break;
    case 24:
        if (!GSYS_GetProcMgrState(gsys)) {
            GSYS_TryBootGameComm(gsys);
            reg_GX_POWCNT |= REG_GX_POWCNT_DSEL_MASK;
            wk->fromWifiSettings = FALSE;
            *state = 2;
            if (wk->bgmPushed) {
                GFL_SndBGMPop();
                GFL_SndBGMSetPaused(FALSE);
                GFL_SndBGMFadeIn(60);
                wk->bgmPushed = FALSE;
            }
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

// Called through GameEvent_CreateOverlayDelegate, by the C-Gear's Game Sync button in overlay 79
GameEvent *EventGameSync_CreateFromArgs(GameSystem *gsys, void *args) {
    return EventGameSync_Create(gsys);
}

GameEvent *EventGameSync_Create(GameSystem *gsys) {
    GameEvent *event = GameEvent_Create(gsys, NULL, EventGameSync_Callback, sizeof(EventGameSync));
    EventGameSync *wk = GameEvent_GetData(event);

    wk->gameData = GSYS_GetGameData(gsys);
    wk->save = GameData_GetSaveControl(wk->gameData);
    wk->gsys = gsys;
    wk->login.gameData = GSYS_GetGameData(gsys);
    wk->login.unk4 = 1;
    wk->login.unk8 = 0;
    wk->login.unkC = 0x31;
    return event;
}
