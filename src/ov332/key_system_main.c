#include "types.h"
#include "app/unova_link.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/os.h"
#include "save/key_info.h"
#include "save/save_control.h"
#include "save/save_outside.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/script/global_10520.h"

// Unova Link's proc: the BG, actors and windows that every part shares, and the main menu that leads to the Key
// System, Memory Link and the Nintendo 3DS Link with Pokémon Dream Radar

// The scrolling BG and its palette fades
struct KeySystemBG {
    fx32 scroll;
    u16 heapId;
    // KEY_SYSTEM_BG_FADE_*, 0 when none runs
    u32 fade;
    u16 frame;
    u16 duration;
    u16 mainColors[16];
    u16 subColors[16];
    u16 palettes[KEY_SYSTEM_BG_PALETTE_COUNT][16];
};

// The choices of the top menu
enum {
    TOP_MENU_KEY_SYSTEM,
    TOP_MENU_MEMORY_LINK,
    TOP_MENU_DREAM_RADAR,
    TOP_MENU_BACK,
};

static BOOL KeySystem_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL KeySystem_ProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL KeySystem_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static KeySystemBG *KeySystemBG_Create(HeapID heapId);
static void KeySystemBG_Free(KeySystemBG *bg);
static void KeySystemBG_Load(KeySystemBG *bg, HeapID heapId);
static void KeySystemBG_Update(KeySystemBG *bg);
static void KeySystemClAct_Init(KeySystemClAct *clact, KeySystemGraphic *graphic, HeapID heapId);
static void KeySystemClAct_LoadResources(KeySystemClAct *clact, HeapID heapId);
static void KeySystemClAct_FreeResources(KeySystemClAct *clact);
static void KeySystemTags_Init(KeySystemTag *tags, HeapID heapId);
static void KeySystemTags_Clear(KeySystemTag *tags);
static void KeySystem_CreateDefaultTitleWin(KeySystemWork *wk, HeapID heapId);
static void KeySystem_SeqSaveCorrupted(KeySystemSeq *seq, int *state, void *work);
static void KeySystem_SeqTopMenu(KeySystemSeq *seq, int *state, void *work);
static void KeySystem_SeqStartTopScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystem_SeqStartMsgScene(KeySystemSeq *seq, int *state, void *work);
static void KeySystemTopScene_Init(void *work, HeapID heapId);
static BOOL KeySystemTopScene_Main(void *work);
static void KeySystemTopScene_Exit(void *work);
static void KeySystemMsgScene_Init(void *work, HeapID heapId);
static BOOL KeySystemMsgScene_Main(void *work);
static void KeySystemMsgScene_Exit(void *work);

const GameProcFunctions UNOVA_LINK_PROC_FUNCTIONS = {
    KeySystem_ProcInit,
    KeySystem_ProcMain,
    KeySystem_ProcExit,
};

// The first step of the sequence for each UNOVA_LINK_MODE_*
static const KeySystemSeqFunc sStartSeqs[] = {
    KeySystemFlow_SeqGameClear,
    KeySystem_SeqTop,
};

static const u32 sPreloadedSeqs[] = {
    SEQ_BGM_DATA_CONV,
    SEQ_SE_MSCL_04,
};

static const KeySystemSceneFuncs sTopSceneFuncs = {
    KeySystemTopScene_Init,
    KeySystemTopScene_Main,
    KeySystemTopScene_Exit,
    NULL,
};

static const KeySystemSceneFuncs sMsgSceneFuncs = {
    KeySystemMsgScene_Init,
    KeySystemMsgScene_Main,
    KeySystemMsgScene_Exit,
    NULL,
};

static BOOL KeySystem_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    KeySystemWork *wk;
    GameData *gameData;
    KeySystemSeqFunc first;

    GFL_HeapCreateChild(HEAPID_USER, HEAPID_KEY_SYSTEM, 0x30000);
    wk = GFL_ProcInitSubsystem(proc, sizeof(KeySystemWork), HEAPID_KEY_SYSTEM);
    sys_memset(wk, 0, sizeof(KeySystemWork));
    wk->param = param;
    if (param == NULL) {
        wk->param = GFL_HeapAllocate(HEAPID_KEY_SYSTEM, sizeof(UnovaLinkParam), TRUE, "key_system_main.c", 152);
        wk->param->mode = UNOVA_LINK_MODE_START_MENU;
        wk->param->gameData = GameData_Create(HEAPID_KEY_SYSTEM);
        InitZoneDataSystem(HEAPID_KEY_SYSTEM);
    }
    gameData = wk->param->gameData;
    wk->net = KeySystemNet_Create(&gameData, HEAPID_KEY_SYSTEM);
    KeySystemTags_Init(wk->tags, HEAPID_KEY_SYSTEM);
    wk->seq = KeySystemSeq_Create(12, wk, NULL, HEAPID_KEY_SYSTEM);
    first = sStartSeqs[wk->param->mode];
    if (func_02007464(GameData_GetSaveControl(wk->param->gameData)) & 2) {
        first = KeySystem_SeqSaveCorrupted;
    }
    KeySystemSeq_Set(wk->seq, first);
    KeySystem_Setup(wk, HEAPID_KEY_SYSTEM);
    return TRUE;
}

