#include "app/comm_tvt/ctvt_game_cam.h"
#include "types.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "app/comm_tvt/camera_system.h"
#include "app/comm_tvt/comm_tvt_sys.h"
#include "app/comm_tvt/ctvt_game.h"
#include "app/comm_tvt/ctvt_game_cam_graphic.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/net_lower_data.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "gfl/tcbl.h"
#include "nitro/fs.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "system/bmp_winframe.h"
#include "system/dsi.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/text_speed.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "twl/dsp.h"

// The minigames' camera: before a game, this machine takes its three pictures with the DSi's camera, or picks one of
// four characters' sets with a roulette when it has none, and swaps its pictures with the others in the call. Named
// after the file name in its allocations

typedef struct CtvtGameCamSeq CtvtGameCamSeq;

// A step of the sequence, with its own state, starting at 0 each time the step is set
typedef void (*CtvtGameCamSeqFunc)(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);

struct CtvtGameCamSeq {
    CtvtGameCamSeqFunc func;
    BOOL done;
    int state;
    CtvtGameCamTask *task;
};

// The cell actors: the camera's frame, the shutter's countdown and the four characters of the roulette
typedef struct {
    // As func_0204bc48, func_0204b81c and func_0204bde0 load them
    u32 resources[3];
    ClActor *actors[8];
} CtvtGameCamActors;

// The camera's frames on the sub screen, and the pictures taken
typedef struct {
    // 256 by 192 direct color pixels
    void *frame;
    // Set when a frame is copied and cleared when the VBlank loads it
    BOOL frameReady;
    u8 picture;
    u16 timer;
    CameraSystem *cameraSystem;
} CtvtGameCamCamera;

// The roulette of the characters' pictures, which this machine's pictures come from without a camera
typedef struct {
    void *pictures[4];
    u8 cursor;
    u16 wait;
    BOOL moving;
    u16 timer;
    u8 blink;
} CtvtGameCamRoulette;

// The exchange of the pictures with the others in the call
typedef struct {
    HeapID heapId;
    // This machine's picture to send, with a camera
    void *buffer;
    // The members whose pictures come from their cameras, and those who can't exchange pictures
    u8 receiving;
    u8 noExchange;
    u8 received[3];
    u8 picture;
    // Until this machine's next picture is sent
    BOOL waiting;
    u8 timer;
    u8 state;
    BOOL done;
    BOOL started;
    int connections;
} CtvtGameCamExchange;

// The message window
typedef struct {
    MsgData *msgData;
    Font *font;
    BmpWin *printWindow;
    u8 printing;
    PrintQueue *printQueue;
    BmpWin *window;
    WordSet *wordSet;
    StrBuf *strbuf;
    StrBuf *expanded;
    u16 color;
} CtvtGameCamMsgWin;

// The message printing in the window, and the step to go on to when it is done
typedef struct {
    PrintStream *stream;
    TCBExManager *tcbManager;
    CtvtGameCamSeqFunc next;
    u16 wait;
    u16 timer;
} CtvtGameCamMsg;

struct CtvtGameCamTask {
    HeapID heapId;
    TCB *vblankTask;
    CtvtGameCamGraphic *graphic;
    CtvtGameCamSeq seq;
    CtvtGameCamActors clact;
    CtvtGameCamCamera camera;
    CtvtGameCamRoulette roulette;
    CtvtGameCamExchange exchange;
    CtvtGameCamMsgWin msgWin;
    CtvtGameCamMsg msg;
    Font *font;
    PrintQueue *printQueue;
    MsgData *msgData;
    WordSet *wordSet;
    CtvtGameCam *cam;
};

static void CtvtGameCam_LoadBG(HeapID heapId);
static void CtvtGameCam_FreeBG(void);
static void CtvtGameCamActors_Init(CtvtGameCamActors *clact, ClActUnit *unit, HeapID heapId);
static void CtvtGameCamActors_Free(CtvtGameCamActors *clact);
static ClActor *CtvtGameCamActors_GetActor(CtvtGameCamActors *clact, u8 index);
static void CtvtGameCamMsgWin_Init(CtvtGameCamMsgWin *win, Font *font, MsgData *msgData, PrintQueue *printQueue,
                                   WordSet *wordSet, HeapID heapId);
static void CtvtGameCamMsgWin_Free(CtvtGameCamMsgWin *win);
static BOOL CtvtGameCamMsgWin_Update(CtvtGameCamMsgWin *win);
static void CtvtGameCamMsg_Init(CtvtGameCamMsg *msg, HeapID heapId);
static void CtvtGameCamMsg_Free(CtvtGameCamMsg *msg);
static void CtvtGameCamMsg_Print(CtvtGameCamMsg *msg, CtvtGameCamMsgWin *win, CtvtGameCamSeqFunc next, u32 msgId,
                                 HeapID heapId);
static void CtvtGameCamMsg_PrintNumber(CtvtGameCamMsg *msg, CtvtGameCamMsgWin *win, CtvtGameCamSeqFunc next, u8 number,
                                       u32 msgId, HeapID heapId);
