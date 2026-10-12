// What the Global Data Server client does with the server's answers: whether each is a success, and the getters of
// what was received. The ROM does not name the file: the name is a guess, after the Generation 4 games'
// gds_rap_response.c, which holds the same functions after gds_rap.c; that it is a file of its own shows in its
// assertion's strings, which gds_rap.c has too.

#include "types.h"
#include "app/gds_rap.h"
#include "gfl/std.h"
#include "poke_net/poke_net_gds.h"
#include "save/battle_rec.h"

// Each answer's result is 0 for a success; the others are the server's errors of the request

BOOL GdsRapResponse_MusicalShotUpload(GdsRapWork *gdsrap, POKE_NET_RESPONSE *res) {
    BOOL ret = FALSE;

    switch (res->Result) {
    case 0:
        ret = TRUE;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        break;
    }
    return ret;
}

BOOL GdsRapResponse_MusicalShotDownload(GdsRapWork *gdsrap, POKE_NET_RESPONSE *res) {
    BOOL ret = FALSE;

    switch (res->Result) {
    case 0:
        ret = TRUE;
        break;
    case 1:
    case 2:
    case 3:
        break;
    }
    return ret;
}

int GdsRapResponse_GetMusicalShots(GdsRapWork *gdsrap, POKE_NET_GDS_MUSICALSHOT_SEND **list, int max) {
    POKE_NET_RESPONSE *res = POKE_NET_GDS_GetResponse();
    POKE_NET_GDS_RESPONSE_MUSICALSHOT_GET *param = (POKE_NET_GDS_RESPONSE_MUSICALSHOT_GET *)res->Param;
    int count = param->Count;
    POKE_NET_GDS_RESPONSE_MUSICALSHOT_RECVDATA *data;
    int i;

    if (count > max) {
        count = max;
    }
    data = param->Data;
    for (i = 0; i < count; i++) {
        list[i] = &data[i].Data;
    }
    for (; i < max; i++) {
        list[i] = NULL;
    }
    return count;
}

BOOL GdsRapResponse_BattleVideoUpload(GdsRapWork *gdsrap, POKE_NET_RESPONSE *res) {
    BOOL ret = FALSE;

    switch (res->Result) {
    case 0:
        ret = TRUE;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        break;
    }
    return ret;
}

u64 GdsRapResponse_GetBattleVideoNumber(GdsRapWork *gdsrap) {
    POKE_NET_RESPONSE *res = POKE_NET_GDS_GetResponse();
    POKE_NET_GDS_RESPONSE_REGIST *param = (POKE_NET_GDS_RESPONSE_REGIST *)res->Param;

    return param->Code;
}

BOOL GdsRapResponse_BattleVideoSearch(GdsRapWork *gdsrap, POKE_NET_RESPONSE *res) {
    BOOL ret = FALSE;

    switch (res->Result) {
    case 0:
        ret = TRUE;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        break;
    }
    return ret;
}

int GdsRapResponse_GetBattleVideoList(GdsRapWork *gdsrap, POKE_NET_GDS_BATTLEDATA_OUTLINE *list, int max) {
    POKE_NET_RESPONSE *res = POKE_NET_GDS_GetResponse();
    POKE_NET_GDS_RESPONSE_BATTLEDATA_SEARCH *param = (POKE_NET_GDS_RESPONSE_BATTLEDATA_SEARCH *)res->Param;
    int count = param->Count;
    POKE_NET_GDS_RESPONSE_BATTLEDATA_SEARCH_RECVDATA *data;
    int i;

    if (count > max) {
        count = max;
        GFL_ASSERT(0);
    }
    sys_memset(list, 0, sizeof(POKE_NET_GDS_BATTLEDATA_OUTLINE) * max);

    data = param->Data;
    for (i = 0; i < count; i++) {
        // The header's number is the server's
        if (data[i].Data.Header.dataNumber != data[i].Code) {
            data[i].Data.Header.dataNumber = data[i].Code;
            data[i].Data.Header.crc = getCRC16(&data[i].Data.Header, BATTLE_REC_HEADER_CRC_SIZE);
        }
        list[i] = data[i].Data;
    }
    return count;
}

BOOL GdsRapResponse_BattleVideoDownload(GdsRapWork *gdsrap, POKE_NET_RESPONSE *res) {
    BOOL ret = FALSE;

    switch (res->Result) {
    case 0:
        ret = TRUE;
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        break;
    }
    return ret;
}

u32 GdsRapResponse_GetBattleVideo(GdsRapWork *gdsrap, BattleRecVideo **video) {
    POKE_NET_RESPONSE *res = POKE_NET_GDS_GetResponse();
    POKE_NET_GDS_RESPONSE_BATTLEDATA_GET *param = (POKE_NET_GDS_RESPONSE_BATTLEDATA_GET *)res->Param;

    *video = &param->Data;
    // The header's number is the server's
    if (param->Data.header.dataNumber != param->Code) {
        param->Data.header.dataNumber = param->Code;
        (*video)->header.crc = getCRC16(&(*video)->header, BATTLE_REC_HEADER_CRC_SIZE);
    }
    return param->Code;
}

BOOL GdsRapResponse_BattleVideoFavorite(GdsRapWork *gdsrap, POKE_NET_RESPONSE *res) {
    BOOL ret = FALSE;

    switch (res->Result) {
    case 0:
        ret = TRUE;
        break;
    case 1:
    case 2:
    case 3:
        break;
    }
    return ret;
}
