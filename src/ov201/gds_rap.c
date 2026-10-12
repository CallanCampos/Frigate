// The game's side of the Global Data Server (GDS) client: it sends the Battle Recorder's requests through the GDS
// library, which runs them in a thread, and calls the request's callback once the answer has come and what follows it
// is done. A battle video's Pokémon are checked by the server of nhttp_rap.c before the video is uploaded, and the
// uploaded video is saved with the number that the server gave it. The name is the ROM's string, which
// GFL_HeapAllocate is given.

#include "types.h"
#include "app/gds_rap.h"
#include "app/battle_rec_ov.h"
#include "constants/arc.h"
#include "constants/language.h"
#include "constants/text_banks.h"
#include "constants/version.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/nhttp_rap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/species_names.h"
#include "poke_net/poke_net_gds.h"
#include "save/battle_rec.h"
#include "save/player_info.h"
#include "system/game_data.h"
#include "system/wordset.h"

// The server, and the frames to wait before a request with data is sent
#define GDS_RAP_SERVER_URL "pkgdsprod.nintendo.co.jp"
#define GDS_RAP_SERVER_PORT 12401
#define GDS_RAP_SEND_WAIT 60
// The check that nhttp_rap.c sends for a battle video
#define GDS_RAP_CHECK_TYPE 3

// The steps of a battle video's upload
enum {
    GDS_RAP_UPLOAD_SEQ_CHECK_START,
    GDS_RAP_UPLOAD_SEQ_CHECK_WAIT,
    GDS_RAP_UPLOAD_SEQ_ENCRYPT,
    GDS_RAP_UPLOAD_SEQ_SEND,
};

// The battle mode of each search and the mask of its bits that the server compares
typedef struct {
    u16 mode;
    u16 mask;
} GdsRapBattleMode;

static BOOL GdsRap_MainSend(GdsRapWork *gdsrap, BOOL *checkError);
static BOOL GdsRap_MainRecv(GdsRapWork *gdsrap);
static BOOL GdsRap_SubProcNone(GdsRapWork *gdsrap, GdsRapSubProc *sub);
static BOOL GdsRap_SubProcSaveDataNumber(GdsRapWork *gdsrap, GdsRapSubProc *sub);
static BOOL GdsRap_SubProcError(GdsRapWork *gdsrap, GdsRapSubProc *sub);
static BOOL GdsRap_GetVideoPokemonPos(GdsRapWork *gdsrap, int index, s32 *client, s32 *pos);
static void GdsRap_CopyNickname(const u16 *src, u16 *dest, int length);
static BOOL GdsRap_RecvResponse(GdsRapWork *gdsrap);

static const GdsRapBattleMode BattleModeBitTbl[] = {
    {0x18, 0xbf},
    {0x98, 0xbf},
    {0x19, 0xbf},
    {0x99, 0xbf},
    {0x1a, 0xbf},
    {0x9a, 0xbf},
    {0x1b, 0xbf},
    {0x9b, 0xbf},
    {0x1c, 0xbf},
    {0x9c, 0xbf},
    {0x00, 0x3f},
    {0x01, 0x3f},
    {0x04, 0x3f},
    {0x28, 0x3f},
    {0x29, 0x3f},
    {0x2a, 0x3f},
    {0x2b, 0x3f},
    {0xaa, 0xbf},
    {0x38, 0x38},
    {0x00, 0x00},
};

