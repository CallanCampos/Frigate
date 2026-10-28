// The ROM names this file guru2_main.c.
#include "types.h"
#include "app/spin_trade.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/union_app.h"
#include "gfl/arc.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/calctool.h"
#include "gfl/clact.h"
#include "gfl/g3d.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/math_util.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_system.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "nnsys/g3d.h"
#include "pml/met_data.h"
#include "pml/poke_party.h"
#include "save/bag.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "system/aeabi.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/net_save.h"
#include "system/palanm.h"
#include "system/printsys.h"
#include "system/time_icon.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "text/system/ability_handlers.h"

// spin_trade_main.c (the ROM's name, from its allocations): the Spin Trade's screen with the disc. The Eggs drop onto
// the disc, which the parent spins while every player's touches of the button make the Eggs jump; when the time is up
// each player takes the Egg in front of them, and those whose Egg stopped on a bonus area get a Berry. Names are ours

// The class of the timing syncs
#define SPIN_TRADE_SYNC_CLASS 0x28
enum {
    SPIN_TRADE_SYNC_START = 0xca,
    SPIN_TRADE_SYNC_COUNTDOWN,
    SPIN_TRADE_SYNC_RESULT,
    SPIN_TRADE_SYNC_SAVE,
    SPIN_TRADE_SYNC_END,
};

// The bits of SPIN_TRADE_NET_CMD_5's signals
#define SPIN_TRADE_MAIN_SIGNAL_JOIN 1
#define SPIN_TRADE_MAIN_SIGNAL_COUNTDOWN 4
#define SPIN_TRADE_MAIN_SIGNAL_START 8

#define FULL_TURN FX32_CONST(360)
// How far from the disc's center the Eggs are
#define EGG_RADIUS 22
// The game's length in frames, and when the disc starts to slow down
#define GAME_FRAMES 600
#define GAME_SLOW_FRAMES 300

#define COLOR_NAME PRINT_COLOR(1, 2, 0)
#define COLOR_NAME_MINE PRINT_COLOR(3, 4, 0)

// The archive's files
enum {
    NARC_SPIN_TRADE_EGG_NSBMD = 0,
    NARC_SPIN_TRADE_DISC_2_NSBMD,
    NARC_SPIN_TRADE_DISC_3_NSBMD,
    NARC_SPIN_TRADE_DISC_4_NSBMD,
    NARC_SPIN_TRADE_DISC_5_NSBMD,
    NARC_SPIN_TRADE_SHADOW_NSBMD,
    NARC_SPIN_TRADE_CURSOR_0_NSBMD,
    NARC_SPIN_TRADE_CURSOR_1_NSBMD,
    NARC_SPIN_TRADE_CURSOR_2_NSBMD,
    NARC_SPIN_TRADE_CURSOR_3_NSBMD,
    NARC_SPIN_TRADE_CURSOR_4_NSBMD,
    NARC_SPIN_TRADE_CURSOR_0_ANM,
    NARC_SPIN_TRADE_CURSOR_1_ANM,
    NARC_SPIN_TRADE_CURSOR_2_ANM,
    NARC_SPIN_TRADE_CURSOR_3_ANM,
    NARC_SPIN_TRADE_CURSOR_4_ANM,
    NARC_SPIN_TRADE_BG_NCGR,
    NARC_SPIN_TRADE_BG_NCLR,
    NARC_SPIN_TRADE_BG3_NSCR,
    NARC_SPIN_TRADE_BG2_NSCR,
    NARC_SPIN_TRADE_SUB_NCLR,
    NARC_SPIN_TRADE_SUB_BG3_NCGR,
    NARC_SPIN_TRADE_SUB_BG2_NCGR,
    NARC_SPIN_TRADE_SUB_BG3_NSCR,
    NARC_SPIN_TRADE_BUTTON_NSCR,
    NARC_SPIN_TRADE_BUTTON_PUSHED_NSCR,
    NARC_SPIN_TRADE_BUTTON_HALF_NSCR,
    NARC_SPIN_TRADE_BG_NSBMD,
};

enum {
    SEQ_INIT,
    SEQ_FADE_IN_WAIT,
    SEQ_PARENT_WAIT_CHILDREN,
    SEQ_PARENT_SEND_JOIN_SIGNAL,
    SEQ_PARENT_WAIT_JOIN,
    SEQ_PARENT_SEND_PLAYER_COUNT,
    SEQ_PARENT_SEND_POSITIONS,
    SEQ_PARENT_SEND_START_SIGNAL,
    SEQ_CHILD_SEND_JOIN,
    SEQ_CHILD_WAIT_START_SIGNAL,
    SEQ_SYNC_START,
    SEQ_SYNC_START_WAIT,
    SEQ_SEND_PARTY_INDEX,
    SEQ_SEND_PARTY,
    SEQ_WAIT_PARTIES,
    SEQ_WAIT_EGG_CHECK,
    SEQ_GAME_INIT,
    SEQ_EGG_FALL,
    SEQ_EGG_FALL_WAIT,
    SEQ_COUNTDOWN_BEFORE,
    SEQ_PARENT_SEND_COUNTDOWN_SIGNAL,
    SEQ_CHILD_WAIT_COUNTDOWN_SIGNAL,
    SEQ_SYNC_COUNTDOWN,
    SEQ_SYNC_COUNTDOWN_WAIT,
    SEQ_COUNTDOWN_START,
    SEQ_COUNTDOWN_WAIT,
    SEQ_GAME_START,
    SEQ_GAME,
    SEQ_GAME_END,
    SEQ_PARENT_ALIGN_OWN,
    SEQ_PARENT_ALIGN,
    SEQ_SYNC_RESULT,
    SEQ_SYNC_RESULT_WAIT,
    SEQ_PARENT_SEND_RESULT,
    SEQ_CHILD_WAIT_RESULT,
    SEQ_RESULT_NAMES,
    SEQ_RESULT_NAMES_WAIT,
    SEQ_RESULT_MESSAGE_WAIT,
    SEQ_BONUS,
    SEQ_BONUS_WAIT,
    SEQ_BAG_FULL_BEFORE,
    SEQ_BAG_FULL_WAIT,
    SEQ_SYNC_SAVE,
    SEQ_SYNC_SAVE_WAIT,
    SEQ_SAVE,
    SEQ_BAD_EGG,
    SEQ_BAD_EGG_WAIT,
    SEQ_SYNC_END,
    SEQ_SYNC_END_WAIT,
    SEQ_END_WAIT,
    SEQ_FADE_OUT,
    SEQ_FADE_OUT_WAIT,
    SEQ_END,
};

// What a sequence function returns: to wait a frame, to run the next one at once, or that the screen has ended
enum {
    SEQ_RESULT_WAIT,
    SEQ_RESULT_CONTINUE,
    SEQ_RESULT_END,
};

typedef struct {
    BOOL active;
    BOOL visible;
    BOOL landed;
    int netId;
    int position;
    // The order in which it dropped, which is the name window it has
    int index;
    int startPosition;
    fx32 angle;
    fx32 angleOffset;
    u16 rotX;
    u16 rotY;
    u16 rotZ;
    VecFx32 rotation;
    VecFx32 scale;
    VecFx32 pos;
    VecFx32 offset;
    fx32 jumpY;
    u8 unk60[0x54];
} SpinTradeEgg;

typedef struct {
    BOOL active;
    u16 rotX;
    u16 rotY;
    u16 rotZ;
    VecFx32 scale;
    VecFx32 pos;
    u8 unk24[0x54];
    SpinTradeEgg *egg;
} SpinTradeEggShadow;

// The arrow over a player's Egg
typedef struct {
    BOOL active;
    VecFx32 pos;
    u8 unk10[0x54];
    SpinTradeEgg *egg;
} SpinTradeEggCursor;

typedef struct {
    u8 unk0[0x140];
    SpinTradeEgg eggs[SPIN_TRADE_MEMBER_MAX];
    SpinTradeEggShadow shadows[SPIN_TRADE_MEMBER_MAX];
    SpinTradeEggCursor cursors[SPIN_TRADE_MEMBER_MAX];
    u32 eggScene;
    u32 shadowScene;
    u32 cursorScenes[SPIN_TRADE_MEMBER_MAX];
} SpinTradeObjects;

typedef struct {
    VecFx32 pos;
    VecFx32 drawPos;
    VecFx32 scale;
    // The shake after the button is pressed, and the push down before it
    VecFx32 shake;
    VecFx32 offset;
    fx32 angle;
    // What turns the player's own position to the front
    fx32 baseAngle;
    fx32 angleOffset;
    fx32 speed;
    u16 rotX;
    s16 rotY;
    u16 rotZ;
    u8 unk52[0x6a];
    u32 scene;
} SpinTradeDisc;

// The work of the sequence that drops the Eggs
typedef struct {
    int count;
    int angle;
    u8 unk8[8];
    int position;
    int playerCount;
} SpinTradeEggFallSeqWork;

typedef struct {
    int seq;
    u32 unk4;
    BOOL sePlayed;
    fx32 speed;
    SpinTradeEgg *egg;
    SpinTradeMainWork *g2m;
} SpinTradeEggFallTask;

typedef struct {
    BOOL active;
    int seq;
    int frame;
    int count;
    fx32 speed;
    fx32 accel;
    u32 unk18;
    SpinTradeEgg *egg;
} SpinTradeEggJumpTask;

typedef struct {
    BOOL active;
    int seq;
    u32 unk8;
    fx32 speed;
    SpinTradeEgg *egg;
} SpinTradeEggSpinTask;

typedef struct {
    BOOL active;
    int seq;
    fx32 amplitude;
    fx32 speed;
    fx32 angle;
    SpinTradeEgg *egg;
} SpinTradeEggShakeTask;

// The disc's bump when the button is pressed
typedef struct {
    int seq;
    // Whether the Eggs are in the air
    BOOL jumping;
    BOOL active;
    int count;
    fx32 shake;
} SpinTradeDiscBumpTask;

typedef struct {
    u16 seq;
    u16 active;
    int frame;
} SpinTradeButtonTask;

// An Egg's hops on a bonus area
typedef struct {
    int seq;
    int frame;
    int wait;
    SpinTradeEgg *egg;
} SpinTradeBonusTask;

typedef struct {
    int seq;
    int wait;
    u32 unk8;
    int count;
    SpinTradeMainWork *g2m;
    ClActor *actor;
} SpinTradeCountdownTask;

typedef struct {
    SpinTradeMainWork *g2m;
    int out;
    int evy;
} SpinTradeFadeTask;

typedef struct {
    int nameWindowCount;
    Font *font;
    MsgData *msgData;
    WordSet *wordSet;
    BmpWin *msgWindow;
    BmpWin *nameWindows[SPIN_TRADE_MEMBER_MAX];
    PrintWindow msgPrint;
    PrintWindow namePrints[SPIN_TRADE_MEMBER_MAX];
    PrintQueue *queue;
    StrBuf *strbuf;
} SpinTradeMsgWork;

typedef struct {
    u8 unk0[0x14];
    G3DCamera *camera;
} SpinTradeCameraWork;

struct SpinTradeMainWork {
    int seq;
    u32 unk04;
    int gameFrame;
    int wait;
    u32 unk10;
    int sendCount;
    BOOL error;
    // The net IDs whose Egg stopped on a bonus area
    u32 bonusBits;
    BOOL timeUp;
    PokeParty *party;
    // The Egg that stopped in front of the player
    SpinTradeEgg *frontEgg;
    int position;
    int playerCount;
    // A press of the button that arrived this frame, and this machine's to send
    u32 buttonReceived;
    u32 buttonSend;
    BOOL gameDataReceived;
    BOOL resultReceived;
    // The party slot each member trades, by net ID
    u32 partyIndices[SPIN_TRADE_MEMBER_MAX];
    SpinTradeGameData gameData;
    SpinTradeResultData result;
    SpinTradePositionData positions[SPIN_TRADE_MEMBER_MAX];
    UnionAppMember *members[SPIN_TRADE_MEMBER_MAX];
    StrBuf *names[SPIN_TRADE_MEMBER_MAX];
    u8 seqWork[0x20];
    SpinTradeProcWork *g2p;
    SpinTradeCommWork *g2c;
    SpinTradeDisc disc;
    SpinTradeObjects objects;
    G3DManager *g3dManager;
    SpinTradeEggJumpTask jumpTasks[SPIN_TRADE_MEMBER_MAX];
    SpinTradeEggSpinTask spinTasks[SPIN_TRADE_MEMBER_MAX];
    SpinTradeEggShakeTask shakeTasks[SPIN_TRADE_MEMBER_MAX];
    SpinTradeDiscBumpTask bumpTask;
    SpinTradeButtonTask buttonTask;
    SpinTradeBonusTask bonusTasks[SPIN_TRADE_MEMBER_MAX];
    SpinTradeMsgWork msg;
    SpinTradeCameraWork camera;
    ArcTool *arc;
    NNSG2dScreenData *screenData;
    NNSG2dCharacterData *charData;
    NNSG2dPaletteData *paletteData;
    ClActUnit *clactUnit;
    u32 chars;
    u32 palette;
    u32 cellAnims;
    PaletteFade *fade;
    TCB *jumpTCB;
    TCB *spinTCB;
    TCB *shakeTCB;
    TCB *bumpTCB;
    TCB *buttonTCB;
    TCB *bonusTCB;
    void *tcbBuffer;
    TCBManager *tcbManager;
    TCB *vblankTCB;
    NetSave *netSave;
    u32 bgScene;
    WaitIcon *waitIcon;
};

typedef int (*SpinTradeMainSeqFunc)(SpinTradeMainWork *g2m);
typedef int (*SpinTradeEggFallFunc)(SpinTradeEggFallTask *task);

// A name window, in the layout of Gen 4's window templates
typedef struct {
    u32 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u16 palette;
    u16 charOffset;
} SpinTradeNameWindowData;

// A bonus area of the disc, in degrees
typedef struct {
    u16 start;
    u16 end;
} SpinTradeBonusArea;

typedef struct {
    int count;
    const SpinTradeBonusArea *areas;
} SpinTradeBonusAreaList;

