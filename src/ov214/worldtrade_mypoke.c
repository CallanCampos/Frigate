#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "pml/item.h"
#include "pml/poke_graphic.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "save/worldtrade_data.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "worldtrade_local.h"

// The Global Trade Station's screen of the Pokémon the player deposited, where they can take it back. The file's name
// is a guess, as the ROM doesn't give it, and so are the names of its functions

enum {
    MYPOKE_SEQ_START,
    MYPOKE_SEQ_MAIN,
    MYPOKE_SEQ_END,
    MYPOKE_SEQ_MESSAGE_WAIT,
    MYPOKE_SEQ_MESSAGE_1MIN_WAIT,
    MYPOKE_SEQ_YESNO,
    MYPOKE_SEQ_YESNO_SELECT,
    MYPOKE_SEQ_SELECT_LIST,
    MYPOKE_SEQ_SELECT_WAIT,
};

// How many windows of information the screen has
#define MYPOKE_INFOWIN_NUM 14

// The upload screen's mode that takes the Pokémon back
#define UPLOAD_MODE_DOWNLOAD 8

typedef struct {
    int x;
    int y;
    int w;
    int h;
} MyPokeWinPos;

static void MyPoke_BgInit(void);
static void MyPoke_BgExit(void);
static void MyPoke_BgGraphicSet(WorldTradeWork *wk);
static void MyPoke_SetCellActor(WorldTradeWork *wk);
static void MyPoke_DelCellActor(WorldTradeWork *wk);
static void MyPoke_BmpWinInit(WorldTradeWork *wk);
static void MyPoke_BmpWinDelete(WorldTradeWork *wk);
static void MyPoke_InitWork(WorldTradeWork *wk);
static void MyPoke_FreeWork(WorldTradeWork *wk);
static int MyPoke_SubSeqStart(WorldTradeWork *wk);
static int MyPoke_SubSeqMain(WorldTradeWork *wk);
static int MyPoke_SubSeqEnd(WorldTradeWork *wk);
static int MyPoke_SubSeqYesNo(WorldTradeWork *wk);
static int MyPoke_SubSeqYesNoSelect(WorldTradeWork *wk);
static int MyPoke_SubSeqSelectList(WorldTradeWork *wk);
static int MyPoke_SubSeqSelectWait(WorldTradeWork *wk);
static int MyPoke_SubSeqMessageWait(WorldTradeWork *wk);
static int MyPoke_SubSeqMessage1MinWait(WorldTradeWork *wk);
static void MyPoke_SubSeqMessagePrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat, PartyPkm *pkm);
static u16 MyPoke_SexMarkColor(int sex);
static void MyPoke_WantPokePrintReWrite(WorldTradeWork *wk);

static int (*sMyPokeSubSeqTable[])(WorldTradeWork *wk) = {
    MyPoke_SubSeqStart,           MyPoke_SubSeqMain,  MyPoke_SubSeqEnd,         MyPoke_SubSeqMessageWait,
    MyPoke_SubSeqMessage1MinWait, MyPoke_SubSeqYesNo, MyPoke_SubSeqYesNoSelect, MyPoke_SubSeqSelectList,
    MyPoke_SubSeqSelectWait,
};

