#ifndef POKEBW2_FIELD_BEACON_VIEW_H
#define POKEBW2_FIELD_BEACON_VIEW_H

#include "types.h"
#include "field/beacon_view_common.h"
#include "field/fest_mission_data.h"
#include "field/field_disp_control.h"
#include "gfl/arc.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nnsys/g2d.h"
#include "save/high_link.h"
#include "struct_decls.h"
#include "system/app_keycursor.h"
#include "system/bmp_oam.h"
#include "system/fest_mission.h"
#include "system/game_beacon.h"
#include "system/game_event.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The C-Gear's log of passersby on the touch screen, overlay 84: the last 30 beacons received, four to a page, with
// the messages of new ones and the Pass Powers and Funfest missions they offer. Its files are beacon_view_setup.c,
// which the ROM names, and beacon_view.c, beacon_view_sub.c and beacon_view_accessor.c, whose names are guesses.
// Overlay 83's beacon_view_common.c is what it shares with overlay 85. swan has no names for this overlay; the names
// are ours
#define OVERLAY_BEACON_VIEW OVERLAY_ID(84)

#define BEACON_VIEW_PANEL_MAX 5
// The log's lines on the screen
#define BEACON_VIEW_LINE_MAX 4
#define BEACON_VIEW_PANEL_DIGITS 3
#define BEACON_VIEW_TRAINER_CHARS 16
#define BEACON_VIEW_ICON_CHARS 12

// The events the view asks the field for (BeaconView_RequestEvent), which BeaconView_CheckEvent creates
#define BEACON_VIEW_EVENT_NONE 0
#define BEACON_VIEW_EVENT_MENU 1
#define BEACON_VIEW_EVENT_MENU_COMM 2
#define BEACON_VIEW_EVENT_MENU_WIFI 3
#define BEACON_VIEW_EVENT_DETAIL 4
#define BEACON_VIEW_EVENT_MEDAL 5
#define BEACON_VIEW_EVENT_PASS_POWER 6
#define BEACON_VIEW_EVENT_PASS_POWER_USE 7
#define BEACON_VIEW_EVENT_PASS_POWER_LIST 8
#define BEACON_VIEW_EVENT_PASS_POWER_LIST_2 9
#define BEACON_VIEW_EVENT_GREETING 10
#define BEACON_VIEW_EVENT_MISSION_START 11
#define BEACON_VIEW_EVENT_MISSION_MESSAGE 12
#define BEACON_VIEW_EVENT_MISSION_INFO 13

// The actors of the screen itself
#define BEACON_VIEW_ACTOR_PASS_POWER 0
#define BEACON_VIEW_ACTOR_GREETING 1
#define BEACON_VIEW_ACTOR_THANKS 2
#define BEACON_VIEW_ACTOR_RETURN 3
#define BEACON_VIEW_ACTOR_UP 4
#define BEACON_VIEW_ACTOR_DOWN 5
#define BEACON_VIEW_ACTOR_BAR 6
#define BEACON_VIEW_ACTOR_COUNT 9
// The four buttons at once, for BeaconView_SetButtonAnim
#define BEACON_VIEW_BUTTON_ALL 4

#define BEACON_VIEW_WIN_MESSAGE 0
#define BEACON_VIEW_WIN_TOTAL 1
#define BEACON_VIEW_WIN_COUNT 2

// The sets of OAM resources: the screen's own, the trainers' and the icons'
#define BEACON_VIEW_RES_COMMON 0
#define BEACON_VIEW_RES_TRAINER 1
#define BEACON_VIEW_RES_ICON 2
#define BEACON_VIEW_RES_COUNT 3

#define BEACON_VIEW_RES_PALETTE 0
#define BEACON_VIEW_RES_CHARS 1
#define BEACON_VIEW_RES_CELLS 2
#define BEACON_VIEW_RES_KINDS 3

// The registered resources of one kind
typedef struct {
    u32 count;
    u32 *ids;
} BeaconViewResList;

typedef struct {
    BeaconViewResList lists[BEACON_VIEW_RES_KINDS];
} BeaconViewRes;

typedef struct {
    BeaconViewWin win;
    u32 unused;
} BeaconViewWinSlot;

// A character file kept in memory, to change an actor's characters from
typedef struct {
    NNSG2dCharacterData *data;
    void *file;
} BeaconViewChars;

typedef struct {
    NNSG2dPaletteData *data;
    u16 *colors;
    void *file;
} BeaconViewPalette;

// A point of the touch screen
typedef struct {
    int x;
    int y;
} BeaconViewTouch;

