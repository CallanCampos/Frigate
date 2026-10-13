// The field gimmick of the Pokémon World Tournament's stadium (zone 193), which script plugin 7 drives (overlay 57's
// scrcmd_wbt_stadium.c). The file's name is descriptive: the ROM names only overlay 134's other file, wbt_battle.c.
// Scene 0 is the stadium's own models (doors, screens and lights), scene 2 the fighters' name plates and portraits,
// whose textures it draws from the Trainers' names and from archive 0x123
#include "field/wbt_stadium_gimmick.h"
#include "types.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/field_task.h"
#include "gfl/arc_util.h"
#include "gfl/bmp.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nnsys/g3d.h"
#include "nnsys/gfd.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/printsys.h"

typedef struct {
    HeapID heapId;
    FieldExpObjSystem *expObj;
    Field *field;
    PlayerState *playerState;
    // Whether scene 2 is loaded
    BOOL sceneLoaded;
    // Whether the intro of actor 5 is playing
    BOOL introPlaying;
    u16 unk18;
    u16 unk1A;
    GFLBitmap *bitmap;
    NNSG2dCharacterData *character;
    NNSG2dPaletteData *palette;
    void *characterFile;
    void *paletteFile;
} GimmickWork;

// The data of the tasks that draw the portraits and then show them
typedef struct {
    u16 state;
    GimmickWork *work;
} PortraitTaskData;

static void WbtStadiumGimmick_LoadMainScene(GimmickWork *work);
static void WbtStadiumGimmick_FreeScenes(GimmickWork *work);
static void WbtStadiumGimmick_UpdateAnimations(GimmickWork *work);
static void WbtStadiumGimmick_SetupActor(FieldExpObjSystem *expObj, u16 actor, s32 anmCount, VecFx32 pos);
static void WbtStadiumGimmick_StartAnm(FieldExpObjSystem *expObj, u16 actor, u16 anm, BOOL looped);
static void WbtStadiumGimmick_StartAnmsActor11(GimmickWork *work);
static void WbtStadiumGimmick_StartAnmsActor12(GimmickWork *work);
static void WbtStadiumGimmick_StartAnmsActor13(GimmickWork *work);
static void WbtStadiumGimmick_StartAnmsActor14(GimmickWork *work);
static void WbtStadiumGimmick_ShowActor4(GimmickWork *work);
static void WbtStadiumGimmick_AddNamesTask(GimmickWork *work, u32 mode);
static void WbtStadiumGimmick_DrawNamePlate(GimmickWork *work, Font *font, u32 plate, StrBuf *name, u16 heapId);
static void WbtStadiumGimmick_DrawPortrait(GimmickWork *work, u32 mode, u16 file);
static void WbtStadiumGimmick_AddPortraitsTask(GimmickWork *work);
static BOOL WbtStadiumGimmick_PortraitsTask(void *data);
static void WbtStadiumGimmick_AddPortraitTask(GimmickWork *work);
static BOOL WbtStadiumGimmick_PortraitTask(void *data);
static u32 WbtStadiumGimmick_GetTexAddr(const NNSG3dResTex *tex, const NNSG3dResDictTexData *texData);
static u32 WbtStadiumGimmick_GetPlttAddr(const NNSG3dResTex *tex, const NNSG3dResDictPlttData *plttData);
static void WbtStadiumGimmick_MakeResName(NNSG3dResName *resName, const char *name);
static u32 WbtStadiumGimmick_FindTexAddr(const NNSG3dResTex *tex, const char *name);
static u32 WbtStadiumGimmick_FindPlttAddr(const NNSG3dResTex *tex, const char *name);
static void WbtStadiumGimmick_LoadPortrait(GimmickWork *work, void *texResource, const char *texName, const char *plttName, u16 file,
                         u16 heapId);
static void WbtStadiumGimmick_LoadSinglePortraitScene(GimmickWork *work, u8 position);
static void WbtStadiumGimmick_AddEndTask(GimmickWork *work);
static BOOL WbtStadiumGimmick_EndTask(void *data);

