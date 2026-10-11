#ifndef POKEBW2_APP_GDS_RAP_H
#define POKEBW2_APP_GDS_RAP_H

#include "types.h"
#include "struct_decls.h"

// The Global Data Storage client (overlay 201's gds_rap.c, which the ROM's string names), which the Global Link's
// battle videos and musical photos go through. Only what its users call is declared; its work is the start of the
// work they allocate, and it keeps the rest of its state in the overlay

// A function the client calls with its work once a request has finished
typedef void (*GdsRapCallback)(void *work);

// What func_ov201_021b5420 is started with
typedef struct {
    // The player's GameSpy profile ID
    s32 profileId;
    u32 unk4;
    GameData *gameData;
    u32 unkC;
    void *work;
    GdsRapCallback onUploadMusicalShotEnd;
    GdsRapCallback onDownloadMusicalShotEnd;
    GdsRapCallback onUploadVideoEnd;
    GdsRapCallback onDownloadVideoListEnd;
    GdsRapCallback onDownloadVideoEnd;
    GdsRapCallback onUnk5d8End;
} GdsRapInit;

// The result of the last request
typedef struct {
    BOOL isValid;
    u32 type;
    s32 code;
    u32 detail;
} GdsRapResult;

void func_ov201_021b5420(void *work, const GdsRapInit *init);
void func_ov201_021b5534(void *work);
BOOL func_ov201_021b558c(void *work, GdsProfile *profile, void *data);
BOOL func_ov201_021b55d8(void *work, u16 species);
BOOL func_ov201_021b5600(void *work, GdsProfile *profile);
BOOL func_ov201_021b56b0(void *work, u16 a1, u32 a2, u8 a3, u8 a4);
BOOL func_ov201_021b5730(void *work);
BOOL func_ov201_021b5754(void *work);
BOOL func_ov201_021b577c(void *work);
BOOL func_ov201_021b57a4(void *work, u32 a1, u32 a2);
BOOL func_ov201_021b57d0(void *work, u32 a1, u32 a2);
void func_ov201_021b57fc(void *work);
u32 func_ov201_021b5ef8(void *work, int a1, int event, int a3);
// Gives the result of the last request in result, and whether there is one
BOOL func_ov201_021b60c4(void *work, GdsRapResult **result);
// Clears the result
void func_ov201_021b60d0(void *work);
BOOL func_ov201_021b60e4(void *work);
// Copy the downloaded entries to list, up to max of them, and give their number
int func_ov201_021b614c(void *work, void *list, int max);
u64 func_ov201_021b61b8(void *work);
int func_ov201_021b61ec(void *work, void *list, int max);
u32 func_ov201_021b62b4(void *work, void **data);

#endif // POKEBW2_APP_GDS_RAP_H
