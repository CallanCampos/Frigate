#include "app/comm_tvt/ctvt_call.h"
#include "types.h"
#include "app/comm_tvt.h"
#include "app/comm_tvt/comm_tvt_sys.h"
#include "app/comm_tvt/ctvt_camera.h"
#include "app/comm_tvt/ctvt_comm.h"
#include "constants/sound.h"
#include "constants/version.h"
#include "field/zone.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_whpipe.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "save/player_info.h"
#include "save/wifi_list.h"
#include "system/app_taskmenu.h"
#include "system/bmp_winframe.h"
#include "system/game_beacon.h"
#include "system/ctvt_beacon.h"
#include "system/game_data.h"
#include "system/printsys.h"
#include "system/wipe.h"
#include "system/wordset.h"

// The Xtransceiver's calls. The bottom screen lists the machines nearby whose players are friends: Xtransceivers in a
// call, which can be joined while it has room, and games in the field, up to three of which can be called. Then it
// waits for the call to connect, ringing while it calls

#define CTVT_CALL_ROWS 15
#define CTVT_CALL_ENTRIES 15
#define CTVT_CALL_INVITES 3
#define CTVT_CALL_NONE 0xff

// The rows the list shows at once, and their height in pixels
#define CTVT_CALL_VISIBLE_ROWS 5
#define CTVT_CALL_ROW_HEIGHT 32
// How far the scroll bar moves, in pixels
#define CTVT_CALL_SCROLL_BAR_RANGE 120

// The members a call can have
#define CTVT_CALL_MEMBERS_MAX 4

// The GFL game service IDs of the beacons listed: a game in the field, and an Xtransceiver
#define CTVT_CALL_SERVICE_FIELD 3
#define CTVT_CALL_SERVICE_CTVT 0x20

// How long to wait for a call to connect, and how often it rings, in frames
#define CTVT_CALL_WAIT_FRAMES 1200
#define CTVT_CALL_RING_FRAMES 80
// How long the notice about the photos stays up
#define CTVT_CALL_PHOTO_NOTICE_FRAMES 300

enum {
    CALL_STATE_FADE_IN,
    CALL_STATE_WAIT_FADE_IN,
    // The call connected: on to the talk
    CALL_STATE_FADE_OUT,
    // Leaving, once the camera's sound is done
    CALL_STATE_LEAVE,
    CALL_STATE_WAIT_FADE_OUT,
    CALL_STATE_LIST,
    // A button was pressed, which flashes before it does anything
    CALL_STATE_DECIDED,
    CALL_STATE_JOINING,
    CALL_STATE_WAITING,
    CALL_STATE_CALLING,
    // The call failed: a message, until the screen is tapped
    CALL_STATE_FAILED,
    CALL_STATE_PHOTO_NOTICE,
    CALL_STATE_WAIT_PHOTO_NOTICE,
};

// What the button of the bottom screen does
enum {
    CALL_MENU_NONE,
    CALL_MENU_CALL,
    CALL_MENU_JOIN,
    // Waiting to be called, with no button
    CALL_MENU_WAIT,
    // Back to the list, whose button isn't made yet
    CALL_MENU_RESET,
    // The return button was pressed
    CALL_MENU_BACK,
};

// What the list's check box shows
enum {
    CALL_CHECK_OFF = 9,
    CALL_CHECK_ON,
    CALL_CHECK_DISABLED,
};

// The beacon of a game in the field: a header, then the game's beacon. Its owner isn't decompiled; the header's fields
// are from their use here
typedef struct {
    u8 unk00[2];
    u8 unk02_0 : 1;
    // The list shows the game only when this is 2
    u8 kind : 5;
    u8 cameraBlocked : 1;
    u8 cameraEnabled : 1;
    u8 unk03;
    u32 trainerId;
    GameBeacon beacon;
} CtvtCallFieldBeacon;

// A machine found by scanning
typedef struct {
    BOOL active;
    // Set before each scan and cleared when the machine is found again, so that the machines gone can be dropped
    BOOL stale;
    // To be joined or called
    BOOL selected;
    BOOL cameraEnabled;
    // Makes its row flash
    BOOL changed;
    // A CtvtCommBeacon or a CtvtCallFieldBeacon, by gameServiceId
    void *beacon;
    u8 gameServiceId;
    u8 mac[6];
    u8 memberCount;
    u8 row;
    u8 romVersion;
} CtvtCallEntry;

// A row of the list, on BG 6
typedef struct {
    BOOL needsDraw;
    BOOL printPending;
    ClActor *frame;
    ClActor *check;
    ClActor *camera;
    BmpWin *window;
    // An index into the entries, or CTVT_CALL_NONE when the row is empty
    u8 entry;
} CtvtCallRow;

struct CtvtCall {
    // The entries to call, CTVT_CALL_NONE where free
    u8 invites[CTVT_CALL_INVITES];
    // The entry to join, or CTVT_CALL_NONE
    u8 joinTarget;
    u16 scrollY;
    u16 scrollBarY;
    int state;
    BOOL listDirty;
    BOOL scrollBarHeld;
    u8 rowCount;
    u8 prevRowCount;
    u32 menuMode;
    AppTaskMenuWin *menu;
    BOOL msgWinPending;
    BOOL infoWinPending;
    BOOL recordingStarted;
    // The bottom screen's message and the top screen's
    BmpWin *msgWin;
    BmpWin *infoWin;
    u16 waitTimer;
    u16 ringTimer;
    // Whether to show the top screen's message once it is printed
    BOOL showInfo;
    u32 unk3C;
    CtvtCallRow rows[CTVT_CALL_ROWS];
    CtvtCallEntry entries[CTVT_CALL_ENTRIES];
    ClActor *returnButton;
    ClActor *scrollBar;
    s32 photoTimer;
};

static void CtvtCall_UpdateTouch(CommTvtWork *sys, CtvtCall *call);
static void CtvtCall_Scan(CommTvtWork *sys, CtvtCall *call);
static void CtvtCall_DrawRows(CommTvtWork *sys, CtvtCall *call);
static void CtvtCall_DrawRow(CommTvtWork *sys, CtvtCall *call, CtvtCallRow *row, u8 index);
static void CtvtCall_UpdateRowActors(CommTvtWork *sys, CtvtCall *call, CtvtCallRow *row, u8 index);
static void CtvtCall_UpdateMenu(CommTvtWork *sys, CtvtCall *call);
static void CtvtCall_UpdateFoundMessage(CommTvtWork *sys, CtvtCall *call);
static BOOL CtvtCall_IsFriend(CommTvtWork *sys, CtvtCall *call, const u16 *name, u32 id, u32 gender);
static void CtvtCall_PrintMessage(CommTvtWork *sys, CtvtCall *call, u32 msgId);
static void CtvtCall_PrintInfo(CommTvtWork *sys, CtvtCall *call, u32 msgId);

