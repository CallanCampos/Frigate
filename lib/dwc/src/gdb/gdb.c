#include "dwc/gdb.h"
#include "dwc/dwc.h"
#include "dwc/ext.h"
#include <stdarg.h>

// Nintendo's Wi-Fi Connection library's record database client (see dwc/gdb.h). It keeps two requests' worth of
// what a request was asked, one for the request that runs and one for the next, and one function that the Sake calls
// when any request ends, which logs it and gives the answer to the function the caller passed.

// The operations the library has, as the texts of its log name them. Only some have calls in the ROM
typedef void (*GdbRecordsCallback)(int recordNum, SAKEField **records, void *work);

typedef enum GdbOperation {
    GDB_OPERATION_NONE,
    GDB_OPERATION_INITIALIZE,
    GDB_OPERATION_CREATE_RECORD,
    GDB_OPERATION_DELETE_RECORD,
    GDB_OPERATION_UPDATE_RECORD,
    GDB_OPERATION_DOWNLOAD_FILE,
    GDB_OPERATION_UPLOAD_FILE,
    GDB_OPERATION_GET_MY_RECORDS,
    GDB_OPERATION_GET_RECORD_LIMIT,
    GDB_OPERATION_GET_SPECIFIC_RECORDS,
    GDB_OPERATION_RATE_RECORD,
    GDB_OPERATION_SEARCH_RECORDS,
    GDB_OPERATION_GET_RANDOM_RECORD,
    GDB_OPERATION_GET_RECORD_COUNT,
    GDB_OPERATION_NUM,
} GdbOperation;

// What a request was asked, which the answer's log and function need: the Sake's request, then what its operation
// takes (the Sake's input first, the rest after it)
typedef struct GdbRequest {
    SAKERequest request;
    union {
        struct {
            SAKECreateRecordInput input;
            int *recordId;
        } create;
        struct {
            const char *table;
            int recordId;
        } delete;
        SAKEUpdateRecordInput update;
        struct {
            SAKEGetMyRecordsInput input;
            GdbRecordsCallback func;
        } getMy;
        struct {
            const char *table;
            int *limitNum;
            int *ownedNum;
        } getRecordLimit;
        struct {
            SAKEGetSpecificRecordsInput input;
            GdbRecordsCallback func;
        } getSpecific;
        struct {
            const char *table;
            int recordId;
            u8 rating;
        } rate;
        struct {
            u8 input[0x30];
            GdbRecordsCallback func;
        } search;
        struct {
            const char *table;
            const char **fieldNames;
            int fieldNum;
            const char *filter;
            GdbRecordsCallback func;
        } getRandom;
        struct {
            const char *table;
            int unk_08;
            int unk_0c;
            int *count;
        } getRecordCount;
    } u;
} GdbRequest;

// The library: the Sake, the state, the operation running, the result and which request is the current
typedef struct Gdb {
    int unk_00;
    SAKE sake;
    int state;
    GdbOperation operation;
    int result;
    int requestIndex;
} Gdb;

// The part of the player's user data that holds the profile ID
typedef struct GdbUserData {
    u8 unk_00[0x1c];
    s32 profileId;
} GdbUserData;

static void Gdb_Printf(int line, const char *format, ...);
static BOOL Gdb_CheckReady(int *error);
static void Gdb_RunLibrary(void);
static void Gdb_OnResult(SAKE sake, SAKERequest request, SAKERequestResult error, void *input, void *output,
                         void *userData);
static int Gdb_ConvertStartupError(int sakeError, int *type);
static int Gdb_ConvertRequestError(int sakeError, int *type);
static int Gdb_GetErrorCode(int step, int error);
static int Gdb_SetStartupError(int sakeError);
static int Gdb_SetRequestError(int operation, int sakeError);
static int Gdb_ConvertResultError(int sakeResult, int *type);
static int Gdb_GetResultErrorCode(int operation, int error);
static int Gdb_SetResultError(int operation, int sakeResult);
static void Gdb_SetOperation(int operation);
static void Gdb_NextRequest(void);
static void Gdb_SetUrls(int server);

