#include "app/local_tvt/local_tvt_callmode.h"
#include "types.h"
#include "app/local_tvt/local_tvt_calldata.h"
#include "app/local_tvt/local_tvt_callselect.h"
#include "app/local_tvt/local_tvt_chara.h"
#include "app/local_tvt/local_tvt_common.h"
#include "app/local_tvt/local_tvt_message.h"
#include "app/local_tvt/local_tvt_person.h"
#include "app/local_tvt/local_tvt_util.h"
#include "app/local_tvt_local.h"
#include "constants/sound.h"
#include "constants/zones.h"
#include "field/player_state.h"
#include "field/zone.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/tcb.h"
#include "save/save_control.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/time_icon.h"

// The calls the player makes on the Xtransceiver, named after the ROM's "local_tvt_callmode.c": the list of the
// persons, the ringing, and the call with the person picked

// The window that shows the Xtransceiver is ringing
struct LocalTvtCalling {
    BmpWin *window;
    WaitIcon *icon;
    LocalTvtMessage *message;
    u16 ring;
    u16 ringCount;
    u16 finished;
    u16 timer;
};

struct LocalTvtCallMode {
    u16 heapId;
    LocalTvtCallModeSetup setup;
    LocalTvtCallSelect *select;
    LocalTvtSeq *seq;
    LocalTvtPerson *person;
    LocalTvtCallData *callData;
    LocalTvtCalling calling;
    // The persons of the list
    u16 persons[LOCAL_TVT_SELECT_MAX];
    int selected;
    u32 timer;
    u32 ringCount;
};

struct LocalTvtCallPerson {
    u16 person;
    u16 nameMsg;
};

static void LocalTvtCallMode_SetMode(LocalTvtCallMode *mode, u32 callMode);
static void LocalTvtCallMode_GetSelectSetup(LocalTvtCallMode *mode, LocalTvtCallSelectSetup *setup);
static void LocalTvtCallMode_StartPerson(LocalTvtCallMode *mode, LocalTvtPersonFuncs *funcs);
static void LocalTvtCallMode_SeqFadeInSelect(LocalTvtSeq *seq, int *step, void *work);
static void LocalTvtCallMode_SeqFadeInCall(LocalTvtSeq *seq, int *step, void *work);
static void LocalTvtCallMode_SeqSelect(LocalTvtSeq *seq, int *step, void *work);
static void LocalTvtCallMode_SeqCalling(LocalTvtSeq *seq, int *step, void *work);
static void LocalTvtCallMode_SeqCall(LocalTvtSeq *seq, int *step, void *work);
static void LocalTvtCallMode_SeqFadeOut(LocalTvtSeq *seq, int *step, void *work);
static void LocalTvtCalling_Init(LocalTvtCalling *calling, LocalTvtMessage *message, u32 ringCount, HeapID heapId);
static void LocalTvtCalling_Exit(LocalTvtCalling *calling);
static void LocalTvtCalling_Main(LocalTvtCalling *calling);
static BOOL LocalTvtCalling_IsFinished(LocalTvtCalling *calling);

// The persons in the order of the list, and the messages of their names
static const LocalTvtCallPerson sLocalTvtCallPersons[LOCAL_TVT_CALL_COUNT] = {
    { LOCAL_TVT_CALL_DOCTOR, 4 },  { LOCAL_TVT_CALL_RIVAL, 1 },  { LOCAL_TVT_CALL_CHELEN, 2 },
    { LOCAL_TVT_CALL_SUPPORT, 3 }, { LOCAL_TVT_CALL_MOTHER, 5 }, { LOCAL_TVT_CALL_SECRET, 6 },
};

LocalTvtCallMode *LocalTvtCallMode_Create(const LocalTvtCallModeSetup *setup, HeapID heapId) {
    LocalTvtCallMode *mode = GFL_HeapAllocate(heapId, sizeof(LocalTvtCallMode), TRUE, "local_tvt_callmode.c", 127);

    mode->setup = *setup;
    mode->heapId = heapId;
    mode->ringCount = 2;
    mode->callData = LocalTvtCallData_Create(mode->setup.gsys, mode->setup.gameData, heapId);
    mode->seq = LocalTvtSeq_Create(mode, NULL, heapId);
    LocalTvtCallMode_SetMode(mode, mode->setup.mode);
    return mode;
}

