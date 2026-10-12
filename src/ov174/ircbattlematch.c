#include "types.h"
#include "app/ircbattlematch.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "field/delivery_irc.h"
#include "field/net_devirc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/button_man.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_system.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "save/join_avenue.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "save/wifi_list.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/palanm.h"
#include "system/wipe.h"
#include "system/wordset.h"

// Overlay 174's ircbattlematch.c, named after its string: the infrared match screen of the infrared menu's battles,
// trades and friend codes, the musical and the Battle Subway. The players make an infrared connection, which shows
// the mode they chose; the screen then trades their modes, trainer data and parties, checks that they chose the same
// thing, and ends with the agreed mode in the parameter's result.

// The game's commands, from the high byte 0xc
enum {
    IRCMATCH_CMD_MODE = 0xc00,
    IRCMATCH_CMD_PLAYER_INFO,
    IRCMATCH_CMD_PARTY,
    IRCMATCH_CMD_FRIEND_DATA,
    IRCMATCH_CMD_MAC,
    IRCMATCH_CMD_ORDER,
    IRCMATCH_CMD_JOIN_AVENUE,
    IRCMATCH_CMD_COUNT = 7,
};

// The numbers the machines synchronize on, in the order they come
enum {
    IRCMATCH_TIMING_COMMANDS = 0x78,
    IRCMATCH_TIMING_FRIEND_DATA,
    IRCMATCH_TIMING_PARTY,
    IRCMATCH_TIMING_PLAYER_INFO,
    IRCMATCH_TIMING_MODE,
    IRCMATCH_TIMING_MAC,
    IRCMATCH_TIMING_ORDER,
    IRCMATCH_TIMING_RESULT,
    IRCMATCH_TIMING_JOIN_AVENUE,
};
#define IRCMATCH_TIMING_GROUP 12

// The archive of the screen's own graphics (not named: our name would be ours alone)
#define IRCMATCH_ARCID 28

// The messages of TEXT_BANK_STARTMENU_2 (the infrared menu's, bank 62 of the system messages) that the screen shows
enum {
    IRCMATCH_MSG_BACK = 2,
    IRCMATCH_MSG_CONNECT_OPPONENTS = 8,
    IRCMATCH_MSG_CONNECT_PARTNER = 9,
    IRCMATCH_MSG_CONNECT_LEADER = 11,
    IRCMATCH_MSG_WAIT = 12,
    IRCMATCH_MSG_QUIT_CONFIRM = 15,
    IRCMATCH_MSG_CONNECT_TRADE_PARTNER = 16,
    IRCMATCH_MSG_WAIT_MOMENT = 23,
    IRCMATCH_MSG_MISMATCH = 24,
    IRCMATCH_MSG_YES = 26,
    IRCMATCH_MSG_NO = 27,
    IRCMATCH_MSG_TRADE_STARTING = 29,
    IRCMATCH_MSG_BATTLE_STARTING = 30,
    IRCMATCH_MSG_CONNECT_FRIEND_CODE = 32,
    IRCMATCH_MSG_FRIEND_ROSTER_FULL = 33,
    IRCMATCH_MSG_FRIEND_COUNT = 34,
    IRCMATCH_MSG_CONFIRM = 35,
    IRCMATCH_MSG_MEMBERS_OK = 36,
    IRCMATCH_MSG_CONNECT_ALL = 37,
    IRCMATCH_MSG_CONNECT_SHOW_LEADER = 38,
    IRCMATCH_MSG_MEMBER_COUNT = 39,
    IRCMATCH_MSG_FRIEND_REGISTERED = 42,
    IRCMATCH_MSG_FRIEND_UPDATED = 43,
    IRCMATCH_MSG_CONNECT_PLAY_FRIEND = 46,
};

// The sprites' slots
enum {
    IRCMATCH_ACTOR_0,
    IRCMATCH_ACTOR_1,
    IRCMATCH_ACTOR_2,
    IRCMATCH_ACTOR_3,
    IRCMATCH_ACTOR_4,
    IRCMATCH_ACTOR_5,
    IRCMATCH_ACTOR_6,
    IRCMATCH_ACTOR_7,
    IRCMATCH_ACTOR_COUNT,
};

typedef struct IrcBattleMatch IrcBattleMatch;
typedef void (*IrcBattleMatchState)(IrcBattleMatch *work);

// A message of the bank, which is passed by address
typedef struct {
    u32 msgId;
} IrcBattleMatchMsg;

struct IrcBattleMatch {
    // The colors of BG 0's palette, then the same colors backwards, which the screen cycles through
    u16 palettes[18][16];
    u16 paletteFrame;
    HeapID paletteHeapId;
    IrcBattleMatchParam *param;
    IrcBattleMatchState state;
    s32 mode;
    // The mode each player sent, by net ID
    s32 modes[4];
    // The MAC addresses of the infrared players, by net ID
    u8 macs[4][6];
    HeapID heapId;
    BmpWin *messageWindow;
    BmpWin *countWindow;
    u32 unk284;
    MsgData *msgData;
    WordSet *wordSet;
    Font *font;
    StrBuf *strbuf;
    StrBuf *strbufFormatted;
    u32 unk29C[5];
    TCB *vblankTcb;
    ClActUnit *actUnit;
    u32 actorPalette;
    u32 actorChars;
    u32 actorCells;
    u32 unk2C4;
    ClActor *actors[IRCMATCH_ACTOR_COUNT];
    // The players that the network accepted
    u32 acceptedPlayers;
    BOOL enoughPlayers;
    u32 unk2F0[3];
    u32 bgChars;
    u32 unk300;
    u32 windowFrameChars;
    u32 menuBgChars;
    TCB *unk30C;
    u32 unk310;
    u8 unk314[0xc7c - 0x314];
    // Whether the link is up, as the network's callback says
    BOOL linked;
    BOOL messagePending;
    BOOL bannerActive;
    s32 bannerFrames;
    BOOL linkChanged;
    u32 friendResult;
    BOOL linkChangePending;
    s32 linkChangeFrames;
    s32 scrollY;
    BOOL connectDone;
    ButtonMan *buttonMan;
    u32 unkCA8;
    s32 counter;
    u32 currentMsgId;
    void *paletteBackup;
    PrintQueue *printQueue;
    // Whether the menu has only the Back button
    BOOL backOnly;
    AppTaskMenu *menu;
    AppTaskMenuRes *menuRes;
    AppTaskMenuItem menuItems[2];
    AppTaskMenuWin *taskWin0;
    AppTaskMenuWin *taskWin1;
    // The member count that the window shows
    s32 shownCount;
    BOOL palettePulse;
    u8 personBuffers[4][0x100];
    JoinAvenuePerson person;
};