// The names of the operations, for the log
// Padded strings and the pointer table share the ROM's .data block.
// clang-format off
static char sOperationNameNone[8] = "none";
static char sOperationNameUploadFile[12] = "upload file";
static char sOperationNameInitialize[12] = "initialize";
static char sOperationNameGetRecords[12] = "get records";
static char sOperationNameRateRecord[12] = "rate record";
static char sOperationNameDeleteRecord[16] = "delete record";
static char sOperationNameSearchRecords[16] = "search records";
static char sOperationNameGetMyRecords[16] = "get my records";
static char sOperationNameCreateRecord[16] = "create record";
static char sOperationNameUpdateRecord[16] = "update record";
static char sOperationNameDownloadFile[16] = "download file";
static char sOperationNameGetRecordLimit[20] = "get record limit";
static char sOperationNameGetRecordCount[20] = "get record count";
static char sOperationNameGetRandomRecord[20] = "get random record";

static const char *sOperationNames[GDB_OPERATION_NUM] = {
    [GDB_OPERATION_NONE] = sOperationNameNone,
    [GDB_OPERATION_INITIALIZE] = sOperationNameInitialize,
    [GDB_OPERATION_CREATE_RECORD] = sOperationNameCreateRecord,
    [GDB_OPERATION_DELETE_RECORD] = sOperationNameDeleteRecord,
    [GDB_OPERATION_UPDATE_RECORD] = sOperationNameUpdateRecord,
    [GDB_OPERATION_DOWNLOAD_FILE] = sOperationNameDownloadFile,
    [GDB_OPERATION_UPLOAD_FILE] = sOperationNameUploadFile,
    [GDB_OPERATION_GET_MY_RECORDS] = sOperationNameGetMyRecords,
    [GDB_OPERATION_GET_RECORD_LIMIT] = sOperationNameGetRecordLimit,
    [GDB_OPERATION_GET_SPECIFIC_RECORDS] = sOperationNameGetRecords,
    [GDB_OPERATION_RATE_RECORD] = sOperationNameRateRecord,
    [GDB_OPERATION_SEARCH_RECORDS] = sOperationNameSearchRecords,
    [GDB_OPERATION_GET_RANDOM_RECORD] = sOperationNameGetRandomRecord,
    [GDB_OPERATION_GET_RECORD_COUNT] = sOperationNameGetRecordCount,
};

// clang-format on

static GdbRequest sGdbRequests[2];
static Gdb sGdb;

int Gdb_Startup(int gameId, const DWCUserData *userData, int server) {
    SakeLoginTicket ticket;
    int error;

    if (sGdb.state != 0) {
        return 0x12;
    }
    if (!DWCi_GetLoginTicket(&ticket)) {
        Gdb_Printf(282, "DWCi_GetLoginTicket failed.\n");
        DWCi_SetError(8, Gdb_GetErrorCode(1, 4));
        return 4;
    }
    gsCoreInitialize();
    error = sakeStartup(&sGdb.sake);
    if (error != 0) {
        Gdb_Printf(295, "sakeStartup failed.[%d]\n", error);
        Gdb_RunLibrary();
        return Gdb_SetStartupError(error);
    }
    Sake_SetGame(sGdb.sake, gcd_gamename, gameId, gcd_secret_key);
    Sake_SetLoginTicket(sGdb.sake, ((const GdbUserData *)userData)->profileId, &ticket);
    Gdb_SetUrls(server);
    DWC_InitGHTTP(NULL);
    sGdb.state = GDB_STATE_IDLE;
    sGdb.result = 0;
    sGdb.operation = 0;
    sGdb.requestIndex = 0;
    return 0;
}

static inline GdbRequest *Gdb_GetCurrentRequest(void) {
    return &sGdbRequests[sGdb.requestIndex];
}

