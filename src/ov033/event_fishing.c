#include "types.h"
#include "battle/btl_setup.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "field/encounter.h"
#include "field/event_battle.h"
#include "field/event_fishing.h"
#include "field/event_wild_battle.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_effect.h"
#include "field/field_g3d_mapper.h"
#include "field/field_map.h"
#include "field/field_player.h"
#include "field/field_script.h"
#include "field/field_surf.h"
#include "field/funfest_scripts.h"
#include "field/game_beacon_set.h"
#include "gfl/key.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/join_avenue.h"
#include "save/records.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

// The fishing form's own animations, the draw statuses after DRAW_STATUS_STOP that the fishing gives the player: the
// line reeled in with nothing on it, a bite, and a hooked Pokémon that the battle starts with
enum {
    FISHING_ANIME_REEL_IN_EMPTY = 1,
    FISHING_ANIME_BITE,
    FISHING_ANIME_HOOKED,
};

BOOL CreateSurfPos(GameData *context, Field *field, VecFx32 *position) {
    VecFx32 targetPosition;
    MapTerrainBuf terrain;
    VecFx32 actorPosition;
    FieldG3DMapper *mapper;
    FieldPlayer *player;
    FieldActor *actor;
    u8 direction;
    fx32 heightDifference;

    mapper = Field_GetG3DMapper(field);
    player = Field_GetPlayer(field);
    actor = FieldPlayer_GetActor(player);
    direction = GetActorFaceDir(actor);
    FieldPlayer_GetWPosInDir(player, direction, &targetPosition);
    if (Field_GetResolvedControllerTypeID(field) == 1) {
        return FALSE;
    }
    if (FieldG3DMapper_GetTerrain(mapper, &targetPosition, &terrain) == FALSE) {
        return FALSE;
    }
    if (MapTile_IsSurfEdge(GetTileClass(terrain.tileType)) == TRUE) {
        ExpandVecInGridDir(direction, &targetPosition, 1 << 16);
        if (FieldG3DMapper_GetTerrain(mapper, &targetPosition, &terrain) == FALSE) {
            return FALSE;
        }
    }
    if (IsTileSurfWater(GetTileClass(terrain.tileType)) == FALSE) {
        return FALSE;
    }
    CopyActorWPos(actor, &actorPosition);
    heightDifference = actorPosition.y - terrain.height;
    if (heightDifference < 0 || heightDifference >= (5 << 14)) {
        return FALSE;
    }
    if (position != NULL) {
        position->x = targetPosition.x;
        position->y = terrain.height;
        position->z = targetPosition.z;
    }
    return TRUE;
}

GameEvent *EventFieldFishing_Create(Field *field, GameSystem *gsys) {
    GameEvent *event;
    FishingEventWork *work;
    MapTerrainBuf terrain;
    VecFx32 position;
    u32 flag;

    event = GameEvent_Create(gsys, NULL, EventFieldFishing_Callback, sizeof(FishingEventWork));
    work = GameEvent_GetData(event);
    MI_CpuFill8(work, 0, sizeof(FishingEventWork));
    work->gsys = gsys;
    work->field = field;
    work->gameData = GSYS_GetGameData(gsys);
    work->encountSystem = Field_GetEncountSystem(work->field);
    work->records = GameData_GetRecords(work->gameData);
    work->actorSystem = Field_GetActorSystem(work->field);
    work->player = Field_GetPlayer(field);
    work->actor = FieldPlayer_GetActor(work->player);
    work->faceDirection = GetActorFaceDir(work->actor);
    FieldPlayer_GetWPos(work->player, &work->playerPos);
    work->playerExState = FieldPlayer_DeriveExState(work->player);
    if (!CreateSurfPos(work->gameData, work->field, &work->fishingPos)) {
        work->noFishing = 1;
    }
    work->gridX = (work->fishingPos.x >> 4) / 4096;
    work->gridY = (work->fishingPos.y >> 4) / 4096;
    work->gridZ = (work->fishingPos.z >> 4) / 4096;
    position = work->playerPos;
    ExpandVecInGridDir(1, &position, 0x10000);
    flag = 1;
    if (!FieldG3DMapper_GetTerrain(Field_GetG3DMapper(field), &position, &terrain)) {
        work->flag5B = flag;
    } else {
        if (GetTileFlags(terrain.tileType) & 0x40) {
            flag = 0;
        }
        work->flag5B = flag;
    }
    if (EncountState_CheckSpecialEncountPos(work->encountSystem, &work->gridX)) {
        work->isPhenomenon = TRUE;
        work->battleSetup = BtlSetup_CreateFishing(work->encountSystem, TRUE);
    } else {
        work->isPhenomenon = FALSE;
        work->battleSetup = BtlSetup_CreateFishing(work->encountSystem, FALSE);
    }
    DisableAllActorsMovement(work->actorSystem);
    return event;
}

