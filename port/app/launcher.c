// Frigate's test launcher (--launch, ADR 0012): opens the field menu's apps (the party, the Pokédex, the bag, the
// trainer card, the summary) after CONTINUE, in place of the field, while the field's process table (overlay 36's
// FIELDMAP_PROC_FUNCS) has no C. It is a test tool: off without the option, so that a normal run still stops on the
// field's trap, and it is to be deleted once overlay 36 lands (ADR 0012 says how).
//
// Where it touches the game. CONTINUE's event asks for the field in EventFieldContinue_Callback's state 2
// (src/ov035/event_mapchange.c); a host-only block there (a Platform row of port/docs/game-changes.md) asks
// port_launcher_take_field_open first. Without --launch it returns 0 and the field opens, or traps, as before; with it,
// the launcher chains its own event in place of the field's and the game's event loop runs it from the next pass on.
//
// What it stands in for. On the DS the field menu (overlay 36) opens an app through EventFieldAppCall
// (src/ov012/event_field_proclink.c): states 6 and 7 fade the field out and close it, 11 creates the app's param and
// queues its proc, 12 waits for the proc to end, 13 reads the app's result and either opens the next app it asks for
// (the party list's SUMMARY opens the summary, whose end opens the party list again) or frees the param and opens the
// field again (8 to 10). The launcher fills the app call's input and work as EventFieldMenu_Create and
// EventFieldAppCall_Create fill them, but for the two calls that need the field (PlayerActionPerms_Create and
// CalcPlayerActionPossibilities): every field action is blocked and the possibilities are zero, as no field event can
// follow. It then runs states 11, 12 and 13 as the game has them, one pass each, and reports each app's result instead
// of opening the field again.
//
// What else it stands in for, through the game's own functions and setters, read from overlay 36's C but never calling
// it:
// - the menu's offer: it opens only the apps the save's field menu offers (FieldMenu_GetMenuType's kind of menu, from
//   the player's zone and two event flags, launcher_spec.c), else nothing, with status 2;
// - the menu's choice by a key, before each app: the UI's touch flag FALSE (GCTX_HIDSetUsingTouch,
//   src/ov036/field_menu.c:709),
//   whatever the last app left it at (an app closed by a touch sets it TRUE);
// - the field's close, before each app: the menu's subscreen recorded as the last one (GameData_SetLastSubscreen with
//   1, a constant: the game records what FieldSubscreen_Free returns, src/ov036/fieldmap.c:632, the subscreen the
//   field closes, the menu's, which the oracle shows as 1), and once, overlay 35 unloaded (UnloadFieldGlueOverlay),
//   leaving overlay 12 as the app call has it.
// The oracle shows both values at the bag's first frame: the touch flag 0 (1 while the menu was open), the last
// subscreen 1 (0 before the menu). What the field leaves on the screens and in memory (VRAM, palettes, the random
// numbers) it can't give: the oracle checks declare such differences.
//
// It never calls or queues overlay 36's code, which is under another contributor's claim upstream (issue #83). It
// reads one thing of it: whether FIELDMAP_PROC_FUNCS is still a trap placeholder (port/hal/trap.h). When it isn't, the
// launcher steps aside with a notice, the field opens as on the DS, and the launcher's tests fail on that notice.
//
// The run's exit status says whether the launch finished (launcher.h's PORT_LAUNCH_STATUS_*): 0 when every app opened
// and closed, 1 when it didn't (the run ended first, CONTINUE never asked for the field, the launcher stepped aside, or
// an app wanted the field), 2 when the save's menu doesn't offer an app. The run's own status, when it has one (a trap,
// or another cause of 1 that port/app/main.c lists), comes first.
#include "types.h"
#include "app/launcher.h"
#include "app/launcher_spec.h"
#include "field/app_call.h"
#include "field/event_game_clear.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/ui.h"
#include "hal/gfl/gfl_frame.h"
#include "hal/trap.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include <stdio.h>
#include <string.h>