// A line of the log on the screen: a plate with the player's sprite, name or message, icon and medal count. One more
// than the screen shows, for the line that scrolls in
typedef struct {
    u8 id;
    // The log entry it shows, or 0xff when it is free
    u8 dataIndex;
    // 1 to 4 on the screen, 0 above it and 5 below
    u8 line;
    u8 rank;
    u8 gender;
    // Whether the beacon is too old for its name to be shown in the fresh color
    u8 old;
    u8 version;
    // 1 for a player of a White version
    u8 colorSet;
    u8 medalCount;
    u16 unkA;
    u16 trainerId;
    s16 x;
    s16 y;
    ClActor *base;
    ClActor *trainer;
    ClActor *icon;
    ClActor *medal;
    ClActor *digits[BEACON_VIEW_PANEL_DIGITS];
    BeaconViewOam oam;
    StrBuf *greeting;
    StrBuf *name;
    PrintQueue *queue;
    // The task that waits for the plate's text to be printed
    TCBEx *printTask;
    // The task that shows the plate's icon for a while
    TCBEx *iconTask;
} BeaconViewPanel;

typedef struct BeaconViewWork {
    GameSystem *gsys;
    GameData *gameData;
    Field *field;
    void *parent;
    FieldDispControl *dispControl;
    TrainerCardSave *trainerCard;
    BagSave *bag;
    GameCommSys *commSys;
    WifiList *wifiList;
    LinkFestival *festival;
    FestMissionConfig *missionConfig;
    FestivalText *festText;
    PassPowerData *passPowerData;
    // The Pass Power the player has set to give
    u32 passPower;
    u32 trainerId;
    u8 gender;
    u8 unk3D;
    u8 version;
    BeaconStatus *status;
    // The beacons received since the last frame
    GameBeaconInfoTable *newTable;
    // The game's log
    GameBeaconInfoTable *log;
    GameBeacon *beacon;
    GameBeacon *countBeacon;
    u16 time;
    u16 newTime;
    GameBeacon *newBeacon;
    BeaconViewPanel *newPanel;
    u8 newOffset;
    u8 newIndex;
    u8 unk62;
    u8 oldCount;
    // How many of the log are of the other gender, and of another version
    u8 otherGenderCount;
    u8 otherVersionCount;
    BOOL isNew;
    BOOL active;
    u32 unk70;
    int seq;
    int subSeq;
    // The sequence to go back to once the bar is let go of
    int barReturnSeq;
    u16 eventRequest;
    u16 eventRunning;
    int wait;
    u8 unk88;
    // Set while the screen is first drawn: text is printed at once, not through the queue
    u8 printNow;
    u8 unk8A[2];
    u8 logCount;
    u8 top;
    u8 viewCount;
    u8 selected;
    u8 selectedPanel;
    u8 barDragging;
    u8 unk92[2];
    u8 barTarget;
    u8 barDistance;
    u8 barUp;
    // The Pass Power the player chose to use, until its event takes it
    u8 usePassPower;
    u8 missionButton;
    u32 missionRefusal;
    // The mission the player chose, until its event takes it
    FestMissionConfig mission;
    u8 scrolling;
    u8 scrollEnd;
    u8 scrollReturnSeq;
    int scrollDir;
    int scrollFrames;
    u32 unkD8[2];
    u16 resHeapId;
    u16 tmpHeapId;
    u16 heapId;
    ArcTool *arc;
    TCBExManager *tcbManager;
    TCB *vblankTask;
    // The tasks still running
    int pending;
    PaletteFade *paletteFade;
    Font *font;
    PrintQueue *printQueue;
    u32 unk104;
    KeyCursor *keyCursor;
    BeaconViewMenu *menu;
    WordSet *wordSet;
    MsgData *msgData;
    StrBuf *strTemplate;
    StrBuf *strExpand;
    StrBuf *strMessage;
    BmpOamSys *bmpOam;
    ClActUnit *clUnit;
    ClActRenderer *renderer;
    u32 unk130;
    BeaconViewRes res[BEACON_VIEW_RES_COUNT];
    BeaconViewWinSlot wins[BEACON_VIEW_WIN_COUNT];
    BeaconViewOam countOam;
    BeaconViewPanel panels[BEACON_VIEW_PANEL_MAX];
    ClActor *actors[BEACON_VIEW_ACTOR_COUNT];
    // How many players the player has passed by
    u32 passerbyCount;
    BeaconViewChars iconChars[BEACON_VIEW_ICON_CHARS];
    BeaconViewChars trainerChars[BEACON_VIEW_TRAINER_CHARS];
    BeaconViewPalette trainerPalette;
    BeaconViewPalette commonPalette;
} BeaconViewWork;

// beacon_view_setup.c
BeaconViewWork *BeaconView_Create(GameSystem *gsys, void *parent);
void BeaconView_Free(BeaconViewWork *work);
void BeaconView_RequestEvent(BeaconViewWork *work, u16 request);
void BeaconView_ClearEventRequest(BeaconViewWork *work);
// The event of the view's request, or NULL. It first waits for the touched button's animation
GameEvent *BeaconView_CreateEvent(BeaconViewWork *work, BOOL enabled);