static const TouchRect sReturnButton[] = {
    { 0xa8, 0xc0, 0xe0, 0xf8 },
    { TOUCH_RECT_END },
};

CtvtCall *CtvtCall_Create(CommTvtWork *sys, HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(CtvtCall), TRUE, "ctvt_call.c", 220);
}

void CtvtCall_Delete(CommTvtWork *sys, CtvtCall *call) {
    GFL_HeapFree(call);
}

// The resources of the actors are the system's: characters, palette and cells
static inline void CtvtCall_CreateActor(CommTvtWork *sys, ClActor **actor, int chars, int palette, int cells,
                                        const ClActorSetup *setup, HeapID heapId) {
    *actor = func_0204c040(CommTvt_GetClActUnit(sys), CommTvt_GetObjResource(sys, chars),
                           CommTvt_GetObjResource(sys, palette), CommTvt_GetObjResource(sys, cells), setup, 1, heapId);
}

void CtvtCall_Enter(CommTvtWork *sys, CtvtCall *call) {
    HeapID heapId = CommTvt_GetHeapId(sys);
    ArcTool *arc = CommTvt_GetArc(sys);
    CommTvtParam *param = CommTvt_GetParam(sys);
    ClActorSetup setup;
    u8 i;
    u8 j;

    if (param->unk4 != 2 && param->unk4 != 3 && CommTvt_CanExchangePhotos(sys) == FALSE) {
        GFL_BGSysLoadArcNCGRStatic(arc, 12, 4, 0, 0x2800, FALSE, heapId);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 18, 4, 0, 0, FALSE, heapId);
        GFL_BGSysLoadScr(4);
    }

    setup.bgPriority = 3;
    for (i = 0; i < CTVT_CALL_ROWS; i++) {
        setup.x = 128;
        setup.y = i * CTVT_CALL_ROW_HEIGHT + 16;
        setup.sequence = 8;
        setup.priority = 32;
        CtvtCall_CreateActor(sys, &call->rows[i].frame, 5, 1, 9, &setup, heapId);
        func_0204c124(call->rows[i].frame, FALSE);
        func_0204c520(call->rows[i].frame, TRUE);

        setup.x = 18;
        setup.sequence = CALL_CHECK_OFF;
        setup.priority = 16;
        CtvtCall_CreateActor(sys, &call->rows[i].check, 5, 1, 9, &setup, heapId);
        func_0204c124(call->rows[i].check, FALSE);

        setup.x = 200;
        setup.sequence = 18;
        setup.priority = 16;
        CtvtCall_CreateActor(sys, &call->rows[i].camera, 5, 1, 9, &setup, heapId);
        func_0204c124(call->rows[i].camera, FALSE);
    }

    setup.x = 224;
    setup.y = 168;
    if (CommTvt_CanExchangePhotos(sys) == FALSE) {
        setup.sequence = 1;
    } else {
        setup.sequence = 15;
    }
    setup.priority = 0;
    setup.bgPriority = 0;
    CtvtCall_CreateActor(sys, &call->returnButton, 6, 2, 10, &setup, heapId);
    func_0204c124(call->returnButton, TRUE);
    func_0204c520(call->returnButton, TRUE);

    setup.x = 248;
    setup.y = 24;
    setup.sequence = 12;
    CtvtCall_CreateActor(sys, &call->scrollBar, 5, 1, 9, &setup, heapId);
    func_0204c124(call->scrollBar, FALSE);

    for (j = 0; j < CTVT_CALL_ENTRIES; j++) {
        call->entries[j].active = FALSE;
        call->entries[j].stale = FALSE;
        call->entries[j].selected = FALSE;
        call->entries[j].changed = FALSE;
        call->entries[j].row = CTVT_CALL_NONE;
        call->entries[j].beacon = GFL_HeapAllocate(heapId, sizeof(CtvtCallFieldBeacon), TRUE, "ctvt_call.c", 334);
        call->entries[j].memberCount = 0;
    }
    for (i = 0; i < CTVT_CALL_ROWS; i++) {
        call->rows[i].needsDraw = FALSE;
        call->rows[i].printPending = FALSE;
        call->rows[i].entry = CTVT_CALL_NONE;
        call->rows[i].window = BmpWin_CreateDynamic(6, 3, i * 4 + 1, 25, 2, 10, TRUE);
    }
    call->msgWin = BmpWin_CreateDynamic(4, 1, 21, 21, 2, 10, TRUE);
    call->infoWin = BmpWin_CreateDynamic(4, 2, 9, 28, 2, 10, TRUE);

    call->showInfo = FALSE;
    call->ringTimer = 0;
    for (i = 0; i < CTVT_CALL_INVITES; i++) {
        call->invites[i] = CTVT_CALL_NONE;
    }
    call->joinTarget = CTVT_CALL_NONE;
    call->scrollY = 0;
    call->scrollBarY = 0;
    call->rowCount = 0;
    call->prevRowCount = CTVT_CALL_NONE;
    // Not one of the menu modes, so that the first update makes the menu
    call->menuMode = 0xff;
    call->listDirty = FALSE;
    call->scrollBarHeld = FALSE;
    call->msgWinPending = FALSE;
    call->infoWinPending = FALSE;
    call->recordingStarted = FALSE;
    call->menu = NULL;
    call->state = CALL_STATE_FADE_IN;
    call->waitTimer = 0;
    call->photoTimer = 0;
    GFL_BGSysMoveBGReq(6, 3, 0);

    if ((param->unk4 == 2 || param->unk4 == 3) && CommTvt_CanExchangePhotos(sys) == FALSE) {
        func_0204c488(call->returnButton, 15);
        CtvtCall_PrintInfo(sys, call, 24);
        call->showInfo = TRUE;
        call->menuMode = CALL_MENU_WAIT;
    }
}

