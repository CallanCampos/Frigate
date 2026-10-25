#include "cellinfo.h"
#include "archive.h"
#include "cell.h"
#include "nitro/mi.h"
#include "obj.h"

// A cell of the table: its count of OBJ attribute lists, the first three halfwords of an OAM entry each, at the offset
// from the table's start
typedef struct DWCUtilCell {
    u16 count;
    u32 offset;
} DWCUtilCell;

static DWCUtilCell *sCells[2];

void DWCi_CEINlRead(int screen, const char *path) {
    u32 size;

    sCells[screen] = DWCi_ARClReadEx(path, &size, 4);
}

void DWCi_CEINlRelease(int screen) {
    DWCi_ARClRelease(sCells[screen]);
    sCells[screen] = NULL;
}

void DWCi_CEINlSet(int screen, int cell, GXOamAttr *oam) {
    DWCUtilCell *cells = sCells[screen];
    const u16 *attrs = (const u16 *)((const u8 *)cells + cells[cell].offset);
    u16 count = cells[cell].count;
    GXOamAttr attr;
    int i;

    MI_CpuClear32(&attr, sizeof(attr));
    for (i = 0; i < count; i++, attrs += 3, oam++) {
        MI_CpuCopy16(attrs, &attr, 6);
        MIi_CpuCopy32(&attr, oam, sizeof(attr));
    }
}

GXOamAttr *DWCi_CEINlSetExObj(int screen, int cell) {
    GXOamAttr *oam = DWCi_OBJlFormGx(screen);

    DWCi_CEINlSet(screen, cell, oam);
    return oam;
}

DWCUtilOamNode *DWCi_CEINlSetExCell(int screen, int cell, BOOL fromFront) {
    DWCUtilOamNode *node = DWCi_CELLlForm(screen, sCells[screen][cell].count, fromFront);

    DWCi_CEINlSet(screen, cell, DWCi_CELLlGetObj(node, 0));
    return node;
}
