#ifndef POKEBW2_APP_BOX_SEARCH_H
#define POKEBW2_APP_BOX_SEARCH_H

#include "types.h"
#include "app/box2.h"
#include "app/box_search_graphic.h"
#include "app/ui/ui_scene.h"
#include "app/ui/touchbar.h"
#include "app/ui/frame_list.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/proc.h"
#include "gfl/str.h"
#include "struct_decls.h"
#include "system/cursor_move.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The PC box's Pokémon search, a sub proc of the box in the same overlay. The ROM doesn't name its file;
// box_search.c is named after its box_search_graphic.c

typedef struct {
    u32 unk0;
    Box2SysWork *syswk;
    Box2Param *param;
} BoxSearchParam;

#define BOX_SEARCH_WINDOW_COUNT 17
// The strings of the main menu's buttons, of "none", and two more
#define BOX_SEARCH_STRING_COUNT 9
#define BOX_SEARCH_ACTOR_COUNT 36
// The counts of the Pokémon caught, by group of species
#define BOX_SEARCH_GROUP_COUNT 50

typedef struct {
    HeapID heapId;
    BoxSearchParam *param;
    BoxSearchGraphic *graphic;
    TouchBar *touchBar;
    Font *font;
    PokeDexSave *pokedex;
    PrintQueue *printQueue;
    MsgData *msgData;
    MsgData *speciesNames;
    MsgData *abilityNames;
    MsgData *abilityInfo;
    MsgData *natureNames;
    WordSet *wordSet;
    StrBuf *strbuf;
    StrBuf *strings[BOX_SEARCH_STRING_COUNT];
    BmpWin *windows[BOX_SEARCH_WINDOW_COUNT];
    PrintWindow printWindows[BOX_SEARCH_WINDOW_COUNT];
    FrameList *list;
    // How many rows of the list show, and how many items it has
    u16 listRows;
    u16 listCount;
    UIObjRes objRes[3];
    ClActor *actors[BOX_SEARCH_ACTOR_COUNT];
    CursorMove *cursorMove;
    // The group of species or abilities chosen by first letter, from 1, and the group within it
    s16 group;
    s16 subGroup;
    // Whether an item of the list was chosen
    BOOL chosen;
    int seq;
    int nextSeq;
    // Where the main menu's cursor is
    u32 menuPos;
    // The button whose animation plays before nextSeq, or -1 for the touch bar's
    int btnActor;
    int btnAnmSeq;
    u32 caughtCounts[BOX_SEARCH_GROUP_COUNT];
} BoxSearchWork;

extern const GameProcFunctions BOX_SEARCH_PROC_FUNCTIONS;

#endif // POKEBW2_APP_BOX_SEARCH_H
