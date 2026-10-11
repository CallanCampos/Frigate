#ifndef POKEBW2_APP_MUSICAL_MUS_SHOT_INFO_H
#define POKEBW2_APP_MUSICAL_MUS_SHOT_INFO_H

// Overlay 209's mus_shot_info.c: the touch screen that goes with a musical's photo

#include "types.h"
#include "gfl/heap.h"
#include "save/save_control.h"
#include "save/musical_savedata.h"
#include "struct_decls.h"

MusShotInfo *MusShotInfo_Create(MusicalShot *shot, MusicalSave *save, BOOL askSave, void *comm, HeapID heapId);
void MusShotInfo_Delete(MusShotInfo *info);
void MusShotInfo_Main(MusShotInfo *info);
BOOL MusShotInfo_IsFinished(MusShotInfo *info);

#endif // POKEBW2_APP_MUSICAL_MUS_SHOT_INFO_H
