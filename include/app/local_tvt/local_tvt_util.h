#ifndef POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_UTIL_H
#define POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_UTIL_H

#include "types.h"
#include "app/local_tvt/local_tvt_common.h"
#include "app/local_tvt/local_tvt_person.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "struct_decls.h"

// local_tvt_util.c

// The part of the day by the season's hours of light
u32 LocalTvtUtil_GetTime(GameData *gameData);
// The backgrounds of both screens
LocalTvtBG *LocalTvtBG_Create(GameData *gameData, HeapID heapId, u32 swapPalette);
void LocalTvtBG_Free(LocalTvtBG *bg);

// A sequence of functions with a step each, until one ends it
typedef void (*LocalTvtSeqFunc)(LocalTvtSeq *seq, int *step, void *work);

LocalTvtSeq *LocalTvtSeq_Create(void *work, LocalTvtSeqFunc func, HeapID heapId);
void LocalTvtSeq_Free(LocalTvtSeq *seq);
void LocalTvtSeq_Main(LocalTvtSeq *seq);
BOOL LocalTvtSeq_IsEnd(LocalTvtSeq *seq);
void LocalTvtSeq_SetNext(LocalTvtSeq *seq, LocalTvtSeqFunc func);
void LocalTvtSeq_End(LocalTvtSeq *seq);

// The list of the party on the lower screen, of which the player picks a Pokémon
struct LocalTvtPokeSelectSetup {
    LocalTvtMessage *message;
    ClActUnit *unit;
    PokeParty *party;
};

// The results of the list
enum {
    LOCAL_TVT_POKE_SELECT_NONE,
    LOCAL_TVT_POKE_SELECT_DECIDE,
    LOCAL_TVT_POKE_SELECT_CANCEL,
};

LocalTvtPokeSelect *LocalTvtPokeSelect_Create(const LocalTvtPokeSelectSetup *setup, HeapID heapId);
void LocalTvtPokeSelect_Free(LocalTvtPokeSelect *select);
void LocalTvtPokeSelect_Main(LocalTvtPokeSelect *select);
void LocalTvtPokeSelect_SetActive(LocalTvtPokeSelect *select, BOOL active);
void LocalTvtPokeSelect_Restart(LocalTvtPokeSelect *select);
u32 LocalTvtPokeSelect_GetResult(LocalTvtPokeSelect *select);
PartyPkm *LocalTvtPokeSelect_GetPkm(LocalTvtPokeSelect *select);

// The menu of topics on the lower screen
struct LocalTvtMenuSetup {
    LocalTvtMessage *message;
    ClActUnit *unit;
    MsgData *msgData;
    u32 items[LOCAL_TVT_TOPIC_MAX];
    u32 count;
    u32 cursor;
};

LocalTvtMenu *LocalTvtMenu_Create(const LocalTvtMenuSetup *setup, HeapID heapId);
void LocalTvtMenu_Free(LocalTvtMenu *menu);
void LocalTvtMenu_Main(LocalTvtMenu *menu);
BOOL LocalTvtMenu_IsEnd(LocalTvtMenu *menu);
u32 LocalTvtMenu_GetSelected(LocalTvtMenu *menu);

// The touch bar of the lower screen with its return button, which the B button presses as well

LocalTvtReturnBar *LocalTvtReturnBar_Create(ClActUnit *unit, HeapID heapId);
void LocalTvtReturnBar_Free(LocalTvtReturnBar *bar);
void LocalTvtReturnBar_Main(LocalTvtReturnBar *bar);
BOOL LocalTvtReturnBar_IsPressed(LocalTvtReturnBar *bar);
// Whether the button's animation of being pressed is over
BOOL LocalTvtReturnBar_IsAnimEnd(LocalTvtReturnBar *bar);

// The input of a list by the keys or the touch screen, whichever the player used last. The cursor shows only with
// the keys

// What the callbacks of the input return
enum {
    LOCAL_TVT_INPUT_NONE,
    // The cursor moved, or the touch only picked an entry
    LOCAL_TVT_INPUT_MOVE,
    LOCAL_TVT_INPUT_DECIDE,
};

struct LocalTvtKeyTouchSetup {
    void *work;
    int (*touch)(void *work);
    int (*key)(void *work);
    // Draws the list with or without its cursor
    void (*draw)(void *work, BOOL touchMode);
};

LocalTvtKeyTouch *LocalTvtKeyTouch_Create(const LocalTvtKeyTouchSetup *setup, HeapID heapId);
void LocalTvtKeyTouch_Free(LocalTvtKeyTouch *input);
void LocalTvtKeyTouch_Main(LocalTvtKeyTouch *input);
BOOL LocalTvtKeyTouch_IsDecided(LocalTvtKeyTouch *input);

#endif // POKEBW2_APP_LOCAL_TVT_LOCAL_TVT_UTIL_H