static BOOL IrcBattleMatch_IsSameGame(int a, int b);
static void *IrcBattleMatch_GetBeaconData(void *work);
static int IrcBattleMatch_GetBeaconDataSize(void *work);
static void IrcBattleMatch_SetState(IrcBattleMatch *work, IrcBattleMatchState state);
static void IrcBattleMatch_Abort(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitWipe(IrcBattleMatch *work);
static void IrcBattleMatch_FadeSubPalettes(IrcBattleMatch *work, u16 buffer, u32 count);
static void IrcBattleMatch_DimBackground(IrcBattleMatch *work, BOOL dim);
static void IrcBattleMatch_StartWipeOut(IrcBattleMatch *work);
static void IrcBattleMatch_LoadActorResources(IrcBattleMatch *work);
static void IrcBattleMatch_CreateActor(IrcBattleMatch *work, s16 x, s16 y, int slot, int sequence);
static void IrcBattleMatch_MoveActor(IrcBattleMatch *work, int dx, int dy, int slot);
static void IrcBattleMatch_SetActorPos(IrcBattleMatch *work, s16 x, s16 y, int slot);
static void IrcBattleMatch_OnNegotiated(void *work_, int netId);
static void IrcBattleMatch_OnConnectDone(void *work_);
static void IrcBattleMatch_RecvMode(int netId, int size, const void *data, void *work_, NetHandle *handle);
static void IrcBattleMatch_RecvPlayerInfo(int netId, int size, const void *data, void *work_, NetHandle *handle);
static void IrcBattleMatch_RecvParty(int netId, int size, const void *data, void *work_, NetHandle *handle);
static void *IrcBattleMatch_GetPartyBuffer(int netId, void *work_, int size);
static void *IrcBattleMatch_GetPersonBuffer(int netId, void *work_, int size);
static void IrcBattleMatch_RecvFriendData(int netId, int size, const void *data, void *work_, NetHandle *handle);
static void IrcBattleMatch_RecvMac(int netId, int size, const void *data, void *work_, NetHandle *handle);
static void IrcBattleMatch_RecvOrder(int netId, int size, const void *data, void *work_, NetHandle *handle);
static void IrcBattleMatch_RecvJoinAvenuePerson(int netId, int size, const void *data, void *work_, NetHandle *handle);
static void IrcBattleMatch_StateWaitBack(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitJingle(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitResultSync(IrcBattleMatch *work);
static void IrcBattleMatch_StateShowFriendResult(IrcBattleMatch *work);
static int IrcBattleMatch_GetUsablePartySize(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitPartySync(IrcBattleMatch *work);
static void IrcBattleMatch_StateSendParty(IrcBattleMatch *work);
static void IrcBattleMatch_StateSendOrder(IrcBattleMatch *work);
static void IrcBattleMatch_StateSendMac(IrcBattleMatch *work);
static void IrcBattleMatch_StateSendFriendData(IrcBattleMatch *work);
static void IrcBattleMatch_StateCheckVersions(IrcBattleMatch *work);
static void IrcBattleMatch_StateSendPlayerInfo(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitModeSync(IrcBattleMatch *work);
static void IrcBattleMatch_StateReshowMessage(IrcBattleMatch *work);
static void IrcBattleMatch_StateQuitPromptAnswer(IrcBattleMatch *work);
static void IrcBattleMatch_StateShowQuitPrompt(IrcBattleMatch *work);
static void IrcBattleMatch_StateSendJoinAvenue(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitJoinAvenueSync(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitStart(IrcBattleMatch *work);
static void IrcBattleMatch_StateRegisterCommands(IrcBattleMatch *work);
static void IrcBattleMatch_OnMacsKnown(void *work_);
static void IrcBattleMatch_OnLinkChanged(void *work_, BOOL linked);
static void IrcBattleMatch_SetupBGs(IrcBattleMatch *work);
static void IrcBattleMatch_VBlank(TCB *tcb, void *data);
static void IrcBattleMatch_CreateYesNoMenu(IrcBattleMatch *work);
static void IrcBattleMatch_CreateBackMenu(IrcBattleMatch *work, BOOL withConfirm);
static void IrcBattleMatch_ShowMessage(IrcBattleMatchMsg *msg, IrcBattleMatch *work);
static void IrcBattleMatch_ShowFriendCount(u32 msgId, IrcBattleMatch *work);
static void IrcBattleMatch_ShowMemberCount(u32 msgId, IrcBattleMatch *work);
static void IrcBattleMatch_FreeWindows(IrcBattleMatch *work);
static void IrcBattleMatch_InitGraphics(IrcBattleMatch *work);
static void IrcBattleMatch_FreeGraphics(IrcBattleMatch *work);
static void IrcBattleMatch_FreeActor(IrcBattleMatch *work, int slot);
static void IrcBattleMatch_ShowConnectMessage(IrcBattleMatch *work);
static void IrcBattleMatch_InitScreen(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitFade(IrcBattleMatch *work);
static BOOL IrcBattleMatch_IsSameMusicalGame(int a, int b);
static void IrcBattleMatch_StateStartNet(IrcBattleMatch *work);
static void IrcBattleMatch_FreeScreen(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitNet(IrcBattleMatch *work);
static void IrcBattleMatch_StateRestoreConnect(IrcBattleMatch *work);
static void IrcBattleMatch_StateQuitAnswerConnect(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitBackWaiting(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitMembers(IrcBattleMatch *work);
static void IrcBattleMatch_StateShowWaitMenu(IrcBattleMatch *work);
static void IrcBattleMatch_StateRestoreAfterConfirm(IrcBattleMatch *work);
static void IrcBattleMatch_StateConfirmAnswer(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitMismatchTouch(IrcBattleMatch *work);
static void IrcBattleMatch_StateShowQuitPromptConnect(IrcBattleMatch *work);
static void IrcBattleMatch_StateShowConfirmPrompt(IrcBattleMatch *work);
static void IrcBattleMatch_StateWaitFriendFull(IrcBattleMatch *work);
static void IrcBattleMatch_StateShowWaitMessage(IrcBattleMatch *work);
static void IrcBattleMatch_StateQuitAnswerWaiting(IrcBattleMatch *work);
static void IrcBattleMatch_StateShowQuitPromptWaiting(IrcBattleMatch *work);
static void IrcBattleMatch_OnLeaderAnimEnd(u32 param, fx32 frame);
static void IrcBattleMatch_OnPartnerAnimEnd(u32 param, fx32 frame);
static void IrcBattleMatch_StateShowMismatch(IrcBattleMatch *work);
static void IrcBattleMatch_UpdateLinkChange(IrcBattleMatch *work);
static void IrcBattleMatch_StateConnectLoop(IrcBattleMatch *work);
static BOOL IrcBattleMatch_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcBattleMatch_Main(GameProc *proc, u32 *state, void *param, void *work);
static BOOL IrcBattleMatch_Exit(GameProc *proc, u32 *state, void *param, void *work);

// What the players' beacons say, which is the same for every mode
static u32 sIrcBattleMatchBeaconData = 11;

static const NetCommand sIrcBattleMatchCommands[IRCMATCH_CMD_COUNT] = {
    { IrcBattleMatch_RecvMode, NULL },
    { IrcBattleMatch_RecvPlayerInfo, NULL },
    { IrcBattleMatch_RecvParty, IrcBattleMatch_GetPartyBuffer },
    { IrcBattleMatch_RecvFriendData, NULL },
    { IrcBattleMatch_RecvMac, NULL },
    { IrcBattleMatch_RecvOrder, NULL },
    { IrcBattleMatch_RecvJoinAvenuePerson, IrcBattleMatch_GetPersonBuffer },
};

static GFLNetInitData sIrcBattleMatchNetInit = {
    NULL,
    0,
    NULL,
    IrcBattleMatch_OnNegotiated,
    NULL,
    NULL,
    IrcBattleMatch_GetBeaconData,
    IrcBattleMatch_GetBeaconDataSize,
    IrcBattleMatch_IsSameGame,
    NULL,
    NULL,
    NULL,
    NULL,
    { 0 },
    NULL,
    NULL,
    0,
    { 1, 0, 0, 0, 0x80, 0x13, 0, 0 },
    HEAPID_USER,
    0xd,
    0xf,
    0x10,
    0xf0,
    0,
    2,
    0x64,
    0x10,
    1,
    0,
    3,
    1,
    0xb,
    { 0xfe, 0xff, 0, 0 },
    0,
    0,
};

static void *IrcBattleMatch_GetBeaconData(void *work) {
    return &sIrcBattleMatchBeaconData;
}

static int IrcBattleMatch_GetBeaconDataSize(void *work) {
    return sizeof(sIrcBattleMatchBeaconData);
}

static BOOL IrcBattleMatch_IsSameGame(int a, int b) {
    return a == b;
}

static void IrcBattleMatch_SetState(IrcBattleMatch *work, IrcBattleMatchState state) {
    work->state = state;
}

// Ends the screen as quit, and the network with it
static void IrcBattleMatch_Abort(IrcBattleMatch *work) {
    work->param->result = IRCBATTLEMATCH_RESULT_QUIT;
    func_020429f0();
    func_02042860(NULL);
}

static void IrcBattleMatch_StateWaitWipe(IrcBattleMatch *work) {
    if (GFL_WipeIsFinished()) {
        IrcBattleMatch_SetState(work, NULL);
    }
}

// Dims count palettes of a palette memory of the sub screen
static void IrcBattleMatch_FadeSubPalettes(IrcBattleMatch *work, u16 buffer, u32 count) {
    PaletteFade *fade = PaletteFade_Create(work->heapId);
    u32 size;
    u16 *faded;

    PaletteFade_AllocBuffer(fade, buffer, 0x200, work->heapId);
    size = count * 32;
    PaletteFade_LoadFromVRAM(fade, buffer, 0, size);
    PaletteFade_BlendBuffer(fade, buffer, 0, count * 16, 6, 0);
    faded = PaletteFade_GetFadedBuffer(fade, buffer);
    cp15_flushDC(faded, size);
    switch (buffer) {
    case PALFADE_BUFFER_SUB_OBJ:
        gfxUploadStdPaletteObjB(faded, 0, size);
        break;
    case PALFADE_BUFFER_SUB_BG:
        gfxUploadStdPaletteBGB(faded, 0, size);
        break;
    }
    PaletteFade_FreeBuffer(fade, buffer);
    PaletteFade_Free(fade);
}

// Dims the sub screen's BG palette behind a menu, or brings it back
static void IrcBattleMatch_DimBackground(IrcBattleMatch *work, BOOL dim) {
    if (dim) {
        sys_memcpy((void *)HW_DB_BG_PLTT, work->paletteBackup, 0x200);
        IrcBattleMatch_FadeSubPalettes(work, PALFADE_BUFFER_SUB_BG, 9);
    } else {
        gfxUploadStdPaletteBGB(work->paletteBackup, 0, 0x120);
    }
}

static void IrcBattleMatch_StartWipeOut(IrcBattleMatch *work) {
    GFL_WipeSet(0, 0, 0, 0, 6, 1, work->heapId);
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitWipe);
}

static void IrcBattleMatch_LoadActorResources(IrcBattleMatch *work) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(IRCMATCH_ARCID, work->heapId);

    work->actorChars = func_0204b81c(arc, 7, FALSE, CLACT_VRAM_MAIN, work->heapId);
    work->actorPalette = func_0204bbb8(arc, 3, CLACT_VRAM_MAIN, 0, 0, 3, work->heapId);
    work->actorCells = func_0204bde0(arc, 9, 15, work->heapId);
    GFL_ArcToolFree(arc);
}

static void IrcBattleMatch_CreateActor(IrcBattleMatch *work, s16 x, s16 y, int slot, int sequence) {
    ClActorSetup setup;

    if (work->actors[slot] == NULL) {
        setup.x = x;
        setup.y = y;
        setup.sequence = sequence;
        setup.priority = slot;
        setup.bgPriority = 2;
        work->actors[slot] = func_0204c040(work->actUnit, work->actorChars, work->actorPalette, work->actorCells,
                                           &setup, CLACT_SURFACE_MAIN, work->heapId);
        func_0204c520(work->actors[slot], TRUE);
        func_0204c124(work->actors[slot], TRUE);
        func_0204c244(work->actors[slot], 2);
    }
}

static void IrcBattleMatch_MoveActor(IrcBattleMatch *work, int dx, int dy, int slot) {
    ClActorPos pos;

    if (work->actors[slot] != NULL) {
        func_0204c178(work->actors[slot], &pos, CLACT_SURFACE_MAIN);
        pos.x += (s16)dx;
        pos.y += (s16)dy;
        func_0204c140(work->actors[slot], &pos, CLACT_SURFACE_MAIN);
    }
}

static void IrcBattleMatch_SetActorPos(IrcBattleMatch *work, s16 x, s16 y, int slot) {
    ClActorPos pos;

    if (work->actors[slot] != NULL) {
        pos.x = x;
        pos.y = y;
        func_0204c140(work->actors[slot], &pos, CLACT_SURFACE_MAIN);
    }
}

// Called when the network accepts a player
static void IrcBattleMatch_OnNegotiated(void *work_, int netId) {
    IrcBattleMatch *work = work_;

    work->acceptedPlayers |= 1 << netId;
    if (countOneBits(work->acceptedPlayers) >= 2) {
        work->enoughPlayers = TRUE;
    }
}

// Called when the connection is complete, which starts the banner of the starting message
static void IrcBattleMatch_OnConnectDone(void *work_) {
    IrcBattleMatch *work = work_;

    if (work->mode != IRCBATTLEMATCH_MODE_MULTI) {
        work->messagePending = TRUE;
        work->bannerActive = TRUE;
        work->connectDone = TRUE;
    } else if (work->linked != 0 && work->linkChangeFrames != 0) {
        work->messagePending = TRUE;
        work->bannerActive = TRUE;
    }
}

static void IrcBattleMatch_RecvMode(int netId, int size, const void *data, void *work_, NetHandle *handle) {
    IrcBattleMatch *work = work_;

    if (handle == func_02040440()) {
        work->modes[netId] = *(const u32 *)data;
    }
}

static void IrcBattleMatch_RecvPlayerInfo(int netId, int size, const void *data, void *work_, NetHandle *handle) {
    IrcBattleMatch *work = work_;
    PlayerInfo *info;
    RivalDataSave *rival;

    if (handle == func_02040440()) {
        info = func_02017378(work->param->gameData, netId);
        func_02008b34(data, info);
        rival = getHollow_RivalData(GameData_GetSaveControl(work->param->gameData));
        Misc_AddMetTrainer(rival, getIDAsUInt(info));
    }
}

// The party goes to the buffer that IrcBattleMatch_GetPartyBuffer gave
static void IrcBattleMatch_RecvParty(int netId, int size, const void *data, void *work_, NetHandle *handle) {
}

static void *IrcBattleMatch_GetPartyBuffer(int netId, void *work_, int size) {
    IrcBattleMatch *work = work_;

    if (netId >= 0 && netId < 4) {
        return work->param->parties[netId];
    }
    return NULL;
}

static void *IrcBattleMatch_GetPersonBuffer(int netId, void *work_, int size) {
    IrcBattleMatch *work = work_;

    if (netId >= 0 && netId < 4) {
        return work->personBuffers[netId];
    }
    return NULL;
}

static void IrcBattleMatch_RecvFriendData(int netId, int size, const void *data, void *work_, NetHandle *handle) {
    IrcBattleMatch *work = work_;
    GameData *gameData = work->param->gameData;

    if (handle == func_02040440()) {
        if (netId != func_0203ffc4()) {
            PlayerInfo *info = func_02017378(gameData, netId);

            work->friendResult = func_02011b30(gameData, info, data, work->heapId);
        }
    }
}

// Finds which machine the MAC address is, to put its player in order
static void IrcBattleMatch_RecvMac(int netId, int size, const void *data, void *work_, NetHandle *handle) {
    IrcBattleMatch *work = work_;
    int i;

    if (handle == func_02040440()) {
        if (func_02042bc4()) {
            for (i = 0; i < 4; i++) {
                if (GFL_STD_MemCmp(work->macs[i], data, 6) == 0) {
                    work->param->order[netId] = i;
                    return;
                }
            }
        }
    }
}

static void IrcBattleMatch_RecvOrder(int netId, int size, const void *data, void *work_, NetHandle *handle) {
    IrcBattleMatch *work = work_;

    if (handle == func_02040440()) {
        if (netId != func_0203ffc4()) {
            sys_memcpy(data, work->param->order, sizeof(work->param->order));
        }
    }
}

static void IrcBattleMatch_RecvJoinAvenuePerson(int netId, int size, const void *data, void *work_, NetHandle *handle) {
    IrcBattleMatch *work = work_;
    GameData *gameData = work->param->gameData;
    JoinAvenueSave *joinAvenue = SaveControl_GetJoinAvenue(GameData_GetSaveControl(gameData));

    if (handle == func_02040440()) {
        if (netId != func_0203ffc4()) {
            JoinAvenuePerson_SetParam((JoinAvenuePerson *)data, JOIN_AVE_PARAM_UNK_26, 4);
            func_02010078(joinAvenue, gameData, (void *)data, 1);
        }
    }
}

static void IrcBattleMatch_StateWaitBack(IrcBattleMatch *work) {
    if (AppTaskMenu_IsFlashFinished(work->menu)) {
        IrcBattleMatch_Abort(work);
        IrcBattleMatch_SetState(work, IrcBattleMatch_StartWipeOut);
    }
}

static void IrcBattleMatch_StateWaitJingle(IrcBattleMatch *work) {
    work->counter++;
    if (!GFL_SndPlayerIsActiveAny() && work->counter > 60) {
        IrcBattleMatch_CreateBackMenu(work, FALSE);
        func_02042860(NULL);
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitBack);
    }
}

static void IrcBattleMatch_StateWaitResultSync(IrcBattleMatch *work) {
    if (func_02040664(func_02040440(), IRCMATCH_TIMING_RESULT, IRCMATCH_TIMING_GROUP)) {
        func_02042e94(0);
        func_02042e9c(0);
        work->counter = 0;
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitJingle);
    }
}

// Shows whether the friend was registered or updated
static void IrcBattleMatch_StateShowFriendResult(IrcBattleMatch *work) {
    IrcBattleMatchMsg *msg;

    if (work->friendResult != 0) {
        IrcBattleMatchMsg registered = { IRCMATCH_MSG_FRIEND_REGISTERED };

        msg = &registered;
    } else {
        IrcBattleMatchMsg updated = { IRCMATCH_MSG_FRIEND_UPDATED };

        msg = &updated;
    }
    IrcBattleMatch_ShowMessage(msg, work);
    IrcBattleMatch_ShowFriendCount(IRCMATCH_MSG_FRIEND_COUNT, work);
    GFL_SndSEPlay(SEQ_SE_FLD_124);
    func_02040624(func_02040440(), IRCMATCH_TIMING_RESULT, IRCMATCH_TIMING_GROUP);
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitResultSync);
}

// How many Pokémon the smaller of the first two parties can battle with
static int IrcBattleMatch_GetUsablePartySize(IrcBattleMatch *work) {
    int a = howManyPokesAreAbleToFight(work->param->parties[0]);
    int b = howManyPokesAreAbleToFight(work->param->parties[1]);

    if (a > b) {
        return b;
    }
    return a;
}

static void IrcBattleMatch_StateWaitPartySync(IrcBattleMatch *work) {
    if (func_02040664(func_02040440(), IRCMATCH_TIMING_PARTY, IRCMATCH_TIMING_GROUP)) {
        switch (work->mode) {
        case IRCBATTLEMATCH_MODE_FRIEND:
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateShowFriendResult);
            return;
        case IRCBATTLEMATCH_MODE_SINGLE:
        case IRCBATTLEMATCH_MODE_DOUBLE:
        case IRCBATTLEMATCH_MODE_TRIPLE:
        case IRCBATTLEMATCH_MODE_ROTATION:
            PokeParty_RecoverAll(work->param->parties[0]);
            PokeParty_RecoverAll(work->param->parties[1]);
            switch (IrcBattleMatch_GetUsablePartySize(work)) {
            case 0:
                break;
            case 1:
                work->param->result = IRCBATTLEMATCH_MODE_SINGLE;
                break;
            case 2:
                if (work->param->result > IRCBATTLEMATCH_MODE_DOUBLE) {
                    work->param->result = IRCBATTLEMATCH_MODE_DOUBLE;
                }
                break;
            }
            IrcBattleMatch_SetState(work, IrcBattleMatch_StartWipeOut);
            break;
        }
        IrcBattleMatch_SetState(work, IrcBattleMatch_StartWipeOut);
    }
}

static void IrcBattleMatch_StateSendParty(IrcBattleMatch *work) {
    if (func_02040664(func_02040440(), IRCMATCH_TIMING_ORDER, IRCMATCH_TIMING_GROUP)) {
        NetHandle *handle = func_02040440();

        if (func_02042c18(handle, 0xff, IRCMATCH_CMD_PARTY, PokeParty_GetSaveDataSize(), work->param->party, 0, FALSE,
                          TRUE)) {
            func_02040624(func_02040440(), IRCMATCH_TIMING_PARTY, IRCMATCH_TIMING_GROUP);
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitPartySync);
        }
    }
}

static void IrcBattleMatch_StateSendOrder(IrcBattleMatch *work) {
    if (func_02040664(func_02040440(), IRCMATCH_TIMING_MAC, IRCMATCH_TIMING_GROUP)) {
        if (func_02042bc4()) {
            if (func_02042be8(func_02040440(), IRCMATCH_CMD_ORDER, sizeof(work->param->order), work->param->order)) {
                func_02040624(func_02040440(), IRCMATCH_TIMING_ORDER, IRCMATCH_TIMING_GROUP);
                IrcBattleMatch_SetState(work, IrcBattleMatch_StateSendParty);
            }
        } else {
            func_02040624(func_02040440(), IRCMATCH_TIMING_ORDER, IRCMATCH_TIMING_GROUP);
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateSendParty);
        }
    }
}

static void IrcBattleMatch_StateSendMac(IrcBattleMatch *work) {
    u8 mac[6];

    if (func_02040664(func_02040440(), IRCMATCH_TIMING_FRIEND_DATA, IRCMATCH_TIMING_GROUP)) {
        switch (work->mode) {
        case IRCBATTLEMATCH_MODE_MUSICAL_ARG0:
        case IRCBATTLEMATCH_MODE_MUSICAL_ARG1:
        case IRCBATTLEMATCH_MODE_BSUBWAY:
            IrcBattleMatch_SetState(work, IrcBattleMatch_StartWipeOut);
            return;
        }
        func_0207c33c(mac);
        if (func_02042be8(func_02040440(), IRCMATCH_CMD_MAC, sizeof(mac), mac)) {
            func_02040624(func_02040440(), IRCMATCH_TIMING_MAC, IRCMATCH_TIMING_GROUP);
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateSendOrder);
        }
    }
}

static void IrcBattleMatch_StateSendFriendData(IrcBattleMatch *work) {
    DWCFriendData data;

    if (work->mode == IRCBATTLEMATCH_MODE_FRIEND) {
        func_0200a3c4(&GameData_GetWifiList(work->param->gameData)->myUserData, &data);
        if (func_02042be8(func_02040440(), IRCMATCH_CMD_FRIEND_DATA, sizeof(data), &data)) {
            func_02040624(func_02040440(), IRCMATCH_TIMING_FRIEND_DATA, IRCMATCH_TIMING_GROUP);
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateSendMac);
            if (work->menu != NULL) {
                AppTaskMenu_Free(work->menu);
                work->menu = NULL;
            }
        }
    } else {
        func_02040624(func_02040440(), IRCMATCH_TIMING_FRIEND_DATA, IRCMATCH_TIMING_GROUP);
        if (work->menu != NULL) {
            AppTaskMenu_Free(work->menu);
            work->menu = NULL;
        }
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateSendMac);
    }
}

// Goes on to the Join Avenue exchange when everyone plays this game
static void IrcBattleMatch_StateCheckVersions(IrcBattleMatch *work) {
    int count;
    int i;
    BOOL allSameGame;

    if (func_02040664(func_02040440(), IRCMATCH_TIMING_PLAYER_INFO, IRCMATCH_TIMING_GROUP)) {
        count = func_02042a78();
        allSameGame = TRUE;
        if (work->mode == IRCBATTLEMATCH_MODE_MUSICAL_ARG0 || work->mode == IRCBATTLEMATCH_MODE_MUSICAL_ARG1) {
            allSameGame = FALSE;
        }
        for (i = 0; i < count; i++) {
            PlayerInfo *info = func_02017378(work->param->gameData, i);

            if (func_02008bfc(info) != VERSION_WHITE2 && func_02008bfc(info) != VERSION_BLACK2) {
                allSameGame = FALSE;
            }
        }
        if (allSameGame) {
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateSendJoinAvenue);
            return;
        }
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateSendFriendData);
    }
}

