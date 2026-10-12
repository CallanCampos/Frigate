#ifndef POKEBW2_APP_WIFI_P2PMATCHROOM_H
#define POKEBW2_APP_WIFI_P2PMATCHROOM_H

// The room of the Wi-Fi Club's match board (overlay 173), where the player and the friends who are online walk around
// on the 2D map of overlay 203. The ROM has no name for the file: it and its names follow the older games'
// wifi_p2pmatchroom.c (ours, not swan's)

#include "types.h"
#include "app/wf2dmap/wf2dmap_cmdq.h"
#include "app/wf2dmap/wf2dmap_common.h"
#include "app/wf2dmap/wf2dmap_map.h"
#include "app/wf2dmap/wf2dmap_obj.h"
#include "app/wf2dmap/wf2dmap_objdraw.h"
#include "app/wf2dmap/wf2dmap_scrdraw.h"
#include "app/wf2dmap/wf2dmap_scroll.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "nnsys/g2d.h"

// The match board's graphics archive, which arc.h has no name for yet
#define ARCID_WIFIP2PMATCH 32

// How many characters the room holds: the player and the friends
#define WIFI_MCR_OBJ_MAX 48
// How many friends a block of the room seats, and so how many each of the room's maps adds
#define WIFI_MCR_BLOCK_FRIEND_NUM 8
// The PCs of the room, one per block
#define WIFI_MCR_PC_NUM 4

// What a square of the room's maps is (its WF2DMAP_MAP param)
enum {
    WIFI_MCR_MAPPM_NONE,
    WIFI_MCR_MAPPM_WALL,
    WIFI_MCR_MAPPM_PC,
    // The square under a friend's seat tells the block it belongs to
    WIFI_MCR_MAPPM_BLOCK0,
    WIFI_MCR_MAPPM_BLOCK1,
    WIFI_MCR_MAPPM_BLOCK2,
    WIFI_MCR_MAPPM_BLOCK3,
    // The seats of a block's friends
    WIFI_MCR_MAPPM_FRIEND0,
    WIFI_MCR_MAPPM_FRIEND1,
    WIFI_MCR_MAPPM_FRIEND2,
    WIFI_MCR_MAPPM_FRIEND3,
    WIFI_MCR_MAPPM_FRIEND4,
    WIFI_MCR_MAPPM_FRIEND5,
    WIFI_MCR_MAPPM_FRIEND6,
    WIFI_MCR_MAPPM_FRIEND7,
    // Where the player comes in and leaves
    WIFI_MCR_MAPPM_EXIT,
};

// What WIFI_MCR_Main returns
enum {
    WIFI_MCR_RET_NONE,
    // Down was held on the exit
    WIFI_MCR_RET_CANCEL,
    // A was pressed in front of a friend: WIFI_MCR_PlayerSelect tells which
    WIFI_MCR_RET_SELECT,
    // A was pressed in front of a PC
    WIFI_MCR_RET_MYSELECT,
};

// How a friend's character behaves
enum {
    // Turns to a random way now and then
    WIFI_MCR_NPC_MOVE_NORMAL,
    // Jumps on the spot
    WIFI_MCR_NPC_MOVE_JUMP,
};

// What a character of the room is doing
enum {
    WIFI_MCR_MOVESTATE_PLAYER,
    WIFI_MCR_MOVESTATE_NPC_IN,
    WIFI_MCR_MOVESTATE_NPC_OUT,
    WIFI_MCR_MOVESTATE_NPC_NORMAL,
    WIFI_MCR_MOVESTATE_NPC_JUMP,
    WIFI_MCR_MOVESTATE_NPC_PAUSE,
};

typedef struct WIFI_MATCHROOM WIFI_MATCHROOM;
typedef struct MCR_MOVEOBJ MCR_MOVEOBJ;

