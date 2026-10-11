#ifndef POKEBW2_APP_UI_YESNO_MENU_H
#define POKEBW2_APP_UI_YESNO_MENU_H

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "app/ui/ui_scene.h"
#include "struct_decls.h"
#include "system/printsys.h"

// yesno_menu.c: the menu of two choices with which the evolution demo asks about learning a move
typedef struct TwoChoiceMenu TwoChoiceMenu;

// What TwoChoiceMenu_Main steps through
#define TWO_CHOICE_MENU_STATE_CLOSED 0
#define TWO_CHOICE_MENU_STATE_OPENING 1
#define TWO_CHOICE_MENU_STATE_CHOOSING 2
#define TWO_CHOICE_MENU_STATE_FLASHING 3
#define TWO_CHOICE_MENU_STATE_DONE 4

#define TWO_CHOICE_MENU_ACTORS 4
#define TWO_CHOICE_MENU_FLASH_STEPS 10

struct TwoChoiceMenu {
    u16 heapId;
    ClActUnit *unit;
    Font *font;
    PrintQueue *queue;
    // The type of VRAM of the BGs and their palettes: 0 for the main screen, 4 for the sub screen
    u32 palType;
    // The BG of the frames' art and the BG of the text, then their priorities, which are one above and equal to
    // the ones given
    u8 frameBg;
    u8 framePriority;
    u8 framePalette;
    u8 framePalette2;
    u8 textBg;
    u8 actorPriority;
    u8 textPalette;
    BOOL isSub;
    u8 objPalette;
    // The three palettes of the art that the choice flashes between
    u16 palettes[3][16];
    u32 frameChars;
    BmpWin *firstWindow;
    BmpWin *secondWindow;
    // Whether the text of each window has been sent
    BOOL firstSent;
    BOOL secondSent;
    u32 state;
    u32 result;
    ClActor *actors[TWO_CHOICE_MENU_ACTORS];
    UIObjRes objRes;
    // The choice that the cursor is on
    u32 cursor;
    u8 flashPalette;
    u16 flashStep;
    u16 flashTimer;
};

// What TwoChoiceMenu_GetResult returns
#define TWO_CHOICE_MENU_SECOND 0
#define TWO_CHOICE_MENU_FIRST 1
#define TWO_CHOICE_MENU_NONE 2

// textBg is the BG of the text and priority its priority; the art is on the BG above it. Sub screen BGs are above 3
TwoChoiceMenu *TwoChoiceMenu_Create(HeapID heapId, u32 textBg, u32 priority, u32 palette, u8 objPalette,
                                   ClActUnit *unit, Font *font, PrintQueue *queue, u32 a8);
void TwoChoiceMenu_Free(TwoChoiceMenu *menu);
// Opens the menu with the two choices
void TwoChoiceMenu_Open(TwoChoiceMenu *menu, StrBuf *first, StrBuf *second);
void TwoChoiceMenu_Close(TwoChoiceMenu *menu);
void TwoChoiceMenu_Main(TwoChoiceMenu *menu);
u32 TwoChoiceMenu_GetResult(TwoChoiceMenu *menu);

#endif // POKEBW2_APP_UI_YESNO_MENU_H
