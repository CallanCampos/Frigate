// The encounter effects: the table of every effect's event, which EncEff_StartEvent starts after loading the overlays
// it needs, and the BGM and effect that a battle picks. The name is the ROM's own, from GFL_HeapAllocate's file
// argument. Function and table names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "battle/trainer_data.h"
#include "constants/pokemon.h"
#include "field/encounter.h"
#include "field/encounter_effect.h"
#include "field/encounter_effect_model.h"
#include "field/encounter_effect_ripple.h"
#include "field/event_encounter_cutin.h"
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
#define ENCEFF_COUNT 38
#define MAX_TRAINER_IDS 50
#define BOXES 24

struct EncEff {
    void *eventData;
    void (*render)(EncEff *effect);
    Field *field;
    u32 overlays[ENCEFF_OVERLAYS];
    s32 overlayCount;
    void *workArea;
};

typedef GameEvent *(*EncEffCreateFunc)(GameSystem *gsys, Field *field, u32 param);

// What it takes to run an effect
typedef struct {
    EncEffCreateFunc create;
    void (*render)(EncEff *effect);
    // The overlay with the effect's code, or -1 for none
    s32 overlay;
    u32 param;
    // The smallest part of the heap that is free for the effect to run
    u32 heapSize;
} EncEffInfo;

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

static void EncEff_FreeWorkArea(EncEff *encEff);
static void EncEff_LoadSubOvl(EncEff *encEff, u32 overlayId);
static void EncEff_UnloadAllSubOverlays(EncEff *encEff);
static GameEventReturnCode func_ov036_021c6288(GameEvent *event, u32 *state, void *data);
static int func_ov036_021c6328(u32 playerId, GameData *gameData, TrainerIdList *ids);
static int func_ov036_021c6384(u32 playerId, GameData *gameData, u32 box, TrainerIdList *ids);
static int func_ov036_021c63f0(u32 playerId, GameData *gameData, u32 box, TrainerIdList *ids);
static BOOL func_ov036_021c6448(u32 playerId, u32 id, TrainerIdList *ids);
static BOOL func_ov036_021c6460(u32 id, TrainerIdList *ids);

static const u16 SPECIAL_TRAINER_BGM[14] = { 0x46c, 0x46f, 0x479, 0x4ee, 0x46a, 0x4ea, 0x471,
                                             0x4e9, 0x4ec, 0x4eb, 0x4f3, 0x4ed, 0x4ea, 0x46d };

static const u8 SPECIAL_TRAINER_ENCEFF[32] = { 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
                                               0x16, 0x05, 0x0a, 0x05, 0x18, 0x19, 0x1a, 0x1a, 0x1a, 0x1a, 0x17,
                                               0x1b, 0x1e, 0x1f, 0x05, 0x05, 0x09, 0x09, 0x00, 0x00, 0x00 };

