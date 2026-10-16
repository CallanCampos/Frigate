// TwlSDK's SCFG library (libscfg, scfg/ARM9.TWL/src/scfg_proc.c; the file name is the SDK's, from its only object):
// the PXI service that gathers the ARM7's configuration data, and the switch of the ARM9's clock. The object's ITCM
// part, SCFGi_SwitchCpuSpeed, is not in this file. Nothing in this build sends a request, so the work's callback stays
// NULL and the fields are named after what the answer's callback does with them.
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "twl/scfg.h"

// The words of the SCFG service (names of the constants are guesses). A word has the answer's type in bits 20 to 25,
// the index of a part in bits 16 to 19 and 16 bits of data
#define SCFG_PXI_DATA_MASK 0x0000ffff
#define SCFG_PXI_INDEX_MASK 0x000f0000
#define SCFG_PXI_INDEX_SHIFT 16
#define SCFG_PXI_TYPE_MASK 0x03f00000
#define SCFG_PXI_TYPE_SHIFT 20

// The type of an answer: a part of the 64 bits, which are complete once the four parts are here, or a single value
#define SCFG_PXI_TYPE_DATA 1
#define SCFG_PXI_TYPE_VALUE 2

#define SCFG_PART_COUNT 4
#define SCFG_ALL_PARTS ((1 << SCFG_PART_COUNT) - 1)

typedef struct SCFGWork {
    BOOL busy;
    u64 data;
    u32 parts;
    SCFGCallback callback;
    void *callbackArg;
} SCFGWork;

static SCFGWork scfgWork;

static void SCFGi_CommonCallback(u32 tag, u32 data, BOOL err);

void SCFG_Init(void) {
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_SCFG, SCFGi_CommonCallback);
    scfgWork.busy = FALSE;
}

void SCFG_SetCpuSpeed(SCFGCpuSpeed speed) {
    u32 irq = OS_DisableInterrupts();

    if (speed != (reg_SCFG_CLK & SCFG_CLK_CPU_2X)) {
        SCFGi_SwitchCpuSpeed(speed);
    }
    OS_RestoreInterrupts(irq);
}

static void SCFGi_CommonCallback(u32 tag, u32 data, BOOL err) {
    u16 index = (data & SCFG_PXI_INDEX_MASK) >> SCFG_PXI_INDEX_SHIFT;
    u16 type = (data & SCFG_PXI_TYPE_MASK) >> SCFG_PXI_TYPE_SHIFT;
    u16 value = data & SCFG_PXI_DATA_MASK;
    // The callback of a single value gets 64 bits too, of which only the low 16 are set
    union {
        u64 data;
        u16 value;
    } response;

    switch (type) {
    case SCFG_PXI_TYPE_DATA:
        scfgWork.data |= (u64)value << (index * 16);
        scfgWork.parts |= 1 << index;
        if (scfgWork.parts == SCFG_ALL_PARTS) {
            if (scfgWork.callback != NULL) {
                scfgWork.callback(scfgWork.data, scfgWork.callbackArg);
                scfgWork.callback = NULL;
            }
            scfgWork.busy = FALSE;
        }
        break;
    case SCFG_PXI_TYPE_VALUE:
        response.value = value;
        if (scfgWork.callback != NULL) {
            scfgWork.callback(response.data, scfgWork.callbackArg);
            scfgWork.callback = NULL;
        }
        scfgWork.busy = FALSE;
        break;
    default:
        OS_Terminate();
        break;
    }
}
