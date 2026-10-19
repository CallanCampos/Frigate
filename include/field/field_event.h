#ifndef POKEBW2_FIELD_FIELD_EVENT_H
#define POKEBW2_FIELD_FIELD_EVENT_H

#include "types.h"
#include "field/zone.h"
#include "gfl/proc.h"
#include "struct_decls.h"
#include "system/game_event.h"

// What a battle started from the field takes from it, which SaveBtlFieldStatus of overlay 36 fills in
struct BtlFieldStatus {
    u32 bgId;
    u32 terrain;
    u8 weather;
    u8 season;
    u16 zoneId;
    u8 hour;
    u8 minute;
};

void SaveBtlFieldStatus(BtlFieldStatus *status, GameData *gameData, Field *field);

// Moves the player to another zone with a transition, such as through a door
struct WarpSequence {
    GameEvent *parent;
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    u32 unk10;
    u32 transitionType;
    u16 zoneId;
    ZoneSpawnInfo spawn;
    u32 outTransition;
    u32 inTransition;
    BOOL seasonChanged;
    u8 startSeason;
    u8 endSeason;
    u32 unk48;
    u32 unk4C;
};

GameEvent *CallFieldMapEntranceInTransition(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4, u8 startSeason,
                                            u8 endSeason);
GameEvent *CallFieldMapEntranceOutTransition(GameSystem *gsys, Field *field, u32 type, u32 a3, u32 a4);
GameEvent *CallFieldMapEntranceOutTransitionDefault(GameSystem *gsys, Field *field, u32 type, u32 a3);
// The season banner's event between two seasons, which calls callback with callbackArg
GameEvent *func_ov036_021b8800(GameSystem *gsys, Field *field, u8 startSeason, u8 endSeason,
                               void (*callback)(void *arg), void *callbackArg);
GameEvent *CreateFieldCloseEvent(GameSystem *gsys, Field *field);
GameEvent *EventDig_Create(GameEvent *event, GameSystem *gsys, Field *field, BOOL seasonChanged);
BOOL EventEntralinkWarpIn_CheckAllowed(GameSystem *gsys);
GameEvent *EventEscapeRope_Create(GameEvent *event, GameSystem *gsys, Field *field, BOOL seasonChanged);
GameEvent *EventFieldCloseKeepSound_Create(GameSystem *gsys, Field *field);
GameEvent *EventFieldOpen_Create(GameSystem *gsys);
GameEvent *EventFieldOpen_CreateHeadless(GameSystem *gsys);
GameEvent *EventFieldOpenRestoreLCD_Create(GameSystem *gsys);
// Restores the field's screens and the BGs that were on
void FieldG3D_RestoreSurface(Field *field);
// Runs the proc as a field subprocess
GameEvent *EventFieldSubprocessCall_Create(GameSystem *gsys, Field *field, s32 overlayId,
                                           const GameProcFunctions *functions, void *param);
// Runs the proc as a field subprocess, then calls callback with work if there is a callback, and frees work
GameEvent *EventFieldSubprocessCall_CreateWithCallback(GameSystem *gsys, Field *field, s32 overlayId,
                                                       const GameProcFunctions *functions, void *param,
                                                       void (*callback)(void *work), void *work);
// Events of overlay 36's other files that scripts start: a name input for the Pokémon, and another input with its
// result
GameEvent *EventPokeNameWordSetInput_Create(GameSystem *gsys, u16 a1, u16 a2, u16 *result, WordSet *wordSet);
// The nickname of the party's Pokémon in slot; transition is passed to the name entry's subprocess
GameEvent *prepareForPokemonNameEntryData(GameSystem *gsys, u16 *result, int slot, BOOL transition);
GameEvent *RivalNameEntry_Create(GameSystem *gsys, u16 *result);
GameEvent *EventFieldSubprocessTransition_Create(GameSystem *gsys, Field *field, s32 overlayId,
                                                 const GameProcFunctions *functions, void *param);
GameEvent *EventPlayerSpinDown_Create(GameEvent *event, GameSystem *gsys, Field *field);
GameEvent *EventQuicksandArrive_Create(GameEvent *event, GameSystem *gsys, Field *field);
GameEvent *EventQuicksandDrawIn_Create(GameEvent *event, GameSystem *gsys, Field *field, VecFx32 *pos);
GameEvent *EventTeleportEffect_Create(GameEvent *event, GameSystem *gsys, Field *field, BOOL a3);
void func_ov036_021b50c8(PlaceName *placeName, s32 zoneId);
void func_ov036_021b5168(PlaceName *placeName);
void func_ov036_021b50f4(PlaceName *placeName, u32 zoneId);
// The field effect of an ID, such as flying off and landing, or NULL
GameEvent *EventFieldEffect_Create(GameSystem *gsys, Fld3DCi *ci, u8 effectId);
// The fade out of the field before flying
GameEvent *func_ov036_021b8890(GameSystem *gsys, Field *field, u32 a2, u32 a3);
GameEvent *func_ov036_021b8850(GameSystem *gsys, Field *field, u32 a2, u32 a3, u32 a4);
GameEvent *func_ov036_021b95ac(GameEvent *event, GameSystem *gsys, Field *field, BOOL seasonChanged, u32 prevSeason,
                               u32 season);
GameEvent *func_ov036_021b95e0(GameEvent *event, GameSystem *gsys, Field *field, BOOL seasonChanged, u32 prevSeason,
                               u32 season);
GameEvent *func_ov036_021b9614(GameEvent *event, GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b9664(GameEvent *event, GameSystem *gsys, Field *field);
GameEvent *func_ov036_021b9df8(GameEvent *event, GameSystem *gsys, Field *field);
GameEvent *EventWarpSequence_CreateIn(WarpSequence *warp);
GameEvent *EventWarpSequence_CreateOut(WarpSequence *warp);
GameEvent *func_ov033_021773e4(GameSystem *gsys, void *args);
// What starts the events of the field in each kind of map
GameEvent *FieldEventProvider_Grid(GameSystem *gsys, void *data);
GameEvent *FieldEventProvider_UnionRoom(GameSystem *gsys, void *data);
GameEvent *FieldEventProvider_NoGrid(GameSystem *gsys, void *data);
GameEvent *FieldEventProvider_Hybrid(GameSystem *gsys, void *data);

#endif // POKEBW2_FIELD_FIELD_EVENT_H