BOOL GdsRap_Init(GdsRapWork *gdsrap, const GdsRapInit *init) {
    BOOL ret;

    GFL_OvlLoad(OVERLAY_ID(189));
    sys_memset(gdsrap, 0, sizeof(GdsRapWork));
    gdsrap->heapId = init->heapId;
    gdsrap->gameData = init->gameData;
    gdsrap->loginBuffer = init->loginBuffer;
    gdsrap->callbacks = init->callbacks;
    gdsrap->lastStatus = -1;
    gdsrap->sendReq = POKE_NET_GDS_REQCODE_LAST;
    gdsrap->recv_wait_req = POKE_NET_GDS_REQCODE_LAST;

    gdsrap->response = GFL_HeapAllocate(init->heapId, POKE_NET_GDS_GetResponseMaxSize(POKE_NET_GDS_REQCODE_ADMIN),
                                        FALSE, "gds_rap.c", 132);
    sys_memset(gdsrap->response, 0, POKE_NET_GDS_GetResponseMaxSize(POKE_NET_GDS_REQCODE_ADMIN));

    gdsrap->msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_WIFI_ERROR, init->heapId);
    gdsrap->wordSet = GFL_WordSetSystemCreateDefault(init->heapId);
    gdsrap->strBuf = GFL_StrBufCreate(0x100, init->heapId);

    gdsrap->auth.PID = init->profileId;
    gdsrap->auth.ROMCode = GAME_VERSION;
    gdsrap->auth.LangCode = GAME_LANGUAGE;
    ret = POKE_NET_GDS_Initialize(&gdsrap->auth, GDS_RAP_SERVER_URL, GDS_RAP_SERVER_PORT);
    GFL_ASSERT(ret == TRUE);
    gdsrap->isActive = TRUE;
    gdsrap->unk3B0 = TRUE;
    return TRUE;
}

void GdsRap_Exit(GdsRapWork *gdsrap) {
    if (gdsrap->p_nhttp != NULL) {
        NHttpRap_FreePostData(gdsrap->p_nhttp);
        NHttpRap_Destroy(gdsrap->p_nhttp);
    }
    POKE_NET_GDS_Release();
    gdsrap->isActive = FALSE;
    GFL_StrBufFree(gdsrap->strBuf);
    GFL_WordSetSystemFree(gdsrap->wordSet);
    GFL_MsgDataFree(gdsrap->msgData);
    GFL_HeapFree(gdsrap->response);
    GFL_OvlUnload(OVERLAY_ID(189));
}

BOOL GdsRap_SendMusicalShotUpload(GdsRapWork *gdsrap, GdsProfile *profile, MusicalShot *shot) {
    if (GdsRap_IsIdle(gdsrap) == FALSE) {
        return FALSE;
    }
    gdsrap->send.musicalShot.Profile = *profile;
    gdsrap->send.musicalShot.Data = *shot;
    gdsrap->sendWait = GDS_RAP_SEND_WAIT;
    gdsrap->sendReq = POKE_NET_GDS_REQCODE_MUSICALSHOT_REGIST;
    return TRUE;
}

BOOL GdsRap_SendMusicalShotDownload(GdsRapWork *gdsrap, u16 species) {
    if (GdsRap_IsIdle(gdsrap) == FALSE) {
        return FALSE;
    }
    gdsrap->species = species;
    gdsrap->sendReq = POKE_NET_GDS_REQCODE_MUSICALSHOT_GET;
    return TRUE;
}

BOOL GdsRap_SendBattleVideoUpload(GdsRapWork *gdsrap, GdsProfile *profile) {
    BattleRecVideo *video = BattleRecOv_GetVideo();
    BattleRecPkm *pkm;
    int i;
    s32 client;
    s32 pos;
    s32 clientCount;
    s32 pkmCount;

    if (GdsRap_IsIdle(gdsrap) == FALSE) {
        return FALSE;
    }

    // Keep the nicknames, in case the check of the Pokémon takes them out of the video
    func_0200bef4(video->header.mode, &clientCount, &pkmCount);
    for (i = 0; i < BATTLE_REC_HEADER_PKM_MAX; i++) {
        if (!GdsRap_GetVideoPokemonPos(gdsrap, i, &client, &pos)) {
            break;
        }
        pkm = &video->body.parties[client].pkm[pos];
        GdsRap_CopyNickname(pkm->nickname, gdsrap->nicknames[i], 11);
    }
    gdsrap->nicknamesReplaced = FALSE;
    gdsrap->checkCount = 0;

    gdsrap->send.video = video;
    sys_memcpy(profile, getVSPlayerAllocation(), sizeof(GdsProfile));
    gdsrap->sendWait = GDS_RAP_SEND_WAIT;
    gdsrap->sendReq = POKE_NET_GDS_REQCODE_BATTLEDATA_REGIST;
    return TRUE;
}

