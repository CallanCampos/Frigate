#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "dpw/dpw_tr.h"
#include "gfl/nhttp_rap.h"
#include "dwc/dwc.h"
#include "field/unity_tower.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/random.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/rtc.h"
#include "pml/item.h"
#include "pml/mail.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/chatter.h"
#include "save/join_avenue.h"
#include "save/medal_box.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/wifi_list.h"
#include "save/worldtrade_data.h"
#include "system/bmp_winframe.h"
#include "system/country_region.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "worldtrade_local.h"

// The Global Trade Station's server exchanges: depositing a Pokémon, taking it back, trading for a Pokémon found by a
// search, checking whether the deposited Pokémon was traded, and the saves after each. The file's name is a guess, as
// the ROM doesn't give it, and so are the names of its functions

// The steps of the screen
enum {
    UPLOAD_SEQ_START,
    UPLOAD_SEQ_MAIN,
    UPLOAD_SEQ_EVIL_CHECK_START,
    UPLOAD_SEQ_EVIL_CHECK_RESULT,
    UPLOAD_SEQ_NAME_CHECK_START,
    UPLOAD_SEQ_NAME_CHECK_WAIT,
    UPLOAD_SEQ_UPLOAD_START,
    UPLOAD_SEQ_UPLOAD_RESULT,
    UPLOAD_SEQ_UPLOAD_FINISH,
    UPLOAD_SEQ_UPLOAD_FINISH_RESULT,
    UPLOAD_SEQ_UPLOAD_SUCCESS_MESSAGE,
    UPLOAD_SEQ_DOWNLOAD_START,
    UPLOAD_SEQ_DOWNLOAD_RESULT,
    UPLOAD_SEQ_DOWNLOAD_FINISH,
    UPLOAD_SEQ_DOWNLOAD_FINISH_RESULT,
    UPLOAD_SEQ_DOWNLOAD_SUCCESS_MESSAGE,
    UPLOAD_SEQ_EXCHANGE_START,
    UPLOAD_SEQ_EXCHANGE_RESULT,
    UPLOAD_SEQ_EXCHANGE_FINISH,
    UPLOAD_SEQ_EXCHANGE_FINISH_RESULT,
    UPLOAD_SEQ_EXCHANGE_SUCCESS_MESSAGE,
    UPLOAD_SEQ_EXCHANGE_FAILED_MESSAGE,
    UPLOAD_SEQ_DOWNLOAD_EX_START,
    UPLOAD_SEQ_DOWNLOAD_EX_FINISH,
    UPLOAD_SEQ_DOWNLOAD_EX_FINISH_RESULT,
    UPLOAD_SEQ_DOWNLOAD_EX_SUCCESS_MESSAGE,
    UPLOAD_SEQ_SERVER_POKE_DELETE,
    UPLOAD_SEQ_SERVER_POKE_DELETE_WAIT,
    UPLOAD_SEQ_SERVER_TRADE_CHECK,
    UPLOAD_SEQ_SERVER_TRADE_CHECK_RESULT,
    UPLOAD_SEQ_SERVER_DOWNLOAD,
    UPLOAD_SEQ_SERVER_DOWNLOAD_RESULT,
    UPLOAD_SEQ_SERVER_TRADE_CHECK_END,
    UPLOAD_SEQ_NOW_SAVE_MESSAGE,
    UPLOAD_SEQ_SAVE,
    UPLOAD_SEQ_SAVE_RANDOM_WAIT,
    UPLOAD_SEQ_SAVE_WAIT,
    UPLOAD_SEQ_SAVE_LAST,
    UPLOAD_SEQ_TIMEOUT_SAVE,
    UPLOAD_SEQ_TIMEOUT_SAVE_WAIT,
    UPLOAD_SEQ_END,
    UPLOAD_SEQ_MES_WAIT,
    UPLOAD_SEQ_MES_WAIT_BUTTON,
    UPLOAD_SEQ_ERROR_MESSAGE,
    UPLOAD_SEQ_ERROR_END,
    UPLOAD_SEQ_RETURN_TITLE_MESSAGE,
    UPLOAD_SEQ_CANCEL,
    UPLOAD_SEQ_CANCEL_WAIT,
    UPLOAD_SEQ_SERVER_SERVICE_END,
};

// What the screen was started for
#define UPLOAD_MODE_UPLOAD 7
#define UPLOAD_MODE_DOWNLOAD 8
#define UPLOAD_MODE_EXCHANGE 9
#define UPLOAD_MODE_DOWNLOAD_EX 10
#define UPLOAD_MODE_SERVER_CHECK 11
#define UPLOAD_MODE_POKEMON_EVO_SAVE 12

// How the trade demo is started
#define DEMO_MODE_UPLOAD 7
#define DEMO_MODE_DOWNLOAD 8
#define DEMO_MODE_EXCHANGE 9
#define DEMO_MODE_DOWNLOAD_EX 10

// How the connection screen is started to log in again
#define ENTER_MODE_RELOGIN 0x15

// The versions of the other color, whose trades earn a medal, and the name that replaces a bad one
#ifdef BLACK2
#define OTHER_COLOR VERSION_WHITE
#define OTHER_COLOR_2 VERSION_WHITE2
#define DEFAULT_NAME_MSG 0x19
#else
#define OTHER_COLOR VERSION_BLACK
#define OTHER_COLOR_2 VERSION_BLACK2
#define DEFAULT_NAME_MSG 0x18
#endif

// Frames to wait for the server
#define UPLOAD_TIMEOUT (30 * 60 * 2)

// The HTTP statuses of the server's check
#define HTTP_STATUS_OK 200
#define HTTP_STATUS_BAD_REQUEST 400
#define HTTP_STATUS_UNAUTHORIZED 401
#define HTTP_STATUS_REQUEST_TIMEOUT 408


