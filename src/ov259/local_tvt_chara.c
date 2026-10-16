#include "types.h"
#include "app/local_tvt_local.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/tcb.h"
#include "nitro/gx.h"
#include "nnsys/g2d.h"
#include "save/player_info.h"
#include "save/save_control.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The faces of the upper screen of the Xtransceiver's calls with the game's characters, named after the ROM's
// "local_tvt_chara.c". A face is a person in front of a background, both 256-color BG characters, with cell actors
// for the eyes and mouth and an object beside it. The characters of the faces are loaded over several frames into
// one of two halves of the character memory of BG2 and BG3, and the BGs switch to that half once all are there

// The states of a face's load
enum {
    FACE_STATE_IDLE,
    FACE_STATE_READ_PERSON,
    FACE_STATE_COPY_PERSON,
    FACE_STATE_READ_BACKGROUND,
    FACE_STATE_COPY_BACKGROUND,
    FACE_STATE_LOADED,
};

// The steps of LocalTvtChara_Update
enum {
    CHARA_SEQ_IDLE,
    CHARA_SEQ_CREATE,
    CHARA_SEQ_LOAD = CHARA_SEQ_CREATE + LOCAL_TVT_FACES,
    CHARA_SEQ_SWITCH = CHARA_SEQ_LOAD + LOCAL_TVT_FACES,
};

// The characters of a face, and of the larger face of a call of two
#define FACE_CHAR_SIZE 0x3000
#define FACE_CHAR_SIZE_LARGE 0x6000
// The characters copied a frame
#define FACE_CHAR_STEP 0x3000
// The two halves of the character memory
#define FACE_CHAR_HALF 0xc000
// The character memory of the persons (BG2) and the backgrounds (BG3)
#define PERSON_CHAR_OFFSET 0x10000
#define BACKGROUND_CHAR_OFFSET 0x28000

#define BACKGROUND_COUNT 7
#define OBJECT_COUNT 3

struct LocalTvtFace {
    u8 index;
    u8 step;
    u8 person;
    u8 background;
    u8 frame;
    u8 unk5;
    u8 frameTimer;
    u8 nameWidth;
    u16 nameMsg;
    u16 object;
    u32 state;
    NNSG2dCharacterData *charData;
    void *charFile;
    BOOL namePending;
    BmpWin *nameWindow;
    u32 palette;
    u32 chars;
    u32 cellAnims;
    ClActor *eyes;
    ClActor *mouth;
    u16 talkTimer;
    u16 blinkTimer;
    BOOL talking;
    LocalTvtChara *chara;
    u16 heapId;
    ClActor *objectActor;
    u32 objectPalette;
    u32 objectChars;
    u32 objectCellAnims;
};

struct LocalTvtChara {
    LocalTvtFaceSetup faceSetups[LOCAL_TVT_FACES];
    LocalTvtCharaSetup setup;
    // The half of the character memory the BGs show
    u32 half;
    int timer;
    int seq;
    LocalTvtFace *faces[LOCAL_TVT_FACES];
    // The face that is loading
    LocalTvtFace *loading;
    MsgData *nameMsgData;
    int count;
    ArcTool *arc;
    u32 cursorPalette;
    u32 cursorChars;
    u32 cursorCellAnims;
    ClActor *cursor;
    u16 heapId;
    BOOL visible;
    BOOL visibleChanged;
    TCB *vblankTask;
    BOOL ready;
};

// The files of a person: the palette and the characters, which the second frame follows, and the cell actor
// resources of the eyes and mouth
typedef struct {
    u8 palette;
    u8 chars;
    u8 obj;
} LocalTvtPersonFiles;

typedef struct {
    u16 palette;
    u16 chars;
} LocalTvtBackgroundFiles;

typedef struct {
    u8 palette;
    u8 chars;
    u8 cells;
    u8 anims;
} LocalTvtObjectFiles;

typedef struct {
    u8 x;
    u8 y;
} LocalTvtNamePos;

static void LocalTvtFace_TalkCore(LocalTvtFace *face, MsgData *msgData, u16 messageId, u32 keyWait);
static LocalTvtFace *LocalTvtFace_Create(LocalTvtChara *chara, const LocalTvtFaceSetup *setup, u32 index,
                                         HeapID heapId);