static const EncEffInfo ENC_EFFECT_CONTROLLERS[ENCEFF_COUNT] = {
    /* 0 */ { (EncEffCreateFunc)EventEncountEffectCutin_Create01, EncEffModel_Render, 149, 1, 0x13010 },
    /* 1 */ { (EncEffCreateFunc)EncEffModel_CreateModel9, EncEffModel_Render, 149, 1, 0x131a4 },
    /* 2 */ { (EncEffCreateFunc)EncEffRipple_CreateWhite, EncEffRipple_ClearAndDraw, 151, 1, 0x0 },
    /* 3 */ { EventEncountEffectCutin_Create25, NULL, 146, 0, 0x1e04 },
    /* 4 */ { EventEncountEffectCutin_Create26, NULL, 146, 1, 0x14bc8 },
    /* 5 */ { (EncEffCreateFunc)EncEffModel_CreateModel5, EncEffModel_Render, 149, 1, 0x12fbc },
    /* 6 */ { EventEncountEffectCutin_Create05, EncEffModel_Render, 149, 1, 0x13b20 },
    /* 7 */ { (EncEffCreateFunc)EncEffCells_Create, EncEffGrid_ClearAndDraw, 152, 0, 0x0 },
    /* 8 */ { (EncEffCreateFunc)EncEffCells_Create, EncEffGrid_ClearAndDraw, 154, 1, 0x0 },
    /* 9 */ { (EncEffCreateFunc)EncEffCells_Create, EncEffGrid_ClearAndDraw, 153, 1, 0x0 },
    /* 10 */ { EventEncountEffectCutin_Create00, NULL, 146, 0, 0xe570 },
    /* 11 */ { EventEncountEffectCutin_Create01, NULL, 146, 0, 0xe570 },
    /* 12 */ { EventEncountEffectCutin_Create02, NULL, 146, 0, 0xe570 },
    /* 13 */ { EventEncountEffectCutin_Create03, NULL, 146, 0, 0xe570 },
    /* 14 */ { EventEncountEffectCutin_Create04, NULL, 146, 0, 0xe570 },
    /* 15 */ { EventEncountEffectCutin_Create05, NULL, 146, 0, 0xe570 },
    /* 16 */ { EventEncountEffectCutin_Create06, NULL, 146, 0, 0xe570 },
    /* 17 */ { EventEncountEffectCutin_Create07, NULL, 146, 0, 0xe570 },
    /* 18 */ { EventEncountEffectCutin_Create08, NULL, 146, 0, 0xe570 },
    /* 19 */ { EventEncountEffectCutin_Create09, NULL, 146, 0, 0x12a68 },
    /* 20 */ { EventEncountEffectCutin_Create10, NULL, 146, 0, 0x12a68 },
    /* 21 */ { EventEncountEffectCutin_Create11, NULL, 146, 0, 0x12a68 },
    /* 22 */ { EventEncountEffectCutin_Create12, NULL, 146, 0, 0x12a68 },
    /* 23 */ { EventEncountEffectCutin_Create13, NULL, 146, 0, 0x12a68 },
    /* 24 */ { EventEncountEffectCutin_Create15, NULL, 146, 0, 0xe570 },
    /* 25 */ { EventEncountEffectCutin_Create14, NULL, 146, 0, 0xe570 },
    /* 26 */ { EventEncountEffectCutin_Create16, NULL, 146, 0, 0xcf20 },
    /* 27 */ { EventEncountEffectCutin_Create17, NULL, 146, 0, 0xd530 },
    /* 28 */ { EventEncountEffectCutin_Create18, NULL, 146, 0, 0xcf20 },
    /* 29 */ { EventEncountEffectCutin_Create19, NULL, 146, 0, 0xcf20 },
    /* 30 */ { EventEncountEffectCutin_Create20, NULL, 146, 0, 0xe570 },
    /* 31 */ { EventEncountEffectCutin_Create21, NULL, 146, 0, 0xe570 },
    /* 32 */ { (EncEffCreateFunc)EncEffModel_CreateModel1, EncEffModel_Render, 149, 1, 0x132e8 },
    /* 33 */ { EventEncountEffectCutin_Create22, NULL, 146, 1, 0x80e0 },
    /* 34 */ { EventEncountEffectCutin_Create23, NULL, 146, 1, 0x619c },
    /* 35 */ { EventEncountEffectCutin_Create24, NULL, 146, 1, 0xac88 },
    /* 36 */ { (EncEffCreateFunc)EncEffModel_CreateZoroark, EncEffModel_Render, 149, 1, 0x12dc0 },
    /* 37 */ { (EncEffCreateFunc)EncEffRipple_CreateBlack, EncEffRipple_ClearAndDraw, 151, 0, 0x0 },
};

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

void func_ov036_021c5ea0(EncEff *encEff) {
    EncEff_LoadSubOvl(encEff, OVERLAY_ID(148));
    EncEff_LoadSubOvl(encEff, OVERLAY_ID(150));
}