static void CtvtGameCam_LoadMainPicture(u16 x, u16 y, u16 width, u16 height, const u16 *src);
static void CtvtGameCamCamera_Init(CtvtGameCamCamera *camera, CameraSystem *cameraSystem, HeapID heapId);
static void CtvtGameCamCamera_Free(CtvtGameCamCamera *camera);
static void CtvtGameCamCamera_TakePicture(CtvtGameCamCamera *camera, void *dest, HeapID heapId);
static void CtvtGameCamCamera_Start(CtvtGameCamCamera *camera);
static void CtvtGameCamCamera_Stop(CtvtGameCamCamera *camera);
static void CtvtGameCamCamera_PlayShutterSound(CtvtGameCamCamera *camera);
static BOOL CtvtGameCamCamera_IsShutterSoundPlaying(CtvtGameCamCamera *camera);
static void CtvtGameCamRoulette_Init(CtvtGameCamRoulette *roulette, HeapID heapId);
static void CtvtGameCamRoulette_Free(CtvtGameCamRoulette *roulette);
static void CtvtGameCamRoulette_Update(CtvtGameCamRoulette *roulette, CtvtGameCamActors *clact);
static void CtvtGameCam_LoadSubPicture(u16 x, u16 y, u16 width, u16 height, const u16 *src);
static void CtvtGameCamRoulette_Move(CtvtGameCamRoulette *roulette, u16 wait);
static BOOL CtvtGameCamRoulette_IsMoving(CtvtGameCamRoulette *roulette);
static void CtvtGameCamExchange_Init(CtvtGameCamExchange *exchange, BOOL cameraEnabled, HeapID heapId);
static void CtvtGameCamExchange_Start(CtvtGameCamExchange *exchange, CtvtGameMember *members, BOOL cameraEnabled);
static void CtvtGameCamExchange_Free(CtvtGameCamExchange *exchange, BOOL cameraEnabled);
static void CtvtGameCamExchange_Update(CtvtGameCamExchange *exchange, CtvtGameMember *members);
static void CtvtGameCamExchange_Send(CtvtGameCamExchange *exchange);
static BOOL CtvtGameCamExchange_CanSend(CtvtGameCamExchange *exchange);
static void *CtvtGameCamExchange_GetBuffer(CtvtGameCamExchange *exchange);
static BOOL CtvtGameCamExchange_IsDone(CtvtGameCamExchange *exchange);
static void CtvtGameCamExchange_CopyBuffer(CtvtGameCamExchange *exchange, void *dest);
static BOOL CtvtGameCamExchange_IsSomeoneGone(CtvtGameCamExchange *exchange);
static void CtvtGameCamSeq_Init(CtvtGameCamSeq *seq, CtvtGameCamTask *task, CtvtGameCamSeqFunc func);
static void CtvtGameCamSeq_Free(CtvtGameCamSeq *seq);
static void CtvtGameCamSeq_Update(CtvtGameCamSeq *seq);
static BOOL CtvtGameCamSeq_IsDone(CtvtGameCamSeq *seq);
static void CtvtGameCamSeq_Set(CtvtGameCamSeq *seq, CtvtGameCamSeqFunc func);
static void CtvtGameCamSeq_End(CtvtGameCamSeq *seq);
static void CtvtGameCam_SeqEnd(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_SeqStart(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_SeqWaitExchange(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_SeqWaitMessage(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_SeqCameraIntro(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_SeqPictureNumber(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_SeqPicturePrompt(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_SeqTakePicture(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_SeqCameraDone(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_OnFrame(void *frame, void *work);
static void CtvtGameCam_SeqRouletteIntro(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_SeqDrawCharacters(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_SeqRoulettePrompt(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_SeqRoulette(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_SeqRouletteDone(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task);
static void CtvtGameCam_VBlank(TCB *tcb, void *work);

// Not referred to: perhaps the area of the camera's frame that becomes a picture, which CtvtGameCamCamera_TakePicture
// crops
const u16 data_ov257_021b223c[4] = { 32, 64, 128, 128 };

// The messages before each picture, by the game's type
static const u16 sCtvtGameCamPromptMessages[2][3] = {
    { 51, 52, 53 },
    { 51, 52, 54 },
};

// Whether the DSP is scaling, which nothing here sets: perhaps a static of TwlSDK's graphics header
static BOOL sCtvtGameCamDspBusy;

// TwlSDK's DSP_Scaling, which scales a whole image
static inline BOOL DSP_Scaling(const void *src, void *dst, u16 width, u16 height, f32 rx, f32 ry, int mode) {
    if (hw_isDSi() && !sCtvtGameCamDspBusy) {
        return func_02707494(src, dst, width, height, rx, ry, mode, 0, 0, width, height, NULL, FALSE);
    }
    return FALSE;
}

CtvtGameCamTask *CtvtGameCam_StartTask(CtvtGameCam *cam, HeapID heapId) {
    CtvtGameCamTask *task = GFL_HeapAllocate(heapId, sizeof(CtvtGameCamTask), TRUE, "ctvt_game_cam.c", 426);

    task->cam = cam;
    task->heapId = heapId;
    task->font = GFL_FontCreate(23, 0, 0, FALSE, heapId);
    task->printQueue = func_020219a8(0x800, heapId);
    task->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_SERVER_FLOW, heapId);
    task->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    task->graphic = CtvtGameCamGraphic_Create(1, heapId);
    CtvtGameCam_LoadBG(heapId);
    CtvtGameCamActors_Init(&task->clact, CtvtGameCamGraphic_GetClActUnit(task->graphic), heapId);
    CtvtGameCamSeq_Init(&task->seq, task, CtvtGameCam_SeqStart);
    CtvtGameCamMsgWin_Init(&task->msgWin, task->font, task->msgData, task->printQueue, task->wordSet, heapId);
    CtvtGameCamMsg_Init(&task->msg, heapId);
    CtvtGameCamExchange_Init(&task->exchange, cam->cameraEnabled, heapId);
    if (task->cam->cameraEnabled) {
        CtvtGameCamCamera_Init(&task->camera, task->cam->cameraSystem, task->heapId);
        CameraSystem_SetFrameCallback(task->cam->cameraSystem, CtvtGameCam_OnFrame, &task->camera);
        CameraSystem_Start(task->cam->cameraSystem);
    } else {
        CtvtGameCamRoulette_Init(&task->roulette, task->heapId);
    }
    task->vblankTask = GFL_VBlankTCBAdd(CtvtGameCam_VBlank, task, 1);
    if (func_02042788()) {
        func_02042ba8(TRUE, heapId);
    }
    return task;
}

int CtvtGameCam_UpdateTask(CtvtGameCamTask *task) {
    if (CtvtGameCamExchange_IsSomeoneGone(&task->exchange) == TRUE) {
        return 2;
    }
    CtvtGameCamSeq_Update(&task->seq);
    CtvtGameCamExchange_Update(&task->exchange, task->cam->members);
    func_02021a3c(task->printQueue);
    if (CtvtGameCamSeq_IsDone(&task->seq) && CtvtGameCamExchange_IsDone(&task->exchange)) {
        return 1;
    }
    return 0;
}

void CtvtGameCam_EndTask(CtvtGameCamTask *task) {
    GFL_TCBRemove(task->vblankTask);
    if (task->cam->cameraEnabled) {
        CameraSystem_Stop(task->cam->cameraSystem);
        CtvtGameCamCamera_Free(&task->camera);
    } else {
        CtvtGameCamRoulette_Free(&task->roulette);
    }
    CtvtGameCamExchange_Free(&task->exchange, task->cam->cameraEnabled);
    CtvtGameCamMsg_Free(&task->msg);
    CtvtGameCamMsgWin_Free(&task->msgWin);
    CtvtGameCamSeq_Free(&task->seq);
    CtvtGameCamActors_Free(&task->clact);
    CtvtGameCam_FreeBG();
    CtvtGameCamGraphic_Free(task->graphic);
    GFL_WordSetSystemFree(task->wordSet);
    GFL_MsgDataFree(task->msgData);
    func_02021c44(task->printQueue);
    func_02021a18(task->printQueue);
    GFL_FontFree(task->font);
    GFL_HeapFree(task);
}

CtvtGameCam *CtvtGameCam_Create(HeapID heapId, BOOL cameraEnabled, int type, u8 character, CtvtGameMember *members,
                                CameraSystem *cameraSystem) {
    u8 i;
    u8 j;
    u32 files[4] = { 9, 12, 15, 18 };
    CtvtGameCam *cam = GFL_HeapAllocate(heapId, sizeof(CtvtGameCam), TRUE, "ctvt_game_cam.c", 600);

    cam->cameraEnabled = cameraEnabled;
    cam->type = type;
    cam->character = character;
    cam->cameraSystem = cameraSystem;
    if (cameraEnabled == TRUE) {
        for (i = 0; i < 3; i++) {
            cam->pictures[i] = allocConfigDSSoftwareFeature(heapId, 0x2000, "ctvt_game_cam.c", 615);
        }
    } else {
        u32 fileId = files[character];

        for (i = 0; i < 3; i++) {
            cam->pictures[i] = GFL_ArcSysReadHeapNewLZ(242, i + fileId, FALSE, heapId);
        }
    }
    for (i = 0; i < 3; i++) {
        cam->members[i] = members[i];
        for (j = 0; j < 3; j++) {
            cam->members[i].pictures[j] = members[i].pictures[j];
        }
    }
    return cam;
}

void CtvtGameCam_Delete(CtvtGameCam *cam) {
    u8 i;

    if (cam->cameraEnabled == TRUE) {
        for (i = 0; i < 3; i++) {
            func_02042ed0(cam->pictures[i]);
        }
    } else {
        for (i = 0; i < 3; i++) {
            GFL_HeapFree(cam->pictures[i]);
        }
    }
    GFL_HeapFree(cam);
}

static void CtvtGameCam_LoadBG(HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(242, heapId);

    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0, 0, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 3, 0, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 6, 0, 0, 0, FALSE, heapId);
    GFL_ArcToolFree(arc);
    arc = GFL_ArcSysCreateFileHandle(23, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 5, 0, 0x1e0, 0x20, heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysFillChar(1, 0, 1, 0);
    LoadSysMsgBox(1, 1, 14, 0, heapId);
    G2S_SetBG3ControlDCBmp(GX_BG_SCRSIZE_DCBMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x00000);
    sys_memset(gfxGetScreenAddrBG3B(), 0, 256 * 192 * 2);
    G2_SetBG3ControlDCBmp(GX_BG_SCRSIZE_DCBMP_256x256, GX_BG_AREAOVER_XLU, GX_BG_BMPSCRBASE_0x14000);
    sys_memset(gfxGetScreenAddrBG3A(), 0, 256 * 192 * 2);
}

static void CtvtGameCam_FreeBG(void) {
    GFL_BGSysFreeFilledChar(1, 1, 0);
}

static void CtvtGameCamActors_Init(CtvtGameCamActors *clact, ClActUnit *unit, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(242, heapId);
    ClActorSetup setup;

    clact->resources[0] = func_0204bc48(arc, 1, 2, 0, heapId);
    clact->resources[1] = func_0204b81c(arc, 4, FALSE, 2, heapId);
    clact->resources[2] = func_0204bde0(arc, 7, 8, heapId);
    GFL_ArcToolFree(arc);

    sys_memset(&setup, 0, sizeof(setup));
    setup.sequence = 0;
    setup.x = 128;
    setup.y = 96;
    setup.bgPriority = 0;
    clact->actors[1] =
        func_0204c040(unit, clact->resources[0], clact->resources[1], clact->resources[2], &setup, 1, heapId);
    func_0204c124(clact->actors[1], FALSE);

    setup.sequence = 1;
    setup.x = 128;
    setup.y = 52;
    setup.bgPriority = 1;
    clact->actors[0] =
        func_0204c040(unit, clact->resources[0], clact->resources[1], clact->resources[2], &setup, 0, heapId);
    func_0204c124(clact->actors[0], TRUE);

    setup.sequence = 2;
    setup.x = 128;
    setup.y = 96;
    setup.bgPriority = 0;
    clact->actors[2] =
        func_0204c040(unit, clact->resources[0], clact->resources[1], clact->resources[2], &setup, 0, heapId);
    func_0204c124(clact->actors[2], FALSE);
    func_0204c520(clact->actors[2], TRUE);

    setup.sequence = 3;
    setup.bgPriority = 0;
    {
        // The roulette's characters
        s16 positions[4][2] = {
            { 74, 50 },
            { 192, 50 },
            { 192, 144 },
            { 74, 144 },
        };
        int i;

        for (i = 3; i < 7; i++) {
            setup.x = positions[i - 3][0];
            setup.y = positions[i - 3][1];
            clact->actors[i] =
                func_0204c040(unit, clact->resources[0], clact->resources[1], clact->resources[2], &setup, 1, heapId);
            func_0204c124(clact->actors[i], FALSE);
        }
    }
}

static void CtvtGameCamActors_Free(CtvtGameCamActors *clact) {
    u8 i;

    for (i = 0; i < 8; i++) {
        if (clact->actors[i] != NULL) {
            func_0204c108(clact->actors[i]);
        }
    }
    func_0204bcd0(clact->resources[0]);
    func_0204b98c(clact->resources[1]);
    func_0204be64(clact->resources[2]);
}

static ClActor *CtvtGameCamActors_GetActor(CtvtGameCamActors *clact, u8 index) {
    return clact->actors[index];
}

static void CtvtGameCamMsgWin_Init(CtvtGameCamMsgWin *win, Font *font, MsgData *msgData, PrintQueue *printQueue,
                                   WordSet *wordSet, HeapID heapId) {
    BmpWin *window;

    sys_memset(win, 0, sizeof(CtvtGameCamMsgWin));
    win->color = 15;
    win->font = font;
    win->wordSet = wordSet;
    win->msgData = msgData;
    win->printQueue = printQueue;
    win->strbuf = GFL_StrBufCreate(109, heapId);
    win->expanded = GFL_StrBufCreate(109, heapId);
    win->window = BmpWin_CreateDynamic(1, 1, 19, 30, 4, 15, TRUE);
    BmpWin_DrawFrame(win->window, 2, 1, 14);
    win->printWindow = win->window;
    win->printing = FALSE;
    GFL_BitmapFill(BmpWin_GetBitmap(win->printWindow), win->color);
    BmpWin_TransferNow(win->window);
}

static void CtvtGameCamMsgWin_Free(CtvtGameCamMsgWin *win) {
    BmpWin_Free(win->window);
    GFL_StrBufFree(win->expanded);
    GFL_StrBufFree(win->strbuf);
    sys_memset(win, 0, sizeof(CtvtGameCamMsgWin));
}

static BOOL CtvtGameCamMsgWin_Update(CtvtGameCamMsgWin *win) {
    PrintQueue *printQueue = win->printQueue;

    if (win->printing) {
        if (!func_02021c1c(printQueue, BmpWin_GetBitmap(win->printWindow))) {
            BmpWin_FlushChar(win->printWindow);
            win->printing = FALSE;
        }
    }
    if (!win->printing) {
        return TRUE;
    }
    return FALSE;
}

static void CtvtGameCamMsg_Init(CtvtGameCamMsg *msg, HeapID heapId) {
    sys_memset(msg, 0, sizeof(CtvtGameCamMsg));
    msg->tcbManager = GFL_TCBExMgrCreate(heapId, heapId, 2, 0);
}

static void CtvtGameCamMsg_Free(CtvtGameCamMsg *msg) {
    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
    }
    GFL_TCBExMgrFree(msg->tcbManager);
    sys_memset(msg, 0, sizeof(CtvtGameCamMsg));
}

static void CtvtGameCamMsg_Print(CtvtGameCamMsg *msg, CtvtGameCamMsgWin *win, CtvtGameCamSeqFunc next, u32 msgId,
                                 HeapID heapId) {
    GFL_MsgDataLoadStrbuf(win->msgData, msgId, win->strbuf);
    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
    }
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    BmpWin_FlushChar(win->window);
    msg->stream = func_02022268(win->window, 0, 0, win->strbuf, win->font, func_02017bcc(), msg->tcbManager, 2, heapId,
                                win->color);
    BmpWin_FlushMap(win->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(win->window));
    func_02022410(msg->stream, 0);
    msg->next = next;
    msg->wait = 90;
    msg->timer = 0;
}

static void CtvtGameCamMsg_PrintNumber(CtvtGameCamMsg *msg, CtvtGameCamMsgWin *win, CtvtGameCamSeqFunc next, u8 number,
                                       u32 msgId, HeapID heapId) {
    GFL_MsgDataLoadStrbuf(win->msgData, msgId, win->strbuf);
    WordSetNumber(win->wordSet, 0, number, 1, 1, TRUE);
    GFL_WordSetFormatStrbuf(win->wordSet, win->expanded, win->strbuf);
    if (msg->stream != NULL) {
        func_020223cc(msg->stream);
    }
    GFL_BitmapFill(BmpWin_GetBitmap(win->window), win->color);
    BmpWin_FlushChar(win->window);
    msg->stream = func_02022268(win->window, 0, 0, win->expanded, win->font, func_02017bcc(), msg->tcbManager, 2,
                                heapId, win->color);
    BmpWin_FlushMap(win->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(win->window));
    func_02022410(msg->stream, 0);
    msg->next = next;
    msg->wait = 60;
    msg->timer = 0;
}

// Loads a picture to the main screen's bitmap BG
static void CtvtGameCam_LoadMainPicture(u16 x, u16 y, u16 width, u16 height, const u16 *src) {
    u8 i;

    cp15_flushDC(src, width * height * 2);
    for (i = 0; i < height; i++) {
        gfxUploadBGScreen3A(&src[width * i], (i + y) * 512 + x * 2, width * 2);
    }
}

static void CtvtGameCamCamera_Init(CtvtGameCamCamera *camera, CameraSystem *cameraSystem, HeapID heapId) {
    camera->frame = allocConfigDSSoftwareFeature(heapId, 256 * 192 * 2, "ctvt_game_cam.c", 1203);
    camera->frameReady = FALSE;
    camera->picture = 0;
    camera->timer = 0;
    camera->cameraSystem = cameraSystem;
}

static void CtvtGameCamCamera_Free(CtvtGameCamCamera *camera) {
    func_02042ed0(camera->frame);
}

// Crops the middle 128 by 128 of the frame and halves it with the DSP into a 64 by 64 picture
static void CtvtGameCamCamera_TakePicture(CtvtGameCamCamera *camera, void *dest, HeapID heapId) {
    u16 *crop = allocConfigDSSoftwareFeature(heapId, 128 * 128 * 2, "ctvt_game_cam.c", 1241);
    FSFile file;
    int i;
    f32 scale;

    CameraSystem_ExitDsp(camera->cameraSystem);
    DSP_OpenStaticComponentGraphics(&file);
    if (!DSP_LoadGraphics(&file, 0xff, 0xff)) {
        sys_exit();
    }
    for (i = 0; i < 128; i++) {
        sys_memcpy32((u16 *)camera->frame + (i + 32) * 256 + 64, crop + i * 128, 128 * 2);
    }
    scale = 0.5009f;
    DSP_Scaling(crop, dest, 128, 128, scale, scale, 2);
    DSP_UnloadGraphics();
    CameraSystem_InitDsp(camera->cameraSystem);
    func_02042ed0(crop);
}

static void CtvtGameCamCamera_Start(CtvtGameCamCamera *camera) {
    CameraSystem_Start(camera->cameraSystem);
}

static void CtvtGameCamCamera_Stop(CtvtGameCamCamera *camera) {
    CameraSystem_Stop(camera->cameraSystem);
}

static void CtvtGameCamCamera_PlayShutterSound(CtvtGameCamCamera *camera) {
    CameraSystem_PlayShutterSound(camera->cameraSystem);
}

static BOOL CtvtGameCamCamera_IsShutterSoundPlaying(CtvtGameCamCamera *camera) {
    return CameraSystem_IsShutterSoundPlaying(camera->cameraSystem);
}

static void CtvtGameCamRoulette_Init(CtvtGameCamRoulette *roulette, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(242, heapId);

    roulette->pictures[0] = GFL_ArcToolReadHeapNewLZ(arc, 9, FALSE, heapId);
    roulette->pictures[1] = GFL_ArcToolReadHeapNewLZ(arc, 12, FALSE, heapId);
    roulette->pictures[2] = GFL_ArcToolReadHeapNewLZ(arc, 15, FALSE, heapId);
    roulette->pictures[3] = GFL_ArcToolReadHeapNewLZ(arc, 18, FALSE, heapId);
    GFL_ArcToolFree(arc);
    roulette->cursor = 0;
    roulette->wait = 0;
    roulette->moving = FALSE;
    roulette->timer = 0;
}

static void CtvtGameCamRoulette_Free(CtvtGameCamRoulette *roulette) {
    u8 i;

    for (i = 0; i < 4; i++) {
        GFL_HeapFree(roulette->pictures[i]);
    }
}

static void CtvtGameCamRoulette_Update(CtvtGameCamRoulette *roulette, CtvtGameCamActors *clact) {
    if (roulette->moving == TRUE) {
        if (roulette->wait != 0) {
            roulette->wait--;
        }
        if (roulette->wait == 0) {
            func_0204c378(CtvtGameCamActors_GetActor(clact, roulette->cursor + 3), 0, TRUE);
            roulette->cursor++;
            roulette->cursor %= 4;
            func_0204c378(CtvtGameCamActors_GetActor(clact, roulette->cursor + 3), 1, TRUE);
            roulette->moving = FALSE;
        }
    }
}

// Loads a picture to the sub screen's bitmap BG
static void CtvtGameCam_LoadSubPicture(u16 x, u16 y, u16 width, u16 height, const u16 *src) {
    u8 i;

    cp15_flushDC(src, width * height * 2);
    for (i = 0; i < height; i++) {
        gfxUploadBGScreen3B(&src[width * i], (i + y) * 512 + x * 2, width * 2);
    }
}

static void CtvtGameCamRoulette_Move(CtvtGameCamRoulette *roulette, u16 wait) {
    roulette->wait = wait;
    roulette->moving = TRUE;
    GFL_SndSEPlay(1355);
}

static BOOL CtvtGameCamRoulette_IsMoving(CtvtGameCamRoulette *roulette) {
    return roulette->moving;
}

static void CtvtGameCamExchange_Init(CtvtGameCamExchange *exchange, BOOL cameraEnabled, HeapID heapId) {
    u8 i;

    sys_memset(exchange, 0, sizeof(CtvtGameCamExchange));
    exchange->heapId = heapId;
    if (cameraEnabled == TRUE) {
        exchange->buffer = GFL_HeapAllocate(heapId, 0x2000, TRUE, "ctvt_game_cam.c", 1561);
    }
    for (i = 0; i < 3; i++) {
        exchange->received[i] = 0;
    }
    exchange->connections = func_02042a78();
    exchange->timer = 0;
    exchange->waiting = TRUE;
    exchange->started = FALSE;
}

static void CtvtGameCamExchange_Start(CtvtGameCamExchange *exchange, CtvtGameMember *members, BOOL cameraEnabled) {
    u8 netId;
    u8 slot = 0;
    u8 selfNetId;

    func_02043868(exchange->heapId, FALSE);
    selfNetId = func_02042a6c(func_02040440());
    for (netId = 0; netId < 4; netId++) {
        if (selfNetId == netId) {
            continue;
        }
        if (func_02042a80(netId) == FALSE) {
            slot++;
        } else {
            CtvtGameMember *member = &members[slot];

            if (member->hasCamera == TRUE && !canPlayerExchangePhotos()) {
                func_020439a4(0x2000, netId, exchange->heapId, member->pictures[0]);
                exchange->receiving |= (u8)(1 << netId);
            }
            if (member->canExchangePhotos == FALSE) {
                exchange->noExchange |= (u8)(1 << netId);
            }
            slot++;
        }
    }
}

static void CtvtGameCamExchange_Free(CtvtGameCamExchange *exchange, BOOL cameraEnabled) {
    if (cameraEnabled == TRUE) {
        GFL_HeapFree(exchange->buffer);
    }
    func_020438dc();
}

static void CtvtGameCamExchange_Update(CtvtGameCamExchange *exchange, CtvtGameMember *members) {
    if (exchange->done == TRUE || exchange->started == FALSE) {
        return;
    }
    switch (exchange->state) {
    case 0: {
        u8 finished = 0;
        u8 slot = 0;
        u8 selfNetId = func_02042a6c(func_02040440());
        u8 netId;

        if (!canPlayerExchangePhotos()) {
            for (netId = 0; netId < 4; netId++) {
                if (exchange->receiving & (1 << netId)) {
                    u8 *received = &exchange->received[slot];

                    if (exchange->picture != *received) {
                        finished |= (u8)(1 << netId);
                    } else if (func_02043b24(netId) == TRUE) {
                        func_02043a1c(netId);
                        (*received)++;
                        if (*received < 3) {
                            func_020439a4(0x2000, netId, exchange->heapId, members[slot].pictures[*received]);
                        }
                    }
                }
                if (selfNetId != netId) {
                    slot++;
                }
            }
        }
        if (finished == exchange->receiving) {
            exchange->state = 1;
            func_02043b10();
            return;
        }
        break;
    }
    case 1:
        if (exchange->waiting == FALSE) {
            func_02040624(func_02040440(), exchange->picture + 32, 32);
            exchange->state = 2;
            return;
        }
        break;
    case 2:
        if (func_02040664(func_02040440(), exchange->picture + 32, 32) == TRUE) {
            exchange->state = 0;
            exchange->waiting = TRUE;
            exchange->picture++;
            if (exchange->picture == 3) {
                exchange->done = TRUE;
                exchange->state = 3;
            }
        }
        break;
    case 3:
        break;
    }
}

static void CtvtGameCamExchange_Send(CtvtGameCamExchange *exchange) {
    func_0204393c(exchange->buffer, 0x2000, exchange->noExchange, FALSE);
    exchange->waiting = FALSE;
}

static BOOL CtvtGameCamExchange_CanSend(CtvtGameCamExchange *exchange) {
    if (!func_02043b10()) {
        return FALSE;
    }
    if (exchange->timer < 80) {
        exchange->timer++;
        return FALSE;
    }
    return exchange->waiting;
}

static void *CtvtGameCamExchange_GetBuffer(CtvtGameCamExchange *exchange) {
    return exchange->buffer;
}

static BOOL CtvtGameCamExchange_IsDone(CtvtGameCamExchange *exchange) {
    return exchange->done;
}

static void CtvtGameCamExchange_CopyBuffer(CtvtGameCamExchange *exchange, void *dest) {
    sys_memcpy32(exchange->buffer, dest, 0x2000);
}

// Whether someone left the call
static BOOL CtvtGameCamExchange_IsSomeoneGone(CtvtGameCamExchange *exchange) {
    if (exchange->connections != func_02042a78()) {
        return TRUE;
    }
    return FALSE;
}

static void CtvtGameCamSeq_Init(CtvtGameCamSeq *seq, CtvtGameCamTask *task, CtvtGameCamSeqFunc func) {
    sys_memset(seq, 0, sizeof(CtvtGameCamSeq));
    seq->task = task;
    CtvtGameCamSeq_Set(seq, func);
}

static void CtvtGameCamSeq_Free(CtvtGameCamSeq *seq) {
    sys_memset(seq, 0, sizeof(CtvtGameCamSeq));
}

static void CtvtGameCamSeq_Update(CtvtGameCamSeq *seq) {
    if (!seq->done) {
        seq->func(seq, &seq->state, seq->task);
    }
}

static BOOL CtvtGameCamSeq_IsDone(CtvtGameCamSeq *seq) {
    return seq->done;
}

static void CtvtGameCamSeq_Set(CtvtGameCamSeq *seq, CtvtGameCamSeqFunc func) {
    seq->func = func;
    seq->state = 0;
}

static void CtvtGameCamSeq_End(CtvtGameCamSeq *seq) {
    seq->done = TRUE;
}

static void CtvtGameCam_SeqEnd(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    CtvtGameCamSeq_End(seq);
}

// Waits for everyone, then starts the exchange and the camera or the roulette
static void CtvtGameCam_SeqStart(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    switch (*state) {
    case 0: {
        u8 selfNetId = func_02042a6c(func_02040440());
        BOOL ready = TRUE;
        u8 netId;

        if (!func_02043b10()) {
            ready = FALSE;
        }
        for (netId = 0; netId < 4; netId++) {
            if (selfNetId != netId && func_02042a80(netId) && !func_02043b24(netId)) {
                ready = FALSE;
                break;
            }
        }
        if (ready == TRUE) {
            *state = 1;
        }
        break;
    }
    case 1:
        func_02040624(func_02040440(), 30, 32);
        *state = 2;
        break;
    case 2:
        if (func_02040664(func_02040440(), 30, 32) == TRUE) {
            *state = 3;
        }
        break;
    case 3:
        func_020438dc();
        CtvtGameCamExchange_Start(&task->exchange, task->cam->members, task->cam->cameraEnabled);
        *state = 4;
        break;
    case 4:
        if (task->cam->cameraEnabled && CommTvt_IsCameraEnabled() == TRUE) {
            CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqCameraIntro);
        } else {
            CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqRouletteIntro);
        }
        task->exchange.started = TRUE;
        break;
    }
}

// Waits for the exchange to end with everyone
static void CtvtGameCam_SeqWaitExchange(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    if (!CtvtGameCamExchange_IsDone(&task->exchange)) {
        return;
    }
    switch (*state) {
    case 0:
        func_02040624(func_02040440(), 31, 32);
        *state = 1;
        break;
    case 1:
        if (func_02040664(func_02040440(), 31, 32) == TRUE) {
            *state = 2;
        }
        break;
    case 2:
        CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqEnd);
        break;
    }
}

// Waits for the message to finish printing, then goes on to its next step
static void CtvtGameCam_SeqWaitMessage(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    CtvtGameCamMsg *msg = &task->msg;

    switch (*state) {
    case 0: {
        u32 result = func_020223b4(msg->stream);

        GFL_TCBExMgrUpdate(msg->tcbManager);
        switch (result) {
        case PRINT_STREAM_RUNNING:
        case PRINT_STREAM_PAUSED:
            break;
        case PRINT_STREAM_DONE:
            *state = 1;
            break;
        }
        break;
    }
    case 1:
        if (msg->timer >= msg->wait) {
            *state = 2;
        } else {
            msg->timer++;
        }
        break;
    case 2:
        if (func_02022458(msg->stream) == TRUE) {
            func_0202243c(msg->stream);
        }
        func_020223cc(msg->stream);
        msg->stream = NULL;
        CtvtGameCamSeq_Set(seq, msg->next);
        break;
    }
}

static void CtvtGameCam_SeqCameraIntro(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    func_0204c124(CtvtGameCamActors_GetActor(&task->clact, 1), TRUE);
    CtvtGameCamMsg_Print(&task->msg, &task->msgWin, CtvtGameCam_SeqPictureNumber, 49, task->heapId);
    CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqWaitMessage);
}

static void CtvtGameCam_SeqPictureNumber(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    CtvtGameCamMsg_PrintNumber(&task->msg, &task->msgWin, CtvtGameCam_SeqPicturePrompt, task->camera.picture + 1, 50,
                               task->heapId);
    CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqWaitMessage);
}

static void CtvtGameCam_SeqPicturePrompt(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    CtvtGameCamMsg_Print(&task->msg, &task->msgWin, CtvtGameCam_SeqTakePicture,
                         sCtvtGameCamPromptMessages[task->cam->type][task->camera.picture], task->heapId);
    CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqWaitMessage);
}

// Counts down and takes a picture
static void CtvtGameCam_SeqTakePicture(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    switch (*state) {
    case 0:
        func_0204c124(CtvtGameCamActors_GetActor(&task->clact, 2), TRUE);
        func_0204c56c(CtvtGameCamActors_GetActor(&task->clact, 2));
        *state = 1;
        GFL_SndSEPlay(1355);
        break;
    case 1: {
        u16 timer;

        task->camera.timer++;
        timer = task->camera.timer;
        if (timer % 60 == 0 && timer <= 180) {
            GFL_SndSEPlay(1355);
        }
        if (func_0204c560(CtvtGameCamActors_GetActor(&task->clact, 2))) {
            break;
        }
        task->camera.timer = 0;
        CtvtGameCamCamera_Stop(&task->camera);
        func_0204c124(CtvtGameCamActors_GetActor(&task->clact, 2), FALSE);
        GFL_WipeSet(4, 0, 0, 0x7fff, 6, 2, task->heapId);
        CtvtGameCamCamera_PlayShutterSound(&task->camera);
        *state = 2;
        break;
    }
    case 2:
        if (GFL_WipeIsFinished() == TRUE && !CtvtGameCamCamera_IsShutterSoundPlaying(&task->camera)) {
            GFL_WipeSet(4, 1, 1, 0x7fff, 6, 2, task->heapId);
            *state = 3;
        }
        break;
    case 3:
        if (GFL_WipeIsFinished() == TRUE) {
            CtvtGameCamCamera_TakePicture(&task->camera, CtvtGameCamExchange_GetBuffer(&task->exchange), task->heapId);
            CtvtGameCamExchange_CopyBuffer(&task->exchange, task->cam->pictures[task->camera.picture]);
            *state = 4;
        }
        break;
    case 4:
        CtvtGameCamExchange_Send(&task->exchange);
        CtvtGameCam_LoadMainPicture(task->camera.picture * 72 + 24, 20, 64, 64,
                                    CtvtGameCamExchange_GetBuffer(&task->exchange));
        task->camera.picture++;
        *state = 6;
        break;
    case 6:
        if (task->camera.picture == 3) {
            *state = 7;
        } else if (CtvtGameCamExchange_CanSend(&task->exchange) == TRUE) {
            task->exchange.timer = 0;
            CtvtGameCamCamera_Start(&task->camera);
            CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqPictureNumber);
        }
        break;
    case 7:
        CtvtGameCamMsg_Print(&task->msg, &task->msgWin, CtvtGameCam_SeqCameraDone, 58, task->heapId);
        CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqWaitMessage);
        break;
    case 5:
        break;
    }
    CtvtGameCamMsgWin_Update(&task->msgWin);
    CameraSystem_UpdateSound(task->cam->cameraSystem);
}

static void CtvtGameCam_SeqCameraDone(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqWaitExchange);
}

static void CtvtGameCam_OnFrame(void *frame, void *work) {
    CtvtGameCamCamera *camera = work;

    if (camera->frameReady != TRUE) {
        sys_memcpy32(frame, camera->frame, 256 * 192 * 2);
        camera->frameReady = TRUE;
    }
}

static void CtvtGameCam_SeqRouletteIntro(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    ArcTool *arc;
    u8 i;

    for (i = 3; i < 7; i++) {
        func_0204c124(CtvtGameCamActors_GetActor(&task->clact, i), TRUE);
    }
    arc = GFL_ArcSysCreateFileHandle(242, task->heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 4, 0, 0, task->heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 2, 4, 0, 0, FALSE, task->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 5, 4, 0, 0, FALSE, task->heapId);
    GFL_ArcToolFree(arc);
    GFL_BGSysSetBGEnabled(4, TRUE);
    CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqDrawCharacters);
}

