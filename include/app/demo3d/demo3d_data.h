#ifndef POKEBW2_APP_DEMO3D_DEMO3D_DATA_H
#define POKEBW2_APP_DEMO3D_DEMO3D_DATA_H

// The 3D demos' scenes (overlay 293): each demo's 3D set-up, models and commands, named after the ROM's
// "demo3d_data.c"

#include "types.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "nitro/gx.h"

#define DEMO3D_ID_COUNT 27

// The commands of demo3d_cmd.c, in the order of its table
enum {
    DEMO3D_CMD_NONE,
    DEMO3D_CMD_SE_PLAY,
    DEMO3D_CMD_SE_STOP,
    DEMO3D_CMD_SE_VOLUME_FADE,
    DEMO3D_CMD_SE_PAN_FADE,
    DEMO3D_CMD_SE_PITCH_FADE,
    DEMO3D_CMD_BGM_PLAY,
    DEMO3D_CMD_BGM_STOP,
    DEMO3D_CMD_BGM_FADE,
    DEMO3D_CMD_BGM_CHANGE,
    DEMO3D_CMD_BRIGHTNESS_FADE,
    DEMO3D_CMD_FLASH,
    DEMO3D_CMD_LIGHT_COLOR,
    DEMO3D_CMD_LIGHT_VECTOR,
    DEMO3D_CMD_MESSAGE,
    DEMO3D_CMD_TELOP,
    DEMO3D_CMD_CAPTURE_START,
    DEMO3D_CMD_CAPTURE_END,
    DEMO3D_CMD_CAMERA,
    DEMO3D_CMD_END,
    DEMO3D_CMD_COUNT = DEMO3D_CMD_END
};

// The frame of a command that runs when the demo starts, whatever frame it starts at
#define DEMO3D_CMD_FRAME_START (-1)
#define DEMO3D_CMD_PARAM_COUNT 8

// What picks one of a model's variants
enum {
    DEMO3D_UNIT_SINGLE,
    DEMO3D_UNIT_BY_SEX,
    DEMO3D_UNIT_BY_PARAM,
    DEMO3D_UNIT_BY_SEASON,
};

// How DEMO3D_CMD_BGM_CHANGE changes the BGM once the old one has faded out
enum {
    DEMO3D_BGM_CHANGE_NOW,
    DEMO3D_BGM_CHANGE_STOP,
    DEMO3D_BGM_CHANGE_PLAY,
    DEMO3D_BGM_CHANGE_PUSH,
    DEMO3D_BGM_CHANGE_POP,
};

typedef struct {
    u32 type;
    // In frames of the camera's animation
    s32 frame;
    s32 params[DEMO3D_CMD_PARAM_COUNT];
} Demo3DCmd;

// A model's files in ARCID_DEMO3D_RESOURCE: the model and then its animations
typedef struct {
    const u16 *files;
    u32 count;
} Demo3DFiles;

// A variant of a model: one set of files, or one for each period of the day
typedef struct {
    const Demo3DFiles *files;
    u32 count;
} Demo3DVariant;

typedef struct {
    const Demo3DVariant *variants;
    u16 count;
    u16 type;
} Demo3DUnit;

typedef struct {
    // The camera's animation in ARCID_DEMO3D_RESOURCE
    u16 cameraFile;
    u16 unk2;
    // The zone whose light the demo takes
    u16 zoneId;
    // The camera's animation has 30 frames a second: every frame takes two
    u16 halfRate : 1;
    u16 dual3D : 1;
    u16 alphaBlend : 1;
    u16 antiAlias : 1;
    u16 fog : 1;
    u16 edgeMarking : 1;
    u16 sortMode : 1;
    u16 bufferMode : 1;
    u16 alphaTest : 1;
    u16 alphaTestRef : 5;
    // The projection's angles in degrees, the sine's and the cosine's
    fx32 fovySin;
    fx32 fovyCos;
    fx32 aspect;
    fx32 near;
    fx32 far;
    fx32 scaleW;
    // How far each screen's view is moved up or down when the demo draws on both
    s16 mainOffset;
    s16 subOffset;
    fx32 polygon1DotDepth;
    GXRgb clearColor;
    u16 clearAlpha;
    u16 clearPolygonId : 15;
    u16 clearDepth;
    GXRgb fogColor;
    u8 fogAlpha;
    u8 fogShift;
    u16 fogOffset : 15;
    u16 fogAlphaMode : 1;
    u8 fogTable[8];
    GXRgb edgeColors[8];
    // Whether the demo fades in from white and how many frames it takes, and the same for its fade-out
    u8 fadeInWhite;
    u8 fadeInFrames;
    u8 fadeOutWhite;
    u8 fadeOutFrames;
} Demo3DSetup;

// What the engine picks a demo's models with
typedef struct {
    u32 demoId;
    u32 startFrame;
    u8 param;
    u8 dayPeriod;
    u8 playerSex;
    u8 hour;
    u8 minute;
    u8 season;
} Demo3DSceneParam;

const Demo3DSetup *Demo3DData_GetSetup(u32 demoId);
G3DCurve *Demo3DData_LoadCamera(u32 demoId, HeapID heapId, u32 preloadCount);
u8 Demo3DData_GetUnitCount(u32 demoId);
// A scene set-up of one actor with up to 5 animations, which Demo3DData_SetSceneSetup fills in for a model
G3DSceneSetup *Demo3DData_CreateSceneSetup(HeapID heapId);
void Demo3DData_FreeSceneSetup(G3DSceneSetup *setup);
void Demo3DData_SetSceneSetup(G3DSceneSetup *setup, const Demo3DSceneParam *param, u8 unit);
// How far the camera's animation goes in a frame
fx32 Demo3DData_GetFrameStep(u32 demoId);
BOOL Demo3DData_IsDual3D(u32 demoId);
void Demo3DData_GetFadeParams(u32 demoId, u8 fadeOut, u8 *white, u8 *frames);
const Demo3DCmd *Demo3DData_GetCmds(u32 demoId);
// The commands that run when the demo ends
const Demo3DCmd *Demo3DData_GetEndCmds(u32 demoId);

#endif // POKEBW2_APP_DEMO3D_DEMO3D_DATA_H