int WorldTrade_MyPoke_Init(WorldTradeWork *wk, int seq) {
    PartyPkm *pkm = WorldTradeData_GetPkmPtr(wk->param->worldtrade_data);

    MyPoke_InitWork(wk);
    MyPoke_BgInit();
    WorldTrade_SubLcdBgInit(wk, 0, 0);
    MyPoke_BgGraphicSet(wk);
    MyPoke_BmpWinInit(wk);
    MyPoke_SetCellActor(wk);

    WorldTrade_MyPokeInfoPrint(wk->msgManager, wk->monsNameManager, wk->wordSet, &wk->infoWin[0],
                               PokeParty_GetBoxPkm(pkm), &wk->uploadPokemonData.postSimple, &wk->print);
    WorldTrade_PokeInfoPrint2(wk->msgManager, &wk->infoWin[7], GetPlayerName(wk->param->mystatus), pkm,
                              &wk->infoWin[12], &wk->print);
    WorldTrade_MyPokeWantPrint(wk->msgManager, wk->monsNameManager, wk->wordSet, &wk->infoWin[9],
                               wk->uploadPokemonData.wantSimple.characterNo, wk->uploadPokemonData.wantSimple.gender,
                               WorldTrade_LevelTermGet(wk->uploadPokemonData.wantSimple.level_min,
                                                       wk->uploadPokemonData.wantSimple.level_max,
                                                       LEVEL_PRINT_TBL_DEPOSIT),
                               &wk->print);
    WorldTrade_TransPokeGraphic(pkm);

    GFL_WipeSet(3, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
    wk->subprocessSeq = MYPOKE_SEQ_START;
    wk->subLcdBgKeep = 0;
    return WT_SEQ_FADEIN;
}

int WorldTrade_MyPoke_Main(WorldTradeWork *wk, int seq) {
    return sMyPokeSubSeqTable[wk->subprocessSeq](wk);
}

int WorldTrade_MyPoke_End(WorldTradeWork *wk, int seq) {
    MyPoke_DelCellActor(wk);
    MyPoke_FreeWork(wk);
    MyPoke_BmpWinDelete(wk);
    MyPoke_BgExit();
    WorldTrade_SubLcdBgExit(wk);
    ClActor_SetVisible(wk->promptDsAct, FALSE);
    WorldTrade_SubProcessUpdate(wk);
    return WT_SEQ_INIT;
}

static void MyPoke_BgInit(void) {
    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };
        u32 enabled = GFL_BGSysGetEnabledBGsB();

        GFL_BGSysSetLCDConfig(&config);
        GFL_BGSysSetEnabledBGsB(enabled);
    }
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_BG1 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ, TRUE);
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(2);
        GFL_BGSysSetBGEnabled(2, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(1, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(3);
        GFL_BGSysSetBGEnabled(3, TRUE);
    }
    GFL_BGSysClearCharCore(2, 32, 0, HEAPID_WORLDTRADE);
    GFL_BGSysClearCharCore(3, 32, 0, HEAPID_WORLDTRADE);
}

static void MyPoke_BgExit(void) {
    GFL_BGSysReleaseBG(0);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(3);
}

static void MyPoke_BgGraphicSet(WorldTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WORLDTRADE, HEAPID_WORLDTRADE);

    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 0, 0, 0x60, HEAPID_WORLDTRADE);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1a0, 0x20, HEAPID_WORLDTRADE);
    LoadSysMsgBox(2, 1, 14, 0, HEAPID_WORLDTRADE);
    LoadSysMsgBox(2, 31, 11, 0, HEAPID_WORLDTRADE);
    GFL_BGSysLoadArcNCGRStatic(arc, 16, 1, 0, 0, TRUE, HEAPID_WORLDTRADE);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 22, 1, 0, 0x600, TRUE, HEAPID_WORLDTRADE);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x20, 0x20, HEAPID_WORLDTRADE);
    WorldTrade_SubLcdWinGraphicSet(wk);
    GFL_ArcToolFree(arc);
}

static void MyPoke_SetCellActor(WorldTradeWork *wk) {
    ClActorSetup setup;

    MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
    setup.x = 208;
    setup.y = 58;
    wk->pokemonAct = ClActor_Create(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CHAR],
                                    wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                                    wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
    ClActor_SetAutoAnim(wk->pokemonAct, TRUE);
    ClActor_SetSequence(wk->pokemonAct, 36);
    ClActor_SetBgPriority(wk->pokemonAct, 1);
    ClActor_SetVisible(wk->pokemonAct, TRUE);
    GFL_NetSetIconScreen(FALSE, HEAPID_WORLDTRADE);
}

static void MyPoke_DelCellActor(WorldTradeWork *wk) {
    ClActor_Delete(wk->pokemonAct);
}

