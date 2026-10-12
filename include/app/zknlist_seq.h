#ifndef POKEBW2_APP_ZKNLIST_SEQ_H
#define POKEBW2_APP_ZKNLIST_SEQ_H

#include "types.h"
#include "app/zknlist_main.h"

// zknlist_seq.c: the sequences of the Pokédex's list. The ROM doesn't name this file; zknlist_seq.c is a guess after
// the search screen's zknsearch_seq.c. The names are ours

// Runs a frame; FALSE once the screen has ended
BOOL ZknListSeq_Main(ZknListWork *wk);

#endif // POKEBW2_APP_ZKNLIST_SEQ_H
