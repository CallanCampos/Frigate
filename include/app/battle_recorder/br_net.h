#ifndef POKEBW2_APP_BATTLE_RECORDER_BR_NET_H
#define POKEBW2_APP_BATTLE_RECORDER_BR_NET_H

#include "types.h"
#include "gfl/heap.h"
#include "save/save_control.h"
#include "save/musical_savedata.h"
#include "struct_decls.h"

// The Battle Recorder's connection to the Global Link (br_net.c), used by its online modes

BrNet *func_ov271_021f6224(GameData *gameData, u32 a1, HeapID heapId);
void func_ov271_021f6300(BrNet *net);
void func_ov271_021f6348(BrNet *net);

// The data of a request to the Global Link
typedef struct {
    union {
        // The musical photo upload, request 0, sends the photo
        void *data;
        // The species whose musical photos request 1 downloads
        u16 species;
    };
    u32 unk4;
} BrNetRequestParam;

// A musical photo downloaded from the Global Link, after the profile of the player who sent it
typedef struct {
    // A GdsProfile, whose layout isn't recovered yet
    u8 profile[0x80];
    MusicalShot shot;
} BrMusicalShotRecv;

// Starts a request, 0 to 8
void BrNet_StartRequest(BrNet *net, u32 type, const BrNetRequestParam *param);
// TRUE once the request has finished
BOOL BrNet_IsRequestEnd(BrNet *net);
// TRUE if the request's result has a message, which it gives in msgID
BOOL BrNet_GetResultMsg(BrNet *net, u32 *msgID);
// 2 once the connection has had an error, which ends the Battle Recorder's screens
u32 BrNet_CheckError(BrNet *net);
// TRUE once the connection has failed
BOOL func_ov271_021f66f8(BrNet *net);
// TRUE once the photos are downloaded, then giving up to tblMax of them in tbl and their number in recvNum
BOOL BrNet_GetDownloadMusicalShot(BrNet *net, BrMusicalShotRecv **tbl, int tblMax, int *recvNum);

#endif // POKEBW2_APP_BATTLE_RECORDER_BR_NET_H
