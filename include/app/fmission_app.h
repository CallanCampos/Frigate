#ifndef POKEBW2_APP_FMISSION_APP_H
#define POKEBW2_APP_FMISSION_APP_H

// The Funfest mission screen (overlay 309): the mission and its ranking while it runs, and its result when it is over.
// The ROM has no name for its files but the graphic file's, "fmission_app_gra.c"; "fmission_app.c" and
// "fmission_app_draw.c" are guesses from it. app/festival.h has the screen's parameters and its process

#include "types.h"
#include "app/festival.h"
#include "app/fmission_app_gra.h"
#include "app/fmission_log_accessor_fapp.h"
#include "field/fest_mission_data.h"
#include "field/festival.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/str.h"
#include "gfl/tcbl.h"
#include "struct_decls.h"
#include "system/app_keycursor.h"
#include "system/app_printsys_common.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wordset.h"

#define HEAPID_FMISSION_APP 0xa7
// The screen's graphics and its table of positions
#define ARCID_FMISSION_APP 270
// The missions' data
#define ARCID_FMISSION_DATA 289

// What FestivalEventParam's mode is: the running mission, or its result
#define FMISSION_APP_MODE_MISSION 0
#define FMISSION_APP_MODE_RESULT 1
#define FMISSION_APP_MODE_COUNT 2

#define FMISSION_APP_WINDOW_COUNT 30
#define FMISSION_APP_WINDOW_MESSAGE 21
#define FMISSION_APP_POS_COUNT 14

// The three groups of actors: the upper screen's (the sub engine's), the touch screen's return button, and the mark of
// the result's new record
#define FMISSION_APP_ACTOR_UPPER 0
#define FMISSION_APP_ACTOR_LOWER 1
#define FMISSION_APP_ACTOR_RECORD 2

#define FMISSION_APP_UPPER_ACTOR_COUNT 20
#define FMISSION_APP_LOWER_ACTOR_COUNT 2
#define FMISSION_APP_RECORD_ACTOR_COUNT 1

#define FMISSION_APP_RES_COUNT 9

#define FMISSION_APP_COLOR_TEXT PRINT_COLOR(15, 2, 0)

// The result's messages
enum {
    FMISSION_APP_MESSAGE_CLEARED,
    FMISSION_APP_MESSAGE_FAILED,
    FMISSION_APP_MESSAGE_ITEM,
    FMISSION_APP_MESSAGE_ITEM_NO_ROOM,
    FMISSION_APP_MESSAGE_BONUS,
    FMISSION_APP_MESSAGE_LEVEL_ALONE,
    FMISSION_APP_MESSAGE_LEVEL,
    FMISSION_APP_MESSAGE_ITEM2,
    FMISSION_APP_MESSAGE_ITEM2_NO_ROOM,
    // The names of the Entralink's two sides, for the level messages. These are places in the table of the messages'
    // texts, which has none for the second item's messages
    FMISSION_APP_MESSAGE_WHITE = FMISSION_APP_MESSAGE_ITEM2,
    FMISSION_APP_MESSAGE_BLACK,
};

typedef struct {
    HeapID heapId;
    // A palette, characters and cells for each of the screen's own, the touch bar's and the record mark's
    u32 resources[FMISSION_APP_RES_COUNT];
    ClActor *lowerActors[FMISSION_APP_LOWER_ACTOR_COUNT];
    ClActor *upperActors[FMISSION_APP_UPPER_ACTOR_COUNT];
    ClActor *recordActors[FMISSION_APP_RECORD_ACTOR_COUNT];
    FMissionAppGra *graphic;
    WifiList *wifiList;
    Font *font;
    PrintQueue *printQueue;
    MsgData *msgData[2];
    PrintWindow windows[FMISSION_APP_WINDOW_COUNT];
    WordSet *wordSet;
    // The result's message, its stream and what they need
    PrintStream *stream;
    TCBExManager *tcbManager;
    StrBuf *message;
    KeyCursor *keyCursor;
    AppPrintsysCommon printCommon;
    FestivalEventParam *param;
    FestivalText *missionText;
    ArcTool *missionArc;
    FestMissionData *missions;
    LinkFestivalRanking ranking;
    FMissionAppResult result;
    LinkFestivalBest *best;
    // The log's last lines as the festival has them, and as the screen keeps them
    FMissionLogFApp *log;
    int logCount;
    FMissionLogFAppEntry logEntries[FMISSION_LOG_FAPP_COUNT];
    // The line the scrolling log shows
    StrBuf *shownName;
    u32 shownScore;
    u32 shownTrainerId;
    int seq;
    int mode;
    u32 flags;
    // The scrolling log's position, in 1/256 pixels
    int scroll;
    // The frames and the step of the result's timeline
    int timer;
    int step;
    // What was last drawn of the mission
    int lastSeconds;
    int lastCount;
    int lastTotal;
    int lastScore;
    s16 *positions;
} FMissionApp;

// fmission_app_draw.c
void FMissionAppDraw_Init(FMissionApp *work, u32 mode);
void FMissionAppDraw_Free(FMissionApp *work);
// Prints a string into a window at x, aligned by a PRINT_ALIGN_*
void FMissionAppDraw_Print(FMissionApp *work, int window, u32 x, u32 y, StrBuf *strbuf, u32 align, u16 color);
void FMissionAppDraw_ClearWindow(FMissionApp *work, int window);
// A value of the positions' table, which holds an x and a y for each position
s16 FMissionAppDraw_GetPos(FMissionApp *work, u32 index);
void FMissionAppDraw_SetActorAnim(FMissionApp *work, int type, int index, u32 sequence);
void FMissionAppDraw_SetActorVisible(FMissionApp *work, int type, int index, BOOL visible);
BOOL FMissionAppDraw_IsActorAnimActive(FMissionApp *work, int type, int index);
void FMissionAppDraw_SetActorPalette(FMissionApp *work, int type, int index, u32 palette);
// Shows a number with the four digit actors from `first`, at x and y
void FMissionAppDraw_SetNumber(FMissionApp *work, int type, int first, int number, int digits, BOOL pad, int x, int y);
// Shows a time with the five actors from `first`: two digits, a colon and two digits
void FMissionAppDraw_SetTime(FMissionApp *work, int type, long first, int minutes, int seconds);
void FMissionAppDraw_SetLevelIcon(FMissionApp *work, int index);
// What of the sub screen is touched this frame: 0 for the return button
int FMissionAppDraw_CheckTouch(FMissionApp *work);
// Whether the message is still being printed
BOOL FMissionAppDraw_UpdateMessage(FMissionApp *work);
// Shows one of the result's messages in a window, at once or as a stream
void FMissionAppDraw_ShowMessage(FMissionApp *work, int window, int message, BOOL stream);
// How many more of an item the bag takes
int FMissionAppDraw_GetItemRoom(FMissionApp *work, u32 item);
// Draws or erases the cursor that waits for a key at the message's end
void FMissionAppDraw_SetKeyCursor(FMissionApp *work, BOOL shown);

#endif // POKEBW2_APP_FMISSION_APP_H
