#include "types.h"
#include "constants/arc.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/fld_faceup.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "save/config.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The face-up (the file name is the ROM's): a close-up of a character's face over the field. The face is on BG 2 and
// its eyes and mouth on BG 3, where two animations write the frames' map entries, which a VBlank task loads

// The script sub event of the message that the mouth moves with (scrcmd_work.c tests it as well)
#define SCRIPT_SUB_EVENT_MSG 3

#define FACEUP_BG_FACE 2
#define FACEUP_BG_PARTS 3

// The frames of the alpha fade from BG 3 to BG 2
#define FACEUP_FADE_FRAMES 30

typedef struct {
    u16 index;
    u16 wait;
} FaceUpAnimFrame;

// An animation of a rectangle of map entries, two rows of up to 6
typedef struct {
    BOOL stopped;
    // The map entries have changed and wait for the VBlank task
    BOOL updated;
    u8 wait;
    u8 interval;
    u16 frame;
    u16 screen[2][6];
} FaceUpAnim;

typedef struct {
    u8 bgPriorities[4];
    u32 enabledBGs;
    GXBg23ControlText bg3Control;
    u32 type;
    FieldScriptEnv *env;
    TCB *tcb;
    TCB *vblankTcb;
    // The message has finished, so the mouth stays still
    BOOL talkEnded;
    u32 unk20;
    u16 fadeCount;
    u16 blinkWait;
    FaceUpAnim eyes;
    FaceUpAnim mouth;
    BOOL endBlack;
    // No message has printed since the mouth was readied
    BOOL waitingForMsg;
} FaceUpWork;

static GameEventReturnCode EventFaceUpStart_Callback(GameEvent *event, u32 *state, void *data);
static void FaceUp_LoadGraphics(FaceUpWork *work, Field *field);
static GameEventReturnCode EventFaceUpEnd_Callback(GameEvent *event, u32 *state, void *data);
static void FaceUp_Free(Field *field, FaceUpWork *work);
static void FaceUp_SaveBGPriorities(FaceUpWork *work);
static void FaceUp_SaveEnabledBGs(FaceUpWork *work);
static void FaceUp_RestoreBGPriorities(FaceUpWork *work);
static void FaceUp_RestoreEnabledBGs(FaceUpWork *work);
static BOOL FaceUp_UpdateFadeIn(FaceUpWork *work);
static void FaceUp_Task(TCB *tcb, void *data);
static void FaceUp_VBlankTask(TCB *tcb, void *data);
static void FaceUpAnim_Update(const FaceUpAnimFrame *frames, u32 count, u32 base, u32 stride, u32 width,
                              FaceUpAnim *anim);
static void FaceUpAnim_Init(FaceUpAnim *anim);

static const FaceUpAnimFrame sMouthFrames[4] = { { 1, 3 }, { 2, 3 }, { 1, 3 }, { 0, 3 } };

static const FaceUpAnimFrame sEyeFrames[5] = { { 0, 2 }, { 1, 2 }, { 2, 2 }, { 1, 2 }, { 0, 2 } };

static const BGSetup sFaceBGSetup = {
    0,
    0,
    0x800,
    0,
    BGRES_256x256,
    GX_BG_COLORMODE_256,
    GX_BG_SCRBASE(0xf800),
    GX_BG_CHARBASE(0x18000),
    0x8000,
    GX_BG_EXTPLTT_01,
    2,
    GX_BG_AREAOVER_XLU,
    FALSE,
};

