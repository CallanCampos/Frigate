#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H

#include "types.h"
#include "cell.h"
#include "dwc/bm.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_heap.h"
#include "dwc_util_list.h"
#include "dwcutil/aoss.h"
#include "dwcutil/atermset.h"
#include "font.h"
#include "input.h"
#include "nitro/gx.h"
#include "obj.h"
#include "objvram.h"
#include "struct_decls.h"
#include "task.h"

// Entry points in later, still unrecovered files of the DWC settings utility.

// The whole touch screen, a rectangle as in dwc_util_display.c: .rodata between font.c's and
// task.c's that no file claims yet
extern const u16 gDWCUtilTouchScreenRect[4];

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
