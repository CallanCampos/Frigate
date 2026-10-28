#include "types.h"
#include "field/el_scoreboard.h"
#include "field/field.h"
#include "field/field_camera.h"
#include "field/field_gimmick_gate.h"
#include "field/field_task.h"
#include "field/gimmick_obj_elboard.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/version.h"

struct ElboardMessage {
    u8 index;
    u8 padding[3];
    u32 active;
    u32 pending;
    ElScoreboard *scoreboard;
    G3DActor *actor;
    u16 animation;
    u16 padding16;
    fx32 elapsed;
    fx32 triggerTime;
    fx32 duration;
};

struct Elboard {
    u16 heapId;
    u8 count;
    u8 capacity;
    u8 unk04;
    u8 unk05;
    ElboardMessage **messages;
    u32 *flags;
    G3DActor *actor;
    Font *font;
    u32 current;
};

// The event that turns the camera to the board and back
struct GimmickGateEventData {
    GameSystem *gameSystem;
    Field *field;
    u16 duration;
    // The camera to return to
    u16 pitch;
    u16 yaw;
    fx32 zoom;
    VecFx32 targetOffset;
};

Elboard *Elboard_Create(ElboardInit *init) {
    s32 i;
    Elboard *state = GFL_HeapAllocate(init->heapId, sizeof(Elboard), FALSE, "gimmick_obj_elboard.c", 0x5f);

    state->heapId = init->heapId;
    state->count = 0;
    state->capacity = init->capacity;
    state->unk04 = init->unk03;
    state->unk05 = init->unk04;
    state->actor = init->actor;
    state->current = 0;
    state->messages =
        GFL_HeapAllocate(init->heapId, init->capacity * sizeof(ElboardMessage *), FALSE, "gimmick_obj_elboard.c", 0x6c);
    state->flags = GFL_HeapAllocate(init->heapId, init->capacity * sizeof(u32), FALSE, "gimmick_obj_elboard.c", 0x6e);
    for (i = 0; i < init->capacity; i++) {
        state->messages[i] = NULL;
        state->flags[i] = 0;
    }
    state->flags[0] = 1;
    state->font = GFL_FontCreate(0x17, 0, 0, 0, state->heapId);
    return state;
}

void Elboard_AddMessage(Elboard *state, const ElboardMessageArg *arg) {
    if (state->capacity > state->count) {
        state->messages[state->count] = ElboardMessage_Create(state, arg, state->count);
        state->count++;
    }
}

void Elboard_Free(Elboard *state) {
    s32 i;

    GFL_FontFree(state->font);
    for (i = 0; i < state->count; i++) {
        ElboardMessage_Free(state->messages[i]);
    }
    GFL_HeapFree(state->messages);
    GFL_HeapFree(state->flags);
    GFL_HeapFree(state);
}

void Elboard_Update(Elboard *state, u32 amount) {
    s32 i;

    for (i = 0; i < state->count; i++) {
        Elboard_UpdateMessage(state, state->messages[i], amount);
    }
    state->current += amount;
}

s32 Elboard_GetTime(Elboard *state) {
    return state->current;
}

u16 Elboard_GetHeapID(Elboard *state) {
    return state->heapId;
}

u8 Elboard_GetMessageCount(Elboard *state) {
    return state->count;
}

void Elboard_SetTime(Elboard *state, fx32 time) {
    s32 seconds = FX_Whole(time);
    s32 *starts =
        GFL_HeapAllocate(HEAPID_TAIL(state->heapId), state->count * sizeof(s32), FALSE, "gimmick_obj_elboard.c", 0x119);
    s32 *frames =
        GFL_HeapAllocate(HEAPID_TAIL(state->heapId), state->count * sizeof(s32), FALSE, "gimmick_obj_elboard.c", 0x11b);
    s32 total = 0;
    s32 offset;
    s32 i;
    s32 next;
    ElboardMessage *message;

    state->current = time;
    for (i = 0; i < state->count; i++) {
        state->flags[i] = 0;
        total += FX_Whole(state->messages[i]->triggerTime);
        if (i == 0) {
            starts[i] = 0;
        } else {
            starts[i] = starts[i - 1] + FX_Whole(state->messages[i - 1]->triggerTime);
        }
    }
    for (i = 0; i < state->count; i++) {
        offset = seconds % total;
        message = state->messages[i];
        frames[i] = offset - starts[i];
        message->active = frames[i] > 0 && frames[i] < FX_Whole(message->duration);
        message->pending = FX_Whole(message->triggerTime) < frames[i] && frames[i] < FX_Whole(message->duration);
        if (frames[i] < 0) {
            frames[i] = 0;
        }
        message->elapsed = frames[i] << FX32_SHIFT;
        GFL_G3DActorSetAnmFrame(message->actor, message->animation, &message->elapsed);
        if (message->active) {
            GFL_G3DActorBindAnm(message->actor, message->animation);
        } else {
            GFL_G3DActorUnbindAnm(message->actor, message->animation);
        }
    }
    if (state->count != 0 && seconds == 0) {
        message = state->messages[0];
        message->active = 1;
        GFL_G3DActorBindAnm(message->actor, message->animation);
    }
    for (i = 0; i < state->count; i++) {
        next = (i + 1) % state->count;
        state->flags[next] = state->messages[i]->pending && !state->messages[next]->active;
    }
    GFL_HeapFree(starts);
    GFL_HeapFree(frames);
}

