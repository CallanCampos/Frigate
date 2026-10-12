#include "types.h"
#include "app/cg_help.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "field/c_gear_sw.h"
#include "field/event_cgear_shutdown.h"
#include "field/event_comm_tvt.h"
#include "field/event_gsync.h"
#include "field/event_irc.h"
#include "field/event_research_radar.h"
#include "field/field.h"
#include "field/field_menu.h"
#include "field/field_sound.h"
#include "field/intrude_work.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "nnsys/g2d.h"
#include "save/c_gear_savedata.h"
#include "save/cgear_picture_save.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/game_beacon.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/palanm.h"
#include "system/wipe.h"

// The C-Gear, the field's touch screen while it is on, named after the ROM's "c_gear_sw.c" ("sw" is probably swan,
// Black 2 and White 2's code name). Five colored panels hold its buttons, and three markers that the player can move
// stand for infrared, wireless and online communication: touching one starts its feature, and its lamp blinks by what
// the communication is doing. The names are ours

#define LAMP_COUNT 3
#define MARKER_COUNT 3
#define TOUCH_EFFECT_COUNT 4
#define PATTERN_COUNT 5
// The five panels, and the background as a sixth
#define PANEL_COUNT 5
#define SKIN_COUNT 5
#define MARKER_POS_COUNT 42
#define BEACON_COUNT 10

// What the player touched, which CGearSw_CheckEvent makes an event of
enum {
    ACTION_NONE,
    ACTION_IRC,
    ACTION_TVT = 5,
    ACTION_GSYNC,
    ACTION_LOG,
    ACTION_RADAR = 11,
    ACTION_HELP,
    ACTION_POWER_OFF,
};

// The touch areas: the markers, the eight of sCGearSwButtonRects, and with the editing on the six color buttons
enum {
    TOUCH_MARKER,
    TOUCH_CLOCK_LEFT = MARKER_COUNT,
    TOUCH_CLOCK_RIGHT,
    TOUCH_POWER,
    TOUCH_HELP,
    TOUCH_EDIT,
    TOUCH_RADAR,
    TOUCH_LOG,
    TOUCH_SKIN,
    TOUCH_COLOR,
};

// The actors
enum {
    ACTOR_CLOCK_DIGIT,
    ACTOR_CLOCK = 4,
    ACTOR_BATTERY,
    ACTOR_TITLE,
    ACTOR_POWER,
    ACTOR_HELP,
    ACTOR_EDIT,
    ACTOR_RADAR,
    ACTOR_LOGO,
    ACTOR_NOTICE,
    ACTOR_MARKER,
    ACTOR_TOUCH = ACTOR_MARKER + MARKER_COUNT * 2,
    ACTOR_COUNT,
};

// How a marker shows: at rest, while the markers can be moved, and while it is being moved
enum {
    MARKER_NORMAL,
    MARKER_EDIT,
    MARKER_DRAG,
};

// What shows that a button was touched, before its event starts
enum {
    EFFECT_TOUCH,
    EFFECT_FLASH,
    EFFECT_BUTTON,
};

// The notice icon's states
enum {
    NOTICE_KEEP,
    NOTICE_NONE,
    NOTICE_LOG,
    NOTICE_NEW,
};

// The sleep callbacks' marks
enum {
    SLEEP_NONE,
    SLEEP_ASLEEP,
    SLEEP_WOKEN,
};

typedef struct {
    ClActor *actor;
    int x;
    int y;
} CGearSwTouchEffect;

// A blinking pattern: a counter for each lamp, up to max, which starts over if the pattern loops
typedef struct {
    u16 count[LAMP_COUNT];
    u16 max;
    u8 loop;
    u8 active;
} CGearSwPattern;

// The flash of palettes when a button is touched
typedef struct {
    u8 count;
    u32 paletteMask;
    PaletteFade *fade;
    TCBManager *tcbMgr;
} CGearSwFlash;

typedef void (*CGearSwStateFunc)(CGearSw *work);

struct CGearSw {
    CGearSwStateFunc state;
    u32 unk004;
    u16 heapId;
    FieldSubscreen *subscreen;
    GameSystem *gsys;
    GameData *gameData;
    CGearSettings *settings;
    ArcTool *arc;
    // The palette, the two character sets, and their two cell sets
    u32 objRes[5];
    ClActUnit *unit;
    ClActRenderer *renderer;
    CGearSwTouchEffect touchEffects[TOUCH_EFFECT_COUNT];
    ClActor *actors[ACTOR_COUNT];
    // The colors of the players that the beacons came from, by favorite color, and the one of none
    u16 beaconColors[17];
    u8 radarNotice;
    u8 logNotice;
    u8 newNotice;
    CGearSwCallback callback;
    void *callbackWork;
    TCBExManager *tcbExMgr;
    TCBManager *tcbMgr;
    TCB *vblankTcb;
    TCBEx *noticeTask;
    void *tcbMgrBuffer;
    PaletteFade *fade;
    // The panels still appearing
    u32 panelTasks;
    u32 touchAction;
    // The action the C-Gear is leaving for
    u32 action;
    CGearSwFlash flash;
    // The lamps' colors: unlit, as shown, and the pattern color each fades to
    u16 lampBaseColors[LAMP_COUNT];
    u16 lampColors[LAMP_COUNT];
    u16 lampTargets[LAMP_COUNT];
    u16 lampPalettes[9][5];
    u16 panelColors[PANEL_COUNT + 1][9];
    u16 skinColors[PANEL_COUNT + 1][16];
    // The marker that was touched
    u8 marker;
    u8 unk2b9;
    u8 buttonActor;
    u8 buttonAnim;
    u8 effect;
    CGearSwPattern patterns[PATTERN_COUNT];
    // The pattern each lamp blinks by
    u8 lampPatterns[LAMP_COUNT];
    u8 wirelessCycle;
    u8 unk2f4[2];
    u8 newBeacon;
    u8 sleep;
    u8 touchX;
    u8 touchY;
    u8 active;
    u8 panelSeq;
    u8 effectSeq;
    u8 wakeWait;
    u8 editing;
    u8 dragging;
    u8 skin;
    u8 markerPos[MARKER_COUNT];
    u8 panels[PANEL_COUNT + 1];
    u8 gender;
    // Whether the game shows other players' beacons
    u8 beacons;
    u8 noticeShown;
    u8 unk30d;
    // The two reasons the C-Gear is dark: an event runs, or the wireless communication is off
    u8 eventDark;
    u8 commDark;
    u8 darkChanged;
    u8 darkWait;
    u8 lampsChanged;
    // Whether someone asks for the player, and whether it is a call
    u8 alert;
    u8 ringing;
    u8 intruded;
    u8 call;
    u8 alertChanged;
    u8 alertWait;
}; // size: 0x31c

// A panel fading in, or flashing
typedef struct {
    CGearSw *work;
    u8 appear;
    u8 panel;
    u8 color;
    u8 unk7;
    u8 delay;
    s8 fraction;
    s8 target;
    s8 step;
    u8 seq;
    u32 *counter;
} CGearSwPanelTask;

// The notice icon's glow
typedef struct {
    CGearSw *work;
    u8 unk4;
    u8 unk5;
    s8 fraction;
    s8 max;
    s8 step;
    u32 unkC;
} CGearSwNoticeTask;

typedef struct {
    CGearSw *work;
    u32 action;
} CGearSwEvent;

typedef struct {
    u8 anim;
    u8 x;
    u8 y;
    u8 bgPriority;
    u8 priority : 4;
    u8 res : 4;
} CGearSwActorData;

// A place a marker can be at, and the panel it is on
typedef struct {
    u8 x;
    u8 y;
    u8 panel;
} CGearSwMarkerPos;

static void CGearSw_SetState(CGearSw *work, CGearSwStateFunc state);
static BOOL CGearSw_IsIdle(CGearSw *work);
static void CGearSw_SetStateDebug(CGearSw *work, CGearSwStateFunc state, int line);
static void CGearSw_AddTouchEffect(CGearSw *work, int x, int y);
static CGearSwTouchEffect *CGearSw_GetFreeTouchEffect(CGearSw *work);
static BOOL CGearSw_UpdateTouchEffect(CGearSw *work);
static void CGearSw_ClearUnk2b9(CGearSw *work);
static void CGearSw_InitPatterns(CGearSw *work);
static void CGearSw_InitPatternsAtStartup(CGearSw *work);
static void CGearSw_TurnLampsOff(CGearSw *work);
static void CGearSw_SetLampPattern(CGearSw *work, int pattern, int lamp, BOOL init, BOOL alert);
static BOOL CGearSw_IsCalled(CGearSw *work);
static void CGearSw_UpdatePatterns(CGearSw *work);
static void CGearSw_SetAlert(CGearSw *work, BOOL alert, BOOL call);
static void CGearSw_UpdateLamps(CGearSw *work);
static void CGearSw_BlinkLampFast(CGearSw *work, int lamp, CGearSwPattern *pattern);
static void CGearSw_BlinkLampSlow(CGearSw *work, int lamp, CGearSwPattern *pattern);
static void CGearSw_BlinkLampTwice(CGearSw *work, int lamp, CGearSwPattern *pattern);
static void CGearSw_BlinkLampQuick(CGearSw *work, int lamp, CGearSwPattern *pattern);
static void CGearSw_LightLamp(CGearSw *work, int lamp, CGearSwPattern *pattern);
static void CGearSw_SetLampColor(CGearSw *work, int lamp, int fraction, int max);
static void CGearSw_ReadLampPalette(CGearSw *work, const u16 *colors, int palette);
static BOOL CGearSw_TryLampPattern(CGearSw *work, int lamp, int palette, int pattern, int wantPattern,
                                   int wantLamp);
static void CGearSwPattern_Init(CGearSwPattern *pattern, u16 max, u16 start, u8 loop);
static void CGearSwPattern_Update(CGearSwPattern *pattern);
static BOOL CGearSwPattern_IsFinished(CGearSwPattern *pattern, int lamp);
static void CGearSwPattern_Loop(CGearSwPattern *pattern, int lamp);
static void CGearSwPattern_Reset(CGearSwPattern *pattern, int lamp);
static u16 CGearSwPattern_GetCount(CGearSwPattern *pattern, int lamp);
static void CGearSwPattern_SetActive(CGearSwPattern *pattern, u8 active);
static BOOL CGearSw_IsScreenShown(CGearSw *work);
static void CGearSw_SetDark(CGearSw *work, BOOL dark);
static void CGearSw_ShowAlert(CGearSw *work, BOOL alert);
static void CGearSw_StopFade(CGearSw *work);
static void CGearSw_LoadBG(CGearSw *work, int gender, int skin, BOOL palettes);
static void CGearSw_LoadObjRes(CGearSw *work, int gender);
static void CGearSw_LoadSkinChars(CGearSw *work);
static void CGearSw_InitBG(CGearSw *work);
static void CGearSw_SetSkinBlend(CGearSw *work);
static int CGearSw_GetTouchedButton(CGearSw *work);
static int CGearSw_GetTouchedColorButton(CGearSw *work);
static void CGearSw_DragMarker(CGearSw *work);
static void CGearSw_UpdateTouch(CGearSw *work);
static void CGearSw_SetEditing(CGearSw *work, BOOL editing);
static ClActor *CGearSw_AddActor(CGearSw *work, s16 x, s16 y, u8 anim, u8 bgPriority, u8 priority, u8 res);
static void CGearSw_SetMarkerMode(CGearSw *work, int marker, int mode);
static void CGearSw_GetMarkerPos(CGearSw *work, int marker, ClActorPos *pos);
static void CGearSw_SetMarkerPos(CGearSw *work, int marker, s16 x, s16 y);
static void CGearSw_SetMarkerVisible(CGearSw *work, int marker, BOOL visible);
static void CGearSw_SetActorPos(ClActor *actor, s16 x, s16 y);
static void CGearSw_SetActorAnim(ClActor *actor, u32 anim);
static void CGearSw_AddActors(CGearSw *work);
static void CGearSw_SetActorsVisible(CGearSw *work, BOOL visible);
static void CGearSw_HidePanels(CGearSw *work);
static void CGearSw_ShowPanels(CGearSw *work);
static BOOL CGearSw_FlashPanels(CGearSw *work);
static BOOL CGearSw_OpenPanels(CGearSw *work);
static void CGearSw_AddPanelTask(CGearSw *work, u8 appear, u8 panel, u8 color, u8 delay, u32 *counter);
static void CGearSw_PanelTask(TCBEx *task, void *data);
static void CGearSw_SetNoticeGlow(CGearSw *work, u8 fraction);
static void CGearSw_StartNoticeGlow(CGearSw *work);
static void CGearSw_StopNoticeGlow(CGearSw *work);
static void CGearSw_NoticeTask(TCBEx *task, void *data);
static void CGearSw_UpdateBeacons(CGearSw *work);
static void CGearSw_LoadSkinColors(CGearSw *work, int color);
static void CGearSw_LoadMarkerColors(CGearSw *work, int marker, int color);
static void CGearSw_LoadPanelColors(CGearSw *work, u8 panel, u8 color);
static void CGearSw_BlendPanel(CGearSw *work, u8 panel, u8 fraction, u16 color);
static void CGearSw_SetPanelVisible(CGearSw *work, int panel, BOOL visible);
static void CGearSw_LoadColors(CGearSw *work);
static void CGearSw_BlendPanels(CGearSw *work);
static u8 CGearSw_GetMarkerPanel(CGearSw *work, int marker);
static void CGearSw_PlaceMarker(CGearSw *work, int marker, int pos);
static void CGearSw_PlaceMarkers(CGearSw *work);
static fx32 CGearSw_GetDistance(int x1, int y1, int x2, int y2);
static BOOL CGearSw_IsMarkerNear(CGearSw *work, int marker, int x, int y);
static u32 CGearSw_FindMarkerPos(CGearSw *work, int marker, u8 x, u8 y);
static void CGearSw_SetDragging(CGearSw *work, int marker, BOOL dragging);
static void CGearSw_InitGraphics(CGearSw *work, BOOL startup);
static void CGearSw_FreeObjects(CGearSw *work);
static void CGearSw_ExitGraphics(CGearSw *work);
static u8 CGearSw_GetBatteryFrame(void);
static void CGearSw_UpdateClock(CGearSw *work);
static void CGearSw_UpdateMain(CGearSw *work);
static void CGearSw_StateMain(CGearSw *work);
static void CGearSw_StateOpen(CGearSw *work);
static void CGearSw_StateFlash(CGearSw *work);
static void CGearSw_StateWake(CGearSw *work);
static BOOL CGearSw_LoadSkin(CGearSw *work, GameSystem *gsys);
static void CGearSw_SleepCallback(void *data);
static void CGearSw_WakeCallback(void *data);
static void CGearSw_VBlankTask(TCB *tcb, void *data);
static void CGearSw_Init(CGearSw *work, CGearSettings *settings, FieldSubscreen *subscreen, GameSystem *gsys,
                         BOOL startup);