BOOL GdsRap_SendBattleVideoSearch(GdsRapWork *gdsrap, u16 species, u32 search_mode_no, u8 a3, u8 a4) {
    if (GdsRap_IsIdle(gdsrap) == FALSE) {
        return FALSE;
    }
    GFL_ASSERT(search_mode_no < NELEMS(BattleModeBitTbl));

    sys_memset(&gdsrap->send.condition, 0, sizeof(POKE_NET_GDS_BATTLEDATA_SEARCHCONDITION));
    gdsrap->send.condition.MonsNo = species;
    gdsrap->send.condition.BattleMode = BattleModeBitTbl[search_mode_no].mode;
    gdsrap->send.condition.BattleModeMask = BattleModeBitTbl[search_mode_no].mask;
    gdsrap->send.condition.unk6 = a3;
    gdsrap->send.condition.unk7 = a4;
    gdsrap->send.condition.ServerVersion = BATTLE_REC_VERSION;
    gdsrap->searchType = POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_CONDITION;
    gdsrap->sendReq = POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH;
    return TRUE;
}

BOOL GdsRap_SendBattleVideoList1(GdsRapWork *gdsrap) {
    if (GdsRap_IsIdle(gdsrap) == FALSE) {
        return FALSE;
    }
    gdsrap->searchType = POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_LIST1;
    gdsrap->sendReq = POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH;
    return TRUE;
}

BOOL GdsRap_SendBattleVideoList2(GdsRapWork *gdsrap) {
    if (GdsRap_IsIdle(gdsrap) == FALSE) {
        return FALSE;
    }
    gdsrap->searchType = POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_LIST2;
    gdsrap->sendReq = POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH;
    return TRUE;
}

BOOL GdsRap_SendBattleVideoList3(GdsRapWork *gdsrap) {
    if (GdsRap_IsIdle(gdsrap) == FALSE) {
        return FALSE;
    }
    gdsrap->searchType = POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_LIST3;
    gdsrap->sendReq = POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH;
    return TRUE;
}

BOOL GdsRap_SendBattleVideoDownload(GdsRapWork *gdsrap, u64 dataNumber) {
    if (GdsRap_IsIdle(gdsrap) == FALSE) {
        return FALSE;
    }
    gdsrap->dataNumber = dataNumber;
    gdsrap->sendReq = POKE_NET_GDS_REQCODE_BATTLEDATA_GET;
    return TRUE;
}

BOOL GdsRap_SendBattleVideoFavorite(GdsRapWork *gdsrap, u64 dataNumber) {
    if (GdsRap_IsIdle(gdsrap) == FALSE) {
        return FALSE;
    }
    gdsrap->dataNumber = dataNumber;
    gdsrap->sendReq = POKE_NET_GDS_REQCODE_BATTLEDATA_FAVORITE;
    return TRUE;
}

BOOL GdsRap_Main(GdsRapWork *gdsrap) {
    BOOL checkError;

    if (gdsrap->isActive == TRUE) {
        // Send the request once the library is free
        if (gdsrap->sendReq != POKE_NET_GDS_REQCODE_LAST) {
            if (gdsrap->sendWait != 0) {
                gdsrap->sendWait--;
            } else {
                switch (POKE_NET_GDS_GetStatus()) {
                case POKE_NET_GDS_STATUS_INITIALIZED:
                case POKE_NET_GDS_STATUS_ABORTED:
                case POKE_NET_GDS_STATUS_FINISHED:
                case POKE_NET_GDS_STATUS_ERROR:
                    if (GdsRap_MainSend(gdsrap, &checkError) == TRUE) {
                        gdsrap->sendReq = POKE_NET_GDS_REQCODE_LAST;
                        gdsrap->uploadSeq = 0;
                        if (checkError == TRUE) {
                            gdsrap->result.type = GDS_RAP_RESULT_TYPE_SERVER_ERROR;
                            gdsrap->result.code = POKE_NET_GDS_REQCODE_BATTLEDATA_REGIST;
                            gdsrap->result.detail = gdsrap->httpError;
                            gdsrap->result.isValid = TRUE;
                            if (gdsrap->callbacks.onUploadVideoEnd != NULL) {
                                gdsrap->callbacks.onUploadVideoEnd(gdsrap->callbacks.work, &gdsrap->result);
                            }
                        }
                    }
                    break;
                }
            }
        }

        GdsRap_MainRecv(gdsrap);

        // What follows the answer, then the request's callback
        if (gdsrap->sub.func != NULL) {
            if (gdsrap->sub.func(gdsrap, &gdsrap->sub) == TRUE) {
                gdsrap->sub.func = NULL;
                if (gdsrap->sub.callback != NULL) {
                    gdsrap->sub.callback(gdsrap->callbacks.work, &gdsrap->result);
                    gdsrap->sub.callback = NULL;
                }
                sys_memset(&gdsrap->sub, 0, sizeof(GdsRapSubProc));
                gdsrap->recv_wait_req = POKE_NET_GDS_REQCODE_LAST;
            }
        }
    }
    return TRUE;
}

