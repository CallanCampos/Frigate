#ifndef POKEBW2_NNSYS_GFD_H
#define POKEBW2_NNSYS_GFD_H

#include "types.h"

// NitroSystem's VRAM managers (NNS_Gfd) for textures and palettes, and its VRAM transfer manager. A key packs where an
// allocation is and its size, 0 when it failed

typedef u32 NNSGfdTexKey;
typedef u32 NNSGfdPlttKey;

#define NNS_GFD_ALLOC_ERROR_TEXKEY 0
#define NNS_GFD_ALLOC_ERROR_PLTTKEY 0

// Where in VRAM a key's allocation is
#define NNS_GFD_KEY_ADDR_SHIFT 3

// A texture key's size counts 16-byte units, the smallest texture allocation
#define NNS_GFD_KEY_SIZE_SHIFT 4
#define GFD_TEX_ALLOC_MIN (1 << NNS_GFD_KEY_SIZE_SHIFT)
// The first texture size a manager refuses
#define GFD_TEX_ALLOC_LIMIT 0x7fff0

// A key for the VRAM at the address, not allocated from a manager
static inline NNSGfdTexKey NNS_GfdMakeTexKey(u32 addr, u32 size, BOOL is4x4comp) {
    return ((size >> NNS_GFD_KEY_SIZE_SHIFT) << 16) | ((addr >> NNS_GFD_KEY_ADDR_SHIFT) & 0xffff) | (is4x4comp << 31);
}

static inline u32 NNS_GfdGetTexKeyAddr(NNSGfdTexKey key) {
    return (u32)((key & 0xffff) << NNS_GFD_KEY_ADDR_SHIFT);
}

static inline u32 NNS_GfdGetTexKeySize(NNSGfdTexKey key) {
    return (u32)(((key & 0x7fff0000) >> 16) << NNS_GFD_KEY_SIZE_SHIFT);
}

// Whether the key is for a 4x4-compressed texture
static inline BOOL GfdTexKeyIs4x4(NNSGfdTexKey key) {
    return (BOOL)((key & 0x80000000) >> 31);
}

// The size a texture allocation takes: whole 16-byte units, at least one
static inline u32 GfdTexAllocSize(u32 size) {
    if (size == 0) {
        return GFD_TEX_ALLOC_MIN;
    }
    return (size + (GFD_TEX_ALLOC_MIN - 1)) & ~(GFD_TEX_ALLOC_MIN - 1);
}

// A palette key's size counts 8-byte units, the smallest palette allocation
#define NNS_GFD_PLTTKEY_SIZE_SHIFT 3
#define GFD_PLTT_ALLOC_MIN (1 << NNS_GFD_PLTTKEY_SIZE_SHIFT)
// The first palette size a manager refuses
#define GFD_PLTT_ALLOC_LIMIT 0x7fff8
// A 4-color palette must lie in the first 64 KB of palette VRAM
#define GFD_PLTT4_LIMIT 0x10000

static inline NNSGfdPlttKey GfdMakePlttKey(u32 addr, u32 size) {
    return ((size >> NNS_GFD_PLTTKEY_SIZE_SHIFT) << 16) | ((addr >> NNS_GFD_KEY_ADDR_SHIFT) & 0xffff);
}

static inline u32 NNS_GfdGetPlttKeyAddr(NNSGfdPlttKey key) {
    return (u32)((key & 0xffff) << NNS_GFD_KEY_ADDR_SHIFT);
}

static inline u32 NNS_GfdGetPlttKeySize(NNSGfdPlttKey key) {
    return (u32)(((key & 0xffff0000) >> 16) << NNS_GFD_PLTTKEY_SIZE_SHIFT);
}

// The size a palette allocation takes: whole 8-byte units, at least one
static inline u32 GfdPlttAllocSize(u32 size) {
    if (size == 0) {
        return GFD_PLTT_ALLOC_MIN;
    }
    return (size + (GFD_PLTT_ALLOC_MIN - 1)) & ~(GFD_PLTT_ALLOC_MIN - 1);
}

typedef NNSGfdTexKey (*NNSGfdFuncAllocTexVram)(u32 szByte, BOOL is4x4comp, u32 opt);
typedef int (*NNSGfdFuncFreeTexVram)(NNSGfdTexKey key);
typedef NNSGfdPlttKey (*NNSGfdFuncAllocPlttVram)(u32 szByte, BOOL is4pltt, u32 opt);
typedef int (*NNSGfdFuncFreePlttVram)(NNSGfdPlttKey key);

