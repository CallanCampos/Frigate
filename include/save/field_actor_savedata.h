#ifndef POKEBW2_SAVE_FIELD_ACTOR_SAVEDATA_H
#define POKEBW2_SAVE_FIELD_ACTOR_SAVEDATA_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// Block 0x29, the field's actors (field_actor_savedata.c, a descriptive name).

#define FIELD_ACTOR_SAVE_COUNT 64

// What is kept of an actor, with the fields of FieldActor narrowed
typedef struct {
    u32 flags;
    u8 collisionWidth;
    u8 collisionHeight;
    u16 padding6;
    u8 uid;
    u8 moveCode;
    s8 areaW;
    s8 areaH;
    s8 defaultDir;
    s8 faceDir;
    s8 motionDir;
    u16 zoneId;
    u16 modelId;
    u16 evType;
    u16 spawnFlag;
    u16 scrId;
    u16 param0;
    u16 param1;
    u16 param2;
    s16 defaultGPosX;
    s16 defaultGPosY;
    s16 defaultGPosZ;
    s16 gPosX;
    s16 gPosY;
    s16 gPosZ;
    fx32 wPosY;
    // The actor's rail position and the word after it
    u8 railPos[0x10];
    u8 unk40[0x10];
} FieldActorSaveEntry;

typedef struct FieldActorSave {
    FieldActorSaveEntry actors[FIELD_ACTOR_SAVE_COUNT];
} FieldActorSave;

u32 ActorSave_GetBlockSize(void);
void ActorSave_Init(FieldActorSave *save);
// Saves the system's actors, but for those with movement flag 0x100000, and makes the system's actors those of a save
void ActorSave_Save(MMSys *system, FieldActorSave *save);
void ActorSave_Load(MMSys *system, FieldActorSave *save);

#endif // POKEBW2_SAVE_FIELD_ACTOR_SAVEDATA_H
