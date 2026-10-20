#include "types.h"
#include "constants/sound.h"
#include "field/field_ambience.h"
#include "field/field_sound.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "nnsys/snd.h"
#include "system/game_data.h"
#include "system/player_volume_fader.h"
#include "system/ringtone_sys.h"

// The field's sound system: a queue of BGM requests (fades, a push and pop of one BGM, changes) run by a state machine,
// the ambient sound effects, the player volume fader and the ringtone. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except FieldSnd_IsSongListed, FieldSnd_IsSeqDataLoaded,
// FieldSnd_CanPlaySeqWhileLoading, FieldSnd_GetLastQueuedCommand, FieldSnd_SetCommandFadeIn and the other
// FieldSnd_SetCommand<Kind> functions (FieldSnd_SetCommand is swan's), FieldSnd_IsBGMChanging, FieldSnd_IsPopDone,
// FieldSnd_RingRingtone, FieldSnd_StopRingtone and FIELD_SND_STATE_ALLOWS_COMMAND. The state values are swan's; the
// type names follow the repo's FieldSound (swan's FieldSoundSystem), and the field names and size macros are ours

#define FIELD_SND_REQUEST_MAX 10
#define FIELD_SND_BGM_STACK_MAX 1
#define FIELD_SND_AMBIENCE_PLAYER_MAX 2
#define FIELD_SND_AMBIENCE_NONE 0xffff
#define FIELD_SND_COMMAND_MAX 8

typedef enum {
    FIELD_SND_STATE_INIT,
    FIELD_SND_STATE_PLAYING,
    FIELD_SND_STATE_IDLE,
    FIELD_SND_STATE_FADING_OUT,
    FIELD_SND_STATE_FADING_IN,
    FIELD_SND_STATE_PUSHING,
    FIELD_SND_STATE_POPPING,
    FIELD_SND_STATE_POPPED,
    FIELD_SND_STATE_CHANGING_OUT,
    FIELD_SND_STATE_CHANGING_IN,
    FIELD_SND_STATE_CHANGED,
    FIELD_SND_STATE_PUSHING_POSTPONED,
    FIELD_SND_STATE_PUSHED_POSTPONED,
    FIELD_SND_STATE_PREPARING,
    FIELD_SND_STATE_PREPARED,
    FIELD_SND_STATE_MAX,
} FieldSoundState;

typedef struct {
    FieldSoundCommand command;
    u32 bgm;
    u16 fadeOutFrames;
    u16 fadeInFrames;
} FieldSoundRequest;

struct FieldSound {
    GameData *gameData;
    FieldSoundState state;
    u32 currentBGM;
    u32 nextBGM;
    // The BGM whose data is loading for a change
    u32 loadingBGM;
    u32 loadStep;
    u16 fadeInFrames;
    u16 fadeOutFrames;
    FieldSoundCommand command;
    int bgmStackCount;
    u32 bgmStack[FIELD_SND_BGM_STACK_MAX];
    FieldSoundRequest requests[FIELD_SND_REQUEST_MAX];
    u8 requestHead;
    u8 requestTail;
    PlayerVolumeFader *volumeFader;
    RingtoneSys *ringtone;
    // The looped ambient sound and its volume on players 1 and 2
    u16 ambienceSE[FIELD_SND_AMBIENCE_PLAYER_MAX];
    u16 ambienceVolume[FIELD_SND_AMBIENCE_PLAYER_MAX];
    u32 ambienceStopFlags;
};

typedef void (*FieldSoundFunc)(FieldSound *fieldSound);

