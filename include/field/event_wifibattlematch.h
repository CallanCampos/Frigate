#ifndef POKEBW2_FIELD_EVENT_WIFIBATTLEMATCH_H
#define POKEBW2_FIELD_EVENT_WIFIBATTLEMATCH_H

// Wi-Fi battles, started by the NetConnectWiFiBattle script command

#include "types.h"
#include "struct_decls.h"

typedef struct {
    Field *field;
    u32 startFrom;
    u32 teamSource;
    u32 battleType;
} EventWifiBattleMatchArgs;

GameEvent *EventWifiBattleMatch_Create(GameSystem *gsys, Field *field, u32 startFrom, u32 teamSource, u32 battleType);
GameEvent *EventWifiBattleMatch_CreateFromArgs(GameSystem *gsys, void *args);

#endif // POKEBW2_FIELD_EVENT_WIFIBATTLEMATCH_H