// The animations of the stage's actors (named by actor), the scene 0 setup, and the textures and palettes of the
// portraits and the name plates (not const, so in .data). These lie in this order only to give the original's .rodata
// and .data: the size sort of MWCC's data puts equal sizes in an order that depends on every declaration
static const G3DSceneResourceSetup sResources[] = {
    { 0x107, 0, 0 },  { 0x107, 1, 0 },  { 0x107, 2, 0 },  { 0x107, 3, 0 },  { 0x107, 4, 0 },  { 0x107, 5, 0 },
    { 0x107, 6, 0 },  { 0x107, 7, 0 },  { 0x107, 14, 0 }, { 0x107, 15, 0 }, { 0x107, 8, 0 },  { 0x107, 9, 0 },
    { 0x107, 10, 0 }, { 0x107, 11, 0 }, { 0x107, 12, 0 }, { 0x107, 13, 0 }, { 0x107, 16, 0 }, { 0x107, 17, 0 },
    { 0x107, 18, 0 }, { 0x107, 19, 0 }, { 0x107, 20, 0 }, { 0x107, 21, 0 }, { 0x107, 22, 0 }, { 0x107, 23, 0 },
    { 0x107, 24, 0 }, { 0x107, 25, 0 }, { 0x107, 26, 0 }, { 0x107, 27, 0 }, { 0x107, 28, 0 }, { 0x107, 29, 0 },
    { 0x107, 30, 0 }, { 0x107, 31, 0 }, { 0x107, 32, 0 }, { 0x107, 33, 0 },
};
static const G3DSceneAnimationSetup sActor0Anms[] = { { 1, 0 }, { 2, 0 } };
static const G3DSceneAnimationSetup sActor2Anms[] = { { 5, 0 }, { 6, 0 } };
static const G3DSceneAnimationSetup sActor5Anms[] = { { 0xb, 0 }, { 0xc, 0 } };
static const G3DSceneAnimationSetup sActor14Anms[] = { { 0x1f, 0 }, { 0x20, 0 }, { 0x21, 0 } };
static const G3DSceneAnimationSetup sActor12Anms[] = { { 0x19, 0 }, { 0x1a, 0 }, { 0x1b, 0 } };
static const G3DSceneAnimationSetup sActor4Anms[] = { { 9, 0 } };
static const G3DSceneAnimationSetup sActor13Anms[] = { { 0x1d, 0 } };
static const G3DSceneAnimationSetup sActor6Anms[] = { { 0xe, 0 }, { 0xf, 0 } };
static const G3DSceneAnimationSetup sActor11Anms[] = { { 0x15, 0 }, { 0x16, 0 }, { 0x17, 0 } };
static const G3DSceneActorSetup sActors[] = {
    { 0, 0, 0, 0, sActor0Anms, 2 },    { 3, 0, 3, 0, NULL, 0 },          { 4, 0, 4, 0, sActor2Anms, 2 },
    { 7, 0, 7, 0, NULL, 0 },           { 8, 0, 8, 0, sActor4Anms, 1 },   { 10, 0, 10, 0, sActor5Anms, 2 },
    { 13, 0, 13, 0, sActor6Anms, 2 },  { 16, 0, 16, 0, NULL, 0 },        { 17, 0, 17, 0, NULL, 0 },
    { 18, 0, 18, 0, NULL, 0 },         { 19, 0, 19, 0, NULL, 0 },        { 20, 0, 20, 0, sActor11Anms, 3 },
    { 24, 0, 24, 0, sActor12Anms, 3 }, { 28, 0, 28, 0, sActor13Anms, 1 }, { 30, 0, 30, 0, sActor14Anms, 3 },
};
static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

static char *sNameTexNames[3] = { "wbt_name01", "wbt_name02", "wbt_name03" };
static char *sFaceTexNames[3] = { "wbt_v_kao1", "wbt_v_kao2", "wbt_v_kao1" };
static char *sFacePlttNames[3] = { "wbt_v_kao1_pl", "wbt_v_kao2_pl", "wbt_v_kao1_pl" };
static char *sNamePlttNames[3] = { "wbt_name01_pl", "wbt_name02_pl", "wbt_name03_pl" };

void WbtStadiumGimmick_Setup(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    GimmickWork *work = Field_AllocGimmickWorkBlock(field, 1, Field_GetHeapID(field), sizeof(GimmickWork));

    work->heapId = HEAPID_FIELDMAP;
    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    work->playerState = GameData_GetPlayerState(gameData);
    work->sceneLoaded = FALSE;
    work->introPlaying = FALSE;
    WbtStadiumGimmick_LoadMainScene(work);
}

void WbtStadiumGimmick_End(Field *field) {
    WbtStadiumGimmick_FreeScenes(Field_GetGimmickWorkBlock(field, 1));
    Field_DeleteGimmickWorkBlock(field, 1);
}

