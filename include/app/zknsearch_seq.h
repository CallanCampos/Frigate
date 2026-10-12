#ifndef POKEBW2_APP_ZKNSEARCH_SEQ_H
#define POKEBW2_APP_ZKNSEARCH_SEQ_H

#include "types.h"
#include "app/zknsearch_main.h"

// zknsearch_seq.c: the sequences of the Pokédex's search screen. The ROM doesn't name this file; zknsearch_seq.c is a
// guess after zknsearch_main.c. The names are ours

// Runs a frame; FALSE once the screen has ended
BOOL ZknSearchSeq_Main(ZknSearchWork *wk);

#endif // POKEBW2_APP_ZKNSEARCH_SEQ_H
