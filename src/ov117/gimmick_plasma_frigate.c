#include "types.h"
#include "constants/sound.h"
#include "constants/vars.h"
#include "field/field.h"
#include "field/field_exp_obj.h"
#include "field/field_g3d_mapper.h"
#include "field/field_map.h"
#include "field/field_task.h"
#include "field/gimmick_plasma_frigate.h"
#include "gfl/calctool.h"
#include "gfl/g3d.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/fx.h"
#include "save/event_work.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

#define GIMMICK_WORK_ID 1
#define GIMMICK_STATE_ID 43
// The archive of the scene's models, a/2/8/6: 18 files, as many as the scene's resources
#define GIMMICK_ARCID 0x11e

// The switches, which the player presses, the panels they flip and the models that show the result. The names of the
// models are guesses, only their numbers and what they do are certain
#define SWITCH_COUNT 4
#define PANELS_PER_SWITCH 4
#define PANEL_COUNT (SWITCH_COUNT * PANELS_PER_SWITCH)
#define INDICATOR_COUNT 18
// The flags of the areas that the panels set
#define AREA_FLAGS 0x800000

// The models of the scene, by the first of each group
enum {
    MODEL_SWITCH = 0,
    MODEL_GROUP_A = 4,
    MODEL_GROUP_B = 8,
    MODEL_PANEL = 12,
    MODEL_INDICATOR = 28,
    MODEL_EFFECT_A = 46,
    MODEL_EFFECT_B = 50,
};

// What the save keeps of the frigate's puzzle
typedef struct {
    // Which panels are on
    u8 panelOn[PANEL_COUNT];
    // The values of the event variables 0x4000 to 0x4003
    u8 eventVars[4];
    // The puzzle has been saved at least once
    BOOL saved;
} PlasmaFrigateState;

typedef struct {
    FieldExpObjSystem *expObj;
    Field *field;
    PlasmaFrigateState *state;
    BOOL panelOn[PANEL_COUNT];
} PlasmaFrigateWork;

// Where a model stands, and which of four turns it has
typedef struct {
    VecFx32 pos;
    u8 rotation;
} ModelPlacement;

// A grid rectangle of the map
typedef struct {
    u8 x;
    u8 z;
    u8 width;
    u8 depth;
} PanelArea;

// A panel's task, which plays its animations
typedef struct {
    u16 state;
    u16 model;
    PlasmaFrigateWork *work;
} PanelTaskWork;

typedef struct {
    u32 switchIndex;
} SwitchEventWork;

static void GimmickPlasmaFrigate_ShowSavedState(PlasmaFrigateWork *work);
static void GimmickPlasmaFrigate_InitPanels(PlasmaFrigateWork *work);
static void GimmickPlasmaFrigate_Save(PlasmaFrigateWork *work);
static void GimmickPlasmaFrigate_Init(PlasmaFrigateWork *work);
static void GimmickPlasmaFrigate_Free(PlasmaFrigateWork *work);
static void GimmickPlasmaFrigate_StepAnimations(PlasmaFrigateWork *work);
static void GimmickPlasmaFrigate_InitModel(FieldExpObjSystem *expObj, u16 model, int animCount, VecFx32 pos);
static void GimmickPlasmaFrigate_PlayAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped);
static void GimmickPlasmaFrigate_StopAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm);
static void GimmickPlasmaFrigate_PlayPanelOff(PlasmaFrigateWork *work, u16 model);
static BOOL GimmickPlasmaFrigate_IsPanelOffDone(PlasmaFrigateWork *work, u16 model);
static void GimmickPlasmaFrigate_PlayPanelOn(PlasmaFrigateWork *work, u16 model);
static BOOL GimmickPlasmaFrigate_IsPanelOnDone(PlasmaFrigateWork *work, u16 model);
static void GimmickPlasmaFrigate_PlayPanelLit(PlasmaFrigateWork *work, u16 model);
static void GimmickPlasmaFrigate_StopLit(PlasmaFrigateWork *work, u16 model);
static void GimmickPlasmaFrigate_ShowPanelOff(PlasmaFrigateWork *work, u16 model);
static void GimmickPlasmaFrigate_ShowPanelOn(PlasmaFrigateWork *work, u16 model);
static void GimmickPlasmaFrigate_PlayFirst(PlasmaFrigateWork *work, u16 index);
static void GimmickPlasmaFrigate_PlaySecond(PlasmaFrigateWork *work, u16 index);
static void GimmickPlasmaFrigate_ShowFirstEnd(PlasmaFrigateWork *work, u16 index);
static void GimmickPlasmaFrigate_ShowSecondEnd(PlasmaFrigateWork *work, u16 index);
static void GimmickPlasmaFrigate_PlayGroupASecond(PlasmaFrigateWork *work, u16 index);
static void GimmickPlasmaFrigate_ShowGroupASecondEnd(PlasmaFrigateWork *work, u16 index);
static void GimmickPlasmaFrigate_PlayGroupAFirst(PlasmaFrigateWork *work, u16 index);
static void GimmickPlasmaFrigate_ShowGroupAFirstEnd(PlasmaFrigateWork *work, u16 index);
static void GimmickPlasmaFrigate_PlayGroupBSecond(PlasmaFrigateWork *work, u16 index);
static void GimmickPlasmaFrigate_ShowGroupBFirstEnd(PlasmaFrigateWork *work, u16 index);
static void GimmickPlasmaFrigate_ShowGroupBSecondEnd(PlasmaFrigateWork *work, u16 index);
static void GimmickPlasmaFrigate_SpawnEffectA(PlasmaFrigateWork *work, u16 panelModel);
static void GimmickPlasmaFrigate_SpawnEffectB(PlasmaFrigateWork *work, u16 panelModel);
static GameEvent *GimmickPlasmaFrigate_CreateSwitchEvent(GameSystem *gsys, int switchIndex);
static GameEventReturnCode GimmickPlasmaFrigate_SwitchEvent(GameEvent *event, u32 *state, void *data);
static void GimmickPlasmaFrigate_PressSwitch(PlasmaFrigateWork *work, int switchIndex);
static void GimmickPlasmaFrigate_TogglePanel(PlasmaFrigateWork *work, int panel);
static void GimmickPlasmaFrigate_ShowPressedSwitch(PlasmaFrigateWork *work, int switchIndex);
static void GimmickPlasmaFrigate_StartPanelOffTask(PlasmaFrigateWork *work, u16 model);
static BOOL GimmickPlasmaFrigate_PanelOffTask(void *data);
static BOOL GimmickPlasmaFrigate_IsPanelListed(u16 model);
static void GimmickPlasmaFrigate_StartPanelOnTask(PlasmaFrigateWork *work, u16 model);
static BOOL GimmickPlasmaFrigate_PanelOnTask(void *data);
static BOOL GimmickPlasmaFrigate_FindLinkedModel(PlasmaFrigateWork *work, u16 model, u16 *linkedModel);
static void GimmickPlasmaFrigate_ClearAreas(PlasmaFrigateWork *work);
static void GimmickPlasmaFrigate_SetPanelArea(PlasmaFrigateWork *work, int panel, int mode);
static void GimmickPlasmaFrigate_SetSavedPanel(PlasmaFrigateState *state, u8 on, int panel);
static BOOL GimmickPlasmaFrigate_GetSavedPanel(PlasmaFrigateState *state, int panel);
static void GimmickPlasmaFrigate_SetSavedVar(PlasmaFrigateState *state, u8 value, int index);
static u8 GimmickPlasmaFrigate_GetSavedVar(PlasmaFrigateState *state, int index);
static void GimmickPlasmaFrigate_SetSaved(PlasmaFrigateState *state);
static BOOL GimmickPlasmaFrigate_GetSaved(PlasmaFrigateState *state);

