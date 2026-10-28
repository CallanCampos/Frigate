// The encounter effects: EncEff_StartEvent starts an effect's event from enceff_data.c's table after loading the
// overlays it needs, and the BGM and effect that a battle picks. The name is the ROM's own, from GFL_HeapAllocate's
// file argument. Function and table names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names are our descriptive guesses.
#include "types.h"
#include "battle/trainer_data.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/zones.h"
#include "field/encounter.h"
#include "field/encounter_effect.h"
#include "field/event_wild_battle.h"
#include "field/field.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_script.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/heapsys.h"
#include "gfl/overlay.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/player_info.h"
#include "system/area_data.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

#define ENCEFF_OVERLAYS 3
#define MAX_TRAINER_IDS 50

struct EncEff {
    void *eventData;
    void (*render)(EncEff *effect);
    Field *field;
    u32 overlays[ENCEFF_OVERLAYS];
    s32 overlayCount;
    void *workArea;
};

// The trainer IDs of the Pokémon that did not come from the player, without repeats
typedef struct {
    s32 count;
    u32 ids[MAX_TRAINER_IDS];
} TrainerIdList;

typedef struct {
    u32 playerId;
    s32 box;
    u16 *result;
    TrainerIdList ids;
} CountTrainersEvent;

enum {
    COUNT_TRAINERS_STATE_PARTY,
    COUNT_TRAINERS_STATE_BOXES,
    COUNT_TRAINERS_STATE_BATTLE_BOX,
};

static void EncEff_FreeWorkArea(EncEff *encEff);
static void EncEff_LoadSubOvl(EncEff *encEff, u32 overlayId);
static void EncEff_UnloadAllSubOverlays(EncEff *encEff);
static GameEventReturnCode CountTrainersEvent_Callback(GameEvent *event, u32 *state, void *data);
static int CountTrainersEvent_ScanParty(u32 playerId, GameData *gameData, TrainerIdList *ids);
static int CountTrainersEvent_ScanBox(u32 playerId, GameData *gameData, u32 box, TrainerIdList *ids);
static int CountTrainersEvent_ScanBattleBox(u32 playerId, GameData *gameData, u32 box, TrainerIdList *ids);
static BOOL TrainerIdList_AddForeignId(u32 playerId, u32 id, TrainerIdList *ids);
static BOOL TrainerIdList_AddUniqueId(u32 id, TrainerIdList *ids);

static const u16 SPECIAL_TRAINER_BGM[14] = { SEQ_BGM_VS_GYMLEADER, SEQ_BGM_VS_SHITENNO,     SEQ_BGM_VS_SHIRONA,
                                             SEQ_BGM_VS_HUE,       SEQ_BGM_VS_TRAINER,      SEQ_BGM_VS_ACHROMA,
                                             SEQ_BGM_VS_SWAN_N,    SEQ_BGM_VS_NEO_PLASMA,   SEQ_BGM_VS_IRIS,
                                             SEQ_BGM_VS_NEW_G_CIS, SEQ_BGM_VS_ELITE_PLASMA, SEQ_BGM_VS_BANJIROU,
                                             SEQ_BGM_VS_ACHROMA,   SEQ_BGM_VS_RIVAL };

static const u8 SPECIAL_TRAINER_ENCEFF[32] = { 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                               0x16, 0x05, 0x0a, 0x05, 0x18, 0x19, 0x1a, 0x1a, 0x1a, 0x1a, 0x17,
                                               0x1b, 0x1e, 0x1f, 0x05, 0x05, 0x09, 0x09, 0x00, 0x00, 0x00 };

EncEff *EncEff_Create(HeapID heapId, Field *field) {
    EncEff *encEff = GFL_HeapAllocate(heapId, sizeof(EncEff), TRUE, "enceff.c", 110);

    encEff->field = field;
    encEff->overlayCount = 0;
    return encEff;
}

void EncEff_Free(EncEff *encEff) {
    EncEff_FreeWorkArea(encEff);
    EncEff_UnloadAllSubOverlays(encEff);
    GFL_HeapFree(encEff);
}