// Draws the four characters' pictures on the sub screen, one a frame
static void CtvtGameCam_SeqDrawCharacters(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    u16 x = task->roulette.cursor % 2 * 118 + 42;
    u16 y = task->roulette.cursor / 2 * 94 + 18;

    switch (*state) {
    case 0:
        CtvtGameCam_LoadSubPicture(x, y, 64, 64, task->roulette.pictures[0]);
        *state = 1;
        break;
    case 1:
        CtvtGameCam_LoadSubPicture(x, y, 64, 64, task->roulette.pictures[1]);
        *state = 3;
        break;
    case 2:
        CtvtGameCam_LoadSubPicture(x, y, 64, 64, task->roulette.pictures[2]);
        *state = 4;
        break;
    case 3:
        CtvtGameCam_LoadSubPicture(x, y, 64, 64, task->roulette.pictures[3]);
        *state = 2;
        break;
    case 4:
        task->roulette.cursor = 0;
        CtvtGameCamMsg_Print(&task->msg, &task->msgWin, CtvtGameCam_SeqRoulettePrompt, 55, task->heapId);
        CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqWaitMessage);
        return;
    }
    task->roulette.cursor++;
}

static void CtvtGameCam_SeqRoulettePrompt(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    CtvtGameCamMsg_Print(&task->msg, &task->msgWin, CtvtGameCam_SeqRoulette, 56, task->heapId);
    CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqWaitMessage);
}