int Gdb_Cancel(void) {
    if (sGdb.state != GDB_STATE_BUSY) {
        Gdb_Printf(335, "not in async process.\n");
        return 0x11;
    }
    sGdb.state = GDB_STATE_CANCELING;
    switch (sGdb.operation) {
    case GDB_OPERATION_CREATE_RECORD:
        Sake_CancelRequest(sGdb.sake, Gdb_GetCurrentRequest()->request);
        break;
    case GDB_OPERATION_DELETE_RECORD:
        Sake_CancelRequest(sGdb.sake, Gdb_GetCurrentRequest()->request);
        break;
    case GDB_OPERATION_UPDATE_RECORD:
        Sake_CancelRequest(sGdb.sake, Gdb_GetCurrentRequest()->request);
        break;
    case GDB_OPERATION_GET_MY_RECORDS:
        Sake_CancelRequest(sGdb.sake, Gdb_GetCurrentRequest()->request);
        break;
    case GDB_OPERATION_GET_RECORD_LIMIT:
        Sake_CancelRequest(sGdb.sake, Gdb_GetCurrentRequest()->request);
        break;
    case GDB_OPERATION_GET_SPECIFIC_RECORDS:
        Sake_CancelRequest(sGdb.sake, Gdb_GetCurrentRequest()->request);
        break;
    case GDB_OPERATION_RATE_RECORD:
        Sake_CancelRequest(sGdb.sake, Gdb_GetCurrentRequest()->request);
        break;
    case GDB_OPERATION_SEARCH_RECORDS:
        Sake_CancelRequest(sGdb.sake, Gdb_GetCurrentRequest()->request);
        break;
    case GDB_OPERATION_GET_RANDOM_RECORD:
        Sake_CancelRequest(sGdb.sake, Gdb_GetCurrentRequest()->request);
        break;
    case GDB_OPERATION_GET_RECORD_COUNT:
        Sake_CancelRequest(sGdb.sake, Gdb_GetCurrentRequest()->request);
        break;
    case GDB_OPERATION_DOWNLOAD_FILE:
        Gdb_Printf(385, "cancel ghttp. state[%d]\n", ghttpGetState((int)Gdb_GetCurrentRequest()->request));
        ghttpCancelRequest((int)Gdb_GetCurrentRequest()->request);
        break;
    case GDB_OPERATION_UPLOAD_FILE:
        Gdb_Printf(392, "cancel ghttp. state[%d]\n", ghttpGetState((int)Gdb_GetCurrentRequest()->u.delete.table));
        ghttpCancelRequest((int)Gdb_GetCurrentRequest()->u.delete.table);
        break;
    default:
        Gdb_Printf(399, "illegal process[%d].\n", sGdb.operation);
        break;
    }
    return 0;
}

int Gdb_Shutdown(void) {
    if (sGdb.state == GDB_STATE_OFF) {
        Gdb_Printf(422, "not initialized.\n");
        return 0x13;
    }
    if (sGdb.state == GDB_STATE_BUSY) {
        Gdb_Printf(427, "in async process.\n");
        return 0x14;
    }
    Gdb_ReleaseUrls();
    sGdb.state = GDB_STATE_OFF;
    DWC_ShutdownGHTTP();
    Sake_Free(sGdb.sake);
    Gdb_RunLibrary();
    return 0;
}

void Gdb_ReleaseUrls(void) {
    char uploadUrl[128];
    char downloadUrl[128];

    DWCi_RemoveGHTTPRootCA(gSakeStorageUrl);
    Sake_GetUploadUrl(sGdb.sake, uploadUrl);
    DWCi_RemoveGHTTPRootCA(uploadUrl);
    Sake_GetDownloadUrl(sGdb.sake, 1, downloadUrl);
    DWCi_RemoveGHTTPRootCA(downloadUrl);
}

void Gdb_Update(void) {
    if (gsCoreIsShutdown() != GSCore_SHUTDOWN_COMPLETE) {
        gsCoreThink(0);
    }
    DWC_ProcessGHTTP();
}

int Gdb_GetState(void) {
    if (DWCi_IsError()) {
        return 4;
    }
    return sGdb.state;
}

int Gdb_GetResult(void) {
    return sGdb.result;
}

