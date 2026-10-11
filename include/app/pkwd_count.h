#ifndef POKEBW2_APP_PKWD_COUNT_H
#define POKEBW2_APP_PKWD_COUNT_H

// The opening of a Pokéstar Studios movie, overlay 321: a film leader that counts down, then the movie's title card.
// The ROM names pkwd_count_graphic.c; pkwd_count.c, the file before it, is a guess from it. Overlay 23's
// event_pokewood.c runs it

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

#define OVERLAY_PKWD_COUNT OVERLAY_ID(321)

// The parameters of the title card
typedef struct {
    u32 unk0;
    u32 movie;
    StrBuf *title;
    u32 gender;
} PokewoodTitleParam;

extern const GameProcFunctions PKWD_COUNT_PROC_FUNCTIONS;

#endif // POKEBW2_APP_PKWD_COUNT_H