static void Upload_BgInit(void);
static void Upload_BgExit(void);
static void Upload_BgGraphicSet(WorldTradeWork *wk);
static void Upload_BmpWinInit(WorldTradeWork *wk);
static void Upload_BmpWinDelete(WorldTradeWork *wk);
static void Upload_InitWork(WorldTradeWork *wk);
static void Upload_FreeWork(WorldTradeWork *wk);
static int Upload_SubSeqStart(WorldTradeWork *wk);
static int Upload_SubSeqEvilCheckStart(WorldTradeWork *wk);
static int Upload_SubSeqEvilCheckResult(WorldTradeWork *wk);
static int Upload_SubSeqNameCheckStart(WorldTradeWork *wk);
static int Upload_SubSeqNameCheckWait(WorldTradeWork *wk);
static int Upload_SubSeqUploadStart(WorldTradeWork *wk);
static int Upload_SubSeqUploadResult(WorldTradeWork *wk);
static int Upload_SubSeqUploadFinish(WorldTradeWork *wk);
static int Upload_SubSeqUploadFinishResult(WorldTradeWork *wk);
static int Upload_SubSeqDownloadStart(WorldTradeWork *wk);
static int Upload_SubSeqDownloadResult(WorldTradeWork *wk);
static int Upload_SubSeqDownloadFinish(WorldTradeWork *wk);
static int Upload_SubSeqDownloadFinishResult(WorldTradeWork *wk);
static int Upload_SubSeqExchangeStart(WorldTradeWork *wk);
static int Upload_SubSeqExchangeResult(WorldTradeWork *wk);
static int Upload_SubSeqExchangeFinish(WorldTradeWork *wk);
static int Upload_SubSeqExchangeFinishResult(WorldTradeWork *wk);
static int Upload_SubSeqServerTradeCheck(WorldTradeWork *wk);
static int Upload_SubSeqServerTradeCheckResult(WorldTradeWork *wk);
static int Upload_SubSeqServerTradeCheckEnd(WorldTradeWork *wk);
static int Upload_SubSeqServerDownload(WorldTradeWork *wk);
static int Upload_SubSeqServerDownloadResult(WorldTradeWork *wk);
static void Upload_ReturnProcess(WorldTradeWork *wk);
static void Upload_MakeTradeExchangeInfo(WorldTradeWork *wk, PartyPkm *received, Dpw_Tr_Data *trData, PartyPkm *sent);
static int Upload_SubSeqCancel(WorldTradeWork *wk);
static int Upload_SubSeqCancelWait(WorldTradeWork *wk);
static int Upload_SubSeqServerServiceEnd(WorldTradeWork *wk);
static int Upload_SubSeqDownloadExStart(WorldTradeWork *wk);
static int Upload_SubSeqDownloadExFinish(WorldTradeWork *wk);
static int Upload_SubSeqDownloadExFinishResult(WorldTradeWork *wk);
static int Upload_SubSeqMain(WorldTradeWork *wk);
static int Upload_SubSeqUploadSuccessMessage(WorldTradeWork *wk);
static int Upload_SubSeqDownloadSuccessMessage(WorldTradeWork *wk);
static int Upload_SubSeqExchangeSuccessMessage(WorldTradeWork *wk);
static int Upload_SubSeqDownloadExSuccessMessage(WorldTradeWork *wk);
static int Upload_SubSeqServerPokeDelete(WorldTradeWork *wk);
static int Upload_SubSeqServerPokeDeleteWait(WorldTradeWork *wk);
static int Upload_SubSeqExchangeFailedMessage(WorldTradeWork *wk);
static void Upload_PrintError(WorldTradeWork *wk);
static int Upload_SubSeqErrorMessage(WorldTradeWork *wk);
static int Upload_SubSeqErrorEnd(WorldTradeWork *wk);
static int Upload_SubSeqReturnTitleMessage(WorldTradeWork *wk);
static int Upload_SubSeqNowSaveMessage(WorldTradeWork *wk);
static int Upload_SubSeqSave(WorldTradeWork *wk);
static int Upload_SubSeqSaveRandomWait(WorldTradeWork *wk);
static int Upload_SubSeqSaveWait(WorldTradeWork *wk);
static int Upload_SubSeqSaveLast(WorldTradeWork *wk);
static int Upload_SubSeqTimeoutSave(WorldTradeWork *wk);
static int Upload_SubSeqTimeoutSaveWait(WorldTradeWork *wk);
static int Upload_SubSeqEnd(WorldTradeWork *wk);
static int Upload_SubSeqMessageWait(WorldTradeWork *wk);
static int Upload_SubSeqMessageWaitButton(WorldTradeWork *wk);
static void Upload_UploadPokemonDataDelete(WorldTradeWork *wk, BOOL keep);
static void Upload_DownloadPokemonDataAdd(WorldTradeWork *wk, PartyPkm *pkm, int boxNo, BOOL traded);
static void Upload_ExchangePokemonDataAdd(WorldTradeWork *wk, PartyPkm *pkm, Dpw_Tr_Data *upload, Dpw_Tr_Data *exchange,
                                          int boxNo);
static void Upload_TradeDateUpDate(WorldTradeData *data, int type);
static void Upload_WifiHistoryDataSet(UnityTowerSurveySave *save, Dpw_Tr_Data *trData);
static int Upload_MyPokemonPocketFullCheck(WorldTradeWork *wk, Dpw_Tr_Data *trData);
static void Upload_SetSaveNextSequence(WorldTradeWork *wk, u16 nextSeq1st, u16 nextSeq2nd);
static BOOL Upload_DuplicateCheck(WorldTradeWork *wk);
static void Upload_ReplaceBadName(PartyPkm *pkm, HeapID heapId);

static int (*sUploadSubSeqTable[])(WorldTradeWork *wk) = {
    Upload_SubSeqStart,
    Upload_SubSeqMain,
    Upload_SubSeqEvilCheckStart,
    Upload_SubSeqEvilCheckResult,
    Upload_SubSeqNameCheckStart,
    Upload_SubSeqNameCheckWait,
    Upload_SubSeqUploadStart,
    Upload_SubSeqUploadResult,
    Upload_SubSeqUploadFinish,
    Upload_SubSeqUploadFinishResult,
    Upload_SubSeqUploadSuccessMessage,
    Upload_SubSeqDownloadStart,
    Upload_SubSeqDownloadResult,
    Upload_SubSeqDownloadFinish,
    Upload_SubSeqDownloadFinishResult,
    Upload_SubSeqDownloadSuccessMessage,
    Upload_SubSeqExchangeStart,
    Upload_SubSeqExchangeResult,
    Upload_SubSeqExchangeFinish,
    Upload_SubSeqExchangeFinishResult,
    Upload_SubSeqExchangeSuccessMessage,
    Upload_SubSeqExchangeFailedMessage,
    Upload_SubSeqDownloadExStart,
    Upload_SubSeqDownloadExFinish,
    Upload_SubSeqDownloadExFinishResult,
    Upload_SubSeqDownloadExSuccessMessage,
    Upload_SubSeqServerPokeDelete,
    Upload_SubSeqServerPokeDeleteWait,
    Upload_SubSeqServerTradeCheck,
    Upload_SubSeqServerTradeCheckResult,
    Upload_SubSeqServerDownload,
    Upload_SubSeqServerDownloadResult,
    Upload_SubSeqServerTradeCheckEnd,
    Upload_SubSeqNowSaveMessage,
    Upload_SubSeqSave,
    Upload_SubSeqSaveRandomWait,
    Upload_SubSeqSaveWait,
    Upload_SubSeqSaveLast,
    Upload_SubSeqTimeoutSave,
    Upload_SubSeqTimeoutSaveWait,
    Upload_SubSeqEnd,
    Upload_SubSeqMessageWait,
    Upload_SubSeqMessageWaitButton,
    Upload_SubSeqErrorMessage,
    Upload_SubSeqErrorEnd,
    Upload_SubSeqReturnTitleMessage,
    Upload_SubSeqCancel,
    Upload_SubSeqCancelWait,
    Upload_SubSeqServerServiceEnd,
};

int WorldTrade_Upload_Init(WorldTradeWork *wk, int seq) {
    Upload_InitWork(wk);
    Upload_BgInit();
    WorldTrade_SubLcdBgInit(wk, 0, FALSE);
    if (wk->subProcessMode == UPLOAD_MODE_SERVER_CHECK) {
        GFL_BGSysSetBGEnabled(4, FALSE);
        GFL_BGSysSetBGEnabled(6, FALSE);
    }
    Upload_BgGraphicSet(wk);
    Upload_BmpWinInit(wk);
    WorldTrade_SetPartnerExchangePos(wk);
    GFL_WipeSet(3, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
    WorldTrade_WifiIconAdd(wk);
    wk->subprocessSeq = UPLOAD_SEQ_START;
    wk->subLcdBgKeep = 0;
    return WT_SEQ_FADEIN;
}

int WorldTrade_Upload_Main(WorldTradeWork *wk, int seq) {
    return sUploadSubSeqTable[wk->subprocessSeq](wk);
}

int WorldTrade_Upload_End(WorldTradeWork *wk, int seq) {
    if (gfxRegGetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR) != 0) {
        WorldTrade_SetPartnerExchangePosIsReturns(wk);
    }
    Upload_FreeWork(wk);
    Upload_BmpWinDelete(wk);
    Upload_BgExit();
    WorldTrade_SubLcdBgExit(wk);
    WorldTrade_SubProcessUpdate(wk);
    return WT_SEQ_INIT;
}

static void Upload_BgInit(void) {
    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
        u32 enabled = GFL_BGSysGetEnabledBGsB();

        GFL_BGSysSetLCDConfig(&config);
        GFL_BGSysSetEnabledBGsB(enabled);
    }
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
        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(0);
        GFL_BGSysSetBGEnabled(0, TRUE);
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
        GFL_BGSysClearScr(1);
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
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(2);
        GFL_BGSysSetBGEnabled(2, TRUE);
    }
    GFL_BGSysClearCharCore(0, 32, 0, HEAPID_WORLDTRADE);
    GFL_BGSysClearCharCore(1, 32, 0, HEAPID_WORLDTRADE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
}

