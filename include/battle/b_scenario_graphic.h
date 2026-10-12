#ifndef POKEBW2_BATTLE_B_SCENARIO_GRAPHIC_H
#define POKEBW2_BATTLE_B_SCENARIO_GRAPHIC_H

#include "types.h"
#include "battle/b_scenario_main.h"
#include "struct_decls.h"

// The scenario screen's graphics, windows, touch rectangles and actors (b_scenario_graphic.c, a guessed name).
// Every name here is ours.

void BScenarioGraphic_LoadBG(BScenarioWork *work);
// Scrolls the pages' BG to a page
void BScenarioGraphic_ScrollBG(BScenarioWork *work, int page);
void BScenarioGraphic_CreateWindows(BScenarioWork *work, int page);
// Prints in the screen's font, or the small one; keep leaves what the window holds
void BScenarioGraphic_Print(BScenarioWork *work, u32 window, int x, int y, StrBuf *str, u32 align, u16 color,
                            BOOL smallFont, BOOL keep);
void BScenarioGraphic_PrintNumber(BScenarioWork *work, u32 window, int x, int y, int number, u32 digits, u32 align,
                                  BOOL smallFont);
void BScenarioGraphic_FreeWindows(BScenarioWork *work);
void BScenarioGraphic_ClearWindow(BScenarioWork *work, u32 window);
void BScenarioGraphic_UpdatePrint(BScenarioWork *work);
// The index of the page's rectangle that is touched, or a negative value
s32 BScenarioGraphic_GetTouched(BScenarioWork *work, int page);
void BScenarioGraphic_SetTypeIcon(BScenarioWork *work, int index, u32 unused, u32 type);
void BScenarioGraphic_SetIconPalette(BScenarioWork *work, int index, u32 species, u32 sex);
void BScenarioGraphic_InitActors(BScenarioWork *work);
void BScenarioGraphic_FreeActors(BScenarioWork *work);
void BScenarioGraphic_SetActorVisible(BScenarioWork *work, int group, int index, BOOL visible);
void BScenarioGraphic_UpdateCursor(BScenarioWork *work, BOOL show, int page, int cursor);
void BScenarioGraphic_SetButtonsVisible(BScenarioWork *work, int page, BOOL unused);
void BScenarioGraphic_SetButtonAnim(BScenarioWork *work, int button, u32 sequence);
BOOL BScenarioGraphic_IsButtonAnimating(BScenarioWork *work, int button);
// Flashes one of a page's choices
void BScenarioGraphic_StartFlash(BScenarioWork *work, int button);
void BScenarioGraphic_UpdateFlash(BScenarioWork *work);
BOOL BScenarioGraphic_IsFlashing(BScenarioWork *work);
void BScenarioGraphic_SetPlateVisible(BScenarioWork *work, int page);
// Puts the marker on one of the script page's two lines
void BScenarioGraphic_SetMarker(BScenarioWork *work, int line, u32 unused, BOOL visible);

#endif // POKEBW2_BATTLE_B_SCENARIO_GRAPHIC_H
