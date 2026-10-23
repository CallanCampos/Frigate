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
    soap->mCallbackFunc = func;
    soap->mCustomFunc = NULL;
    soap->mURL = url;
    soap->mService = service;
    soap->mRequestSoap = request;
    soap->mPostData = NULL;
    soap->mResponseSoap = NULL;
    soap->mResponseBuffer = NULL;
    soap->mUserData = userData;
    soap->mRequestResult = GHTTPSuccess;
    soap->mCompleted = gsi_false;
    task = gsiCoreCreateTask();
    if (task == NULL) {
        gsifree(soap);
        return NULL;
    }
    task->mCallbackFunc = gsiSoapTaskCallback;
    task->mExecuteFunc = gsiSoapTaskExecute;
    task->mThinkFunc = gsiSoapTaskThink;
    task->mCleanupFunc = gsiSoapTaskCleanup;
    task->mCancelFunc = gsiSoapTaskCancel;
    task->mTaskData = soap;
    soap->mCoreTask = task;
    gsiCoreExecuteTask(task, timeout);
    return soap;
}

GSSoapTask *gsiExecuteSoapCustom(const char *url, const char *service, GSXmlStreamWriter request,
                                 GSSoapCallbackFunc func, GSSoapCustomFunc customFunc, void *userData) {
    GSSoapTask *soap = gsimalloc(sizeof(GSSoapTask));
    GSTask *task;

    soap->mCallbackFunc = func;
    soap->mCustomFunc = customFunc;
    soap->mURL = url;
    soap->mService = service;
    soap->mRequestSoap = request;
    soap->mPostData = NULL;
    soap->mResponseSoap = NULL;
    soap->mResponseBuffer = NULL;
    soap->mUserData = userData;
    soap->mRequestResult = GHTTPSuccess;
    soap->mCompleted = gsi_false;
    task = gsiCoreCreateTask();
    task->mCallbackFunc = gsiSoapTaskCallback;
    task->mExecuteFunc = gsiSoapTaskExecute;
    task->mThinkFunc = gsiSoapTaskThink;
    task->mCleanupFunc = gsiSoapTaskCleanup;
    task->mCancelFunc = gsiSoapTaskCancel;
    task->mTaskData = soap;
    soap->mCoreTask = task;
    gsiCoreExecuteTask(task, 0);
    return soap;
}

// Reads the answer, which the request then frees, when the HTTP request ended
static GHTTPBool gsiSoapTaskHttpCompletedCallback(GHTTPRequest request, GHTTPResult result, char *buffer,
                                                  GHTTPByteCount bufferLen, void *param) {
    GSSoapTask *soap = param;

    soap->mRequestResult = result;
    soap->mCompleted = gsi_true;
    soap->mResponseBuffer = buffer;
    if (result == GHTTPSuccess) {
        soap->mResponseSoap = gsXmlCreateStreamReader();
        if (soap->mResponseSoap == NULL) {
            soap->mRequestResult = GHTTPOutOfMemory;
        } else if (gsXmlParseBuffer(soap->mResponseSoap, buffer, bufferLen) == gsi_false) {
            soap->mRequestResult = GHTTPBadResponse;
        }
    }
    return GHTTPFalse;
}

static GSTaskResult gsiSoapTaskThink(void *task) {
    GSSoapTask *soap = task;

    if (soap->mCompleted != gsi_false) {
        return GSTaskResult_Finished;
    }
    ghttpRequestThink(soap->mRequestId);
    return GSTaskResult_InProgress;
}

static void gsiSoapTaskExecute(void *task) {
    GSSoapTask *soap = task;

    soap->mPostData = ghttpNewPost();
    if (soap->mPostData == NULL) {
        soap->mCompleted = gsi_true;
        soap->mRequestResult = GHTTPOutOfMemory;
        return;
    }
    ghttpPostSetAutoFree(soap->mPostData, GHTTPFalse);
    ghttpPostAddXml(soap->mPostData, soap->mRequestSoap);
    if (soap->mCustomFunc != NULL) {
        soap->mCustomFunc(soap->mPostData, soap->mUserData);
    }
    soap->mRequestId = ghttpGetExA(soap->mURL, soap->mService, NULL, 0, soap->mPostData, GHTTPFalse, GHTTPFalse, NULL,
                                   gsiSoapTaskHttpCompletedCallback, soap);
}

static void gsiSoapTaskCancel(void *task) {
    GSSoapTask *soap = task;

    if (soap->mCompleted == gsi_false) {
        if (soap->mRequestId >= 0) {
            ghttpCancelRequest(soap->mRequestId);
        }
        soap->mRequestResult = GHTTPRequestCancelled;
    }
}

static void gsiSoapTaskCallback(void *task, GSTaskResult result) {
    GSSoapTask *soap = task;

    soap->mCallbackFunc(soap->mRequestResult, soap->mRequestSoap, soap->mResponseSoap, soap->mUserData);
}

static gsi_bool gsiSoapTaskCleanup(void *task) {
    GSSoapTask *soap = task;

    if (soap->mResponseSoap != NULL) {
        gsXmlFreeReader(soap->mResponseSoap);
    }
    if (soap->mResponseBuffer != NULL) {
        gsifree(soap->mResponseBuffer);
    }
    if (soap->mPostData != NULL) {
        ghttpFreePost(soap->mRequestId, soap->mPostData);
    }
    gsifree(soap);
    return gsi_true;
}