// The managers that NNS_GfdAllocTexVram and the others use, which a manager's init sets when it is the default
extern NNSGfdFuncAllocTexVram NNS_GfdDefaultFuncAllocTexVram;
extern NNSGfdFuncFreeTexVram NNS_GfdDefaultFuncFreeTexVram;
extern NNSGfdFuncAllocPlttVram NNS_GfdDefaultFuncAllocPlttVram;
extern NNSGfdFuncFreePlttVram NNS_GfdDefaultFuncFreePlttVram;

static inline NNSGfdTexKey NNS_GfdAllocTexVram(u32 szByte, BOOL is4x4comp, u32 opt) {
    return (*NNS_GfdDefaultFuncAllocTexVram)(szByte, is4x4comp, opt);
}

static inline int NNS_GfdFreeTexVram(NNSGfdTexKey key) {
    return (*NNS_GfdDefaultFuncFreeTexVram)(key);
}

static inline NNSGfdPlttKey NNS_GfdAllocPlttVram(u32 szByte, BOOL is4pltt, u32 opt) {
    return (*NNS_GfdDefaultFuncAllocPlttVram)(szByte, is4pltt, opt);
}

static inline int NNS_GfdFreePlttVram(NNSGfdPlttKey key) {
    return (*NNS_GfdDefaultFuncFreePlttVram)(key);
}

// Where each region of a frame manager is allocated up to, to go back to later
typedef struct {
    u32 address[10];
} NNSGfdFrmTexVramState;

typedef struct {
    u32 address[2];
} NNSGfdFrmPlttVramState;

// The frame managers, which free only by going back to a saved state
// Sets the order normal textures try the regions in, by region index
void NNSi_GfdSetTexNrmSearchArray(int first, int second, int third, int fourth, int fifth);
void NNS_GfdInitFrmTexVramManager(u16 numSlot, BOOL useAsDefault);
void NNS_GfdResetFrmTexVramState(void);
NNSGfdTexKey NNS_GfdAllocFrmTexVram(u32 szByte, BOOL is4x4comp, u32 opt);
int NNS_GfdFreeFrmTexVram(NNSGfdTexKey key);
void NNS_GfdGetFrmTexVramState(NNSGfdFrmTexVramState *state);
void NNS_GfdSetFrmTexVramState(const NNSGfdFrmTexVramState *state);
// Allocates from the low end of palette VRAM when bAllocFromLo is set
NNSGfdPlttKey NNS_GfdAllocFrmPlttVram(u32 szByte, BOOL is4pltt, u32 bAllocFromLo);
void NNS_GfdGetFrmPlttVramState(NNSGfdFrmPlttVramState *state);
void NNS_GfdSetFrmPlttVramState(const NNSGfdFrmPlttVramState *state);

// The linked-list managers, which free what they allocated and need work memory
u32 NNS_GfdGetLnkTexVramManagerWorkSize(u32 numMemBlk);
void NNS_GfdInitLnkTexVramManager(u32 szByte, u32 szByteFor4x4, void *pManagementWork, u32 szByteManagementWork,
                                  BOOL useAsDefault);
NNSGfdTexKey NNS_GfdAllocLnkTexVram(u32 szByte, BOOL is4x4comp, u32 opt);
int NNS_GfdFreeLnkTexVram(NNSGfdTexKey key);
void NNS_GfdResetLnkTexVramState(void);
u32 NNS_GfdGetLnkPlttVramManagerWorkSize(u32 numMemBlk);
void NNS_GfdInitLnkPlttVramManager(u32 szByte, void *pManagementWork, u32 szByteManagementWork, BOOL useAsDefault);
NNSGfdPlttKey NNS_GfdAllocLnkPlttVram(u32 szByte, BOOL is4pltt, u32 opt);
int NNS_GfdFreeLnkPlttVram(NNSGfdPlttKey key);
void NNS_GfdResetLnkPlttVramState(void);

