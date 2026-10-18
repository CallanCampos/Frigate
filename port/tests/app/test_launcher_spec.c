// Tests of Frigate's test launcher's option and of its gate (port/app/launcher_spec.c, port/hal/trap.c; ADR 0012):
// --launch's parsing, which apps each kind of field menu offers, and port_trap_is_placeholder, by which the launcher
// steps aside once overlay 36's process table has C. The kinds of menu are checked against src/ov036/field_menu.c's
// FieldMenu_GetMenuType and its table of items, data_ov036_021d0054, read by hand. The gate runs on a made-up table of
// placeholders, as port/tools/gen_stubs.py writes one.
#include "app/launcher_spec.h"
#include "hal/trap.h"
#include <stdio.h>
#include <string.h>

static int sFailed;

static void Check(int ok, const char *what) {
    if (!ok) {
        printf("FAIL: %s\n", what);
        sFailed = 1;
    }
}

// Two placeholders as gen_stubs.py defines them: their storage, their table and the paddings around them
static unsigned char sPaddingBegin[64];
static unsigned char sProcTable[0x20];
static unsigned char sHeap[0x10];
static unsigned char sPaddingEnd[64];
const PortTrapSymbol port_trap_data_symbols[] = {
    { "FIELDMAP_PROC_FUNCS", 0x021c9eb8, sizeof(sProcTable), "ov036" },
    { "g_FieldHeapMemory", 0x021ca000, sizeof(sHeap), "ov036" },
};
const size_t port_trap_data_count = 2;
unsigned char *const port_trap_data_begin = sPaddingBegin;
unsigned char *const port_trap_data_end = sPaddingEnd + sizeof(sPaddingEnd);
void *const port_trap_data_addresses[] = { sProcTable, sHeap };

static int Parses(const char *text, PortLaunchSpec *spec) {
    char error[200];
    int ok = port_launch_parse(text, spec, error, sizeof(error));
    if (!ok) {
        printf("  (%s: %s)\n", text, error);
    }
    return ok;
}

static void TestNames(void) {
    const PortLaunchApp *app;

    app = port_launch_find_app("party", 5);
    Check(app != NULL && app->appId == 0, "party is FIELD_APP_POKELIST");
    app = port_launch_find_app("pokedex", 7);
    Check(app != NULL && app->appId == 1, "pokedex is FIELD_APP_ZUKAN");
    app = port_launch_find_app("bag", 3);
    Check(app != NULL && app->appId == 2, "bag is FIELD_APP_BAG");
    app = port_launch_find_app("card", 4);
    Check(app != NULL && app->appId == 3, "card is FIELD_APP_TRAINER_CARD");
    app = port_launch_find_app("summary", 7);
    Check(app != NULL && app->appId == 7, "summary is FIELD_APP_POKESTATUS");
    Check(port_launch_find_app("save", 4) == NULL, "save isn't launched: it needs the field");
    Check(port_launch_find_app("bags", 3) != NULL, "a name is matched on its length");
    Check(port_launch_find_app("ba", 2) == NULL, "a prefix isn't a name");
}

static void TestParse(void) {
    PortLaunchSpec spec;
    char error[200];

    Check(Parses("bag", &spec) && spec.count == 1 && spec.entries[0].app->appId == 2 && !spec.entries[0].hasCount,
          "one app, at once");
    Check(Parses("bag@4100", &spec) && spec.count == 1 && spec.entries[0].hasCount && spec.entries[0].count == 4100,
          "one app at a count");
    Check(Parses("bag@4100,card@4600,summary", &spec) && spec.count == 3 && spec.entries[1].app->appId == 3 &&
              spec.entries[1].count == 4600 && spec.entries[2].app->appId == 7 && !spec.entries[2].hasCount,
          "a list");
    Check(Parses("party@0", &spec) && spec.entries[0].hasCount && spec.entries[0].count == 0, "count 0");
    Check(Parses("bag@4294967295", &spec) && spec.entries[0].count == 4294967295u, "the largest count");

    Check(!port_launch_parse("nosuchapp", &spec, error, sizeof(error)) && strstr(error, "nosuchapp") != NULL,
          "an unknown app is refused by name");
    Check(!port_launch_parse("", &spec, error, sizeof(error)), "an empty value is refused");
    Check(!port_launch_parse("bag,,card", &spec, error, sizeof(error)), "an empty entry is refused");
    Check(!port_launch_parse("bag,", &spec, error, sizeof(error)), "a trailing comma is refused");
    Check(!port_launch_parse("bag@", &spec, error, sizeof(error)), "@ without a count is refused");
    Check(!port_launch_parse("bag@12x", &spec, error, sizeof(error)), "a count with other characters is refused");
    Check(!port_launch_parse("bag@-1", &spec, error, sizeof(error)), "a negative count is refused");
    Check(!port_launch_parse("bag@4294967296", &spec, error, sizeof(error)), "a count past 32 bits is refused");
    Check(!port_launch_parse("@100", &spec, error, sizeof(error)), "a count without an app is refused");
    Check(!port_launch_parse("bag@4600,card@4100", &spec, error, sizeof(error)), "counts that go down are refused");
    Check(Parses("bag@4100,card,summary@4600", &spec), "an entry without a count between two with counts");
    Check(!port_launch_parse("bag@4600,card,summary@4100", &spec, error, sizeof(error)),
          "a count below the last earlier count is refused across an entry without one");
    Check(Parses("bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag", &spec) && spec.count == 16,
          "16 apps");
    Check(!port_launch_parse("bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag,bag", &spec, error,
                             sizeof(error)),
          "17 apps are refused");
}