static void IrcBattleMatch_StateSendPlayerInfo(IrcBattleMatch *work) {
    NetHandle *handle = func_02040440();
    u32 size = PlayerInfo_GetSize();

    if (func_02042be8(handle, IRCMATCH_CMD_PLAYER_INFO, size, GetGameDataPlayerInfo(work->param->gameData))) {
        func_02040624(func_02040440(), IRCMATCH_TIMING_PLAYER_INFO, IRCMATCH_TIMING_GROUP);
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateCheckVersions);
    }
}

// The battles go on with the lowest mode of the two players
static void IrcBattleMatch_StateWaitModeSync(IrcBattleMatch *work) {
    u16 netId;

    if (func_02040664(func_02040440(), IRCMATCH_TIMING_MODE, IRCMATCH_TIMING_GROUP)) {
        netId = func_0203ffc4();
        switch (work->mode) {
        case IRCBATTLEMATCH_MODE_SINGLE:
        case IRCBATTLEMATCH_MODE_DOUBLE:
        case IRCBATTLEMATCH_MODE_TRIPLE:
        case IRCBATTLEMATCH_MODE_ROTATION:
            if (work->mode > work->modes[1 - netId]) {
                work->param->result = work->modes[1 - netId];
            }
            break;
        }
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateSendPlayerInfo);
    }
}