// The indicator of each panel that has none shared with another
static int sPanelIndicators[12] = { 0, 1, 6, 2, 10, 3, 14, 7, 15, 11, 16, 17 };

static const G3DSceneAnimationSetup sEffectAAnimations[] = { { 11, 0 }, { 12, 0 }, { 13, 0 } };
static const G3DSceneAnimationSetup sEffectBAnimations[] = { { 15, 0 }, { 16, 0 }, { 17, 0 } };
static const G3DSceneAnimationSetup sSwitchAnimations[] = { { 1, 0 }, { 2, 0 } };
static const G3DSceneAnimationSetup sPanelAnimations[] = { { 7, 0 }, { 8, 0 }, { 9, 0 } };
static const G3DSceneAnimationSetup sGroupAAnimations[] = { { 4, 0 }, { 5, 0 } };

// The values the areas take when a panel is off and on
static const u32 sAreaValues[2] = { 0xfffcffd1, 0xffff8000 };

// The areas of the panels when they are on and off
static const PanelArea sAreasOn[PANEL_COUNT] = {
    { 8, 41, 1, 2 },  { 5, 44, 2, 1 },  { 10, 44, 2, 1 }, { 8, 46, 1, 2 },  { 8, 48, 1, 2 },  { 5, 51, 2, 1 },
    { 10, 51, 2, 1 }, { 8, 53, 1, 3 },  { 22, 41, 1, 2 }, { 19, 44, 2, 1 }, { 24, 44, 2, 1 }, { 22, 46, 1, 2 },
    { 22, 48, 1, 2 }, { 19, 51, 2, 1 }, { 24, 51, 2, 1 }, { 22, 53, 1, 3 },
};
static const PanelArea sAreasOff[PANEL_COUNT] = {
    { 7, 42, 3, 1 },  { 5, 43, 2, 3 },  { 10, 43, 2, 3 }, { 7, 47, 3, 1 },  { 7, 49, 3, 1 },  { 5, 50, 2, 3 },
    { 10, 50, 2, 3 }, { 7, 54, 3, 1 },  { 21, 42, 3, 1 }, { 19, 43, 2, 3 }, { 24, 43, 2, 3 }, { 21, 47, 3, 1 },
    { 21, 49, 3, 1 }, { 19, 50, 2, 3 }, { 24, 50, 2, 3 }, { 21, 54, 3, 1 },
};

// The panels that go together, in pairs of one that is flipped with the other, or alone, with 0xff in place of the
// other
static const u32 sPanelPairs[14][2] = {
    { 3, 4 },    { 11, 12 },  { 0, 0xff }, { 1, 0xff },  { 2, 0xff },  { 5, 0xff },  { 6, 0xff },
    { 7, 0xff }, { 8, 0xff }, { 9, 0xff }, { 10, 0xff }, { 13, 0xff }, { 14, 0xff }, { 15, 0xff },
};

static const G3DSceneResourceSetup sResources[18] = {
    { GIMMICK_ARCID, 0, 0 },  { GIMMICK_ARCID, 1, 0 },  { GIMMICK_ARCID, 2, 0 },  { GIMMICK_ARCID, 3, 0 },
    { GIMMICK_ARCID, 4, 0 },  { GIMMICK_ARCID, 5, 0 },  { GIMMICK_ARCID, 6, 0 },  { GIMMICK_ARCID, 7, 0 },
    { GIMMICK_ARCID, 8, 0 },  { GIMMICK_ARCID, 9, 0 },  { GIMMICK_ARCID, 10, 0 }, { GIMMICK_ARCID, 11, 0 },
    { GIMMICK_ARCID, 12, 0 }, { GIMMICK_ARCID, 13, 0 }, { GIMMICK_ARCID, 14, 0 }, { GIMMICK_ARCID, 15, 0 },
    { GIMMICK_ARCID, 16, 0 }, { GIMMICK_ARCID, 17, 0 },
};

