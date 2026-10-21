#include "types.h"
#include "app/demo3d/demo3d_capture.h"
#include "app/demo3d/demo3d_cmd.h"
#include "app/demo3d/demo3d_cmdsys.h"
#include "app/demo3d/demo3d_data.h"
#include "app/demo3d/demo3d_engine.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/sound.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "system/gf_font.h"
#include "system/text_speed.h"

// What runs a 3D demo's commands, named after the ROM's "demo3d_cmdsys.c". A demo's commands are sorted by the frame
// of the camera's animation they run at; the ones that come first with DEMO3D_CMD_FRAME_START run when the demo
// starts, and a second list runs when it ends

// No list is longer than this
#define CMD_MAX 9999

static void Demo3DCmdSys_Init(Demo3DCmdSys *sys);
static void Demo3DCmdSys_Exit(Demo3DCmdSys *sys);
static BOOL Demo3DCmdSys_RunStartCmds(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *index);
static void Demo3DCmdSys_Run(Demo3DCmdSys *sys, Demo3DEngine *engine, const Demo3DCmd *cmd);

Demo3DCmdSys *Demo3DCmdSys_Create(Demo3DEngine *engine, HeapID heapId) {
    Demo3DCmdSys *sys = GFL_HeapAllocate(heapId, sizeof(Demo3DCmdSys), TRUE, "demo3d_cmdsys.c", 78);

    sys->engine = engine;
    sys->heapId = heapId;
    sys->tailHeapId = HEAPID_TAIL(heapId);
    sys->demoId = engine->scene.demoId;
    sys->lastFrame = -1;
    Demo3DCmdSys_Init(sys);
    sys->finished = Demo3DCmdSys_RunStartCmds(sys, engine, &sys->cmdIndex);
    return sys;
}

void Demo3DCmdSys_Free(Demo3DCmdSys *sys) {
    Demo3DEngine *engine = sys->engine;
    const Demo3DCmd *cmd = Demo3DData_GetEndCmds(sys->demoId);
    int i;

    for (i = 0; i < CMD_MAX; i++) {
        if (cmd->type == DEMO3D_CMD_END) {
            break;
        }
        Demo3DCmdSys_Run(sys, engine, cmd);
        cmd++;
    }
    sys->ending = TRUE;
    GFL_TCBExMgrUpdate(sys->tcbMgr);
    Demo3DCmdSys_Exit(sys);
    GFL_HeapFree(sys);
}

void Demo3DCmdSys_Main(Demo3DCmdSys *sys, fx32 frame, u32 droppedFrame) {
    Demo3DEngine *engine = sys->engine;
    const Demo3DCmd *cmd;
    int i;

    sys->droppedFrame = droppedFrame != 0;
    GFL_TCBExMgrUpdate(sys->tcbMgr);
    if (sys->finished || frame == -FX32_ONE) {
        return;
    }
    cmd = &Demo3DData_GetCmds(sys->demoId)[sys->cmdIndex];
    frame >>= FX32_SHIFT;
    if (cmd->frame == frame && sys->lastFrame != frame) {
        for (i = 0; i < CMD_MAX; i++) {
            Demo3DCmdSys_Run(sys, engine, cmd);
            cmd++;
            sys->finished = cmd->type == DEMO3D_CMD_END;
            sys->cmdIndex++;
            if (sys->finished == TRUE || cmd->frame != frame) {
                break;
            }
        }
    }
    sys->lastFrame = frame;
}

static void Demo3DCmdSys_Init(Demo3DCmdSys *sys) {
    sys->tcbMgr = GFL_TCBExMgrCreate(sys->heapId, sys->heapId, 16, 0x80);
    sys->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, sys->heapId);
    sys->arc = GFL_ArcSysCreateFileHandle(ARCID_DEMO3D_2D, sys->heapId);
    sys->textSpeed = TextSpeed_GetCurrentWait();
}

static void Demo3DCmdSys_Exit(Demo3DCmdSys *sys) {
    if (sys->capture != NULL) {
        Demo3DCapture_Stop(sys->capture);
    }
    GFL_TCBExMgrFreeTasks(sys->tcbMgr);
    if (sys->bgmPushed) {
        GFL_SndBGMPop();
        GFL_SndBGMSetPaused(FALSE);
        GFL_SndBGMFadeIn(60);
    }
    GFL_ArcToolFree(sys->arc);
    GFL_FontFree(sys->font);
    GFL_TCBExMgrFree(sys->tcbMgr);
}

// Runs the commands of the demo's start, and finds the first command at or after the frame the demo starts at.
// Returns whether the demo has no commands left
static BOOL Demo3DCmdSys_RunStartCmds(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *index) {
    const Demo3DCmd *cmds = Demo3DData_GetCmds(engine->scene.demoId);
    int i;

    for (i = 0; i < CMD_MAX; i++) {
        const Demo3DCmd *cmd = &cmds[i];

        if (cmd->type == DEMO3D_CMD_END) {
            return TRUE;
        }
        if (cmd->frame == DEMO3D_CMD_FRAME_START) {
            Demo3DCmdSys_Run(sys, engine, cmd);
        } else if (cmd->frame >= engine->scene.startFrame) {
            *index = i;
            return FALSE;
        }
    }
    return FALSE;
}

static void Demo3DCmdSys_Run(Demo3DCmdSys *sys, Demo3DEngine *engine, const Demo3DCmd *cmd) {
    gDemo3DCmdFuncs[cmd->type](sys, engine, (s32 *)cmd->params);
}
