#include "types.h"
#include "system/game_system.h"
#include "save/c_gear_savedata.h"
#include "save/save_control.h"
#include "dsprot/dsprot.h"
#include "field/event_game_clear.h"
#include "field/festival.h"
#include "field/field.h"
#include "field/field_sound.h"
#include "field/intrude_work.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_irc_wireless.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/random.h"
#include "gfl/tcb.h"
#include "gfl/ui.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "system/aeabi.h"
#include "system/game_beacon.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/iss_sys.h"
#include "system/playtime_ctrl.h"

// The game system: the process that runs the game from the title screen's choice on, and the accessors of what it
// owns. The file's name is a guess, from the GameSystem and GSYS names: the ROM has no string for it. The functions'
// names are swan's

static void *GameSystem_DSProtTamper1(void *arg0, void *arg1);
static void *GameSystem_DSProtNop1(void *arg0, void *arg1);
static void *GameSystem_DSProtTamper2(void *arg0, void *arg1);
static void *GameSystem_DSProtNop2(void *arg0, void *arg1);
static BOOL GameSystem_Init(GameProc *proc, u32 *state, void *param, void *work);
static BOOL GameSystem_MainLoop(GameProc *proc, u32 *state, void *param, void *work);
static BOOL GameSystem_End(GameProc *proc, u32 *state, void *param, void *work);
static u32 GameSystem_GetWorkSize(void);
static void GSYS_Create(GameSystem *gsys, HeapID heapId, GameSystemProcData *procData);
static void GSYS_Free(GameSystem *gsys);
static void SetupGameSystem(GameSystem *gsys, HeapID heapId, GameSystemProcData *procData);
static BOOL GSYS_Update(GameSystem *gsys);
static void GSYS_UpdateGameCommBoot(GameSystem *gsys);
static void GSYS_Terminate(GameSystem *gsys);
static void GSYS_UpdateEventRunningFlag(GameSystem *gsys);
static BOOL GSYS_CanBootGameComm(GameSystem *gsys);
static void GSYS_UICallback(void *work);
static void GSYS_SetupUICallback(GameSystem *gsys);
static void GSYS_ClearUICallback(GameSystem *gsys);
static BOOL GSYS_SleepCheckCallback(void *work);
static void GSYS_SetupSleepCheckCallback(GameSystem *gsys);
static void GSYS_ClearSleepCheckCallback(GameSystem *gsys);

u32 g_AntiPiracyTaskCount;
GameSystem *g_GameSystem;
GameSystemProcData g_GameSysProcData;

const GameProcFunctions GAMESYSTEM_PROC_FUNCTIONS = {
    GameSystem_Init,
    GameSystem_MainLoop,
    GameSystem_End,
};

void get_mt(TCB *tcb, void *data) {
    GFL_RandomMT();
    GFL_RandomMT();
}

// What a failed anti-piracy check does: each one makes the random numbers of every frame after it skip two more
static void *GameSystem_DSProtTamper1(void *arg0, void *arg1) {
    u32 *seed = arg0;

    GFL_VBlankTCBAdd(get_mt, NULL, 0x7f);
    g_AntiPiracyTaskCount++;
    *seed += 0x47;
    return seed;
}

static void *GameSystem_DSProtNop1(void *arg0, void *arg1) {
    GFL_HeapCreateChild(HEAPID_DEVICE_ALLOC, HEAPID_TRIAL_HOUSE, 0x1400);
    GameSystem_GetWorkSize();
    GFL_HeapDelete(HEAPID_TRIAL_HOUSE);
    return arg1;
}

static void *GameSystem_DSProtTamper2(void *arg0, void *arg1) {
    u32 *seed = arg0;

    *seed += 0x191;
    return seed;
}

static void *GameSystem_DSProtNop2(void *arg0, void *arg1) {
    return arg0;
}