static const G3DSceneActorSetup sActors[54] = {
    { 0, 0, 0, 0, sSwitchAnimations, NELEMS(sSwitchAnimations) },
    { 0, 0, 0, 0, sSwitchAnimations, NELEMS(sSwitchAnimations) },
    { 0, 0, 0, 0, sSwitchAnimations, NELEMS(sSwitchAnimations) },
    { 0, 0, 0, 0, sSwitchAnimations, NELEMS(sSwitchAnimations) },
    { 3, 0, 3, 0, sGroupAAnimations, NELEMS(sGroupAAnimations) },
    { 3, 0, 3, 0, sGroupAAnimations, NELEMS(sGroupAAnimations) },
    { 3, 0, 3, 0, sGroupAAnimations, NELEMS(sGroupAAnimations) },
    { 3, 0, 3, 0, sGroupAAnimations, NELEMS(sGroupAAnimations) },
    { 0, 0, 0, 0, sSwitchAnimations, NELEMS(sSwitchAnimations) },
    { 0, 0, 0, 0, sSwitchAnimations, NELEMS(sSwitchAnimations) },
    { 0, 0, 0, 0, sSwitchAnimations, NELEMS(sSwitchAnimations) },
    { 0, 0, 0, 0, sSwitchAnimations, NELEMS(sSwitchAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 6, 0, 6, 0, sPanelAnimations, NELEMS(sPanelAnimations) },
    { 10, 0, 10, 0, sEffectAAnimations, NELEMS(sEffectAAnimations) },
    { 10, 0, 10, 0, sEffectAAnimations, NELEMS(sEffectAAnimations) },
    { 10, 0, 10, 0, sEffectAAnimations, NELEMS(sEffectAAnimations) },
    { 10, 0, 10, 0, sEffectAAnimations, NELEMS(sEffectAAnimations) },
    { 14, 0, 14, 0, sEffectBAnimations, NELEMS(sEffectBAnimations) },
    { 14, 0, 14, 0, sEffectBAnimations, NELEMS(sEffectBAnimations) },
    { 14, 0, 14, 0, sEffectBAnimations, NELEMS(sEffectBAnimations) },
    { 14, 0, 14, 0, sEffectBAnimations, NELEMS(sEffectBAnimations) },
};

static const ModelPlacement sPanelPlacements[PANEL_COUNT] = {
    { { FX32_CONST(136), FX32_CONST(-48), FX32_CONST(712) }, 0 },
    { { FX32_CONST(136), FX32_CONST(-48), FX32_CONST(712) }, 2 },
    { { FX32_CONST(136), FX32_CONST(-48), FX32_CONST(712) }, 3 },
    { { FX32_CONST(136), FX32_CONST(-48), FX32_CONST(712) }, 1 },
    { { FX32_CONST(136), FX32_CONST(-48), FX32_CONST(824) }, 0 },
    { { FX32_CONST(136), FX32_CONST(-48), FX32_CONST(824) }, 2 },
    { { FX32_CONST(136), FX32_CONST(-48), FX32_CONST(824) }, 3 },
    { { FX32_CONST(136), FX32_CONST(-48), FX32_CONST(824) }, 1 },
    { { FX32_CONST(360), FX32_CONST(-48), FX32_CONST(712) }, 0 },
    { { FX32_CONST(360), FX32_CONST(-48), FX32_CONST(712) }, 2 },
    { { FX32_CONST(360), FX32_CONST(-48), FX32_CONST(712) }, 3 },
    { { FX32_CONST(360), FX32_CONST(-48), FX32_CONST(712) }, 1 },
    { { FX32_CONST(360), FX32_CONST(-48), FX32_CONST(824) }, 0 },
    { { FX32_CONST(360), FX32_CONST(-48), FX32_CONST(824) }, 2 },
    { { FX32_CONST(360), FX32_CONST(-48), FX32_CONST(824) }, 3 },
    { { FX32_CONST(360), FX32_CONST(-48), FX32_CONST(824) }, 1 },
};

static const ModelPlacement sIndicatorPlacements[INDICATOR_COUNT] = {
    { { FX32_CONST(136), FX32_CONST(-48), FX32_CONST(600) }, 1 },
    { { FX32_CONST(24), FX32_CONST(-48), FX32_CONST(712) }, 3 },
    { { FX32_CONST(24), FX32_CONST(-48), FX32_CONST(824) }, 3 },
    { { FX32_CONST(136), FX32_CONST(-48), FX32_CONST(936) }, 0 },
    { { FX32_CONST(248), FX32_CONST(-48), FX32_CONST(600) }, 1 },
    { { FX32_CONST(248), FX32_CONST(-48), FX32_CONST(712) }, 0 },
    { { FX32_CONST(248), FX32_CONST(-48), FX32_CONST(712) }, 2 },
    { { FX32_CONST(248), FX32_CONST(-48), FX32_CONST(712) }, 3 },
    { { FX32_CONST(248), FX32_CONST(-48), FX32_CONST(712) }, 1 },
    { { FX32_CONST(248), FX32_CONST(-48), FX32_CONST(824) }, 0 },
    { { FX32_CONST(248), FX32_CONST(-48), FX32_CONST(824) }, 2 },
    { { FX32_CONST(248), FX32_CONST(-48), FX32_CONST(824) }, 3 },
    { { FX32_CONST(248), FX32_CONST(-48), FX32_CONST(824) }, 1 },
    { { FX32_CONST(248), FX32_CONST(-48), FX32_CONST(936) }, 0 },
    { { FX32_CONST(360), FX32_CONST(-48), FX32_CONST(600) }, 1 },
    { { FX32_CONST(472), FX32_CONST(-48), FX32_CONST(712) }, 2 },
    { { FX32_CONST(472), FX32_CONST(-48), FX32_CONST(824) }, 2 },
    { { FX32_CONST(360), FX32_CONST(-48), FX32_CONST(936) }, 0 },
};

static const G3DSceneSetup sSceneSetup = { sResources, NELEMS(sResources), sActors, NELEMS(sActors) };

void GimmickPlasmaFrigate_Setup(Field *field) {
    GameData *gameData = GSYS_GetGameData(Field_GetGameSystem(field));
    PlasmaFrigateWork *work =
        Field_AllocGimmickWorkBlock(field, GIMMICK_WORK_ID, Field_GetHeapID(field), sizeof(PlasmaFrigateWork));

    work->expObj = Field_GetExpObjSystem(field);
    work->field = field;
    work->state = GimmickState_GetUserData(GameData_GetGimmickState(gameData), GIMMICK_STATE_ID);
    GimmickPlasmaFrigate_Init(work);
    GimmickPlasmaFrigate_ShowSavedState(work);
}

void GimmickPlasmaFrigate_End(Field *field) {
    PlasmaFrigateWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    GimmickPlasmaFrigate_Save(work);
    GimmickPlasmaFrigate_ClearAreas(work);
    GimmickPlasmaFrigate_Free(work);
    Field_DeleteGimmickWorkBlock(field, GIMMICK_WORK_ID);
}

void GimmickPlasmaFrigate_Update(Field *field) {
    GimmickPlasmaFrigate_StepAnimations(Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID));
}