// Shows the message that was on the screen again, with a Back button
static void IrcBattleMatch_StateReshowMessage(IrcBattleMatch *work) {
    IrcBattleMatchMsg msg;

    IrcBattleMatch_FreeWindows(work);
    msg.msgId = work->currentMsgId;
    IrcBattleMatch_ShowMessage(&msg, work);
    IrcBattleMatch_CreateBackMenu(work, FALSE);
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitStart);
}

static void IrcBattleMatch_StateQuitPromptAnswer(IrcBattleMatch *work) {
    if (AppTaskMenu_IsFlashFinished(work->menu)) {
        u8 pos = AppTaskMenu_GetCursorPos(work->menu);

        AppTaskMenu_Free(work->menu);
        work->menu = NULL;
        GFL_BGSysQueueScrLoad(6);
        if (pos == 0) {
            IrcBattleMatch_FreeWindows(work);
            IrcBattleMatch_Abort(work);
            IrcBattleMatch_SetState(work, IrcBattleMatch_StartWipeOut);
        } else {
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateReshowMessage);
        }
        IrcBattleMatch_DimBackground(work, FALSE);
    }
}

static void IrcBattleMatch_StateShowQuitPrompt(IrcBattleMatch *work) {
    IrcBattleMatchMsg msg = { IRCMATCH_MSG_QUIT_CONFIRM };
    u32 currentMsgId = work->currentMsgId;

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x180, 0x20, work->heapId);
    IrcBattleMatch_ShowMessage(&msg, work);
    work->currentMsgId = currentMsgId;
    IrcBattleMatch_CreateYesNoMenu(work);
    func_020232d8();
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateQuitPromptAnswer);
}

static void IrcBattleMatch_StateSendJoinAvenue(IrcBattleMatch *work) {
    func_02036dc0(&work->person, work->param->gameData);
    if (func_02042c18(func_02040440(), 0xff, IRCMATCH_CMD_JOIN_AVENUE, sizeof(work->person), &work->person, 1, FALSE,
                      TRUE)) {
        func_02040624(func_02040440(), IRCMATCH_TIMING_JOIN_AVENUE, IRCMATCH_TIMING_GROUP);
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitJoinAvenueSync);
    }
}

static void IrcBattleMatch_StateWaitJoinAvenueSync(IrcBattleMatch *work) {
    if (func_02040664(func_02040440(), IRCMATCH_TIMING_JOIN_AVENUE, IRCMATCH_TIMING_GROUP)) {
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateSendFriendData);
    }
}

static void IrcBattleMatch_StateWaitStart(IrcBattleMatch *work) {
    if (work->menu != NULL) {
        if (AppTaskMenu_IsFlashFinished(work->menu)) {
            AppTaskMenu_GetCursorPos(work->menu);
            AppTaskMenu_Free(work->menu);
            work->menu = NULL;
            GFL_BGSysQueueScrLoad(6);
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateShowQuitPrompt);
            return;
        }
    } else {
        IrcBattleMatch_CreateBackMenu(work, FALSE);
        return;
    }
    if (func_02040664(func_02040440(), IRCMATCH_TIMING_COMMANDS, IRCMATCH_TIMING_GROUP)) {
        if (func_02042be8(func_02040440(), IRCMATCH_CMD_MODE, sizeof(work->mode), &work->mode)) {
            func_02040624(func_02040440(), IRCMATCH_TIMING_MODE, IRCMATCH_TIMING_GROUP);
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitModeSync);
        }
    }
}

static void IrcBattleMatch_StateRegisterCommands(IrcBattleMatch *work) {
    func_02040c20(IRCMATCH_CMD_MODE, sIrcBattleMatchCommands, IRCMATCH_CMD_COUNT, work);
    func_02040624(func_02040440(), IRCMATCH_TIMING_COMMANDS, IRCMATCH_TIMING_GROUP);
    func_02042e94(1);
    func_02042e9c(1);
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitStart);
}

// Called when the link is up: collects the MAC addresses of the players
static void IrcBattleMatch_OnMacsKnown(void *work_) {
    IrcBattleMatch *work = work_;
    int i;

    if (func_02042bc4()) {
        for (i = 0; i < 4; i++) {
            func_020429e0(work->macs[i], i);
        }
    }
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateRegisterCommands);
}

// Called when the link goes up or down
static void IrcBattleMatch_OnLinkChanged(void *work_, BOOL linked) {
    IrcBattleMatch *work = work_;

    work->linked = linked;
    work->linkChanged = TRUE;
    work->linkChangePending = TRUE;
    if (work->mode == IRCBATTLEMATCH_MODE_MULTI && linked == FALSE) {
        work->messagePending = TRUE;
        work->bannerActive = TRUE;
    }
}

static const BGSysVRAMConfig sIrcBattleMatchVRAMConfig = {
    GX_VRAM_BG_128_A,  GX_VRAM_BGEXTPLTT_NONE,  GX_VRAM_SUB_BG_128_C,        GX_VRAM_SUB_BGEXTPLTT_NONE,
    GX_VRAM_OBJ_128_B, GX_VRAM_OBJEXTPLTT_NONE, GX_VRAM_SUB_OBJ_128_D,       GX_VRAM_SUB_OBJEXTPLTT_NONE,
    GX_VRAM_TEX_NONE,  GX_VRAM_TEXPLTT_NONE,    GX_OBJVRAMMODE_CHAR_1D_128K, GX_OBJVRAMMODE_CHAR_1D_128K,
};

static const BGSysLCDConfig sIrcBattleMatchLCDConfig = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };

