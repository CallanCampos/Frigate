#ifndef POKEBW2_FIELD_SUBSCREEN_H
#define POKEBW2_FIELD_SUBSCREEN_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The field touch screen's screens, swan's FieldSubscreenID
enum FieldSubscreenID {
    FLD_SUBSCREEN_ID_DEFAULT,
    FLD_SUBSCREEN_ID_FIELD_MENU,
    FLD_SUBSCREEN_ID_UNION_ROOM_STANDBY,
    FLD_SUBSCREEN_ID_BLANK_STANDBY,
    FLD_SUBSCREEN_ID_ENTRALINK,
    FLD_SUBSCREEN_ID_BLANK_STANDBY_2,
    FLD_SUBSCREEN_ID_DOWSING,
    FLD_SUBSCREEN_ID_SAVE_REPORT,
    FLD_SUBSCREEN_ID_CGEAR_WELCOME,
    FLD_SUBSCREEN_ID_CGEAR_POWEROFF,
    FLD_SUBSCREEN_ID_FUNFEST,
    FLD_SUBSCREEN_ID_PWT,
    // inf_dungeon_retire.c's menu for leaving the dungeon, which swan calls FLD_SUBSCREEN_ID_12
    FLD_SUBSCREEN_ID_INF_DUNGEON_RETIRE,
    FLD_SUBSCREEN_ID_MAX,
};

void FieldSubscreen_ReqChange(FieldSubscreen *subscreen, u32 mode);
// Changes the subscreen, calling the callback with work once it has changed
void FieldSubscreen_ReqChangeEx(FieldSubscreen *subscreen, u32 mode, void (*callback)(void *work), void *work);
u32 FieldSubscreen_GetIDForChange(FieldSubscreen *subscreen, u32 param);
u32 FieldSubscreen_GetScreenID(FieldSubscreen *subscreen);
u32 FieldSubscreen_GetReturnSubscreen(FieldSubscreen *subscreen);
// FieldSubscreen_SaveReturnSubscreen with FLD_SUBSCREEN_ID_DEFAULT
void FieldSubscreen_ResetReturnSubscreen(FieldSubscreen *subscreen);
void FieldSubscreen_SetMenuSelectedButton(FieldSubscreen *subscreen, u32 param);
BOOL FieldSubscreen_IsReady(FieldSubscreen *subscreen);
// The report's subscreen
void FieldSubscreen_ResumeField(FieldSubscreen *subscreen, HeapID heapId);
// Sets the subscreen to change to once the field menu closes
void FieldSubscreen_SaveReturnSubscreen(FieldSubscreen *subscreen, u32 screenId);
void FieldSubscreen_LoadBackdrop(FieldSubscreen *subscreen, HeapID heapId);
BOOL func_ov036_02198b04(FieldSubscreen *subscreen);
void func_ov036_02198b10(FieldSubscreen *subscreen);
void func_ov036_02198b1c(FieldSubscreen *subscreen);
BOOL func_ov036_02198b28(FieldSubscreen *subscreen);
void func_ov036_02198b34(FieldSubscreen *subscreen);
BOOL func_ov036_02198b40(FieldSubscreen *subscreen);
u32 FieldSubscreen_GetMenuSelectedButton(FieldSubscreen *subscreen);
// heapId is a u32, not a HeapID: the original keeps the whole word and only truncates it to 16 bits where it passes it
// to GFL_HeapAllocate and stores it
FieldSubscreen *FieldSubscreen_Create(u32 heapId, Field *field, u32 mode);
u8 FieldSubscreen_Free(FieldSubscreen *subscreen);
void FieldSubscreen_Update(FieldSubscreen *subscreen);
void FieldSubscreen_UpdateAltFrame(FieldSubscreen *subscreen);
// The current screen's event, or NULL, while no change is under way
GameEvent *FieldSubscreen_CheckGameEvents(FieldSubscreen *subscreen, BOOL enabled);
// The C-Gear while it is the current screen, or NULL
CGearSw *FieldSubscreen_GetCGear(FieldSubscreen *subscreen);

#endif // POKEBW2_FIELD_SUBSCREEN_H