static inline void CtvtCall_ClearWindow(BmpWin *window) {
    BmpWin_ClearScreen(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
}

static inline void CtvtCall_ClearWindowQueued(BmpWin *window) {
    BmpWin_ClearScreen(window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
}

void CtvtCall_Leave(CommTvtWork *sys, CtvtCall *call) {
    u8 i;

    if (call->menu != NULL) {
        AppTaskMenuWin_Free(call->menu);
        call->menu = NULL;
    }
    CtvtCall_ClearWindow(call->msgWin);
    BmpWin_Free(call->msgWin);
    CtvtCall_ClearWindow(call->infoWin);
    BmpWin_Free(call->infoWin);
    func_0204c108(call->scrollBar);
    func_0204c108(call->returnButton);
    for (i = 0; i < CTVT_CALL_ROWS; i++) {
        BmpWin_Free(call->rows[i].window);
        func_0204c108(call->rows[i].camera);
        func_0204c108(call->rows[i].check);
        func_0204c108(call->rows[i].frame);
    }
    for (i = 0; i < CTVT_CALL_ENTRIES; i++) {
        GFL_HeapFree(call->entries[i].beacon);
    }
    GFL_BGSysClearScr(6);
    GFL_BGSysLoadScr(6);
    GFL_BGSysMoveBGReq(6, 3, 0);
    GFL_BGSysClearScr(4);
    GFL_BGSysLoadScr(4);
    GFL_BGSysMoveBGReq(4, 3, 0);
}

int CtvtCall_Main(CommTvtWork *sys, CtvtCall *call) {
    HeapID heapId = CommTvt_GetHeapId(sys);
    CommTvtParam *param = CommTvt_GetParam(sys);

    switch (call->state) {
    case CALL_STATE_FADE_IN:
        if (func_ov257_021aaa74(sys) == TRUE) {
            GFL_WipeSet(0, 1, 1, 0, 6, 1, heapId);
        } else {
            GFL_WipeSet(4, 1, 1, 0, 6, 1, heapId);
        }
        call->state = CALL_STATE_WAIT_FADE_IN;
        break;
    case CALL_STATE_WAIT_FADE_IN:
        if (GFL_WipeIsFinished() == TRUE) {
            CommTvtParam *param = CommTvt_GetParam(sys);

            if (CommTvt_CanExchangePhotos(sys) == TRUE) {
                call->state = CALL_STATE_PHOTO_NOTICE;
            } else if (param->unk4 == 2 || param->unk4 == 3) {
                call->state = CALL_STATE_WAITING;
            } else {
                call->state = CALL_STATE_LIST;
            }
        }
        break;
    case CALL_STATE_FADE_OUT:
        GFL_WipeSet(4, 0, 0, 0, 6, 1, heapId);
        func_ov257_021aaa78(sys, FALSE);
        call->state = CALL_STATE_WAIT_FADE_OUT;
        break;
    case CALL_STATE_LEAVE:
        if (CtvtCamera_IsSoundDone(sys, CommTvt_GetCamera(sys)) == TRUE) {
            GFL_WipeSet(0, 0, 0, 0, 6, 1, heapId);
            func_ov257_021aaa78(sys, FALSE);
            call->state = CALL_STATE_WAIT_FADE_OUT;
        }
        break;
    case CALL_STATE_WAIT_FADE_OUT:
        if (GFL_WipeIsFinished() == TRUE) {
            if (call->menuMode == CALL_MENU_NONE || call->menuMode == CALL_MENU_BACK) {
                return COMM_TVT_MODE_EXIT;
            }
            return COMM_TVT_MODE_TALK;
        }
        break;
    case CALL_STATE_LIST:
        CtvtCall_UpdateTouch(sys, call);
        CtvtCall_UpdateFoundMessage(sys, call);
        break;
    case CALL_STATE_DECIDED:
        if (call->menuMode == CALL_MENU_NONE || call->menuMode == CALL_MENU_BACK) {
            if (func_0204c560(call->returnButton) == FALSE) {
                CtvtCamera_StopCamera(sys, CommTvt_GetCamera(sys));
                call->menuMode = CALL_MENU_NONE;
                call->state = CALL_STATE_LEAVE;
            }
        } else if (AppTaskMenuWin_IsFlashFinished(call->menu) == TRUE) {
            if (call->menuMode == CALL_MENU_JOIN) {
                CtvtComm *comm = CommTvt_GetComm(sys);

                CtvtComm_SetNextConnectType(sys, comm, CTVT_CONNECT_MAC);
                CtvtComm_SetParentMac(sys, comm, call->entries[call->joinTarget].mac);
                CtvtCall_PrintInfo(sys, call, 24);
                call->showInfo = TRUE;
                call->state = CALL_STATE_JOINING;
                call->waitTimer = 0;
                call->ringTimer = 0;
                AppTaskMenuWin_Free(call->menu);
                call->menu = NULL;
            } else {
                CtvtComm *comm = CommTvt_GetComm(sys);
                CtvtCommBeacon *beacon;
                u8 i, j;

                call->state = CALL_STATE_CALLING;
                CtvtCall_PrintInfo(sys, call, 20);
                call->showInfo = TRUE;
                call->ringTimer = 0;
                CtvtComm_SetNextConnectType(sys, comm, CTVT_CONNECT_SCAN);
                AppTaskMenuWin_Free(call->menu);
                call->menu = NULL;
                beacon = CtvtComm_GetBeacon(sys, comm);
                for (i = 0; i < CTVT_CALL_INVITES; i++) {
                    if (call->invites[i] == CTVT_CALL_NONE) {
                        for (j = 0; j < 6; j++) {
                            beacon->inviteMacs[i][j] = 0xff;
                        }
                    } else {
                        for (j = 0; j < 6; j++) {
                            beacon->inviteMacs[i][j] = call->entries[call->invites[i]].mac[j];
                        }
                    }
                }
                beacon->inviteOnly = TRUE;
                CtvtComm_StartInviteTimer(sys, comm);
            }
            func_0204c488(call->returnButton, 15);
        }
        break;
    case CALL_STATE_JOINING:
    case CALL_STATE_WAITING:
    case CALL_STATE_CALLING: {
        CtvtComm *comm = CommTvt_GetComm(sys);

        if (CommTvt_GetMemberCount(sys) >= 2) {
            if (call->recordingStarted == FALSE) {
                CtvtCamera *camera = CommTvt_GetCamera(sys);

                GFL_SndStop();
                GFL_SndSEPlay(SEQ_SE_SYS_72);
                CtvtCamera_StartRecording(sys, camera);
            }
            call->state = CALL_STATE_FADE_OUT;
        }
        if (call->state == CALL_STATE_CALLING) {
            call->ringTimer++;
            if (call->ringTimer >= CTVT_CALL_RING_FRAMES) {
                call->ringTimer = 0;
                GFL_SEPlayKeepVol(SEQ_SE_SYS_71, 2);
            }
        }
        if (call->state == CALL_STATE_CALLING) {
            if (CtvtComm_IsInviteTimerDone(sys, CommTvt_GetComm(sys)) == TRUE) {
                CtvtComm_SetNextConnectType(sys, CommTvt_GetComm(sys), CTVT_CONNECT_PARENT);
                CtvtCall_PrintInfo(sys, call, 39);
                call->state = CALL_STATE_FAILED;
            }
        } else if (call->state == CALL_STATE_JOINING || call->state == CALL_STATE_WAITING) {
            call->waitTimer++;
            if (call->waitTimer >= CTVT_CALL_WAIT_FRAMES) {
                CtvtComm_SetNextConnectType(sys, CommTvt_GetComm(sys), CTVT_CONNECT_PARENT);
                CtvtCall_PrintInfo(sys, call, 40);
                call->state = CALL_STATE_FAILED;
            }
        }
        break;
    }
    case CALL_STATE_FAILED:
        if (func_0203da48() == TRUE) {
            if (CommTvt_GetParam(sys)->unk4 == 2) {
                CtvtCamera_StopCamera(sys, CommTvt_GetCamera(sys));
                call->menuMode = CALL_MENU_NONE;
                call->state = CALL_STATE_LEAVE;
            } else {
                u8 i;

                for (i = 0; i < CTVT_CALL_INVITES; i++) {
                    call->invites[i] = CTVT_CALL_NONE;
                }
                call->state = CALL_STATE_LIST;
                call->joinTarget = CTVT_CALL_NONE;
                BmpWin_ClearFrame(call->infoWin, 1);
                CtvtCall_ClearWindow(call->infoWin);
                func_ov257_021aaeb0(sys);
                func_0204c488(call->returnButton, 1);
                call->menuMode = CALL_MENU_RESET;
            }
        }
        break;
    case CALL_STATE_PHOTO_NOTICE:
        BmpWin_Free(call->msgWin);
        call->msgWin = BmpWin_CreateDynamic(4, 2, 5, 28, 14, 10, TRUE);
        CtvtCall_PrintMessage(sys, call, 41);
        call->state = CALL_STATE_WAIT_PHOTO_NOTICE;
        call->photoTimer = CTVT_CALL_PHOTO_NOTICE_FRAMES;
        break;
    case CALL_STATE_WAIT_PHOTO_NOTICE:
        if (CommTvt_GetMemberCount(sys) >= 2 && call->recordingStarted == FALSE) {
            GFL_SndStop();
            call->recordingStarted = TRUE;
            GFL_SndSEPlay(SEQ_SE_SYS_72);
        }
        if (call->photoTimer > 0) {
            call->photoTimer--;
        }
        if (func_0203da48() == TRUE || func_ov257_021aab34(sys) == TRUE || call->photoTimer <= 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            BmpWin_ClearFrame(call->msgWin, 1);
            CtvtCall_ClearWindow(call->msgWin);
            BmpWin_Free(call->msgWin);
            call->msgWin = BmpWin_CreateDynamic(4, 1, 21, 21, 2, 10, TRUE);
            CommTvt_ClearCanExchangePhotos(sys);
            if (param->unk4 == 2 || param->unk4 == 3) {
                func_0204c488(call->returnButton, 15);
                if (CommTvt_GetMemberCount(sys) < 2) {
                    CtvtCall_PrintInfo(sys, call, 24);
                }
                call->showInfo = TRUE;
                call->menuMode = CALL_MENU_WAIT;
                call->state = CALL_STATE_WAITING;
            } else {
                HeapID heapId;
                ArcTool *arc;

                call->state = CALL_STATE_LIST;
                call->menuMode = CALL_MENU_RESET;
                func_0204c488(call->returnButton, 1);
                heapId = CommTvt_GetHeapId(sys);
                arc = CommTvt_GetArc(sys);
                GFL_BGSysLoadArcNCGRStatic(arc, 12, 4, 0, 0x2800, FALSE, heapId);
                loadBGScrToVramByFileNoReserveNegAlign(arc, 18, 4, 0, 0, FALSE, heapId);
                GFL_BGSysLoadScr(4);
            }
        }
        break;
    }

    if (param->unk4 != 3 && CommTvt_CanExchangePhotos(sys) == FALSE) {
        u8 i;

        CtvtCall_Scan(sys, call);
        for (i = 0; i < CTVT_CALL_ROWS; i++) {
            if (call->rows[i].printPending == TRUE &&
                !func_02021c1c(CommTvt_GetPrintQueue(sys), BmpWin_GetBitmap(call->rows[i].window))) {
                BmpWin_Transfer(call->rows[i].window);
                call->rows[i].printPending = FALSE;
            }
        }
        if (call->listDirty == TRUE) {
            u8 inviteCount = 0;

            for (i = 0; i < CTVT_CALL_ROWS; i++) {
                CtvtCall_UpdateRowActors(sys, call, &call->rows[i], i);
            }
            GFL_BGSysMoveBGReq(6, 3, call->scrollY);
            for (i = 0; i < CTVT_CALL_INVITES; i++) {
                if (call->invites[i] != CTVT_CALL_NONE) {
                    inviteCount++;
                }
            }
            for (i = 0; i < CTVT_CALL_ENTRIES; i++) {
                if (call->entries[i].active == TRUE) {
                    CtvtCallRow *row = &call->rows[call->entries[i].row];

                    if (call->entries[i].selected == TRUE && call->entries[i].memberCount < CTVT_CALL_MEMBERS_MAX &&
                        (call->joinTarget == CTVT_CALL_NONE || call->joinTarget == i)) {
                        func_0204c488(row->check, CALL_CHECK_ON);
                    } else if (inviteCount == CTVT_CALL_INVITES || call->joinTarget != CTVT_CALL_NONE ||
                               call->entries[i].memberCount == CTVT_CALL_MEMBERS_MAX) {
                        func_0204c488(row->check, CALL_CHECK_DISABLED);
                    } else {
                        func_0204c488(row->check, CALL_CHECK_OFF);
                    }
                    if (call->entries[i].changed == TRUE) {
                        call->entries[i].changed = FALSE;
                        func_0204c488(row->frame, 34);
                    }
                }
            }
            call->listDirty = FALSE;
        }
    }

    if (call->msgWinPending == TRUE && !func_02021c1c(CommTvt_GetPrintQueue(sys), BmpWin_GetBitmap(call->msgWin))) {
        BmpWin_FlushChar(call->msgWin);
        BmpWin_FlushMap(call->msgWin);
        GFL_BGSysLoadScr(4);
        call->msgWinPending = FALSE;
    }
    if (call->infoWinPending == TRUE && !func_02021c1c(CommTvt_GetPrintQueue(sys), BmpWin_GetBitmap(call->infoWin))) {
        BmpWin_FlushChar(call->infoWin);
        BmpWin_FlushMap(call->infoWin);
        GFL_BGSysLoadScr(4);
        call->infoWinPending = FALSE;
        if (call->showInfo == TRUE) {
            call->showInfo = FALSE;
            func_ov257_021aae7c(sys, call->infoWin);
        }
    }
    if (call->menu != NULL) {
        AppTaskMenuWin_Update(call->menu);
    }
    return COMM_TVT_MODE_CALL;
}

BOOL CtvtCall_IsBlackWhite(CommTvtWork *sys, CtvtCall *call, const u8 *mac) {
    BOOL found = FALSE;
    int i;

    for (i = 0; i < CTVT_CALL_ENTRIES; i++) {
        BOOL same = TRUE;
        u8 j;

        for (j = 0; j < 6; j++) {
            if (call->entries[i].mac[j] != mac[j]) {
                same = FALSE;
                break;
            }
        }
        if (same == TRUE) {
            found = TRUE;
            break;
        }
    }
    if (found == TRUE &&
        (call->entries[i].romVersion == VERSION_WHITE || call->entries[i].romVersion == VERSION_BLACK)) {
        return TRUE;
    }
    return FALSE;
}

static void CtvtCall_UpdateTouch(CommTvtWork *sys, CtvtCall *call) {
    u32 tapX, tapY;
    u32 touchX, touchY;
    BOOL tapped = func_0203dac8(&tapX, &tapY);
    BOOL touching = func_0203da84(&touchX, &touchY);
    CtvtComm *comm = CommTvt_GetComm(sys);

    if (tapped == TRUE) {
        if (tapX > 8 && tapX < 224) {
            s16 rowIndex = (tapY + call->scrollY) / CTVT_CALL_ROW_HEIGHT;

            if (rowIndex >= 0 && rowIndex < CTVT_CALL_ROWS) {
                u8 index = call->rows[rowIndex].entry;
                CtvtCallEntry *entry = &call->entries[index];
                BOOL changed = FALSE;

                if (entry->active == TRUE && entry->memberCount < CTVT_CALL_MEMBERS_MAX) {
                    if (entry->gameServiceId == CTVT_CALL_SERVICE_CTVT) {
                        if (entry->selected == FALSE) {
                            call->joinTarget = index;
                            changed = TRUE;
                            entry->selected = TRUE;
                            entry->changed = TRUE;
                            call->listDirty = TRUE;
                            GFL_SndSEPlay(SEQ_SE_SYS_69);
                        } else {
                            call->joinTarget = CTVT_CALL_NONE;
                            entry->selected = FALSE;
                            changed = TRUE;
                            entry->changed = TRUE;
                            call->listDirty = TRUE;
                            GFL_SndSEPlay(SEQ_SE_SYS_70);
                        }
                    } else if (call->joinTarget == CTVT_CALL_NONE) {
                        u8 j;

                        if (entry->selected == FALSE) {
                            for (j = 0; j < CTVT_CALL_INVITES; j++) {
                                if (call->invites[j] == CTVT_CALL_NONE) {
                                    call->invites[j] = index;
                                    break;
                                }
                            }
                            if (j < CTVT_CALL_INVITES) {
                                changed = TRUE;
                                entry->selected = TRUE;
                                entry->changed = TRUE;
                                call->listDirty = TRUE;
                                GFL_SndSEPlay(SEQ_SE_SYS_69);
                            }
                        } else {
                            for (j = 0; j < CTVT_CALL_INVITES; j++) {
                                if (index == call->invites[j]) {
                                    call->invites[j] = CTVT_CALL_NONE;
                                    break;
                                }
                            }
                            entry->selected = FALSE;
                            changed = TRUE;
                            entry->changed = TRUE;
                            call->listDirty = TRUE;
                            GFL_SndSEPlay(SEQ_SE_SYS_70);
                        }
                    }
                }
                if (changed == TRUE) {
                    CtvtCall_UpdateMenu(sys, call);
                }
            }
        } else if (tapX >= 236 && tapX <= 260 && tapY >= call->scrollBarY + 12 && tapY <= call->scrollBarY + 36 &&
                   call->rowCount > CTVT_CALL_VISIBLE_ROWS) {
            call->scrollBarHeld = TRUE;
            func_0204c488(call->scrollBar, 13);
            GFL_SndSEPlay(SEQ_SE_SYS_45);
        }
    }

    if (touching == TRUE && call->scrollBarHeld == TRUE && call->rowCount > CTVT_CALL_VISIBLE_ROWS) {
        if (touchX >= 236 && touchX <= 260 && touchY >= 12 && touchY <= 156) {
            u32 range = (call->rowCount - CTVT_CALL_VISIBLE_ROWS) * CTVT_CALL_ROW_HEIGHT;
            int y = touchY - 24;
            ClActorPos pos;

            if (y < 0) {
                y = 0;
            } else if (y > CTVT_CALL_SCROLL_BAR_RANGE) {
                y = CTVT_CALL_SCROLL_BAR_RANGE;
            }
            call->scrollBarY = y;
            pos.x = 248;
            pos.y = call->scrollBarY + 24;
            func_0204c140(call->scrollBar, &pos, 1);
            call->scrollY = call->scrollBarY * range / CTVT_CALL_SCROLL_BAR_RANGE;
            call->listDirty = TRUE;
        }
    } else {
        call->scrollBarHeld = FALSE;
        func_0204c488(call->scrollBar, 12);
    }

    if (call->menu != NULL && AppTaskMenuWin_IsTouched(call->menu) == TRUE) {
        AppTaskMenuWin_SetFlashing(call->menu, TRUE);
        call->state = CALL_STATE_DECIDED;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        if (call->menuMode == CALL_MENU_CALL) {
            GFL_SEPlayKeepVol(SEQ_SE_SYS_71, 2);
        }
    }
    if (func_0203da0c(sReturnButton) == 0 || (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B)) {
        call->state = CALL_STATE_DECIDED;
        call->menuMode = CALL_MENU_BACK;
        func_0204c488(call->returnButton, 9);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    }
}

static void CtvtCall_Scan(CommTvtWork *sys, CtvtCall *call) {
    BOOL listChanged = FALSE;
    BOOL menuChanged = FALSE;
    int i;

    if (CtvtComm_IsConnected(sys, CommTvt_GetComm(sys)) == FALSE) {
        return;
    }

    for (i = 0; i < CTVT_CALL_ENTRIES; i++) {
        if (call->entries[i].active == TRUE) {
            call->entries[i].stale = TRUE;
        }
    }

    if (call->state != CALL_STATE_JOINING && call->state != CALL_STATE_CALLING && call->state != CALL_STATE_FAILED &&
        call->state >= CALL_STATE_LIST) {
        for (i = 0; i < 16; i++) {
            BOOL found = FALSE;
            BOOL replace = FALSE;
            void *beacon = func_020428a8(i);

            if (beacon != NULL) {
                int slot = CTVT_CALL_NONE;
                u8 *mac = func_020428c8(i);
                u32 service = func_ov030_02173b78(i);
                BOOL friend;
                u8 j, k;

                if (service == CTVT_CALL_SERVICE_FIELD && ((CtvtCallFieldBeacon *)beacon)->kind != 2) {
                    continue;
                }
                friend = FALSE;
                if (service == CTVT_CALL_SERVICE_CTVT) {
                    const u16 *name = GetPlayerName(beacon);
                    u32 gender = getTrainerGender(beacon);
                    u32 id = getIDAsUInt(beacon);
                    u8 selfMac[6];

                    func_0207c33c(selfMac);
                    if (((CtvtCommBeacon *)beacon)->inviteOnly == FALSE ||
                        CtvtComm_IsInvited(beacon, selfMac) == TRUE) {
                        friend = CtvtCall_IsFriend(sys, call, name, id, gender);
                    }
                } else {
                    CtvtCallFieldBeacon *fieldBeacon = beacon;

                    if (IsZoneEntralinkHub(fieldBeacon->beacon.zoneId) == FALSE &&
                        GetZoneIsEntreeForest(fieldBeacon->beacon.zoneId) == FALSE) {
                        friend = CtvtCall_IsFriend(sys, call, fieldBeacon->beacon.name, fieldBeacon->trainerId,
                                                   fieldBeacon->beacon.gender);
                    }
                }
                if (friend == FALSE) {
                    continue;
                }

                for (j = 0; j < CTVT_CALL_ENTRIES; j++) {
                    if (call->entries[j].active == TRUE) {
                        BOOL same = TRUE;

                        for (k = 0; k < 6; k++) {
                            if (mac[k] != call->entries[j].mac[k]) {
                                same = FALSE;
                                break;
                            }
                        }
                        if (same == TRUE) {
                            call->entries[j].stale = FALSE;
                            found = TRUE;
                            // A machine that started or stopped a call: its entry is made again
                            if (service != call->entries[j].gameServiceId) {
                                found = FALSE;
                                replace = TRUE;
                                slot = j;
                                for (k = 0; k < CTVT_CALL_INVITES; k++) {
                                    if (j == call->invites[k]) {
                                        call->invites[k] = CTVT_CALL_NONE;
                                        menuChanged = TRUE;
                                    }
                                }
                                if (call->joinTarget == j) {
                                    call->joinTarget = CTVT_CALL_NONE;
                                    menuChanged = TRUE;
                                }
                            }
                            if (service == CTVT_CALL_SERVICE_CTVT) {
                                CtvtCommBeacon *entryBeacon = call->entries[j].beacon;

                                if (entryBeacon->memberCount != ((CtvtCommBeacon *)beacon)->memberCount) {
                                    found = FALSE;
                                    replace = TRUE;
                                    slot = j;
                                }
                            }
                            break;
                        }
                    } else if (slot == CTVT_CALL_NONE) {
                        slot = j;
                    }
                }

                if (found == FALSE && slot != CTVT_CALL_NONE) {
                    call->entries[slot].active = TRUE;
                    call->entries[slot].stale = FALSE;
                    call->entries[slot].selected = FALSE;
                    for (k = 0; k < 6; k++) {
                        call->entries[slot].mac[k] = mac[k];
                    }
                    if (service == CTVT_CALL_SERVICE_CTVT) {
                        call->entries[slot].gameServiceId = CTVT_CALL_SERVICE_CTVT;
                        sys_memcpy(beacon, call->entries[slot].beacon, sizeof(CtvtCommBeacon));
                        call->entries[slot].romVersion = func_02008bfc(call->entries[slot].beacon);
                    } else {
                        call->entries[slot].gameServiceId = CTVT_CALL_SERVICE_FIELD;
                        sys_memcpy(beacon, call->entries[slot].beacon, sizeof(CtvtCallFieldBeacon));
                        call->entries[slot].romVersion =
                            ((CtvtCallFieldBeacon *)call->entries[slot].beacon)->beacon.version;
                    }
                    if (replace == FALSE) {
                        for (k = 0; k < CTVT_CALL_ROWS; k++) {
                            if (call->rows[k].entry == CTVT_CALL_NONE) {
                                call->rows[k].entry = slot;
                                call->entries[slot].row = k;
                                call->rows[k].needsDraw = TRUE;
                                break;
                            }
                        }
                    } else {
                        call->rows[call->entries[slot].row].needsDraw = TRUE;
                    }
                    listChanged = TRUE;
                }
            }
        }
    }

    // Drops the machines no longer found, moving the rows below theirs up
    for (i = 0; i < CTVT_CALL_ENTRIES; i++) {
        if (call->entries[i].active == TRUE && call->entries[i].stale == TRUE) {
            u8 row;
            u8 k;

            for (row = call->entries[i].row; row < CTVT_CALL_ROWS - 1; row++) {
                if (call->rows[row + 1].entry == CTVT_CALL_NONE) {
                    break;
                }
                call->rows[row].entry = call->rows[row + 1].entry;
                call->entries[call->rows[row].entry].row = row;
                call->rows[row].needsDraw = TRUE;
            }
            call->rows[row].entry = CTVT_CALL_NONE;
            call->rows[row].needsDraw = TRUE;
            call->entries[i].row = CTVT_CALL_NONE;
            call->entries[i].active = FALSE;
            for (k = 0; k < CTVT_CALL_INVITES; k++) {
                if (i == call->invites[k]) {
                    call->invites[k] = CTVT_CALL_NONE;
                    menuChanged = TRUE;
                }
            }
            if (call->joinTarget == i) {
                call->joinTarget = CTVT_CALL_NONE;
                menuChanged = TRUE;
            }
            // Looks at the entry again, though it is no longer active
            i--;
            listChanged = TRUE;
        }
    }

    if (listChanged == TRUE) {
        CtvtCall_DrawRows(sys, call);
        menuChanged = TRUE;
        call->listDirty = TRUE;
    }
    if (menuChanged == TRUE && call->state == CALL_STATE_LIST) {
        CtvtCall_UpdateMenu(sys, call);
    }
}

static void CtvtCall_DrawRows(CommTvtWork *sys, CtvtCall *call) {
    u8 count = 0;
    u8 i;

    for (i = 0; i < CTVT_CALL_ROWS; i++) {
        CtvtCall_DrawRow(sys, call, &call->rows[i], i);
    }
    for (i = 0; i < CTVT_CALL_ROWS; i++) {
        if (call->rows[i].entry != CTVT_CALL_NONE) {
            count++;
        }
    }
    if (call->rowCount != count) {
        if (count > CTVT_CALL_VISIBLE_ROWS) {
            u32 range = (count - CTVT_CALL_VISIBLE_ROWS) * CTVT_CALL_ROW_HEIGHT;

            func_0204c124(call->scrollBar, TRUE);
            if (call->scrollY > range) {
                call->scrollY = range;
            }
            call->scrollBarY = call->scrollY * CTVT_CALL_SCROLL_BAR_RANGE / range;
            if (call->scrollBarHeld == FALSE) {
                ClActorPos pos;

                pos.x = 248;
                pos.y = call->scrollBarY + 24;
                func_0204c140(call->scrollBar, &pos, 1);
            }
        } else {
            func_0204c124(call->scrollBar, FALSE);
            call->scrollY = 0;
            call->listDirty = TRUE;
        }
        call->rowCount = count;
    }
}

static void CtvtCall_DrawRow(CommTvtWork *sys, CtvtCall *call, CtvtCallRow *row, u8 index) {
    if (row->needsDraw != TRUE) {
        return;
    }
    if (row->entry != CTVT_CALL_NONE) {
        MsgData *msgData;
        PrintQueue *queue;
        StrBuf *name;
        StrBuf *str;
        CtvtCommBeacon *beacon;
        HeapID heapId;
        StrBuf *format;
        Font *font;
        CtvtCallEntry *entry;
        WordSet *wordSet;

        heapId = CommTvt_GetHeapId(sys);
        font = CommTvt_GetFont(sys);
        msgData = CommTvt_GetMsgData(sys);
        queue = CommTvt_GetPrintQueue(sys);
        entry = &call->entries[row->entry];
        wordSet = GFL_WordSetSystemCreateDefault(heapId);

        GFL_BitmapFill(BmpWin_GetBitmap(row->window), 0);
        name = GFL_StrBufCreate(32, heapId);
        if (entry->gameServiceId == CTVT_CALL_SERVICE_CTVT) {
            GFL_StrBufLoadFixedString(name, CtvtBeacon_GetPlayerName(entry->beacon), 8);
        } else {
            GFL_StrBufLoadFixedString(name, ((CtvtCallFieldBeacon *)entry->beacon)->beacon.name, 8);
        }
        func_02021c7c(queue, BmpWin_GetBitmap(row->window), 6, 0, name, font, 0x3c40);
        GFL_StrBufFree(name);

        format = GFL_MsgDataLoadStrbufNew(msgData, 14);
        str = GFL_StrBufCreate(32, heapId);
        if (entry->gameServiceId == CTVT_CALL_SERVICE_CTVT) {
            WordSetNumber(wordSet, 0, CtvtBeacon_GetTrainerID(entry->beacon), 5, 2, TRUE);
        } else {
            WordSetNumber(wordSet, 0, ((CtvtCallFieldBeacon *)entry->beacon)->beacon.trainerId, 5, 2, TRUE);
        }
        GFL_WordSetFormatStrbuf(wordSet, str, format);
        GFL_StrBufFree(format);
        func_02021c7c(queue, BmpWin_GetBitmap(row->window), 80, 0, str, font, 0x3c40);
        GFL_StrBufFree(str);

        if (entry->gameServiceId == CTVT_CALL_SERVICE_CTVT) {
            StrBuf *memberFormat;
            StrBuf *members;

            beacon = entry->beacon;
            entry->memberCount = beacon->memberCount;
            memberFormat = GFL_MsgDataLoadStrbufNew(msgData, 16);
            members = GFL_StrBufCreate(32, heapId);
            WordSetNumber(wordSet, 0, beacon->memberCount, 1, 0, TRUE);
            GFL_WordSetFormatStrbuf(wordSet, members, memberFormat);
            GFL_StrBufFree(memberFormat);
            func_02021c7c(queue, BmpWin_GetBitmap(row->window), 144, 0, members, font, 0x3c40);
            GFL_StrBufFree(members);
        } else {
            entry->memberCount = 0;
        }

        if (entry->gameServiceId == CTVT_CALL_SERVICE_CTVT) {
            if (((CtvtCommBeacon *)entry->beacon)->cameraEnabled == TRUE) {
                entry->cameraEnabled = TRUE;
            } else {
                entry->cameraEnabled = FALSE;
            }
        } else {
            CtvtCallFieldBeacon *fieldBeacon = entry->beacon;

            if (fieldBeacon->cameraEnabled == TRUE && fieldBeacon->cameraBlocked == FALSE) {
                entry->cameraEnabled = TRUE;
            } else {
                entry->cameraEnabled = FALSE;
            }
        }
        GFL_WordSetSystemFree(wordSet);
        row->printPending = TRUE;
    } else {
        CtvtCall_ClearWindowQueued(row->window);
    }
    row->needsDraw = FALSE;
}

static inline void CtvtCall_HideRow(CtvtCallRow *row) {
    func_0204c124(row->frame, FALSE);
    func_0204c124(row->check, FALSE);
    func_0204c124(row->camera, FALSE);
}

static void CtvtCall_UpdateRowActors(CommTvtWork *sys, CtvtCall *call, CtvtCallRow *row, u8 index) {
    s16 y;

    if (row->entry == CTVT_CALL_NONE) {
        CtvtCall_HideRow(row);
        return;
    }
    y = index * CTVT_CALL_ROW_HEIGHT - call->scrollY;
    if (y >= -CTVT_CALL_ROW_HEIGHT && y <= 160) {
        ClActorPos pos;

        pos.x = 128;
        pos.y = y + 16;
        func_0204c140(row->frame, &pos, 1);
        func_0204c124(row->frame, TRUE);
        pos.x = 18;
        func_0204c140(row->check, &pos, 1);
        func_0204c124(row->check, TRUE);
        pos.x = 200;
        func_0204c140(row->camera, &pos, 1);
        if (call->entries[row->entry].cameraEnabled == TRUE) {
            func_0204c488(call->rows[index].camera, 18);
            func_0204c124(row->camera, TRUE);
        } else {
            func_0204c124(row->camera, FALSE);
        }
    } else {
        CtvtCall_HideRow(row);
    }
}

static void CtvtCall_UpdateMenu(CommTvtWork *sys, CtvtCall *call) {
    u8 inviteCount = 0;
    HeapID heapId = CommTvt_GetHeapId(sys);
    MsgData *msgData = CommTvt_GetMsgData(sys);
    AppTaskMenuRes *res = CommTvt_GetTaskMenuRes(sys);
    u8 i;

    for (i = 0; i < CTVT_CALL_INVITES; i++) {
        if (call->invites[i] != CTVT_CALL_NONE) {
            inviteCount++;
        }
    }
    if (call->joinTarget != CTVT_CALL_NONE) {
        if (call->menuMode != CALL_MENU_JOIN) {
            AppTaskMenuItem item = { 0 };

            CtvtCall_ClearWindowQueued(call->msgWin);
            BmpWin_ClearFrame(call->msgWin, 1);
            if (call->menu != NULL) {
                AppTaskMenuWin_Free(call->menu);
                call->menu = NULL;
            }
            item.str = GFL_MsgDataLoadStrbufNew(msgData, 19);
            item.color = 0x39e3;
            item.type = 0;
            call->menu = AppTaskMenuWin_Create(res, &item, 0, 21, 21, heapId);
            GFL_StrBufFree(item.str);
            call->menuMode = CALL_MENU_JOIN;
        }
    } else if (inviteCount != 0) {
        if (call->menuMode != CALL_MENU_CALL) {
            AppTaskMenuItem item = { 0 };

            CtvtCall_ClearWindowQueued(call->msgWin);
            BmpWin_ClearFrame(call->msgWin, 1);
            if (call->menu != NULL) {
                AppTaskMenuWin_Free(call->menu);
                call->menu = NULL;
            }
            item.str = GFL_MsgDataLoadStrbufNew(msgData, 18);
            item.color = 0x39e3;
            item.type = 0;
            call->menu = AppTaskMenuWin_Create(res, &item, 0, 21, 21, heapId);
            GFL_StrBufFree(item.str);
            call->menuMode = CALL_MENU_CALL;
        }
    } else if (call->menuMode != CALL_MENU_NONE) {
        if (call->menu != NULL) {
            AppTaskMenuWin_Free(call->menu);
            call->menu = NULL;
        }
        if (call->rowCount != 0) {
            CtvtCall_PrintMessage(sys, call, 17);
        }
        call->menuMode = CALL_MENU_NONE;
    }
}

// Shows "nobody was found" on the top screen when the list empties, and the list's message when it fills again
static void CtvtCall_UpdateFoundMessage(CommTvtWork *sys, CtvtCall *call) {
    if (call->rowCount != call->prevRowCount) {
        if (call->rowCount == 0) {
            CtvtCall_PrintInfo(sys, call, 28);
            call->showInfo = TRUE;
            CtvtCall_ClearWindowQueued(call->msgWin);
            BmpWin_ClearFrame(call->msgWin, 1);
        } else if (call->prevRowCount == 0 && call->rowCount != 0) {
            CtvtCall_PrintMessage(sys, call, 17);
            func_ov257_021aaeb0(sys);
            BmpWin_ClearFrame(call->infoWin, 1);
            CtvtCall_ClearWindow(call->infoWin);
        }
        call->prevRowCount = call->rowCount;
    }
}

static BOOL CtvtCall_IsFriend(CommTvtWork *sys, CtvtCall *call, const u16 *name, u32 id, u32 gender) {
    return func_0200a46c(GameData_GetWifiList(CommTvt_GetParam(sys)->gameData), name, id, gender, NULL);
}

static void CtvtCall_PrintMessage(CommTvtWork *sys, CtvtCall *call, u32 msgId) {
    Font *font = CommTvt_GetFont(sys);
    MsgData *msgData = CommTvt_GetMsgData(sys);
    PrintQueue *queue = CommTvt_GetPrintQueue(sys);
    StrBuf *str;

    GFL_BitmapFill(BmpWin_GetBitmap(call->msgWin), 15);
    str = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    func_02021c7c(queue, BmpWin_GetBitmap(call->msgWin), 0, 0, str, font, 0x440);
    GFL_StrBufFree(str);
    BmpWin_DrawFrame(call->msgWin, 2, 0x140, 9);
    call->msgWinPending = TRUE;
}

static void CtvtCall_PrintInfo(CommTvtWork *sys, CtvtCall *call, u32 msgId) {
    Font *font = CommTvt_GetFont(sys);
    MsgData *msgData = CommTvt_GetMsgData(sys);
    PrintQueue *queue = CommTvt_GetPrintQueue(sys);
    StrBuf *str;

    GFL_BitmapFill(BmpWin_GetBitmap(call->infoWin), 15);
    str = GFL_MsgDataLoadStrbufNew(msgData, msgId);
    func_02021c7c(queue, BmpWin_GetBitmap(call->infoWin), 0, 0, str, font, 0x440);
    GFL_StrBufFree(str);
    BmpWin_DrawFrame(call->infoWin, 2, 0x140, 9);
    call->infoWinPending = TRUE;
    call->showInfo = FALSE;
    func_ov257_021aaeb0(sys);
}
