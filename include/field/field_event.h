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
    // The transition types of the warps the player leaves by and arrives at, WARP_TRANSITION_NONE for a move to a
    // position, which pick the warp's events
    u32 departureTransitionType;
    u32 transitionType;
    u16 zoneId;
    ZoneSpawnInfo spawn;
    u32 outTransition;
    u32 inTransition;
    BOOL seasonChanged;
    u8 startSeason;
    u8 endSeason;
    // Whether the departure changed to the next zone's BGM, which the arrival then fades in
    u32 zoneBGMChanged;
    // Whether the departure faded the BGM out to keep it, which the arrival then fades back in
    u32 bgmFadedOut;
    // The task that walks the player one tile, which the entrance events start
    FieldAcmdTCB *walkTask;
};

// field_transition.c's transitions of the screen around a warp, by type: a fade to black or white, the building
// entrance's (the default), the season banner's (on the way in), a wipe out, the wipes of the player's facing up, down,
// left and right, and the one the player's facing picks among them. skipWait ends the fade's event without waiting for
// it; the way out takes the fade's slowness, and the way in whether to restore the field's BGs first
enum {
    FIELD_TRANSITION_FADE_BLACK,
    FIELD_TRANSITION_FADE_WHITE,
    FIELD_TRANSITION_BUILDING,
    FIELD_TRANSITION_SEASON,
    FIELD_TRANSITION_WIPE,
    FIELD_TRANSITION_WIPE_UP,
    FIELD_TRANSITION_WIPE_DOWN,
    FIELD_TRANSITION_WIPE_LEFT,
    FIELD_TRANSITION_WIPE_RIGHT,
    FIELD_TRANSITION_WIPE_FACING,
};
GameEvent *CallFieldMapEntranceInTransition(GameSystem *gsys, Field *field, u32 type, u32 skipWait, u32 keepLCD,
                                            u8 startSeason, u8 endSeason);
GameEvent *CallFieldMapEntranceOutTransition(GameSystem *gsys, Field *field, u32 type, u32 skipWait, u32 fadeSlowness);
GameEvent *CallFieldMapEntranceOutTransitionDefault(GameSystem *gsys, Field *field, u32 type, u32 skipWait);
// The season banner's event between two seasons, which calls callback with callbackArg
GameEvent *EventGameTransition_CreateSeasonFadeInEventWithCallback(GameSystem *gsys, Field *field, u8 startSeason,
                                                                   u8 endSeason, void (*callback)(void *arg),
                                                                   void *callbackArg);
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
// What the input's result says: no name, the species' own, or another, which the word set takes
enum {
    POKE_NAME_INPUT_CANCELED,
    POKE_NAME_INPUT_SPECIES,
    POKE_NAME_INPUT_NICKNAME,
};
GameEvent *EventPokeNameWordSetInput_Create(GameSystem *gsys, u16 species, u16 form, u16 *result, WordSet *wordSet);
// The nickname of the party's Pokémon in slot; transition is passed to the name entry's subprocess
GameEvent *prepareForPokemonNameEntryData(GameSystem *gsys, u16 *result, int slot, BOOL transition);
GameEvent *RivalNameEntry_Create(GameSystem *gsys, u16 *result);
GameEvent *EventFieldSubprocessTransition_Create(GameSystem *gsys, Field *field, s32 overlayId,
                                                 const GameProcFunctions *functions, void *param);
GameEvent *EventPlayerSpinDown_Create(GameEvent *event, GameSystem *gsys, Field *field);
GameEvent *EventQuicksandArrive_Create(GameEvent *event, GameSystem *gsys, Field *field);
GameEvent *EventQuicksandDrawIn_Create(GameEvent *event, GameSystem *gsys, Field *field, const VecFx32 *pos);
GameEvent *EventTeleportEffect_Create(GameEvent *event, GameSystem *gsys, Field *field, BOOL teleport);
void PlaceName_OnZoneChange(PlaceName *placeName, s32 zoneId);
void PlaceName_Cancel(PlaceName *placeName);
void PlaceName_ShowForZone(PlaceName *placeName, u32 zoneId);
// The field effect of an ID, such as flying off and landing, or NULL. An ID is the effect's file in archive 0x76; the
// ones the code names are the Pokémon sprite of a field move, the player's flight off and landing, by the player's
// sex, and Alder's flight
enum {
    FLD3DCI_POKE_SPRITE = 3,
    FLD3DCI_FLY_OUT_MALE = 11,
    FLD3DCI_FLY_IN_MALE = 12,
    FLD3DCI_ALDER_FLY = 30,
    FLD3DCI_FLY_OUT_FEMALE = 31,
    FLD3DCI_FLY_IN_FEMALE = 32,
};
GameEvent *EventFieldEffect_Create(GameSystem *gsys, Fld3DCi *ci, u8 effectId);
// The fade out of the field before flying
GameEvent *EventGameTransition_CreateFadeOutEvent(GameSystem *gsys, Field *field, u32 type, u32 skipWait);
GameEvent *EventGameTransition_CreateFadeInEventWithBgRestore(GameSystem *gsys, Field *field, u32 type, u32 skipWait,
                                                              u32 fadeSlowness);
GameEvent *EventQuicksandArrive_CreateSpinZoom(GameEvent *event, GameSystem *gsys, Field *field, BOOL seasonChanged,
                                               u32 prevSeason, u32 season);
GameEvent *EventQuicksandArrive_CreateSpinZoomAlt(GameEvent *event, GameSystem *gsys, Field *field, BOOL seasonChanged,
                                                  u32 prevSeason, u32 season);
GameEvent *EventQuicksandArrive_CreateSpinZoomSmoke(GameEvent *event, GameSystem *gsys, Field *field);
GameEvent *EventQuicksandArrive_CreateMoveAnime(GameEvent *event, GameSystem *gsys, Field *field);
GameEvent *EventDig_CreateRisingSpin(GameEvent *event, GameSystem *gsys, Field *field);
GameEvent *EventWarpSequence_CreateIn(WarpSequence *warp);
GameEvent *EventWarpSequence_CreateOut(WarpSequence *warp);
GameEvent *func_ov033_021773e4(GameSystem *gsys, void *args);
// What starts the events of the field in each kind of map
GameEvent *FieldEventProvider_Grid(GameSystem *gsys, void *data);
GameEvent *FieldEventProvider_UnionRoom(GameSystem *gsys, void *data);
GameEvent *FieldEventProvider_NoGrid(GameSystem *gsys, void *data);
GameEvent *FieldEventProvider_Hybrid(GameSystem *gsys, void *data);
// How many Pokémon in the party can battle: not Eggs, and not fainted
u32 FieldEventProvider_CountAlivePartyMembers(GameSystem *gsys);

#endif // POKEBW2_FIELD_FIELD_EVENT_H
