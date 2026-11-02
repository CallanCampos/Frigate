#ifndef POKEBW2_APP_CDEMO_H
#define POKEBW2_APP_CDEMO_H

#include "types.h"
#include "gfl/proc.h"
#include "gfl/tcb.h"

// The demos the game plays before the title screen, the Game Freak logo and the opening (ov264, cdemo_main.c). The
// names are ours

// The states of CDemo_Main, indices into its table of state functions; a state function returns the next one
enum {
    CDEMO_STATE_INIT,
    CDEMO_STATE_EXIT,
    CDEMO_STATE_WAIT_FADE,
    CDEMO_STATE_TRANSITION,
    CDEMO_STATE_MOVIE,
    CDEMO_STATE_SCRIPT,
    CDEMO_STATE_END,
};

// A demo script's opcodes, indices into the table of command functions. CDEMO_CMD_END has no function: it ends the
// script
enum {
    CDEMO_CMD_WAIT_FRAMES,
    CDEMO_CMD_FADE_IN_BLACK,
    CDEMO_CMD_FADE_OUT_BLACK,
    CDEMO_CMD_FADE_IN_WHITE,
    CDEMO_CMD_FADE_OUT_WHITE,
    CDEMO_CMD_WAIT_FADE,
    CDEMO_CMD_LOAD_BACKGROUND,
    CDEMO_CMD_ENABLE_BACKGROUND,
    CDEMO_CMD_SET_BACKGROUND_PRIORITY,
    CDEMO_CMD_SET_BACKGROUND_SETTINGS,
    CDEMO_CMD_SET_TRANSITION_SETTINGS,
    CDEMO_CMD_RESET_BACKGROUND_SETTINGS,
    CDEMO_CMD_UNUSED12,
    CDEMO_CMD_RESET_SOUND,
    CDEMO_CMD_PLAY_BGM,
    CDEMO_CMD_UNUSED15,
    CDEMO_CMD_UNUSED16,
    CDEMO_CMD_UNUSED17,
    CDEMO_CMD_START_MOVIE,
    CDEMO_CMD_NOP19,
    CDEMO_CMD_END,
};

// What a command function returns: stay on this command, go on to the next in the same frame, or go to the next and
// return to CDemo_Main
enum {
    CDEMO_CMD_RESULT_WAIT,
    CDEMO_CMD_RESULT_NEXT,
    CDEMO_CMD_RESULT_YIELD,
};

// The steps of the movie, in CDemoWork's frameState
enum {
    CDEMO_MOVIE_LOAD_FIRST,
    CDEMO_MOVIE_PLAY,
    CDEMO_MOVIE_SHOW_LAST,
    CDEMO_MOVIE_DONE,
};

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
void CDemo_InitGraphics(CDemoWork *work);
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
