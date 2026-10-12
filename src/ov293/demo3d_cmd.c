#include "types.h"
#include "app/demo3d/demo3d_cmd.h"
#include "app/demo3d/demo3d_capture.h"
#include "app/demo3d/demo3d_cmdsys.h"
#include "app/demo3d/demo3d_data.h"
#include "app/demo3d/demo3d_engine.h"
#include "app/demo3d/demo3d_graphic.h"
#include "constants/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "system/bmp_winframe.h"
#include "system/brightness.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/version.h"

// The 3D demos' commands: sound effects and BGM, the screens' brightness, the lights, a message window, a picture
// faded in over the 3D, the motion blur and the camera's projection. The ROM has no name for this file, which lies
// between demo3d_data.c and demo3d_cmdsys.c; demo3d_cmd.c is a guess. A command that takes time adds a task to the
// command system's task manager

// What a sound effect's fade changes
enum {
    SE_PARAM_VOLUME,
    SE_PARAM_PAN,
    SE_PARAM_PITCH,
};

#define SE_PARAM_KEEP (-1)
#define BGM_CHANNELS_ALL 0xffff
#define BRIGHTNESS_PLANES_ALL 0x3f

typedef struct {
    Demo3DCmdSys *sys;
    u8 unk4;
    u8 param;
    u8 player;
    s32 frames;
    s32 start;
    s32 end;
    fx32 value;
    fx32 step;
} Demo3DSeFade;

typedef struct {
    Demo3DCmdSys *sys;
    u32 unk4;
    u8 state;
    u8 push;
    u8 pop;
    u8 type;
    u32 bgm;
    s32 fadeOutFrames;
    s32 fadeInFrames;
} Demo3DBgmChange;

typedef struct {
    Demo3DCmdSys *sys;
    u32 unk4;
    u32 screens;
    u32 planes;
    s32 frames;
    s32 start;
    s32 end;
    fx32 value;
    fx32 step;
    // Counts the fades of a flash that are running
    int *counter;
} Demo3DBrightnessFade;

typedef struct {
    Demo3DCmdSys *sys;
    u32 unk4;
    s8 state;
    s8 start;
    s8 peak;
    s8 end;
    s32 inFrames;
    s32 outFrames;
    int fades;
    u32 screens;
    u32 planes;
} Demo3DFlash;

typedef struct {
    Demo3DCmdSys *sys;
    u32 state;
    s32 counter;
    s32 frames;
    u32 textBank;
    u32 message;
    u32 frameChars;
    BmpWin *window;
    PrintStream *stream;
    StrBuf *strbuf;
} Demo3DMessage;

typedef struct {
    Demo3DCmdSys *sys;
    u32 state;
    s32 counter;
    s32 frames;
    u32 paletteFile;
    u32 charFile;
    u32 screenFile;
    s32 fadeInFrames;
    s32 fadeOutFrames;
    fx32 alpha;
    fx32 step;
} Demo3DTelop;