// The windows of the Pokémon's information
static const MyPokeWinPos sMyPokeInfoWinPos[MYPOKE_INFOWIN_NUM] = {
    { 2, 1, 12, 2 },  { 7, 4, 9, 2 },    { 11, 1, 4, 2 },  { 14, 1, 4, 2 },  { 1, 10, 6, 2 },
    { 7, 10, 13, 2 }, { 1, 4, 6, 2 },    { 1, 13, 10, 2 }, { 12, 13, 8, 2 }, { 1, 16, 12, 2 },
    { 2, 18, 10, 2 }, { 13, 18, 12, 2 }, { 1, 7, 5, 2 },   { 7, 7, 9, 2 },
};

static void MyPoke_BmpWinInit(WorldTradeWork *wk) {
    BmpWin *win;
    int i;

    wk->msgWin = BmpWin_CreateDynamic(2, 2, 21, 27, 2, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWin), 0);
    win = wk->msgWin;
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));

    wk->menuWin[0] = BmpWin_CreateDynamic(2, 21, 15, 10, 4, 13, TRUE);
    win = wk->menuWin[0];
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));

    for (i = 0; i < MYPOKE_INFOWIN_NUM; i++) {
        wk->infoWin[i] = BmpWin_CreateDynamic(3, sMyPokeInfoWinPos[i].x, sMyPokeInfoWinPos[i].y, sMyPokeInfoWinPos[i].w,
                                              sMyPokeInfoWinPos[i].h, 13, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->infoWin[i]), 0);
        win = wk->infoWin[i];
        BmpWin_FlushChar(win);
        BmpWin_FlushMap(win);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
    }

    WorldTrade_SubLcdExplainPut(wk, 2);
}

static void MyPoke_BmpWinDelete(WorldTradeWork *wk) {
    int i;

    WorldTrade_PrintClear(&wk->print);
    BmpWin_Free(wk->explainWin);
    BmpWin_Free(wk->msgWin);
    BmpWin_Free(wk->menuWin[0]);
    for (i = 0; i < MYPOKE_INFOWIN_NUM; i++) {
        BmpWin_Free(wk->infoWin[i]);
    }
}

static void MyPoke_InitWork(WorldTradeWork *wk) {
    int i;

    wk->talkString = GFL_StrBufCreate(180, HEAPID_WORLDTRADE);
    wk->titleString = GFL_MsgDataLoadStrbufNew(wk->msgManager, 0x2d);
    for (i = 0; i < 10; i++) {
        wk->infoString[i] = GFL_StrBufCreate(20, HEAPID_WORLDTRADE);
    }
}

static void MyPoke_FreeWork(WorldTradeWork *wk) {
    int i;

    for (i = 0; i < 10; i++) {
        GFL_StrBufFree(wk->infoString[i]);
    }
    GFL_StrBufFree(wk->talkString);
    GFL_StrBufFree(wk->titleString);
}

static int MyPoke_SubSeqStart(WorldTradeWork *wk) {
    wk->subprocessSeq = MYPOKE_SEQ_MAIN;
    return WT_SEQ_MAIN;
}

static int MyPoke_SubSeqMain(WorldTradeWork *wk) {
    PartyPkm *pkm = WorldTradeData_GetPkmPtr(wk->param->worldtrade_data);

    if ((GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) || GFL_HIDIsTouchPressed()) {
        MyPoke_SubSeqMessagePrint(wk, 5, 1, 0, 0xf0f, pkm);
        WorldTrade_SetNextSeq(wk, MYPOKE_SEQ_MESSAGE_WAIT, MYPOKE_SEQ_SELECT_LIST);
        GFL_SndSEPlay(0x54c);
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        wk->subprocessSeq = MYPOKE_SEQ_END;
        WorldTrade_SubProcessChange(wk, WORLDTRADE_TITLE, 0);
        GFL_SndSEPlay(0x551);
    }
    return WT_SEQ_MAIN;
}

