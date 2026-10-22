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
// file's name and its globals' are ours, from what the code does

// The room for the fields of an answer and how many are used
typedef struct NasFieldTable {
    DWCHttpLabelValue *fields;
    int capacity;
    int count;
} NasFieldTable;

static BOOL DWCi_Http_SleepUnlessAborted(DWCHttp *http);
static void DWCi_Http_RegisterSocket(DWCHttp *http);
static CPSInAddr DWCi_Http_ResolveHost(DWCHttp *http);
static BOOL DWCi_Http_ParseContentLength(DWCHttp *http);
static void DWCi_Http_Thread(void *arg);
static int DWCi_Http_MakeRequestLine(DWCHttp *http);
static int DWCi_Http_VerifyCertificate(int result, DWCHttpSslConnection *con, int index);
static int DWCi_Http_AllocBuffer(DWCHttp *http, DWCHttpBuffer *buffer, int size);
static void DWCi_Http_FreeBuffer(DWCHttp *http, DWCHttpBuffer *buffer);
static int DWCi_Http_GrowBuffer(DWCHttp *http, DWCHttpBuffer *buffer, int extra);
static int DWCi_Http_ParseUrl(DWCHttp *http, const char *url);
static int DWCi_Http_AddLabelValue(DWCHttp *http, NasFieldTable *table, const char *label, const char *value);
static void DWCi_Http_FreeLabelValues(DWCHttp *http, DWCHttpLabelValue *fields, int count);

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
    DWCHttpAlloc alloc = config->alloc;

    MI_CpuFill8(http, 0, sizeof(DWCHttp));
    http->contentLength = -1;
    http->bodyLength = -1;
    http->param = *config;
    http->rcvBuffer = alloc(0, 0xb68);
    if (http->rcvBuffer == NULL) {
        http->result = 1;
        return 1;
    }
    http->sndBuffer = alloc(0, 0x5ea);
    if (http->sndBuffer == NULL) {
        http->result = 1;
        return 1;
    }
    if (DWCi_Http_AllocBuffer(http, &http->response, http->param.bufferSize) == 0) {
        http->result = 1;
        return 1;
    }
    if (DWCi_Http_ParseUrl(http, config->url) == 0) {
        http->result = 1;
        return 1;
    }
    http->result = DWCi_Http_MakeRequestLine(http);
    if (http->result == 0) {
        http->started = 0xff;
    }
    return http->result;
}