static void LocalTvtFace_Free(LocalTvtChara *chara, LocalTvtFace *face);
static void LocalTvtFace_Update(LocalTvtFace *face);
static void LocalTvtFace_StartLoad(LocalTvtFace *face, u8 half);
static BOOL LocalTvtFace_IsLoaded(LocalTvtFace *face);
static void LocalTvtFace_LoadPalettes(LocalTvtFace *face, LocalTvtChara *chara, HeapID heapId);
static void LocalTvtFace_LoadGraphics(LocalTvtFace *face, LocalTvtChara *chara, HeapID heapId);
static void LocalTvtFace_ReadChars(LocalTvtFace *face, ArcTool *arc, u32 fileId, HeapID heapId);
static void LocalTvtFace_CopyChars(LocalTvtFace *face, u32 srcOffset, u32 destOffset, u32 size);
static void LocalTvtFace_CreateName(LocalTvtFace *face, LocalTvtChara *chara, HeapID heapId);
static u8 LocalTvtFace_GetNameWidth(LocalTvtFace *face);
static void LocalTvtFace_SetTalking(LocalTvtFace *face, BOOL talking);
static void LocalTvtFace_StopTalking(LocalTvtFace *face);
static void LocalTvtFace_CreateObject(LocalTvtFace *face, LocalTvtChara *chara, u16 object, int count, HeapID heapId);
static void LocalTvtFace_FreeObject(LocalTvtFace *face);
static void LocalTvtFace_SetVisible(LocalTvtFace *face, BOOL visible);
static void LocalTvtChara_CreateCursor(LocalTvtChara *chara);
static void LocalTvtChara_FreeCursor(LocalTvtChara *chara);
static void LocalTvtChara_ShowCursor(LocalTvtChara *chara, u32 index);
static void LocalTvtChara_HideCursor(LocalTvtChara *chara);
static void LocalTvtChara_VBlank(TCB *tcb, void *data);

// Where the names of the faces are, in tiles
static const LocalTvtNamePos sLocalTvtNamePositions[LOCAL_TVT_FACES] = {
    { 0, 0 },
    { 16, 0 },
    { 0, 12 },
    { 16, 12 },
};

static const ClActorPos sLocalTvtObjectPositions[LOCAL_TVT_FACES] = {
    { 128, 96 },
    { 256, 96 },
    { 128, 192 },
    { 256, 192 },
};

// The files of the objects for a call of two and for the others
static const LocalTvtObjectFiles sLocalTvtObjectFiles[OBJECT_COUNT][2] = {
    { { 100, 101, 102, 103 }, { 100, 104, 105, 106 } },
    { { 107, 108, 109, 110 }, { 107, 111, 112, 113 } },
    { { 114, 115, 116, 117 }, { 114, 118, 119, 120 } },
};

static const LocalTvtPersonFiles sLocalTvtPersonFiles[LOCAL_TVT_PERSON_COUNT] = {
    { 2, 18, 56 }, { 3, 20, 60 }, { 4, 22, 64 },  { 5, 24, 68 },  { 0, 14, 44 },  { 6, 26, 48 },  { 8, 30, 76 },
    { 9, 32, 80 }, { 7, 28, 72 }, { 10, 34, 84 }, { 11, 36, 88 }, { 12, 38, 92 }, { 13, 40, 96 }, { 1, 16, 52 },
};

static const LocalTvtBackgroundFiles sLocalTvtBackgroundFiles[BACKGROUND_COUNT][LOCAL_TVT_TIME_COUNT] = {
    { { 129, 130 }, { 129, 130 }, { 129, 130 } }, { { 131, 132 }, { 131, 132 }, { 131, 132 } },
    { { 133, 134 }, { 135, 136 }, { 137, 138 } }, { { 139, 140 }, { 141, 142 }, { 143, 144 } },
    { { 145, 146 }, { 147, 148 }, { 149, 150 } }, { { 151, 152 }, { 153, 154 }, { 155, 156 } },
    { { 157, 158 }, { 159, 160 }, { 161, 162 } },
};

LocalTvtChara *LocalTvtChara_Create(const LocalTvtCharaSetup *setup, HeapID heapId) {
    LocalTvtChara *chara = GFL_HeapAllocate(heapId, sizeof(LocalTvtChara), TRUE, "local_tvt_chara.c", 309);

    chara->setup = *setup;
    chara->timer = 0;
    chara->half = 1;
    chara->seq = CHARA_SEQ_IDLE;
    chara->heapId = heapId;
    chara->arc = GFL_ArcSysCreateFileHandle(ARC_LOCAL_TVT, chara->heapId);
    chara->nameMsgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_0393, chara->heapId);
    LocalTvtChara_CreateCursor(chara);
    GFL_BGSysSetBGEnabled(0, FALSE);
    GFL_BGSysSetBGEnabled(3, FALSE);
    GFL_BGSysSetBGEnabled(2, FALSE);
    chara->vblankTask = GFL_VBlankTCBAdd(LocalTvtChara_VBlank, chara, 0);
    return chara;
}