void WbtStadiumGimmick_Update(Field *field) {
    WbtStadiumGimmick_UpdateAnimations(Field_GetGimmickWorkBlock(field, 1));
}

// Shows the Trainers' names on the plates (mode 0 for both, 1 and 2 for the player's alone) and, once the portraits
// are drawn, sets the screens up
void WbtStadiumGimmick_SetNames(Field *field, u8 mode, StrBuf *name, StrBuf *playerName, u16 file, u16 playerFile) {
    GimmickWork *work = Field_GetGimmickWorkBlock(field, 1);
    HeapID heapId = HEAPID_TAIL(work->heapId);
    Font *font;

    work->unk18 = file;
    work->unk1A = playerFile;
    FieldExpObj_SetActorHidden(work->expObj, 0, 0, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 0, 1, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 0, 2, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 0, 3, TRUE);
    font = func_ov036_0218799c(Field_GetMsgBGSys(field));
    switch (mode) {
    case 0:
        WbtStadiumGimmick_DrawNamePlate(work, font, 0, name, heapId);
        WbtStadiumGimmick_DrawNamePlate(work, font, 1, playerName, heapId);
        break;
    case 1:
    case 2:
        WbtStadiumGimmick_DrawNamePlate(work, font, 2, name, heapId);
        break;
    }
}

void WbtStadiumGimmick_ShowPortraits(Field *field, u8 mode) {
    GimmickWork *work = Field_GetGimmickWorkBlock(field, 1);

    FieldExpObj_SetActorHidden(work->expObj, 0, 0, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 0, 1, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 0, 2, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 0, 3, TRUE);
    switch (mode) {
    case 0:
    case 1:
    case 2:
        WbtStadiumGimmick_AddNamesTask(work, mode);
        break;
    default:
        WbtStadiumGimmick_ShowActor4(work);
        break;
    }
}

// Loads scene 2, the portraits' one, with the resources of the fighters of the given position
void WbtStadiumGimmick_LoadPortraitScene(Field *field, u8 position, u8 side) {
    GimmickWork *work = Field_GetGimmickWorkBlock(field, 1);
    G3DSceneResourceSetup resources[4] = { 0 };
    G3DSceneAnimationSetup anms[3] = { 0 };
    G3DSceneActorSetup actor = { 0 };
    G3DSceneSetup setup = { 0 };
    u32 resourceIds[4] = { 0x22, 0x23, 0x24, 0x25 };
    u8 i = 0;
    u8 n = 0;
    FieldExpObjAnm *anm;
    SRTMatrix *matrix;

    if (position == 20) {
        WbtStadiumGimmick_LoadSinglePortraitScene(work, side);
        return;
    }
    for (i = 0; i < 4; i++) {
        resources[i].arcId = 0x107;
        resources[i].fileId = resourceIds[i] + position * 8 + side * 4;
        resources[i].type = 0;
    }
    anms[n].resource = 1;
    anms[n].index = 0;
    n++;
    anms[n].resource = 2;
    anms[n].index = 0;
    n++;
    anms[n].resource = 3;
    anms[n].index = 0;
    n++;
    actor.modelResource = 0;
    actor.modelIndex = 0;
    actor.texResource = 0;
    actor.animations = anms;
    actor.animationCount = n;
    setup.resources = resources;
    setup.resourceCount = i;
    setup.actors = &actor;
    setup.actorCount = 1;
    if (work->sceneLoaded) {
        FieldExpObj_FreeScene(work->expObj, 2);
    }
    LoadFieldExpandObjData(work->expObj, &setup, 2);
    work->sceneLoaded = TRUE;
    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 2, 0);
    matrix->translation.x = (side << 21) + (1 << 20);
    matrix->translation.y = 0;
    matrix->translation.z = 1 << 20;
    func_ov036_021b8248(work->expObj, 2, 0, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 2, 0, FALSE);
    for (i = 0; i < 3; i++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 2, 0, i);
        FieldExpObj_SetAnm(work->expObj, 2, 0, i, TRUE);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
}

void WbtStadiumGimmick_PlayIntro(Field *field) {
    GimmickWork *work = Field_GetGimmickWorkBlock(field, 1);
    u8 i;

    FieldExpObj_SetActorHidden(work->expObj, 0, 5, FALSE);
    for (i = 0; i < 2; i++) {
        FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, 5, i);

        FieldExpObj_SetAnm(work->expObj, 0, 5, i, TRUE);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
    FieldExpObj_SetActorHidden(work->expObj, 0, 6, TRUE);
    work->introPlaying = TRUE;
}