int Gdb_CreateRecord(const char *table, GdbField *fields, int fieldNum, int *recordId) {
    GdbRequest *slot;
    SAKERequest request;
    int error;
    int checkError;

    if (Gdb_CheckReady(&checkError)) {
        return checkError;
    }
    Gdb_NextRequest();
    slot = &sGdbRequests[sGdb.requestIndex];
    slot->u.create.input.tableId = (char *)table;
    slot->u.create.input.fields = (SAKEField *)fields;
    slot->u.create.input.fieldNum = fieldNum;
    slot->u.create.recordId = recordId;
    error = 0;
    request = sakeCreateRecord(sGdb.sake, &slot->u.create.input, Gdb_OnResult, NULL);
    if (request == NULL) {
        error = Sake_GetLastError(sGdb.sake);
        Gdb_Printf(559, "sakeCreateRecord failed.[%d]\n", error);
        return Gdb_SetRequestError(2, error);
    }
    sGdbRequests[sGdb.requestIndex].request = request;
    Gdb_SetOperation(GDB_OPERATION_CREATE_RECORD);
    return error;
}

int Gdb_UpdateRecord(const char *table, int recordId, GdbField *fields, int fieldNum) {
    GdbRequest *slot;
    SAKERequest request;
    int error;
    int checkError;

    if (Gdb_CheckReady(&checkError)) {
        return checkError;
    }
    Gdb_NextRequest();
    slot = &sGdbRequests[sGdb.requestIndex];
    slot->u.update.tableId = (char *)table;
    slot->u.update.recordId = recordId;
    slot->u.update.fields = (SAKEField *)fields;
    slot->u.update.fieldNum = fieldNum;
    error = 0;
    request = sakeUpdateRecord(sGdb.sake, &slot->u.update, Gdb_OnResult, NULL);
    if (request == NULL) {
        error = Sake_GetLastError(sGdb.sake);
        Gdb_Printf(668, "sakeUpdateRecord failed.[%d]\n", error);
        return Gdb_SetRequestError(4, error);
    }
    sGdbRequests[sGdb.requestIndex].request = request;
    Gdb_SetOperation(GDB_OPERATION_UPDATE_RECORD);
    return error;
}

int Gdb_GetMyRecords(const char *table, const char **fieldNames, int fieldNum, GdbRecordsFunc func, void *work) {
    GdbRequest *slot;
    SAKERequest request;
    int error;
    int checkError;

    if (Gdb_CheckReady(&checkError)) {
        return checkError;
    }
    Gdb_NextRequest();
    slot = &sGdbRequests[sGdb.requestIndex];
    slot->u.getMy.input.tableId = (char *)table;
    slot->u.getMy.input.fieldNames = (char **)fieldNames;
    slot->u.getMy.input.fieldNum = fieldNum;
    slot->u.getMy.func = (GdbRecordsCallback)func;
    request = sakeGetMyRecords(sGdb.sake, &slot->u.getMy.input, Gdb_OnResult, work);
    if (request == NULL) {
        error = Sake_GetLastError(sGdb.sake);
        Gdb_Printf(859, "sakeGetMyRecords failed.[%d]\n", error);
        return Gdb_SetRequestError(7, error);
    }
    sGdbRequests[sGdb.requestIndex].request = request;
    Gdb_SetOperation(GDB_OPERATION_GET_MY_RECORDS);
    return 0;
}

int Gdb_GetSpecificRecords(const char *table, int *recordIds, int recordNum, const char **fieldNames, int fieldNum,
                           GdbRecordsFunc func, void *work) {
    GdbRequest *slot;
    SAKERequest request;
    int error;
    int checkError;

    if (Gdb_CheckReady(&checkError)) {
        return checkError;
    }
    Gdb_NextRequest();
    slot = &sGdbRequests[sGdb.requestIndex];
    slot->u.getSpecific.input.tableId = (char *)table;
    slot->u.getSpecific.input.recordIds = recordIds;
    slot->u.getSpecific.input.recordNum = recordNum;
    slot->u.getSpecific.input.fieldNames = (char **)fieldNames;
    slot->u.getSpecific.input.fieldNum = fieldNum;
    slot->u.getSpecific.func = (GdbRecordsCallback)func;
    request = sakeGetSpecificRecords(sGdb.sake, &slot->u.getSpecific.input, Gdb_OnResult, work);
    if (request == NULL) {
        error = Sake_GetLastError(sGdb.sake);
        Gdb_Printf(970, "sakeGetSpecificRecords failed.[%d]\n", error);
        return Gdb_SetRequestError(9, error);
    }
    sGdbRequests[sGdb.requestIndex].request = request;
    Gdb_SetOperation(GDB_OPERATION_GET_SPECIFIC_RECORDS);
    return 0;
}