void LocalTvtChara_Free(LocalTvtChara *chara) {
    int i;

    GFL_TCBRemove(chara->vblankTask);
    LocalTvtChara_FreeCursor(chara);
    GFL_MsgDataFree(chara->nameMsgData);
    GFL_ArcToolFree(chara->arc);
    for (i = 0; i < chara->count; i++) {
        if (chara->faces[i] != NULL) {
            LocalTvtFace_Free(chara, chara->faces[i]);
            chara->faces[i] = NULL;
        }
    }
    GFL_HeapFree(chara);
}

void LocalTvtChara_Update(LocalTvtChara *chara) {
    int i;

    if (chara->timer < 20) {
        chara->timer++;
    }
    if (chara->loading == NULL && chara->seq == CHARA_SEQ_SWITCH) {
        u32 personCharBase;
        u32 backgroundCharBase;

        if (chara->half == 0) {
            personCharBase = GX_BG_CHARBASE(PERSON_CHAR_OFFSET);
            backgroundCharBase = GX_BG_CHARBASE(BACKGROUND_CHAR_OFFSET);
        } else {
            personCharBase = GX_BG_CHARBASE(PERSON_CHAR_OFFSET + FACE_CHAR_HALF);
            backgroundCharBase = GX_BG_CHARBASE(BACKGROUND_CHAR_OFFSET + FACE_CHAR_HALF);
        }
        G2_SetBG2Control256x16Pltt(GX_BG_SCRSIZE_256x16PLTT_256x256, GX_BG_AREAOVER_REPEAT, GX_BG_SCRBASE(0x6800),
                                   personCharBase);
        G2_SetBG3Control256x16Pltt(GX_BG_SCRSIZE_256x16PLTT_256x256, GX_BG_AREAOVER_REPEAT, GX_BG_SCRBASE(0x7000),
                                   backgroundCharBase);
        if (chara->half == 0) {
            chara->half = 1;
        } else {
            chara->half = 0;
        }
        chara->seq = CHARA_SEQ_LOAD;
        chara->timer = 0;
    }
    if (chara->loading != NULL) {
        if (LocalTvtFace_IsLoaded(chara->loading) == TRUE) {
            chara->loading = NULL;
            if (chara->seq >= CHARA_SEQ_SWITCH - 1) {
                chara->ready = TRUE;
            }
        }
    }
    if (chara->loading == NULL) {
        switch (chara->seq) {
        case CHARA_SEQ_CREATE:
        case CHARA_SEQ_CREATE + 1:
        case CHARA_SEQ_CREATE + 2:
        case CHARA_SEQ_CREATE + 3: {
            u8 index = chara->seq - CHARA_SEQ_CREATE;

            if (chara->faceSetups[index].person != LOCAL_TVT_PERSON_NONE) {
                chara->faces[index] = LocalTvtFace_Create(chara, &chara->faceSetups[index], index, chara->heapId);
            }
            chara->seq++;
            break;
        }
        case CHARA_SEQ_LOAD:
        case CHARA_SEQ_LOAD + 1:
        case CHARA_SEQ_LOAD + 2:
        case CHARA_SEQ_LOAD + 3: {
            u8 index = chara->seq - CHARA_SEQ_LOAD;

            if (chara->faces[index] != NULL) {
                LocalTvtFace_StartLoad(chara->faces[index], chara->half);
                chara->loading = chara->faces[index];
                // A call of two has no more faces to load
                if (chara->count == 2 && chara->seq == CHARA_SEQ_LOAD + 1) {
                    chara->seq = CHARA_SEQ_SWITCH;
                } else {
                    chara->seq++;
                }
            } else {
                chara->seq++;
            }
            break;
        }
        }
    }
    for (i = 0; i < chara->count; i++) {
        if (chara->faces[i] != NULL) {
            LocalTvtFace_Update(chara->faces[i]);
        }
    }
}

