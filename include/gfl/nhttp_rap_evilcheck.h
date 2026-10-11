#ifndef POKEBW2_GFL_NHTTP_RAP_EVILCHECK_H
#define POKEBW2_GFL_NHTTP_RAP_EVILCHECK_H

#include "types.h"
#include "gfl/heap.h"

// nhttp_rap_evilcheck.c in overlay 189, which the ROM names: signs data, and the allocations around it, for the check
// of Pokémon that nhttp_rap.c sends. The names are not known

// Signs count elements of size bytes with the key of the server's check, in a heap that the signing allocates from
int NHttpRapEvilCheck_Sign(const void *data, u32 size, u32 count, void *signature, HeapID heapId);
// Allocates count elements of size bytes in the heap, cleared
void *NHttpRapEvilCheck_AllocArray(u32 size, u32 count, HeapID heapId);
void NHttpRapEvilCheck_FreeArray(void *ptr);
// Copies the element of size bytes to an index of the array
void NHttpRapEvilCheck_SetElement(void *array, const void *element, u32 size, u32 index);

#endif // POKEBW2_GFL_NHTTP_RAP_EVILCHECK_H