void WbtStadiumGimmick_SwitchLights(Field *field, u8 a1, u8 a2) {
    GimmickWork *work = Field_GetGimmickWorkBlock(field, 1);
    u16 hidden;
    u16 shown;

    if (a2 == 0) {
        hidden = a1 == 0 ? 7 : 9;
        shown = a1 == 0 ? 9 : 7;
    } else {
        hidden = a1 == 0 ? 8 : 10;
        shown = a1 == 0 ? 10 : 8;
    }
    FieldExpObj_SetActorHidden(work->expObj, 0, shown, FALSE);
    FieldExpObj_SetActorHidden(work->expObj, 0, hidden, TRUE);
}

void WbtStadiumGimmick_PlayCrowd(Field *field, u8 a1) {
    GimmickWork *work = Field_GetGimmickWorkBlock(field, 1);

    switch (a1) {
    case 0:
        WbtStadiumGimmick_StartAnmsActor11(work);
        break;
    case 1:
        WbtStadiumGimmick_StartAnmsActor12(work);
        break;
    case 2:
        WbtStadiumGimmick_StartAnmsActor11(work);
        WbtStadiumGimmick_StartAnmsActor12(work);
        WbtStadiumGimmick_StartAnmsActor13(work);
        break;
    case 3:
        WbtStadiumGimmick_StartAnmsActor14(work);
        break;
    }
}

static void WbtStadiumGimmick_LoadMainScene(GimmickWork *work) {
    // Nothing reads it, but 4 bytes of it are in the original's .rodata, before the animation lists
    static const u8 sUnused[4] = { 4, 28, 4, 32 };
    VecFx32 posA = { 0x170000, 0, 0xa0000 };
    VecFx32 posB = { 0x100000, 0, 0x100000 };
    VecFx32 posC = { 0x300000, 0, 0x100000 };

    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    WbtStadiumGimmick_SetupActor(work->expObj, 0, 2, posA);
    WbtStadiumGimmick_SetupActor(work->expObj, 1, 0, posA);
    WbtStadiumGimmick_SetupActor(work->expObj, 2, 2, posA);
    WbtStadiumGimmick_SetupActor(work->expObj, 3, 0, posA);
    WbtStadiumGimmick_SetupActor(work->expObj, 4, 1, posA);
    WbtStadiumGimmick_SetupActor(work->expObj, 5, 2, posB);
    WbtStadiumGimmick_SetupActor(work->expObj, 6, 2, posB);
    func_ov036_021b8248(work->expObj, 0, 5, FALSE);
    func_ov036_021b8248(work->expObj, 0, 6, FALSE);
    WbtStadiumGimmick_SetupActor(work->expObj, 7, 0, posB);
    WbtStadiumGimmick_SetupActor(work->expObj, 8, 0, posC);
    WbtStadiumGimmick_SetupActor(work->expObj, 9, 0, posB);
    WbtStadiumGimmick_SetupActor(work->expObj, 10, 0, posC);
    WbtStadiumGimmick_SetupActor(work->expObj, 11, 3, posB);
    WbtStadiumGimmick_SetupActor(work->expObj, 12, 3, posB);
    WbtStadiumGimmick_SetupActor(work->expObj, 13, 1, posB);
    WbtStadiumGimmick_SetupActor(work->expObj, 14, 3, posB);
}

static void WbtStadiumGimmick_FreeScenes(GimmickWork *work) {
    // Nothing reads it either
    static const G3DSceneAnimationSetup sUnusedAnm[1] = { { 6, 0 } };
    FieldExpObj_FreeScene(work->expObj, 0);
    if (work->sceneLoaded) {
        FieldExpObj_FreeScene(work->expObj, 2);
    }
}

// Steps the animations and, once the intro of actor 5 is over, loops the one of actor 6 in its place
static void WbtStadiumGimmick_UpdateAnimations(GimmickWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
    if (work->introPlaying == TRUE) {
        if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(work->expObj, 0, 5, 0)) == TRUE) {
            u8 i;

            FieldExpObj_SetActorHidden(work->expObj, 0, 5, TRUE);
            FieldExpObj_SetActorHidden(work->expObj, 0, 6, FALSE);
            for (i = 0; i < 2; i++) {
                FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, 6, i);

                FieldExpObj_SetAnm(work->expObj, 0, 6, i, TRUE);
                FieldExpObjAnm_SetLooped(anm, TRUE);
                FieldExpObjAnm_SetPaused(anm, FALSE);
            }
            work->introPlaying = FALSE;
        }
    }
}