static void SpinTradeMain_ClearReceived(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_Init(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_FadeInWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ParentWaitChildren(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ParentSendJoinSignal(SpinTradeMainWork *g2m);
static UnionApp *SpinTradeMain_GetUnionApp(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ParentWaitJoin(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ParentSendPlayerCount(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ParentSendPositions(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ParentSendStartSignal(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ChildSendJoin(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ChildWaitStartSignal(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_SyncStart(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_SyncStartWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_SendPartyIndex(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_SendParty(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_WaitParties(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_WaitEggCheck(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_GameInit(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_EggFall(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_EggFallWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_CountdownBefore(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ParentSendCountdownSignal(SpinTradeMainWork *g2m);
static void SpinTradeMain_AddMetTrainers(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ChildWaitCountdownSignal(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_SyncCountdown(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_SyncCountdownWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_CountdownStart(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_CountdownWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_GameStart(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_Game(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_GameEnd(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ParentAlignOwn(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ParentAlign(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_SyncResult(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_SyncResultWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ParentSendResult(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ChildWaitResult(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ResultNames(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ResultNamesWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_ResultMessageWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_Bonus(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_BonusWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_BagFullBefore(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_BagFullWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_SyncSave(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_SyncSaveWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_Save(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_BadEgg(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_BadEggWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_SyncEnd(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_SyncEndWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_EndWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_FadeOut(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_FadeOutWait(SpinTradeMainWork *g2m);
static int SpinTradeMainSeq_End(SpinTradeMainWork *g2m);
static void SpinTradeMain_VBlank(TCB *tcb, void *data);
static void SpinTradeMain_InitGraphics(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeGraphics(SpinTradeMainWork *g2m);
static void SpinTradeMain_Draw(SpinTradeMainWork *g2m);
static void SpinTradeMain_InitVRAM(SpinTradeMainWork *g2m);
static void SpinTradeMain_ShowBGs(SpinTradeMainWork *g2m);
static void SpinTradeMain_HideBGs(SpinTradeMainWork *g2m);
static void SpinTradeMain_Setup3D(void);
static void SpinTradeMain_Init3D(SpinTradeMainWork *g2m);
static void SpinTradeMain_InitBGs(SpinTradeMainWork *g2m);
static void SpinTradeMain_LoadBGs(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeBGs(SpinTradeMainWork *g2m);
static void SpinTradeMain_LoadButtonScreen(SpinTradeMainWork *g2m, u32 fileId);
static void SpinTradeMain_ShowButtonHalf(SpinTradeMainWork *g2m);
static void SpinTradeMain_ShowButton(SpinTradeMainWork *g2m);
static void SpinTradeMain_ShowButtonPushed(SpinTradeMainWork *g2m);
static void SpinTradeMain_InitFade(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeFade(SpinTradeMainWork *g2m);
static void SpinTradeMain_SetSubFade(SpinTradeMainWork *g2m, int evy);
static void SpinTradeMain_InitClAct(SpinTradeMainWork *g2m);
static void SpinTradeMain_LoadClActResources(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeClAct(SpinTradeMainWork *g2m);
static void SpinTradeMain_InitMsg(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeMsg(SpinTradeMainWork *g2m);
static void SpinTradeMain_PrintMsg(SpinTradeMainWork *g2m, u32 messageId);
static void SpinTradeMain_PrintEggMsg(SpinTradeMainWork *g2m, u32 messageId, UnionAppMember *member);
static void SpinTradeMain_PrintItemMsg(SpinTradeMainWork *g2m, u32 messageId, u32 item);
static void SpinTradeMain_ClearMsg(SpinTradeMainWork *g2m);
static void SpinTradeMain_CreateNameWindows(SpinTradeMainWork *g2m, int count);
static void SpinTradeMain_FreeNameWindows(SpinTradeMainWork *g2m);
static void SpinTradeMain_PrintNameColor(SpinTradeMainWork *g2m, StrBuf *name, int index, u32 color);
static void SpinTradeMain_PrintName(SpinTradeMainWork *g2m, StrBuf *name, int index, int netId);
static void SpinTradeMain_ClearName(SpinTradeMainWork *g2m, int index);
static void SpinTradeMain_FreeCamera(SpinTradeMainWork *g2m);
static BOOL SpinTradeMain_IsButtonTouched(SpinTradeMainWork *g2m);
static void SpinTradeMain_InitDisc(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeDisc(SpinTradeMainWork *g2m);
static void SpinTradeMain_UpdateDisc(SpinTradeMainWork *g2m);
static void SpinTradeMain_DrawDisc(SpinTradeMainWork *g2m);
static void SpinTradeDisc_Turn(SpinTradeDisc *disc, fx32 angle);
static void SpinTradeMain_InitBG3D(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeBG3D(SpinTradeMainWork *g2m);
static void SpinTradeMain_DrawBG3D(SpinTradeMainWork *g2m);
static void SpinTradeMain_InitObjects(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeObjects(SpinTradeMainWork *g2m);
static void SpinTradeMain_ShowEgg(SpinTradeMainWork *g2m, SpinTradeEgg *egg);
static void SpinTradeMain_UpdateObjects(SpinTradeMainWork *g2m);
static void SpinTradeMain_DrawObjects(SpinTradeMainWork *g2m);
static void SpinTradeMain_TurnEggs(SpinTradeMainWork *g2m, fx32 angle);
static void SpinTradeEgg_Update(SpinTradeEgg *egg, const VecFx32 *offset, fx32 angle, fx32 jumpY);
static SpinTradeEgg *SpinTradeMain_FindFrontEgg(SpinTradeMainWork *g2m);
static void SpinTradeMain_AddEggShadow(SpinTradeMainWork *g2m, SpinTradeEgg *egg);
static void SpinTradeMain_UpdateEggShadow(SpinTradeMainWork *g2m, SpinTradeEggShadow *shadow, fx32 jumpY);
static void SpinTradeMain_AddEggCursor(SpinTradeMainWork *g2m, SpinTradeEgg *egg);
static void SpinTradeMain_UpdateEggCursor(SpinTradeMainWork *g2m, SpinTradeEggCursor *cursor, int index);
static void SpinTradeMain_AddEgg(SpinTradeMainWork *g2m, int netId, int position, int index, int angle, int angleOffset,
                                 SpinTradeEgg *egg);
static void SpinTradeEggFallTask_Main(TCB *tcb, void *data);
static int SpinTradeEggFallTask_Init(SpinTradeEggFallTask *task);
static int SpinTradeEggFallTask_Fall(SpinTradeEggFallTask *task);
static int SpinTradeEggFallTask_Land(SpinTradeEggFallTask *task);
static void SpinTradeEggJumpTask_Update(SpinTradeMainWork *g2m, SpinTradeEggJumpTask *task);
static void SpinTradeEggJumpTask_Main(TCB *tcb, void *data);
static void SpinTradeMain_InitEggJump(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeEggJump(SpinTradeMainWork *g2m);
static void SpinTradeMain_StartEggJump(SpinTradeMainWork *g2m, SpinTradeEgg *egg);
static void SpinTradeMain_StartEggJumps(SpinTradeMainWork *g2m);
static BOOL SpinTradeMain_IsEggJumping(SpinTradeMainWork *g2m);
static void SpinTradeEggSpinTask_Update(SpinTradeEggSpinTask *task);
static void SpinTradeEggSpinTask_Main(TCB *tcb, void *data);
static void SpinTradeMain_InitEggSpin(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeEggSpin(SpinTradeMainWork *g2m);
static void SpinTradeMain_StartEggSpin(SpinTradeMainWork *g2m, SpinTradeEgg *egg);
static void SpinTradeEggShakeTask_Update(SpinTradeEggShakeTask *task);
static void SpinTradeEggShakeTask_Main(TCB *tcb, void *data);
static void SpinTradeMain_InitEggShake(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeEggShake(SpinTradeMainWork *g2m);
static void SpinTradeMain_StartEggShake(SpinTradeMainWork *g2m, SpinTradeEgg *egg);
static void SpinTradeDiscBumpTask_Update(SpinTradeMainWork *g2m, SpinTradeDiscBumpTask *task);
static void SpinTradeDiscBumpTask_Main(TCB *tcb, void *data);
static void SpinTradeMain_InitDiscBump(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeDiscBump(SpinTradeMainWork *g2m);
static void SpinTradeMain_StartDiscBump(SpinTradeMainWork *g2m);
static BOOL SpinTradeMain_AreEggsJumping(SpinTradeMainWork *g2m);
static BOOL SpinTradeMain_IsDiscBumping(SpinTradeMainWork *g2m);
static void SpinTradeCountdownTask_Main(TCB *tcb, void *data);
static void SpinTradeMain_StartCountdown(SpinTradeMainWork *g2m);
static void SpinTradeButtonTask_Update(SpinTradeMainWork *g2m, SpinTradeButtonTask *task);
static void SpinTradeButtonTask_Main(TCB *tcb, void *data);
static void SpinTradeMain_InitButton(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeButton(SpinTradeMainWork *g2m);
static u32 SpinTradeMain_IsButtonPushed(SpinTradeMainWork *g2m);
static void SpinTradeMain_PushButton(SpinTradeMainWork *g2m);
static void SpinTradeBonusTask_Update(SpinTradeMainWork *g2m, SpinTradeBonusTask *task);
static void SpinTradeBonusTask_Main(TCB *tcb, void *data);
static void SpinTradeMain_InitBonus(SpinTradeMainWork *g2m);
static void SpinTradeMain_FreeBonus(SpinTradeMainWork *g2m);
static void SpinTradeMain_StartBonus(SpinTradeMainWork *g2m, SpinTradeEgg *egg);
static void SpinTradeMain_StartBonuses(SpinTradeMainWork *g2m);
static void SpinTradeFadeTask_Main(TCB *tcb, void *data);
static void SpinTradeMain_StartSubFade(SpinTradeMainWork *g2m, int out);
static void SpinTradeMain_AddAngle(fx32 *angle, fx32 addend);
static void SpinTradeMain_Turn(SpinTradeMainWork *g2m, fx32 angle);
static BOOL SpinTradeMain_CountGameFrame(SpinTradeMainWork *g2m);
static void *SpinTradeMain_LoadFile(SpinTradeMainWork *g2m, u32 fileId, BOOL head);
static void *SpinTradeMain_InitSeqWork(SpinTradeMainWork *g2m, u32 size);
static void *SpinTradeMain_GetSeqWork(SpinTradeMainWork *g2m);
static BOOL SpinTradeMain_HasSignal(SpinTradeMainWork *g2m, u32 signal);
static BOOL SpinTradeMain_HaveChildrenJoined(SpinTradeMainWork *g2m);
static int SpinTradeMain_CountChildren(SpinTradeMainWork *g2m);
static BOOL SpinTradeMain_IsMember(SpinTradeMainWork *g2m, int netId);
static void SpinTradeMain_TradeEgg(SpinTradeMainWork *g2m, int netId);
static int SpinTradeMain_CountParties(SpinTradeMainWork *g2m);
static BOOL SpinTradeMain_IsBonusAngle(SpinTradeMainWork *g2m, fx32 angle, int playerCount);
static BOOL SpinTradeMain_PartyHasBadEgg(PokeParty *party);
static BOOL SpinTradeMain_AnyPartyHasBadEgg(SpinTradeMainWork *g2m);
static int SpinTradeMain_CountEggChecks(SpinTradeMainWork *g2m);
static BOOL SpinTradeMain_IsBadEggFound(SpinTradeMainWork *g2m);
static void SpinTradeMain_PlayME(u32 me);
static BOOL SpinTradeMain_IsMEPlaying(void);
static void SpinTradeMain_ParentTurn(SpinTradeMainWork *g2m, fx32 angle);

static const BGSysVRAMConfig sSpinTradeVRAMConfig = {
    GX_VRAM_BG_128_B,      GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_E | GX_VRAM_F, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_A,       GX_VRAM_TEXPLTT_0_G,     GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

// How much each of the Eggs' hops on a bonus area lifts it
static int sBonusHopHeights[] = { 4, 6, 8, 10, 11, 12, 12, 12, 11, 10, 9, 8, 6, 4, 0, 0 };

static const SpinTradeNameWindowData sNameWindows5[] = {
    { 1, 12, 19, 8, 2, 15, 0x94 }, { 1, 4, 14, 8, 2, 15, 0xbc },   { 1, 6, 9, 8, 2, 15, 0xe4 },
    { 1, 18, 9, 8, 2, 15, 0x10c }, { 1, 20, 14, 8, 2, 15, 0x134 },
};

static const TouchRect sSpinTradeButtonRects[] = {
    { TOUCH_RECT_CIRCLE, 128, 96, 64 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The angle that turns each position to the front, by player count
static const u16 sPositionAngles[SPIN_TRADE_MEMBER_MAX + 1][SPIN_TRADE_MEMBER_MAX] = {
    { 0, 0, 0, 0, 0 },     { 0, 0, 0, 0, 0 },      { 0, 180, 0, 0, 0 },
    { 0, 240, 120, 0, 0 }, { 0, 270, 180, 90, 0 }, { 0, 288, 216, 144, 72 },
};

// What turns each disc's model to match its positions, by player count
static const fx32 sDiscAngleOffsets[SPIN_TRADE_MEMBER_MAX + 1] = {
    0, 0, FX32_CONST(-72), 0, FX32_CONST(-181), 0,
};

static const G3DSceneResourceSetup sEggResources[] = {
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_EGG_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sDisc2Resources[] = {
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_DISC_2_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const GXRgb sEdgeColors[8] = {
    GX_RGB(4, 4, 4), GX_RGB(4, 4, 4), GX_RGB(4, 4, 4), GX_RGB(4, 4, 4),
    GX_RGB(4, 4, 4), GX_RGB(4, 4, 4), GX_RGB(4, 4, 4), GX_RGB(4, 4, 4),
};

static const SpinTradeNameWindowData sNameWindows4[] = {
    { 1, 12, 18, 8, 2, 15, 0x94 },
    { 1, 4, 13, 8, 2, 15, 0xbc },
    { 1, 12, 6, 8, 2, 15, 0xe4 },
    { 1, 20, 13, 8, 2, 15, 0x10c },
};

static const VecFx32 sSpinTradeCameraTarget = { 0, 0, 0 };

static const G3DSceneActorSetup sDiscActors[] = {
    { 0, 0, 0, 0, NULL, 0 },
};

// The first and last of the Berries a bonus area gives, by player count
static const u32 sBonusItems[SPIN_TRADE_MEMBER_MAX + 1][2] = {
    { ITEM_NONE, ITEM_NONE },
    { ITEM_NONE, ITEM_NONE },
    { ITEM_FIGY_BERRY, ITEM_IAPAPA_BERRY },
    { ITEM_FIGY_BERRY, ITEM_IAPAPA_BERRY },
    { ITEM_CHERI_BERRY, ITEM_ASPEAR_BERRY },
    { ITEM_POMEG_BERRY, ITEM_TAMATO_BERRY },
};

static const G3DSceneActorSetup sBGActors[] = {
    { 0, 0, 0, 0, NULL, 0 },
};

// Where on the disc each position's Egg is, by player count
static const u16 sEggAngles[SPIN_TRADE_MEMBER_MAX + 1][SPIN_TRADE_MEMBER_MAX] = {
    { 0, 0, 0, 0, 0 },      { 0, 0, 0, 0, 0 },      { 90, 270, 0, 0, 0 },
    { 90, 210, 330, 0, 0 }, { 90, 180, 270, 0, 0 }, { 90, 162, 234, 306, 18 },
};

static const G3DSceneResourceSetup sDisc4Resources[] = {
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_DISC_4_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sCursor3Resources[] = {
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_CURSOR_3_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_CURSOR_3_ANM, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sDisc5Resources[] = {
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_DISC_5_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const SpinTradeNameWindowData sNameWindows3[] = {
    { 1, 12, 18, 8, 2, 15, 0x94 },
    { 1, 5, 10, 8, 2, 15, 0xbc },
    { 1, 19, 10, 8, 2, 15, 0xe4 },
};

static const G3DSceneResourceSetup sShadowResources[] = {
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_SHADOW_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sDisc3Resources[] = {
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_DISC_3_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const VecFx32 sSpinTradeCameraPosition = { 0, FX32_CONST(48), FX32_CONST(112) };

static const SpinTradeNameWindowData sNameWindows2[] = {
    { 1, 12, 18, 8, 2, 15, 0x94 },
    { 1, 12, 6, 8, 2, 15, 0xbc },
};

static const G3DSceneActorSetup sEggActors[] = {
    { 0, 0, 0, 0, NULL, 0 },
};

static const G3DSceneResourceSetup sBGResources[] = {
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_BG_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sCursor0Resources[] = {
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_CURSOR_0_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_CURSOR_0_ANM, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneAnimationSetup sCursorAnimations[] = {
    { 1, 0 },
};

static const G3DSceneResourceSetup sCursor1Resources[] = {
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_CURSOR_1_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_CURSOR_1_ANM, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneActorSetup sShadowActors[] = {
    { 0, 0, 0, 0, NULL, 0 },
};

static const G3DSceneResourceSetup sCursor4Resources[] = {
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_CURSOR_4_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_CURSOR_4_ANM, 0, G3D_SCENE_RES_ARCSYS },
};

// By player count, from two
static const G3DSceneSetup sDiscScenes[] = {
    { sDisc2Resources, NELEMS(sDisc2Resources), sDiscActors, NELEMS(sDiscActors) },
    { sDisc3Resources, NELEMS(sDisc3Resources), sDiscActors, NELEMS(sDiscActors) },
    { sDisc4Resources, NELEMS(sDisc4Resources), sDiscActors, NELEMS(sDiscActors) },
    { sDisc5Resources, NELEMS(sDisc5Resources), sDiscActors, NELEMS(sDiscActors) },
};

static const G3DSceneActorSetup sCursorActors[] = {
    { 0, 0, 0, 0, sCursorAnimations, NELEMS(sCursorAnimations) },
};

static const G3DSceneResourceSetup sCursor2Resources[] = {
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_CURSOR_2_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_SPIN_TRADE, NARC_SPIN_TRADE_CURSOR_2_ANM, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneSetup sBGScene = { sBGResources, NELEMS(sBGResources), sBGActors, NELEMS(sBGActors) };

static const SpinTradeEggFallFunc sEggFallFuncs[] = {
    SpinTradeEggFallTask_Init,
    SpinTradeEggFallTask_Fall,
    SpinTradeEggFallTask_Land,
};

static const SpinTradeBonusArea sBonusAreas2[] = {
    { 0, 180 },
};

static const SpinTradeNameWindowData *const sNameWindows[SPIN_TRADE_MEMBER_MAX + 1] = {
    NULL, NULL, sNameWindows2, sNameWindows3, sNameWindows4, sNameWindows5,
};

static const SpinTradeMainSeqFunc sSpinTradeMainSeqFuncs[] = {
    SpinTradeMainSeq_Init,
    SpinTradeMainSeq_FadeInWait,
    SpinTradeMainSeq_ParentWaitChildren,
    SpinTradeMainSeq_ParentSendJoinSignal,
    SpinTradeMainSeq_ParentWaitJoin,
    SpinTradeMainSeq_ParentSendPlayerCount,
    SpinTradeMainSeq_ParentSendPositions,
    SpinTradeMainSeq_ParentSendStartSignal,
    SpinTradeMainSeq_ChildSendJoin,
    SpinTradeMainSeq_ChildWaitStartSignal,
    SpinTradeMainSeq_SyncStart,
    SpinTradeMainSeq_SyncStartWait,
    SpinTradeMainSeq_SendPartyIndex,
    SpinTradeMainSeq_SendParty,
    SpinTradeMainSeq_WaitParties,
    SpinTradeMainSeq_WaitEggCheck,
    SpinTradeMainSeq_GameInit,
    SpinTradeMainSeq_EggFall,
    SpinTradeMainSeq_EggFallWait,
    SpinTradeMainSeq_CountdownBefore,
    SpinTradeMainSeq_ParentSendCountdownSignal,
    SpinTradeMainSeq_ChildWaitCountdownSignal,
    SpinTradeMainSeq_SyncCountdown,
    SpinTradeMainSeq_SyncCountdownWait,
    SpinTradeMainSeq_CountdownStart,
    SpinTradeMainSeq_CountdownWait,
    SpinTradeMainSeq_GameStart,
    SpinTradeMainSeq_Game,
    SpinTradeMainSeq_GameEnd,
    SpinTradeMainSeq_ParentAlignOwn,
    SpinTradeMainSeq_ParentAlign,
    SpinTradeMainSeq_SyncResult,
    SpinTradeMainSeq_SyncResultWait,
    SpinTradeMainSeq_ParentSendResult,
    SpinTradeMainSeq_ChildWaitResult,
    SpinTradeMainSeq_ResultNames,
    SpinTradeMainSeq_ResultNamesWait,
    SpinTradeMainSeq_ResultMessageWait,
    SpinTradeMainSeq_Bonus,
    SpinTradeMainSeq_BonusWait,
    SpinTradeMainSeq_BagFullBefore,
    SpinTradeMainSeq_BagFullWait,
    SpinTradeMainSeq_SyncSave,
    SpinTradeMainSeq_SyncSaveWait,
    SpinTradeMainSeq_Save,
    SpinTradeMainSeq_BadEgg,
    SpinTradeMainSeq_BadEggWait,
    SpinTradeMainSeq_SyncEnd,
    SpinTradeMainSeq_SyncEndWait,
    SpinTradeMainSeq_EndWait,
    SpinTradeMainSeq_FadeOut,
    SpinTradeMainSeq_FadeOutWait,
    SpinTradeMainSeq_End,
};

BOOL SpinTradeMain_Init(GameProc *proc, u32 *state, void *param, void *work) {
    SpinTradeProcWork *g2p = param;
    int i;
    SpinTradeMainWork *g2m;

    GflNet_SetAutoExit(1);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_SPIN_TRADE, 0x80000);
    g2m = GFL_ProcInitSubsystem(proc, sizeof(SpinTradeMainWork), HEAPID_SPIN_TRADE);
    i = 0;
    MI_CpuFill8(g2m, 0, sizeof(SpinTradeMainWork));
    g2p->g2m = g2m;
    g2m->g2p = g2p;
    g2m->g2c = g2p->g2c;
    g2m->party = GameData_GetParty(g2m->g2p->param->gameData);
    g2m->arc = GFL_ArcSysCreateFileHandle(ARCID_SPIN_TRADE, HEAPID_SPIN_TRADE);
    SpinTradeMain_InitGraphics(g2m);
    g2m->vblankTCB = GFL_VBlankTCBAdd(SpinTradeMain_VBlank, g2m, 1);
    g2m->g3dManager = GFL_G3DMgrCreate(20, 11, HEAPID_SPIN_TRADE);
    SpinTradeMain_InitBG3D(g2m);
    SpinTradeMain_InitObjects(g2m);
    SpinTradeMain_InitDisc(g2m);

    // Until the parent sends the positions, the members are in net ID order
    {
        int position = 0;
        int netId = NetSystem_GetNetId();
        SpinTradeDisc *disc = &g2m->disc;

        for (; i < SPIN_TRADE_MEMBER_MAX; i++) {
            if (g2m->g2p->memberBits & (1 << i)) {
                if (i == netId) {
                    break;
                }
                position++;
            }
        }
        disc->baseAngle = sPositionAngles[g2m->g2p->memberCount][position] << FX32_SHIFT;
    }

    g2m->tcbBuffer =
        GFL_HeapAllocate(HEAPID_TAIL(HEAPID_SPIN_TRADE), GFL_TCBMgrCalcAllocSize(32), FALSE, "guru2_main.c", 1260);
    g2m->tcbManager = GFL_TCBMgrCreate(32, g2m->tcbBuffer);
    SpinTradeMain_InitEggJump(g2m);
    SpinTradeMain_InitEggSpin(g2m);
    SpinTradeMain_InitEggShake(g2m);
    SpinTradeMain_InitDiscBump(g2m);
    SpinTradeMain_InitButton(g2m);
    SpinTradeMain_InitBonus(g2m);
    GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 8, 1, HEAPID_SPIN_TRADE);
    GFL_NetSetDisconnectError(TRUE);
    return TRUE;
}

BOOL SpinTradeMain_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    SpinTradeMainWork *g2m = work;

    PrintQueue_Clear(g2m->msg.queue);
    PrintQueue_Free(g2m->msg.queue);
    if (g2m->waitIcon != NULL) {
        WaitIcon_Free(g2m->waitIcon);
    }
    SpinTradeMain_FreeEggJump(g2m);
    SpinTradeMain_FreeEggSpin(g2m);
    SpinTradeMain_FreeEggShake(g2m);
    SpinTradeMain_FreeDiscBump(g2m);
    SpinTradeMain_FreeButton(g2m);
    SpinTradeMain_FreeBonus(g2m);
    GFL_TCBMgrDelete(g2m->tcbManager);
    GFL_HeapFree(g2m->tcbBuffer);
    SpinTradeMain_FreeBG3D(g2m);
    SpinTradeMain_FreeDisc(g2m);
    SpinTradeMain_FreeObjects(g2m);
    GFL_G3DMgrFree(g2m->g3dManager);
    GFL_G3DSysFree();
    SpinTradeMain_FreeGraphics(g2m);
    GFL_TCBRemove(g2m->vblankTCB);
    GFL_ArcToolFree(g2m->arc);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_SPIN_TRADE);
    GCTX_HIDUnblockSoftReset(8);
    return TRUE;
}

BOOL SpinTradeMain_Main(GameProc *proc, u32 *state, void *param, void *work) {
    SpinTradeMainWork *g2m = work;
    int result;

    do {
        result = sSpinTradeMainSeqFuncs[g2m->seq](g2m);
    } while (result == SEQ_RESULT_CONTINUE);
    if (result == SEQ_RESULT_END) {
        return TRUE;
    }

    SpinTradeMain_UpdateDisc(g2m);
    SpinTradeMain_UpdateObjects(g2m);
    SpinTradeMain_Draw(g2m);
    SpinTradeMain_ClearReceived(g2m);
    GFL_TCBMgrUpdate(g2m->tcbManager);
    if (GFL_WipeIsFinished() && GFL_NetErrCheck()) {
        return TRUE;
    }
    return FALSE;
}

static void SpinTradeMain_ClearReceived(SpinTradeMainWork *g2m) {
    g2m->buttonReceived = 0;
    g2m->gameDataReceived = FALSE;
}

static int SpinTradeMainSeq_Init(SpinTradeMainWork *g2m) {
    SpinTradeMain_PrintMsg(g2m, AbilityHandlers_Text_PleaseStandBy);
    g2m->seq = SEQ_FADE_IN_WAIT;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_FadeInWait(SpinTradeMainWork *g2m) {
    if (GFL_WipeIsFinished()) {
        if (NetSystem_GetNetId() == 0) {
            g2m->seq = SEQ_PARENT_WAIT_CHILDREN;
        } else {
            g2m->seq = SEQ_CHILD_SEND_JOIN;
        }
        g2m->waitIcon = WaitIcon_Create(g2m->tcbManager, g2m->msg.msgWindow, 15, 16, HEAPID_SPIN_TRADE);
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_ParentWaitChildren(SpinTradeMainWork *g2m) {
    if (SpinTradeMain_HaveChildrenJoined(g2m)) {
        g2m->seq = SEQ_PARENT_SEND_JOIN_SIGNAL;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_ParentSendJoinSignal(SpinTradeMainWork *g2m) {
    u16 signal = SPIN_TRADE_MAIN_SIGNAL_JOIN;

    if (SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_5, &signal, sizeof(signal)) == TRUE) {
        g2m->seq = SEQ_PARENT_WAIT_JOIN;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static UnionApp *SpinTradeMain_GetUnionApp(SpinTradeMainWork *g2m) {
    return g2m->g2p->param->app;
}

static int SpinTradeMainSeq_ParentWaitJoin(SpinTradeMainWork *g2m) {
    int count = SpinTradeMain_CountChildren(g2m) + 1;

    if (count != UnionApp_CountJoined(SpinTradeMain_GetUnionApp(g2m))) {
        return SEQ_RESULT_WAIT;
    }
    g2m->seq = SEQ_PARENT_SEND_PLAYER_COUNT;
    return SEQ_RESULT_CONTINUE;
}

static int SpinTradeMainSeq_ParentSendPlayerCount(SpinTradeMainWork *g2m) {
    g2m->playerCount = SpinTradeMain_CountChildren(g2m) + 1;
    if (SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_10, &g2m->playerCount, sizeof(g2m->playerCount)) == TRUE) {
        g2m->sendCount = 0;
        g2m->seq = SEQ_PARENT_SEND_POSITIONS;
    }
    return SEQ_RESULT_WAIT;
}

// Sends a member's position a frame, the parent's first
static int SpinTradeMainSeq_ParentSendPositions(SpinTradeMainWork *g2m) {
    SpinTradePositionData data;
    int i;
    int count;

    if (g2m->sendCount == 0) {
        data.netId = 0;
        data.position = 0;
        if (SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_9, &data, sizeof(data)) == TRUE) {
            g2m->sendCount++;
        }
        return SEQ_RESULT_WAIT;
    }

    for (i = 1, count = 1; i < SPIN_TRADE_MEMBER_MAX; i++) {
        if (g2m->g2c->unk42 & (1 << i)) {
            if (count >= g2m->sendCount) {
                data.netId = i;
                data.position = g2m->sendCount;
                if (SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_9, &data, sizeof(data)) == TRUE) {
                    g2m->sendCount++;
                }
                break;
            }
            count++;
        }
    }
    if (g2m->sendCount >= g2m->playerCount) {
        g2m->seq = SEQ_PARENT_SEND_START_SIGNAL;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_ParentSendStartSignal(SpinTradeMainWork *g2m) {
    u16 signal = SPIN_TRADE_MAIN_SIGNAL_START;

    if (SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_5, &signal, sizeof(signal)) == TRUE) {
        g2m->seq = SEQ_SYNC_START;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_ChildSendJoin(SpinTradeMainWork *g2m) {
    if (SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_6, NULL, 0) == TRUE) {
        g2m->seq = SEQ_CHILD_WAIT_START_SIGNAL;
        g2m->unk10 = 0;
        return SEQ_RESULT_WAIT;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_ChildWaitStartSignal(SpinTradeMainWork *g2m) {
    if (SpinTradeMain_HasSignal(g2m, SPIN_TRADE_MAIN_SIGNAL_START) == TRUE) {
        g2m->playerCount = SpinTradeMain_CountChildren(g2m) + 1;
        g2m->seq = SEQ_SYNC_START;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_SyncStart(SpinTradeMainWork *g2m) {
    NetHandle *handle = NetHandle_GetOwn();

    WaitIcon_Free(g2m->waitIcon);
    g2m->waitIcon = NULL;
    NetHandle_SendTimingPair(handle, SPIN_TRADE_SYNC_START, SPIN_TRADE_SYNC_CLASS);
    g2m->seq = SEQ_SYNC_START_WAIT;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_SyncStartWait(SpinTradeMainWork *g2m) {
    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), SPIN_TRADE_SYNC_START, SPIN_TRADE_SYNC_CLASS)) {
        g2m->seq = SEQ_SEND_PARTY_INDEX;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_SendPartyIndex(SpinTradeMainWork *g2m) {
    if (SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_11, &g2m->g2p->partyIndex, sizeof(g2m->g2p->partyIndex))) {
        g2m->seq = SEQ_SEND_PARTY;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_SendParty(SpinTradeMainWork *g2m) {
    NetHandle *handle = NetHandle_GetOwn();
    u32 joined = UnionApp_GetJoined(SpinTradeMain_GetUnionApp(g2m));

    if (GFL_NetSendCommandToBits(handle, (u8)joined, SPIN_TRADE_NET_CMD_BLOCK, SPIN_TRADE_COMM_BLOCK_SIZE, g2m->party,
                                 TRUE, TRUE, TRUE)) {
        g2m->seq = SEQ_WAIT_PARTIES;
    }
    return SEQ_RESULT_WAIT;
}

// Once every party has arrived, tells the others whether one of them has a bad Egg
static int SpinTradeMainSeq_WaitParties(SpinTradeMainWork *g2m) {
    if (g2m->playerCount == SpinTradeMain_CountParties(g2m)) {
        if (SpinTradeMain_AnyPartyHasBadEgg(g2m) == TRUE) {
            SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_13, NULL, 0);
            g2m->seq = SEQ_BAD_EGG;
        } else {
            SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_14, NULL, 0);
            g2m->seq = SEQ_WAIT_EGG_CHECK;
        }
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_WaitEggCheck(SpinTradeMainWork *g2m) {
    if (SpinTradeMain_IsBadEggFound(g2m) == TRUE) {
        g2m->seq = SEQ_BAD_EGG;
    } else if (g2m->playerCount == SpinTradeMain_CountEggChecks(g2m)) {
        g2m->seq = SEQ_GAME_INIT;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_GameInit(SpinTradeMainWork *g2m) {
    SpinTradeEggFallSeqWork *seqWork = SpinTradeMain_InitSeqWork(g2m, sizeof(SpinTradeEggFallSeqWork));
    int i;

    seqWork->position = g2m->position;
    seqWork->playerCount = g2m->playerCount;
    seqWork->angle = sPositionAngles[seqWork->playerCount][g2m->position];
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        if (SpinTradeMain_IsMember(g2m, i)) {
            g2m->members[i] = UnionApp_GetMember(SpinTradeMain_GetUnionApp(g2m), i);
            textCopy(g2m->members[i]->info.name, g2m->names[i]);
        }
    }
    SpinTradeMain_AddMetTrainers(g2m);
    g2m->disc.baseAngle = seqWork->angle << FX32_SHIFT;
    g2m->seq = SEQ_EGG_FALL;
    GFL_SndBGMPlay(SEQ_BGM_GURUGURU, 0xffff);
    g2m->g2p->param->started = TRUE;
    SpinTradeMain_ClearMsg(g2m);
    return SEQ_RESULT_CONTINUE;
}

// Drops an Egg every 15 frames, the player's own first
static int SpinTradeMainSeq_EggFall(SpinTradeMainWork *g2m) {
    SpinTradeEggFallSeqWork *seqWork;
    int angle;
    int netId;

    g2m->wait--;
    if (g2m->wait > 0) {
        return SEQ_RESULT_WAIT;
    }

    g2m->wait = 15;
    seqWork = SpinTradeMain_GetSeqWork(g2m);
    angle = sEggAngles[seqWork->playerCount][seqWork->position];
    netId = g2m->positions[seqWork->position].netId;
    g2m->gameData.eggAngles[seqWork->position] = angle;
    SpinTradeMain_AddEgg(g2m, netId, seqWork->position, seqWork->count, angle, seqWork->angle,
                         &g2m->objects.eggs[seqWork->position]);
    seqWork->position++;
    seqWork->position %= seqWork->playerCount;
    seqWork->count++;
    if (seqWork->count == seqWork->playerCount) {
        g2m->wait = 0;
        g2m->seq = SEQ_EGG_FALL_WAIT;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_EggFallWait(SpinTradeMainWork *g2m) {
    SpinTradeObjects *objects = &g2m->objects;
    int i = 0;
    int landed = 0;
    int count = g2m->playerCount;

    do {
        if (objects->eggs[i].landed == TRUE) {
            landed++;
        }
        i++;
    } while (i < count);
    if (landed == count) {
        g2m->seq = SEQ_COUNTDOWN_BEFORE;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_CountdownBefore(SpinTradeMainWork *g2m) {
    g2m->wait++;
    if (g2m->wait > 30) {
        g2m->wait = 0;
        if (NetSystem_GetNetId() == 0) {
            g2m->seq = SEQ_PARENT_SEND_COUNTDOWN_SIGNAL;
        } else {
            g2m->seq = SEQ_CHILD_WAIT_COUNTDOWN_SIGNAL;
        }
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_ParentSendCountdownSignal(SpinTradeMainWork *g2m) {
    u16 signal = SPIN_TRADE_MAIN_SIGNAL_COUNTDOWN;

    if (SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_5, &signal, sizeof(signal)) == TRUE) {
        g2m->seq = SEQ_SYNC_COUNTDOWN;
    }
    return SEQ_RESULT_WAIT;
}

// Records the other members as trainers the player has met
static void SpinTradeMain_AddMetTrainers(SpinTradeMainWork *g2m) {
    RivalDataSave *rivalData = getHollow_RivalData(GameData_GetSaveControl(g2m->g2p->param->gameData));
    int i;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        if (g2m->members[i] != NULL && i != NetSystem_GetNetId()) {
            Misc_AddMetTrainer(rivalData, getIDAsUInt(&g2m->members[i]->info));
        }
    }
}

static int SpinTradeMainSeq_ChildWaitCountdownSignal(SpinTradeMainWork *g2m) {
    if (SpinTradeMain_HasSignal(g2m, SPIN_TRADE_MAIN_SIGNAL_COUNTDOWN) == TRUE) {
        g2m->seq = SEQ_SYNC_COUNTDOWN;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_SyncCountdown(SpinTradeMainWork *g2m) {
    NetHandle_SendTimingPair(NetHandle_GetOwn(), SPIN_TRADE_SYNC_COUNTDOWN, SPIN_TRADE_SYNC_CLASS);
    g2m->seq = SEQ_SYNC_COUNTDOWN_WAIT;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_SyncCountdownWait(SpinTradeMainWork *g2m) {
    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), SPIN_TRADE_SYNC_COUNTDOWN, SPIN_TRADE_SYNC_CLASS)) {
        g2m->seq = SEQ_COUNTDOWN_START;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_CountdownStart(SpinTradeMainWork *g2m) {
    SpinTradeMain_StartSubFade(g2m, TRUE);
    SpinTradeMain_StartCountdown(g2m);
    g2m->seq = SEQ_COUNTDOWN_WAIT;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_CountdownWait(SpinTradeMainWork *g2m) {
    int i;

    g2m->wait++;
    if (g2m->wait < 95) {
        return SEQ_RESULT_WAIT;
    }

    for (i = 0; i < g2m->playerCount; i++) {
        SpinTradeMain_ClearName(g2m, i);
    }
    g2m->wait = 0;
    g2m->seq = SEQ_GAME_START;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_GameStart(SpinTradeMainWork *g2m) {
    NetSystem_GetNetId();
    g2m->seq = SEQ_GAME;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_Game(SpinTradeMainWork *g2m) {
    SpinTradeGameData data;
    int i;
    BOOL timeUp = SpinTradeMain_CountGameFrame(g2m);
    SpinTradeDisc *disc;

    // The game ends once the Eggs are down after the time is up
    if (timeUp == TRUE && !SpinTradeMain_IsDiscBumping(g2m) && g2m->gameData.jumpY == 0) {
        g2m->timeUp = TRUE;
    }
    if (g2m->gameData.timeUp == TRUE) {
        g2m->seq = SEQ_GAME_END;
        return SEQ_RESULT_CONTINUE;
    }

    if (timeUp == FALSE) {
        if (g2m->buttonReceived != 0 && !SpinTradeMain_IsDiscBumping(g2m)) {
            SpinTradeMain_StartDiscBump(g2m);
        }
        if (!SpinTradeMain_IsButtonPushed(g2m) && SpinTradeMain_IsButtonTouched(g2m) == TRUE) {
            if (!SpinTradeMain_IsDiscBumping(g2m)) {
                g2m->buttonSend = 1;
            }
            SpinTradeMain_PushButton(g2m);
        }
        if (g2m->buttonSend != 0) {
            if (SpinTradeComm_Send(g2m->g2p->g2c, SPIN_TRADE_NET_CMD_7, &g2m->buttonSend, 1) == TRUE) {
                g2m->buttonSend = 0;
            }
        }
    }

    // The parent speeds the disc up in the first half and slows it down in the second
    disc = &g2m->disc;
    if (NetSystem_GetNetId() == 0) {
        if (g2m->gameFrame < GAME_SLOW_FRAMES) {
            disc->speed += 0x5f;
            if (disc->speed >= FX32_CONST(7)) {
                disc->speed = FX32_CONST(7);
            }
        } else {
            disc->speed -= 0x5f;
            if (disc->speed < FX32_CONST(4)) {
                disc->speed = FX32_CONST(4);
            }
        }
    }
    if (NetSystem_GetNetId() == 0) {
        data.discAngle = g2m->disc.angle;
        data.timeUp = g2m->timeUp;
        data.jumpY = g2m->objects.eggs[0].jumpY;
        for (i = 0; i < g2m->playerCount; i++) {
            data.eggAngles[i] = g2m->objects.eggs[i].angle / FX32_ONE;
        }
        SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_8, &data, sizeof(data));
    }
    if (NetSystem_GetNetId() == 0) {
        fx32 speed = g2m->disc.speed;

        // While the Eggs are in the air, the disc turns under them at twice the speed
        if (SpinTradeMain_AreEggsJumping(g2m) == TRUE) {
            SpinTradeDisc_Turn(&g2m->disc, speed * 2);
        } else {
            SpinTradeMain_Turn(g2m, speed);
        }
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_GameEnd(SpinTradeMainWork *g2m) {
    SpinTradeMain_StartSubFade(g2m, FALSE);
    g2m->frontEgg = SpinTradeMain_FindFrontEgg(g2m);
    if (NetSystem_GetNetId() == 0) {
        if (g2m->frontEgg->netId == NetSystem_GetNetId()) {
            g2m->seq = SEQ_PARENT_ALIGN_OWN;
        } else {
            g2m->seq = SEQ_PARENT_ALIGN;
        }
    } else {
        g2m->seq = SEQ_SYNC_RESULT;
    }
    GFL_SndSEPlay(SEQ_SE_SYS_55);
    return SEQ_RESULT_CONTINUE;
}

// The parent never takes its own Egg: the disc turns on until another is in front
static int SpinTradeMainSeq_ParentAlignOwn(SpinTradeMainWork *g2m) {
    SpinTradeMain_ParentTurn(g2m, FX32_CONST(4));
    g2m->frontEgg = SpinTradeMain_FindFrontEgg(g2m);
    if (g2m->frontEgg->netId != NetSystem_GetNetId()) {
        g2m->seq = SEQ_PARENT_ALIGN;
    }
    return SEQ_RESULT_WAIT;
}

// Turns the disc until the front Egg is at the parent's position
static int SpinTradeMainSeq_ParentAlign(SpinTradeMainWork *g2m) {
    int position = g2m->position;
    int count = g2m->playerCount;
    fx32 angle;
    fx32 target;
    fx32 speed;
    fx32 distance;

    speed = FX32_CONST(4);
    angle = g2m->frontEgg->angle;
    target = sEggAngles[count][position] << FX32_SHIFT;
    distance = target - angle;
    if (distance < 0) {
        distance = target + FULL_TURN - angle;
    }
    if (distance <= FX32_CONST(16)) {
        speed >>= 1;
    }
    if (distance < speed) {
        speed = distance;
    }
    SpinTradeMain_ParentTurn(g2m, speed);
    SpinTradeMain_AddAngle(&angle, speed);
    if (angle / FX32_ONE == target / FX32_ONE) {
        g2m->seq = SEQ_SYNC_RESULT;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_SyncResult(SpinTradeMainWork *g2m) {
    NetHandle_SendTimingPair(NetHandle_GetOwn(), SPIN_TRADE_SYNC_RESULT, SPIN_TRADE_SYNC_CLASS);
    g2m->seq = SEQ_SYNC_RESULT_WAIT;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_SyncResultWait(SpinTradeMainWork *g2m) {
    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), SPIN_TRADE_SYNC_RESULT, SPIN_TRADE_SYNC_CLASS)) {
        if (NetSystem_GetNetId() == 0) {
            g2m->seq = SEQ_PARENT_SEND_RESULT;
        } else {
            g2m->seq = SEQ_CHILD_WAIT_RESULT;
        }
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_ParentSendResult(SpinTradeMainWork *g2m) {
    SpinTradeResultData data;
    int i;

    data.discAngle = g2m->gameData.discAngle / FX32_ONE;
    g2m->bonusBits = 0;
    for (i = 0; i < g2m->playerCount; i++) {
        SpinTradeEgg *egg = &g2m->objects.eggs[i];

        data.eggAngles[egg->position] = g2m->gameData.eggAngles[i];
        if (SpinTradeMain_IsBonusAngle(g2m, g2m->gameData.eggAngles[i] << FX32_SHIFT, g2m->playerCount) == TRUE) {
            g2m->bonusBits |= 1 << egg->netId;
        }
    }
    data.bonusBits = g2m->bonusBits;
    if (SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_12, &data, sizeof(data)) == TRUE) {
        g2m->seq = SEQ_RESULT_NAMES;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_ChildWaitResult(SpinTradeMainWork *g2m) {
    if (g2m->resultReceived == TRUE) {
        int i;
        SpinTradeResultData *result = &g2m->result;

        g2m->disc.angle = result->discAngle << FX32_SHIFT;
        g2m->gameData.discAngle = result->discAngle << FX32_SHIFT;
        for (i = 0; i < g2m->playerCount; i++) {
            SpinTradeEgg *egg = &g2m->objects.eggs[i];

            egg->angle = result->eggAngles[egg->position] << FX32_SHIFT;
            g2m->gameData.eggAngles[i] = result->eggAngles[egg->position];
        }
        g2m->bonusBits = result->bonusBits;
        SpinTradeMain_UpdateDisc(g2m);
        SpinTradeMain_UpdateObjects(g2m);
        g2m->frontEgg = SpinTradeMain_FindFrontEgg(g2m);
        g2m->seq = SEQ_RESULT_NAMES;
    }
    return SEQ_RESULT_WAIT;
}

// Shows the names of the Eggs' owners where the Eggs stopped
static int SpinTradeMainSeq_ResultNames(SpinTradeMainWork *g2m) {
    int i;
    int position = g2m->frontEgg->position;

    g2m->gameData.jumpY = 0;
    for (i = 0; i < g2m->playerCount; i++) {
        int netId = g2m->objects.eggs[position].netId;

        SpinTradeMain_PrintName(g2m, g2m->names[netId], i, netId);
        position = (position + 1) % g2m->playerCount;
    }
    g2m->wait = 0;
    g2m->seq = SEQ_RESULT_NAMES_WAIT;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_ResultNamesWait(SpinTradeMainWork *g2m) {
    int i;

    g2m->wait++;
    if (g2m->wait < 120) {
        return SEQ_RESULT_WAIT;
    }

    g2m->wait = 0;
    for (i = 0; i < g2m->playerCount; i++) {
        SpinTradeMain_ClearName(g2m, i);
    }
    SpinTradeMain_PrintEggMsg(g2m, AbilityHandlers_Text_ReceivedSEgg, g2m->members[g2m->frontEgg->netId]);
    SpinTradeMain_PlayME(SEQ_ME_ITEM);
    g2m->seq = SEQ_RESULT_MESSAGE_WAIT;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_ResultMessageWait(SpinTradeMainWork *g2m) {
    if (g2m->wait < 120) {
        g2m->wait++;
    } else if (!SpinTradeMain_IsMEPlaying()) {
        g2m->wait = 0;
        g2m->seq = SEQ_BONUS;
    }
    return SEQ_RESULT_WAIT;
}

// Gives the player a Berry if the Egg they got stopped on a bonus area
static int SpinTradeMainSeq_Bonus(SpinTradeMainWork *g2m) {
    int count;
    u32 item;
    u32 last;
    const u32 *range;
    BOOL added;

    SpinTradeMain_StartBonuses(g2m);
    if (!(g2m->bonusBits & (1 << g2m->frontEgg->netId))) {
        g2m->seq = SEQ_SYNC_SAVE;
        return SEQ_RESULT_WAIT;
    }

    count = g2m->playerCount;
    item = sBonusItems[count][0];
    last = sBonusItems[count][1];
    item += GFL_RandomLC(0xffff) % (last - item + 1);
    range = sBonusItems[count];
    if (item < range[0]) {
        item = range[0];
    } else if (item > last) {
        item = last;
    }
    SpinTradeMain_PrintItemMsg(g2m, AbilityHandlers_Text_LandedBonusAreaReceived, item);
    added = BagSave_AddItem(GameData_GetBag(g2m->g2p->param->gameData), item, 1, HEAPID_SPIN_TRADE);
    SpinTradeMain_PlayME(SEQ_ME_ITEM);
    if (added == TRUE) {
        g2m->seq = SEQ_BONUS_WAIT;
    } else {
        g2m->seq = SEQ_BAG_FULL_BEFORE;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_BonusWait(SpinTradeMainWork *g2m) {
    if (g2m->wait < 180) {
        g2m->wait++;
    }
    if (!SpinTradeMain_IsMEPlaying()) {
        g2m->wait = 0;
        g2m->seq = SEQ_SYNC_SAVE;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_BagFullBefore(SpinTradeMainWork *g2m) {
    if (g2m->wait < 180) {
        g2m->wait++;
    } else if (!SpinTradeMain_IsMEPlaying()) {
        g2m->wait = 0;
        g2m->seq = SEQ_BAG_FULL_WAIT;
        SpinTradeMain_PrintMsg(g2m, AbilityHandlers_Text_OhNoBagFull);
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_BagFullWait(SpinTradeMainWork *g2m) {
    if (g2m->wait < 180) {
        g2m->wait++;
    } else {
        g2m->wait = 0;
        g2m->seq = SEQ_SYNC_SAVE;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_SyncSave(SpinTradeMainWork *g2m) {
    NetHandle *handle = NetHandle_GetOwn();

    RecordAddOne(g2m->g2p->param->records, 90);
    SpinTradeMain_PrintMsg(g2m, AbilityHandlers_Text_SavingDontTurnOff);
    NetHandle_SendTimingPair(handle, SPIN_TRADE_SYNC_SAVE, SPIN_TRADE_SYNC_CLASS);
    g2m->seq = SEQ_SYNC_SAVE_WAIT;
    GCTX_HIDBlockSoftReset(8);
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_SyncSaveWait(SpinTradeMainWork *g2m) {
    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), SPIN_TRADE_SYNC_SAVE, SPIN_TRADE_SYNC_CLASS)) {
        SpinTradeMain_TradeEgg(g2m, g2m->frontEgg->netId);
        g2m->netSave = NetSave_Create(HEAPID_SPIN_TRADE, g2m->g2p->param->gameData);
        g2m->waitIcon = WaitIcon_Create(g2m->tcbManager, g2m->msg.msgWindow, 15, 16, HEAPID_SPIN_TRADE);
        g2m->seq = SEQ_SAVE;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_Save(SpinTradeMainWork *g2m) {
    BOOL done = NetSave_Update(g2m->netSave);

    if (GFL_NetErrCheck()) {
        GFL_NetErrShutdown();
        GFL_NetErrShow(1);
        Wipe_SetScreenCovered(0, WIPE_COLOR_BLACK);
        Wipe_SetScreenCovered(1, WIPE_COLOR_BLACK);
        return SEQ_RESULT_WAIT;
    }
    if (done) {
        WaitIcon_Free(g2m->waitIcon);
        g2m->waitIcon = NULL;
        NetSave_Free(g2m->netSave);
        g2m->seq = SEQ_SYNC_END;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_BadEgg(SpinTradeMainWork *g2m) {
    SpinTradeMain_PrintMsg(g2m, AbilityHandlers_Text_ThereEggCantTraded);
    g2m->seq = SEQ_BAD_EGG_WAIT;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_BadEggWait(SpinTradeMainWork *g2m) {
    g2m->wait++;
    if (g2m->wait >= 180) {
        g2m->wait = 0;
        g2m->seq = SEQ_SYNC_END;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_SyncEnd(SpinTradeMainWork *g2m) {
    NetHandle *handle = NetHandle_GetOwn();

    if (!g2m->error) {
        NetHandle_SendTimingPair(handle, SPIN_TRADE_SYNC_END, SPIN_TRADE_SYNC_CLASS);
    }
    SpinTradeMain_PrintMsg(g2m, AbilityHandlers_Text_CommunicatingPleaseStandBy_2);
    g2m->seq = SEQ_SYNC_END_WAIT;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_SyncEndWait(SpinTradeMainWork *g2m) {
    NetHandle *handle = NetHandle_GetOwn();

    if (!g2m->error) {
        if (NetHandle_IsTimingPairRecv(handle, SPIN_TRADE_SYNC_END, SPIN_TRADE_SYNC_CLASS)) {
            g2m->waitIcon = WaitIcon_Create(g2m->tcbManager, g2m->msg.msgWindow, 15, 16, HEAPID_SPIN_TRADE);
            GFL_NetSetDisconnectError(FALSE);
            NetCommand_RemoveTable(SPIN_TRADE_NET_CMD_BASE);
            UnionApp_RequestStart(SpinTradeMain_GetUnionApp(g2m));
            g2m->seq = SEQ_END_WAIT;
        }
    } else {
        g2m->wait++;
        if (g2m->wait >= 180) {
            g2m->wait = 0;
            if (g2m->g2c->unk3C == 0) {
                GflNet_SetAutoExit(0);
            }
            g2m->seq = SEQ_END_WAIT;
        }
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_EndWait(SpinTradeMainWork *g2m) {
    if (UnionApp_IsStarted(SpinTradeMain_GetUnionApp(g2m))) {
        WaitIcon_Free(g2m->waitIcon);
        g2m->waitIcon = NULL;
        g2m->seq = SEQ_FADE_OUT;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_FadeOut(SpinTradeMainWork *g2m) {
    GFL_WipeSet(WIPE_MODE_SUB_FIRST, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 8, 1, HEAPID_SPIN_TRADE);
    g2m->seq = SEQ_FADE_OUT_WAIT;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_FadeOutWait(SpinTradeMainWork *g2m) {
    if (GFL_WipeIsFinished()) {
        g2m->seq = SEQ_END;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int SpinTradeMainSeq_End(SpinTradeMainWork *g2m) {
    return SEQ_RESULT_END;
}

static void SpinTradeMain_VBlank(TCB *tcb, void *data) {
    SpinTradeMainWork *g2m = data;

    PaletteFade_Transfer(g2m->fade);
    ClActSys_VBlank();
    GFL_BGSysUpdate();
}

static void SpinTradeMain_InitGraphics(SpinTradeMainWork *g2m) {
    SpinTradeMain_InitVRAM(g2m);
    SpinTradeMain_Init3D(g2m);
    SpinTradeMain_InitBGs(g2m);
    SpinTradeMain_InitFade(g2m);
    SpinTradeMain_InitClAct(g2m);
    SpinTradeMain_LoadBGs(g2m);
    SpinTradeMain_InitMsg(g2m);
    SpinTradeMain_LoadClActResources(g2m);
    SpinTradeMain_ShowBGs(g2m);
}

static void SpinTradeMain_FreeGraphics(SpinTradeMainWork *g2m) {
    SpinTradeMain_HideBGs(g2m);
    SpinTradeMain_FreeClAct(g2m);
    SpinTradeMain_FreeMsg(g2m);
    SpinTradeMain_FreeBGs(g2m);
    SpinTradeMain_FreeFade(g2m);
    SpinTradeMain_FreeCamera(g2m);
}

static void SpinTradeMain_Draw(SpinTradeMainWork *g2m) {
    int i;
    fx32 far = FX32_CONST(4096);

    GFL_G3DCameraSetProjectionZFar(g2m->camera.camera, &far);
    GFL_G3DSysReset();
    GFL_G3DCameraFlush(g2m->camera.camera);
    GFL_G3DSysMtxViewFlush();
    NNS_G3dGlbLightVector(GX_LIGHTID_0, 0, -FX32_ONE, 0);
    NNS_G3dGlbLightColor(GX_LIGHTID_0, GX_RGB(31, 31, 31));
    NNS_G3dGlbMaterialColorDiffAmb(GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), FALSE);
    NNS_G3dGlbMaterialColorSpecEmi(GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), FALSE);
    SpinTradeMain_DrawBG3D(g2m);
    SpinTradeMain_DrawDisc(g2m);
    SpinTradeMain_DrawObjects(g2m);
    GFL_G3DSysReqSwapBuffers();
    ClActSys_Draw();

    PrintQueue_Main(g2m->msg.queue);
    PrintWindow_Flush(&g2m->msg.msgPrint, g2m->msg.queue);
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        PrintWindow_Flush(&g2m->msg.namePrints[i], g2m->msg.queue);
    }
    if (g2m->waitIcon != NULL) {
        WaitIcon_Main(g2m->waitIcon);
    }
}

static void SpinTradeMain_InitVRAM(SpinTradeMainWork *g2m) {
    GFL_BGSysSetVRAMBanks(&sSpinTradeVRAMConfig);
    BmpWin_InitAllocator(HEAPID_SPIN_TRADE);
}

static void SpinTradeMain_ShowBGs(SpinTradeMainWork *g2m) {
    GFL_BGSysSetBGEnabled(0, TRUE);
    GFL_BGSysSetBGEnabled(1, TRUE);
    GFL_BGSysSetBGEnabled(2, TRUE);
    GFL_BGSysSetBGEnabled(3, TRUE);
    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysSetBGEnabled(5, FALSE);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysSetBGEnabled(7, TRUE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
    G2_SetBG0Priority(1);
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_BD, 11, 10);
}

static void SpinTradeMain_HideBGs(SpinTradeMainWork *g2m) {
    GFL_BGSysSetBGEnabled(0, FALSE);
    GFL_BGSysSetBGEnabled(1, FALSE);
    GFL_BGSysSetBGEnabled(2, FALSE);
    GFL_BGSysSetBGEnabled(3, FALSE);
    GFL_BGSysSetBGEnabled(4, FALSE);
    GFL_BGSysSetBGEnabled(5, FALSE);
    GFL_BGSysSetBGEnabled(6, FALSE);
    GFL_BGSysSetBGEnabled(7, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, FALSE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
}

static void SpinTradeMain_Setup3D(void) {
    G3X_SetShading(GX_SHADING_TOON);
    G3X_AntiAlias(TRUE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(TRUE);
    G3X_SetEdgeColorTable(sEdgeColors);
    G3X_SetClearColor(GX_RGB(31, 31, 31), 0, 0x7fff, 63, FALSE);
    G3_ViewPort(0, 0, 255, 191);
}

static void SpinTradeMain_Init3D(SpinTradeMainWork *g2m) {
    SpinTradeCameraWork *camera = &g2m->camera;

    GFL_G3DSysCreate(FALSE, 1, FALSE, 1, 0, HEAPID_SPIN_TRADE, SpinTradeMain_Setup3D);
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
    {
        VecFx32 up = { 0, FX32_ONE, 0 };

        camera->camera = GFL_G3DCameraCreate(
            G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)), FX_CosIdx(DEG_TO_IDX(20)), FX32_ONE * 4 / 3, 0,
            FX32_ONE, FX32_CONST(1024), 0, &sSpinTradeCameraPosition, &up, &sSpinTradeCameraTarget, HEAPID_SPIN_TRADE);
    }
    GFL_G3DCameraFlush(camera->camera);
}

static void SpinTradeMain_InitBGs(SpinTradeMainWork *g2m) {
    GFL_BGSysCreate(HEAPID_SPIN_TRADE);
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    {
        BGSysLCDConfig config = { 1, 0, 0, TRUE };

        GFL_BGSysSetLCDConfig(&config);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x0000),
            GX_BG_CHARBASE(0x04000),
            0x4000,
            GX_BG_EXTPLTT_01,
            0,
            0,
            FALSE,
        };

        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(1);
        GFL_BGSysClearCharCore(1, BGSYS_TILE_SIZE_16, 0, HEAPID_SPIN_TRADE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x0800),
            GX_BG_CHARBASE(0x0c000),
            0x4000,
            GX_BG_EXTPLTT_01,
            2,
            0,
            FALSE,
        };

        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(2);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x1000),
            GX_BG_CHARBASE(0x0c000),
            0x4000,
            GX_BG_EXTPLTT_01,
            3,
            0,
            FALSE,
        };

        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(3);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x1000),
            GX_BG_CHARBASE(0x14000),
            0,
            GX_BG_EXTPLTT_01,
            2,
            0,
            FALSE,
        };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(6);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x1800),
            GX_BG_CHARBASE(0x1c000),
            0,
            GX_BG_EXTPLTT_01,
            3,
            0,
            FALSE,
        };

        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(7);
    }
}

static void SpinTradeMain_LoadBGs(SpinTradeMainWork *g2m) {
    void *file;

    file = SpinTradeMain_LoadFile(g2m, NARC_SPIN_TRADE_BG_NCLR, FALSE);
    NNS_G2dGetUnpackedPaletteData(file, &g2m->paletteData);
    PaletteFade_LoadData(g2m->fade, g2m->paletteData->pRawData, 0, 0, 0x40);
    GFL_HeapFree(file);

    file = SpinTradeMain_LoadFile(g2m, NARC_SPIN_TRADE_SUB_NCLR, FALSE);
    NNS_G2dGetUnpackedPaletteData(file, &g2m->paletteData);
    PaletteFade_LoadData(g2m->fade, g2m->paletteData->pRawData, 1, 0, 0x40);
    GFL_HeapFree(file);
    PaletteFade_BlendPalettes(g2m->fade, 1, 0xffff, 8, GX_RGB(0, 0, 0));

    file = SpinTradeMain_LoadFile(g2m, NARC_SPIN_TRADE_BG_NCGR, FALSE);
    NNS_G2dGetUnpackedCharacterData(file, &g2m->charData);
    GFL_BGSysLoadChar(3, g2m->charData->pRawData, g2m->charData->szByte, 0);
    GFL_HeapFree(file);

    file = SpinTradeMain_LoadFile(g2m, NARC_SPIN_TRADE_SUB_BG2_NCGR, FALSE);
    NNS_G2dGetUnpackedCharacterData(file, &g2m->charData);
    GFL_BGSysLoadChar(6, g2m->charData->pRawData, g2m->charData->szByte, 0);
    GFL_HeapFree(file);

    file = SpinTradeMain_LoadFile(g2m, NARC_SPIN_TRADE_SUB_BG3_NCGR, FALSE);
    NNS_G2dGetUnpackedCharacterData(file, &g2m->charData);
    GFL_BGSysLoadChar(7, g2m->charData->pRawData, g2m->charData->szByte, 0);
    GFL_HeapFree(file);

    file = SpinTradeMain_LoadFile(g2m, NARC_SPIN_TRADE_BG2_NSCR, FALSE);
    NNS_G2dGetUnpackedScreenData(file, &g2m->screenData);
    GFL_BGSysBufferScrDefault(2, g2m->screenData->rawData, g2m->screenData->szByte);
    GFL_BGSysLoadScr(2);
    GFL_HeapFree(file);

    file = SpinTradeMain_LoadFile(g2m, NARC_SPIN_TRADE_BG3_NSCR, FALSE);
    NNS_G2dGetUnpackedScreenData(file, &g2m->screenData);
    GFL_BGSysBufferScrDefault(3, g2m->screenData->rawData, g2m->screenData->szByte);
    GFL_BGSysLoadScr(3);
    GFL_HeapFree(file);

    file = SpinTradeMain_LoadFile(g2m, NARC_SPIN_TRADE_BUTTON_NSCR, FALSE);
    NNS_G2dGetUnpackedScreenData(file, &g2m->screenData);
    GFL_BGSysBufferScrDefault(6, g2m->screenData->rawData, g2m->screenData->szByte);
    GFL_BGSysLoadScr(6);
    GFL_HeapFree(file);

    file = SpinTradeMain_LoadFile(g2m, NARC_SPIN_TRADE_SUB_BG3_NSCR, FALSE);
    NNS_G2dGetUnpackedScreenData(file, &g2m->screenData);
    GFL_BGSysBufferScrDefault(7, g2m->screenData->rawData, g2m->screenData->szByte);
    GFL_BGSysLoadScr(7);
    GFL_HeapFree(file);
}

static void SpinTradeMain_FreeBGs(SpinTradeMainWork *g2m) {
    BmpWin_FreeAllocator();
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysFree();
}

static void SpinTradeMain_LoadButtonScreen(SpinTradeMainWork *g2m, u32 fileId) {
    void *file = SpinTradeMain_LoadFile(g2m, fileId, FALSE);

    NNS_G2dGetUnpackedScreenData(file, &g2m->screenData);
    GFL_BGSysBufferScrDefault(6, g2m->screenData->rawData, g2m->screenData->szByte);
    GFL_BGSysLoadScr(6);
    GFL_HeapFree(file);
}

static void SpinTradeMain_ShowButtonHalf(SpinTradeMainWork *g2m) {
    SpinTradeMain_LoadButtonScreen(g2m, NARC_SPIN_TRADE_BUTTON_HALF_NSCR);
}

static void SpinTradeMain_ShowButton(SpinTradeMainWork *g2m) {
    SpinTradeMain_LoadButtonScreen(g2m, NARC_SPIN_TRADE_BUTTON_NSCR);
}

static void SpinTradeMain_ShowButtonPushed(SpinTradeMainWork *g2m) {
    SpinTradeMain_LoadButtonScreen(g2m, NARC_SPIN_TRADE_BUTTON_PUSHED_NSCR);
}

static void SpinTradeMain_InitFade(SpinTradeMainWork *g2m) {
    g2m->fade = PaletteFade_Create(HEAPID_SPIN_TRADE);
    PaletteFade_SetTransferAll(g2m->fade, TRUE);
    PaletteFade_AllocBuffer(g2m->fade, 0, 0x200, HEAPID_SPIN_TRADE);
    PaletteFade_AllocBuffer(g2m->fade, 1, 0x200, HEAPID_SPIN_TRADE);
}

static void SpinTradeMain_FreeFade(SpinTradeMainWork *g2m) {
    PaletteFade_FreeBuffer(g2m->fade, 0);
    PaletteFade_FreeBuffer(g2m->fade, 1);
    PaletteFade_Free(g2m->fade);
}

// Darkens the touch screen
static void SpinTradeMain_SetSubFade(SpinTradeMainWork *g2m, int evy) {
    PaletteFade_BlendPalettes(g2m->fade, 1, 0xffff, evy, GX_RGB(0, 0, 0));
}

static void SpinTradeMain_InitClAct(SpinTradeMainWork *g2m) {
    ClActSys_Create(&data_02093f08, &sSpinTradeVRAMConfig, HEAPID_SPIN_TRADE);
    g2m->clactUnit = ClActUnit_Create(30, 1, HEAPID_SPIN_TRADE);
}

static void SpinTradeMain_LoadClActResources(SpinTradeMainWork *g2m) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_SPIN_TRADE_OBJ, HEAPID_SPIN_TRADE);

    g2m->chars = ClActSys_LoadChars(arc, 6, FALSE, 0, HEAPID_SPIN_TRADE);
    g2m->palette = ClActSys_LoadPltt(arc, 0, 0, 0, HEAPID_SPIN_TRADE);
    g2m->cellAnims = ClActSys_LoadCellAnims(arc, 10, 14, HEAPID_SPIN_TRADE);
    GFL_ArcToolFree(arc);
    GFL_NetSetIconScreen(TRUE, HEAPID_SPIN_TRADE);
}

static void SpinTradeMain_FreeClAct(SpinTradeMainWork *g2m) {
    ClActSys_FreeChars(g2m->chars);
    ClActSys_FreePltt(g2m->palette);
    ClActSys_FreeCellAnims(g2m->cellAnims);
    ClActUnit_Delete(g2m->clactUnit);
    ClActSys_Delete();
}

static void SpinTradeMain_InitMsg(SpinTradeMainWork *g2m) {
    SpinTradeMsgWork *msg = &g2m->msg;
    int i;

    LoadSysMsgBox(1, 1, 15, 0, HEAPID_SPIN_TRADE);
    PaletteFade_LoadNCLR(g2m->fade, ARCID_WINFRAME, 3, HEAPID_SPIN_TRADE, 0, 0x20, 14 * 16);
    PaletteFade_LoadNCLR(g2m->fade, ARCID_FONT, 5, HEAPID_SPIN_TRADE, 0, 0x20, 15 * 16);
    msg->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_SPIN_TRADE);
    msg->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS, HEAPID_SPIN_TRADE);
    msg->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_SPIN_TRADE);
    msg->msgWindow = BmpWin_CreateDynamic(1, 2, 19, 26, 4, 15, TRUE);
    msg->strbuf = GFL_StrBufCreate(0x100, HEAPID_SPIN_TRADE);
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        g2m->names[i] = GFL_StrBufCreate(8, HEAPID_SPIN_TRADE);
    }
    PrintWindow_Init(&msg->msgPrint, msg->msgWindow);
    msg->queue = PrintQueue_CreateDefault(HEAPID_SPIN_TRADE);
    SpinTradeMain_CreateNameWindows(g2m, g2m->g2p->memberCount);
}

static void SpinTradeMain_FreeMsg(SpinTradeMainWork *g2m) {
    SpinTradeMsgWork *msg = &g2m->msg;
    int i;

    BmpWin_ClearScreenNow(msg->msgWindow);
    BmpWin_Free(msg->msgWindow);
    SpinTradeMain_FreeNameWindows(g2m);
    GFL_MsgDataFree(msg->msgData);
    GFL_WordSetSystemFree(msg->wordSet);
    GFL_FontFree(msg->font);
    GFL_StrBufFree(msg->strbuf);
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        GFL_StrBufFree(g2m->names[i]);
    }
    BmpWin_FreeAllocator();
}

static void SpinTradeMain_PrintMsg(SpinTradeMainWork *g2m, u32 messageId) {
    SpinTradeMsgWork *msg = &g2m->msg;
    BmpWin *window = msg->msgWindow;

    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    BmpWin_DrawFrame(window, 2, 1, 14);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    GFL_MsgDataLoadStrbuf(msg->msgData, messageId, msg->strbuf);
    PrintWindow_Print(&msg->msgPrint, msg->queue, 0, 0, msg->strbuf, msg->font, COLOR_NAME);
    BmpWin_Transfer(window);
}

// Prints a message with the player's name and another member's
static void SpinTradeMain_PrintEggMsg(SpinTradeMainWork *g2m, u32 messageId, UnionAppMember *member) {
    SpinTradeMsgWork *msg = &g2m->msg;
    BmpWin *window = msg->msgWindow;
    UnionApp *app = SpinTradeMain_GetUnionApp(g2m);
    StrBuf *strbuf;

    copyVarForText(msg->wordSet, 1, &UnionApp_GetMember(app, NetSystem_GetNetId())->info);
    copyVarForText(msg->wordSet, 2, &member->info);
    strbuf = GFL_StrBufCreate(0x100, HEAPID_SPIN_TRADE);
    GFL_MsgDataLoadStrbuf(msg->msgData, messageId, strbuf);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->strbuf, strbuf);
    GFL_StrBufFree(strbuf);
    BmpWin_DrawFrame(window, 2, 1, 14);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    PrintWindow_Print(&msg->msgPrint, msg->queue, 0, 0, msg->strbuf, msg->font, COLOR_NAME);
    BmpWin_Transfer(window);
}

static void SpinTradeMain_PrintItemMsg(SpinTradeMainWork *g2m, u32 messageId, u32 item) {
    SpinTradeMsgWork *msg = &g2m->msg;
    BmpWin *window = msg->msgWindow;
    StrBuf *strbuf;

    loadItemNameToStrbuf(msg->wordSet, 0, item);
    strbuf = GFL_StrBufCreate(0x100, HEAPID_SPIN_TRADE);
    GFL_MsgDataLoadStrbuf(msg->msgData, messageId, strbuf);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->strbuf, strbuf);
    GFL_StrBufFree(strbuf);
    BmpWin_DrawFrame(window, 2, 1, 14);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    PrintWindow_Print(&msg->msgPrint, msg->queue, 0, 0, msg->strbuf, msg->font, COLOR_NAME);
    BmpWin_Transfer(window);
}

static void SpinTradeMain_ClearMsg(SpinTradeMainWork *g2m) {
    BmpWin *window = g2m->msg.msgWindow;

    BmpWin_ClearFrame(window, 0);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    BmpWin_Transfer(window);
}

static void SpinTradeMain_CreateNameWindows(SpinTradeMainWork *g2m, int count) {
    int i;
    SpinTradeMsgWork *msg = &g2m->msg;
    const SpinTradeNameWindowData *data = sNameWindows[count];

    msg->nameWindowCount = count;
    for (i = 0; i < count; i++) {
        msg->nameWindows[i] = BmpWin_CreateDynamic(data[i].bg, data[i].x, data[i].y, data[i].width, data[i].height,
                                                   data[i].palette, TRUE);
        PrintWindow_Init(&msg->namePrints[i], msg->nameWindows[i]);
    }
}

static void SpinTradeMain_FreeNameWindows(SpinTradeMainWork *g2m) {
    int i;
    SpinTradeMsgWork *msg = &g2m->msg;

    for (i = 0; i < msg->nameWindowCount; i++) {
        BmpWin_ClearScreenNow(msg->nameWindows[i]);
        BmpWin_Free(msg->nameWindows[i]);
    }
}

static void SpinTradeMain_PrintNameColor(SpinTradeMainWork *g2m, StrBuf *name, int index, u32 color) {
    SpinTradeMsgWork *msg = &g2m->msg;
    BmpWin *window = msg->nameWindows[index];

    BmpWin_DrawFrame(window, 2, 1, 14);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    PrintWindow_Print(&msg->namePrints[index], msg->queue, 4, 1, name, msg->font, color);
    BmpWin_Transfer(window);
}

// Prints a member's name in a name window, the player's own in another color
static void SpinTradeMain_PrintName(SpinTradeMainWork *g2m, StrBuf *name, int index, int netId) {
    u32 color = COLOR_NAME;

    if (netId == NetSystem_GetNetId()) {
        color = COLOR_NAME_MINE;
    }
    SpinTradeMain_PrintNameColor(g2m, name, index, color);
}

static void SpinTradeMain_ClearName(SpinTradeMainWork *g2m, int index) {
    BmpWin *window = g2m->msg.nameWindows[index];

    BmpWin_ClearFrame(window, 0);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    BmpWin_Transfer(window);
}

static void SpinTradeMain_FreeCamera(SpinTradeMainWork *g2m) {
    GFL_G3DCameraFree(g2m->camera.camera);
}

static BOOL SpinTradeMain_IsButtonTouched(SpinTradeMainWork *g2m) {
    if (GFL_HIDGetTouchPressedRect(sSpinTradeButtonRects) != TOUCH_RECT_NONE) {
        return TRUE;
    }
    return FALSE;
}

static void SpinTradeMain_InitDisc(SpinTradeMainWork *g2m) {
    int count = g2m->g2p->memberCount;
    SpinTradeDisc *disc = &g2m->disc;

    disc->scene = GFL_G3DMgrNewScene(g2m->g3dManager, &sDiscScenes[count - 2]);
    disc->angle = 0;
    disc->angleOffset = sDiscAngleOffsets[count];
    VEC_Set(&disc->scale, FX32_ONE, FX32_ONE, FX32_ONE);
    disc->rotX = 0;
    disc->rotY = 0;
    disc->rotZ = 0;
    VEC_Set(&disc->pos, 0, FX32_CONST(-36), 0);
    disc->drawPos = disc->pos;
}

static void SpinTradeMain_FreeDisc(SpinTradeMainWork *g2m) {
    GFL_G3DMgrDeleteScene(g2m->g3dManager, g2m->disc.scene);
}

static void SpinTradeMain_UpdateDisc(SpinTradeMainWork *g2m) {
    SpinTradeDisc *disc = &g2m->disc;
    fx32 angle = g2m->gameData.discAngle;

    SpinTradeMain_AddAngle(&angle, disc->baseAngle);
    SpinTradeMain_AddAngle(&angle, disc->angleOffset);
    disc->rotY = (360 - angle / FX32_ONE) % 360;
    disc->drawPos.x = disc->pos.x + disc->offset.x + disc->shake.x;
    disc->drawPos.y = disc->pos.y + disc->offset.y + disc->shake.y;
    disc->drawPos.z = disc->pos.z + disc->offset.z + disc->shake.z;
}

static void SpinTradeMain_DrawDisc(SpinTradeMainWork *g2m) {
    SpinTradeDisc *disc = &g2m->disc;
    G3DActor *actor = GFL_G3DMgrGetActor(g2m->g3dManager, disc->scene);
    SRTMatrix srt;

    VEC_Set(&srt.translation, disc->drawPos.x, disc->drawPos.y, disc->drawPos.z);
    VEC_Set(&srt.scale, disc->scale.x, disc->scale.y, disc->scale.z);
    MAT3_RotationEulerZYX(disc->rotX, DEG_TO_IDX(1) * disc->rotY, disc->rotZ, &srt.rotation);
    GFL_G3DSysDrawObj(actor, &srt);
}

static void SpinTradeDisc_Turn(SpinTradeDisc *disc, fx32 angle) {
    SpinTradeMain_AddAngle(&disc->angle, angle);
}

// The model behind the disc
static void SpinTradeMain_InitBG3D(SpinTradeMainWork *g2m) {
    g2m->bgScene = GFL_G3DMgrNewScene(g2m->g3dManager, &sBGScene);
    G3X_SetClearColor(GX_RGB(0, 0, 0), 0, 0x7fff, 0, FALSE);
}

static void SpinTradeMain_FreeBG3D(SpinTradeMainWork *g2m) {
    GFL_G3DMgrDeleteScene(g2m->g3dManager, g2m->bgScene);
}

static void SpinTradeMain_DrawBG3D(SpinTradeMainWork *g2m) {
    G3DActor *actor = GFL_G3DMgrGetActor(g2m->g3dManager, g2m->bgScene);
    SRTMatrix srt;

    VEC_Set(&srt.translation, 0, 0, 0);
    VEC_Set(&srt.scale, FX32_ONE, FX32_ONE, FX32_ONE);
    MAT3_RotationEulerZYX(0, 0, 0, &srt.rotation);
    GFL_G3DSysDrawObj(actor, &srt);
}

static const SpinTradeBonusArea sBonusAreas4[] = {
    { 37, 142 },
    { 215, 323 },
};

static const SpinTradeBonusArea sBonusAreas5[] = {
    { 0, 215 },
    { 323, 359 },
};

static const SpinTradeBonusArea sBonusAreas3[] = {
    { 0, 198 },
    { 342, 359 },
};

static const SpinTradeBonusAreaList sBonusAreas[SPIN_TRADE_MEMBER_MAX + 1] = {
    { 0, NULL },
    { 0, NULL },
    { NELEMS(sBonusAreas2), sBonusAreas2 },
    { NELEMS(sBonusAreas3), sBonusAreas3 },
    { NELEMS(sBonusAreas4), sBonusAreas4 },
    { NELEMS(sBonusAreas5), sBonusAreas5 },
};

static const G3DSceneSetup sCursorScenes[SPIN_TRADE_MEMBER_MAX] = {
    { sCursor0Resources, NELEMS(sCursor0Resources), sCursorActors, NELEMS(sCursorActors) },
    { sCursor1Resources, NELEMS(sCursor1Resources), sCursorActors, NELEMS(sCursorActors) },
    { sCursor2Resources, NELEMS(sCursor2Resources), sCursorActors, NELEMS(sCursorActors) },
    { sCursor3Resources, NELEMS(sCursor3Resources), sCursorActors, NELEMS(sCursorActors) },
    { sCursor4Resources, NELEMS(sCursor4Resources), sCursorActors, NELEMS(sCursorActors) },
};

// The Eggs' scene and their shadows'
static const G3DSceneSetup sEggScenes[] = {
    { sEggResources, NELEMS(sEggResources), sEggActors, NELEMS(sEggActors) },
    { sShadowResources, NELEMS(sShadowResources), sShadowActors, NELEMS(sShadowActors) },
};

static void SpinTradeMain_InitObjects(SpinTradeMainWork *g2m) {
    SpinTradeObjects *objects = &g2m->objects;
    int i;

    objects->eggScene = GFL_G3DMgrNewScene(g2m->g3dManager, &sEggScenes[0]);
    objects->shadowScene = GFL_G3DMgrNewScene(g2m->g3dManager, &sEggScenes[1]);
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        objects->cursorScenes[i] = GFL_G3DMgrNewScene(g2m->g3dManager, &sCursorScenes[i]);
    }
}

static void SpinTradeMain_FreeObjects(SpinTradeMainWork *g2m) {
    int i;
    SpinTradeObjects *objects = &g2m->objects;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        GFL_G3DMgrDeleteScene(g2m->g3dManager, objects->cursorScenes[i]);
    }
    GFL_G3DMgrDeleteScene(g2m->g3dManager, objects->eggScene);
    GFL_G3DMgrDeleteScene(g2m->g3dManager, objects->shadowScene);
}

static void SpinTradeMain_ShowEgg(SpinTradeMainWork *g2m, SpinTradeEgg *egg) {
    egg->visible = TRUE;
    SpinTradeMain_AddEggShadow(g2m, egg);
    SpinTradeMain_AddEggCursor(g2m, egg);
}

static void SpinTradeMain_UpdateObjects(SpinTradeMainWork *g2m) {
    int i = 0;
    int count = g2m->playerCount;
    SpinTradeObjects *objects = &g2m->objects;

    for (; i < count; i++) {
        if (objects->eggs[i].active) {
            SpinTradeEgg_Update(&objects->eggs[i], &g2m->disc.offset, g2m->gameData.eggAngles[i] << FX32_SHIFT,
                                g2m->gameData.jumpY);
        }
        if (objects->shadows[i].active) {
            SpinTradeMain_UpdateEggShadow(g2m, &objects->shadows[i], g2m->gameData.jumpY);
        }
        if (objects->cursors[i].active) {
            SpinTradeMain_UpdateEggCursor(g2m, &objects->cursors[i], i);
        }
    }
}

static void SpinTradeMain_DrawObjects(SpinTradeMainWork *g2m) {
    int i = 0;
    int count = g2m->playerCount;
    SpinTradeObjects *objects = &g2m->objects;
    SpinTradeEgg *egg;
    SpinTradeEggShadow *shadow = objects->shadows;
    SpinTradeEggCursor *cursor = objects->cursors;

    egg = objects->eggs;
    for (; i < count; egg++, shadow++, cursor++, i++) {
        if (egg->visible) {
            G3DActor *actor = GFL_G3DMgrGetActor(g2m->g3dManager, objects->eggScene);
            SRTMatrix srt;

            VEC_Set(&srt.translation, egg->pos.x, egg->pos.y, egg->pos.z);
            VEC_Set(&srt.scale, egg->scale.x, egg->scale.y, egg->scale.z);
            MAT3_RotationEulerZYX(egg->rotX, egg->rotY, egg->rotZ, &srt.rotation);
            GFL_G3DSysDrawObj(actor, &srt);
        }
        if (shadow->active) {
            G3DActor *actor = GFL_G3DMgrGetActor(g2m->g3dManager, objects->shadowScene);
            SRTMatrix srt;

            VEC_Set(&srt.translation, shadow->pos.x, shadow->pos.y, shadow->pos.z);
            VEC_Set(&srt.scale, shadow->scale.x, shadow->scale.y, shadow->scale.z);
            MAT3_RotationEulerZYX(shadow->rotX, shadow->rotY, shadow->rotZ, &srt.rotation);
            GFL_G3DSysDrawObj(actor, &srt);
        }
        if (cursor->active) {
            G3DActor *actor = GFL_G3DMgrGetActor(g2m->g3dManager, objects->cursorScenes[i]);
            SRTMatrix srt;

            VEC_Set(&srt.translation, cursor->pos.x, cursor->pos.y, cursor->pos.z);
            VEC_Set(&srt.scale, FX32_ONE, FX32_ONE, FX32_ONE);
            MTX_Identity33_(&srt.rotation);
            GFL_G3DSysDrawObj(actor, &srt);
        }
    }
}

static void SpinTradeMain_TurnEggs(SpinTradeMainWork *g2m, fx32 angle) {
    int i = 0;
    int count = g2m->playerCount;
    SpinTradeObjects *objects = &g2m->objects;

    for (; i < count; i++) {
        if (objects->eggs[i].active) {
            SpinTradeMain_AddAngle(&objects->eggs[i].angle, angle);
        }
    }
}

// Places an Egg on the disc at an angle
static void SpinTradeEgg_Update(SpinTradeEgg *egg, const VecFx32 *offset, fx32 angle, fx32 jumpY) {
    u16 degrees;

    SpinTradeMain_AddAngle(&angle, egg->angleOffset);
    degrees = angle / FX32_ONE;
    egg->pos.x = egg->offset.x + offset->x + EGG_RADIUS * GFL_MathCosDeg(degrees);
    egg->pos.y = egg->offset.y - FX32_CONST(6) + offset->y + jumpY;
    egg->pos.z = egg->offset.z + offset->z + EGG_RADIUS * GFL_MathSinDeg(degrees);
    egg->rotX = egg->rotation.x / FX32_ONE;
    egg->rotY = egg->rotation.y / FX32_ONE;
    egg->rotZ = egg->rotation.z / FX32_ONE;
}

// The Egg nearest the player's position, going back against the disc's turn
static SpinTradeEgg *SpinTradeMain_FindFrontEgg(SpinTradeMainWork *g2m) {
    int distances[SPIN_TRADE_MEMBER_MAX];
    int order[SPIN_TRADE_MEMBER_MAX];
    int index;
    int count = g2m->playerCount;
    SpinTradeObjects *objects = &g2m->objects;
    int i = 0;
    int j;
    int nearest;
    fx32 target = sEggAngles[count][g2m->position] << FX32_SHIFT;

    do {
        order[i] = i;
        distances[i] = 0xffff;
        i++;
    } while (i < count);

    for (i = 0; i < count; i++) {
        fx32 distance = target - objects->eggs[i].angle;

        if (distance < 0) {
            distance = target + FULL_TURN - objects->eggs[i].angle;
        }
        distances[i] = distance / FX32_ONE;
    }

    for (i = 1; i < count; i++) {
        index = order[i];
        nearest = distances[index];

        for (j = i - 1; j >= 0 && distances[order[j]] > nearest; j--) {
            order[j + 1] = order[j];
        }
        order[j + 1] = index;
    }
    return &objects->eggs[order[0]];
}

static void SpinTradeMain_AddEggShadow(SpinTradeMainWork *g2m, SpinTradeEgg *egg) {
    SpinTradeEggShadow *shadow = &g2m->objects.shadows[egg->position];

    shadow->active = TRUE;
    shadow->egg = egg;
    SpinTradeMain_UpdateEggShadow(g2m, shadow, 0);
}

// The shadow stays on the disc under its Egg, smaller the higher the Egg is
static void SpinTradeMain_UpdateEggShadow(SpinTradeMainWork *g2m, SpinTradeEggShadow *shadow, fx32 jumpY) {
    fx32 height;
    fx32 shrink;

    shadow->rotX = 0;
    shadow->rotY = 0;
    shadow->rotZ = 0;
    shadow->pos = shadow->egg->pos;
    shadow->pos.x -= shadow->egg->offset.x;
    shadow->pos.y -= shadow->egg->offset.y + jumpY;
    shadow->pos.z -= shadow->egg->offset.z;
    shadow->pos.y += FX32_CONST(2);

    height = shadow->egg->offset.y + jumpY;
    if (height < 0) {
        height = -height;
    }
    shrink = height / 128;
    if (shrink > FX32_CONST(0.5)) {
        shrink = FX32_CONST(0.5);
    }
    shadow->scale.x = FX32_ONE - shrink;
    shadow->scale.y = FX32_ONE - shrink;
    shadow->scale.z = FX32_ONE - shrink;
}

static void SpinTradeMain_AddEggCursor(SpinTradeMainWork *g2m, SpinTradeEgg *egg) {
    SpinTradeEggCursor *cursor = &g2m->objects.cursors[egg->position];

    cursor->active = TRUE;
    cursor->egg = egg;
}

static void SpinTradeMain_UpdateEggCursor(SpinTradeMainWork *g2m, SpinTradeEggCursor *cursor, int index) {
    G3DActor *actor = GFL_G3DMgrGetActor(g2m->g3dManager, g2m->objects.cursorScenes[index]);

    cursor->pos = cursor->egg->pos;
    cursor->pos.y += FX32_CONST(20);
    GFL_G3DActorStepAnmFrameLoop(actor, 0, FX32_ONE);
}

// Sets up a member's Egg and starts its drop onto the disc
static void SpinTradeMain_AddEgg(SpinTradeMainWork *g2m, int netId, int position, int index, int angle, int angleOffset,
                                 SpinTradeEgg *egg) {
    SpinTradeEggFallTask *task;

    egg->netId = netId;
    egg->position = position;
    egg->index = index;
    egg->startPosition = position;
    egg->angle = angle << FX32_SHIFT;
    egg->angleOffset = angleOffset << FX32_SHIFT;
    egg->active = TRUE;
    egg->landed = FALSE;

    task = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_SPIN_TRADE), sizeof(SpinTradeEggFallTask), FALSE, "guru2_main.c", 4878);
    MI_CpuFill8(task, 0, sizeof(SpinTradeEggFallTask));
    task->g2m = g2m;
    task->egg = egg;
    GFL_TCBMgrAddTask(g2m->tcbManager, SpinTradeEggFallTask_Main, task, 0);
}

static void SpinTradeEggFallTask_Main(TCB *tcb, void *data) {
    SpinTradeEggFallTask *task = data;
    int result;

    do {
        result = sEggFallFuncs[task->seq](task);
    } while (result == SEQ_RESULT_CONTINUE);
    if (result == SEQ_RESULT_END) {
        task->egg->landed = TRUE;
        GFL_HeapFree(task);
        GFL_TCBRemove(tcb);
    }
}

static int SpinTradeEggFallTask_Init(SpinTradeEggFallTask *task) {
    SpinTradeEgg *egg = task->egg;

    egg->scale.x = FX32_ONE;
    egg->scale.y = FX32_ONE;
    egg->scale.z = FX32_ONE;
    egg->offset.y = FX32_CONST(192);
    SpinTradeMain_ShowEgg(task->g2m, egg);
    task->speed = FX32_CONST(8);
    task->seq = 1;
    return SEQ_RESULT_WAIT;
}

static int SpinTradeEggFallTask_Fall(SpinTradeEggFallTask *task) {
    SpinTradeEgg *egg = task->egg;

    egg->offset.y -= task->speed;
    if (!task->sePlayed && egg->offset.y <= FX32_CONST(104)) {
        task->sePlayed = TRUE;
        GFL_SndSEPlay(SEQ_SE_SYS_51);
    }
    if (egg->offset.y <= FX32_CONST(32)) {
        egg->offset.y = FX32_CONST(32);
        task->seq = 2;
    }
    return SEQ_RESULT_WAIT;
}

// Slows the Egg down onto the disc, and shows its owner's name
static int SpinTradeEggFallTask_Land(SpinTradeEggFallTask *task) {
    SpinTradeEgg *egg = task->egg;

    if (task->speed > FX32_ONE) {
        task->speed -= FX32_ONE;
        if (task->speed < FX32_ONE) {
            task->speed = FX32_ONE;
        }
    }
    egg->offset.y -= task->speed;
    if (egg->offset.y <= 0) {
        egg->offset.y = 0;
        SpinTradeMain_PrintName(task->g2m, task->g2m->names[egg->netId], egg->index, egg->netId);
        return SEQ_RESULT_END;
    }
    return SEQ_RESULT_WAIT;
}

// An Egg's jump, up and down over 15 frames each
static void SpinTradeEggJumpTask_Update(SpinTradeMainWork *g2m, SpinTradeEggJumpTask *task) {
    SpinTradeEgg *egg = task->egg;

    switch (task->seq) {
    case 0:
        SpinTradeMain_StartEggSpin(g2m, egg);
        task->speed = FX32_CONST(4);
        task->accel = -0x444;
        task->seq++;
    case 1:
        egg->jumpY += task->speed;
        task->speed += task->accel;
        task->frame++;
        if (task->frame >= 15) {
            task->frame = 0;
            task->speed = 0;
            if (task->count == 0) {
                task->count = 1;
            } else {
                egg->jumpY = 0;
                SpinTradeMain_StartEggShake(g2m, egg);
                MI_CpuFill8(task, 0, sizeof(SpinTradeEggJumpTask));
            }
        }
        break;
    }
}

static void SpinTradeEggJumpTask_Main(TCB *tcb, void *data) {
    SpinTradeMainWork *g2m = data;
    int i;
    SpinTradeEggJumpTask *task = g2m->jumpTasks;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++, task++) {
        if (task->active) {
            SpinTradeEggJumpTask_Update(g2m, task);
        }
    }
}

static void SpinTradeMain_InitEggJump(SpinTradeMainWork *g2m) {
    MI_CpuFill8(g2m->jumpTasks, 0, sizeof(g2m->jumpTasks));
    g2m->jumpTCB = GFL_TCBMgrAddTask(g2m->tcbManager, SpinTradeEggJumpTask_Main, g2m, 0x100);
}

static void SpinTradeMain_FreeEggJump(SpinTradeMainWork *g2m) {
    GFL_TCBRemove(g2m->jumpTCB);
}

static void SpinTradeMain_StartEggJump(SpinTradeMainWork *g2m, SpinTradeEgg *egg) {
    int i;
    SpinTradeEggJumpTask *task = g2m->jumpTasks;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++, task++) {
        if (!task->active) {
            task->active = TRUE;
            task->egg = egg;
            return;
        }
    }
}

static void SpinTradeMain_StartEggJumps(SpinTradeMainWork *g2m) {
    SpinTradeObjects *objects = &g2m->objects;
    int count = g2m->playerCount;
    int i = 0;

    do {
        if (objects->eggs[i].active) {
            SpinTradeMain_StartEggJump(g2m, &objects->eggs[i]);
        }
        i++;
    } while (i < count);
}

static BOOL SpinTradeMain_IsEggJumping(SpinTradeMainWork *g2m) {
    int i;
    SpinTradeEggJumpTask *task = g2m->jumpTasks;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++, task++) {
        if (task->active) {
            return TRUE;
        }
    }
    return FALSE;
}

// An Egg's spin as it jumps, which slows down each turn
static void SpinTradeEggSpinTask_Update(SpinTradeEggSpinTask *task) {
    SpinTradeEgg *egg = task->egg;

    switch (task->seq) {
    case 0:
        task->speed = FX32_CONST(24);
        egg->rotation.y = 0;
        task->seq++;
    case 1:
        SpinTradeMain_AddAngle(&egg->rotation.y, task->speed);
        if (egg->rotation.y < FX32_CONST(4)) {
            task->speed = (u32)task->speed >> 1;
            if (task->speed < FX32_CONST(4)) {
                egg->rotation.y = 0;
                MI_CpuFill8(task, 0, sizeof(SpinTradeEggSpinTask));
            }
        }
        break;
    }
}

static void SpinTradeEggSpinTask_Main(TCB *tcb, void *data) {
    SpinTradeMainWork *g2m = data;
    int i;
    SpinTradeEggSpinTask *task = g2m->spinTasks;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++, task++) {
        if (task->active) {
            SpinTradeEggSpinTask_Update(task);
        }
    }
}

static void SpinTradeMain_InitEggSpin(SpinTradeMainWork *g2m) {
    MI_CpuFill8(g2m->spinTasks, 0, sizeof(g2m->spinTasks));
    g2m->spinTCB = GFL_TCBMgrAddTask(g2m->tcbManager, SpinTradeEggSpinTask_Main, g2m, 0x101);
}

static void SpinTradeMain_FreeEggSpin(SpinTradeMainWork *g2m) {
    GFL_TCBRemove(g2m->spinTCB);
}

// Starts an Egg's spin, or starts it over
static void SpinTradeMain_StartEggSpin(SpinTradeMainWork *g2m, SpinTradeEgg *egg) {
    SpinTradeEggSpinTask *tasks = g2m->spinTasks;
    int i;
    SpinTradeEggSpinTask *task;

    task = tasks;
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++, task++) {
        if (task->active == TRUE && task->egg == egg) {
            task->seq = 0;
            return;
        }
    }

    task = tasks;
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++, task++) {
        if (!task->active) {
            task->active = TRUE;
            task->egg = egg;
            return;
        }
    }
}

// An Egg's wobble after it lands, which halves each swing
static void SpinTradeEggShakeTask_Update(SpinTradeEggShakeTask *task) {
    SpinTradeEgg *egg = task->egg;
    fx32 angle;
    fx32 rotation;

    switch (task->seq) {
    case 0:
        egg->rotation.z = 0;
        task->angle = 0;
        task->amplitude = FX32_CONST(30);
        task->speed = FX32_CONST(20);
        task->seq++;
    case 1:
        rotation = task->amplitude / FX32_ONE * GFL_MathSinDeg(task->angle / FX32_ONE);
        egg->rotation.z = 0;
        SpinTradeMain_AddAngle(&egg->rotation.z, rotation);
        angle = task->angle;
        SpinTradeMain_AddAngle(&task->angle, task->speed);
        if (task->angle < angle) {
            task->amplitude = (u32)task->amplitude >> 1;
            if (task->amplitude == 0) {
                egg->rotation.z = 0;
                MI_CpuFill8(task, 0, sizeof(SpinTradeEggShakeTask));
            }
        }
        break;
    }
}

static void SpinTradeEggShakeTask_Main(TCB *tcb, void *data) {
    SpinTradeMainWork *g2m = data;
    int i;
    SpinTradeEggShakeTask *task = g2m->shakeTasks;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++, task++) {
        if (task->active) {
            SpinTradeEggShakeTask_Update(task);
        }
    }
}

static void SpinTradeMain_InitEggShake(SpinTradeMainWork *g2m) {
    MI_CpuFill8(g2m->shakeTasks, 0, sizeof(g2m->shakeTasks));
    g2m->shakeTCB = GFL_TCBMgrAddTask(g2m->tcbManager, SpinTradeEggShakeTask_Main, g2m, 0x102);
}

static void SpinTradeMain_FreeEggShake(SpinTradeMainWork *g2m) {
    GFL_TCBRemove(g2m->shakeTCB);
}

static void SpinTradeMain_StartEggShake(SpinTradeMainWork *g2m, SpinTradeEgg *egg) {
    SpinTradeEggShakeTask *tasks = g2m->shakeTasks;
    int i;
    SpinTradeEggShakeTask *task;

    task = tasks;
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++, task++) {
        if (task->active == TRUE && task->egg == egg) {
            task->seq = 0;
            return;
        }
    }

    task = tasks;
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++, task++) {
        if (!task->active) {
            task->active = TRUE;
            task->egg = egg;
            return;
        }
    }
}

// The disc sinks, springs back up and throws the Eggs, then shakes until they have landed
static void SpinTradeDiscBumpTask_Update(SpinTradeMainWork *g2m, SpinTradeDiscBumpTask *task) {
    SpinTradeDisc *disc = &g2m->disc;

    switch (task->seq) {
    case 0:
        break;
    case 1:
        task->shake = FX32_CONST(4);
        task->count = 0;
        task->seq++;
    case 2:
        disc->offset.y -= FX32_ONE;
        if (disc->offset.y <= FX32_CONST(-8)) {
            task->seq++;
        }
        break;
    case 3:
        disc->offset.y += FX32_CONST(2);
        if (disc->offset.y >= 0) {
            disc->offset.y = 0;
            SpinTradeMain_StartEggJumps(g2m);
            task->jumping = TRUE;
            task->seq++;
            GFL_SndSEPlay(SEQ_SE_DANSA);
        }
        break;
    case 4:
        disc->shake.y = task->shake;
        task->shake = -task->shake;
        task->count++;
        if ((task->count & 3) == 0) {
            if (task->shake <= FX32_ONE) {
                disc->shake.y = 0;
                task->seq++;
            } else {
                task->shake >>= 1;
            }
        }
        break;
    case 5:
        if (!SpinTradeMain_IsEggJumping(g2m)) {
            task->active = FALSE;
            task->jumping = FALSE;
            task->seq = 0;
            GFL_SndSEPlay(SEQ_SE_SYS_83);
        }
        break;
    }
}

static void SpinTradeDiscBumpTask_Main(TCB *tcb, void *data) {
    SpinTradeMainWork *g2m = data;

    SpinTradeDiscBumpTask_Update(g2m, &g2m->bumpTask);
}

static void SpinTradeMain_InitDiscBump(SpinTradeMainWork *g2m) {
    MI_CpuFill8(&g2m->bumpTask, 0, sizeof(SpinTradeDiscBumpTask));
    g2m->bumpTCB = GFL_TCBMgrAddTask(g2m->tcbManager, SpinTradeDiscBumpTask_Main, g2m, 0xff);
}

static void SpinTradeMain_FreeDiscBump(SpinTradeMainWork *g2m) {
    GFL_TCBRemove(g2m->bumpTCB);
}

static void SpinTradeMain_StartDiscBump(SpinTradeMainWork *g2m) {
    SpinTradeDiscBumpTask *task = &g2m->bumpTask;

    task->active = TRUE;
    task->seq = 1;
}

static BOOL SpinTradeMain_AreEggsJumping(SpinTradeMainWork *g2m) {
    return g2m->bumpTask.jumping;
}

static BOOL SpinTradeMain_IsDiscBumping(SpinTradeMainWork *g2m) {
    return g2m->bumpTask.active;
}

// Plays the countdown's sounds while its actor's animation runs
static void SpinTradeCountdownTask_Main(TCB *tcb, void *data) {
    SpinTradeCountdownTask *task = data;

    switch (task->seq) {
    case 0:
        if (task->count < 4) {
            u32 se[] = { SEQ_SE_SYS_52, SEQ_SE_SYS_52, SEQ_SE_SYS_52, SEQ_SE_SYS_53 };

            task->wait--;
            if (task->wait <= 0) {
                task->wait = 30;
                GFL_SndSEPlay(se[task->count]);
                task->count++;
            }
        }
        ClActor_SetAnimSpeed(task->actor, FX32_CONST(1.5));
        if (!ClActor_IsAnimActive(task->actor)) {
            task->seq++;
        }
        break;
    case 1:
        ClActor_Delete(task->actor);
        GFL_HeapFree(task);
        GFL_TCBRemove(tcb);
        break;
    }
}

static void SpinTradeMain_StartCountdown(SpinTradeMainWork *g2m) {
    SpinTradeCountdownTask *task;
    ClActorSetup setup;

    task =
        GFL_HeapAllocate(HEAPID_TAIL(HEAPID_SPIN_TRADE), sizeof(SpinTradeCountdownTask), FALSE, "guru2_main.c", 5638);
    MI_CpuFill8(task, 0, sizeof(SpinTradeCountdownTask));
    setup.x = 128;
    setup.y = 100;
    setup.sequence = 0;
    setup.bgPriority = 0;
    setup.priority = 0;
    task->actor =
        ClActor_Create(g2m->clactUnit, g2m->chars, g2m->palette, g2m->cellAnims, &setup, 0, HEAPID_SPIN_TRADE);
    ClActor_SetAnimSpeed(task->actor, FX32_ONE);
    task->g2m = g2m;
    GFL_TCBMgrAddTask(g2m->tcbManager, SpinTradeCountdownTask_Main, task, 0);
}

// The button's push on the touch screen
static void SpinTradeButtonTask_Update(SpinTradeMainWork *g2m, SpinTradeButtonTask *task) {
    switch (task->seq) {
    case 0:
        break;
    case 1:
        SpinTradeMain_ShowButtonHalf(g2m);
        GFL_SndSEPlay(SEQ_SE_SYS_54);
        task->frame = 0;
        task->seq++;
    case 2:
        task->frame++;
        if (task->frame >= 2) {
            task->frame = 0;
            task->seq++;
            SpinTradeMain_ShowButtonPushed(g2m);
        }
        break;
    case 3:
        task->frame++;
        if (task->frame >= 4) {
            SpinTradeMain_ShowButtonHalf(g2m);
            task->frame = 0;
            task->seq++;
        }
        break;
    case 4:
        task->frame++;
        if (task->frame >= 2) {
            SpinTradeMain_ShowButton(g2m);
            task->active = FALSE;
            task->seq = 0;
        }
        break;
    }
}

static void SpinTradeButtonTask_Main(TCB *tcb, void *data) {
    SpinTradeMainWork *g2m = data;

    SpinTradeButtonTask_Update(g2m, &g2m->buttonTask);
}

static void SpinTradeMain_InitButton(SpinTradeMainWork *g2m) {
    MI_CpuFill8(&g2m->buttonTask, 0, sizeof(SpinTradeButtonTask));
    g2m->buttonTCB = GFL_TCBMgrAddTask(g2m->tcbManager, SpinTradeButtonTask_Main, g2m, 0x103);
}

static void SpinTradeMain_FreeButton(SpinTradeMainWork *g2m) {
    GFL_TCBRemove(g2m->buttonTCB);
}

static u32 SpinTradeMain_IsButtonPushed(SpinTradeMainWork *g2m) {
    return g2m->buttonTask.active;
}

static void SpinTradeMain_PushButton(SpinTradeMainWork *g2m) {
    SpinTradeButtonTask *task = &g2m->buttonTask;

    task->active = TRUE;
    task->seq = 1;
}

// An Egg hops on its bonus area, with a rest between the hops
static void SpinTradeBonusTask_Update(SpinTradeMainWork *g2m, SpinTradeBonusTask *task) {
    switch (task->seq) {
    case 0:
        break;
    case 1:
        task->egg->offset.y = sBonusHopHeights[task->frame] << FX32_SHIFT;
        task->frame++;
        if (task->frame >= (int)NELEMS(sBonusHopHeights)) {
            task->frame = 0;
            task->seq++;
        }
        break;
    case 2:
        task->wait++;
        if (task->wait >= 15) {
            task->wait = 0;
            task->seq = 1;
        }
        break;
    }
}

static void SpinTradeBonusTask_Main(TCB *tcb, void *data) {
    SpinTradeMainWork *g2m = data;
    int i;
    SpinTradeBonusTask *task = g2m->bonusTasks;

    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++, task++) {
        SpinTradeBonusTask_Update(g2m, task);
    }
}

static void SpinTradeMain_InitBonus(SpinTradeMainWork *g2m) {
    MI_CpuFill8(g2m->bonusTasks, 0, sizeof(g2m->bonusTasks));
    g2m->bonusTCB = GFL_TCBMgrAddTask(g2m->tcbManager, SpinTradeBonusTask_Main, g2m, 0x104);
}

static void SpinTradeMain_FreeBonus(SpinTradeMainWork *g2m) {
    GFL_TCBRemove(g2m->bonusTCB);
}

static void SpinTradeMain_StartBonus(SpinTradeMainWork *g2m, SpinTradeEgg *egg) {
    SpinTradeBonusTask *task = &g2m->bonusTasks[egg->netId];

    task->seq = 1;
    task->egg = egg;
}

static void SpinTradeMain_StartBonuses(SpinTradeMainWork *g2m) {
    u32 bonusBits = g2m->bonusBits;
    int i = 0;
    int count = g2m->playerCount;
    SpinTradeObjects *objects = &g2m->objects;

    for (; i < count; i++) {
        if (objects->eggs[i].active && (bonusBits & (1 << objects->eggs[i].netId))) {
            SpinTradeMain_StartBonus(g2m, &objects->eggs[i]);
        }
    }
}

static void SpinTradeFadeTask_Main(TCB *tcb, void *data) {
    SpinTradeFadeTask *task = data;
    BOOL done = FALSE;

    if (task->out == TRUE) {
        task->evy--;
        if (task->evy <= 0) {
            task->evy = 0;
            done = TRUE;
        }
    } else {
        task->evy++;
        if (task->evy >= 8) {
            task->evy = 8;
            done = TRUE;
        }
    }
    SpinTradeMain_SetSubFade(task->g2m, task->evy);
    if (done == TRUE) {
        GFL_HeapFree(task);
        GFL_TCBRemove(tcb);
    }
}

// Lights the touch screen up for the game, or darkens it again
static void SpinTradeMain_StartSubFade(SpinTradeMainWork *g2m, int out) {
    int start[] = { 0, 8 };
    SpinTradeFadeTask *task =
        GFL_HeapAllocate(HEAPID_TAIL(HEAPID_SPIN_TRADE), sizeof(SpinTradeFadeTask), FALSE, "guru2_main.c", 5992);

    task->g2m = g2m;
    task->out = out;
    task->evy = start[out];
    GFL_TCBMgrAddTask(g2m->tcbManager, SpinTradeFadeTask_Main, task, 0);
}

// Adds to an angle, which stays within a turn
static void SpinTradeMain_AddAngle(fx32 *angle, fx32 addend) {
    *angle += addend;
    while (*angle < 0) {
        *angle += FULL_TURN;
    }
    *angle %= FULL_TURN;
}

static void SpinTradeMain_Turn(SpinTradeMainWork *g2m, fx32 angle) {
    SpinTradeDisc_Turn(&g2m->disc, angle);
    SpinTradeMain_TurnEggs(g2m, angle);
}

// Returns whether the game's time is up
static BOOL SpinTradeMain_CountGameFrame(SpinTradeMainWork *g2m) {
    g2m->gameFrame++;
    if (g2m->gameFrame < GAME_FRAMES) {
        return FALSE;
    }
    g2m->gameFrame = GAME_FRAMES;
    return TRUE;
}

static void *SpinTradeMain_LoadFile(SpinTradeMainWork *g2m, u32 fileId, BOOL head) {
    u32 size = GFL_ArcToolGetDataLength(g2m->arc, fileId);
    void *file;

    if (head == TRUE) {
        file = GFL_HeapAllocate(HEAPID_SPIN_TRADE, size, FALSE, "guru2_main.c", 6090);
    } else {
        file = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_SPIN_TRADE), size, FALSE, "guru2_main.c", 6092);
    }
    GFL_ArcToolRead(g2m->arc, fileId, file);
    return file;
}

// SPIN_TRADE_NET_CMD_7: someone pressed the button
void SpinTradeMain_OnButton(SpinTradeMainWork *g2m, u32 value) {
    if (value != 0) {
        g2m->buttonReceived = value;
    }
}

// SPIN_TRADE_NET_CMD_8: the parent's SpinTradeGameData
void SpinTradeMain_OnGameData(SpinTradeMainWork *g2m, const SpinTradeGameData *data) {
    g2m->gameDataReceived = TRUE;
    g2m->gameData = *data;
}

static void *SpinTradeMain_InitSeqWork(SpinTradeMainWork *g2m, u32 size) {
    MI_CpuFill8(g2m->seqWork, 0, sizeof(g2m->seqWork));
    return g2m->seqWork;
}

static void *SpinTradeMain_GetSeqWork(SpinTradeMainWork *g2m) {
    return g2m->seqWork;
}

static BOOL SpinTradeMain_HasSignal(SpinTradeMainWork *g2m, u32 signal) {
    if (g2m->g2c->unk40 & signal) {
        return TRUE;
    }
    return FALSE;
}

// Whether every child of the receipt screen has joined this one
static BOOL SpinTradeMain_HaveChildrenJoined(SpinTradeMainWork *g2m) {
    int count = 0;
    int max = g2m->g2p->memberCount - 1;
    u32 bits = g2m->g2c->unk42;

    while (bits != 0) {
        count += bits & 1;
        bits >>= 1;
    }
    return count >= max;
}

static int SpinTradeMain_CountChildren(SpinTradeMainWork *g2m) {
    int count = 0;
    u32 bits = g2m->g2c->unk42;

    while (bits != 0) {
        count += bits & 1;
        bits >>= 1;
    }
    return count;
}

// SPIN_TRADE_NET_CMD_9: a member's position
void SpinTradeMain_OnPosition(SpinTradeMainWork *g2m, const SpinTradePositionData *position) {
    g2m->positions[position->position] = *position;
    if (position->netId == NetSystem_GetNetId()) {
        g2m->position = position->position;
    }
}

// SPIN_TRADE_NET_CMD_10: the parent's player count, which this machine counts itself
void SpinTradeMain_OnPlayerCount(SpinTradeMainWork *g2m, u32 value) {
    g2m->playerCount = UnionApp_CountJoined(SpinTradeMain_GetUnionApp(g2m));
}

static BOOL SpinTradeMain_IsMember(SpinTradeMainWork *g2m, int netId) {
    if (netId != 0 && !(g2m->g2c->unk42 & (1 << netId))) {
        return FALSE;
    }
    return TRUE;
}

// Replaces the Egg the player put in with the one of the given member
static void SpinTradeMain_TradeEgg(SpinTradeMainWork *g2m, int netId) {
    PokeParty *party = g2m->party;
    PokeParty *otherParty = SpinTradeComm_GetBlock(g2m->g2c, netId);
    u32 otherIndex = g2m->partyIndices[netId];
    PartyPkm *pkm = PokeParty_GetPkm(party, g2m->g2p->partyIndex);
    PartyPkm *otherPkm = PokeParty_GetPkm(otherParty, otherIndex);
    UnionApp *app = SpinTradeMain_GetUnionApp(g2m);

    PokeParty_SetupMetData(otherPkm, 6, &UnionApp_GetMember(app, NetSystem_GetNetId())->info, 30002, HEAPID_SPIN_TRADE);
    copyPartyPkm(otherPkm, pkm);
}

// How many members' parties have arrived
static int SpinTradeMain_CountParties(SpinTradeMainWork *g2m) {
    int count = 0;
    u32 bits = g2m->g2c->blockBits;

    while (bits != 0) {
        count += bits & 1;
        bits >>= 1;
    }
    return count;
}

// SPIN_TRADE_NET_CMD_11: the party slot a member trades
void SpinTradeMain_OnPartyIndex(SpinTradeMainWork *g2m, int netId, u32 value) {
    g2m->partyIndices[netId] = value;
}

// SPIN_TRADE_NET_CMD_12: the parent's SpinTradeResultData
void SpinTradeMain_OnResult(SpinTradeMainWork *g2m, const SpinTradeResultData *data) {
    g2m->result = *data;
    g2m->resultReceived = TRUE;
}

// Whether an Egg at this angle of the disc is over a bonus area
static BOOL SpinTradeMain_IsBonusAngle(SpinTradeMainWork *g2m, fx32 angle, int playerCount) {
    fx32 edge;
    u32 start;
    const SpinTradeBonusAreaList *list;
    int count;
    u32 end;
    const SpinTradeBonusArea *area;

    SpinTradeMain_AddAngle(&angle, -g2m->disc.angle);
    SpinTradeMain_AddAngle(&angle, -g2m->disc.angleOffset);
    edge = angle;
    SpinTradeMain_AddAngle(&edge, FX32_CONST(-4));
    start = edge / FX32_ONE;
    edge = angle;
    SpinTradeMain_AddAngle(&edge, FX32_CONST(5));
    end = edge / FX32_ONE;

    list = &sBonusAreas[playerCount];
    count = list->count;
    area = list->areas;
    for (; count != 0; area++, count--) {
        if ((start >= area->start && start <= area->end) || (end >= area->start && end <= area->end)) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL SpinTradeMain_PartyHasBadEgg(PokeParty *party) {
    int count = PokeParty_GetPkmCount(party);
    int i;

    for (i = 0; i < count; i++) {
        PartyPkm *pkm = PokeParty_GetPkm(party, i);

        if (PokeParty_GetParam(pkm, PKM_PARAM_IS_EGG, NULL) && PokeParty_GetParam(pkm, PKM_PARAM_BAD_EGG, NULL)) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL SpinTradeMain_AnyPartyHasBadEgg(SpinTradeMainWork *g2m) {
    int i;

    if (SpinTradeMain_PartyHasBadEgg(g2m->party) == TRUE) {
        return TRUE;
    }
    for (i = 0; i < SPIN_TRADE_MEMBER_MAX; i++) {
        if (SpinTradeMain_IsMember(g2m, i) &&
            SpinTradeMain_PartyHasBadEgg(SpinTradeComm_GetBlock(g2m->g2c, i)) == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}

// How many members have found no bad Egg
static int SpinTradeMain_CountEggChecks(SpinTradeMainWork *g2m) {
    int count = 0;
    u32 bits = g2m->g2c->unk4A;

    while (bits != 0) {
        count += bits & 1;
        bits >>= 1;
    }
    return count;
}

static BOOL SpinTradeMain_IsBadEggFound(SpinTradeMainWork *g2m) {
    if (g2m->g2c->unk48 != 0) {
        return TRUE;
    }
    return FALSE;
}

// Plays a jingle over the music
static void SpinTradeMain_PlayME(u32 me) {
    GFL_SndBGMSetPaused(TRUE);
    GFL_SndBGMPush();
    GFL_SndBGMPlay(me, 0xffff);
}

// Returns whether the jingle still plays, and brings the music back once it doesn't
static BOOL SpinTradeMain_IsMEPlaying(void) {
    if (!GFL_SndBGMIsPlaying()) {
        GFL_SndBGMPop();
        GFL_SndBGMSetPaused(FALSE);
        return FALSE;
    }
    return TRUE;
}

// The parent turns the disc and the Eggs, and sends the others where they are
static void SpinTradeMain_ParentTurn(SpinTradeMainWork *g2m, fx32 angle) {
    if (NetSystem_GetNetId() == 0) {
        SpinTradeGameData data;
        int i;

        SpinTradeMain_Turn(g2m, angle);
        data.discAngle = g2m->disc.angle;
        data.jumpY = g2m->objects.eggs[0].jumpY;
        for (i = 0; i < g2m->playerCount; i++) {
            data.eggAngles[i] = g2m->objects.eggs[i].angle / FX32_ONE;
        }
        SpinTradeComm_Send(g2m->g2c, SPIN_TRADE_NET_CMD_8, &data, sizeof(data));
    }
}
