#include "types.h"
#include "constants/arc.h"
#include "constants/battle.h"
#include "constants/pokemon.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nnsys/g2d.h"
#include "pml/poke_party.h"
#include "system/app_menu_common.h"

// The graphics that the game's menus share: the archive's id, and the files of its icons. The file's name is a
// guess: the ROM has no string for it

// The menu bar's screen, and where it sits in a 32-tile-wide screen: its bottom three rows
#define BAR_SCREEN_FILE 0x1d
#define BAR_SCREEN_START (21 * 32)
#define BAR_SCREEN_TILES (3 * 32)

// The palette within the type icons' palette file of each type and contest category
static const u8 sTypeIconPalettes[] = {
    0, 0, 1, 1, 0, 0, 2, 1, 0, 0, 1, 2, 0, 1, 1, 2, 0, 0, 1, 1, 2, 0,
};

static const u8 sIconPalettes[] = {
    0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 1,
};

BOOL func_0202d7d8(void) {
    return 0;
}

void func_0202d7dc(void) {
}

u32 getUINarcIdx(void) {
    return ARCID_APP_MENU_COMMON;
}

u32 func_0202d7e4(void) {
    return 0x21;
}

u8 func_0202d7e8(u8 type) {
    return sTypeIconPalettes[type];
}

u32 func_0202d7f4(u8 type) {
    return 0x22 + type;
}

u32 func_0202d7f8(u32 mapping) {
    return 0x3b + mapping;
}

u32 func_0202d7fc(u32 mapping) {
    return 0x3e + mapping;
}

u8 func_0202d800(u8 index) {
    return sIconPalettes[index];
}

u32 func_0202d80c(u8 index) {
    return 0x38 + index;
}

u32 func_0202d810(void) {
    return 0x13;
}

u32 func_0202d814(void) {
    return 0x14;
}

u32 func_0202d818(u32 mapping) {
    return 0x15 + mapping;
}

u32 func_0202d81c(u32 mapping) {
    return 0x18 + mapping;
}

u32 func_0202d820(void) {
    return 0x1b;
}

u32 func_0202d824(void) {
    return 0x1c;
}

u32 func_0202d828(void) {
    return BAR_SCREEN_FILE;
}

u32 func_0202d82c(void) {
    return 0x1e;
}

void AppMenuCommon_LoadBarScreen(ArcTool *arc, u8 bg, HeapID heapId, u32 charBase, u32 palette) {
    u16 *dest = GFL_BGSysIsScrHeapExists(bg);
    NNSG2dScreenData *screen;
    void *file = GFL_G2DIOReadNSCRArc(arc, BAR_SCREEN_FILE, FALSE, &screen, heapId);
    int i;

    MI_CpuCopy8((u16 *)screen->rawData + BAR_SCREEN_START, dest + BAR_SCREEN_START, BAR_SCREEN_TILES * sizeof(u16));
    GFL_HeapFree(file);

    for (i = 0; i < BAR_SCREEN_TILES; i++) {
        dest[BAR_SCREEN_START + i] = (dest[BAR_SCREEN_START + i] & 0xfff) + charBase + (palette << 12);
    }
}

u32 func_0202d890(void) {
    return 0x42;
}

u32 func_0202d894(void) {
    return 0x41;
}

u32 func_0202d898(u32 mapping) {
    return 0x43 + mapping;
}

u32 func_0202d89c(u32 mapping) {
    return 0x46 + mapping;
}

u32 func_0202d8a0(void) {
    return 0x49;
}

u32 func_0202d8a4(void) {
    return 0x4a;
}

u32 func_0202d8a8(u32 mapping) {
    return 0x4b + mapping;
}

u32 func_0202d8ac(u32 mapping) {
    return 0x4e + mapping;
}

u32 func_0202d8b0(void) {
    return 0xb;
}

u32 func_0202d8b4(void) {
    return 0xc;
}

u32 func_0202d8b8(u32 mapping) {
    return 0xd + mapping;
}

u32 func_0202d8bc(u32 mapping) {
    return 0x10 + mapping;
}

u32 AppMenuCommon_GetStatusIcon(PartyPkm *pkm) {
    u32 status;

    if (PokeParty_GetParam(pkm, PKM_PARAM_HP, NULL) == 0) {
        return APP_STATUS_ICON_FAINTED;
    }

    status = PokeParty_GetParam(pkm, PKM_PARAM_STATUS, NULL);
    if (status == CONDITION_PARALYSIS) {
        return APP_STATUS_ICON_PARALYSIS;
    } else if (status == CONDITION_SLEEP) {
        return APP_STATUS_ICON_SLEEP;
    } else if (status == CONDITION_FREEZE) {
        return APP_STATUS_ICON_FREEZE;
    } else if (status == CONDITION_BURN) {
        return APP_STATUS_ICON_BURN;
    } else if (status == CONDITION_POISON) {
        return APP_STATUS_ICON_POISON;
    }
    return APP_STATUS_ICON_NONE;
}

u32 func_0202d90c(u32 mapping) {
    return 0x51 + mapping;
}

u32 func_0202d910(u32 mapping) {
    return 0x54 + mapping;
}

u32 func_0202d914(u32 mapping) {
    return 0xa6 + mapping;
}

u32 func_0202d918(u32 mapping) {
    return 0xa9 + mapping;
}

u32 func_0202d91c(u32 ball) {
    if (ball == 0) {
        ball = 4;
    }
    return 0x56 + ball;
}

u32 func_0202d928(u32 ball) {
    if (ball == 0) {
        ball = 4;
    }
    return 0x6f + ball;
}

u32 func_0202d934(u32 ball, u32 mapping) {
    return 0x89 + mapping;
}

u32 func_0202d93c(u32 ball, u32 mapping) {
    return 0x8c + mapping;
}

u32 func_0202d944(void) {
    return 0x8f;
}

u32 func_0202d948(u32 mapping) {
    return 0x90 + mapping;
}

u32 func_0202d94c(u32 mapping) {
    return 0x93 + mapping;
}

u32 func_0202d950(u32 mapping) {
    return 0x96 + mapping;
}

u32 func_0202d954(void) {
    return 0;
}

u32 func_0202d958(u32 mapping) {
    return 2 + mapping;
}

u32 func_0202d95c(u32 mapping) {
    return 5 + mapping;
}

u32 func_0202d960(u32 mapping) {
    return 8 + mapping;
}

u32 func_0202d964(void) {
    return 0x99;
}

u32 func_0202d968(u32 mapping) {
    return 0x9a + mapping;
}

u32 func_0202d96c(u32 mapping) {
    return 0x9d + mapping;
}

u32 func_0202d970(u32 mapping) {
    return 0xa0 + mapping;
}
