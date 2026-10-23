#include "dwc/common.h"
#include "dwc/ext.h"
#include "dwc/memfunc.h"
#include "dwc/nasfunc.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitrowifi/cps.h"
#include <errno.h>

// DWC's HTTP client (overlay 11, Thumb), which the login, the connection test, the profanity check and the service
// locator make their requests to the Nintendo server with: it builds a request, runs it in a thread of its own over CPS
// (with SSL for an https URL) and parses the answer's headers and Base64 form fields. The ROM embeds no name, so the
// file's name is ours, from what the code does; its globals' names are NitroDWC's own, as its libraries' symbols and
// ntrtwl's reconstruction of NitroDWC have them where the code shows them, and ours elsewhere

// The room for the fields of an answer and how many are used
typedef struct NasFieldTable {
    DWCHttpLabelValue *fields;
    int capacity;
    int count;
} NasFieldTable;

static BOOL DWCi_Http_Yield(DWCHttp *http);
static void DWCi_Http_InitCpsSocket(DWCHttp *http);
static CPSInAddr DWCi_Http_Resolve(DWCHttp *http);
static BOOL DWCi_Http_CheckHeaderReceived(DWCHttp *http);
static void DWCi_Http_Thread(void *arg);
static int DWCi_Http_WriteBasicHeader(DWCHttp *http);
static int DWCi_Http_CPSCallback(int result, DWCHttpSslConnection *con, int index);
static int DWCi_Http_AllocBuffer(DWCHttp *http, DWCHttpBuffer *buffer, int size);
static void DWCi_Http_FreeBuffer(DWCHttp *http, DWCHttpBuffer *buffer);
static int DWCi_Http_ReallocBuffer(DWCHttp *http, DWCHttpBuffer *buffer, int extra);
static int DWCi_Http_SetHostinfo(DWCHttp *http, const char *url);
static int DWCi_Http_AddResult(DWCHttp *http, NasFieldTable *table, const char *label, const char *value);
static void DWCi_Http_DestroyResult(DWCHttp *http, DWCHttpLabelValue *fields, int count);

// The root certificates that the HTTPS connections accept
CPSCaInfo *DWCi_NasRootCerts[12] = {
    &DWCi_RootCertVeriSignClass3G3,    &DWCi_RootCertVeriSignClass3G2,    &DWCi_RootCertVeriSignClass3,
    &DWCi_RootCertRsaSecureServer,     &DWCi_RootCertGlobalSign,          &DWCi_RootCertGteCyberTrustGlobal,
    &DWCi_RootCertGteCyberTrust,       &DWCi_RootCertBaltimoreCyberTrust, &DWCi_RootCertThawteServer,
    &DWCi_RootCertThawtePremiumServer, &DWCi_RootCertGlobalSignR2,        &DWCi_RootCertNintendo,
};
// Whether the request that runs leaves the server's certificate unchecked (a URL other than the retail server's)
int DWCi_HttpTestServer;

DWCHttpError DWC_Http_Create(DWCHttp *http, DWCHttpParam *config) {
    DWCAuthAlloc alloc = config->alloc;

    MI_CpuFill8(http, 0, sizeof(DWCHttp));
    http->content_len = -1;
    http->receivedbody_len = -1;
    http->param = *config;
    http->lowrecvbuf = alloc(0, 0xb68);
    if (http->lowrecvbuf == NULL) {
        http->error = 1;
        return 1;
    }
    http->lowsendbuf = alloc(0, 0x5ea);
    if (http->lowsendbuf == NULL) {
        http->error = 1;
        return 1;
    }
    if (DWCi_Http_AllocBuffer(http, &http->rep, http->param.len_recvbuf) == 0) {
        http->error = 1;
        return 1;
    }
    if (DWCi_Http_SetHostinfo(http, config->url) == 0) {
        http->error = 1;
        return 1;
    }
    http->error = DWCi_Http_WriteBasicHeader(http);
    if (http->error == 0) {
        http->initflag = 0xff;
    }
    return http->error;
}