// Sends the request, returning TRUE once it is sent. A battle video's Pokémon are checked first, which takes several
// calls; if the check could not be made, this gives TRUE in checkError as well
static BOOL GdsRap_MainSend(GdsRapWork *gdsrap, BOOL *checkError) {
    BOOL result = FALSE;

    *checkError = FALSE;
    switch (gdsrap->sendReq) {
    case POKE_NET_GDS_REQCODE_MUSICALSHOT_REGIST:
        result = POKE_NET_GDS_MusicalShotRegist(&gdsrap->send.musicalShot, gdsrap->response);
        break;
    case POKE_NET_GDS_REQCODE_MUSICALSHOT_GET:
        result = POKE_NET_GDS_MusicalShotGet(gdsrap->species, gdsrap->response);
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_REGIST:
        switch (gdsrap->uploadSeq) {
        case GDS_RAP_UPLOAD_SEQ_CHECK_START: {
            BattleRecVideo *video = BattleRecOv_GetVideo();
            s32 clientCount;
            s32 pkmCount;
            int i;
            BOOL ret;

            GFL_ASSERT(gdsrap->p_nhttp == NULL);
            gdsrap->p_nhttp = NHttpRap_Create(gdsrap->heapId, func_02008bdc(GetGameDataPlayerInfo(gdsrap->gameData)),
                                              gdsrap->loginBuffer);
            NHttpRap_BeginPost(gdsrap->p_nhttp, gdsrap->heapId, sizeof(BattleRecPkm) * BATTLE_REC_HEADER_PKM_MAX,
                               GDS_RAP_CHECK_TYPE);
            func_0200bef4(video->header.mode, &clientCount, &pkmCount);
            gdsrap->pkmCount = 0;
            for (i = 0; i < clientCount; i++) {
                NHttpRap_AddPostData(gdsrap->p_nhttp, video->body.parties[i].pkm,
                                     sizeof(BattleRecPkm) * video->body.parties[i].count);
                gdsrap->pkmCount += video->body.parties[i].count;
            }
            ret = NHttpRap_SendValidate(gdsrap->p_nhttp);
            GFL_ASSERT(ret);
            ret = NHttpRap_StartRequest(gdsrap->p_nhttp) == 0;
            GFL_ASSERT(ret);
            gdsrap->uploadSeq++;
            break;
        }
        case GDS_RAP_UPLOAD_SEQ_CHECK_WAIT: {
            int status = NHttpRap_GetStatus(gdsrap->p_nhttp);
            int error = NHttpRap_Poll(gdsrap->p_nhttp);

            if (status != 200 && error != 15) {
                switch (NHttpRap_GetStatus(gdsrap->p_nhttp)) {
                case 400:
                    gdsrap->httpError = GDS_RAP_HTTP_ERROR_BAD_REQUEST;
                    break;
                case 401:
                    gdsrap->httpError = GDS_RAP_HTTP_ERROR_UNAUTHORIZED;
                    break;
                case 408:
                    gdsrap->httpError = GDS_RAP_HTTP_ERROR_TIMEOUT;
                    break;
                default:
                    gdsrap->httpError = GDS_RAP_HTTP_ERROR_OTHER;
                    break;
                }
                NHttpRap_FreePostData(gdsrap->p_nhttp);
                NHttpRap_EndRequest(gdsrap->p_nhttp);
                NHttpRap_Destroy(gdsrap->p_nhttp);
                gdsrap->p_nhttp = NULL;
                *checkError = TRUE;
                return TRUE;
            }
            if (error == 0) {
                void *answer = NHttpRap_GetAnswer(gdsrap->p_nhttp);
                int i;

                if (NHttpRap_GetCheckStatus(answer) == 0) {
                    // The Pokémon passed: the upload carries the check's signature
                    gdsrap->checkFailed = FALSE;
                    sys_memcpy(NHttpRap_GetCheckSignature(answer, gdsrap->pkmCount), gdsrap->signature,
                               GDS_RAP_SIGNATURE_SIZE);
                    for (i = 0; i < gdsrap->pkmCount; i++) {
                        gdsrap->checkResult = NHttpRap_GetCheckResult(answer, i);
                    }
                    gdsrap->uploadSeq++;
                } else {
                    // Give the Pokémon that failed their species' names and check once more
                    BattleRecVideo *video = BattleRecOv_GetVideo();
                    s32 client;
                    s32 pos;
                    s32 clientCount;
                    s32 pkmCount;

                    if (gdsrap->checkCount == 1) {
                        gdsrap->checkFailed = TRUE;
                    }
                    func_0200bef4(video->header.mode, &clientCount, &pkmCount);
                    for (i = 0; i < gdsrap->pkmCount; i++) {
                        gdsrap->checkResult = NHttpRap_GetCheckResult(answer, i);
                        if (gdsrap->checkResult != 0 && gdsrap->checkCount == 0) {
                            BattleRecPkm *pkm;

                            if (!GdsRap_GetVideoPokemonPos(gdsrap, i, &client, &pos)) {
                                break;
                            }
                            pkm = &video->body.parties[client].pkm[pos];
                            GFL_MsgDataLoadRawStr(g_PMLSpeciesNamesResident, pkm->species, pkm->nickname, 11);
                            gdsrap->nicknamesReplaced = TRUE;
                        }
                    }
                    if (gdsrap->checkCount == 0) {
                        func_0200c1dc(func_0200c0b4());
                        gdsrap->uploadSeq = GDS_RAP_UPLOAD_SEQ_CHECK_START;
                        gdsrap->checkCount++;
                    } else {
                        gdsrap->uploadSeq++;
                    }
                }
                NHttpRap_FreePostData(gdsrap->p_nhttp);
                NHttpRap_Destroy(gdsrap->p_nhttp);
                gdsrap->p_nhttp = NULL;
            } else {
                if (error != 15 && gdsrap->p_nhttp != NULL) {
                    NHttpRap_FreePostData(gdsrap->p_nhttp);
                    NHttpRap_EndRequest(gdsrap->p_nhttp);
                    NHttpRap_Destroy(gdsrap->p_nhttp);
                    gdsrap->p_nhttp = NULL;
                    return TRUE;
                }
                return FALSE;
            }
            break;
        }
        case GDS_RAP_UPLOAD_SEQ_ENCRYPT:
            BattleRecOv_EncryptBody(GameData_GetSaveControl(gdsrap->gameData));
            gdsrap->uploadSeq++;
            break;
        case GDS_RAP_UPLOAD_SEQ_SEND:
            result = POKE_NET_GDS_BattleDataRegist(gdsrap->send.video, gdsrap->signature, GDS_RAP_SIGNATURE_SIZE,
                                                   gdsrap->response);
            break;
        }
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH:
        switch (gdsrap->searchType) {
        case POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_CONDITION:
            result = POKE_NET_GDS_BattleDataSearchCondition(&gdsrap->send.condition, gdsrap->response);
            break;
        case POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_LIST1:
            result = POKE_NET_GDS_BattleDataSearchList1(BATTLE_REC_VERSION, gdsrap->response);
            break;
        case POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_LIST2:
            result = POKE_NET_GDS_BattleDataSearchList2(BATTLE_REC_VERSION, gdsrap->response);
            break;
        case POKE_NET_GDS_BATTLEDATA_SEARCHTYPE_LIST3:
            result = POKE_NET_GDS_BattleDataSearchList3(BATTLE_REC_VERSION, gdsrap->response);
            break;
        }
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_GET:
        result = POKE_NET_GDS_BattleDataGet(gdsrap->dataNumber, BATTLE_REC_VERSION, gdsrap->response);
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_FAVORITE:
        result = POKE_NET_GDS_BattleDataFavorite(gdsrap->dataNumber, gdsrap->response);
        break;
    default:
        GFL_ASSERT(0);
        break;
    }

    if (result == TRUE) {
        gdsrap->recv_wait_req = gdsrap->sendReq;
    }
    return result;
}

