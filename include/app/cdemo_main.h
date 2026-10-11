#ifndef POKEBW2_APP_CDEMO_MAIN_H
#define POKEBW2_APP_CDEMO_MAIN_H

#include "types.h"
#include "app/cdemo.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The work of the demos (cdemo_main.c and cdemo_script.c, overlay 264) and the functions that cdemo_script.c calls.
// The names are ours

#define CDEMO_PALETTE_COUNT 3

// The states of the demo
enum {
    CDEMO_STATE_INIT,
    CDEMO_STATE_FINISH,
    CDEMO_STATE_WIPE,
    CDEMO_STATE_IDLE,
    CDEMO_STATE_ANIMATION,
    CDEMO_STATE_SCRIPT,
    CDEMO_STATE_END,
};

typedef struct {
    u8 data[0x2000];
    u32 size;
} CDemoPalette;

typedef struct CDemo {
    CDemoParam *param;
    ArcTool *arc;
    CDemoPalette palettes[CDEMO_PALETTE_COUNT];
    TCB *vblankTask;
    const u32 *scriptStart;
    const u32 *script;
    u32 waitFrames;
    // Set by the script for the two layers
    u32 layerParams[2][2];
    u32 idleParams[2];
    u16 frame;
    u8 layer;
    u8 animationState;
    u64 lastTick;
    u64 elapsedUs;
    u32 state;
    u32 nextState;
    BOOL initialized;
} CDemo;

void CDemo_SetVRAMBanks(void);
// Sets the engines up for two direct color bitmap BGs, 2 and 3, and clears them
void CDemo_SetupBGs(CDemo *demo);
void CDemo_HideBGs(void);
// Points the script at the start of the demo's
void CDemo_InitScript(CDemo *demo);
void CDemo_Free(CDemo *demo);
void CDemo_AddVBlankTask(CDemo *demo);
void CDemo_RemoveVBlankTask(CDemo *demo);
// The screens fade in from black, out to black, in from white or out to white, and the demo goes to nextState once the
// fade ends
void CDemo_FadeInBlack(CDemo *demo, s32 division, s32 sync, u32 nextState);
void CDemo_FadeOutBlack(CDemo *demo, s32 division, s32 sync, u32 nextState);
void CDemo_FadeInWhite(CDemo *demo, s32 division, s32 sync, u32 nextState);
void CDemo_FadeOutWhite(CDemo *demo, s32 division, s32 sync, u32 nextState);
// Loads a palette file into one of the demo's palettes
void CDemo_LoadPalette(CDemo *demo, u32 index, u32 fileId);

#endif // POKEBW2_APP_CDEMO_MAIN_H