void LocalTvtCallMode_Free(LocalTvtCallMode *mode) {
    LocalTvtSeq_Free(mode->seq);
    LocalTvtCallData_Free(mode->callData);
    GFL_HeapFree(mode);
}

BOOL LocalTvtCallMode_Main(LocalTvtCallMode *mode) {
    LocalTvtSeq_Main(mode->seq);
    if (LocalTvtSeq_IsEnd(mode->seq)) {
        return TRUE;
    }
    return FALSE;
}

static void LocalTvtCallMode_SetMode(LocalTvtCallMode *mode, u32 callMode) {
    switch (callMode) {
    case LOCAL_TVT_CALL_MODE_SELECT:
    default:
        LocalTvtSeq_SetNext(mode->seq, LocalTvtCallMode_SeqFadeInSelect);
        break;
    case LOCAL_TVT_CALL_MODE_SECRET:
        mode->persons[0] = LOCAL_TVT_CALL_SECRET;
        mode->selected = 0;
        LocalTvtSeq_SetNext(mode->seq, LocalTvtCallMode_SeqFadeInCall);
        break;
    }
}

static void LocalTvtCallMode_GetSelectSetup(LocalTvtCallMode *mode, LocalTvtCallSelectSetup *setup) {
    int count = 0;
    u32 i;

    sys_memset(setup, 0, sizeof(LocalTvtCallSelectSetup));
    setup->rival = getHollow_RivalData(GameData_GetSaveControl(mode->setup.gameData));
    setup->message = mode->setup.message;
    setup->unit = mode->setup.unit;
    setup->msgData = LocalTvtMessage_GetMenuMsgData(mode->setup.message);
    // Nobody can be called from the Plasma Frigate
    if (GetZoneParentZone(PlayerState_GetZoneID(GameData_GetPlayerState(mode->setup.gameData))) ==
        ZONE_PLASMA_FRIGATE) {
        return;
    }
    for (i = 0; i < LOCAL_TVT_CALL_COUNT && count < LOCAL_TVT_SELECT_MAX; i++) {
        u16 person = sLocalTvtCallPersons[i].person;

        if (LocalTvtCallData_CanCall(mode->callData, person) && LocalTvtCallData_IsListed(mode->callData, person)) {
            u32 name;

            if (i == LOCAL_TVT_CALL_SECRET) {
                BOOL male = LocalTvtCallData_IsPlayerMale(mode->callData);

                person = LOCAL_TVT_CALL_SECRET;
                if (!male) {
                    name = 6;
                } else {
                    name = 7;
                }
            } else {
                name = sLocalTvtCallPersons[i].nameMsg;
            }
            mode->persons[count] = person;
            setup->names[count] = name;
            setup->enabled[count] = TRUE;
            count++;
        }
    }
    setup->count = count;
}

