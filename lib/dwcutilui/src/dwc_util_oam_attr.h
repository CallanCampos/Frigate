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

#endif // POKEBW2_DWCUTIL_DWC_UTIL_OAM_ATTR_H
