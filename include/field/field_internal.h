#ifndef POKEBW2_FIELD_FIELD_INTERNAL_H
#define POKEBW2_FIELD_FIELD_INTERNAL_H

#include "field/field.h"
#include "field/field_controller.h"
#include "field/field_g3d_mapper.h"
#include "field/field_palace.h"
#include "field/field_scene_area.h"
#include "field/zone.h"
#include "gfl/blact.h"

// The gimmick work of Field_AllocGimmickWorkBlock, password 0xffffffff for none
typedef struct {
    u32 password;
    void *work;
} FieldGimmickWorkBlock;

// Layout from swan
struct Field {
    u16 heapId;
    u16 unk2;
    GameSystem *gameSystem;
    GameData *gameData;
    AreaData *areaData;
    FieldCamera *cameraSystem;
    FieldLight *lightSystem;
    FieldFog *fog;
    FieldFogCtrl *fogCtrl;
    FieldWeather *weatherSystem;
    FieldSubscreen *subscreen;
    FieldMsgBG *msgBGSys;
    MoneyWin *moneyWin;
    PlaceName *placeName;
    void *unk34;
    FesGimmick *fesGimmick;
    FieldExpObjSystem *expObjSystem;
    MMSys *actorSystem;
    NoGridMapper *noGridMapper;
    FieldSceneArea *sceneArea;
    FieldSceneAreaLoader *sceneAreaLoader;
    FieldG3DMapper *g3DMapper;
    u32 unk54;
    FieldG3DMapperConfig mapperConfig;
    FieldPlayer *player;
    EncountSystem *encountSystem;
    FieldEffects *fieldEffects;
    void *unkA0;
    FieldPalaceSys *palaceSys;
    FieldColorPostFX colorPostFX;
    FieldSkillMapEff *skillMapEff;
    G3DCamera *g3dCamera;
    G3DLight *g3dLights;
    void *asyncActorMatLoadTCB;
    BlActSys *actorBlAct;
    BlActSys *effectBlAct;
    BlActSys *wildEffectBlAct;
    FieldAsyncProcManager *asyncProcManager;
    // 1 when the map is loaded, 2 when closing
    u32 routineState;
    u32 routineID;
    u8 subroutinePhase;
    u8 routineAlternator;
    u16 unkDA;
    u32 framesSinceStart;
    u16 zoneId;
    u16 unkE2;
    ZoneSpawnInfo playerZoneState;
    VecFx32 *playerPosPtr;
    VecFx32 playerPos;
    fx32 actorYOffset;
    fx32 objectProjectionMatrixOffset;
    const FieldmapCtrlVTable *ctrlVTable;
    void *controller;
    TCBManager *tcbManager;
    void *tcbManagerHeap;
    FieldGimmickWorkBlock gimmickWork;
    FieldParticle *particleSystem;
    Fld3DCi *g3dCi;
    DayCare *dayCare;
    u32 renderMode;
    FieldG3DObjSystem *g3DObjSystem;
    FieldTaskManager *taskManager;
    BOOL fadeFlag;
    EncEff *encEff;
    BOOL effectRunningFlag;
    BOOL seasonBannerOverdrawFlag;
    void *nDemoData;
    void *dispControl;
    CasteliaRush *casteliaRush;
    FieldLensFlare *lensFlare;
};

void *func_ov036_0218051c(Field *field);
void func_ov036_02180fe4(FieldGimmickWorkBlock *block);

// The memory of the field's heap 0x89, in overlay 36's .bss (which file defines it isn't known yet)
extern u8 g_FieldHeapMemory[0x10000];

#endif // POKEBW2_FIELD_FIELD_INTERNAL_H
