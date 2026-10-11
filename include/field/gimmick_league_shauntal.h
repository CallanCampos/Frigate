#ifndef POKEBW2_FIELD_GIMMICK_LEAGUE_SHAUNTAL_H
#define POKEBW2_FIELD_GIMMICK_LEAGUE_SHAUNTAL_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of Shauntal's room in the Pokémon League, zone 140 (overlay 122): ten models with animations, a
// room change under a fade and a fog that stays off until it. The name is a guess: the ROM has no string for it. Its
// sounds use Shauntal's Japanese name, SEQ_SE_SW_SHIKIMI_*

// Overlay 36's gimmick table calls these
void GimmickShauntal_Setup(Field *field);
void GimmickShauntal_End(Field *field);
void GimmickShauntal_Update(Field *field);

// The League's script plugin (overlay 52) calls these
// The event that plays model 8's last animations once the first is done, and saves it
GameEvent *GimmickShauntal_CallModel8Event(GameSystem *gsys);
// Plays one of models 1 to 5, the index-th, and sets a task to show it idle once it has played
void GimmickShauntal_PlayModelFromIndex(GameSystem *gsys, u16 index);
// The event that has the player act, opens model 0, and changes the room under a fade, and saves it. wait is the
// frames to wait before the fade
GameEvent *GimmickShauntal_CallRoomChangeEvent(GameSystem *gsys, u16 wait);
// The event that plays the animations of models 6 and 9
GameEvent *GimmickShauntal_CallModels6And9Event(GameSystem *gsys);

#endif // POKEBW2_FIELD_GIMMICK_LEAGUE_SHAUNTAL_H
