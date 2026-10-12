#ifndef POKEBW2_APP_PKWD_BOOK_H
#define POKEBW2_APP_PKWD_BOOK_H

// Pokéstar Studios' scenario book, overlay 322: the list of the movies the player can film, with the plot, the
// mission and the cast of each, to pick the one to film. The ROM names only "pkwd_book_graphic.c"; pkwd_book.c is our
// guess for the screen's own file. Overlay 23's event_pokewood.c runs it

#include "types.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "struct_decls.h"

#define OVERLAY_PKWD_BOOK OVERLAY_ID(322)

// The heap of the screen (ours, not from swan)
#define HEAPID_PKWD_BOOK 0x9f

// What the screen leaves in the result when the player backs out, rather than a movie
#define PKWD_BOOK_CANCEL (-1)

typedef struct {
    PlayerInfo *playerInfo;
    PokewoodSave *save;
    // The movie that was picked, or PKWD_BOOK_CANCEL. May be NULL
    s16 *result;
} PkwdBookParam;

extern const GameProcFunctions PKWD_BOOK_PROC_FUNCTIONS;

#endif // POKEBW2_APP_PKWD_BOOK_H
