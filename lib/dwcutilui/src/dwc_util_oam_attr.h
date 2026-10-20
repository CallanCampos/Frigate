#ifndef POKEBW2_DWCUTIL_DWC_UTIL_OAM_ATTR_H
#define POKEBW2_DWCUTIL_DWC_UTIL_OAM_ATTR_H

#include "types.h"
#include "nitro/gx.h"

// The first word of an OAM entry, returned by value: the struct keeps a stack slot in each caller that inlines it,
// as the ROM's position updates have (docs/matching.md, "Stack slots")
typedef struct DWCUtilOamAttr01 {
    u32 attr01;
} DWCUtilOamAttr01;

static inline DWCUtilOamAttr01 DWCUtilOam_ReadAttr01(const GXOamAttr *oam) {
    DWCUtilOamAttr01 word;

    word.attr01 = oam->attr01;
    return word;
}

// Moves the entry to (x, y): NitroSDK's G2_SetOBJPosition with its terms the other way round, y before x
static inline void DWCUtilOam_SetPosition(GXOamAttr *oam, int x, int y) {
    oam->attr01 =
        (oam->attr01 & ~(GX_OAM_ATTR01_X_MASK | GX_OAM_ATTR01_Y_MASK)) | (u8)y | ((x & 0x1ff) << GX_OAM_ATTR01_X_SHIFT);
}

// The entry's position: NitroSDK's G2_GetOBJPosition
static inline void DWCUtilOam_GetPosition(const GXOamAttr *oam, int *x, int *y) {
    *x = (oam->attr01 & GX_OAM_ATTR01_X_MASK) >> GX_OAM_ATTR01_X_SHIFT;
    *y = (u8)oam->attr01;
}

#endif // POKEBW2_DWCUTIL_DWC_UTIL_OAM_ATTR_H
