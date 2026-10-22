#ifndef POKEBW2_DWC_GT2AUTH_H
#define POKEBW2_DWC_GT2AUTH_H

#include "dwc/gt2Main.h"

// The challenges of GameSpy's GT2 (gt2Auth.c), in overlay 11, which the two sides of a connection send each other in
// its handshake: a challenge is 32 random characters from '!' to '}' whose parities follow from the ones before, and
// the response to one mixes it with the key below, except at 0 and 13, which are random

// The key that the responses are made with
extern char GT2ChallengeKey[33];

GT2Byte *gti2GetChallenge(GT2Byte *buffer);
GT2Byte *gti2GetResponse(GT2Byte *buffer, const GT2Byte *challenge);
// Whether two responses are the same, apart from their random characters
GT2Bool gti2CheckResponse(const GT2Byte *response1, const GT2Byte *response2);

#endif // POKEBW2_DWC_GT2AUTH_H
