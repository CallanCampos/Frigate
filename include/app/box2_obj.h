#ifndef POKEBW2_APP_BOX2_OBJ_H
#define POKEBW2_APP_BOX2_OBJ_H

#include "types.h"
#include "app/box2_main.h"
#include "struct_decls.h"

// The PC box's actors, in box2_obj.c. Names are ours; none of these functions has a name yet

// The actors, by ID: those of sActorData first, then these groups
#define BOX2_ACTOR_TYPE_ICON 32
#define BOX2_ACTOR_TRAY_ICON 49
#define BOX2_ACTOR_POKEICON 55
#define BOX2_ACTOR_ITEM_OUTLINE 121
#define BOX2_ACTOR_RANGE_DOT 129
#define BOX2_ACTOR_MAX 145
// How many type icons and copies of the item icon that outline it there are
#define BOX2_TYPE_ICON_MAX 17
#define BOX2_ITEM_OUTLINE_MAX 8

// Sets an actor's animation
void Box2Obj_Init(Box2SysWork *syswk);
void Box2Obj_Exit(Box2AppWork *app);
void Box2Obj_UpdateAnim(Box2AppWork *app);
// Sets an actor's animation
void Box2Obj_SetAnim(Box2AppWork *app, u32 id, u32 anim);
void Box2Obj_SetAnimAndPlay(Box2AppWork *app, u32 id, u32 anim);
BOOL Box2Obj_IsAnimActive(Box2AppWork *app, u32 id);
void Box2Obj_SetVisible(Box2AppWork *app, u32 id, BOOL visible);
BOOL Box2Obj_IsVisible(Box2AppWork *app, u32 id);
void Box2Obj_SetPos(Box2AppWork *app, u32 id, s16 x, s16 y, u16 surface);
// Where an actor is
void Box2Obj_GetPos(Box2AppWork *app, u32 id, s16 *x, s16 *y, u16 surface);
void Box2Obj_LoadTrayPokeIcons(Box2SysWork *syswk, u32 tray);
void Box2Obj_SetTrayPokeIcon(Box2SysWork *syswk, u32 tray, u32 pos, u32 id);
void Box2Obj_SetTrayPokeIcons(Box2SysWork *syswk);
void Box2Obj_SetPartyPokeIcons(Box2SysWork *syswk);
// Where an icon at a position is
void Box2Obj_GetIconPos(u32 pos, s16 *x, s16 *y, u32 mode);
void Box2Obj_PutPartyIconsLeftBelow(Box2SysWork *syswk, BOOL hideGet);
void Box2Obj_HidePartyRangeIcons(Box2SysWork *syswk);
void Box2Obj_PutPartyIconsRightBelow(Box2SysWork *syswk);
void Box2Obj_PutPartyIconsLeft(Box2SysWork *syswk);
void Box2Obj_PutPartyIconsRight(Box2SysWork *syswk);
void Box2Obj_SetIconPriority(Box2AppWork *app, u32 iconPos, BOOL put);
void Box2Obj_SetIconPriorityAt(Box2AppWork *app, u32 iconPos, u32 pos, BOOL put);
void Box2Obj_ScrollTrayIcons(Box2SysWork *syswk, s32 mv);
void Box2Obj_MovePartyIconsWithFrame(Box2SysWork *syswk);
void Box2PokeFree_Start(Box2PokeFreeWork *wk);
BOOL Box2PokeFree_Shrink(Box2PokeFreeWork *wk);
BOOL Box2PokeFree_Grow(Box2PokeFreeWork *wk);
void Box2PokeFree_End(Box2PokeFreeWork *wk);
void Box2PokeFree_ResetActor(Box2PokeFreeWork *wk);
void Box2Obj_SetIconsBlend(Box2SysWork *syswk, u32 flags, BOOL blend);
void Box2Obj_SetIconBlend(Box2AppWork *app, u32 iconPos, BOOL blend);
void Box2Obj_PickUpRangeIcons(Box2SysWork *syswk, u32 pos, int width, int height);
void Box2Obj_PutDownRangeIcons(Box2AppWork *app, u32 pos, int width, int height);
void Box2Obj_MoveHeldIcons(Box2SysWork *syswk);
void Box2Obj_UpdatePokeIconPalette(Box2SysWork *syswk, u32 tray, u32 pos);
void Box2Obj_ShowPokeGra(Box2SysWork *syswk, Box2PokeInfo *info, u32 id);
void Box2Obj_PutTypeIcons(Box2AppWork *app, Box2PokeInfo *info);
void Box2Obj_SetItemIcon(Box2AppWork *app, u16 item);
void Box2Obj_SetItemIconAffine(Box2AppWork *app, BOOL affine);
void Box2Obj_SetItemIconPos(Box2AppWork *app, s16 x, s16 y);
void Box2Obj_PutItemIconAtIcon(Box2AppWork *app, u32 pos, u32 mode);
void Box2Obj_PutItemIconAtCursor(Box2AppWork *app, u32 pos, u32 mode);
void Box2Obj_InitItemOutline(Box2AppWork *app);
void Box2Obj_ShowItemOutline(Box2AppWork *app);
void Box2Obj_MoveItemOutline(Box2AppWork *app);
void Box2Obj_SetToolButtons(Box2SysWork *syswk, unsigned int set, BOOL visible);
void Box2Obj_SetCursorAnim(Box2SysWork *syswk, u32 anim);
void Box2Obj_ShowCursor(Box2SysWork *syswk, BOOL show);
void Box2Obj_PutPokeOutline(Box2SysWork *syswk);
void Box2Obj_PutPokeOutlineAt(Box2SysWork *syswk, u32 pos);
void Box2Obj_ShowPokeOutline(Box2SysWork *syswk, BOOL visible);
void Box2Obj_UpdateOutlinePriority(Box2SysWork *syswk, u32 pos);
void Box2Obj_MoveOutlineToIcon(Box2AppWork *app, u32 pos);
void Box2Obj_ShowScrollArrows(Box2AppWork *app, BOOL visible);
void Box2Obj_MoveMarkIcons(Box2SysWork *syswk);
void Box2Obj_ResetBoxList(Box2SysWork *syswk);
void Box2Obj_ScrollBoxListX(Box2SysWork *syswk, s32 mv);
void Box2Obj_HideTrayHighlight(Box2AppWork *app);
void Box2Obj_GetTrayIconPos(Box2AppWork *app, u32 row, s16 *x, s16 *y);
void Box2Obj_MakeTrayIconChar(Box2SysWork *syswk, u32 tray);
void Box2Obj_MakeAllTrayIconChars(Box2SysWork *syswk);
void Box2Obj_RefreshTrayIcon(Box2SysWork *syswk, u32 tray);
void Box2Obj_ScrollBoxListY(Box2SysWork *syswk, s16 mv);
void Box2Obj_StartHighlightScroll(Box2SysWork *syswk, s32 mv);
void Box2Obj_PlaceTrayHighlight(Box2SysWork *syswk);
void Box2Obj_ShowBoxNameFrame(Box2SysWork *syswk, u32 index, BOOL show);
void Box2Obj_SetButtons(Box2SysWork *syswk, u8 button1, u8 button2, u8 button3, u8 button4);
void Box2Obj_HideButtons(Box2SysWork *syswk);
void Box2Obj_SetFontOamVisible(Box2AppWork *app, u32 index, BOOL visible);
BOOL Box2Obj_IsFontOamVisible(Box2AppWork *app, u32 index);
void Box2Obj_PutFontOamOffscreen(Box2AppWork *app, u32 index, u32 dir);
void Box2Obj_ScrollFontOams(Box2AppWork *app, s32 mv);
void Box2Obj_SetRangeAnm(Box2SysWork *syswk, u32 anm);
void Box2Obj_UpdateRangeDots(Box2SysWork *syswk);
void Box2Obj_SetRangeRect(Box2SysWork *syswk, int start, int end, u32 mode);
void Box2Obj_GetRangeSize(int start, int end, u32 *width, u32 *height);
// The top left of a range
u32 Box2Obj_GetTrayRangeStart(int start, int end);
// The top left of a range in the party
u32 Box2Obj_GetPartyRangeStart(int start, int end);
void Box2Obj_ResetRangeIconPriority(Box2AppWork *app, u32 pos, u32 width, u32 height, BOOL put);
void Box2Obj_ShowRangeCount(Box2AppWork *app, BOOL visible);
void Box2Obj_PutRangeCount(Box2AppWork *app, s16 x, s16 y);
void Box2Obj_ShowRangeIcons(Box2SysWork *syswk, BOOL visible);

#endif // POKEBW2_APP_BOX2_OBJ_H
