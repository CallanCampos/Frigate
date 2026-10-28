#ifndef POKEBW2_APP_POKEDEX_AREA_LIST_H
#define POKEBW2_APP_POKEDEX_AREA_LIST_H

#include "types.h"
#include "app/pokedex_area_list_view.h"
#include "app/pokedex_common.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/tcb.h"
#include "struct_decls.h"
#include "system/wordset.h"

// pokedex_area_list.c: the Pokédex's list of the areas whose habitats it has (overlay 304), under the heading
// "HABITAT LIST": each area's row has a mark for each kind of habitat, and choosing one opens the area's habitats
// (overlay 303). Buttons at the left go to the Pokédex's own lists. The ROM names neither of the overlay's files;
// the names are guesses after what they do, and the names of the functions are ours

// What the screen returns in PokedexHabitatSummaryParam's result
enum {
    POKEDEX_AREA_LIST_RESULT_EXIT,
    POKEDEX_AREA_LIST_RESULT_RETURN,
    POKEDEX_AREA_LIST_RESULT_AREA,
    POKEDEX_AREA_LIST_RESULT_NATIONAL,
    POKEDEX_AREA_LIST_RESULT_REGIONAL,
};

// 0x740 bytes
typedef struct {
    BOOL end;
    u32 state;
    u32 step;
    PokedexHabitatSummaryParam *param;
    PokedexAreaListView view;
    MsgData *msgData;
    TCB *vblankTask;
    TCB *hblankTask;
    BOOL nationalObtained;
    WordSet *wordSet;
    u32 unk718;
    PokedexHabitatSave *habitatSave;
    // 9 for a button whose animation the next state does not wait for, else 8
    u32 stateParam;
    // What the list returned this frame
    s32 listResult;
    // The button that was pressed, the animation it plays, and the state that follows it
    u8 buttonAnim;
    u8 buttonActor;
    u32 nextState;
    u32 nextStateParam;
    u8 blinkTimer;
    // How far the touch bar and the upper screen's heading are from their places
    s32 touchbarOffset;
    s32 headingOffset;
} PokedexAreaListWork;

extern const GameProcFunctions PokedexAreaList_ProcFunctions;

#endif // POKEBW2_APP_POKEDEX_AREA_LIST_H
