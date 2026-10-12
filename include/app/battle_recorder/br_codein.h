#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_CODEIN_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_CODEIN_H

#include "types.h"
#include "app/battle_recorder/br_res.h"
#include "app/battle_recorder/br_util.h"
#include "gfl/clact.h"
#include "gfl/str.h"
#include "struct_decls.h"

// The Battle Recorder's code input (br_codein.c, the ROM's name; overlay 268): the code input screen of overlay 205
// (app/codein.h) as a part of a Battle Recorder screen, on the lower screen, where the player types a battle video's
// number. Our names

// The groups the digits are shown in, the last one repeated after them
#define BR_CODEIN_BLOCK_COUNT 3

// What the player did
enum {
    BR_CODEIN_RESULT_NONE,
    // Went back with no digit left to erase
    BR_CODEIN_RESULT_CANCEL,
    BR_CODEIN_RESULT_DECIDE,
};

typedef struct {
    // How many digits the code has
    u32 length;
    int blocks[BR_CODEIN_BLOCK_COUNT + 1];
    u32 unk14;
    // The code typed, length characters. The original's name, from BrCodeInParam_Free's assertions
    StrBuf *strbuf;
    ClActUnit *unit;
    BrRes *res;
    // Shown where the player touches a key
    BrBallEffect *ballEff;
} BrCodeInParam;

typedef struct BrCodeInWork BrCodeInWork;

BrCodeInWork *BrCodeIn_Init(const BrCodeInParam *param, HeapID heapId);
void BrCodeIn_Main(BrCodeInWork *wk);
// BR_CODEIN_RESULT_*
int BrCodeIn_GetResult(const BrCodeInWork *wk);
// The number typed, once the result is BR_CODEIN_RESULT_DECIDE
u64 BrCodeIn_GetNumber(const BrCodeInWork *wk);
void BrCodeIn_Exit(BrCodeInWork *wk);
// blocks holds the number of digits of each group
BrCodeInParam *BrCodeInParam_Create(u32 heapId, u32 length, ClActUnit *unit, BrRes *res, BrBallEffect *ballEff,
                                    const int *blocks);
void BrCodeInParam_Free(BrCodeInParam *codein_param);

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_CODEIN_H