static BOOL KeySystem_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    KeySystemWork *wk = work;

    if (wk->enteredCode != NULL) {
        GFL_StrBufFree(wk->enteredCode);
    }
    KeySystem_Teardown(wk, FALSE);
    KeySystemSeq_Free(wk->seq);
    KeySystemNet_Free(wk->net);
    if (param == NULL) {
        FreeZoneDataSystem();
        GameData_Free(wk->param->gameData);
        GFL_HeapFree(wk->param);
        sys_reset(0);
    }
    if (wk->wbSave != NULL) {
        GFL_HeapFree(wk->wbSave);
    }
    GFL_ProcReleaseSubsystem(proc);
    GFL_HeapDelete(HEAPID_KEY_SYSTEM);
    return TRUE;
}

static BOOL KeySystem_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    KeySystemWork *wk = work;

    if (wk->net != NULL) {
        switch (KeySystemNet_CheckError(wk->net)) {
        case 0:
            break;
        case 1:
        case 2:
        default:
            KeySystemNet_Reset(wk->net);
            GFL_SndStop();
            if (wk->wbSave != NULL) {
                GFL_HeapFree(wk->wbSave);
                wk->wbSave = NULL;
            }
            KeySystemNet_SetMode(wk->net, 0);
            KeySystemSeq_PopTo(wk->seq, KeySystem_SeqTop);
            if (wk->menu != NULL) {
                KeySystem_FreeMenu(wk);
            }
            KeySystemScene_Abort(wk->scene);
            KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_0, 1);
            if (wk->msgWin != NULL) {
                KeySystem_FreeMsgWin(wk);
            }
            KeySystemTags_Clear(wk->tags);
            KeySystemSeq_Push(wk->seq, KeySystem_SeqTopMenu);
            return FALSE;
        }
    }
    KeySystemSeq_Run(wk->seq);
    if (wk->graphic != NULL) {
        KeySystemGraphic_Update(wk->graphic);
        KeySystemGraphic_Begin3D(wk->graphic);
        KeySystemGraphic_End3D(wk->graphic);
    }
    if (wk->scene != NULL) {
        KeySystemScene_Update(wk->scene);
    }
    if (wk->bg != NULL) {
        KeySystemBG_Update(wk->bg);
    }
    if (wk->printQueue != NULL) {
        func_02021a3c(wk->printQueue);
    }
    if (wk->titleWin != NULL) {
        KeySystemMsgWin_Update(wk->titleWin);
    }
    if (wk->msgWin != NULL) {
        KeySystemMsgWin_Update(wk->msgWin);
    }
    if (wk->infoWin != NULL) {
        KeySystemMsgWin_Update(wk->infoWin);
    }
    if (wk->msgWinGroup != NULL) {
        KeySystemMsgWinGroup_Update(wk->msgWinGroup);
    }
    if (wk->list != NULL) {
        KeySystemList_Draw(wk->list);
    }
    if (wk->menu != NULL) {
        KeySystemMenu_UpdatePrint(wk->menu);
    }
    if (wk->net != NULL) {
        KeySystemNet_Update(wk->net);
    }
    if (KeySystemSeq_IsEmpty(wk->seq)) {
        return TRUE;
    }
    return FALSE;
}