// beacon_view.c
BeaconViewWork *BeaconView_Init(GameSystem *gsys, void *parent);
void BeaconView_Exit(BeaconViewWork *work);
void BeaconView_Update(BeaconViewWork *work, BOOL active);
BOOL BeaconView_Draw(BeaconViewWork *work);
GameEvent *BeaconView_CheckEvent(BeaconViewWork *work, BOOL enabled);
// Sets the animation of one of the four buttons, or of all with BEACON_VIEW_BUTTON_ALL: 0 idle, 1 touched, 2 off
void BeaconView_SetButtonAnim(BeaconViewWork *work, int button, int state);
BOOL BeaconView_IsButtonAnimating(BeaconViewWork *work, int button);

// beacon_view_sub.c
void BeaconView_CountOthers(BeaconViewWork *work, u8 *genderCount, u8 *versionCount);
void BeaconView_CheckRecords(BeaconViewWork *work, const GameBeacon *beacon, u8 oldCount);
u32 BeaconView_GetRecordPassPower(BeaconViewWork *work);
int BeaconView_HitTestPanel(BeaconViewWork *work, BeaconViewTouch *touch);
int BeaconView_HitTestMedal(BeaconViewWork *work, BeaconViewTouch *touch);
int BeaconView_HitTestArrow(BeaconViewWork *work, BeaconViewTouch *touch);
int BeaconView_HitTestBar(BeaconViewWork *work, BeaconViewTouch *touch);
int BeaconView_HitTestButton(BeaconViewWork *work, BeaconViewTouch *touch);
void BeaconView_PrintCount(BeaconViewWork *work);
void BeaconView_PrintTotal(BeaconViewWork *work, u8 messageId, int *pending);
void BeaconView_UpdateArrows(BeaconViewWork *work);
void BeaconView_UpdateBar(BeaconViewWork *work);
void BeaconView_UpdateThanksButton(BeaconViewWork *work);
void BeaconViewPanel_SetVisible(BeaconViewPanel *panel, BOOL visible);
void BeaconViewPanel_Assign(BeaconViewPanel *panel, u8 dataIndex, u8 line);
BeaconViewPanel *BeaconView_FindPanel(BeaconViewWork *work, u8 dataIndex);
BOOL BeaconViewPanel_HasNoMedals(BeaconViewPanel *panel);
void BeaconViewPanel_SetBeacon(BeaconViewWork *work, BeaconViewPanel *panel, const GameBeacon *beacon, int mode,
                               BOOL quiet, int *pending);
void BeaconView_UpdateNameColors(BeaconViewWork *work);
void BeaconView_ScrollLine(BeaconViewWork *work, int dir, u8 frames);
void BeaconView_StartScroll(BeaconViewWork *work, int dir, u8 returnSeq);
BOOL BeaconView_DragBar(BeaconViewWork *work, int y);
void BeaconView_AddLine(BeaconViewWork *work, const GameBeacon *beacon, u8 dataIndex, u8 dir, u8 frames, int mode);
void BeaconView_StartJump(BeaconViewWork *work, u8 top);
BOOL BeaconView_ShowBeaconMessage(BeaconViewWork *work, const GameBeacon *beacon, BOOL isNew);
void BeaconView_ShowMessage(BeaconViewWork *work, u8 messageId);
void BeaconView_OfferRecordPassPower(BeaconViewWork *work, u32 passPower);
void BeaconView_OfferPassPower(BeaconViewWork *work, u32 passPower, u8 record, int *pending);
void BeaconView_OfferMission(BeaconViewWork *work, const FestMissionConfig *config, int *pending);
void BeaconView_FlashSelected(BeaconViewWork *work, u8 offset);

// beacon_view_accessor.c
// The first mission another player offers that has time left, with the player's name; those of a set unk0_30 first
BOOL BeaconView_GetMissionOffer(LinkFestival *festival, FestMissionConfig *dest, StrBuf *name);
// The records the player has reached, each of which gives a Pass Power once the view offers it
void BeaconStatus_AddRecord(BeaconStatus *status, u8 record);
u8 BeaconStatus_GetRecord(BeaconStatus *status);
void BeaconStatus_PopRecord(BeaconStatus *status);
// The Pass Power a beacon gave that the player has not answered yet, with the name of its player
void BeaconStatus_SetGivenPassPower(BeaconStatus *status, u32 passPower, const StrBuf *name);
u32 BeaconStatus_GetGivenPassPower(BeaconStatus *status, StrBuf *name);
void BeaconStatus_ClearGivenPassPower(BeaconStatus *status);

#endif // POKEBW2_FIELD_BEACON_VIEW_H
