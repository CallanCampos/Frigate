// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/text_banks.h"
#include "field/battle_cutin.h"
#include "field/encounter_effect.h"
#include "field/event_encounter_cutin.h"
#include "field/field.h"
#include "field/field_3dci.h"
#include "field/field_actor.h"
#include "field/field_environment.h"
#include "field/field_event.h"
#include "field/field_fog.h"
#include "field/field_msgbg.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/fade.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/heapsys.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/particle.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/math.h"
#include "nnsys/gfd.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "save/player_savedata.h"
#include "save/save_control.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/gf_font.h"
#include "system/str_tool.h"
#include "system/wordset.h"
#include "text/system/0368.h"

// The field effects of the 3D scenes of archive 0x76 (fld3d_ci.c, after the file name that its allocations pass): the
// Pokemon's sprite of a field move, the battle cut-ins and Alder's flight, with their particles, sounds and screen
// fades. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0); the layouts are ours

typedef struct {
    u32 flags;
    fx32 current;
    fx32 target;
    u32 frames;
    u32 elapsed;
    fx32 step;
    TCB *tcb;
} Fld3DCiFade;

typedef BOOL (*Fld3DCiControlFunc)(GameEvent *event, Fld3DCi *ci);

// The scene of an effect, as Fld3DCi_LoadG3DSceneTemplate builds it from the effect's file, and what the effect
// does with it
typedef struct {
    G3DSceneResourceSetup *resources;
    G3DSceneActorSetup *actors;
    G3DSceneAnimationSetup *animations1;
    G3DSceneAnimationSetup *animations2;
    G3DSceneSetup setup;
    u16 particleDelay[2];
    u16 actorDelay[2];
    u32 particle;
    u8 resourceCount;
    u8 actorCount;
    u8 animation1Count;
    u8 animation2Count;
    u16 dimCapture;
    u16 fadeSpeed;
    u32 disableMovement;
    u32 param;
    u32 fogEnabled;
} Fld3DCiScene;

// An effect's file in archive 0x76
typedef struct {
    u32 particle;
    u16 particleDelay[2];
    u16 models[2];
    u16 actorDelay[2];
    u16 animations1[4];
    u16 animations2[4];
    u16 dimCapture;
    u16 fadeSpeed;
} Fld3DCiFile;

// The data of the event that runs a field effect
typedef struct {
    Fld3DCi *ci;
    Fld3DCiScene scene;
} Fld3DCiEvent;

typedef struct {
    u32 species;
    u32 form;
    u32 sex;
    u32 rare;
    u32 egg;
    u32 unk14;
    u16 unk18;
    u16 unk1a;
    u32 voice;
} PokeSpriteParam;

typedef struct {
    u32 animation;
    u16 color;
    u16 color2;
    u8 flag;
} CutinParam;

typedef struct {
    void *entries;
    u8 file;
    u8 count;
    u8 index;
} BitmapConvParam;

// An entry of an effect's bitmap file in archive 0x10c: the names of a texture and a palette of the effect's
// model, and the graphics to put in them
typedef struct {
    char texName[0x14];
    char palName[0x14];
    u8 resource;
    u8 charFile;
    u8 palFile;
    u8 palOffset;
    u8 width;
    u8 height;
    u8 srcStride;
    u8 srcOffset;
} Fld3DCiBitmapEntry;

typedef union {
    struct {
        u8 unk0[0x80];
        BitmapConvParam conv;
    } bitmapConv;
    PokeSpriteParam pokeSprite;
    CutinParam cutin;
    u32 words[34];
} Fld3DCiParam;

// The state of a field effect: its 3D scene, its cameras and the effect that runs
struct Fld3DCi {
    FieldParticle *particleSystem;
    G3DCamera *camera;
    G3DCamera *camera2;
    u32 cameraSel;
    u16 heapId;
    G3DManager *mgr;
    u32 scene;
    SRTMatrix srt;
    ParticleSystem *particle;
    u16 particleDelay[2];
    u16 actorDelay[2];
    u16 particleDone;
    u16 effectId;
    u32 enabledBGs;
    u8 bgPriorities[4];
    u32 screenCapDone;
    u32 particleStarted[2];
    u32 actor1Enabled;
    u32 actor2Enabled;
    NNSG2dCharacterData *character;
    NNSG2dPaletteData *palette;
    GFLBitmap *bitmap;
    void *charData;
    void *paletteData;
    Fld3DCiParam param;
    u32 particleFinished;
    u32 actor1Finished;
    u32 actor2Finished;
    GameEventCallback callback;
    Fld3DCiControlFunc control;
    u32 unk134;
    u16 frame;
    u16 soundIndex;
    u16 fadeIndex;
    u8 unk13e[2];
    u32 dimDone;
    Fld3DCiFade *fade;
};

// The default camera, as the field's (FieldG3D_CreateCamera): a 40-degree perspective at 4:3, from 1 to 1024 units.
// The up vector is the inline's own local, so it takes the lowest stack slot of Fld3DCi_Create's frame, and its
// initializer is listed in MWCC's size sort of the file's data where the inline is defined, before the tables
static inline G3DCamera *Fld3DCi_CreateDefaultCamera(const VecFx32 *position, const VecFx32 *target, HeapID heapId) {
    VecFx32 up = { 0, FX32_ONE, 0 };

    return GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)),
                               FX32_CONST(4.0 / 3.0), 0, FX32_ONE, FX32_CONST(1024), 0, position, &up, target, heapId);
}

#define FIELD_EFFECT_COUNT 117

static const Fld3DCiSoundKey data_ov036_021d1cd0[] = {
    { SEQ_SE_TDEMO_003, 25 },        { SEQ_SE_SW_CI_VSCHAMP_01, 54 },
    { SEQ_SE_SW_CI_VSCHAMP_02, 55 }, { SEQ_SE_SW_CI_VSCHAMP_02, 65 },
    { SEQ_SE_SW_CI_VSCHAMP_03, 89 }, { SEQ_SE_SW_CI_VSCHAMP_04, 90 },
    { SEQ_SE_W009_01, 150 },         { -1 },
};

static const Fld3DCiSoundKey data_ov036_021d1d10[] = {
    { SEQ_SE_SW_KUSABI_IN_02, 2 },   { SEQ_SE_SW_KUSABI_IN_03, 5 },
    { SEQ_SE_SW_KUSABI_IN_04, 169 }, { SEQ_SE_SW_KUSABI_IN_05, 257 },
    { SEQ_SE_SW_KUSABI_IN_06, 260 }, { SEQ_SE_SW_KUSABI_IN_07, 446 },
    { SEQ_SE_SW_KUSABI_IN_08, 447 }, { SEQ_SE_SW_KUSABI_IN_09, 521 },
    { SEQ_SE_SW_KUSABI_IN_10, 540 }, { SEQ_SE_SW_KUSABI_IN_01, 553 },
    { SEQ_SE_SW_KUSABI_IN_11, 560 }, { SEQ_SE_SW_KUSABI_IN_12, 575 },
    { SEQ_SE_SW_KUSABI_IN_13, 576 }, { -1 },
};

static const Fld3DCiSoundKey data_ov036_021d1d80[] = {
    { SEQ_SE_SW_KUSABI_IN_02, 2 },   { SEQ_SE_SW_KUSABI_IN_03, 5 },
    { SEQ_SE_SW_KUSABI_IN_04, 169 }, { SEQ_SE_SW_KUSABI_IN_05, 257 },
    { SEQ_SE_SW_KUSABI_IN_06, 260 }, { SEQ_SE_SW_KUSABI_IN_07, 446 },
    { SEQ_SE_SW_KUSABI_IN_08, 447 }, { SEQ_SE_SW_KUSABI_IN_09, 521 },
    { SEQ_SE_SW_KUSABI_IN_10, 540 }, { SEQ_SE_SW_KUSABI_IN_01, 553 },
    { SEQ_SE_SW_KUSABI_IN_11, 560 }, { SEQ_SE_SW_KUSABI_IN_12, 575 },
    { SEQ_SE_SW_KUSABI_IN_13, 576 }, { -1 },
};