// Spins the roulette, slowing down until it stops on this machine's character
static void CtvtGameCam_SeqRoulette(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    u16 wait;

    switch (*state) {
    case 0:
        if (!CtvtGameCamRoulette_IsMoving(&task->roulette)) {
            CtvtGameCamRoulette_Move(&task->roulette, 5);
        }
        task->roulette.timer++;
        if (task->roulette.timer >= 300) {
            *state = 1;
        }
        break;
    case 1:
        wait = (task->roulette.timer - 300) / 25 + 5;
        if (!CtvtGameCamRoulette_IsMoving(&task->roulette)) {
            CtvtGameCamRoulette_Move(&task->roulette, wait);
        }
        task->roulette.timer++;
        if (task->roulette.timer >= 720) {
            *state = 2;
        }
        break;
    case 2:
        wait = (task->roulette.timer - 300) / 25 + 5;
        if (task->roulette.cursor != task->cam->character) {
            if (!CtvtGameCamRoulette_IsMoving(&task->roulette)) {
                CtvtGameCamRoulette_Move(&task->roulette, wait);
            }
        } else {
            task->roulette.timer = 0;
            task->roulette.blink = 0;
            task->roulette.moving = FALSE;
            *state = 3;
            GFL_SndSEPlay(1358);
        }
        break;
    case 3:
        if (task->roulette.timer % 6 == 0) {
            task->roulette.blink ^= 1;
            func_0204c378(CtvtGameCamActors_GetActor(&task->clact, task->roulette.cursor + 3), task->roulette.blink,
                          TRUE);
        }
        task->roulette.timer++;
        if (task->roulette.timer > 80) {
            *state = 4;
        }
        break;
    case 4: {
        u32 files[4] = { 9, 12, 15, 18 };
        u8 i;

        for (i = 0; i < 3; i++) {
            void *picture = GFL_ArcSysReadHeapNewLZ(242, i + files[task->roulette.cursor], FALSE, task->heapId);

            CtvtGameCam_LoadMainPicture(i * 72 + 24, 20, 64, 64, picture);
            GFL_HeapFree(picture);
        }
        CtvtGameCamMsg_Print(&task->msg, &task->msgWin, CtvtGameCam_SeqRouletteDone, 57, task->heapId);
        CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqWaitMessage);
        break;
    }
    }
    CtvtGameCamMsgWin_Update(&task->msgWin);
    CtvtGameCamRoulette_Update(&task->roulette, &task->clact);
    task->exchange.waiting = FALSE;
}

static void CtvtGameCam_SeqRouletteDone(CtvtGameCamSeq *seq, int *state, CtvtGameCamTask *task) {
    if (CtvtGameCamExchange_IsDone(&task->exchange) == TRUE) {
        CtvtGameCamSeq_Set(seq, CtvtGameCam_SeqWaitExchange);
    }
    task->exchange.waiting = FALSE;
}

static void CtvtGameCam_VBlank(TCB *tcb, void *work) {
    CtvtGameCamTask *task = work;

    if (task->camera.frameReady == TRUE && task->camera.frame != NULL) {
        cp15_flushDC(task->camera.frame, 256 * 192 * 2);
        gfxUploadBGScreen3B(task->camera.frame, 0, 256 * 192 * 2);
        task->camera.frameReady = FALSE;
    }
    func_0204b7c8();
}