void EncEff_StartEvent(EncEff *encEff, GameEvent *event, s32 effect) {
    GameEvent *effectEvent;
    GameSystem *gsys;
    Field *field = encEff->field;

    gsys = Field_GetGameSystem(field);

    if (effect >= ENCEFF_COUNT) {
        effect = 0;
    }
    if (GFL_HeapGetHighestAllocatableSize(HEAPID_FIELDMAP) < ENC_EFFECT_CONTROLLERS[effect].heapSize) {
        effect = 7;
    }
    if (ENC_EFFECT_CONTROLLERS[effect].overlay != -1) {
        EncEff_LoadSubOvl(encEff, ENC_EFFECT_CONTROLLERS[effect].overlay);
    }
    effectEvent = ENC_EFFECT_CONTROLLERS[effect].create(gsys, field, ENC_EFFECT_CONTROLLERS[effect].param);
    GameEvent_ChainNext(event, effectEvent);
    encEff->eventData = GameEvent_GetData(effectEvent);
    encEff->render = ENC_EFFECT_CONTROLLERS[effect].render;
}

void *EncEff_GetEventData(EncEff *encEff) {
    return encEff->eventData;
}

void EncEff_CallRenderFunc(EncEff *encEff) {
    if (encEff->render != NULL) {
        encEff->render(encEff);
    }
}

void *EncEff_AllocWorkArea(EncEff *encEff, u32 size, HeapID heapId) {
    encEff->workArea = GFL_HeapAllocate(heapId, size, TRUE, "enceff.c", 229);
    return encEff->workArea;
}

static void EncEff_FreeWorkArea(EncEff *encEff) {
    if (encEff->workArea != NULL) {
        GFL_HeapFree(encEff->workArea);
        encEff->workArea = NULL;
    }
}

void *EncEff_GetWorkArea(EncEff *encEff) {
    return encEff->workArea;
}

static void EncEff_LoadSubOvl(EncEff *encEff, u32 overlayId) {
    GFL_OvlLoad(overlayId);
    encEff->overlays[encEff->overlayCount++] = overlayId;
}

void EncEff_LoadCaptureGridSubOvls(EncEff *encEff) {
    EncEff_LoadSubOvl(encEff, OVERLAY_ID(148));
    EncEff_LoadSubOvl(encEff, OVERLAY_ID(150));
}

void EncEff_LoadCaptureSubOvl(EncEff *encEff) {
    EncEff_LoadSubOvl(encEff, OVERLAY_ID(148));
}

static void EncEff_UnloadAllSubOverlays(EncEff *encEff) {
    s32 i = encEff->overlayCount;

    if (i > 0) {
        while (i > 0) {
            s32 idx = i - 1;

            if (encEff->overlays[idx] != 0) {
                GFL_OvlUnload(encEff->overlays[idx]);
            }
            i = --encEff->overlayCount;
        }
    }
}

