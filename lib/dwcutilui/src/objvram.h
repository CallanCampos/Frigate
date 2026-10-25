#ifndef POKEBW2_DWCUTIL_OBJVRAM_H
#define POKEBW2_DWCUTIL_OBJVRAM_H

#include "types.h"
#include "struct_decls.h"

// The connection utility's OBJ character allocator: blocks of each screen's OBJ characters (0 for the main engine's,
// 1 for the sub engine's) in units of 0x80 bytes, from unit 0x300 to 0x400, handed out from the front of that range or
// from its back. The text canvases draw into them.
// The file is the DWC utility library's objvram.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to
// that library's objects (layout/symbols_sdk.txt) shows it.

// A block on its screen's list, which is kept in address order
struct DWCUtilVramNode {
    DWCUtilVramNode *prev;
    DWCUtilVramNode *next;
    u16 pos;
    u16 size;
};

void DWCi_OVRlInit(void);
// A block for size characters, the first that fits from the front if fromFront, else from the back, and its first
// unit in pos
DWCUtilVramNode *DWCi_OVRlAlloc(int screen, int size, s16 fromFront, u32 *pos);
void DWCi_OVRlFree(DWCUtilVramNode *node);

#endif // POKEBW2_DWCUTIL_OBJVRAM_H
