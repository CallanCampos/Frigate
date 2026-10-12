#include "types.h"
#include "save/shortcut.h"
#include "gfl/std.h"
#include "nitro/math.h"
#include "save/c_gear_savedata.h"
#include "save/save_control.h"
#include "system/shortcut_util.h"

// The shortcuts registered to the Y button, a list without gaps inside the C-Gear's save block. The ROM has no
// string for the file, so the name is a guess; the functions' names are swan's.

#define SAVE_BLOCK_CGEAR 0x20

static void ShortcutSave_Condense(ShortcutSave *shortcutSave);

void ShortcutSave_Init(ShortcutSave *shortcutSave) {
    sys_memset(shortcutSave, SHORTCUT_NONE, sizeof(ShortcutSave));
}

ShortcutSave *SaveControl_GetShortcutSave(SaveControl *save) {
    return SaveControl_GetShortcutSaveCore(save);
}

ShortcutSave *SaveControl_GetShortcutSaveCore(SaveControl *save) {
    CGearSave *cgear = SaveControl_GetBlockPtr(save, SAVE_BLOCK_CGEAR);

    return &cgear->shortcuts;
}

void ShortcutSave_SetKeyItemRegistration(ShortcutSave *shortcutSave, u32 shortcut, BOOL registered) {
    int i;

    if (registered) {
        for (i = 0; i < SHORTCUT_SAVE_MAX; i++) {
            if (shortcutSave->ids[i] == shortcut) {
                return;
            }
            if (shortcutSave->ids[i] == SHORTCUT_NONE) {
                break;
            }
        }
        shortcutSave->ids[i] = shortcut;
    } else {
        for (i = 0; i < SHORTCUT_SAVE_MAX; i++) {
            if (shortcutSave->ids[i] == shortcut) {
                shortcutSave->ids[i] = SHORTCUT_NONE;
            }
        }
    }
    ShortcutSave_Condense(shortcutSave);
}

BOOL ShortcutSave_IsRegistered(ShortcutSave *shortcutSave, u32 shortcut) {
    int i;

    for (i = 0; i < SHORTCUT_SAVE_MAX; i++) {
        if (shortcutSave->ids[i] == shortcut) {
            return TRUE;
        }
    }
    return FALSE;
}

u8 ShortcutSave_GetRegistItem(ShortcutSave *shortcutSave, u32 index) {
    return shortcutSave->ids[index];
}

u16 ShortcutSave_GetShortcutCount(ShortcutSave *shortcutSave) {
    int i;
    int count = 0;

    for (i = 0; i < SHORTCUT_SAVE_MAX; i++) {
        if (shortcutSave->ids[i] != SHORTCUT_NONE) {
            count++;
        }
    }
    return count;
}

u8 ShortcutSave_MoveItemToIndex(ShortcutSave *shortcutSave, u32 shortcut, u8 index) {
    s16 i;
    s16 old;

    for (old = 0; old < SHORTCUT_SAVE_MAX; old++) {
        if (shortcutSave->ids[old] == shortcut) {
            break;
        }
    }
    ShortcutSave_SetKeyItemRegistration(shortcutSave, shortcut, FALSE);
    if (index == SHORTCUT_NONE) {
        for (index = 0; index < SHORTCUT_SAVE_MAX; index++) {
            if (shortcutSave->ids[index] == SHORTCUT_NONE) {
                shortcutSave->ids[index] = shortcut;
                break;
            }
        }
    } else {
        if (old < index) {
            index--;
        }
        for (i = SHORTCUT_SAVE_MAX - 1; i >= index; i--) {
            shortcutSave->ids[i] = shortcutSave->ids[i - 1];
        }
        shortcutSave->ids[index] = shortcut;
    }
    return index;
}

void func_0200cc34(ShortcutSave *shortcutSave, u32 shortcut) {
    int i;
    u32 other;

    if (shortcut == 7) {
        other = 8;
    } else if (shortcut == 8) {
        other = 7;
    } else {
        return;
    }
    for (i = 0; i < SHORTCUT_SAVE_MAX; i++) {
        if (shortcutSave->ids[i] == shortcut) {
            shortcutSave->ids[i] = other;
        }
    }
}

static void ShortcutSave_Condense(ShortcutSave *shortcutSave) {
    u16 i;

    for (i = 0; i < SHORTCUT_SAVE_MAX; i++) {
        u16 next = MATH_CLAMP(i + 1, 1, SHORTCUT_SAVE_MAX - 1);
        u8 id = shortcutSave->ids[i];

        if (id == SHORTCUT_NONE) {
            shortcutSave->ids[i] = shortcutSave->ids[next];
            shortcutSave->ids[next] = id;
        }
    }
}