// Sets the faces of the call with the person picked: the person at the top and the player at the bottom
static void LocalTvtCallMode_StartPerson(LocalTvtCallMode *mode, LocalTvtPersonFuncs *funcs) {
    LocalTvtFaceSetup setups[LOCAL_TVT_FACES];
    int i;
    u16 zoneId;
    BOOL unk;

    sys_memset(setups, 0, sizeof(setups));
    sys_memset(funcs, 0, sizeof(LocalTvtPersonFuncs));
    for (i = 0; i < LOCAL_TVT_FACES; i++) {
        setups[i].nameMsg = LOCAL_TVT_NAME_DEFAULT;
    }
    zoneId = PlayerState_GetZoneID(GameData_GetPlayerState(mode->setup.gameData));
    setups[1].background = LocalTvtCallData_GetZoneBackground(mode->callData, zoneId);
    setups[1].object = LocalTvtCallData_GetZoneObject(mode->callData, zoneId);
    setups[1].person = LOCAL_TVT_PERSON_PLAYER;
    setups[2].person = LOCAL_TVT_PERSON_NONE;
    setups[3].person = LOCAL_TVT_PERSON_NONE;
    unk = FALSE;
    switch (mode->persons[mode->selected]) {
    case LOCAL_TVT_CALL_DOCTOR:
        setups[0].person = 4;
        setups[0].background = LocalTvtCallData_GetBackground(mode->callData, LOCAL_TVT_CALL_DOCTOR);
        setups[0].object = LocalTvtCallData_GetObject(mode->callData, LOCAL_TVT_CALL_DOCTOR);
        LocalTvtDoctor_Setup(funcs, LocalTvtMessage_GetTalkMsgData(mode->setup.message), mode->setup.gameData,
                             mode->heapId);
        break;
    case LOCAL_TVT_CALL_RIVAL:
        setups[0].person = LOCAL_TVT_PERSON_RIVAL;
        setups[0].background = LocalTvtCallData_GetBackground(mode->callData, LOCAL_TVT_CALL_RIVAL);
        setups[0].object = LocalTvtCallData_GetObject(mode->callData, LOCAL_TVT_CALL_RIVAL);
        LocalTvtRival_Setup(funcs, LocalTvtMessage_GetTalkMsgData(mode->setup.message), mode->setup.gameData,
                            mode->heapId);
        break;
    case LOCAL_TVT_CALL_CHELEN:
        setups[0].person = 2;
        setups[0].background = LocalTvtCallData_GetBackground(mode->callData, LOCAL_TVT_CALL_CHELEN);
        setups[0].object = LocalTvtCallData_GetObject(mode->callData, LOCAL_TVT_CALL_CHELEN);
        LocalTvtChelen_Setup(funcs, LocalTvtMessage_GetTalkMsgData(mode->setup.message), mode->setup.gameData,
                             mode->heapId);
        break;
    case LOCAL_TVT_CALL_SUPPORT:
        setups[0].person = 3;
        setups[0].background = LocalTvtCallData_GetBackground(mode->callData, LOCAL_TVT_CALL_SUPPORT);
        setups[0].object = LocalTvtCallData_GetObject(mode->callData, LOCAL_TVT_CALL_SUPPORT);
        LocalTvtSupport_Setup(funcs, LocalTvtMessage_GetTalkMsgData(mode->setup.message), mode->setup.gameData,
                              mode->heapId);
        break;
    case LOCAL_TVT_CALL_MOTHER:
        setups[0].person = 5;
        setups[0].background = LocalTvtCallData_GetBackground(mode->callData, LOCAL_TVT_CALL_MOTHER);
        setups[0].object = LocalTvtCallData_GetObject(mode->callData, LOCAL_TVT_CALL_MOTHER);
        LocalTvtMother_Setup(funcs, mode->setup.message, LocalTvtMessage_GetTalkMsgData(mode->setup.message),
                             mode->setup.gameData, mode->heapId);
        break;
    case LOCAL_TVT_CALL_SECRET:
        setups[0].person = LocalTvtSecret_GetFacePerson(mode->callData);
        setups[0].background = LocalTvtCallData_GetBackground(mode->callData, LOCAL_TVT_CALL_SECRET);
        setups[0].object = LocalTvtCallData_GetObject(mode->callData, LOCAL_TVT_CALL_SECRET);
        setups[0].nameMsg = LocalTvtSecret_GetNameMsg(mode->callData);
        if (mode->setup.mode == LOCAL_TVT_CALL_MODE_2) {
            unk = TRUE;
        }
        LocalTvtSecret_Setup(funcs, LocalTvtMessage_GetTalkMsgData(mode->setup.message), mode->setup.gameData,
                             mode->callData, unk, mode->heapId);
        break;
    default:
        LocalTvtSupport_Setup(funcs, LocalTvtMessage_GetTalkMsgData(mode->setup.message), mode->setup.gameData,
                              mode->heapId);
        break;
    }
    LocalTvtChara_Start(mode->setup.chara, setups, 2);
}