GameEvent *GimmickPlasmaFrigate_CallSwitchEvent(GameSystem *gsys, int switchIndex) {
    PlasmaFrigateWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    if (switchIndex >= SWITCH_COUNT) {
        return NULL;
    }
    return GimmickPlasmaFrigate_CreateSwitchEvent(gsys, switchIndex);
}

void GimmickPlasmaFrigate_SetLightsState(GameSystem *gsys, int index, BOOL on) {
    PlasmaFrigateWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    if (on == TRUE) {
        GimmickPlasmaFrigate_PlayGroupAFirst(work, index);
        GimmickPlasmaFrigate_ShowGroupBFirstEnd(work, index);
    } else {
        GimmickPlasmaFrigate_ShowGroupASecondEnd(work, index);
        GimmickPlasmaFrigate_ShowGroupBSecondEnd(work, index);
    }
}

void GimmickPlasmaFrigate_CallPlayGroupASecond(GameSystem *gsys, int index) {
    PlasmaFrigateWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    GimmickPlasmaFrigate_PlayGroupASecond(work, index);
    GFL_SndSEPlay(SEQ_SE_SW_PLAZMASHIP_05);
}

void GimmickPlasmaFrigate_CallPlayGroupBSecond(GameSystem *gsys, int index) {
    PlasmaFrigateWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), GIMMICK_WORK_ID);

    GimmickPlasmaFrigate_PlayGroupBSecond(work, index);
    GFL_SndSEPlay(SEQ_SE_SW_PLAZMASHIP_04);
}

// Shows the panels as the saved state left them
static void GimmickPlasmaFrigate_ShowSavedState(PlasmaFrigateWork *work) {
    int i;
    int j;

    if (GimmickPlasmaFrigate_GetSaved(work->state) == FALSE) {
        GimmickPlasmaFrigate_InitPanels(work);
    } else {
        for (i = 0; i < PANEL_COUNT; i++) {
            if (GimmickPlasmaFrigate_GetSavedPanel(work->state, i) == FALSE) {
                work->panelOn[i] = FALSE;
            } else {
                work->panelOn[i] = TRUE;
            }
        }
    }
    for (i = 0; i < PANEL_COUNT; i++) {
        u16 model = MODEL_PANEL + i;

        if (work->panelOn[i] == TRUE) {
            GimmickPlasmaFrigate_ShowPanelOn(work, model);
        } else {
            GimmickPlasmaFrigate_ShowPanelOff(work, model);
        }
    }
    for (i = 0; i < INDICATOR_COUNT; i++) {
        u16 model = MODEL_INDICATOR + i;

        for (j = 0; j < 12; j++) {
            if (i == sPanelIndicators[j]) {
                if (work->panelOn[sPanelPairs[j + 2][0]] == FALSE) {
                    GimmickPlasmaFrigate_StopLit(work, model);
                }
                break;
            }
        }
    }
    for (i = 0; i < SWITCH_COUNT; i++) {
        if (GimmickPlasmaFrigate_GetSavedVar(work->state, i)) {
            GimmickPlasmaFrigate_ShowSecondEnd(work, MODEL_SWITCH + i);
        } else {
            GimmickPlasmaFrigate_ShowFirstEnd(work, MODEL_SWITCH + i);
        }
    }
    for (i = 0; i < PANEL_COUNT; i++) {
        GimmickPlasmaFrigate_SetPanelArea(work, i, work->panelOn[i]);
    }
}

// Fills the panels in with their starting states
static void GimmickPlasmaFrigate_InitPanels(PlasmaFrigateWork *work) {
    u8 panelOn[PANEL_COUNT] = { 0 };
#ifdef BLACK2
    u8 startingOn[PANEL_COUNT] = { 1, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 1 };
#else
    u8 startingOn[PANEL_COUNT] = { 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1 };
#endif
    int i;

    sys_memcpy(startingOn, panelOn, sizeof(panelOn));
    for (i = 0; i < PANEL_COUNT; i++) {
        if (panelOn[i] == 0) {
            work->panelOn[i] = FALSE;
        } else {
            work->panelOn[i] = TRUE;
        }
    }
}

// Keeps the panels and the event variables in the save
static void GimmickPlasmaFrigate_Save(PlasmaFrigateWork *work) {
    EventWork *eventWork = GameData_GetEventWork(GSYS_GetGameData(Field_GetGameSystem(work->field)));
    u16 *var0 = EventWork_GetWkPtr(eventWork, EVENT_WORK_0x4000);
    u16 *var1 = EventWork_GetWkPtr(eventWork, EVENT_WORK_0x4001);
    u16 *var2 = EventWork_GetWkPtr(eventWork, EVENT_WORK_0x4002);
    u16 *var3 = EventWork_GetWkPtr(eventWork, EVENT_WORK_0x4003);
    int i;

    GimmickPlasmaFrigate_SetSavedVar(work->state, *var0, 0);
    GimmickPlasmaFrigate_SetSavedVar(work->state, *var1, 1);
    GimmickPlasmaFrigate_SetSavedVar(work->state, *var2, 2);
    GimmickPlasmaFrigate_SetSavedVar(work->state, *var3, 3);
    for (i = 0; i < PANEL_COUNT; i++) {
        GimmickPlasmaFrigate_SetSavedPanel(work->state, work->panelOn[i], i);
    }
    GimmickPlasmaFrigate_SetSaved(work->state);
}

