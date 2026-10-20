#include "dwc/gt2Auth.h"
#include "dwc/ext.h"

// GameSpy's GT2 challenges (gt2Auth.c). The ROM names neither the file nor its functions: the file, gti2GetChallenge,
// gti2GetResponse and gti2CheckResponse are GameSpy's, as pret's pokerevo Wii build has them (gt2Auth.s), and
// gti2VerifyChallenge and GT2ChallengeKey are the later SDK's

// Whether a character of a challenge has to be odd: from the parity of the one before, its index, the first
// character's and whether the first or the one before is the larger
#define GTI2_ODD_MODE(buffer, i, oddmode)                                                                              \
    (((buffer)[(i) - 1] & 1) ^ ((i) & 1) ^ (oddmode) ^ ((buffer)[0] & 1) ^ ((buffer)[0] < 79 ? 1 : 0) ^                \
     ((buffer)[(i) - 1] < (buffer)[0] ? 1 : 0))

static BOOL gti2VerifyChallenge(const u8 *buffer);

char GT2ChallengeKey[33] = "3b8dd8995f7c40a9a5c5b7dd5b481341";

static BOOL gti2VerifyChallenge(const u8 *buffer) {
    int oddmode = 0;
    int i;

    for (i = 1; i < 32; i++) {
        oddmode = GTI2_ODD_MODE(buffer, i, oddmode);
        if ((oddmode && (buffer[i] & 1) == 0) || (!oddmode && (buffer[i] & 1) == 1)) {
            return FALSE;
        }
    }
    return TRUE;
}

u8 *gti2GetChallenge(u8 *buffer) {
    int i;
    int oddmode;

    srand(current_time());
    buffer[0] = 33 + rand() % 93;
    oddmode = 0;
    for (i = 1; i < 32; i++) {
        oddmode = GTI2_ODD_MODE(buffer, i, oddmode);
        buffer[i] = 33 + rand() % 93;
        if ((oddmode && (buffer[i] & 1) == 0) || (!oddmode && (buffer[i] & 1) == 1)) {
            buffer[i]++;
        }
    }
    return buffer;
}

u8 *gti2GetResponse(u8 *buffer, const u8 *challenge) {
    int i;
    BOOL valid;
    char cchar;
    int keylen = strlen(GT2ChallengeKey);
    int chalrand;

    valid = gti2VerifyChallenge(challenge);
    for (i = 0; i < 32; i++) {
        if (!valid || i == 0 || i == 13) {
            buffer[i] = 33 + rand() % 93;
        } else {
            if (i == 1 || i == 14) {
                cchar = challenge[i];
            } else {
                cchar = challenge[i - 1];
            }
            chalrand = abs(challenge[(i * challenge[i] + GT2ChallengeKey[(i + challenge[i]) % keylen]) % 32] ^
                           GT2ChallengeKey[i * 17991 * cchar % keylen]);
            buffer[i] = 33 + chalrand % 93;
        }
    }
    return buffer;
}

BOOL gti2CheckResponse(const u8 *response1, const u8 *response2) {
    int i;

    for (i = 0; i < 32; i++) {
        if (i != 0 && i != 13 && response1[i] != response2[i]) {
            return FALSE;
        }
    }
    return TRUE;
}
