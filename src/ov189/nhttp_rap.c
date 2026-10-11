// nhttp_rap.c: Game Freak's wrapper of HTTP requests to the game's servers. The name is the ROM's own; the
// functions' names are ours

#include "gfl/nhttp_rap.h"
#include "types.h"
#include "constants/language.h"
#include "constants/version.h"
#include "dwc/nhttp.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/std.h"
#include "nitro/os.h"

#define NHTTP_RAP_URL_SIZE 0x190
#define NHTTP_RAP_BODY_SIZE 0x400
#define NHTTP_RAP_NUM_REQUESTS 11
#define NHTTP_RAP_REQUEST_VALIDATE 7

typedef struct NHttpRapLogin {
    u8 unk_00[0x45];
    char token[0x174 - 0x45];
} NHttpRapLogin;

struct NHttpRap {
    u8 *data;
    u32 size;
    NHttpRapLogin *login;
    int connection;
    u8 body[NHTTP_RAP_BODY_SIZE];
    char url[NHTTP_RAP_URL_SIZE];
    u32 unk_5A0;
    s32 profileId;
    void *answer;
    u32 answerSize;
};

typedef struct NHttpRapRequest {
    const char *url;
    u32 method;
} NHttpRapRequest;

static void *NHttpRap_Alloc(u32 size, int align);
static void NHttpRap_Free(void *ptr);
static int NHttpRap_RequestCallback(int request, int event, void *arg);
static BOOL NHttpRap_CreateRequest(u32 index, NHttpRap *rap);
static void NHttpRap_BuildUrl(NHttpRap *rap, u32 index);
static void NHttpRap_WriteHeader(NHttpRap *rap, int type);

static void *sRootCertificates[1] = { data_ov011_021866c0 };
static NHttpRapRequest sRequests[NHTTP_RAP_NUM_REQUESTS] = {
    { "https://en-ds.pokemon-gl.com/dsio/gw?p=account.playstatus&gsid=%u&rom=%u&langcode=%u&dreamw=%u&tok=%s", 0 },
    { "https://en-ds.pokemon-gl.com/dsio/gw?p=sleepily.bitlist&gsid=%u&rom=%u&langcode=%u&dreamw=%u&tok=%s", 0 },
    { "https://en-ds.pokemon-gl.com/dsio/gw?p=savedata.download&gsid=%u&rom=%u&langcode=%u&dreamw=%u&tok=%s", 0 },
    { "https://en-ds.pokemon-gl.com/dsio/gw?p=savedata.upload&gsid=%u&rom=%u&langcode=%u&dreamw=%u&tok=%s", 1 },
    { "https://en-ds.pokemon-gl.com/dsio/gw?p=account.createdata&tok=%s", 1 },
    { "https://en-ds.pokemon-gl.com/dsio/gw?p=worldbattle.download&gsid=%u&rom=%u&langcode=%u&dreamw=%u&tok=%s", 0 },
    { "https://en-ds.pokemon-gl.com/dsio/gw?p=worldbattle.upload&gsid=%u&rom=%u&langcode=%u&dreamw=%u&tok=%s", 1 },
    { "https://pkvldtprod.nintendo.co.jp/pokemon/validate", 1 },
    { "https://en-ds.pokemon-gl.com/dsio/gw?p=savedata.download.finish&gsid=%u&rom=%u&langcode=%u&dreamw=%u&tok=%s",
      1 },
    { "https://en-ds.pokemon-gl.com/dsio/gw?p=account.create.upload&gsid=%u&rom=%u&langcode=%u&dreamw=%u&tok=%s", 1 },
    { "https://en-ds.pokemon-gl.com/dsio/gw?p=savedata.getbw&gsid=%u&rom=%u&langcode=%u&dreamw=%u&tok=%s", 0 },
};
static NHttpRap *sNHttpRap;

static void *NHttpRap_Alloc(u32 size, int align) {
    return DWCRapCommon_Alloc(0xd, size, align);
}

static void NHttpRap_Free(void *ptr) {
    DWCRapCommon_Free(0xd, ptr, 0);
}

static int NHttpRap_RequestCallback(int request, int event, void *arg) {
    NHttpRap *rap = arg;

    switch (event) {
    case 1:
        break;
    case 2:
    case 3:
        rap->login = NULL;
        break;
    }
    return 0;
}

