#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H

#include "types.h"
#include "dwc/bm.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_heap.h"
#include "dwc_util_list.h"
#include "dwc_util_oam.h"
#include "dwc_util_oam_buf.h"
#include "dwc_util_task.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_vram_alloc.h"
#include "dwcutil/dwc_aoss.h"
#include "dwcutil/dwcutil_ac.h"
#include "nitro/gx.h"
#include "struct_decls.h"

// Entry points in later, still unrecovered files of the DWC settings utility.

// The whole touch screen, a rectangle as in dwc_util_display.c: .rodata between dwc_util_text.c's and
// dwc_util_task.c's that no file claims yet
extern const u16 gDWCUtilTouchScreenRect[4];

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
