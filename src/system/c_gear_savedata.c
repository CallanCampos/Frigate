#include "types.h"
#include "gfl/std.h"
#include "save/c_gear_savedata.h"
#include "save/save_control.h"
#include "save/shortcut.h"
#include "system/game_data.h"
#include "system/pms.h"

// The C-Gear's save block, which the C-Gear's phrases, the shortcuts and a name share, and the C-Gear's own
// settings in it: the order of its panels, its colors and its downloaded skin. The ROM has no string for the file,
// so the name is a guess (swan's getCGearDataBlkAddress). Block 0x20.

#define SAVE_BLOCK_CGEAR 0x20

u32 CGearSave_GetSize(void) {
    return sizeof(CGearSave);
}

void CGearSave_Init(CGearSave *cgear) {
    MI_CpuFill8(cgear, 0, CGearSave_GetSize());
    CGearPhrases_Init(cgear->phrases);
    CGearSettings_Init(&cgear->settings);
    ShortcutSave_Init(&cgear->shortcuts);
    CGearName_Init(cgear->name);
}

u32 CGearSettings_GetSize(void) {
    return sizeof(CGearSettings);
}

void CGearSettings_Init(CGearSettings *settings) {
    s32 i;

    MI_CpuFill8(settings, 0, CGearSettings_GetSize());
    for (i = 0; i < CGEAR_PANEL_MAX; i++) {
        settings->panels[i] = 0;
    }
    settings->colors[0] = 12;
    settings->colors[1] = 21;
    settings->colors[2] = 9;
}

void CGearSettings_SetOn(CGearSettings *settings, u8 on) {
    settings->on = on;
}

u8 CGearSettings_IsOn(CGearSettings *settings) {
    return settings->on;
}

void func_020098c4(CGearSettings *settings, u8 value) {
    settings->skin = value;
}

u8 func_020098c8(CGearSettings *settings) {
    return settings->skin;
}

void CGearSettings_SetSkinValid(CGearSettings *settings, u8 valid) {
    settings->skinValid = valid;
}

u8 CGearSettings_GetSkinValid(CGearSettings *settings) {
    return settings->skinValid;
}

void CGearSettings_SetSkinCrc(CGearSettings *settings, u16 crc) {
    settings->skinCrc = crc;
}

u16 CGearSettings_GetSkinCrc(CGearSettings *settings) {
    return settings->skinCrc;
}

void CGearSettings_SetPanel(CGearSettings *settings, u32 place, u32 panel) {
    if (place < CGEAR_PANEL_MAX && panel < CGEAR_PANEL_MAX) {
        settings->panels[place] = panel;
    }
}

u8 CGearSettings_GetPanel(CGearSettings *settings, u32 place) {
    if (place >= CGEAR_PANEL_MAX) {
        return 0;
    }
    return settings->panels[place];
}

void CGearSettings_SetColor(CGearSettings *settings, s32 index, u8 color) {
    if (index < CGEAR_COLOR_MAX) {
        settings->colors[index] = color;
    }
}

u8 CGearSettings_GetColor(CGearSettings *settings, s32 index) {
    if (index >= CGEAR_COLOR_MAX) {
        return 0;
    }
    return settings->colors[index];
}

void func_02009910(CGearSettings *settings, u32 value) {
    settings->wasOn = value;
}

u8 func_02009914(CGearSettings *settings) {
    return settings->wasOn;
}

CGearSettings *CGearSettings_GetFromSave(SaveControl *save) {
    CGearSave *cgear = SaveControl_GetBlockPtr(save, SAVE_BLOCK_CGEAR);
    return &cgear->settings;
}

CGearSettings *CGearSettings_GetFromGameData(GameData *gameData) {
    return CGearSettings_GetFromSave(GameData_GetSaveControl(gameData));
}
