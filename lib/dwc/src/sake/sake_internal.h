#ifndef POKEBW2_DWC_SAKE_INTERNAL_H
#define POKEBW2_DWC_SAKE_INTERNAL_H

#include "dwc/ext.h"
#include "dwc/sake.h"
#include "dwc/soap.h"

// The parts of Sake that its files share (sake.c, sake_error.c, sake_storage.c and sake_records.c): a request, and the
// description of each kind of request that tells how to check, write and read it

// The kinds of request; the Sake has calls for CREATE_RECORD, UPDATE_RECORD, GET_MY_RECORDS and GET_SPECIFIC_RECORDS
typedef enum {
    SAKEIRequestType_CREATE_RECORD,
    SAKEIRequestType_UPDATE_RECORD,
    SAKEIRequestType_DELETE_RECORD,
    SAKEIRequestType_SEARCH_FOR_RECORDS,
    SAKEIRequestType_GET_MY_RECORDS,
    SAKEIRequestType_GET_SPECIFIC_RECORDS,
    SAKEIRequestType_GET_RANDOM_RECORD
} SAKEIRequestType;

typedef struct SAKEIRequestInfo SAKEIRequestInfo;

// The inputs of the requests that read records and that the Sake has no call for: searching a table, and getting
// random records
typedef struct {
    char *tableId;
    char **fieldNames;
    int fieldNum;
    char *filter;
    char *sort;
    int offset;
    int max;
    char *targetFilter;
    int surrounding;
    int *ownerIds;
    int ownerNum;
    gsi_bool cacheFlag;
} SAKESearchForRecordsInput;

typedef struct {
    char *tableId;
    char **fieldNames;
    int fieldNum;
    char *filter;
} SAKEGetRandomRecordInput;

// The answer of a request for a random record: the record
typedef struct {
    SAKEField *record;
} SAKEGetRandomRecordOutput;

// The input of the request to delete a record (the Sake has no call for it)
typedef struct {
    char *tableId;
    int recordId;
} SAKEDeleteRecordInput;

// A request in progress: the Sake it is for, what it asks and what it answers, the function to call, the message it
// posts and the answer it got, and its kind
struct SAKERequestInternal {
    SAKE sake;
    SAKEIRequestType kind;
    void *input;
    void *output;
    SAKERequestCallback callback;
    void *userData;
    GSXmlStreamWriter writer;
    GSXmlStreamReader response;
    SAKEIRequestInfo *type;
};

// How a kind of request goes: the size of its answer, the element that carries it, its SOAPAction header, the elements
// of the answer and of the result in it, and the functions that check the input, write the message, read the answer
// into the output and free the output's parts. The type also keeps the HTTP request that posts it, which can be stopped
struct SAKEIRequestInfo {
    size_t outputSize;
    const char *name;
    const char *soapAction;
    const char *responseName;
    const char *resultName;
    SAKEStartRequestResult (*validate)(SAKERequest request);
    SAKEStartRequestResult (*build)(SAKERequest request);
    SAKERequestResult (*parse)(SAKERequest request);
    void (*freeOutput)(SAKERequest request);
    GSTask *http;
};

// sake.c
void Sake_FreeRequest(SAKERequest request);
SAKERequestResult Sake_GetHttpResultError(GHTTPResult status);
// sake_error.c
void Sake_OnResponse(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader, void *userData);
SAKEStartRequestResult Sake_BuildMessage(SAKERequest request);
// sake_storage.c
SAKEStartRequestResult Sake_Submit(SAKERequest request, SAKEIRequestInfo *type);
SAKEStartRequestResult Sake_SubmitCreateRecord(SAKERequest request);
SAKEStartRequestResult Sake_SubmitUpdateRecord(SAKERequest request);
// sake_records.c
SAKEStartRequestResult Sake_SubmitGetMyRecords(SAKERequest request);
SAKEStartRequestResult Sake_SubmitGetSpecificRecords(SAKERequest request);

#endif // POKEBW2_DWC_SAKE_INTERNAL_H
