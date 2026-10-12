#include "types.h"
#include "save/c_gear_savedata.h"
#include "gfl/std.h"
#include "save/save_control.h"
#include "save/shortcut.h"
#include "system/game_data.h"
#include "system/pms.h"

// The C-Gear's save block, which the C-Gear's phrases, the shortcuts and a name share, and the C-Gear's own
// settings in it: the order of its panels, its colors and its downloaded skin. The ROM has no string for the file,
// so the name is a guess (swan's getCGearDataBlkAddress). Block 0x20.

#define SAVE_BLOCK_CGEAR 0x20

u32 func_02009854(void) {
    return sizeof(CGearSave);
}

void func_02009858(CGearSave *cgear) {
    sys_memset(cgear, 0, func_02009854());
    CGearPhrases_Init(cgear->phrases);
    func_02009890(&cgear->settings);
    ShortcutSave_Init(&cgear->shortcuts);
    func_020114a8(cgear->name);
}

u32 func_0200988c(void) {
    return sizeof(CGearSettings);
}

void func_02009890(CGearSettings *settings) {
    s32 i;

    sys_memset(settings, 0, func_0200988c());
    for (i = 0; i < CGEAR_PANEL_MAX; i++) {
        settings->panels[i] = 0;
    }
    settings->colors[0] = 12;
    settings->colors[1] = 21;
    settings->colors[2] = 9;
}

void func_020098bc(CGearSettings *settings, u8 on) {
    settings->on = on;
}

u8 func_020098c0(CGearSettings *settings) {
    return settings->on;
}

void func_020098c4(CGearSettings *settings, u8 value) {
    settings->unk0B = value;
}

u8 func_020098c8(CGearSettings *settings) {
    return settings->unk0B;
}

void func_020098cc(CGearSettings *settings, u8 valid) {
    settings->skinValid = valid;
}

u8 func_020098d0(CGearSettings *settings) {
    return settings->skinValid;
}

void func_020098d4(CGearSettings *settings, u16 crc) {
    settings->skinCrc = crc;
}

u16 func_020098d8(CGearSettings *settings) {
    return settings->skinCrc;
}

void func_020098dc(CGearSettings *settings, u32 place, u32 panel) {
    if (place < CGEAR_PANEL_MAX && panel < CGEAR_PANEL_MAX) {
        settings->panels[place] = panel;
    }
}

u8 func_020098e8(CGearSettings *settings, u32 place) {
    if (place >= CGEAR_PANEL_MAX) {
        return 0;
    }
    return settings->panels[place];
}

void func_020098f4(CGearSettings *settings, s32 index, u8 color) {
    if (index < CGEAR_COLOR_MAX) {
        settings->colors[index] = color;
    }
}

u8 func_02009900(CGearSettings *settings, s32 index) {
    if (index >= CGEAR_COLOR_MAX) {
        return 0;
    }
    return settings->colors[index];
}

void func_02009910(CGearSettings *settings, u8 value) {
    settings->unk06 = value;
}

u8 func_02009914(CGearSettings *settings) {
    return settings->unk06;
}

CGearSettings *func_02009918(SaveControl *save) {
    CGearSave *cgear = SaveControl_GetBlockPtr(save, SAVE_BLOCK_CGEAR);
    return &cgear->settings;
}

CGearSettings *func_02009924(GameData *gameData) {
    return func_02009918(GameData_GetSaveControl(gameData));
}