static BOOL FieldSnd_IsSongListed(u32 seq);
static BOOL FieldSnd_IsSeqDataLoaded(u32 seq);
static BOOL FieldSnd_CanPlaySeqWhileLoading(u32 seq);
static void FieldSnd_Reset(FieldSound *fieldSound, GameData *gameData);
static void FieldSnd_ReleaseCore(FieldSound *fieldSound, GameData *gameData);
static void FieldSnd_UpdateCommand(FieldSound *fieldSound);
static FieldSoundRequest *FieldSnd_GetQueuedCommand(FieldSound *fieldSound);
static FieldSoundRequest *FieldSnd_GetLastQueuedCommand(FieldSound *fieldSound);
static BOOL FieldSnd_HasQueuedCommand(FieldSound *fieldSound);
static u8 FieldSnd_GetCommandCountByID(const FieldSound *fieldSound, FieldSoundCommand command);
static void FieldSnd_SendRequestCore(FieldSound *fieldSound, const FieldSoundRequest *request);
static BOOL FieldSnd_IsCommandSuccessionPermitted(FieldSound *fieldSound, FieldSoundCommand command);
static void FieldSnd_NextCommand(FieldSound *fieldSound);
static void FieldSnd_SetCommand(FieldSound *fieldSound, const FieldSoundRequest *request);
static void FieldSnd_SetCommandFadeIn(FieldSound *fieldSound, u16 fadeInFrames);
static void FieldSnd_SetCommandFadeOut(FieldSound *fieldSound, u16 fadeOutFrames);
static void FieldSnd_SetCommandPush(FieldSound *fieldSound, u16 fadeOutFrames);
static void FieldSnd_SetCommandPop(FieldSound *fieldSound, u16 fadeOutFrames, u16 fadeInFrames);
static void FieldSnd_SetCommandChange(FieldSound *fieldSound, u32 bgm, u16 fadeOutFrames, u16 fadeInFrames);
static void FieldSnd_SetCommandPrepare(FieldSound *fieldSound, u32 bgm, u16 fadeOutFrames);
static void FieldSnd_SetCommandPlay(FieldSound *fieldSound, u16 bgm);
static BOOL FieldSnd_IsCommandPermitted(FieldSound *fieldSound, FieldSoundCommand command);
static void FieldSnd_Control_Init(FieldSound *fieldSound);
static void FieldSnd_Control_Playing(FieldSound *fieldSound);
static void FieldSnd_Control_Idle(FieldSound *fieldSound);
static void FieldSnd_Control_FadeOut(FieldSound *fieldSound);
static void FieldSnd_Control_FadeIn(FieldSound *fieldSound);
static void FieldSnd_Control_Push(FieldSound *fieldSound);
static void FieldSnd_Control_Pop(FieldSound *fieldSound);
static void FieldSnd_Control_PopFinish(FieldSound *fieldSound);
static void FieldSnd_Control_ChangeOut(FieldSound *fieldSound);
static void FieldSnd_Control_ChangeIn(FieldSound *fieldSound);
static void FieldSnd_Control_ChangeFinish(FieldSound *fieldSound);
static void FieldSnd_Control_PostponedPush(FieldSound *fieldSound);
static void FieldSnd_Control_PostponedPushFinish(FieldSound *fieldSound);
static void FieldSnd_Control_Prepare(FieldSound *fieldSound);
static void FieldSnd_Control_PrepareFinish(FieldSound *fieldSound);
static void FieldSnd_Exec_Init(FieldSound *fieldSound);
static void FieldSnd_Exec_Playing(FieldSound *fieldSound);
static void FieldSnd_Exec_Idle(FieldSound *fieldSound);
static void FieldSnd_Exec_FadeOut(FieldSound *fieldSound);
static void FieldSnd_Exec_FadeIn(FieldSound *fieldSound);
static void FieldSnd_Exec_Push(FieldSound *fieldSound);
static void FieldSnd_Exec_Pop(FieldSound *fieldSound);
static void FieldSnd_Exec_PopFinish(FieldSound *fieldSound);
static void FieldSnd_Exec_ChangeOut(FieldSound *fieldSound);
static void FieldSnd_Exec_ChangeIn(FieldSound *fieldSound);
static void FieldSnd_Exec_ChangeFinish(FieldSound *fieldSound);
static void FieldSnd_Exec_PostponedPush(FieldSound *fieldSound);
static void FieldSnd_Exec_PostponedPushFinish(FieldSound *fieldSound);
static void FieldSnd_Exec_Prepare(FieldSound *fieldSound);
static void FieldSnd_Exec_PrepareFinish(FieldSound *fieldSound);
static void FieldSnd_SetState(FieldSound *fieldSound, FieldSoundState state);
static void FieldSnd_FinishCommandCore(FieldSound *fieldSound);
static void FieldSnd_DiscardCommand(FieldSound *fieldSound);
static void FieldSnd_FinishCommand(FieldSound *fieldSound);
static void FieldSnd_CmdFadeIn(FieldSound *fieldSound);
static void FieldSnd_CmdFadeOut(FieldSound *fieldSound);
static void FieldSnd_BGMPush(FieldSound *fieldSound);
static void FieldSnd_BGMPop(FieldSound *fieldSound);
static BOOL FieldSnd_BGMChangeOut(FieldSound *fieldSound);
static BOOL FieldSnd_BGMChangeIn(FieldSound *fieldSound);
static void FieldSnd_CmdBGMPlay(FieldSound *fieldSound);
static BOOL FieldSnd_CheckEventsPaused(FieldSound *fieldSound);

// Whether a command may follow another in the queue, by the earlier command
static const u8 FIELD_SND_COMMAND_ALLOWS_SUCCESSOR[FIELD_SND_COMMAND_MAX][FIELD_SND_COMMAND_MAX] = {
    [FIELD_SND_CMD_NULL] = { 0, 0, 0, 0, 0, 0, 0, 0 },    [FIELD_SND_FADE_IN] = { 0, 1, 1, 1, 1, 1, 1, 1 },
    [FIELD_SND_FADE_OUT] = { 0, 1, 0, 0, 0, 0, 0, 1 },    [FIELD_SND_BGM_PUSH] = { 0, 0, 0, 0, 1, 1, 1, 1 },
    [FIELD_SND_BGM_POP] = { 0, 1, 1, 1, 1, 1, 1, 1 },     [FIELD_SND_BGM_CHANGE] = { 0, 1, 1, 1, 1, 1, 1, 1 },
    [FIELD_SND_BGM_PREPARE] = { 0, 1, 0, 0, 0, 0, 0, 1 }, [FIELD_SND_BGM_PLAY] = { 0, 1, 1, 1, 1, 1, 1, 1 },
};

// The sequences that may play while the sound thread loads, when their data is in memory
static const u16 song_list[] = {
    SEQ_SE_MESSAGE,  SEQ_SE_SELECT1, SEQ_SE_SELECT2, SEQ_SE_SELECT4, SEQ_SE_DECIDE1, SEQ_SE_DECIDE2, SEQ_SE_DECIDE3,
    SEQ_SE_CANCEL1,  SEQ_SE_CANCEL2, SEQ_SE_CANCEL3, SEQ_SE_OPEN1,   SEQ_SE_CLOSE1,  SEQ_SE_BEEP,    SEQ_SE_DANSA,
    SEQ_SE_WALL_HIT, SEQ_SE_BICYCLE, SEQ_SE_FLD_07,  SEQ_SE_FLD_08,  SEQ_SE_FLD_09,  SEQ_SE_FLD_10,  SEQ_SE_FLD_11,
    SEQ_SE_FLD_12,   SEQ_SE_FLD_13,  SEQ_SE_FLD_14,  SEQ_SE_FLD_31,  SEQ_SE_FLD_32,  SEQ_SE_FLD_49,  SEQ_SE_FLD_84,
    SEQ_SE_FLD_85,   SEQ_SE_FLD_91,  SEQ_SE_FLD_120, SEQ_SE_FLD_122, SEQ_SE_SYS_35,  SEQ_SE_MSCL_07, SEQ_SE_SYS_11,
    SEQ_SE_SYS_79,   SEQ_SE_SYS_100,
};