// clang-format off
static void IrcBattleMatch_SetupBGs(IrcBattleMatch *work) {
    GFL_BGSysSetVRAMBanks(&sIrcBattleMatchVRAMConfig);
    GFL_BGSysSetLCDConfig(&sIrcBattleMatchLCDConfig);
    GFL_BGSysSetEnabledBGsA(0);
    GFL_BGSysSetEnabledBGsB(0);
    {
        BGSetup setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xd000),
                          GX_BG_CHARBASE(0x10000), 0x8000, GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE };

        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysFillScrArea(0, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(0);
    }
    {
        BGSetup setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xd800),
                          GX_BG_CHARBASE(0x10000), 0x8000, GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE };

        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysFillScrArea(1, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(1);
    }
    {
        BGSetup setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe000),
                          GX_BG_CHARBASE(0x0), 0x8000, GX_BG_EXTPLTT_01, 3, GX_BG_AREAOVER_XLU, FALSE };

        GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
        GFL_BGSysFillScrArea(4, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
    }
    {
        BGSetup setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xe800),
                          GX_BG_CHARBASE(0x8000), 0x8000, GX_BG_EXTPLTT_01, 2, GX_BG_AREAOVER_XLU, FALSE };

        GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        GFL_BGSysFillScrArea(5, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(5);
    }
    {
        BGSetup setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf000),
                          GX_BG_CHARBASE(0x8000), 0x8000, GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE };

        GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
        GFL_BGSysFillChar(6, 0, 1, 0);
        GFL_BGSysFillScrArea(6, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(6);
    }
    {
        BGSetup setup = { 0, 0, 0x800, 0, BGRES_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE(0xf800),
                          GX_BG_CHARBASE(0x10000), 0x8000, GX_BG_EXTPLTT_01, 0, GX_BG_AREAOVER_XLU, FALSE };

        GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
        GFL_BGSysFillScrArea(7, 0, 0, 0, 32, 32, BGSYS_FILL_TILE_PALETTE);
        GFL_BGSysLoadScr(7);
    }
}
// clang-format on

static void IrcBattleMatch_VBlank(TCB *tcb, void *data) {
    func_0204b7c8();
}

// A Yes and No menu
static void IrcBattleMatch_CreateYesNoMenu(IrcBattleMatch *work) {
    AppTaskMenuInit init;

    init.heapId = work->heapId;
    init.itemCount = 2;
    init.items = work->menuItems;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 13;
    init.width = 13;
    init.height = 3;
    work->menuItems[0].str = GFL_StrBufCreate(100, work->heapId);
    GFL_MsgDataLoadStrbuf(work->msgData, IRCMATCH_MSG_YES, work->menuItems[0].str);
    work->menuItems[0].color = 0x39e3;
    work->menuItems[1].str = GFL_StrBufCreate(100, work->heapId);
    GFL_MsgDataLoadStrbuf(work->msgData, IRCMATCH_MSG_NO, work->menuItems[1].str);
    work->menuItems[1].color = 0x39e3;
    work->menuItems[0].type = 0;
    work->menuItems[1].type = 0;
    work->menu = AppTaskMenu_Create(&init, work->menuRes);
    if (work->mode != IRCBATTLEMATCH_MODE_MUSICAL_ARG0 && work->mode != IRCBATTLEMATCH_MODE_MUSICAL_ARG1 &&
        work->mode != IRCBATTLEMATCH_MODE_BSUBWAY) {
        AppTaskMenu_SetLocked(work->menu, TRUE);
    }
    GFL_StrBufFree(work->menuItems[0].str);
    GFL_StrBufFree(work->menuItems[1].str);
    IrcBattleMatch_DimBackground(work, TRUE);
}

// A Back button, with a Confirm button above it
static void IrcBattleMatch_CreateBackMenu(IrcBattleMatch *work, BOOL withConfirm) {
    AppTaskMenuInit init;

    init.heapId = work->heapId;
    init.posType = APP_TASKMENU_POS_BOTTOM_RIGHT;
    init.x = 32;
    init.y = 24;
    init.width = 13;
    init.height = 3;
    if (withConfirm) {
        init.itemCount = 2;
        init.items = work->menuItems;
        work->menuItems[0].str = GFL_StrBufCreate(100, work->heapId);
        GFL_MsgDataLoadStrbuf(work->msgData, IRCMATCH_MSG_CONFIRM, work->menuItems[0].str);
        work->menuItems[0].color = 0x39e3;
        work->menuItems[0].type = 0;
        work->menuItems[1].str = GFL_StrBufCreate(100, work->heapId);
        GFL_MsgDataLoadStrbuf(work->msgData, IRCMATCH_MSG_BACK, work->menuItems[1].str);
        work->menuItems[1].color = 0x39e3;
        work->menuItems[1].type = APP_TASKMENU_ITEM_RETURN;
        work->menu = AppTaskMenu_Create(&init, work->menuRes);
        GFL_StrBufFree(work->menuItems[0].str);
        GFL_StrBufFree(work->menuItems[1].str);
        work->backOnly = FALSE;
    } else {
        init.itemCount = 1;
        init.items = work->menuItems;
        work->menuItems[0].str = GFL_StrBufCreate(100, work->heapId);
        GFL_MsgDataLoadStrbuf(work->msgData, IRCMATCH_MSG_BACK, work->menuItems[0].str);
        work->menuItems[0].color = 0x39e3;
        work->menuItems[0].type = APP_TASKMENU_ITEM_RETURN;
        work->menu = AppTaskMenu_Create(&init, work->menuRes);
        GFL_StrBufFree(work->menuItems[0].str);
        work->backOnly = TRUE;
    }
    if (work->mode != IRCBATTLEMATCH_MODE_MUSICAL_ARG0 && work->mode != IRCBATTLEMATCH_MODE_MUSICAL_ARG1 &&
        work->mode != IRCBATTLEMATCH_MODE_BSUBWAY) {
        AppTaskMenu_SetLocked(work->menu, TRUE);
    }
}

// Shows a message in the window at the top, which it makes if it has none
static void IrcBattleMatch_ShowMessage(IrcBattleMatchMsg *msg, IrcBattleMatch *work) {
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1a0, 0x20, work->heapId);
    if (work->messageWindow == NULL) {
        work->messageWindow = BmpWin_CreateDynamic(5, 1, 1, 30, 5, 13, 0);
        BmpWin_FlushMap(work->messageWindow);
        BmpWin_DrawFrame(work->messageWindow, 0, CHAR_POS(work->windowFrameChars), 14);
    }
    GFL_BitmapFill(BmpWin_GetBitmap(work->messageWindow), 0xff);
    work->currentMsgId = msg->msgId;
    GFL_MsgDataLoadStrbuf(work->msgData, msg->msgId, work->strbufFormatted);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(work->messageWindow), 4, 4, work->strbufFormatted, work->font);
    BmpWin_FlushChar(work->messageWindow);
}

// Shows how many friends the Friend Roster holds in the window below the message
static void IrcBattleMatch_ShowFriendCount(u32 msgId, IrcBattleMatch *work) {
    int count;

    if (work->countWindow == NULL) {
        work->countWindow = BmpWin_CreateDynamic(5, 1, 8, 18, 4, 13, 0);
        BmpWin_FlushMap(work->countWindow);
        BmpWin_DrawFrame(work->countWindow, 0, CHAR_POS(work->windowFrameChars), 14);
    }
    GFL_BitmapFill(BmpWin_GetBitmap(work->countWindow), 0xff);
    GFL_MsgDataLoadStrbuf(work->msgData, msgId, work->strbuf);
    count = func_0200a150(GameData_GetWifiList(work->param->gameData));
    WordSetNumber(work->wordSet, 0, count, 2, 1, TRUE);
    WordSetNumber(work->wordSet, 1, 32, 2, 1, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbufFormatted, work->strbuf);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(work->countWindow), 0, 2, work->strbufFormatted, work->font);
    BmpWin_FlushChar(work->countWindow);
}

// Shows how many players have joined
static void IrcBattleMatch_ShowMemberCount(u32 msgId, IrcBattleMatch *work) {
    if (work->countWindow == NULL) {
        work->countWindow = BmpWin_CreateDynamic(5, 1, 8, 23, 4, 13, 0);
    }
    GFL_BitmapFill(BmpWin_GetBitmap(work->countWindow), 0xff);
    BmpWin_FlushMap(work->countWindow);
    BmpWin_FlushChar(work->countWindow);
    BmpWin_DrawFrame(work->countWindow, 0, CHAR_POS(work->windowFrameChars), 14);
    GFL_MsgDataLoadStrbuf(work->msgData, msgId, work->strbuf);
    WordSetNumber(work->wordSet, 0, func_02042a48(), 1, 0, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strbufFormatted, work->strbuf);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(work->countWindow), 0, 2, work->strbufFormatted, work->font);
    BmpWin_FlushChar(work->countWindow);
}

static void IrcBattleMatch_FreeWindows(IrcBattleMatch *work) {
    if (work->messageWindow != NULL) {
        BmpWin_Free(work->messageWindow);
    }
    if (work->countWindow != NULL) {
        BmpWin_Free(work->countWindow);
    }
    work->messageWindow = NULL;
    work->countWindow = NULL;
}

static void IrcBattleMatch_InitGraphics(IrcBattleMatch *work) {
    GFL_BGSysCreate(work->heapId);
    BmpWin_InitAllocator(work->heapId);
    func_020232d0();
}

static void IrcBattleMatch_FreeGraphics(IrcBattleMatch *work) {
    GFL_BGSysFree();
    BmpWin_FreeAllocator();
}

static void IrcBattleMatch_FreeActor(IrcBattleMatch *work, int slot) {
    if (work->actors[slot] != NULL) {
        func_0204c108(work->actors[slot]);
        work->actors[slot] = NULL;
    }
}