static void LocalTvtCallMode_SeqFadeInSelect(LocalTvtSeq *seq, int *step, void *work) {
    LocalTvtCallMode *mode = work;

    switch (*step) {
    case 0: {
        LocalTvtCallSelectSetup setup;

        LocalTvtCallMode_GetSelectSetup(mode, &setup);
        mode->select = LocalTvtCallSelect_Create(&setup, mode->heapId);
        func_02042ba8(FALSE, mode->heapId);
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        (*step)++;
        break;
    }
    case 1:
        if (!GFL_FadeIsRunning()) {
            (*step)++;
        }
        break;
    case 2:
        LocalTvtSeq_SetNext(seq, LocalTvtCallMode_SeqSelect);
        break;
    }
}

static void LocalTvtCallMode_SeqFadeInCall(LocalTvtSeq *seq, int *step, void *work) {
    LocalTvtCallMode *mode = work;

    switch (*step) {
    case 0: {
        LocalTvtPersonFuncs funcs;
        LocalTvtPersonSetup setup;

        LocalTvtCallMode_StartPerson(mode, &funcs);
        sys_memset(&setup, 0, sizeof(setup));
        setup.message = mode->setup.message;
        setup.chara = mode->setup.chara;
        setup.unit = mode->setup.unit;
        setup.callData = mode->callData;
        mode->person = LocalTvtPerson_Create(&funcs, &setup, mode->heapId);
        (*step)++;
        break;
    }
    case 1:
        if (LocalTvtChara_IsReady(mode->setup.chara)) {
            LocalTvtChara_SetVisible(mode->setup.chara, TRUE);
            (*step)++;
        }
        break;
    case 2:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        (*step)++;
        break;
    case 3:
        if (!GFL_FadeIsRunning()) {
            (*step)++;
        }
        break;
    case 4:
        LocalTvtSeq_SetNext(seq, LocalTvtCallMode_SeqCall);
        break;
    }
}

static void LocalTvtCallMode_SeqSelect(LocalTvtSeq *seq, int *step, void *work) {
    LocalTvtCallMode *mode = work;

    switch (*step) {
    case 0:
        if (mode->select == NULL) {
            LocalTvtCallSelectSetup setup;

            LocalTvtCallMode_GetSelectSetup(mode, &setup);
            mode->select = LocalTvtCallSelect_Create(&setup, mode->heapId);
            func_02042ba8(FALSE, mode->heapId);
        }
        (*step)++;
        break;
    case 1:
        LocalTvtCallSelect_Main(mode->select);
        if (LocalTvtCallSelect_GetResult(mode->select) == LOCAL_TVT_SELECT_CALL) {
            *step = 2;
        } else if (LocalTvtCallSelect_GetResult(mode->select) == LOCAL_TVT_SELECT_EXIT) {
            *step = 3;
        }
        break;
    case 2:
        mode->selected = LocalTvtCallSelect_GetSelected(mode->select);
        LocalTvtCallSelect_Free(mode->select);
        mode->select = NULL;
        LocalTvtSeq_SetNext(seq, LocalTvtCallMode_SeqCalling);
        break;
    case 3:
        LocalTvtCallSelect_Free(mode->select);
        mode->select = NULL;
        LocalTvtSeq_SetNext(seq, LocalTvtCallMode_SeqFadeOut);
        break;
    }
}

static void LocalTvtCallMode_SeqCalling(LocalTvtSeq *seq, int *step, void *work) {
    LocalTvtCallMode *mode = work;

    switch (*step) {
    case 0:
        if (mode->persons[mode->selected] == LOCAL_TVT_CALL_SECRET) {
            mode->ringCount = LocalTvtSecret_GetRingCount(mode->callData);
        }
        LocalTvtCalling_Init(&mode->calling, mode->setup.message, mode->ringCount, mode->heapId);
        (*step)++;
        break;
    case 1:
        LocalTvtCalling_Main(&mode->calling);
        if (LocalTvtCalling_IsFinished(&mode->calling)) {
            (*step)++;
        }
        break;
    case 2:
        LocalTvtCalling_Exit(&mode->calling);
        LocalTvtSeq_SetNext(seq, LocalTvtCallMode_SeqCall);
        break;
    }
}