ElboardMessage *ElboardMessage_Create(Elboard *state, const ElboardMessageArg *arg, u32 index) {
    ElboardMessage *message =
        GFL_HeapAllocate(state->heapId, sizeof(ElboardMessage), FALSE, "gimmick_obj_elboard.c", 0x17f);
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, arg->messageArc, arg->messageFile, state->heapId);
    StrBuf *text;
    StrBuf *format;
    s32 width;
    fx32 frameCount;
    s32 start;
    s32 end;
    s32 length;
    s32 gap;

    if (arg->wordSet != NULL) {
        text = GFL_StrBufCreate(0xc0, state->heapId);
        format = GFL_MsgDataLoadStrbufNew(msgData, arg->messageId);
        GFL_WordSetFormatStrbuf(arg->wordSet, text, format);
        GFL_HeapFree(format);
    } else {
        text = GFL_MsgDataLoadStrbufNew(msgData, arg->messageId);
    }
    width = GFL_FontGetBlockWidth(text, state->font, 1);
    message->scoreboard = ElScoreboard_Create(GFL_G3DMdlGetTexResource(GFL_G3DActorGetMdl(state->actor)), arg->name,
                                              arg->plName, text, (u16)(state->unk04 * 14), 1, state->heapId);
    GFL_HeapFree(text);
    GFL_MsgDataFree(msgData);
    frameCount = NNS_G3dAnmObjGetNumFrame(GFL_G3DAnmGetRenderObj(GFL_G3DActorGetAnm(state->actor, arg->kind)));
    start = state->unk04 * 13;
    end = start + width;
    length = end - start;
    gap = state->unk05 * 13;
    length += gap;
    message->elapsed = 0;
    message->triggerTime = FX32_CONST(length / 1024.0f);
    message->triggerTime = FX_Mul(message->triggerTime, frameCount);
    message->duration = FX32_CONST(end / 1024.0f);
    message->duration = FX_Mul(message->duration, frameCount);
    message->index = index;
    message->active = 0;
    message->pending = 0;
    message->actor = state->actor;
    message->animation = arg->kind;
    GFL_G3DActorUnbindAnm(message->actor, message->animation);
    GFL_G3DActorResetAnmFrame(message->actor, message->animation);
    return message;
}

void ElboardMessage_Free(ElboardMessage *message) {
    ElScoreboard_Free(message->scoreboard);
    GFL_HeapFree(message);
}

void ElboardMessage_Start(ElboardMessage *message) {
    if (message->active != 1) {
        message->active = 1;
        message->pending = 0;
        message->elapsed = 0;
        GFL_G3DActorBindAnm(message->actor, message->animation);
        GFL_G3DActorResetAnmFrame(message->actor, message->animation);
    }
}

void Elboard_UpdateMessage(Elboard *state, ElboardMessage *message, u32 amount) {
    s32 next;

    if (message->active == 1) {
        ElScoreboard_Update(message->scoreboard);
        GFL_G3DActorStepAnmFrame(message->actor, message->animation, amount);
        message->elapsed += amount;
        if (message->triggerTime < message->elapsed && message->pending != 1) {
            next = (message->index + 1) % state->count;
            state->flags[next] = 1;
            message->pending = 1;
        }
        if (message->duration < message->elapsed) {
            ElboardMessage_Stop(message);
        }
    } else if (state->flags[message->index] == 1) {
        ElboardMessage_Start(message);
        state->flags[message->index] = 0;
    }
}

void ElboardMessage_Stop(ElboardMessage *message) {
    if (message->active == 1) {
        message->active = 0;
        GFL_G3DActorUnbindAnm(message->actor, message->animation);
        GFL_G3DActorResetAnmFrame(message->actor, message->animation);
    }
}

BOOL GimmickGateZoneData_Load(GimmickGateZoneData *dest, u32 arcId, u32 fileId) {
    GFL_ArcSysReadRange(dest, arcId, fileId, 0, 0x7c);
    return TRUE;
}