// Whether a state starts a command
static const u8 FIELD_SND_STATE_ALLOWS_COMMAND[FIELD_SND_STATE_MAX][FIELD_SND_COMMAND_MAX] = {
    [FIELD_SND_STATE_INIT] = { 0, 0, 0, 0, 1, 1, 1, 1 },
    [FIELD_SND_STATE_PLAYING] = { 0, 1, 1, 1, 1, 1, 1, 1 },
    [FIELD_SND_STATE_IDLE] = { 0, 1, 0, 0, 0, 0, 0, 1 },
    [FIELD_SND_STATE_FADING_OUT] = { 0, 0, 0, 0, 0, 0, 0, 0 },
    [FIELD_SND_STATE_FADING_IN] = { 0, 0, 0, 0, 0, 0, 0, 0 },
    [FIELD_SND_STATE_PUSHING] = { 0, 0, 0, 0, 0, 0, 0, 0 },
    [FIELD_SND_STATE_POPPING] = { 0, 0, 0, 0, 0, 0, 0, 0 },
    [FIELD_SND_STATE_POPPED] = { 0, 0, 0, 0, 0, 0, 0, 0 },
    [FIELD_SND_STATE_CHANGING_OUT] = { 0, 0, 0, 1, 0, 1, 0, 0 },
    [FIELD_SND_STATE_CHANGING_IN] = { 0, 0, 0, 1, 0, 1, 0, 0 },
    [FIELD_SND_STATE_CHANGED] = { 0, 0, 0, 0, 0, 1, 0, 0 },
    [FIELD_SND_STATE_PUSHING_POSTPONED] = { 0, 0, 0, 0, 0, 0, 0, 0 },
    [FIELD_SND_STATE_PUSHED_POSTPONED] = { 0, 0, 0, 0, 0, 0, 0, 0 },
    [FIELD_SND_STATE_PREPARING] = { 0, 0, 0, 0, 0, 0, 0, 0 },
    [FIELD_SND_STATE_PREPARED] = { 0, 0, 0, 0, 0, 0, 0, 0 },
};

static FieldSoundFunc FIELD_SND_EXEC_FUNCS[FIELD_SND_STATE_MAX] = {
    FieldSnd_Exec_Init,
    FieldSnd_Exec_Playing,
    FieldSnd_Exec_Idle,
    FieldSnd_Exec_FadeOut,
    FieldSnd_Exec_FadeIn,
    FieldSnd_Exec_Push,
    FieldSnd_Exec_Pop,
    FieldSnd_Exec_PopFinish,
    FieldSnd_Exec_ChangeOut,
    FieldSnd_Exec_ChangeIn,
    FieldSnd_Exec_ChangeFinish,
    FieldSnd_Exec_PostponedPush,
    FieldSnd_Exec_PostponedPushFinish,
    FieldSnd_Exec_Prepare,
    FieldSnd_Exec_PrepareFinish,
};

static FieldSoundFunc FIELD_SND_CONTROL_FUNCS[FIELD_SND_STATE_MAX] = {
    FieldSnd_Control_Init,
    FieldSnd_Control_Playing,
    FieldSnd_Control_Idle,
    FieldSnd_Control_FadeOut,
    FieldSnd_Control_FadeIn,
    FieldSnd_Control_Push,
    FieldSnd_Control_Pop,
    FieldSnd_Control_PopFinish,
    FieldSnd_Control_ChangeOut,
    FieldSnd_Control_ChangeIn,
    FieldSnd_Control_ChangeFinish,
    FieldSnd_Control_PostponedPush,
    FieldSnd_Control_PostponedPushFinish,
    FieldSnd_Control_Prepare,
    FieldSnd_Control_PrepareFinish,
};

FieldSound *FieldSnd_Create(GameData *gameData, HeapID heapId) {
    FieldSound *fieldSound = GFL_HeapAllocate(heapId, sizeof(FieldSound), TRUE, "field_sound_system.c", 331);

    FieldSnd_Reset(fieldSound, gameData);
    fieldSound->volumeFader = PlayerVolumeFader_Create(heapId, 0);
    fieldSound->ringtone = RingtoneSys_Create(heapId, fieldSound->volumeFader);
    GFL_SndSetSeqVerifyCallback(FieldSnd_CanPlaySeqWhileLoading);
    return fieldSound;
}

void FieldSnd_Free(FieldSound *fieldSound) {
    GFL_SndSetSeqVerifyCallback(NULL);
    RingtoneSys_Free(fieldSound->ringtone);
    PlayerVolumeFader_Free(fieldSound->volumeFader);
    GFL_HeapFree(fieldSound);
}

u32 FieldSnd_BGMGetStackIndex(FieldSound *fieldSound) {
    int index = fieldSound->bgmStackCount;
    FieldSoundCommand command;

    index += FieldSnd_GetCommandCountByID(fieldSound, FIELD_SND_BGM_PUSH);
    index -= FieldSnd_GetCommandCountByID(fieldSound, FIELD_SND_BGM_POP);

    command = fieldSound->command;
    if (command == FIELD_SND_BGM_PUSH &&
        (fieldSound->state == FIELD_SND_STATE_PUSHING || fieldSound->state == FIELD_SND_STATE_PUSHING_POSTPONED ||
         fieldSound->state == FIELD_SND_STATE_PUSHED_POSTPONED)) {
        index++;
    }
    if (command == FIELD_SND_BGM_POP && fieldSound->state == FIELD_SND_STATE_POPPING) {
        index--;
    }
    return index;
}

u32 FieldSnd_GetNowBGM(const FieldSound *fieldSound) {
    u32 stack[FIELD_SND_BGM_STACK_MAX];
    u32 bgm = fieldSound->currentBGM;
    int depth;
    int i;
    int pos;

    if (fieldSound->nextBGM != 0) {
        bgm = fieldSound->nextBGM;
    }
    if (fieldSound->loadingBGM != 0) {
        bgm = fieldSound->loadingBGM;
    }

    depth = fieldSound->bgmStackCount;
    pos = fieldSound->requestHead;
    for (i = 0; i < depth; i++) {
        stack[i] = fieldSound->bgmStack[i];
    }

    for (; pos != fieldSound->requestTail; pos = (pos + 1) % FIELD_SND_REQUEST_MAX) {
        const FieldSoundRequest *request = &fieldSound->requests[pos];

        if (request->command == FIELD_SND_BGM_PUSH) {
            stack[depth++] = bgm;
        } else if (request->command == FIELD_SND_BGM_POP) {
            bgm = stack[--depth];
        } else if (request->command == FIELD_SND_BGM_CHANGE || request->command == FIELD_SND_BGM_PREPARE ||
                   request->command == FIELD_SND_BGM_PLAY) {
            bgm = request->bgm;
        }
    }
    return bgm;
}

