// The events of the special Pokémon (overlay 129), which scrcmd_sp_poke_gimmick.c of overlay 12 runs: a Pokémon that
// jumps from one place to another, and an effect (actor 2) that ends by replacing the actor of an NPC. The ROM embeds
// no name for this file; its name is descriptive, and the boundary with sp_poke_gimmick.c is at the first event that
// reads no scene setup. It has no data of its own: the tables all belong to sp_poke_gimmick.c. The work they use is
// the gimmick's work block 1, which also keeps their path
#include "types.h"
#include "constants/sound.h"
#include "field/field.h"
#include "field/field_actor.h"
#include "field/field_exp_obj.h"
#include "field/player_state.h"
#include "field/sp_poke_event.h"
#include "field/sp_poke_gimmick.h"
#include "gfl/sound.h"
#include "nitro/fx.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"

#define WORK_ID 1

static GameEventReturnCode SpPokeEvent_JumpMain(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode SpPokeEvent_WaitFrame(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode SpPokeEvent_WaitAnimEnd(GameEvent *event, u32 *state, void *data);
static GameEventReturnCode SpPokeEvent_AppearMain(GameEvent *event, u32 *state, void *data);

GameEvent *SpPokeEvent_CreateJump(GameSystem *gsys, u16 mode, const VecFx32 *start, const VecFx32 *end, fx32 height,
                               u32 frames) {
    Field *field = GSYS_GetField(gsys);
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(field, WORK_ID);
    int actor = 1;
    GameEvent *event;
    int i;
    FieldExpObjSystem *system = Field_GetExpObjSystem(field);

    if (mode == 0) {
        actor = 0;
        GFL_SndSEPlay(SEQ_SE_NAGERU);
    }
    FieldExpObj_GetActorMatrixPtr(system, 0, actor)->translation = *start;
    FieldExpObj_SetActorHidden(system, 0, actor, FALSE);
    for (i = 0; i < 3; i++) {
        FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(system, 0, actor, i), TRUE);
        FieldExpObj_SetAnm(system, 0, actor, i, FALSE);
        FieldExpObj_SetAnmFrame(system, 0, actor, i, 0);
    }
    work->start = *start;
    work->end = *end;
    work->frames = frames;
    work->frame = 0;
    work->height = height;
    event = GameEvent_Create(gsys, NULL, SpPokeEvent_JumpMain, 0);
    work->mode = mode;
    return event;
}

void SpPokeEvent_Appear(GameSystem *gsys, u16 a1, const VecFx32 *pos) {
    Field *field = GSYS_GetField(gsys);
    int actor = 1;
    int i;
    FieldExpObjSystem *system;

    Field_GetGimmickWorkBlock(field, WORK_ID);
    system = Field_GetExpObjSystem(field);
    if (a1 == 0) {
        actor = 0;
        GFL_SndSEPlay(SEQ_SE_BOWA2);
    } else {
        GFL_SndSEPlay(SEQ_SE_FLD_146);
    }
    FieldExpObj_GetActorMatrixPtr(system, 0, actor)->translation = *pos;
    FieldExpObj_SetActorHidden(system, 0, actor, FALSE);
    for (i = 0; i < 3; i++) {
        FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(system, 0, actor, i), FALSE);
        FieldExpObj_SetAnm(system, 0, actor, i, TRUE);
        FieldExpObj_SetAnmFrame(system, 0, actor, i, 0);
    }
}

GameEvent *SpPokeEvent_CreateWaitFrame(GameSystem *gsys, u16 a1) {
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), WORK_ID);
    GameEvent *event = GameEvent_Create(gsys, NULL, SpPokeEvent_WaitFrame, 0);

    work->mode = a1;
    return event;
}

GameEvent *SpPokeEvent_CreateWaitAnimEnd(GameSystem *gsys, u16 a1) {
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(GSYS_GetField(gsys), WORK_ID);
    GameEvent *event = GameEvent_Create(gsys, NULL, SpPokeEvent_WaitAnimEnd, 0);

    work->mode = a1;
    return event;
}

// The value that is step of steps of the way from a to b
static inline fx32 SpPokeEvent_Lerp(fx32 a, fx32 b, int step, int steps) {
    fx32 d = b - a;
    return a + d * step / steps;
}

