#ifndef POKEBW2_FIELD_SHAYMIN_FORM_H
#define POKEBW2_FIELD_SHAYMIN_FORM_H

// Overlay 12's shaymin_form.c (a descriptive name): Shaymin's Sky Forme turns back into its Land Forme at night

#include "types.h"
#include "nitro/rtc.h"
#include "struct_decls.h"

// Whether the night came within the minutes passed up to the time, turning the party's Shaymin back if so
BOOL ShayminForm_CheckNightPassed(GameData *gameData, PokeParty *party, s32 minutes, const RTCTime *time, u32 season);
// Whether it is night at the time, turning the party's Shaymin back and registering the first in the Pokédex if so
BOOL ShayminForm_CheckNight(GameData *gameData, PokeParty *party, const RTCTime *time, u8 season);
void ShayminForm_RevertPartyAndRegister(GameData *gameData, PokeParty *party);

#endif // POKEBW2_FIELD_SHAYMIN_FORM_H