void KeySystem_SeqFadeIn(KeySystemSeq *seq, int *state, void *work) {
    switch (*state) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        (*state)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

void KeySystem_SeqFadeOut(KeySystemSeq *seq, int *state, void *work) {
    switch (*state) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 0, 16, 0);
        (*state)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

void KeySystem_SeqFadeInUnused(KeySystemSeq *seq, int *state, void *work) {
    switch (*state) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 0);
        (*state)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

void KeySystem_SeqFadeOutWhite(KeySystemSeq *seq, int *state, void *work) {
    switch (*state) {
    case 0:
        GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 0, 16, 0);
        (*state)++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

void KeySystem_SeqEndScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_RequestEnd(wk->scene);
        (*state)++;
        break;
    case 1:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

void KeySystem_SeqWirelessOff(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;
    MsgData *msgData;
    StrBuf *str;

    switch (*state) {
    case 0:
        KeySystem_CreateMsgWin(wk, HEAPID_KEY_SYSTEM);
        msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_GLOBAL_10520, HEAPID_KEY_SYSTEM);
        str = GFL_MsgDataLoadStrbufNew(msgData, Global10520_Text_WirelessCommunicationsTurnedOff);
        KeySystemMsgWin_PrintStr(wk->msgWin, str, KEY_SYSTEM_MSG_STREAM);
        GFL_StrBufFree(str);
        GFL_MsgDataFree(msgData);
        GFL_BGSysQueueScrLoad(0);
        *state = 1;
        break;
    case 1:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            KeySystem_FreeMsgWin(wk);
            GFL_BGSysQueueScrLoad(0);
            *state = 2;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static KeySystemBG *KeySystemBG_Create(HeapID heapId) {
    return GFL_HeapAllocate(heapId, sizeof(KeySystemBG), TRUE, "key_system_main.c", 676);
}

static void KeySystemBG_Free(KeySystemBG *bg) {
    GFL_HeapFree(bg);
}

static void KeySystemBG_Load(KeySystemBG *bg, HeapID heapId) {
    ArcTool *arc;
    void *file;
    NNSG2dPaletteData *palette;
    u16 *colors;

    bg->heapId = heapId;
    GFL_BGSysClearCharCore(0, 32, 0, heapId);
    GFL_BGSysClearCharCore(1, 32, 0, heapId);
    GFL_BGSysClearCharCore(4, 32, 0, heapId);

    arc = GFL_ArcSysCreateFileHandle(ARCID_KEY_SYSTEM, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 0, 0, 32, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 0, 4, 0, 32, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 3, 3, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 3, 2, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 3, 7, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 3, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 6, 3, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 6, 7, 0, 0, FALSE, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 0, 32, 64, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 4, 1, 10, 0, FALSE, heapId);

    file = GFL_G2DIOReadNCLRArc(arc, 0, &palette, heapId);
    colors = palette->rawData;
    sys_memcpy(&colors[0], bg->palettes[0], sizeof(bg->palettes[0]));
    sys_memcpy(&colors[16], bg->palettes[1], sizeof(bg->palettes[1]));
    sys_memcpy(&colors[32], bg->palettes[2], sizeof(bg->palettes[2]));
    sys_memcpy(&colors[48], bg->palettes[3], sizeof(bg->palettes[3]));
    sys_memcpy(&colors[64], bg->palettes[4], sizeof(bg->palettes[4]));
    GFL_HeapFree(file);
    GFL_ArcToolFree(arc);

    LoadSysMsgBox(0, 1, 15, 0, HEAPID_KEY_SYSTEM);
    LoadSysMsgBox(4, 1, 15, 0, HEAPID_KEY_SYSTEM);

    arc = GFL_ArcSysCreateFileHandle(ARCID_FONT, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 5, 0, 14 * 32, 32, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, 5, 4, 14 * 32, 32, heapId);
    GFL_ArcToolFree(arc);
}

static void KeySystemBG_Update(KeySystemBG *bg) {
    u16 *to;
    u16 *from;
    u16 t;

    bg->scroll -= 0x333;
    GFL_BGSysMoveBGReq(3, 3, bg->scroll >> FX32_SHIFT);
    GFL_BGSysMoveBGReq(7, 3, bg->scroll >> FX32_SHIFT);
    if (bg->fade != KEY_SYSTEM_BG_FADE_NONE) {
        t = 0x7fff * bg->frame / bg->duration;
        switch (bg->fade) {
        case KEY_SYSTEM_BG_FADE_0_TO_1:
            to = bg->palettes[1];
            from = bg->palettes[0];
            break;
        case KEY_SYSTEM_BG_FADE_0_TO_2:
            to = bg->palettes[2];
            from = bg->palettes[0];
            break;
        case KEY_SYSTEM_BG_FADE_1_TO_0:
            to = bg->palettes[0];
            from = bg->palettes[1];
            break;
        case KEY_SYSTEM_BG_FADE_2_TO_0:
            to = bg->palettes[0];
            from = bg->palettes[2];
            break;
        case KEY_SYSTEM_BG_FADE_3_TO_0:
            to = bg->palettes[0];
            from = bg->palettes[3];
            break;
        case KEY_SYSTEM_BG_FADE_0_TO_3:
            to = bg->palettes[3];
            from = bg->palettes[0];
            break;
        case KEY_SYSTEM_BG_FADE_4_TO_3:
            to = bg->palettes[3];
            from = bg->palettes[4];
            break;
        case KEY_SYSTEM_BG_FADE_3_TO_4:
            to = bg->palettes[4];
            from = bg->palettes[3];
            break;
        case KEY_SYSTEM_BG_FADE_0:
            to = bg->palettes[0];
            from = to;
            break;
        }
        KeySystem_BlendPalette(15, bg->mainColors, t, 0, from, to);
        KeySystem_BlendPalette(31, bg->subColors, t, 0, from, to);
        if (bg->frame++ >= bg->duration) {
            bg->fade = KEY_SYSTEM_BG_FADE_NONE;
        }
    }
}

void KeySystemBG_StartFade(KeySystemBG *bg, u32 fade, u16 duration) {
    bg->fade = fade;
    bg->frame = 0;
    bg->duration = duration;
}

void KeySystemBG_LoadScreen(KeySystemBG *bg, u8 bgId, u32 screen) {
    u32 fileId;

    switch (screen) {
    case 0:
        fileId = 7;
        break;
    case 3:
        fileId = 10;
        break;
    case 4:
        fileId = 11;
        break;
    case 1:
        fileId = 9;
        break;
    case 2:
        fileId = 8;
        break;
    default:
        return;
    }
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_KEY_SYSTEM, fileId, bgId, 0, 0, FALSE, bg->heapId);
}

static void KeySystemClAct_Init(KeySystemClAct *clact, KeySystemGraphic *graphic, HeapID heapId) {
    sys_memset(clact, 0, sizeof(KeySystemClAct));
    clact->unit = KeySystemGraphic_GetClActUnit(graphic);
}

static void KeySystemClAct_LoadResources(KeySystemClAct *clact, HeapID heapId) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_KEY_SYSTEM, heapId);

    clact->resources[1] = func_0204b81c(arc, 5, FALSE, 0, heapId);
    clact->resources[0] = func_0204bbb8(arc, 2, 0, 0, 0, 4, heapId);
    clact->resources[2] = func_0204bde0(arc, 13, 14, heapId);
    GFL_ArcToolFree(arc);
}

