#ifndef POKEBW2_FIELD_FIELD_H
#define POKEBW2_FIELD_FIELD_H

#include "types.h"
// For the files that call field_msgbg.c through this header: comm_entry_menu.c, field_subscreen.c and fieldmap.c
#include "field/field_msgbg.h"
#include "field/height_ex.h"
#include "gfl/bmpwin.h"
#include "gfl/g3d.h"
#include "gfl/heap.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "struct_decls.h"
#include "system/bmp_menulist.h"

void BeginContinuePlaceNameDisp(PlaceName *placeName, s32 zoneId);
void BeginForcePlaceNameDisp(PlaceName *placeName, s32 zoneId);
void EncEff_StartEvent(EncEff *encEff, GameEvent *event, s32 effect);
void FieldG2D_Prepare3DSurface(Field *field);
void FieldG2D_SetLCDConfig(void);
FieldActor *FieldPlayer_GetActor(FieldPlayer *player);
// GENDER_MALE or GENDER_FEMALE
u32 FieldPlayer_GetSex(FieldPlayer *player);
void FieldPlayer_GetWPos(FieldPlayer *player, VecFx32 *pos);
u16 FieldPlayer_GetFaceDir(FieldPlayer *player);
// The unit vector of the direction on the grid, and the rail position a step in the direction on rails
void FieldPlayer_GetMoveOffsetInDir(FieldPlayer *player, u16 dir, VecFx32 *vec);
// Whether the player is on a catwalk, and takes them off it
BOOL FieldPlayer_IsGridHopping(FieldPlayer *player);
void FieldPlayer_FinishGridHop(FieldPlayer *player);
void FieldPlayer_GetNogridLocationByDir(FieldPlayer *player, u16 dir, RailPosition *pos);
// The player's object code for a sex, in a form or an extra state
u16 FieldPlayer_GetObjCodeByForme(u32 sex, u32 forme);
u16 FieldPlayer_GetObjCodeByExState(u32 sex, u32 exState);
FieldMsgBG *Field_GetMsgBGSys(Field *field);
BOOL Field_IsEventRunning(Field *field);
u32 Field_GetRenderMode(Field *field);
void Field_SetRenderMode(Field *field, u32 mode);
FieldSceneArea *Field_GetSceneArea(Field *field);
void Field_SetFadeFlag(Field *field, BOOL flag);
u16 Field_GetDayPeriod(Field *field);
BOOL Field_GetSeasonBannerOverdrawFlag(Field *field);
void Field_SetEffectRunningFlag(Field *field, BOOL flag);
FaceUpWork **Field_GetNDemoDataHandle(Field *field);
void Field_SetCasteliaRush(Field *field, CasteliaRush *rush);
CasteliaRush *Field_GetCasteliaRush(Field *field);
FieldColorPostFX *Field_GetColorPostFX(Field *field);
void Field_SetPlayerPosPtr(Field *field, VecFx32 *position);
// The money window that the scripts show on the field's message BG
MoneyWin *Field_GetMoneyWin(Field *field);
void Field_SetMoneyWin(Field *field, MoneyWin *moneyWin);
// The field's color post-FX (field_light.c), which recolors textures as they load: by a luminance table, or to the
// colors of a flashback
struct FieldColorPostFX {
    const u8 *lut;
    BOOL vintage;
};

// Recolors a texture resource with the field's color post-FX
void FieldColorPostFX_Apply(FieldColorPostFX *postFx, G3DResource *texture);
fx32 Field_GetActorYOffset(Field *field);
void FieldPlayer_GetGPos(FieldPlayer *player, s16 *x, s16 *y, s16 *z);
// A number below 6 that overlay 137 reads from the game data
u32 TrainerCard_CountStars(GameData *gameData);
// The grid position in front of the player, facing dir
void GetPlayerGPosPlusDir(FieldPlayer *player, u16 dir, s16 *x, s16 *y, s16 *z);
// What a Join Avenue name entry names: the avenue, its title, and the player's favorite phrase and phrase when
// impressed, which the passerbys say
enum {
    JOIN_AVENUE_ENTRY_NAME,
    JOIN_AVENUE_ENTRY_TITLE,
    JOIN_AVENUE_ENTRY_PHRASE,
    JOIN_AVENUE_ENTRY_IMPRESSED,
};
GameEvent *JoinAvenueNameEntry_Create(u16 target, GameSystem *gsys, u16 *result, u16 useCurrent);
// Check the party and the Battle Box against a regulation. Regulation_CreatePartyCheckTask returns the event that lets
// the player choose between them, or NULL when neither can enter
GameEvent *Regulation_CreatePartyCheckTask(GameSystem *gsys, u32 pickFromParty, Regulation *regulation, u16 *result,
                                           HeapID heapId);
