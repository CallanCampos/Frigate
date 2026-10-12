#ifndef POKEBW2_SAVE_HALL_OF_FAME_H
#define POKEBW2_SAVE_HALL_OF_FAME_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "nitro/rtc.h"
#include "struct_decls.h"

// The Hall of Fame, extra save block 8: the last 15 teams that entered it, which s00EA_HOFCheckIntegrity checks and
// overlay 256 shows from the PC. Block 0x40 of the main save has the size of one team
// (hall_of_fame_savedata.c, a descriptive name).

#define HALL_OF_FAME_TEAMS 15
#define HALL_OF_FAME_TEAM_SIZE 6
#define HALL_OF_FAME_MAX_COUNT 9999
#define HALL_OF_FAME_NICKNAME_LENGTH 11
#define HALL_OF_FAME_TRAINER_NAME_LENGTH 8

// A Pokémon of a team, as it is saved
typedef struct {
    u16 species;
    u8 level;
    u8 form : 6;
    u8 sex : 2;
    u32 personality;
    u32 trainerId;
    u16 nickname[HALL_OF_FAME_NICKNAME_LENGTH];
    u16 trainerName[HALL_OF_FAME_TRAINER_NAME_LENGTH];
    u16 moves[4];
    u16 padding;
} HallOfFameSavePokemon;

// A team and the date it entered the Hall of Fame. A slot of the team is free when its species is 0
typedef struct {
    HallOfFameSavePokemon pokemon[HALL_OF_FAME_TEAM_SIZE];
    u16 year;
    u8 month;
    u8 day;
} HallOfFameTeam;

typedef struct HallOfFameSave {
    HallOfFameTeam teams[HALL_OF_FAME_TEAMS];
    // The team that the next entry overwrites
    u32 next;
    // How many times the player entered the Hall of Fame, up to 9999
    u32 count;
} HallOfFameSave;

// A Pokémon of a team, as HallOfFameSave_GetPokemon copies it out. The names are loaded into the caller's buffers
typedef struct {
    StrBuf *nickname;
    StrBuf *trainerName;
    u32 personality;
    u32 trainerId;
    u16 species;
    u8 level;
    u8 form : 6;
    u8 sex : 2;
    u16 moves[4];
} HallOfFamePokemon;

// The size and the initial state of the Hall of Fame, and of block 0x40's single team
u32 HallOfFameSave_GetSize(void);
u32 HallOfFameTeam_GetSize(void);
void HallOfFameSave_Init(HallOfFameSave *save);
void HallOfFameTeam_Init(HallOfFameTeam *team);
// Adds a team to the Hall of Fame, dated date, with its strings in the heap
void HallOfFameSave_AddTeam(HallOfFameSave *save, PokeParty *party, const RTCDate *date, HeapID heapId);
// A team's number of Pokémon, one of its Pokémon and its date
s32 HallOfFameTeam_GetPokemonCount(HallOfFameTeam *team);
void HallOfFameTeam_GetPokemon(const HallOfFameTeam *team, s32 slot, HallOfFamePokemon *pokemon);
void HallOfFameTeam_GetDate(const HallOfFameTeam *team, RTCDate *date);
// The number of teams, at most 15
u32 HallOfFameSave_GetTeamCount(HallOfFameSave *save);
// Which entry into the Hall of Fame a team was, counting back from the newest
u32 HallOfFameSave_GetEntryNumber(HallOfFameSave *save, u32 team);
// The number of Pokémon in a team, one of its Pokémon and its date, counting back from the newest
s32 HallOfFameSave_GetPokemonCount(HallOfFameSave *save, s32 team);
void HallOfFameSave_GetPokemon(HallOfFameSave *save, s32 team, s32 slot, HallOfFamePokemon *pokemon);
void HallOfFameSave_GetDate(HallOfFameSave *save, s32 team, RTCDate *date);

#endif // POKEBW2_SAVE_HALL_OF_FAME_H