static BOOL GameSystem_Init(GameProc *proc, u32 *state, void *param, void *work) {
    GameSystem *gsys;
    u32 size = GameSystem_GetWorkSize();
    u32 seed;
    u8 flag = 0;
    s32 i;
    s32 badges;

    seed = 0xc35ffb;
    GFL_OvlLoad(OVERLAY_DSPROT);
    DSProt_Init();
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_GAMESYSTEM, 0xc000);
    {
        u32 index = OS_GetVBlankCount() & 1;

        data_ov337_02182440 = index;
        data_ov337_02182444[index] = GameSystem_DSProtNop1;
        data_ov337_02182444[index ^ 1] = GameSystem_DSProtTamper1;
    }
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_GAMEEVENT, 0x10000);
    {
        u32 shift;
        u32 checksum;
        const u32 *code = (const u32 *)DSProt_CallRunChecks2;

        for (shift = 0x25, checksum = 0; shift != 0; shift--) {
            checksum ^= (*code >> shift) | (*code << (32 - shift));
            code++;
        }
        if (checksum == DSPROT_CHECKSUM) {
            DSProt_CallRunChecks2(&seed, &flag);
        } else {
            data_ov337_02182444[data_ov337_02182440 ^ 1](&seed, &flag);
        }
    }
    {
        u32 index = OS_GetVBlankCount() & 1;

        data_ov337_02182440 = index;
        data_ov337_02182444[index] = GameSystem_DSProtNop2;
        data_ov337_02182444[index ^ 1] = GameSystem_DSProtTamper2;
    }
    GFL_HeapCreateChild(HEAPID_DEVICE_ALLOC, HEAPID_TRIAL_HOUSE, 0x1400);
    {
        u32 shift;
        u32 checksum;
        const u32 *code = (const u32 *)DSProt_CallRunChecks1;

        for (shift = 0x25, checksum = 0; shift != 0; shift--) {
            checksum ^= (*code >> shift) | (*code << (32 - shift));
            code++;
        }
        if (checksum == DSPROT_CHECKSUM) {
            DSProt_CallRunChecks1(&seed, &size);
        } else {
            data_ov337_02182444[data_ov337_02182440 ^ 1](&seed, &size);
        }
    }
    GFL_OvlUnload(OVERLAY_DSPROT);

    gsys = GFL_ProcInitSubsystem(proc, size, HEAPID_GAMESYSTEM);
    sys_memset(gsys, 0, size);
    g_GameSystem = gsys;
    SetupGameSystem(gsys, HEAPID_GAMESYSTEM, param);
    GSYS_SetNowEvent(gsys, CallCreateGameEntryPointEvent(gsys, param));

    if (seed % 0x12af != 0) {
        badges = getBadgeCount(getTrainerCardDataBlkAddress(GSYS_GetGameData(gsys)));
        for (i = 0; i < badges; i++) {
            GameSystem_DSProtTamper1(&seed, &flag);
        }
    }
    return TRUE;
}

static BOOL GameSystem_MainLoop(GameProc *proc, u32 *state, void *param, void *work) {
    if (GSYS_Update(work)) {
        return TRUE;
    }
    return FALSE;
}

static BOOL GameSystem_End(GameProc *proc, u32 *state, void *param, void *work) {
    GSYS_Terminate(work);
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_GAMESYSTEM);
    GFL_HeapDelete(HEAPID_GAMEEVENT);
    GFL_HeapDelete(HEAPID_TRIAL_HOUSE);
    sys_reset(0);
    return TRUE;
}

GameSystemProcData *GameSystem_CreateProcData(GameEntryPoint entryPoint, u16 zoneId, VecFx32 *spawnPos, s16 unk12) {
    sys_memset(&g_GameSysProcData, 0, sizeof(g_GameSysProcData));
    g_GameSysProcData.entryPoint = entryPoint;
    g_GameSysProcData.zoneId = zoneId;
    g_GameSysProcData.spawnPos = *spawnPos;
    g_GameSysProcData.unk12 = unk12;
    return &g_GameSysProcData;
}

static u32 GameSystem_GetWorkSize(void) {
    return sizeof(GameSystem);
}

static void GSYS_Create(GameSystem *gsys, HeapID heapId, GameSystemProcData *procData) {
    gsys->heapId = heapId;
    gsys->procData = procData;
    gsys->procManager = CreateGameProcManager(gsys->heapId);
    gsys->procManagerState = 0;
    gsys->eventProvider = NULL;
    gsys->eventProviderData = NULL;
    gsys->event = NULL;
    gsys->gameData = GameData_Create(gsys->heapId);
    gsys->field = NULL;
    gsys->comm = GameCommSys_Create(gsys->heapId, gsys->gameData);
    gsys->unk28 = 0;
    gsys->iss = ISS_Create(gsys->gameData, heapId);
    gsys->festival = CreateLinkFestival(heapId, gsys);
    gsys->unk36 = 0;
    gsys->unk37_0 = FALSE;
    GSYS_SetupUICallback(gsys);
    GSYS_SetupSleepCheckCallback(gsys);
    InitZoneDataSystem(heapId);
}