BOOL FieldSnd_IsBGMChanging(FieldSound *fieldSound) {
    FieldSoundState state = fieldSound->state;

    if (state == FIELD_SND_STATE_FADING_IN || state == FIELD_SND_STATE_FADING_OUT || state == FIELD_SND_STATE_POPPING ||
        state == FIELD_SND_STATE_POPPED || state == FIELD_SND_STATE_PUSHING || state == FIELD_SND_STATE_CHANGING_OUT ||
        state == FIELD_SND_STATE_CHANGED || state == FIELD_SND_STATE_PUSHING_POSTPONED ||
        state == FIELD_SND_STATE_PREPARING) {
        return TRUE;
    }
    return FALSE;
}

BOOL FieldSnd_IsBusy(FieldSound *fieldSound) {
    if (!FieldSnd_HasQueuedCommand(fieldSound) && fieldSound->command == FIELD_SND_CMD_NULL) {
        return FALSE;
    }
    return TRUE;
}

BOOL FieldSnd_IsPopDone(FieldSound *fieldSound) {
    if (fieldSound->command == FIELD_SND_BGM_POP && fieldSound->state == FIELD_SND_STATE_POPPING) {
        return FALSE;
    }
    if (FieldSnd_GetCommandCountByID(fieldSound, FIELD_SND_BGM_POP) != 0) {
        return FALSE;
    }
    return TRUE;
}

void FieldSnd_SendRequest(FieldSound *fieldSound, FieldSoundCommand command, u32 bgm, u16 fadeOutFrames,
                          u16 fadeInFrames) {
    FieldSoundRequest request;

    request.command = command;
    request.bgm = bgm;
    request.fadeOutFrames = fadeOutFrames;
    request.fadeInFrames = fadeInFrames;
    FieldSnd_SendRequestCore(fieldSound, &request);
}

void FieldSnd_Update(FieldSound *fieldSound) {
    FieldSnd_UpdateCommand(fieldSound);
    FIELD_SND_CONTROL_FUNCS[fieldSound->state](fieldSound);
    FIELD_SND_EXEC_FUNCS[fieldSound->state](fieldSound);
    RingtoneSys_Update(fieldSound->ringtone);
    PlayerVolumeFader_Update(fieldSound->volumeFader);
}

void FieldSnd_Release(FieldSound *fieldSound, GameData *gameData) {
    FieldSnd_ReleaseCore(fieldSound, gameData);
}

void FieldSnd_SetPlayerVolumeFade(FieldSound *fieldSound, u8 volume, u8 duration) {
    PlayerVolumeFader_SetFade(fieldSound->volumeFader, volume, duration);
}

void FieldSnd_RingRingtone(FieldSound *fieldSound) {
    RingtoneSys_Ring(fieldSound->ringtone);
}

void FieldSnd_StopRingtone(FieldSound *fieldSound) {
    RingtoneSys_Stop(fieldSound->ringtone);
}

void FieldSnd_PlayAmbience(FieldSound *fieldSound, u32 se) {
    if (!FieldAmbience_CheckSoundID(se)) {
        GFL_SndSEPlay(se);
        return;
    }

    if (FieldAmbience_IsLooped(se)) {
        int index = GFL_SndSeqGetPlayerIndex(se) - 1;

        fieldSound->ambienceSE[index] = se;
        fieldSound->ambienceVolume[index] = FIELD_SND_AMBIENCE_VOLUME_DEFAULT;
    }
    if (fieldSound->ambienceStopFlags == 0) {
        GFL_SndSEPlay(se);
    }
}

void FieldSnd_PlayAmbienceEx(FieldSound *fieldSound, u32 se, u32 volume) {
    if (!FieldAmbience_CheckSoundID(se)) {
        GFL_SndSEPlay(se);
        return;
    }

    if (FieldAmbience_IsLooped(se)) {
        int index = GFL_SndSeqGetPlayerIndex(se) - 1;

        fieldSound->ambienceSE[index] = se;
        fieldSound->ambienceVolume[index] = volume;
    }
    if (fieldSound->ambienceStopFlags == 0) {
        GFL_SndSEPlayEx(se, volume);
    }
}

void FieldSnd_SetAmbienceVolume(FieldSound *fieldSound, u32 se, u16 volume) {
    int i;

    if (!FieldAmbience_CheckSoundID(se)) {
        return;
    }

    for (i = 0; i < FIELD_SND_AMBIENCE_PLAYER_MAX; i++) {
        if (se == fieldSound->ambienceSE[i]) {
            GFL_SndPlayerSetVolume(i + 1, volume);
            fieldSound->ambienceVolume[i] = volume;
            return;
        }
    }
}

void FieldSnd_StopAmbience(FieldSound *fieldSound, u32 se) {
    s32 player = GFL_SndSeqGetPlayerIndex(se);

    if (!FieldAmbience_CheckSoundID(se)) {
        GFL_SndPlayerStop(player);
        return;
    }

    if (FieldAmbience_IsLooped(se) && se == fieldSound->ambienceSE[player - 1]) {
        fieldSound->ambienceSE[player - 1] = FIELD_SND_AMBIENCE_NONE;
        fieldSound->ambienceVolume[player - 1] = FIELD_SND_AMBIENCE_VOLUME_DEFAULT;
    }
    GFL_SndPlayerStop(player);
}

void FieldSnd_StopAmbienceAll(FieldSound *fieldSound, u32 flags) {
    int i;

    if (fieldSound->ambienceStopFlags == 0) {
        for (i = 0; i < FIELD_SND_AMBIENCE_PLAYER_MAX; i++) {
            if (fieldSound->ambienceSE[i] != FIELD_SND_AMBIENCE_NONE) {
                GFL_SndPlayerStop(i + 1);
            }
        }
    }
    fieldSound->ambienceStopFlags |= flags;
}