// Watches the library's status while an answer is waited for: the answer, an abort or an error ends the request
static BOOL GdsRap_MainRecv(GdsRapWork *gdsrap) {
    if (gdsrap->recv_wait_req == POKE_NET_GDS_REQCODE_LAST) {
        return TRUE;
    }

    gdsrap->status = POKE_NET_GDS_GetStatus();
    if (gdsrap->status != gdsrap->lastStatus) {
        switch (gdsrap->status) {
        case POKE_NET_GDS_STATUS_INACTIVE:
        case POKE_NET_GDS_STATUS_INITIALIZED:
        case POKE_NET_GDS_STATUS_REQUEST:
        case POKE_NET_GDS_STATUS_NETSETTING:
        case POKE_NET_GDS_STATUS_CONNECTING:
        case POKE_NET_GDS_STATUS_SENDING:
        case POKE_NET_GDS_STATUS_RECEIVING:
            break;
        case POKE_NET_GDS_STATUS_ABORTED:
            gdsrap->result.type = GDS_RAP_RESULT_TYPE_ABORTED;
            gdsrap->result.code = 0;
            gdsrap->result.detail = POKE_NET_GDS_STATUS_ABORTED;
            gdsrap->result.isValid = TRUE;
            gdsrap->sub.func = GdsRap_SubProcError;
            break;
        case POKE_NET_GDS_STATUS_FINISHED:
            gdsrap->sub.func = GdsRap_SubProcNone;
            GdsRap_RecvResponse(gdsrap);
            break;
        case POKE_NET_GDS_STATUS_ERROR:
            gdsrap->result.type = GDS_RAP_RESULT_TYPE_LIB_ERROR;
            gdsrap->result.code = 0;
            gdsrap->result.detail = POKE_NET_GDS_GetLastErrorCode();
            gdsrap->result.isValid = TRUE;
            gdsrap->sub.func = GdsRap_SubProcError;
            break;
        }
        gdsrap->lastStatus = gdsrap->status;
    }
    return TRUE;
}