static void KeySystemClAct_FreeResources(KeySystemClAct *clact) {
    func_0204bcd0(clact->resources[0]);
    func_0204b98c(clact->resources[1]);
    func_0204be64(clact->resources[2]);
}

ClActor *KeySystemClAct_Create(KeySystemClAct *clact, u32 id, HeapID heapId) {
    ClActorSetup setup;
    BOOL autoAnim = TRUE;
    BOOL visible = TRUE;
    u32 affineMode = 2;
    u32 surface;

    switch (id) {
    case 0:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 128;
        setup.y = 113;
        setup.sequence = 0;
        setup.priority = 0;
        setup.bgPriority = 1;
        break;
    case 1:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 128;
        setup.y = 77;
        setup.sequence = 1;
        setup.priority = 0;
        setup.bgPriority = 1;
        break;
    case 2:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 57;
        setup.y = 154;
        setup.sequence = 2;
        setup.priority = 0;
        setup.bgPriority = 1;
        affineMode = 1;
        break;
    case 3:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 200;
        setup.y = 38;
        setup.sequence = 3;
        setup.priority = 0;
        setup.bgPriority = 1;
        affineMode = 1;
        break;
    case 4:
    case 5:
    case 6:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 0;
        setup.y = 0;
        setup.sequence = 7;
        setup.priority = 0;
        setup.bgPriority = 0;
        break;
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
        visible = FALSE;
    case 7:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 200;
        setup.y = 38;
        setup.sequence = 6;
        setup.priority = 0;
        setup.bgPriority = 2;
        break;
    case 8:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 128;
        setup.y = 98;
        setup.sequence = 8;
        setup.priority = 0;
        setup.bgPriority = 1;
        autoAnim = FALSE;
        visible = FALSE;
        break;
    case 9:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 128;
        setup.y = 98;
        setup.sequence = 10;
        setup.priority = 2;
        setup.bgPriority = 1;
        autoAnim = FALSE;
        visible = FALSE;
        break;
    case 10:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 128;
        setup.y = 98;
        setup.sequence = 11;
        setup.priority = 0;
        setup.bgPriority = 0;
        visible = FALSE;
        autoAnim = FALSE;
        break;
    case 12:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 128;
        setup.y = 98;
        setup.sequence = 12;
        setup.priority = 1;
        setup.bgPriority = 1;
        autoAnim = FALSE;
        visible = FALSE;
        break;
    case 22:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 248;
        setup.y = 40;
        setup.sequence = 13;
        setup.priority = 0;
        setup.bgPriority = 0;
        visible = FALSE;
        break;
    case 23:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 248;
        setup.y = 174;
        setup.sequence = 14;
        setup.priority = 0;
        setup.bgPriority = 0;
        visible = FALSE;
        break;
    case 24:
        surface = CLACT_SURFACE_MAIN;
        setup.x = 128;
        setup.y = 96;
        setup.sequence = 15;
        setup.priority = 0;
        setup.bgPriority = 0;
        visible = FALSE;
        break;
    default:
        return NULL;
    }
    clact->actors[id] = func_0204c040(clact->unit, clact->resources[1], clact->resources[0], clact->resources[2],
                                      &setup, surface, heapId);
    func_0204c520(clact->actors[id], autoAnim);
    func_0204c124(clact->actors[id], visible);
    func_0204c244(clact->actors[id], affineMode);
    return clact->actors[id];
}