static int MyPoke_SubSeqEnd(WorldTradeWork *wk) {
    GFL_WipeSet(3, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
    wk->subprocessSeq = MYPOKE_SEQ_START;
    return WT_SEQ_FADEOUT;
}

static int MyPoke_SubSeqYesNo(WorldTradeWork *wk) {
    WorldTrade_TouchWinYesNoMakeEx(wk, 20, 0x16a, 8, 2, TRUE);
    WorldTrade_ClearPassive();
    WorldTrade_SetPassiveKeepBG2(TRUE);
    wk->subprocessSeq = MYPOKE_SEQ_YESNO_SELECT;
    return WT_SEQ_MAIN;
}

static int MyPoke_SubSeqYesNoSelect(WorldTradeWork *wk) {
    u32 ret = WorldTrade_TouchSwMain(wk);

    if (ret == 1) {
        WorldTrade_TouchWinYesNoDel(wk);
        wk->subprocessSeq = MYPOKE_SEQ_END;
        wk->subOutFlag = 1;
        WorldTrade_SubProcessChange(wk, WORLDTRADE_UPLOAD, UPLOAD_MODE_DOWNLOAD);
        MyPoke_WantPokePrintReWrite(wk);
    } else if (ret == 2) {
        WorldTrade_TouchWinYesNoDel(wk);
        wk->subprocessSeq = MYPOKE_SEQ_START;
        BmpWin_ClearFrame(wk->msgWin, 0);
        MyPoke_WantPokePrintReWrite(wk);
    }
    return WT_SEQ_MAIN;
}

static int MyPoke_SubSeqSelectList(WorldTradeWork *wk) {
    wk->menuList = ListMenuCore_CreateOptionList(2, HEAPID_WORLDTRADE);
    ListMenuCore_AppendMsgOption(wk->menuList, wk->msgManager, 0x3d, 1, HEAPID_WORLDTRADE);
    ListMenuCore_AppendMsgOption(wk->menuList, wk->msgManager, 0x3e, 2, HEAPID_WORLDTRADE);
    WorldTrade_SelBoxInit(wk, 2, 2, 20);
    WorldTrade_SetPassiveKeepBG2(TRUE);
    wk->subprocessSeq = MYPOKE_SEQ_SELECT_WAIT;
    return WT_SEQ_MAIN;
}

static int MyPoke_SubSeqSelectWait(WorldTradeWork *wk) {
    PartyPkm *pkm;
    int ret = WorldTrade_SelBoxMain(wk);

    if (ret == 1) {
        WorldTrade_SelBoxEnd(wk);
        ListMenuCore_FreeOptionList(wk->menuList);
        pkm = WorldTradeData_GetPkmPtr(wk->param->worldtrade_data);
        // A Pokémon with mail goes to the party, which must have room
        if (WorldTrade_PokemonMailCheck(pkm) && PokeParty_GetPkmCount(wk->param->myparty) == 6) {
            MyPoke_SubSeqMessagePrint(wk, 0x2a, 1, 0, 0xf0f, pkm);
            WorldTrade_SetNextSeq(wk, MYPOKE_SEQ_MESSAGE_WAIT, MYPOKE_SEQ_MAIN);
            return WT_SEQ_MAIN;
        }
        MyPoke_SubSeqMessagePrint(wk, 6, 1, 0, 0xf0f, pkm);
        WorldTrade_SetNextSeq(wk, MYPOKE_SEQ_MESSAGE_WAIT, MYPOKE_SEQ_YESNO);
        MyPoke_WantPokePrintReWrite(wk);
    } else if (ret == 2 || ret == BMPMENULIST_CANCEL) {
        WorldTrade_SelBoxEnd(wk);
        ListMenuCore_FreeOptionList(wk->menuList);
        wk->subprocessSeq = MYPOKE_SEQ_END;
        WorldTrade_SubProcessChange(wk, WORLDTRADE_TITLE, 0);
        MyPoke_WantPokePrintReWrite(wk);
    }
    return WT_SEQ_MAIN;
}

static int MyPoke_SubSeqMessageWait(WorldTradeWork *wk) {
    if (!WorldTrade_PrintIsBusy(&wk->print)) {
        wk->subprocessSeq = wk->subprocessNextSeq;
    }
    return WT_SEQ_MAIN;
}

static int MyPoke_SubSeqMessage1MinWait(WorldTradeWork *wk) {
    if (!WorldTrade_PrintIsBusy(&wk->print)) {
        wk->wait++;
        if (wk->wait > 45) {
            wk->wait = 0;
            wk->subprocessSeq = wk->subprocessNextSeq;
        }
    }
    return WT_SEQ_MAIN;
}

static void MyPoke_SubSeqMessagePrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat, PartyPkm *pkm) {
    StrBuf *str;
    BmpWin *win;

    setPartyPokemonSpeciesNameToStrbuf(wk->wordSet, 0, pkm);
    str = GFL_MsgDataLoadStrbufNew(wk->msgManager, msgNo);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->talkString, str);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWin), 15);
    BmpWin_DrawFrame(wk->msgWin, 0, 1, 14);
    win = wk->msgWin;
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
    WorldTrade_Print(wk->msgWin, 0, wk->talkString, 0, 0, &wk->print);
    GFL_StrBufFree(str);
}