static BOOL GdsRap_SubProcNone(GdsRapWork *gdsrap, GdsRapSubProc *sub) {
    return TRUE;
}

// After a battle video's upload: gives the video its nicknames back, and saves it with the number the server gave it
static BOOL GdsRap_SubProcSaveDataNumber(GdsRapWork *gdsrap, GdsRapSubProc *sub) {
    POKE_NET_RESPONSE *res;
    POKE_NET_GDS_RESPONSE_REGIST *param;
    u32 result;

    if (sub->seq == 0 && gdsrap->nicknamesReplaced) {
        BattleRecVideo *video = BattleRecOv_GetVideo();
        BattleRecPkm *pkm;
        int i;
        s32 client;
        s32 pos;
        s32 clientCount;
        s32 pkmCount;

        func_0200bbb4();
        func_0200bef4(video->header.mode, &clientCount, &pkmCount);
        for (i = 0; i < BATTLE_REC_HEADER_PKM_MAX; i++) {
            if (!GdsRap_GetVideoPokemonPos(gdsrap, i, &client, &pos)) {
                break;
            }
            pkm = &video->body.parties[client].pkm[pos];
            GdsRap_CopyNickname(gdsrap->nicknames[i], pkm->nickname, 11);
        }
        func_0200c1dc(func_0200c0b4());
        func_0200bbdc();
    }

    res = POKE_NET_GDS_GetResponse();
    param = (POKE_NET_GDS_RESPONSE_REGIST *)res->Param;
    result = BattleRecOv_SaveDataNumber(gdsrap->gameData, param->Code, &sub->seq, &sub->saveSeq, gdsrap->heapId);
    if (result == 2 || result == 3) {
        return TRUE;
    }
    return FALSE;
}

static BOOL GdsRap_SubProcError(GdsRapWork *gdsrap, GdsRapSubProc *sub) {
    return TRUE;
}

void GdsRap_OnNetEvent(GdsRapWork *gdsrap, int a1, int event, int a3) {
    if (event == 3 || event == 6) {
        if (gdsrap->p_nhttp != NULL) {
            NHttpRap_FreePostData(gdsrap->p_nhttp);
            NHttpRap_EndRequest(gdsrap->p_nhttp);
            NHttpRap_Destroy(gdsrap->p_nhttp);
            gdsrap->p_nhttp = NULL;
        }
    }
}