static void Gdb_Printf(int line, const char *format, ...) {
    char buffer[0x100];
    int length;
    va_list args;

    length = OS_SNPrintf(buffer, sizeof(buffer), "Gdb[%d]> ", line);
    va_start(args, format);
    OS_VSNPrintf(buffer + length, sizeof(buffer) - length, format, args);
    va_end(args);
}

static BOOL Gdb_CheckReady(int *error) {
    if (DWCi_IsError()) {
        Gdb_Printf(1384, "in dwc error.[%d]\n", DWC_GetLastError(0));
        if (error != NULL) {
            *error = 0x16;
        }
        return TRUE;
    }
    switch (sGdb.state) {
    case GDB_STATE_OFF:
        Gdb_Printf(1392, "not initialized.\n");
        if (error != NULL) {
            *error = 0x13;
        }
        return TRUE;
    case GDB_STATE_BUSY:
        Gdb_Printf(1397, "in async process.\n");
        if (error != NULL) {
            *error = 0x14;
        }
        return TRUE;
    case GDB_STATE_CANCELING:
        Gdb_Printf(1402, "in cancel process.\n");
        if (error != NULL) {
            *error = 0x17;
        }
        return TRUE;
    }
    return FALSE;
}

static void Gdb_RunLibrary(void) {
    gsCoreShutdown();
    gsCoreThink(0);
    if (gsCoreIsShutdown() != GSCore_SHUTDOWN_COMPLETE) {
        gsCoreThink(0);
    }
}

