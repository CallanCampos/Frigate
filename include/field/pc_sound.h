#ifndef POKEBW2_FIELD_PC_SOUND_H
#define POKEBW2_FIELD_PC_SOUND_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "system/game_event.h"

struct PCSoundEventData {
    GameSystem *gameSystem;
    Field *field;
    // volatile only because the original reloads it at each use; nothing is known to change it behind the compiler
    FieldChunkPropHolder *volatile pcProp;
    u32 unkC;
    u32 skipSound;
};

GameEventReturnCode pcEntrySound(GameEvent *event, u32 *state, void *data);
GameEvent *CreatePCSoundCallEvent(GameEvent *parent, GameSystem *gsys, Field *field);
GameEventReturnCode func_ov033_021799e8(GameEvent *event, u32 *state, void *data);
GameEvent *func_ov033_02179a58(GameEvent *parent, GameSystem *gsys, Field *field);
GameEventReturnCode pcLogOffSound(GameEvent *event, u32 *state, void *data);
GameEvent *FieldDoorAnime_StartClose(GameEvent *parent, GameSystem *gsys, Field *field, u32 skipSound);
GameEvent *Scrcmd_CreateSubProcEvent(GameSystem *gsys, u16 option, u16 *result);
GameEventReturnCode Scrcmd_SubProcEventMain(GameEvent *event, u32 *state, void *data);

extern const GameProcFunctions HALL_OF_FAME_PC_PROC_FUNCTIONS;

#endif // POKEBW2_FIELD_PC_SOUND_H
