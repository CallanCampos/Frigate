#ifndef POKEBW2_DWC_GT2AUTH_H
#define POKEBW2_DWC_GT2AUTH_H

#include "types.h"

// The challenges of GameSpy's GT2 (gt2Auth.c), in overlay 11, which the two sides of a connection send each other in
// its handshake: a challenge is 32 random characters from '!' to '}' whose parities follow from the ones before, and
// the response to one mixes it with the key below, except at 0 and 13, which are random

// The key that the responses are made with
extern char GT2ChallengeKey[33];

u8 *gti2GetChallenge(u8 *buffer);
u8 *gti2GetResponse(u8 *buffer, const u8 *challenge);
// Whether two responses are the same, apart from their random characters
BOOL gti2CheckResponse(const u8 *response1, const u8 *response2);

#endif // POKEBW2_DWC_GT2AUTH_H