void KeySystemClAct_Delete(KeySystemClAct *clact, u32 id) {
    if (clact->actors[id] != NULL) {
        func_0204c108(clact->actors[id]);
        clact->actors[id] = NULL;
    }
}

ClActor *KeySystemClAct_GetActor(KeySystemClAct *clact, u32 id) {
    return clact->actors[id];
}

u32 KeySystemClAct_GetResource(KeySystemClAct *clact, u32 id) {
    return clact->resources[id];
}

ClActUnit *KeySystemClAct_GetUnit(KeySystemClAct *clact) {
    return clact->unit;
}

static void KeySystemTags_Init(KeySystemTag *tags, HeapID heapId) {
    sys_memset(tags, 0, sizeof(KeySystemTag) * KEY_SYSTEM_TAG_COUNT);
}

void KeySystemTags_Set(KeySystemTag *tags, const char *tag, u32 value) {
    int i;

    for (i = 0; i < KEY_SYSTEM_TAG_COUNT; i++) {
        if (tags[i].tag[0] == '\0') {
            sys_memcpy(tag, tags[i].tag, sizeof(tags[i].tag));
            tags[i].value = value;
            return;
        }
    }
}

void KeySystemTags_Remove(KeySystemTag *tags, const char *tag) {
    int i;

    for (i = 0; i < KEY_SYSTEM_TAG_COUNT; i++) {
        if (_STD_CompareNString(tags[i].tag, tag) == 0) {
            sys_memset(tags[i].tag, 0, sizeof(tags[i].tag));
            tags[i].value = 0;
            return;
        }
    }
}

