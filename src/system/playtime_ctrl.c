#include "types.h"
#include "nitro/os.h"
#include "system/game_data.h"
#include "system/playtime_ctrl.h"

// The play time count. The file's name is a guess: the ROM has no string for it

typedef struct {
    BOOL active;
    u64 lastTick;
    // The seconds counted into the play time since startTick
    u64 seconds;
    u64 startTick;
} PlayTimeCtrl;

static PlayTimeCtrl g_GameSysTimer;

void GameSystemTimer_Disable(void) {
    g_GameSysTimer.active = FALSE;
}

void GameSystemTimer_Start(void) {
    g_GameSysTimer.active = TRUE;
    g_GameSysTimer.lastTick = 0;
    g_GameSysTimer.seconds = 0;
    g_GameSysTimer.startTick = OS_GetTick();
}

void GameSystemTimer_Update(GameData *gameData) {
    u64 tick;
    u64 seconds;

    if (g_GameSysTimer.active == FALSE) {
        return;
    }
    tick = OS_GetTick();
    if (tick < g_GameSysTimer.lastTick) {
        tick += g_GameSysTimer.lastTick;
    } else {
        g_GameSysTimer.lastTick = tick;
    }
    seconds = OS_TicksToSeconds(tick - g_GameSysTimer.startTick);
    if (g_GameSysTimer.seconds < seconds) {
        GameData_UpdateTime(gameData, seconds - g_GameSysTimer.seconds);
        if (g_GameSysTimer.lastTick == tick) {
            g_GameSysTimer.seconds = seconds;
        } else {
            tick = OS_GetTick();
            g_GameSysTimer.startTick = tick;
            g_GameSysTimer.seconds = OS_TicksToSeconds(tick);
            g_GameSysTimer.lastTick = tick;
        }
    }
}
