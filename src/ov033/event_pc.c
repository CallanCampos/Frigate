#include "types.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_g3d_mapper.h"
#include "field/field_prop.h"
#include "field/pc_sound.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "struct_decls.h"
#include "system/game_event.h"
#include "system/game_system.h"

struct PCSubprocessEventData {
    GameSystem *gsys;
    Field *field;
    GameData *gameData;
    u16 option;
    u16 selection;
    u16 *result;
};

GameEvent *Scrcmd_CreateSubProcEvent(GameSystem *gsys, u16 option, u16 *result) {
    GameEvent *event;
    PCSubprocessEventData *data;

    event = GameEvent_Create(gsys, NULL, Scrcmd_SubProcEventMain, sizeof(PCSubprocessEventData));
    data = GameEvent_GetData(event);
    data->gsys = gsys;
    data->field = GSYS_GetField(gsys);
    data->result = result;
    data->gameData = GSYS_GetGameData(gsys);
    data->option = option;
    return event;
}

GameEventReturnCode Scrcmd_SubProcEventMain(GameEvent *event, u32 *state, void *eventData) {
    PCSubprocessEventData *data;
    GameEvent *next;

    data = eventData;
    switch (*state) {
    case 0:
        next = EventFieldSubprocessTransition_Create(data->gsys, data->field, OVERLAY_ID(256),
                                                     &HALL_OF_FAME_PC_PROC_FUNCTIONS, &data->gameData);
        GameEvent_ChainNext(event, next);
        (*state)++;
        break;
    case 1:
        switch (data->selection) {
        case 0:
            *data->result = 0;
            break;
        case 1:
            *data->result = 1;
            break;
        }
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEventReturnCode pcEntrySound(GameEvent *event, u32 *state, void *eventData) {
    struct PCSoundEventData *data = eventData;
    VecFx32 position;
    FieldPropAreaBounds bounds;
    FieldPropSystem *propSystem;
    FieldChunkPropHolder *prop;

    switch (*state) {
    case 0:
        GFL_SndSEPlay(0x55b);
        FieldPlayer_GetWPos(Field_GetPlayer(data->field), &position);
        bounds.minZ = position.z - (1 << 16);
        bounds.maxZ = position.z + (1 << 16);
        bounds.minX = position.x - (1 << 16);
        bounds.maxX = position.x + (1 << 16);
        propSystem = FieldG3DMapper_GetBMSystem(Field_GetG3DMapper(data->field));
        prop = FieldPropSystem_FindProp(propSystem, 4, &bounds);
        if (prop != NULL) {
            data->pcProp = prop;
            FieldChunkPropHolder_CallAnmCmd(propSystem, prop, 0, 0);
        }
        (*state)++;
        break;
    case 1:
        if (GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(0x55b))) {
            break;
        }
        (*state)++;
        break;
    case 2:
        GFL_SndSEPlay(0x55c);
        if (data->pcProp != NULL) {
            FieldChunkPropHolder_CallAnmCmd(FieldG3DMapper_GetBMSystem(Field_GetG3DMapper(data->field)), data->pcProp,
                                            1, 2);
        }
        (*state)++;
        break;
    case 3:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *CreatePCSoundCallEvent(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event = GameEvent_Create(gsys, parent, pcEntrySound, sizeof(struct PCSoundEventData));
    struct PCSoundEventData *data = GameEvent_GetData(event);

    data->gameSystem = gsys;
    data->field = field;
    data->pcProp = NULL;
    return event;
}

GameEventReturnCode func_ov033_021799e8(GameEvent *event, u32 *state, void *eventData) {
    struct PCSoundEventData *data;
    VecFx32 position;
    FieldPropAreaBounds bounds;
    FieldPropSystem *propSystem;
    FieldChunkPropHolder *prop;

    data = eventData;
    switch (*state) {
    case 0:
        FieldPlayer_GetWPos(Field_GetPlayer(data->field), &position);
        bounds.minZ = position.z - (1 << 16);
        bounds.maxZ = position.z + (1 << 16);
        bounds.minX = position.x - (1 << 16);
        bounds.maxX = position.x + (1 << 16);
        propSystem = FieldG3DMapper_GetBMSystem(Field_GetG3DMapper(data->field));
        prop = FieldPropSystem_FindProp(propSystem, 4, &bounds);
        if (prop != NULL) {
            data->pcProp = prop;
            FieldChunkPropHolder_CallAnmCmd(propSystem, prop, 1, 2);
        }
        (*state)++;
        break;
    case 1:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *func_ov033_02179a58(GameEvent *parent, GameSystem *gsys, Field *field) {
    GameEvent *event;
    struct PCSoundEventData *data;

    event = GameEvent_Create(gsys, parent, func_ov033_021799e8, sizeof(struct PCSoundEventData));
    data = GameEvent_GetData(event);
    data->gameSystem = gsys;
    data->field = field;
    data->pcProp = NULL;
    return event;
}

GameEventReturnCode pcLogOffSound(GameEvent *event, u32 *state, void *eventData) {
    struct PCSoundEventData *data = eventData;
    VecFx32 position;
    FieldPropAreaBounds bounds;
    FieldPropSystem *propSystem;
    FieldChunkPropHolder *prop;
    u32 currentState = *state;

    switch (currentState) {
    case 0:
        if (data->skipSound == 0) {
            GFL_SndSEPlay(0x55d);
        }
        FieldPlayer_GetWPos(Field_GetPlayer(data->field), &position);
        bounds.minZ = position.z - (1 << 16);
        bounds.maxZ = position.z + (1 << 16);
        bounds.minX = position.x - (1 << 16);
        bounds.maxX = position.x + (1 << 16);
        propSystem = FieldG3DMapper_GetBMSystem(Field_GetG3DMapper(data->field));
        prop = FieldPropSystem_FindProp(propSystem, 4, &bounds);
        if (prop != NULL) {
            data->pcProp = prop;
            FieldChunkPropHolder_CallAnmCmd(propSystem, prop, 2, 0);
        }
        (*state)++;
        break;
    case 1:
        if (data->skipSound == 0) {
            if (GFL_SndPlayerIsActive(GFL_SndSeqGetPlayerIndex(0x55d))) {
                break;
            }
            (*state)++;
        } else {
            *state = currentState + 1;
        }
        break;
    case 2:
        return GAMEEVENT_DONE;
    }
    return GAMEEVENT_CONTINUE;
}

GameEvent *FieldDoorAnime_StartClose(GameEvent *parent, GameSystem *gsys, Field *field, u32 skipSound) {
    GameEvent *event;
    struct PCSoundEventData *data;

    event = GameEvent_Create(gsys, parent, pcLogOffSound, sizeof(struct PCSoundEventData));
    data = GameEvent_GetData(event);
    data->gameSystem = gsys;
    data->field = field;
    data->pcProp = NULL;
    data->skipSound = skipSound;
    return event;
}