void FieldSnd_ResumeAmbience(FieldSound *fieldSound, u32 flags) {
    int i;

    if (!(fieldSound->ambienceStopFlags & flags)) {
        return;
    }

    fieldSound->ambienceStopFlags &= ~flags;
    if (fieldSound->ambienceStopFlags != 0) {
        return;
    }

    for (i = 0; i < FIELD_SND_AMBIENCE_PLAYER_MAX; i++) {
        if (fieldSound->ambienceSE[i] != FIELD_SND_AMBIENCE_NONE) {
            if (fieldSound->ambienceVolume[i] == FIELD_SND_AMBIENCE_VOLUME_DEFAULT) {
                GFL_SndSEPlay(fieldSound->ambienceSE[i]);
            } else {
                GFL_SndSEPlayEx(fieldSound->ambienceSE[i], fieldSound->ambienceVolume[i]);
            }
        }
    }
}

static BOOL FieldSnd_IsSongListed(u32 seq) {
    u32 i;

    for (i = 0; i < NELEMS(song_list); i++) {
        if (seq == song_list[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

static BOOL FieldSnd_IsSeqDataLoaded(u32 seq) {
    int i;
    const NNSSndArcSeqInfo *seqInfo = NNS_SndArcGetSeqInfo(seq);
    const NNSSndArcBankInfo *bankInfo;

    if (NNS_SndArcGetFileAddress(seqInfo->fileId) == NULL) {
        return FALSE;
    }

    bankInfo = NNS_SndArcGetBankInfo(seqInfo->param.bankNo);
    if (NNS_SndArcGetFileAddress(bankInfo->fileId) == NULL) {
        return FALSE;
    }

    for (i = 0; i < NNS_SND_ARC_BANK_TO_WAVEARC_NUM; i++) {
        if (bankInfo->waveArcNo[i] != NNS_SND_ARC_INVALID_WAVEARC_NO) {
            if (NNS_SndArcGetFileAddress(NNS_SndArcGetWaveArcInfo(bankInfo->waveArcNo[i])->fileId) == NULL) {
                return FALSE;
            }
        }
    }
    return TRUE;
}

static BOOL FieldSnd_CanPlaySeqWhileLoading(u32 seq) {
    if (!FieldSnd_IsSongListed(seq)) {
        return FALSE;
    }
    if (FieldSnd_IsSeqDataLoaded(seq)) {
        return TRUE;
    }
    return FALSE;
}

static void FieldSnd_Reset(FieldSound *fieldSound, GameData *gameData) {
    int i;

    fieldSound->gameData = gameData;
    fieldSound->state = FIELD_SND_STATE_INIT;
    fieldSound->command = FIELD_SND_CMD_NULL;
    fieldSound->currentBGM = 0;
    fieldSound->nextBGM = 0;
    fieldSound->loadingBGM = 0;
    fieldSound->loadStep = 0;
    fieldSound->fadeInFrames = 0;
    fieldSound->fadeOutFrames = 0;
    fieldSound->bgmStackCount = 0;
    for (i = 0; i < FIELD_SND_BGM_STACK_MAX; i++) {
        fieldSound->bgmStack[i] = 0;
    }
    for (i = 0; i < FIELD_SND_REQUEST_MAX; i++) {
        fieldSound->requests[i].command = FIELD_SND_CMD_NULL;
    }
    fieldSound->requestHead = 0;
    fieldSound->requestTail = 0;
    for (i = 0; i < FIELD_SND_AMBIENCE_PLAYER_MAX; i++) {
        fieldSound->ambienceSE[i] = FIELD_SND_AMBIENCE_NONE;
    }
    fieldSound->ambienceStopFlags = 0;
}

static void FieldSnd_ReleaseCore(FieldSound *fieldSound, GameData *gameData) {
    while (fieldSound->bgmStackCount != 0) {
        FieldSnd_BGMPop(fieldSound);
    }
    GFL_SndBGMStopImmediate();
    FieldSnd_Reset(fieldSound, gameData);
}

static void FieldSnd_UpdateCommand(FieldSound *fieldSound) {
    FieldSoundRequest *request = FieldSnd_GetQueuedCommand(fieldSound);

    if (FieldSnd_IsCommandPermitted(fieldSound, request->command)) {
        FieldSnd_SetCommand(fieldSound, request);
        FieldSnd_NextCommand(fieldSound);
    }
}

static FieldSoundRequest *FieldSnd_GetQueuedCommand(FieldSound *fieldSound) {
    return &fieldSound->requests[fieldSound->requestHead];
}

static FieldSoundRequest *FieldSnd_GetLastQueuedCommand(FieldSound *fieldSound) {
    int pos;

    if (FieldSnd_HasQueuedCommand(fieldSound) == FALSE) {
        pos = fieldSound->requestHead;
    } else {
        pos = (fieldSound->requestTail + FIELD_SND_REQUEST_MAX - 1) % FIELD_SND_REQUEST_MAX;
    }
    return &fieldSound->requests[pos];
}

static BOOL FieldSnd_HasQueuedCommand(FieldSound *fieldSound) {
    if (FieldSnd_GetQueuedCommand(fieldSound)->command != FIELD_SND_CMD_NULL) {
        return TRUE;
    }
    return FALSE;
}

static u8 FieldSnd_GetCommandCountByID(const FieldSound *fieldSound, FieldSoundCommand command) {
    int count = 0;
    int pos;

    for (pos = fieldSound->requestHead; pos != fieldSound->requestTail; pos = (pos + 1) % FIELD_SND_REQUEST_MAX) {
        if (command == fieldSound->requests[pos].command) {
            count++;
        }
    }
    return count;
}

static void FieldSnd_SendRequestCore(FieldSound *fieldSound, const FieldSoundRequest *request) {
    int tail = fieldSound->requestTail;

    if (!FieldSnd_IsCommandSuccessionPermitted(fieldSound, request->command)) {
        return;
    }
    if (fieldSound->requests[tail].command != FIELD_SND_CMD_NULL) {
        return;
    }

    fieldSound->requests[tail] = *request;
    fieldSound->requestTail = (tail + 1) % FIELD_SND_REQUEST_MAX;
}

static BOOL FieldSnd_IsCommandSuccessionPermitted(FieldSound *fieldSound, FieldSoundCommand command) {
    FieldSoundRequest *last = FieldSnd_GetLastQueuedCommand(fieldSound);

    if (last->command != FIELD_SND_CMD_NULL) {
        return FIELD_SND_COMMAND_ALLOWS_SUCCESSOR[last->command][command];
    }
    if (fieldSound->command != FIELD_SND_CMD_NULL) {
        return FIELD_SND_COMMAND_ALLOWS_SUCCESSOR[fieldSound->command][command];
    }
    return FieldSnd_IsCommandPermitted(fieldSound, command);
}

static void FieldSnd_NextCommand(FieldSound *fieldSound) {
    int head = fieldSound->requestHead;

    if (FieldSnd_HasQueuedCommand(fieldSound)) {
        fieldSound->requests[head].command = FIELD_SND_CMD_NULL;
        fieldSound->requestHead = (head + 1) % FIELD_SND_REQUEST_MAX;
    }
}

static void FieldSnd_SetCommand(FieldSound *fieldSound, const FieldSoundRequest *request) {
    switch (request->command) {
    case FIELD_SND_FADE_IN:
        FieldSnd_SetCommandFadeIn(fieldSound, request->fadeInFrames);
        break;
    case FIELD_SND_FADE_OUT:
        FieldSnd_SetCommandFadeOut(fieldSound, request->fadeOutFrames);
        break;
    case FIELD_SND_BGM_PUSH:
        FieldSnd_SetCommandPush(fieldSound, request->fadeOutFrames);
        break;
    case FIELD_SND_BGM_POP:
        FieldSnd_SetCommandPop(fieldSound, request->fadeOutFrames, request->fadeInFrames);
        break;
    case FIELD_SND_BGM_CHANGE:
        FieldSnd_SetCommandChange(fieldSound, request->bgm, request->fadeOutFrames, request->fadeInFrames);
        break;
    case FIELD_SND_BGM_PREPARE:
        FieldSnd_SetCommandPrepare(fieldSound, request->bgm, request->fadeOutFrames);
        break;
    case FIELD_SND_BGM_PLAY:
        FieldSnd_SetCommandPlay(fieldSound, request->bgm);
        break;
    }
}

static void FieldSnd_SetCommandFadeIn(FieldSound *fieldSound, u16 fadeInFrames) {
    if (FieldSnd_IsCommandPermitted(fieldSound, FIELD_SND_FADE_IN)) {
        fieldSound->command = FIELD_SND_FADE_IN;
        fieldSound->fadeInFrames = fadeInFrames;
    }
}

static void FieldSnd_SetCommandFadeOut(FieldSound *fieldSound, u16 fadeOutFrames) {
    if (FieldSnd_IsCommandPermitted(fieldSound, FIELD_SND_FADE_OUT)) {
        fieldSound->command = FIELD_SND_FADE_OUT;
        fieldSound->fadeOutFrames = fadeOutFrames;
    }
}

static void FieldSnd_SetCommandPush(FieldSound *fieldSound, u16 fadeOutFrames) {
    if (FieldSnd_IsCommandPermitted(fieldSound, FIELD_SND_BGM_PUSH)) {
        fieldSound->command = FIELD_SND_BGM_PUSH;
        fieldSound->fadeOutFrames = fadeOutFrames;
    }
}

static void FieldSnd_SetCommandPop(FieldSound *fieldSound, u16 fadeOutFrames, u16 fadeInFrames) {
    if (FieldSnd_IsCommandPermitted(fieldSound, FIELD_SND_BGM_POP)) {
        fieldSound->command = FIELD_SND_BGM_POP;
        fieldSound->fadeInFrames = fadeInFrames;
        fieldSound->fadeOutFrames = fadeOutFrames;
    }
}

static void FieldSnd_SetCommandChange(FieldSound *fieldSound, u32 bgm, u16 fadeOutFrames, u16 fadeInFrames) {
    if (FieldSnd_IsCommandPermitted(fieldSound, FIELD_SND_BGM_CHANGE)) {
        fieldSound->command = FIELD_SND_BGM_CHANGE;
        fieldSound->nextBGM = bgm;
        fieldSound->fadeInFrames = fadeInFrames;
        fieldSound->fadeOutFrames = fadeOutFrames;
    }
}

static void FieldSnd_SetCommandPrepare(FieldSound *fieldSound, u32 bgm, u16 fadeOutFrames) {
    if (FieldSnd_IsCommandPermitted(fieldSound, FIELD_SND_BGM_PREPARE)) {
        fieldSound->command = FIELD_SND_BGM_PREPARE;
        fieldSound->nextBGM = bgm;
        fieldSound->fadeOutFrames = fadeOutFrames;
    }
}

static void FieldSnd_SetCommandPlay(FieldSound *fieldSound, u16 bgm) {
    if (FieldSnd_IsCommandPermitted(fieldSound, FIELD_SND_BGM_PLAY)) {
        fieldSound->command = FIELD_SND_BGM_PLAY;
        fieldSound->nextBGM = bgm;
    }
}

static BOOL FieldSnd_IsCommandPermitted(FieldSound *fieldSound, FieldSoundCommand command) {
    return FIELD_SND_STATE_ALLOWS_COMMAND[fieldSound->state][command];
}

static void FieldSnd_Control_Init(FieldSound *fieldSound) {
    switch (fieldSound->command) {
    case FIELD_SND_BGM_POP:
        FieldSnd_BGMPop(fieldSound);
        FieldSnd_CmdFadeIn(fieldSound);
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_POPPED);
        break;
    case FIELD_SND_BGM_CHANGE:
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_CHANGING_OUT);
        break;
    case FIELD_SND_BGM_PREPARE:
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PREPARING);
        break;
    case FIELD_SND_BGM_PLAY:
        FieldSnd_CmdBGMPlay(fieldSound);
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PLAYING);
        FieldSnd_FinishCommand(fieldSound);
        break;
    }
}