GameEventReturnCode GimmickGateEvent_ViewBoard(GameEvent *event, u32 *state, void *data) {
    GimmickGateEventData *work = data;
    Field *field = work->field;
    FieldCamera *camera = Field_GetCameraSystem(field);
    FieldTaskManager *tasks = Field_GetTaskManager(field);
    u32 pitch;
    u32 yaw;
    fx32 zoom;
    VecFx32 offset;
    FieldTask *zoomTask;
    FieldTask *pitchTask;
    FieldTask *yawTask;
    FieldTask *offsetTask;
    u32 keys;

    switch (*state) {
    case 0:
        work->pitch = FieldCamera_CoordsGetPitch(camera);
        work->yaw = FieldCamera_CoordsGetYaw(camera);
        work->zoom = FieldCamera_CoordsGetZoom(camera);
        FieldCamera_CoordsGetTargetOffset(camera, &work->targetOffset);
        (*state)++;
        break;
    case 1:
        switch (GimmickGate_GetDirection(field)) {
        case 1:
        default:
            pitch = 0xee5;
            zoom = FX32_CONST(134);
            yaw = 0;
            offset.x = 0;
            offset.y = FX32_CONST(27);
            offset.z = FX32_CONST(-108);
            break;
        case 3:
            pitch = 0xee5;
            zoom = FX32_CONST(134);
            yaw = 0x3fff;
            offset.x = FX32_CONST(-108);
            offset.y = FX32_CONST(27);
            offset.z = 0;
            break;
        }
        zoomTask = FieldCameraMoveTaskZoom_Create(field, work->duration, zoom);
        pitchTask = FieldCameraMoveTaskPitch_Callback(field, work->duration, pitch);
        yawTask = FieldCameraMoveTaskYaw_Callback(field, work->duration, yaw);
        offsetTask = FieldCameraMoveTaskTargetOffs_Create(field, work->duration, &offset);
        FieldTaskManager_AddTask(tasks, zoomTask, 0);
        FieldTaskManager_AddTask(tasks, pitchTask, 0);
        FieldTaskManager_AddTask(tasks, yawTask, 0);
        FieldTaskManager_AddTask(tasks, offsetTask, 0);
        (*state)++;
        break;
    case 2:
        if (FieldTaskManager_IsIdle(tasks)) {
            (*state)++;
        }
        break;
    case 3:
        keys = GCTX_HIDGetPressedKeys();
        if (keys & PAD_KEY_LEFT || keys & PAD_KEY_RIGHT || keys & PAD_KEY_DOWN || keys & PAD_BUTTON_B) {
            (*state)++;
        }
        break;
    case 4:
        zoomTask = FieldCameraMoveTaskZoom_Create(field, work->duration, work->zoom);
        pitchTask = FieldCameraMoveTaskPitch_Callback(field, work->duration, work->pitch);
        yawTask = FieldCameraMoveTaskYaw_Callback(field, work->duration, work->yaw);
        offsetTask = FieldCameraMoveTaskTargetOffs_Create(field, work->duration, &work->targetOffset);
        FieldTaskManager_AddTask(tasks, zoomTask, 0);
        FieldTaskManager_AddTask(tasks, pitchTask, 0);
        FieldTaskManager_AddTask(tasks, yawTask, 0);
        FieldTaskManager_AddTask(tasks, offsetTask, 0);
        (*state)++;
        break;
    case 5:
        if (FieldTaskManager_IsIdle(tasks)) {
            (*state)++;
        }
        break;
    case 6:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *GimmickGateEvent_Create(GameSystem *gsys, Field *field, u32 duration) {
    GameEvent *event;
    GimmickGateEventData *data;

    event = GameEvent_Create(gsys, 0, GimmickGateEvent_ViewBoard, sizeof(struct GimmickGateEventData));
    data = GameEvent_GetData(event);
    data->gameSystem = gsys;
    data->field = field;
    data->duration = duration;
    return event;
}

BOOL GimmickGateBoardEntry_Load(GimmickGateBoardEntry *entry, u32 arcId, u32 fileId) {
    GFL_ArcSysReadRange(entry, arcId, fileId, 0, 0x24);
    return TRUE;
}

BOOL GimmickGateBoardEntry_MatchesZone(GimmickGateBoardEntry *entry, u16 zone) {
    if (entry->zones[0] == ZONE_MAX && entry->zones[1] == ZONE_MAX && entry->zones[2] == ZONE_MAX &&
        entry->zones[3] == ZONE_MAX) {
        return TRUE;
    }
    if (entry->zones[0] == zone || entry->zones[1] == zone || entry->zones[2] == zone || entry->zones[3] == zone) {
        return TRUE;
    }
    return FALSE;
}

BOOL GimmickGateBoardEntry_MatchesVersion(GimmickGateBoardEntry *entry) {
    if (entry->unk00 == 0) {
        return TRUE;
    }
    if (entry->unk00 == getGameVersion()) {
        return TRUE;
    }
    return FALSE;
}