// What src/ov012/itemuse_event.c's CheckFieldActionBlocked gives a blocked field action (FIELD_ACTION_BLOCKED)
#define ACTION_BLOCKED (-1)
// The field menu's subscreen, which EventFieldMenu_Callback's state 0 changes to (src/ov012/event_field_menu.c:91), and
// which the field's close records as the last one
#define SUBSCREEN_FIELD_MENU 1
// The event types after which the field menu does something in the field (EventFieldMenu_Callback): 2, a field move; 3,
// a field event, such as an item's use; 5, a script. 0 shows the menu again, 1 closes it
#define EVENT_TYPE_FIELD_MOVE 2
#define EVENT_TYPE_FIELD_EVENT 3
#define EVENT_TYPE_SCRIPT 5
// How many apps FIELD_PROC_LINK_LIST has
#define APP_COUNT 15
// How many apps one entry's chain of next apps can go through before the report stops naming them
#define CHAIN_MAX 8

// The apps of FIELD_PROC_LINK_LIST by index, for the report
static const char *const sAppNames[APP_COUNT] = {
    "party",           "pokedex",      "bag",       "card",      "save", "options",
    "retire",          "summary",      "town map",  "wifi note", "mail", "evolution",
    "battle recorder", "xtransceiver", "medal box",
};

typedef enum LauncherState {
    // Whether the save's field menu offers the apps, then the field's close
    LAUNCHER_START,
    // Until the entry's frame count
    LAUNCHER_WAIT,
    // EventFieldAppCall's states 11, 12 and 13
    LAUNCHER_OPEN,
    LAUNCHER_RUNNING,
    LAUNCHER_RESULT,
    // Asks the program to stop, and stays: the event never ends, so the parent never opens the field
    LAUNCHER_DONE,
} LauncherState;

// The launcher's event's data: the app call's input and work, as the field menu's event and the app call's event hold
// them, and where the launcher is
typedef struct LauncherWork {
    FieldAppCallInput input;
    FieldAppCallWork work;
    size_t entry;
    u32 openedAt;
    int chain[CHAIN_MAX];
    int chainLength;
} LauncherWork;

static PortLaunchSpec sSpec;
static int sConfigured;
static int sFieldAsked;
static int sSteppedAside;
static int sFinished;
static int sStatus;
// How many entries have closed, and whether the next one's app is open, for the message when a run ends first
static size_t sClosed;
static int sOpen;

int port_launcher_configure(const char *spec) {
    char error[200];

    if (spec == NULL) {
        return 1;
    }
    if (!port_launch_parse(spec, &sSpec, error, sizeof(error))) {
        fprintf(stderr, "port: --launch %s: %s\n", spec, error);
        return 0;
    }
    sConfigured = 1;
    return 1;
}

int port_launcher_finished(void) {
    return sFinished;
}

int port_launcher_end(void) {
    if (!sConfigured) {
        return PORT_LAUNCH_STATUS_DONE;
    }
    if (sFinished) {
        return sStatus;
    }
    if (!sFieldAsked) {
        fprintf(stderr, "port: --launch: the game never asked for the field, so no app was opened (the launcher "
                        "starts after CONTINUE)\n");
    } else if (sSteppedAside) {
        fprintf(stderr, "port: --launch: no app was opened: the launcher stepped aside\n");
    } else {
        fprintf(stderr, "port: --launch: the run ended before the launch did:");
        for (size_t i = sClosed; i < sSpec.count; i++) {
            fprintf(stderr, "%s %s %s", i == sClosed ? "" : ",", sSpec.entries[i].app->name,
                    i == sClosed && sOpen ? "still open" : "not opened");
        }
        fprintf(stderr, "\n");
    }
    return PORT_LAUNCH_STATUS_UNFINISHED;
}

static void Finish(int status) {
    sFinished = 1;
    sStatus = status;
}

