// Tests of Frigate's test launcher's option and of its gate (port/app/launcher_spec.c, port/hal/trap.c; ADR 0012):
// --launch's parsing, and port_trap_is_placeholder, by which the launcher steps aside once overlay 36's process table
// has C. The gate runs on a made-up table of placeholders, as port/tools/gen_stubs.py writes one.
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
    Check(app != NULL && app->appId == 0 && app->flag == 0x961, "party is FIELD_APP_POKELIST, behind flag 0x961");
    app = port_launch_find_app("pokedex", 7);
    Check(app != NULL && app->appId == 1 && app->flag == 0x962, "pokedex is FIELD_APP_ZUKAN, behind flag 0x962");
    app = port_launch_find_app("bag", 3);
    Check(app != NULL && app->appId == 2 && app->flag == 0, "bag is FIELD_APP_BAG, always offered");
    app = port_launch_find_app("card", 4);
    Check(app != NULL && app->appId == 3 && app->flag == 0, "card is FIELD_APP_TRAINER_CARD, always offered");
    app = port_launch_find_app("summary", 7);
    Check(app != NULL && app->appId == 7 && app->flag == 0x961, "summary is FIELD_APP_POKESTATUS, behind 0x961");
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
    TestGate();
    printf("launcher_spec: %s\n", sFailed ? "FAIL" : "ok");
    return sFailed;
}
