#ifndef POKEBW2_FIELD_EVENT_3D_DEMO_H
#define POKEBW2_FIELD_EVENT_3D_DEMO_H

// Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "app/demo3d.h"
#include "struct_decls.h"
#include "system/game_event.h"

struct Event3DDemoWork {
    GameSystem *gameSystem;
    GameCommSys *gameCommSys;
    Demo3DParam param;
    // What GSYS_IsGameCommPaused returned before the demo
    u32 savedState;
    u16 demoId;
    u16 unk26;
};

GameEvent *Event3DDemo_Create(GameSystem *gsys, GameEvent *parent, u32 demoId, u32 param, u32 realTime);
GameEventReturnCode Event3DDemo_Callback(GameEvent *event, u32 *state, void *data);
void CreateSeqLoadParam(Demo3DParam *out, GameSystem *gsys, u32 demoId, u8 param, u32 arg4, u8 hour, u8 minute,
                        u8 season);
void SetupPlaySequenceEventRealTime(Demo3DParam *out, GameSystem *gsys, u8 demoId, u8 param);
void SetupPlaySequenceEventGameTime(Demo3DParam *out, GameSystem *gsys, u8 demoId, u8 param);

#endif // POKEBW2_FIELD_EVENT_3D_DEMO_H