DWCHttpError DWC_Http_FinishHeader(DWCHttp *http) {
    char length[8];
    int size;

    if (DWC_Http_Add_HeaderItem(http, "Connection", "close") != 0) {
        return TRUE;
    }
    size = strlen(strstr(http->request.start, "\r\n\r\n") + 4);
    if (size != 0) {
        OS_SNPrintf(length, 7, "%d", size);
        if (DWC_Http_Add_HeaderItem(http, "Content-Length", length) != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

void DWC_Http_StartThread(DWCHttp *http, u32 priority) {
    http->cancelled = 0;
    OS_InitMutex(&http->cancelLock);
    OS_InitMutex(&http->lock);
    if (http->param.ignoreCa == 1) {
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
    if (http->started == 0xff) {
        OS_LockMutex(&http->cancelLock);
        http->cancelled = 1;
        OS_UnlockMutex(&http->cancelLock);
    }
}

static BOOL DWCi_Http_SleepUnlessAborted(DWCHttp *http) {
    if (http->secure == 1) {
        OS_GetLowEntropyData(http->entropy);
        CPS_SslAddRandomSeed(http->entropy, 0x20);
    }
    OS_LockMutex(&http->cancelLock);
    if (http->cancelled == 1) {
        OS_UnlockMutex(&http->cancelLock);
        return FALSE;
    }
    OS_UnlockMutex(&http->cancelLock);
    OS_Sleep(10);
    return TRUE;
}

static void DWCi_Http_RegisterSocket(DWCHttp *http) {
    MI_CpuFill8(&http->soc, 0, sizeof(CPSSoc));
    http->soc.rcvbuf.size = 0xb68;
    http->soc.rcvbuf.data = http->rcvBuffer;
    http->soc.sndbuf.size = 0x5ea;
    http->soc.sndbuf.data = http->sndBuffer;
    CPS_SocRegister(&http->soc);
}

static CPSInAddr DWCi_Http_ResolveHost(DWCHttp *http) {
    return CPS_Resolve(http->host);
}

static BOOL DWCi_Http_ParseContentLength(DWCHttp *http) {
    DWCHttpBuffer *response = &http->response;
    char *text = response->start;
    char *body;
    char *lineEnd;
    char *lengthText;
    char saved;
    int labelLength;

    if (strstr(text, "\r\n\r\n") == NULL) {
        return FALSE;
    }
    body = strstr(text, "\r\n\r\n") + 4;
    OS_LockMutex(&http->lock);
    http->bodyLength = response->cur - body;
    OS_UnlockMutex(&http->lock);
    lengthText = strstr(response->start, "Content-Length: ");
    if (lengthText == NULL) {
        return TRUE;
    }
    labelLength = strlen("Content-Length: ");
    lineEnd = strstr(lengthText + labelLength, "\r\n");
    saved = *lineEnd;
    *lineEnd = 0;
    OS_LockMutex(&http->lock);
    http->contentLength = atol(lengthText + labelLength);
    OS_UnlockMutex(&http->lock);
    *lineEnd = saved;
    return TRUE;
}

static void DWCi_Http_Thread(void *arg) {
    DWCHttp *http = arg;
    CPSInAddr ip;
    u64 lastData;
    u32 length;
    int headersDone = 0;
    CPSSoc *soc = &http->soc;
    DWCHttpSslConnection *con = &http->con;
    DWCHttpBuffer *response = &http->response;
    int timeout = http->param.timeout;
    char *request;
    int sent;
    int room;
    int size;
    u8 *data;
    char *old;

    if (timeout <= 0) {
        timeout = 60000;
    }
    DWCi_Http_RegisterSocket(http);
    ip = DWCi_Http_ResolveHost(http);
    if (ip == 0) {
        http->result = 2;
        return;
    }
    http->ip = ip;
    CPS_SocUse();
    if (http->secure == 1) {
        MI_CpuFill8(con, 0, sizeof(DWCHttpSslConnection));
        con->validateCertificate = DWCi_Http_VerifyCertificate;
        con->hostname = http->host;
        soc->con = con;
        CPS_SetRootCa(DWCi_NasRootCerts, 12);
        CPS_SetSsl(TRUE);
    }
    CPS_SocBind(0, http->port, ip);
    if (CPS_TcpConnect() != 0) {
        http->result = 3;
        CPS_SocRelease();
        CPS_SocUnRegister();
        return;
    }
    request = http->request.start;
    sent = CPS_SocWrite((u8 *)request, strlen(request));
    if (sent <= 0) {
        http->result = 5;
        goto cleanup;
    }
    CPS_SocFlush();
    if (!DWCi_Http_SleepUnlessAborted(http)) {
        http->result = 7;
        goto cleanup;
    }
    response->cur = response->start;
    response->end = response->start + response->capacity;
    lastData = OS_GetTick();
    for (;;) {
        if (CPSMyIp == 0) {
            http->result = 5;
            goto cleanup;
        }
        size = CPS_SocGetLength();
        length = size;
        if (size < 0) {
            break;
        }
        if (size > 0) {
            lastData = OS_GetTick();
            data = CPS_SocRead(&length);
            if (data == NULL) {
                break;
            }
            room = response->end - 1 - response->cur;
            size = length;
            if (size >= room) {
                size = room;
            }
            MI_CpuCopy8(data, response->cur, size);
            old = response->cur;
            response->cur = old + size;
            old[size] = 0;
            if (headersDone == 1) {
                OS_LockMutex(&http->lock);
                http->bodyLength += size;
                OS_UnlockMutex(&http->lock);
            } else {
                headersDone = DWCi_Http_ParseContentLength(http);
            }
            if (length > size) {
                CPS_SocConsume(length);
                break;
            }
            CPS_SocConsume(size);
        }
        if (http->contentLength >= 0 && http->bodyLength >= http->contentLength) {
            break;
        }
        if (OS_TicksToMilliSeconds(OS_GetTick() - lastData) > timeout) {
            http->result = 6;
            goto cleanup;
        }
        if (!DWCi_Http_SleepUnlessAborted(http)) {
            http->result = 7;
            goto cleanup;
        }
    }
    CPS_TcpShutdown();
    CPS_TcpClose();
    CPS_SocRelease();
    CPS_SocUnRegister();
    http->result = 8;
    return;
cleanup:
    CPS_TcpShutdown();
    CPS_TcpClose();
    CPS_SocRelease();
    CPS_SocUnRegister();
}

void DWC_Http_Destroy(DWCHttp *http) {
    DWCHttpFree free = http->param.free;

    if (http != NULL) {
        DWCi_Http_FreeLabelValues(http, http->fields, 0x20);
        DWCi_Http_FreeBuffer(http, &http->response);
        DWCi_Http_FreeBuffer(http, &http->request);
        if (http->rcvBuffer != NULL) {
            free(0, http->rcvBuffer, 0);
            http->rcvBuffer = NULL;
        }
        if (http->sndBuffer != NULL) {
            free(0, http->sndBuffer, 0);
            http->sndBuffer = NULL;
        }
        MI_CpuFill8(http, 0, sizeof(DWCHttp));
    }
}

static int DWCi_Http_MakeRequestLine(DWCHttp *http) {
    DWCHttpBuffer *request = &http->request;
    const char *format = "POST /%s HTTP/1.0\r\nContent-type: application/x-www-form-urlencoded\r\nHost: %s\r\n\r\n";
    int length;

    if (http->param.action != DWC_HTTP_POST) {
        format = "GET /%s HTTP/1.0\r\nHost: %s\r\n\r\n";
    }
    length = strlen(http->host) + (strlen(format) - 4 + strlen(http->path));
    if (DWCi_Http_AllocBuffer(http, &http->request, length + 0x400) != 1) {
        return 1;
    }
    request->cur += OS_SNPrintf(request->cur, request->capacity, format, http->path, http->host);
    return 0;
}

DWCHttpError DWC_Http_Add_HeaderItem(DWCHttp *http, char *name, char *value) {
    int length;
    DWCHttpBuffer *request = &http->request;
    char *line;
    char saved;

    length = strlen(value) + (strlen("%s: %s\r\n") - 4 + strlen(name));
    if (length + 1 > request->end - request->cur) {
        if (DWCi_Http_GrowBuffer(http, request, length - (request->end - request->cur) + 1) == 0) {
            return 1;
        }
    }
    line = strstr(request->start, "\r\n\r\n") + 2;
    saved = *line;
    memmove(line + length, line, strlen(line) + 1);
    line[OS_SNPrintf(line, length + 1, "%s: %s\r\n", name, value)] = saved;
    request->cur += length;
    return 0;
}

DWCHttpError DWC_Http_Add_PostBase64Item(DWCHttp *http, const char *name, const char *value, unsigned long size) {
    DWCHttpBuffer *request = &http->request;
    const char *format = "%s=";
    int encoded;
    int length;
    int room;

    if (http->fieldCount != 0) {
        format = "&%s=";
    }
    http->fieldCount++;
    encoded = DWC_Base64Encode(value, size, NULL, 0);
    length = encoded + (strlen(format) - 2 + strlen(name));
    room = request->end - request->cur;
    if (length > room) {
        if (DWCi_Http_GrowBuffer(http, request, length - room + 1) == 0) {
            return 1;
        }
        room = request->end - request->cur;
    }
    request->cur += OS_SNPrintf(request->cur, room, format, name);
    if (DWC_Base64Encode(value, size, request->cur, request->end - request->cur - 1) < 0) {
        return 1;
    }
    request->cur += encoded;
    *request->cur = 0;
    return 0;
}

DWCHttpError DWC_Http_Add_Body(DWCHttp *http, const char *text) {
    int length;
    DWCHttpBuffer *request = &http->request;
    int room;
    int written;

    length = strlen(text);
    room = request->end - request->cur;
    if (length > room) {
        if (DWCi_Http_GrowBuffer(http, request, length - room + 1) == 0) {
            return 1;
        }
        room = request->end - request->cur;
    }
    written = OS_SNPrintf(request->cur, room, "%s", text);
    if (written != length) {
        return 1;
    }
    request->cur += written;
    return 0;
}

static int DWCi_Http_VerifyCertificate(int result, DWCHttpSslConnection *con, int index) {
    if (result & 0x8000) {
        result &= ~0x8000;
    }
    return result;
}

static int DWCi_Http_AllocBuffer(DWCHttp *http, DWCHttpBuffer *buffer, int size) {
    DWCHttpAlloc alloc = http->param.alloc;

    if (size == 0) {
        return 0;
    }
    buffer->start = alloc(0, size);
    if (buffer->start == NULL) {
        return 0;
    }
    buffer->cur = buffer->start;
    buffer->end = buffer->start + size;
    buffer->capacity = size;
    return 1;
}

static void DWCi_Http_FreeBuffer(DWCHttp *http, DWCHttpBuffer *buffer) {
    DWCHttpFree free = http->param.free;

    if (buffer->start != NULL) {
        free(0, buffer->start, 0);
    }
    MI_CpuFill8(buffer, 0, sizeof(DWCHttpBuffer));
}

static int DWCi_Http_GrowBuffer(DWCHttp *http, DWCHttpBuffer *buffer, int extra) {
    DWCHttpAlloc alloc = http->param.alloc;
    DWCHttpFree free = http->param.free;
    char *start;

    if (extra <= 0) {
        return 0;
    }
    start = alloc(0, buffer->capacity + extra);
    if (start == NULL) {
        return 0;
    }
    MI_CpuCopy8(buffer->start, start, buffer->capacity);
    free(0, buffer->start, 0);
    if (start == NULL) {
        return 0;
    }
    buffer->cur += start - buffer->start;
    buffer->start = start;
    buffer->capacity += extra;
    buffer->end = start + buffer->capacity;
    return 1;
}

static int DWCi_Http_ParseUrl(DWCHttp *http, const char *url) {
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
        http->host = http->url + 7;
        http->secure = 0;
        http->port = 80;
    } else {
        found = strstr(http->url, "https://");
        if (found == NULL) {
            return 0;
        }
        http->host = found + 8;
        http->secure = 1;
        http->port = 443;
    }
    found = strstr(http->host, ":");
    if (found != NULL) {
        port = found + 1;
        *found = 0;
    }
    found = strstr(http->host, "/");
    if (found == NULL) {
        http->path = NULL;
    } else {
        *found = 0;
        http->path = found + 1;
    }
    if (port != NULL) {
        http->port = atol(port);
    }
    return 1;
}

static int DWCi_Http_AddLabelValue(DWCHttp *http, NasFieldTable *table, const char *name, const char *value) {
    DWCHttpAlloc alloc = http->param.alloc;
    DWCHttpFree free = http->param.free;

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

    table.fields = http->fields;
    table.capacity = 32;
    table.count = 0;
    MI_CpuFill8(http->fields, 0, sizeof(http->fields));
    response = http->response.start;
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
    if (DWCi_Http_AddLabelValue(http, &table, "httpresult", status) != 1) {
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
        if (DWCi_Http_AddLabelValue(http, &table, line, value) != 1) {
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
        if (DWCi_Http_AddLabelValue(http, &table, name, value) != 1) {
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
        if (http->fields[i].label == NULL) {
            break;
        }
        if (strcmp(name, http->fields[i].label) == 0) {
            return http->fields[i].value;
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

static void DWCi_Http_FreeLabelValues(DWCHttp *http, DWCHttpLabelValue *fields, int count) {
    DWCHttpFree free = http->param.free;
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