// The sound effects of each effect, by its fade speed. Declared before the fades, which puts the two tables in the
// ROM's order in MWCC's size sort of the file's data
static const Fld3DCiSoundKey *const data_ov036_021d1e80[] = {
    data_ov036_021d17ac, data_ov036_021d17b4, data_ov036_021d17bc, data_ov036_021d1c0c, data_ov036_021d1980,
    data_ov036_021d1990, data_ov036_021d1cd0, data_ov036_021d1a98, data_ov036_021d1b54, data_ov036_021d1b7c,
    data_ov036_021d1a40, data_ov036_021d19a0, data_ov036_021d19b0, data_ov036_021d19c0, data_ov036_021d19d0,
    data_ov036_021d1794, data_ov036_021d179c, data_ov036_021d1a58, data_ov036_021d1ba4, data_ov036_021d19f8,
    data_ov036_021d17c4, data_ov036_021d17a4, data_ov036_021d1bd4, data_ov036_021d17cc, data_ov036_021d178c,
    data_ov036_021d1784, data_ov036_021d1960, data_ov036_021d1a10, data_ov036_021d1970, data_ov036_021d1adc,
    data_ov036_021d1a78, data_ov036_021d1b04, data_ov036_021d1b2c, data_ov036_021d1d10, data_ov036_021d1d80,
    data_ov036_021d1a28,
};

// The screen fades of each effect, by its fade speed
static const Fld3DCiFadeKey *const FLD3DCI_FADE_ANIMATIONS[] = {
    data_ov036_021d1ab8, data_ov036_021d1918, data_ov036_021d190c, data_ov036_021d181c, data_ov036_021d18f4,
    data_ov036_021d18c4, data_ov036_021d1c44, data_ov036_021d18d0, data_ov036_021d1834, data_ov036_021d17d4,
    data_ov036_021d1954, data_ov036_021d1930, data_ov036_021d1894, data_ov036_021d17ec, data_ov036_021d18e8,
    data_ov036_021d17e0, data_ov036_021d1864, data_ov036_021d18a0, data_ov036_021d17f8, data_ov036_021d1840,
    data_ov036_021d1948, data_ov036_021d1828, data_ov036_021d1924, data_ov036_021d1870, data_ov036_021d18ac,
    data_ov036_021d1888, data_ov036_021d184c, data_ov036_021d1900, data_ov036_021d19e0, data_ov036_021d187c,
    data_ov036_021d1858, data_ov036_021d1804, data_ov036_021d193c, data_ov036_021d1810, data_ov036_021d18b8,
    data_ov036_021d18dc,
};

// The free heap each effect needs; the effects after the 71st need none
static const u32 FIELD_EFFECT_HEAP_SIZES[FIELD_EFFECT_COUNT] = {
    0x57e4,  0x3438,  0x20290, 0x51a0, 0x21cc8, 0x2409c, 0x24f50, 0x27fc0, 0x28fd0, 0x29fac, 0x27630, 0xa478,
    0x884c,  0xe570,  0xe570,  0xe570, 0xe570,  0xe570,  0xe570,  0xe570,  0xe570,  0xe570,  0x12a68, 0x12a68,
    0x12a68, 0x12a68, 0x12a68, 0xcf20, 0xd530,  0x8710,  0xa478,  0x884c,  0xb28,   0x1e04,  0x14bc8, 0x619c,
    0xac88,  0x80e0,  0x57e4,  0x57e4, 0x57e4,  0x3438,  0x3438,  0x3438,  0x3438,  0x3438,  0x3438,  0x3438,
    0x3438,  0x3438,  0x3438,  0x3438, 0x3438,  0x3438,  0x20290, 0x21cc8, 0x2409c, 0x24f50, 0x27fc0, 0x28fd0,
    0x29fac, 0x27630, 0x3438,  0x3438, 0x3438,  0x3438,  0xd530,  0xcf20,  0xcf20,  0xe570,  0xd530,
};

static void Fld3DCi_LoadG3DScene(Fld3DCi *ci, const G3DSceneSetup *setup);
static GameEvent *EventFieldEffect_CreatePokeSpriteCore(GameSystem *gsys, Fld3DCi *ci, u32 species, u32 form, u32 sex,
                                                        u32 rare, u32 egg);
static GameEventReturnCode EventFieldEffect_Callback(GameEvent *event, u32 *state, void *work);
static BOOL Fld3DCi_UpdateParticles(Fld3DCi *ci);
static BOOL Fld3DCi_UpdateActor1(Fld3DCi *ci);
static BOOL Fld3DCi_UpdateActor2(Fld3DCi *ci);
static void EventFieldEffect_LoadGraphics(GameEvent *event, Fld3DCi *ci, Fld3DCiScene *scene, u8 effectId);
static void Fld3DCi_FreeG3D(Fld3DCi *ci, Fld3DCiScene *scene);
static void Fld3DCi_CreateEmitter(Fld3DCi *ci, int id);
static void Fld3DCi_FollowTargetCallback(SPLEmitter *emitter);
static void Fld3DCi_LoadG3DSceneTemplate(Fld3DCiScene *scene, u32 fileId, HeapID heapId);
static void Fld3DCi_FreeSceneTemplate(Fld3DCiScene *scene);
static void Fld3DCi_RequestScreenCap(Fld3DCi *ci);
static void Fld3DCi_ScreenCapTCB(TCB *tcb, void *work);
static void Fld3DCi_BackupBGPriorities(Fld3DCi *ci);
static void Fld3DCi_BackupScreenDispA(Fld3DCi *ci);
static void Fld3DCi_RestoreBGPriorities(Fld3DCi *ci);
static void Fld3DCi_RestoreScreenDispA(Fld3DCi *ci);
static GameEventReturnCode EventFieldEffectPokeSpriteSub_Callback(GameEvent *event, u32 *state, void *work);
static BOOL EventFieldEffectPokeSpriteSub_ControlEffect(GameEvent *event, Fld3DCi *ci);
static GameEventReturnCode EventFieldEffectFadeOutWhite_Callback(GameEvent *event, u32 *state, void *work);
static GameEventReturnCode EventFieldEffectFadeInWhiteTop_Callback(GameEvent *event, u32 *state, void *work);
static GameEventReturnCode EventFieldEffectFadeInAllBlack_Callback(GameEvent *event, u32 *state, void *work);
static BOOL func_ov036_021bc828(u16 effectId);
static BOOL func_ov036_021bc838(u16 effectId);
static u32 Fld3DCi_GetTexDataOffset(const NNSG3dResTex *tex, const NNSG3dResDictTexData *data);
static u32 Fld3DCi_GetPlttDataOffset(const NNSG3dResTex *tex, const NNSG3dResDictPlttData *data);
static u32 NNS_G3DResTexGetTexRAMAddress(const NNSG3dResTex *tex, const char *name);
static u32 NNS_G3DResTexGetPltRAMAddress(const NNSG3dResTex *tex, const char *name);
static void NNS_G3DNormalizeName(NNSG3dResName *dest, const char *name);
static void EventFieldEffect_DimCaptureColor(void);
static void Fld3DCi_InitEventData(Fld3DCi *ci, Fld3DCiEvent *data);
static void Fld3DCi_UpdateSound(Fld3DCi *ci, u8 speed);
static void Fld3DCi_UpdateFade(Fld3DCi *ci, u8 speed);
static void Fld3DCi_DisplayRestoreTask(TCB *tcb, void *work);
static Fld3DCiFade *Fld3DCiFade_Create(u32 heapId);
static void Fld3DCi_AllFadeTCB(TCB *tcb, void *work);
static void Fld3DCiFadeState_Apply(Fld3DCiFade *fade);
static void Fld3DCiFadeState_Set(Fld3DCiFade *fade, u32 flags, int from, int to, int frames);
static void Fld3DCiFade_Free(Fld3DCiFade *fade);
static BOOL Fld3DCiFade_IsRunning(Fld3DCiFade *fade);
static void Fld3DCi_ConvG2DToG3D(Fld3DCi *ci, const char *texName, const char *palName, u32 charFile, u32 palFile,
                                 u8 width, u8 height, u8 srcStride, u8 srcOffset, u8 resource, u8 palOffset);
