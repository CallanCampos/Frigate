// Reads the --launch option of Frigate's test launcher and says which apps the save's field menu offers
// (launcher_spec.h; port/app/launcher.c opens the field menu's apps by them, ADR 0012). Plain C11.
#include "app/launcher_spec.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The kinds of menu that have each item: src/ov036/field_menu.c's data_ov036_021d0054 by FieldMenu_GetMenuType's
// kinds, its items being src/ov012/event_field_menu.c's sFieldMenuAppIds (1 POKéMON, 2 POKéDEX, 3 BAG, 4 TRAINER CARD)
#define MENU(kind) (1u << PORT_LAUNCH_MENU_##kind)
#define EVERY_MENU ((1u << PORT_LAUNCH_MENU_COUNT) - 1)
#define POKEMON_MENUS (EVERY_MENU & ~MENU(NO_POKEMON))
#define POKEDEX_MENUS (EVERY_MENU & ~MENU(NO_POKEMON) & ~MENU(NO_POKEDEX))
#define BAG_MENUS (EVERY_MENU & ~MENU(BLACK_TOWER))
#define CARD_MENUS EVERY_MENU

// The apps, by their index in FIELD_PROC_LINK_LIST (include/field/app_call.h: FIELD_APP_POKELIST 0, FIELD_APP_ZUKAN 1,
// FIELD_APP_BAG 2, FIELD_APP_TRAINER_CARD 3, FIELD_APP_POKESTATUS 7). The summary is the one the game opens on its own
// (its param's standalone path in event_field_proclink.c), not through the party list; the menu offers it where it
// offers the party list
static const PortLaunchApp sApps[] = {
    { "party", 0, POKEMON_MENUS }, { "pokedex", 1, POKEDEX_MENUS }, { "bag", 2, BAG_MENUS },
    { "card", 3, CARD_MENUS },     { "summary", 7, POKEMON_MENUS },
};

static const char *const sMenuNames[PORT_LAUNCH_MENU_COUNT] = {
    "the full menu (type 0)",
    "the Union Room's menu (type 1), which has no SAVE",
    "the menu without the Pokédex (type 2, event flag 0x962 clear), which has no POKéDEX",
    "the menu without a Pokémon (type 3, event flag 0x961 clear), which has no POKéMON and no POKéDEX",
    "the Royal Unova's menu (type 4), which has no SAVE",
    "the menu of a season's change (type 5), which has no SAVE",
    "the Black Tower's and White Treehollow's menu (type 6), which has no BAG",
};

const PortLaunchApp *port_launch_find_app(const char *name, size_t length) {
    for (size_t i = 0; i < sizeof(sApps) / sizeof(sApps[0]); i++) {
        if (strlen(sApps[i].name) == length && strncmp(sApps[i].name, name, length) == 0) {
            return &sApps[i];
        }
    }
    return NULL;
}

// One entry, APP or APP@COUNT, of length bytes
static int ParseEntry(const char *text, size_t length, PortLaunchEntry *entry, char *error, size_t errorSize) {
    const char *at = memchr(text, '@', length);
    size_t nameLength = at != NULL ? (size_t)(at - text) : length;

    entry->app = port_launch_find_app(text, nameLength);
    if (entry->app == NULL) {
        snprintf(error, errorSize, "\"%.*s\" isn't an app the launcher opens (party, pokedex, bag, card, summary)",
                 (int)nameLength, text);
        return 0;
    }
    entry->count = 0;
    entry->hasCount = at != NULL;
    if (at != NULL) {
        char digits[16];
        size_t count = length - nameLength - 1;
        char *end;
        unsigned long value;

        if (count == 0 || count >= sizeof(digits) || strspn(at + 1, "0123456789") < count) {
            snprintf(error, errorSize, "\"%.*s\" needs a frame count after @", (int)length, text);
            return 0;
        }
        memcpy(digits, at + 1, count);
        digits[count] = '\0';
        errno = 0;
        value = strtoul(digits, &end, 10);
        if (errno != 0 || value > UINT32_MAX) {
            snprintf(error, errorSize, "\"%.*s\": the frame count is too large", (int)length, text);
            return 0;
        }
        entry->count = (uint32_t)value;
    }
    return 1;
}

int port_launch_parse(const char *text, PortLaunchSpec *spec, char *error, size_t errorSize) {
    const char *start = text;

    spec->count = 0;
    while (1) {
        const char *comma = strchr(start, ',');
        size_t length = comma != NULL ? (size_t)(comma - start) : strlen(start);

        if (length == 0) {
            snprintf(error, errorSize, "an empty entry: name an app, as in --launch bag@4100,card@4600");
            return 0;
        }
        if (spec->count == PORT_LAUNCH_MAX_ENTRIES) {
            snprintf(error, errorSize, "more than %d apps", PORT_LAUNCH_MAX_ENTRIES);
            return 0;
        }
        if (!ParseEntry(start, length, &spec->entries[spec->count], error, errorSize)) {
            return 0;
        }
        if (spec->count > 0 && spec->entries[spec->count].hasCount) {
            for (size_t i = spec->count; i-- > 0;) {
                if (spec->entries[i].hasCount) {
                    if (spec->entries[spec->count].count < spec->entries[i].count) {
                        snprintf(error, errorSize, "\"%.*s\" comes before an earlier entry's count, %u", (int)length,
                                 start, (unsigned)spec->entries[i].count);
                        return 0;
                    }
                    break;
                }
            }
        }
        spec->count++;
        if (comma == NULL) {
            return 1;
        }
        start = comma + 1;
    }
}

PortLaunchMenu port_launch_menu(const PortLaunchPlace *place) {
    if (place->unionRoom) {
        return PORT_LAUNCH_MENU_UNION_ROOM;
    }
    if (place->royalUnova) {
        return PORT_LAUNCH_MENU_ROYAL_UNOVA;
    }
    if (place->seasonSync) {
        return PORT_LAUNCH_MENU_SEASON_SYNC;
    }
    if (!place->hasPokemon) {
        return PORT_LAUNCH_MENU_NO_POKEMON;
    }
    if (!place->hasPokedex) {
        return PORT_LAUNCH_MENU_NO_POKEDEX;
    }
    if (place->blackTower) {
        return PORT_LAUNCH_MENU_BLACK_TOWER;
    }
    return PORT_LAUNCH_MENU_FULL;
}

int port_launch_menu_offers(PortLaunchMenu menu, const PortLaunchApp *app) {
    return (app->menus & (1u << menu)) != 0;
}

const char *port_launch_menu_name(PortLaunchMenu menu) {
    return sMenuNames[menu];
}
