#ifndef POKEBW2_SAVE_SHORTCUT_H
#define POKEBW2_SAVE_SHORTCUT_H

#include "types.h"
#include "struct_decls.h"

// The shortcuts registered to the Y button (shortcut_savedata.c, a guessed name), inside the C-Gear's save block.
// The functions' names are swan's

// How many of the slots are used
#define SHORTCUT_SAVE_MAX 0x22

// The registered shortcuts, without gaps, SHORTCUT_NONE when a slot is empty
struct ShortcutSave {
    u8 ids[0x24];
}; // size: 0x24

void ShortcutSave_Init(ShortcutSave *shortcutSave);
ShortcutSave *SaveControl_GetShortcutSave(SaveControl *save);
// The same block, which SaveControl_GetShortcutSave returns through it
ShortcutSave *SaveControl_GetShortcutSaveCore(SaveControl *save);
// Registers a shortcut in the first free slot, or removes it
void ShortcutSave_SetKeyItemRegistration(ShortcutSave *shortcutSave, u32 shortcut, BOOL registered);
BOOL ShortcutSave_IsRegistered(ShortcutSave *shortcutSave, u32 shortcut);
u8 ShortcutSave_GetRegistItem(ShortcutSave *shortcutSave, u32 index);
u16 ShortcutSave_GetShortcutCount(ShortcutSave *shortcutSave);
// Moves a shortcut to a slot, or to the first free one when index is SHORTCUT_NONE. Returns the slot
u8 ShortcutSave_MoveItemToIndex(ShortcutSave *shortcutSave, u32 shortcut, u8 index);
// Replaces shortcut 7 (DNA Splicers that fuse) with 8 (that separate), or 8 with 7
void func_0200cc34(ShortcutSave *shortcutSave, u32 shortcut);

#endif // POKEBW2_SAVE_SHORTCUT_H