// Moves the Pokémon a step along the line from the start to the end, raised by an arc that peaks half way
static GameEventReturnCode SpPokeEvent_JumpMain(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    FieldExpObjSystem *system = Field_GetExpObjSystem(field);
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(field, WORK_ID);
    int actor = 1;
    VecFx32 pos;
    int arcFrame;

    if (work->mode == 0) {
        actor = 0;
    }
    work->frame++;
    pos.x = SpPokeEvent_Lerp(work->start.x, work->end.x, work->frame, work->frames);
    pos.y = SpPokeEvent_Lerp(work->start.y, work->end.y, work->frame, work->frames);
    pos.z = SpPokeEvent_Lerp(work->start.z, work->end.z, work->frame, work->frames);
    arcFrame = work->frame;
    if (arcFrame >= (work->frames + 1) / 2) {
        arcFrame = work->frames - arcFrame;
    }
    pos.y += work->height * arcFrame / work->frames;
    FieldExpObj_GetActorMatrixPtr(system, 0, actor)->translation = pos;
    if (work->frame >= work->frames) {
        if (work->mode == 1) {
            FieldExpObj_SetActorHidden(system, 0, actor, TRUE);
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

// Waits until the Pokémon's first animation is at a frame
static GameEventReturnCode SpPokeEvent_WaitFrame(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    FieldExpObjSystem *system = Field_GetExpObjSystem(field);
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(field, WORK_ID);
    fx32 frame;
    int actor = 1;

    if (work->mode == 0) {
        frame = FX32_CONST(5);
        actor = 0;
    } else {
        frame = FX32_CONST(34);
    }
    if ((fx32)func_ov036_021b8520(system, 0, actor, 0) >= frame) {
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

static GameEventReturnCode SpPokeEvent_WaitAnimEnd(GameEvent *event, u32 *state, void *data) {
    Field *field = GSYS_GetField(GameEvent_GetGameSystem(event));
    FieldExpObjSystem *system = Field_GetExpObjSystem(field);
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(field, WORK_ID);
    int actor = 1;

    if (work->mode == 0) {
        actor = 0;
    }
    if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(system, 0, actor, 0))) {
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *SpPokeEvent_CreateAppear(GameSystem *gsys, const VecFx32 *pos, u16 actorId, u16 uid) {
    Field *field = GSYS_GetField(gsys);
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(field, WORK_ID);
    int i;
    FieldExpObjSystem *system = Field_GetExpObjSystem(field);
    SpPokeGimmickNpc *npc = work->data.npc;

    npc->actorId = actorId;
    npc->uid = uid;
    FieldExpObj_GetActorMatrixPtr(system, 0, 2)->translation = *pos;
    FieldExpObj_SetActorHidden(system, 0, 2, FALSE);
    for (i = 0; i < 3; i++) {
        FieldExpObjAnm_SetPaused(FieldExpObj_GetAnmInfo(system, 0, 2, i), FALSE);
        FieldExpObj_SetAnm(system, 0, 2, i, TRUE);
        FieldExpObj_SetAnmFrame(system, 0, 2, i, 0);
    }
    return GameEvent_Create(gsys, NULL, SpPokeEvent_AppearMain, 0);
}

// At a frame of actor 2's animation, replaces the NPC's actor with the zone's own definition of it, and waits for the
// animation to end
static GameEventReturnCode SpPokeEvent_AppearMain(GameEvent *event, u32 *state, void *data) {
    GameSystem *gsys = GameEvent_GetGameSystem(event);
    Field *field = GSYS_GetField(gsys);
    FieldExpObjSystem *system = Field_GetExpObjSystem(field);
    SpPokeGimmickWork *work = Field_GetGimmickWorkBlock(field, WORK_ID);

    if (func_ov036_021b8520(system, 0, 2, 0) == FX32_CONST(15)) {
        u16 actorId = work->data.npc->actorId;
        u16 uid = work->data.npc->uid;
        MMSys *actorSystem = Field_GetActorSystem(field);
        GameData *gameData;
        EventData *eventData;
        u32 npcCount;
        u32 zoneId;

        DeleteActor(FindFieldActor(actorSystem, actorId));
        gameData = GSYS_GetGameData(gsys);
        eventData = GameData_GetEventData(gameData);
        npcCount = GetZoneNPCsCount(eventData);
        zoneId = PlayerState_GetZoneID(GameData_GetPlayerState(gameData));
        if (npcCount != 0) {
            EventWork *eventWork = GameData_GetEventWork(gameData);
            FieldActor *actor =
                func_ov012_021668f8(actorSystem, GetZoneNPCs(eventData), zoneId, npcCount, eventWork, uid);

            if (actor != NULL) {
                DisableActorMovement(actor);
            }
        }
    }
    if (FieldExpObjAnm_IsPlaybackFinished(FieldExpObj_GetAnmInfo(system, 0, 2, 0))) {
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}
