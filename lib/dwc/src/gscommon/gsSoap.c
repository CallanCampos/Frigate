#include "dwc/gsSoap.h"
#include "../ghttp/ghttp_internal.h"
#include "dwc/gsMemory.h"

// GameSpy's SOAP requests (gsSoap.c). The ROM names neither the file nor its functions; both are the later SDK's

static gsi_bool gsiSoapTaskHttpCompletedCallback(int request, int result, char *buffer, int bufferLen, void *param);
static int gsiSoapTaskThink(void *task);
static void gsiSoapTaskExecute(void *task);
static void gsiSoapTaskCancel(void *task);
static void gsiSoapTaskCallback(void *task, int result);
static gsi_bool gsiSoapTaskCleanup(void *task);

GsHttpRequest *gsiExecuteSoap(const char *url, const char *service, SoapWriter *request, GsHttpResponseFunc func,
                              void *userData) {
    return gsiExecuteSoapWithTimeout(url, service, request, func, 0, userData);
}

GsHttpRequest *gsiExecuteSoapWithTimeout(const char *url, const char *service, SoapWriter *request,
                                         GsHttpResponseFunc func, gsi_u32 timeout, void *userData) {
    GsHttpRequest *soap = gsimalloc(sizeof(GsHttpRequest));
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
    soap->requestResult = 0;
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

GsHttpRequest *gsiExecuteSoapCustom(const char *url, const char *service, SoapWriter *request, GsHttpResponseFunc func,
                                    GSSoapCustomFunc customFunc, void *userData) {
    GsHttpRequest *soap = gsimalloc(sizeof(GsHttpRequest));
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
    soap->requestResult = 0;
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
static gsi_bool gsiSoapTaskHttpCompletedCallback(int request, int result, char *buffer, int bufferLen, void *param) {
    GsHttpRequest *soap = param;

    soap->requestResult = result;
    soap->completed = gsi_true;
    soap->responseBuffer = buffer;
    if (result == 0) {
        soap->responseSoap = gsXmlCreateStreamReader();
        if (soap->responseSoap == NULL) {
            soap->requestResult = 1;
        } else if (gsXmlParseXml(soap->responseSoap, buffer, bufferLen) == gsi_false) {
            soap->requestResult = 7;
        }
    }
    return gsi_false;
}

static int gsiSoapTaskThink(void *task) {
    GsHttpRequest *soap = task;

    if (soap->completed != gsi_false) {
        return 4;
    }
    ghttpThinkRequest(soap->requestId);
    return 1;
}

static void gsiSoapTaskExecute(void *task) {
    GsHttpRequest *soap = task;

    soap->postData = ghttpNewPost();
    if (soap->postData == NULL) {
        soap->completed = gsi_true;
        soap->requestResult = 1;
        return;
    }
    ghttpPostSetAutoFree(soap->postData, gsi_false);
    ghttpPostAddXml(soap->postData, soap->requestSoap);
    if (soap->customFunc != NULL) {
        soap->customFunc(soap->postData, soap->userData);
    }
    soap->requestId = ghttpGetExA(soap->url, soap->service, NULL, 0, soap->postData, gsi_false, gsi_false, NULL,
                                  gsiSoapTaskHttpCompletedCallback, soap);
}

static void gsiSoapTaskCancel(void *task) {
    GsHttpRequest *soap = task;

    if (soap->completed == gsi_false) {
        if (soap->requestId >= 0) {
            ghttpCancelRequest(soap->requestId);
        }
        soap->requestResult = 0x12;
    }
}

static void gsiSoapTaskCallback(void *task, int result) {
    GsHttpRequest *soap = task;

    soap->callbackFunc(soap->requestResult, (int)soap->requestSoap, soap->responseSoap, soap->userData);
}

static gsi_bool gsiSoapTaskCleanup(void *task) {
    GsHttpRequest *soap = task;

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