static void CGearSw_Exit(CGearSw *work);
static BOOL CGearSw_UpdateButtonAnim(CGearSw *work);
static BOOL CGearSw_UpdateButtonFlash(CGearSw *work);
static void CGearSw_StateLeave(CGearSw *work);
static void CGearSw_SetEventDark(CGearSw *work);
static void CGearSw_SetCommDark(CGearSw *work);
static void CGearSw_ClearCommDark(CGearSw *work);
static void CGearSw_ClearEventDark(CGearSw *work);
static void CGearSw_UpdateDark(CGearSw *work);
static void CGearSw_ApplyDark(CGearSw *work);
static BOOL CGearSw_IsDark(CGearSw *work);
static BOOL CGearSw_IsDarkOrChanging(CGearSw *work);
static void CGearSw_UpdateAlert(CGearSw *work);
static void CGearSwFlash_Init(CGearSw *work, CGearSwFlash *flash);
static void CGearSwFlash_Exit(CGearSwFlash *flash);
static void CGearSwFlash_Start(CGearSwFlash *flash, u32 paletteMask);
static BOOL CGearSwFlash_Update(CGearSwFlash *flash);
static GameEvent *CGearSw_CreateEvent(CGearSw *work, u32 action);
static GameEventReturnCode CGearSw_EventMain(GameEvent *event, u32 *state, void *data);
static void CGearSw_StartTouchEffect(CGearSw *work);
static void CGearSw_FreeTouchEffects(CGearSw *work);
static BOOL CGearSw_AreTouchEffectsFinished(CGearSw *work);
static void CGearSw_StateRefused(CGearSw *work);
static void CGearSw_SetActive(CGearSw *work, BOOL active);
static void CGearSw_UpdateIntrudeAlert(CGearSw *work, u32 state);
static void CGearSw_UpdateCallAlert(CGearSw *work, u32 state);
static void CGearSw_SetNotice(CGearSw *work, int notice);
static void CGearSw_UpdateNotices(CGearSw *work);

// The order of these tables, and that the actors' tables come after the function that sets the BGs up, is what gives
// the ROM's order of the data: MWCC sorts a file's objects by size, and the sort is not stable

static const u16 sPatternLengths[PATTERN_COUNT] = { 23, 160, 80, 40, 16 };

static const u8 sPatternStarts[PATTERN_COUNT] = { 0, 12, 3, 0, 0 };

static const u8 sPatternLoops[PATTERN_COUNT] = { TRUE, TRUE, TRUE, TRUE, FALSE };

// The color the screen fades to while it is dark, by the player's gender
static const u16 sDarkColors[2] = { 0x0441, 0x0423 };

// The wireless states that the wireless lamp shows in turn, and the lamp palette of each
static const u32 sWirelessBits[3] = { 0x20, 0x10, 0 };

static const u32 sWirelessPalettes[3] = { 0, 1, 4 };

static const u8 sBlinkFractions[] = {
    0, 4, 8, 10, 14, 18, 20, 22, 24, 26, 28, 30, 31, 31, 31, 31, 31, 30, 28, 26, 24, 22, 20, 18, 14, 10, 8, 4, 0,
};

static const u8 sDoubleBlinkFractions[] = {
    0,  3,  7,  11, 18, 23, 27, 31, 31, 31, 31, 31, 29, 27, 26, 25, 24,
    26, 27, 29, 31, 31, 31, 31, 31, 29, 28, 27, 23, 15, 7,  3,  0,
};

static const u8 sLightFractions[] = { 0, 4, 8, 10, 14, 18, 20, 22, 24, 26, 28, 30, 31 };

// The actors that stop animating while the screen is dark
static const u8 sDarkActors[] = {
    ACTOR_EDIT, ACTOR_MARKER + 1, ACTOR_MARKER + 3, ACTOR_MARKER + 5, ACTOR_NOTICE, ACTOR_RADAR,
};

static const TouchRect sCGearSwButtonRects[8] = {
    { TOUCH_RECT_CIRCLE, 48, 22, 16 },   { TOUCH_RECT_CIRCLE, 208, 22, 16 },  { TOUCH_RECT_CIRCLE, 200, 180, 10 },
    { TOUCH_RECT_CIRCLE, 215, 163, 10 }, { TOUCH_RECT_CIRCLE, 228, 144, 10 }, { TOUCH_RECT_CIRCLE, 35, 160, 15 },
    { 84, 108, 84, 172 },                { 10, 26, 102, 154 },
};

