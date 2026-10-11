#ifndef POKEBW2_SAVE_HALL_OF_FAME_H
#define POKEBW2_SAVE_HALL_OF_FAME_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "nitro/rtc.h"
#include "struct_decls.h"

// The Hall of Fame, extra save block 8: the last 15 teams that entered it, which s00EA_HOFCheckIntegrity checks and
// overlay 256 shows from the PC

typedef struct HallOfFameSave HallOfFameSave;

// A Pokémon of a team, as func_0200f69c copies it out. The names are loaded into the caller's buffers
typedef struct {
    StrBuf *nickname;
    StrBuf *trainerName;
    u32 unk08;
    u32 unk0C;
    u16 species;
    u8 level;
    u8 unk13_0 : 6;
    u8 unk13_6 : 2;
    u16 moves[4];
} HallOfFamePokemon;

// Adds a team to the Hall of Fame, dated date, with its strings in the heap
void func_0200f57c(HallOfFameSave *save, PokeParty *party, const RTCDate *date, HeapID heapId);
// The number of teams, at most 15
u32 func_0200f660(HallOfFameSave *save);
// The number of Pokémon in a team, counting back from the newest
s32 func_0200f67c(HallOfFameSave *save, u32 team);
void func_0200f69c(HallOfFameSave *save, u32 team, u32 slot, HallOfFamePokemon *pokemon);

#endif // POKEBW2_SAVE_HALL_OF_FAME_H