static void FieldSnd_Control_Playing(FieldSound *fieldSound) {
    switch (fieldSound->command) {
    case FIELD_SND_FADE_IN:
        FieldSnd_DiscardCommand(fieldSound);
        break;
    case FIELD_SND_FADE_OUT:
        FieldSnd_CmdFadeOut(fieldSound);
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_FADING_OUT);
        break;
    case FIELD_SND_BGM_PUSH:
        FieldSnd_CmdFadeOut(fieldSound);
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PUSHING);
        break;
    case FIELD_SND_BGM_POP:
        FieldSnd_CmdFadeOut(fieldSound);
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_POPPING);
        break;
    case FIELD_SND_BGM_CHANGE:
        if (fieldSound->nextBGM == fieldSound->currentBGM) {
            FieldSnd_DiscardCommand(fieldSound);
            break;
        }
        FieldSnd_CmdFadeOut(fieldSound);
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_CHANGING_OUT);
        break;
    case FIELD_SND_BGM_PREPARE:
        if (fieldSound->nextBGM == fieldSound->currentBGM) {
            FieldSnd_DiscardCommand(fieldSound);
            break;
        }
        FieldSnd_CmdFadeOut(fieldSound);
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PREPARING);
        break;
    case FIELD_SND_BGM_PLAY:
        if (fieldSound->nextBGM == fieldSound->currentBGM) {
            FieldSnd_DiscardCommand(fieldSound);
            break;
        }
        FieldSnd_CmdBGMPlay(fieldSound);
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PLAYING);
        FieldSnd_FinishCommand(fieldSound);
        break;
    }
}