// The color buttons, each after the gap that precedes it
static const TouchRect sColorButtonRects[] = {
    { TOUCH_RECT_CIRCLE, 128, 96, 28 },  { TOUCH_RECT_CIRCLE, 128, 96, 36 },  { TOUCH_RECT_CIRCLE, 128, 96, 44 },
    { TOUCH_RECT_CIRCLE, 128, 96, 52 },  { TOUCH_RECT_CIRCLE, 128, 96, 64 },  { TOUCH_RECT_CIRCLE, 128, 96, 72 },
    { TOUCH_RECT_CIRCLE, 128, 96, 84 },  { TOUCH_RECT_CIRCLE, 128, 96, 92 },  { TOUCH_RECT_CIRCLE, 128, 96, 108 },
    { TOUCH_RECT_CIRCLE, 128, 96, 116 }, { TOUCH_RECT_CIRCLE, 128, 96, 136 }, { TOUCH_RECT_CIRCLE, 128, 96, 160 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// What touching each marker starts
static const u8 sMarkerActions[MARKER_COUNT] = { ACTION_IRC, ACTION_TVT, ACTION_GSYNC };

static void CGearSw_SetState(CGearSw *work, CGearSwStateFunc state) {
    work->state = state;
    work->wakeWait = 0;
    work->panelSeq = 0;
}

static BOOL CGearSw_IsIdle(CGearSw *work) {
    if (work->sleep == SLEEP_NONE && work->state == CGearSw_StateMain) {
        return TRUE;
    }
    return FALSE;
}

// The line is the caller's, which a debug build would print
static void CGearSw_SetStateDebug(CGearSw *work, CGearSwStateFunc state, int line) {
    CGearSw_SetState(work, state);
}

static void CGearSw_AddTouchEffect(CGearSw *work, int x, int y) {
    CGearSwTouchEffect *effect = CGearSw_GetFreeTouchEffect(work);

    effect->x = x;
    effect->y = y;
    if (effect->actor != NULL) {
        CGearSw_SetActorAnim(effect->actor, work->marker + 28);
        CGearSw_SetActorPos(effect->actor, x, y);
    } else {
        effect->actor = CGearSw_AddActor(work, x, y, work->marker + 28, 2, 3, 0);
        func_0204c124(effect->actor, TRUE);
        func_0204c5c8(effect->actor, TRUE);
    }
}

// A touch effect that isn't in use, or the one that has played the longest
static CGearSwTouchEffect *CGearSw_GetFreeTouchEffect(CGearSw *work) {
    int i;
    int oldestFrame = -1;
    fx32 oldestTime = -1;
    int oldest = 0;

    for (i = 0; i < TOUCH_EFFECT_COUNT; i++) {
        int frame;
        fx32 time;

        if (work->touchEffects[i].actor == NULL) {
            return &work->touchEffects[i];
        }
        frame = func_0204c510(work->touchEffects[i].actor);
        time = func_0204c4f8(work->touchEffects[i].actor);
        if (frame > oldestFrame || (frame == oldestFrame && time > oldestTime)) {
            oldestFrame = frame;
            oldestTime = time;
            oldest = i;
        }
    }
    return &work->touchEffects[oldest];
}

static BOOL CGearSw_UpdateTouchEffect(CGearSw *work) {
    switch (work->effectSeq) {
    case 0:
        CGearSw_SetActorPos(work->actors[ACTOR_TOUCH], work->touchX, work->touchY);
        CGearSw_SetActorAnim(work->actors[ACTOR_TOUCH], work->marker + 28);
        func_0204c124(work->actors[ACTOR_TOUCH], TRUE);
        work->effectSeq++;
        break;
    case 1:
        if (!func_0204c560(work->actors[ACTOR_TOUCH])) {
            func_0204c124(work->actors[ACTOR_TOUCH], FALSE);
            work->effectSeq = 0;
            return TRUE;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static void CGearSw_ClearUnk2b9(CGearSw *work) {
    work->unk2b9 = 0;
}

static void CGearSw_InitPatterns(CGearSw *work) {
    int i;
    int lamp;

    for (i = 0; i < PATTERN_COUNT; i++) {
        CGearSwPattern_Init(&work->patterns[i], sPatternLengths[i], sPatternStarts[i], sPatternLoops[i]);
        if (i == 0) {
            CGearSwPattern_SetActive(&work->patterns[i], FALSE);
        }
    }
    for (i = 0; i < PATTERN_COUNT; i++) {
        for (lamp = 0; lamp < LAMP_COUNT; lamp++) {
            CGearSw_SetLampPattern(work, i, lamp, FALSE, FALSE);
        }
    }
}

static void CGearSw_InitPatternsAtStartup(CGearSw *work) {
    int i;
    int lamp;

    for (i = 0; i < PATTERN_COUNT; i++) {
        CGearSwPattern_Init(&work->patterns[i], sPatternLengths[i], sPatternStarts[i], sPatternLoops[i]);
        if (i != 0) {
            CGearSwPattern_SetActive(&work->patterns[i], FALSE);
        }
    }
    for (i = 0; i < PATTERN_COUNT; i++) {
        for (lamp = 0; lamp < LAMP_COUNT; lamp++) {
            CGearSw_SetLampPattern(work, i, lamp, TRUE, work->alert);
        }
    }
}

static void CGearSw_TurnLampsOff(CGearSw *work) {
    int lamp;

    for (lamp = 0; lamp < LAMP_COUNT; lamp++) {
        CGearSw_SetLampColor(work, lamp, 0, 31);
    }
}

// Picks what each lamp shows, for the lamp of a pattern that has run out: the pattern and color are only taken if
// they are for this lamp and pattern
static void CGearSw_SetLampPattern(CGearSw *work, int pattern, int lamp, BOOL init, BOOL alert) {
    GameCommSys *comm = GSYS_GetGameCommSystem(work->gsys);
    u32 busy;
    BOOL idle;
    u32 intrude;
    int light;
    int fast;
    int i;
    int twice;
    int slow;
    u32 bits;

    busy = 0;
    idle = TRUE;
    slow = 1;
    light = 4;
    fast = 3;
    twice = 2;
    if (init) {
        fast = 0;
        twice = 0;
        slow = 0;
        light = 0;
    }
    bits = func_02012be4(GameData_GetWifiList(work->gameData));
    intrude = func_ov012_02153658(comm);
    if (alert) {
        if (intrude == 3) {
            CGearSw_TryLampPattern(work, 1, 3, fast, pattern, lamp);
        } else if (bits & 8) {
            busy |= 2;
            CGearSw_TryLampPattern(work, 1, 2, fast, pattern, lamp);
        }
    }
    if (busy == 0) {
        idle = FALSE;
        if (bits & 2) {
            busy |= 1;
        }
        if (bits & 0x3c) {
            busy |= 2;
        }
        if (bits & 0x3c0) {
            busy |= 4;
        }
        if (bits & 0x40) {
            CGearSw_TryLampPattern(work, 2, 3, twice, pattern, lamp);
        } else if (bits & 0x100) {
            CGearSw_TryLampPattern(work, 2, 0, twice, pattern, lamp);
        } else if (bits & 0x80) {
            CGearSw_TryLampPattern(work, 2, 1, twice, pattern, lamp);
        } else if (bits & 0x200) {
            CGearSw_TryLampPattern(work, 2, 0, twice, pattern, lamp);
        }
        if (intrude != 0) {
            if (intrude != 3) {
                CGearSw_TryLampPattern(work, 1, 3, twice, pattern, lamp);
            }
        } else {
            for (i = 0; i < 3; i++) {
                work->wirelessCycle = (work->wirelessCycle + 1) % 3;
                if ((sWirelessBits[work->wirelessCycle] & bits) || (work->wirelessCycle == 2 && work->newBeacon)) {
                    if (CGearSw_TryLampPattern(work, 1, sWirelessPalettes[work->wirelessCycle], twice, pattern,
                                               lamp)) {
                        if (work->wirelessCycle == 2) {
                            work->newBeacon = FALSE;
                        }
                        break;
                    }
                }
            }
            if (i == 3) {
                CGearSw_TryLampPattern(work, 1, 8, slow, pattern, lamp);
            }
        }
    }
    if (!idle) {
        CGearSw_TryLampPattern(work, 0, 6, slow, pattern, lamp);
        if (!(busy & 4)) {
            CGearSw_TryLampPattern(work, 2, 7, slow, pattern, lamp);
        }
        if (!(busy & 2)) {
            CGearSw_TryLampPattern(work, 1, 8, slow, pattern, lamp);
        }
    } else {
        CGearSw_TryLampPattern(work, 0, 5, light, pattern, lamp);
        CGearSw_TryLampPattern(work, 2, 5, light, pattern, lamp);
    }
}

static BOOL CGearSw_IsCalled(CGearSw *work) {
    if (work->call == TRUE && work->alert == TRUE) {
        return TRUE;
    }
    return FALSE;
}

static void CGearSw_UpdatePatterns(CGearSw *work) {
    int i;
    BOOL restart = FALSE;
    int lamp;

    for (i = 0; i < PATTERN_COUNT; i++) {
        CGearSwPattern_Update(&work->patterns[i]);
        for (lamp = 0; lamp < LAMP_COUNT; lamp++) {
            if (CGearSwPattern_IsFinished(&work->patterns[i], lamp) || work->lampsChanged) {
                CGearSw_SetLampPattern(work, i, lamp, FALSE, work->alert);
                if (i == 0) {
                    restart = TRUE;
                }
                if (CGearSwPattern_IsFinished(&work->patterns[i], lamp)) {
                    CGearSwPattern_Loop(&work->patterns[i], lamp);
                }
            }
        }
    }
    if (restart) {
        for (i = 0; i < PATTERN_COUNT; i++) {
            if (i != 0) {
                CGearSwPattern_SetActive(&work->patterns[i], TRUE);
                for (lamp = 0; lamp < LAMP_COUNT; lamp++) {
                    CGearSw_SetLampPattern(work, i, lamp, FALSE, work->alert);
                }
            } else {
                CGearSwPattern_SetActive(&work->patterns[i], FALSE);
            }
        }
    }
    work->lampsChanged = FALSE;
}

static void CGearSw_SetAlert(CGearSw *work, BOOL alert, BOOL call) {
    int i;
    int lamp;

    CGearSw_TurnLampsOff(work);
    for (i = 0; i < PATTERN_COUNT; i++) {
        for (lamp = 0; lamp < LAMP_COUNT; lamp++) {
            CGearSwPattern_Reset(&work->patterns[i], lamp);
        }
    }
    work->alert = alert;
    work->lampsChanged = TRUE;
    work->alertChanged = TRUE;
    work->alertWait = 0;
    work->call = call;
}

static void CGearSw_UpdateLamps(CGearSw *work) {
    int lamp;

    for (lamp = 0; lamp < LAMP_COUNT; lamp++) {
        switch (work->lampPatterns[lamp]) {
        case 3:
            CGearSw_BlinkLampFast(work, lamp, &work->patterns[work->lampPatterns[lamp]]);
            break;
        case 1:
            CGearSw_BlinkLampSlow(work, lamp, &work->patterns[work->lampPatterns[lamp]]);
            break;
        case 2:
            CGearSw_BlinkLampTwice(work, lamp, &work->patterns[work->lampPatterns[lamp]]);
            break;
        case 0:
            CGearSw_BlinkLampQuick(work, lamp, &work->patterns[work->lampPatterns[lamp]]);
            break;
        case 4:
            CGearSw_LightLamp(work, lamp, &work->patterns[work->lampPatterns[lamp]]);
            break;
        }
    }
}

static void CGearSw_BlinkLampFast(CGearSw *work, int lamp, CGearSwPattern *pattern) {
    u16 count = CGearSwPattern_GetCount(pattern, lamp);

    if (count <= 36 && count >= 4) {
        count -= 4;
        CGearSw_SetLampColor(work, lamp, sBlinkFractions[(NELEMS(sBlinkFractions) - 1) * (count % 8) / 8], 31);
    }
}

static void CGearSw_BlinkLampSlow(CGearSw *work, int lamp, CGearSwPattern *pattern) {
    u16 count = CGearSwPattern_GetCount(pattern, lamp);

    if (count <= 112 && count >= 12) {
        count -= 12;
        CGearSw_SetLampColor(work, lamp, sBlinkFractions[(NELEMS(sBlinkFractions) - 1) * count / 100], 31);
    }
}

static void CGearSw_BlinkLampTwice(CGearSw *work, int lamp, CGearSwPattern *pattern) {
    u16 count = CGearSwPattern_GetCount(pattern, lamp);

    if (count <= 78 && count >= 8) {
        count -= 8;
        CGearSw_SetLampColor(work, lamp, sDoubleBlinkFractions[(NELEMS(sDoubleBlinkFractions) - 1) * count / 70], 31);
    }
}

static void CGearSw_BlinkLampQuick(CGearSw *work, int lamp, CGearSwPattern *pattern) {
    u16 count = CGearSwPattern_GetCount(pattern, lamp);

    if (count <= 20) {
        CGearSw_SetLampColor(work, lamp, sBlinkFractions[(NELEMS(sBlinkFractions) - 1) * (count % 10) / 10], 31);
    }
}

static void CGearSw_LightLamp(CGearSw *work, int lamp, CGearSwPattern *pattern) {
    u16 count = CGearSwPattern_GetCount(pattern, lamp);

    if (count <= 15) {
        CGearSw_SetLampColor(work, lamp, sLightFractions[(NELEMS(sLightFractions) - 1) * count / 15], 31);
    }
}

// Shows a lamp in the color that is fraction/max of the way from its unlit color to its pattern's
static void CGearSw_SetLampColor(CGearSw *work, int lamp, int fraction, int max) {
    int i;

    work->lampColors[lamp] = 0;
    for (i = 0; i < 3; i++) {
        int shift = i * 5;
        int from = (work->lampBaseColors[lamp] >> shift) & 0x1f;
        int to = (work->lampPalettes[work->lampTargets[lamp]][0] >> shift) & 0x1f;
        int diff = to - from;

        work->lampColors[lamp] |= ((from + diff * fraction / max) & 0x1f) << shift;
    }
    PaletteFade_LoadData(work->fade, &work->lampColors[lamp], PALFADE_BUFFER_SUB_OBJ, (lamp + 2) * 16 + 13, 2);
}

static void CGearSw_ReadLampPalette(CGearSw *work, const u16 *colors, int palette) {
    int i;

    for (i = 0; i < 5; i++) {
        work->lampPalettes[palette][i] = colors[palette * 16 + i];
    }
}

static BOOL CGearSw_TryLampPattern(CGearSw *work, int lamp, int palette, int pattern, int wantPattern,
                                   int wantLamp) {
    if (pattern == wantPattern && lamp == wantLamp) {
        work->lampTargets[lamp] = palette;
        work->lampPatterns[lamp] = pattern;
        return TRUE;
    }
    return FALSE;
}

static void CGearSwPattern_Init(CGearSwPattern *pattern, u16 max, u16 start, u8 loop) {
    pattern->count[2] = start;
    pattern->max = max;
    pattern->count[1] = 0;
    pattern->count[0] = start * 2;
    pattern->loop = loop;
    pattern->active = TRUE;
}

static void CGearSwPattern_Update(CGearSwPattern *pattern) {
    int lamp;

    if (pattern->active) {
        for (lamp = 0; lamp < LAMP_COUNT; lamp++) {
            if (pattern->count[lamp] < pattern->max) {
                pattern->count[lamp]++;
            }
        }
    }
}

static BOOL CGearSwPattern_IsFinished(CGearSwPattern *pattern, int lamp) {
    if (pattern->count[lamp] >= pattern->max) {
        return TRUE;
    }
    return FALSE;
}

static void CGearSwPattern_Loop(CGearSwPattern *pattern, int lamp) {
    if (pattern->loop) {
        pattern->count[lamp] = 0;
    }
}

static void CGearSwPattern_Reset(CGearSwPattern *pattern, int lamp) {
    pattern->count[lamp] = 0;
}

static u16 CGearSwPattern_GetCount(CGearSwPattern *pattern, int lamp) {
    return pattern->count[lamp];
}

static void CGearSwPattern_SetActive(CGearSwPattern *pattern, u8 active) {
    pattern->active = active;
}

static BOOL CGearSw_IsScreenShown(CGearSw *work) {
    if (!GFL_WipeIsFinished()) {
        return FALSE;
    }
    if (gfxRegGetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR) != 0) {
        return FALSE;
    }
    if (!Field_GetSeasonBannerOverdrawFlag(GSYS_GetField(work->gsys))) {
        return TRUE;
    }
    return FALSE;
}

static void CGearSw_SetDark(CGearSw *work, BOOL dark) {
    u32 fraction = dark ? 8 : 0;
    u32 i = 0;
    BOOL animate = TRUE;

    if (dark) {
        animate = FALSE;
    }
    for (; i < NELEMS(sDarkActors); i++) {
        func_0204c520(work->actors[sDarkActors[i]], animate);
    }
    PaletteFade_BlendPalettes(work->fade, PALFADE_BUFFER_SUB_OBJ, 0x3fff, fraction, sDarkColors[work->gender]);
    PaletteFade_BlendPalettes(work->fade, PALFADE_BUFFER_SUB_BG, 0x3e, fraction, sDarkColors[work->gender]);
}

// Shows the buttons that can't be used while the player is called as dark
static void CGearSw_ShowAlert(CGearSw *work, BOOL alert) {
    u32 fraction;

    if (alert) {
        fraction = 8;
        CGearSw_SetActorAnim(work->actors[ACTOR_RADAR], 14);
        func_0204c378(work->actors[ACTOR_NOTICE], 1, 0);
        func_0204c520(work->actors[ACTOR_NOTICE], FALSE);
    } else {
        fraction = 0;
        if (work->radarNotice) {
            CGearSw_SetActorAnim(work->actors[ACTOR_RADAR], 15);
        } else {
            CGearSw_SetActorAnim(work->actors[ACTOR_RADAR], 12);
        }
        func_0204c378(work->actors[ACTOR_NOTICE], 0, 0);
        func_0204c520(work->actors[ACTOR_NOTICE], TRUE);
    }
    PaletteFade_BlendPalettes(work->fade, PALFADE_BUFFER_SUB_OBJ, 0x20, fraction, sDarkColors[work->gender]);
}

static void CGearSw_StopFade(CGearSw *work) {
    PaletteFade_RequestStop(work->fade);
}

static void CGearSw_LoadBG(CGearSw *work, int gender, int skin, BOOL palettes) {
    NNSG2dPaletteData *palette;
    void *file;
    u16 *colors;
    int i;

    if (palettes) {
        GFL_G2DIOLoadArcNCLRDefault(work->arc, gender + 4, 4, 0, 0x1c0, HEAPID_TAIL(work->heapId));

        file = GFL_G2DIOReadNCLRArc(work->arc, gender + 14, &palette, HEAPID_TAIL(work->heapId));
        colors = palette->rawData;
        for (i = 0; i < LAMP_COUNT; i++) {
            work->lampBaseColors[i] = colors[(i + 2) * 16 + 13];
        }
        GFL_HeapFree(file);

        file = GFL_G2DIOReadNCLRArc(work->arc, 13, &palette, HEAPID_TAIL(work->heapId));
        for (i = 0; i < 9; i++) {
            CGearSw_ReadLampPalette(work, palette->rawData, i);
        }
        GFL_HeapFree(file);

        // Six color schemes for each gender
        file = GFL_G2DIOReadNCLRArc(work->arc, 6, &palette, HEAPID_TAIL(work->heapId));
        colors = palette->rawData;
        for (i = 0; i < PANEL_COUNT + 1; i++) {
            sys_memcpy(&colors[(i + gender * 6) * 16], work->panelColors[i], sizeof(work->panelColors[i]));
        }
        GFL_HeapFree(file);

        file = GFL_G2DIOReadNCLRArc(work->arc, 7, &palette, HEAPID_TAIL(work->heapId));
        colors = palette->rawData;
        for (i = 0; i < PANEL_COUNT + 1; i++) {
            sys_memcpy(&colors[(i + gender * 6) * 16], work->skinColors[i], sizeof(work->skinColors[i]));
        }
        GFL_HeapFree(file);
    }
    GFL_BGSysLoadArcNCGRStatic(work->arc, 8, 4, 0, 0, FALSE, HEAPID_TAIL(HEAPID_FIELD_MENU));
    GFL_BGSysClearChar(5);
    loadBGScrToVramByFileNoReserveNegAlign(work->arc, 9, 4, 0, 0x800, FALSE, HEAPID_TAIL(HEAPID_FIELD_MENU));
    if (work->beacons) {
        loadBGScrToVramByFileNoReserveNegAlign(work->arc, 11, 6, 0, 0x800, FALSE, HEAPID_TAIL(HEAPID_FIELD_MENU));
    } else {
        loadBGScrToVramByFileNoReserveNegAlign(work->arc, 10, 6, 0, 0x800, FALSE, HEAPID_TAIL(HEAPID_FIELD_MENU));
    }
    loadBGScrToVramByFileNoReserveNegAlign(work->arc, 12, 7, 0, 0x800, FALSE, HEAPID_TAIL(HEAPID_FIELD_MENU));
}

static void CGearSw_LoadObjRes(CGearSw *work, int gender) {
    NNSG2dPaletteData *palette;
    void *file;

    work->objRes[0] = func_0204bba0(work->arc, work->gender + 14, 1, 0, work->heapId);
    work->objRes[1] = func_0204b81c(work->arc, 16, FALSE, 1, work->heapId);
    work->objRes[2] = func_0204b81c(work->arc, work->gender * SKIN_COUNT + work->skin + 19, FALSE, 1, work->heapId);
    work->objRes[3] = func_0204bde0(work->arc, 17, 18, work->heapId);
    work->objRes[4] = func_0204bde0(work->arc, 29, 30, work->heapId);

    file = GFL_G2DIOReadNCLRArc(work->arc, 13, &palette, work->heapId);
    sys_memcpy16(&((u16 *)palette->rawData)[9 * 16], work->beaconColors, sizeof(work->beaconColors));
    GFL_HeapFree(file);

    func_02042f2c(200, 10);
    func_02042ba8(FALSE, work->heapId);
}

static void CGearSw_LoadSkinChars(CGearSw *work) {
    NNSG2dCharacterData *character;
    void *file = GFL_ArcToolReadHeapNew(work->arc, work->gender * SKIN_COUNT + work->skin + 19,
                                        HEAPID_TAIL(work->heapId));

    NNS_G2dGetUnpackedCharacterData(file, &character);
    func_0204ba40(work->objRes[2], character);
    GFL_HeapFree(file);
}

static void CGearSw_InitBG(CGearSw *work) {
    int bg;

    for (bg = 4; bg <= 7; bg++) {
        GFL_BGSysSetBGEnabled(bg, FALSE);
    }
    {
        BGSetup setup = {
            0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6000), GX_BG_CHARBASE(0x00000), 0x4000,
            GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
        };

        GFL_BGSysCreateBG(7, &setup, 0);
        GFL_BGSysSetBGEnabled(7, TRUE);
        GFL_BGSysSetBGPriority(7, 3);
        GFL_BGSysFillScrArea(7, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(7);
    }
    {
        BGSetup setup = {
            0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x6800), GX_BG_CHARBASE(0x00000), 0x4000,
            GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
        };

        GFL_BGSysCreateBG(6, &setup, 0);
        GFL_BGSysSetBGEnabled(6, TRUE);
        GFL_BGSysSetBGPriority(6, 2);
        GFL_BGSysFillScrArea(6, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(6);
    }
    {
        BGSetup setup = {
            0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7000), GX_BG_CHARBASE(0x00000), 0x4000,
            GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
        };

        GFL_BGSysCreateBG(4, &setup, 0);
        GFL_BGSysSetBGEnabled(4, TRUE);
        GFL_BGSysSetBGPriority(4, 0);
        GFL_BGSysFillScrArea(4, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(4);
    }
    {
        // The skin's
        BGSetup setup = {
            0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0x7800), GX_BG_CHARBASE(0x04000), 0x2000,
            GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE,
        };

        GFL_BGSysCreateBG(5, &setup, 0);
        GFL_BGSysSetBGEnabled(5, TRUE);
        GFL_BGSysSetBGPriority(5, 1);
        GFL_BGSysFillScrArea(5, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(5);
    }
    G2S_BlendNone();
    GXS_SetVisibleWnd(0);
}

static void CGearSw_SetSkinBlend(CGearSw *work) {
    gfxRegSetAlphaBlend(REG_DB_BLDCNT_ADDR, GX_PLANEMASK_BG1,
                        GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ, 21, 10);
}

// The markers and the buttons. Without the beacons only the Xtransceiver's marker can be touched
static int CGearSw_GetTouchedButton(CGearSw *work) {
    TouchRect rects[MARKER_COUNT + NELEMS(sCGearSwButtonRects) + 1];
    ClActorPos pos;
    int i;
    int touched;

    for (i = 0; i < MARKER_COUNT; i++) {
        TouchRect *rect = &rects[i];

        CGearSw_GetMarkerPos(work, i, &pos);
        rect->top = TOUCH_RECT_CIRCLE;
        rect->bottom = pos.x;
        rect->left = pos.y;
        rect->right = 20;
    }
    sys_memcpy(sCGearSwButtonRects, &rects[MARKER_COUNT], sizeof(sCGearSwButtonRects));
    rects[MARKER_COUNT + NELEMS(sCGearSwButtonRects)].top = TOUCH_RECT_END;
    touched = func_0203da0c(rects);
    if (touched == TOUCH_RECT_NONE || (!work->beacons && touched != TOUCH_POWER)) {
        touched = TOUCH_RECT_NONE;
    }
    return touched;
}

static int CGearSw_GetTouchedColorButton(CGearSw *work) {
    int touched;

    if (!work->beacons || !work->editing) {
        return TOUCH_RECT_NONE;
    }
    touched = func_0203da0c(sColorButtonRects);
    if (touched == TOUCH_RECT_NONE || touched % 2 == 0) {
        return TOUCH_RECT_NONE;
    }
    return touched / 2 + TOUCH_COLOR;
}

static void CGearSw_DragMarker(CGearSw *work) {
    u32 x;
    u32 y;

    if (!func_0203da84(&x, &y)) {
        u32 pos = CGearSw_FindMarkerPos(work, work->marker, work->touchX, work->touchY);

        if (pos != 0xff) {
            work->markerPos[work->marker] = pos;
            func_020098f4(work->settings, work->marker, pos);
        }
        CGearSw_PlaceMarker(work, work->marker, work->markerPos[work->marker]);
        CGearSw_SetDragging(work, work->marker, FALSE);
    } else {
        work->touchX = x;
        work->touchY = y;
        CGearSw_SetMarkerPos(work, work->marker, x, y);
    }
}

static void CGearSw_UpdateTouch(CGearSw *work) {
    ClActorPos pos;
    int touched;

    if (CGearSw_IsDark(work) || work->touchAction != ACTION_NONE) {
        return;
    }
    if (work->dragging) {
        CGearSw_DragMarker(work);
        return;
    }
    if (!func_0203da48()) {
        return;
    }
    touched = CGearSw_GetTouchedButton(work);
    if (touched == TOUCH_RECT_NONE) {
        touched = CGearSw_GetTouchedColorButton(work);
    }
    if (touched == TOUCH_RECT_NONE) {
        return;
    }
    func_0203d564(TRUE);
    switch (touched) {
    case TOUCH_MARKER:
    case TOUCH_MARKER + 1:
    case TOUCH_MARKER + 2:
        CGearSw_GetMarkerPos(work, touched, &pos);
        work->touchX = pos.x;
        work->touchY = pos.y;
        work->marker = touched;
        if (work->editing) {
            CGearSw_SetDragging(work, touched, TRUE);
        } else {
            work->touchAction = sMarkerActions[touched];
        }
        break;
    case TOUCH_EDIT:
        GFL_SndSEPlay(SEQ_SE_MSCL_07);
        work->editing ^= TRUE;
        CGearSw_SetEditing(work, work->editing);
        break;
    case TOUCH_HELP:
        work->touchAction = ACTION_HELP;
        break;
    case TOUCH_LOG:
        work->touchAction = ACTION_LOG;
        break;
    case TOUCH_RADAR:
        work->touchAction = ACTION_RADAR;
        break;
    case TOUCH_POWER:
        work->touchAction = ACTION_POWER_OFF;
        break;
    case TOUCH_SKIN:
        GFL_SndSEPlay(SEQ_SE_MSCL_07);
        work->skin++;
        work->skin %= SKIN_COUNT;
        func_020098c4(work->settings, work->skin);
        CGearSw_LoadSkinChars(work);
        break;
    case TOUCH_COLOR:
    case TOUCH_COLOR + 1:
    case TOUCH_COLOR + 2:
    case TOUCH_COLOR + 3:
    case TOUCH_COLOR + 4:
    case TOUCH_COLOR + 5: {
        u8 panel = touched - TOUCH_COLOR;

        GFL_SndSEPlay(SEQ_SE_MSCL_07);
        work->panels[panel]++;
        work->panels[panel] %= 6;
        func_020098dc(work->settings, panel, work->panels[panel]);
        if (panel == PANEL_COUNT) {
            CGearSw_LoadSkinColors(work, work->panels[panel]);
        } else {
            CGearSw_LoadPanelColors(work, panel, work->panels[panel]);
        }
        break;
    }
    }
}

static void CGearSw_SetEditing(CGearSw *work, BOOL editing) {
    int mode;
    int i;

    if (editing) {
        CGearSw_SetActorAnim(work->actors[ACTOR_EDIT], 11);
        mode = MARKER_EDIT;
    } else {
        CGearSw_SetActorAnim(work->actors[ACTOR_EDIT], 10);
        mode = MARKER_NORMAL;
    }
    for (i = 0; i < MARKER_COUNT; i++) {
        CGearSw_SetMarkerMode(work, i, mode);
    }
}

// How far each digit of the clock is from the one before
static const u8 sClockDigitSteps[4] = { 0, 6, 9, 6 };

// The actors from ACTOR_CLOCK on
static const CGearSwActorData sActorData[ACTOR_COUNT - ACTOR_CLOCK] = {
    { 1, 48, 22, 1, 2, 0 },   { 5, 208, 22, 1, 2, 0 },  { 21, 128, 10, 1, 1, 0 }, { 6, 200, 180, 1, 1, 0 },
    { 8, 215, 163, 1, 1, 0 }, { 10, 228, 144, 1, 1, 0 }, { 12, 35, 160, 1, 1, 0 }, { 3, 128, 96, 1, 7, 0 },
    { 16, 128, 86, 1, 7, 0 }, { 0, 68, 60, 1, 5, 1 },   { 22, 68, 60, 1, 6, 0 },  { 1, 128, 164, 1, 5, 1 },
    { 23, 128, 164, 1, 6, 0 }, { 2, 188, 60, 1, 5, 1 },  { 24, 188, 60, 1, 6, 0 }, { 28, 68, 60, 1, 3, 0 },
};

static const CGearSwMarkerPos sMarkerPositions[MARKER_POS_COUNT] = {
    { 16, 96, 4 },   { 40, 96, 3 },   { 60, 96, 2 },   { 196, 96, 2 },  { 216, 96, 3 },  { 240, 96, 4 },
    { 128, 64, 0 },  { 128, 128, 0 }, { 128, 144, 1 }, { 128, 164, 2 }, { 128, 184, 3 }, { 52, 52, 3 },
    { 68, 60, 2 },   { 88, 72, 1 },   { 168, 120, 1 }, { 188, 132, 2 }, { 204, 140, 3 }, { 52, 140, 3 },
    { 68, 132, 2 },  { 88, 120, 1 },  { 168, 72, 1 },  { 188, 60, 2 },  { 204, 52, 3 },  { 84, 20, 3 },
    { 96, 36, 2 },   { 104, 56, 1 },  { 112, 68, 0 },  { 144, 124, 0 }, { 152, 136, 1 }, { 160, 156, 2 },
    { 172, 172, 3 }, { 84, 172, 3 },  { 96, 156, 2 },  { 104, 136, 1 }, { 112, 124, 0 }, { 144, 68, 0 },
    { 152, 56, 1 },  { 160, 36, 2 },  { 172, 20, 3 },  { 128, 48, 1 },  { 32, 40, 4 },   { 224, 40, 4 },
};

static const ClActSurfaceSetup sSurfaceSetup = { 0, 0, 256, 192, 1, 2 };

// The battery icon's frame, by the DSi's battery level
static u8 sBatteryFrames[8] = { 0, 0, 1, 1, 2, 3 };

static ClActor *CGearSw_AddActor(CGearSw *work, s16 x, s16 y, u8 anim, u8 bgPriority, u8 priority, u8 res) {
    ClActorSetup setup;
    ClActor *actor;

    setup.x = x;
    setup.y = y;
    setup.sequence = anim;
    setup.priority = priority;
    setup.bgPriority = bgPriority;
    actor = func_0204c040(work->unit, work->objRes[1 + res], work->objRes[0], work->objRes[3 + res], &setup, 0,
                          work->heapId);
    func_0204c124(actor, FALSE);
    func_0204c520(actor, TRUE);
    func_0204c5c8(actor, FALSE);
    return actor;
}

// A marker is two actors, its base and its lamp
static void CGearSw_SetMarkerMode(CGearSw *work, int marker, int mode) {
    int actor = ACTOR_MARKER + marker * 2;

    switch (mode) {
    case MARKER_NORMAL:
        CGearSw_SetActorAnim(work->actors[actor], marker);
        func_0204c124(work->actors[actor], TRUE);
        CGearSw_SetActorAnim(work->actors[actor + 1], marker + 22);
        func_0204c438(work->actors[actor + 1], 6);
        func_0204c5c8(work->actors[actor + 1], FALSE);
        func_0204c124(work->actors[actor + 1], TRUE);
        break;
    case MARKER_EDIT:
        CGearSw_SetActorAnim(work->actors[actor], marker);
        func_0204c124(work->actors[actor], TRUE);
        CGearSw_SetActorAnim(work->actors[actor + 1], marker + 25);
        func_0204c438(work->actors[actor + 1], 6);
        func_0204c5c8(work->actors[actor + 1], TRUE);
        func_0204c124(work->actors[actor + 1], TRUE);
        break;
    case MARKER_DRAG:
        func_0204c124(work->actors[actor], FALSE);
        CGearSw_SetActorAnim(work->actors[actor + 1], marker + 25);
        func_0204c438(work->actors[actor + 1], 0);
        func_0204c5c8(work->actors[actor + 1], TRUE);
        func_0204c124(work->actors[actor + 1], TRUE);
        break;
    }
}

static void CGearSw_GetMarkerPos(CGearSw *work, int marker, ClActorPos *pos) {
    func_0204c178(work->actors[ACTOR_MARKER + marker * 2], pos, 0);
}

static void CGearSw_SetMarkerPos(CGearSw *work, int marker, s16 x, s16 y) {
    int i;

    for (i = 0; i < 2; i++) {
        CGearSw_SetActorPos(work->actors[ACTOR_MARKER + marker * 2 + i], x, y);
    }
}

static void CGearSw_SetMarkerVisible(CGearSw *work, int marker, BOOL visible) {
    int i;

    for (i = 0; i < 2; i++) {
        func_0204c124(work->actors[ACTOR_MARKER + marker * 2 + i], visible);
    }
}

static void CGearSw_SetActorPos(ClActor *actor, s16 x, s16 y) {
    ClActorPos pos;

    pos.x = x;
    pos.y = y;
    func_0204c140(actor, &pos, 0);
}

static void CGearSw_SetActorAnim(ClActor *actor, u32 anim) {
    func_0204c488(actor, anim);
    func_0204c56c(actor);
}

static void CGearSw_AddActors(CGearSw *work) {
    int i;
    int x;

    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    x = 33;
    for (i = 0; i < 4; i++) {
        x += sClockDigitSteps[i];
        work->actors[ACTOR_CLOCK_DIGIT + i] = CGearSw_AddActor(work, x, 18, 0, 1, 1, 0);
        func_0204c520(work->actors[ACTOR_CLOCK_DIGIT + i], FALSE);
    }
    for (i = 0; i < ACTOR_COUNT - ACTOR_CLOCK; i++) {
        work->actors[ACTOR_CLOCK + i] =
            CGearSw_AddActor(work, sActorData[i].x, sActorData[i].y, sActorData[i].anim, sActorData[i].bgPriority,
                             sActorData[i].priority, sActorData[i].res);
    }
    func_0204c520(work->actors[ACTOR_CLOCK], FALSE);
    if (hw_isDSi()) {
        CGearSw_SetActorAnim(work->actors[ACTOR_BATTERY], 4);
    }
    func_0204c5c8(work->actors[ACTOR_TOUCH], TRUE);
    CGearSw_UpdateClock(work);
}

static void CGearSw_SetActorsVisible(CGearSw *work, BOOL visible) {
    int i;

    for (i = 0; i < TOUCH_EFFECT_COUNT; i++) {
        if (work->touchEffects[i].actor != NULL) {
            func_0204c124(work->touchEffects[i].actor, visible);
        }
    }
    for (i = 0; i < ACTOR_TOUCH; i++) {
        if (work->actors[i] != NULL) {
            // Without the beacons only the clock, the battery and the power button show
            if (i > ACTOR_POWER) {
                if (work->beacons || !visible) {
                    func_0204c124(work->actors[i], visible);
                }
            } else {
                func_0204c124(work->actors[i], visible);
            }
        }
    }
    if (!work->noticeShown) {
        func_0204c124(work->actors[ACTOR_NOTICE], FALSE);
    }
    if (visible == TRUE) {
        CGearSw_UpdateClock(work);
    }
}

static void CGearSw_HidePanels(CGearSw *work) {
    CGearSw_BlendPanels(work);
    CGearSw_TurnLampsOff(work);
    CGearSw_SetActorsVisible(work, FALSE);
}

static void CGearSw_ShowPanels(CGearSw *work) {
    CGearSw_SetNotice(work, NOTICE_NONE);
    GFL_TCBExMgrFreeTasks(work->tcbExMgr);
    CGearSw_SetActorsVisible(work, TRUE);
    CGearSw_LoadColors(work);
}

// The panels flash one after the other, once the screen shows
static BOOL CGearSw_FlashPanels(CGearSw *work) {
    int i;

    switch (work->panelSeq) {
    case 0:
        CGearSw_HidePanels(work);
        work->panelSeq++;
        break;
    case 1:
        if (CGearSw_IsScreenShown(work)) {
            work->panelTasks = 0;
            for (i = 0; i < PANEL_COUNT; i++) {
                CGearSw_AddPanelTask(work, FALSE, i, work->panels[i], i * 3, &work->panelTasks);
            }
            work->panelSeq++;
        }
        break;
    case 2:
        if (work->panelTasks == 0) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// The panels appear from the outermost in
static BOOL CGearSw_OpenPanels(CGearSw *work) {
    int i;

    switch (work->panelSeq) {
    case 0:
        work->panelTasks = 0;
        for (i = PANEL_COUNT - 1; i >= 0; i--) {
            CGearSw_AddPanelTask(work, TRUE, i, work->panels[i], (PANEL_COUNT - 1 - i) * 3, &work->panelTasks);
        }
        work->panelSeq++;
        break;
    case 1:
        if (work->panelTasks == 0) {
            CGearSw_ShowPanels(work);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static void CGearSw_AddPanelTask(CGearSw *work, u8 appear, u8 panel, u8 color, u8 delay, u32 *counter) {
    CGearSwPanelTask *task =
        GFL_TCBExGetData(GFL_TCBExMgrAddTask(work->tcbExMgr, CGearSw_PanelTask, sizeof(CGearSwPanelTask), 0));

    sys_memset(task, 0, sizeof(CGearSwPanelTask));
    task->work = work;
    task->appear = appear;
    task->panel = panel;
    task->color = color;
    task->delay = delay;
    task->fraction = -16;
    task->target = 0;
    task->step = 2;
    CGearSw_LoadPanelColors(work, panel, color);
    CGearSw_BlendPanel(work, panel, 16, sDarkColors[work->gender]);
    if (counter != NULL) {
        task->counter = counter;
        (*task->counter)++;
    }
}

static void CGearSw_PanelTask(TCBEx *task, void *data) {
    CGearSwPanelTask *panelTask = data;
    CGearSw *work = panelTask->work;
    u16 colors[2];
    int fraction;
    int alpha;
    int index;

    colors[0] = 0x7fff;
    colors[1] = sDarkColors[work->gender];
    switch (panelTask->seq) {
    case 0:
        if (panelTask->delay-- == 0) {
            panelTask->seq++;
        }
        break;
    case 1:
        panelTask->fraction += panelTask->step;
        fraction = panelTask->fraction;
        alpha = fraction < 0 ? -fraction : fraction;
        index = fraction > 0 ? 0 : 1;
        CGearSw_BlendPanel(work, panelTask->panel, alpha, colors[index]);
        if (panelTask->fraction == panelTask->target) {
            if (!panelTask->appear) {
                panelTask->step = -2;
                panelTask->target = -16;
            } else {
                panelTask->step = 0;
                panelTask->target = 0;
                CGearSw_SetPanelVisible(work, panelTask->panel, TRUE);
            }
            panelTask->seq++;
        }
        break;
    case 2:
        panelTask->fraction += panelTask->step;
        fraction = panelTask->fraction;
        alpha = fraction < 0 ? -fraction : fraction;
        index = fraction > 0 ? 0 : 1;
        CGearSw_BlendPanel(work, panelTask->panel, alpha, colors[index]);
        if (panelTask->fraction != panelTask->target) {
            break;
        }
        // fallthrough
    default:
        if (panelTask->counter != NULL) {
            (*panelTask->counter)--;
        }
        GFL_TCBExRequestEnd(task);
        break;
    }
}

static void CGearSw_SetNoticeGlow(CGearSw *work, u8 fraction) {
    PaletteFade_BlendBuffer(work->fade, PALFADE_BUFFER_SUB_BG, 17, 3, fraction, 0x7fff);
}

static void CGearSw_StartNoticeGlow(CGearSw *work) {
    TCBEx *task;
    CGearSwNoticeTask *noticeTask;

    if (work->noticeTask != NULL) {
        work->newNotice = TRUE;
        return;
    }
    task = GFL_TCBExMgrAddTask(work->tcbExMgr, CGearSw_NoticeTask, sizeof(CGearSwNoticeTask), 0);
    noticeTask = GFL_TCBExGetData(task);
    sys_memset(noticeTask, 0, sizeof(CGearSwNoticeTask));
    noticeTask->work = work;
    noticeTask->unk5 = 2;
    noticeTask->fraction = 0;
    noticeTask->max = 10;
    noticeTask->step = 1;
    work->noticeTask = task;
    work->newNotice = TRUE;
}

static void CGearSw_StopNoticeGlow(CGearSw *work) {
    if (work->noticeTask == NULL) {
        work->newNotice = FALSE;
        return;
    }
    CGearSw_SetNoticeGlow(work, 0);
    GFL_TCBExRequestEnd(work->noticeTask);
    work->noticeTask = NULL;
    work->newNotice = FALSE;
}

static void CGearSw_NoticeTask(TCBEx *task, void *data) {
    CGearSwNoticeTask *noticeTask = data;
    CGearSw *work = noticeTask->work;

    if (!CGearSw_IsDarkOrChanging(work)) {
        noticeTask->fraction += noticeTask->step;
        CGearSw_SetNoticeGlow(work, noticeTask->fraction);
        if (noticeTask->fraction == 0 || noticeTask->fraction == noticeTask->max) {
            noticeTask->step *= -1;
        }
    }
}

// Shows the favorite colors of the players the last beacons came from
static void CGearSw_UpdateBeacons(CGearSw *work) {
    int beaconNo;
    int shown = 0;
    int index;
    int i;

    if (!work->beacons) {
        return;
    }
    if (CGearSw_IsCalled(work)) {
        return;
    }
    beaconNo = GameBeaconSys_GetReceiveCount();
    if (!work->newBeacon) {
        index = 0;
        if (GameBeaconSys_GetNextNew(&index) != 30) {
            work->newBeacon = TRUE;
        }
    }
    beaconNo--;
    for (i = 0; i < 30; i++) {
        if (shown >= BEACON_COUNT) {
            break;
        }
        if (beaconNo < 0) {
            PaletteFade_LoadData(work->fade, &work->beaconColors[16], PALFADE_BUFFER_SUB_OBJ, 0x51 + shown, 2);
            shown++;
        } else {
            GameBeacon *beacon = GameBeaconSys_GetRecent(beaconNo);

            if (beacon != NULL) {
                PaletteFade_LoadData(work->fade, &work->beaconColors[GameBeacon_GetFavoriteColor(beacon)],
                                     PALFADE_BUFFER_SUB_OBJ, 0x51 + shown, 2);
                if (!work->noticeShown) {
                    work->noticeShown = TRUE;
                    func_0204c124(work->actors[ACTOR_NOTICE], TRUE);
                }
                shown++;
            }
            beaconNo--;
        }
    }
}

static void CGearSw_LoadSkinColors(CGearSw *work, int color) {
    PaletteFade_LoadData(work->fade, work->skinColors[color], PALFADE_BUFFER_SUB_BG, 0,
                         sizeof(work->skinColors[color]));
}

static void CGearSw_LoadMarkerColors(CGearSw *work, int marker, int color) {
    u16 *colors = work->panelColors[color];

    PaletteFade_LoadData(work->fade, &colors[3], PALFADE_BUFFER_SUB_OBJ, marker * 16 + 0x21, 6);
}

static void CGearSw_LoadPanelColors(CGearSw *work, u8 panel, u8 color) {
    u16 *colors = work->panelColors[color];
    int i;

    PaletteFade_LoadData(work->fade, colors, PALFADE_BUFFER_SUB_BG, panel * 3 + 0x11, 6);
    for (i = 0; i < MARKER_COUNT; i++) {
        if (panel == CGearSw_GetMarkerPanel(work, i)) {
            CGearSw_LoadMarkerColors(work, i, color);
        }
    }
    switch (panel) {
    case 2:
        PaletteFade_LoadData(work->fade, &colors[3], PALFADE_BUFFER_SUB_OBJ, 4, 6);
        PaletteFade_LoadData(work->fade, &colors[6], PALFADE_BUFFER_SUB_OBJ, 0x14, 6);
        break;
    case 4:
        PaletteFade_LoadData(work->fade, &colors[3], PALFADE_BUFFER_SUB_OBJ, 1, 6);
        PaletteFade_LoadData(work->fade, &colors[6], PALFADE_BUFFER_SUB_OBJ, 0x11, 6);
        break;
    }
}

static void CGearSw_BlendPanel(CGearSw *work, u8 panel, u8 fraction, u16 color) {
    int i;

    PaletteFade_BlendBuffer(work->fade, PALFADE_BUFFER_SUB_BG, panel * 3 + ((1 - work->beacons) * 16 + 0x11), 3,
                            fraction, color);
    for (i = 0; i < MARKER_COUNT; i++) {
        if (panel == CGearSw_GetMarkerPanel(work, i)) {
            PaletteFade_BlendBuffer(work->fade, PALFADE_BUFFER_SUB_OBJ, panel * 16 + 0x21, 3, fraction, color);
        }
    }
    switch (panel) {
    case 2:
        PaletteFade_BlendBuffer(work->fade, PALFADE_BUFFER_SUB_OBJ, 4, 3, fraction, color);
        PaletteFade_BlendBuffer(work->fade, PALFADE_BUFFER_SUB_OBJ, 0x14, 3, fraction, color);
        break;
    case 4:
        PaletteFade_BlendBuffer(work->fade, PALFADE_BUFFER_SUB_OBJ, 1, 3, fraction, color);
        PaletteFade_BlendBuffer(work->fade, PALFADE_BUFFER_SUB_OBJ, 0x11, 3, fraction, color);
        break;
    }
}

// Shows what is on a panel
static void CGearSw_SetPanelVisible(CGearSw *work, int panel, BOOL visible) {
    int i;

    if (work->beacons) {
        for (i = 0; i < MARKER_COUNT; i++) {
            if (panel == CGearSw_GetMarkerPanel(work, i)) {
                CGearSw_SetMarkerVisible(work, i, visible);
            }
        }
    }
    switch (panel) {
    case 0:
        if (work->beacons) {
            func_0204c124(work->actors[ACTOR_LOGO], visible);
            if (work->noticeShown) {
                func_0204c124(work->actors[ACTOR_NOTICE], visible);
            }
        }
        break;
    case 2:
        func_0204c124(work->actors[ACTOR_TITLE], visible);
        break;
    case 4:
        for (i = 0; i <= ACTOR_RADAR; i++) {
            if (i == ACTOR_TITLE) {
                continue;
            }
            if (!work->beacons && i > ACTOR_POWER) {
                continue;
            }
            func_0204c124(work->actors[i], visible);
        }
        break;
    }
}

static void CGearSw_LoadColors(CGearSw *work) {
    int i;

    for (i = 0; i < PANEL_COUNT + 1; i++) {
        work->panels[i] = func_020098e8(work->settings, (u8)i);
        if (i < PANEL_COUNT) {
            CGearSw_LoadPanelColors(work, i, work->panels[i]);
        } else {
            CGearSw_LoadSkinColors(work, work->panels[i]);
        }
    }
}

static void CGearSw_BlendPanels(CGearSw *work) {
    int i;

    CGearSw_LoadColors(work);
    for (i = 0; i < PANEL_COUNT; i++) {
        CGearSw_BlendPanel(work, i, 16, sDarkColors[work->gender]);
    }
}

static u8 CGearSw_GetMarkerPanel(CGearSw *work, int marker) {
    return sMarkerPositions[work->markerPos[marker]].panel;
}

static void CGearSw_PlaceMarker(CGearSw *work, int marker, int pos) {
    const CGearSwMarkerPos *data;

    work->markerPos[marker] = pos;
    data = &sMarkerPositions[pos];
    CGearSw_SetMarkerPos(work, marker, data->x, data->y);
    CGearSw_LoadMarkerColors(work, marker, work->panels[data->panel]);
}

static void CGearSw_PlaceMarkers(CGearSw *work) {
    int i;

    for (i = 0; i < MARKER_COUNT; i++) {
        work->markerPos[i] = func_02009900(work->settings, i);
        CGearSw_PlaceMarker(work, i, work->markerPos[i]);
    }
}

static fx32 CGearSw_GetDistance(int x1, int y1, int x2, int y2) {
    VecFx32 a;
    VecFx32 b;

    a.x = FX32_CONST(x1);
    a.y = FX32_CONST(y1);
    a.z = 0;
    b.x = FX32_CONST(x2);
    b.y = FX32_CONST(y2);
    b.z = 0;
    return vecfx_dist(&a, &b);
}

// Whether another marker is too close to a point for this one to be there
static BOOL CGearSw_IsMarkerNear(CGearSw *work, int marker, int x, int y) {
    ClActorPos pos;
    int i;

    for (i = 0; i < MARKER_COUNT; i++) {
        if (i == marker) {
            continue;
        }
        CGearSw_GetMarkerPos(work, i, &pos);
        if (CGearSw_GetDistance(pos.x, pos.y, x, y) <= FX32_CONST(40)) {
            return TRUE;
        }
    }
    return FALSE;
}

// The free place nearest to a point, or 0xff if none is near
static u32 CGearSw_FindMarkerPos(CGearSw *work, int marker, u8 x, u8 y) {
    fx32 nearestDistance = 0x7fffffff;
    int nearest;
    u32 i;

    for (i = 0; i < MARKER_POS_COUNT; i++) {
        const CGearSwMarkerPos *pos = &sMarkerPositions[i];

        if (!CGearSw_IsMarkerNear(work, marker, pos->x, pos->y)) {
            fx32 distance = CGearSw_GetDistance(pos->x, pos->y, x, y);

            if (distance < nearestDistance) {
                nearestDistance = distance;
                nearest = i;
            }
        }
    }
    if (nearestDistance > FX32_CONST(32)) {
        return 0xff;
    }
    return (u8)nearest;
}

static void CGearSw_SetDragging(CGearSw *work, int marker, BOOL dragging) {
    int i;

    work->dragging = dragging;
    if (dragging) {
        for (i = 0; i < MARKER_COUNT; i++) {
            if (marker == i) {
                CGearSw_SetMarkerMode(work, i, MARKER_DRAG);
            } else {
                CGearSw_SetMarkerMode(work, i, MARKER_NORMAL);
            }
        }
    } else {
        for (i = 0; i < MARKER_COUNT; i++) {
            CGearSw_SetMarkerMode(work, i, MARKER_EDIT);
        }
    }
}

static void CGearSw_InitGraphics(CGearSw *work, BOOL startup) {
    int i;

    work->gender = getTrainerGender(GetGameDataPlayerInfo(work->gameData)) & 1;
    work->skin = func_020098c8(work->settings);
    for (i = 0; i < PANEL_COUNT + 1; i++) {
        work->panels[i] = func_020098e8(work->settings, (u8)i);
    }
    work->unit = func_0204bf1c(84, 0, work->heapId);
    work->renderer = func_0204be9c(&sSurfaceSetup, 1, work->heapId);
    func_0204c018(work->unit, work->renderer);
    func_0204bff0(work->unit, TRUE);
    CGearSw_LoadBG(work, work->gender, work->skin, TRUE);
    CGearSw_LoadObjRes(work, work->gender);
    CGearSw_AddActors(work);
    if (startup) {
        CGearSw_SetActorsVisible(work, FALSE);
    } else {
        CGearSw_SetActorsVisible(work, TRUE);
    }
}

static void CGearSw_FreeObjects(CGearSw *work) {
    int i;

    CGearSw_ShowPanels(work);
    for (i = 0; i < ACTOR_COUNT; i++) {
        if (work->actors[i] != NULL) {
            func_0204c108(work->actors[i]);
            work->actors[i] = NULL;
        }
    }
    for (i = 0; i < TOUCH_EFFECT_COUNT; i++) {
        if (work->touchEffects[i].actor != NULL) {
            func_0204c108(work->touchEffects[i].actor);
            work->touchEffects[i].actor = NULL;
        }
    }
    func_0204be64(work->objRes[4]);
    func_0204be64(work->objRes[3]);
    func_0204b98c(work->objRes[2]);
    func_0204b98c(work->objRes[1]);
    func_0204bcd0(work->objRes[0]);
}

static void CGearSw_ExitGraphics(CGearSw *work) {
    CGearSw_FreeObjects(work);
    func_0204becc(work->renderer);
    func_0204bf98(work->unit);
    GFL_BGSysSetBGEnabled(7, FALSE);
    GFL_BGSysSetBGEnabled(6, FALSE);
    GFL_BGSysSetBGEnabled(5, FALSE);
    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysReleaseBG(5);
    GFL_BGSysReleaseBG(4);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(7);
}

static u8 CGearSw_GetBatteryFrame(void) {
    return sBatteryFrames[GCTX_HIDGetBatteryLevel()];
}

static void CGearSw_UpdateClock(CGearSw *work) {
    RTCTime time;
    ClActor *actor;
    int frame;

    RTC_GetCachedTime(&time);
    frame = time.hour / 12 + 1;
    if (frame != func_0204c4a0(work->actors[ACTOR_CLOCK])) {
        CGearSw_SetActorAnim(work->actors[ACTOR_CLOCK], frame);
    }
    actor = work->actors[ACTOR_CLOCK_DIGIT];
    frame = time.hour / 10;
    if (frame != func_0204c510(actor)) {
        func_0204c504(actor, frame);
    }
    actor = work->actors[ACTOR_CLOCK_DIGIT + 1];
    frame = time.hour % 10;
    if (frame != func_0204c510(actor)) {
        func_0204c504(actor, frame);
    }
    // The colon blinks
    frame = time.second & 1;
    actor = work->actors[ACTOR_CLOCK];
    if (frame != func_0204c510(actor)) {
        func_0204c504(actor, frame);
    }
    actor = work->actors[ACTOR_CLOCK_DIGIT + 2];
    frame = time.minute / 10;
    if (frame != func_0204c510(actor)) {
        func_0204c504(actor, frame);
    }
    actor = work->actors[ACTOR_CLOCK_DIGIT + 3];
    frame = time.minute % 10;
    if (frame != func_0204c510(actor)) {
        func_0204c504(actor, frame);
    }
    actor = work->actors[ACTOR_BATTERY];
    if (hw_isDSi()) {
        frame = CGearSw_GetBatteryFrame();
        if (frame != func_0204c510(actor)) {
            func_0204c504(actor, frame);
        }
    } else {
        frame = GCTX_HIDGetBatteryLevel() == 5 ? 1 : 0;
        if (frame != func_0204c510(actor)) {
            func_0204c504(actor, frame);
        }
    }
}

static void CGearSw_UpdateMain(CGearSw *work) {
    CGearSw_UpdateTouch(work);
    CGearSw_UpdateDark(work);
    CGearSw_UpdateAlert(work);
    CGearSw_UpdateClock(work);
    if (!CGearSw_IsDark(work)) {
        if (work->beacons) {
            CGearSw_UpdatePatterns(work);
            CGearSw_UpdateLamps(work);
            CGearSw_UpdateBeacons(work);
        }
    } else {
        work->touchAction = ACTION_NONE;
    }
}

static void CGearSw_StateMain(CGearSw *work) {
    if (work->callback != NULL) {
        work->callback(work->callbackWork);
    }
    work->callback = NULL;
    work->callbackWork = NULL;
    if (CGearSw_IsScreenShown(work)) {
        CGearSw_UpdateMain(work);
    }
}

static void CGearSw_StateOpen(CGearSw *work) {
    if (!CGearSw_OpenPanels(work)) {
        if (work->callback != NULL) {
            work->callback(work->callbackWork);
        }
        work->callback = NULL;
        work->callbackWork = NULL;
    } else {
        CGearSw_InitPatternsAtStartup(work);
        CGearSw_SetStateDebug(work, CGearSw_StateMain, 3934);
    }
}

static void CGearSw_StateFlash(CGearSw *work) {
    if (CGearSw_FlashPanels(work)) {
        CGearSw_SetStateDebug(work, CGearSw_StateOpen, 3947);
    }
}

static void CGearSw_StateWake(CGearSw *work) {
    work->wakeWait++;
    if (work->wakeWait >= 16) {
        work->wakeWait = 0;
        CGearSw_SetStateDebug(work, CGearSw_StateOpen, 3962);
    }
}

// Shows the downloaded skin, if its extra save block is the one the settings have the CRC of
static BOOL CGearSw_LoadSkin(CGearSw *work, GameSystem *gsys) {
    BOOL loaded = FALSE;
    CGearPictureSave *picture = GFL_HeapAllocate(HEAPID_FIELD_MENU, 0x2800, FALSE, "c_gear_sw.c", 3980);
    SaveControl *save = GameData_GetSaveControl(work->gameData);

    if (func_02007560(save, 4, HEAPID_FIELD_MENU, picture, 0x2800) == 1) {
        u16 crc = getCRC16(picture, sizeof(CGearPictureSave));

        if (crc == func_020098d8(func_02009918(save))) {
            if (func_0200ce5c(picture)) {
                loaded = TRUE;
                GFL_BGSysLoadChar(5, picture, 0x1fe0, 0);
                GFL_BGSysUploadStdPalette(5, &picture->chars[0x1fe0], 0x20, 13 * 0x20);
                GFL_BGSysBufferScrDefault(5, picture->unk2000, sizeof(picture->unk2000));
                GFL_BGSysSetScrPaletteNo(5, 0, 0, 32, 24, 13);
                GFL_BGSysLoadScr(5);
                CGearSw_SetSkinBlend(work);
            }
        }
    }
    freeIntermediateSaveExtraBlksAfterLoad2(save, 4);
    GFL_HeapFree(picture);
    return loaded;
}

static void CGearSw_SleepCallback(void *data) {
    CGearSw *work = data;

    CGearSw_SetNotice(work, NOTICE_NONE);
    GFL_TCBExMgrFreeTasks(work->tcbExMgr);
    CGearSw_StopFade(work);
    if (work->tcbMgr != NULL) {
        GFL_TCBMgrUpdate(work->tcbMgr);
    }
    CGearSw_HidePanels(work);
    work->sleep = SLEEP_ASLEEP;
}

static void CGearSw_WakeCallback(void *data) {
    CGearSw *work = data;

    work->sleep = SLEEP_WOKEN;
}

static void CGearSw_VBlankTask(TCB *tcb, void *data) {
    CGearSw *work = data;

    if (work->fade != NULL) {
        PaletteFade_Transfer(work->fade);
    }
}

static void CGearSw_Init(CGearSw *work, CGearSettings *settings, FieldSubscreen *subscreen, GameSystem *gsys,
                         BOOL startup) {
    GameCommSys *comm;
    BOOL active;

    work->heapId = HEAPID_FIELD_MENU;
    work->settings = settings;
    work->subscreen = subscreen;
    work->gsys = gsys;
    work->gameData = GSYS_GetGameData(gsys);
    work->active = TRUE;
    work->beacons = func_02016b34(work->gsys);
    work->unk30d = TRUE;
    work->touchAction = ACTION_NONE;
    work->action = ACTION_NONE;
    work->arc = GFL_ArcSysCreateFileHandle(ARCID_C_GEAR, work->heapId);
    work->tcbExMgr = GFL_TCBExMgrCreate(work->heapId, work->heapId, 16, 0x80);
    work->tcbMgrBuffer = GFL_HeapAllocate(work->heapId, GFL_TCBMgrCalcAllocSize(4), TRUE, "c_gear_sw.c", 4088);
    work->tcbMgr = GFL_TCBMgrCreate(4, work->tcbMgrBuffer);
    if (startup) {
        CGearSw_SetStateDebug(work, CGearSw_StateFlash, 4092);
    } else {
        CGearSw_InitPatterns(work);
        CGearSw_SetStateDebug(work, CGearSw_StateMain, 4095);
    }
    CGearSw_InitBG(work);
    CGearSw_InitGraphics(work, startup);
    if (func_020098d0(work->settings)) {
        CGearSw_LoadSkin(work, gsys);
    }
    work->fade = PaletteFade_Create(HEAPID_FIELD_MENU);
    active = TRUE;
    PaletteFade_SetTransferAll(work->fade, TRUE);
    PaletteFade_AllocBuffer(work->fade, PALFADE_BUFFER_SUB_OBJ, 0x1c0, HEAPID_FIELD_MENU);
    PaletteFade_AllocBuffer(work->fade, PALFADE_BUFFER_SUB_BG, 0x1c0, HEAPID_FIELD_MENU);
    PaletteFade_LoadFromVRAM(work->fade, PALFADE_VRAM_SUB_OBJ, 0, 0x1c0);
    PaletteFade_LoadFromVRAM(work->fade, PALFADE_VRAM_SUB_BG, 0, 0x1c0);
    CGearSw_PlaceMarkers(work);
    CGearSw_LoadColors(work);
    PaletteFade_Transfer(work->fade);
    work->vblankTcb = GFL_VBlankTCBAdd(CGearSw_VBlankTask, work, 1);
    GCTX_HIDSetSleepCallback(CGearSw_SleepCallback, work);
    GCTX_HIDSetWakeCallback(CGearSw_WakeCallback, work);
    CGearSwFlash_Init(work, &work->flash);
    CGearSw_UpdateBeacons(work);
    CGearSw_UpdateClock(work);
    if (!startup) {
        if (GSYS_GetEventRunningFlag(gsys)) {
            active = FALSE;
        }
        CGearSw_SetActive(work, active);
        CGearSw_ApplyDark(work);
    } else {
        CGearSw_StateFlash(work);
    }
    comm = GSYS_GetGameCommSystem(gsys);
    if (comm != NULL && func_ov012_02153650(comm) == 2) {
        func_ov012_02153654(comm);
    }
}

static void CGearSw_Exit(CGearSw *work) {
    GameCommSys *comm = GSYS_GetGameCommSystem(work->gsys);

    if (comm != NULL && func_ov012_02153650(comm) == 2) {
        func_ov012_02153654(comm);
    }
    CGearSw_StopNoticeGlow(work);
    CGearSwFlash_Exit(&work->flash);
    GCTX_HIDSetSleepCallback(NULL, NULL);
    GCTX_HIDSetWakeCallback(NULL, NULL);
    func_02042f40();
    func_02042ba8(FALSE, work->heapId);
    GFL_TCBRemove(work->vblankTcb);
    func_0203a610(work->tcbMgr);
    GFL_HeapFree(work->tcbMgrBuffer);
    GFL_TCBExMgrFreeTasks(work->tcbExMgr);
    GFL_TCBExMgrFree(work->tcbExMgr);
    PaletteFade_FreeBuffer(work->fade, PALFADE_BUFFER_SUB_OBJ);
    PaletteFade_FreeBuffer(work->fade, PALFADE_BUFFER_SUB_BG);
    PaletteFade_Free(work->fade);
    if (work->action != ACTION_TVT) {
        FieldSnd_StopRingtone(GameData_GetFieldSoundSystem(work->gameData));
    }
    GFL_ArcToolFree(work->arc);
    CGearSw_ExitGraphics(work);
    G2S_BlendNone();
    sys_memset(work, 0, sizeof(CGearSw));
}

CGearSw *CGearSw_Create(CGearSettings *settings, FieldSubscreen *subscreen, GameSystem *gsys, BOOL startup) {
    CGearSw *work = GFL_HeapAllocate(HEAPID_FIELD_MENU, sizeof(CGearSw), TRUE, "c_gear_sw.c", 4234);

    CGearSw_Init(work, settings, subscreen, gsys, startup);
    return work;
}

CGearSw *CGearSw_CreateWithCallback(CGearSettings *settings, FieldSubscreen *subscreen, GameSystem *gsys,
                                    CGearSwCallback callback, void *callbackWork, BOOL startup) {
    CGearSw *work = CGearSw_Create(settings, subscreen, gsys, startup);

    work->callback = callback;
    work->callbackWork = callbackWork;
    if (startup) {
        CGearSw_SetStateDebug(work, CGearSw_StateFlash, 4259);
        work->unk30d = FALSE;
    } else {
        if (work->callback != NULL) {
            work->callback(work->callbackWork);
        }
        work->callback = NULL;
        work->callbackWork = NULL;
    }
    return work;
}

void CGearSw_Update(CGearSw *work, BOOL active) {
    if (work->sleep == SLEEP_ASLEEP) {
        return;
    }
    CGearSw_SetActive(work, active);
    if (!CGearSw_IsDark(work)) {
        u32 intrude = func_ov012_02153658(GSYS_GetGameCommSystem(work->gsys));

        CGearSw_UpdateIntrudeAlert(work, intrude);
        CGearSw_UpdateCallAlert(work, intrude);
        CGearSw_UpdateNotices(work);
    }
    if (work->sleep == SLEEP_WOKEN) {
        work->sleep = SLEEP_NONE;
        if (!CGearSw_IsDark(work)) {
            CGearSw_SetStateDebug(work, CGearSw_StateWake, 4310);
        } else {
            CGearSw_ShowPanels(work);
            CGearSw_SetDark(work, CGearSw_IsDark(work));
            CGearSw_InitPatterns(work);
            CGearSw_SetStateDebug(work, CGearSw_StateMain, 4325);
        }
    }
    if (work->state != NULL) {
        work->state(work);
    }
    if (work->tcbExMgr != NULL) {
        GFL_TCBExMgrUpdate(work->tcbExMgr);
    }
    if (work->tcbMgr != NULL) {
        GFL_TCBMgrUpdate(work->tcbMgr);
    }
}

void CGearSw_Draw(CGearSw *work) {
}

void CGearSw_Free(CGearSw *work) {
    CGearSw_Exit(work);
    GFL_HeapFree(work);
}

BOOL CGearSw_Reload(CGearSw *work) {
    CGearSettings *settings = work->settings;
    FieldSubscreen *subscreen = work->subscreen;
    GameSystem *gsys = work->gsys;

    if (work->state == CGearSw_StateMain) {
        Wipe_SetScreenCovered(1, 0);
        CGearSw_Exit(work);
        CGearSw_Init(work, settings, subscreen, gsys, FALSE);
        killBrightnessEitherEngine(1);
        return TRUE;
    }
    return FALSE;
}

GameEvent *CGearSw_CheckEvent(CGearSw *work, BOOL enabled) {
    GameEvent *event = NULL;
    Field *field;

    GSYS_GetGameCommSystem(work->gsys);
    field = GSYS_GetField(work->gsys);
    if (!enabled || field == NULL) {
        return NULL;
    }
    switch (work->touchAction) {
    case ACTION_IRC:
    case ACTION_GSYNC:
        if (!CGearSw_IsCalled(work)) {
            GFL_SndSEPlay(SEQ_SE_DECIDE3);
            event = CGearSw_CreateEvent(work, work->touchAction);
        } else {
            // The lamps are busy with the call
            GFL_SndSEPlay(SEQ_SE_SYS_100);
            CGearSw_StartTouchEffect(work);
            CGearSw_SetStateDebug(work, CGearSw_StateRefused, 4425);
        }
        break;
    case ACTION_TVT:
        GFL_SndSEPlay(SEQ_SE_DECIDE3);
        event = CGearSw_CreateEvent(work, work->touchAction);
        break;
    case ACTION_HELP:
    case ACTION_POWER_OFF:
        GFL_SndSEPlay(SEQ_SE_MSCL_07);
        event = CGearSw_CreateEvent(work, work->touchAction);
        break;
    case ACTION_LOG:
    case ACTION_RADAR:
        if (!CGearSw_IsCalled(work)) {
            GFL_SndSEPlay(SEQ_SE_MSCL_07);
            event = CGearSw_CreateEvent(work, work->touchAction);
        } else {
            GFL_SndSEPlay(SEQ_SE_SYS_100);
        }
        break;
    }
    if (event != NULL) {
        work->action = work->touchAction;
        CGearSw_SetStateDebug(work, CGearSw_StateLeave, 4455);
    }
    work->touchAction = ACTION_NONE;
    return event;
}

BOOL CGearSw_IsStartingTvt(CGearSw *work) {
    if (work == NULL) {
        return FALSE;
    }
    if (work->action == ACTION_TVT) {
        return TRUE;
    }
    return FALSE;
}

void CGearSw_StopRinging(CGearSw *work) {
    if (work != NULL) {
        work->ringing = FALSE;
        CGearSw_SetAlert(work, FALSE, FALSE);
    }
}

static BOOL CGearSw_UpdateButtonAnim(CGearSw *work) {
    switch (work->effectSeq) {
    case 0:
        CGearSw_SetActorAnim(work->actors[work->buttonActor], work->buttonAnim);
        func_0204c520(work->actors[work->buttonActor], TRUE);
        func_0204c520(work->actors[work->buttonActor + 1], TRUE);
        work->effectSeq++;
        break;
    case 1:
        func_0204c520(work->actors[work->buttonActor], TRUE);
        func_0204c520(work->actors[work->buttonActor + 1], TRUE);
        if (!func_0204c560(work->actors[work->buttonActor])) {
            work->effectSeq = 0;
            return TRUE;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static BOOL CGearSw_UpdateButtonFlash(CGearSw *work) {
    switch (work->effectSeq) {
    case 0:
        if (work->buttonActor == ACTOR_EDIT) {
            CGearSwFlash_Start(&work->flash, 1 << 5);
        }
        work->effectSeq++;
        break;
    case 1:
        if (CGearSwFlash_Update(&work->flash)) {
            work->effectSeq = 0;
            return TRUE;
        }
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

static void CGearSw_StateLeave(CGearSw *work) {
}

static void CGearSw_SetEventDark(CGearSw *work) {
    if (!work->eventDark) {
        work->eventDark = TRUE;
        work->darkChanged = TRUE;
        work->darkWait = 0;
    }
}

static void CGearSw_SetCommDark(CGearSw *work) {
    if (!work->commDark) {
        work->commDark = TRUE;
        work->darkChanged = TRUE;
        work->darkWait = 0;
    }
}

static void CGearSw_ClearCommDark(CGearSw *work) {
    if (work->commDark) {
        work->commDark = FALSE;
        work->darkChanged = TRUE;
        work->darkWait = 0;
    }
}

static void CGearSw_ClearEventDark(CGearSw *work) {
    if (work->eventDark == TRUE) {
        work->eventDark = FALSE;
        work->darkChanged = TRUE;
        work->darkWait = 0;
    }
}

static void CGearSw_UpdateDark(CGearSw *work) {
    if (work->darkChanged) {
        work->darkWait++;
        if (work->darkWait >= 8) {
            CGearSw_SetDark(work, CGearSw_IsDark(work));
            work->darkChanged = FALSE;
        }
    }
}

static void CGearSw_ApplyDark(CGearSw *work) {
    if (work->darkChanged) {
        CGearSw_SetDark(work, CGearSw_IsDark(work));
        work->darkChanged = FALSE;
    }
}

static BOOL CGearSw_IsDark(CGearSw *work) {
    if (work->eventDark) {
        return TRUE;
    }
    if (work->commDark) {
        return TRUE;
    }
    return FALSE;
}

static BOOL CGearSw_IsDarkOrChanging(CGearSw *work) {
    if (CGearSw_IsDark(work)) {
        return TRUE;
    }
    return work->darkChanged;
}

static void CGearSw_UpdateAlert(CGearSw *work) {
    if (!CGearSw_IsDark(work) && work->alertChanged) {
        work->alertWait++;
        if (work->alertWait >= 8) {
            if (CGearSw_IsCalled(work)) {
                CGearSw_ShowAlert(work, TRUE);
            } else {
                CGearSw_ShowAlert(work, FALSE);
            }
            work->alertChanged = FALSE;
        }
    }
}

static void CGearSwFlash_Init(CGearSw *work, CGearSwFlash *flash) {
    flash->fade = work->fade;
    flash->tcbMgr = work->tcbMgr;
}

static void CGearSwFlash_Exit(CGearSwFlash *flash) {
}

static void CGearSwFlash_Start(CGearSwFlash *flash, u32 paletteMask) {
    flash->count = 0;
    flash->paletteMask = paletteMask;
}

static BOOL CGearSwFlash_Update(CGearSwFlash *flash) {
    u8 count = flash->count;

    if (count >= 12) {
        return TRUE;
    }
    flash->count++;
    PaletteFade_BlendPalettes(flash->fade, PALFADE_BUFFER_SUB_OBJ, flash->paletteMask, (count / 2) % 2 * 9, 0);
    return FALSE;
}

static GameEvent *CGearSw_CreateEvent(CGearSw *work, u32 action) {
    GameEvent *event = GameEvent_Create(work->gsys, NULL, CGearSw_EventMain, sizeof(CGearSwEvent));
    CGearSwEvent *data = GameEvent_GetData(event);

    data->work = work;
    work->effectSeq = 0;
    data->action = action;
    switch (action) {
    case ACTION_IRC:
    case ACTION_TVT:
    case ACTION_GSYNC:
        work->effect = EFFECT_TOUCH;
        CGearSw_ClearUnk2b9(data->work);
        break;
    case ACTION_LOG:
        work->effect = EFFECT_FLASH;
        work->buttonActor = ACTOR_EDIT;
        break;
    case ACTION_RADAR:
        work->effect = EFFECT_BUTTON;
        work->buttonActor = ACTOR_RADAR;
        work->buttonAnim = 13;
        break;
    case ACTION_HELP:
        work->effect = EFFECT_BUTTON;
        work->buttonActor = ACTOR_HELP;
        work->buttonAnim = 9;
        break;
    case ACTION_POWER_OFF:
        work->effect = EFFECT_BUTTON;
        work->buttonActor = ACTOR_POWER;
        work->buttonAnim = 7;
        break;
    }
    return event;
}

// Plays the touched button's effect, then becomes the event of what it starts
static GameEventReturnCode CGearSw_EventMain(GameEvent *event, u32 *state, void *data) {
    CGearSwEvent *eventData = data;
    CGearSw *work = eventData->work;
    Field *field = GSYS_GetField(work->gsys);
    GameEvent *next;
    BOOL finished;

    switch (eventData->work->effect) {
    case EFFECT_TOUCH:
        finished = CGearSw_UpdateTouchEffect(eventData->work);
        break;
    case EFFECT_FLASH:
        finished = CGearSw_UpdateButtonFlash(eventData->work);
        break;
    case EFFECT_BUTTON:
        finished = CGearSw_UpdateButtonAnim(eventData->work);
        break;
    default:
        finished = TRUE;
        break;
    }
    if (!finished) {
        return 0;
    }
    switch (eventData->action) {
    case ACTION_GSYNC:
        next = GameEvent_CreateOverlayDelegate(work->gsys, OVERLAY_ID(0), EventGameSync_CreateFromArgs, field);
        break;
    case ACTION_IRC:
        next = CallIRC(work->gsys, field, NULL, TRUE);
        break;
    case ACTION_TVT:
        next = GameEvent_CreateOverlayDelegate(work->gsys, OVERLAY_ID(17), func_ov017_0216e79c, NULL);
        break;
    case ACTION_RADAR:
        next = GameEvent_CreateOverlayDelegate(work->gsys, OVERLAY_ID(18), (GameEventProvider)func_ov018_0216e708,
                                               field);
        break;
    case ACTION_LOG:
        GameBeaconSys_ClearNotice(1);
        func_0201740c(work->gameData, 1);
        next = EventFieldMenuReturn_Create(work->gsys, field, 4);
        break;
    case ACTION_HELP: {
        CGHelpParam *param = GFL_HeapAllocate(HEAPID_GAMEEVENT, sizeof(CGHelpParam), TRUE, "c_gear_sw.c", 4957);

        param->playerInfo = GetGameDataPlayerInfo(work->gameData);
        param->gsys = work->gsys;
        next = EventFieldSubprocessTransition_CreateWithCallback(work->gsys, field, OVERLAY_ID(200),
                                                                 &CG_HELP_PROC_FUNCTIONS, param, NULL, param);
        break;
    }
    case ACTION_POWER_OFF:
        next = EventCGearShutdown_Create(work->gsys);
        break;
    }
    GameEvent_Replace(event, next);
    return 0;
}

static void CGearSw_StartTouchEffect(CGearSw *work) {
    work->unk2b9 = 0;
    CGearSw_AddTouchEffect(work, work->touchX, work->touchY);
}

static void CGearSw_FreeTouchEffects(CGearSw *work) {
    int i;

    for (i = 0; i < TOUCH_EFFECT_COUNT; i++) {
        if (work->touchEffects[i].actor != NULL) {
            func_0204c108(work->touchEffects[i].actor);
            work->touchEffects[i].actor = NULL;
        }
    }
}

static BOOL CGearSw_AreTouchEffectsFinished(CGearSw *work) {
    BOOL finished = TRUE;
    int i;

    for (i = 0; i < TOUCH_EFFECT_COUNT; i++) {
        if (work->touchEffects[i].actor != NULL && func_0204c560(work->touchEffects[i].actor)) {
            finished = FALSE;
            break;
        }
    }
    return finished;
}

static void CGearSw_StateRefused(CGearSw *work) {
    CGearSw_UpdateMain(work);
    if (CGearSw_AreTouchEffectsFinished(work)) {
        CGearSw_FreeTouchEffects(work);
        CGearSw_SetStateDebug(work, CGearSw_StateMain, 5045);
    }
}

static void CGearSw_SetActive(CGearSw *work, BOOL active) {
    if (!func_02042788() && work->beacons) {
        CGearSw_SetCommDark(work);
    } else {
        CGearSw_ClearCommDark(work);
    }
    if (work->active != active) {
        if (!active) {
            CGearSw_SetEventDark(work);
        } else {
            CGearSw_ClearEventDark(work);
        }
        work->active = active;
    }
}

static void CGearSw_UpdateIntrudeAlert(CGearSw *work, u32 state) {
    if (state == 3) {
        if (!work->intruded) {
            CGearSw_SetAlert(work, TRUE, FALSE);
            work->intruded = TRUE;
        }
    } else if (work->intruded == TRUE) {
        CGearSw_SetAlert(work, FALSE, FALSE);
        work->intruded = FALSE;
    }
}

// Rings while the Xtransceiver is called
static void CGearSw_UpdateCallAlert(CGearSw *work, u32 state) {
    if (state == 0) {
        u32 bits = func_02012be4(GameData_GetWifiList(work->gameData));
        FieldSound *fieldSound = GameData_GetFieldSoundSystem(work->gameData);

        if (bits & 8) {
            if (!work->ringing) {
                work->ringing = TRUE;
                CGearSw_SetAlert(work, TRUE, TRUE);
            }
            FieldSnd_RingRingtone(fieldSound);
        } else if (!(bits & 8)) {
            if (work->ringing) {
                FieldSnd_StopRingtone(fieldSound);
                work->ringing = FALSE;
                CGearSw_SetAlert(work, FALSE, FALSE);
            }
        }
    }
}

static void CGearSw_SetNotice(CGearSw *work, int notice) {
    switch (notice) {
    case NOTICE_KEEP:
        break;
    case NOTICE_NONE:
        CGearSw_StopNoticeGlow(work);
        work->logNotice = FALSE;
        CGearSw_SetActorAnim(work->actors[ACTOR_NOTICE], 16);
        break;
    case NOTICE_LOG:
        CGearSw_StopNoticeGlow(work);
        work->logNotice = TRUE;
        CGearSw_SetActorAnim(work->actors[ACTOR_NOTICE], 20);
        break;
    case NOTICE_NEW:
        CGearSw_StartNoticeGlow(work);
        work->logNotice = FALSE;
        CGearSw_SetActorAnim(work->actors[ACTOR_NOTICE], 18);
        break;
    }
}

static void CGearSw_UpdateNotices(CGearSw *work) {
    if (!CGearSw_IsIdle(work)) {
        return;
    }
    if (GameBeaconSys_CheckNotice(0)) {
        if (!work->radarNotice) {
            CGearSw_SetActorAnim(work->actors[ACTOR_RADAR], 15);
            work->radarNotice = TRUE;
        }
    } else if (work->radarNotice) {
        CGearSw_SetActorAnim(work->actors[ACTOR_RADAR], 12);
        work->radarNotice = FALSE;
    }
    if (GameBeaconSys_CheckNotice(2)) {
        if (!work->newNotice) {
            CGearSw_SetNotice(work, NOTICE_NEW);
        }
    } else if (GameBeaconSys_CheckNotice(1)) {
        if (!work->logNotice) {
            CGearSw_SetNotice(work, NOTICE_LOG);
        }
    } else if (work->logNotice || work->newNotice) {
        CGearSw_SetNotice(work, NOTICE_NONE);
    }
}
