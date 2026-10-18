#ifndef PORT_APP_LAUNCHER_SPEC_H
#define PORT_APP_LAUNCHER_SPEC_H

// What Frigate's test launcher opens (port/app/launcher.c, ADR 0012): the program's option
//
//   --launch APP[@COUNT][,APP[@COUNT]]...
//
// names apps of the field menu (party, pokedex, bag, card, summary), opened one after the other once a continued game
// asks for the field, each at the first pass of the game's main loop at which GFL's frame count reaches COUNT (at once
// without one); and which of them the save's field menu offers. Plain C11 without the decomp's headers, so that it is
// tested on its own (port/tests/app/test_launcher_spec.c).
#include <stddef.h>
#include <stdint.h>

// How many apps one --launch can name
#define PORT_LAUNCH_MAX_ENTRIES 16

// The kinds of field menu, as src/ov036/field_menu.c's FieldMenu_GetMenuType tells them apart (read, not called: it is
// overlay 36's, which has no C on the port yet), by its numbers, and the items each one has (its table
// data_ov036_021d0054)
typedef enum PortLaunchMenu {
    PORT_LAUNCH_MENU_FULL,        // POKéMON, POKéDEX, BAG, TRAINER CARD, SAVE, OPTIONS
    PORT_LAUNCH_MENU_UNION_ROOM,  // the Union Room, and zones 150 and 151: no SAVE
    PORT_LAUNCH_MENU_NO_POKEDEX,  // event flag 0x962 clear: no POKéDEX
    PORT_LAUNCH_MENU_NO_POKEMON,  // event flag 0x961 clear: no POKéMON, no POKéDEX
    PORT_LAUNCH_MENU_ROYAL_UNOVA, // aboard the Royal Unova: no SAVE
    PORT_LAUNCH_MENU_SEASON_SYNC, // while the game forces a season's change: no SAVE
    PORT_LAUNCH_MENU_BLACK_TOWER, // the Black Tower or the White Treehollow: no BAG, and RETIRE
    PORT_LAUNCH_MENU_COUNT,
} PortLaunchMenu;

// What FieldMenu_GetMenuType asks of the game, each 0 or 1: of the player's zone (GetZoneIsUnionRoom or
// IsZone150Or151, IsZoneRoyalUnova, IsZoneBlackTowerOrWhiteTreehollow), of GameData (GameData_IsForceSeasonSync) and of
// the event flags 0x961 (a Pokémon) and 0x962 (the Pokédex)
typedef struct PortLaunchPlace {
    int unionRoom;
    int royalUnova;
    int seasonSync;
    int hasPokemon;
    int hasPokedex;
    int blackTower;
} PortLaunchPlace;

// An app the launcher can open: its name in --launch, its index in the game's table of the apps the field opens
// (FIELD_PROC_LINK_LIST, src/ov012/event_field_proclink.c; include/field/app_call.h's FIELD_APP_*), and the kinds of
// menu that offer it, a bit per PortLaunchMenu
typedef struct PortLaunchApp {
    const char *name;
    int appId;
    unsigned menus;
} PortLaunchApp;

typedef struct PortLaunchEntry {
    const PortLaunchApp *app;
    // The frame count to open it at, and whether one was given (without one it opens at once)
    uint32_t count;
    int hasCount;
} PortLaunchEntry;

typedef struct PortLaunchSpec {
    PortLaunchEntry entries[PORT_LAUNCH_MAX_ENTRIES];
    size_t count;
} PortLaunchSpec;

// The app named name (length bytes of it), or NULL
const PortLaunchApp *port_launch_find_app(const char *name, size_t length);

// Reads --launch's value into spec. Returns 1, or 0 with why in error (a name that isn't an app, a count that isn't a
// number, an empty entry, too many entries, or counts that go down)
int port_launch_parse(const char *text, PortLaunchSpec *spec, char *error, size_t errorSize);

// The kind of field menu the player gets at place, in FieldMenu_GetMenuType's order: the zone's own menus first, then
// the flags, then the Black Tower's
PortLaunchMenu port_launch_menu(const PortLaunchPlace *place);

// Whether that kind of menu offers the app (the summary, reached from the party list, wherever POKéMON is)
int port_launch_menu_offers(PortLaunchMenu menu, const PortLaunchApp *app);

// The kind of menu in words, for the launcher's messages
const char *port_launch_menu_name(PortLaunchMenu menu);

#endif // PORT_APP_LAUNCHER_SPEC_H