BOOL KeySystemTags_Has(KeySystemTag *tags, const char *tag) {
    int i;

    for (i = 0; i < KEY_SYSTEM_TAG_COUNT; i++) {
        if (_STD_CompareNString(tags[i].tag, tag) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

u32 KeySystemTags_Get(KeySystemTag *tags, const char *tag) {
    int i;

    for (i = 0; i < KEY_SYSTEM_TAG_COUNT; i++) {
        if (_STD_CompareNString(tags[i].tag, tag) == 0) {
            return tags[i].value;
        }
    }
    return 0;
}

static void KeySystemTags_Clear(KeySystemTag *tags) {
    int i;

    for (i = 0; i < KEY_SYSTEM_TAG_COUNT; i++) {
        sys_memset(tags[i].tag, 0, sizeof(tags[i].tag));
        tags[i].value = 0;
    }
}

void KeySystem_Setup(KeySystemWork *wk, HeapID heapId) {
    void *data;
    SaveControl *save;

    GFL_OvlLoad(OVERLAY_ID(331));
    GFL_OvlLoad(OVERLAY_ID(139));
    wk->ov331Work = SaveOutside_Load(heapId);
    data = SaveOutside_GetKeyData(wk->ov331Work);
    save = GameData_GetSaveControl(wk->param->gameData);
    wk->memoryLink = func_02010470(data);
    if (SaveControl_IsDataAlreadyPresent(save)) {
        wk->keyInfo = getKeyInfoSaveBlk(save);
    } else {
        wk->keyInfo = func_0201046c(data);
    }
    wk->graphic = KeySystemGraphic_Create(1, heapId);
    wk->font = GFL_FontCreate(ARCID_FONT, 0, 0, FALSE, heapId);
    wk->printQueue = func_02021998(heapId);
    wk->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_2, heapId);
    wk->wordSet = GFL_WordSetSystemCreateDefault(heapId);
    wk->strBuf = GFL_StrBufCreate(128, heapId);
    wk->bg = KeySystemBG_Create(heapId);
    KeySystemBG_Load(wk->bg, heapId);
    KeySystemClAct_Init(&wk->clact, wk->graphic, heapId);
    KeySystemClAct_LoadResources(&wk->clact, heapId);
    wk->scene = KeySystemScene_Create(wk, heapId);
    KeySystemFlow_Init(wk, HEAPID_KEY_SYSTEM);
    DataConvert_Init(wk, HEAPID_KEY_SYSTEM);
    CygnusFlow_Init(wk, HEAPID_KEY_SYSTEM);
    KeySystem_CreateDefaultTitleWin(wk, heapId);
    if (wk->preloadedSeqs == 0) {
        wk->preloadedSeqs = func_02005af4(sPreloadedSeqs, NELEMS(sPreloadedSeqs));
        GFL_SndBGMPlay(SEQ_BGM_DATA_CONV, 0xffff);
    }
}

void KeySystem_Teardown(KeySystemWork *wk, BOOL keepSounds) {
    if (!keepSounds) {
        func_02005d8c();
        func_02005b60(wk->preloadedSeqs);
        wk->preloadedSeqs = 0;
    }
    CygnusFlow_Exit(wk);
    KeySystemFlow_Exit(wk);
    KeySystemScene_Abort(wk->scene);
    if (wk->titleWin != NULL) {
        KeySystem_FreeTitleWin(wk);
    }
    if (wk->infoWin != NULL) {
        KeySystemMsgWin_Free(wk->infoWin);
        wk->infoWin = NULL;
    }
    if (wk->msgWin != NULL) {
        KeySystemMsgWin_Free(wk->msgWin);
        wk->msgWin = NULL;
    }
    KeySystemScene_Free(wk->scene);
    wk->scene = NULL;
    KeySystemBG_Free(wk->bg);
    wk->bg = NULL;
    KeySystemClAct_FreeResources(&wk->clact);
    KeySystemGraphic_Free(wk->graphic);
    wk->graphic = NULL;
    SaveOutside_Free(wk->ov331Work);
    wk->ov331Work = NULL;
    GFL_StrBufFree(wk->strBuf);
    wk->strBuf = NULL;
    GFL_WordSetSystemFree(wk->wordSet);
    wk->wordSet = NULL;
    GFL_MsgDataFree(wk->msgData);
    wk->msgData = NULL;
    func_02021a18(wk->printQueue);
    wk->printQueue = NULL;
    GFL_FontFree(wk->font);
    wk->font = NULL;
    GFL_OvlUnload(OVERLAY_ID(139));
    GFL_OvlUnload(OVERLAY_ID(331));
}

void KeySystem_CreateMsgWin(KeySystemWork *wk, HeapID heapId) {
    KeySystem_CreateMsgWinOn(wk, 0, heapId);
}

void KeySystem_FreeMsgWin(KeySystemWork *wk) {
    KeySystem_FreeMsgWinOn(wk, 0);
}

void KeySystem_CreateMsgWinOn(KeySystemWork *wk, u8 bg, HeapID heapId) {
    wk->msgWin = KeySystemMsgWin_Create(bg, 1, 19, 30, 4, 14, wk->font, heapId);
    KeySystemMsgWin_SetColor(wk->msgWin, PRINT_COLOR(1, 2, 15));
    KeySystemMsgWin_Clear(wk->msgWin);
    KeySystemMsgWin_DrawFrame(wk->msgWin, 1, 15);
}

void KeySystem_FreeMsgWinOn(KeySystemWork *wk, u32 bg) {
    KeySystemMsgWin_Free(wk->msgWin);
    wk->msgWin = NULL;
    GFL_BGSysQueueScrLoad(bg);
}

static void KeySystem_CreateDefaultTitleWin(KeySystemWork *wk, HeapID heapId) {
    KeySystem_CreateTitleWin(wk, 14, heapId);
}

void KeySystem_FreeTitleWin(KeySystemWork *wk) {
    KeySystemMsgWin_Clear(wk->titleWin);
    KeySystemMsgWin_Free(wk->titleWin);
    wk->titleWin = NULL;
    GFL_BGSysQueueScrLoad(0);
}

void KeySystem_CreateTitleWin(KeySystemWork *wk, u8 width, HeapID heapId) {
    wk->titleWin = KeySystemMsgWin_Create(0, 0, 1, width, 2, 14, wk->font, heapId);
    KeySystemMsgWin_SetColor(wk->titleWin, PRINT_COLOR(1, 2, 0));
    KeySystemMsgWin_SetPos(wk->titleWin, 0, 0, KEY_SYSTEM_ALIGN_CENTER);
}

void KeySystem_CreateInfoWin(KeySystemWork *wk, HeapID heapId) {
    wk->infoWin = KeySystemMsgWin_Create(4, 2, 2, 28, 20, 14, wk->font, heapId);
    KeySystemMsgWin_SetColor(wk->infoWin, PRINT_COLOR(1, 2, 15));
    KeySystemMsgWin_DrawFrame(wk->infoWin, 1, 15);
}

void KeySystem_FreeInfoWin(KeySystemWork *wk) {
    KeySystemMsgWin_Free(wk->infoWin);
    wk->infoWin = NULL;
    GFL_BGSysQueueScrLoad(4);
}

void KeySystem_CreateYesNoMenu(KeySystemWork *wk, HeapID heapId) {
    KeySystemMenuSetup setup;

    sys_memset(&setup, 0, sizeof(KeySystemMenuSetup));
    setup.msgData = wk->msgData;
    setup.font = wk->font;
    setup.printQueue = wk->printQueue;
    setup.bg = 0;
    setup.palette = 14;
    setup.framePalette = 15;
    setup.frameChar = 1;
    setup.msgIds[0] = 65;
    setup.msgIds[1] = 66;
    setup.count = 2;
    setup.cancelable = TRUE;
    setup.cancelValue = 1;
    setup.cursor = 0;
    wk->menu = KeySystemMenu_Create(&setup, heapId);
}

void KeySystem_FreeMenu(KeySystemWork *wk) {
    KeySystemMenu_Free(wk->menu);
    wk->menu = NULL;
}

void KeySystem_SeqTop(KeySystemSeq *seq, int *state, void *work) {
    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, KeySystem_SeqStartTopScene);
        (*state)++;
        break;
    case 1:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeIn);
        (*state)++;
        break;
    case 2:
        KeySystemSeq_Push(seq, KeySystem_SeqTopMenu);
        (*state)++;
        break;
    case 3:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeOutWhite);
        (*state)++;
        break;
    case 4:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 5:
        KeySystemSeq_Reset(seq);
        break;
    }
}

