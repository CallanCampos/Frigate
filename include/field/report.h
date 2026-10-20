#ifndef POKEBW2_FIELD_REPORT_H
#define POKEBW2_FIELD_REPORT_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "struct_decls.h"

// The report's touch screen, shown while the game saves: the player's name, the date, the time, the location, the
// badges, the Pokédex, the play time and the last save, the party's icons and a bar of ten icons that fills as the
// save is written. Overlay 87, named after the ROM's "report.c". Overlay 36's field subscreen table calls these
#define OVERLAY_REPORT OVERLAY_ID(87)

typedef struct ReportScreen ReportScreen;

ReportScreen *Report_Create(GameSystem *gsys, HeapID heapId);
void Report_Free(ReportScreen *wk);
// Shows the party's icons once the text is printed
void Report_Update(ReportScreen *wk);
// Prints the text and shows the windows once it is printed
void Report_UpdateAltFrame(ReportScreen *wk);
// Whether the windows and the party's icons are shown
BOOL Report_IsReady(ReportScreen *wk);
// Reads how much the save will write, for the bar
void Report_InitSave(ReportScreen *wk);
// Whether at least a third of the save's blocks changed, so that the bar is worth showing
BOOL Report_IsLargeSave(ReportScreen *wk);
// Replaces the last save's window with the bar, which fills as the save is written
void Report_StartBar(ReportScreen *wk);
// Whether the bar is full, which stops it
BOOL Report_IsBarFull(ReportScreen *wk);
void Report_StopBar(ReportScreen *wk);

#endif // POKEBW2_FIELD_REPORT_H
