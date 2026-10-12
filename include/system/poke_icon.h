#ifndef POKEBW2_SYSTEM_POKE_ICON_H
#define POKEBW2_SYSTEM_POKE_ICON_H

#include "types.h"
#include "struct_decls.h"

// The Pokémon icons, in ARCID_POKEICON. The ROM doesn't name the file that holds these

// The file of a Pokémon's icon, the female one if sex is 1 and the species has one
u32 PokeParty_GetIconIndex(u32 species, u32 form, u32 sex, BOOL egg);
// The palette of a Pokémon's icon
u32 PokeIcon_GetPalette(u32 species, u32 form, u32 sex, BOOL egg);
// The file and palette of a Pokémon's icon
u32 PokeIcon_GetFileOfPkm(BoxPkm *pkm);
u32 PokeIcon_GetPaletteOfPkm(BoxPkm *pkm);
// The files of the icons' palette, and of the second palette that the PC boxes load
u32 PokeIcon_GetPaletteFile(void);
u32 PokeIcon_GetAltPaletteFile(void);
// The files of the icons' cells and cell animations, by the OBJ mapping mode of the main and of the sub engine
u32 PokeIcon_GetCellFile_Main(void);
u32 PokeIcon_GetCellFile_Sub(void);
u32 getOBJTileMapping_MainEng(void);
u32 getOBJTileMapping_SubEng(void);
// A species with its form and sex in one u16: ten bits of species, four of form and two of sex, or six bits of form
// for Unown and Arceus. Unpacking gives zeros for anything past the last species
u16 PokeIcon_PackSpecies(u32 species, u32 form, u32 sex);
void PokeIcon_UnpackSpecies(int packed, u32 *species, u32 *form, u32 *sex);

#endif // POKEBW2_SYSTEM_POKE_ICON_H