void EventBattleCall_DecideEnvWild(s32 species, s32 form, s32 encountMode, BOOL isDouble, Field *field, u32 *encEffect,
                                   u16 *bgm) {
    u16 zoneId = Field_GetPlayerStateZoneID(field);
    u16 parentZone = GetZoneParentZone(zoneId);
    u32 tileType;
    u32 tileClass;
    u32 env;
    u16 music;

    switch (species) {
    case SPECIES_ZOROARK:
        *bgm = SEQ_BGM_VS_TSUYOPOKE;
        *encEffect = 0x24;
        return;
    case SPECIES_RESHIRAM:
        *bgm = SEQ_BGM_VS_SHIN;
        *encEffect = 0x21;
        return;
    case SPECIES_ZEKROM:
        *bgm = SEQ_BGM_VS_MU;
        *encEffect = 0x21;
        return;
    case SPECIES_KYUREM:
        if (form == 1) {
            *bgm = SEQ_BGM_VS_KYURAMU;
        } else if (form == 2) {
            *bgm = SEQ_BGM_VS_KYUROMU;
        } else {
            *bgm = SEQ_BGM_VS_RAI;
        }
        *encEffect = 0x21;
        return;
    case SPECIES_VOLCARONA:
        *bgm = SEQ_BGM_VS_TSUYOPOKE;
        *encEffect = 0x20;
        return;
    case SPECIES_VICTINI:
        *bgm = SEQ_BGM_VS_SETPOKE;
        *encEffect = 0x20;
        return;
    case SPECIES_COBALION:
    case SPECIES_TERRAKION:
    case SPECIES_VIRIZION:
    case SPECIES_KELDEO:
        *bgm = SEQ_BGM_VS_SETPOKE;
        *encEffect = 0x23;
        return;
    case SPECIES_TORNADUS:
    case SPECIES_THUNDURUS:
    case SPECIES_LANDORUS:
        *bgm = SEQ_BGM_VS_MOVEPOKE;
        *encEffect = 0x22;
        return;
    case SPECIES_LATIAS:
    case SPECIES_LATIOS:
    case SPECIES_CRESSELIA:
    case SPECIES_HAXORUS:
        *bgm = SEQ_BGM_VS_TSUYOPOKE;
        *encEffect = 0x20;
        return;
    case SPECIES_REGIROCK:
    case SPECIES_REGICE:
    case SPECIES_REGISTEEL:
    case SPECIES_REGIGIGAS:
        *bgm = SEQ_BGM_VS_REGI;
        *encEffect = 0x20;
        return;
    case SPECIES_UXIE:
    case SPECIES_MESPRIT:
    case SPECIES_AZELF:
        *bgm = SEQ_BGM_VS_UMA;
        *encEffect = 0x20;
        return;
    case SPECIES_HEATRAN:
        *bgm = SEQ_BGM_VS_DPLEGEND;
        *encEffect = 0x20;
        return;
    }

    if (encountMode == ENCOUNT_MODE_EFFECT) {
        *bgm = SEQ_BGM_VS_TSUYOPOKE;
        *encEffect = 0x20;
        return;
    }
    if (encountMode == ENCOUNT_MODE_FISHING) {
        if (parentZone == ZONE_VICTORY_ROAD_2) {
            *bgm = SEQ_BGM_VS_TSUYOPOKE;
        } else {
            *bgm = SEQ_BGM_VS_NORAPOKE;
        }
        *encEffect = 2;
        return;
    }
    tileType = FieldPlayer_GetTileTypeUnder(Field_GetPlayer(field));
    tileClass = GetTileClass(tileType);
    if (GetTileFlags(tileType) & TILE_FLAG_WATER) {
        music = SEQ_BGM_VS_NORAPOKE;
        env = 2;
    } else if (MapTile_IsTallGrass(tileClass)) {
        if (isDouble) {
            music = SEQ_BGM_VS_TSUYOPOKE;
            env = 1;
        } else {
            music = SEQ_BGM_VS_NORAPOKE;
            env = 0;
        }
    } else if (MapTile_IsSwamp(tileClass)) {
        music = SEQ_BGM_VS_NORAPOKE;
        env = 0;
    } else if (IsTileSand(tileClass)) {
        music = SEQ_BGM_VS_NORAPOKE;
        env = 4;
    } else if (GetZoneIsHiddenGroto(zoneId)) {
        music = SEQ_BGM_VS_NORAPOKE;
        env = 0;
    } else {
        music = SEQ_BGM_VS_NORAPOKE;
        env = 3;
    }
    *encEffect = env;
    if (parentZone == ZONE_VICTORY_ROAD_2) {
        *bgm = SEQ_BGM_VS_TSUYOPOKE;
    } else {
        *bgm = music;
    }
}

void EventBattleCall_DecideEnvTrainer(u32 trainerId, Field *field, u32 *encEffect, u16 *bgm) {
    u32 trainerClass = TrainerData_GetParam(trainerId, TRAINER_PARAM_CLASS);
    u32 bgmGroup = GetTrainerClassBGMGroupId(trainerClass);
    u8 special = GetTrainerClassSpecialId(trainerClass);
    u32 tileType;
    u32 tileClass;
    u32 env;
    BOOL exterior = TRUE;

    if (bgmGroup < NELEMS(SPECIAL_TRAINER_BGM)) {
        *bgm = SPECIAL_TRAINER_BGM[bgmGroup];
        *encEffect = SPECIAL_TRAINER_ENCEFF[special];
        return;
    }
    tileType = FieldPlayer_GetTileTypeUnder(Field_GetPlayer(field));
    tileClass = GetTileClass(tileType);
    if (GetTileFlags(tileType) & TILE_FLAG_WATER) {
        env = 6;
    } else if (IsTileSand(tileClass)) {
        env = 8;
    } else {
        if (!AreaData_IsExterior(Field_GetAreaData(field))) {
            exterior = FALSE;
        }
        env = 5;
        if (!exterior) {
            env = 7;
        }
    }
    *bgm = SEQ_BGM_VS_TRAINER;
    *encEffect = env;
}

