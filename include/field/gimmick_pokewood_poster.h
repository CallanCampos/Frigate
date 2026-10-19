#ifndef POKEBW2_FIELD_GIMMICK_POKEWOOD_POSTER_H
#define POKEBW2_FIELD_GIMMICK_POKEWOOD_POSTER_H

#include "types.h"
#include "struct_decls.h"

// The movie posters of Pokéstar Studios (Pokewood), overlay 136: a model whose four poster textures show the series of
// the last four movies filmed. Overlay 36's gimmick table calls these
void GimmickPokewoodPoster_Setup(Field *field);
void GimmickPokewoodPoster_End(Field *field);
void GimmickPokewoodPoster_Update(Field *field);

#endif // POKEBW2_FIELD_GIMMICK_POKEWOOD_POSTER_H