// The app call's input and work for an entry, as the field menu's event (EventFieldMenu_Create, src/ov012/
// event_field_menu.c) and EventFieldAppCall_Create (src/ov012/event_field_proclink.c) fill them, without the field
static void StartEntry(LauncherWork *launch, GameEvent *event, size_t entry) {
    FieldAppCallInput *input = &launch->input;
    FieldAppCallWork *work = &launch->work;
    int appId = sSpec.entries[entry].app->appId;

    memset(input, 0, sizeof(*input));
    input->gameSystem = GameEvent_GetGameSystem(event);
    input->field = NULL;
    input->parent = event;
    input->appId = appId;
    input->appParam = -1;

    memset(work, 0, sizeof(*work));
    work->code = 0;
    work->input = input;
    work->result = FIELD_APP_RESULT_NEXT;
    work->event = event;
    work->appId = appId;
    work->prevAppId = appId;
    work->nextAppId = appId;
    FieldAppCallParam_Init(&work->params, input, NULL, NULL, NULL, NULL);
    for (int i = 0; i < PLAYER_ACTION_COUNT; i++) {
        PlayerActionPerms_SetActionBlocked(&work->perms, (u32)i, ACTION_BLOCKED);
    }
    // The menu's choice of the app by a key (src/ov036/field_menu.c:709, after EventFieldMenu_Create's TRUE in
    // src/ov012/event_field_menu.c), whatever the last app left the flag at, and the field's close, which records the
    // subscreen it frees, the menu's (src/ov036/fieldmap.c:632)
    GCTX_HIDSetUsingTouch(FALSE);
    GameData_SetLastSubscreen(GSYS_GetGameData(input->gameSystem), SUBSCREEN_FIELD_MENU);

    launch->entry = entry;
    launch->chainLength = 0;
}

// Whether the field menu the save gets offers each app: the kind of menu FieldMenu_GetMenuType gives
// (src/ov036/field_menu.c:480-496, read, not called) from what it asks, as FieldMenu_Create asks it; says which isn't
static int SaveOffersApps(GameSystem *gsys) {
    GameData *gameData = GSYS_GetGameData(gsys);
    EventWork *eventWork = GameData_GetEventWork(gameData);
    u16 zoneId = PlayerState_GetZoneID(GSYS_GetPlayerState(gsys));
    PortLaunchPlace place;
    PortLaunchMenu menu;

    place.unionRoom = GetZoneIsUnionRoom(zoneId) || IsZone150Or151(zoneId);
    place.royalUnova = IsZoneRoyalUnova(zoneId) != FALSE;
    place.seasonSync = GameData_IsForceSeasonSync(gameData) != FALSE;
    place.hasPokemon = EventWork_FlagGet(eventWork, 0x961) != FALSE;
    place.hasPokedex = EventWork_FlagGet(eventWork, 0x962) != FALSE;
    place.blackTower = IsZoneBlackTowerOrWhiteTreehollow(zoneId) != FALSE;
    menu = port_launch_menu(&place);
    for (size_t i = 0; i < sSpec.count; i++) {
        const PortLaunchApp *app = sSpec.entries[i].app;
        if (!port_launch_menu_offers(menu, app)) {
            fprintf(stderr, "port: --launch: %s: the field menu doesn't offer it in this save, which gets %s\n",
                    app->name, port_launch_menu_name(menu));
            return 0;
        }
    }
    return 1;
}

static void Report(LauncherWork *launch, u32 closedAt) {
    const PortLaunchApp *app = sSpec.entries[launch->entry].app;

    fprintf(stderr, "port: --launch: %s", app->name);
    for (int i = 1; i < launch->chainLength; i++) {
        fprintf(stderr, "%s%s", i == 1 ? ", then " : ", ", sAppNames[launch->chain[i]]);
    }
    fprintf(stderr, ": opened at count %u, closed at count %u: result %u, event type %u\n", (unsigned)launch->openedAt,
            (unsigned)closedAt, (unsigned)launch->work.result, (unsigned)launch->input.eventType);
}

