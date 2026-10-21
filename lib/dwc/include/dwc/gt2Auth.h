#ifndef POKEBW2_DWC_GT2AUTH_H
#define POKEBW2_DWC_GT2AUTH_H

#include "dwc/nonport.h"

// The challenges of GameSpy's GT2 (gt2Auth.c), in overlay 11, which the two sides of a connection send each other in
// its handshake: a challenge is 32 random characters from '!' to '}' whose parities follow from the ones before, and
// the response to one mixes it with the key below, except at 0 and 13, which are random

// The key that the responses are made with
extern char GT2ChallengeKey[33];

gsi_u8 *gti2GetChallenge(gsi_u8 *buffer);
gsi_u8 *gti2GetResponse(gsi_u8 *buffer, const gsi_u8 *challenge);
// Whether two responses are the same, apart from their random characters
gsi_bool gti2CheckResponse(const gsi_u8 *response1, const gsi_u8 *response2);

#endif // POKEBW2_DWC_GT2AUTH_H