static void KeySystem_SeqSaveCorrupted(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, KeySystem_SeqStartMsgScene);
        (*state)++;
        break;
    case 1:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeIn);
        (*state)++;
        break;
    case 2:
        KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 161, KEY_SYSTEM_MSG_STREAM);
        (*state)++;
        break;
    case 3:
        if (KeySystemMsgWin_IsDone(wk->msgWin)) {
            (*state)++;
        }
        break;
    case 4:
        KeySystemSeq_Push(seq, KeySystem_SeqFadeOutWhite);
        (*state)++;
        break;
    case 5:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 6:
        KeySystemSeq_Reset(seq);
        break;
    }
}

static void KeySystem_SeqTopMenu(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemSeq_Push(seq, KeySystem_SeqStartTopScene);
        (*state)++;
        break;
    case 1:
        KeySystemList_Update(wk->list);
        if (KeySystemList_IsChanged(wk->list) && KeySystemMsgWin_IsDone(wk->infoWin)) {
            KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemList_GetCursor(wk->list) + 72,
                                     KEY_SYSTEM_MSG_PRINT);
        }
        if (KeySystemList_IsDecided(wk->list)) {
            wk->choice = KeySystemList_GetCursor(wk->list);
            if (wk->choice == TOP_MENU_BACK) {
                *state = 4;
                return;
            }
            KeySystemTags_Set(wk->tags, "MAN", KeySystemList_GetCursor(wk->list));
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Push(seq, KeySystem_SeqEndScene);
        (*state)++;
        break;
    case 3:
        switch (wk->choice) {
        case TOP_MENU_KEY_SYSTEM:
            KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_1_TO_0, 30);
            KeySystemSeq_Push(seq, KeySystemFlow_SeqMenu);
            break;
        case TOP_MENU_MEMORY_LINK:
            KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_2_TO_0, 30);
            KeySystemSeq_Push(seq, DataConvert_SeqMenu);
            break;
        case TOP_MENU_DREAM_RADAR:
            KeySystemBG_StartFade(wk->bg, KEY_SYSTEM_BG_FADE_3_TO_0, 30);
            KeySystemSeq_Push(seq, CygnusFlow_SeqMenu);
            break;
        }
        *state = 0;
        break;
    case 4:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystem_SeqStartTopScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sTopSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case 1:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystem_SeqStartMsgScene(KeySystemSeq *seq, int *state, void *work) {
    KeySystemWork *wk = work;

    switch (*state) {
    case 0:
        KeySystemScene_Start(wk->scene, &sMsgSceneFuncs, HEAPID_KEY_SYSTEM);
        (*state)++;
        break;
    case 1:
        if (KeySystemScene_IsIdle(wk->scene)) {
            (*state)++;
        }
        break;
    case 2:
        KeySystemSeq_Pop(seq);
        break;
    }
}