static BOOL EventFieldEffectCutinSub_ControlEffect(GameEvent *event, Fld3DCi *ci);
static GameEventReturnCode EventFieldEffectCutinSub_Callback(GameEvent *event, u32 *state, void *work);
static GameEventReturnCode EventFld3DCiBitmapConv_Callback(GameEvent *event, u32 *state, void *work);
static void EventFieldEffectCutinSub_DrawStrToTexture(PlayerInfo *info, RivalDataSave *rival, Fld3DCi *ci,
                                                      const char *texName, const char *palName, u32 messageId,
                                                      u16 color, u16 color2, BOOL measure);
static void EventFieldEffectPokeSpriteSub_ConvertSprite(Fld3DCi *ci);

Fld3DCi *Fld3DCi_Create(HeapID heapId, FieldParticle *particleSystem) {
    Fld3DCi *ci = GFL_HeapAllocate(heapId, sizeof(Fld3DCi), TRUE, "fld3d_ci.c", 481);

    ci->mgr = GFL_G3DMgrCreate(8, 2, heapId);
    ci->heapId = heapId;
    ci->particleSystem = particleSystem;
    ci->scene = -1;
    {
        VecFx32 position = { 0, 0, FX32_CONST(128) };
        VecFx32 target = { 0, 0, 0 };

        ci->camera = Fld3DCi_CreateDefaultCamera(&position, &target, heapId);
    }
    {
        VecFx32 position = { 0, 0, FX32_CONST(128) };
        VecFx32 target = { 0, 0, 0 };
        VecFx32 up = { 0, FX32_ONE, 0 };

        ci->camera2 = GFL_G3DCameraCreate(G3DCAM_PROJECTION_ORTHO, 0x30000, -0x30000, -0x40000, 0x40000, FX32_ONE,
                                          FX32_CONST(1024), 0, &position, &up, &target, heapId);
    }
    ci->cameraSel = 0;
    return ci;
}

void Fld3DCi_Free(Fld3DCi *ci) {
    GFL_G3DCameraFree(ci->camera);
    GFL_G3DCameraFree(ci->camera2);
    GFL_G3DMgrFree(ci->mgr);
    GFL_HeapFree(ci);
}

static void Fld3DCi_LoadG3DScene(Fld3DCi *ci, const G3DSceneSetup *setup) {
    ci->scene = GFL_G3DMgrNewScene(ci->mgr, setup);
    ci->srt.scale.x = FX32_ONE;
    ci->srt.scale.y = FX32_ONE;
    ci->srt.scale.z = FX32_ONE;
    MTX_Identity33_(&ci->srt.rotation);
    ci->srt.translation.x = 0;
    ci->srt.translation.y = 0;
    ci->srt.translation.z = 0;
}

void Fld3DCi_Draw(Fld3DCi *ci) {
    int first;
    int count;
    u8 i;

    GFL_G3DCameraFlush(ci->cameraSel == 0 ? ci->camera : ci->camera2);
    if (ci->scene != -1) {
        first = GFL_G3DMgrGetSceneFirstActorIdx(ci->mgr, ci->scene);
        count = GFL_G3DSceneGetNodeActorCount(ci->mgr, ci->scene);
        for (i = 0; i < count; i++) {
            GFL_G3DSysDrawObj(GFL_G3DMgrGetActor(ci->mgr, first + i), &ci->srt);
        }
    }
}

GameEvent *EventFieldEffect_Create(GameSystem *gsys, Fld3DCi *ci, u8 effectId) {
    GameEvent *event;

    if (GFL_HeapGetHighestAllocatableSize(HEAPID_FIELDMAP) < FIELD_EFFECT_HEAP_SIZES[effectId]) {
        return NULL;
    }
    ci->particleDone = 0;
    ci->unk134 = 0;
    ci->frame = 0;
    ci->soundIndex = 0;
    ci->fadeIndex = 0;
    ci->effectId = effectId;
    ci->callback = 0;
    ci->control = 0;
    ci->cameraSel = 0;
    event = GameEvent_Create(gsys, NULL, EventFieldEffect_Callback, 0x44);
    Fld3DCi_InitEventData(ci, GameEvent_GetData(event));
    return event;
}

static GameEvent *EventFieldEffect_CreatePokeSpriteCore(GameSystem *gsys, Fld3DCi *ci, u32 species, u32 form, u32 sex,
                                                        u32 rare, u32 egg) {
    GameEvent *event = EventFieldEffect_Create(gsys, ci, 3);
    PokeSpriteParam *param;

    if (event == NULL) {
        return NULL;
    }
    ci->callback = EventFieldEffectPokeSpriteSub_Callback;
    ci->control = EventFieldEffectPokeSpriteSub_ControlEffect;
    param = &ci->param.pokeSprite;
    param->species = species;
    param->form = form;
    param->sex = sex;
    param->rare = rare;
    param->egg = egg;
    param->unk18 = 5;
    param->unk14 = 1;
    param->unk1a = 30;
    return event;
}

GameEvent *EventFieldEffect_CreatePokeSprite(GameSystem *gsys, Fld3DCi *ci, u8 partySlot) {
    PartyPkm *pkm = PokeParty_GetPkm(GameData_GetParty(GSYS_GetGameData(gsys)), partySlot);
    u32 species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    u32 form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    u32 sex = PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL);
    BOOL rare = PokeParty_IsRare(pkm);
    u32 egg = PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL);

    return EventFieldEffect_CreatePokeSpriteCore(gsys, ci, species, form, sex, rare, egg);
}

GameEvent *EventFieldEffect_CreateBattleCutin(GameSystem *gsys, Fld3DCi *ci, u32 selection, u32 param) {
    GameEvent *event;
    Fld3DCiEvent *data;
    const BattleCutinParam *cutin;
    CutinParam *cutinParam;
    u8 header[4];

    GFL_OvlLoad(OVERLAY_ID(147));
    cutin = BattleCutinDB_GetParam(selection);
    event = EventFieldEffect_Create(gsys, ci, cutin->cutin);
    if (event == NULL) {
        GFL_OvlUnload(OVERLAY_ID(147));
        return NULL;
    }
    data = GameEvent_GetData(event);
    Fld3DCi_InitEventData(ci, data);
    data->scene.param = param;
    GFL_ArcSysReadRange(header, 0x10c, cutin->cutin, 0, 4);
    if (header[1] != 0 || header[0] != 0) {
        ci->callback = EventFieldEffectCutinSub_Callback;
    } else {
        ci->callback = NULL;
    }
    ci->control = EventFieldEffectCutinSub_ControlEffect;
    ci->cameraSel = 2;
    cutinParam = &ci->param.cutin;
    cutinParam->animation = cutin->animation;
    cutinParam->color = cutin->color;
    cutinParam->color2 = cutin->color2;
    cutinParam->flag = header[1];
    GFL_OvlUnload(OVERLAY_ID(147));
    return event;
}