static void FieldSnd_Control_Idle(FieldSound *fieldSound) {
    switch (fieldSound->command) {
    case FIELD_SND_CMD_NULL:
        break;
    case FIELD_SND_FADE_IN:
        FieldSnd_CmdFadeIn(fieldSound);
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_FADING_IN);
        break;
    case FIELD_SND_BGM_PLAY:
        FieldSnd_CmdBGMPlay(fieldSound);
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PLAYING);
        FieldSnd_FinishCommand(fieldSound);
        break;
    }
}

static void FieldSnd_Control_FadeOut(FieldSound *fieldSound) {
}

static void FieldSnd_Control_FadeIn(FieldSound *fieldSound) {
}

static void FieldSnd_Control_Push(FieldSound *fieldSound) {
}

static void FieldSnd_Control_Pop(FieldSound *fieldSound) {
}

static void FieldSnd_Control_PopFinish(FieldSound *fieldSound) {
}

// A push while the BGM changes waits for the change to finish
static void FieldSnd_Control_ChangeOut(FieldSound *fieldSound) {
    switch (fieldSound->command) {
    case FIELD_SND_CMD_NULL:
        break;
    case FIELD_SND_BGM_PUSH:
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PUSHING_POSTPONED);
        break;
    case FIELD_SND_BGM_CHANGE:
        break;
    }
}

static void FieldSnd_Control_ChangeIn(FieldSound *fieldSound) {
    switch (fieldSound->command) {
    case FIELD_SND_CMD_NULL:
        break;
    case FIELD_SND_BGM_PUSH:
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PUSHED_POSTPONED);
        break;
    case FIELD_SND_BGM_CHANGE:
        break;
    }
}

static void FieldSnd_Control_ChangeFinish(FieldSound *fieldSound) {
}

static void FieldSnd_Control_PostponedPush(FieldSound *fieldSound) {
}

static void FieldSnd_Control_PostponedPushFinish(FieldSound *fieldSound) {
}

static void FieldSnd_Control_Prepare(FieldSound *fieldSound) {
}

static void FieldSnd_Control_PrepareFinish(FieldSound *fieldSound) {
}

static void FieldSnd_Exec_Init(FieldSound *fieldSound) {
}

static void FieldSnd_Exec_Playing(FieldSound *fieldSound) {
}

static void FieldSnd_Exec_Idle(FieldSound *fieldSound) {
}

static void FieldSnd_Exec_FadeOut(FieldSound *fieldSound) {
    if (GFL_SndBGMIsFading()) {
        return;
    }
    FieldSnd_SetState(fieldSound, FIELD_SND_STATE_IDLE);
    FieldSnd_FinishCommand(fieldSound);
}

static void FieldSnd_Exec_FadeIn(FieldSound *fieldSound) {
    if (GFL_SndBGMIsFading()) {
        return;
    }
    FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PLAYING);
    FieldSnd_FinishCommand(fieldSound);
}

static void FieldSnd_Exec_Push(FieldSound *fieldSound) {
    if (GFL_SndBGMIsFading()) {
        return;
    }
    FieldSnd_BGMPush(fieldSound);
    FieldSnd_SetState(fieldSound, FIELD_SND_STATE_INIT);
    FieldSnd_FinishCommand(fieldSound);
}

static void FieldSnd_Exec_Pop(FieldSound *fieldSound) {
    if (GFL_SndBGMIsFading()) {
        return;
    }
    FieldSnd_BGMPop(fieldSound);
    FieldSnd_CmdFadeIn(fieldSound);
    FieldSnd_SetState(fieldSound, FIELD_SND_STATE_POPPED);
}

static void FieldSnd_Exec_PopFinish(FieldSound *fieldSound) {
    if (GFL_SndBGMIsFading()) {
        return;
    }
    FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PLAYING);
    FieldSnd_FinishCommand(fieldSound);
}

static void FieldSnd_Exec_ChangeOut(FieldSound *fieldSound) {
    if (GFL_SndBGMIsFading()) {
        return;
    }
    if (FieldSnd_CheckEventsPaused(fieldSound)) {
        return;
    }
    FieldSnd_BGMChangeOut(fieldSound);
    FieldSnd_SetState(fieldSound, FIELD_SND_STATE_CHANGING_IN);
}

static void FieldSnd_Exec_ChangeIn(FieldSound *fieldSound) {
    if (!FieldSnd_BGMChangeIn(fieldSound)) {
        return;
    }

    // Another change came while this one loaded
    if (fieldSound->nextBGM != 0 && fieldSound->nextBGM != fieldSound->currentBGM) {
        GFL_SndBGMStopImmediate();
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_CHANGING_OUT);
        return;
    }
    FieldSnd_CmdFadeIn(fieldSound);
    FieldSnd_SetState(fieldSound, FIELD_SND_STATE_CHANGED);
}

