#ifndef POKEBW2_FIELD_INTRUDE_WORK_H
#define POKEBW2_FIELD_INTRUDE_WORK_H

// Overlay 12's intrude_work.c (the name is a guess, after Black and White's intrusion functions): what the field asks
// of the communication about other players. Most of it is left as stubs that report no one. getGameOrigin and
// getSeasonFromPlayerData are swan's names

#include "types.h"
#include "struct_decls.h"

// Whether the season may change, which it may not while it is kept in sync with another player
BOOL func_ov012_021535dc(GameSystem *gsys);
void *func_ov012_02153608(GameCommSys *commSys);
// The game the other player plays, this one's
u32 getGameOrigin(GameCommSys *commSys);
u8 getSeasonFromPlayerData(GameCommSys *commSys);
u32 func_ov012_0215364c(GameCommSys *commSys, GameData *gameData);
u32 func_ov012_02153650(GameCommSys *commSys);
void func_ov012_02153654(GameCommSys *commSys);
u32 func_ov012_02153658(GameCommSys *commSys);
u32 func_ov012_0215365c(void);
u32 func_ov012_02153660(void);
u32 func_ov012_02153664(GameCommSys *commSys);
void func_ov012_02153668(GameCommSys *commSys);

#endif // POKEBW2_FIELD_INTRUDE_WORK_H