void LocalTvtChara_Start(LocalTvtChara *chara, const LocalTvtFaceSetup *setups, int count) {
    BOOL freed;
    int i;
    u32 screenFile;

    chara->ready = FALSE;
    chara->count = count;
    chara->timer = 0;
    chara->half = 1;
    chara->seq = CHARA_SEQ_CREATE;
    chara->loading = NULL;
    screenFile = count == 2 ? 42 : 43;
    freed = FALSE;
    loadBGScrToVramByFileNoReserveNegAlign(chara->arc, screenFile, 2, 0, 0, FALSE, chara->heapId);
    loadBGScrToVramByFileNoReserveNegAlign(chara->arc, screenFile, 3, 0, 0, FALSE, chara->heapId);
    for (i = 0; i < LOCAL_TVT_FACES; i++) {
        if (chara->faces[i] != NULL) {
            LocalTvtFace_Free(chara, chara->faces[i]);
            chara->faces[i] = NULL;
            freed = TRUE;
        }
    }
    if (freed) {
        GFL_BGSysLoadScr(0);
    }
    MI_CpuCopy8(setups, chara->faceSetups, sizeof(chara->faceSetups));
}

BOOL LocalTvtChara_IsReady(LocalTvtChara *chara) {
    return chara->ready;
}

void LocalTvtChara_SetVisible(LocalTvtChara *chara, BOOL visible) {
    int i;

    chara->visibleChanged = TRUE;
    chara->visible = visible;
    for (i = 0; i < chara->count; i++) {
        if (chara->faces[i] != NULL) {
            LocalTvtFace_SetVisible(chara->faces[i], chara->visible);
        }
    }
    if (!chara->visible) {
        LocalTvtChara_HideCursor(chara);
    }
}

LocalTvtFace *LocalTvtChara_GetFace(LocalTvtChara *chara, int index) {
    return chara->faces[index];
}

static void LocalTvtFace_TalkCore(LocalTvtFace *face, MsgData *msgData, u16 messageId, u32 keyWait) {
    // The window is on the other half of the screen than the face
    LocalTvtMessage_OpenTalkWindow(face->chara->setup.message,
                                   face->index < 2 ? LOCAL_TVT_TALK_BOTTOM : LOCAL_TVT_TALK_TOP);
    LocalTvtChara_ShowCursor(face->chara, face->index);
    LocalTvtMessage_StartTalk(face->chara->setup.message, msgData, messageId, keyWait);
}

void LocalTvtFace_Talk(LocalTvtFace *face, MsgData *msgData, u16 messageId) {
    LocalTvtFace_TalkCore(face, msgData, messageId, FALSE);
}

void LocalTvtFace_TalkKeyWait(LocalTvtFace *face, MsgData *msgData, u16 messageId) {
    LocalTvtFace_TalkCore(face, msgData, messageId, TRUE);
}

BOOL LocalTvtFace_UpdateTalk(LocalTvtFace *face, LocalTvtChara *chara) {
    if (LocalTvtMessage_IsTalkFinished(face->chara->setup.message)) {
        LocalTvtChara_HideCursor(face->chara);
        LocalTvtFace_StopTalking(face);
        return TRUE;
    }
    // The mouth moves while characters are printed
    if (LocalTvtMessage_IsTalkPaused(face->chara->setup.message)) {
        LocalTvtFace_SetTalking(face, FALSE);
    } else {
        LocalTvtFace_SetTalking(face, TRUE);
    }
    return FALSE;
}

static LocalTvtFace *LocalTvtFace_Create(LocalTvtChara *chara, const LocalTvtFaceSetup *setup, u32 index,
                                         HeapID heapId) {
    LocalTvtFace *face = GFL_HeapAllocate(heapId, sizeof(LocalTvtFace), TRUE, "local_tvt_chara.c", 794);

    face->chara = chara;
    face->heapId = heapId;
    face->index = index;
    face->person = setup->person;
    face->background = setup->background;
    face->object = setup->object;
    face->nameMsg = setup->nameMsg;
    face->frame = 0;
    face->unk5 = 0;
    face->namePending = FALSE;
    face->talking = FALSE;
    face->blinkTimer = GFL_RandomLCAlt(75);
    face->objectPalette = 0xff;
    face->objectChars = 0xff;
    face->objectCellAnims = 0xff;
    if (face->person == LOCAL_TVT_PERSON_PLAYER && getTrainerGender(chara->setup.playerInfo) == 1) {
        face->person = LOCAL_TVT_PERSON_PLAYER_FEMALE;
    }
    LocalTvtFace_LoadGraphics(face, chara, heapId);
    LocalTvtFace_LoadPalettes(face, chara, heapId);
    LocalTvtFace_CreateName(face, chara, heapId);
    LocalTvtFace_CreateObject(face, chara, setup->object, chara->count, heapId);
    face->state = FACE_STATE_IDLE;
    face->charFile = NULL;
    face->frameTimer = GFL_RandomLCAlt(40);
    face->talkTimer = 0;
    return face;
}