u32 Regulation_CheckTeams(GameSystem *gsys, u32 pickFromParty, Regulation *regulation, HeapID heapId);
// PartySelectList_Delete's result, the row the player picked
enum {
    PARTY_SELECT_PARTY,
    PARTY_SELECT_BATTLE_BOX,
    PARTY_SELECT_CANCEL,
};

// The screen that picks the party or the Battle Box for a battle, whether it is done, and the choice. partyIneligible
// and battleBoxIneligible are set for a team that fails the regulation, and PartySelectList_Delete sets *ineligible
// when the team picked is one
PartySelectList *PartySelectList_Create(Field *field, PokeParty *party, PokeParty *battleBoxParty, u8 partyIneligible,
                                        u32 battleBoxIneligible, HeapID heapId);
BOOL PartySelectList_Main(PartySelectList *select);
int PartySelectList_Delete(PartySelectList *select, u32 *ineligible);
FieldEffectTask *FldeffBtrain_Start(FieldEffects *effects, u32 trainIdx, const VecFx32 *pos);
void FldeffBtrain_SetAnime(FieldEffectTask *task, u16 anmIdx);
void FldeffBtrain_SetHidden(FieldEffectTask *task, u16 hidden);
void FieldPlayer_SetWPos(FieldPlayer *player, const VecFx32 *pos);
void FieldPlayer_SetDirection(FieldPlayer *player, u16 dir);
// field_fade.c's fade of the field's screens, by type: in or out, quickly or at the slower speed. white fades from or
// to white rather than black, and skipWait lets the field go on without waiting for the fade
enum {
    FIELD_FADE_TYPE_IN_QUICK,
    FIELD_FADE_TYPE_OUT_QUICK,
    FIELD_FADE_TYPE_IN,
    FIELD_FADE_TYPE_OUT,
};
void FieldFadeTCB_Start(GameSystem *gsys, Field *field, u32 type, u32 white, u32 skipWait);

// The work of the zone's gimmick, such as a gym's puzzle, which the gimmick's overlay allocates at the zone's start
void *Field_AllocGimmickWorkBlock(Field *field, u32 id, HeapID heapId, u32 size);
void *Field_GetGimmickWorkBlock(Field *field, u32 id);
// Whether the gimmick work was allocated with the password
BOOL Field_CheckGimmickWorkPassword(Field *field, u32 password);
void Field_DeleteGimmickWorkBlock(Field *field, u32 id);

