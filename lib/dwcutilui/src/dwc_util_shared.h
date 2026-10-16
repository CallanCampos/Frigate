#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H

#include "types.h"
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
#include "nitro/gx.h"
#include "struct_decls.h"

// Entry points in later, still unrecovered files of the DWC settings utility.

// The rows of buttons of the two layouts
void func_ov182_021b8154(int variant);
void func_ov182_021b84c0(int variant);

// The utility's message file, which a later file opens
extern DWCUtilMsg *data_ov182_021c40b8;

// The utility's sound: its start and its end, around the main loop
void func_ov182_021b8050(void);
void func_ov182_021b80d4(void);

// The update function of the utility's first screen
void func_ov182_021ae574(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SHARED_H