// Puts the actor at pos, hidden, with its animations stopped
static void WbtStadiumGimmick_SetupActor(FieldExpObjSystem *expObj, u16 actor, s32 anmCount, VecFx32 pos) {
    s32 i;
    fx32 z = pos.z;
    fx32 y = pos.y;
    SRTMatrix *matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, actor);

    matrix->translation.x = pos.x;
    matrix->translation.y = y;
    matrix->translation.z = z;
    func_ov036_021b8248(expObj, 0, actor, TRUE);
    FieldExpObj_SetActorHidden(expObj, 0, actor, TRUE);
    for (i = 0; i < anmCount; i++) {
        FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(expObj, 0, actor, i);

        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(expObj, 0, actor, i, FALSE);
    }
}

// Shows the actor and plays its animation from the start
static void WbtStadiumGimmick_StartAnm(FieldExpObjSystem *expObj, u16 actor, u16 anm, BOOL looped) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, actor, anm);

    FieldExpObj_SetActorHidden(expObj, 0, actor, FALSE);
    FieldExpObjAnm_SetPaused(anmInfo, FALSE);
    FieldExpObjAnm_SetLooped(anmInfo, looped);
    FieldExpObj_SetAnm(expObj, 0, actor, anm, TRUE);
    FieldExpObj_SetAnmFrame(expObj, 0, actor, anm, 0);
}

static void WbtStadiumGimmick_StartAnmsActor11(GimmickWork *work) {
    WbtStadiumGimmick_StartAnm(work->expObj, 11, 0, FALSE);
    WbtStadiumGimmick_StartAnm(work->expObj, 11, 1, FALSE);
    WbtStadiumGimmick_StartAnm(work->expObj, 11, 2, FALSE);
}

static void WbtStadiumGimmick_StartAnmsActor12(GimmickWork *work) {
    WbtStadiumGimmick_StartAnm(work->expObj, 12, 0, FALSE);
    WbtStadiumGimmick_StartAnm(work->expObj, 12, 1, FALSE);
    WbtStadiumGimmick_StartAnm(work->expObj, 12, 2, FALSE);
}

static void WbtStadiumGimmick_StartAnmsActor13(GimmickWork *work) {
    WbtStadiumGimmick_StartAnm(work->expObj, 13, 0, FALSE);
}

static void WbtStadiumGimmick_StartAnmsActor14(GimmickWork *work) {
    WbtStadiumGimmick_StartAnm(work->expObj, 14, 0, FALSE);
    WbtStadiumGimmick_StartAnm(work->expObj, 14, 1, FALSE);
    WbtStadiumGimmick_StartAnm(work->expObj, 14, 2, FALSE);
}

static void WbtStadiumGimmick_ShowActor4(GimmickWork *work) {
    int i;

    FieldExpObj_SetActorHidden(work->expObj, 0, 4, FALSE);
    for (i = 0; i < 1u; i++) {
        FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, 4, i);

        FieldExpObj_SetAnm(work->expObj, 0, 4, i, TRUE);
        FieldExpObjAnm_SetLooped(anm, TRUE);
        FieldExpObjAnm_SetPaused(anm, FALSE);
    }
}

static void WbtStadiumGimmick_AddNamesTask(GimmickWork *work, u32 mode) {
    switch (mode) {
    case 1:
    case 2:
        WbtStadiumGimmick_AddPortraitTask(work);
        break;
    case 0:
        WbtStadiumGimmick_AddPortraitsTask(work);
        break;
    }
}

// Draws a Trainer's name on a plate of scene 0's actors: plates 0 and 1 on actor 1, plate 2 on actor 3
static void WbtStadiumGimmick_DrawNamePlate(GimmickWork *work, Font *font, u32 plate, StrBuf *name, u16 heapId) {
    int x = 0;
    u16 actor;
    G3DTextDrawResource resource;
    u32 width;
    void *tex;

    switch (plate) {
    case 0:
    case 1:
        x = 31;
        actor = 1;
        break;
    case 2:
        x = 38;
        actor = 3;
        break;
    }
    width = GFL_FontGetBlockWidth(name, font, 0);
    tex = GFL_G3DMdlGetTexResource(GFL_G3DActorGetMdl(FieldExpObj_GetActor(work->expObj, 0, actor)));
    G3DTextDraw_CreateResource(tex, sNameTexNames[plate], 0, sNamePlttNames[plate], name, x - (width >> 1), 0, 0x440,
                               heapId, &resource);
}

