#ifndef POKEBW2_FIELD_SHORTCUT_MENU_H
#define POKEBW2_FIELD_SHORTCUT_MENU_H

#include "types.h"
#include "app/name_entry.h"
#include "field/app_call.h"
#include "struct_decls.h"
#include "system/game_event.h"
#include "system/shortcut_util.h"

struct ShortcutMenuWork {
    GameEvent *event;
    GameSystem *gameSystem;
    Field *field;
    ShortcutMenuApp *app;
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

// How the menu starts: closed, to slide in, or open, as it comes back from what it called
enum {
    SHORTCUT_MENU_START_CLOSED,
    SHORTCUT_MENU_START_OPEN,
};
ShortcutMenuApp *ShortcutMenu_Create(GameData *gameData, u32 mode, ShortcutMenuPos *listPos, HeapID menuHeapId,
                                     HeapID heapId);
void ShortcutMenu_Delete(ShortcutMenuApp *app);
u32 ShortcutMenu_RunPrint(ShortcutMenuApp *app);
void ShortcutMenu_Main(ShortcutMenuApp *app);
void ShortcutMenu_Start(ShortcutMenuApp *app);
void ShortcutMenu_RequestClose(ShortcutMenuApp *app);
BOOL ShortcutMenu_IsWaitingInput(ShortcutMenuApp *app);
u32 ShortcutMenu_GetResult(ShortcutMenuApp *app, u32 *item);
// Puts the field back through a subprocess of overlay 0x118, then finishes
GameEvent *ReturnToField_CreateNameInputEvent(GameSystem *gsys, Field *field, NameEntryParam *param, BOOL transition);

#endif // POKEBW2_FIELD_SHORTCUT_MENU_H
