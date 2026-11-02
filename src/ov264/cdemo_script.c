#include "app/cdemo.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "nitro/gx.h"

// Descriptive file name: the command table separates this interpreter from the demo controller.
typedef u32 (*CDemoCommandFunc)(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_WaitFrames(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_FadeInBlack(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_FadeOutBlack(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_FadeInWhite(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_FadeOutWhite(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_WaitFade(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_LoadBackground(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_EnableBackground(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_SetBackgroundPriority(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_SetBackgroundSettings(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_SetTransitionSettings(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_ResetBackgroundSettings(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_Unused12(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_ResetSound(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_PlayBGM(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_Unused15(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_Unused16(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_Unused17(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_StartMovie(CDemoWork *work, const u32 *command);
static u32 CDemoCmd_Nop19(CDemoWork *work, const u32 *command);
static const CDemoCommandFunc CDEMO_COMMANDS[] = { CDemoCmd_WaitFrames,
                                                   CDemoCmd_FadeInBlack,
                                                   CDemoCmd_FadeOutBlack,
                                                   CDemoCmd_FadeInWhite,
                                                   CDemoCmd_FadeOutWhite,
                                                   CDemoCmd_WaitFade,
                                                   CDemoCmd_LoadBackground,
                                                   CDemoCmd_EnableBackground,
                                                   CDemoCmd_SetBackgroundPriority,
                                                   CDemoCmd_SetBackgroundSettings,
                                                   CDemoCmd_SetTransitionSettings,
                                                   CDemoCmd_ResetBackgroundSettings,
                                                   CDemoCmd_Unused12,
                                                   CDemoCmd_ResetSound,
                                                   CDemoCmd_PlayBGM,
                                                   CDemoCmd_Unused15,
                                                   CDemoCmd_Unused16,
                                                   CDemoCmd_Unused17,
                                                   CDemoCmd_StartMovie,
                                                   CDemoCmd_Nop19 };
static const u32 CDEMO_COMMAND_LENGTHS[] = { 2, 3, 3, 3, 3, 1, 3, 3, 3, 4, 3, 1, 1, 1, 2, 2, 3, 2, 2, 2, 1 };
u32 CDemo_RunCommand(CDemoWork *work) {
    u32 result = CDEMO_COMMANDS[*work->scriptCursor](work, work->scriptCursor);
    if (result != CDEMO_CMD_RESULT_WAIT) {
        work->scriptCursor += CDEMO_COMMAND_LENGTHS[*work->scriptCursor];
    }
    return result;
}
static u32 CDemoCmd_WaitFrames(CDemoWork *work, const u32 *command) {
    if (work->waitCounter == command[1]) {
        work->waitCounter = 0;
        return CDEMO_CMD_RESULT_YIELD;
    }
    work->waitCounter++;
    return CDEMO_CMD_RESULT_WAIT;
}
static u32 CDemoCmd_FadeInBlack(CDemoWork *work, const u32 *command) {
    CDemo_FadeInBlack(work, command[1], command[2], CDEMO_STATE_SCRIPT);
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_FadeOutBlack(CDemoWork *work, const u32 *command) {
    CDemo_FadeOutBlack(work, command[1], command[2], CDEMO_STATE_SCRIPT);
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_FadeInWhite(CDemoWork *work, const u32 *command) {
    CDemo_FadeInWhite(work, command[1], command[2], CDEMO_STATE_SCRIPT);
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_FadeOutWhite(CDemoWork *work, const u32 *command) {
    CDemo_FadeOutWhite(work, command[1], command[2], CDEMO_STATE_SCRIPT);
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_WaitFade(CDemoWork *work, const u32 *command) {
    work->state = CDEMO_STATE_WAIT_FADE;
    work->resumeState = CDEMO_STATE_SCRIPT;
    return CDEMO_CMD_RESULT_YIELD;
}
static u32 CDemoCmd_LoadBackground(CDemoWork *work, const u32 *command) {
    GFL_BGSysLoadArcNCGRStatic(work->arc, command[2], command[1], 0, 0, TRUE, HEAPID_CDEMO);
    if (command[2] == 0) {
        loadBGScrToVramByFileNoReserveNegAlign(work->arc, command[2], command[1], 0, 0, FALSE, HEAPID_CDEMO);
    }
    CDemo_LoadPalette(work, command[1], command[2]);
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_EnableBackground(CDemoWork *work, const u32 *command) {
    GFL_BGSysSetBGEnabled((u8)command[1], (u8)command[2]);
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_SetBackgroundPriority(CDemoWork *work, const u32 *command) {
    GFL_BGSysSetBGPriority((u8)command[1], (u8)command[2]);
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_SetBackgroundSettings(CDemoWork *work, const u32 *command) {
    if (command[1] == 0) {
        work->unk6024[0] = command[2];
        work->unk6024[1] = command[3];
    } else {
        work->unk6024[2] = command[2];
        work->unk6024[3] = command[3];
    }
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_SetTransitionSettings(CDemoWork *work, const u32 *command) {
    work->unk6034[0] = command[1];
    work->unk6034[1] = command[2];
    work->state = CDEMO_STATE_TRANSITION;
    work->resumeState = CDEMO_STATE_SCRIPT;
    return CDEMO_CMD_RESULT_YIELD;
}
static u32 CDemoCmd_ResetBackgroundSettings(CDemoWork *work, const u32 *command) {
    reg_G2_BLDCNT = 0;
    work->unk6024[0] = 0;
    work->unk6024[1] = 0;
    work->unk6024[2] = 0;
    work->unk6024[3] = 0;
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_Unused12(CDemoWork *work, const u32 *command) {
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_ResetSound(CDemoWork *work, const u32 *command) {
    GFL_SndBGMStopImmediate();
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_PlayBGM(CDemoWork *work, const u32 *command) {
    GFL_SndBGMStop(command[1]);
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_Unused15(CDemoWork *work, const u32 *command) {
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_Unused16(CDemoWork *work, const u32 *command) {
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_Unused17(CDemoWork *work, const u32 *command) {
    return CDEMO_CMD_RESULT_NEXT;
}
static u32 CDemoCmd_StartMovie(CDemoWork *work, const u32 *command) {
    work->nextFrame = 0;
    work->currentBuffer = 0;
    work->frameState = CDEMO_MOVIE_LOAD_FIRST;
    work->lastTick = 0;
    work->elapsedFrameTime = 0;
    work->state = CDEMO_STATE_MOVIE;
    return CDEMO_CMD_RESULT_YIELD;
}
static u32 CDemoCmd_Nop19(CDemoWork *work, const u32 *command) {
    return CDEMO_CMD_RESULT_NEXT;
}