// The color of the gender mark
static u16 MyPoke_SexMarkColor(int sex) {
    switch (sex) {
    case 0:
        return 0;
    case 1:
        return 0x14c0;
    case 2:
        return 0xc80;
    }
    return 0;
}

void WorldTrade_MyPokeInfoPrint(MsgData *msgManager, MsgData *monsNameManager, WordSet *wordSet, BmpWin **win,
                                BoxPkm *pkm, Dpw_Tr_PokemonDataSimple *post, WorldTradePrint *print) {
    StrBuf *strbuf;
    StrBuf *sexbuf;
    StrBuf *levelLabel;
    StrBuf *levelbuf;
    StrBuf *itemLabel;
    StrBuf *nameLabel;
    StrBuf *namebuf = GFL_StrBufCreate(26, HEAPID_WORLDTRADE);
    StrBuf *itembuf = GFL_StrBufCreate(38, HEAPID_WORLDTRADE);
    int sex;
    int level;
    int itemNo;
    int i;
    int monsno;
    int showSex;

    PML_PkmGetParam(pkm, PKM_PARAM_NICKNAME, namebuf);
    monsno = post->characterNo;
    sex = post->gender;
    level = post->level;
    itemNo = PML_PkmGetParam(pkm, PKM_PARAM_ITEM, NULL);
    showSex = PML_PkmGetParam(pkm, 0xad, NULL);

    itemLabel = GFL_MsgDataLoadStrbufNew(msgManager, 0x4f);
    sexbuf = GFL_MsgDataLoadStrbufNew(msgManager, WorldTrade_SexStringTable[sex]);
    levelLabel = GFL_MsgDataLoadStrbufNew(msgManager, 0x70);
    WordSetNumber(wordSet, 3, level, 3, 0, TRUE);
    levelbuf = WorldTrade_ExpandMessage(wordSet, msgManager, 0x71, HEAPID_WORLDTRADE);
    strbuf = GFL_MsgDataLoadStrbufNew(monsNameManager, monsno);
    setItemNameToStrbuf(itembuf, itemNo, HEAPID_WORLDTRADE);
    nameLabel = GFL_MsgDataLoadStrbufNew(msgManager, 0x41);

    for (i = 0; i < 6; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(win[i]), 0);
    }

    WorldTrade_SysPrint(win[0], namebuf, 0, 0, 0, 0x440, print);
    if (sex != 3 && showSex) {
        WorldTrade_SysPrint(win[0], sexbuf, 64, 0, 0, MyPoke_SexMarkColor(sex), print);
    }
    WorldTrade_SysPrint(win[1], strbuf, 7, 0, 0, 0x440, print);
    WorldTrade_SysPrint(win[2], levelLabel, 0, 0, 0, 0x440, print);
    WorldTrade_SysPrint(win[3], levelbuf, 0, 0, 0, 0x440, print);
    WorldTrade_SysPrint(win[4], itemLabel, 0, 0, 0, 0x3c40, print);
    WorldTrade_SysPrint(win[5], itembuf, 7, 0, 0, 0x440, print);
    WorldTrade_SysPrint(win[6], nameLabel, 0, 0, 0, 0x3c40, print);

    GFL_StrBufFree(itemLabel);
    GFL_StrBufFree(itembuf);
    GFL_StrBufFree(levelLabel);
    GFL_StrBufFree(levelbuf);
    GFL_StrBufFree(sexbuf);
    GFL_StrBufFree(namebuf);
    GFL_StrBufFree(strbuf);
    GFL_StrBufFree(nameLabel);
}

