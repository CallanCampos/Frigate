#ifndef POKEBW2_APP_BOX2_UI_H
#define POKEBW2_APP_BOX2_UI_H

#include "types.h"
#include "struct_decls.h"

// The PC box's cursor and touch handling, in box2_ui.c. None of these functions has a name yet

// What Box2Ui_UpdateCursor3 returns besides a position
#define CURSORMOVE_NONE 0xffffffff
#define CURSORMOVE_CANCEL 0xfffffffe
#define CURSORMOVE_CURSOR_MOVE 0xfffffffd
#define CURSORMOVE_CURSOR_ON 0xfffffffc
#define CURSORMOVE_SCROLL_R 0xfffffffb
#define CURSORMOVE_SCROLL_L 0xfffffffa
#define CURSORMOVE_UNK_7 0xfffffff9
#define CURSORMOVE_UNK_8 0xfffffff8

void Box2Ui_CreateCursor(Box2SysWork *syswk);
void Box2Ui_ChangeCursor(Box2SysWork *syswk, u32 a1, u32 pos);
void Box2Ui_PlaceCursorActor(Box2SysWork *syswk, u32 pos);
void Box2Ui_DeleteCursor(Box2SysWork *syswk);
u32 Box2Ui_UpdateCursor0(Box2SysWork *syswk);
void Box2Ui_StartMoveToPartyPos(Box2SysWork *syswk, u32 pos);
void Box2Ui_StartMoveToCursor1(Box2SysWork *syswk, u32 pos);
u32 Box2Ui_UpdateCursor2(Box2SysWork *syswk);
u32 Box2Ui_UpdateCursor3(Box2SysWork *syswk);
// The party's version of Box2Ui_UpdateCursor3
u32 Box2Ui_UpdateCursor5(Box2SysWork *syswk);
void Box2Ui_SetButtonStates(Box2SysWork *syswk, u32 pos);
u32 Box2Ui_UpdateCursor4(Box2SysWork *syswk);
u32 Box2Ui_UpdateCursor6(Box2SysWork *syswk);
u32 Box2Ui_UpdateCursor7(Box2SysWork *syswk);
u32 Box2Ui_UpdateCursor8(Box2SysWork *syswk);
u32 Box2Ui_UpdateCursor13(Box2SysWork *syswk);
u32 Box2Ui_UpdateCursor14(Box2SysWork *syswk);
u32 Box2Ui_UpdateCursor15(Box2SysWork *syswk);
void Box2Ui_StartCursorMove(Box2AppWork *app, u32 pos, u32 curPos);
// The position at a touch
u32 Box2Ui_GetTrayPokeAt(u32 x, u32 y);
u32 Box2Ui_GetTrayPokeTrigger(void);
u32 Box2Ui_GetTrayPokeHeld(void);
u32 Box2Ui_GetMenuButtonTrigger(void);
u32 Box2Ui_GetPartyPokeTrigger(void);
u32 Box2Ui_GetPartyPokeHeld(void);
u32 Box2Ui_GetTrayArrowHeld(void);
u32 Box2Ui_GetBoxListArrowHeld(void);
BOOL Box2Ui_GetBoxListScrollTouch(u32 *x, u32 *y);
BOOL Box2Ui_GetBoxListTouch(u32 *x, u32 *y);
int Box2Ui_GetBoxListRowAt(u32 x, u32 y);
// The position of the box list at a touch, or BOX2_GET_NONE
u32 Box2Ui_GetBoxListPosAt(u32 x, u32 y);
BOOL Box2Ui_IsPartyLeftHeld(void);
BOOL Box2Ui_IsPartyRightHeld(void);
u32 Box2Ui_GetSwitchButtonHeld(void);
BOOL Box2Ui_IsTrayHeld(void);

#endif // POKEBW2_APP_BOX2_UI_H