// Shows what to do for the mode
static void IrcBattleMatch_ShowConnectMessage(IrcBattleMatch *work) {
    if (work->connectDone == TRUE) {
        IrcBattleMatchMsg msg = { IRCMATCH_MSG_CONNECT_LEADER };

        IrcBattleMatch_ShowMessage(&msg, work);
    } else if (work->mode == IRCBATTLEMATCH_MODE_FRIEND) {
        if (func_0200a150(GameData_GetWifiList(work->param->gameData)) == 32) {
            IrcBattleMatchMsg msg = { IRCMATCH_MSG_FRIEND_ROSTER_FULL };

            IrcBattleMatch_ShowMessage(&msg, work);
        } else {
            IrcBattleMatchMsg msg = { IRCMATCH_MSG_CONNECT_FRIEND_CODE };

            IrcBattleMatch_ShowMessage(&msg, work);
            IrcBattleMatch_ShowFriendCount(IRCMATCH_MSG_FRIEND_COUNT, work);
        }
    } else if (work->mode == IRCBATTLEMATCH_MODE_TRADE) {
        IrcBattleMatchMsg msg = { IRCMATCH_MSG_CONNECT_TRADE_PARTNER };

        IrcBattleMatch_ShowMessage(&msg, work);
    } else if (work->mode == IRCBATTLEMATCH_MODE_MULTI) {
        IrcBattleMatchMsg msg = { IRCMATCH_MSG_CONNECT_PARTNER };

        IrcBattleMatch_ShowMessage(&msg, work);
    } else if (work->mode == IRCBATTLEMATCH_MODE_MUSICAL_ARG0) {
        IrcBattleMatchMsg msg = { IRCMATCH_MSG_CONNECT_ALL };

        IrcBattleMatch_ShowMessage(&msg, work);
        IrcBattleMatch_ShowMemberCount(IRCMATCH_MSG_MEMBER_COUNT, work);
    } else if (work->mode == IRCBATTLEMATCH_MODE_MUSICAL_ARG1) {
        IrcBattleMatchMsg msg = { IRCMATCH_MSG_CONNECT_SHOW_LEADER };

        IrcBattleMatch_ShowMessage(&msg, work);
    } else if (work->mode == IRCBATTLEMATCH_MODE_BSUBWAY) {
        IrcBattleMatchMsg msg = { IRCMATCH_MSG_CONNECT_PLAY_FRIEND };

        IrcBattleMatch_ShowMessage(&msg, work);
    } else {
        IrcBattleMatchMsg msg = { IRCMATCH_MSG_CONNECT_OPPONENTS };

        IrcBattleMatch_ShowMessage(&msg, work);
    }
}

static void IrcBattleMatch_InitScreen(IrcBattleMatch *work) {
    ArcTool *arc;
    int i;
    void *paletteFile;
    NNSG2dPaletteData *palette;

    func_020232d8();
    work->wordSet = GFL_WordSetSystemCreate(11, 200, work->heapId);
    IrcBattleMatch_InitGraphics(work);
    IrcBattleMatch_SetupBGs(work);
    work->strbufFormatted = GFL_StrBufCreate(200, work->heapId);
    work->strbuf = GFL_StrBufCreate(200, work->heapId);
    work->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, work->heapId);
    work->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_STARTMENU_2, work->heapId);
    arc = GFL_ArcSysCreateFileHandle(IRCMATCH_ARCID, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 0, 0, 0, work->heapId);
    work->bgChars = GFL_BGSysLoadArcNCGRDynamic(arc, 6, 0, 0, FALSE, work->heapId);
    if (work->mode != IRCBATTLEMATCH_MODE_MULTI) {
        GFL_G2DIOLoadNSCRSync(arc, 13, 1, 0, CHAR_POS(work->bgChars), 0, FALSE, work->heapId);
    } else {
        GFL_G2DIOLoadNSCRSync(arc, 13, 0, 0, CHAR_POS(work->bgChars), 0, FALSE, work->heapId);
        GFL_G2DIOLoadNSCRSync(arc, 14, 1, 0, CHAR_POS(work->bgChars), 0, FALSE, work->heapId);
    }
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(ARCID_GSYNC_MENU, work->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 4, 0, 0, work->heapId);
    work->menuBgChars = GFL_BGSysLoadArcNCGRDynamic(arc, 4, 4, 0, FALSE, work->heapId);
    GFL_G2DIOLoadNSCRSync(arc, 9, 4, 0, CHAR_POS(work->menuBgChars), 0, FALSE, work->heapId);
    GFL_ArcToolFree(arc);
    work->windowFrameChars = LoadCursorImageEndOfHeap(5, 14, 0, work->heapId);
    GFL_FadeSet(3, 16, 0, 2);
    IrcBattleMatch_ShowConnectMessage(work);
    ClActSys_Create(&data_02093f08, &sIrcBattleMatchVRAMConfig, work->heapId);
    work->actUnit = func_0204bf1c(40, 0, work->heapId);
    work->vblankTcb = GFL_VBlankTCBAdd(IrcBattleMatch_VBlank, work, 0);
    work->printQueue = func_02021998(work->heapId);
    work->menuRes = AppTaskMenuRes_Create(6, 9, work->font, work->printQueue, work->heapId);
    IrcBattleMatch_LoadActorResources(work);
    if (work->mode != IRCBATTLEMATCH_MODE_MULTI) {
        IrcBattleMatch_CreateActor(work, 57, 154, 2, 10);
        IrcBattleMatch_CreateActor(work, 128, 113, 4, 2);
        IrcBattleMatch_CreateActor(work, 200, 38, 0, 11);
        IrcBattleMatch_CreateActor(work, 128, 77, 5, 3);
    } else {
        IrcBattleMatch_CreateActor(work, 57, 154, 0, 4);
        IrcBattleMatch_CreateActor(work, 198, 154, 1, 5);
        IrcBattleMatch_CreateActor(work, 200, 38, 2, 6);
        IrcBattleMatch_CreateActor(work, 55, 38, 3, 7);
        IrcBattleMatch_CreateActor(work, 152, 146, 6, 0);
        IrcBattleMatch_CreateActor(work, 103, 146, 7, 1);
        IrcBattleMatch_CreateActor(work, 152, 46, 4, 0);
        IrcBattleMatch_CreateActor(work, 103, 46, 5, 1);
    }
    GFL_BGSysSetEnabledBGsA(0x13);
    GFL_BGSysSetEnabledBGsB(0x17);
    work->paletteHeapId = work->heapId;
    arc = GFL_ArcSysCreateFileHandle(IRCMATCH_ARCID, work->paletteHeapId);
    paletteFile = GFL_G2DIOReadNCLRArc(arc, 2, &palette, work->paletteHeapId);
    sys_memcpy(palette->rawData, work->palettes, 0x120);
    for (i = 0; i < 9; i++) {
        sys_memcpy(work->palettes[8 - i], work->palettes[9 + i], 0x20);
    }
    GFL_HeapFree(paletteFile);
    GFL_ArcToolFree(arc);
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitFade);
}

static void IrcBattleMatch_StateWaitFade(IrcBattleMatch *work) {
    if (GFL_FadeIsRunning()) {
        if (work->mode == IRCBATTLEMATCH_MODE_FRIEND &&
            func_0200a150(GameData_GetWifiList(work->param->gameData)) == 32) {
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitFriendFull);
            work->counter = 180;
            return;
        }
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateStartNet);
    }
}

// Whether the commands' base is that of the musical, as a player of the other musical mode has
static BOOL IrcBattleMatch_IsSameMusicalGame(int a, int b) {
    if (a == 0x13 && b == 0x12) {
        return TRUE;
    }
    if (b == 0x13 && a == 0x12) {
        return TRUE;
    }
    return FALSE;
}

static void IrcBattleMatch_StateStartNet(IrcBattleMatch *work) {
    GFLNetInitData init = sIrcBattleMatchNetInit;

    init.bNetType = 4;
    switch (work->mode) {
    case IRCBATTLEMATCH_MODE_SINGLE:
    case IRCBATTLEMATCH_MODE_DOUBLE:
    case IRCBATTLEMATCH_MODE_TRIPLE:
    case IRCBATTLEMATCH_MODE_ROTATION:
        init.gameCommandBase = 0xc;
        break;
    case IRCBATTLEMATCH_MODE_MULTI:
        init.gameCommandBase = 0x26;
        init.maxConnectNum = 4;
        break;
    case IRCBATTLEMATCH_MODE_FRIEND:
        init.gameCommandBase = 0x1d;
        break;
    case IRCBATTLEMATCH_MODE_TRADE:
        init.gameCommandBase = 0x1c;
        break;
    case IRCBATTLEMATCH_MODE_BSUBWAY:
        init.gameCommandBase = 0x2c;
        break;
    case IRCBATTLEMATCH_MODE_MUSICAL_ARG0:
        init.unk6E = 0x11;
        init.gameCommandBase = 0x13;
        goto musical;
    case IRCBATTLEMATCH_MODE_MUSICAL_ARG1:
        init.unk6E = 0x11;
        init.gameCommandBase = 0x12;
    musical:
        init.bMPMode = TRUE;
        init.maxConnectNum = work->param->musicalNetInit->maxConnectNum;
        init.maxSendSize = work->param->musicalNetInit->maxSendSize;
        init.unk6C = work->param->musicalNetInit->unk6C;
        work->shownCount = -1;
        break;
    }
    func_020425ec(&init, NULL, work);
    if (work->mode == IRCBATTLEMATCH_MODE_MULTI) {
        func_020429d8(0x25);
    } else if (work->mode == IRCBATTLEMATCH_MODE_MUSICAL_ARG0) {
        func_020429e8(IrcBattleMatch_IsSameMusicalGame);
        func_02042a40(0);
        func_020429d8(0x11);
    } else if (work->mode == IRCBATTLEMATCH_MODE_MUSICAL_ARG1) {
        func_020429e8(IrcBattleMatch_IsSameMusicalGame);
        func_02042a40(1);
        func_020429d8(0x11);
    }
    IrcBattleMatch_CreateBackMenu(work, FALSE);
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitNet);
}