static void GimmickPlasmaFrigate_Init(PlasmaFrigateWork *work) {
    int i;
    int model;
    u16 *rotation;

    LoadFieldExpandObjData(work->expObj, &sSceneSetup, 0);
    VecFx32 switchPositions[4] = {
        { FX32_CONST(136), FX32_CONST(0), FX32_CONST(712) },
        { FX32_CONST(136), FX32_CONST(0), FX32_CONST(824) },
        { FX32_CONST(360), FX32_CONST(0), FX32_CONST(712) },
        { FX32_CONST(360), FX32_CONST(0), FX32_CONST(824) },
    };
    for (i = 0; i < SWITCH_COUNT; i++) {
        GimmickPlasmaFrigate_InitModel(work->expObj, MODEL_SWITCH + i, NELEMS(sSwitchAnimations), switchPositions[i]);
    }
    VecFx32 groupAPositions[4] = {
        { FX32_CONST(248), FX32_CONST(0), FX32_CONST(272) },
        { FX32_CONST(248), FX32_CONST(0), FX32_CONST(256) },
        { FX32_CONST(248), FX32_CONST(0), FX32_CONST(240) },
        { FX32_CONST(248), FX32_CONST(0), FX32_CONST(224) },
    };
    for (i = 0; i < SWITCH_COUNT; i++) {
        model = MODEL_GROUP_A + i;
        GimmickPlasmaFrigate_InitModel(work->expObj, model, NELEMS(sGroupAAnimations), groupAPositions[i]);
        GimmickPlasmaFrigate_PlayAnimation(work->expObj, model, 0, TRUE);
    }
    VecFx32 groupBPositions[4] = {
        { FX32_CONST(152), FX32_CONST(0), FX32_CONST(264) },
        { FX32_CONST(152), FX32_CONST(0), FX32_CONST(344) },
        { FX32_CONST(344), FX32_CONST(0), FX32_CONST(264) },
        { FX32_CONST(344), FX32_CONST(0), FX32_CONST(344) },
    };
    for (i = 0; i < SWITCH_COUNT; i++) {
        model = MODEL_GROUP_B + i;
        GimmickPlasmaFrigate_InitModel(work->expObj, model, NELEMS(sSwitchAnimations), groupBPositions[i]);
#ifdef BLACK2
        FieldExpObj_SetActorHidden(work->expObj, 0, model, TRUE);
#endif
    }
    for (i = 0; i < PANEL_COUNT; i++) {
        model = MODEL_PANEL + i;
        GimmickPlasmaFrigate_InitModel(work->expObj, model, NELEMS(sPanelAnimations), sPanelPlacements[i].pos);
        u16 rotations[4][3] = { { 0, 0x4000, 0 }, { 0, 0xc000, 0 }, { 0, 0x8000, 0 }, { 0, 0, 0 } };

        rotation = rotations[sPanelPlacements[i].rotation];
        MAT3_RotationEulerZYX(rotation[0], rotation[1], rotation[2],
                              &FieldExpObj_GetActorMatrixPtr(work->expObj, 0, model)->rotation);
    }
    for (i = 0; i < INDICATOR_COUNT; i++) {
        model = MODEL_INDICATOR + i;
        GimmickPlasmaFrigate_InitModel(work->expObj, model, NELEMS(sPanelAnimations), sIndicatorPlacements[i].pos);
        u16 rotations[4][3] = { { 0, 0x4000, 0 }, { 0, 0xc000, 0 }, { 0, 0x8000, 0 }, { 0, 0, 0 } };

        rotation = rotations[sIndicatorPlacements[i].rotation];
        MAT3_RotationEulerZYX(rotation[0], rotation[1], rotation[2],
                              &FieldExpObj_GetActorMatrixPtr(work->expObj, 0, model)->rotation);
        GimmickPlasmaFrigate_PlayPanelLit(work, model);
    }
    VecFx32 zero = { 0, 0, 0 };

    for (i = 0; i < SWITCH_COUNT; i++) {
        model = MODEL_EFFECT_A + i;
        GimmickPlasmaFrigate_InitModel(work->expObj, model, NELEMS(sEffectAAnimations), zero);
        FieldExpObj_SetActorHidden(work->expObj, 0, model, TRUE);
    }
    for (i = 0; i < SWITCH_COUNT; i++) {
        model = MODEL_EFFECT_B + i;
        GimmickPlasmaFrigate_InitModel(work->expObj, model, NELEMS(sEffectBAnimations), zero);
        FieldExpObj_SetActorHidden(work->expObj, 0, model, TRUE);
    }
}

static void GimmickPlasmaFrigate_Free(PlasmaFrigateWork *work) {
    FieldExpObj_FreeScene(work->expObj, 0);
}

static void GimmickPlasmaFrigate_StepAnimations(PlasmaFrigateWork *work) {
    FieldExpObj_StepAllAnimations(work->expObj);
}

// Places the model, hidden, with its animations set and paused
static void GimmickPlasmaFrigate_InitModel(FieldExpObjSystem *expObj, u16 model, int animCount, VecFx32 pos) {
    SRTMatrix *matrix;
    FieldExpObjAnm *anm;
    int i;
    fx32 z = pos.z;
    fx32 y = pos.y;

    matrix = FieldExpObj_GetActorMatrixPtr(expObj, 0, model);
    matrix->translation.x = pos.x;
    matrix->translation.y = y;
    matrix->translation.z = z;
    func_ov036_021b8248(expObj, 0, model, 1);
    FieldExpObj_SetActorHidden(expObj, 0, model, FALSE);
    for (i = 0; i < animCount; i++) {
        anm = FieldExpObj_GetAnmInfo(expObj, 0, model, i);
        FieldExpObjAnm_SetLooped(anm, FALSE);
        FieldExpObjAnm_SetPaused(anm, TRUE);
        FieldExpObj_SetAnm(expObj, 0, model, i, FALSE);
    }
}

// Plays the animation from its first frame
static void GimmickPlasmaFrigate_PlayAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm, BOOL looped) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, FALSE);
    FieldExpObjAnm_SetLooped(anmInfo, looped);
    FieldExpObj_SetAnm(expObj, 0, model, anm, TRUE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Stops the animation at its first frame
static void GimmickPlasmaFrigate_StopAnimation(FieldExpObjSystem *expObj, u16 model, u16 anm) {
    FieldExpObjAnm *anmInfo = FieldExpObj_GetAnmInfo(expObj, 0, model, anm);

    FieldExpObjAnm_SetPaused(anmInfo, TRUE);
    FieldExpObj_SetAnm(expObj, 0, model, anm, FALSE);
    FieldExpObj_SetAnmFrame(expObj, 0, model, anm, 0);
}

// Plays animation 0 once and stops the others
static void GimmickPlasmaFrigate_PlayPanelOff(PlasmaFrigateWork *work, u16 model) {
    GimmickPlasmaFrigate_PlayAnimation(work->expObj, model, 0, FALSE);
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 1);
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 2);
}

