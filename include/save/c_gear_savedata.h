#ifndef POKEBW2_SAVE_C_GEAR_SAVEDATA_H
#define POKEBW2_SAVE_C_GEAR_SAVEDATA_H

#include "types.h"
#include "save/shortcut.h"
#include "struct_decls.h"
#include "system/pms_data.h"

#define CGEAR_PHRASE_MAX 5
#define CGEAR_PANEL_MAX 6
#define CGEAR_COLOR_MAX 3

// The C-Gear's settings, in the middle of its save block (CGearSettings_GetFromSave)
struct CGearSettings {
    // Which panel sits in each of the six places
    u8 panels[CGEAR_PANEL_MAX];
    u8 unk06;
    // Whether the C-Gear is on, which the start menu reads to know if it was on when the game was saved
    u8 on;
    u8 colors[CGEAR_COLOR_MAX];
    u8 unk0B;
    // The CRC of the downloaded skin, and whether there is one
    u16 skinCrc;
    u8 skinValid;
    u8 padding0F[0x25];
}; // size: 0x34

// Block 0x20 (swan's getCGearDataBlkAddress): the C-Gear's phrases, its settings, the shortcuts and a name
struct CGearSave {
    PMSData phrases[CGEAR_PHRASE_MAX];
    CGearSettings settings;
    ShortcutSave shortcuts;
    u16 name[9];
}; // size: 0x92

u32 CGearSave_GetSize(void);
void CGearSave_Init(CGearSave *cgear);
u32 CGearSettings_GetSize(void);
void CGearSettings_Init(CGearSettings *settings);
void CGearSettings_SetOn(CGearSettings *settings, u8 on);
u8 CGearSettings_IsOn(CGearSettings *settings);
void func_020098c4(CGearSettings *settings, u8 value);
u8 func_020098c8(CGearSettings *settings);
void CGearSettings_SetSkinValid(CGearSettings *settings, u8 valid);
u8 CGearSettings_GetSkinValid(CGearSettings *settings);
void CGearSettings_SetSkinCrc(CGearSettings *settings, u16 crc);
u16 CGearSettings_GetSkinCrc(CGearSettings *settings);
void CGearSettings_SetPanel(CGearSettings *settings, u32 place, u32 panel);
u8 CGearSettings_GetPanel(CGearSettings *settings, u32 place);
void CGearSettings_SetColor(CGearSettings *settings, s32 index, u8 color);
u8 CGearSettings_GetColor(CGearSettings *settings, s32 index);
void func_02009910(CGearSettings *settings, u8 value);
u8 func_02009914(CGearSettings *settings);
CGearSettings *CGearSettings_GetFromSave(SaveControl *save);
CGearSettings *CGearSettings_GetFromGameData(GameData *gameData);

#endif // POKEBW2_SAVE_C_GEAR_SAVEDATA_H