DWCHttpError DWC_Http_FinishHeader(DWCHttp *http) {
    char length[8];
    int size;

    if (DWC_Http_Add_HeaderItem(http, "Connection", "close") != 0) {
        return TRUE;
    }
    size = strlen(strstr(http->req.buffer, "\r\n\r\n") + 4);
    if (size != 0) {
        OS_SNPrintf(length, 7, "%d", size);
        if (DWC_Http_Add_HeaderItem(http, "Content-Length", length) != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

void DWC_Http_StartThread(DWCHttp *http, u32 priority) {
    http->abort = 0;
    OS_InitMutex(&http->mutex);
    OS_InitMutex(&http->content_len_mutex);
    if (http->param.ignoreca == 1) {
        DWCi_HttpTestServer = 1;
    } else {
        DWCi_HttpTestServer = 0;
    }
    if (http->thread.id == 0 || OS_IsThreadTerminated(&http->thread)) {
        OS_CreateThread(&http->thread, DWCi_Http_Thread, http, http->stack + 0x1000, 0x1000, priority);
        OS_WakeupThreadDirect(&http->thread);
    }
}

void DWC_Http_Abort(DWCHttp *http) {
    if (http->initflag == 0xff) {
        OS_LockMutex(&http->mutex);
        http->abort = 1;
        OS_UnlockMutex(&http->mutex);
    }
}

static BOOL DWCi_Http_Yield(DWCHttp *http) {
    if (http->ssl_enabled == 1) {
        OS_GetLowEntropyData(http->lowentropydata);
        CPS_SslAddRandomSeed(http->lowentropydata, 0x20);
    }
    OS_LockMutex(&http->mutex);
    if (http->abort == 1) {
        OS_UnlockMutex(&http->mutex);
        return FALSE;
    }
    OS_UnlockMutex(&http->mutex);
    OS_Sleep(10);
    return TRUE;
}

static void DWCi_Http_InitCpsSocket(DWCHttp *http) {
    MI_CpuFill8(&http->soc, 0, sizeof(CPSSoc));
    http->soc.rcvbuf.size = 0xb68;
    http->soc.rcvbuf.data = http->lowrecvbuf;
    http->soc.sndbuf.size = 0x5ea;
    http->soc.sndbuf.data = http->lowsendbuf;
    CPS_SocRegister(&http->soc);
}

static CPSInAddr DWCi_Http_Resolve(DWCHttp *http) {
    return CPS_Resolve(http->hostname);
}

static BOOL DWCi_Http_CheckHeaderReceived(DWCHttp *http) {
    DWCHttpBuffer *response = &http->rep;
    char *text = response->buffer;
    char *body;
    char *lineEnd;
    char *lengthText;
    char saved;
    int labelLength;

    if (strstr(text, "\r\n\r\n") == NULL) {
        return FALSE;
    }
    body = strstr(text, "\r\n\r\n") + 4;
    OS_LockMutex(&http->content_len_mutex);
    http->receivedbody_len = response->write_index - body;
    OS_UnlockMutex(&http->content_len_mutex);
    lengthText = strstr(response->buffer, "Content-Length: ");
    if (lengthText == NULL) {
        return TRUE;
    }
    labelLength = strlen("Content-Length: ");
    lineEnd = strstr(lengthText + labelLength, "\r\n");
    saved = *lineEnd;
    *lineEnd = 0;
    OS_LockMutex(&http->content_len_mutex);
    http->content_len = atol(lengthText + labelLength);
    OS_UnlockMutex(&http->content_len_mutex);
    *lineEnd = saved;
    return TRUE;
}

static void DWCi_Http_Thread(void *arg) {
    DWCHttp *http = arg;
    CPSSoc *soc = &http->soc;
    DWCHttpSslConnection *con = &http->con;
    DWCHttpBuffer *response = &http->rep;
    u32 copyLength;
    CPSInAddr ip;
    u8 *data;
    s32 length;
    u64 lastData;
    u64 now;
    int timeout;
    u32 connectError;
    int headersDone = 0;

    timeout = http->param.timeout;
    if (timeout <= 0) {
        timeout = 60000;
    }
    DWCi_Http_InitCpsSocket(http);
    ip = DWCi_Http_Resolve(http);
    if (ip == 0) {
        http->error = 2;
        return;
    }
    http->hostip = ip;
    CPS_SocUse();
    if (http->ssl_enabled == 1) {
        MI_CpuFill8(con, 0, sizeof(DWCHttpSslConnection));
        con->validateCertificate = DWCi_Http_CPSCallback;
        con->hostname = http->hostname;
        soc->con = con;
        CPS_SetRootCa(DWCi_NasRootCerts, 12);
        CPS_SetSsl(TRUE);
    }
    CPS_SocBind(0, http->port, ip);
    connectError = CPS_TcpConnect();
    if (connectError != 0) {
        http->error = 3;
        CPS_SocRelease();
        CPS_SocUnRegister();
        return;
    }
    data = (u8 *)http->req.buffer;
    length = CPS_SocWrite(data, strlen((const char *)data));
    if (length <= 0) {
        http->error = 5;
        goto cleanup;
    }
    CPS_SocFlush();
    if (!DWCi_Http_Yield(http)) {
        http->error = 7;
        goto cleanup;
    }
    response->write_index = response->buffer;
    response->buffer_tail = response->buffer + response->length;
    lastData = OS_GetTick();
    for (;;) {
        if (CPSMyIp == 0) {
            http->error = 5;
            goto cleanup;
        }
        length = CPS_SocGetLength();
        if (length < 0) {
            break;
        }
        if (length > 0) {
            lastData = OS_GetTick();
            data = CPS_SocRead((u32 *)&length);
            if (data == NULL) {
                break;
            }
            copyLength = (length < response->buffer_tail - 1 - response->write_index)
                             ? length
                             : response->buffer_tail - 1 - response->write_index;
            MI_CpuCopy8(data, response->write_index, copyLength);
            response->write_index += copyLength;
            *response->write_index = 0;
            if (headersDone == 1) {
                OS_LockMutex(&http->content_len_mutex);
                http->receivedbody_len += copyLength;
                OS_UnlockMutex(&http->content_len_mutex);
            } else {
                headersDone = DWCi_Http_CheckHeaderReceived(http);
            }
            if (length > copyLength) {
                CPS_SocConsume(length);
                break;
            }
            CPS_SocConsume(copyLength);
        }
        if (http->content_len >= 0 && http->receivedbody_len >= http->content_len) {
            break;
        }
        now = OS_GetTick();
        if (OS_TicksToMilliSeconds(now - lastData) > timeout) {
            http->error = 6;
            goto cleanup;
        }
        if (!DWCi_Http_Yield(http)) {
            http->error = 7;
            goto cleanup;
        }
    }
    CPS_TcpShutdown();
    CPS_TcpClose();
    CPS_SocRelease();
    CPS_SocUnRegister();
    http->error = 8;
    return;
cleanup:
    CPS_TcpShutdown();
    CPS_TcpClose();
    CPS_SocRelease();
    CPS_SocUnRegister();
}

void DWC_Http_Destroy(DWCHttp *http) {
    DWCAuthFree free = http->param.free;

    if (http != NULL) {
        DWCi_Http_DestroyResult(http, http->labelvalue, 0x20);
        DWCi_Http_FreeBuffer(http, &http->rep);
        DWCi_Http_FreeBuffer(http, &http->req);
        if (http->lowrecvbuf != NULL) {
            free(0, http->lowrecvbuf, 0);
            http->lowrecvbuf = NULL;
        }
        if (http->lowsendbuf != NULL) {
            free(0, http->lowsendbuf, 0);
            http->lowsendbuf = NULL;
        }
        MI_CpuFill8(http, 0, sizeof(DWCHttp));
    }
}

static int DWCi_Http_WriteBasicHeader(DWCHttp *http) {
    DWCHttpBuffer *request = &http->req;
    const char *format = "POST /%s HTTP/1.0\r\nContent-type: application/x-www-form-urlencoded\r\nHost: %s\r\n\r\n";
    int length;

    if (http->param.action != DWCHTTP_POST) {
        format = "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n";
    }
    length = strlen(http->hostname) + (strlen(format) - 4 + strlen(http->filepath));
    if (DWCi_Http_AllocBuffer(http, &http->req, length + 0x400) != 1) {
        return 1;
    }
    request->write_index += OS_SNPrintf(request->write_index, request->length, format, http->filepath, http->hostname);
    return 0;
}

DWCHttpError DWC_Http_Add_HeaderItem(DWCHttp *http, char *name, char *value) {
    int length;
    DWCHttpBuffer *request = &http->req;
    char *line;
    char saved;

    length = strlen(value) + (strlen("%s: %s\r\n") - 4 + strlen(name));
    if (length + 1 > request->buffer_tail - request->write_index) {
        if (DWCi_Http_ReallocBuffer(http, request, length - (request->buffer_tail - request->write_index) + 1) == 0) {
            return 1;
        }
    }
    line = strstr(request->buffer, "\r\n\r\n") + 2;
    saved = *line;
    memmove(line + length, line, strlen(line) + 1);
    line[OS_SNPrintf(line, length + 1, "%s: %s\r\n", name, value)] = saved;
    request->write_index += length;
    return 0;
}

DWCHttpError DWC_Http_Add_PostBase64Item(DWCHttp *http, const char *name, const char *value, unsigned long size) {
    DWCHttpBuffer *request = &http->req;
    const char *format = "%s=";
    int encoded;
    int length;
    int room;

    if (http->num_postitem != 0) {
        format = "&%s=";
    }
    http->num_postitem++;
    encoded = DWC_Base64Encode(value, size, NULL, 0);
    length = encoded + (strlen(format) - 2 + strlen(name));
    room = request->buffer_tail - request->write_index;
    if (length > room) {
        if (DWCi_Http_ReallocBuffer(http, request, length - room + 1) == 0) {
            return 1;
        }
        room = request->buffer_tail - request->write_index;
    }
    request->write_index += OS_SNPrintf(request->write_index, room, format, name);
    if (DWC_Base64Encode(value, size, request->write_index, request->buffer_tail - request->write_index - 1) < 0) {
        return 1;
    }
    request->write_index += encoded;
    *request->write_index = 0;
    return 0;
}

DWCHttpError DWC_Http_Add_Body(DWCHttp *http, const char *text) {
    int length;
    DWCHttpBuffer *request = &http->req;
    int room;
    int written;

    length = strlen(text);
    room = request->buffer_tail - request->write_index;
    if (length > room) {
        if (DWCi_Http_ReallocBuffer(http, request, length - room + 1) == 0) {
            return 1;
        }
        room = request->buffer_tail - request->write_index;
    }
    written = OS_SNPrintf(request->write_index, room, "%s", text);
    if (written != length) {
        return 1;
    }
    request->write_index += written;
    return 0;
}

static int DWCi_Http_CPSCallback(int result, DWCHttpSslConnection *con, int index) {
    if (result & 0x8000) {
        result &= ~0x8000;
    }
    return result;
}

static int DWCi_Http_AllocBuffer(DWCHttp *http, DWCHttpBuffer *buffer, int size) {
    DWCAuthAlloc alloc = http->param.alloc;

    if (size == 0) {
        return 0;
    }
    buffer->buffer = alloc(0, size);
    if (buffer->buffer == NULL) {
        return 0;
    }
    buffer->write_index = buffer->buffer;
    buffer->buffer_tail = buffer->buffer + size;
    buffer->length = size;
    return 1;
}

static void DWCi_Http_FreeBuffer(DWCHttp *http, DWCHttpBuffer *buffer) {
    DWCAuthFree free = http->param.free;

    if (buffer->buffer != NULL) {
        free(0, buffer->buffer, 0);
    }
    MI_CpuFill8(buffer, 0, sizeof(DWCHttpBuffer));
}

static int DWCi_Http_ReallocBuffer(DWCHttp *http, DWCHttpBuffer *buffer, int extra) {
    DWCAuthAlloc alloc = http->param.alloc;
    DWCAuthFree free = http->param.free;
    char *start;

    if (extra <= 0) {
        return 0;
    }
    start = alloc(0, buffer->length + extra);
    if (start == NULL) {
        return 0;
    }
    MI_CpuCopy8(buffer->buffer, start, buffer->length);
    free(0, buffer->buffer, 0);
    if (start == NULL) {
        return 0;
    }
    buffer->write_index += start - buffer->buffer;
    buffer->buffer = start;
    buffer->length += extra;
    buffer->buffer_tail = start + buffer->length;
    return 1;
}

static int DWCi_Http_SetHostinfo(DWCHttp *http, const char *url) {
    char *port = NULL;
    char *found;

    if (strlen(url) >= 0x100) {
        return 0;
    }
    strncpy(http->url, url, 0x100);
    if (strlen(url) != strlen(http->url)) {
        return 0;
    }
    if (strstr(http->url, "http://") != NULL) {
        http->hostname = http->url + 7;
        http->ssl_enabled = 0;
        http->port = 80;
    } else {
        found = strstr(http->url, "https://");
        if (found == NULL) {
            return 0;
        }
        http->hostname = found + 8;
        http->ssl_enabled = 1;
        http->port = 443;
    }
    found = strstr(http->hostname, ":");
    if (found != NULL) {
        port = found + 1;
        *found = 0;
    }
    found = strstr(http->hostname, "/");
    if (found == NULL) {
        http->filepath = NULL;
    } else {
        *found = 0;
        http->filepath = found + 1;
    }
    if (port != NULL) {
        http->port = atol(port);
    }
    return 1;
}

static int DWCi_Http_AddResult(DWCHttp *http, NasFieldTable *table, const char *name, const char *value) {
    DWCAuthAlloc alloc = http->param.alloc;
    DWCAuthFree free = http->param.free;

    if (table->count > table->capacity) {
        return 0;
    }
    table->fields[table->count].label = alloc(0, strlen(name) + 1);
    if (table->fields[table->count].label != NULL) {
        table->fields[table->count].value = alloc(0, strlen(value) + 1);
        if (table->fields[table->count].value != NULL) {
            strcpy(table->fields[table->count].label, name);
            strcpy(table->fields[table->count].value, value);
            table->count++;
            return 1;
        }
    }
    if (table->fields[table->count].label != NULL) {
        free(0, table->fields[table->count].label, 0);
        table->fields[table->count].label = NULL;
    }
    if (table->fields[table->count].value != NULL) {
        free(0, table->fields[table->count].value, 0);
        table->fields[table->count].value = NULL;
    }
    return 0;
}

BOOL DWC_Http_ParseResult(DWCHttp *http, BOOL headersOnly) {
    NasFieldTable table;
    char savedColon;
    char savedEnd;
    char *response;
    char *bodyStart;
    char *bodyEnd;
    char *space;
    char *status;
    char *line;
    char *name;
    char saved;
    char savedEquals;

    table.fields = http->labelvalue;
    table.capacity = 32;
    table.count = 0;
    MI_CpuFill8(http->labelvalue, 0, sizeof(http->labelvalue));
    response = http->rep.buffer;
    bodyStart = strstr(response, "\r\n\r\n");
    if (bodyStart == NULL) {
        return 0;
    }
    bodyEnd = bodyStart + 4 + strlen(bodyStart + 4);
    space = strstr(response, " ");
    if (space == NULL) {
        return 0;
    }
    status = space + 1;
    saved = status[3];
    status[3] = 0;
    if (DWCi_Http_AddResult(http, &table, "httpresult", status) != 1) {
        status[3] = saved;
        return 0;
    }
    status[3] = saved;
    if (headersOnly == 1 || strncmp(status, "200", 3) != 0) {
        return 1;
    }
    line = strstr(status + 4, "\r\n");
    if (line == NULL) {
        return 0;
    }
    line += 2;
    while (line[0] != '\r' && line[1] != '\n') {
        char *colon;
        char *eol;
        char *value;

        colon = strstr(line, ": ");
        if (colon == NULL) {
            break;
        }
        savedColon = *colon;
        *colon = 0;
        value = colon + 2;
        eol = strstr(value, "\r\n");
        if (eol == NULL) {
            *colon = savedColon;
            break;
        }
        savedEnd = *eol;
        *eol = 0;
        if (DWCi_Http_AddResult(http, &table, line, value) != 1) {
            *colon = savedColon;
            *eol = savedEnd;
            return 0;
        }
        line = value + strlen(value) + 2;
        *colon = savedColon;
        *eol = savedEnd;
    }
    name = bodyStart + 4;
    while (name < bodyEnd) {
        char *equals;
        char *amp;
        char *value;

        equals = strstr(name, "=");
        if (equals == NULL) {
            break;
        }
        savedEquals = *equals;
        *equals = 0;
        value = equals + 1;
        amp = strstr(value, "&");
        if (amp == NULL) {
            amp = strstr(value, "\r\n");
        }
        if (amp != NULL) {
            savedEnd = *amp;
            *amp = 0;
        }
        if (DWCi_Http_AddResult(http, &table, name, value) != 1) {
            *equals = savedEquals;
            if (amp != NULL) {
                *amp = savedEnd;
            }
            return 0;
        }
        name = value + strlen(value) + 1;
        *equals = savedEquals;
        if (amp != NULL) {
            *amp = savedEnd;
        }
    }
    return 1;
}

char *DWC_Http_GetResult(DWCHttp *http, char *name) {
    int i;

    for (i = 0; i < 32; i++) {
        if (http->labelvalue[i].label == NULL) {
            break;
        }
        if (strcmp(name, http->labelvalue[i].label) == 0) {
            return http->labelvalue[i].value;
        }
    }
    return NULL;
}

int DWC_Http_GetBase64DecodedResult(DWCHttp *http, char *name, char *dest, unsigned long size) {
    const char *value = DWC_Http_GetResult(http, name);
    u32 length;

    if (value == NULL) {
        return 0;
    }
    length = DWC_Base64Decode(value, strlen(value), dest, size);
    if (length != -1 && length < size) {
        dest[length] = 0;
    }
    return length;
}

BOOL DWC_Http_GetRawResult(DWCHttp *http, char *name, char *dest, int size) {
    const char *value = DWC_Http_GetResult(http, name);
    int length;

    if (value == NULL) {
        return 0;
    }
    length = strlen(value);
    if (length >= size) {
        return 0;
    }
    strcpy(dest, value);
    return 1;
}

static void DWCi_Http_DestroyResult(DWCHttp *http, DWCHttpLabelValue *fields, int count) {
    DWCAuthFree free = http->param.free;
    int i;

    for (i = 0; i < count; i++) {
        if (fields[i].label != NULL) {
            free(0, fields[i].label, 0);
            fields[i].label = NULL;
        }
        if (fields[i].value != NULL) {
            free(0, fields[i].value, 0);
            fields[i].value = NULL;
        }
    }
}