static void FieldSnd_Exec_ChangeFinish(FieldSound *fieldSound) {
    if (GFL_SndBGMIsFading()) {
        return;
    }

    if (fieldSound->nextBGM != 0 && fieldSound->nextBGM != fieldSound->currentBGM) {
        FieldSnd_CmdFadeOut(fieldSound);
        FieldSnd_SetState(fieldSound, FIELD_SND_STATE_CHANGING_OUT);
        return;
    }
    FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PLAYING);
    FieldSnd_FinishCommand(fieldSound);
}

static void FieldSnd_Exec_PostponedPush(FieldSound *fieldSound) {
    if (GFL_SndBGMIsFading()) {
        return;
    }
    if (FieldSnd_CheckEventsPaused(fieldSound)) {
        return;
    }
    FieldSnd_BGMChangeOut(fieldSound);
    FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PUSHED_POSTPONED);
}

static void FieldSnd_Exec_PostponedPushFinish(FieldSound *fieldSound) {
    if (!FieldSnd_BGMChangeIn(fieldSound)) {
        return;
    }
    FieldSnd_BGMPush(fieldSound);
    FieldSnd_SetState(fieldSound, FIELD_SND_STATE_INIT);
    FieldSnd_FinishCommand(fieldSound);
}

static void FieldSnd_Exec_Prepare(FieldSound *fieldSound) {
    if (GFL_SndBGMIsFading()) {
        return;
    }
    FieldSnd_SetState(fieldSound, FIELD_SND_STATE_PREPARED);
}

// Starts the new BGM paused, for a fade in to start
static void FieldSnd_Exec_PrepareFinish(FieldSound *fieldSound) {
    FieldSnd_CmdBGMPlay(fieldSound);
    GFL_SndBGMSetPaused(TRUE);
    FieldSnd_SetState(fieldSound, FIELD_SND_STATE_IDLE);
    FieldSnd_FinishCommand(fieldSound);
}

static void FieldSnd_SetState(FieldSound *fieldSound, FieldSoundState state) {
    fieldSound->state = state;
}

static void FieldSnd_FinishCommandCore(FieldSound *fieldSound) {
    fieldSound->command = FIELD_SND_CMD_NULL;
    fieldSound->nextBGM = 0;
    fieldSound->fadeInFrames = 0;
    fieldSound->fadeOutFrames = 0;
}

static void FieldSnd_DiscardCommand(FieldSound *fieldSound) {
    if (fieldSound->command != FIELD_SND_CMD_NULL) {
        FieldSnd_FinishCommandCore(fieldSound);
    }
}

static void FieldSnd_FinishCommand(FieldSound *fieldSound) {
    if (fieldSound->command != FIELD_SND_CMD_NULL) {
        FieldSnd_FinishCommandCore(fieldSound);
    }
}

static void FieldSnd_CmdFadeIn(FieldSound *fieldSound) {
    if (fieldSound->currentBGM != 0) {
        GFL_SndBGMFadeIn(fieldSound->fadeInFrames);
        GFL_SndBGMSetPaused(FALSE);
    }
}

static void FieldSnd_CmdFadeOut(FieldSound *fieldSound) {
    if (fieldSound->currentBGM != 0) {
        GFL_SndBGMFadeOut(fieldSound->fadeOutFrames);
    }
}

static void FieldSnd_BGMPush(FieldSound *fieldSound) {
    if (fieldSound->currentBGM == 0 || fieldSound->bgmStackCount >= FIELD_SND_BGM_STACK_MAX) {
        return;
    }

    if (fieldSound->bgmStackCount == 0) {
        FieldSnd_StopAmbienceAll(fieldSound, FIELD_SND_AMBIENCE_STOP_BGM_PUSH);
    }
    GFL_SndBGMSetPaused(TRUE);
    GFL_SndBGMPush();
    fieldSound->bgmStack[fieldSound->bgmStackCount] = fieldSound->currentBGM;
    fieldSound->bgmStackCount++;
    fieldSound->currentBGM = 0;
}

static void FieldSnd_BGMPop(FieldSound *fieldSound) {
    if (fieldSound->bgmStackCount <= 0) {
        return;
    }

    GFL_SndBGMPop();
    GFL_SndBGMSetPaused(FALSE);
    fieldSound->bgmStackCount--;
    fieldSound->currentBGM = fieldSound->bgmStack[fieldSound->bgmStackCount];
    if (fieldSound->bgmStackCount == 0) {
        FieldSnd_ResumeAmbience(fieldSound, FIELD_SND_AMBIENCE_STOP_BGM_PUSH);
    }
}

// Starts loading the next BGM's data
static BOOL FieldSnd_BGMChangeOut(FieldSound *fieldSound) {
    if (fieldSound->nextBGM == 0) {
        return FALSE;
    }

    GFL_SndBGMLoadStep(fieldSound->nextBGM, &fieldSound->loadStep, TRUE);
    fieldSound->loadingBGM = fieldSound->nextBGM;
    fieldSound->nextBGM = 0;
    fieldSound->currentBGM = 0;
    func_02042a1c(2);
    return TRUE;
}

// Goes on loading the BGM, which becomes the current one once loaded
static BOOL FieldSnd_BGMChangeIn(FieldSound *fieldSound) {
    BOOL loaded = GFL_SndBGMLoadStep(fieldSound->loadingBGM, &fieldSound->loadStep, FALSE);

    func_02042a1c(2);
    if (loaded) {
        fieldSound->currentBGM = fieldSound->loadingBGM;
        fieldSound->loadingBGM = 0;
    }
    return loaded;
}

static void FieldSnd_CmdBGMPlay(FieldSound *fieldSound) {
    if (fieldSound->nextBGM != 0) {
        GFL_SndBGMPlay(fieldSound->nextBGM, SND_CHANNEL_MASK_ALL);
        fieldSound->currentBGM = fieldSound->nextBGM;
        fieldSound->nextBGM = 0;
        fieldSound->loadingBGM = 0;
    }
}

static BOOL FieldSnd_CheckEventsPaused(FieldSound *fieldSound) {
    return GameData_CheckEventsPaused(fieldSound->gameData);
}