// Gives the client and the place in its party of the loaded video's Pokémon of an index, counting through the
// parties; FALSE if there are fewer
static BOOL GdsRap_GetVideoPokemonPos(GdsRapWork *gdsrap, int index, s32 *client, s32 *pos) {
    int count = 0;
    BattleRecVideo *video = BattleRecOv_GetVideo();
    s32 clientCount;
    s32 pkmCount;

    func_0200bef4(video->header.mode, &clientCount, &pkmCount);
    for (*client = 0; *client < clientCount; (*client)++) {
        for (*pos = 0; *pos < video->body.parties[*client].count; (*pos)++) {
            if (count == index) {
                return TRUE;
            }
            count++;
        }
    }
    return FALSE;
}

static void GdsRap_CopyNickname(const u16 *src, u16 *dest, int length) {
    sys_memcpy(src, dest, length * sizeof(u16));
}

// Takes the library's answer: whether the server's result is a success, the callback of the request, and what is
// left to do before it is called
static BOOL GdsRap_RecvResponse(GdsRapWork *gdsrap) {
    BOOL ret = TRUE;
    POKE_NET_RESPONSE *res = POKE_NET_GDS_GetResponse();

    POKE_NET_GDS_GetResponseSize();
    GFL_ASSERT(gdsrap->recv_wait_req == res->ReqCode);

    gdsrap->sub.callback = NULL;
    switch (res->ReqCode) {
    case POKE_NET_GDS_REQCODE_DEBUG_MESSAGE:
        break;
    case POKE_NET_GDS_REQCODE_MUSICALSHOT_REGIST:
        ret = GdsRapResponse_MusicalShotUpload(gdsrap, res);
        gdsrap->sub.callback = gdsrap->callbacks.onUploadMusicalShotEnd;
        break;
    case POKE_NET_GDS_REQCODE_MUSICALSHOT_GET:
        ret = GdsRapResponse_MusicalShotDownload(gdsrap, res);
        gdsrap->sub.callback = gdsrap->callbacks.onDownloadMusicalShotEnd;
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_REGIST:
        ret = GdsRapResponse_BattleVideoUpload(gdsrap, res);
        gdsrap->sub.callback = gdsrap->callbacks.onUploadVideoEnd;
        if (ret == TRUE) {
            gdsrap->sub.func = GdsRap_SubProcSaveDataNumber;
        }
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_SEARCH:
        ret = GdsRapResponse_BattleVideoSearch(gdsrap, res);
        gdsrap->sub.callback = gdsrap->callbacks.onDownloadVideoListEnd;
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_GET:
        ret = GdsRapResponse_BattleVideoDownload(gdsrap, res);
        gdsrap->sub.callback = gdsrap->callbacks.onDownloadVideoEnd;
        break;
    case POKE_NET_GDS_REQCODE_BATTLEDATA_FAVORITE:
        ret = GdsRapResponse_BattleVideoFavorite(gdsrap, res);
        gdsrap->sub.callback = gdsrap->callbacks.onFavoriteVideoEnd;
        break;
    }

    if (ret == FALSE) {
        gdsrap->result.type = GDS_RAP_RESULT_TYPE_SERVER_ERROR;
        gdsrap->result.code = res->ReqCode;
        gdsrap->result.detail = res->Result;
        gdsrap->result.isValid = TRUE;
    } else {
        gdsrap->result.isValid = FALSE;
    }
    return ret;
}

BOOL GdsRap_GetResult(GdsRapWork *gdsrap, GdsRapResult **result) {
    *result = &gdsrap->result;
    return (*result)->isValid;
}

void GdsRap_ClearResult(GdsRapWork *gdsrap) {
    sys_memset(&gdsrap->result, 0, sizeof(GdsRapResult));
}

BOOL GdsRap_IsIdle(GdsRapWork *gdsrap) {
    if (gdsrap->sendReq == POKE_NET_GDS_REQCODE_LAST && gdsrap->recv_wait_req == POKE_NET_GDS_REQCODE_LAST) {
        return TRUE;
    }
    return FALSE;
}
