#ifndef POKEBW2_APP_BEACON_DETAIL_LOCAL_H
#define POKEBW2_APP_BEACON_DETAIL_LOCAL_H

// What the files of the screen of a beacon's details (overlay 308) share: its work, and the functions of
// beacon_detail_sub.c that beacon_detail.c's sequences call. The ROM names only "beacon_detail_gra.c"; the other
// files' names and this header's are guesses

#include "types.h"
#include "app/beacon_detail.h"
#include "app/beacon_detail_gra.h"
#include "app/ui/touchbar.h"
#include "gfl/arc.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "nnsys/g2d.h"
#include "struct_decls.h"
#include "system/game_beacon.h"
#include "system/gf_font.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "system/wordset.h"

#define HEAPID_BEACON_DETAIL 0x77

// The two panels of details: the one shown and the one that slides in when the player turns the page
#define BEACON_DETAIL_PANEL_COUNT 2
#define BEACON_DETAIL_UNION_VIEW_COUNT 16

// The windows of a panel: the labels and values of the name, the job, the hobby and the play time, the "FROM:"
// label, the country and region, and the numbers of thanks and of passersby
enum {
    BEACON_DETAIL_WINDOW_NAME,
    BEACON_DETAIL_WINDOW_JOB,
    BEACON_DETAIL_WINDOW_HOBBY,
    BEACON_DETAIL_WINDOW_TIME,
    BEACON_DETAIL_WINDOW_FROM,
    BEACON_DETAIL_WINDOW_HOME,
    BEACON_DETAIL_WINDOW_COUNTS,
    BEACON_DETAIL_WINDOW_COUNT,
};

// The map's actors: the trainer where the beacon was sent from, and the icon of what the trainer was doing
enum {
    BEACON_DETAIL_ACTOR_TRAINER,
    BEACON_DETAIL_ACTOR_RECENT,
    BEACON_DETAIL_ACTOR_COUNT,
};

// The sequences of the screen's main function
enum {
    BEACON_DETAIL_SEQ_INIT,
    BEACON_DETAIL_SEQ_FADE_IN,
    BEACON_DETAIL_SEQ_INPUT,
    // Until the tasks that the input started have ended
    BEACON_DETAIL_SEQ_WAIT,
    BEACON_DETAIL_SEQ_FADE_OUT,
    BEACON_DETAIL_SEQ_END,
};

// A window with its bitmap, printed into through the print queue
typedef struct {
    BmpWin *window;
    GFLBitmap *bitmap;
    PrintWindow print;
} BeaconDetailWindow;

// The cell actor resources of a set of graphics
typedef struct {
    u32 palette;
    u32 chars;
    u32 cellAnims;
} BeaconDetailObjRes;

typedef struct {
    NNSG2dCharacterData *data;
    void *file;
} BeaconDetailChars;

typedef struct {
    NNSG2dPaletteData *data;
    u16 *raw;
    void *file;
} BeaconDetailPalette;

typedef struct {
    BeaconDetailWindow windows[BEACON_DETAIL_WINDOW_COUNT];
    // The sender's C-Gear record
    BmpWin *recordWindow;
    ClActor *rankActor;
    ClActor *trainerActor;
    u8 trainerView;
    // Nothing sets it, so showing and hiding a panel leaves its rank icon alone
    u8 rankShown;
    u8 bg;
} BeaconDetailPanel;

typedef struct {
    HeapID heapId;
    HeapID tailHeapId;
    // The step of the sequence
    int seq;
    u32 unk8;
    // How many tasks are running
    int taskCount;
    // Set when the log holds no beacon to show, which ends the screen at once
    BOOL empty;
    BeaconDetailParam *param;
    TCBExManager *tcbManager;
    void *townMapData;
    BeaconStatus *beaconStatus;
    GameBeaconInfoTable *infoTable;
    // The beacon shown, and when it was received: the hour in the high byte and the minute in the low
    GameBeacon *beacon;
    u16 time;
    // The panel shown
    u8 page;
    // The beacons of the log that the screen shows, their offsets in the log, and the one shown
    u8 count;
    u8 index;
    u8 startLogIndex;
    u8 logIndices[GAME_BEACON_LOG_MAX];
    BeaconDetailGraphic *graphic;
    ArcTool *arc;
    ArcTool *trainerArc;
    BeaconDetailObjRes iconRes;
    BeaconDetailObjRes trainerRes[BEACON_DETAIL_PANEL_COUNT];
    // The map's trainer, whose characters and palette are replaced with those of the sender's view
    BeaconDetailObjRes unionRes;
    BeaconDetailChars unionChars[BEACON_DETAIL_UNION_VIEW_COUNT];
    BeaconDetailPalette unionPalette;
    // The palettes of the versions' frames
    BeaconDetailPalette versionPalette;
    TouchBar *touchBar;
    Font *font;
    PrintQueue *printQueue;
    MsgData *msgData;
    MsgData *countryMsgData;
    WordSet *wordSet;
    StrBuf *formatBuf;
    StrBuf *textBuf;
    StrBuf *unk144;
    // What the sender was doing and when the beacon came, over the map
    BeaconDetailWindow infoWindow;
    PMSDraw *pmsDraw;
    BeaconDetailPanel panels[BEACON_DETAIL_PANEL_COUNT];
    // Where the sender is on the map, when the map has the place
    s16 markerX;
    s16 markerY;
    u8 hasPlace;
    ClActor *actors[BEACON_DETAIL_ACTOR_COUNT];
} BeaconDetailWork;

// Draws the beacon that the screen starts with
void BeaconDetailSub_Start(BeaconDetailWork *work);
// Handles a frame's input and returns the sequence to go on with
u32 BeaconDetailSub_Input(BeaconDetailWork *work);
// After the tasks of an input have ended
void BeaconDetailSub_EndWait(BeaconDetailWork *work);

#endif // POKEBW2_APP_BEACON_DETAIL_LOCAL_H