// Returns TRUE when the character is to be deleted
typedef BOOL (*MCR_MOVEFUNC)(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
typedef void (*MCR_DRAWFUNC)(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);

// A character of the room
struct MCR_MOVEOBJ {
    WF2DMAP_OBJWK *obj;
    WF2DMAP_OBJDRAWWK *draw;
    s16 moveCount;
    // The friend the player pressed A in front of, or 0
    u8 accessFriend;
    // WIFI_MCR_NPC_MOVE_*
    u8 moveId;
    // WIFI_MCR_MOVESTATE_*, and what it was before the pause
    u16 moveState;
    u16 moveStateTmp;
    MCR_MOVEFUNC move;
    // What move was before the pause
    MCR_MOVEFUNC moveTmp;
    MCR_DRAWFUNC drawFunc;
};

// The cell actors of the room: the arrow on the exit and the cursor over a friend
typedef struct {
    ClActUnit *unit;
    ClActRenderer *renderer;
    u32 chars;
    u32 palette;
    u32 cellAnims;
    ClActor *exitCursor;
    ClActor *cursor;
} MCR_CLACT;

// The palette animation of the PCs' screens
typedef struct {
    void *buffer;
    NNSG2dPaletteData *palette;
    // A bit per PC that nobody uses, whose screen blinks
    u8 freeBits;
    u8 frame;
    s16 wait;
    // The PC the player is using, and the step of its animation
    u8 pcNo;
    u8 seq;
    s16 seqWait;
    // Set to go on from a step that waits
    u8 nextReq;
} MCR_PCANM;

struct WIFI_MATCHROOM {
    BOOL init;
    u32 heapId;
    u32 unk8;
    // Whether the player's keys are ignored
    BOOL playerPause;
    // Never set
    void *unk10;
    MCR_CLACT clact;
    MCR_MOVEOBJ objs[WIFI_MCR_OBJ_MAX];
    MCR_MOVEOBJ *player;
    MCR_PCANM pcAnm;
    WF2DMAP_MAPSYS *mapSys;
    WF2DMAP_OBJSYS *objSys;
    WF2DMAP_OBJDRAWSYS *objDrawSys;
    WF2DMAP_SCROLL scroll;
    WF2DMAP_SCRDRAW *scrDraw;
    WF2DMAP_REQCMDQ *cmdQueue;
};

// heroView is the player's character (a WF_2DC_CHARA_*), friendNum the number of friends in the friend list, which
// picks the map, and arcId the archive of arc
void WIFI_MCR_Init(WIFI_MATCHROOM *wk, u32 heapId, ArcTool *arc, u32 heroView, u32 friendNum, u32 arcId,
                   ClActUnit *unit, ClActRenderer *renderer);
void WIFI_MCR_Dest(WIFI_MATCHROOM *wk);
BOOL WIFI_MCR_GetInitFlag(const WIFI_MATCHROOM *wk);
// Returns a WIFI_MCR_RET_*
u32 WIFI_MCR_Main(WIFI_MATCHROOM *wk);
// The friend number the player pressed A in front of, or 0
u8 WIFI_MCR_PlayerSelect(const WIFI_MATCHROOM *wk);
// The WIFI_MCR_MAPPM_* of the square the player stands on, and of the square below it
u32 WIFI_MCR_GetPlayerOnMapParam(const WIFI_MATCHROOM *wk);
u32 WIFI_MCR_GetPlayerOnUnderMapParam(const WIFI_MATCHROOM *wk);
// Shows the cursor over a character, whose shadow it hides
void WIFI_MCR_CursorOn(WIFI_MATCHROOM *wk, const MCR_MOVEOBJ *obj);
void WIFI_MCR_ShadowOn(WIFI_MATCHROOM *wk, const MCR_MOVEOBJ *obj);
void WIFI_MCR_CursorOff(WIFI_MATCHROOM *wk);
void WIFI_MCR_PlayerMovePause(WIFI_MATCHROOM *wk, BOOL pause);
BOOL WIFI_MCR_PlayerMovePauseGet(const WIFI_MATCHROOM *wk);
MCR_MOVEOBJ *WIFI_MCR_SetPlayer(WIFI_MATCHROOM *wk, u32 view);
// friendNo is the friend's index in the friend list, plus 1. Returns NULL when the player stands on the friend's seat
MCR_MOVEOBJ *WIFI_MCR_SetNpc(WIFI_MATCHROOM *wk, u32 view, u32 friendNo);
// Makes the character spin away, after which it is deleted
void WIFI_MCR_DelPeopleReq(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
void WIFI_MCR_DelPeople(MCR_MOVEOBJ *obj);
u8 WIFI_MCR_GetFriendNo(const MCR_MOVEOBJ *obj);
// The way opposite to the one the character faces
u32 WIFI_MCR_GetRetWay(const MCR_MOVEOBJ *obj);
void WIFI_MCR_NpcPauseOn(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj, u32 way);
void WIFI_MCR_NpcPauseOff(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj);
// moveId is a WIFI_MCR_NPC_MOVE_*
void WIFI_MCR_NpcMoveSet(WIFI_MATCHROOM *wk, MCR_MOVEOBJ *obj, u32 moveId);
// The animation of the PC in front of the player
void WIFI_MCR_PCAnmStart(WIFI_MATCHROOM *wk);
void WIFI_MCR_PCAnmOff(WIFI_MATCHROOM *wk);
void WIFI_MCR_PCAnmMain(WIFI_MATCHROOM *wk);

#endif // POKEBW2_APP_WIFI_P2PMATCHROOM_H