GameEvent *EventFaceUpStart_Create(u8 type, u8 unused, u16 endBlack, GameSystem *gsys, FieldScriptEnv *env) {
    FaceUpWork *work;
    Field *field = GSYS_GetField(gsys);
    u32 textSpeed;
    GameEvent *event;

    work = GFL_HeapAllocate(HEAPID_FIELD_PARTICLE, sizeof(FaceUpWork), TRUE, "fld_faceup.c", 162);
    if (*(FaceUpWork **)Field_GetNDemoDataHandle(field) != NULL) {
        // BUG: The work allocated above is never freed
#ifdef BUGFIX
        GFL_HeapFree(work);
#endif
        return NULL;
    }
    *(FaceUpWork **)Field_GetNDemoDataHandle(field) = work;
    work->type = type;
    work->env = env;
    work->endBlack = endBlack;
    FaceUpAnim_Init(&work->eyes);
    FaceUpAnim_Init(&work->mouth);

    textSpeed =
        Config_GetTextSpeed((Config *)getTrainerDataBlkAddress(GameData_GetSaveControl(GSYS_GetGameData(gsys))));
    if (textSpeed == 0) {
        work->mouth.interval = 3;
    } else if (textSpeed == 1) {
        work->mouth.interval = 2;
    } else {
        work->mouth.interval = 1;
    }
    work->eyes.interval = 2;

    event = GameEvent_Create(gsys, NULL, EventFaceUpStart_Callback, 0);
    work->tcb = GFL_TCBMgrAddTask(Field_GetTCBMgr(field), FaceUp_Task, work, 0);
    work->vblankTcb = GFL_VBlankTCBAdd(FaceUp_VBlankTask, work, 1);
    work->talkEnded = FALSE;
    work->waitingForMsg = TRUE;
    return event;
}

static GameEventReturnCode EventFaceUpStart_Callback(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    FaceUpWork *work = *(FaceUpWork **)Field_GetNDemoDataHandle(field);

    switch (*state) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_BLACK, 0, 16, 0);
        (*state)++;
        break;
    case 1:
        if (GFL_FadeIsRunning()) {
            break;
        }
        FaceUp_LoadGraphics(work, field);
        GFL_FadeSet(FADE_ENGINE_A_BLACK, 16, 0, 0);
        (*state)++;
        break;
    case 2:
        if (GFL_FadeIsRunning()) {
            break;
        }
        work->fadeCount = 0;
        (*state)++;
        break;
    case 3:
        if (FaceUp_UpdateFadeIn(work)) {
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}

static void FaceUp_LoadGraphics(FaceUpWork *work, Field *field) {
    void *msgBGSys;
    BGSetup setup;
    u32 charFile;
    u32 scrFile;
    u32 palFile;
    void *file;
    NNSG2dCharacterData *character;
    NNSG2dScreenData *screen;
    NNSG2dPaletteData *palette;
    NNSG2dCharacterData *partsCharacter;
    NNSG2dScreenData *partsScreen;

    FaceUp_SaveBGPriorities(work);
    FaceUp_SaveEnabledBGs(work);
    work->bg3Control = G2_GetBG3ControlText();
    G2_SetBG3ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_256, GX_BG_SCRBASE(0x1000),
                         GX_BG_CHARBASE(0x04000));
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG3, GX_PLANEMASK_BG2, 0, 16);

    msgBGSys = Field_GetMsgBGSys(field);
    if (func_ov036_02187868(msgBGSys)) {
        func_ov036_021879c0(msgBGSys);
    } else {
        GFL_BGSysReleaseBG(FACEUP_BG_FACE);
    }

    setup = sFaceBGSetup;
    GFL_BGSysCreateBG(FACEUP_BG_FACE, &setup, BGMODE_TEXT);

    if (work->type == 0) {
        charFile = 4;
        scrFile = 8;
        palFile = 0;
    } else if (work->type == 1) {
        charFile = 5;
        scrFile = 9;
        palFile = 1;
    } else {
        charFile = 6;
        scrFile = 10;
        palFile = 2;
    }

    file = GFL_ArcSysReadHeapNew(ARCID_FLD_FACEUP, charFile, HEAPID_TAIL(HEAPID_FIELDMAP));
    NNS_G2dGetUnpackedBGCharacterData(file, &character);
    GFL_BGSysLoadChar(FACEUP_BG_FACE, character->rawData, character->size, 0);
    GFL_HeapFree(file);

    file = GFL_ArcSysReadHeapNew(ARCID_FLD_FACEUP, scrFile, HEAPID_TAIL(HEAPID_FIELDMAP));
    NNS_G2dGetUnpackedScreenData(file, &screen);
    GFL_BGSysLoadScrCore(FACEUP_BG_FACE, screen->rawData, screen->size, 0);
    GFL_HeapFree(file);

    file = GFL_ArcSysReadHeapNew(ARCID_FLD_FACEUP, palFile, HEAPID_TAIL(HEAPID_FIELDMAP));
    NNS_G2dGetUnpackedPaletteData(file, &palette);
    GFL_BGSysUploadStdPalette(FACEUP_BG_FACE, palette->rawData, 0x140, 0);
    GFL_HeapFree(file);

    file = GFL_ArcSysReadHeapNew(ARCID_FLD_FACEUP, 7, HEAPID_TAIL(HEAPID_FIELDMAP));
    NNS_G2dGetUnpackedBGCharacterData(file, &partsCharacter);
    GFL_BGSysLoadChar(FACEUP_BG_PARTS, partsCharacter->rawData, partsCharacter->size, 0);
    GFL_HeapFree(file);

    file = GFL_ArcSysReadHeapNew(ARCID_FLD_FACEUP, 11, HEAPID_TAIL(HEAPID_FIELDMAP));
    NNS_G2dGetUnpackedScreenData(file, &partsScreen);
    GFL_BGSysLoadScrCore(FACEUP_BG_PARTS, partsScreen->rawData, partsScreen->size, 0);
    GFL_HeapFree(file);

    GFL_BGSysSetBGPriority(0, 3);
    GFL_BGSysSetBGPriority(1, 0);
    GFL_BGSysSetBGPriority(FACEUP_BG_FACE, 2);
    GFL_BGSysSetBGPriority(FACEUP_BG_PARTS, 1);
    GFL_BGSysSetBGEnabled(FACEUP_BG_FACE, TRUE);
    GFL_BGSysSetBGEnabled(FACEUP_BG_PARTS, TRUE);
}