// Draws the portrait of an archive 0x123 file on a face texture
static void WbtStadiumGimmick_DrawPortrait(GimmickWork *work, u32 mode, u16 file) {
    u16 actor;
    HeapID heapId = HEAPID_TAIL(work->heapId);
    void *tex;

    switch (mode) {
    case 0:
    case 1:
        actor = 0;
        break;
    case 2:
        actor = 2;
        break;
    }
    work->bitmap = GFL_BitmapCreate(4, 4, 32, heapId);
    tex = GFL_G3DMdlGetTexResource(GFL_G3DActorGetMdl(FieldExpObj_GetActor(work->expObj, 0, actor)));
    WbtStadiumGimmick_LoadPortrait(work, tex, sFaceTexNames[mode], sFacePlttNames[mode], file, heapId);
    GFL_BitmapFree(work->bitmap);
    GFL_HeapFree(work->characterFile);
    GFL_HeapFree(work->paletteFile);
}

static void WbtStadiumGimmick_AddPortraitsTask(GimmickWork *work) {
    HeapID heapId = HEAPID_TAIL(work->heapId);
    FieldTaskManager *manager = Field_GetTaskManager(work->field);
    FieldTask *task = FieldTask_Create(heapId, sizeof(PortraitTaskData), WbtStadiumGimmick_PortraitsTask);
    PortraitTaskData *data = FieldTask_GetData(task);

    data->state = 0;
    data->work = work;
    FieldTaskManager_AddTask(manager, task, 0);
}

static BOOL WbtStadiumGimmick_PortraitsTask(void *data) {
    PortraitTaskData *task = data;
    GimmickWork *work = task->work;
    FieldExpObjSystem *expObj = work->expObj;

    switch (task->state) {
    case 0:
        WbtStadiumGimmick_DrawPortrait(work, 0, work->unk18);
        task->state = 1;
        break;
    case 1:
        WbtStadiumGimmick_DrawPortrait(work, 1, work->unk1A);
        task->state = 2;
        break;
    case 2:
        WbtStadiumGimmick_StartAnm(expObj, 0, 0, TRUE);
        WbtStadiumGimmick_StartAnm(expObj, 0, 1, FALSE);
        task->state = 3;
        break;
    case 3:
        if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(expObj, 0, 0, 1)) == TRUE) {
            FieldExpObj_SetActorHidden(expObj, 0, 4, TRUE);
            FieldExpObj_SetActorHidden(expObj, 0, 1, FALSE);
            task->state = 4;
        }
        break;
    case 4:
        return TRUE;
    }
    return FALSE;
}

static void WbtStadiumGimmick_AddPortraitTask(GimmickWork *work) {
    HeapID heapId = HEAPID_TAIL(work->heapId);
    FieldTaskManager *manager = Field_GetTaskManager(work->field);
    FieldTask *task = FieldTask_Create(heapId, sizeof(PortraitTaskData), WbtStadiumGimmick_PortraitTask);
    PortraitTaskData *data = FieldTask_GetData(task);

    data->state = 0;
    data->work = work;
    FieldTaskManager_AddTask(manager, task, 0);
}

static BOOL WbtStadiumGimmick_PortraitTask(void *data) {
    PortraitTaskData *task = data;
    GimmickWork *work = task->work;
    FieldExpObjSystem *expObj = work->expObj;

    switch (task->state) {
    case 0:
        WbtStadiumGimmick_DrawPortrait(work, 2, work->unk18);
        task->state = 1;
        break;
    case 1:
        WbtStadiumGimmick_StartAnm(expObj, 2, 0, TRUE);
        WbtStadiumGimmick_StartAnm(expObj, 2, 1, FALSE);
        FieldExpObj_SetActorHidden(expObj, 0, 4, TRUE);
        task->state = 2;
        break;
    case 2:
        if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(expObj, 0, 2, 1)) == TRUE) {
            FieldExpObj_SetActorHidden(expObj, 0, 3, FALSE);
            task->state = 3;
        }
        break;
    case 3:
        return TRUE;
    }
    return FALSE;
}