// The steps of fishing: the form change, casting the line, waiting for a bite and for A to hook it, the scripts of no
// bite, of A pressed before the bite and of A not pressed in time, the form change back, and the battle
enum {
    FISHING_STEP_START,
    FISHING_STEP_WAIT_FORM_CHANGE,
    FISHING_STEP_CAST,
    FISHING_STEP_WAIT_BITE,
    FISHING_STEP_WAIT_HOOK,
    FISHING_STEP_NO_BITE,
    FISHING_STEP_TOO_EARLY,
    FISHING_STEP_TOO_LATE,
    FISHING_STEP_RETURN_FORM,
    FISHING_STEP_WAIT_RETURN_FORM,
    FISHING_STEP_BATTLE,
};

GameEventReturnCode EventFieldFishing_Callback(GameEvent *event, u32 *state, void *data) {
    FishingEventWork *work;
    PokeParty *party;
    PartyPkm *pkm;
    GameEvent *next;
    u32 result;
    u32 species;
    u8 battleMode;

    work = data;
    switch (*state) {
    case FISHING_STEP_START:
        if (work->noFishing != 0) {
            *state = FISHING_STEP_RETURN_FORM;
            break;
        }
        FieldPlayer_SetSpecialSeq(work->player, 0x200);
        ++*state;
    case FISHING_STEP_WAIT_FORM_CHANGE:
        if (FieldPlayer_StepFormChange(work->player) == TRUE) {
            FldAct_SetDrawStatus(work->actor, DRAW_STATUS_STOP);
            SetActorReflectionHidden(work->actor, work->flag5B);
            GFL_SndSEPlay(SEQ_SE_FLD_19);
            ++*state;
        }
        break;
    case FISHING_STEP_CAST:
        if (Fishing_StepCounter(work, 15) != 0) {
            Fishing_StartLine(work);
            if (work->battleSetup == NULL) {
                *state = FISHING_STEP_NO_BITE;
            } else {
                work->timer = GFL_RandomLCAlt(90) + 30;
                ++*state;
            }
        }
        break;
    case FISHING_STEP_WAIT_BITE:
        result = Fishing_WaitBite(work, work->timer);
        if (result == FISHING_WAIT_PRESSED) {
            *state = FISHING_STEP_TOO_EARLY;
        } else if (result == FISHING_WAIT_TIMEOUT) {
            Fishing_ShowBite(work);
            ++*state;
        }
        break;
    case FISHING_STEP_WAIT_HOOK:
        result = Fishing_WaitBite(work, 30);
        if (result != FISHING_WAIT_NONE) {
            Fishing_DeleteBubble(work);
            if (result == FISHING_WAIT_PRESSED) {
                Fishing_DeleteLine(work);
                FldAct_SetDrawStatus(work->actor, FISHING_ANIME_HOOKED);
                party = BtlSetup_GetParty(work->battleSetup, 1);
                pkm = PokeParty_GetPkm(party, 0);
                species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
                GameBeacon_BroadcastMission(0x1c, species);
                EventScriptCall_Start(event, 0x2796, NULL, 0, HEAPID_FIELDMAP);
                *state = FISHING_STEP_BATTLE;
            } else if (result == FISHING_WAIT_TIMEOUT) {
                RecordAddOne(work->records, 0x50);
                *state = FISHING_STEP_TOO_LATE;
            }
        }
        break;
    case FISHING_STEP_NO_BITE:
        result = Fishing_WaitBite(work, 120);
        if (result != FISHING_WAIT_TIMEOUT) {
            if (result == FISHING_WAIT_PRESSED) {
                *state = FISHING_STEP_TOO_EARLY;
            }
            break;
        }
    case FISHING_STEP_TOO_EARLY:
    case FISHING_STEP_TOO_LATE:
        Fishing_DeleteLine(work);
        if (work->battleSetup != NULL) {
            if (work->isPhenomenon == 1) {
                EncountSystem_CancelPhenomenon(work->encountSystem);
            }
            BtlSetup_Free(work->battleSetup);
        }
        FldAct_SetDrawStatus(work->actor, FISHING_ANIME_REEL_IN_EMPTY);
        EventScriptCall_Start(event, 0x2792 + *state, NULL, 0, HEAPID_FIELDMAP);
        *state = FISHING_STEP_RETURN_FORM;
        break;
    case FISHING_STEP_RETURN_FORM:
        FieldPlayer_SetSpecialSeq(work->player, 8);
        *state = FISHING_STEP_WAIT_RETURN_FORM;
    case FISHING_STEP_WAIT_RETURN_FORM:
        if (FieldPlayer_StepFormChange(work->player) != TRUE) {
            break;
        }
        SetActorReflectionHidden(work->actor, 0);
        return GAMEEVENT_DONE;
    case FISHING_STEP_BATTLE:
        RecordAddOne(work->records, 8);
        JoinAvenue_RecordActivity(13);
        battleMode = work->isPhenomenon == 1 ? ENCOUNT_MODE_EFFECT : ENCOUNT_MODE_FISHING;
        next = EventWildBattleCall_Create(work->gsys, work->field, work->battleSetup, 0, battleMode);
        GameEvent_Replace(event, next);
        return GAMEEVENT_CONTINUE;
    }
    return GAMEEVENT_CONTINUE;
}

