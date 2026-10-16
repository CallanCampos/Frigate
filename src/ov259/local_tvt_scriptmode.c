#include "types.h"
#include "app/local_tvt_local.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "nitro/gx.h"
#include "save/player_info.h"
#include "system/game_data.h"
#include "system/wipe.h"

// Plays the script of a call the game's story makes on the Xtransceiver, named after the ROM's
// "local_tvt_scriptmode.c". A script names the faces and their zones, then has commands that let a face talk, wait
// or play a sound

// The archive of the scripts
#define ARC_LOCAL_TVT_SCRIPT 234

// The steps of LocalTvtScriptMode_Main
enum {
    SCRIPT_SEQ_WAIT_FACES,
    SCRIPT_SEQ_FADE_IN,
    SCRIPT_SEQ_WAIT_FADE_IN,
    SCRIPT_SEQ_END,
    SCRIPT_SEQ_FADE_OUT,
    SCRIPT_SEQ_WAIT_FADE_OUT,
    SCRIPT_SEQ_COMMANDS,
};

// The commands of a script
enum {
    SCRIPT_COMMAND_END,
    SCRIPT_COMMAND_TALK,
    SCRIPT_COMMAND_WAIT,
    SCRIPT_COMMAND_SE,
    SCRIPT_COMMAND_WAIT_SE,
    SCRIPT_COMMAND_COUNT,
};

// The persons of a script whose face depends on the player's gender, and the one whose name does
#define SCRIPT_PERSON_6 6
#define SCRIPT_PERSON_8 8
#define SCRIPT_PERSON_9 9

typedef struct {
    u8 persons[LOCAL_TVT_FACES];
    u16 zones[LOCAL_TVT_FACES];
} LocalTvtScriptHeader;

typedef struct {
    u8 command;
    u8 face;
    u16 param;
} LocalTvtScriptCommand;

struct LocalTvtScriptMode {
    u16 heapId;
    int faceCount;
    LocalTvtChara *chara;
    GameData *gameData;
    u16 scriptId;
    u8 seq;
    u8 timer;
    ArcTool *arc;
    BOOL commandStarted;
    void *file;
    LocalTvtScriptHeader *header;
    LocalTvtScriptCommand *commands;
    MsgData *msgData;
    u8 talker;
    u8 command;
    u16 wait;
};

typedef struct {
    void (*init)(LocalTvtScriptMode *mode);
    BOOL (*main)(LocalTvtScriptMode *mode);
} LocalTvtScriptCommandFuncs;

static void LocalTvtScriptMode_InitGraphic(LocalTvtScriptMode *mode);
static void LocalTvtScriptMode_ExitGraphic(LocalTvtScriptMode *mode);
static void LocalTvtScriptMode_LoadScript(LocalTvtScriptMode *mode);
static void LocalTvtScriptMode_UpdateCommand(LocalTvtScriptMode *mode);
static void LocalTvtScriptMode_InitTalk(LocalTvtScriptMode *mode);
static BOOL LocalTvtScriptMode_MainTalk(LocalTvtScriptMode *mode);
static void LocalTvtScriptMode_InitWait(LocalTvtScriptMode *mode);
static BOOL LocalTvtScriptMode_MainWait(LocalTvtScriptMode *mode);
static void LocalTvtScriptMode_InitSE(LocalTvtScriptMode *mode);
static BOOL LocalTvtScriptMode_MainSE(LocalTvtScriptMode *mode);
static void LocalTvtScriptMode_InitWaitSE(LocalTvtScriptMode *mode);
static BOOL LocalTvtScriptMode_MainWaitSE(LocalTvtScriptMode *mode);

static const LocalTvtScriptCommandFuncs sLocalTvtScriptCommands[SCRIPT_COMMAND_COUNT] = {
    { NULL, NULL },
    { LocalTvtScriptMode_InitTalk, LocalTvtScriptMode_MainTalk },
    { LocalTvtScriptMode_InitWait, LocalTvtScriptMode_MainWait },
    { LocalTvtScriptMode_InitSE, LocalTvtScriptMode_MainSE },
    { LocalTvtScriptMode_InitWaitSE, LocalTvtScriptMode_MainWaitSE },
};

