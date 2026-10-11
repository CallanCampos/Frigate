#ifndef POKEBW2_APP_MONOLITH_MONOLITH_MISSION_SELECT_H
#define POKEBW2_APP_MONOLITH_MONOLITH_MISSION_SELECT_H

#include "types.h"
#include "gfl/proc.h"
#include "struct_decls.h"

// The Entralink monolith's list of Funfest missions to join, on the bottom screen (monolith_mission_select.c, a
// guessed name; the ROM's asserts call its work `mfw`)

extern const GameProcFunctions MONOLITH_MISSION_SELECT_PROC_FUNCTIONS;
// The flag of the first visit twice (monolith_main.h), which nothing reads
extern const u16 MONOLITH_MISSION_UNUSED_FLAGS[2];

#endif // POKEBW2_APP_MONOLITH_MONOLITH_MISSION_SELECT_H
