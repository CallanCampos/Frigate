#ifndef POKEBW2_APP_BOX2_BMP_H
#define POKEBW2_APP_BOX2_BMP_H

#include "types.h"
#include "app/box2_main.h"
#include "struct_decls.h"

// The PC box's windows. The ROM doesn't name this file; box2_bmp.c is a guess after box2_main.c. None of these
// functions has a name yet

// The messages of Box2Bmp_PrintReleaseMsg
enum {
    BOX2_RELEASE_MSG_ASK,
    BOX2_RELEASE_MSG_RELEASED,
    BOX2_RELEASE_MSG_BYE_BYE,
    BOX2_RELEASE_MSG_EGG,
    BOX2_RELEASE_MSG_CAME_BACK,
    BOX2_RELEASE_MSG_WORRIED,
    BOX2_RELEASE_MSG_LAST_POKEMON,
    BOX2_RELEASE_MSG_FUSED_KYUREM,
};

// The prompts of Box2Bmp_PrintPrompt
enum {
    BOX2_PROMPT_WHAT_TO_DO,
    BOX2_PROMPT_JUMP_TO_BOX,
    BOX2_PROMPT_THEME,
    BOX2_PROMPT_WALLPAPER,
};

// The messages of Box2Bmp_PrintPokeMoveMsg
enum {
    BOX2_POKEMOVE_MSG_WHICH_TO_TAKE,
    BOX2_POKEMOVE_MSG_SELECTED,
    BOX2_POKEMOVE_MSG_PARTY_FULL,
    BOX2_POKEMOVE_MSG_WHICH_BOX,
    BOX2_POKEMOVE_MSG_BOX_FULL,
    BOX2_POKEMOVE_MSG_REMOVE_MAIL,
};

// The messages of Box2Bmp_PrintItemMsg
enum {
    BOX2_ITEM_MSG_CAN_HOLD,
    BOX2_ITEM_MSG_SELECTED,
    BOX2_ITEM_MSG_PUT_AWAY,
    BOX2_ITEM_MSG_PLACED_IN_BAG,
    BOX2_ITEM_MSG_MAIL_CANT_TAKE,
    BOX2_ITEM_MSG_EGG_CANT_HOLD,
    BOX2_ITEM_MSG_BAG_FULL,
    BOX2_ITEM_MSG_MAIL_CANT_MOVE,
};

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
// Tells why a Pokémon can't be moved or picked: type is BOX2_MOVE_ERR_MAIL, _LAST_BATTLER, _EGG or _NOT_SLEEPY
void Box2Bmp_PrintDenyMsg(Box2SysWork *syswk, u32 type, u32 index);
void Box2Bmp_PrintTuckIn(Box2SysWork *syswk);
// Says that the locked Battle Box's Pokémon can't be moved, or its items given or taken
void Box2Bmp_PrintBattleBoxLocked(Box2SysWork *syswk, BOOL items);
// Shows how many Pokémon the picked range holds
void Box2Bmp_PutRangeCount(Box2SysWork *syswk);

#endif // POKEBW2_APP_BOX2_BMP_H