static GameEventReturnCode Launcher_Callback(GameEvent *event, u32 *state, void *data) {
    LauncherWork *launch = data;
    FieldAppCallWork *work = &launch->work;
    GameSystem *gsys = GameEvent_GetGameSystem(event);
    const PortLaunchEntry *entry;
    void *param;

    switch (*state) {
    case LAUNCHER_START:
        if (!SaveOffersApps(gsys)) {
            Finish(PORT_LAUNCH_STATUS_NOT_OFFERED);
            *state = LAUNCHER_DONE;
            break;
        }
        // The field's close (src/system/field_event.c:86)
        UnloadFieldGlueOverlay();
        StartEntry(launch, event, 0);
        *state = LAUNCHER_WAIT;
        break;
    case LAUNCHER_WAIT:
        entry = &sSpec.entries[launch->entry];
        if (entry->hasCount && port_gfl_frame_count() < entry->count) {
            break;
        }
        launch->openedAt = port_gfl_frame_count();
        *state = LAUNCHER_OPEN;
        // The app opens on the pass the count is reached
        // fall through
    case LAUNCHER_OPEN:
        // EventFieldAppCall_Callback's state 11
        if (FIELD_PROC_LINK_LIST[work->appId].functions == NULL) {
            fprintf(stderr, "port: --launch: %s asked for %s, which runs in the field: the launcher stops here\n",
                    sSpec.entries[launch->entry].app->name, sAppNames[work->appId]);
            Finish(PORT_LAUNCH_STATUS_UNFINISHED);
            *state = LAUNCHER_DONE;
            break;
        }
        param =
            FIELD_PROC_LINK_LIST[work->appId].createParam(work, work->input->appParam, work->prevAppId, work->appParam);
        if (work->appParam != NULL) {
            FIELD_PROC_LINK_LIST[work->prevAppId].freeParam(work->appParam);
            work->appParam = NULL;
        }
        work->appParam = param;
        GSYS_QueueProc(gsys, FIELD_PROC_LINK_LIST[work->appId].overlayId, FIELD_PROC_LINK_LIST[work->appId].functions,
                       param);
        if (launch->chainLength < CHAIN_MAX) {
            launch->chain[launch->chainLength++] = work->appId;
        }
        sOpen = 1;
        *state = LAUNCHER_RUNNING;
        break;
    case LAUNCHER_RUNNING:
        // State 12
        if (GSYS_GetProcMgrState(gsys)) {
            break;
        }
        *state = LAUNCHER_RESULT;
        break;
    case LAUNCHER_RESULT:
        // State 13, which opens the next app the app asked for, else frees the param; then the launcher's report in
        // place of the field's return
        work->result = FIELD_PROC_LINK_LIST[work->appId].getResult(work, work->appParam);
        EventFieldAppCall_ConvAppResultToEventType(work->result, &work->input->eventType);
        if (work->result == FIELD_APP_RESULT_NEXT) {
            work->prevAppId = work->appId;
            work->appId = work->nextAppId;
            *state = LAUNCHER_OPEN;
            break;
        }
        FieldAppCall_ClearBagFreeSpaceFilter(work);
        if (work->appParam != NULL) {
            GFL_HeapFree(work->appParam);
            work->appParam = NULL;
        }
        Report(launch, port_gfl_frame_count());
        sClosed = launch->entry + 1;
        sOpen = 0;
        // What the field menu does next in the field, the launcher can't do: the launch ends there, unfinished, the
        // DS's next steps not taken
        if (launch->input.eventType == EVENT_TYPE_FIELD_MOVE || launch->input.eventType == EVENT_TYPE_FIELD_EVENT ||
            launch->input.eventType == EVENT_TYPE_SCRIPT) {
            fprintf(stderr, "port: --launch: event type %u needs the field: the launcher stops here\n",
                    (unsigned)launch->input.eventType);
            Finish(PORT_LAUNCH_STATUS_UNFINISHED);
            *state = LAUNCHER_DONE;
        } else if (launch->entry + 1 < sSpec.count) {
            StartEntry(launch, event, launch->entry + 1);
            *state = LAUNCHER_WAIT;
        } else {
            Finish(PORT_LAUNCH_STATUS_DONE);
            *state = LAUNCHER_DONE;
        }
        break;
    case LAUNCHER_DONE:
        break;
    }
    return GAMEEVENT_CONTINUE;
}

int port_launcher_take_field_open(struct GameEvent *event) {
    GameEvent *launcher;

    if (!sConfigured) {
        return 0;
    }
    sFieldAsked = 1;
    if (!port_trap_is_placeholder(&FIELDMAP_PROC_FUNCS)) {
        fprintf(stderr, "port: --launch steps aside: overlay 36's FIELDMAP_PROC_FUNCS has C, so the field opens as on "
                        "the DS; retire the launcher (ADR 0012)\n");
        sSteppedAside = 1;
        return 0;
    }
    // On HEAPID_GAMEEVENT, as every event (src/system/game_event.c)
    launcher = GameEvent_Create(GameEvent_GetGameSystem(event), NULL, Launcher_Callback, sizeof(LauncherWork));
    GameEvent_ChainNext(event, launcher);
    return 1;
}