LocalTvtScriptMode *LocalTvtScriptMode_Create(HeapID heapId, GameSystem *gsys, GameData *gameData, u16 scriptId,
                                              LocalTvtChara *chara) {
    LocalTvtScriptMode *mode =
        GFL_HeapAllocate(heapId, sizeof(LocalTvtScriptMode), TRUE, "local_tvt_scriptmode.c", 137);
    LocalTvtFaceSetup setups[LOCAL_TVT_FACES];
    LocalTvtCallData *callData;
    u32 gender;
    int i;

    mode->scriptId = scriptId;
    mode->gameData = gameData;
    mode->heapId = heapId;
    mode->chara = chara;
    LocalTvtScriptMode_InitGraphic(mode);
    mode->arc = GFL_ArcSysCreateFileHandle(ARC_LOCAL_TVT_SCRIPT, mode->heapId);
    LocalTvtScriptMode_LoadScript(mode);
    // Each script has a bank of its own
    mode->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0384 + scriptId, mode->heapId);
    gender = getTrainerGender(GetGameDataPlayerInfo(gameData));
    callData = LocalTvtCallData_Create(gsys, gameData, heapId);
    MI_CpuFill8(setups, 0, sizeof(setups));
    for (i = 0; i < LOCAL_TVT_FACES; i++) {
        u16 zoneId = mode->header->zones[i];
        u32 person;

        setups[i].nameMsg = LOCAL_TVT_NAME_DEFAULT;
        person = mode->header->persons[i];
        switch (person) {
        case SCRIPT_PERSON_6:
            if (gender == 1) {
                setups[i].person = 6;
            } else {
                setups[i].person = 7;
            }
            break;
        case SCRIPT_PERSON_9:
            if (gender == 1) {
                setups[i].person = 9;
            } else {
                setups[i].person = 10;
            }
            break;
        case SCRIPT_PERSON_8:
            setups[i].person = person;
            setups[i].nameMsg = LocalTvtSecret_GetNameMsg(callData);
            break;
        default:
            setups[i].person = person;
            break;
        }
        setups[i].background = LocalTvtCallData_GetZoneBackground(callData, zoneId);
        setups[i].object = LocalTvtCallData_GetZoneObject(callData, zoneId);
    }
    LocalTvtCallData_Free(callData);
    LocalTvtChara_Start(mode->chara, setups, mode->faceCount);
    mode->seq = SCRIPT_SEQ_WAIT_FACES;
    mode->commandStarted = TRUE;
    mode->command = 0;
    func_02042ba8(FALSE, mode->heapId);
    return mode;
}

void LocalTvtScriptMode_Free(LocalTvtScriptMode *mode) {
    GFL_MsgDataFree(mode->msgData);
    GFL_ArcToolFree(mode->arc);
    LocalTvtScriptMode_ExitGraphic(mode);
    GFL_HeapFree(mode->file);
    GFL_HeapFree(mode);
}

BOOL LocalTvtScriptMode_Main(LocalTvtScriptMode *mode) {
    switch (mode->seq) {
    case SCRIPT_SEQ_WAIT_FACES:
        if (LocalTvtChara_IsReady(mode->chara)) {
            LocalTvtChara_SetVisible(mode->chara, TRUE);
            mode->seq = SCRIPT_SEQ_FADE_IN;
        }
        break;
    case SCRIPT_SEQ_FADE_IN:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_IN, WIPE_TYPE_FADE_IN, WIPE_COLOR_BLACK, 6, 1, mode->heapId);
        mode->seq = SCRIPT_SEQ_WAIT_FADE_IN;
        break;
    case SCRIPT_SEQ_WAIT_FADE_IN:
        if (GFL_WipeIsFinished() == TRUE) {
            mode->seq = SCRIPT_SEQ_COMMANDS;
        }
        break;
    case SCRIPT_SEQ_END:
        if (mode->timer++ > 30) {
            GFL_SndSEPlay(SEQ_SE_SYS_72);
            mode->seq = SCRIPT_SEQ_FADE_OUT;
        }
        break;
    case SCRIPT_SEQ_FADE_OUT:
        GFL_WipeSet(WIPE_MODE_BOTH, WIPE_TYPE_FADE_OUT, WIPE_TYPE_FADE_OUT, WIPE_COLOR_BLACK, 6, 1, mode->heapId);
        mode->seq = SCRIPT_SEQ_WAIT_FADE_OUT;
        break;
    case SCRIPT_SEQ_WAIT_FADE_OUT:
        if (GFL_WipeIsFinished() == TRUE) {
            return TRUE;
        }
        break;
    case SCRIPT_SEQ_COMMANDS:
        LocalTvtScriptMode_UpdateCommand(mode);
        break;
    }
    return FALSE;
}