static void Gdb_OnResult(SAKE sake, SAKERequest request, SAKERequestResult error, void *input, void *output,
                         void *userData) {
    GdbRequest *slot;
    GdbOperation operation = sGdb.operation;
    int state = sGdb.state;

    if (state != GDB_STATE_BUSY && state != GDB_STATE_CANCELING) {
        return;
    }
    if (error == 0) {
        sGdb.result = 1;
        sGdb.state = GDB_STATE_IDLE;
        sGdb.operation = 0;
        switch (operation) {
        case GDB_OPERATION_CREATE_RECORD:
            slot = &sGdbRequests[sGdb.requestIndex];
            *slot->u.create.recordId = ((int *)output)[0];
            Gdb_Printf(1455, "%s[gdb=>%s field_num=>%d record_id=>%d].\n", sOperationNames[operation],
                       slot->u.create.input.tableId, slot->u.create.input.fieldNum, *slot->u.create.recordId);
            break;
        case GDB_OPERATION_DELETE_RECORD:
            slot = &sGdbRequests[sGdb.requestIndex];
            Gdb_Printf(1464, "%s[gdb=>%s record_id=>%d].\n", sOperationNames[operation], slot->u.delete.table,
                       slot->u.delete.recordId);
            break;
        case GDB_OPERATION_UPDATE_RECORD:
            slot = &sGdbRequests[sGdb.requestIndex];
            Gdb_Printf(1474, "%s[gdb=>%s record_id=>%d field_num=>%d].\n", sOperationNames[operation],
                       slot->u.update.tableId, slot->u.update.recordId, slot->u.update.fieldNum);
            break;
        case GDB_OPERATION_GET_MY_RECORDS:
            slot = &sGdbRequests[sGdb.requestIndex];
            Gdb_Printf(1483, "%s[record_num=>%d].\n", sOperationNames[operation],
                       ((SAKEGetMyRecordsOutput *)output)->recordNum);
            slot->u.getMy.func(((SAKEGetMyRecordsOutput *)output)->recordNum,
                               ((SAKEGetMyRecordsOutput *)output)->records, userData);
            break;
        case GDB_OPERATION_GET_RECORD_LIMIT:
            slot = &sGdbRequests[sGdb.requestIndex];
            *slot->u.getRecordLimit.limitNum = ((int *)output)[0];
            *slot->u.getRecordLimit.ownedNum = ((int *)output)[1];
            Gdb_Printf(1496, "%s[gdb=>%s limit_num=>%d owned_num=>%d].\n", sOperationNames[operation],
                       slot->u.getRecordLimit.table, *slot->u.getRecordLimit.limitNum,
                       *slot->u.getRecordLimit.ownedNum);
            break;
        case GDB_OPERATION_GET_SPECIFIC_RECORDS:
            slot = &sGdbRequests[sGdb.requestIndex];
            Gdb_Printf(1505, "%s[record_num=>%d].\n", sOperationNames[operation],
                       ((SAKEGetSpecificRecordsOutput *)output)->recordNum);
            slot->u.getSpecific.func(((SAKEGetSpecificRecordsOutput *)output)->recordNum,
                                     ((SAKEGetSpecificRecordsOutput *)output)->records, userData);
            break;
        case GDB_OPERATION_RATE_RECORD:
            slot = &sGdbRequests[sGdb.requestIndex];
            Gdb_Printf(1516, "%s[gdb=>%s record_id=>%d rating=>%d].\n", sOperationNames[operation], slot->u.rate.table,
                       slot->u.rate.recordId, slot->u.rate.rating);
            break;
        case GDB_OPERATION_SEARCH_RECORDS:
            slot = &sGdbRequests[sGdb.requestIndex];
            Gdb_Printf(1525, "%s[record_num=>%d].\n", sOperationNames[operation],
                       ((SAKESearchForRecordsOutput *)output)->recordNum);
            slot->u.search.func(((SAKESearchForRecordsOutput *)output)->recordNum,
                                ((SAKESearchForRecordsOutput *)output)->records, userData);
            break;
        case GDB_OPERATION_GET_RANDOM_RECORD:
            slot = &sGdbRequests[sGdb.requestIndex];
            if (((int *)output)[0] != 0) {
                Gdb_Printf(1535, "%s[record_num=>1].\n", sOperationNames[operation]);
                slot->u.getRandom.func(1, output, userData);
            } else {
                Gdb_Printf(1540, "%s[record_num=>0].\n", sOperationNames[operation]);
                slot->u.getRandom.func(0, NULL, userData);
            }
            break;
        case GDB_OPERATION_GET_RECORD_COUNT:
            slot = &sGdbRequests[sGdb.requestIndex];
            *slot->u.getRecordCount.count = ((int *)output)[0];
            Gdb_Printf(1549, "%s[count=>%d].\n", sOperationNames[operation], *slot->u.getRecordCount.count);
            break;
        default:
            Gdb_Printf(1555, "illegal process[%d].\n", operation);
            break;
        }
    } else if (error == SAKERequestResult_CANCELED) {
        Gdb_Printf(1562, "%s canceled.[%d]\n", sOperationNames[sGdb.operation], error);
        sGdb.result = 0x20;
        sGdb.state = GDB_STATE_IDLE;
        sGdb.operation = 0;
    } else {
        Gdb_Printf(1569, "%s failed.[%d]\n", sOperationNames[sGdb.operation], error);
        sGdb.result = Gdb_SetResultError(sGdb.operation, error);
        sGdb.state = GDB_STATE_IDLE;
        sGdb.operation = 0;
    }
}

static int Gdb_ConvertStartupError(int sakeError, int *type) {
    switch (sakeError) {
    case 0:
        *type = 0;
        return 0;
    case 1:
        *type = 8;
        return 2;
    case 2:
        *type = 8;
        return 3;
    case 3:
        *type = 9;
        return 1;
    default:
        return 0x11;
    }
}

static int Gdb_ConvertRequestError(int sakeError, int *type) {
    *type = 0x13;
    switch (sakeError) {
    case 0:
        *type = 0;
        return 0;
    case 1:
        return 5;
    case 2:
        *type = 9;
        return 1;
    case 3:
        return 6;
    case 4:
        return 7;
    case 5:
        return 8;
    case 6:
        return 9;
    case 7:
        return 10;
    case 8:
        return 11;
    case 9:
        return 12;
    case 10:
        return 13;
    case 11:
        return 14;
    case 12:
        return 15;
    case 13:
        return 16;
    case 14:
        return 17;
    default:
        return 0x11;
    }
}

static int Gdb_GetErrorCode(int step, int error) {
    return -40000 - (error + step * 100);
}