static void LocalTvtCallMode_SeqCall(LocalTvtSeq *seq, int *step, void *work) {
    LocalTvtCallMode *mode = work;

    switch (*step) {
    case 0:
        if (mode->person == NULL) {
            LocalTvtPersonFuncs funcs;
            LocalTvtPersonSetup setup;

            LocalTvtCallMode_StartPerson(mode, &funcs);
            sys_memset(&setup, 0, sizeof(setup));
            setup.message = mode->setup.message;
            setup.chara = mode->setup.chara;
            setup.unit = mode->setup.unit;
            setup.callData = mode->callData;
            mode->person = LocalTvtPerson_Create(&funcs, &setup, mode->heapId);
            (*step)++;
        } else {
            *step = 2;
        }
        break;
    case 1:
        if (LocalTvtChara_IsReady(mode->setup.chara)) {
            LocalTvtChara_SetVisible(mode->setup.chara, TRUE);
            GFL_SndSEPlay(SEQ_SE_SYS_72);
            (*step)++;
        }
        break;
    case 2:
        LocalTvtPerson_Main(mode->person);
        if (LocalTvtPerson_IsEnd(mode->person)) {
            (*step)++;
        }
        break;
    case 3:
        LocalTvtPerson_Free(mode->person);
        mode->person = NULL;
        mode->timer = 0;
        (*step)++;
        break;
    case 4:
        if (mode->timer++ > 30) {
            GFL_SndSEPlay(SEQ_SE_SYS_72);
            if (mode->setup.mode == LOCAL_TVT_CALL_MODE_SELECT) {
                LocalTvtChara_SetVisible(mode->setup.chara, FALSE);
                LocalTvtSeq_SetNext(seq, LocalTvtCallMode_SeqSelect);
            } else {
                LocalTvtSeq_SetNext(seq, LocalTvtCallMode_SeqFadeOut);
            }
        }
        break;
    }
}

static void LocalTvtCallMode_SeqFadeOut(LocalTvtSeq *seq, int *step, void *work) {
    switch (*step) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        (*step)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            (*step)++;
        }
        break;
    case 2:
        LocalTvtSeq_End(seq);
        break;
    }
}

static void LocalTvtCalling_Init(LocalTvtCalling *calling, LocalTvtMessage *message, u32 ringCount, HeapID heapId) {
    BmpWin *window;
    LocalTvtMessage *printMessage;

    sys_memset(calling, 0, sizeof(LocalTvtCalling));
    calling->message = message;
    calling->ringCount = ringCount;
    window = BmpWin_CreateDynamic(4, 2, 9, 28, 2, 15, TRUE);
    calling->window = window;
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
    LocalTvtMessage_SetColor(calling->message, PRINT_COLOR(1, 2, 15));
    LoadSysMsgBox(4, 0x140, 9, 0, heapId);
    printMessage = calling->message;
    window = calling->window;
    LocalTvtMessage_PrintMsg(printMessage, window, LocalTvtMessage_GetTalkMsgData(printMessage), 2);
    LocalTvtMessage_DrawFrame(calling->message, calling->window);
    calling->icon = WaitIcon_Create(GFL_VBlankGetTCBMgr(), calling->window, 15, 16, heapId);
    GFL_BGSysLoadScr(4);
    GFL_SEPlayKeepVol(SEQ_SE_SYS_71, 2);
}

static void LocalTvtCalling_Exit(LocalTvtCalling *calling) {
    BmpWin *window;

    WaitIcon_Free(calling->icon);
    LocalTvtMessage_ClearFrame(calling->message, calling->window);
    window = calling->window;
    BmpWin_ClearScreen(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    BmpWin_Free(calling->window);
    sys_memset(calling, 0, sizeof(LocalTvtCalling));
}

static void LocalTvtCalling_Main(LocalTvtCalling *calling) {
    if (!calling->finished) {
        if (calling->timer++ >= 80) {
            calling->timer = 0;
            calling->ring++;
            if (calling->ring >= calling->ringCount) {
                GFL_SndStop();
                calling->finished = TRUE;
            } else {
                GFL_SEPlayKeepVol(SEQ_SE_SYS_71, 2);
            }
        }
    }
}

static BOOL LocalTvtCalling_IsFinished(LocalTvtCalling *calling) {
    return calling->finished;
}