static void LocalTvtFace_Free(LocalTvtChara *chara, LocalTvtFace *face) {
    LocalTvtFace_FreeObject(face);
    func_0204c108(face->eyes);
    func_0204c108(face->mouth);
    func_0204bcd0(face->palette);
    func_0204b98c(face->chars);
    func_0204be64(face->cellAnims);
    BmpWin_ClearScreen(face->nameWindow);
    BmpWin_Free(face->nameWindow);
    if (face->charFile != NULL) {
        GFL_HeapFree(face->charFile);
    }
    GFL_HeapFree(face);
}

static void LocalTvtFace_Update(LocalTvtFace *face) {
    u32 size = face->chara->count == 2 ? FACE_CHAR_SIZE_LARGE : FACE_CHAR_SIZE;
    u32 half = face->chara->half == 0 ? 0 : FACE_CHAR_HALF;
    u32 srcOffset = face->chara->count == 2 ? 0 : 0x1c00;

    // Load the other frame of the person and the background again into the half that isn't shown
    switch (face->state) {
    case FACE_STATE_READ_PERSON:
        face->step = 0;
        LocalTvtFace_ReadChars(face, face->chara->arc, face->frame + sLocalTvtPersonFiles[face->person].chars,
                               face->heapId);
        face->state = FACE_STATE_COPY_PERSON;
        break;
    case FACE_STATE_COPY_PERSON:
        LocalTvtFace_CopyChars(face, srcOffset + face->step * FACE_CHAR_STEP,
                               half + (size * face->index + PERSON_CHAR_OFFSET + face->step * FACE_CHAR_STEP),
                               FACE_CHAR_STEP);
        face->step++;
        if (face->step * FACE_CHAR_STEP >= size) {
            face->state = FACE_STATE_READ_BACKGROUND;
            GFL_HeapFree(face->charFile);
            face->charFile = NULL;
        }
        break;
    case FACE_STATE_READ_BACKGROUND:
        face->step = 0;
        LocalTvtFace_ReadChars(face, face->chara->arc,
                               sLocalTvtBackgroundFiles[face->background][face->chara->setup.time].chars, face->heapId);
        face->state = FACE_STATE_COPY_BACKGROUND;
        break;
    case FACE_STATE_COPY_BACKGROUND:
        LocalTvtFace_CopyChars(face, srcOffset + face->step * FACE_CHAR_STEP,
                               half + (size * face->index + BACKGROUND_CHAR_OFFSET + face->step * FACE_CHAR_STEP),
                               FACE_CHAR_STEP);
        face->step++;
        if (face->step * FACE_CHAR_STEP >= size) {
            face->state = FACE_STATE_LOADED;
            GFL_HeapFree(face->charFile);
            face->charFile = NULL;
        }
        break;
    }
    if (face->frameTimer < 40) {
        face->frameTimer++;
    } else if (face->state == FACE_STATE_LOADED || face->state == FACE_STATE_IDLE) {
        face->frameTimer = 0;
        face->frame = face->frame == 0;
        if (face->unk5 == 0) {
            face->unk5 = 1;
        } else {
            face->unk5 = 0;
        }
    }
    {
        PrintQueue *queue = LocalTvtMessage_GetPrintQueue(face->chara->setup.message);

        if (face->namePending == TRUE && func_02021c0c(queue) == TRUE) {
            BmpWin_FlushChar(face->nameWindow);
            face->namePending = FALSE;
        }
    }
    face->blinkTimer += (u16)(GFL_RandomLC(2) + 1);
    if (face->blinkTimer >= 150) {
        func_0204c540(face->eyes);
        func_0204c56c(face->eyes);
        face->blinkTimer = 0;
    }
    // The mouth keeps moving for a while after the characters stop
    if (face->talking == TRUE || face->talkTimer != 0) {
        if (!func_0204c560(face->mouth)) {
            func_0204c540(face->mouth);
        }
    } else {
        if (func_0204c560(face->mouth) == TRUE) {
            if (func_0204c510(face->mouth) == 0) {
                func_0204c550(face->mouth);
            }
        }
    }
    if (face->talkTimer != 0) {
        face->talkTimer--;
    }
}

