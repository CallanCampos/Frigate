#ifndef POKEBW2_SAVE_POKEDEX_H
#define POKEBW2_SAVE_POKEDEX_H

#include "types.h"
#include "gfl/heap.h"
#include "struct_decls.h"

BOOL PokeDex_IsNationalObtained(PokeDexSave *pokedex);
u32 getSizeofPokedexData(void);
void PokeDex_SetNationalObtained(PokeDexSave *pokedex);
void PokeDex_EnableHabitatList(PokeDexSave *pokedex);
BOOL PokeDex_IsHabitatListEnabled(PokeDexSave *pokedex);
void givePlayerPokedex(PokeDexSave *pokedex);
u32 func_0200d1dc(PokeDexSave *pokedex);
// Which Pokédex the detail screen counts: 0 the regional one, 1 the national one, 2 the national one once obtained
u32 func_0200d1f8(PokeDexSave *pokedex);
// Sets which Pokédex the detail screen counts, with func_0200d1f8's values
void func_0200d1e4(PokeDexSave *pokedex, u32 mode);
BOOL PokeDex_IsCaught(PokeDexSave *pokedex, u16 species);
BOOL PokeDex_IsSeen(PokeDexSave *pokedex, u16 species);
// The species that the Pokédex's list was last on, and sets it
void func_0200d200(PokeDexSave *pokedex, u16 species);
u16 func_0200d214(PokeDexSave *pokedex);
// Sets the sex, shininess and form that the Pokédex shows of a species
void addToDex(PokeDexSave *pokedex, u16 species, u32 sex, BOOL rare, u32 form);
// The forms of a species, 1 for one without any
u16 getNumberOfForms(u16 species);
// The sex, shininess and form that the Pokédex shows of a species
void func_0200d3c8(PokeDexSave *pokedex, u16 species, u32 *sex, u32 *rare, u32 *form, HeapID heapId);
// Whether the Pokédex has seen the species in that sex, shininess and form
BOOL func_0200d8d4(PokeDexSave *pokedex, u16 species, u32 sex, BOOL rare, u32 form);
// The personality of the Spinda the Pokédex shows, for index 0
u32 func_0200da18(PokeDexSave *pokedex, u32 index);
// Whether the Pokédex has the species' entry in a language
BOOL func_0200db5c(PokeDexSave *pokedex, u16 species, u32 language);
u32 PokeDex_GetSeenNoNational(PokeDexSave *pokedex);
u32 PokeDex_GetCaughtNoNational(PokeDexSave *pokedex);
void PokeDex_RegistPkm(PokeDexSave *pokedex, PartyPkm *pkm);
void addPkmToDex(PokeDexSave *pokedex, PartyPkm *pkm);
// The count of seen Pokémon, in the national Pokédex once the player has it
u32 countSeenDexPokes(PokeDexSave *pokedex, HeapID heapId);
BOOL PokeDex_IsCompleteNational(PokeDexSave *pokedex);

#endif // POKEBW2_SAVE_POKEDEX_H
