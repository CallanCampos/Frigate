#include "types.h"
#include "app/guru2.h"
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

// guru2_main.c (the ROM's name, from its allocations): the Spin Trade's screen with the disc. The Eggs drop onto the
// disc, which the parent spins while every player's touches of the button make the Eggs jump; when the time is up
// each player takes the Egg in front of them, and those whose Egg stopped on a bonus area get a Berry. Names are ours

// The Spin Trade's archives, which constants/arc.h doesn't have yet: its models and BGs, and the countdown's actor
#define ARCID_GURU2 180
#define ARCID_GURU2_OBJ 181

// The class of the timing syncs
#define GURU2_SYNC_CLASS 0x28
enum {
    GURU2_SYNC_START = 0xca,
    GURU2_SYNC_COUNTDOWN,
    GURU2_SYNC_RESULT,
    GURU2_SYNC_SAVE,
    GURU2_SYNC_END,
};

// The bits of GURU2_NET_CMD_5's signals
#define GURU2_MAIN_SIGNAL_JOIN 1
#define GURU2_MAIN_SIGNAL_COUNTDOWN 4
#define GURU2_MAIN_SIGNAL_START 8

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
    NARC_GURU2_EGG_NSBMD = 0,
    NARC_GURU2_DISC_2_NSBMD,
    NARC_GURU2_DISC_3_NSBMD,
    NARC_GURU2_DISC_4_NSBMD,
    NARC_GURU2_DISC_5_NSBMD,
    NARC_GURU2_SHADOW_NSBMD,
    NARC_GURU2_CURSOR_0_NSBMD,
    NARC_GURU2_CURSOR_1_NSBMD,
    NARC_GURU2_CURSOR_2_NSBMD,
    NARC_GURU2_CURSOR_3_NSBMD,
    NARC_GURU2_CURSOR_4_NSBMD,
    NARC_GURU2_CURSOR_0_ANM,
    NARC_GURU2_CURSOR_1_ANM,
    NARC_GURU2_CURSOR_2_ANM,
    NARC_GURU2_CURSOR_3_ANM,
    NARC_GURU2_CURSOR_4_ANM,
    NARC_GURU2_BG_NCGR,
    NARC_GURU2_BG_NCLR,
    NARC_GURU2_BG3_NSCR,
    NARC_GURU2_BG2_NSCR,
    NARC_GURU2_SUB_NCLR,
    NARC_GURU2_SUB_BG3_NCGR,
    NARC_GURU2_SUB_BG2_NCGR,
    NARC_GURU2_SUB_BG3_NSCR,
    NARC_GURU2_BUTTON_NSCR,
    NARC_GURU2_BUTTON_PUSHED_NSCR,
    NARC_GURU2_BUTTON_HALF_NSCR,
    NARC_GURU2_BG_NSBMD,
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

// GURU2_NET_CMD_8: what the parent sends each frame of the game
typedef struct {
    fx32 discAngle;
    // How high the Eggs have jumped
    fx32 jumpY;
    // In degrees
    u16 eggAngles[GURU2_MEMBER_MAX];
    u16 timeUp;
} Guru2GameData;

// GURU2_NET_CMD_12: where everything stopped, in degrees
typedef struct {
    u16 discAngle;
    // The net IDs whose Egg stopped on a bonus area
    u16 bonusBits;
    // By position
    u16 eggAngles[6];
} Guru2ResultData;

// GURU2_NET_CMD_9: a member's position around the disc
typedef struct {
    u16 position;
    u16 netId;
} Guru2PositionData;

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
} Guru2Egg;

typedef struct {
    BOOL active;
    u16 rotX;
    u16 rotY;
    u16 rotZ;
    VecFx32 scale;
    VecFx32 pos;
    u8 unk24[0x54];
    Guru2Egg *egg;
} Guru2EggShadow;

// The arrow over a player's Egg
typedef struct {
    BOOL active;
    VecFx32 pos;
    u8 unk10[0x54];
    Guru2Egg *egg;
} Guru2EggCursor;

typedef struct {
    u8 unk0[0x140];
    Guru2Egg eggs[GURU2_MEMBER_MAX];
    Guru2EggShadow shadows[GURU2_MEMBER_MAX];
    Guru2EggCursor cursors[GURU2_MEMBER_MAX];
    u32 eggScene;
    u32 shadowScene;
    u32 cursorScenes[GURU2_MEMBER_MAX];
} Guru2Objects;

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
} Guru2Disc;

// The work of the sequence that drops the Eggs
typedef struct {
    int count;
    int angle;
    u8 unk8[8];
    int position;
    int playerCount;
} Guru2EggFallSeqWork;

typedef struct {
    int seq;
    u32 unk4;
    BOOL sePlayed;
    fx32 speed;
    Guru2Egg *egg;
    Guru2MainWork *g2m;
} Guru2EggFallTask;

typedef struct {
    BOOL active;
    int seq;
    int frame;
    int count;
    fx32 speed;
    fx32 accel;
    u32 unk18;
    Guru2Egg *egg;
} Guru2EggJumpTask;

typedef struct {
    BOOL active;
    int seq;
    u32 unk8;
    fx32 speed;
    Guru2Egg *egg;
} Guru2EggSpinTask;

typedef struct {
    BOOL active;
    int seq;
    fx32 amplitude;
    fx32 speed;
    fx32 angle;
    Guru2Egg *egg;
} Guru2EggShakeTask;

// The disc's bump when the button is pressed
typedef struct {
    int seq;
    // Whether the Eggs are in the air
    BOOL jumping;
    BOOL active;
    int count;
    fx32 shake;
} Guru2DiscBumpTask;

typedef struct {
    u16 seq;
    u16 active;
    int frame;
} Guru2ButtonTask;

// An Egg's hops on a bonus area
typedef struct {
    int seq;
    int frame;
    int wait;
    Guru2Egg *egg;
} Guru2BonusTask;

typedef struct {
    int seq;
    int wait;
    u32 unk8;
    int count;
    Guru2MainWork *g2m;
    ClActor *actor;
} Guru2CountdownTask;

typedef struct {
    Guru2MainWork *g2m;
    int out;
    int evy;
} Guru2FadeTask;

typedef struct {
    int nameWindowCount;
    Font *font;
    MsgData *msgData;
    WordSet *wordSet;
    BmpWin *msgWindow;
    BmpWin *nameWindows[GURU2_MEMBER_MAX];
    PrintWindow msgPrint;
    PrintWindow namePrints[GURU2_MEMBER_MAX];
    PrintQueue *queue;
    StrBuf *strbuf;
} Guru2MsgWork;

typedef struct {
    u8 unk0[0x14];
    G3DCamera *camera;
} Guru2CameraWork;