static void LocalTvtFace_StartLoad(LocalTvtFace *face, u8 half) {
    face->state = FACE_STATE_READ_PERSON;
}

static BOOL LocalTvtFace_IsLoaded(LocalTvtFace *face) {
    if (face->state == FACE_STATE_LOADED) {
        return TRUE;
    }
    return FALSE;
}

static void LocalTvtFace_LoadPalettes(LocalTvtFace *face, LocalTvtChara *chara, HeapID heapId) {
    u8 height = 24;

    if (chara->count != 2) {
        height = 12;
    }

    GFL_G2DIOLoadArcNCLR(chara->arc, sLocalTvtPersonFiles[face->person].palette, 2, 0, face->index * 0x200 + 0x4000,
                         0x200, heapId);
    GFL_G2DIOLoadArcNCLR(chara->arc, sLocalTvtBackgroundFiles[face->background][face->chara->setup.time].palette, 2, 0,
                         face->index * 0x200 + 0x6000, 0x200, heapId);
    GFL_BGSysSetScrPaletteNo(2, (face->index % 2) * 16, (face->index / 2) * height, 16, height, face->index);
    GFL_BGSysSetScrPaletteNo(3, (face->index % 2) * 16, (face->index / 2) * height, 16, height, face->index);
    GFL_BGSysQueueScrLoad(3);
    GFL_BGSysQueueScrLoad(2);
}

static void LocalTvtFace_LoadGraphics(LocalTvtFace *face, LocalTvtChara *chara, HeapID heapId) {
    u32 size;
    u32 srcOffset;
    ClActorSetup setup;

    if (chara->count == 2) {
        size = FACE_CHAR_SIZE_LARGE;
        srcOffset = 0;
    } else {
        size = FACE_CHAR_SIZE;
        srcOffset = 0x1c00;
    }
    LocalTvtFace_ReadChars(face, chara->arc, sLocalTvtPersonFiles[face->person].chars, heapId);
    LocalTvtFace_CopyChars(face, srcOffset, size * face->index + PERSON_CHAR_OFFSET, size);
    GFL_HeapFree(face->charFile);
    LocalTvtFace_ReadChars(face, chara->arc, sLocalTvtBackgroundFiles[face->background][chara->setup.time].chars,
                           heapId);
    LocalTvtFace_CopyChars(face, srcOffset, size * face->index + BACKGROUND_CHAR_OFFSET, size);
    GFL_HeapFree(face->charFile);
    face->palette =
        func_0204bc48(chara->arc, sLocalTvtPersonFiles[face->person].obj, 0, (face->index * 2 + 3) * 0x20, heapId);
    face->chars = func_0204b81c(chara->arc, sLocalTvtPersonFiles[face->person].obj + 1, FALSE, 0, heapId);
    face->cellAnims = func_0204bde0(chara->arc, sLocalTvtPersonFiles[face->person].obj + 2,
                                    sLocalTvtPersonFiles[face->person].obj + 3, heapId);
    setup.priority = 10;
    setup.bgPriority = 1;
    setup.x = (face->index % 2) * 128 + 64;
    setup.y = (face->index / 2) * 96 + 48;
    if (chara->count == 2) {
        setup.y += 56;
    }
    setup.sequence = 0;
    face->eyes = func_0204c040(chara->setup.unit, face->chars, face->palette, face->cellAnims, &setup, 0, heapId);
    func_0204c124(face->eyes, FALSE);
    func_0204c520(face->eyes, TRUE);
    func_0204c550(face->eyes);
    setup.sequence = 1;
    face->mouth = func_0204c040(chara->setup.unit, face->chars, face->palette, face->cellAnims, &setup, 0, heapId);
    func_0204c124(face->mouth, FALSE);
    func_0204c520(face->mouth, TRUE);
    func_0204c550(face->mouth);
}

static void LocalTvtFace_ReadChars(LocalTvtFace *face, ArcTool *arc, u32 fileId, HeapID heapId) {
    face->charFile = GFL_G2DIOReadBGNCGRArc(arc, fileId, FALSE, &face->charData, heapId);
}

static void LocalTvtFace_CopyChars(LocalTvtFace *face, u32 srcOffset, u32 destOffset, u32 size) {
    MIi_CpuCopy32((u8 *)face->charData->rawData + srcOffset, (void *)(HW_BG_VRAM + destOffset), size);
}