static BOOL GimmickPlasmaFrigate_IsPanelOffDone(PlasmaFrigateWork *work, u16 model) {
    return FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(work->expObj, 0, model, 0));
}

// Plays animation 2 once and stops the others
static void GimmickPlasmaFrigate_PlayPanelOn(PlasmaFrigateWork *work, u16 model) {
    GimmickPlasmaFrigate_PlayAnimation(work->expObj, model, 2, FALSE);
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 0);
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 1);
}

static BOOL GimmickPlasmaFrigate_IsPanelOnDone(PlasmaFrigateWork *work, u16 model) {
    return FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(work->expObj, 0, model, 2));
}

// Plays animation 1 once and stops the others
static void GimmickPlasmaFrigate_PlayPanelLit(PlasmaFrigateWork *work, u16 model) {
    GimmickPlasmaFrigate_PlayAnimation(work->expObj, model, 1, FALSE);
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 0);
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 2);
}

static void GimmickPlasmaFrigate_StopLit(PlasmaFrigateWork *work, u16 model) {
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 1);
}

// Puts the panel at the end of animation 0, with the others stopped
static void GimmickPlasmaFrigate_ShowPanelOff(PlasmaFrigateWork *work, u16 model) {
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 0);

    FieldExpObj_SetAnm(work->expObj, 0, model, 0, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 0, func_ov036_021b8580(anm));
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 1);
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 2);
}

// Puts the panel at the end of animation 2, and of animation 1 if what it goes with is on
static void GimmickPlasmaFrigate_ShowPanelOn(PlasmaFrigateWork *work, u16 model) {
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 2);
    BOOL showLit = TRUE;
    u16 linkedModel;

    FieldExpObj_SetAnm(work->expObj, 0, model, 2, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 2, func_ov036_021b8580(anm));
    if (GimmickPlasmaFrigate_IsPanelListed(model) == TRUE) {
        linkedModel = 0;
        if (GimmickPlasmaFrigate_FindLinkedModel(work, model, &linkedModel) == FALSE) {
            showLit = FALSE;
        }
    }
    if (showLit == TRUE) {
        anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 1);
        FieldExpObj_SetAnm(work->expObj, 0, model, 1, TRUE);
        FieldExpObj_SetAnmFrame(work->expObj, 0, model, 1, func_ov036_021b8580(anm));
    }
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 0);
}

// Plays animation 0 once and stops animation 1
static void GimmickPlasmaFrigate_PlayFirst(PlasmaFrigateWork *work, u16 index) {
    u16 model = index + MODEL_SWITCH;
    GimmickPlasmaFrigate_PlayAnimation(work->expObj, model, 0, FALSE);
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 1);
}

// Plays animation 1 once and stops animation 0
static void GimmickPlasmaFrigate_PlaySecond(PlasmaFrigateWork *work, u16 index) {
    u16 model = index + MODEL_SWITCH;
    GimmickPlasmaFrigate_PlayAnimation(work->expObj, model, 1, FALSE);
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 0);
}

// Puts the model at the end of animation 0, with animation 1 stopped
static void GimmickPlasmaFrigate_ShowFirstEnd(PlasmaFrigateWork *work, u16 index) {
    u16 model = index + MODEL_SWITCH;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 0);

    FieldExpObj_SetAnm(work->expObj, 0, model, 0, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 0, func_ov036_021b8580(anm));
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 1);
}

// Puts the model at the end of animation 1, with animation 0 stopped
static void GimmickPlasmaFrigate_ShowSecondEnd(PlasmaFrigateWork *work, u16 index) {
    u16 model = index + MODEL_SWITCH;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 1);

    FieldExpObj_SetAnm(work->expObj, 0, model, 1, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 1, func_ov036_021b8580(anm));
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 0);
}

// Plays animation 1 of the index-th model of group A once
static void GimmickPlasmaFrigate_PlayGroupASecond(PlasmaFrigateWork *work, u16 index) {
    GimmickPlasmaFrigate_PlayAnimation(work->expObj, index + MODEL_GROUP_A, 1, FALSE);
}

static void GimmickPlasmaFrigate_ShowGroupASecondEnd(PlasmaFrigateWork *work, u16 index) {
    u16 model = index + MODEL_GROUP_A;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 1);

    FieldExpObj_SetAnm(work->expObj, 0, model, 1, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 1, func_ov036_021b8580(anm));
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 0);
}

// Loops animation 0 of the index-th model of group A and stops animation 1
static void GimmickPlasmaFrigate_PlayGroupAFirst(PlasmaFrigateWork *work, u16 index) {
    u16 model = index + MODEL_GROUP_A;

    GimmickPlasmaFrigate_PlayAnimation(work->expObj, model, 0, TRUE);
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 1);
}

static void GimmickPlasmaFrigate_PlayGroupBSecond(PlasmaFrigateWork *work, u16 index) {
    u16 model = index + MODEL_GROUP_B;

    GimmickPlasmaFrigate_PlayAnimation(work->expObj, model, 1, FALSE);
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 0);
}

static void GimmickPlasmaFrigate_ShowGroupBFirstEnd(PlasmaFrigateWork *work, u16 index) {
    u16 model = index + MODEL_GROUP_B;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 0);

    FieldExpObj_SetAnm(work->expObj, 0, model, 0, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 0, func_ov036_021b8580(anm));
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 1);
}

static void GimmickPlasmaFrigate_ShowGroupBSecondEnd(PlasmaFrigateWork *work, u16 index) {
    u16 model = index + MODEL_GROUP_B;
    FieldExpObjAnm *anm = FieldExpObj_GetAnmInfo(work->expObj, 0, model, 1);

    FieldExpObj_SetAnm(work->expObj, 0, model, 1, TRUE);
    FieldExpObj_SetAnmFrame(work->expObj, 0, model, 1, func_ov036_021b8580(anm));
    GimmickPlasmaFrigate_StopAnimation(work->expObj, model, 0);
}