u32 Fishing_StepCounter(FishingEventWork *work, u32 value) {
    u32 elapsed;

    elapsed = work->elapsed++;
    if (elapsed < value) {
        return 0;
    }
    work->elapsed = 0;
    return 1;
}

u32 Fishing_WaitBite(FishingEventWork *work, u32 value) {
    u32 elapsed;

    elapsed = work->elapsed++;
    if (elapsed < value) {
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            work->elapsed = 0;
            return FISHING_WAIT_PRESSED;
        }
    } else {
        work->elapsed = 0;
        return FISHING_WAIT_TIMEOUT;
    }
    return FISHING_WAIT_NONE;
}

void Fishing_ShowBite(FishingEventWork *work) {
    void *effects;

    effects = Field_GetFieldEffects(work->field);
    FldAct_SetDrawStatus(work->actor, FISHING_ANIME_BITE);
    work->bubble = FldeffEmotionBubble_SetActor(effects, work->actor, 0, 1);
    FldeffFishing_SetAnimeSpeed(work->line, 1);
}

void Fishing_DeleteBubble(FishingEventWork *work) {
    if (work->bubble != NULL) {
        FieldEffects_DeleteObject(work->bubble);
        work->bubble = NULL;
    }
}

void Fishing_StartLine(FishingEventWork *work) {
    void *effects;
    u32 sameHeight;

    effects = Field_GetFieldEffects(work->field);
    sameHeight = 1;
    if (work->fishingPos.y != work->playerPos.y) {
        sameHeight = 0;
    }
    work->line = FldeffFishing_Start(effects, &work->fishingPos, work->faceDirection, sameHeight);
}

void Fishing_DeleteLine(FishingEventWork *work) {
    if (work->line != NULL) {
        FieldEffects_DeleteObject(work->line);
        work->line = NULL;
    }
}