static void LocalTvtFace_CreateName(LocalTvtFace *face, LocalTvtChara *chara, HeapID heapId) {
    u32 messageId;
    Font *font;
    PrintQueue *queue;
    StrBuf *name;

    face->nameWindow = BmpWin_CreateDynamic(0, sLocalTvtNamePositions[face->index].x + 2,
                                            sLocalTvtNamePositions[face->index].y + 1, 12, 2, 12, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(face->nameWindow), 0);
    BmpWin_FlushChar(face->nameWindow);
    BmpWin_FlushMap(face->nameWindow);
    GFL_BGSysLoadScr(0);
    messageId = face->person;
    font = LocalTvtMessage_GetFont(chara->setup.message);
    queue = LocalTvtMessage_GetPrintQueue(chara->setup.message);
    if (face->nameMsg != LOCAL_TVT_NAME_DEFAULT) {
        messageId = face->nameMsg;
    }
    name = GFL_MsgDataLoadStrbufNew(chara->nameMsgData, messageId);
    // The names of the player and the rival are words of their messages
    if (messageId == LOCAL_TVT_PERSON_PLAYER || messageId == LOCAL_TVT_PERSON_PLAYER_FEMALE ||
        messageId == LOCAL_TVT_PERSON_RIVAL) {
        StrBuf *formatted = GFL_StrBufCreate(0x80, heapId);
        PlayerInfo *playerInfo = chara->setup.playerInfo;
        RivalDataSave *rival = chara->setup.rival;
        WordSet *wordSet = GFL_WordSetSystemCreateDefault(heapId);

        copyVarForText(wordSet, 0, playerInfo);
        GFL_WordSetLoadStr(wordSet, 1, getPtrToRivalName(rival));
        GFL_WordSetFormatStrbuf(wordSet, formatted, name);
        GFL_StrBufFree(name);
        GFL_WordSetSystemFree(wordSet);
        name = formatted;
    }
    face->nameWidth = GFL_FontGetBlockWidth(name, font, 0);
    // The outline, then the name
    func_02021c7c(queue, BmpWin_GetBitmap(face->nameWindow), (96 - face->nameWidth) / 2 + 1, 1, name, font,
                  PRINT_COLOR(15, 0, 0));
    func_02021c7c(queue, BmpWin_GetBitmap(face->nameWindow), (96 - face->nameWidth) / 2 - 1, 1, name, font,
                  PRINT_COLOR(15, 0, 0));
    func_02021c7c(queue, BmpWin_GetBitmap(face->nameWindow), (96 - face->nameWidth) / 2, 2, name, font,
                  PRINT_COLOR(15, 0, 0));
    func_02021c7c(queue, BmpWin_GetBitmap(face->nameWindow), (96 - face->nameWidth) / 2, 0, name, font,
                  PRINT_COLOR(15, 0, 0));
    func_02021c7c(queue, BmpWin_GetBitmap(face->nameWindow), (96 - face->nameWidth) / 2, 1, name, font,
                  PRINT_COLOR(1, 0, 0));
    GFL_StrBufFree(name);
    face->namePending = TRUE;
}

static u8 LocalTvtFace_GetNameWidth(LocalTvtFace *face) {
    return face->nameWidth;
}

static void LocalTvtFace_SetTalking(LocalTvtFace *face, BOOL talking) {
    if (face->talking == TRUE && !talking) {
        face->talkTimer = 70;
    }
    face->talking = talking;
}

static void LocalTvtFace_StopTalking(LocalTvtFace *face) {
    face->talkTimer = 0;
    face->talking = FALSE;
}

