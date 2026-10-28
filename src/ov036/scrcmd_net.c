// The script commands of the network: the game communication's status, the Wi-Fi friend list's state, and starting
// the Wi-Fi Club, the GTS and GTS negotiations from their overlays. The ROM has no name for the file; scrcmd_net.c
// is descriptive. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except our
// descriptive ones of commands 0x15B and 0x15C.
#include "types.h"
#include "field/event_gtsnego.h"
#include "field/event_wificlub.h"
#include "field/event_worldtrade.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/scrcmd_net.h"
#include "gfl/overlay.h"
#include "save/wifi_list.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

BOOL Scrcmd_Net_GameCommGetStatus(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 *result = ScriptReadVar(vm, env);

    *result = Field_GetCommKind(GSYS_GetGameCommSystem(gsys));
    return TRUE;
}

BOOL Scrcmd_Net_WifiListHasValidProfile(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);

    *result = WifiList_HasValidProfile(GameData_GetWifiList(FieldScriptEnv_GetGameData(env)));
    return FALSE;
}

BOOL Scrcmd_Net_WifiListCountFriends(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);

    *result = WifiList_CountFriends(GameData_GetWifiList(FieldScriptEnv_GetGameData(env)));
    return FALSE;
}

BOOL Scrcmd_Net_NetConnectWiFiClub(VM *vm, FieldScriptEnv *env) {
    EventWifiClubArgs args;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    args.field = GSYS_GetField(gsys);
    args.useTransitions = FALSE;
    ScriptWork_CallEvent(work,
                         GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(8), EventWifiClub_CreateFromArgs, &args));
    return TRUE;
}

BOOL Scrcmd_Net_NetConnectGTS(VM *vm, FieldScriptEnv *env) {
    EventWorldTradeArgs args;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    args.field = GSYS_GetField(gsys);
    args.unused = 0;
    ScriptWork_CallEvent(work,
                         GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(4), EventWorldTrade_CreateFromArgs, &args));
    return TRUE;
}

BOOL Scrcmd_Net_NetConnectGTSNegotiation(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    ScriptWork_CallEvent(
        work, GameEvent_CreateOverlayDelegate(gsys, OVERLAY_ID(1), EventGtsNego_CreateFromArgs, GSYS_GetField(gsys)));
    return TRUE;
}