static void Demo3DCmd_SePlay(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_SeStop(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_SeVolumeFade(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_SePanFade(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_SePitchFade(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_AddSeFade(Demo3DCmdSys *sys, u8 param, u16 se, s32 start, s32 end, s32 frames, s32 player);
static void Demo3DCmd_SeFadeTask(TCBEx *task, void *data);
static void Demo3DCmd_SetSeParam(u8 param, u8 player, s32 value);
static void Demo3DCmd_BgmPlay(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_BgmStop(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_BgmFade(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_BgmChange(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_AddBgmChange(Demo3DCmdSys *sys, u32 bgm, s32 fadeOutFrames, s32 fadeInFrames, s32 type);
static void Demo3DCmd_BgmChangeTask(TCBEx *task, void *data);
static void Demo3DCmd_BrightnessFade(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_AddBrightnessFade(Demo3DCmdSys *sys, u32 screens, u32 planes, s32 start, s32 end, s32 frames,
                                        int *counter);
static void Demo3DCmd_BrightnessFadeTask(TCBEx *task, void *data);
static void Demo3DCmd_Flash(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_AddFlash(Demo3DCmdSys *sys, u8 screens, s8 start, s8 peak, s8 end, s32 inFrames, s32 outFrames);
static void Demo3DCmd_FlashTask(TCBEx *task, void *data);
static void Demo3DCmd_LightColor(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_LightVector(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_Message(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_AddMessage(Demo3DCmdSys *sys, u32 textBank, u32 message, s32 frames, u8 x, u8 y, u8 width,
                                 u8 height);
static void Demo3DCmd_MessageTask(TCBEx *task, void *data);
static void Demo3DCmd_InitMessage(Demo3DCmdSys *sys, Demo3DMessage *work, u32 textBank, u32 message, s32 frames, u8 x,
                                  u8 y, u8 width, u8 height);
static void Demo3DCmd_FreeMessage(Demo3DMessage *work);
static void Demo3DCmd_Telop(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_AddTelop(Demo3DCmdSys *sys, u32 paletteFile, u32 charFile, u32 screenFile, s32 frames,
                               s32 fadeInFrames, s32 fadeOutFrames);
static void Demo3DCmd_TelopTask(TCBEx *task, void *data);
static void Demo3DCmd_InitTelop(Demo3DCmdSys *sys, Demo3DTelop *work, u32 paletteFile, u32 charFile, u32 screenFile,
                                s32 frames, s32 fadeInFrames, s32 fadeOutFrames);
static void Demo3DCmd_FreeTelop(Demo3DTelop *work);
static void Demo3DCmd_CaptureStart(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_CaptureEnd(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static void Demo3DCmd_Camera(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params);
static s32 Demo3DCmd_ConvFrames(Demo3DCmdSys *sys, s32 frames);

Demo3DCmdFunc gDemo3DCmdFuncs[DEMO3D_CMD_COUNT + 1] = {
    NULL,
    Demo3DCmd_SePlay,
    Demo3DCmd_SeStop,
    Demo3DCmd_SeVolumeFade,
    Demo3DCmd_SePanFade,
    Demo3DCmd_SePitchFade,
    Demo3DCmd_BgmPlay,
    Demo3DCmd_BgmStop,
    Demo3DCmd_BgmFade,
    Demo3DCmd_BgmChange,
    Demo3DCmd_BrightnessFade,
    Demo3DCmd_Flash,
    Demo3DCmd_LightColor,
    Demo3DCmd_LightVector,
    Demo3DCmd_Message,
    Demo3DCmd_Telop,
    Demo3DCmd_CaptureStart,
    Demo3DCmd_CaptureEnd,
    Demo3DCmd_Camera,
    NULL,
};

// Plays a sound effect: the effect, its volume, pan and pitch, and the player to play it on. -1 leaves any of the last
// four as it is
static void Demo3DCmd_SePlay(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    s32 player;

    if (params[4] != SE_PARAM_KEEP) {
        GFL_SEPlayKeepVol(params[0], params[4]);
        player = params[4];
    } else {
        GFL_SndSEPlay(params[0]);
        player = GFL_SndSeqGetPlayerIndex(params[0]);
    }
    if (params[1] != SE_PARAM_KEEP) {
        GFL_SndPlayerSetVolume(player, params[1]);
    }
    if (params[2] != SE_PARAM_KEEP) {
        GFL_SndPlayerSetParams(player, -1, -1, params[2]);
    }
    if (params[3] != SE_PARAM_KEEP) {
        GFL_SndPlayerSetParams(player, -1, params[3], -1);
    }
}

// Stops a sound effect, or a player
static void Demo3DCmd_SeStop(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    s32 player = params[1];

    if (player == SE_PARAM_KEEP) {
        player = GFL_SndSeqGetPlayerIndex(params[0]);
    }
    GFL_SndPlayerStop(player);
}

// The fades of a sound effect: the effect, the value at the start and at the end, the frames and the player
static void Demo3DCmd_SeVolumeFade(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    Demo3DCmd_AddSeFade(sys, SE_PARAM_VOLUME, params[0], params[1], params[2], params[3], params[4]);
}

static void Demo3DCmd_SePanFade(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    Demo3DCmd_AddSeFade(sys, SE_PARAM_PAN, params[0], params[1], params[2], params[3], params[4]);
}

static void Demo3DCmd_SePitchFade(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    Demo3DCmd_AddSeFade(sys, SE_PARAM_PITCH, params[0], params[1], params[2], params[3], params[4]);
}

static void Demo3DCmd_AddSeFade(Demo3DCmdSys *sys, u8 param, u16 se, s32 start, s32 end, s32 frames, s32 player) {
    Demo3DSeFade *work =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(sys->tcbMgr, Demo3DCmd_SeFadeTask, sizeof(Demo3DSeFade), 0));

    sys_memset(work, 0, sizeof(Demo3DSeFade));
    work->sys = sys;
    work->param = param;
    work->start = start;
    work->end = end;
    work->frames = Demo3DCmd_ConvFrames(sys, frames);
    if (player == SE_PARAM_KEEP) {
        work->player = GFL_SndSeqGetPlayerIndex(se);
    } else {
        work->player = player;
    }
    work->value = FX32_CONST(start);
    work->step = FX_Div(FX32_CONST(end) - work->value, FX32_CONST(work->frames));
}

static void Demo3DCmd_SeFadeTask(TCBEx *task, void *data) {
    Demo3DSeFade *work = data;

    if (work->frames-- > 0) {
        Demo3DCmd_SetSeParam(work->param, work->player, work->value >> FX32_SHIFT);
        work->value += work->step;
    } else {
        Demo3DCmd_SetSeParam(work->param, work->player, work->end);
        GFL_TCBExRequestEnd(task);
    }
}

static void Demo3DCmd_SetSeParam(u8 param, u8 player, s32 value) {
    switch (param) {
    case SE_PARAM_VOLUME:
        GFL_SndPlayerSetVolume(player, value);
        break;
    case SE_PARAM_PAN:
        GFL_SndPlayerSetParams(player, -1, -1, value);
        break;
    case SE_PARAM_PITCH:
        GFL_SndPlayerSetParams(player, -1, value, -1);
        break;
    }
}

static void Demo3DCmd_BgmPlay(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    GFL_SndBGMPlay(params[0], BGM_CHANNELS_ALL);
}

static void Demo3DCmd_BgmStop(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    func_02005d8c();
}

// Fades the BGM in (0) or out, in a number of frames
static void Demo3DCmd_BgmFade(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    s32 frames = Demo3DCmd_ConvFrames(sys, params[1]);

    if (params[0] == 0) {
        GFL_SndBGMFadeIn(frames);
    } else {
        GFL_SndBGMFadeOut(frames);
    }
}

// Changes the BGM: the new one, the frames the old one fades out in and the new one fades in, and a
// DEMO3D_BGM_CHANGE_*
static void Demo3DCmd_BgmChange(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    u32 bgm = params[0];
    s32 fadeOutFrames = Demo3DCmd_ConvFrames(sys, params[1]);
    s32 fadeInFrames = Demo3DCmd_ConvFrames(sys, params[2]);

    if (params[3] == DEMO3D_BGM_CHANGE_NOW) {
        GFL_SndBGMPlay(bgm, BGM_CHANNELS_ALL);
        GFL_SndBGMFadeIn(fadeInFrames);
    } else {
        Demo3DCmd_AddBgmChange(sys, bgm, fadeOutFrames, fadeInFrames, params[3]);
    }
}

static void Demo3DCmd_AddBgmChange(Demo3DCmdSys *sys, u32 bgm, s32 fadeOutFrames, s32 fadeInFrames, s32 type) {
    BOOL push = FALSE;
    Demo3DBgmChange *work =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(sys->tcbMgr, Demo3DCmd_BgmChangeTask, sizeof(Demo3DBgmChange), 0));

    sys_memset(work, 0, sizeof(Demo3DBgmChange));
    work->sys = sys;
    work->type = type;
    if (type == DEMO3D_BGM_CHANGE_PUSH) {
        push = TRUE;
    }
    work->push = push;
    work->pop = type == DEMO3D_BGM_CHANGE_POP;
    work->bgm = bgm;
    work->fadeOutFrames = fadeOutFrames;
    work->fadeInFrames = fadeInFrames;
    if (fadeOutFrames != 0) {
        GFL_SndBGMFadeOut(fadeOutFrames);
    } else {
        work->state = 1;
    }
}

static void Demo3DCmd_BgmChangeTask(TCBEx *task, void *data) {
    Demo3DBgmChange *work = data;

    switch (work->state) {
    case 0:
        if (GFL_SndBGMIsFading()) {
            return;
        }
        work->state++;
        // fallthrough
    case 1:
        if (work->push) {
            if (!work->sys->bgmPushed) {
                GFL_SndBGMSetPaused(TRUE);
                GFL_SndBGMPush();
                work->sys->bgmPushed = TRUE;
            }
        } else {
            func_02005d8c();
            if (work->type == DEMO3D_BGM_CHANGE_STOP) {
                break;
            }
        }
        if (work->pop) {
            if (work->sys->bgmPushed) {
                GFL_SndBGMPop();
                GFL_SndBGMSetPaused(FALSE);
                work->sys->bgmPushed = FALSE;
            }
        } else {
            GFL_SndBGMPlay(work->bgm, BGM_CHANNELS_ALL);
        }
        if (work->fadeInFrames != 0) {
            GFL_SndBGMFadeIn(work->fadeInFrames);
        }
        break;
    }
    GFL_TCBExRequestEnd(task);
}

// Fades the screens' brightness: the screens, the frames, and the brightness at the start and at the end. With no
// frames it sets the first brightness at once
static void Demo3DCmd_BrightnessFade(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    if (params[1] == 0) {
        BrightnessController_SetScreenBrightness(params[2], BRIGHTNESS_PLANES_ALL, params[0]);
    } else {
        Demo3DCmd_AddBrightnessFade(sys, params[0], BRIGHTNESS_PLANES_ALL, params[2], params[3],
                                    Demo3DCmd_ConvFrames(sys, params[1]), NULL);
    }
}

static void Demo3DCmd_AddBrightnessFade(Demo3DCmdSys *sys, u32 screens, u32 planes, s32 start, s32 end, s32 frames,
                                        int *counter) {
    Demo3DBrightnessFade *work = GFL_TCBExGetData(
        GFL_TCBExMgrAddTask(sys->tcbMgr, Demo3DCmd_BrightnessFadeTask, sizeof(Demo3DBrightnessFade), 0));

    sys_memset(work, 0, sizeof(Demo3DBrightnessFade));
    work->sys = sys;
    work->screens = screens;
    work->planes = planes;
    work->start = start;
    work->end = end;
    work->frames = frames;
    work->counter = counter;
    work->value = FX32_CONST(start);
    work->step = FX_Div(FX32_CONST(end) - work->value, FX32_CONST(frames));
    if (counter != NULL) {
        (*counter)++;
    }
    BrightnessController_SetScreenBrightness(work->start, work->planes, work->screens);
}

static void Demo3DCmd_BrightnessFadeTask(TCBEx *task, void *data) {
    Demo3DBrightnessFade *work = data;

    if (work->sys->ending) {
        BrightnessController_SetScreenBrightness(work->end, work->planes, work->screens);
        if (work->counter != NULL) {
            (*work->counter)--;
        }
        GFL_TCBExRequestEnd(task);
        return;
    }
    if (work->frames-- > 0) {
        BrightnessController_SetScreenBrightness(work->value >> FX32_SHIFT, work->planes, work->screens);
        work->value += work->step;
    } else {
        BrightnessController_SetScreenBrightness(work->end, work->planes, work->screens);
        if (work->counter != NULL) {
            (*work->counter)--;
        }
        GFL_TCBExRequestEnd(task);
    }
}

// Flashes the screens: the screens, the brightness at the start, at the peak and at the end, and the frames to the
// peak and from it
static void Demo3DCmd_Flash(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    s32 inFrames = Demo3DCmd_ConvFrames(sys, params[4]);
    s32 outFrames = Demo3DCmd_ConvFrames(sys, params[5]);

    Demo3DCmd_AddFlash(sys, params[0], params[1], params[2], params[3], inFrames, outFrames);
}

static void Demo3DCmd_AddFlash(Demo3DCmdSys *sys, u8 screens, s8 start, s8 peak, s8 end, s32 inFrames, s32 outFrames) {
    Demo3DFlash *work =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(sys->tcbMgr, Demo3DCmd_FlashTask, sizeof(Demo3DFlash), 1));

    sys_memset(work, 0, sizeof(Demo3DFlash));
    work->sys = sys;
    work->start = start;
    work->peak = peak;
    work->end = end;
    work->screens = screens;
    work->planes = BRIGHTNESS_PLANES_ALL;
    work->inFrames = inFrames;
    work->outFrames = outFrames;
    work->fades = 0;
}

static void Demo3DCmd_FlashTask(TCBEx *task, void *data) {
    Demo3DFlash *work = data;
    Demo3DCmdSys *sys = work->sys;

    if (sys->ending) {
        BrightnessController_SetScreenBrightness(work->end, work->planes, work->screens);
        GFL_TCBExRequestEnd(task);
        return;
    }
    switch (work->state) {
    case 0:
        Demo3DCmd_AddBrightnessFade(sys, work->screens, work->planes, work->start, work->peak, work->inFrames,
                                    &work->fades);
        work->state++;
        break;
    case 1:
        if (work->fades == 0) {
            work->state++;
        }
        break;
    case 2:
        Demo3DCmd_AddBrightnessFade(sys, work->screens, work->planes, work->peak, work->end, work->outFrames,
                                    &work->fades);
        work->state++;
        break;
    case 3:
        if (work->fades == 0) {
            GFL_TCBExRequestEnd(task);
        }
        break;
    }
}

// Sets a light's color: the light and the color's red, green and blue
static void Demo3DCmd_LightColor(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    Demo3DGraphic_SetLightColor(engine->graphic, params[0], GX_RGB(params[1], params[2], params[3]));
}

// Sets a light's direction: the light and the vector
static void Demo3DCmd_LightVector(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    VecFx16 direction;

    direction.x = params[1];
    direction.y = params[2];
    direction.z = params[3];
    Demo3DGraphic_SetLightVector(engine->graphic, params[0], &direction);
}

// Shows a message in a window on the upper screen: the text bank, the first message and how many messages on the one
// for each value of the demo's parameter, the frames it stays, and the window's place and size
static void Demo3DCmd_Message(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    u32 message = params[1] + params[2] * engine->scene.param;

    Demo3DCmd_AddMessage(sys, params[0], message, Demo3DCmd_ConvFrames(sys, params[3]), params[4], params[5],
                         params[6], params[7]);
}

static void Demo3DCmd_AddMessage(Demo3DCmdSys *sys, u32 textBank, u32 message, s32 frames, u8 x, u8 y, u8 width,
                                 u8 height) {
    Demo3DMessage *work =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(sys->tcbMgr, Demo3DCmd_MessageTask, sizeof(Demo3DMessage), 1));

    Demo3DCmd_InitMessage(sys, work, textBank, message, frames, x, y, width, height);
}

static void Demo3DCmd_MessageTask(TCBEx *task, void *data) {
    Demo3DMessage *work = data;
    Demo3DCmdSys *sys = work->sys;

    if (sys->ending) {
        Demo3DCmd_FreeMessage(work);
        GFL_TCBExRequestEnd(task);
        return;
    }
    switch (work->state) {
    case 0: {
        MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, work->textBank, sys->tailHeapId);
        BmpWin *window;

        work->strbuf = GFL_MsgDataLoadStrbufNew(msgData, work->message);
        GFL_TextRndUpdateColorIndexLUT(1, 2, 15);
        work->stream = func_02022268(work->window, 0, 0, work->strbuf, sys->font, sys->textSpeed, sys->tcbMgr, 0,
                                     sys->heapId, 15);
        window = work->window;
        BmpWin_FlushChar(window);
        BmpWin_FlushMap(window);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
        BmpWin_DrawFrameEndOfHeap(work->window, 1, work->frameChars, 0);
        GFL_MsgDataFree(msgData);
        GFL_BGSysSetBGPriority(1, 0);
        GFL_BGSysSetBGPriority(0, 1);
        GFL_BGSysSetBGEnabled(1, TRUE);
        work->state++;
        break;
    }
    case 1:
        if (work->counter++ >= work->frames) {
            Demo3DCmd_FreeMessage(work);
            GFL_TCBExRequestEnd(task);
        }
        break;
    }
}

static void Demo3DCmd_InitMessage(Demo3DCmdSys *sys, Demo3DMessage *work, u32 textBank, u32 message, s32 frames, u8 x,
                                  u8 y, u8 width, u8 height) {
    sys_memset(work, 0, sizeof(Demo3DMessage));
    work->textBank = textBank;
    work->frames = frames;
    work->sys = sys;
    work->message = message;
    work->frameChars = LoadCursorImageEndOfHeap(1, 0, 1, sys->tailHeapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x20, 0x20, sys->tailHeapId);
    work->window = BmpWin_CreateDynamic(1, x, y, width, height, 1, 0);
    GFL_BitmapFill(BmpWin_GetBitmap(work->window), 15);
}

static void Demo3DCmd_FreeMessage(Demo3DMessage *work) {
    if (work->stream != NULL) {
        func_020223cc(work->stream);
    }
    GFL_StrBufFree(work->strbuf);
    func_020232d8();
    BmpWin_ClearFrame(work->window, 1);
    BmpWin_Free(work->window);
    FreeCursorImageEndOfHeap(1, work->frameChars);
    sys_memset(work, 0, sizeof(Demo3DMessage));
    GFL_BGSysSetBGPriority(1, 1);
    GFL_BGSysSetBGPriority(0, 0);
    GFL_BGSysSetBGEnabled(1, FALSE);
}

// Fades a picture of ARCID_DEMO3D_2D in over the 3D and out again: the game version it is for or 0xff for both, its
// palette, characters and screen, the frames it stays, and the frames it fades in and out in
static void Demo3DCmd_Telop(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    if (params[0] == 0xff || params[0] == game_version) {
        s32 frames = Demo3DCmd_ConvFrames(sys, params[4]);
        s32 fadeInFrames = Demo3DCmd_ConvFrames(sys, params[5]);
        s32 fadeOutFrames = Demo3DCmd_ConvFrames(sys, params[6]);

        Demo3DCmd_AddTelop(sys, params[1], params[2], params[3], frames, fadeInFrames, fadeOutFrames);
    }
}

static void Demo3DCmd_AddTelop(Demo3DCmdSys *sys, u32 paletteFile, u32 charFile, u32 screenFile, s32 frames,
                               s32 fadeInFrames, s32 fadeOutFrames) {
    Demo3DTelop *work =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(sys->tcbMgr, Demo3DCmd_TelopTask, sizeof(Demo3DTelop), 1));

    Demo3DCmd_InitTelop(sys, work, paletteFile, charFile, screenFile, frames, fadeInFrames, fadeOutFrames);
}

static void Demo3DCmd_TelopTask(TCBEx *task, void *data) {
    Demo3DTelop *work = data;
    Demo3DCmdSys *sys = work->sys;

    if (sys->ending) {
        Demo3DCmd_FreeTelop(work);
        GFL_TCBExRequestEnd(task);
        return;
    }
    switch (work->state) {
    case 0:
        GFL_G2DIOLoadArcNCLRDefault(sys->arc, work->paletteFile, 0, 0, 0, sys->tailHeapId);
        GFL_G2DIOLoadNSCRSync(sys->arc, work->screenFile, 1, 0, 0, 0, TRUE, sys->tailHeapId);
        work->state++;
        break;
    case 1:
        work->counter = 0;
        work->alpha = 0;
        work->step = FX_Div(FX32_CONST(16), FX32_CONST(work->fadeInFrames));
        GFL_BGSysSetBGPriority(1, 0);
        GFL_BGSysSetBGPriority(0, 1);
        gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1, GX_BLEND_PLANEMASK_BG0, 0, 16);
        GFL_BGSysSetBGEnabled(1, TRUE);
        work->state++;
        break;
    case 2:
        if (work->counter++ < work->fadeInFrames) {
            work->alpha += work->step;
            G2_ChangeBlendAlpha(work->alpha >> FX32_SHIFT, 16 - (work->alpha >> FX32_SHIFT));
        } else {
            G2_ChangeBlendAlpha(16, 0);
            work->counter = 0;
            work->state++;
        }
        break;
    case 3:
        if (work->counter++ >= work->frames) {
            work->counter = 0;
            work->alpha = 0;
            work->step = FX_Div(FX32_CONST(16), FX32_CONST(work->fadeOutFrames));
            work->state++;
        }
        break;
    case 4:
        if (work->counter++ < work->fadeOutFrames) {
            work->alpha += work->step;
            G2_ChangeBlendAlpha(16 - (work->alpha >> FX32_SHIFT), work->alpha >> FX32_SHIFT);
        } else {
            G2_ChangeBlendAlpha(0, 16);
            work->counter = 0;
            work->state++;
        }
        break;
    default:
        Demo3DCmd_FreeTelop(work);
        GFL_TCBExRequestEnd(task);
        break;
    }
}

static void Demo3DCmd_InitTelop(Demo3DCmdSys *sys, Demo3DTelop *work, u32 paletteFile, u32 charFile, u32 screenFile,
                                s32 frames, s32 fadeInFrames, s32 fadeOutFrames) {
    sys_memset(work, 0, sizeof(Demo3DTelop));
    work->sys = sys;
    work->frames = frames;
    work->paletteFile = paletteFile;
    work->screenFile = screenFile;
    work->charFile = charFile;
    work->fadeInFrames = fadeInFrames;
    work->fadeOutFrames = fadeOutFrames;
    GFL_BGSysLoadArcNCGRStatic(sys->arc, work->charFile, 1, 0, 0, TRUE, sys->tailHeapId);
}

static void Demo3DCmd_FreeTelop(Demo3DTelop *work) {
    sys_memset(work, 0, sizeof(Demo3DTelop));
    GFL_BGSysSetBGEnabled(1, FALSE);
    GFL_BGSysSetBGPriority(0, 0);
    GFL_BGSysSetBGPriority(1, 1);
    G2_BlendNone();
    GFL_BGSysClearBG(1);
}

// Starts the motion blur: the blend of the new frame and of the frames before it
static void Demo3DCmd_CaptureStart(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    sys->capture = Demo3DCapture_Start(sys->tcbMgr, params[0], params[1]);
}

static void Demo3DCmd_CaptureEnd(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    Demo3DCapture_Stop(sys->capture);
    sys->capture = NULL;
}

// Changes the camera's projection: Demo3DEngine_SetCamera's parameters
static void Demo3DCmd_Camera(Demo3DCmdSys *sys, Demo3DEngine *engine, s32 *params) {
    Demo3DEngine_SetCamera(engine, params[0], params[1], params[2], params[3]);
}

// A number of frames of the screen as frames of a demo that plays every other frame
static s32 Demo3DCmd_ConvFrames(Demo3DCmdSys *sys, s32 frames) {
    if (sys->engine->setup->halfRate == TRUE) {
        frames *= 2;
    }
    return frames;
}
