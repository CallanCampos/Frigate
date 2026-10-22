#include "dwc/gsSoap.h"
#include "../ghttp/ghttp_internal.h"
#include "dwc/gsMemory.h"

// GameSpy's SOAP requests (gsSoap.c). The ROM names neither the file nor its functions; both are the later SDK's

static GHTTPBool gsiSoapTaskHttpCompletedCallback(GHTTPRequest request, GHTTPResult result, char *buffer,
                                                  GHTTPByteCount bufferLen, void *param);
static GSTaskResult gsiSoapTaskThink(void *task);
static void gsiSoapTaskExecute(void *task);
static void gsiSoapTaskCancel(void *task);
static void gsiSoapTaskCallback(void *task, GSTaskResult result);
static gsi_bool gsiSoapTaskCleanup(void *task);

GSSoapTask *gsiExecuteSoap(const char *url, const char *service, GSXmlStreamWriter request, GSSoapCallbackFunc func,
                           void *userData) {
    return gsiExecuteSoapWithTimeout(url, service, request, func, 0, userData);
}

GSSoapTask *gsiExecuteSoapWithTimeout(const char *url, const char *service, GSXmlStreamWriter request,
                                      GSSoapCallbackFunc func, gsi_u32 timeout, void *userData) {
    GSSoapTask *soap = gsimalloc(sizeof(GSSoapTask));
    GSTask *task;

    if (soap == NULL) {
        return NULL;
    }
    soap->callbackFunc = func;
    soap->customFunc = NULL;
    soap->url = url;
    soap->service = service;
    soap->requestSoap = request;
    soap->postData = NULL;
    soap->responseSoap = NULL;
    soap->responseBuffer = NULL;
    soap->userData = userData;
    soap->requestResult = GHTTPSuccess;
    soap->completed = gsi_false;
    task = gsiCoreCreateTask();
    if (task == NULL) {
        gsifree(soap);
        return NULL;
    }
    task->callbackFunc = gsiSoapTaskCallback;
    task->executeFunc = gsiSoapTaskExecute;
    task->thinkFunc = gsiSoapTaskThink;
    task->cleanupFunc = gsiSoapTaskCleanup;
    task->cancelFunc = gsiSoapTaskCancel;
    task->taskData = soap;
    soap->handle = task;
    gsiCoreExecuteTask(task, timeout);
    return soap;
}

GSSoapTask *gsiExecuteSoapCustom(const char *url, const char *service, GSXmlStreamWriter request,
                                 GSSoapCallbackFunc func, GSSoapCustomFunc customFunc, void *userData) {
    GSSoapTask *soap = gsimalloc(sizeof(GSSoapTask));
    GSTask *task;

    soap->callbackFunc = func;
    soap->customFunc = customFunc;
    soap->url = url;
    soap->service = service;
    soap->requestSoap = request;
    soap->postData = NULL;
    soap->responseSoap = NULL;
    soap->responseBuffer = NULL;
    soap->userData = userData;
    soap->requestResult = GHTTPSuccess;
    soap->completed = gsi_false;
    task = gsiCoreCreateTask();
    task->callbackFunc = gsiSoapTaskCallback;
    task->executeFunc = gsiSoapTaskExecute;
    task->thinkFunc = gsiSoapTaskThink;
    task->cleanupFunc = gsiSoapTaskCleanup;
    task->cancelFunc = gsiSoapTaskCancel;
    task->taskData = soap;
    soap->handle = task;
    gsiCoreExecuteTask(task, 0);
    return soap;
}

// Reads the answer, which the request then frees, when the HTTP request ended
static GHTTPBool gsiSoapTaskHttpCompletedCallback(GHTTPRequest request, GHTTPResult result, char *buffer,
                                                  GHTTPByteCount bufferLen, void *param) {
    GSSoapTask *soap = param;

    soap->requestResult = result;
    soap->completed = gsi_true;
    soap->responseBuffer = buffer;
    if (result == GHTTPSuccess) {
        soap->responseSoap = gsXmlCreateStreamReader();
        if (soap->responseSoap == NULL) {
            soap->requestResult = GHTTPOutOfMemory;
        } else if (gsXmlParseXml(soap->responseSoap, buffer, bufferLen) == gsi_false) {
            soap->requestResult = GHTTPBadResponse;
        }
    }
    return GHTTPFalse;
}

static GSTaskResult gsiSoapTaskThink(void *task) {
    GSSoapTask *soap = task;

    if (soap->completed != gsi_false) {
        return GSTaskResult_Finished;
    }
    ghttpThinkRequest(soap->requestId);
    return GSTaskResult_InProgress;
}

static void gsiSoapTaskExecute(void *task) {
    GSSoapTask *soap = task;

    soap->postData = ghttpNewPost();
    if (soap->postData == NULL) {
        soap->completed = gsi_true;
        soap->requestResult = GHTTPOutOfMemory;
        return;
    }
    ghttpPostSetAutoFree(soap->postData, GHTTPFalse);
    ghttpPostAddXml(soap->postData, soap->requestSoap);
    if (soap->customFunc != NULL) {
        soap->customFunc(soap->postData, soap->userData);
    }
    soap->requestId = ghttpGetExA(soap->url, soap->service, NULL, 0, soap->postData, GHTTPFalse, GHTTPFalse, NULL,
                                  gsiSoapTaskHttpCompletedCallback, soap);
}

static void gsiSoapTaskCancel(void *task) {
    GSSoapTask *soap = task;

    if (soap->completed == gsi_false) {
        if (soap->requestId >= 0) {
            ghttpCancelRequest(soap->requestId);
        }
        soap->requestResult = GHTTPRequestCancelled;
    }
}

static void gsiSoapTaskCallback(void *task, GSTaskResult result) {
    GSSoapTask *soap = task;

    soap->callbackFunc(soap->requestResult, soap->requestSoap, soap->responseSoap, soap->userData);
}

static gsi_bool gsiSoapTaskCleanup(void *task) {
    GSSoapTask *soap = task;

    if (soap->responseSoap != NULL) {
        gsXmlFreeReader(soap->responseSoap);
    }
    if (soap->responseBuffer != NULL) {
        gsifree(soap->responseBuffer);
    }
    if (soap->postData != NULL) {
        ghttpFreePost(soap->requestId, soap->postData);
    }
    gsifree(soap);
    return gsi_true;
}