static GameEventReturnCode EventFieldEffect_Callback(GameEvent *event, u32 *state, void *work) {
    Fld3DCiEvent *data = work;
    Fld3DCi *ci = data->ci;
    GameSystem *gsys = GameEvent_GetGameSystem(event);
    Field *field = GSYS_GetField(gsys);

    switch (*state) {
    case 0:
        if (data->scene.disableMovement) {
            DisableAllActorsMovement(Field_GetActorSystem(field));
        }
        Fld3DCi_BackupBGPriorities(ci);
        Fld3DCi_BackupScreenDispA(ci);
        if (!func_ov036_021bc828(ci->effectId) && !func_ov036_021bc838(ci->effectId)) {
            GameEvent_ChainNext(event, GameEvent_Create(gsys, event, EventFieldEffectFadeOutWhite_Callback, 0));
        }
        (*state)++;
        break;
    case 1:
        (*state)++;
        if (ci->effectId == 0xb || ci->effectId == 0x1f) {
            SetActorHidden(FieldPlayer_GetActor(Field_GetPlayer(field)), TRUE);
            break;
        }
        if (ci->effectId == 0x1e) {
            FieldActor *actor = FindFieldActor(Field_GetActorSystem(field), ci->param.words[0]);

            if (actor != NULL) {
                DeleteActor(actor);
            }
            break;
        }
    case 2:
        Field_SetEffectRunningFlag(field, TRUE);
        Fld3DCi_RequestScreenCap(ci);
        (*state)++;
        break;
    case 3:
        if (ci->screenCapDone == 0) {
            break;
        }
        GFL_BGSysSetBGPriority(0, 0);
        GFL_BGSysSetBGPriority(2, 1);
        {
            BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_5, GX_BGMODE_0, GX_BG0_AS_3D };

            GFL_BGSysSetLCDConfig(&lcdConfig);
        }
        reg_G2_BG2CNT = (reg_G2_BG2CNT & 0x43) | 0x4084;
        GX_SetBankForBG(GX_VRAM_BG_128_D);
        G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, 0, 4, 0, 0);
        GFL_BGSysSetBGEnabled(2, 1);
        {
            FieldFog *fog = Field_GetFog(field);

            data->scene.fogEnabled = FieldFog_IsEnabled(fog);
            FieldFog_SetEnabled(fog, FALSE);
        }
        (*state)++;
        break;
    case 4:
        EventFieldEffect_LoadGraphics(event, ci, &data->scene, ci->effectId);
        (*state)++;
        break;
    case 5:
        (*state)++;
        if (ci->callback != NULL) {
            GameSystem *gameSystem = GameEvent_GetGameSystem(event);

            GameEvent_ChainNext(event, GameEvent_Create(gameSystem, event, ci->callback, 0));
            break;
        }
    case 6:
        ci->fade = Fld3DCiFade_Create(ci->heapId);
        GFL_BGSysSetBGEnabled(2, 1);
        GFL_BGSysSetBGEnabled(0, 1);
        GFL_HeapGetFreeSize(ci->heapId);
        Field_SetRenderMode(field, 1);
        if (data->scene.dimCapture != 0) {
            EventFieldEffect_DimCaptureColor();
        }
        (*state)++;
        break;
    case 7:
        if (func_ov036_021bc828(ci->effectId)) {
            GameEvent_ChainNext(event, GameEvent_Create(gsys, event, EventFieldEffectFadeInAllBlack_Callback, 0));
        } else {
            GameEvent_ChainNext(event, GameEvent_Create(gsys, event, EventFieldEffectFadeInWhiteTop_Callback, 0));
        }
        (*state)++;
        break;
    case 8:
        if (GFL_SndIsLoadThreadRunning()) {
            break;
        }
        (*state)++;
    case 9: {
        BOOL particleFinished;
        BOOL actor1Finished;
        BOOL actor2Finished;
        BOOL finished;
        u8 speed;

        ci->frame++;
        particleFinished = Fld3DCi_UpdateParticles(ci);
        actor1Finished = Fld3DCi_UpdateActor1(ci);
        actor2Finished = Fld3DCi_UpdateActor2(ci);
        ci->particleFinished = particleFinished;
        ci->actor1Finished = actor1Finished;
        ci->actor2Finished = actor2Finished;
        speed = data->scene.fadeSpeed;
        Fld3DCi_UpdateSound(ci, speed);
        Fld3DCi_UpdateFade(ci, speed);
        finished = FALSE;
        if (ci->control != NULL) {
            finished = ci->control(event, ci);
        } else if (ci->particleFinished != 0 && ci->actor1Finished != 0 && ci->actor2Finished != 0) {
            finished = TRUE;
        }
        if (finished) {
            (*state)++;
        }
        break;
    }
    case 10:
        ci->frame++;
        Fld3DCi_UpdateFade(ci, data->scene.fadeSpeed);
        if (!Fld3DCiFade_IsRunning(ci->fade)) {
            (*state)++;
        }
        break;
    case 11:
        Field_SetRenderMode(field, 0);
        Fld3DCi_FreeG3D(ci, &data->scene);
        Fld3DCiFade_Free(ci->fade);
        if (func_ov036_021bc828(ci->effectId)) {
            FieldActor *player = FieldPlayer_GetActor(Field_GetPlayer(field));

            SetActorHidden(player, FALSE);
            CheckSetActorFaceDir(player, 1);
            Field_SetEffectRunningFlag(field, FALSE);
        } else {
            GFL_BGSysSetBGEnabled(0, 0);
            G3X_SetClearColor(0, 0x1f, 0x7fff, 0, 0);
        }
        FieldFog_SetEnabled(Field_GetFog(field), data->scene.fogEnabled);
        (*state)++;
        break;
    case 12:
        FieldLight_Flush(Field_GetLightSystem(field), TRUE);
        ci->dimDone = 0;
        GFL_VBlankTCBAdd(Fld3DCi_DisplayRestoreTask, &ci->dimDone, 0);
        (*state)++;
        break;
    case 13: {
        FieldMsgBG *msgBGSys;
        PlaceName *placeName;

        if (ci->dimDone == 0) {
            break;
        }
        GFL_HeapGetFreeSize(ci->heapId);
        msgBGSys = Field_GetMsgBGSys(field);
        placeName = Field_GetPlaceName(field);
        MsgBGPrinter_ReloadBg(msgBGSys);
        PlaceName_RestoreWindow(placeName);
        MsgBGPrinter_RequestSysFrame(msgBGSys);
        reg_G2_BG2CNT = (reg_G2_BG2CNT & 0x43) | 0x1f18;
        GFL_BGSysClearBG(2);
        Fld3DCi_RestoreBGPriorities(ci);
        Fld3DCi_RestoreScreenDispA(ci);
        Field_SetEffectRunningFlag(field, FALSE);
        if (data->scene.disableMovement) {
            EnableAllActorsMovement(Field_GetActorSystem(field));
        }
        (*state)++;
        break;
    }
    case 14:
        return TRUE;
    }
    return FALSE;
}

