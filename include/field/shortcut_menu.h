#ifndef POKEBW2_FIELD_SHORTCUT_MENU_H
#define POKEBW2_FIELD_SHORTCUT_MENU_H

#include "types.h"
#include "field/app_call.h"
#include "struct_decls.h"
#include "system/game_event.h"

struct ShortcutMenuWork {
    GameEvent *event;
    GameSystem *gameSystem;
    Field *field;
    void *app;
    // The shortcut's app, opened through EventFieldAppCall_Create
    FieldAppCallInput *input;
    u16 code;
    u32 done;
    u32 state;
    // Nonzero if the shortcut's action can't be used here, which EventFieldItemUseBlock_Call tells the player
    s32 blocked;
};

BOOL IsExistAnyYShortcut(GameSystem *gsys);
BOOL CheckAnyRibbon(ShortcutMenuWork *work);
u32 ShortcutMenu_SetKeyItemID(FieldAppCallInput *input, u32 item);
BOOL ShortcutMenu_GetActionFromKeyItem(s32 item, u32 *action, u32 *invalid);
void ShortcutMenu_Open(u32 arg0, ShortcutMenuWork *work);
BOOL ShortcutMenu_Close(ShortcutMenuWork *work);
BOOL ShortcutMenu_CanRetry(FieldAppCallInput *input, void *arg);
BOOL ShortcutMenu_CanLeaveField(FieldAppCallInput *input, void *arg);
GameEvent *CallYButtonShortcutMenu(GameSystem *gsys, Field *field, u16 code);
GameEventReturnCode EventShortcutChoicePopup_Callback(GameEvent *event, u32 *state, void *data);
GameEventReturnCode EventShortcutCallDirect_Callback(GameEvent *event, u32 *state, void *data);

void *func_ov036_021bedd8(GameData *gameData, u32 mode, u16 *listPos, HeapID heapId, u32 unused);
void func_ov036_021bf004(void *app);
u32 func_ov036_021bf198(void *app);
void func_ov036_021bf088(void *app);
void func_ov036_021bf1ac(void *app);
void func_ov036_021bf1d0(void *app);
BOOL func_ov036_021bf1e4(void *app);
u32 func_ov036_021bf1f8(void *app, u32 *item);
// Puts the field back through a subprocess of overlay 0x118, then finishes
GameEvent *func_ov036_021bf890(GameSystem *gsys, Field *field, void *param, BOOL transition);

#endif // POKEBW2_FIELD_SHORTCUT_MENU_H
