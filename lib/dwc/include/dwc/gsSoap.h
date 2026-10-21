#ifndef POKEBW2_DWC_GSSOAP_H
#define POKEBW2_DWC_GSSOAP_H

#include "dwc/gsCore.h"
#include "dwc/nonport.h"
#include "dwc/soap.h"

// GameSpy's SOAP requests (gsSoap.c), in overlay 11: a request posts a message of dwc/gsXML.h's writer to a URL with a
// SOAPAction header, through the HTTP code of overlay 189, as a task of the core (dwc/gsCore.h), and gives the answer
// to its callback, read with dwc/gsXML.h's reader

// The callback of a request, with the result of the HTTP request (0 when it worked), the message it posted (as an
// int), the reader of the answer and the user data; and the function that adds more to the post (a form)
typedef void (*GsHttpResponseFunc)(int status, int request, SoapReader *reader, void *userData);
typedef void (*GSSoapCustomFunc)(void *post, void *userData);

// A request: the callback, what it posts and where, the answer, the HTTP post, the task that runs the request
// (handle) and the HTTP request's number, its result and whether it ended
typedef struct GsHttpRequest {
    GsHttpResponseFunc callbackFunc;
    GSSoapCustomFunc customFunc;
    const char *url;
    const char *service;
    SoapWriter *requestSoap;
    SoapReader *responseSoap;
    char *responseBuffer;
    struct GHIPost *postData;
    void *userData;
    GSTask *handle;
    int requestId;
    int requestResult;
    gsi_bool completed;
} GsHttpRequest;

// Starts a request, with a time limit or none; returns it, or NULL when it could not start
GsHttpRequest *gsiExecuteSoap(const char *url, const char *service, SoapWriter *request, GsHttpResponseFunc func,
                              void *userData);
GsHttpRequest *gsiExecuteSoapWithTimeout(const char *url, const char *service, SoapWriter *request,
                                         GsHttpResponseFunc func, gsi_u32 timeout, void *userData);
// The same with no time limit and a function that adds to the post before it is sent
GsHttpRequest *gsiExecuteSoapCustom(const char *url, const char *service, SoapWriter *request, GsHttpResponseFunc func,
                                    GSSoapCustomFunc customFunc, void *userData);

#endif // POKEBW2_DWC_GSSOAP_H
