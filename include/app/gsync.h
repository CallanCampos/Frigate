#ifndef POKEBW2_APP_GSYNC_H
#define POKEBW2_APP_GSYNC_H

#include "types.h"
#include "app/box2.h"
#include "app/wifi_login.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// Game Sync and the Dream World account, overlay 199. Game Sync's event (field/event_gsync.h) runs the menu,
// then Game Sync itself, and the start menu's Game Sync settings run the account screens
#define OVERLAY_GSYNC OVERLAY_ID(199)

// Results of the Game Sync procs
#define GSYNC_RESULT_NONE 0
#define GSYNC_RESULT_ACCOUNT 1
#define GSYNC_RESULT_CONNECT 2
#define GSYNC_RESULT_WIFI_SETTINGS 3
#define GSYNC_RESULT_NO_POKEMON 4
#define GSYNC_RESULT_SELECT_POKEMON 5
#define GSYNC_RESULT_DONE 6
#define GSYNC_RESULT_RETRY_LOGIN 7

// Errors of Game Sync's own, beside the server's, which the account screens share but for GSYNC_ERROR_DOWNLOAD
enum {
    GSYNC_ERROR_DOWNLOAD = 0xfff0,
    GSYNC_ERROR_SERVICE_UNAVAILABLE,
    GSYNC_ERROR_BAD_GATEWAY,
    GSYNC_ERROR_HTTP,
};

// The answer to Game Sync's account.playstatus request. Status 0 is an account that isn't set up yet, 1 one whose
// Pokémon isn't dreaming yet and 2 one whose Pokémon is dreaming. With 0 and 4, the Pokémon and items below come back
// without the server's result, and with 4 Game Sync doesn't ask the server again
typedef struct {
    u16 status;
    u16 species;
    u16 unk4;
    u8 form;
    u8 sex;
    u16 items[20];
    u8 itemCounts[20];
} GSyncAccountInfo;

// Game Sync's event work, which its procs also take as their parameter
typedef struct {
    GameSystem *gsys;
    GameData *gameData;
    SaveControl *save;
    GSyncAccountInfo accountInfo;
    BOOL hasAccountInfo;
    u8 loginBuffer[0x174];
    u32 gsyncResult;
    BOOL bgmPushed;
    u16 boxTray;
    u16 boxPosition;
    u32 bgm;
    // A flag for each species that can be sent to the Dream World, from Game Sync's sleepily.bitlist request
    u8 sendableSpecies[0x80];
    WifiLoginParam login;
    WifiLogoutParam logout;
    Box2Param box;
    // Whether the menu comes back from the Wi-Fi settings, which it fades in from white
    BOOL fromWifiSettings;
} EventGameSync;

// PDWACC_PROC_FUNCTIONS's parameter, from the start menu's Game Sync settings
typedef struct {
    GameData *gameData;
    void *loginBuffer;
    HeapID heapId;
    // PDWACC_MODE_*
    u32 mode;
    // Whether the connection was lost
    BOOL disconnected;
} PdwAccParam;

// Whether the account screens create the account or only show the Game Sync ID
#define PDWACC_MODE_CREATE 0
#define PDWACC_MODE_SHOW_ID 1

// The Game Sync menu (gsync_menu.c): Game Sync, the Wi-Fi settings or back
extern const GameProcFunctions GSYNC_MENU_PROC_FUNCTIONS;
// The start menu's Game Sync settings, in the main program
extern const GameProcFunctions GAME_SYNC_SETTINGS_PROC_FUNCTIONS;
// Game Sync itself (gsync_state.c)
extern const GameProcFunctions GSYNC_PROC_FUNCTIONS;
// The Dream World account screens (pdwacc_state.c), which the Game Sync settings start
extern const GameProcFunctions PDWACC_PROC_FUNCTIONS;

#endif // POKEBW2_APP_GSYNC_H