static void LocalTvtScriptMode_InitGraphic(LocalTvtScriptMode *mode) {
    G2x_SetBlendAlpha_(REG_BLDCNT_ADDR, GX_PLANEMASK_BG0,
                       GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ, 16, 4);
}

static void LocalTvtScriptMode_ExitGraphic(LocalTvtScriptMode *mode) {
}

static void LocalTvtScriptMode_LoadScript(LocalTvtScriptMode *mode) {
    mode->file = GFL_ArcToolReadHeapNewLZ(mode->arc, mode->scriptId - 1, FALSE, mode->heapId);
    mode->header = mode->file;
    mode->commands = (LocalTvtScriptCommand *)((u8 *)mode->file + sizeof(LocalTvtScriptHeader));
    if (mode->header->persons[2] == LOCAL_TVT_PERSON_NONE && mode->header->persons[3] == LOCAL_TVT_PERSON_NONE) {
        mode->faceCount = 2;
    } else {
        mode->faceCount = 4;
    }
}

static void LocalTvtScriptMode_UpdateCommand(LocalTvtScriptMode *mode) {
    u8 command = mode->commands[mode->command].command;

    if (mode->commandStarted == TRUE) {
        mode->commandStarted = FALSE;
        sLocalTvtScriptCommands[command].init(mode);
    }
    if (sLocalTvtScriptCommands[command].main(mode) == TRUE) {
        mode->command++;
        if (mode->commands[mode->command].command != SCRIPT_COMMAND_END) {
            sLocalTvtScriptCommands[mode->commands[mode->command].command].init(mode);
        } else {
            mode->timer = 0;
            mode->seq = SCRIPT_SEQ_END;
        }
    }
}

static void LocalTvtScriptMode_InitTalk(LocalTvtScriptMode *mode) {
    u8 face = mode->commands[mode->command].face;
    u8 messageId = mode->commands[mode->command].param;
    LocalTvtFace *talker = LocalTvtChara_GetFace(mode->chara, face);

    mode->talker = face;
    LocalTvtFace_Talk(talker, mode->msgData, messageId);
}

static BOOL LocalTvtScriptMode_MainTalk(LocalTvtScriptMode *mode) {
    LocalTvtFace *talker = LocalTvtChara_GetFace(mode->chara, mode->talker);

    return LocalTvtFace_UpdateTalk(talker, mode->chara);
}

static void LocalTvtScriptMode_InitWait(LocalTvtScriptMode *mode) {
    mode->wait = 0;
}

static BOOL LocalTvtScriptMode_MainWait(LocalTvtScriptMode *mode) {
    u8 frames = mode->commands[mode->command].param;

    mode->wait++;
    if (mode->wait > frames) {
        return TRUE;
    }
    return FALSE;
}

static void LocalTvtScriptMode_InitSE(LocalTvtScriptMode *mode) {
}

static BOOL LocalTvtScriptMode_MainSE(LocalTvtScriptMode *mode) {
    u8 se = mode->commands[mode->command].param;

    GFL_SndSEPlay(se);
    return TRUE;
}

static void LocalTvtScriptMode_InitWaitSE(LocalTvtScriptMode *mode) {
}

static BOOL LocalTvtScriptMode_MainWaitSE(LocalTvtScriptMode *mode) {
    if (!GFL_SndPlayerIsActiveAny()) {
        return TRUE;
    }
    return FALSE;
}