static BOOL NHttpRap_CreateRequest(u32 index, NHttpRap *rap) {
    int handle;
    int result;

    handle = func_ov189_021a0e1c(rap->url, sRequests[index].method, rap->answer, rap->answerSize,
                                 NHttpRap_RequestCallback, rap);
    GFL_ASSERT(handle);
    if (handle == 0) {
        return FALSE;
    }
    rap->connection = handle;
    result = func_ov189_021a09ec(handle, sRootCertificates, 1);
    if (result < 0) {
        GFL_ASSERT_MSG(0, " NHTTP_SetRootCA(%d)\n", result);
        return FALSE;
    }
    if (func_ov189_021a07bc(handle, "Accept", "*/*") != 0) {
        GFL_ASSERT(0);
        return FALSE;
    }
    if (func_ov189_021a07bc(handle, "User-Agent", "Nintendo-DS") != 0) {
        GFL_ASSERT(0);
        return FALSE;
    }
    if (func_ov189_021a0954(handle, "pokemon", "2Phfv9MY") != 0) {
        GFL_ASSERT(0);
        return FALSE;
    }
    return TRUE;
}

static void NHttpRap_BuildUrl(NHttpRap *rap, u32 index) {
    sys_memset(rap->url, 0, 4);
    if (index == 4) {
        OS_SNPrintf(rap->url, NHTTP_RAP_URL_SIZE, sRequests[index].url, rap->login->token);
    } else {
        OS_SNPrintf(rap->url, NHTTP_RAP_URL_SIZE, sRequests[index].url, rap->profileId, GAME_VERSION, GAME_LANGUAGE, 1,
                    rap->login->token);
    }
}

BOOL NHttpRap_SendRequest(u32 index, NHttpRap *rap) {
    if (func_ov189_021a076c(NHttpRap_Alloc, NHttpRap_Free, 12) != 0) {
        GFL_ASSERT(0);
        return FALSE;
    }
    if (rap == NULL) {
        return FALSE;
    }
    NHttpRap_BuildUrl(rap, index);
    return NHttpRap_CreateRequest(index, rap);
}

BOOL NHttpRap_SendRequestWithId(u32 index, u32 id, NHttpRap *rap) {
    if (func_ov189_021a076c(NHttpRap_Alloc, NHttpRap_Free, 12) != 0) {
        GFL_ASSERT(0);
        return FALSE;
    }
    if (rap == NULL) {
        return FALSE;
    }
    if (index == 10) {
        sys_memset(rap->url, 0, 4);
        OS_SNPrintf(rap->url, NHTTP_RAP_URL_SIZE, sRequests[10].url, id, GAME_VERSION, GAME_LANGUAGE, 1,
                    rap->login->token);
    } else {
        NHttpRap_BuildUrl(rap, index);
    }
    return NHttpRap_CreateRequest(index, rap);
}

int NHttpRap_GetConnection(NHttpRap *rap) {
    if (rap != NULL) {
        return rap->connection;
    }
    return 0;
}

int NHttpRap_StartRequest(NHttpRap *rap) {
    int error;

    if (rap != NULL) {
        error = func_ov189_021a0fe4(rap->connection);
        if (error != 0) {
            func_020424ac(error, 2, error, 0x3f6);
            return error;
        }
        return 0;
    }
    return 1;
}

void NHttpRap_EndRequest(NHttpRap *rap) {
    if (rap != NULL && rap->connection != 0) {
        func_ov189_021a0edc(rap->connection);
        rap->connection = 0;
        func_ov189_021a07a0();
    }
}

int NHttpRap_Poll(NHttpRap *rap) {
    int state;
    int received;
    int size;
    int answerSize;
    int error;

    if (rap == NULL) {
        return -1;
    }
    if (rap->connection == 0) {
        return 0;
    }
    state = func_ov189_021a10f4(rap->connection);
    if (state == 15) {
        func_ov189_021a1114(rap->connection, &received, &size);
    } else {
        error = func_ov189_021a10f4(rap->connection);
        if (error == 0) {
            func_ov189_021a08cc(rap->connection, &answerSize);
        } else {
            func_020424ac(error, 2, error, 0x3f6);
        }
        NHttpRap_EndRequest(rap);
        return state;
    }
    return state;
}

void *NHttpRap_GetAnswer(NHttpRap *rap) {
    if (rap == NULL) {
        return NULL;
    }
    return rap->answer;
}

