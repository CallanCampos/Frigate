#ifndef POKEBW2_APP_CDEMO_H
#define POKEBW2_APP_CDEMO_H

#include "types.h"
#include "gfl/proc.h"
#include "gfl/tcb.h"

// The demos the game plays before the title screen, the Game Freak logo and the opening (ov264, cdemo_main.c). The
// names are ours

struct CDemoParam {
    // Which demo plays
    u16 demo;
    // Whether A, B or Start skips the demo
    u8 canSkip;
    // Set when the demo was skipped
    u8 skipped;
};

struct CDemoPalette {
    u8 data[0x2000];
    u32 size;
};

struct CDemoWork {
    CDemoParam *param;
    ArcTool *arc;
    CDemoPalette palettes[3];
    TCB *vblankTask;
    const u32 *scriptStart;
    const u32 *scriptCursor;
    u32 waitCounter;
    u32 unk6024[4];
    u32 unk6034[2];
    u16 nextFrame;
    u8 currentBuffer;
    u8 frameState;
    u64 lastTick;
    u64 elapsedFrameTime;
    u32 state;
    u32 resumeState;
    BOOL initialized;
};

u32 CDemo_RunCommand(CDemoWork *work);
BOOL CDemo_Main(CDemoWork *work);
void CDemo_ConfigureVRAM(void);
void CDemo_InitGraphics(void);
void CDemo_DisableGraphics(void);
void CDemo_InitScript(CDemoWork *work);
void CDemo_FreeScript(CDemoWork *work);
void CDemo_StartVBlank(CDemoWork *work);
void CDemo_StopVBlank(CDemoWork *work);
void CDemo_FadeInBlack(CDemoWork *work, int division, int sync, u32 resumeState);
void CDemo_FadeOutBlack(CDemoWork *work, int division, int sync, u32 resumeState);
void CDemo_FadeInWhite(CDemoWork *work, int division, int sync, u32 resumeState);
void CDemo_FadeOutWhite(CDemoWork *work, int division, int sync, u32 resumeState);
void CDemo_LoadPalette(CDemoWork *work, u32 index, u32 fileId);

extern const GameProcFunctions CDEMO_PROC_FUNCTIONS;

#endif // POKEBW2_APP_CDEMO_H
