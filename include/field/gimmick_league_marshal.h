#ifndef POKEBW2_FIELD_GIMMICK_LEAGUE_MARSHAL_H
#define POKEBW2_FIELD_GIMMICK_LEAGUE_MARSHAL_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of Marshal's room in the Pokémon League, zone 142 (overlay 124): its models, and the platform that
// lowers the player with a shake of the camera. The name is a guess: the ROM has no string for it. Its sound uses
// Marshal's Japanese name, SEQ_SE_SW_RENBU_04

// Overlay 36's gimmick table calls these
void MarshalRoom_Init(Field *field);
void MarshalRoom_End(Field *field);
void MarshalRoom_Update(Field *field);

// The League's script plugin (overlay 52) calls these
// Plays the two animations of model 0
void MarshalRoom_PlayModel0Anims(GameSystem *gsys);
// Plays one of model 3's two animations, the first when which is 0, and pauses the other
void MarshalRoom_PlayModel3Anim(GameSystem *gsys, u16 which);
// Plays the three animations of model 1
void MarshalRoom_PlayModel1Anims(GameSystem *gsys);
// The event that lowers the platform with the player on it
GameEvent *MarshalRoom_LowerPlatform(GameSystem *gsys);

#endif // POKEBW2_FIELD_GIMMICK_LEAGUE_MARSHAL_H
