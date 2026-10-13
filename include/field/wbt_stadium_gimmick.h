#ifndef POKEBW2_FIELD_WBT_STADIUM_GIMMICK_H
#define POKEBW2_FIELD_WBT_STADIUM_GIMMICK_H

#include "types.h"
#include "gfl/str.h"
#include "struct_decls.h"

// wbt_stadium_gimmick.c, overlay 134, named after what it does (the ROM names only the overlay's other file,
// wbt_battle.c): the field gimmick of the Pokémon World Tournament's stadium (zone 193), which script plugin 7 drives.
// Scene 0 holds the stadium's models and scene 2 the portraits of the fighters

// Overlay 36's gimmick table calls the first three: setup, end and update of the gimmick
void WbtStadiumGimmick_Setup(Field *field);
void WbtStadiumGimmick_End(Field *field);
void WbtStadiumGimmick_Update(Field *field);
// Writes the names on the name plates, mode 0 for both Trainers and 1 and 2 for one, and the files of the portraits
void WbtStadiumGimmick_SetNames(Field *field, u8 mode, StrBuf *name, StrBuf *playerName, u16 file, u16 playerFile);
// Draws the portraits and starts the screens' animations
void WbtStadiumGimmick_ShowPortraits(Field *field, u8 mode);
// Loads scene 2 with the portrait of a position (20 for a lone one) on a side
void WbtStadiumGimmick_LoadPortraitScene(Field *field, u8 position, u8 side);
// Plays the intro of actor 5
void WbtStadiumGimmick_PlayIntro(Field *field);
// Switches between the lights of actors 7 to 10
void WbtStadiumGimmick_SwitchLights(Field *field, u8 a1, u8 a2);
// Plays the animations of actors 11 to 14
void WbtStadiumGimmick_PlayCrowd(Field *field, u8 a1);

#endif // POKEBW2_FIELD_WBT_STADIUM_GIMMICK_H