NHttpRap *NHttpRap_Create(HeapID heapId, s32 profileId, void *loginBuffer) {
    NHttpRap *rap = GFL_HeapAllocate(heapId, sizeof(NHttpRap), TRUE, "nhttp_rap.c", 0x1cb);

    sNHttpRap = rap;
    rap->profileId = profileId;
    rap->login = loginBuffer;
    NHttpRap_ResetAnswerBuffer(rap);
    return rap;
}

void NHttpRap_Destroy(NHttpRap *rap) {
    if (rap != NULL) {
        NHttpRap_FreePostData(rap);
        GFL_HeapFree(rap);
    }
}

static void NHttpRap_WriteHeader(NHttpRap *rap, int type) {
    int i;
    char ch;

    type &= 0xffff;
    if (rap != NULL) {
        rap->size = 0;
        for (i = 0; (ch = rap->login->token[i]) != '\0'; i = rap->size) {
            rap->data[i] = ch;
            rap->size++;
        }
        rap->size++;
        rap->data[i] = '\0';
        rap->data[rap->size++] = type >> 8;
        rap->data[rap->size++] = type;
    }
}

void NHttpRap_BeginPost(NHttpRap *rap, HeapID heapId, u32 size, int type) {
    if (rap != NULL) {
        rap->data = allocConfigDSSoftwareFeature(heapId, size + 0x130, "nhttp_rap.c", 0x219);
        sys_memset(rap->data, 0, size + 0x130);
        NHttpRap_WriteHeader(rap, type);
    }
}

void NHttpRap_AddPostData(NHttpRap *rap, const void *data, u32 size) {
    if (rap != NULL) {
        sys_memcpy(data, rap->data + rap->size, size);
        rap->size += size;
    }
}

BOOL NHttpRap_SendValidate(NHttpRap *rap) {
    int request;
    int result;

    if (rap == NULL) {
        return FALSE;
    }
    if (func_ov189_021a076c(NHttpRap_Alloc, NHttpRap_Free, 12) != 0) {
        GFL_ASSERT(0);
        return FALSE;
    }
    sys_memset(rap->url, 0, 4);
    func_0207f7a4(rap->url, sRequests[NHTTP_RAP_REQUEST_VALIDATE].url);
    request =
        func_ov189_021a0e1c(sRequests[NHTTP_RAP_REQUEST_VALIDATE].url, sRequests[NHTTP_RAP_REQUEST_VALIDATE].method,
                            rap->answer, rap->answerSize, NHttpRap_RequestCallback, NULL);
    if (request == 0) {
        int error = func_ov189_021a07b0();

        GFL_ASSERT_MSG(request != 0, "Connection\x82\xaaNULL!, error=%d\n", error);
        return FALSE;
    }
    rap->connection = request;
    result = func_ov189_021a09ec(request, sRootCertificates, 1);
    if (result < 0) {
        GFL_ASSERT_MSG(0, " NHTTP_SetRootCA(%d)\n", result);
        return FALSE;
    }
    func_ov189_021a0854(request, rap->data, rap->size);
    return TRUE;
}

void NHttpRap_FreePostData(NHttpRap *rap) {
    if (rap != NULL) {
        if (rap->data != NULL) {
            func_02042ed0(rap->data);
        }
        rap->data = NULL;
        rap->size = 0;
    }
    sNHttpRap = NULL;
}

int NHttpRap_GetStatus(NHttpRap *rap) {
    if (rap != NULL) {
        return func_ov189_021a0900(rap->connection);
    }
    return 0;
}

void NHttpRap_SetAnswerBuffer(NHttpRap *rap, void *buffer, u32 size) {
    rap->answer = buffer;
    rap->answerSize = size;
}

void NHttpRap_ResetAnswerBuffer(NHttpRap *rap) {
    rap->answer = rap->body;
    rap->answerSize = NHTTP_RAP_BODY_SIZE;
}

u8 NHttpRap_GetCheckStatus(const void *body) {
    return *(const u8 *)body;
}

u32 NHttpRap_GetCheckResult(const void *body, int index) {
    const u8 *result = (const u8 *)body + 1 + index * 4;

    return (result[0] << 24) | (result[1] << 16) | (result[2] << 8) | result[3];
}

void *NHttpRap_GetCheckSignature(void *body, int index) {
    return (u8 *)body + 1 + index * 4;
}