void WorldTrade_PokeInfoPrint2(MsgData *msgManager, BmpWin **win, u16 *name, PartyPkm *pkm, BmpWin **oyaWin,
                               WorldTradePrint *print) {
    StrBuf *ownerbuf;
    StrBuf *ownerLabel;
    StrBuf *oyaLabel;
    StrBuf *oyabuf;

    ownerbuf = GFL_StrBufCreate(34, HEAPID_WORLDTRADE);
    oyabuf = GFL_StrBufCreate(34, HEAPID_WORLDTRADE);
    ownerLabel = GFL_MsgDataLoadStrbufNew(msgManager, 0x37);
    GFL_StrBufLoadString(ownerbuf, name);
    oyaLabel = GFL_MsgDataLoadStrbufNew(msgManager, 0xc2);
    PokeParty_GetParam(pkm, PKM_PARAM_OT_NAME, oyabuf);

    WorldTrade_SysPrint(win[0], ownerLabel, 0, 0, 0, 0x3c40, print);
    WorldTrade_SysPrint(win[1], ownerbuf, 0, 0, 0, 0x440, print);
    WorldTrade_SysPrint(oyaWin[0], oyaLabel, 0, 0, 0, 0x3c40, print);
    WorldTrade_SysPrint(oyaWin[1], oyabuf, 7, 0, 0, 0x440, print);

    GFL_StrBufFree(ownerLabel);
    GFL_StrBufFree(ownerbuf);
    GFL_StrBufFree(oyaLabel);
    GFL_StrBufFree(oyabuf);
}

void WorldTrade_TransPokeGraphic(PartyPkm *pkm) {
    NNSG2dCharacterData *charData;
    void *buf = LoadSingleCellSpindaGraphicsByBoxData(&charData, PokeParty_GetBoxPkm(pkm), 0, HEAPID_WORLDTRADE);
    u32 arcId;
    u32 palette;

    DC_FlushRange(charData->rawData, 0x1200);
    GX_LoadOBJ(charData->rawData, 0x3f80, 0x1200);
    GFL_HeapFree(buf);

    arcId = GetPokemonGraphicsARCID();
    palette = GetPokemonPaletteDataNo(arcId, PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL),
                                      PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL),
                                      PokeParty_GetParam(pkm, PKM_PARAM_SEX, NULL), PokeParty_IsRare(pkm), 0, 0);
    GFL_BGSysLoadNCLRDefault(GetPokemonGraphicsARCID(), palette, 1, 0x1a0, 0x20, HEAPID_WORLDTRADE);
}

static void MyPoke_WantPokePrintReWrite(WorldTradeWork *wk) {
    WorldTrade_MyPokeWantPrint(wk->msgManager, wk->monsNameManager, wk->wordSet, &wk->infoWin[9],
                               wk->uploadPokemonData.wantSimple.characterNo, wk->uploadPokemonData.wantSimple.gender,
                               WorldTrade_LevelTermGet(wk->uploadPokemonData.wantSimple.level_min,
                                                       wk->uploadPokemonData.wantSimple.level_max,
                                                       LEVEL_PRINT_TBL_DEPOSIT),
                               &wk->print);
}
