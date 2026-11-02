#ifndef POKEBW2_APP_COMM_TVT_CTVT_GAME_TARGET_H
#define POKEBW2_APP_COMM_TVT_CTVT_GAME_TARGET_H

#include "types.h"
#include "gfl/g3d.h"
#include "gfl/tcb.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// What a hit on a target scored: by the target's own member or by another, before or after the hurry. Each is an
// actor of scene 4, the effects of the hits, in this order
enum {
    CTVT_TARGET_HIT_OWN,
    CTVT_TARGET_HIT_OWN_HURRY,
    CTVT_TARGET_HIT_OTHER,
    CTVT_TARGET_HIT_OTHER_HURRY,
};

// A face that floats up in the target game. Its scene is its member's, whose actors are the face as it is, as its
// member hit it and as another hit it; scene 4 holds the effects of the hits
struct CtvtGameTarget {
    BOOL active;
    TCB *tcb;
    CtvtGame *game;
    G3DManager *g3d;
    u16 scene;
    SRTMatrix srt;
    // 0 until it is hit, then 1 by its member, 2 by another
    int actor;
    // 1 to 3, the nearest
    int depth;
    s8 alpha;
    fx32 hitFrame;
    BOOL hitAnimating;
    fx32 grow;
    // A CTVT_TARGET_HIT_*, what the hit scored
    u32 hitKind;
    VecFx32 velocity;
    // The last frame of the host that it moved for
    u16 frame;
    s8 fadeFrames;
    u8 hitPending;
};

void CtvtGameTarget_Init(CtvtGameTarget *target);
void CtvtGameTarget_Delete(CtvtGameTarget *target);
// Moves the target for each of the host's frames since the last update, and deletes it once it has left the screen
void CtvtGameTarget_Update(CtvtGameTarget *target);
void CtvtGameTarget_Draw(CtvtGameTarget *target);

#endif // POKEBW2_APP_COMM_TVT_CTVT_GAME_TARGET_H