static void IrcBattleMatch_FreeScreen(IrcBattleMatch *work) {
    func_020232d8();
    IrcBattleMatch_FreeWindows(work);
    GFL_BGSysFreeFilledChar(5, 1, 0);
    GFL_BGSysFreeCharMemory(5, CHAR_POS(work->windowFrameChars), CHAR_SIZE(work->windowFrameChars));
    GFL_BGSysReleaseBG(5);
    GFL_MsgDataFree(work->msgData);
    GFL_FontFree(work->font);
    GFL_StrBufFree(work->strbuf);
    GFL_StrBufFree(work->strbufFormatted);
    GFL_BGSysSetBGEnabled(5, FALSE);
    AppTaskMenuRes_Free(work->menuRes);
    IrcBattleMatch_FreeGraphics(work);
}

static void IrcBattleMatch_StateWaitNet(IrcBattleMatch *work) {
    if (func_02042788() == TRUE) {
        switch (work->mode) {
        case IRCBATTLEMATCH_MODE_MUSICAL_ARG0:
        case IRCBATTLEMATCH_MODE_MUSICAL_ARG1:
            NetDevIrc_SetPeerFilter(IrcBattleMatch_IsSameMusicalGame);
            break;
        }
        func_020429a8(IrcBattleMatch_OnMacsKnown, IrcBattleMatch_OnLinkChanged, IrcBattleMatch_OnConnectDone);
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateConnectLoop);
    }
}

// Shows the message that was on the screen again, after the quit prompt in the connecting screen
static void IrcBattleMatch_StateRestoreConnect(IrcBattleMatch *work) {
    IrcBattleMatchMsg msg;

    IrcBattleMatch_FreeWindows(work);
    msg.msgId = work->currentMsgId;
    IrcBattleMatch_ShowMessage(&msg, work);
    if (work->mode == IRCBATTLEMATCH_MODE_MUSICAL_ARG0 && work->shownCount > 0) {
        IrcBattleMatch_CreateBackMenu(work, TRUE);
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateConnectLoop);
        return;
    }
    IrcBattleMatch_CreateBackMenu(work, FALSE);
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateConnectLoop);
}

static void IrcBattleMatch_StateQuitAnswerConnect(IrcBattleMatch *work) {
    if (AppTaskMenu_IsFlashFinished(work->menu)) {
        u8 pos = AppTaskMenu_GetCursorPos(work->menu);

        AppTaskMenu_Free(work->menu);
        work->menu = NULL;
        GFL_BGSysQueueScrLoad(6);
        if (pos == 0) {
            IrcBattleMatch_FreeWindows(work);
            IrcBattleMatch_Abort(work);
            IrcBattleMatch_SetState(work, IrcBattleMatch_StartWipeOut);
        } else {
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateRestoreConnect);
        }
        IrcBattleMatch_DimBackground(work, FALSE);
    }
}

static void IrcBattleMatch_StateWaitBackWaiting(IrcBattleMatch *work) {
    if (AppTaskMenu_IsFlashFinished(work->menu)) {
        AppTaskMenu_GetCursorPos(work->menu);
        AppTaskMenu_Free(work->menu);
        work->menu = NULL;
        GFL_BGSysQueueScrLoad(6);
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateShowQuitPromptWaiting);
    }
}

static void IrcBattleMatch_StateWaitMembers(IrcBattleMatch *work) {
    if (func_02042a38()) {
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitBackWaiting);
        return;
    }
    if (AppTaskMenu_IsFlashFinished(work->menu)) {
        AppTaskMenu_GetCursorPos(work->menu);
        AppTaskMenu_Free(work->menu);
        work->menu = NULL;
        GFL_BGSysQueueScrLoad(6);
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateShowQuitPromptWaiting);
    }
}

static void IrcBattleMatch_StateShowWaitMenu(IrcBattleMatch *work) {
    IrcBattleMatch_CreateBackMenu(work, FALSE);
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitMembers);
}

static void IrcBattleMatch_StateRestoreAfterConfirm(IrcBattleMatch *work) {
    IrcBattleMatchMsg msg = { work->currentMsgId };

    IrcBattleMatch_ShowMessage(&msg, work);
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateConnectLoop);
    if (work->mode == IRCBATTLEMATCH_MODE_MUSICAL_ARG0 && work->shownCount > 0) {
        IrcBattleMatch_CreateBackMenu(work, TRUE);
    } else {
        IrcBattleMatch_CreateBackMenu(work, FALSE);
    }
}

static void IrcBattleMatch_StateConfirmAnswer(IrcBattleMatch *work) {
    if (AppTaskMenu_IsFlashFinished(work->menu)) {
        u8 pos = AppTaskMenu_GetCursorPos(work->menu);

        AppTaskMenu_Free(work->menu);
        work->menu = NULL;
        if (pos == 0) {
            IrcBattleMatch_FreeWindows(work);
            GFL_BGSysQueueScrLoad(6);
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateShowWaitMenu);
        } else {
            IrcBattleMatch_FreeWindows(work);
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateRestoreAfterConfirm);
        }
    }
}

// Waits for a touch, then goes back to the infrared menu
static void IrcBattleMatch_StateWaitMismatchTouch(IrcBattleMatch *work) {
    if (func_0203da48()) {
        IrcBattleMatch_Abort(work);
        work->param->result = IRCBATTLEMATCH_RESULT_BACK;
        IrcBattleMatch_SetState(work, IrcBattleMatch_StartWipeOut);
    }
}

static void IrcBattleMatch_StateShowQuitPromptConnect(IrcBattleMatch *work) {
    IrcBattleMatchMsg msg = { IRCMATCH_MSG_QUIT_CONFIRM };
    u32 currentMsgId = work->currentMsgId;

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x180, 0x20, work->heapId);
    IrcBattleMatch_ShowMessage(&msg, work);
    work->currentMsgId = currentMsgId;
    IrcBattleMatch_CreateYesNoMenu(work);
    func_020232d8();
    if (work->buttonMan != NULL) {
        GFL_BMN_Delete(work->buttonMan);
    }
    work->buttonMan = NULL;
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateQuitAnswerConnect);
}

static void IrcBattleMatch_StateShowConfirmPrompt(IrcBattleMatch *work) {
    IrcBattleMatchMsg msg = { IRCMATCH_MSG_MEMBERS_OK };
    u32 currentMsgId = work->currentMsgId;

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x180, 0x20, work->heapId);
    IrcBattleMatch_ShowMessage(&msg, work);
    work->currentMsgId = currentMsgId;
    IrcBattleMatch_CreateYesNoMenu(work);
    func_020232d8();
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateConfirmAnswer);
}

static void IrcBattleMatch_StateWaitFriendFull(IrcBattleMatch *work) {
    work->counter--;
    if (work->counter == 0) {
        IrcBattleMatch_FreeWindows(work);
        IrcBattleMatch_Abort(work);
        IrcBattleMatch_SetState(work, IrcBattleMatch_StartWipeOut);
    }
}

static void IrcBattleMatch_StateShowWaitMessage(IrcBattleMatch *work) {
    IrcBattleMatchMsg msg;

    IrcBattleMatch_FreeWindows(work);
    msg.msgId = IRCMATCH_MSG_WAIT;
    IrcBattleMatch_ShowMessage(&msg, work);
    IrcBattleMatch_CreateBackMenu(work, FALSE);
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitBackWaiting);
}

static void IrcBattleMatch_StateQuitAnswerWaiting(IrcBattleMatch *work) {
    if (AppTaskMenu_IsFlashFinished(work->menu)) {
        u8 pos = AppTaskMenu_GetCursorPos(work->menu);

        AppTaskMenu_Free(work->menu);
        work->menu = NULL;
        GFL_BGSysQueueScrLoad(6);
        if (pos == 0) {
            IrcBattleMatch_FreeWindows(work);
            IrcBattleMatch_Abort(work);
            IrcBattleMatch_SetState(work, IrcBattleMatch_StartWipeOut);
        } else {
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateShowWaitMessage);
        }
        IrcBattleMatch_DimBackground(work, FALSE);
    }
}

static void IrcBattleMatch_StateShowQuitPromptWaiting(IrcBattleMatch *work) {
    IrcBattleMatchMsg msg = { IRCMATCH_MSG_QUIT_CONFIRM };
    u32 currentMsgId = work->currentMsgId;

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x180, 0x20, work->heapId);
    IrcBattleMatch_ShowMessage(&msg, work);
    work->currentMsgId = currentMsgId;
    IrcBattleMatch_CreateYesNoMenu(work);
    func_020232d8();
    if (work->buttonMan != NULL) {
        GFL_BMN_Delete(work->buttonMan);
    }
    work->buttonMan = NULL;
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateQuitAnswerWaiting);
}

static void IrcBattleMatch_OnLeaderAnimEnd(u32 param, fx32 frame) {
    IrcBattleMatch *work = (IrcBattleMatch *)param;
    u16 sequence = func_0204c4a0(work->actors[0]);

    switch (sequence) {
    case 8:
        func_0204c488(work->actors[0], 11);
        IrcBattleMatch_SetActorPos(work, 200, 38, 0);
        IrcBattleMatch_CreateActor(work, 128, 113, 4, 2);
        break;
    case 11:
        break;
    default:
        func_0204c488(work->actors[0], 8);
        break;
    }
}

static void IrcBattleMatch_OnPartnerAnimEnd(u32 param, fx32 frame) {
    IrcBattleMatch *work = (IrcBattleMatch *)param;
    u16 sequence = func_0204c4a0(work->actors[2]);

    switch (sequence) {
    case 9:
        func_0204c488(work->actors[2], 10);
        IrcBattleMatch_SetActorPos(work, 57, 154, 2);
        IrcBattleMatch_CreateActor(work, 128, 77, 5, 3);
        break;
    case 10:
        break;
    default:
        func_0204c488(work->actors[2], 9);
        break;
    }
}