// Where a transfer goes, which picks the NitroSDK function that does it
typedef enum {
    NNS_GFD_DST_3D_TEX_VRAM,
    NNS_GFD_DST_3D_TEX_PLTT,
    NNS_GFD_DST_3D_CLRIMG_COLOR,
    NNS_GFD_DST_3D_CLRIMG_DEPTH,
    NNS_GFD_DST_2D_BG0_CHAR_MAIN,
    NNS_GFD_DST_2D_BG1_CHAR_MAIN,
    NNS_GFD_DST_2D_BG2_CHAR_MAIN,
    NNS_GFD_DST_2D_BG3_CHAR_MAIN,
    NNS_GFD_DST_2D_BG0_SCR_MAIN,
    NNS_GFD_DST_2D_BG1_SCR_MAIN,
    NNS_GFD_DST_2D_BG2_SCR_MAIN,
    NNS_GFD_DST_2D_BG3_SCR_MAIN,
    NNS_GFD_DST_2D_BG2_BMP_MAIN,
    NNS_GFD_DST_2D_BG3_BMP_MAIN,
    NNS_GFD_DST_2D_OBJ_PLTT_MAIN,
    NNS_GFD_DST_2D_BG_PLTT_MAIN,
    NNS_GFD_DST_2D_OBJ_EXTPLTT_MAIN,
    NNS_GFD_DST_2D_BG_EXTPLTT_MAIN,
    NNS_GFD_DST_2D_OBJ_OAM_MAIN,
    NNS_GFD_DST_2D_OBJ_CHAR_MAIN,
    NNS_GFD_DST_2D_BG0_CHAR_SUB,
    NNS_GFD_DST_2D_BG1_CHAR_SUB,
    NNS_GFD_DST_2D_BG2_CHAR_SUB,
    NNS_GFD_DST_2D_BG3_CHAR_SUB,
    NNS_GFD_DST_2D_BG0_SCR_SUB,
    NNS_GFD_DST_2D_BG1_SCR_SUB,
    NNS_GFD_DST_2D_BG2_SCR_SUB,
    NNS_GFD_DST_2D_BG3_SCR_SUB,
    NNS_GFD_DST_2D_BG2_BMP_SUB,
    NNS_GFD_DST_2D_BG3_BMP_SUB,
    NNS_GFD_DST_2D_OBJ_PLTT_SUB,
    NNS_GFD_DST_2D_BG_PLTT_SUB,
    NNS_GFD_DST_2D_OBJ_EXTPLTT_SUB,
    NNS_GFD_DST_2D_BG_EXTPLTT_SUB,
    NNS_GFD_DST_2D_OBJ_OAM_SUB,
    NNS_GFD_DST_2D_OBJ_CHAR_SUB,
    NNS_GFD_DST_MAX
} NNS_GFD_DST_TYPE;

typedef struct {
    NNS_GFD_DST_TYPE type;
    void *pSrc;
    u32 dstAddr;
    u32 szByte;
} NNSGfdVramTransferTask;

// A ring buffer of transfers in the array given to NNS_GfdInitVramTransferManager
typedef struct {
    NNSGfdVramTransferTask *tasks;
    u32 capacity;
    // The next transfer to do, and where the next one registered goes
    u16 head;
    u16 tail;
    u16 count;
    u16 pad;
    // What the queued transfers come to
    u32 pendingBytes;
} NNSGfdVramTransferTaskQueue;

BOOL NNSi_GfdPushVramTransferTaskQueue(NNSGfdVramTransferTaskQueue *queue);
NNSGfdVramTransferTask *NNSi_GfdGetFrontVramTransferTaskQueue(NNSGfdVramTransferTaskQueue *queue);
NNSGfdVramTransferTask *NNSi_GfdGetEndVramTransferTaskQueue(NNSGfdVramTransferTaskQueue *queue);
BOOL NNSi_GfdPopVramTransferTaskQueue(NNSGfdVramTransferTaskQueue *queue);
void NNS_GfdInitVramTransferManager(NNSGfdVramTransferTask *pTaskArray, u32 lengthOfArray);
// Drops every queued transfer
void GfdClearVramTransferQueue(void);
// Does every queued transfer, in the V-blank
void NNS_GfdDoVramTransfer(void);
// Queues a transfer to VRAM for the next V-blank, or returns FALSE if the queue is full
BOOL NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_TYPE type, u32 dstAddr, void *pSrc, u32 szByte);

#endif // POKEBW2_NNSYS_GFD_H