static int Gdb_SetStartupError(int sakeError) {
    int type;
    int error = Gdb_ConvertStartupError(sakeError, &type);

    if (error != 0) {
        Gdb_Printf(1755, "DWCGdbError[%d]\n", error);
        DWCi_SetError(type, Gdb_GetErrorCode(1, error));
    }
    return error;
}

static int Gdb_SetRequestError(int operation, int sakeError) {
    int type;
    int error = Gdb_ConvertRequestError(sakeError, &type);

    if (error != 0) {
        Gdb_Printf(1772, "DWCGdbError[%d]\n", error);
        DWCi_SetError(type, Gdb_GetErrorCode(operation, error));
    }
    return error;
}

static int Gdb_ConvertResultError(int sakeResult, int *type) {
    *type = 0x13;
    switch (sakeResult) {
    case 0:
        *type = 0;
        return 1;
    case 1:
        return 2;
    case 2:
        return 3;
    case 3:
        return 4;
    case 4:
        return 5;
    case 5:
        return 6;
    case 6:
        *type = 9;
        return 7;
    case 7:
        return 8;
    case 8:
        return 9;
    case 9:
        return 10;
    case 10:
        return 11;
    case 11:
        return 12;
    case 12:
        return 13;
    case 13:
        return 14;
    case 14:
        return 15;
    case 15:
        return 16;
    case 16:
        return 17;
    case 17:
        return 18;
    case 18:
        return 0x13;
    case 19:
        return 0x14;
    case 20:
        return 0x15;
    case 21:
        return 0x1e;
    case 23:
        return 0x20;
    case 22:
        return 0x16;
    default:
        return 0x16;
    }
}

static int Gdb_GetResultErrorCode(int operation, int error) {
    return -40000 - (error + operation * 100 + 50);
}

static int Gdb_SetResultError(int operation, int sakeResult) {
    int type;
    int error = Gdb_ConvertResultError(sakeResult, &type);

    if (error != 1) {
        Gdb_Printf(1883, "DWCGdbAsyncResult[%d]\n", error);
        DWCi_SetError(type, Gdb_GetResultErrorCode(operation, error));
    }
    return error;
}

static void Gdb_SetOperation(int operation) {
    sGdb.state = GDB_STATE_BUSY;
    sGdb.operation = operation;
    sGdb.result = 0;
}

static void Gdb_NextRequest(void) {
    if (sGdb.requestIndex == 0) {
        sGdb.requestIndex = 1;
    } else {
        sGdb.requestIndex = 0;
    }
}

static void Gdb_SetUrls(int server) {
    char uploadUrl[128];
    char downloadUrl[128];

    switch (server) {
    case 1:
        OS_SNPrintf(gSakeStorageUrl, 128,
                    "https://%s.secure2.sake.gs.nintendowifi.net/SakeStorageServer/StorageServer.asmx", gcd_gamename);
        DWCi_SetGHTTPRootCA(gSakeStorageUrl);
        OS_SNPrintf(uploadUrl, 128, "https://%s.secure2.sake.gs.nintendowifi.net/SakeFileServer/upload.aspx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(uploadUrl);
        OS_SNPrintf(downloadUrl, 128, "https://%s.secure2.sake.gs.nintendowifi.net/SakeFileServer/download.aspx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(downloadUrl);
        break;
    case 0:
    default:
        OS_SNPrintf(gSakeStorageUrl, 128,
                    "https://%s.secure.sake.gs.nintendowifi.net/SakeStorageServer/StorageServer.asmx", gcd_gamename);
        DWCi_SetGHTTPRootCA(gSakeStorageUrl);
        OS_SNPrintf(uploadUrl, 128, "https://%s.secure.sake.gs.nintendowifi.net/SakeFileServer/upload.aspx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(uploadUrl);
        OS_SNPrintf(downloadUrl, 128, "https://%s.secure.sake.gs.nintendowifi.net/SakeFileServer/download.aspx",
                    gcd_gamename);
        DWCi_SetGHTTPRootCA(downloadUrl);
        break;
    }
    Sake_SetUploadUrl(sGdb.sake, uploadUrl);
    Sake_SetDownloadUrl(sGdb.sake, downloadUrl);
}
