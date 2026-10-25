#ifndef POKEBW2_DWCUTIL_CELLINFO_H
#define POKEBW2_DWCUTIL_CELLINFO_H

#include "types.h"
#include "cell.h"
#include "nitro/gx.h"

// The connection utility's sprite cells: a table per screen, from a file, of the OBJ attribute lists that are copied
// into OAM entries. The file is the DWC utility library's cellinfo.o (libdwcutil.TWL.HYB.a), as aletheia's map of
// Black 1's SDK code to that library's objects (layout/symbols_sdk.txt) shows it: its five functions, then deque.o's.
void DWCi_CEINlRead(int screen, const char *path);
void DWCi_CEINlRelease(int screen);
void DWCi_CEINlSet(int screen, int cell, GXOamAttr *oam);
// Copies the cell into entries from the screen's OAM buffer, or into a run of entries
GXOamAttr *DWCi_CEINlSetExObj(int screen, int cell);
DWCUtilOamNode *DWCi_CEINlSetExCell(int screen, int cell, BOOL fromFront);

#endif // POKEBW2_DWCUTIL_CELLINFO_H
