#ifndef POKEBW2_FIELD_EVENT_SAVE_H
#define POKEBW2_FIELD_EVENT_SAVE_H

#include "types.h"
#include "struct_decls.h"
#include "system/game_event.h"

typedef struct ReportWork ReportWork;

struct EventSaveWork {
    SaveControl *save;
    GameSystem *gameSystem;
    Field *field;
    FieldMsgBG *msgBgSys;
    u16 heapId;
    u16 pad12;
    u32 *result;
    // The subscreen to go back to
    u32 screenId;
    ReportWork *report;
};

GameEvent *EventSave_Create(GameSystem *gsys, Field *field, u16 heapId, FieldMsgBG *msgBgSys, u32 screenId,
                            u32 *result);
GameEventReturnCode EventSave_Callback(GameEvent *event, u32 *state, void *data);
// Callbacks of the save in FIELD_PROC_LINK_LIST
void FieldProcLink_CallInfDungeonRetire(FieldAppCallWork *work, s32 appParam);
void FieldProcLink_FreeParam(void *param);
// Overlay 12's report_event.c: the report screen, which asks whether to save and saves. Returns 0 once saved, 1 when
// the player backed out, and 2 until then
u32 EventSave_Update(EventSaveWork *work, u32 *state);

#endif // POKEBW2_FIELD_EVENT_SAVE_H