GameEvent *EventFaceUpEnd_Create(GameSystem *gsys) {
    if (*(FaceUpWork **)Field_GetNDemoDataHandle(GSYS_GetField(gsys)) == NULL) {
        return NULL;
    }
    return GameEvent_Create(gsys, NULL, EventFaceUpEnd_Callback, 0);
}

void FaceUp_FreeNow(Field *field) {
    FaceUpWork *work = *(FaceUpWork **)Field_GetNDemoDataHandle(field);

    FaceUp_Free(field, work);
}

void FaceUp_StartTalk(Field *field) {
    FaceUpWork *work = *(FaceUpWork **)Field_GetNDemoDataHandle(field);

    work->talkEnded = FALSE;
    work->waitingForMsg = TRUE;
}

static GameEventReturnCode EventFaceUpEnd_Callback(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    FaceUpWork *work = *(FaceUpWork **)Field_GetNDemoDataHandle(field);
    BOOL endBlack;

    switch (*state) {
    case 0:
        if (work->endBlack) {
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, -8);
        } else {
            GFL_FadeSet(FADE_ENGINE_A_BLACK, 0, 16, 0);
        }
        (*state)++;
        break;
    case 1:
        if (GFL_FadeIsRunning()) {
            break;
        }
        GFL_TCBRemove(work->tcb);
        (*state)++;
        break;
    case 2:
        if (work->eyes.updated || work->mouth.updated) {
            break;
        }
        GFL_TCBRemove(work->vblankTcb);
        endBlack = work->endBlack;
        FaceUp_Free(field, work);
        if (endBlack) {
            return GAMEEVENT_DONE;
        }
        GFL_FadeSet(FADE_ENGINE_A_BLACK, 16, 0, 0);
        (*state)++;
        break;
    case 3:
        if (GFL_FadeIsRunning()) {
            break;
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static void FaceUp_Free(Field *field, FaceUpWork *work) {
    PlaceName *placeName = Field_GetPlaceName(field);

    GFL_BGSysClearBG(FACEUP_BG_FACE);
    G2_SetBG3ControlText(work->bg3Control.screenSize, work->bg3Control.colorMode, work->bg3Control.screenBase,
                         work->bg3Control.charBase);
    func_ov036_021b5180(placeName);
    func_ov036_021879c0(Field_GetMsgBGSys(field));
    G2_BlendNone();
    FaceUp_RestoreBGPriorities(work);
    FaceUp_RestoreEnabledBGs(work);
    GFL_HeapFree(work);
    *(FaceUpWork **)Field_GetNDemoDataHandle(field) = NULL;
}

static void FaceUp_SaveBGPriorities(FaceUpWork *work) {
    int i;

    for (i = 0; i < 4; i++) {
        work->bgPriorities[i] = GFL_BGSysGetBGPriority(i);
    }
}

static void FaceUp_SaveEnabledBGs(FaceUpWork *work) {
    work->enabledBGs = GFL_BGSysGetEnabledBGsA();
}

static void FaceUp_RestoreBGPriorities(FaceUpWork *work) {
    int i;

    for (i = 0; i < 4; i++) {
        GFL_BGSysSetBGPriority((u8)i, work->bgPriorities[i]);
    }
}

static void FaceUp_RestoreEnabledBGs(FaceUpWork *work) {
    GFL_BGSysSetEnabledBGsA(work->enabledBGs);
}

static BOOL FaceUp_UpdateFadeIn(FaceUpWork *work) {
    int alpha;

    work->fadeCount++;
    alpha = work->fadeCount * 16 / FACEUP_FADE_FRAMES;
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG3, GX_PLANEMASK_BG2, alpha, 16 - alpha);
    if (work->fadeCount >= FACEUP_FADE_FRAMES) {
        return TRUE;
    }
    return FALSE;
}

static void FaceUp_Task(TCB *tcb, void *data) {
    FaceUpWork *work = data;
    void *window;
    u32 msgState;

    if (!work->talkEnded && FieldScriptSubEvent_IsRegistered(SCRIPT_SUB_EVENT_MSG) &&
        (window = getMapDisplayInfoPtr(work->env)) != NULL) {
        msgState = func_ov036_02188cbc(window);
        if (msgState == 0) {
            work->mouth.stopped = FALSE;
            work->waitingForMsg = FALSE;
        } else if (msgState == 2 && !work->waitingForMsg) {
            work->talkEnded = TRUE;
        }
    }

    FaceUpAnim_Update(sMouthFrames, NELEMS(sMouthFrames), 0x184, 8, 4, &work->mouth);
    if (work->blinkWait == 0) {
        work->eyes.stopped = FALSE;
        work->blinkWait = GFL_RandomLC(20) + 80;
    } else {
        work->blinkWait--;
    }
    FaceUpAnim_Update(sEyeFrames, NELEMS(sEyeFrames), 0x160, 12, 6, &work->eyes);
}

static void FaceUp_VBlankTask(TCB *tcb, void *data) {
    FaceUpWork *work = data;
    FaceUpAnim *eyes = &work->eyes;
    FaceUpAnim *mouth;

    if (eyes->updated) {
        GFL_BGSysLoadScrCore(FACEUP_BG_PARTS, eyes->screen[0], sizeof(eyes->screen[0]), 0x10d);
        GFL_BGSysLoadScrCore(FACEUP_BG_PARTS, eyes->screen[1], sizeof(eyes->screen[1]), 0x10d + 32);
        eyes->updated = FALSE;
    }
    mouth = &work->mouth;
    if (mouth->updated) {
        GFL_BGSysLoadScrCore(FACEUP_BG_PARTS, mouth->screen[0], 4 * sizeof(u16), 0x16e);
        GFL_BGSysLoadScrCore(FACEUP_BG_PARTS, mouth->screen[1], 4 * sizeof(u16), 0x16e + 32);
        mouth->updated = FALSE;
    }
}

// Every interval frames, moves to the next frame and writes its map entries, the frame's index times stride after
// base, until the last frame stops the animation
static void FaceUpAnim_Update(const FaceUpAnimFrame *frames, u32 count, u32 base, u32 stride, u32 width,
                              FaceUpAnim *anim) {
    int tile;
    int row;
    u32 col;

    if (!anim->updated) {
        if (anim->wait == 0) {
            if (anim->frame != 0 || !anim->stopped) {
                anim->updated = TRUE;
                tile = base + frames[anim->frame].index * stride;
                for (row = 0; row < 2; row++) {
                    for (col = 0; col < width; col++) {
                        anim->screen[row][col] = tile + row * width + col;
                    }
                }
                anim->wait = anim->interval;
                anim->frame++;
                if (anim->frame >= count) {
                    anim->frame = 0;
                    anim->stopped = TRUE;
                }
            }
        } else {
            anim->wait--;
        }
    }
}

static void FaceUpAnim_Init(FaceUpAnim *anim) {
    anim->stopped = TRUE;
    anim->updated = FALSE;
    anim->wait = 0;
    anim->interval = 0;
    anim->frame = 0;
}
