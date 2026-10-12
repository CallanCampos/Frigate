#ifndef POKEBW2_FIELD_FMISSION_VIEW_LOCAL_H
#define POKEBW2_FIELD_FMISSION_VIEW_LOCAL_H

#include "types.h"
#include "field/beacon_view_common.h"
#include "field/fest_mission_data.h"
#include "field/field_disp_control.h"
#include "field/fmission_view.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nnsys/g2d.h"
#include "struct_decls.h"
#include "system/app_keycursor.h"
#include "system/bmp_oam.h"
#include "system/fest_mission.h"
#include "system/fest_mission_beacon.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wordset.h"

// What the two files of the Funfest mission's view share (fmission_view.c and fmission_view_setup.c). The header's
// name is a guess

// The log shows three entries at a time
#define FMISSION_VIEW_PANEL_COUNT 3
// The lines of a panel: the player's name, what the player did and the score
#define FMISSION_VIEW_PANEL_LINE_COUNT 3
#define FMISSION_VIEW_PANEL_NONE 0xff
// The looks a player's icon can have: eight for each gender
#define FMISSION_VIEW_LOOK_COUNT 16

#define FMISSION_VIEW_PANEL_X 8
#define FMISSION_VIEW_PANEL_Y 48
#define FMISSION_VIEW_PANEL_HEIGHT 40

// The buttons, which are actors. FMISSION_VIEW_BUTTON_ALL stands for all of them in FMissionView_SetButtonState
enum {
    FMISSION_VIEW_BUTTON_PASS_POWER,
    FMISSION_VIEW_BUTTON_GREETING,
    FMISSION_VIEW_BUTTON_QUIT,
    FMISSION_VIEW_BUTTON_INFO,
    FMISSION_VIEW_BUTTON_UP,
    FMISSION_VIEW_BUTTON_DOWN,
    FMISSION_VIEW_BUTTON_COUNT,
    FMISSION_VIEW_BUTTON_ALL,
};

// A button's animations come in threes, from sequence 3 on
enum {
    FMISSION_VIEW_BUTTON_STATE_NORMAL,
    FMISSION_VIEW_BUTTON_STATE_PRESSED,
    FMISSION_VIEW_BUTTON_STATE_DISABLED,
    FMISSION_VIEW_BUTTON_STATE_COUNT,
};

// The screens a button leaves the view for, which the field asks for with FMissionView_CheckEvent
enum {
    FMISSION_VIEW_EVENT_NONE,
    FMISSION_VIEW_EVENT_MENU,
    FMISSION_VIEW_EVENT_MENU_END_COMM,
    FMISSION_VIEW_EVENT_MENU_3,
    FMISSION_VIEW_EVENT_INFO,
    FMISSION_VIEW_EVENT_QUIT,
    FMISSION_VIEW_EVENT_USE_PASS_POWER,
    FMISSION_VIEW_EVENT_PASS_POWER,
    FMISSION_VIEW_EVENT_PASS_POWER_LIST,
    FMISSION_VIEW_EVENT_PASS_POWER_LIST_9,
    FMISSION_VIEW_EVENT_GREETING,
};

// The windows
enum {
    FMISSION_VIEW_WIN_MESSAGE,
    FMISSION_VIEW_WIN_MISSION,
    FMISSION_VIEW_WIN_GOAL,
    FMISSION_VIEW_WIN_MEMBERS,
    FMISSION_VIEW_WIN_COUNT,
};

// The palettes, characters and cells with animations of an archive, as the cell actor system's resources
enum {
    FMISSION_VIEW_RES_PALETTE,
    FMISSION_VIEW_RES_CHARS,
    FMISSION_VIEW_RES_CELL_ANIMS,
    FMISSION_VIEW_RES_TYPE_COUNT,
};

typedef struct {
    u32 count;
    u32 *ids;
} FMissionViewResList;

typedef struct {
    FMissionViewResList lists[FMISSION_VIEW_RES_TYPE_COUNT];
} FMissionViewRes;

// The view's own resources, and those of the players' icons
enum {
    FMISSION_VIEW_RES_MAIN,
    FMISSION_VIEW_RES_ICON,
    FMISSION_VIEW_RES_COUNT,
};

// A window of the view. The view's windows are 0x20 bytes apart, four more than beacon_view_common.h's BeaconViewWin
typedef struct {
    BeaconViewWin win;
    u32 unk1C;
} FMissionViewWin;

// A line of the log on screen
typedef struct {
    u8 index;
    // Which entry of the log it shows, or FMISSION_VIEW_PANEL_NONE
    u8 logIndex;
    // Set when the lines' bitmaps are to be copied to VRAM at the next VBlank
    u8 dirty : 1;
    // Its place on screen, from the top
    u8 slot : 2;
    u8 look : 5;
    u8 gender : 1;
    u8 own : 1;
    // The frame's animation: 1 for an entry that counts for the mission, 2 for a player's words
    u8 kind : 2;
    // The mark that pops up next to a player's words, or 0
    u8 mark : 4;
    u16 unk4;
    // The entry's value, which for a player's words picks the mark
    u16 value;
    s16 x;
    s16 y;
    ClActor *frame;
    ClActor *markActor;
    ClActor *icon;
    BeaconViewOam lines[FMISSION_VIEW_PANEL_LINE_COUNT];
    StrBuf *strs[FMISSION_VIEW_PANEL_LINE_COUNT];
    // The task that takes the mark away again
    TCBEx *markTask;
    PrintQueue *queue;
} FMissionViewPanel;