static u32 WbtStadiumGimmick_GetTexAddr(const NNSG3dResTex *tex, const NNSG3dResDictTexData *texData) {
    return ((texData->texImageParam & 0xffff) << 3) + NNS_GfdGetTexKeyAddr(tex->texInfo.vramKey);
}

static u32 WbtStadiumGimmick_GetPlttAddr(const NNSG3dResTex *tex, const NNSG3dResDictPlttData *plttData) {
    return (plttData->offset << 3) + NNS_GfdGetPlttKeyAddr(tex->plttInfo.vramKey);
}

static void WbtStadiumGimmick_MakeResName(NNSG3dResName *resName, const char *name) {
    int len = NNS_STD_StrLen(name);
    u8 i;

    for (i = 0; i < 4; i++) {
        resName->val[i] = 0;
    }
    for (i = 0; i < len; i++) {
        resName->name[i] = name[i];
    }
}

static u32 WbtStadiumGimmick_FindTexAddr(const NNSG3dResTex *tex, const char *name) {
    NNSG3dResName resName;
    const NNSG3dResDictTexData *texData;

    WbtStadiumGimmick_MakeResName(&resName, name);
    if (tex != NULL) {
        texData = NNS_G3DFind(&tex->dict, &resName);
    } else {
        texData = NULL;
    }
    if (texData == NULL) {
        return 0;
    }
    return WbtStadiumGimmick_GetTexAddr(tex, texData);
}

static u32 WbtStadiumGimmick_FindPlttAddr(const NNSG3dResTex *tex, const char *name) {
    NNSG3dResName resName;
    const NNSG3dResDictPlttData *plttData;

    WbtStadiumGimmick_MakeResName(&resName, name);
    if (tex != NULL && tex->plttInfo.ofsDict != 0) {
        plttData = NNS_G3DFind((const NNSG3dResDict *)((u8 *)tex + tex->plttInfo.ofsDict), &resName);
    } else {
        plttData = NULL;
    }
    if (plttData == NULL) {
        return 0;
    }
    return WbtStadiumGimmick_GetPlttAddr(tex, plttData);
}

// Reads the portrait's character and palette, 4 by 4 characters taken from the 28th character of the first four
// rows, and sends them to the texture and palette in VRAM
static void WbtStadiumGimmick_LoadPortrait(GimmickWork *work, void *texResource, const char *texName, const char *plttName, u16 file,
                         u16 heapId) {
    NNSG3dResTex *tex;
    u32 texAddr;
    u32 plttAddr;
    u8 *src;
    u8 *pixels;
    int x;
    int y;

    GFL_G3DResGetTexVRAMHandle(texResource);
    GFL_G3DResGetPltVRAMHandle(texResource);
    tex = NNS_G3DResGetTexBlock(GFL_G3DResGetResData(texResource));
    texAddr = WbtStadiumGimmick_FindTexAddr(tex, texName);
    plttAddr = WbtStadiumGimmick_FindPlttAddr(tex, plttName);
    work->character = NULL;
    work->palette = NULL;
    work->characterFile = GFL_G2DIOReadBGNCGR(0x123, file * 2, TRUE, &work->character, heapId);
    work->paletteFile = GFL_G2DIOReadNCLR(0x123, file * 2 + 1, &work->palette, heapId);
    src = work->character->rawData;
    pixels = GFL_BitmapGetPixelData(work->bitmap);
    sys_memset32(0, pixels, 16 * 32);
    for (y = 0; y < 4; y++) {
        for (x = 0; x < 4; x++) {
            sys_memcpy32(src + (y * 32 + x + 28) * 32, pixels + (y * 4 + x) * 32, 32);
        }
    }
    GFL_BitmapMakeLinear(work->bitmap, FALSE, heapId);
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_3D_TEX_VRAM, texAddr, GFL_BitmapGetPixelData(work->bitmap), 0x200);
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_3D_TEX_PLTT, plttAddr, work->palette->rawData, 0x20);
}