static BOOL Fld3DCi_UpdateParticles(Fld3DCi *ci) {
    if (ci->particleDone == 0) {
        if (ci->particleStarted[0] == 0) {
            if (ci->particleDelay[0] != 0) {
                ci->particleDelay[0]--;
            } else {
                Fld3DCi_CreateEmitter(ci, 0);
                ci->particleStarted[0] = TRUE;
            }
        }
        if (ci->particleStarted[1] == 0) {
            if (ci->particleDelay[1] != 0) {
                ci->particleDelay[1]--;
            } else {
                Fld3DCi_CreateEmitter(ci, 1);
                ci->particleStarted[1] = TRUE;
            }
        }
        if (ci->particleStarted[0] != 0 && ci->particleStarted[1] != 0) {
            ci->particleDone = 1;
        }
    } else if (FieldParticle_IsFinished(ci->particleSystem)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL Fld3DCi_UpdateActor1(Fld3DCi *ci) {
    G3DActor *actor;
    u8 i;
    int count;
    BOOL running;

    if (ci->actor1Enabled == 0) {
        return TRUE;
    }
    if (ci->actorDelay[0] != 0) {
        ci->actorDelay[0]--;
    } else {
        actor = GFL_G3DMgrGetActor(ci->mgr, GFL_G3DMgrGetSceneFirstActorIdx(ci->mgr, ci->scene));
        count = GFL_G3DActorGetAnmCount(actor);
        for (i = 0; i < count; i++) {
            running = GFL_G3DActorStepAnmFrame(actor, i, FX32_ONE);
        }
        if (!running) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL Fld3DCi_UpdateActor2(Fld3DCi *ci) {
    G3DActor *actor;
    u8 i;
    int count;
    BOOL running;

    if (ci->actor2Enabled == 0) {
        return TRUE;
    }
    if (ci->actorDelay[1] != 0) {
        ci->actorDelay[1]--;
    } else {
        actor = GFL_G3DMgrGetActor(ci->mgr, GFL_G3DMgrGetSceneFirstActorIdx(ci->mgr, ci->scene) + 1);
        count = GFL_G3DActorGetAnmCount(actor);
        for (i = 0; i < count; i++) {
            running = GFL_G3DActorStepAnmFrame(actor, i, FX32_ONE);
        }
        if (!running) {
            return TRUE;
        }
    }
    return FALSE;
}

static void EventFieldEffect_LoadGraphics(GameEvent *event, Fld3DCi *ci, Fld3DCiScene *scene, u8 effectId) {
    void *particleRes;
    u8 i;
    int first;
    int count;
    u8 header[4];

    ci->particle = FieldParticle_CreateSystem(ci->particleSystem);
    Fld3DCi_LoadG3DSceneTemplate(scene, effectId, ci->heapId);
    Fld3DCi_LoadG3DScene(ci, &scene->setup);
    if (scene->particle != 0xffff) {
        particleRes = FieldParticle_LoadResource(ci->particleSystem, 0x74, scene->particle);
        FieldParticle_SetResource(ci->particleSystem, particleRes, TRUE, NULL);
    } else {
        particleRes = NULL;
    }
    first = GFL_G3DMgrGetSceneFirstActorIdx(ci->mgr, ci->scene);
    count = GFL_G3DSceneGetNodeActorCount(ci->mgr, ci->scene);
    for (i = 0; i < count; i++) {
        G3DActor *actor;
        u8 j;
        int animCount;

        actor = GFL_G3DMgrGetActor(ci->mgr, first + i);
        animCount = GFL_G3DActorGetAnmCount(actor);
        for (j = 0; j < animCount; j++) {
            GFL_G3DActorBindAnm(actor, j);
        }
    }
    ci->particleDelay[0] = scene->particleDelay[0];
    ci->particleDelay[1] = scene->particleDelay[1];
    ci->actorDelay[0] = scene->actorDelay[0];
    ci->actorDelay[1] = scene->actorDelay[1];
    count = particleRes != NULL ? Particle_GetResourceEmitterCount(particleRes) : 0;
    if (count == 2) {
        ci->particleStarted[0] = 0;
        ci->particleStarted[1] = 0;
    } else if (count == 1) {
        ci->particleStarted[0] = 0;
        ci->particleStarted[1] = 1;
    } else {
        ci->particleStarted[0] = 1;
        ci->particleStarted[1] = 1;
    }
    ci->actor2Enabled = 0;
    ci->actor1Enabled = 0;
    if (scene->animation1Count != 0) {
        ci->actor1Enabled = 1;
    }
    if (scene->animation2Count != 0) {
        ci->actor2Enabled = 1;
    }
    GFL_ArcSysReadRange(header, 0x10c, effectId, 0, 4);
    if (header[0] != 0) {
        GameEvent_ChainNext(
            event, GameEvent_Create(GameEvent_GetGameSystem(event), event, EventFld3DCiBitmapConv_Callback, 0));
        ci->param.bitmapConv.conv.count = header[0];
        ci->param.bitmapConv.conv.file = effectId;
    }
}

static void Fld3DCi_FreeG3D(Fld3DCi *ci, Fld3DCiScene *scene) {
    Fld3DCi_FreeSceneTemplate(scene);
    GFL_G3DMgrDeleteScene(ci->mgr, ci->scene);
    ci->scene = -1;
    FieldParticle_DeleteSystem(ci->particleSystem);
}

static void Fld3DCi_CreateEmitter(Fld3DCi *ci, int id) {
    Particle_CreateEmitterWithCallback(ci->particle, id, Fld3DCi_FollowTargetCallback, NULL);
}

static void Fld3DCi_FollowTargetCallback(SPLEmitter *emitter) {
    VecFx32 offset = { 0, 0, 0 };

    offset.z = 64;
    emitter->position.x = offset.x + emitter->resource->header->emitterBasePos.x;
    emitter->position.y = offset.y + emitter->resource->header->emitterBasePos.y;
    emitter->position.z = offset.z + emitter->resource->header->emitterBasePos.z;
}

static void Fld3DCi_LoadG3DSceneTemplate(Fld3DCiScene *scene, u32 fileId, HeapID heapId) {
    Fld3DCiFile file;
    u16 ids[8];
    u8 actorCount;
    u8 anim1Count;
    u8 anim2Count;
    u8 modelStart;
    u8 anim1Start;
    u8 anim2Start;
    u8 total;
    u8 i;
    u8 j;

    GFL_ArcSysRead(&file, 0x76, fileId);
    actorCount = 0;
    for (i = 0; i < 2; i++) {
        if (file.models[i] != 0xffff) {
            actorCount++;
        }
    }
    anim2Count = 0;
    anim1Count = 0;
    for (i = 0; i < 4; i++) {
        if (file.animations1[i] != 0xffff) {
            anim1Count++;
        }
    }
    for (i = 0; i < 4; i++) {
        if (file.animations2[i] != 0xffff) {
            anim2Count++;
        }
    }
    // The scene's resources: the models, then the first animations, then the second ones
    modelStart = 0;
    anim1Start = modelStart + actorCount;
    anim2Start = anim1Start + anim1Count;
    total = actorCount + anim1Count + anim2Count;
    if (total != 0) {
        scene->resources = GFL_HeapAllocate(heapId, total * sizeof(G3DSceneResourceSetup), TRUE, "fld3d_ci.c", 1591);
    }
    if (actorCount != 0) {
        scene->actors = GFL_HeapAllocate(heapId, actorCount * sizeof(G3DSceneActorSetup), TRUE, "fld3d_ci.c", 1595);
    }
    if (anim1Count != 0) {
        scene->animations1 =
            GFL_HeapAllocate(heapId, anim1Count * sizeof(G3DSceneAnimationSetup), TRUE, "fld3d_ci.c", 1599);
    }
    if (anim2Count != 0) {
        scene->animations2 =
            GFL_HeapAllocate(heapId, anim2Count * sizeof(G3DSceneAnimationSetup), TRUE, "fld3d_ci.c", 1603);
    }
    scene->resourceCount = total;
    scene->actorCount = actorCount;
    scene->animation1Count = anim1Count;
    scene->animation2Count = anim2Count;
    scene->particleDelay[0] = file.particleDelay[0];
    scene->particleDelay[1] = file.particleDelay[1];
    scene->actorDelay[0] = file.actorDelay[0];
    scene->actorDelay[1] = file.actorDelay[1];
    ids[0] = file.models[0];
    ids[1] = file.models[1];
    ids[2] = file.animations1[0];
    ids[3] = file.animations1[1];
    ids[4] = file.animations1[2];
    ids[5] = file.animations2[0];
    ids[6] = file.animations2[1];
    ids[7] = file.animations2[2];
    // j walks the file's IDs, i counts the resources they add
    i = 0;
    for (j = 0; j < 8; j++) {
        if (ids[j] != 0xffff) {
            scene->resources[i].arcId = 0x74;
            scene->resources[i].fileId = ids[j];
            scene->resources[i].type = 0;
            i++;
        }
    }
    for (i = 0; i < anim1Count; i++) {
        scene->animations1[i].resource = anim1Start + i;
        scene->animations1[i].index = 0;
    }
    for (i = 0; i < anim2Count; i++) {
        scene->animations2[i].resource = anim2Start + i;
        scene->animations2[i].index = 0;
    }
    for (i = 0; i < actorCount; i++) {
        scene->actors[i].modelResource = modelStart + i;
        scene->actors[i].modelIndex = 0;
        scene->actors[i].texResource = modelStart + i;
    }
    if (actorCount >= 1) {
        scene->actors[0].animations = scene->animations1;
        scene->actors[0].animationCount = anim1Count;
    }
    if (actorCount >= 2) {
        scene->actors[1].animations = scene->animations2;
        scene->actors[1].animationCount = anim2Count;
    }
    scene->setup.resources = scene->resources;
    scene->setup.resourceCount = total;
    scene->setup.actorCount = actorCount;
    scene->setup.actors = scene->actors;
    scene->particle = file.particle;
    scene->dimCapture = file.dimCapture;
    scene->fadeSpeed = file.fadeSpeed;
}

static void Fld3DCi_FreeSceneTemplate(Fld3DCiScene *scene) {
    if (scene->animations1 != NULL) {
        GFL_HeapFree(scene->animations1);
    }
    if (scene->animations2 != NULL) {
        GFL_HeapFree(scene->animations2);
    }
    if (scene->actors != NULL) {
        GFL_HeapFree(scene->actors);
    }
    if (scene->resources != NULL) {
        GFL_HeapFree(scene->resources);
    }
}

static void Fld3DCi_RequestScreenCap(Fld3DCi *ci) {
    ci->screenCapDone = 0;
    GX_SetBankForLCDC(8);
    GFL_VBlankTCBAdd(Fld3DCi_ScreenCapTCB, &ci->screenCapDone, 0);
}

static void Fld3DCi_ScreenCapTCB(TCB *tcb, void *work) {
    u32 *done = work;

    reg_GX_DISPCAPCNT = 0x81330010;
    GFL_TCBRemove(tcb);
    if (done != NULL) {
        *done = TRUE;
    }
}

static void Fld3DCi_BackupBGPriorities(Fld3DCi *ci) {
    int i;

    for (i = 0; i < 4; i++) {
        ci->bgPriorities[i] = GFL_BGSysGetBGPriority(i);
    }
}

static void Fld3DCi_BackupScreenDispA(Fld3DCi *ci) {
    ci->enabledBGs = GFL_BGSysGetEnabledBGsA();
}

static void Fld3DCi_RestoreBGPriorities(Fld3DCi *ci) {
    int i;

    for (i = 0; i < 4; i++) {
        GFL_BGSysSetBGPriority(i, ci->bgPriorities[i]);
    }
}

static void Fld3DCi_RestoreScreenDispA(Fld3DCi *ci) {
    GFL_BGSysSetEnabledBGsA(ci->enabledBGs);
}

static GameEventReturnCode EventFieldEffectPokeSpriteSub_Callback(GameEvent *event, u32 *state, void *work) {
    Fld3DCiEvent *data = GameEvent_GetData(GameEvent_GetReturnEvent(event));
    Fld3DCi *ci = data->ci;

    GSYS_GetField(GameEvent_GetGameSystem(event));
    switch (*state) {
    case 0:
        EventFieldEffectPokeSpriteSub_ConvertSprite(ci);
        (*state)++;
        break;
    case 1:
        GFL_BitmapFree(ci->bitmap);
        GFL_HeapFree(ci->charData);
        GFL_HeapFree(ci->paletteData);
        (*state)++;
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

static BOOL EventFieldEffectPokeSpriteSub_ControlEffect(GameEvent *event, Fld3DCi *ci) {
    BOOL done;
    PokeSpriteParam *param = &ci->param.pokeSprite;

    if (param->unk14 != 0 && param->unk18 != 0) {
        param->unk18--;
        if (param->unk18 == 0) {
            if (param->species == SPECIES_CHATOT) {
                PokeVoiceChatterInfo info;

                PokeVoice_CreateChatterInfo(&info);
                param->voice = PokeVoice_Play(param->species, param->form, 0x40, 0, 0, 0, 0, &info);
            } else {
                param->voice = PokeVoice_Play(param->species, param->form, 0x40, 0, 0, 0, 0, NULL);
            }
        }
        param->unk1a = 30;
        return FALSE;
    }
    done = FALSE;
    if (param->unk18 == 0) {
        if (!PokeVoice_IsPlayingAny()) {
            done = TRUE;
        } else if (param->unk1a != 0) {
            param->unk1a--;
        } else if (PokeVoice_GetVolume(param->voice) > 0) {
            PokeVoice_AdjustVolume(param->voice, -16);
        } else {
            PokeVoice_ReleaseAll();
            done = TRUE;
        }
    }
    if (ci->particleFinished != 0 && ci->actor1Finished != 0 && ci->actor2Finished != 0 && done) {
        return TRUE;
    }
    return FALSE;
}

static void EventFieldEffectPokeSpriteSub_ConvertSprite(Fld3DCi *ci) {
    PokeSpriteParam *param;
    HeapID heapId = ci->heapId;
    G3DResource *res;
    u8 *charRaw;
    u8 *pixels;
    int i;
    int j;
    u32 texAddr;
    u32 palAddr;
    u32 charFile;
    u32 palFile;
    int tile;
    NNSGfdTexKey texKey;
    NNSGfdPlttKey palKey;

    ci->bitmap = GFL_BitmapCreate(16, 16, 32, heapId);
    res = GFL_G3DMgrGetResource(ci->mgr, 0);
    texKey = GFL_G3DResGetTexVRAMHandle(res);
    palKey = GFL_G3DResGetPltVRAMHandle(res);
    texAddr = NNS_GfdGetTexKeyAddr(texKey);
    palAddr = NNS_GfdGetPlttKeyAddr(palKey);
    param = &ci->param.pokeSprite;
    charFile = GetPokemonSingleCellCharacterDataNo(GetPokemonGraphicsARCID(), param->species, param->form, param->sex,
                                                   param->rare, 0, param->egg);
    palFile = GetPokemonPaletteDataNo(GetPokemonGraphicsARCID(), param->species, param->form, param->sex, param->rare,
                                      0, param->egg);
    tile = 0;
    ci->character = NULL;
    ci->palette = NULL;
    ci->charData = GFL_G2DIOReadOBJNCGR(GetPokemonGraphicsARCID(), charFile, TRUE, &ci->character, heapId);
    ci->paletteData = GFL_G2DIOReadNCLR(GetPokemonGraphicsARCID(), palFile, &ci->palette, heapId);
    charRaw = ci->character->pRawData;
    pixels = GFL_BitmapGetPixelData(ci->bitmap);
    MIi_CpuClear32(0, pixels, 0x2000);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            MIi_CpuCopy32(charRaw + tile * 32, pixels + 0x440 + (j + i * 16) * 32, 32);
            tile++;
        }
    }
    for (i = 0; i < 8; i++) {
        for (j = 8; j < 12; j++) {
            MIi_CpuCopy32(charRaw + tile * 32, pixels + 0x440 + (j + i * 16) * 32, 32);
            tile++;
        }
    }
    for (i = 8; i < 12; i++) {
        for (j = 0; j < 8; j++) {
            MIi_CpuCopy32(charRaw + tile * 32, pixels + 0x440 + (j + i * 16) * 32, 32);
            tile++;
        }
    }
    for (i = 8; i < 12; i++) {
        for (j = 8; j < 12; j++) {
            MIi_CpuCopy32(charRaw + tile * 32, pixels + 0x440 + (j + i * 16) * 32, 32);
            tile++;
        }
    }
    GFL_BitmapMakeLinear(ci->bitmap, FALSE, heapId);
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_3D_TEX_VRAM, texAddr, GFL_BitmapGetPixelData(ci->bitmap), 0x2000);
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_3D_TEX_PLTT, palAddr, ci->palette->pRawData, 32);
    GFL_HeapGetFreeSize(ci->heapId);
}

static GameEventReturnCode EventFieldEffectFadeOutWhite_Callback(GameEvent *event, u32 *state, void *work) {
    GSYS_GetField(GameEvent_GetGameSystem(event));
    switch (*state) {
    case 0:
        GFL_FadeSet(4, 0, 16, -1);
        (*state)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static GameEventReturnCode EventFieldEffectFadeInWhiteTop_Callback(GameEvent *event, u32 *state, void *work) {
    GSYS_GetField(GameEvent_GetGameSystem(event));
    switch (*state) {
    case 0:
        GFL_FadeSet(4, 16, 0, -1);
        (*state)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static GameEventReturnCode EventFieldEffectFadeInAllBlack_Callback(GameEvent *event, u32 *state, void *work) {
    GSYS_GetField(GameEvent_GetGameSystem(event));
    switch (*state) {
    case 0:
        GFL_FadeSet(3, 16, 0, 0);
        (*state)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL func_ov036_021bc828(u16 effectId) {
    if (effectId == 12 || effectId == 32) {
        return TRUE;
    }
    return FALSE;
}

static BOOL func_ov036_021bc838(u16 effectId) {
    if (effectId == 115) {
        return TRUE;
    }
    return FALSE;
}

static void Fld3DCi_ConvG2DToG3D(Fld3DCi *ci, const char *texName, const char *palName, u32 charFile, u32 palFile,
                                 u8 width, u8 height, u8 srcStride, u8 srcOffset, u8 resource, u8 palOffset) {
    G3DResource *res;
    NNSG3dResTex *tex;
    u32 texAddr;
    u32 palAddr;
    HeapID heapId = ci->heapId;
    u8 *charRaw;
    u8 *pixels;
    int row;
    int col;

    res = GFL_G3DMgrGetResource(ci->mgr, resource);
    GFL_G3DResGetTexVRAMHandle(res);
    GFL_G3DResGetPltVRAMHandle(res);
    tex = NNS_G3dGetTex(GFL_G3DResGetResData(res));
    texAddr = NNS_G3DResTexGetTexRAMAddress(tex, texName);
    palAddr = NNS_G3DResTexGetPltRAMAddress(tex, palName);
    ci->character = NULL;
    ci->palette = NULL;
    ci->charData = GFL_G2DIOReadBGNCGR(ARCID_FLD3D_CI, charFile, TRUE, &ci->character, heapId);
    ci->paletteData = GFL_G2DIOReadNCLR(ARCID_FLD3D_CI, palFile, &ci->palette, heapId);
    charRaw = ci->character->pRawData;
    pixels = GFL_BitmapGetPixelData(ci->bitmap);
    MIi_CpuClear32(0, pixels, width * height * 32);
    for (row = 0; row < height; row++) {
        for (col = 0; col < width; col++) {
            MIi_CpuCopy32(charRaw + (col + row * srcStride + srcOffset) * 32, pixels + (col + row * width) * 32, 32);
        }
    }
    GFL_BitmapMakeLinear(ci->bitmap, FALSE, heapId);
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_3D_TEX_VRAM, texAddr, GFL_BitmapGetPixelData(ci->bitmap),
                                       width * height * 32);
    NNS_GfdRegisterNewVramTransferTask(NNS_GFD_DST_3D_TEX_PLTT, palAddr, (u8 *)ci->palette->pRawData + palOffset * 32,
                                       32);
    GFL_HeapGetFreeSize(ci->heapId);
}

static BOOL EventFieldEffectCutinSub_ControlEffect(GameEvent *event, Fld3DCi *ci) {
    switch (ci->unk134) {
    case 0:
        if (ci->particleFinished != 0 && ci->actor1Finished != 0 && ci->actor2Finished != 0) {
            ci->unk134++;
        }
        break;
    case 1:
        GFL_FadeSet(((Fld3DCiEvent *)GameEvent_GetData(event))->scene.param != 0 ? 12 : 3, 0, 16, -1);
        ci->unk134++;
    case 2:
        if (!GFL_FadeIsRunning()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static GameEventReturnCode EventFieldEffectCutinSub_Callback(GameEvent *event, u32 *state, void *work) {
    Fld3DCiEvent *data = GameEvent_GetData(GameEvent_GetReturnEvent(event));
    GameSystem *gsys = GameEvent_GetGameSystem(event);
    PlayerInfo *info = GetGameDataPlayerInfo(GSYS_GetGameData(gsys));
    RivalDataSave *rival = getHollow_RivalData(GameData_GetSaveControl(GSYS_GetGameData(gsys)));
    Fld3DCi *ci = data->ci;
    CutinParam *param;
    u32 charFile;
    u32 palFile;

    GSYS_GetField(gsys);
    param = &ci->param.cutin;
    switch (*state) {
    case 0:
        EventFieldEffectCutinSub_DrawStrToTexture(info, rival, ci, "name_up", "name_up_pl", param->animation,
                                                  param->color, param->color2, FALSE);
        (*state)++;
        break;
    case 1:
        if (param->flag == 0) {
            *state = 4;
            break;
        }
        ci->bitmap = GFL_BitmapCreate(16, 16, 32, ci->heapId);
        if (param->flag == 1) {
            if (getTrainerGender(info) == 0) {
                charFile = 0x2f;
                palFile = 0x48;
            } else {
                charFile = 0x32;
                palFile = 0x4a;
            }
            *state = 3;
        } else {
            if (getTrainerGender(info) == 0) {
                charFile = 0x30;
                palFile = 0x49;
            } else {
                charFile = 0x33;
                palFile = 0x4b;
            }
            (*state)++;
        }
        Fld3DCi_ConvG2DToG3D(ci, "trsw_hero01", "trsw_hero01_pl", charFile, palFile, 16, 16, 16, 0, 0, 0);
        break;
    case 2:
        GFL_BitmapFree(ci->bitmap);
        GFL_HeapFree(ci->charData);
        GFL_HeapFree(ci->paletteData);
        ci->bitmap = GFL_BitmapCreate(16, 16, 32, ci->heapId);
        if (getTrainerGender(info) == 0) {
            charFile = 0x31;
            palFile = 0x49;
        } else {
            charFile = 0x34;
            palFile = 0x4b;
        }
        Fld3DCi_ConvG2DToG3D(ci, "trsw_hero02", "trsw_hero02_pl", charFile, palFile, 16, 16, 16, 0, 0, 1);
        (*state)++;
        break;
    case 3:
        GFL_BitmapFree(ci->bitmap);
        GFL_HeapFree(ci->charData);
        GFL_HeapFree(ci->paletteData);
        EventFieldEffectCutinSub_DrawStrToTexture(info, rival, ci, "name_down", "name_down_pl", Bank0368_Text_Empty,
                                                  param->color, param->color2, TRUE);
        (*state)++;
        break;
    case 4:
        ci->srt.translation.x = 0;
        ci->srt.translation.y = 0;
        ci->srt.translation.z = FX32_CONST(-700);
        GFL_G3DSysSetSwapBufferParams(0, 0);
        return TRUE;
    }
    return FALSE;
}

static GameEventReturnCode EventFld3DCiBitmapConv_Callback(GameEvent *event, u32 *state, void *work) {
    Fld3DCiEvent *data = GameEvent_GetData(GameEvent_GetReturnEvent(event));
    Fld3DCi *ci = data->ci;
    BitmapConvParam *param = &ci->param.bitmapConv.conv;
    Fld3DCiBitmapEntry *entry;
    u32 offset;
    u8 width;
    u8 height;
    u8 srcStride;
    u8 srcOffset;

    switch (*state) {
    case 0:
        param->entries = GFL_ArcSysReadHeapNewRange(0x10c, param->file, ci->heapId, 4, param->count * 0x30);
        param->index = 0;
        (*state)++;
    case 1:
        offset = param->index * 0x30;
        entry = (Fld3DCiBitmapEntry *)((u8 *)param->entries + offset);
        width = entry->width;
        height = entry->height;
        srcStride = entry->srcStride;
        srcOffset = entry->srcOffset;
        if (width == 0) {
            width = 16;
        }
        if (height == 0) {
            height = 16;
        }
        ci->bitmap = GFL_BitmapCreate(width, height, 32, ci->heapId);
        entry = (Fld3DCiBitmapEntry *)((u8 *)param->entries + offset);
        Fld3DCi_ConvG2DToG3D(ci, entry->texName, entry->palName, entry->charFile, entry->palFile, width, height,
                             srcStride, srcOffset, entry->resource, entry->palOffset);
        GFL_HeapFree(ci->charData);
        GFL_HeapFree(ci->paletteData);
        GFL_BitmapFree(ci->bitmap);
        param->index++;
        if (param->index == param->count) {
            GFL_HeapFree(param->entries);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static u32 Fld3DCi_GetTexDataOffset(const NNSG3dResTex *tex, const NNSG3dResDictTexData *data) {
    return (data->texImageParam << 16 >> 13) + NNS_GfdGetTexKeyAddr(tex->texInfo.vramKey);
}

static u32 Fld3DCi_GetPlttDataOffset(const NNSG3dResTex *tex, const NNSG3dResDictPlttData *data) {
    return (data->offset << 3) + NNS_GfdGetPlttKeyAddr(tex->plttInfo.vramKey);
}

static u32 NNS_G3DResTexGetTexRAMAddress(const NNSG3dResTex *tex, const char *name) {
    NNSG3dResName resName;
    const NNSG3dResDictTexData *data;

    NNS_G3DNormalizeName(&resName, name);
    data = tex != NULL ? NNS_G3dGetResDataByName(&tex->dict, &resName) : NULL;
    if (data == NULL) {
        return 0;
    }
    return Fld3DCi_GetTexDataOffset(tex, data);
}

static u32 NNS_G3DResTexGetPltRAMAddress(const NNSG3dResTex *tex, const char *name) {
    NNSG3dResName resName;
    const NNSG3dResDictPlttData *data;

    NNS_G3DNormalizeName(&resName, name);
    data = tex != NULL && tex->plttInfo.ofsDict != 0
               ? NNS_G3dGetResDataByName((const NNSG3dResDict *)((u8 *)tex + tex->plttInfo.ofsDict), &resName)
               : NULL;
    if (data == NULL) {
        return 0;
    }
    return Fld3DCi_GetPlttDataOffset(tex, data);
}

static void NNS_G3DNormalizeName(NNSG3dResName *dest, const char *name) {
    int length = STD_GetStringLength(name);
    u8 i;
    u8 j;

    for (i = 0; i < 4; i++) {
        dest->val[i] = 0;
    }
    for (j = 0; j < length; j++) {
        dest->name[j] = name[j];
    }
}

static void EventFieldEffectCutinSub_DrawStrToTexture(PlayerInfo *info, RivalDataSave *rival, Fld3DCi *ci,
                                                      const char *texName, const char *palName, u32 messageId,
                                                      u16 color, u16 color2, BOOL measure) {
    MsgData *msgData;
    G3DResource *res = GFL_G3DMgrGetResource(ci->mgr, 0);
    HeapID heapId = ci->heapId;
    u16 *raw;
    StrBuf *str;
    StrBuf *name;
    WordSet *wordSet;
    u32 width;
    G3DTextDrawResource resource;

    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0368, heapId);
    if (messageId == Bank0368_Text_Empty) {
        wordSet = GFL_WordSetSystemCreateDefault(HEAPID_TAIL(heapId));
        str = GFL_StrBufCreate(8, HEAPID_TAIL(heapId));
        name = GFL_MsgDataLoadStrbufNew(msgData, messageId);
        copyVarForText(wordSet, 0, info);
        GFL_WordSetFormatStrbuf(wordSet, str, name);
        GFL_StrBufFree(name);
        GFL_WordSetSystemFree(wordSet);
    } else if (messageId == Bank0368_Text_Empty_2) {
        wordSet = GFL_WordSetSystemCreateDefault(HEAPID_TAIL(heapId));
        str = GFL_StrBufCreate(8, HEAPID_TAIL(heapId));
        name = GFL_MsgDataLoadStrbufNew(msgData, messageId);
        GFL_WordSetLoadStr(wordSet, 0, getPtrToRivalName(rival));
        GFL_WordSetFormatStrbuf(wordSet, str, name);
        GFL_StrBufFree(name);
        GFL_WordSetSystemFree(wordSet);
    } else {
        name = GFL_MsgDataLoadStrbufNew(msgData, messageId);
        str = GFL_StrBufCreate(16, HEAPID_TAIL(heapId));
        GFL_StrBufUncompress(str, name);
        GFL_StrBufFree(name);
    }
    width = 0;
    if (measure) {
        Font *font = GFL_FontCreate(ARCID_FONT, 0, 0, 0, heapId);

        width = GFL_FontGetBlockWidth(str, font, 0);
        GFL_FontFree(font);
        width = 60 - width;
        if ((s32)width < 0) {
            width = 0;
        }
    }
    ci->paletteData = GFL_G2DIOReadNCLR(ARCID_FONT, 5, &ci->palette, heapId);
    raw = ci->palette->pRawData;
    raw[1] = color;
    raw[2] = color2;
    G3DTextDraw_CreateResource(res, texName, (u32)raw, palName, str, (u16)width, 0, 0x440, heapId, &resource);
    GFL_HeapFree(ci->paletteData);
    GFL_HeapFree(str);
    GFL_MsgDataFree(msgData);
}

GameEvent *EventFieldEffect_CreateAlderFly(GameSystem *gsys, Fld3DCi *ci, u32 actorId) {
    GameEvent *event = EventFieldEffect_Create(gsys, ci, 30);

    if (event == NULL) {
        return NULL;
    }
    ci->param.words[0] = actorId;
    return event;
}

static void EventFieldEffect_DimCaptureColor(void) {
    u32 banks = GX_ResetBankForBG();
    u16 *vram = (u16 *)0x6860000;
    int i;

    for (i = 0; i < 0xc000; i++) {
        u16 color = *vram;
        u8 b = (color >> 10) & 0x1f;
        u8 g = (color >> 5) & 0x1f;
        u8 r = color & 0x1f;
        u8 a = (color >> 15) & 1;

        b = b >> 1;
        g = g >> 1;
        r = r >> 1;
        *vram = (a << 15) | (b << 10) | (g << 5) | r;
        vram++;
    }
    GX_SetBankForBG(banks);
}

static void Fld3DCi_InitEventData(Fld3DCi *ci, Fld3DCiEvent *data) {
    MI_CpuFill8(data, 0, sizeof(Fld3DCiEvent));
    data->ci = ci;
    data->scene.disableMovement = 0;
    data->scene.param = 0;
}

static void Fld3DCi_UpdateSound(Fld3DCi *ci, u8 speed) {
    const Fld3DCiSoundKey *keys = data_ov036_021d1e80[speed];

    if (keys[ci->soundIndex].se != -1 && ci->frame == keys[ci->soundIndex].frame) {
        GFL_SndSEPlay(keys[ci->soundIndex].se);
        ci->soundIndex++;
    }
}

static void Fld3DCi_UpdateFade(Fld3DCi *ci, u8 speed) {
    const Fld3DCiFadeKey *keys = FLD3DCI_FADE_ANIMATIONS[speed];

    if (keys[ci->fadeIndex].frame != -1 && ci->frame == keys[ci->fadeIndex].frame) {
        Fld3DCiFadeState_Set(ci->fade, keys[ci->fadeIndex].flags, keys[ci->fadeIndex].from, keys[ci->fadeIndex].to,
                             keys[ci->fadeIndex].frames);
        ci->fadeIndex++;
    }
}

static void Fld3DCi_DisplayRestoreTask(TCB *tcb, void *work) {
    u32 *done = work;

    GFL_BGSysSetBGEnabled(0, 1);
    {
        BGSysLCDConfig lcdConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };

        GFL_BGSysSetLCDConfig(&lcdConfig);
    }
    GFL_TCBRemove(tcb);
    if (done != NULL) {
        *done = TRUE;
    }
}

static Fld3DCiFade *Fld3DCiFade_Create(u32 heapId) {
    Fld3DCiFade *fade = GFL_HeapAllocate(heapId, sizeof(Fld3DCiFade), FALSE, "fld3d_ci.c", 3030);

    MI_CpuFill8(fade, 0, sizeof(Fld3DCiFade));
    fade->tcb = GFL_VBlankTCBAdd(Fld3DCi_AllFadeTCB, fade, 1);
    return fade;
}

static void Fld3DCi_AllFadeTCB(TCB *tcb, void *work) {
    Fld3DCiFade *fade = work;

    if (fade != NULL && (fade->flags & 0xf) != 0) {
        fade->current += fade->step;
        fade->elapsed++;
        if ((fade->current >= fade->target && fade->step > 0) || (fade->current <= fade->target && fade->step < 0) ||
            fade->elapsed >= fade->frames) {
            fade->current = fade->target;
        }
        Fld3DCiFadeState_Apply(fade);
        if (fade->current == fade->target) {
            fade->flags = 0;
        }
    }
}

static void Fld3DCiFadeState_Apply(Fld3DCiFade *fade) {
    if (fade->flags & 1) {
        GX_SetMasterBrightness(-(fade->current >> 12));
    }
    if (fade->flags & 2) {
        GXS_SetMasterBrightness(-(fade->current >> 12));
    }
    if (fade->flags & 4) {
        GX_SetMasterBrightness(fade->current >> 12);
    }
    if (fade->flags & 8) {
        GXS_SetMasterBrightness(fade->current >> 12);
    }
}

static void Fld3DCiFadeState_Set(Fld3DCiFade *fade, u32 flags, int from, int to, int frames) {
    int diff;
    fx32 numer;
    fx32 denom;

    fade->flags = flags;
    fade->current = FX32_CONST(from);
    fade->target = FX32_CONST(to);
    fade->frames = frames;
    fade->elapsed = 0;
    diff = from - to;
    numer = FX32_CONST(MATH_ABS(diff));
    denom = FX32_CONST(frames);
    fade->step = FX_Div(numer, denom);
    if (fade->current > fade->target) {
        fade->step *= -1;
    }
    Fld3DCiFadeState_Apply(fade);
}

static void Fld3DCiFade_Free(Fld3DCiFade *fade) {
    GFL_TCBRemove(fade->tcb);
    GFL_HeapFree(fade);
}

static BOOL Fld3DCiFade_IsRunning(Fld3DCiFade *fade) {
    if (fade->flags != 0) {
        return TRUE;
    }
    return FALSE;
}
