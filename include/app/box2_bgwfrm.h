#ifndef POKEBW2_APP_BOX2_BGWFRM_H
#define POKEBW2_APP_BOX2_BGWFRM_H

#include "types.h"
#include "struct_decls.h"

// The PC box's sliding frames, over bgwinfrm.c. The ROM doesn't name this file; box2_bgwfrm.c is a guess. None of
// these functions has a name yet

void Box2Frame_Init(Box2SysWork *syswk);
void Box2Frame_Delete(Box2AppWork *app);
void Box2Frame_PutPartyLeft(BGWinFrame *frames);
void Box2Frame_PutPartyRight(BGWinFrame *frames);
void Box2Frame_PutPartyLeftBelow(BGWinFrame *frames);
void Box2Frame_PutPartyRightBelow(BGWinFrame *frames);
void Box2Frame_SlidePartyUp(BGWinFrame *frames);
void Box2Frame_SlidePartyDown(BGWinFrame *frames);
void Box2Frame_SlidePartyRight(BGWinFrame *frames);
void Box2Frame_SlidePartyLeft(BGWinFrame *frames);
BOOL Box2Frame_PartyMoveStep(Box2SysWork *syswk);
BOOL Box2Frame_IsPartyAtRight(BGWinFrame *frames);
BOOL Box2Frame_IsPartyAtLeft(BGWinFrame *frames);
BOOL Box2Frame_IsPartyUp(BGWinFrame *frames);
void Box2Frame_ShowMenu(BGWinFrame *frames);
void Box2Frame_HideMenu(BGWinFrame *frames);
void Box2Frame_SlideMenuIn(BGWinFrame *frames);
void Box2Frame_SlideMenuOut(BGWinFrame *frames);
BOOL Box2Frame_MenuMoveStep(BGWinFrame *frames);
BOOL Box2Frame_IsMenuOnScreen(BGWinFrame *frames);
BOOL Box2Frame_IsMenuIn(BGWinFrame *frames);
void Box2Frame_PutMessage(BGWinFrame *frames);
void Box2Frame_ShowPartyButton(Box2AppWork *app);
void Box2Frame_HidePartyButton(Box2AppWork *app);
void Box2Frame_ShowBoxListButton(Box2AppWork *app);
void Box2Frame_HideBoxListButton(Box2AppWork *app);
void Box2Frame_SlideMarkingIn(BGWinFrame *frames);
void Box2Frame_SlideMarkingOut(BGWinFrame *frames);
BOOL Box2Frame_MarkingMoveStep(Box2SysWork *syswk);
void Box2Frame_SlideBoxListIn(BGWinFrame *frames);
void Box2Frame_SlideBoxListOut(BGWinFrame *frames);
BOOL Box2Frame_IsBoxListIn(BGWinFrame *frames);

#endif // POKEBW2_APP_BOX2_BGWFRM_H