// Shows the effect over a panel, as a copy of its model's placement, and plays it
static void GimmickPlasmaFrigate_SpawnEffectA(PlasmaFrigateWork *work, u16 panelModel) {
    u16 effect = MODEL_EFFECT_A + panelModel % 4;
    SRTMatrix *dst;
    SRTMatrix *src;
    u32 i;

    FieldExpObj_SetActorHidden(work->expObj, 0, effect, FALSE);
    dst = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, effect);
    src = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, panelModel);
    dst->translation = src->translation;
    dst->rotation = src->rotation;
    dst->scale = src->scale;
    for (i = 0; i < NELEMS(sEffectAAnimations); i++) {
        GimmickPlasmaFrigate_PlayAnimation(work->expObj, effect, i, FALSE);
    }
}

static void GimmickPlasmaFrigate_SpawnEffectB(PlasmaFrigateWork *work, u16 panelModel) {
    u16 effect = MODEL_EFFECT_B + panelModel % 4;
    SRTMatrix *dst;
    SRTMatrix *src;
    u32 i;

    FieldExpObj_SetActorHidden(work->expObj, 0, effect, FALSE);
    dst = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, effect);
    src = FieldExpObj_GetActorMatrixPtr(work->expObj, 0, panelModel);
    dst->translation = src->translation;
    dst->rotation = src->rotation;
    dst->scale = src->scale;
    for (i = 0; i < NELEMS(sEffectBAnimations); i++) {
        GimmickPlasmaFrigate_PlayAnimation(work->expObj, effect, i, FALSE);
    }
}

static GameEvent *GimmickPlasmaFrigate_CreateSwitchEvent(GameSystem *gsys, int switchIndex) {
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GameEvent_Create(gsys, NULL, GimmickPlasmaFrigate_SwitchEvent, sizeof(SwitchEventWork));
    SwitchEventWork *eventWork = GameEvent_GetData(event);

    eventWork->switchIndex = switchIndex;
    return event;
}

