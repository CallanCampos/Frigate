#ifndef POKEBW2_GFL_NHTTP_RAP_H
#define POKEBW2_GFL_NHTTP_RAP_H

#include "types.h"
#include "dwc/nhttp.h"
#include "gfl/heap.h"

// nhttp_rap.c in overlay 189, which the ROM names: Game Freak's wrapper of HTTP requests to the game's servers, such as
// the check of a Pokémon before the Global Trade Station takes it. None of its functions has a name yet; what they do
// is read off the Global Trade Station's calls. A request takes the player's login buffer, which holds the token that
// the servers want, and sends one of the table of requests, or the check of Pokémon data written with the calls below

typedef struct NHttpRap NHttpRap;

// Sets up the request at an index of overlay 189's table of URLs, such as Unova Link's requests to the Pokémon Dream
// Radar's server and Game Sync's, and sends it; and one with an ID
BOOL NHttpRap_SendRequest(u32 index, NHttpRap *rap);
BOOL NHttpRap_SendRequestWithId(u32 index, u32 id, NHttpRap *rap);

// The NHTTP request of the last one sent
int NHttpRap_GetConnection(NHttpRap *rap);
// The request's error, 0 when it started
int NHttpRap_StartRequest(NHttpRap *rap);
// Ends the request
void NHttpRap_EndRequest(NHttpRap *rap);
// 0 once the request ended, 15 while it runs, or an error
int NHttpRap_Poll(NHttpRap *rap);
// The buffer that receives the answer
void *NHttpRap_GetAnswer(NHttpRap *rap);
// Allocates a request in the heap, for a player's profile ID and the login buffer
NHttpRap *NHttpRap_Create(HeapID heapId, s32 profileId, void *loginBuffer);
void NHttpRap_Destroy(NHttpRap *rap);
// Allocates the data of a request of the given type with room for size bytes, and starts it with the token
void NHttpRap_BeginPost(NHttpRap *rap, HeapID heapId, u32 size, int type);
// Adds data to the request
void NHttpRap_AddPostData(NHttpRap *rap, const void *data, u32 size);
// Sends the check of the data that was added
BOOL NHttpRap_SendValidate(NHttpRap *rap);
// Frees the request's data
void NHttpRap_FreePostData(NHttpRap *rap);
// The HTTP status of the answer
int NHttpRap_GetStatus(NHttpRap *rap);
// Sets the buffer that receives the answer, which a new request has of its own
void NHttpRap_SetAnswerBuffer(NHttpRap *rap, void *buffer, u32 size);
// Gives the request the buffer of its own again
void NHttpRap_ResetAnswerBuffer(NHttpRap *rap);

// The answer to a Pokémon's check: its status, each Pokémon's result and its signature
u8 NHttpRap_GetCheckStatus(const void *body);
u32 NHttpRap_GetCheckResult(const void *body, int index);
void *NHttpRap_GetCheckSignature(void *body, int index);

#endif // POKEBW2_GFL_NHTTP_RAP_H