static void LocalTvtFace_CreateObject(LocalTvtFace *face, LocalTvtChara *chara, u16 object, int count, HeapID heapId) {
    u32 paletteFile;
    u32 charFile;
    u32 cellFile;
    u32 animFile;
    ClActorSetup setup;

    if (object == LOCAL_TVT_OBJECT_NONE) {
        return;
    }
    if (count == 2) {
        paletteFile = sLocalTvtObjectFiles[object][0].palette;
        charFile = sLocalTvtObjectFiles[object][0].chars;
        cellFile = sLocalTvtObjectFiles[object][0].cells;
        animFile = sLocalTvtObjectFiles[object][0].anims;
    } else {
        paletteFile = sLocalTvtObjectFiles[object][1].palette;
        charFile = sLocalTvtObjectFiles[object][1].chars;
        cellFile = sLocalTvtObjectFiles[object][1].cells;
        animFile = sLocalTvtObjectFiles[object][1].anims;
    }
    face->objectPalette = func_0204bc48(chara->arc, paletteFile, 0, (face->index + 12) * 0x20, heapId);
    face->objectChars = func_0204b81c(chara->arc, charFile, FALSE, 0, heapId);
    face->objectCellAnims = func_0204bde0(chara->arc, cellFile, animFile, heapId);
    setup.priority = 10;
    setup.bgPriority = 2;
    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    setup.x = sLocalTvtObjectPositions[face->index].x;
    setup.y = sLocalTvtObjectPositions[face->index].y;
    face->objectActor = func_0204c040(chara->setup.unit, face->objectChars, face->objectPalette, face->objectCellAnims,
                                      &setup, 0, heapId);
    func_0204c124(face->objectActor, FALSE);
    func_0204c520(face->objectActor, TRUE);
    func_0204c4d4(face->objectActor, GFL_RandomLCAlt(0x4000));
}

static void LocalTvtFace_FreeObject(LocalTvtFace *face) {
    if (face->objectActor != NULL) {
        func_0204c108(face->objectActor);
        face->objectActor = NULL;
    }
    if (face->objectPalette != 0xff) {
        func_0204bcd0(face->objectPalette);
        face->objectPalette = 0xff;
    }
    if (face->objectChars != 0xff) {
        func_0204b98c(face->objectChars);
        face->objectChars = 0xff;
    }
    if (face->objectCellAnims != 0xff) {
        func_0204be64(face->objectCellAnims);
        face->objectCellAnims = 0xff;
    }
}

static void LocalTvtFace_SetVisible(LocalTvtFace *face, BOOL visible) {
    if (face->objectActor != NULL) {
        func_0204c124(face->objectActor, visible);
    }
    func_0204c124(face->eyes, visible);
    func_0204c124(face->mouth, visible);
}

// The cursor that points at the name of the face that talks
static void LocalTvtChara_CreateCursor(LocalTvtChara *chara) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARC_LOCAL_TVT_COMMON, chara->heapId);
    ClActorSetup setup;

    chara->cursorPalette = func_0204bc48(arc, 3, 0, 0, chara->heapId);
    chara->cursorChars = func_0204b81c(arc, 27, FALSE, 0, chara->heapId);
    chara->cursorCellAnims = func_0204bde0(arc, 21, 24, chara->heapId);
    GFL_ArcToolFree(arc);
    setup.priority = 10;
    setup.bgPriority = 1;
    setup.x = 0;
    setup.y = 0;
    setup.sequence = 0;
    chara->cursor = func_0204c040(chara->setup.unit, chara->cursorChars, chara->cursorPalette, chara->cursorCellAnims,
                                  &setup, 0, chara->heapId);
    func_0204c124(chara->cursor, FALSE);
    func_0204c520(chara->cursor, TRUE);
}

static void LocalTvtChara_FreeCursor(LocalTvtChara *chara) {
    func_0204c108(chara->cursor);
    func_0204bcd0(chara->cursorPalette);
    func_0204b98c(chara->cursorChars);
    func_0204be64(chara->cursorCellAnims);
}

static void LocalTvtChara_ShowCursor(LocalTvtChara *chara, u32 index) {
    ClActorPos pos;
    u8 nameWidth = LocalTvtFace_GetNameWidth(LocalTvtChara_GetFace(chara, index));

    pos.x = nameWidth / 2 + (sLocalTvtNamePositions[index].x * 8 + 80);
    pos.y = sLocalTvtNamePositions[index].y * 8 + 16;
    func_0204c140(chara->cursor, &pos, 0);
    func_0204c124(chara->cursor, TRUE);
    func_0204c56c(chara->cursor);
}

static void LocalTvtChara_HideCursor(LocalTvtChara *chara) {
    func_0204c124(chara->cursor, FALSE);
}

static void LocalTvtChara_VBlank(TCB *tcb, void *data) {
    LocalTvtChara *chara = data;

    if (chara->visibleChanged) {
        chara->visibleChanged = FALSE;
        GFL_BGSysSetBGEnabled(0, chara->visible);
        GFL_BGSysSetBGEnabled(3, chara->visible);
        GFL_BGSysSetBGEnabled(2, chara->visible);
    }
}
