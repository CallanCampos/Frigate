#include "nnsys/gfd.h"

// NitroSystem's gfd_PlttVramMan.c: the palette VRAM manager that NNS_GfdAllocPlttVram and NNS_GfdFreePlttVram use until
// a manager's init replaces it, which allocates nothing. The name is NitroSystem's, as ntrtwl's reconstruction of
// NitroSystem has it, and pokediamond's link map has it as gfd_plttvramman. swan calls NNS_GfdDefaultFuncAllocPlttVram
// and NNS_GfdDefaultFuncFreePlttVram g_PltVRAMAllocFunc and g_PltVRAMFreeFunc

static NNSGfdPlttKey AllocPlttVram_(u32 szByte, BOOL is4pltt, u32 opt);
static int FreePlttVram_(NNSGfdPlttKey key);

NNSGfdFuncAllocPlttVram NNS_GfdDefaultFuncAllocPlttVram = AllocPlttVram_;
NNSGfdFuncFreePlttVram NNS_GfdDefaultFuncFreePlttVram = FreePlttVram_;

static NNSGfdPlttKey AllocPlttVram_(u32 szByte, BOOL is4pltt, u32 opt) {
    return NNS_GFD_ALLOC_ERROR_PLTTKEY;
}

static int FreePlttVram_(NNSGfdPlttKey key) {
    return -1;
}