// Presses the switch, waits for the panels to finish, and saves
static GameEventReturnCode GimmickPlasmaFrigate_SwitchEvent(GameEvent *event, u32 *state, void *data) {
    SwitchEventWork *eventWork = data;
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    PlasmaFrigateWork *work = Field_GetGimmickWorkBlock(field, GIMMICK_WORK_ID);

    switch (*state) {
    case 0:
        GimmickPlasmaFrigate_PressSwitch(work, eventWork->switchIndex);
        GimmickPlasmaFrigate_ShowPressedSwitch(work, eventWork->switchIndex);
        GFL_SndSEPlay(SEQ_SE_SW_PLAZMASHIP_04);
        *state = 1;
        break;
    case 1:
        if (FieldTaskManager_IsIdle(Field_GetTaskManager(field)) == TRUE) {
            *state = 2;
        }
        break;
    case 2:
        GimmickPlasmaFrigate_Save(work);
        *state = 3;
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Flips the switch's panels
static void GimmickPlasmaFrigate_PressSwitch(PlasmaFrigateWork *work, int switchIndex) {
    int i;

    for (i = 0; i < PANELS_PER_SWITCH; i++) {
        GimmickPlasmaFrigate_TogglePanel(work, switchIndex * PANELS_PER_SWITCH + i);
    }
}

static void GimmickPlasmaFrigate_TogglePanel(PlasmaFrigateWork *work, int panel) {
    int model = MODEL_PANEL + panel;

    if (work->panelOn[panel] == TRUE) {
        GimmickPlasmaFrigate_StartPanelOffTask(work, model);
    } else {
        GimmickPlasmaFrigate_StartPanelOnTask(work, model);
    }
    work->panelOn[panel] = work->panelOn[panel] == FALSE;
    GimmickPlasmaFrigate_SetPanelArea(work, panel, work->panelOn[panel]);
}

// Shows the switch pressed and the others not
static void GimmickPlasmaFrigate_ShowPressedSwitch(PlasmaFrigateWork *work, int switchIndex) {
    int i;

    GimmickPlasmaFrigate_PlaySecond(work, switchIndex);
    for (i = 0; i < SWITCH_COUNT; i++) {
        if (i != switchIndex) {
            GimmickPlasmaFrigate_PlayFirst(work, i);
        }
    }
}

static void GimmickPlasmaFrigate_StartPanelOffTask(PlasmaFrigateWork *work, u16 model) {
    HeapID heapId = Field_GetHeapID(work->field);
    FieldTaskManager *manager = Field_GetTaskManager(work->field);
    FieldTask *task = FieldTask_Create(heapId, sizeof(PanelTaskWork), GimmickPlasmaFrigate_PanelOffTask);
    PanelTaskWork *taskWork = FieldTask_GetData(task);

    taskWork->state = 0;
    taskWork->model = model;
    taskWork->work = work;
    FieldTaskManager_AddTask(manager, task, 0);
}

static BOOL GimmickPlasmaFrigate_PanelOffTask(void *data) {
    PanelTaskWork *taskWork = data;
    u16 linkedModel;

    switch (taskWork->state) {
    case 0:
        GimmickPlasmaFrigate_PlayPanelOff(taskWork->work, taskWork->model);
        GFL_SndSEPlay(SEQ_SE_SW_PLAZMASHIP_07);
        if (GimmickPlasmaFrigate_IsPanelListed(taskWork->model)) {
            linkedModel = 0;
            if (GimmickPlasmaFrigate_FindLinkedModel(taskWork->work, taskWork->model, &linkedModel) == TRUE) {
                GimmickPlasmaFrigate_StopLit(taskWork->work, linkedModel);
            }
        }
        taskWork->state++;
        break;
    case 1:
        if (GimmickPlasmaFrigate_IsPanelOffDone(taskWork->work, taskWork->model)) {
            taskWork->state++;
        }
        break;
    case 2:
        return TRUE;
    }
    return FALSE;
}

static void GimmickPlasmaFrigate_StartPanelOnTask(PlasmaFrigateWork *work, u16 model) {
    HeapID heapId = Field_GetHeapID(work->field);
    FieldTaskManager *manager = Field_GetTaskManager(work->field);
    FieldTask *task = FieldTask_Create(heapId, sizeof(PanelTaskWork), GimmickPlasmaFrigate_PanelOnTask);
    PanelTaskWork *taskWork = FieldTask_GetData(task);

    taskWork->state = 0;
    taskWork->model = model;
    taskWork->work = work;
    FieldTaskManager_AddTask(manager, task, 0);
}

static BOOL GimmickPlasmaFrigate_PanelOnTask(void *data) {
    PanelTaskWork *taskWork = data;
    u16 linkedModel;

    switch (taskWork->state) {
    case 0:
        GimmickPlasmaFrigate_PlayPanelOn(taskWork->work, taskWork->model);
        GimmickPlasmaFrigate_SpawnEffectA(taskWork->work, taskWork->model);
        GFL_SndSEPlay(SEQ_SE_SW_PLAZMASHIP_06);
        taskWork->state++;
        break;
    case 1:
        if (GimmickPlasmaFrigate_IsPanelOnDone(taskWork->work, taskWork->model)) {
            taskWork->state++;
        }
        break;
    case 2:
        if (GimmickPlasmaFrigate_IsPanelListed(taskWork->model)) {
            linkedModel = 0;
            if (GimmickPlasmaFrigate_FindLinkedModel(taskWork->work, taskWork->model, &linkedModel) == FALSE) {
                return TRUE;
            }
            GimmickPlasmaFrigate_PlayPanelLit(taskWork->work, linkedModel);
        }
        GimmickPlasmaFrigate_PlayPanelLit(taskWork->work, taskWork->model);
        GimmickPlasmaFrigate_SpawnEffectB(taskWork->work, taskWork->model);
        GFL_SndSEPlay(SEQ_SE_SW_PLAZMASHIP_07);
        taskWork->state++;
        break;
    case 3:
        return TRUE;
    }
    return FALSE;
}

// Whether the panel is in the table of panels
static BOOL GimmickPlasmaFrigate_IsPanelListed(u16 model) {
    int panel = model - MODEL_PANEL;
    int i;
    int j;
    int linked;

    for (i = 0; i < 14; i++) {
        for (j = 0; j < 2; j++) {
            linked = sPanelPairs[i][j];
            if (linked == panel) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

// Finds what goes with the panel: the panel it is paired with, whose state it returns, or its indicator, when it has
// none
static BOOL GimmickPlasmaFrigate_FindLinkedModel(PlasmaFrigateWork *work, u16 model, u16 *linkedModel) {
    int i;
    int panel;
    int first;
    int second;

    GimmickPlasmaFrigate_IsPanelListed(model);
    i = 0;
    panel = model - MODEL_PANEL;
    for (; i < 14; i++) {
        first = sPanelPairs[i][0];
        if (first == panel) {
            second = sPanelPairs[i][1];
            if (second == 0xff) {
                *linkedModel = sPanelIndicators[i - 2] + MODEL_INDICATOR;
                return TRUE;
            }
            *linkedModel = second + MODEL_PANEL;
            return work->panelOn[second];
        }
        second = sPanelPairs[i][1];
        if (second == panel) {
            *linkedModel = first + MODEL_PANEL;
            return work->panelOn[first];
        }
    }
    return FALSE;
}

static void GimmickPlasmaFrigate_ClearAreas(PlasmaFrigateWork *work) {
    void *list = func_ov036_02184590(Field_GetG3DMapper(work->field));
    int count = func_ov036_021ba684(list);
    int i;

    for (i = 0; i < count; i++) {
        func_ov036_021ba6bc(i, 0, list);
    }
}

// Sets the area of the panel, and merges or splits those of the panels that go together
static void GimmickPlasmaFrigate_SetPanelArea(PlasmaFrigateWork *work, int panel, int mode) {
    void *list = func_ov036_02184590(Field_GetG3DMapper(work->field));
    const PanelArea *area;
    int first;

    if (mode == 0) {
        area = &sAreasOff[panel];
        u16 model = MODEL_PANEL + panel;
        u16 linkedModel = 0;

        first = -1;
        if (panel == 3 || panel == 11 || panel == 4 || panel == 12) {
            if (GimmickPlasmaFrigate_FindLinkedModel(work, model, &linkedModel) == FALSE) {
                first = panel;
                if (panel == 4 || panel == 12) {
                    first--;
                }
                func_ov036_021ba6bc(first, 0, list);
                func_ov036_021ba624(first, sAreasOff[first].x, sAreasOff[first].z, sAreasOff[first].width,
                                    sAreasOff[first].depth + 1, sAreaValues[0], AREA_FLAGS, list);
            }
            if (first == panel) {
                return;
            }
        }
    } else {
        area = &sAreasOn[panel];
        u16 model = MODEL_PANEL + panel;
        u16 linkedModel = 0;

        if (panel == 4 || panel == 12) {
            if (GimmickPlasmaFrigate_FindLinkedModel(work, model, &linkedModel) == FALSE) {
                first = panel - 1;
                func_ov036_021ba6bc(first, 0, list);
                func_ov036_021ba624(first, sAreasOff[first].x, sAreasOff[first].z, sAreasOff[first].width,
                                    sAreasOff[first].depth, sAreaValues[0], AREA_FLAGS, list);
            }
        }
    }
    func_ov036_021ba6bc(panel, 0, list);
    func_ov036_021ba624(panel, area->x, area->z, area->width, area->depth, sAreaValues[mode], AREA_FLAGS, list);
}

static void GimmickPlasmaFrigate_SetSavedPanel(PlasmaFrigateState *state, u8 on, int panel) {
    state->panelOn[panel] = on;
}

static BOOL GimmickPlasmaFrigate_GetSavedPanel(PlasmaFrigateState *state, int panel) {
    return state->panelOn[panel];
}

static void GimmickPlasmaFrigate_SetSavedVar(PlasmaFrigateState *state, u8 value, int index) {
    state->eventVars[index] = value;
}

static u8 GimmickPlasmaFrigate_GetSavedVar(PlasmaFrigateState *state, int index) {
    return state->eventVars[index];
}

static void GimmickPlasmaFrigate_SetSaved(PlasmaFrigateState *state) {
    state->saved = TRUE;
}

static BOOL GimmickPlasmaFrigate_GetSaved(PlasmaFrigateState *state) {
    return state->saved;
}