// Loads scene 2 with the two actors of a lone portrait (the Trainer's own and a cutscene's)
static void WbtStadiumGimmick_LoadSinglePortraitScene(GimmickWork *work, u8 side) {
    G3DSceneResourceSetup resources[6] = { 0 };
    G3DSceneAnimationSetup anms1[1] = { 0 };
    G3DSceneAnimationSetup anms2[3] = { 0 };
    G3DSceneActorSetup actors[2] = { 0 };
    G3DSceneSetup setup = { 0 };
    u32 resourceIds[6] = { 0xc2, 0xc3, 0xc6, 0xc7, 0xc8, 0xc9 };
    u8 i;
    u8 n = 0;
    FieldExpObjAnm *anm;
    SRTMatrix *matrix;

    for (i = 0; i < 2; i++) {
        resources[i].arcId = 0x107;
        resources[i].fileId = resourceIds[i] + side * 2;
        resources[i].type = 0;
    }
    for (; i < 6; i++) {
        resources[i].arcId = 0x107;
        resources[i].fileId = resourceIds[i] + side * 4;
        resources[i].type = 0;
    }
    anms1[0].resource = 1;
    anms1[0].index = 0;
    anms2[n].resource = 3;
    anms2[n].index = 0;
    n++;
    anms2[n].resource = 4;
    anms2[n].index = 0;
    n++;
    anms2[n].resource = 5;
    anms2[n].index = 0;
    n++;
    actors[0].modelResource = 0;
    actors[0].modelIndex = 0;
    actors[0].texResource = 0;
    actors[0].animations = anms1;
    actors[0].animationCount = 1;
    actors[1].modelResource = 2;
    actors[1].modelIndex = 0;
    actors[1].texResource = 2;
    actors[1].animations = anms2;
    actors[1].animationCount = n;
    setup.resources = resources;
    setup.resourceCount = i;
    setup.actors = actors;
    setup.actorCount = 2;
    if (work->sceneLoaded) {
        FieldExpObj_FreeScene(work->expObj, 2);
    }
    LoadFieldExpandObjData(work->expObj, &setup, 2);
    work->sceneLoaded = TRUE;
    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 2, 0);
    matrix->translation.x = (side << 21) + (1 << 20);
    matrix->translation.y = 0;
    matrix->translation.z = 1 << 20;
    func_ov036_021b8248(work->expObj, 2, 0, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 2, 0, FALSE);
    anm = FieldExpObj_GetAnmInfo(work->expObj, 2, 0, 0);
    FieldExpObj_SetAnm(work->expObj, 2, 0, 0, TRUE);
    FieldExpObjAnm_SetLooped(anm, FALSE);
    FieldExpObjAnm_SetPaused(anm, FALSE);
    FieldExpObj_SetAnmFrame(work->expObj, 2, 0, 0, 0);
    matrix = FieldExpObj_GetActorMatrixPtr(work->expObj, 2, 1);
    matrix->translation.x = (side << 21) + (1 << 20);
    matrix->translation.y = 0;
    matrix->translation.z = 1 << 20;
    func_ov036_021b8248(work->expObj, 2, 1, TRUE);
    FieldExpObj_SetActorHidden(work->expObj, 2, 1, TRUE);
    for (i = 0; i < 3; i++) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 2, 1, i);
        FieldExpObj_SetAnm(work->expObj, 2, 1, i, FALSE);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
    }
    WbtStadiumGimmick_AddEndTask(work);
}

static void WbtStadiumGimmick_AddEndTask(GimmickWork *work) {
    HeapID heapId = HEAPID_TAIL(work->heapId);
    FieldTaskManager *manager = Field_GetTaskManager(work->field);
    FieldTask *task = FieldTask_Create(heapId, sizeof(PortraitTaskData), WbtStadiumGimmick_EndTask);
    PortraitTaskData *data = FieldTask_GetData(task);

    data->state = 0;
    data->work = work;
    FieldTaskManager_AddTask(manager, task, 0);
}

static BOOL WbtStadiumGimmick_EndTask(void *data) {
    PortraitTaskData *task = data;
    FieldExpObjSystem *expObj = task->work->expObj;

    switch (task->state) {
    case 0:
        if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(expObj, 2, 0, 0)) == TRUE) {
            u8 i;

            FieldExpObj_SetActorHidden(expObj, 2, 1, FALSE);
            for (i = 0; i < 3; i++) {
                FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(expObj, 2, 1, i);

                FieldExpObj_SetAnm(expObj, 2, 1, i, TRUE);
                FieldExpObjAnm_SetLooped(anm, FALSE);
                FieldExpObjAnm_SetPaused(anm, FALSE);
            }
            task->state = 1;
        }
        break;
    case 1:
        return TRUE;
    }
    return FALSE;
}
