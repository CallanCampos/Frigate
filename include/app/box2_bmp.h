#ifndef POKEBW2_APP_BOX2_BMP_H
#define POKEBW2_APP_BOX2_BMP_H

#include "types.h"
#include "app/box2_main.h"
#include "struct_decls.h"

// The PC box's windows. The ROM doesn't name this file; box2_bmp.c is a guess after box2_main.c. None of these
// functions has a name yet

// Creates and frees the windows
void Box2Bmp_CreateWindows(Box2SysWork *syswk);
void Box2Bmp_FreeWindows(Box2SysWork *syswk);
// Sends the characters of the printed windows to VRAM
void Box2Bmp_FlushChars(Box2AppWork *app);
// Runs the print queue, and sends the characters of the windows it has finished
void Box2Bmp_UpdatePrint(Box2AppWork *app);
void Box2Bmp_PrintTitle(Box2SysWork *syswk);
void Box2Bmp_PrintInfoLabels(Box2SysWork *syswk);
// Shows a Pokémon's data on the upper screen, or clears it
void Box2Bmp_PrintPokeInfo(Box2SysWork *syswk, Box2PokeInfo *info);
void Box2Bmp_ClearPokeInfo(Box2AppWork *app);
// Makes the frames of the menu's buttons and of the buttons on the lower screen
void Box2Bmp_InitMenuFrames(Box2AppWork *app);
void Box2Bmp_InitPartyButton(Box2SysWork *syswk);
void Box2Bmp_InitBoxListButton(Box2SysWork *syswk);
// Shows a box's name on a text object
void Box2Bmp_PutTrayName(Box2SysWork *syswk, u32 tray, u32 index);
void Box2Bmp_FlushTrayName(Box2SysWork *syswk);
void Box2Bmp_DrawScrolledTrayName(Box2SysWork *syswk, u32 mv);
// Shows how many Pokémon a box holds on a text object
void Box2Bmp_PutTrayCount(Box2SysWork *syswk, u32 tray, u32 index);
void Box2Bmp_InitMarkingFrame(Box2SysWork *syswk);
// Opens a menu of items
void Box2Bmp_OpenMenu(Box2SysWork *syswk, const Box2MenuItem *items, u32 count);
void Box2Bmp_ClearMsgWindow(Box2AppWork *app, u32 index);
void Box2Bmp_ClearMsgWindowNow(Box2AppWork *app, u32 index);
// The messages of the message window
void Box2Bmp_PrintContinue(Box2SysWork *syswk, u32 index);
void Box2Bmp_PrintTakeItem(Box2SysWork *syswk, u32 item, u32 index);
void Box2Bmp_PrintMailCantTake(Box2SysWork *syswk, u32 index);
void Box2Bmp_PrintMailCantMove(Box2SysWork *syswk, u32 index);
void Box2Bmp_PrintTookItem(Box2SysWork *syswk, u32 item, u32 index);
void Box2Bmp_PrintBagFull(Box2SysWork *syswk, u32 index);
void Box2Bmp_PrintGiveItem(Box2SysWork *syswk, u32 item, u32 index);
void Box2Bmp_PrintTouchMark(Box2SysWork *syswk, u32 index);
void Box2Bmp_PrintReleaseMsg(Box2SysWork *syswk, u32 type, u32 index);
void Box2Bmp_PrintPrompt(Box2SysWork *syswk, u32 type, u32 index);
void Box2Bmp_PrintPokeMoveMsg(Box2SysWork *syswk, u32 pos, u32 type, u32 index);
void Box2Bmp_PrintItemMsg(Box2SysWork *syswk, u32 type, u32 index);
void Box2Bmp_PrintDenyMsg(Box2SysWork *syswk, u32 type, u32 index);
void Box2Bmp_PrintTuckIn(Box2SysWork *syswk);
void Box2Bmp_PrintBattleBoxLocked(Box2SysWork *syswk, u32 a1);
// Shows how many Pokémon the picked range holds
void Box2Bmp_PutRangeCount(Box2SysWork *syswk);

#endif // POKEBW2_APP_BOX2_BMP_H