// FieldMenu_GetMenuType's kind of menu for a place given as its six answers, in its order of questions
static PortLaunchMenu Menu(int unionRoom, int royalUnova, int seasonSync, int hasPokemon, int hasPokedex,
                           int blackTower) {
    PortLaunchPlace place = { unionRoom, royalUnova, seasonSync, hasPokemon, hasPokedex, blackTower };
    return port_launch_menu(&place);
}

// Whether a kind of menu offers each of party, pokedex, bag, card and summary, as a string of 1s and 0s
static int Offers(PortLaunchMenu menu, const char *expected) {
    static const char *const names[] = { "party", "pokedex", "bag", "card", "summary" };
    char got[6];

    for (int i = 0; i < 5; i++) {
        got[i] = port_launch_menu_offers(menu, port_launch_find_app(names[i], strlen(names[i]))) ? '1' : '0';
    }
    got[5] = '\0';
    if (strcmp(got, expected) != 0) {
        printf("  (menu type %d offers %s, not %s)\n", (int)menu, got, expected);
        return 0;
    }
    return 1;
}

static void TestMenus(void) {
    // The kinds by their numbers in FieldMenu_GetMenuType
    Check(Menu(0, 0, 0, 1, 1, 0) == 0, "a save with a Pokémon and the Pokédex gets the full menu, type 0");
    Check(Menu(1, 0, 0, 1, 1, 0) == 1, "the Union Room's menu is type 1");
    Check(Menu(0, 0, 0, 1, 0, 0) == 2, "without the Pokédex, type 2");
    Check(Menu(0, 0, 0, 0, 0, 0) == 3, "without a Pokémon, type 3");
    Check(Menu(0, 1, 0, 1, 1, 0) == 4, "the Royal Unova's menu is type 4");
    Check(Menu(0, 0, 1, 1, 1, 0) == 5, "a forced season's change gives type 5");
    Check(Menu(0, 0, 0, 1, 1, 1) == 6, "the Black Tower's menu is type 6");
    // Its order: the zone's own menus before the flags, the flags before the Black Tower
    Check(Menu(1, 0, 0, 0, 0, 0) == 1, "the Union Room's menu even without a Pokémon");
    Check(Menu(0, 1, 1, 0, 0, 0) == 4, "the Royal Unova's before a season's change and the flags");
    Check(Menu(0, 0, 1, 0, 0, 0) == 5, "a season's change before the flags");
    Check(Menu(0, 0, 0, 0, 1, 1) == 3, "no Pokémon before the Black Tower");
    Check(Menu(0, 0, 0, 1, 0, 1) == 2, "no Pokédex before the Black Tower");

    // Their items (data_ov036_021d0054's rows; items 1 to 4 are POKéMON, POKéDEX, BAG and TRAINER CARD), in the order
    // party, pokedex, bag, card, summary; the summary goes with POKéMON
    Check(Offers(0, "11111"), "type 0, items 1 2 3 4 5 6: every app");
    Check(Offers(1, "11111"), "type 1, items 1 2 3 4 6: every app");
    Check(Offers(2, "10111"), "type 2, items 1 3 4 5 6: no Pokédex");
    Check(Offers(3, "00110"), "type 3, items 3 4 5 6: the bag and the card");
    Check(Offers(4, "11111"), "type 4, items 1 2 3 4 6: every app");
    Check(Offers(5, "11111"), "type 5, items 1 2 3 4 6: every app");
    Check(Offers(6, "11011"), "type 6, items 1 2 4 5 6 8: no bag");
    for (int menu = 0; menu < PORT_LAUNCH_MENU_COUNT; menu++) {
        Check(port_launch_menu_name((PortLaunchMenu)menu) != NULL, "every kind of menu has a name");
    }
}

static void TestGate(void) {
    Check(port_trap_is_placeholder(sProcTable), "a placeholder's first byte is in it");
    Check(port_trap_is_placeholder(sProcTable + sizeof(sProcTable) - 1), "and its last");
    Check(port_trap_is_placeholder(sHeap + 4), "another placeholder's byte is in it");
    Check(!port_trap_is_placeholder(sPaddingBegin), "the paddings aren't placeholders");
    Check(!port_trap_is_placeholder(sPaddingEnd + 8), "the paddings aren't placeholders");
    Check(!port_trap_is_placeholder(&sFailed), "an object with C isn't one: the launcher steps aside for it");
    Check(!port_trap_is_placeholder(NULL), "NULL isn't one");
}

int main(void) {
    TestNames();
    TestParse();
    TestMenus();
    TestGate();
    printf("launcher_spec: %s\n", sFailed ? "FAIL" : "ok");
    return sFailed;
}