static void Upload_BgExit(void) {
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
}

static void Upload_BgGraphicSet(WorldTradeWork *wk) {
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1a0, 0x20, HEAPID_WORLDTRADE);
    LoadSysMsgBox(0, 1, 14, 0, HEAPID_WORLDTRADE);
    LoadSysMsgBox(0, 31, 11, 0, HEAPID_WORLDTRADE);
    // The trade room only shows once the player walked in
    if (wk->demoEnd == 0) {
        GFL_BGSysSetBGEnabled(4, FALSE);
        GFL_BGSysSetBGEnabled(5, FALSE);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
    }
    if (wk->oldSubProcess == WORLDTRADE_DEMO) {
        WorldTrade_SubLcdBgGraphicSet(wk);
        WorldTrade_SubLcdWinGraphicSet(wk);
    }
}

static void Upload_BmpWinInit(WorldTradeWork *wk) {
    BmpWin *win;

    wk->msgWin = BmpWin_CreateDynamic(0, 2, 19, 27, 4, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWin), 0);
    win = wk->msgWin;
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
}

static void Upload_BmpWinDelete(WorldTradeWork *wk) {
    WorldTrade_PrintClear(&wk->print);
    BmpWin_Free(wk->msgWin);
}

static void Upload_InitWork(WorldTradeWork *wk) {
    wk->talkString = GFL_StrBufCreate(180, HEAPID_WORLDTRADE);
}

static void Upload_FreeWork(WorldTradeWork *wk) {
    GFL_StrBufFree(wk->talkString);
}