struct Guru2MainWork {
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
    Guru2Egg *frontEgg;
    int position;
    int playerCount;
    // A press of the button that arrived this frame, and this machine's to send
    u32 buttonReceived;
    u32 buttonSend;
    BOOL gameDataReceived;
    BOOL resultReceived;
    // The party slot each member trades, by net ID
    u32 partyIndices[GURU2_MEMBER_MAX];
    Guru2GameData gameData;
    Guru2ResultData result;
    Guru2PositionData positions[GURU2_MEMBER_MAX];
    UnionAppMember *members[GURU2_MEMBER_MAX];
    StrBuf *names[GURU2_MEMBER_MAX];
    u8 seqWork[0x20];
    Guru2ProcWork *g2p;
    Guru2CommWork *g2c;
    Guru2Disc disc;
    Guru2Objects objects;
    G3DManager *g3dManager;
    Guru2EggJumpTask jumpTasks[GURU2_MEMBER_MAX];
    Guru2EggSpinTask spinTasks[GURU2_MEMBER_MAX];
    Guru2EggShakeTask shakeTasks[GURU2_MEMBER_MAX];
    Guru2DiscBumpTask bumpTask;
    Guru2ButtonTask buttonTask;
    Guru2BonusTask bonusTasks[GURU2_MEMBER_MAX];
    Guru2MsgWork msg;
    Guru2CameraWork camera;
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

typedef int (*Guru2MainSeqFunc)(Guru2MainWork *g2m);
typedef int (*Guru2EggFallFunc)(Guru2EggFallTask *task);

// A name window, in the layout of Gen 4's window templates
typedef struct {
    u32 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u16 palette;
    u16 charOffset;
} Guru2NameWindowData;

// A bonus area of the disc, in degrees
typedef struct {
    u16 start;
    u16 end;
} Guru2BonusArea;

typedef struct {
    int count;
    const Guru2BonusArea *areas;
} Guru2BonusAreaList;

static void Guru2Main_ClearReceived(Guru2MainWork *g2m);
static int Guru2MainSeq_Init(Guru2MainWork *g2m);
static int Guru2MainSeq_FadeInWait(Guru2MainWork *g2m);
static int Guru2MainSeq_ParentWaitChildren(Guru2MainWork *g2m);
static int Guru2MainSeq_ParentSendJoinSignal(Guru2MainWork *g2m);
static UnionApp *Guru2Main_GetUnionApp(Guru2MainWork *g2m);
static int Guru2MainSeq_ParentWaitJoin(Guru2MainWork *g2m);
static int Guru2MainSeq_ParentSendPlayerCount(Guru2MainWork *g2m);
static int Guru2MainSeq_ParentSendPositions(Guru2MainWork *g2m);
static int Guru2MainSeq_ParentSendStartSignal(Guru2MainWork *g2m);
static int Guru2MainSeq_ChildSendJoin(Guru2MainWork *g2m);
static int Guru2MainSeq_ChildWaitStartSignal(Guru2MainWork *g2m);
static int Guru2MainSeq_SyncStart(Guru2MainWork *g2m);
static int Guru2MainSeq_SyncStartWait(Guru2MainWork *g2m);
static int Guru2MainSeq_SendPartyIndex(Guru2MainWork *g2m);
static int Guru2MainSeq_SendParty(Guru2MainWork *g2m);
static int Guru2MainSeq_WaitParties(Guru2MainWork *g2m);
static int Guru2MainSeq_WaitEggCheck(Guru2MainWork *g2m);
static int Guru2MainSeq_GameInit(Guru2MainWork *g2m);
static int Guru2MainSeq_EggFall(Guru2MainWork *g2m);
static int Guru2MainSeq_EggFallWait(Guru2MainWork *g2m);
static int Guru2MainSeq_CountdownBefore(Guru2MainWork *g2m);
static int Guru2MainSeq_ParentSendCountdownSignal(Guru2MainWork *g2m);
static void Guru2Main_AddMetTrainers(Guru2MainWork *g2m);
static int Guru2MainSeq_ChildWaitCountdownSignal(Guru2MainWork *g2m);
static int Guru2MainSeq_SyncCountdown(Guru2MainWork *g2m);
static int Guru2MainSeq_SyncCountdownWait(Guru2MainWork *g2m);
static int Guru2MainSeq_CountdownStart(Guru2MainWork *g2m);
static int Guru2MainSeq_CountdownWait(Guru2MainWork *g2m);
static int Guru2MainSeq_GameStart(Guru2MainWork *g2m);
static int Guru2MainSeq_Game(Guru2MainWork *g2m);
static int Guru2MainSeq_GameEnd(Guru2MainWork *g2m);
static int Guru2MainSeq_ParentAlignOwn(Guru2MainWork *g2m);
static int Guru2MainSeq_ParentAlign(Guru2MainWork *g2m);
static int Guru2MainSeq_SyncResult(Guru2MainWork *g2m);
static int Guru2MainSeq_SyncResultWait(Guru2MainWork *g2m);
static int Guru2MainSeq_ParentSendResult(Guru2MainWork *g2m);
static int Guru2MainSeq_ChildWaitResult(Guru2MainWork *g2m);
static int Guru2MainSeq_ResultNames(Guru2MainWork *g2m);
static int Guru2MainSeq_ResultNamesWait(Guru2MainWork *g2m);
static int Guru2MainSeq_ResultMessageWait(Guru2MainWork *g2m);
static int Guru2MainSeq_Bonus(Guru2MainWork *g2m);
static int Guru2MainSeq_BonusWait(Guru2MainWork *g2m);
static int Guru2MainSeq_BagFullBefore(Guru2MainWork *g2m);
static int Guru2MainSeq_BagFullWait(Guru2MainWork *g2m);
static int Guru2MainSeq_SyncSave(Guru2MainWork *g2m);
static int Guru2MainSeq_SyncSaveWait(Guru2MainWork *g2m);
static int Guru2MainSeq_Save(Guru2MainWork *g2m);
static int Guru2MainSeq_BadEgg(Guru2MainWork *g2m);
static int Guru2MainSeq_BadEggWait(Guru2MainWork *g2m);
static int Guru2MainSeq_SyncEnd(Guru2MainWork *g2m);
static int Guru2MainSeq_SyncEndWait(Guru2MainWork *g2m);
static int Guru2MainSeq_EndWait(Guru2MainWork *g2m);
static int Guru2MainSeq_FadeOut(Guru2MainWork *g2m);
static int Guru2MainSeq_FadeOutWait(Guru2MainWork *g2m);
static int Guru2MainSeq_End(Guru2MainWork *g2m);
static void Guru2Main_VBlank(TCB *tcb, void *data);
static void Guru2Main_InitGraphics(Guru2MainWork *g2m);
static void Guru2Main_FreeGraphics(Guru2MainWork *g2m);
static void Guru2Main_Draw(Guru2MainWork *g2m);
static void Guru2Main_InitVRAM(Guru2MainWork *g2m);
static void Guru2Main_ShowBGs(Guru2MainWork *g2m);
static void Guru2Main_HideBGs(Guru2MainWork *g2m);
static void Guru2Main_Setup3D(void);
static void Guru2Main_Init3D(Guru2MainWork *g2m);
static void Guru2Main_InitBGs(Guru2MainWork *g2m);
static void Guru2Main_LoadBGs(Guru2MainWork *g2m);
static void Guru2Main_FreeBGs(Guru2MainWork *g2m);
static void Guru2Main_LoadButtonScreen(Guru2MainWork *g2m, u32 fileId);
static void Guru2Main_ShowButtonHalf(Guru2MainWork *g2m);
static void Guru2Main_ShowButton(Guru2MainWork *g2m);
static void Guru2Main_ShowButtonPushed(Guru2MainWork *g2m);
static void Guru2Main_InitFade(Guru2MainWork *g2m);
static void Guru2Main_FreeFade(Guru2MainWork *g2m);
static void Guru2Main_SetSubFade(Guru2MainWork *g2m, int evy);
static void Guru2Main_InitClAct(Guru2MainWork *g2m);
static void Guru2Main_LoadClActResources(Guru2MainWork *g2m);
static void Guru2Main_FreeClAct(Guru2MainWork *g2m);
static void Guru2Main_InitMsg(Guru2MainWork *g2m);
static void Guru2Main_FreeMsg(Guru2MainWork *g2m);
static void Guru2Main_PrintMsg(Guru2MainWork *g2m, u32 messageId);
static void Guru2Main_PrintEggMsg(Guru2MainWork *g2m, u32 messageId, UnionAppMember *member);
static void Guru2Main_PrintItemMsg(Guru2MainWork *g2m, u32 messageId, u32 item);
static void Guru2Main_ClearMsg(Guru2MainWork *g2m);
static void Guru2Main_CreateNameWindows(Guru2MainWork *g2m, int count);
static void Guru2Main_FreeNameWindows(Guru2MainWork *g2m);
static void Guru2Main_PrintNameColor(Guru2MainWork *g2m, StrBuf *name, int index, u32 color);
static void Guru2Main_PrintName(Guru2MainWork *g2m, StrBuf *name, int index, int netId);
static void Guru2Main_ClearName(Guru2MainWork *g2m, int index);
static void Guru2Main_FreeCamera(Guru2MainWork *g2m);
static BOOL Guru2Main_IsButtonTouched(Guru2MainWork *g2m);
static void Guru2Main_InitDisc(Guru2MainWork *g2m);
static void Guru2Main_FreeDisc(Guru2MainWork *g2m);
static void Guru2Main_UpdateDisc(Guru2MainWork *g2m);
static void Guru2Main_DrawDisc(Guru2MainWork *g2m);
static void Guru2Disc_Turn(Guru2Disc *disc, fx32 angle);
static void Guru2Main_InitBG3D(Guru2MainWork *g2m);
static void Guru2Main_FreeBG3D(Guru2MainWork *g2m);
static void Guru2Main_DrawBG3D(Guru2MainWork *g2m);
static void Guru2Main_InitObjects(Guru2MainWork *g2m);
static void Guru2Main_FreeObjects(Guru2MainWork *g2m);
static void Guru2Main_ShowEgg(Guru2MainWork *g2m, Guru2Egg *egg);
static void Guru2Main_UpdateObjects(Guru2MainWork *g2m);
static void Guru2Main_DrawObjects(Guru2MainWork *g2m);
static void Guru2Main_TurnEggs(Guru2MainWork *g2m, fx32 angle);
static void Guru2Egg_Update(Guru2Egg *egg, const VecFx32 *offset, fx32 angle, fx32 jumpY);
static Guru2Egg *Guru2Main_FindFrontEgg(Guru2MainWork *g2m);
static void Guru2Main_AddEggShadow(Guru2MainWork *g2m, Guru2Egg *egg);
static void Guru2Main_UpdateEggShadow(Guru2MainWork *g2m, Guru2EggShadow *shadow, fx32 jumpY);
static void Guru2Main_AddEggCursor(Guru2MainWork *g2m, Guru2Egg *egg);
static void Guru2Main_UpdateEggCursor(Guru2MainWork *g2m, Guru2EggCursor *cursor, int index);
static void Guru2Main_AddEgg(Guru2MainWork *g2m, int netId, int position, int index, int angle, int angleOffset,
                             Guru2Egg *egg);
static void Guru2EggFallTask_Main(TCB *tcb, void *data);
static int Guru2EggFallTask_Init(Guru2EggFallTask *task);
static int Guru2EggFallTask_Fall(Guru2EggFallTask *task);
static int Guru2EggFallTask_Land(Guru2EggFallTask *task);
static void Guru2EggJumpTask_Update(Guru2MainWork *g2m, Guru2EggJumpTask *task);
static void Guru2EggJumpTask_Main(TCB *tcb, void *data);
static void Guru2Main_InitEggJump(Guru2MainWork *g2m);
static void Guru2Main_FreeEggJump(Guru2MainWork *g2m);
static void Guru2Main_StartEggJump(Guru2MainWork *g2m, Guru2Egg *egg);
static void Guru2Main_StartEggJumps(Guru2MainWork *g2m);
static BOOL Guru2Main_IsEggJumping(Guru2MainWork *g2m);
static void Guru2EggSpinTask_Update(Guru2EggSpinTask *task);
static void Guru2EggSpinTask_Main(TCB *tcb, void *data);
static void Guru2Main_InitEggSpin(Guru2MainWork *g2m);
static void Guru2Main_FreeEggSpin(Guru2MainWork *g2m);
static void Guru2Main_StartEggSpin(Guru2MainWork *g2m, Guru2Egg *egg);
static void Guru2EggShakeTask_Update(Guru2EggShakeTask *task);
static void Guru2EggShakeTask_Main(TCB *tcb, void *data);
static void Guru2Main_InitEggShake(Guru2MainWork *g2m);
static void Guru2Main_FreeEggShake(Guru2MainWork *g2m);
static void Guru2Main_StartEggShake(Guru2MainWork *g2m, Guru2Egg *egg);
static void Guru2DiscBumpTask_Update(Guru2MainWork *g2m, Guru2DiscBumpTask *task);
static void Guru2DiscBumpTask_Main(TCB *tcb, void *data);
static void Guru2Main_InitDiscBump(Guru2MainWork *g2m);
static void Guru2Main_FreeDiscBump(Guru2MainWork *g2m);
static void Guru2Main_StartDiscBump(Guru2MainWork *g2m);
static BOOL Guru2Main_AreEggsJumping(Guru2MainWork *g2m);
static BOOL Guru2Main_IsDiscBumping(Guru2MainWork *g2m);
static void Guru2CountdownTask_Main(TCB *tcb, void *data);
static void Guru2Main_StartCountdown(Guru2MainWork *g2m);
static void Guru2ButtonTask_Update(Guru2MainWork *g2m, Guru2ButtonTask *task);
static void Guru2ButtonTask_Main(TCB *tcb, void *data);
static void Guru2Main_InitButton(Guru2MainWork *g2m);
static void Guru2Main_FreeButton(Guru2MainWork *g2m);
static u32 Guru2Main_IsButtonPushed(Guru2MainWork *g2m);
static void Guru2Main_PushButton(Guru2MainWork *g2m);
static void Guru2BonusTask_Update(Guru2MainWork *g2m, Guru2BonusTask *task);
static void Guru2BonusTask_Main(TCB *tcb, void *data);
static void Guru2Main_InitBonus(Guru2MainWork *g2m);
static void Guru2Main_FreeBonus(Guru2MainWork *g2m);
static void Guru2Main_StartBonus(Guru2MainWork *g2m, Guru2Egg *egg);
static void Guru2Main_StartBonuses(Guru2MainWork *g2m);
static void Guru2FadeTask_Main(TCB *tcb, void *data);
static void Guru2Main_StartSubFade(Guru2MainWork *g2m, int out);
static void Guru2Main_AddAngle(fx32 *angle, fx32 addend);
static void Guru2Main_Turn(Guru2MainWork *g2m, fx32 angle);
static BOOL Guru2Main_CountGameFrame(Guru2MainWork *g2m);
static void *Guru2Main_LoadFile(Guru2MainWork *g2m, u32 fileId, BOOL head);
static void *Guru2Main_InitSeqWork(Guru2MainWork *g2m, u32 size);
static void *Guru2Main_GetSeqWork(Guru2MainWork *g2m);
static BOOL Guru2Main_HasSignal(Guru2MainWork *g2m, u32 signal);
static BOOL Guru2Main_HaveChildrenJoined(Guru2MainWork *g2m);
static int Guru2Main_CountChildren(Guru2MainWork *g2m);
static BOOL Guru2Main_IsMember(Guru2MainWork *g2m, int netId);
static void Guru2Main_TradeEgg(Guru2MainWork *g2m, int netId);
static int Guru2Main_CountParties(Guru2MainWork *g2m);
static BOOL Guru2Main_IsBonusAngle(Guru2MainWork *g2m, fx32 angle, int playerCount);
static BOOL Guru2Main_PartyHasBadEgg(PokeParty *party);
static BOOL Guru2Main_AnyPartyHasBadEgg(Guru2MainWork *g2m);
static int Guru2Main_CountEggChecks(Guru2MainWork *g2m);
static BOOL Guru2Main_IsBadEggFound(Guru2MainWork *g2m);
static void Guru2Main_PlayME(u32 me);
static BOOL Guru2Main_IsMEPlaying(void);
static void Guru2Main_ParentTurn(Guru2MainWork *g2m, fx32 angle);

static const BGSysVRAMConfig sGuru2VRAMConfig = {
    GX_VRAM_BG_128_B,      GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,       GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_E | GX_VRAM_F, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_16_I,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_0_A,       GX_VRAM_TEXPLTT_0_G,     GX_OBJVRAMMODE_CHAR_1D_64K, GX_OBJVRAMMODE_CHAR_1D_32K,
};

// How much each of the Eggs' hops on a bonus area lifts it
static int sBonusHopHeights[] = { 4, 6, 8, 10, 11, 12, 12, 12, 11, 10, 9, 8, 6, 4, 0, 0 };

static const Guru2NameWindowData sNameWindows5[] = {
    { 1, 12, 19, 8, 2, 15, 0x94 }, { 1, 4, 14, 8, 2, 15, 0xbc },   { 1, 6, 9, 8, 2, 15, 0xe4 },
    { 1, 18, 9, 8, 2, 15, 0x10c }, { 1, 20, 14, 8, 2, 15, 0x134 },
};

static const TouchRect sGuru2ButtonRects[] = {
    { TOUCH_RECT_CIRCLE, 128, 96, 64 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The angle that turns each position to the front, by player count
static const u16 sPositionAngles[GURU2_MEMBER_MAX + 1][GURU2_MEMBER_MAX] = {
    { 0, 0, 0, 0, 0 },     { 0, 0, 0, 0, 0 },      { 0, 180, 0, 0, 0 },
    { 0, 240, 120, 0, 0 }, { 0, 270, 180, 90, 0 }, { 0, 288, 216, 144, 72 },
};

// What turns each disc's model to match its positions, by player count
static const fx32 sDiscAngleOffsets[GURU2_MEMBER_MAX + 1] = {
    0, 0, FX32_CONST(-72), 0, FX32_CONST(-181), 0,
};

static const G3DSceneResourceSetup sEggResources[] = {
    { ARCID_GURU2, NARC_GURU2_EGG_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sDisc2Resources[] = {
    { ARCID_GURU2, NARC_GURU2_DISC_2_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const GXRgb sEdgeColors[8] = {
    GX_RGB(4, 4, 4), GX_RGB(4, 4, 4), GX_RGB(4, 4, 4), GX_RGB(4, 4, 4),
    GX_RGB(4, 4, 4), GX_RGB(4, 4, 4), GX_RGB(4, 4, 4), GX_RGB(4, 4, 4),
};

static const Guru2NameWindowData sNameWindows4[] = {
    { 1, 12, 18, 8, 2, 15, 0x94 },
    { 1, 4, 13, 8, 2, 15, 0xbc },
    { 1, 12, 6, 8, 2, 15, 0xe4 },
    { 1, 20, 13, 8, 2, 15, 0x10c },
};

static const VecFx32 sGuru2CameraTarget = { 0, 0, 0 };

static const G3DSceneActorSetup sDiscActors[] = {
    { 0, 0, 0, 0, NULL, 0 },
};

// The first and last of the Berries a bonus area gives, by player count
static const u32 sBonusItems[GURU2_MEMBER_MAX + 1][2] = {
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
static const u16 sEggAngles[GURU2_MEMBER_MAX + 1][GURU2_MEMBER_MAX] = {
    { 0, 0, 0, 0, 0 },      { 0, 0, 0, 0, 0 },      { 90, 270, 0, 0, 0 },
    { 90, 210, 330, 0, 0 }, { 90, 180, 270, 0, 0 }, { 90, 162, 234, 306, 18 },
};

static const G3DSceneResourceSetup sDisc4Resources[] = {
    { ARCID_GURU2, NARC_GURU2_DISC_4_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sCursor3Resources[] = {
    { ARCID_GURU2, NARC_GURU2_CURSOR_3_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_GURU2, NARC_GURU2_CURSOR_3_ANM, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sDisc5Resources[] = {
    { ARCID_GURU2, NARC_GURU2_DISC_5_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const Guru2NameWindowData sNameWindows3[] = {
    { 1, 12, 18, 8, 2, 15, 0x94 },
    { 1, 5, 10, 8, 2, 15, 0xbc },
    { 1, 19, 10, 8, 2, 15, 0xe4 },
};

static const G3DSceneResourceSetup sShadowResources[] = {
    { ARCID_GURU2, NARC_GURU2_SHADOW_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sDisc3Resources[] = {
    { ARCID_GURU2, NARC_GURU2_DISC_3_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const VecFx32 sGuru2CameraPosition = { 0, FX32_CONST(48), FX32_CONST(112) };

static const Guru2NameWindowData sNameWindows2[] = {
    { 1, 12, 18, 8, 2, 15, 0x94 },
    { 1, 12, 6, 8, 2, 15, 0xbc },
};

static const G3DSceneActorSetup sEggActors[] = {
    { 0, 0, 0, 0, NULL, 0 },
};

static const G3DSceneResourceSetup sBGResources[] = {
    { ARCID_GURU2, NARC_GURU2_BG_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneResourceSetup sCursor0Resources[] = {
    { ARCID_GURU2, NARC_GURU2_CURSOR_0_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_GURU2, NARC_GURU2_CURSOR_0_ANM, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneAnimationSetup sCursorAnimations[] = {
    { 1, 0 },
};

static const G3DSceneResourceSetup sCursor1Resources[] = {
    { ARCID_GURU2, NARC_GURU2_CURSOR_1_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_GURU2, NARC_GURU2_CURSOR_1_ANM, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneActorSetup sShadowActors[] = {
    { 0, 0, 0, 0, NULL, 0 },
};

static const G3DSceneResourceSetup sCursor4Resources[] = {
    { ARCID_GURU2, NARC_GURU2_CURSOR_4_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_GURU2, NARC_GURU2_CURSOR_4_ANM, 0, G3D_SCENE_RES_ARCSYS },
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
    { ARCID_GURU2, NARC_GURU2_CURSOR_2_NSBMD, 0, G3D_SCENE_RES_ARCSYS },
    { ARCID_GURU2, NARC_GURU2_CURSOR_2_ANM, 0, G3D_SCENE_RES_ARCSYS },
};

static const G3DSceneSetup sBGScene = { sBGResources, NELEMS(sBGResources), sBGActors, NELEMS(sBGActors) };

static const Guru2EggFallFunc sEggFallFuncs[] = {
    Guru2EggFallTask_Init,
    Guru2EggFallTask_Fall,
    Guru2EggFallTask_Land,
};

static const Guru2BonusArea sBonusAreas2[] = {
    { 0, 180 },
};

static const Guru2NameWindowData *const sNameWindows[GURU2_MEMBER_MAX + 1] = {
    NULL, NULL, sNameWindows2, sNameWindows3, sNameWindows4, sNameWindows5,
};

static const Guru2MainSeqFunc sGuru2MainSeqFuncs[] = {
    Guru2MainSeq_Init,
    Guru2MainSeq_FadeInWait,
    Guru2MainSeq_ParentWaitChildren,
    Guru2MainSeq_ParentSendJoinSignal,
    Guru2MainSeq_ParentWaitJoin,
    Guru2MainSeq_ParentSendPlayerCount,
    Guru2MainSeq_ParentSendPositions,
    Guru2MainSeq_ParentSendStartSignal,
    Guru2MainSeq_ChildSendJoin,
    Guru2MainSeq_ChildWaitStartSignal,
    Guru2MainSeq_SyncStart,
    Guru2MainSeq_SyncStartWait,
    Guru2MainSeq_SendPartyIndex,
    Guru2MainSeq_SendParty,
    Guru2MainSeq_WaitParties,
    Guru2MainSeq_WaitEggCheck,
    Guru2MainSeq_GameInit,
    Guru2MainSeq_EggFall,
    Guru2MainSeq_EggFallWait,
    Guru2MainSeq_CountdownBefore,
    Guru2MainSeq_ParentSendCountdownSignal,
    Guru2MainSeq_ChildWaitCountdownSignal,
    Guru2MainSeq_SyncCountdown,
    Guru2MainSeq_SyncCountdownWait,
    Guru2MainSeq_CountdownStart,
    Guru2MainSeq_CountdownWait,
    Guru2MainSeq_GameStart,
    Guru2MainSeq_Game,
    Guru2MainSeq_GameEnd,
    Guru2MainSeq_ParentAlignOwn,
    Guru2MainSeq_ParentAlign,
    Guru2MainSeq_SyncResult,
    Guru2MainSeq_SyncResultWait,
    Guru2MainSeq_ParentSendResult,
    Guru2MainSeq_ChildWaitResult,
    Guru2MainSeq_ResultNames,
    Guru2MainSeq_ResultNamesWait,
    Guru2MainSeq_ResultMessageWait,
    Guru2MainSeq_Bonus,
    Guru2MainSeq_BonusWait,
    Guru2MainSeq_BagFullBefore,
    Guru2MainSeq_BagFullWait,
    Guru2MainSeq_SyncSave,
    Guru2MainSeq_SyncSaveWait,
    Guru2MainSeq_Save,
    Guru2MainSeq_BadEgg,
    Guru2MainSeq_BadEggWait,
    Guru2MainSeq_SyncEnd,
    Guru2MainSeq_SyncEndWait,
    Guru2MainSeq_EndWait,
    Guru2MainSeq_FadeOut,
    Guru2MainSeq_FadeOutWait,
    Guru2MainSeq_End,
};

BOOL Guru2Main_Init(GameProc *proc, u32 *state, void *param, void *work) {
    Guru2ProcWork *g2p = param;
    int i;
    Guru2MainWork *g2m;

    func_02042e94(1);
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_GURU2, 0x80000);
    g2m = GFL_ProcInitSubsystem(proc, sizeof(Guru2MainWork), HEAPID_GURU2);
    i = 0;
    sys_memset(g2m, 0, sizeof(Guru2MainWork));
    g2p->g2m = g2m;
    g2m->g2p = g2p;
    g2m->g2c = g2p->g2c;
    g2m->party = GameData_GetParty(g2m->g2p->param->gameData);
    g2m->arc = GFL_ArcSysCreateFileHandle(ARCID_GURU2, HEAPID_GURU2);
    Guru2Main_InitGraphics(g2m);
    g2m->vblankTCB = GFL_VBlankTCBAdd(Guru2Main_VBlank, g2m, 1);
    g2m->g3dManager = GFL_G3DMgrCreate(20, 11, HEAPID_GURU2);
    Guru2Main_InitBG3D(g2m);
    Guru2Main_InitObjects(g2m);
    Guru2Main_InitDisc(g2m);

    // Until the parent sends the positions, the members are in net ID order
    {
        int position = 0;
        int netId = func_0203ffc4();
        Guru2Disc *disc = &g2m->disc;

        for (; i < GURU2_MEMBER_MAX; i++) {
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
        GFL_HeapAllocate(HEAPID_TAIL(HEAPID_GURU2), GFL_TCBMgrCalcAllocSize(32), FALSE, "guru2_main.c", 1260);
    g2m->tcbManager = GFL_TCBMgrCreate(32, g2m->tcbBuffer);
    Guru2Main_InitEggJump(g2m);
    Guru2Main_InitEggSpin(g2m);
    Guru2Main_InitEggShake(g2m);
    Guru2Main_InitDiscBump(g2m);
    Guru2Main_InitButton(g2m);
    Guru2Main_InitBonus(g2m);
    GFL_WipeSet(0, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 8, 1, HEAPID_GURU2);
    func_02042e9c(TRUE);
    return TRUE;
}

BOOL Guru2Main_Exit(GameProc *proc, u32 *state, void *param, void *work) {
    Guru2MainWork *g2m = work;

    func_02021c44(g2m->msg.queue);
    func_02021a18(g2m->msg.queue);
    if (g2m->waitIcon != NULL) {
        WaitIcon_Free(g2m->waitIcon);
    }
    Guru2Main_FreeEggJump(g2m);
    Guru2Main_FreeEggSpin(g2m);
    Guru2Main_FreeEggShake(g2m);
    Guru2Main_FreeDiscBump(g2m);
    Guru2Main_FreeButton(g2m);
    Guru2Main_FreeBonus(g2m);
    func_0203a610(g2m->tcbManager);
    GFL_HeapFree(g2m->tcbBuffer);
    Guru2Main_FreeBG3D(g2m);
    Guru2Main_FreeDisc(g2m);
    Guru2Main_FreeObjects(g2m);
    GFL_G3DMgrFree(g2m->g3dManager);
    GFL_G3DSysFree();
    Guru2Main_FreeGraphics(g2m);
    GFL_TCBRemove(g2m->vblankTCB);
    GFL_ArcToolFree(g2m->arc);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_GURU2);
    GCTX_HIDUnblockSoftReset(8);
    return TRUE;
}

BOOL Guru2Main_Main(GameProc *proc, u32 *state, void *param, void *work) {
    Guru2MainWork *g2m = work;
    int result;

    do {
        result = sGuru2MainSeqFuncs[g2m->seq](g2m);
    } while (result == SEQ_RESULT_CONTINUE);
    if (result == SEQ_RESULT_END) {
        return TRUE;
    }

    Guru2Main_UpdateDisc(g2m);
    Guru2Main_UpdateObjects(g2m);
    Guru2Main_Draw(g2m);
    Guru2Main_ClearReceived(g2m);
    GFL_TCBMgrUpdate(g2m->tcbManager);
    if (GFL_WipeIsFinished() && GFL_NetErrCheck()) {
        return TRUE;
    }
    return FALSE;
}

static void Guru2Main_ClearReceived(Guru2MainWork *g2m) {
    g2m->buttonReceived = 0;
    g2m->gameDataReceived = FALSE;
}

static int Guru2MainSeq_Init(Guru2MainWork *g2m) {
    Guru2Main_PrintMsg(g2m, AbilityHandlers_Text_PleaseStandBy);
    g2m->seq = SEQ_FADE_IN_WAIT;
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_FadeInWait(Guru2MainWork *g2m) {
    if (GFL_WipeIsFinished()) {
        if (func_0203ffc4() == 0) {
            g2m->seq = SEQ_PARENT_WAIT_CHILDREN;
        } else {
            g2m->seq = SEQ_CHILD_SEND_JOIN;
        }
        g2m->waitIcon = WaitIcon_Create(g2m->tcbManager, g2m->msg.msgWindow, 15, 16, HEAPID_GURU2);
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_ParentWaitChildren(Guru2MainWork *g2m) {
    if (Guru2Main_HaveChildrenJoined(g2m)) {
        g2m->seq = SEQ_PARENT_SEND_JOIN_SIGNAL;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_ParentSendJoinSignal(Guru2MainWork *g2m) {
    u16 signal = GURU2_MAIN_SIGNAL_JOIN;

    if (Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_5, &signal, sizeof(signal)) == TRUE) {
        g2m->seq = SEQ_PARENT_WAIT_JOIN;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static UnionApp *Guru2Main_GetUnionApp(Guru2MainWork *g2m) {
    return g2m->g2p->param->app;
}

static int Guru2MainSeq_ParentWaitJoin(Guru2MainWork *g2m) {
    int count = Guru2Main_CountChildren(g2m) + 1;

    if (count != UnionApp_CountJoined(Guru2Main_GetUnionApp(g2m))) {
        return SEQ_RESULT_WAIT;
    }
    g2m->seq = SEQ_PARENT_SEND_PLAYER_COUNT;
    return SEQ_RESULT_CONTINUE;
}

static int Guru2MainSeq_ParentSendPlayerCount(Guru2MainWork *g2m) {
    g2m->playerCount = Guru2Main_CountChildren(g2m) + 1;
    if (Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_10, &g2m->playerCount, sizeof(g2m->playerCount)) == TRUE) {
        g2m->sendCount = 0;
        g2m->seq = SEQ_PARENT_SEND_POSITIONS;
    }
    return SEQ_RESULT_WAIT;
}

// Sends a member's position a frame, the parent's first
static int Guru2MainSeq_ParentSendPositions(Guru2MainWork *g2m) {
    Guru2PositionData data;
    int i;
    int count;

    if (g2m->sendCount == 0) {
        data.netId = 0;
        data.position = 0;
        if (Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_9, &data, sizeof(data)) == TRUE) {
            g2m->sendCount++;
        }
        return SEQ_RESULT_WAIT;
    }

    for (i = 1, count = 1; i < GURU2_MEMBER_MAX; i++) {
        if (g2m->g2c->unk42 & (1 << i)) {
            if (count >= g2m->sendCount) {
                data.netId = i;
                data.position = g2m->sendCount;
                if (Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_9, &data, sizeof(data)) == TRUE) {
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

static int Guru2MainSeq_ParentSendStartSignal(Guru2MainWork *g2m) {
    u16 signal = GURU2_MAIN_SIGNAL_START;

    if (Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_5, &signal, sizeof(signal)) == TRUE) {
        g2m->seq = SEQ_SYNC_START;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_ChildSendJoin(Guru2MainWork *g2m) {
    if (Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_6, NULL, 0) == TRUE) {
        g2m->seq = SEQ_CHILD_WAIT_START_SIGNAL;
        g2m->unk10 = 0;
        return SEQ_RESULT_WAIT;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_ChildWaitStartSignal(Guru2MainWork *g2m) {
    if (Guru2Main_HasSignal(g2m, GURU2_MAIN_SIGNAL_START) == TRUE) {
        g2m->playerCount = Guru2Main_CountChildren(g2m) + 1;
        g2m->seq = SEQ_SYNC_START;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_SyncStart(Guru2MainWork *g2m) {
    NetHandle *handle = func_02040440();

    WaitIcon_Free(g2m->waitIcon);
    g2m->waitIcon = NULL;
    func_02040624(handle, GURU2_SYNC_START, GURU2_SYNC_CLASS);
    g2m->seq = SEQ_SYNC_START_WAIT;
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_SyncStartWait(Guru2MainWork *g2m) {
    if (func_02040664(func_02040440(), GURU2_SYNC_START, GURU2_SYNC_CLASS)) {
        g2m->seq = SEQ_SEND_PARTY_INDEX;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_SendPartyIndex(Guru2MainWork *g2m) {
    if (Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_11, &g2m->g2p->partyIndex, sizeof(g2m->g2p->partyIndex))) {
        g2m->seq = SEQ_SEND_PARTY;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_SendParty(Guru2MainWork *g2m) {
    NetHandle *handle = func_02040440();
    u32 joined = UnionApp_GetJoined(Guru2Main_GetUnionApp(g2m));

    if (func_02042c9c(handle, (u8)joined, GURU2_NET_CMD_BLOCK, GURU2_COMM_BLOCK_SIZE, g2m->party, TRUE, TRUE, TRUE)) {
        g2m->seq = SEQ_WAIT_PARTIES;
    }
    return SEQ_RESULT_WAIT;
}

// Once every party has arrived, tells the others whether one of them has a bad Egg
static int Guru2MainSeq_WaitParties(Guru2MainWork *g2m) {
    if (g2m->playerCount == Guru2Main_CountParties(g2m)) {
        if (Guru2Main_AnyPartyHasBadEgg(g2m) == TRUE) {
            Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_13, NULL, 0);
            g2m->seq = SEQ_BAD_EGG;
        } else {
            Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_14, NULL, 0);
            g2m->seq = SEQ_WAIT_EGG_CHECK;
        }
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_WaitEggCheck(Guru2MainWork *g2m) {
    if (Guru2Main_IsBadEggFound(g2m) == TRUE) {
        g2m->seq = SEQ_BAD_EGG;
    } else if (g2m->playerCount == Guru2Main_CountEggChecks(g2m)) {
        g2m->seq = SEQ_GAME_INIT;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_GameInit(Guru2MainWork *g2m) {
    Guru2EggFallSeqWork *seqWork = Guru2Main_InitSeqWork(g2m, sizeof(Guru2EggFallSeqWork));
    int i;

    seqWork->position = g2m->position;
    seqWork->playerCount = g2m->playerCount;
    seqWork->angle = sPositionAngles[seqWork->playerCount][g2m->position];
    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        if (Guru2Main_IsMember(g2m, i)) {
            g2m->members[i] = UnionApp_GetMember(Guru2Main_GetUnionApp(g2m), i);
            textCopy(g2m->members[i]->info.name, g2m->names[i]);
        }
    }
    Guru2Main_AddMetTrainers(g2m);
    g2m->disc.baseAngle = seqWork->angle << FX32_SHIFT;
    g2m->seq = SEQ_EGG_FALL;
    GFL_SndBGMPlay(SEQ_BGM_GURUGURU, 0xffff);
    g2m->g2p->param->started = TRUE;
    Guru2Main_ClearMsg(g2m);
    return SEQ_RESULT_CONTINUE;
}

// Drops an Egg every 15 frames, the player's own first
static int Guru2MainSeq_EggFall(Guru2MainWork *g2m) {
    Guru2EggFallSeqWork *seqWork;
    int angle;
    int netId;

    g2m->wait--;
    if (g2m->wait > 0) {
        return SEQ_RESULT_WAIT;
    }

    g2m->wait = 15;
    seqWork = Guru2Main_GetSeqWork(g2m);
    angle = sEggAngles[seqWork->playerCount][seqWork->position];
    netId = g2m->positions[seqWork->position].netId;
    g2m->gameData.eggAngles[seqWork->position] = angle;
    Guru2Main_AddEgg(g2m, netId, seqWork->position, seqWork->count, angle, seqWork->angle,
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

static int Guru2MainSeq_EggFallWait(Guru2MainWork *g2m) {
    Guru2Objects *objects = &g2m->objects;
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

static int Guru2MainSeq_CountdownBefore(Guru2MainWork *g2m) {
    g2m->wait++;
    if (g2m->wait > 30) {
        g2m->wait = 0;
        if (func_0203ffc4() == 0) {
            g2m->seq = SEQ_PARENT_SEND_COUNTDOWN_SIGNAL;
        } else {
            g2m->seq = SEQ_CHILD_WAIT_COUNTDOWN_SIGNAL;
        }
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_ParentSendCountdownSignal(Guru2MainWork *g2m) {
    u16 signal = GURU2_MAIN_SIGNAL_COUNTDOWN;

    if (Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_5, &signal, sizeof(signal)) == TRUE) {
        g2m->seq = SEQ_SYNC_COUNTDOWN;
    }
    return SEQ_RESULT_WAIT;
}

// Records the other members as trainers the player has met
static void Guru2Main_AddMetTrainers(Guru2MainWork *g2m) {
    RivalDataSave *rivalData = getHollow_RivalData(GameData_GetSaveControl(g2m->g2p->param->gameData));
    int i;

    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        if (g2m->members[i] != NULL && i != func_0203ffc4()) {
            Misc_AddMetTrainer(rivalData, getIDAsUInt(&g2m->members[i]->info));
        }
    }
}

static int Guru2MainSeq_ChildWaitCountdownSignal(Guru2MainWork *g2m) {
    if (Guru2Main_HasSignal(g2m, GURU2_MAIN_SIGNAL_COUNTDOWN) == TRUE) {
        g2m->seq = SEQ_SYNC_COUNTDOWN;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_SyncCountdown(Guru2MainWork *g2m) {
    func_02040624(func_02040440(), GURU2_SYNC_COUNTDOWN, GURU2_SYNC_CLASS);
    g2m->seq = SEQ_SYNC_COUNTDOWN_WAIT;
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_SyncCountdownWait(Guru2MainWork *g2m) {
    if (func_02040664(func_02040440(), GURU2_SYNC_COUNTDOWN, GURU2_SYNC_CLASS)) {
        g2m->seq = SEQ_COUNTDOWN_START;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_CountdownStart(Guru2MainWork *g2m) {
    Guru2Main_StartSubFade(g2m, TRUE);
    Guru2Main_StartCountdown(g2m);
    g2m->seq = SEQ_COUNTDOWN_WAIT;
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_CountdownWait(Guru2MainWork *g2m) {
    int i;

    g2m->wait++;
    if (g2m->wait < 95) {
        return SEQ_RESULT_WAIT;
    }

    for (i = 0; i < g2m->playerCount; i++) {
        Guru2Main_ClearName(g2m, i);
    }
    g2m->wait = 0;
    g2m->seq = SEQ_GAME_START;
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_GameStart(Guru2MainWork *g2m) {
    func_0203ffc4();
    g2m->seq = SEQ_GAME;
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_Game(Guru2MainWork *g2m) {
    Guru2GameData data;
    int i;
    BOOL timeUp = Guru2Main_CountGameFrame(g2m);
    Guru2Disc *disc;

    // The game ends once the Eggs are down after the time is up
    if (timeUp == TRUE && !Guru2Main_IsDiscBumping(g2m) && g2m->gameData.jumpY == 0) {
        g2m->timeUp = TRUE;
    }
    if (g2m->gameData.timeUp == TRUE) {
        g2m->seq = SEQ_GAME_END;
        return SEQ_RESULT_CONTINUE;
    }

    if (timeUp == FALSE) {
        if (g2m->buttonReceived != 0 && !Guru2Main_IsDiscBumping(g2m)) {
            Guru2Main_StartDiscBump(g2m);
        }
        if (!Guru2Main_IsButtonPushed(g2m) && Guru2Main_IsButtonTouched(g2m) == TRUE) {
            if (!Guru2Main_IsDiscBumping(g2m)) {
                g2m->buttonSend = 1;
            }
            Guru2Main_PushButton(g2m);
        }
        if (g2m->buttonSend != 0) {
            if (Guru2Comm_Send(g2m->g2p->g2c, GURU2_NET_CMD_7, &g2m->buttonSend, 1) == TRUE) {
                g2m->buttonSend = 0;
            }
        }
    }

    // The parent speeds the disc up in the first half and slows it down in the second
    disc = &g2m->disc;
    if (func_0203ffc4() == 0) {
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
    if (func_0203ffc4() == 0) {
        data.discAngle = g2m->disc.angle;
        data.timeUp = g2m->timeUp;
        data.jumpY = g2m->objects.eggs[0].jumpY;
        for (i = 0; i < g2m->playerCount; i++) {
            data.eggAngles[i] = g2m->objects.eggs[i].angle / FX32_ONE;
        }
        Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_8, &data, sizeof(data));
    }
    if (func_0203ffc4() == 0) {
        fx32 speed = g2m->disc.speed;

        // While the Eggs are in the air, the disc turns under them at twice the speed
        if (Guru2Main_AreEggsJumping(g2m) == TRUE) {
            Guru2Disc_Turn(&g2m->disc, speed * 2);
        } else {
            Guru2Main_Turn(g2m, speed);
        }
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_GameEnd(Guru2MainWork *g2m) {
    Guru2Main_StartSubFade(g2m, FALSE);
    g2m->frontEgg = Guru2Main_FindFrontEgg(g2m);
    if (func_0203ffc4() == 0) {
        if (g2m->frontEgg->netId == func_0203ffc4()) {
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
static int Guru2MainSeq_ParentAlignOwn(Guru2MainWork *g2m) {
    Guru2Main_ParentTurn(g2m, FX32_CONST(4));
    g2m->frontEgg = Guru2Main_FindFrontEgg(g2m);
    if (g2m->frontEgg->netId != func_0203ffc4()) {
        g2m->seq = SEQ_PARENT_ALIGN;
    }
    return SEQ_RESULT_WAIT;
}

// Turns the disc until the front Egg is at the parent's position
static int Guru2MainSeq_ParentAlign(Guru2MainWork *g2m) {
    fx32 angle = g2m->frontEgg->angle;
    fx32 speed = FX32_CONST(4);
    fx32 target = sEggAngles[g2m->playerCount][g2m->position] << FX32_SHIFT;
    fx32 distance = target - angle;

    if (distance < 0) {
        distance = target + FULL_TURN - angle;
    }
    if (distance <= FX32_CONST(16)) {
        speed >>= 1;
    }
    if (distance < speed) {
        speed = distance;
    }
    Guru2Main_ParentTurn(g2m, speed);
    Guru2Main_AddAngle(&angle, speed);
    if (angle / FX32_ONE == target / FX32_ONE) {
        g2m->seq = SEQ_SYNC_RESULT;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_SyncResult(Guru2MainWork *g2m) {
    func_02040624(func_02040440(), GURU2_SYNC_RESULT, GURU2_SYNC_CLASS);
    g2m->seq = SEQ_SYNC_RESULT_WAIT;
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_SyncResultWait(Guru2MainWork *g2m) {
    if (func_02040664(func_02040440(), GURU2_SYNC_RESULT, GURU2_SYNC_CLASS)) {
        if (func_0203ffc4() == 0) {
            g2m->seq = SEQ_PARENT_SEND_RESULT;
        } else {
            g2m->seq = SEQ_CHILD_WAIT_RESULT;
        }
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_ParentSendResult(Guru2MainWork *g2m) {
    Guru2ResultData data;
    int i;

    data.discAngle = g2m->gameData.discAngle / FX32_ONE;
    g2m->bonusBits = 0;
    for (i = 0; i < g2m->playerCount; i++) {
        Guru2Egg *egg = &g2m->objects.eggs[i];

        data.eggAngles[egg->position] = g2m->gameData.eggAngles[i];
        if (Guru2Main_IsBonusAngle(g2m, g2m->gameData.eggAngles[i] << FX32_SHIFT, g2m->playerCount) == TRUE) {
            g2m->bonusBits |= 1 << egg->netId;
        }
    }
    data.bonusBits = g2m->bonusBits;
    if (Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_12, &data, sizeof(data)) == TRUE) {
        g2m->seq = SEQ_RESULT_NAMES;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_ChildWaitResult(Guru2MainWork *g2m) {
    if (g2m->resultReceived == TRUE) {
        int i;
        Guru2ResultData *result = &g2m->result;

        g2m->disc.angle = result->discAngle << FX32_SHIFT;
        g2m->gameData.discAngle = result->discAngle << FX32_SHIFT;
        for (i = 0; i < g2m->playerCount; i++) {
            Guru2Egg *egg = &g2m->objects.eggs[i];

            egg->angle = result->eggAngles[egg->position] << FX32_SHIFT;
            g2m->gameData.eggAngles[i] = result->eggAngles[egg->position];
        }
        g2m->bonusBits = result->bonusBits;
        Guru2Main_UpdateDisc(g2m);
        Guru2Main_UpdateObjects(g2m);
        g2m->frontEgg = Guru2Main_FindFrontEgg(g2m);
        g2m->seq = SEQ_RESULT_NAMES;
    }
    return SEQ_RESULT_WAIT;
}

// Shows the names of the Eggs' owners where the Eggs stopped
static int Guru2MainSeq_ResultNames(Guru2MainWork *g2m) {
    int i;
    int position = g2m->frontEgg->position;

    g2m->gameData.jumpY = 0;
    for (i = 0; i < g2m->playerCount; i++) {
        int netId = g2m->objects.eggs[position].netId;

        Guru2Main_PrintName(g2m, g2m->names[netId], i, netId);
        position = (position + 1) % g2m->playerCount;
    }
    g2m->wait = 0;
    g2m->seq = SEQ_RESULT_NAMES_WAIT;
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_ResultNamesWait(Guru2MainWork *g2m) {
    int i;

    g2m->wait++;
    if (g2m->wait < 120) {
        return SEQ_RESULT_WAIT;
    }

    g2m->wait = 0;
    for (i = 0; i < g2m->playerCount; i++) {
        Guru2Main_ClearName(g2m, i);
    }
    Guru2Main_PrintEggMsg(g2m, AbilityHandlers_Text_ReceivedSEgg, g2m->members[g2m->frontEgg->netId]);
    Guru2Main_PlayME(SEQ_ME_ITEM);
    g2m->seq = SEQ_RESULT_MESSAGE_WAIT;
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_ResultMessageWait(Guru2MainWork *g2m) {
    if (g2m->wait < 120) {
        g2m->wait++;
    } else if (!Guru2Main_IsMEPlaying()) {
        g2m->wait = 0;
        g2m->seq = SEQ_BONUS;
    }
    return SEQ_RESULT_WAIT;
}

// Gives the player a Berry if the Egg they got stopped on a bonus area
static int Guru2MainSeq_Bonus(Guru2MainWork *g2m) {
    int count;
    u32 item;
    u32 last;
    BOOL added;

    Guru2Main_StartBonuses(g2m);
    if (!(g2m->bonusBits & (1 << g2m->frontEgg->netId))) {
        g2m->seq = SEQ_SYNC_SAVE;
        return SEQ_RESULT_WAIT;
    }

    count = g2m->playerCount;
    item = sBonusItems[count][0];
    last = sBonusItems[count][1];
    item += GFL_RandomLC(0xffff) % (last - item + 1);
    if (item < sBonusItems[count][0]) {
        item = sBonusItems[count][0];
    } else if (item > last) {
        item = last;
    }
    Guru2Main_PrintItemMsg(g2m, AbilityHandlers_Text_LandedBonusAreaReceived, item);
    added = BagSave_AddItem(GameData_GetBag(g2m->g2p->param->gameData), item, 1, HEAPID_GURU2);
    Guru2Main_PlayME(SEQ_ME_ITEM);
    if (added == TRUE) {
        g2m->seq = SEQ_BONUS_WAIT;
    } else {
        g2m->seq = SEQ_BAG_FULL_BEFORE;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_BonusWait(Guru2MainWork *g2m) {
    if (g2m->wait < 180) {
        g2m->wait++;
    }
    if (!Guru2Main_IsMEPlaying()) {
        g2m->wait = 0;
        g2m->seq = SEQ_SYNC_SAVE;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_BagFullBefore(Guru2MainWork *g2m) {
    if (g2m->wait < 180) {
        g2m->wait++;
    } else if (!Guru2Main_IsMEPlaying()) {
        g2m->wait = 0;
        g2m->seq = SEQ_BAG_FULL_WAIT;
        Guru2Main_PrintMsg(g2m, AbilityHandlers_Text_OhNoBagFull);
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_BagFullWait(Guru2MainWork *g2m) {
    if (g2m->wait < 180) {
        g2m->wait++;
    } else {
        g2m->wait = 0;
        g2m->seq = SEQ_SYNC_SAVE;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_SyncSave(Guru2MainWork *g2m) {
    NetHandle *handle = func_02040440();

    RecordAddOne(g2m->g2p->param->records, 90);
    Guru2Main_PrintMsg(g2m, AbilityHandlers_Text_SavingDontTurnOff);
    func_02040624(handle, GURU2_SYNC_SAVE, GURU2_SYNC_CLASS);
    g2m->seq = SEQ_SYNC_SAVE_WAIT;
    GCTX_HIDBlockSoftReset(8);
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_SyncSaveWait(Guru2MainWork *g2m) {
    if (func_02040664(func_02040440(), GURU2_SYNC_SAVE, GURU2_SYNC_CLASS)) {
        Guru2Main_TradeEgg(g2m, g2m->frontEgg->netId);
        g2m->netSave = func_02012ee4(HEAPID_GURU2, g2m->g2p->param->gameData);
        g2m->waitIcon = WaitIcon_Create(g2m->tcbManager, g2m->msg.msgWindow, 15, 16, HEAPID_GURU2);
        g2m->seq = SEQ_SAVE;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_Save(Guru2MainWork *g2m) {
    BOOL done = func_02012f5c(g2m->netSave);

    if (GFL_NetErrCheck()) {
        func_02012154();
        GFL_NetErrShow(1);
        Wipe_SetScreenCovered(0, WIPE_COLOR_BLACK);
        Wipe_SetScreenCovered(1, WIPE_COLOR_BLACK);
        return SEQ_RESULT_WAIT;
    }
    if (done) {
        WaitIcon_Free(g2m->waitIcon);
        g2m->waitIcon = NULL;
        func_02012f8c(g2m->netSave);
        g2m->seq = SEQ_SYNC_END;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_BadEgg(Guru2MainWork *g2m) {
    Guru2Main_PrintMsg(g2m, AbilityHandlers_Text_ThereEggCantTraded);
    g2m->seq = SEQ_BAD_EGG_WAIT;
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_BadEggWait(Guru2MainWork *g2m) {
    g2m->wait++;
    if (g2m->wait >= 180) {
        g2m->wait = 0;
        g2m->seq = SEQ_SYNC_END;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_SyncEnd(Guru2MainWork *g2m) {
    NetHandle *handle = func_02040440();

    if (!g2m->error) {
        func_02040624(handle, GURU2_SYNC_END, GURU2_SYNC_CLASS);
    }
    Guru2Main_PrintMsg(g2m, AbilityHandlers_Text_CommunicatingPleaseStandBy_2);
    g2m->seq = SEQ_SYNC_END_WAIT;
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_SyncEndWait(Guru2MainWork *g2m) {
    NetHandle *handle = func_02040440();

    if (!g2m->error) {
        if (func_02040664(handle, GURU2_SYNC_END, GURU2_SYNC_CLASS)) {
            g2m->waitIcon = WaitIcon_Create(g2m->tcbManager, g2m->msg.msgWindow, 15, 16, HEAPID_GURU2);
            func_02042e9c(FALSE);
            func_02040c64(GURU2_NET_CMD_BASE);
            UnionApp_RequestStart(Guru2Main_GetUnionApp(g2m));
            g2m->seq = SEQ_END_WAIT;
        }
    } else {
        g2m->wait++;
        if (g2m->wait >= 180) {
            g2m->wait = 0;
            if (g2m->g2c->unk3C == 0) {
                func_02042e94(0);
            }
            g2m->seq = SEQ_END_WAIT;
        }
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_EndWait(Guru2MainWork *g2m) {
    if (UnionApp_IsStarted(Guru2Main_GetUnionApp(g2m))) {
        WaitIcon_Free(g2m->waitIcon);
        g2m->waitIcon = NULL;
        g2m->seq = SEQ_FADE_OUT;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_FadeOut(Guru2MainWork *g2m) {
    GFL_WipeSet(2, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 8, 1, HEAPID_GURU2);
    g2m->seq = SEQ_FADE_OUT_WAIT;
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_FadeOutWait(Guru2MainWork *g2m) {
    if (GFL_WipeIsFinished()) {
        g2m->seq = SEQ_END;
        return SEQ_RESULT_CONTINUE;
    }
    return SEQ_RESULT_WAIT;
}

static int Guru2MainSeq_End(Guru2MainWork *g2m) {
    return SEQ_RESULT_END;
}

static void Guru2Main_VBlank(TCB *tcb, void *data) {
    Guru2MainWork *g2m = data;

    PaletteFade_Transfer(g2m->fade);
    func_0204b7c8();
    GFL_BGSysUpdate();
}

static void Guru2Main_InitGraphics(Guru2MainWork *g2m) {
    Guru2Main_InitVRAM(g2m);
    Guru2Main_Init3D(g2m);
    Guru2Main_InitBGs(g2m);
    Guru2Main_InitFade(g2m);
    Guru2Main_InitClAct(g2m);
    Guru2Main_LoadBGs(g2m);
    Guru2Main_InitMsg(g2m);
    Guru2Main_LoadClActResources(g2m);
    Guru2Main_ShowBGs(g2m);
}

static void Guru2Main_FreeGraphics(Guru2MainWork *g2m) {
    Guru2Main_HideBGs(g2m);
    Guru2Main_FreeClAct(g2m);
    Guru2Main_FreeMsg(g2m);
    Guru2Main_FreeBGs(g2m);
    Guru2Main_FreeFade(g2m);
    Guru2Main_FreeCamera(g2m);
}

static void Guru2Main_Draw(Guru2MainWork *g2m) {
    int i;
    fx32 far = FX32_CONST(4096);

    GFL_G3DCameraSetProjectionZFar(g2m->camera.camera, &far);
    GFL_G3DSysReset();
    GFL_G3DCameraFlush(g2m->camera.camera);
    GFL_G3DSysMtxViewFlush();
    NNS_G3DSetLightVector(0, 0, -FX32_ONE, 0);
    NNS_G3DSetLightColor(0, GX_RGB(31, 31, 31));
    NNS_G3DSetMatDifAmb(GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), FALSE);
    NNS_G3DSetMatSpeEmi(GX_RGB(31, 31, 31), GX_RGB(31, 31, 31), FALSE);
    Guru2Main_DrawBG3D(g2m);
    Guru2Main_DrawDisc(g2m);
    Guru2Main_DrawObjects(g2m);
    GFL_G3DSysReqSwapBuffers();
    func_0204b794();

    func_02021a3c(g2m->msg.queue);
    PrintWindow_Flush(&g2m->msg.msgPrint, g2m->msg.queue);
    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        PrintWindow_Flush(&g2m->msg.namePrints[i], g2m->msg.queue);
    }
    if (g2m->waitIcon != NULL) {
        WaitIcon_Main(g2m->waitIcon);
    }
}

static void Guru2Main_InitVRAM(Guru2MainWork *g2m) {
    GFL_BGSysSetVRAMBanks(&sGuru2VRAMConfig);
    BmpWin_InitAllocator(HEAPID_GURU2);
}

static void Guru2Main_ShowBGs(Guru2MainWork *g2m) {
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
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG2, GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_BD, 11,
                        10);
}

static void Guru2Main_HideBGs(Guru2MainWork *g2m) {
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

static void Guru2Main_Setup3D(void) {
    G3X_SetShading(GX_SHADING_TOON);
    G3X_AntiAlias(TRUE);
    G3X_AlphaTest(FALSE, 0);
    G3X_AlphaBlend(TRUE);
    G3X_EdgeMarking(TRUE);
    gfxSetEdgeColorTable(sEdgeColors);
    gfxClearColor(GX_RGB(31, 31, 31), 0, 0x7fff, 63, FALSE);
    G3_ViewPort(0, 0, 255, 191);
}

static void Guru2Main_Init3D(Guru2MainWork *g2m) {
    Guru2CameraWork *camera = &g2m->camera;

    GFL_G3DSysCreate(FALSE, 1, FALSE, 1, 0, HEAPID_GURU2, Guru2Main_Setup3D);
    GFL_G3DSysSetSwapBufferParams(GX_SORTMODE_AUTO, GX_BUFFERMODE_Z);
    {
        VecFx32 up = { 0, FX32_ONE, 0 };

        camera->camera = GFL_G3DCameraCreate(G3DCAM_PROJECTION_PERSPECTIVE, FX_SinIdx(DEG_TO_IDX(20)),
                                             FX_CosIdx(DEG_TO_IDX(20)), FX32_ONE * 4 / 3, 0, FX32_ONE, FX32_CONST(1024),
                                             0, &sGuru2CameraPosition, &up, &sGuru2CameraTarget, HEAPID_GURU2);
    }
    GFL_G3DCameraFlush(camera->camera);
}

static void Guru2Main_InitBGs(Guru2MainWork *g2m) {
    GFL_BGSysCreate(HEAPID_GURU2);
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
        GFL_BGSysClearCharCore(1, BGSYS_TILE_SIZE_16, 0, HEAPID_GURU2);
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

static void Guru2Main_LoadBGs(Guru2MainWork *g2m) {
    void *file;

    file = Guru2Main_LoadFile(g2m, NARC_GURU2_BG_NCLR, FALSE);
    NNS_G2dGetUnpackedPaletteData(file, &g2m->paletteData);
    PaletteFade_LoadData(g2m->fade, g2m->paletteData->rawData, 0, 0, 0x40);
    GFL_HeapFree(file);

    file = Guru2Main_LoadFile(g2m, NARC_GURU2_SUB_NCLR, FALSE);
    NNS_G2dGetUnpackedPaletteData(file, &g2m->paletteData);
    PaletteFade_LoadData(g2m->fade, g2m->paletteData->rawData, 1, 0, 0x40);
    GFL_HeapFree(file);
    PaletteFade_BlendPalettes(g2m->fade, 1, 0xffff, 8, GX_RGB(0, 0, 0));

    file = Guru2Main_LoadFile(g2m, NARC_GURU2_BG_NCGR, FALSE);
    NNS_G2dGetUnpackedCharacterData(file, &g2m->charData);
    GFL_BGSysLoadChar(3, g2m->charData->rawData, g2m->charData->size, 0);
    GFL_HeapFree(file);

    file = Guru2Main_LoadFile(g2m, NARC_GURU2_SUB_BG2_NCGR, FALSE);
    NNS_G2dGetUnpackedCharacterData(file, &g2m->charData);
    GFL_BGSysLoadChar(6, g2m->charData->rawData, g2m->charData->size, 0);
    GFL_HeapFree(file);

    file = Guru2Main_LoadFile(g2m, NARC_GURU2_SUB_BG3_NCGR, FALSE);
    NNS_G2dGetUnpackedCharacterData(file, &g2m->charData);
    GFL_BGSysLoadChar(7, g2m->charData->rawData, g2m->charData->size, 0);
    GFL_HeapFree(file);

    file = Guru2Main_LoadFile(g2m, NARC_GURU2_BG2_NSCR, FALSE);
    NNS_G2dGetUnpackedScreenData(file, &g2m->screenData);
    GFL_BGSysBufferScrDefault(2, g2m->screenData->rawData, g2m->screenData->size);
    GFL_BGSysLoadScr(2);
    GFL_HeapFree(file);

    file = Guru2Main_LoadFile(g2m, NARC_GURU2_BG3_NSCR, FALSE);
    NNS_G2dGetUnpackedScreenData(file, &g2m->screenData);
    GFL_BGSysBufferScrDefault(3, g2m->screenData->rawData, g2m->screenData->size);
    GFL_BGSysLoadScr(3);
    GFL_HeapFree(file);

    file = Guru2Main_LoadFile(g2m, NARC_GURU2_BUTTON_NSCR, FALSE);
    NNS_G2dGetUnpackedScreenData(file, &g2m->screenData);
    GFL_BGSysBufferScrDefault(6, g2m->screenData->rawData, g2m->screenData->size);
    GFL_BGSysLoadScr(6);
    GFL_HeapFree(file);

    file = Guru2Main_LoadFile(g2m, NARC_GURU2_SUB_BG3_NSCR, FALSE);
    NNS_G2dGetUnpackedScreenData(file, &g2m->screenData);
    GFL_BGSysBufferScrDefault(7, g2m->screenData->rawData, g2m->screenData->size);
    GFL_BGSysLoadScr(7);
    GFL_HeapFree(file);
}

static void Guru2Main_FreeBGs(Guru2MainWork *g2m) {
    BmpWin_FreeAllocator();
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(3);
    GFL_BGSysReleaseBG(6);
    GFL_BGSysReleaseBG(7);
    GFL_BGSysFree();
}

static void Guru2Main_LoadButtonScreen(Guru2MainWork *g2m, u32 fileId) {
    void *file = Guru2Main_LoadFile(g2m, fileId, FALSE);

    NNS_G2dGetUnpackedScreenData(file, &g2m->screenData);
    GFL_BGSysBufferScrDefault(6, g2m->screenData->rawData, g2m->screenData->size);
    GFL_BGSysLoadScr(6);
    GFL_HeapFree(file);
}

static void Guru2Main_ShowButtonHalf(Guru2MainWork *g2m) {
    Guru2Main_LoadButtonScreen(g2m, NARC_GURU2_BUTTON_HALF_NSCR);
}

static void Guru2Main_ShowButton(Guru2MainWork *g2m) {
    Guru2Main_LoadButtonScreen(g2m, NARC_GURU2_BUTTON_NSCR);
}

static void Guru2Main_ShowButtonPushed(Guru2MainWork *g2m) {
    Guru2Main_LoadButtonScreen(g2m, NARC_GURU2_BUTTON_PUSHED_NSCR);
}

static void Guru2Main_InitFade(Guru2MainWork *g2m) {
    g2m->fade = PaletteFade_Create(HEAPID_GURU2);
    PaletteFade_SetTransferAll(g2m->fade, TRUE);
    PaletteFade_AllocBuffer(g2m->fade, 0, 0x200, HEAPID_GURU2);
    PaletteFade_AllocBuffer(g2m->fade, 1, 0x200, HEAPID_GURU2);
}

static void Guru2Main_FreeFade(Guru2MainWork *g2m) {
    PaletteFade_FreeBuffer(g2m->fade, 0);
    PaletteFade_FreeBuffer(g2m->fade, 1);
    PaletteFade_Free(g2m->fade);
}

// Darkens the touch screen
static void Guru2Main_SetSubFade(Guru2MainWork *g2m, int evy) {
    PaletteFade_BlendPalettes(g2m->fade, 1, 0xffff, evy, GX_RGB(0, 0, 0));
}

static void Guru2Main_InitClAct(Guru2MainWork *g2m) {
    ClActSys_Create(&data_02093f08, &sGuru2VRAMConfig, HEAPID_GURU2);
    g2m->clactUnit = func_0204bf1c(30, 1, HEAPID_GURU2);
}

static void Guru2Main_LoadClActResources(Guru2MainWork *g2m) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_GURU2_OBJ, HEAPID_GURU2);

    g2m->chars = func_0204b81c(arc, 6, FALSE, 0, HEAPID_GURU2);
    g2m->palette = func_0204bba0(arc, 0, 0, 0, HEAPID_GURU2);
    g2m->cellAnims = func_0204bde0(arc, 10, 14, HEAPID_GURU2);
    GFL_ArcToolFree(arc);
    func_02042ba8(TRUE, HEAPID_GURU2);
}

static void Guru2Main_FreeClAct(Guru2MainWork *g2m) {
    func_0204b98c(g2m->chars);
    func_0204bcd0(g2m->palette);
    func_0204be64(g2m->cellAnims);
    func_0204bf98(g2m->clactUnit);
    func_0204b758();
}

static void Guru2Main_InitMsg(Guru2MainWork *g2m) {
    Guru2MsgWork *msg = &g2m->msg;
    int i;

    LoadSysMsgBox(1, 1, 15, 0, HEAPID_GURU2);
    PaletteFade_LoadNCLR(g2m->fade, ARCID_WINFRAME, 3, HEAPID_GURU2, 0, 0x20, 14 * 16);
    PaletteFade_LoadNCLR(g2m->fade, ARCID_FONT, 5, HEAPID_GURU2, 0, 0x20, 15 * 16);
    msg->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, HEAPID_GURU2);
    msg->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS, HEAPID_GURU2);
    msg->wordSet = GFL_WordSetSystemCreateDefault(HEAPID_GURU2);
    msg->msgWindow = BmpWin_CreateDynamic(1, 2, 19, 26, 4, 15, TRUE);
    msg->strbuf = GFL_StrBufCreate(0x100, HEAPID_GURU2);
    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        g2m->names[i] = GFL_StrBufCreate(8, HEAPID_GURU2);
    }
    PrintWindow_Init(&msg->msgPrint, msg->msgWindow);
    msg->queue = func_02021998(HEAPID_GURU2);
    Guru2Main_CreateNameWindows(g2m, g2m->g2p->memberCount);
}

static void Guru2Main_FreeMsg(Guru2MainWork *g2m) {
    Guru2MsgWork *msg = &g2m->msg;
    int i;

    BmpWin_ClearScreenNow(msg->msgWindow);
    BmpWin_Free(msg->msgWindow);
    Guru2Main_FreeNameWindows(g2m);
    GFL_MsgDataFree(msg->msgData);
    GFL_WordSetSystemFree(msg->wordSet);
    GFL_FontFree(msg->font);
    GFL_StrBufFree(msg->strbuf);
    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        GFL_StrBufFree(g2m->names[i]);
    }
    BmpWin_FreeAllocator();
}

static void Guru2Main_PrintMsg(Guru2MainWork *g2m, u32 messageId) {
    Guru2MsgWork *msg = &g2m->msg;
    BmpWin *window = msg->msgWindow;

    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    BmpWin_DrawFrame(window, 2, 1, 14);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    GFL_MsgDataLoadStrbuf(msg->msgData, messageId, msg->strbuf);
    PrintWindow_Print(&msg->msgPrint, msg->queue, 0, 0, msg->strbuf, msg->font, COLOR_NAME);
    BmpWin_Transfer(window);
}

// Prints a message with the player's name and another member's
static void Guru2Main_PrintEggMsg(Guru2MainWork *g2m, u32 messageId, UnionAppMember *member) {
    Guru2MsgWork *msg = &g2m->msg;
    BmpWin *window = msg->msgWindow;
    UnionApp *app = Guru2Main_GetUnionApp(g2m);
    StrBuf *strbuf;

    copyVarForText(msg->wordSet, 1, &UnionApp_GetMember(app, func_0203ffc4())->info);
    copyVarForText(msg->wordSet, 2, &member->info);
    strbuf = GFL_StrBufCreate(0x100, HEAPID_GURU2);
    GFL_MsgDataLoadStrbuf(msg->msgData, messageId, strbuf);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->strbuf, strbuf);
    GFL_StrBufFree(strbuf);
    BmpWin_DrawFrame(window, 2, 1, 14);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    PrintWindow_Print(&msg->msgPrint, msg->queue, 0, 0, msg->strbuf, msg->font, COLOR_NAME);
    BmpWin_Transfer(window);
}

static void Guru2Main_PrintItemMsg(Guru2MainWork *g2m, u32 messageId, u32 item) {
    Guru2MsgWork *msg = &g2m->msg;
    BmpWin *window = msg->msgWindow;
    StrBuf *strbuf;

    loadItemNameToStrbuf(msg->wordSet, 0, item);
    strbuf = GFL_StrBufCreate(0x100, HEAPID_GURU2);
    GFL_MsgDataLoadStrbuf(msg->msgData, messageId, strbuf);
    GFL_WordSetFormatStrbuf(msg->wordSet, msg->strbuf, strbuf);
    GFL_StrBufFree(strbuf);
    BmpWin_DrawFrame(window, 2, 1, 14);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    PrintWindow_Print(&msg->msgPrint, msg->queue, 0, 0, msg->strbuf, msg->font, COLOR_NAME);
    BmpWin_Transfer(window);
}

static void Guru2Main_ClearMsg(Guru2MainWork *g2m) {
    BmpWin *window = g2m->msg.msgWindow;

    BmpWin_ClearFrame(window, 0);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    BmpWin_Transfer(window);
}

static void Guru2Main_CreateNameWindows(Guru2MainWork *g2m, int count) {
    int i;
    Guru2MsgWork *msg = &g2m->msg;
    const Guru2NameWindowData *data = sNameWindows[count];

    msg->nameWindowCount = count;
    for (i = 0; i < count; i++) {
        msg->nameWindows[i] = BmpWin_CreateDynamic(data[i].bg, data[i].x, data[i].y, data[i].width, data[i].height,
                                                   data[i].palette, TRUE);
        PrintWindow_Init(&msg->namePrints[i], msg->nameWindows[i]);
    }
}

static void Guru2Main_FreeNameWindows(Guru2MainWork *g2m) {
    int i;
    Guru2MsgWork *msg = &g2m->msg;

    for (i = 0; i < msg->nameWindowCount; i++) {
        BmpWin_ClearScreenNow(msg->nameWindows[i]);
        BmpWin_Free(msg->nameWindows[i]);
    }
}

static void Guru2Main_PrintNameColor(Guru2MainWork *g2m, StrBuf *name, int index, u32 color) {
    Guru2MsgWork *msg = &g2m->msg;
    BmpWin *window = msg->nameWindows[index];

    BmpWin_DrawFrame(window, 2, 1, 14);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    PrintWindow_Print(&msg->namePrints[index], msg->queue, 4, 1, name, msg->font, color);
    BmpWin_Transfer(window);
}

// Prints a member's name in a name window, the player's own in another color
static void Guru2Main_PrintName(Guru2MainWork *g2m, StrBuf *name, int index, int netId) {
    u32 color = COLOR_NAME;

    if (netId == func_0203ffc4()) {
        color = COLOR_NAME_MINE;
    }
    Guru2Main_PrintNameColor(g2m, name, index, color);
}

static void Guru2Main_ClearName(Guru2MainWork *g2m, int index) {
    BmpWin *window = g2m->msg.nameWindows[index];

    BmpWin_ClearFrame(window, 0);
    GFL_BitmapFill(BmpWin_GetBitmap(window), 0);
    BmpWin_Transfer(window);
}

static void Guru2Main_FreeCamera(Guru2MainWork *g2m) {
    GFL_G3DCameraFree(g2m->camera.camera);
}

static BOOL Guru2Main_IsButtonTouched(Guru2MainWork *g2m) {
    if (func_0203da0c(sGuru2ButtonRects) != TOUCH_RECT_NONE) {
        return TRUE;
    }
    return FALSE;
}

static void Guru2Main_InitDisc(Guru2MainWork *g2m) {
    int count = g2m->g2p->memberCount;
    Guru2Disc *disc = &g2m->disc;

    disc->scene = GFL_G3DMgrNewScene(g2m->g3dManager, &sDiscScenes[count - 2]);
    disc->angleOffset = sDiscAngleOffsets[count];
    VEC_Set(&disc->scale, FX32_ONE, FX32_ONE, FX32_ONE);
    disc->rotX = 0;
    disc->rotY = 0;
    disc->rotZ = 0;
    disc->angle = 0;
    VEC_Set(&disc->pos, 0, FX32_CONST(-36), 0);
    disc->drawPos = disc->pos;
}

static void Guru2Main_FreeDisc(Guru2MainWork *g2m) {
    GFL_G3DMgrDeleteScene(g2m->g3dManager, g2m->disc.scene);
}

static void Guru2Main_UpdateDisc(Guru2MainWork *g2m) {
    Guru2Disc *disc = &g2m->disc;
    fx32 angle = g2m->gameData.discAngle;

    Guru2Main_AddAngle(&angle, disc->baseAngle);
    Guru2Main_AddAngle(&angle, disc->angleOffset);
    disc->rotY = (360 - angle / FX32_ONE) % 360;
    disc->drawPos.x = disc->pos.x + disc->offset.x + disc->shake.x;
    disc->drawPos.y = disc->pos.y + disc->offset.y + disc->shake.y;
    disc->drawPos.z = disc->pos.z + disc->offset.z + disc->shake.z;
}

static void Guru2Main_DrawDisc(Guru2MainWork *g2m) {
    Guru2Disc *disc = &g2m->disc;
    G3DActor *actor = GFL_G3DMgrGetActor(g2m->g3dManager, disc->scene);
    SRTMatrix srt;

    VEC_Set(&srt.translation, disc->drawPos.x, disc->drawPos.y, disc->drawPos.z);
    VEC_Set(&srt.scale, disc->scale.x, disc->scale.y, disc->scale.z);
    MAT3_RotationEulerZYX(disc->rotX, DEG_TO_IDX(1) * disc->rotY, disc->rotZ, &srt.rotation);
    GFL_G3DSysDrawObj(actor, &srt);
}

static void Guru2Disc_Turn(Guru2Disc *disc, fx32 angle) {
    Guru2Main_AddAngle(&disc->angle, angle);
}

// The model behind the disc
static void Guru2Main_InitBG3D(Guru2MainWork *g2m) {
    g2m->bgScene = GFL_G3DMgrNewScene(g2m->g3dManager, &sBGScene);
    gfxClearColor(GX_RGB(0, 0, 0), 0, 0x7fff, 0, FALSE);
}

static void Guru2Main_FreeBG3D(Guru2MainWork *g2m) {
    GFL_G3DMgrDeleteScene(g2m->g3dManager, g2m->bgScene);
}

static void Guru2Main_DrawBG3D(Guru2MainWork *g2m) {
    G3DActor *actor = GFL_G3DMgrGetActor(g2m->g3dManager, g2m->bgScene);
    SRTMatrix srt;

    VEC_Set(&srt.translation, 0, 0, 0);
    VEC_Set(&srt.scale, FX32_ONE, FX32_ONE, FX32_ONE);
    MAT3_RotationEulerZYX(0, 0, 0, &srt.rotation);
    GFL_G3DSysDrawObj(actor, &srt);
}

static const Guru2BonusArea sBonusAreas4[] = {
    { 37, 142 },
    { 215, 323 },
};

static const Guru2BonusArea sBonusAreas5[] = {
    { 0, 215 },
    { 323, 359 },
};

static const Guru2BonusArea sBonusAreas3[] = {
    { 0, 198 },
    { 342, 359 },
};

static const Guru2BonusAreaList sBonusAreas[GURU2_MEMBER_MAX + 1] = {
    { 0, NULL },
    { 0, NULL },
    { NELEMS(sBonusAreas2), sBonusAreas2 },
    { NELEMS(sBonusAreas3), sBonusAreas3 },
    { NELEMS(sBonusAreas4), sBonusAreas4 },
    { NELEMS(sBonusAreas5), sBonusAreas5 },
};

static const G3DSceneSetup sCursorScenes[GURU2_MEMBER_MAX] = {
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

static void Guru2Main_InitObjects(Guru2MainWork *g2m) {
    Guru2Objects *objects = &g2m->objects;
    int i;

    objects->eggScene = GFL_G3DMgrNewScene(g2m->g3dManager, &sEggScenes[0]);
    objects->shadowScene = GFL_G3DMgrNewScene(g2m->g3dManager, &sEggScenes[1]);
    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        objects->cursorScenes[i] = GFL_G3DMgrNewScene(g2m->g3dManager, &sCursorScenes[i]);
    }
}

static void Guru2Main_FreeObjects(Guru2MainWork *g2m) {
    int i;
    Guru2Objects *objects = &g2m->objects;

    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        GFL_G3DMgrDeleteScene(g2m->g3dManager, objects->cursorScenes[i]);
    }
    GFL_G3DMgrDeleteScene(g2m->g3dManager, objects->eggScene);
    GFL_G3DMgrDeleteScene(g2m->g3dManager, objects->shadowScene);
}

static void Guru2Main_ShowEgg(Guru2MainWork *g2m, Guru2Egg *egg) {
    egg->visible = TRUE;
    Guru2Main_AddEggShadow(g2m, egg);
    Guru2Main_AddEggCursor(g2m, egg);
}

static void Guru2Main_UpdateObjects(Guru2MainWork *g2m) {
    int i = 0;
    int count = g2m->playerCount;
    Guru2Objects *objects = &g2m->objects;

    for (; i < count; i++) {
        if (objects->eggs[i].active) {
            Guru2Egg_Update(&objects->eggs[i], &g2m->disc.offset, g2m->gameData.eggAngles[i] << FX32_SHIFT,
                            g2m->gameData.jumpY);
        }
        if (objects->shadows[i].active) {
            Guru2Main_UpdateEggShadow(g2m, &objects->shadows[i], g2m->gameData.jumpY);
        }
        if (objects->cursors[i].active) {
            Guru2Main_UpdateEggCursor(g2m, &objects->cursors[i], i);
        }
    }
}

static void Guru2Main_DrawObjects(Guru2MainWork *g2m) {
    int i = 0;
    int count = g2m->playerCount;
    Guru2Objects *objects = &g2m->objects;
    Guru2Egg *egg;
    Guru2EggShadow *shadow = objects->shadows;
    Guru2EggCursor *cursor = objects->cursors;

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
            MAT3_Identity(&srt.rotation);
            GFL_G3DSysDrawObj(actor, &srt);
        }
    }
}

static void Guru2Main_TurnEggs(Guru2MainWork *g2m, fx32 angle) {
    int i = 0;
    int count = g2m->playerCount;
    Guru2Objects *objects = &g2m->objects;

    for (; i < count; i++) {
        if (objects->eggs[i].active) {
            Guru2Main_AddAngle(&objects->eggs[i].angle, angle);
        }
    }
}

// Places an Egg on the disc at an angle
static void Guru2Egg_Update(Guru2Egg *egg, const VecFx32 *offset, fx32 angle, fx32 jumpY) {
    u16 degrees;

    Guru2Main_AddAngle(&angle, egg->angleOffset);
    degrees = angle / FX32_ONE;
    egg->pos.x = egg->offset.x + offset->x + EGG_RADIUS * func_02044330(degrees);
    egg->pos.y = egg->offset.y - FX32_CONST(6) + offset->y + jumpY;
    egg->pos.z = egg->offset.z + offset->z + EGG_RADIUS * func_02044304(degrees);
    egg->rotX = egg->rotation.x / FX32_ONE;
    egg->rotY = egg->rotation.y / FX32_ONE;
    egg->rotZ = egg->rotation.z / FX32_ONE;
}

// The Egg nearest the player's position, going back against the disc's turn
static Guru2Egg *Guru2Main_FindFrontEgg(Guru2MainWork *g2m) {
    int distances[GURU2_MEMBER_MAX];
    int order[GURU2_MEMBER_MAX];
    int i;
    int j;
    int count = g2m->playerCount;
    Guru2Objects *objects = &g2m->objects;
    fx32 target = sEggAngles[count][g2m->position] << FX32_SHIFT;

    i = 0;
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
        int index = order[i];
        int distance = distances[index];

        for (j = i - 1; j >= 0 && distances[order[j]] > distance; j--) {
            order[j + 1] = order[j];
        }
        order[j + 1] = index;
    }
    return &objects->eggs[order[0]];
}

static void Guru2Main_AddEggShadow(Guru2MainWork *g2m, Guru2Egg *egg) {
    Guru2EggShadow *shadow = &g2m->objects.shadows[egg->position];

    shadow->active = TRUE;
    shadow->egg = egg;
    Guru2Main_UpdateEggShadow(g2m, shadow, 0);
}

// The shadow stays on the disc under its Egg, smaller the higher the Egg is
static void Guru2Main_UpdateEggShadow(Guru2MainWork *g2m, Guru2EggShadow *shadow, fx32 jumpY) {
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

static void Guru2Main_AddEggCursor(Guru2MainWork *g2m, Guru2Egg *egg) {
    Guru2EggCursor *cursor = &g2m->objects.cursors[egg->position];

    cursor->active = TRUE;
    cursor->egg = egg;
}

static void Guru2Main_UpdateEggCursor(Guru2MainWork *g2m, Guru2EggCursor *cursor, int index) {
    G3DActor *actor = GFL_G3DMgrGetActor(g2m->g3dManager, g2m->objects.cursorScenes[index]);

    cursor->pos = cursor->egg->pos;
    cursor->pos.y += FX32_CONST(20);
    GFL_G3DActorStepAnmFrameLoop(actor, 0, FX32_ONE);
}

// Sets up a member's Egg and starts its drop onto the disc
static void Guru2Main_AddEgg(Guru2MainWork *g2m, int netId, int position, int index, int angle, int angleOffset,
                             Guru2Egg *egg) {
    Guru2EggFallTask *task;

    egg->netId = netId;
    egg->position = position;
    egg->index = index;
    egg->startPosition = position;
    egg->angle = angle << FX32_SHIFT;
    egg->angleOffset = angleOffset << FX32_SHIFT;
    egg->active = TRUE;
    egg->landed = FALSE;

    task = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_GURU2), sizeof(Guru2EggFallTask), FALSE, "guru2_main.c", 4878);
    sys_memset(task, 0, sizeof(Guru2EggFallTask));
    task->g2m = g2m;
    task->egg = egg;
    GFL_TCBMgrAddTask(g2m->tcbManager, Guru2EggFallTask_Main, task, 0);
}

static void Guru2EggFallTask_Main(TCB *tcb, void *data) {
    Guru2EggFallTask *task = data;
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

static int Guru2EggFallTask_Init(Guru2EggFallTask *task) {
    Guru2Egg *egg = task->egg;

    egg->scale.x = FX32_ONE;
    egg->scale.y = FX32_ONE;
    egg->scale.z = FX32_ONE;
    egg->offset.y = FX32_CONST(192);
    Guru2Main_ShowEgg(task->g2m, egg);
    task->speed = FX32_CONST(8);
    task->seq = 1;
    return SEQ_RESULT_WAIT;
}

static int Guru2EggFallTask_Fall(Guru2EggFallTask *task) {
    Guru2Egg *egg = task->egg;

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
static int Guru2EggFallTask_Land(Guru2EggFallTask *task) {
    Guru2Egg *egg = task->egg;

    if (task->speed > FX32_ONE) {
        task->speed -= FX32_ONE;
        if (task->speed < FX32_ONE) {
            task->speed = FX32_ONE;
        }
    }
    egg->offset.y -= task->speed;
    if (egg->offset.y <= 0) {
        egg->offset.y = 0;
        Guru2Main_PrintName(task->g2m, task->g2m->names[egg->netId], egg->index, egg->netId);
        return SEQ_RESULT_END;
    }
    return SEQ_RESULT_WAIT;
}

// An Egg's jump, up and down over 15 frames each
static void Guru2EggJumpTask_Update(Guru2MainWork *g2m, Guru2EggJumpTask *task) {
    Guru2Egg *egg = task->egg;

    switch (task->seq) {
    case 0:
        Guru2Main_StartEggSpin(g2m, egg);
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
                Guru2Main_StartEggShake(g2m, egg);
                sys_memset(task, 0, sizeof(Guru2EggJumpTask));
            }
        }
        break;
    }
}

static void Guru2EggJumpTask_Main(TCB *tcb, void *data) {
    Guru2MainWork *g2m = data;
    int i;
    Guru2EggJumpTask *task = g2m->jumpTasks;

    for (i = 0; i < GURU2_MEMBER_MAX; i++, task++) {
        if (task->active) {
            Guru2EggJumpTask_Update(g2m, task);
        }
    }
}

static void Guru2Main_InitEggJump(Guru2MainWork *g2m) {
    sys_memset(g2m->jumpTasks, 0, sizeof(g2m->jumpTasks));
    g2m->jumpTCB = GFL_TCBMgrAddTask(g2m->tcbManager, Guru2EggJumpTask_Main, g2m, 0x100);
}

static void Guru2Main_FreeEggJump(Guru2MainWork *g2m) {
    GFL_TCBRemove(g2m->jumpTCB);
}

static void Guru2Main_StartEggJump(Guru2MainWork *g2m, Guru2Egg *egg) {
    int i;
    Guru2EggJumpTask *task = g2m->jumpTasks;

    for (i = 0; i < GURU2_MEMBER_MAX; i++, task++) {
        if (!task->active) {
            task->active = TRUE;
            task->egg = egg;
            return;
        }
    }
}

static void Guru2Main_StartEggJumps(Guru2MainWork *g2m) {
    Guru2Objects *objects = &g2m->objects;
    int count = g2m->playerCount;
    int i = 0;

    do {
        if (objects->eggs[i].active) {
            Guru2Main_StartEggJump(g2m, &objects->eggs[i]);
        }
        i++;
    } while (i < count);
}

static BOOL Guru2Main_IsEggJumping(Guru2MainWork *g2m) {
    int i;
    Guru2EggJumpTask *task = g2m->jumpTasks;

    for (i = 0; i < GURU2_MEMBER_MAX; i++, task++) {
        if (task->active) {
            return TRUE;
        }
    }
    return FALSE;
}

// An Egg's spin as it jumps, which slows down each turn
static void Guru2EggSpinTask_Update(Guru2EggSpinTask *task) {
    Guru2Egg *egg = task->egg;

    switch (task->seq) {
    case 0:
        task->speed = FX32_CONST(24);
        egg->rotation.y = 0;
        task->seq++;
    case 1:
        Guru2Main_AddAngle(&egg->rotation.y, task->speed);
        if (egg->rotation.y < FX32_CONST(4)) {
            task->speed = (u32)task->speed >> 1;
            if (task->speed < FX32_CONST(4)) {
                egg->rotation.y = 0;
                sys_memset(task, 0, sizeof(Guru2EggSpinTask));
            }
        }
        break;
    }
}

static void Guru2EggSpinTask_Main(TCB *tcb, void *data) {
    Guru2MainWork *g2m = data;
    int i;
    Guru2EggSpinTask *task = g2m->spinTasks;

    for (i = 0; i < GURU2_MEMBER_MAX; i++, task++) {
        if (task->active) {
            Guru2EggSpinTask_Update(task);
        }
    }
}

static void Guru2Main_InitEggSpin(Guru2MainWork *g2m) {
    sys_memset(g2m->spinTasks, 0, sizeof(g2m->spinTasks));
    g2m->spinTCB = GFL_TCBMgrAddTask(g2m->tcbManager, Guru2EggSpinTask_Main, g2m, 0x101);
}

static void Guru2Main_FreeEggSpin(Guru2MainWork *g2m) {
    GFL_TCBRemove(g2m->spinTCB);
}

// Starts an Egg's spin, or starts it over
static void Guru2Main_StartEggSpin(Guru2MainWork *g2m, Guru2Egg *egg) {
    Guru2EggSpinTask *tasks = g2m->spinTasks;
    int i;
    Guru2EggSpinTask *task;

    task = tasks;
    for (i = 0; i < GURU2_MEMBER_MAX; i++, task++) {
        if (task->active == TRUE && task->egg == egg) {
            task->seq = 0;
            return;
        }
    }

    task = tasks;
    for (i = 0; i < GURU2_MEMBER_MAX; i++, task++) {
        if (!task->active) {
            task->active = TRUE;
            task->egg = egg;
            return;
        }
    }
}

// An Egg's wobble after it lands, which halves each swing
static void Guru2EggShakeTask_Update(Guru2EggShakeTask *task) {
    Guru2Egg *egg = task->egg;
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
        rotation = task->amplitude / FX32_ONE * func_02044304(task->angle / FX32_ONE);
        egg->rotation.z = 0;
        Guru2Main_AddAngle(&egg->rotation.z, rotation);
        angle = task->angle;
        Guru2Main_AddAngle(&task->angle, task->speed);
        if (task->angle < angle) {
            task->amplitude = (u32)task->amplitude >> 1;
            if (task->amplitude == 0) {
                egg->rotation.z = 0;
                sys_memset(task, 0, sizeof(Guru2EggShakeTask));
            }
        }
        break;
    }
}

static void Guru2EggShakeTask_Main(TCB *tcb, void *data) {
    Guru2MainWork *g2m = data;
    int i;
    Guru2EggShakeTask *task = g2m->shakeTasks;

    for (i = 0; i < GURU2_MEMBER_MAX; i++, task++) {
        if (task->active) {
            Guru2EggShakeTask_Update(task);
        }
    }
}

static void Guru2Main_InitEggShake(Guru2MainWork *g2m) {
    sys_memset(g2m->shakeTasks, 0, sizeof(g2m->shakeTasks));
    g2m->shakeTCB = GFL_TCBMgrAddTask(g2m->tcbManager, Guru2EggShakeTask_Main, g2m, 0x102);
}

static void Guru2Main_FreeEggShake(Guru2MainWork *g2m) {
    GFL_TCBRemove(g2m->shakeTCB);
}

static void Guru2Main_StartEggShake(Guru2MainWork *g2m, Guru2Egg *egg) {
    Guru2EggShakeTask *tasks = g2m->shakeTasks;
    int i;
    Guru2EggShakeTask *task;

    task = tasks;
    for (i = 0; i < GURU2_MEMBER_MAX; i++, task++) {
        if (task->active == TRUE && task->egg == egg) {
            task->seq = 0;
            return;
        }
    }

    task = tasks;
    for (i = 0; i < GURU2_MEMBER_MAX; i++, task++) {
        if (!task->active) {
            task->active = TRUE;
            task->egg = egg;
            return;
        }
    }
}

// The disc sinks, springs back up and throws the Eggs, then shakes until they have landed
static void Guru2DiscBumpTask_Update(Guru2MainWork *g2m, Guru2DiscBumpTask *task) {
    Guru2Disc *disc = &g2m->disc;

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
            Guru2Main_StartEggJumps(g2m);
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
        if (!Guru2Main_IsEggJumping(g2m)) {
            task->active = FALSE;
            task->jumping = FALSE;
            task->seq = 0;
            GFL_SndSEPlay(SEQ_SE_SYS_83);
        }
        break;
    }
}

static void Guru2DiscBumpTask_Main(TCB *tcb, void *data) {
    Guru2MainWork *g2m = data;

    Guru2DiscBumpTask_Update(g2m, &g2m->bumpTask);
}

static void Guru2Main_InitDiscBump(Guru2MainWork *g2m) {
    sys_memset(&g2m->bumpTask, 0, sizeof(Guru2DiscBumpTask));
    g2m->bumpTCB = GFL_TCBMgrAddTask(g2m->tcbManager, Guru2DiscBumpTask_Main, g2m, 0xff);
}

static void Guru2Main_FreeDiscBump(Guru2MainWork *g2m) {
    GFL_TCBRemove(g2m->bumpTCB);
}

static void Guru2Main_StartDiscBump(Guru2MainWork *g2m) {
    Guru2DiscBumpTask *task = &g2m->bumpTask;

    task->active = TRUE;
    task->seq = 1;
}

static BOOL Guru2Main_AreEggsJumping(Guru2MainWork *g2m) {
    return g2m->bumpTask.jumping;
}

static BOOL Guru2Main_IsDiscBumping(Guru2MainWork *g2m) {
    return g2m->bumpTask.active;
}

// Plays the countdown's sounds while its actor's animation runs
static void Guru2CountdownTask_Main(TCB *tcb, void *data) {
    Guru2CountdownTask *task = data;

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
        func_0204c4e0(task->actor, FX32_CONST(1.5));
        if (!func_0204c560(task->actor)) {
            task->seq++;
        }
        break;
    case 1:
        func_0204c108(task->actor);
        GFL_HeapFree(task);
        GFL_TCBRemove(tcb);
        break;
    }
}

static void Guru2Main_StartCountdown(Guru2MainWork *g2m) {
    Guru2CountdownTask *task;
    ClActorSetup setup;

    task = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_GURU2), sizeof(Guru2CountdownTask), FALSE, "guru2_main.c", 5638);
    sys_memset(task, 0, sizeof(Guru2CountdownTask));
    setup.x = 128;
    setup.y = 100;
    setup.sequence = 0;
    setup.bgPriority = 0;
    setup.priority = 0;
    task->actor = func_0204c040(g2m->clactUnit, g2m->chars, g2m->palette, g2m->cellAnims, &setup, 0, HEAPID_GURU2);
    func_0204c4e0(task->actor, FX32_ONE);
    task->g2m = g2m;
    GFL_TCBMgrAddTask(g2m->tcbManager, Guru2CountdownTask_Main, task, 0);
}

// The button's push on the touch screen
static void Guru2ButtonTask_Update(Guru2MainWork *g2m, Guru2ButtonTask *task) {
    switch (task->seq) {
    case 0:
        break;
    case 1:
        Guru2Main_ShowButtonHalf(g2m);
        GFL_SndSEPlay(SEQ_SE_SYS_54);
        task->frame = 0;
        task->seq++;
    case 2:
        task->frame++;
        if (task->frame >= 2) {
            task->frame = 0;
            task->seq++;
            Guru2Main_ShowButtonPushed(g2m);
        }
        break;
    case 3:
        task->frame++;
        if (task->frame >= 4) {
            Guru2Main_ShowButtonHalf(g2m);
            task->frame = 0;
            task->seq++;
        }
        break;
    case 4:
        task->frame++;
        if (task->frame >= 2) {
            Guru2Main_ShowButton(g2m);
            task->active = FALSE;
            task->seq = 0;
        }
        break;
    }
}

static void Guru2ButtonTask_Main(TCB *tcb, void *data) {
    Guru2MainWork *g2m = data;

    Guru2ButtonTask_Update(g2m, &g2m->buttonTask);
}

static void Guru2Main_InitButton(Guru2MainWork *g2m) {
    sys_memset(&g2m->buttonTask, 0, sizeof(Guru2ButtonTask));
    g2m->buttonTCB = GFL_TCBMgrAddTask(g2m->tcbManager, Guru2ButtonTask_Main, g2m, 0x103);
}

static void Guru2Main_FreeButton(Guru2MainWork *g2m) {
    GFL_TCBRemove(g2m->buttonTCB);
}

static u32 Guru2Main_IsButtonPushed(Guru2MainWork *g2m) {
    return g2m->buttonTask.active;
}

static void Guru2Main_PushButton(Guru2MainWork *g2m) {
    Guru2ButtonTask *task = &g2m->buttonTask;

    task->active = TRUE;
    task->seq = 1;
}

// An Egg hops on its bonus area, with a rest between the hops
static void Guru2BonusTask_Update(Guru2MainWork *g2m, Guru2BonusTask *task) {
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

static void Guru2BonusTask_Main(TCB *tcb, void *data) {
    Guru2MainWork *g2m = data;
    int i;
    Guru2BonusTask *task = g2m->bonusTasks;

    for (i = 0; i < GURU2_MEMBER_MAX; i++, task++) {
        Guru2BonusTask_Update(g2m, task);
    }
}

static void Guru2Main_InitBonus(Guru2MainWork *g2m) {
    sys_memset(g2m->bonusTasks, 0, sizeof(g2m->bonusTasks));
    g2m->bonusTCB = GFL_TCBMgrAddTask(g2m->tcbManager, Guru2BonusTask_Main, g2m, 0x104);
}

static void Guru2Main_FreeBonus(Guru2MainWork *g2m) {
    GFL_TCBRemove(g2m->bonusTCB);
}

static void Guru2Main_StartBonus(Guru2MainWork *g2m, Guru2Egg *egg) {
    Guru2BonusTask *task = &g2m->bonusTasks[egg->netId];

    task->seq = 1;
    task->egg = egg;
}

static void Guru2Main_StartBonuses(Guru2MainWork *g2m) {
    u32 bonusBits = g2m->bonusBits;
    int i = 0;
    int count = g2m->playerCount;
    Guru2Objects *objects = &g2m->objects;

    for (; i < count; i++) {
        if (objects->eggs[i].active && (bonusBits & (1 << objects->eggs[i].netId))) {
            Guru2Main_StartBonus(g2m, &objects->eggs[i]);
        }
    }
}

static void Guru2FadeTask_Main(TCB *tcb, void *data) {
    Guru2FadeTask *task = data;
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
    Guru2Main_SetSubFade(task->g2m, task->evy);
    if (done == TRUE) {
        GFL_HeapFree(task);
        GFL_TCBRemove(tcb);
    }
}

// Lights the touch screen up for the game, or darkens it again
static void Guru2Main_StartSubFade(Guru2MainWork *g2m, int out) {
    int start[] = { 0, 8 };
    Guru2FadeTask *task =
        GFL_HeapAllocate(HEAPID_TAIL(HEAPID_GURU2), sizeof(Guru2FadeTask), FALSE, "guru2_main.c", 5992);

    task->g2m = g2m;
    task->out = out;
    task->evy = start[out];
    GFL_TCBMgrAddTask(g2m->tcbManager, Guru2FadeTask_Main, task, 0);
}

// Adds to an angle, which stays within a turn
static void Guru2Main_AddAngle(fx32 *angle, fx32 addend) {
    *angle += addend;
    while (*angle < 0) {
        *angle += FULL_TURN;
    }
    *angle %= FULL_TURN;
}

static void Guru2Main_Turn(Guru2MainWork *g2m, fx32 angle) {
    Guru2Disc_Turn(&g2m->disc, angle);
    Guru2Main_TurnEggs(g2m, angle);
}

// Returns whether the game's time is up
static BOOL Guru2Main_CountGameFrame(Guru2MainWork *g2m) {
    g2m->gameFrame++;
    if (g2m->gameFrame < GAME_FRAMES) {
        return FALSE;
    }
    g2m->gameFrame = GAME_FRAMES;
    return TRUE;
}

static void *Guru2Main_LoadFile(Guru2MainWork *g2m, u32 fileId, BOOL head) {
    u32 size = GFL_ArcToolGetDataLength(g2m->arc, fileId);
    void *file;

    if (head == TRUE) {
        file = GFL_HeapAllocate(HEAPID_GURU2, size, FALSE, "guru2_main.c", 6090);
    } else {
        file = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_GURU2), size, FALSE, "guru2_main.c", 6092);
    }
    GFL_ArcToolRead(g2m->arc, fileId, file);
    return file;
}

// GURU2_NET_CMD_7: someone pressed the button
void Guru2Main_OnButton(Guru2MainWork *g2m, u32 value) {
    if (value != 0) {
        g2m->buttonReceived = value;
    }
}

// GURU2_NET_CMD_8: the parent's Guru2GameData
void Guru2Main_OnGameData(Guru2MainWork *g2m, const void *data) {
    g2m->gameDataReceived = TRUE;
    g2m->gameData = *(const Guru2GameData *)data;
}

static void *Guru2Main_InitSeqWork(Guru2MainWork *g2m, u32 size) {
    sys_memset(g2m->seqWork, 0, sizeof(g2m->seqWork));
    return g2m->seqWork;
}

static void *Guru2Main_GetSeqWork(Guru2MainWork *g2m) {
    return g2m->seqWork;
}

static BOOL Guru2Main_HasSignal(Guru2MainWork *g2m, u32 signal) {
    if (g2m->g2c->unk40 & signal) {
        return TRUE;
    }
    return FALSE;
}

// Whether every child of the receipt screen has joined this one
static BOOL Guru2Main_HaveChildrenJoined(Guru2MainWork *g2m) {
    int count = 0;
    int max = g2m->g2p->memberCount - 1;
    u32 bits = g2m->g2c->unk42;

    while (bits != 0) {
        count += bits & 1;
        bits >>= 1;
    }
    return count >= max;
}

static int Guru2Main_CountChildren(Guru2MainWork *g2m) {
    int count = 0;
    u32 bits = g2m->g2c->unk42;

    while (bits != 0) {
        count += bits & 1;
        bits >>= 1;
    }
    return count;
}

// GURU2_NET_CMD_9: a member's position
void Guru2Main_OnPosition(Guru2MainWork *g2m, const void *data) {
    const Guru2PositionData *position = data;

    g2m->positions[position->position] = *position;
    if (position->netId == func_0203ffc4()) {
        g2m->position = position->position;
    }
}

// GURU2_NET_CMD_10: the parent's player count, which this machine counts itself
void Guru2Main_OnPlayerCount(Guru2MainWork *g2m, u32 value) {
    g2m->playerCount = UnionApp_CountJoined(Guru2Main_GetUnionApp(g2m));
}

static BOOL Guru2Main_IsMember(Guru2MainWork *g2m, int netId) {
    if (netId != 0 && !(g2m->g2c->unk42 & (1 << netId))) {
        return FALSE;
    }
    return TRUE;
}

// Replaces the Egg the player put in with the one of the given member
static void Guru2Main_TradeEgg(Guru2MainWork *g2m, int netId) {
    PokeParty *party = g2m->party;
    PokeParty *otherParty = Guru2Comm_GetBlock(g2m->g2c, netId);
    u32 otherIndex = g2m->partyIndices[netId];
    PartyPkm *pkm = PokeParty_GetPkm(party, g2m->g2p->partyIndex);
    PartyPkm *otherPkm = PokeParty_GetPkm(otherParty, otherIndex);
    UnionApp *app = Guru2Main_GetUnionApp(g2m);

    PokeParty_SetupMetData(otherPkm, 6, &UnionApp_GetMember(app, func_0203ffc4())->info, 30002, HEAPID_GURU2);
    copyPartyPkm(otherPkm, pkm);
}

// How many members' parties have arrived
static int Guru2Main_CountParties(Guru2MainWork *g2m) {
    int count = 0;
    u32 bits = g2m->g2c->blockBits;

    while (bits != 0) {
        count += bits & 1;
        bits >>= 1;
    }
    return count;
}

// GURU2_NET_CMD_11: the party slot a member trades
void Guru2Main_OnPartyIndex(Guru2MainWork *g2m, int netId, u32 value) {
    g2m->partyIndices[netId] = value;
}

// GURU2_NET_CMD_12: the parent's Guru2ResultData
void Guru2Main_OnResult(Guru2MainWork *g2m, const void *data) {
    g2m->result = *(const Guru2ResultData *)data;
    g2m->resultReceived = TRUE;
}

// Whether an Egg at this angle of the disc is over a bonus area
static BOOL Guru2Main_IsBonusAngle(Guru2MainWork *g2m, fx32 angle, int playerCount) {
    fx32 edge;
    u32 start;
    const Guru2BonusAreaList *list;
    int count;
    u32 end;
    const Guru2BonusArea *area;

    Guru2Main_AddAngle(&angle, -g2m->disc.angle);
    Guru2Main_AddAngle(&angle, -g2m->disc.angleOffset);
    edge = angle;
    Guru2Main_AddAngle(&edge, FX32_CONST(-4));
    start = edge / FX32_ONE;
    edge = angle;
    Guru2Main_AddAngle(&edge, FX32_CONST(5));
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

static BOOL Guru2Main_PartyHasBadEgg(PokeParty *party) {
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

static BOOL Guru2Main_AnyPartyHasBadEgg(Guru2MainWork *g2m) {
    int i;

    if (Guru2Main_PartyHasBadEgg(g2m->party) == TRUE) {
        return TRUE;
    }
    for (i = 0; i < GURU2_MEMBER_MAX; i++) {
        if (Guru2Main_IsMember(g2m, i) && Guru2Main_PartyHasBadEgg(Guru2Comm_GetBlock(g2m->g2c, i)) == TRUE) {
            return TRUE;
        }
    }
    return FALSE;
}

// How many members have found no bad Egg
static int Guru2Main_CountEggChecks(Guru2MainWork *g2m) {
    int count = 0;
    u32 bits = g2m->g2c->unk4A;

    while (bits != 0) {
        count += bits & 1;
        bits >>= 1;
    }
    return count;
}

static BOOL Guru2Main_IsBadEggFound(Guru2MainWork *g2m) {
    if (g2m->g2c->unk48 != 0) {
        return TRUE;
    }
    return FALSE;
}

// Plays a jingle over the music
static void Guru2Main_PlayME(u32 me) {
    GFL_SndBGMSetPaused(TRUE);
    GFL_SndBGMPush();
    GFL_SndBGMPlay(me, 0xffff);
}

// Returns whether the jingle still plays, and brings the music back once it doesn't
static BOOL Guru2Main_IsMEPlaying(void) {
    if (!GFL_SndBGMIsPlaying()) {
        GFL_SndBGMPop();
        GFL_SndBGMSetPaused(FALSE);
        return FALSE;
    }
    return TRUE;
}

// The parent turns the disc and the Eggs, and sends the others where they are
static void Guru2Main_ParentTurn(Guru2MainWork *g2m, fx32 angle) {
    if (func_0203ffc4() == 0) {
        Guru2GameData data;
        int i;

        Guru2Main_Turn(g2m, angle);
        data.discAngle = g2m->disc.angle;
        data.jumpY = g2m->objects.eggs[0].jumpY;
        for (i = 0; i < g2m->playerCount; i++) {
            data.eggAngles[i] = g2m->objects.eggs[i].angle / FX32_ONE;
        }
        Guru2Comm_Send(g2m->g2c, GURU2_NET_CMD_8, &data, sizeof(data));
    }
}
