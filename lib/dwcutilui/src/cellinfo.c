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

void DWCUtilCell_Load(int screen, const char *path) {
    u32 size;

    sCells[screen] = DWCUtilRomResource_LoadFile(path, &size, 4);
}

void DWCUtilCell_Unload(int screen) {
    DWCi_ARClRelease(sCells[screen]);
    sCells[screen] = NULL;
}

void DWCUtilCell_Copy(int screen, int cell, GXOamAttr *oam) {
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

GXOamAttr *DWCUtilCell_AllocOam(int screen, int cell) {
    GXOamAttr *oam = DWCi_OBJlFormGx(screen);

    DWCUtilCell_Copy(screen, cell, oam);
    return oam;
}

DWCUtilOamNode *DWCUtilCell_AllocOamRun(int screen, int cell, BOOL fromFront) {
    DWCUtilOamNode *node = DWCi_CELLlForm(screen, sCells[screen][cell].count, fromFront);

    DWCUtilCell_Copy(screen, cell, DWCi_CELLlGetObj(node, 0));
    return node;
}