static int Upload_SubSeqStart(WorldTradeWork *wk) {
    switch (wk->subProcessMode) {
    case UPLOAD_MODE_UPLOAD:
        Enter_MessagePrintNoStream(wk, wk->msgManager, 0x18, 1, 0xf0f);
        WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT, UPLOAD_SEQ_EVIL_CHECK_START);
        wk->evilCheckRetry = 0;
        wk->nameCheckNextSeq = UPLOAD_SEQ_UPLOAD_START;
        break;
    case UPLOAD_MODE_DOWNLOAD:
        Enter_MessagePrintNoStream(wk, wk->msgManager, 0x18, 1, 0xf0f);
        WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT, UPLOAD_SEQ_DOWNLOAD_START);
        break;
    case UPLOAD_MODE_EXCHANGE:
        Enter_MessagePrintNoStream(wk, wk->msgManager, 0x18, 1, 0xf0f);
        WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT, UPLOAD_SEQ_EVIL_CHECK_START);
        wk->evilCheckRetry = 0;
        wk->nameCheckNextSeq = UPLOAD_SEQ_EXCHANGE_START;
        break;
    case UPLOAD_MODE_DOWNLOAD_EX:
        Enter_MessagePrintNoStream(wk, wk->msgManager, 0x18, 1, 0xf0f);
        WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT, UPLOAD_SEQ_DOWNLOAD_EX_START);
        wk->subOutFlag = 1;
        break;
    case UPLOAD_MODE_SERVER_CHECK:
        Enter_MessagePrintNoStream(wk, wk->msgManager, 0x18, 1, 0xf0f);
        WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT, UPLOAD_SEQ_SERVER_TRADE_CHECK);
        break;
    case UPLOAD_MODE_POKEMON_EVO_SAVE:
        Enter_MessagePrintNoStream(wk, wk->msgManager, 0x9a, 1, 0xf0f);
        wk->subprocessSeq = UPLOAD_SEQ_NOW_SAVE_MESSAGE;
        wk->subNextProcess = WORLDTRADE_TITLE;
        func_02042ba8(TRUE, HEAPID_WORLDTRADE);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }
    WorldTrade_TimeIconAdd(wk);
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqEvilCheckStart(WorldTradeWork *wk) {
    PartyPkm *pkm = (PartyPkm *)wk->uploadPokemonData.postData;
    BOOL ret;

    // Keep the Pokémon as it was before the check, for the trade demo
    copyPartyPkm(pkm, wk->demoPokemon);
    wk->uploadPokemonData.countryCode = Country_GetValidCountry(wk->uploadPokemonData.countryCode,
                                                      wk->uploadPokemonData.localCode, wk->uploadPokemonData.langCode);
    wk->uploadPokemonData.localCode = Country_GetValidRegion(wk->uploadPokemonData.countryCode, wk->uploadPokemonData.localCode,
                                                    wk->uploadPokemonData.langCode);
    wk->evilCheck = NHttpRap_Create(HEAPID_WORLDTRADE, func_02008bdc(wk->param->mystatus), wk->wifiLoginBuffer);
    NHttpRap_BeginPost(wk->evilCheck, HEAPID_WORLDTRADE, PokeParty_GetPkmRawSize(), 1);
    NHttpRap_AddPostData(wk->evilCheck, pkm, PokeParty_GetPkmRawSize());
    ret = NHttpRap_SendValidate(wk->evilCheck);
    GFL_ASSERT(ret);
    ret = NHttpRap_StartRequest(wk->evilCheck) == 0;
    GFL_ASSERT(ret);
    wk->subprocessSeq = UPLOAD_SEQ_EVIL_CHECK_RESULT;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqEvilCheckResult(WorldTradeWork *wk) {
    int status = NHttpRap_GetStatus(wk->evilCheck);
    int ret = NHttpRap_Poll(wk->evilCheck);

    if (ret == 0) {
        void *body = NHttpRap_GetAnswer(wk->evilCheck);

        switch (status) {
        case HTTP_STATUS_BAD_REQUEST:
            func_020120f0(0x41);
            GFL_NetErrShow(0);
            NHttpRap_EndRequest(wk->evilCheck);
            NHttpRap_FreePostData(wk->evilCheck);
            NHttpRap_Destroy(wk->evilCheck);
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_END;
            return WT_SEQ_MAIN;
        case HTTP_STATUS_UNAUTHORIZED:
            func_020120f0(0x42);
            GFL_NetErrShow(0);
            NHttpRap_EndRequest(wk->evilCheck);
            NHttpRap_FreePostData(wk->evilCheck);
            NHttpRap_Destroy(wk->evilCheck);
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_END;
            return WT_SEQ_MAIN;
        case HTTP_STATUS_REQUEST_TIMEOUT:
            func_020120f0(0x44);
            GFL_NetErrShow(0);
            NHttpRap_EndRequest(wk->evilCheck);
            NHttpRap_FreePostData(wk->evilCheck);
            NHttpRap_Destroy(wk->evilCheck);
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_END;
            return WT_SEQ_MAIN;
        default:
            func_020120f0(0x43);
            GFL_NetErrShow(0);
            NHttpRap_EndRequest(wk->evilCheck);
            NHttpRap_FreePostData(wk->evilCheck);
            NHttpRap_Destroy(wk->evilCheck);
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_END;
            return WT_SEQ_MAIN;
        case HTTP_STATUS_OK:
            wk->evilCheckStatus = NHttpRap_GetCheckStatus(body);
            wk->evilCheckResult = NHttpRap_GetCheckResult(body, 0);
            sys_memcpy(NHttpRap_GetCheckSignature(body, 1), wk->evilCheckSign, sizeof(wk->evilCheckSign));
            NHttpRap_FreePostData(wk->evilCheck);
            NHttpRap_Destroy(wk->evilCheck);
            if (wk->evilCheckStatus == 0) {
                if (wk->evilCheckResult == 0) {
                    wk->subprocessSeq = UPLOAD_SEQ_NAME_CHECK_START;
                } else if (wk->evilCheckRetry == 0) {
                    // Check once more with the trainer's name replaced
                    wk->evilCheckRetry++;
                    wk->subprocessSeq = UPLOAD_SEQ_EVIL_CHECK_START;
                    Upload_ReplaceBadName((PartyPkm *)wk->uploadPokemonData.postData, HEAPID_WORLDTRADE);
                } else {
                    wk->connectErrorNo = -7;
                    wk->subprocessSeq = UPLOAD_SEQ_RETURN_TITLE_MESSAGE;
                }
            } else if (wk->evilCheckRetry == 0) {
                wk->evilCheckRetry++;
                wk->subprocessSeq = UPLOAD_SEQ_EVIL_CHECK_START;
                Upload_ReplaceBadName((PartyPkm *)wk->uploadPokemonData.postData, HEAPID_WORLDTRADE);
            } else {
                wk->connectErrorNo = -7;
                wk->subprocessSeq = UPLOAD_SEQ_RETURN_TITLE_MESSAGE;
            }
            break;
        }
    } else if (ret != 15) {
        wk->connectErrorNo = -15;
        wk->subprocessSeq = UPLOAD_SEQ_ERROR_MESSAGE;
        NHttpRap_FreePostData(wk->evilCheck);
        NHttpRap_Destroy(wk->evilCheck);
        func_02012154();
        func_020424e4();
        func_02012144();
        func_02042478();
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqNameCheckStart(WorldTradeWork *wk) {
    StrBuf *name = GFL_StrBufCreate(8, HEAPID_WORLDTRADE);
    const u16 *str;
    u32 i;

    GFL_StrBufLoadFixedString(name, wk->uploadPokemonData.name, 8);
    sys_memset(&wk->nameCheck, 0, sizeof(WorldTradeNameCheck));
    wk->nameCheck.unk24 = HEAPID_WORLDTRADE;
    str = GFL_StrBufGetStringPtr(name);
    for (i = 0; i < (u32)GFL_StrBufGetCharCount(name) + 1; i++) {
        u16 c = str[i];

        if (c == GFL_StrBufGetTerminator()) {
            wk->nameCheck.name[i] = 0;
        } else {
            wk->nameCheck.name[i] = c;
        }
    }
    wk->nameCheck.words = wk->nameCheck.name;
    wk->nameCheck.badWordCount = 0;
    func_ov011_0216bea4(&wk->nameCheck.words, 1, NULL, 0, wk->nameCheck.result, &wk->nameCheck.badWordCount, 0x80);
    GFL_StrBufFree(name);
    wk->subprocessSeq = UPLOAD_SEQ_NAME_CHECK_WAIT;
    wk->timeoutCount = 0;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqNameCheckWait(WorldTradeWork *wk) {
    BOOL done = func_ov011_0216bed4() == 2;
    BOOL bad;

    if (done) {
        bad = wk->nameCheck.badWordCount > 0;
    }
    if (done) {
        if (bad) {
            // A bad name is replaced with the version's default
            u16 *dest = wk->uploadPokemonData.name;
            MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_BTL_MAIN, HEAPID_WORLDTRADE);
            StrBuf *str = GFL_MsgDataLoadStrbufNew(msgData, DEFAULT_NAME_MSG);
            const u16 *src;
            int i;

            GFL_MsgDataFree(msgData);
            src = GFL_StrBufGetStringPtr(str);
            for (i = 0; i < 7 && i < (u32)GFL_StrBufGetCharCount(str); i++) {
                dest[i] = src[i];
            }
            dest[i] = GFL_StrBufGetTerminator();
            GFL_StrBufFree(str);
        }
        wk->subprocessSeq = wk->nameCheckNextSeq;
    } else if (wk->timeoutCount++ == UPLOAD_TIMEOUT) {
        wk->connectErrorNo = -5;
        wk->subprocessSeq = UPLOAD_SEQ_ERROR_MESSAGE;
        func_02012154();
        func_020424e4();
        func_02012144();
        func_02042478();
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqUploadStart(WorldTradeWork *wk) {
    func_ov189_021a779c(&wk->uploadPokemonData, wk->evilCheckSign, sizeof(wk->evilCheckSign));
    wk->subprocessSeq = UPLOAD_SEQ_UPLOAD_RESULT;
    wk->timeoutCount = 0;
    Upload_SetSaveNextSequence(wk, UPLOAD_SEQ_UPLOAD_FINISH, UPLOAD_SEQ_UPLOAD_SUCCESS_MESSAGE);
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqUploadResult(WorldTradeWork *wk) {
    if (func_ov189_021a7750()) {
        s32 result = func_ov189_021a778c();

        wk->timeoutCount = 0;
        switch (result) {
        case 0:
            Upload_UploadPokemonDataDelete(wk, TRUE);
            RecordAddOne(wk->param->record, 0x1c);
            wk->subprocessSeq = UPLOAD_SEQ_SAVE;
            break;
        case -1:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_MESSAGE;
            func_02012154();
            func_020424e4();
            func_02012144();
            func_02042478();
            break;
        case -5:
        case -6:
        case -7:
        case -8:
        case -9:
        case -10:
        case -11:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_RETURN_TITLE_MESSAGE;
            break;
        case -4:
        case -12:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_RETURN_TITLE_MESSAGE;
            break;
        case -2:
        case -14:
        case -15:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_MESSAGE;
            func_02012154();
            func_020424e4();
            func_02012144();
            func_02042478();
            break;
        case -13:
            WorldTrade_ShowFatalError(wk);
            break;
        }
    } else if (++wk->timeoutCount == UPLOAD_TIMEOUT) {
        WorldTrade_ShowFatalError(wk);
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqUploadFinish(WorldTradeWork *wk) {
    func_ov189_021a7854();
    wk->subprocessSeq = UPLOAD_SEQ_UPLOAD_FINISH_RESULT;
    wk->timeoutCount = 0;
    wk->depositFlag = 1;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqUploadFinishResult(WorldTradeWork *wk) {
    if (func_ov189_021a7750()) {
        s32 result = func_ov189_021a778c();

        wk->timeoutCount = 0;
        switch (result) {
        case 0:
            wk->serverWaitTime = 0;
            wk->subprocessSeq = UPLOAD_SEQ_SAVE_LAST;
            break;
        case -4:
        case -12:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_RETURN_TITLE_MESSAGE;
            break;
        case -1:
        case -2:
        case -3:
        case -5:
        case -14:
        case -15:
            WorldTrade_ShowFatalError(wk);
            break;
        case -13:
            WorldTrade_ShowFatalError(wk);
            break;
        }
    } else if (++wk->timeoutCount == UPLOAD_TIMEOUT) {
        WorldTrade_ShowFatalError(wk);
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqDownloadStart(WorldTradeWork *wk) {
    func_ov189_021a78e0(&wk->uploadPokemonData);
    wk->subprocessSeq = UPLOAD_SEQ_DOWNLOAD_RESULT;
    wk->timeoutCount = 0;
    Upload_SetSaveNextSequence(wk, UPLOAD_SEQ_DOWNLOAD_FINISH, UPLOAD_SEQ_DOWNLOAD_SUCCESS_MESSAGE);
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqDownloadResult(WorldTradeWork *wk) {
    if (func_ov189_021a7750()) {
        s32 result = func_ov189_021a778c();

        wk->timeoutCount = 0;
        switch (result) {
        case 0:
            if (wk->uploadPokemonData.isTrade) {
                // Traded in the meantime: take back the Pokémon it was traded for
                wk->subprocessSeq = UPLOAD_SEQ_SERVER_TRADE_CHECK;
            } else {
                Upload_DownloadPokemonDataAdd(wk, func_0200b4d0(wk->param->worldtrade_data),
                                              func_0200b4fc(wk->param->worldtrade_data), wk->uploadPokemonData.isTrade);
                wk->subprocessSeq = UPLOAD_SEQ_SAVE;
            }
            break;
        case -3:
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_MESSAGE;
            func_02012154();
            func_020424e4();
            func_02012144();
            func_02042478();
            break;
        case -4:
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_MESSAGE;
            func_02012154();
            func_020424e4();
            func_02012144();
            func_02042478();
            break;
        case -12:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_RETURN_TITLE_MESSAGE;
            break;
        case -2:
        case -14:
        case -15:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_MESSAGE;
            func_02012154();
            func_020424e4();
            func_02012144();
            func_02042478();
            break;
        case -13:
            WorldTrade_ShowFatalError(wk);
            break;
        }
    } else if (++wk->timeoutCount == UPLOAD_TIMEOUT) {
        WorldTrade_ShowFatalError(wk);
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqDownloadFinish(WorldTradeWork *wk) {
    func_ov189_021a7a5c();
    wk->subprocessSeq = UPLOAD_SEQ_DOWNLOAD_FINISH_RESULT;
    wk->timeoutCount = 0;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqDownloadFinishResult(WorldTradeWork *wk) {
    if (func_ov189_021a7750()) {
        s32 result = func_ov189_021a778c();

        wk->timeoutCount = 0;
        switch (result) {
        case 0:
            wk->subprocessSeq = UPLOAD_SEQ_SAVE_LAST;
            break;
        case -5:
        case -4:
        case -3:
            WorldTrade_ShowFatalError(wk);
            break;
        case -15:
        case -14:
        case -12:
        case -2:
            WorldTrade_ShowFatalError(wk);
            break;
        case -13:
            WorldTrade_ShowFatalError(wk);
            break;
        }
    } else if (++wk->timeoutCount == UPLOAD_TIMEOUT) {
        WorldTrade_ShowFatalError(wk);
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqExchangeStart(WorldTradeWork *wk) {
    func_ov189_021a7d4c(wk->downloadPokemonData[wk->touchTrainerPos].id, &wk->uploadPokemonData,
                        &wk->exchangePokemonData, wk->evilCheckSign, sizeof(wk->evilCheckSign));
    Upload_SetSaveNextSequence(wk, UPLOAD_SEQ_EXCHANGE_FINISH, UPLOAD_SEQ_EXCHANGE_SUCCESS_MESSAGE);
    wk->subprocessSeq = UPLOAD_SEQ_EXCHANGE_RESULT;
    wk->timeoutCount = 0;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqExchangeResult(WorldTradeWork *wk) {
    if (func_ov189_021a7750()) {
        s32 result = func_ov189_021a778c();

        wk->timeoutCount = 0;
        switch (result) {
        case 0:
            wk->subprocessSeq = UPLOAD_SEQ_SAVE;
            Upload_UploadPokemonDataDelete(wk, FALSE);
            Upload_ExchangePokemonDataAdd(wk, (PartyPkm *)wk->exchangePokemonData.postData, &wk->uploadPokemonData,
                                          &wk->exchangePokemonData, wk->boxTrayNo);
            Upload_WifiHistoryDataSet(wk->param->wifihistory, &wk->exchangePokemonData);
            Upload_MakeTradeExchangeInfo(wk, (PartyPkm *)wk->exchangePokemonData.postData, &wk->exchangePokemonData,
                                         (PartyPkm *)wk->uploadPokemonData.postData);
            break;
        case -5:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_EXCHANGE_FAILED_MESSAGE;
            break;
        case -6:
        case -7:
        case -8:
        case -9:
        case -10:
        case -11:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_RETURN_TITLE_MESSAGE;
            break;
        case -12:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_RETURN_TITLE_MESSAGE;
            break;
        case -2:
        case -14:
        case -15:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_MESSAGE;
            func_02012154();
            func_020424e4();
            func_02012144();
            func_02042478();
            break;
        case -13:
            WorldTrade_ShowFatalError(wk);
            break;
        }
    } else if (++wk->timeoutCount == UPLOAD_TIMEOUT) {
        WorldTrade_ShowFatalError(wk);
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqExchangeFinish(WorldTradeWork *wk) {
    func_ov189_021a7df8();
    wk->subprocessSeq = UPLOAD_SEQ_EXCHANGE_FINISH_RESULT;
    wk->timeoutCount = 0;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqExchangeFinishResult(WorldTradeWork *wk) {
    if (func_ov189_021a7750()) {
        s32 result = func_ov189_021a778c();

        wk->timeoutCount = 0;
        switch (result) {
        case 0:
            wk->subprocessSeq = UPLOAD_SEQ_SAVE_LAST;
            break;
        case -5:
            WorldTrade_ShowFatalError(wk);
            break;
        case -15:
        case -14:
        case -12:
        case -2:
            WorldTrade_ShowFatalError(wk);
            break;
        case -13:
            WorldTrade_ShowFatalError(wk);
            break;
        }
    } else if (++wk->timeoutCount == UPLOAD_TIMEOUT) {
        WorldTrade_ShowFatalError(wk);
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqServerTradeCheck(WorldTradeWork *wk) {
    func_ov189_021a7960(&wk->uploadPokemonData);
    wk->subprocessSeq = UPLOAD_SEQ_SERVER_TRADE_CHECK_RESULT;
    wk->timeoutCount = 0;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqServerTradeCheckResult(WorldTradeWork *wk) {
    if (func_ov189_021a7750()) {
        s32 result = func_ov189_021a778c();

        wk->timeoutCount = 0;
        switch (result) {
        case 0:
            // Not traded
            wk->subprocessSeq = UPLOAD_SEQ_SERVER_DOWNLOAD;
            break;
        case 1:
            // Traded: check that the Pokémon received fits
            wk->depositFlag = 1;
            switch (Upload_MyPokemonPocketFullCheck(wk, &wk->uploadPokemonData)) {
            case 1:
                WorldTrade_TimeIconDel(wk);
                Enter_MessagePrintNoStream(wk, wk->msgManager, 0x23, 1, 0xf0f);
                WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT_BUTTON, UPLOAD_SEQ_SERVER_TRADE_CHECK_END);
                break;
            case 2:
                WorldTrade_TimeIconDel(wk);
                Enter_MessagePrintNoStream(wk, wk->msgManager, 0x29, 1, 0xf0f);
                WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT_BUTTON, UPLOAD_SEQ_SERVER_TRADE_CHECK_END);
                break;
            case 0:
                wk->subprocessSeq = UPLOAD_SEQ_DOWNLOAD_EX_START;
                wk->subOutFlag = 1;
                wk->checkEvolution = 1;
                break;
            }
            break;
        case -3:
            // The deposited Pokémon is gone from the server: take back the save's copy
            wk->depositFlag = 0;
            if (func_0200b4a8(wk->param->worldtrade_data)) {
                PartyPkm *pkm = WorldTrade_AllocPartyPkm(HEAPID_WORLDTRADE);

                func_0200b4b8(wk->param->worldtrade_data, pkm);
                loadPokemonNicknameToStrbuf(wk->wordSet, 0, pkm);
                wk->errorMesNo = 2;
                wk->subprocessSeq = UPLOAD_SEQ_TIMEOUT_SAVE;
                Upload_DownloadPokemonDataAdd(wk, pkm, func_0200b4fc(wk->param->worldtrade_data), FALSE);
                func_0200b4b0(wk->param->worldtrade_data, 0);
                GFL_HeapFree(pkm);
            } else {
                Upload_ReturnProcess(wk);
            }
            break;
        case -4:
            wk->depositFlag = 0;
            if (func_0200b4a8(wk->param->worldtrade_data)) {
                PartyPkm *pkm = WorldTrade_AllocPartyPkm(HEAPID_WORLDTRADE);

                func_0200b4b8(wk->param->worldtrade_data, pkm);
                loadPokemonNicknameToStrbuf(wk->wordSet, 0, pkm);
                wk->errorMesNo = 3;
                wk->subprocessSeq = UPLOAD_SEQ_TIMEOUT_SAVE;
                func_0200b4b0(wk->param->worldtrade_data, 0);
                GFL_HeapFree(pkm);
            }
            break;
        case -2:
        case -12:
        case -14:
        case -15:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_MESSAGE;
            func_02012154();
            func_020424e4();
            func_02012144();
            func_02042478();
            break;
        case -13:
            WorldTrade_ShowFatalError(wk);
            break;
        }
    } else if (++wk->timeoutCount == UPLOAD_TIMEOUT) {
        WorldTrade_ShowFatalError(wk);
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqServerTradeCheckEnd(WorldTradeWork *wk) {
    WorldTrade_SubProcessChange(wk, WORLDTRADE_TITLE, 0);
    wk->subprocessSeq = UPLOAD_SEQ_END;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqServerDownload(WorldTradeWork *wk) {
    func_ov189_021a78e0(&wk->uploadPokemonData);
    wk->subprocessSeq = UPLOAD_SEQ_SERVER_DOWNLOAD_RESULT;
    wk->timeoutCount = 0;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqServerDownloadResult(WorldTradeWork *wk) {
    if (func_ov189_021a7750()) {
        s32 result = func_ov189_021a778c();

        wk->timeoutCount = 0;
        switch (result) {
        case 0:
            if (Upload_DuplicateCheck(wk)) {
                // The server still has the Pokémon the save took back: delete it there
                wk->subprocessSeq = UPLOAD_SEQ_SERVER_POKE_DELETE;
                wk->depositFlag = 0;
                return WT_SEQ_MAIN;
            }
            wk->depositFlag = 1;
            break;
        case -3:
            wk->depositFlag = 0;
            break;
        case -2:
        case -12:
        case -14:
        case -15:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_MESSAGE;
            func_02012154();
            func_020424e4();
            func_02012144();
            func_02042478();
            return WT_SEQ_MAIN;
        case -13:
            WorldTrade_ShowFatalError(wk);
            return WT_SEQ_MAIN;
        case -1:
        case -4:
        case -5:
        case -6:
        case -7:
        case -8:
        case -9:
        case -10:
        case -11:
        default:
            break;
        }
        Upload_ReturnProcess(wk);
    } else if (++wk->timeoutCount == UPLOAD_TIMEOUT) {
        WorldTrade_ShowFatalError(wk);
    }
    return WT_SEQ_MAIN;
}

// Goes back to the screen that started the server check
static void Upload_ReturnProcess(WorldTradeWork *wk) {
    switch (wk->subReturnProcess) {
    case WORLDTRADE_TITLE:
        WorldTrade_SubProcessChange(wk, WORLDTRADE_TITLE, 0);
        wk->subprocessSeq = UPLOAD_SEQ_END;
        break;
    case WORLDTRADE_MYPOKE:
        WorldTrade_SubProcessChange(wk, WORLDTRADE_MYPOKE, 3);
        wk->subprocessSeq = UPLOAD_SEQ_END;
        break;
    }
}

static void Upload_MakeTradeExchangeInfo(WorldTradeWork *wk, PartyPkm *received, Dpw_Tr_Data *trData, PartyPkm *sent) {
    PlayerInfo *partner;
    PlayerInfo *visitorInfo;
    GameData *gameData;
    PlayerInfo *avenuePartner;
    JoinAvenueSave *joinAvenue;
    void *entry;
    UnityTowerVisitor visitor;
    BOOL otherColor;

    RecordAddOne(wk->param->record, 0x10);
    func_0200f700(getHollow_RivalData(wk->param->savedata), trData->trainerID);

    partner = WorldTrade_MakePartnerStatus(trData);
    func_0200a504(getPalPadFriendListAddress(wk->param->savedata), partner);
    GFL_HeapFree(partner);

    visitorInfo = WorldTrade_MakePartnerStatus(trData);
    sys_memset(&visitor, 0, sizeof(UnityTowerVisitor));
    func_02008b34(visitorInfo, &visitor.info);
    visitor.sentSpecies =
        sent != NULL ? (u16)PokeParty_GetParam(sent, PKM_PARAM_SPECIES, NULL) : (u16)trData->postSimple.characterNo;
    otherColor = FALSE;
    visitor.receivedSpecies = PokeParty_GetParam(received, PKM_PARAM_SPECIES, NULL);
    visitor.hobby = trData->unk126;
    visitor.unk26_0 = trData->unkF7;
    visitor.unk25 = trData->unk127;
    UnityTowerSurvey_RegisterTrade(wk->param->wifihistory, &visitor);
    GFL_HeapFree(visitorInfo);

    gameData = GSYS_GetGameData(wk->param->gsys);
    avenuePartner = WorldTrade_MakePartnerStatus(trData);
    joinAvenue = SaveControl_GetJoinAvenue(wk->param->savedata);
    entry = func_02037a40(HEAPID_TAIL(HEAPID_WORLDTRADE));
    func_02037ab4(entry, avenuePartner, PokeParty_GetParam(received, PKM_PARAM_SPECIES, NULL), 6);
    func_02010078(joinAvenue, gameData, entry, 2);
    func_02037a68(entry);
    GFL_HeapFree(avenuePartner);

    // A trade with the other color earns a medal
    if (trData->versionCode == OTHER_COLOR || trData->versionCode == OTHER_COLOR_2) {
        otherColor = TRUE;
    }
    if (otherColor) {
        MedalBox_GiveMedal(SaveControl_GetMedalBox(GameData_GetSaveControl(GSYS_GetGameData(wk->param->gsys))), 0xa5);
    }
}

static int Upload_SubSeqCancel(WorldTradeWork *wk) {
    wk->timeoutCount = 0;
    func_ov189_021a7ad4();
    wk->subprocessSeq = UPLOAD_SEQ_CANCEL_WAIT;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqCancelWait(WorldTradeWork *wk) {
    if (func_ov189_021a7750()) {
        switch (func_ov189_021a778c()) {
        case -12:
            wk->subprocessSeq = UPLOAD_SEQ_SERVER_SERVICE_END;
            break;
        case -2:
        case -14:
        case -15:
            wk->subprocessSeq = UPLOAD_SEQ_SERVER_SERVICE_END;
            break;
        case -13:
            WorldTrade_ShowFatalError(wk);
            break;
        }
    } else if (++wk->timeoutCount == UPLOAD_TIMEOUT) {
        wk->subprocessSeq = UPLOAD_SEQ_SERVER_SERVICE_END;
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqServerServiceEnd(WorldTradeWork *wk) {
    func_ov189_021a773c();
    WorldTrade_TimeIconDel(wk);
    GFL_WipeSet(0, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
    wk->subprocessSeq = UPLOAD_SEQ_START;
    return WT_SEQ_FADEOUT;
}

static int Upload_SubSeqDownloadExStart(WorldTradeWork *wk) {
    Upload_DownloadPokemonDataAdd(wk, (PartyPkm *)wk->uploadPokemonData.postData,
                                  func_0200b4fc(wk->param->worldtrade_data), wk->uploadPokemonData.isTrade);
    Upload_WifiHistoryDataSet(wk->param->wifihistory, &wk->uploadPokemonData);
    Upload_MakeTradeExchangeInfo(wk, (PartyPkm *)wk->uploadPokemonData.postData, &wk->uploadPokemonData, NULL);
    func_0200b4b0(wk->param->worldtrade_data, 0);
    wk->subprocessSeq = UPLOAD_SEQ_SAVE;
    Upload_SetSaveNextSequence(wk, UPLOAD_SEQ_DOWNLOAD_EX_FINISH, UPLOAD_SEQ_DOWNLOAD_SUCCESS_MESSAGE);
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqDownloadExFinish(WorldTradeWork *wk) {
    func_ov189_021a79e0();
    wk->subprocessSeq = UPLOAD_SEQ_DOWNLOAD_EX_FINISH_RESULT;
    wk->timeoutCount = 0;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqDownloadExFinishResult(WorldTradeWork *wk) {
    if (func_ov189_021a7750()) {
        s32 result = func_ov189_021a778c();

        wk->timeoutCount = 0;
        switch (result) {
        case 0:
            wk->subprocessSeq = UPLOAD_SEQ_SAVE_LAST;
            break;
        case -3:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_MESSAGE;
            func_02012154();
            func_020424e4();
            func_02012144();
            func_02042478();
            break;
        case -4:
        case -12:
            wk->connectErrorNo = result;
            // fallthrough
        case -2:
        case -14:
        case -15:
            WorldTrade_ShowFatalError(wk);
            break;
        case -13:
            WorldTrade_ShowFatalError(wk);
            break;
        }
    } else if (++wk->timeoutCount == UPLOAD_TIMEOUT) {
        WorldTrade_ShowFatalError(wk);
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqMain(WorldTradeWork *wk) {
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqUploadSuccessMessage(WorldTradeWork *wk) {
    wk->depositFlag = 1;
    WorldTrade_SubProcessChange(wk, WORLDTRADE_DEMO, DEMO_MODE_UPLOAD);
    wk->subprocessSeq = UPLOAD_SEQ_END;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqDownloadSuccessMessage(WorldTradeWork *wk) {
    wk->depositFlag = 0;
    WorldTrade_SubProcessChange(wk, WORLDTRADE_DEMO, DEMO_MODE_DOWNLOAD);
    wk->subprocessSeq = UPLOAD_SEQ_END;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqExchangeSuccessMessage(WorldTradeWork *wk) {
    WorldTrade_SubProcessChange(wk, WORLDTRADE_DEMO, DEMO_MODE_EXCHANGE);
    wk->subprocessSeq = UPLOAD_SEQ_END;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqDownloadExSuccessMessage(WorldTradeWork *wk) {
    wk->depositFlag = 0;
    WorldTrade_SubProcessChange(wk, WORLDTRADE_DEMO, DEMO_MODE_DOWNLOAD_EX);
    wk->subprocessSeq = UPLOAD_SEQ_SAVE;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqServerPokeDelete(WorldTradeWork *wk) {
    func_ov189_021a7a5c();
    wk->subprocessSeq = UPLOAD_SEQ_SERVER_POKE_DELETE_WAIT;
    wk->timeoutCount = 0;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqServerPokeDeleteWait(WorldTradeWork *wk) {
    if (func_ov189_021a7750()) {
        s32 result = func_ov189_021a778c();

        wk->timeoutCount = 0;
        switch (result) {
        case 0:
            Upload_ReturnProcess(wk);
            break;
        case -3:
            Upload_ReturnProcess(wk);
            // fallthrough
        case -4:
            Upload_ReturnProcess(wk);
            // fallthrough
        case -5:
            wk->connectErrorNo = result;
            wk->subprocessSeq = UPLOAD_SEQ_ERROR_MESSAGE;
            func_02012154();
            func_020424e4();
            func_02012144();
            func_02042478();
            break;
        case -2:
        case -12:
        case -14:
        case -15:
            WorldTrade_ShowFatalError(wk);
            break;
        case -13:
            WorldTrade_ShowFatalError(wk);
            break;
        }
    } else if (++wk->timeoutCount == UPLOAD_TIMEOUT) {
        WorldTrade_ShowFatalError(wk);
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqExchangeFailedMessage(WorldTradeWork *wk) {
    Enter_MessagePrintNoStream(wk, wk->msgManager, 0x99, 1, 0xf0f);
    WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT_BUTTON, UPLOAD_SEQ_END);
    WorldTrade_SubProcessChange(wk, WORLDTRADE_TITLE, 0);
    WorldTrade_TimeIconDel(wk);
    WorldTrade_SubLcdMatchObjHide(wk);
    return WT_SEQ_MAIN;
}

static void Upload_PrintError(WorldTradeWork *wk) {
    int msgNo;

    switch (wk->connectErrorNo) {
    case -1:
        msgNo = 0x9d;
        break;
    case -2:
        msgNo = 0xa9;
        break;
    case -3:
        msgNo = 0xa2;
        break;
    case -4:
        msgNo = 0xa4;
        break;
    case -5:
        msgNo = 0xa3;
        break;
    case -6:
        msgNo = 0x1b;
        break;
    case -7:
        msgNo = 0x1c;
        break;
    case -8:
        msgNo = 0x1d;
        break;
    case -9:
        msgNo = 0x1e;
        break;
    case -10:
        msgNo = 0x1f;
        break;
    case -11:
        msgNo = 0x20;
        break;
    case -12:
        msgNo = 0xa0;
        break;
    case -13:
        msgNo = 0xa5;
        break;
    case -14:
        msgNo = 0xaa;
        break;
    case -15:
        msgNo = 0xa1;
        break;
    default:
        msgNo = 0x9f;
        break;
    }
    Enter_MessagePrint(wk, wk->msgManager, msgNo, 1, 0xf0f);
}

static int Upload_SubSeqErrorMessage(WorldTradeWork *wk) {
    Upload_PrintError(wk);
    if (func_ov189_021a7750()) {
        WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT, UPLOAD_SEQ_SERVER_SERVICE_END);
    } else {
        WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT, UPLOAD_SEQ_CANCEL);
    }
    WorldTrade_SubProcessChange(wk, WORLDTRADE_ENTER, ENTER_MODE_RELOGIN);
    WorldTrade_TimeIconDel(wk);
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqErrorEnd(WorldTradeWork *wk) {
    if (func_ov189_021a7750()) {
        wk->subprocessSeq = UPLOAD_SEQ_SERVER_SERVICE_END;
    } else {
        wk->subprocessSeq = UPLOAD_SEQ_CANCEL;
    }
    WorldTrade_SubProcessChange(wk, WORLDTRADE_ENTER, ENTER_MODE_RELOGIN);
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqReturnTitleMessage(WorldTradeWork *wk) {
    Upload_PrintError(wk);
    WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT, UPLOAD_SEQ_END);
    WorldTrade_SubProcessChange(wk, WORLDTRADE_TITLE, 0);
    WorldTrade_TimeIconDel(wk);
    if (wk->searchResult > 0) {
        WorldTrade_SubLcdMatchObjHide(wk);
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqNowSaveMessage(WorldTradeWork *wk) {
    WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT, UPLOAD_SEQ_SAVE);
    Upload_SetSaveNextSequence(wk, UPLOAD_SEQ_SAVE_LAST, UPLOAD_SEQ_END);
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqSave(WorldTradeWork *wk) {
    func_0201782c(GSYS_GetGameData(wk->param->gsys));
    wk->subprocessSeq = UPLOAD_SEQ_SAVE_RANDOM_WAIT;
    wk->wait = GFL_RandomLC(60) + 2;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqSaveRandomWait(WorldTradeWork *wk) {
    if (--wk->wait == 0) {
        wk->subprocessSeq = UPLOAD_SEQ_SAVE_WAIT;
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqSaveWait(WorldTradeWork *wk) {
    if (func_02017850(GSYS_GetGameData(wk->param->gsys)) == 1) {
        wk->subprocessSeq = wk->saveNextSeq1st;
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqSaveLast(WorldTradeWork *wk) {
    if (func_02017850(GSYS_GetGameData(wk->param->gsys)) == 2) {
        wk->subprocessSeq = wk->saveNextSeq2nd;
        WorldTrade_TimeIconDel(wk);
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqTimeoutSave(WorldTradeWork *wk) {
    func_0201782c(GSYS_GetGameData(wk->param->gsys));
    wk->subprocessSeq = UPLOAD_SEQ_TIMEOUT_SAVE_WAIT;
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqTimeoutSaveWait(WorldTradeWork *wk) {
    if (func_02017850(GSYS_GetGameData(wk->param->gsys)) == 2) {
        WorldTrade_SubProcessChange(wk, WORLDTRADE_TITLE, 0);
        WorldTrade_TimeIconDel(wk);
        Enter_MessagePrintNoStream(wk, wk->msgManager, wk->errorMesNo, 1, 0xf0f);
        WorldTrade_SetNextSeq(wk, UPLOAD_SEQ_MES_WAIT_BUTTON, UPLOAD_SEQ_SERVER_TRADE_CHECK_END);
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqEnd(WorldTradeWork *wk) {
    WorldTrade_TimeIconDel(wk);
    if (wk->subOutFlag == 1) {
        if (gfxRegGetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR) == -16) {
            GFL_WipeSet(3, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
        } else {
            GFL_WipeSet(0, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
        }
    } else {
        GFL_WipeSet(3, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
    }
    wk->subprocessSeq = UPLOAD_SEQ_START;
    return WT_SEQ_FADEOUT;
}

static int Upload_SubSeqMessageWait(WorldTradeWork *wk) {
    if (!WorldTrade_PrintIsBusy(&wk->print)) {
        wk->subprocessSeq = wk->subprocessNextSeq;
    }
    return WT_SEQ_MAIN;
}

static int Upload_SubSeqMessageWaitButton(WorldTradeWork *wk) {
    if ((!WorldTrade_PrintIsBusy(&wk->print) && (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A)) || func_0203da48()) {
        GFL_SndSEPlay(SEQ_SE_MESSAGE);
        wk->subprocessSeq = wk->subprocessNextSeq;
    }
    return WT_SEQ_MAIN;
}

// Takes the Pokémon to deposit or trade out of its box or the party, keeping a copy in the save when keep is set
static void Upload_UploadPokemonDataDelete(WorldTradeWork *wk, BOOL keep) {
    if (wk->boxTrayNo != 0xff) {
        PartyPkm *pkm = WorldTrade_AllocPartyPkm(HEAPID_WORLDTRADE);

        WorldTrade_BoxPkmToPartyPkm(BoxSaveAccessor_GetPkm(wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos), pkm);
        if (keep) {
            func_0200b4d4(wk->param->worldtrade_data, pkm, wk->boxTrayNo);
        }
        BoxSaveAccessor_ClearPkm(wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos);
        GFL_HeapFree(pkm);
    } else {
        PartyPkm *pkm = PokeParty_GetPkm(wk->param->myparty, wk->boxCursorPos);
        GameData *gameData;

        if (keep) {
            func_0200b4d4(wk->param->worldtrade_data, pkm, wk->boxTrayNo);
        }
        PokeParty_RemovePkm(wk->param->myparty, wk->boxCursorPos);
        gameData = GSYS_GetGameData(wk->param->gsys);
        checkChatotInParty(getChatterDataAddress(gameData), GameData_GetParty(gameData));
    }
    if (keep) {
        func_0200b4b0(wk->param->worldtrade_data, 1);
    }
}

// Puts a Pokémon received in the party, or in a box when the party is full
static void Upload_DownloadPokemonDataAdd(WorldTradeWork *wk, PartyPkm *pkm, int boxNo, BOOL traded) {
    PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
    addPkmToDex(wk->param->pokedex, pkm);
    boxNo = 0xff;
    if (PokeParty_GetPkmCount(wk->param->myparty) == 6) {
        boxNo = 0;
    }
    if (traded) {
        PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, 70);
        PokeParty_SetParam(pkm, PKM_PARAM_SEX,
                           PML_UtilDerivePkmSex(PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL),
                                                PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL),
                                                PokeParty_GetParam(pkm, PKM_PARAM_PID, NULL)));
        Upload_TradeDateUpDate(wk->param->worldtrade_data, 0);
    }
    if (boxNo == 0xff) {
        int count;

        PokeParty_AddPkm(wk->param->myparty, pkm);
        count = PokeParty_GetPkmCount(wk->param->myparty);
        wk->evoPokeInfo.boxNo = 0xff;
        wk->evoPokeInfo.pos = count - 1;
    } else {
        int pos = 0;

        BoxSaveAccessor_GetNextFreeBoxSlot(wk->param->mybox, &boxNo, &pos);
        BoxSaveAccessor_InsertPkmCore(wk->param->mybox, boxNo, WorldTrade_GetBoxPkm(pkm));
        wk->evoPokeInfo.boxNo = boxNo;
        wk->evoPokeInfo.pos = pos;
    }
    func_0200b4b0(wk->param->worldtrade_data, 0);
}

static void Upload_ExchangePokemonDataAdd(WorldTradeWork *wk, PartyPkm *pkm, Dpw_Tr_Data *upload, Dpw_Tr_Data *exchange,
                                          int boxNo) {
    addPkmToDex(wk->param->pokedex, pkm);
    boxNo = 0xff;
    if (PokeParty_GetPkmCount(wk->param->myparty) == 6) {
        boxNo = 0;
    }
    PokeParty_SetParam(pkm, PKM_PARAM_HAPPINESS, 70);
    PokeParty_SetParam(pkm, PKM_PARAM_SEX,
                       PML_UtilDerivePkmSex(PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL),
                                            PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL),
                                            PokeParty_GetParam(pkm, PKM_PARAM_PID, NULL)));
    if (boxNo == 0xff) {
        int count;

        PokeParty_AddPkm(wk->param->myparty, pkm);
        count = PokeParty_GetPkmCount(wk->param->myparty);
        wk->evoPokeInfo.boxNo = 0xff;
        wk->evoPokeInfo.pos = count - 1;
    } else {
        int pos = 0;

        BoxSaveAccessor_GetNextFreeBoxSlot(wk->param->mybox, &boxNo, &pos);
        BoxSaveAccessor_InsertPkmCore(wk->param->mybox, boxNo, WorldTrade_GetBoxPkm(pkm));
        wk->evoPokeInfo.boxNo = boxNo;
        wk->evoPokeInfo.pos = pos;
    }
    Upload_TradeDateUpDate(wk->param->worldtrade_data, 1);
}

// Records the date of a deposit or a trade
static void Upload_TradeDateUpDate(WorldTradeData *data, int type) {
    RTCDate date;
    RTCTime time;
    u32 value;

    func_ov011_0215dda8(&date, &time);
    value = (date.year << 24) | ((date.month & 0xff) << 16) | ((date.day & 0xff) << 8) | date.week;
    if (type == 1) {
        func_0200b4f4(data, value);
    } else {
        func_0200b4ec(data, value);
    }
}

static void Upload_WifiHistoryDataSet(UnityTowerSurveySave *save, Dpw_Tr_Data *trData) {
    u8 region = trData->localCode;
    u8 country = trData->countryCode;

    if (!func_02009ba4(save, country, region)) {
        func_02009be0(save, country, region, 1);
    }
}

// Whether the Pokémon received has no room: 2 for a Pokémon with mail and a full party, 1 for full boxes and party
static int Upload_MyPokemonPocketFullCheck(WorldTradeWork *wk, Dpw_Tr_Data *trData) {
    if (WorldTrade_PokemonMailCheck((PartyPkm *)trData->postData) && PokeParty_GetPkmCount(wk->param->myparty) == 6) {
        return 2;
    }
    if (wk->boxPokeNum == 24 * BOX_POKE_NUM && PokeParty_GetPkmCount(wk->param->myparty) == 6) {
        return 1;
    }
    return 0;
}

static void Upload_SetSaveNextSequence(WorldTradeWork *wk, u16 nextSeq1st, u16 nextSeq2nd) {
    wk->saveNextSeq1st = nextSeq1st;
    wk->saveNextSeq2nd = nextSeq2nd;
}

// Whether the save took the Pokémon back while the server still had it
static BOOL Upload_DuplicateCheck(WorldTradeWork *wk) {
    if (func_0200b4a8(wk->param->worldtrade_data) == 0 && wk->depositFlag != 0) {
        return TRUE;
    }
    return FALSE;
}

// Replaces the original trainer's name, on the Pokémon and its mail, with the version's default, and resets the
// nickname
static void Upload_ReplaceBadName(PartyPkm *pkm, HeapID heapId) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_HANDLERS_BTL_MAIN, HEAPID_TAIL(heapId));
    StrBuf *name = GFL_MsgDataLoadStrbufNew(msgData, DEFAULT_NAME_MSG);

    GFL_MsgDataFree(msgData);
    setNicknameToNick(pkm);
    PokeParty_SetParam(pkm, PKM_PARAM_OT_NAME_RAW, (u32)GFL_StrBufGetStringPtr(name));
    if (PML_ItemIsMail(PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL))) {
        MailData *mail = CreateMailData(HEAPID_TAIL(heapId));

        PokeParty_GetParam(pkm, PKM_PARAM_MAIL, mail);
        func_02009738(mail, GFL_StrBufGetStringPtr(name));
        PokeParty_SetParam(pkm, PKM_PARAM_MAIL, (u32)mail);
        GFL_HeapFree(mail);
    }
    GFL_StrBufFree(name);
}