void func_ov036_021c5ebc(EncEff *encEff) {
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

void EventBattleCall_DecideEnvWild(s32 species, s32 form, s32 a2, BOOL a3, Field *field, u32 *encEffect, u16 *bgm) {
    u16 zoneId = Field_GetPlayerStateZoneID(field);
    u16 parentZone = GetZoneParentZone(zoneId);
    u32 tileType;
    u32 tileClass;
    u32 env;
    u16 music;

    switch (species) {
    case 0x23b:
        *bgm = 0x469;
        *encEffect = 0x24;
        return;
    case 0x283:
        *bgm = 0x474;
        *encEffect = 0x21;
        return;
    case 0x284:
        *bgm = 0x475;
        *encEffect = 0x21;
        return;
    case 0x286:
        if (form == 1) {
            *bgm = 0x4e7;
        } else if (form == 2) {
            *bgm = 0x4e8;
        } else {
            *bgm = 0x476;
        }
        *encEffect = 0x21;
        return;
    case 0x27d:
        *bgm = 0x469;
        *encEffect = 0x20;
        return;
    case 0x1ee:
        *bgm = 0x478;
        *encEffect = 0x20;
        return;
    case 0x27e:
    case 0x27f:
    case 0x280:
    case 0x287:
        *bgm = 0x478;
        *encEffect = 0x23;
        return;
    case 0x281:
    case 0x282:
    case 0x285:
        *bgm = 0x477;
        *encEffect = 0x22;
        return;
    case 0x17c:
    case 0x17d:
    case 0x1e8:
    case 0x264:
        *bgm = 0x469;
        *encEffect = 0x20;
        return;
    case 0x179:
    case 0x17a:
    case 0x17b:
    case 0x1e6:
        *bgm = 0x473;
        *encEffect = 0x20;
        return;
    case 0x1e0:
    case 0x1e1:
    case 0x1e2:
        *bgm = 0x472;
        *encEffect = 0x20;
        return;
    case 0x1e5:
        *bgm = 0x46e;
        *encEffect = 0x20;
        return;
    }

    if (a2 == 1) {
        *bgm = 0x469;
        *encEffect = 0x20;
        return;
    }
    if (a2 == 4) {
        if (parentZone == 0xd6) {
            *bgm = 0x469;
        } else {
            *bgm = 0x468;
        }
        *encEffect = 2;
        return;
    }
    tileType = FieldPlayer_GetTileTypeUnder(Field_GetPlayer(field));
    tileClass = GetTileClass(tileType);
    if (GetTileFlags(tileType) & 2) {
        music = 0x468;
        env = 2;
    } else if (MapTile_IsTallGrass(tileClass)) {
        if (a3) {
            music = 0x469;
            env = 1;
        } else {
            music = 0x468;
            env = 0;
        }
    } else if (MapTile_IsSwamp(tileClass)) {
        music = 0x468;
        env = 0;
    } else if (IsTileSand(tileClass)) {
        music = 0x468;
        env = 4;
    } else if (GetZoneIsHiddenGroto(zoneId)) {
        music = 0x468;
        env = 0;
    } else {
        music = 0x468;
        env = 3;
    }
    *encEffect = env;
    if (parentZone == 0xd6) {
        *bgm = 0x469;
    } else {
        *bgm = music;
    }
}

void EventBattleCall_DecideEnvTrainer(u32 trainerId, Field *field, u32 *encEffect, u16 *bgm) {
    u32 trainerClass = TrainerData_GetParam(trainerId, 1);
    u32 bgmGroup = GetTrainerClassBGMGroupId(trainerClass);
    u8 special = GetTrainerClassSpecialId(trainerClass);
    u32 tileType;
    u32 tileClass;
    u32 env;
    BOOL exterior = TRUE;

    if (bgmGroup < 14) {
        *bgm = SPECIAL_TRAINER_BGM[bgmGroup];
        *encEffect = SPECIAL_TRAINER_ENCEFF[special];
        return;
    }
    tileType = FieldPlayer_GetTileTypeUnder(Field_GetPlayer(field));
    tileClass = GetTileClass(tileType);
    if (GetTileFlags(tileType) & 2) {
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
    *bgm = 0x46a;
    *encEffect = env;
}

BOOL func_ov036_021c6220(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = FieldScriptEnv_GetGameData(env);
    u16 *result = ScriptReadVar(vm, env);
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021c6288, sizeof(CountTrainersEvent));
    PlayerInfo *player = GetGameDataPlayerInfo(gameData);
    CountTrainersEvent *data = GameEvent_GetData(event);

    data->playerId = getIDAsUInt(player);
    data->ids.count = 0;
    data->box = 0;
    data->result = result;
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static GameEventReturnCode func_ov036_021c6288(GameEvent *event, u32 *state, void *data) {
    CountTrainersEvent *count = data;
    BOOL done = FALSE;
    GameData *gameData;
    int i;

    gameData = GSYS_GetGameData(GameEvent_GetGameSystem(event));
    switch (*state) {
    case 0:
        func_ov036_021c6328(count->playerId, gameData, &count->ids);
        (*state)++;
        break;
    case 1: {
        s32 box = count->box;

        for (i = 0; i < 2; i++) {
            func_ov036_021c6384(count->playerId, gameData, box + i, &count->ids);
            count->box++;
            if (count->box >= BOXES) {
                (*state)++;
                break;
            }
        }
        break;
    }
    case 2:
        func_ov036_021c63f0(count->playerId, gameData, 0, &count->ids);
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

static int func_ov036_021c6328(u32 playerId, GameData *gameData, TrainerIdList *ids) {
    PokeParty *party = GameData_GetParty(gameData);
    int partyCount = PokeParty_GetPkmCount(party);
    int added = 0;
    int i;

    for (i = 0; i < partyCount; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) == 0) {
            if (func_ov036_021c6448(playerId, PokeParty_GetParam(pkm, PKM_PARAM_ID, NULL), ids)) {
                added++;
            }
        }
    }
    return added;
}

static int func_ov036_021c6384(u32 playerId, GameData *gameData, u32 box, TrainerIdList *ids) {
    BoxSaveAccessor *boxes = GameData_GetBoxSaveAccessor(gameData);
    int added = 0;
    int slot;

    for (slot = 0; slot < 30; slot++) {
        if (BoxSaveAccessor_GetPkmParam(boxes, box, slot, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            if (BoxSaveAccessor_GetPkmParam(boxes, box, slot, PKM_PARAM_IS_EGG, NULL) == 0) {
                if (func_ov036_021c6448(playerId, BoxSaveAccessor_GetPkmParam(boxes, box, slot, PKM_PARAM_ID, NULL),
                                        ids)) {
                    added++;
                }
            }
        }
    }
    return added;
}

static int func_ov036_021c63f0(u32 playerId, GameData *gameData, u32 box, TrainerIdList *ids) {
    BattleBoxSave *battleBox;
    int slot;
    int added;

    battleBox = getBattleBox(GameData_GetSaveControl(gameData));
    added = 0;
    for (slot = 0; slot < 6; slot++) {
        BoxPkm *pkm = getBoxSlotAddress(battleBox, box, slot);

        if (PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL) != 0) {
            if (func_ov036_021c6448(playerId, PML_PkmGetParam(pkm, PKM_PARAM_ID, NULL), ids)) {
                added++;
            }
        }
    }
    return added;
}

static BOOL func_ov036_021c6448(u32 playerId, u32 id, TrainerIdList *ids) {
    BOOL added = FALSE;

    if (playerId != id) {
        added = func_ov036_021c6460(id, ids);
    }
    return added;
}

static BOOL func_ov036_021c6460(u32 id, TrainerIdList *ids) {
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