// A list of areas of the map, which the list's entry index sets: a grid rectangle, a value and flags
HeightExList *HeightEx_CreateTable(u32 count, u8 heapId);
void HeightEx_SetArea(u8 index, u32 x, u32 z, u32 width, u32 depth, u32 value, u32 flags, HeightExList *list);
void HeightEx_FreeTable(HeightExList *list);
u32 HeightEx_GetAreaCount(HeightExList *list);
BOOL HeightEx_IsPointInAreaAt(s32 x, s32 z, HeightExList *list, u8 index);
u32 HeightEx_GetAreaHeight(u8 index, HeightExList *list);
u32 HeightEx_GetAreaAttr(u8 index, HeightExList *list);
// Sets whether the entry at index is in use, which HeightEx_SetArea checks before it fills one in
void HeightEx_SetAreaInUse(u8 index, BOOL inUse, HeightExList *list);
// Overlay 36's field_fade.c: whether a grid position is in the rectangle of an entry of the list
BOOL HeightEx_IsPointInArea(s32 x, s32 z, const HeightExEntry *entry);
// The mapper's WFBC work
FieldWFBC *FieldWFBC_Create(HeapID heapId);
void FieldWFBC_Free(FieldWFBC *wfbc);
void FieldWFBC_Setup(FieldWFBC *wfbc, CityState *city, BOOL isOther, HeapID heapId);
void FieldSubscreen_ChangeImm(FieldSubscreen *subscreen, u32 mode);
BOOL FieldTaskManager_IsIdle(FieldTaskManager *taskManager);
u32 FieldTaskManager_GetActiveTaskCount(FieldTaskManager *taskManager);
MMSys *Field_GetActorSystem(Field *field);
FieldCamera *Field_GetCameraSystem(Field *field);
NoGridMapper *Field_GetNoGridMapper(Field *field);
FieldExpObjSystem *Field_GetExpObjSystem(Field *field);
// Whether a fade that FieldFadeTCB_Start started is still running
BOOL Field_GetFadeFlag(Field *field);
FieldFog *Field_GetFog(Field *field);
FieldG3DMapper *Field_GetG3DMapper(Field *field);
GameSystem *Field_GetGameSystem(Field *field);
AreaData *Field_GetAreaData(Field *field);
TCBManager *Field_GetTCBMgr(Field *field);
EncEff *Field_GetEncEff(Field *field);
EncountSystem *Field_GetEncountSystem(Field *field);
u16 Field_GetHeapID(Field *field);
FieldLensFlare *Field_GetLensFlare(Field *field);
PlaceName *Field_GetPlaceName(Field *field);
FieldPlayer *Field_GetPlayer(Field *field);
u16 Field_GetPlayerStateZoneID(Field *field);
u32 Field_GetResolvedControllerTypeID(Field *field);
FieldSubscreen *Field_GetSubscreen(Field *field);
FieldTaskManager *Field_GetTaskManager(Field *field);
void Field_SetSeasonBannerOverdrawFlag(Field *field, BOOL flag);
u32 GetZoneFogIndexAll(Field *field, u16 zoneId);
void ShutdownFollowWork(GameData *gameData);
void EvTime_Update(GameData *gameData);
void FestMission_OnZoneEnter(GameSystem *gsys, u16 zoneId);
void FieldPlayer_GetNogridNextRailLocation(FieldPlayer *player, RailPosition *pos);
void EffectEncount_SetEffectPaused(EncountSystem *encount, u32 paused);

// One of N's Pokémon, which createNPkm makes
typedef struct {
    // The zones it appears in, from firstZone on
    u16 firstZone : 14;
    u16 zoneCount : 2;
    u16 species;
    u8 level;
    u8 nature;
    u8 sex;
    // 0 or 1 for the species' abilities, 2 for its hidden ability
    u8 ability;
} NPokeSpec;

void createNPkm(PartyPkm *pkm, const NPokeSpec *spec);

void Field_RequestClose(Field *field);
BOOL Field_CheckMapLoadFinished(Field *field);
void FieldRenderPhase2_FieldEffect(Field *field);
void FieldRenderPhase2_EncountEffect(Field *field);
FieldDispControl *FieldG2D_GetDispControl(Field *field);
FieldEffects *Field_GetFieldEffects(Field *field);
FieldG3DObjSystem *Field_GetG3DObjSys(Field *field);
// No fog, which GetZoneFogIndex and GetZoneFogIndexAll return for a zone without one
#define ZONE_FOG_NONE 0x0fffffff