BOOL Scrcmd_Enceff_StartCountOtherOtIdsTask(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 *result = ScriptReadVar(vm, env);
    GameEvent *event = GameEvent_Create(gsys, NULL, CountTrainersEvent_Callback, sizeof(CountTrainersEvent));
    PlayerInfo *player = GetGameDataPlayerInfo(gameData);
    CountTrainersEvent *data = GameEvent_GetData(event);

    data->playerId = getIDAsUInt(player);
    data->ids.count = 0;
    data->box = 0;
    data->result = result;
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static GameEventReturnCode CountTrainersEvent_Callback(GameEvent *event, u32 *state, void *data) {
    CountTrainersEvent *count = data;
    BOOL done = FALSE;
    GameData *gameData;
    int i;

    gameData = GSYS_GetGameData(GameEvent_GetGameSystem(event));
    switch (*state) {
    case COUNT_TRAINERS_STATE_PARTY:
        CountTrainersEvent_ScanParty(count->playerId, gameData, &count->ids);
        (*state)++;
        break;
    case COUNT_TRAINERS_STATE_BOXES: {
        s32 box = count->box;

        for (i = 0; i < 2; i++) {
            CountTrainersEvent_ScanBox(count->playerId, gameData, box + i, &count->ids);
            count->box++;
            if (count->box >= BOX_MAX_TRAY) {
                (*state)++;
                break;
            }
        }
        break;
    }
    case COUNT_TRAINERS_STATE_BATTLE_BOX:
        CountTrainersEvent_ScanBattleBox(count->playerId, gameData, 0, &count->ids);
        done = TRUE;
        break;
    }
    if (count->ids.count >= MAX_TRAINER_IDS) {
        done = TRUE;
    }
    if (done) {
        *count->result = count->ids.count;
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static int CountTrainersEvent_ScanParty(u32 playerId, GameData *gameData, TrainerIdList *ids) {
    PokeParty *party = GameData_GetParty(gameData);
    int partyCount = PokeParty_GetPkmCount(party);
    int added = 0;
    int i;

    for (i = 0; i < partyCount; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
            if (TrainerIdList_AddForeignId(playerId, PokeParty_GetParam(pkm, PKM_PARAM_ID, NULL), ids)) {
                added++;
            }
        }
    }
    return added;
}

static int CountTrainersEvent_ScanBox(u32 playerId, GameData *gameData, u32 box, TrainerIdList *ids) {
    BoxSaveAccessor *boxes = GameData_GetBoxSaveAccessor(gameData);
    int added = 0;
    int slot;

    for (slot = 0; slot < 30; slot++) {
        if (BoxSaveAccessor_GetPkmParam(boxes, box, slot, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            if (BoxSaveAccessor_GetPkmParam(boxes, box, slot, PKM_PARAM_IS_EGG, NULL) == 0) {
                if (TrainerIdList_AddForeignId(
                        playerId, BoxSaveAccessor_GetPkmParam(boxes, box, slot, PKM_PARAM_ID, NULL), ids)) {
                    added++;
                }
            }
        }
    }
    return added;
}

static int CountTrainersEvent_ScanBattleBox(u32 playerId, GameData *gameData, u32 box, TrainerIdList *ids) {
    BattleBoxSave *battleBox;
    int slot;
    int added;

    battleBox = getBattleBox(GameData_GetSaveControl(gameData));
    added = 0;
    for (slot = 0; slot < 6; slot++) {
        BoxPkm *pkm = getBoxSlotAddress(battleBox, box, slot);

        if (PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            if (TrainerIdList_AddForeignId(playerId, PML_PkmGetParam(pkm, PKM_PARAM_ID, NULL), ids)) {
                added++;
            }
        }
    }
    return added;
}

static BOOL TrainerIdList_AddForeignId(u32 playerId, u32 id, TrainerIdList *ids) {
    BOOL added = FALSE;

    if (playerId != id) {
        added = TrainerIdList_AddUniqueId(id, ids);
    }
    return added;
}

static BOOL TrainerIdList_AddUniqueId(u32 id, TrainerIdList *ids) {
    int i;

    if (ids->count >= MAX_TRAINER_IDS) {
        return FALSE;
    }
    for (i = 0; i < ids->count; i++) {
        if (id == ids->ids[i]) {
            return FALSE;
        }
    }
    ids->ids[i] = id;
    ids->count++;
    return TRUE;
}
