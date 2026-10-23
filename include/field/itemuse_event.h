#ifndef POKEBW2_FIELD_ITEMUSE_EVENT_H
#define POKEBW2_FIELD_ITEMUSE_EVENT_H

// Overlay 12's itemuse_event.c (the name is a guess): what the player can do where they stand, and the field's
// common events that the menu and the shortcuts start, such as the bike and the Dowsing MCHN. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/game_event.h"

void PlayerActionPerms_Create(PlayerActionPerms *perms, GameSystem *gsys, Field *field);
// The field's common events, by their place in FIELD_COMMON_EVENTS
enum {
    FIELD_COMMON_EVENT_CYCLING,
    FIELD_COMMON_EVENT_ENTRALINK,
    FIELD_COMMON_EVENT_ESCAPE_ROPE,
    FIELD_COMMON_EVENT_SWEET_SCENT,
    FIELD_COMMON_EVENT_FISHING,
    FIELD_COMMON_EVENT_DOWSING,
    FIELD_COMMON_EVENT_COUNT,
};

// Runs one of the field's common events, a FIELD_COMMON_EVENT_*
GameEvent *CallFieldCommonEventFunc(u32 id, GameSystem *gsys, Field *field);
GameEvent *EventFieldToggleCycling_Create(Field *field, GameSystem *gsys);
GameEvent *EventEntralinkWarpIn_CreateDefault(Field *field, GameSystem *gsys);
GameEvent *EventFieldToggleDowsing_Create(Field *field, GameSystem *gsys);

#endif // POKEBW2_FIELD_ITEMUSE_EVENT_H
