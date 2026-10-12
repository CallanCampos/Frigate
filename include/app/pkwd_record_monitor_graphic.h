#ifndef POKEBW2_APP_PKWD_RECORD_MONITOR_GRAPHIC_H
#define POKEBW2_APP_PKWD_RECORD_MONITOR_GRAPHIC_H

// The BGs and cell actors of Pokéstar Studios' record monitor (overlay 325), named after the ROM's
// "pkwd_record_monitor_graphic.c"

#include "types.h"
#include "gfl/clact.h"
#include "gfl/heap.h"

typedef struct PkwdRecordMonitorGraphic PkwdRecordMonitorGraphic;

PkwdRecordMonitorGraphic *PkwdRecordMonitorGraphic_Create(u32 layout, HeapID heapId);
void PkwdRecordMonitorGraphic_Free(PkwdRecordMonitorGraphic *graphic);
void PkwdRecordMonitorGraphic_Update(PkwdRecordMonitorGraphic *graphic);
// Empty: the template's screens with a 3D system draw between them
void PkwdRecordMonitorGraphic_Begin3D(PkwdRecordMonitorGraphic *graphic);
void PkwdRecordMonitorGraphic_End3D(PkwdRecordMonitorGraphic *graphic);
ClActUnit *PkwdRecordMonitorGraphic_GetClActUnit(PkwdRecordMonitorGraphic *graphic);

#endif // POKEBW2_APP_PKWD_RECORD_MONITOR_GRAPHIC_H
