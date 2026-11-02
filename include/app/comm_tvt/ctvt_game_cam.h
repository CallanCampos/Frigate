#ifndef POKEBW2_APP_COMM_TVT_CTVT_GAME_CAM_H
#define POKEBW2_APP_COMM_TVT_CTVT_GAME_CAM_H

#include "types.h"
#include "app/comm_tvt/ctvt_game.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// The minigames' camera: the screen that takes this machine's pictures before a game, with the DSi's camera or from
// a roulette of characters without one, and swaps them with the others in the call. The pictures go on this
// machine's face or balloon

struct CtvtGameCam {
    BOOL cameraEnabled;
    // The character whose pictures are this machine's without a camera
    u8 character;
    // This machine's pictures
    void *pictures[3];
    CtvtGameMember members[3];
    CameraSystem *cameraSystem;
    // CTVT_GAME_TYPE_*
    int type;
};

// What CtvtGameCam_UpdateTask returns
enum {
    CTVT_GAME_CAM_RUNNING,
    // The pictures are taken and exchanged
    CTVT_GAME_CAM_DONE,
    // Someone left the call
    CTVT_GAME_CAM_ERROR,
};

// Starts taking the pictures
CtvtGameCamTask *CtvtGameCam_StartTask(CtvtGameCam *cam, HeapID heapId);
// A CTVT_GAME_CAM_*
int CtvtGameCam_UpdateTask(CtvtGameCamTask *task);
void CtvtGameCam_EndTask(CtvtGameCamTask *task);
// type is the game's, CTVT_GAME_TYPE_*, and members the others in the call
CtvtGameCam *CtvtGameCam_Create(HeapID heapId, BOOL cameraEnabled, int type, u8 character, CtvtGameMember *members,
                                CameraSystem *cameraSystem);
void CtvtGameCam_Delete(CtvtGameCam *cam);

#endif // POKEBW2_APP_COMM_TVT_CTVT_GAME_CAM_H