u32 GetZoneFogIndex(u16 zoneId);
u32 ZoneData_GetObjectProjectionMatrixType(u16 zoneId);
u32 GetObjectProjectionMatrixOffset(u16 zoneId);
BOOL IsZoneShadowsEnabled(u16 zoneId);
BOOL IsZoneTwoPassLoad(u16 zoneId);
BOOL IsZoneFirstBlackTowerOrWhiteTreehollow(u16 zoneId);
u32 GetZoneMapType(u16 zoneId);
u32 GetZoneMapType2(u16 zoneId);
void SetupLoadZoneMapTypeData(u16 zoneId, AreaData *area, FieldG3DMapperConfig *config, MapMatrix *matrix);
const FieldmapCtrlVTable *GetZoneFieldmapCtrlVTable(u16 zoneId);
u32 GetFieldmapZoneHeapSize(u16 zoneId);
PlaceName *FieldPlaceName_Create(GameSystem *gsys, HeapID heapId, FieldMsgBG *msgBGSys);
void FieldPlaceName_Free(PlaceName *placeName);
void PlaceName_Main(PlaceName *placeName);
void PlaceName_Draw(PlaceName *placeName);
void Field_InitGimmick(Field *field);
void Field_TerminateGimmick(Field *field);
void Field_UpdateGimmick(Field *field);
BOOL Field_CheckGimmickID(Field *field, u32 id);
// Gimmick IDs that Field_CheckGimmickID checks for
#define GIMMICK_MISTRALTON_GYM 5
#define GIMMICK_CASTELIA_GYM 6
#define GIMMICK_DRIFTVEIL_GYM 7
#define GIMMICK_CHARGESTONE_CAVE 13
#define GIMMICK_HUMILAU_GYM 38
#define GIMMICK_ROLLER_COASTER 48
void PlaceName_RestoreWindow(PlaceName *placeName);
BOOL FieldmapProc_Init(GameProc *proc, u32 *seq, void *param, void *work);
BOOL FieldmapProc_Update(GameProc *proc, u32 *seq, void *param, void *work);
BOOL FieldmapProc_End(GameProc *proc, u32 *seq, void *param, void *work);
Field *Field_Create(GameSystem *gsys, HeapID heapId);
void Field_Free(Field *field);
void Field_RenderStart(Field *field);
BOOL Field_CallRoutines(GameSystem *gsys, Field *field);
void FieldG2D_Init(Field *field);
void FieldG2D_Prepare3DSurfaceDelayed(Field *field);
void Field_FreeGraphicsSystems(Field *field);
void FieldG3D_InitCallback(void);
void FieldG3D_Init(Field *field);
void FieldG3D_Update(Field *field);
void FieldG3D_RenderPhase1(Field *field);
void FieldG3D_RenderPhase2(Field *field);
void FieldG3D_Free(Field *field);
void FldActSys_AsyncMatLoadTCBFunc(TCB *tcb, void *data);
void FldActSys_VRAMUploadFunc(BOOL type, u32 dest, void *src, u32 size);
void Field_LoadEdgeColorTable(AreaData *area, u16 zoneId);
void Field_LoadActorMatColorPreset(Field *field);
void Field_InitActorSystem(Field *field);
void Field_SuspendActorSystem(Field *field);
BOOL Field_CheckDoRealTimeLoad(Field *field);
BOOL Field_UpdateZoneStatePos(Field *field);
BOOL Field_ShouldSwapZone(Field *field);
void Field_SwapZoneByMatrix(Field *field);
void Field_HotswapSpawnActors(Field *field, GameData *gameData, MMSys *actorSystem, EventData *eventData, u32 zoneId);
void Field_SwapZoneBGM(Field *field, u32 zoneId);
void Field_SwapWeather(Field *field, u32 zoneId);
void Field_CheckGiveDiamondDustMedal(Field *field);
void Field_SwapFog(Field *field, u32 zoneId);
void Field_UpdatePlayerStateZoneID(GameData *gameData, u32 zoneId);
void Field_SwapCameraBoundaries(Field *field, u32 zoneId);
void Field_ResetController(Field *field);
void FieldCameraBoundary_ChangeZone(Field *field, u32 zoneId, HeapID heapId);
void Field_LoadWFBC(GameData *gameData, Field *field, u32 zoneId);
void Field_LoadJoinAvenue(GameData *gameData, Field *field, u32 zoneId);
void Field_LoadSceneArea(Field *field, u32 zoneId);
u32 Field_GetCommKind(GameCommSys *comm);
BOOL Field_CheckCommExit(GameCommSys *comm);

void FieldRenderPhase1_Fieldmap(Field *field);
void FieldRenderPhase2_Fieldmap(Field *field);
fx32 Field_CalcActorYOffset(Field *field);
void FieldColorPostFX_Set(FieldColorPostFX *postFx, const u8 *luminanceTable, BOOL flashback);

#endif // POKEBW2_FIELD_FIELD_H