static void IrcBattleMatch_StateShowMismatch(IrcBattleMatch *work) {
    IrcBattleMatchMsg msg = { IRCMATCH_MSG_MISMATCH };

    IrcBattleMatch_FreeWindows(work);
    IrcBattleMatch_ShowMessage(&msg, work);
    IrcBattleMatch_SetState(work, IrcBattleMatch_StateWaitMismatchTouch);
}

// Updates the infrared multi battle's screen when the link goes up or down
static void IrcBattleMatch_UpdateLinkChange(IrcBattleMatch *work) {
    ClActorCallback callback;

    IrcBattleMatch_FreeWindows(work);
    if (work->linkChanged == TRUE) {
        if (work->linked != 0) {
            IrcBattleMatchMsg msg = { IRCMATCH_MSG_CONNECT_LEADER };

            IrcBattleMatch_ShowMessage(&msg, work);
        } else {
            IrcBattleMatchMsg msg = { IRCMATCH_MSG_WAIT };

            IrcBattleMatch_ShowMessage(&msg, work);
            IrcBattleMatch_FreeActor(work, 0);
            IrcBattleMatch_FreeActor(work, 2);
        }
        IrcBattleMatch_FreeActor(work, 6);
        IrcBattleMatch_FreeActor(work, 7);
        IrcBattleMatch_FreeActor(work, 4);
        IrcBattleMatch_FreeActor(work, 5);
        IrcBattleMatch_FreeActor(work, 1);
        IrcBattleMatch_FreeActor(work, 3);
        if (work->linked != 0) {
            callback.type = CLACT_CALLBACK_LAST_FRAME;
            callback.param = (u32)work;
            callback.func = IrcBattleMatch_OnLeaderAnimEnd;
            func_0204c5b0(work->actors[0], &callback);
            callback.func = IrcBattleMatch_OnPartnerAnimEnd;
            func_0204c5b0(work->actors[2], &callback);
        }
        work->linkChanged = FALSE;
    }
    work->linkChangeFrames++;
    if (work->linkChangeFrames > 8 && work->palettePulse != 0) {
        work->linkChangePending = FALSE;
        GFL_BGSysSetBGEnabled(1, FALSE);
    }
}

// The message the multi battle and the others show once the link is lost, by mode
static IrcBattleMatchMsg sIrcBattleMatchWaitMessages[] = {
    { IRCMATCH_MSG_BATTLE_STARTING },
    { IRCMATCH_MSG_BATTLE_STARTING },
    { IRCMATCH_MSG_BATTLE_STARTING },
    { IRCMATCH_MSG_BATTLE_STARTING },
    { IRCMATCH_MSG_BATTLE_STARTING },
    { IRCMATCH_MSG_BATTLE_STARTING },
    { IRCMATCH_MSG_TRADE_STARTING },
    { IRCMATCH_MSG_TRADE_STARTING },
    { IRCMATCH_MSG_WAIT_MOMENT },
    { IRCMATCH_MSG_WAIT_MOMENT },
    { IRCMATCH_MSG_WAIT_MOMENT },
    { IRCMATCH_MSG_BATTLE_STARTING },
    { IRCMATCH_MSG_WAIT_MOMENT },
    { IRCMATCH_MSG_WAIT_MOMENT },
};

static void IrcBattleMatch_StateConnectLoop(IrcBattleMatch *work) {
    if (work->linkChangePending && work->mode == IRCBATTLEMATCH_MODE_MULTI) {
        IrcBattleMatch_UpdateLinkChange(work);
    } else if (work->bannerActive == TRUE && work->mode != IRCBATTLEMATCH_MODE_MUSICAL_ARG0) {
        if (work->messagePending == TRUE) {
            IrcBattleMatch_FreeWindows(work);
            IrcBattleMatch_ShowMessage(&sIrcBattleMatchWaitMessages[work->mode], work);
            IrcBattleMatch_FreeActor(work, 4);
            IrcBattleMatch_FreeActor(work, 5);
            work->messagePending = FALSE;
            if (work->actors[0] != NULL) {
                func_0204c520(work->actors[0], FALSE);
                func_0204c520(work->actors[2], FALSE);
            }
        }
        IrcBattleMatch_MoveActor(work, 0, -2, 0);
        IrcBattleMatch_MoveActor(work, 0, 2, 2);
        work->bannerFrames++;
        if (work->bannerFrames > 30) {
            if (work->actors[0] != NULL) {
                func_0204c124(work->actors[0], FALSE);
                func_0204c124(work->actors[2], FALSE);
            }
            work->bannerActive = FALSE;
        }
    }
    if (func_02042788() && func_02040464(func_02040440()) == 7) {
        IrcBattleMatch_SetState(work, IrcBattleMatch_StateShowMismatch);
        return;
    }
    if (AppTaskMenu_IsFlashFinished(work->menu)) {
        u8 pos = AppTaskMenu_GetCursorPos(work->menu);

        AppTaskMenu_Free(work->menu);
        work->menu = NULL;
        GFL_BGSysQueueScrLoad(6);
        if (work->backOnly) {
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateShowQuitPromptConnect);
        } else if (pos == 0) {
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateShowConfirmPrompt);
        } else {
            IrcBattleMatch_SetState(work, IrcBattleMatch_StateShowQuitPromptConnect);
        }
    } else if (work->mode == IRCBATTLEMATCH_MODE_MUSICAL_ARG0) {
        if (work->shownCount != func_02042a48()) {
            IrcBattleMatch_ShowMemberCount(IRCMATCH_MSG_MEMBER_COUNT, work);
            work->shownCount = func_02042a48();
        }
        if (work->shownCount > 0 && work->backOnly == TRUE) {
            AppTaskMenu_Free(work->menu);
            work->menu = NULL;
            IrcBattleMatch_CreateBackMenu(work, TRUE);
        }
    }
}

static BOOL IrcBattleMatch_Init(GameProc *proc, u32 *state, void *param, void *work_) {
    IrcBattleMatch *work;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_IRC_BATTLE_MENU, 0x20000);
    work = GFL_ProcInitSubsystem(proc, sizeof(IrcBattleMatch), HEAPID_IRC_BATTLE_MENU);
    sys_memset(work, 0, sizeof(IrcBattleMatch));
    work->param = param;
    work->param->netError = FALSE;
    work->heapId = HEAPID_IRC_BATTLE_MENU;
    work->mode = work->param->result;
    work->paletteBackup = GFL_HeapAllocate(work->heapId, 0x200, FALSE, "ircbattlematch.c", 2946);
    IrcBattleMatch_InitScreen(work);
    func_020429f0();
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 0x10, 3, 9, 14);
    return TRUE;
}

// Cycles the palette of BG 0 every eighth frame, and says when it is halfway
static inline BOOL IrcBattleMatch_CyclePalette(IrcBattleMatch *work) {
    if ((OS_GetVBlankCount() & 7) == 1) {
        gfxUploadStdPaletteBGA(work->palettes[work->paletteFrame], 0, 0x20);
        work->paletteFrame++;
        if (work->paletteFrame == 9) {
            return TRUE;
        }
        if (work->paletteFrame >= 18) {
            work->paletteFrame = 0;
        }
    }
    return FALSE;
}

static BOOL IrcBattleMatch_Main(GameProc *proc, u32 *state, void *param, void *work_) {
    IrcBattleMatch *work = work_;
    BOOL done = TRUE;
    IrcBattleMatchState stateFunc = work->state;
    work->palettePulse = IrcBattleMatch_CyclePalette(work);
    if (stateFunc != NULL) {
        stateFunc(work);
        if (work->state != NULL) {
            done = FALSE;
        }
    }
    if (work->menu != NULL) {
        AppTaskMenu_Update(work->menu);
    }
    if (work->taskWin0 != NULL) {
        AppTaskMenuWin_Update(work->taskWin0);
    }
    if (work->taskWin1 != NULL) {
        AppTaskMenuWin_Update(work->taskWin1);
    }
    GFL_BGSysMoveBG(4, 3, work->scrollY);
    work->scrollY--;
    func_0204b794();
    func_02021a3c(work->printQueue);
    if (GFL_NetErrCheck()) {
        GFL_NetErrMarkShown();
        work->param->result = IRCBATTLEMATCH_RESULT_QUIT;
        done = TRUE;
        work->param->netError = TRUE;
        Wipe_SetScreenCovered(0, 0);
        Wipe_SetScreenCovered(1, 0);
    }
    return done;
}

static BOOL IrcBattleMatch_Exit(GameProc *proc, u32 *state, void *param, void *work_) {
    IrcBattleMatch *work = work_;

    if (!GFL_WipeIsFinished()) {
        return FALSE;
    }
    if (func_02042788()) {
        func_02040c64(IRCMATCH_CMD_MODE);
    }
    if (work->menu != NULL) {
        AppTaskMenu_Free(work->menu);
    }
    if (work->taskWin0 != NULL) {
        AppTaskMenuWin_Free(work->taskWin0);
    }
    if (work->taskWin1 != NULL) {
        AppTaskMenuWin_Free(work->taskWin1);
    }
    if (work->buttonMan != NULL) {
        GFL_BMN_Delete(work->buttonMan);
    }
    GFL_HeapFree(work->paletteBackup);
    work->buttonMan = NULL;
    GFL_WordSetSystemFree(work->wordSet);
    IrcBattleMatch_FreeScreen(work);
    GFL_TCBRemove(work->vblankTcb);
    func_0204bf98(work->actUnit);
    func_0204b758();
    func_02021c44(work->printQueue);
    func_02021a18(work->printQueue);
    if (work->unk30C != NULL) {
        GFL_TCBRemove(work->unk30C);
        work->unk30C = NULL;
        work->unk310 = 0;
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_IRC_BATTLE_MENU);
    return TRUE;
}

const GameProcFunctions IRCBATTLEMATCH_PROC_FUNCTIONS = {
    IrcBattleMatch_Init,
    IrcBattleMatch_Main,
    IrcBattleMatch_Exit,
};