static void GSYS_Free(GameSystem *gsys) {
    GSYS_ClearSleepCheckCallback(gsys);
    GSYS_ClearUICallback(gsys);
    FreeLinkFestival(gsys->festival);
    ISS_Free(gsys->iss);
    GameData_Free(gsys->gameData);
    FreeGameComm(gsys->comm);
    FreeZoneDataSystem();
}

static void SetupGameSystem(GameSystem *gsys, HeapID heapId, GameSystemProcData *procData) {
    GSYS_Create(gsys, heapId, procData);
    GameBeaconSys_SetGameSystem(gsys);
    GameBeaconSys_SetGameData(gsys->gameData);
    GameSystemTimer_Disable();
}

// Runs a frame of the game. Whether it is over: no process and no event is left
static BOOL GSYS_Update(GameSystem *gsys) {
    GSYS_UpdateEventRunningFlag(gsys);
    if (GameData_Is30FPSMode(gsys->gameData)) {
        GCTX_HIDSetUpdateRate(30);
    }
    if (!GameData_Is30FPSSkipFrame(gsys->gameData)) {
        GSYS_FeedEventsFromProvider(gsys, gsys->eventProvider, gsys->eventProviderData);
        GSYS_UpdateGameCommBoot(gsys);
        GSYS_UpdateEvents(gsys);
        GameCommSys_Main(gsys->comm);
    }
    gsys->procManagerState = GFL_ProcMgrUpdate(gsys->procManager);
    FieldSnd_Update(GameData_GetFieldSoundSystem(gsys->gameData));
    ISS_Update(gsys->iss);
    GameSystemTimer_Update(gsys->gameData);
    LinkFestival_Update(gsys->festival);
    if (GameData_Is30FPSMode(gsys->gameData)) {
        GCTX_HIDSetUpdateRate(60);
    }
    if (gsys->procManagerState == 2 || gsys->procManagerState == 0) {
        GFL_NetErrShow(0);
    }
    if (gsys->procManagerState == 0 && gsys->event == NULL) {
        return TRUE;
    }
    return FALSE;
}

// Starts the field's communication once the map is up and no event runs
static void GSYS_UpdateGameCommBoot(GameSystem *gsys) {
    if (!GSYS_GetEventRunningFlag(gsys) && GSYS_CheckField(gsys) == TRUE
        && Field_CheckMapLoadFinished(GSYS_GetField(gsys)) == TRUE) {
        GSYS_TryBootGameComm(gsys);
    }
}

static void GSYS_Terminate(GameSystem *gsys) {
    FreeGameProcManager(gsys->procManager);
    GSYS_Free(gsys);
}

void GSYS_SetEventProvider(GameSystem *gsys, void *provider, void *data) {
    gsys->eventProvider = provider;
    gsys->eventProviderData = data;
}

static void GSYS_UpdateEventRunningFlag(GameSystem *gsys) {
    gsys->eventRunning = GSYS_CheckNowEvent(gsys);
}

void GSYS_QueueProc(GameSystem *gsys, s32 overlayId, const GameProcFunctions *functions, void *param) {
    QueueGameProc(gsys->procManager, overlayId, functions, param);
}

void GSYS_StartFieldmapProc(GameSystem *gsys) {
    GSYS_QueueProc(gsys, OVERLAY_ID(36), &FIELDMAP_PROC_FUNCS, gsys);
}

BOOL GSYS_GetEventRunningFlag(GameSystem *gsys) {
    if (GSYS_CheckNowEvent(gsys)) {
        return TRUE;
    }
    return gsys->eventRunning;
}

BOOL GSYS_GetProcMgrState(GameSystem *gsys) {
    return gsys->procManagerState;
}

GameData *GSYS_GetGameData(GameSystem *gsys) {
    return gsys->gameData;
}

PlayerState *GSYS_GetPlayerState(GameSystem *gsys) {
    return GameData_GetPlayerState(gsys->gameData);
}

