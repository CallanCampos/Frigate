#ifndef POKEBW2_APP_OV337_H
#define POKEBW2_APP_OV337_H

// Overlay 337, the anti-piracy checks that overlay 12's event_battle.c loads around a battle. Its two checks are ARM
// functions that the caller checksums before calling; when the checksum is wrong, the caller calls one of the two
// fallbacks in data_ov337_02182444 instead, picked by data_ov337_02182440

#include "types.h"

typedef void *(*AntiPiracyCheck)(u32 *seed, void *work);

void DSProt_Init(void);
void *DSProt_CallRunChecks1(u32 *seed, void *work);
void *DSProt_CallRunChecks3(u32 *seed, void *work);

extern u32 data_ov337_02182440;
extern AntiPiracyCheck data_ov337_02182444[2];

#endif // POKEBW2_APP_OV337_H