static void KeySystemTopScene_Init(void *work, HeapID heapId) {
    KeySystemWork *wk = work;
    KeySystemListSetup setup;
    u32 i;

    if (wk->titleWin == NULL) {
        KeySystem_CreateDefaultTitleWin(wk, heapId);
    }
    sys_memset(&setup, 0, sizeof(KeySystemListSetup));
    setup.bg = 1;
    setup.unk04 = 14;
    setup.frameChar = 10;
    setup.palette = 1;
    setup.msgData = wk->msgData;
    setup.font = wk->font;
    setup.count = 4;
    if (KeySystemTags_Has(wk->tags, "MAN")) {
        setup.cursor = KeySystemTags_Get(wk->tags, "MAN");
        KeySystemTags_Remove(wk->tags, "MAN");
    } else {
        setup.cursor = 0;
    }
    for (i = 0; i < setup.count; i++) {
        setup.items[i].width = 26;
        setup.items[i].height = 2;
        setup.items[i].x = 3;
        setup.items[i].y = (setup.items[i].height + 3) * i + 5;
    }
    setup.items[0].msgId = 69;
    setup.items[1].msgId = 70;
    setup.items[2].msgId = 162;
    setup.items[3].msgId = 71;
    wk->list = KeySystemList_Create(&setup, heapId);
    KeySystemMsgWin_PrintMsg(wk->titleWin, wk->msgData, 84, KEY_SYSTEM_MSG_PRINT);
    KeySystem_CreateInfoWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->infoWin, wk->msgData, KeySystemList_GetCursor(wk->list) + 72, KEY_SYSTEM_MSG_PRINT);
    KeySystemBG_LoadScreen(wk->bg, 2, 0);
}

static BOOL KeySystemTopScene_Main(void *work) {
    KeySystemWork *wk = work;
    BOOL titleDone = KeySystemMsgWin_IsDone(wk->titleWin);
    BOOL infoDone = KeySystemMsgWin_IsDone(wk->infoWin);
    BOOL listDone = KeySystemList_IsPrinted(wk->list);

    if (infoDone && listDone && titleDone) {
        GFL_BGSysQueueScrLoad(0);
        GFL_BGSysQueueScrLoad(2);
        GFL_BGSysQueueScrLoad(1);
        GFL_BGSysQueueScrLoad(4);
        return TRUE;
    }
    return FALSE;
}

static void KeySystemTopScene_Exit(void *work) {
    KeySystemWork *wk = work;

    KeySystem_FreeInfoWin(wk);
    KeySystemList_Free(wk->list);
    wk->list = NULL;
    GFL_BGSysQueueScrLoad(1);
    GFL_BGSysQueueScrLoad(4);
}

static void KeySystemMsgScene_Init(void *work, HeapID heapId) {
    KeySystemWork *wk = work;

    KeySystem_CreateMsgWin(wk, heapId);
    KeySystemMsgWin_PrintMsg(wk->msgWin, wk->msgData, 0, KEY_SYSTEM_MSG_PRINT);
    KeySystemMsgWin_PrintMsg(wk->titleWin, wk->msgData, 84, KEY_SYSTEM_MSG_PRINT);
    KeySystemBG_LoadScreen(wk->bg, 2, 0);
}

static BOOL KeySystemMsgScene_Main(void *work) {
    KeySystemWork *wk = work;
    BOOL titleDone = KeySystemMsgWin_IsDone(wk->titleWin);

    if (KeySystemMsgWin_IsDone(wk->msgWin) && titleDone) {
        GFL_BGSysQueueScrLoad(2);
        GFL_BGSysQueueScrLoad(0);
        return TRUE;
    }
    return FALSE;
}

static void KeySystemMsgScene_Exit(void *work) {
    KeySystem_FreeMsgWin(work);
}