GameEvent *GSYS_GetNowEvent(GameSystem *gsys) {
    return gsys->event;
}

void GSYS_SetNowEvent(GameSystem *gsys, GameEvent *event) {
    gsys->event = event;
}

Field *GSYS_GetField(GameSystem *gsys) {
    return gsys->field;
}

void GSYS_SetField(GameSystem *gsys, Field *field) {
    gsys->field = field;
}

BOOL GSYS_CheckField(GameSystem *gsys) {
    if (gsys->field != NULL) {
        return TRUE;
    }
    return FALSE;
}

LinkFestival *GSYS_GetLinkFestival(GameSystem *gsys) {
    return gsys->festival;
}

void func_02016b0c(GameSystem *gsys, u32 a1) {
    gsys->unk34 = a1;
}

u8 func_02016b14(GameSystem *gsys) {
    return gsys->unk34;
}

ISS *GameSystem_GetISS(GameSystem *gsys) {
    return gsys->iss;
}

GameCommSys *GSYS_GetGameCommSystem(GameSystem *gsys) {
    return gsys->comm;
}

void func_02016b24(GameSystem *gsys, u32 value) {
    gsys->unk36 = value;
}

u8 func_02016b2c(GameSystem *gsys) {
    return gsys->unk36;
}

u32 func_02016b34(GameSystem *gsys) {
    return func_02017614(gsys->gameData);
}

void func_02016b40(GameSystem *gsys, u32 value) {
    func_0201762c(gsys->gameData, value);
}

static BOOL GSYS_CanBootGameComm(GameSystem *gsys) {
    GameCommSys *comm = GSYS_GetGameCommSystem(gsys);
    u16 zoneId = PlayerState_GetZoneID(GSYS_GetPlayerState(gsys));

    if (gsys->unk36 == 0 && !IsZoneGameCommDisabled(zoneId) && !GFL_NetErrCheck() && func_02016b34(gsys) == TRUE
        && func_ov012_021535dc(gsys) == TRUE && !(GCTX_HIDIsLidClosed() == TRUE && GetIsShouldUICallback() == TRUE)
        && func_020098c0(func_02009924(gsys->gameData))) {
        return TRUE;
    }
    return FALSE;
}

BOOL GSYS_TryBootGameComm(GameSystem *gsys) {
    GameCommSys *comm = GSYS_GetGameCommSystem(gsys);

    if (!func_02042788() && !GameCommSys_BootCheck(comm) && GSYS_CanBootGameComm(gsys) == TRUE) {
        GameCommSys_Boot(comm, 1, gsys);
        return TRUE;
    }
    return FALSE;
}

BOOL func_02016bec(GameSystem *gsys) {
    return gsys->unk37_0;
}

void func_02016bf8(GameSystem *gsys) {
    gsys->unk37_0 = FALSE;
}

// The UI callback: turns the communication setting off and remembers that it did
static void GSYS_UICallback(void *work) {
    GameSystem *gsys = work;

    if (func_02016b34(gsys)) {
        func_02016b40(gsys, FALSE);
        func_0201740c(GSYS_GetGameData(gsys), 0);
        gsys->unk37_0 = TRUE;
    }
}

static void GSYS_SetupUICallback(GameSystem *gsys) {
    SetUICallbackStatic(GSYS_UICallback, gsys);
}

static void GSYS_ClearUICallback(GameSystem *gsys) {
    SetUICallbackStatic(NULL, NULL);
}

// The sleep check: whether closing the lid must not put the console to sleep
static BOOL GSYS_SleepCheckCallback(void *work) {
    GameSystem *gsys = work;

    if (GameCommSys_BootCheck(GSYS_GetGameCommSystem(gsys))) {
        return TRUE;
    }
    if (func_02043068()) {
        return TRUE;
    }
    if (GetIsShouldUICallback()) {
        return FALSE;
    }
    if (func_02016b34(gsys)) {
        return TRUE;
    }
    return FALSE;
}

static void GSYS_SetupSleepCheckCallback(GameSystem *gsys) {
    GCTX_HIDSetSleepCheckCallback(GSYS_SleepCheckCallback, gsys);
}

static void GSYS_ClearSleepCheckCallback(GameSystem *gsys) {
    GCTX_HIDSetSleepCheckCallback(NULL, NULL);
}
