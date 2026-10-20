#ifndef POKEBW2_FIELD_FMISSION_VIEW_H
#define POKEBW2_FIELD_FMISSION_VIEW_H

#include "types.h"
#include "gfl/overlay.h"
#include "struct_decls.h"

// The touch screen during a Funfest mission, overlay 85: the mission, its players and a log of what they did, with
// buttons for the Pass Powers, the greeting, the festival's info and quitting. The field's subscreen (overlay 36) runs
// it, with overlay 83's beacon_view_common.c loaded. The ROM names "fmission_view_setup.c"; the main file's name,
// fmission_view.c, is a guess. swan has no names for this overlay; the names are ours
#define OVERLAY_FMISSION_VIEW OVERLAY_ID(85)

typedef struct FMissionView FMissionView;

FMissionView *FMissionView_Create(GameSystem *gsys, FieldSubscreen *subscreen);
void FMissionView_Free(FMissionView *view);
// Runs the view for a frame. While it is not active it is dimmed, and only waits
void FMissionView_Update(FMissionView *view, BOOL active);
// Whether the print queue still has text to draw
BOOL FMissionView_IsPrinting(FMissionView *view);
// The event of the screen that a button asked for, or NULL
GameEvent *FMissionView_CheckEvent(FMissionView *view, BOOL enabled);

#endif // POKEBW2_FIELD_FMISSION_VIEW_H
