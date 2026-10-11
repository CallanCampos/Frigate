#ifndef POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_LOCAL_H
#define POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_LOCAL_H

#include "types.h"
#include "app/ui/frame_list.h"
#include "app/t_download/t_download_graphic.h"
#include "field/wbt.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "struct_decls.h"
#include "system/app_printsys_common.h"
#include "system/app_taskmenu.h"
#include "system/printsys.h"
#include "system/time_icon.h"
#include "system/wordset.h"

// The work of the downloaded tournaments, which t_download.c, t_download_util.c and t_download_save.c share. The
// names are ours

// The screens, each with its own init, main and exit
enum {
    // The tournaments kept in the save's slots
    T_DOWNLOAD_MODE_SAVED,
    // The tournaments received over Wi-Fi
    T_DOWNLOAD_MODE_RECEIVED,
    // The slots, to pick a tournament to enter
    T_DOWNLOAD_MODE_SELECT,
    // The details of a tournament: its Pokémon, its items and its rules, a tab each
    T_DOWNLOAD_MODE_DETAIL,
    T_DOWNLOAD_MODE_COUNT,
};

#define T_DOWNLOAD_WINDOW_COUNT 17
#define T_DOWNLOAD_SAVE_SLOTS 3
#define T_DOWNLOAD_RECEIVED_MAX 12
#define T_DOWNLOAD_TABS 3

// The cell actors' kinds, each with its own array in the work
enum {
    T_DOWNLOAD_ACTOR_OBJ,
    T_DOWNLOAD_ACTOR_BUTTON,
};

#define T_DOWNLOAD_OBJ_COUNT 9
#define T_DOWNLOAD_BUTTON_COUNT 6

struct TDownloadWork {
    HeapID heapId;
    // The cell resources: the touch bar's characters, palette and cells, then the screen's own
    u32 res[6];
    ClActor *buttons[T_DOWNLOAD_BUTTON_COUNT];
    ClActor *actors[T_DOWNLOAD_OBJ_COUNT];
    TDownloadGraphic *graphic;
    WbtOv326Param2 *param;
    Font *font;
    PrintQueue *printQueue;
    MsgData *msgData[2];
    PrintWindow windows[T_DOWNLOAD_WINDOW_COUNT];
    PrintStream *stream;
    TCBExManager *tcbEx;
    StrBuf *streamStr;
    KeyCursor *keyCursor;
    AppPrintsysCommon printWait;
    WordSet *wordSet;
    AppTaskMenu *taskMenu;
    AppTaskMenuItem menuItems[3];
    AppTaskMenuRes *taskMenuRes;
    WaitIcon *waitIcon;
    // The step of the screen's sequence, and the screen
    int seq;
    int mode;
    u32 flags;
    u32 objFlags;
    u32 unk154;
    // The screen to go back to from the details, or -1 to end
    int returnMode;
    int tab;
    // The scroll of the background, in 1/256 pixels
    int bgScroll;
    // The windows whose screen is still to be sent, a bit each
    u32 mapFlushMask;
    // Bit 0 while saving, and bit 4 + i while save slot i holds a tournament
    u32 saveFlags;
    // The slot picked in the select screen, and in the saved tournaments
    int slotPos;
    int slotCursor;
    int seqAfterList;
    // The received tournament to save, and the slot to save it in
    int copySrc;
    int copyDest;
    int slotCount;
    int receivedCount;
    int palAnimCount;
    u16 palFadeFrom[16];
    u16 palFadeTo[16];
    u16 palFadeWork[16];
    u16 textPltt[2][16];
    FrameList *list;
    int listCount;
    // The list's item, cursor and scroll, live in [0] and kept in [2] while the details are shown
    int listPos[3];
    int listCursor[3];
    int listScroll[3];
    int lastSlot;
    // For each tab of the details: its items, its pages and the page shown
    int itemCount[T_DOWNLOAD_TABS];
    int pageCount[T_DOWNLOAD_TABS];
    int page[T_DOWNLOAD_TABS];
    s16 *objPos;
    void *unk284;
    void *record;
    void *savedData[T_DOWNLOAD_SAVE_SLOTS];
    void *receivedData[T_DOWNLOAD_RECEIVED_MAX];
};

#endif // POKEBW2_APP_T_DOWNLOAD_T_DOWNLOAD_LOCAL_H