// The characters of a look's icon
typedef struct {
    NNSG2dCharacterData *data;
    void *file;
} FMissionViewIconChars;

struct FMissionView {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    FieldSubscreen *subscreen;
    FieldDispControl *dispControl;
    TrainerCardSave *trainerCard;
    BagSave *bag;
    GameCommSys *commSys;
    WifiList *wifiList;
    LinkFestival *festival;
    FestMissionConfig *config;
    void *passPowerData;
    FestivalText *festText;
    // The Pass Power of the player's first slot
    u32 passPower;
    u32 trainerId;
    u8 gender;
    u8 unk3D;
    u8 version;
    BeaconStatus *beaconStatus;
    // The festival's queue of new entries, the log the view shows, and the Pass Powers the players gave
    FestMissionLog *newLog;
    FestMissionLog *log;
    FestMissionPassPowers *passPowers;
    // Buffers for an entry: one for drawing and one for the entry being added
    FestMissionLogEntry *entry;
    FestMissionLogEntry *unk54;
    FestMissionLogEntry *newEntry;
    u32 unk5C;
    // Where the new entry is in the log: its offset from the newest, and its index
    u8 newOffset;
    u8 newIndex;
    u8 unk62;
    u8 prevCount;
    u32 unk64;
    BOOL active;
    u32 unk6C;
    int seq;
    int subSeq;
    // The FMISSION_VIEW_EVENT_ that is asked for, whether its event runs, and whether it has run
    u16 eventRequest;
    u8 eventRunning;
    u8 eventDone;
    // The frames until the next entry may be added
    int wait;
    u8 unk80;
    // Set to draw text at once instead of through the print queue
    u8 printNow;
    u8 unk82[2];
    // The log's entries, the offset of the one at the top of the list, and how many are on screen
    u8 logCount;
    u8 logTop;
    u8 logVisible;
    u8 unk87;
    // The Pass Power that the player chose to use, or PASS_POWER_NONE
    u8 usePassPower;
    u8 unk89;
    // The players in the mission
    u8 memberCount;
    u8 unk8B[0x2d];
    // The list scrolls while an arrow is held
    u8 scrolling;
    u8 scrollEnd;
    u8 scrollNextSeq;
    int scrollDir;
    int scrollFrames;
    u32 unkC4[2];
    HeapID fieldHeapId;
    HeapID tmpHeapId;
    HeapID heapId;
    ArcTool *archive;
    TCBExManager *tcbManager;
    TCB *vblankTask;
    // The tasks that still run
    int pending;
    int memberPending;
    PaletteFade *fade;
    Font *font;
    PrintQueue *printQueue;
    u32 unkF4;
    KeyCursor *keyCursor;
    BeaconViewMenu *menu;
    WordSet *wordSet;
    MsgData *msgData;
    StrBuf *tmpStr;
    StrBuf *message;
    StrBuf *messageCopy;
    BmpOamSys *bmpOam;
    ClActUnit *clUnit;
    ClActRenderer *renderer;
    u32 unk120;
    FMissionViewRes res[FMISSION_VIEW_RES_COUNT];
    u8 unk154[0x18];
    FMissionViewWin wins[FMISSION_VIEW_WIN_COUNT];
    FMissionViewPanel panels[FMISSION_VIEW_PANEL_COUNT];
    ClActor *buttons[FMISSION_VIEW_BUTTON_COUNT];
    u32 unk33C;
    u8 unk340[0x60];
    // The icons' characters and palettes, which are copied into a panel's own
    FMissionViewIconChars iconChars[FMISSION_VIEW_LOOK_COUNT];
    NNSG2dPaletteData *iconPaletteData;
    u16 *iconPalettes;
    void *iconPaletteFile;
    u8 unk42C[0xc];
};

// fmission_view.c
void FMissionView_SetButtonState(FMissionView *view, u32 button, u32 state);
BOOL FMissionView_IsButtonAnimating(FMissionView *view, int button);
// Copies the panels' bitmaps that changed to VRAM
void FMissionView_TransferPanels(FMissionView *view);

// fmission_view_setup.c
FMissionView *FMissionViewSetup_Create(GameSystem *gsys, FieldSubscreen *subscreen);
void FMissionViewSetup_Free(FMissionView *view);
void FMissionViewSetup_RequestEvent(FMissionView *view, u16 request);
void FMissionViewSetup_ClearEventRequest(FMissionView *view);
GameEvent *FMissionViewSetup_CheckEvent(FMissionView *view, BOOL enabled);

#endif // POKEBW2_FIELD_FMISSION_VIEW_LOCAL_H
