#ifndef POKEBW2_DWC_SAKE_INTERNAL_H
#define POKEBW2_DWC_SAKE_INTERNAL_H

#include "dwc/sake.h"
#include "dwc/soap.h"
#include "dwc/ext.h"

// The parts of Sake that its files share (sake.c, sake_error.c, sake_storage.c and sake_records.c): a request, and the
// description of each kind of request that tells how to check, write and read it

// The requests that have a call: they are the kinds 0, 1, 4 and 5 of the table of request kinds
typedef enum SakeRequestKind {
    SAKE_REQUEST_CREATE_RECORD = 0,
    SAKE_REQUEST_UPDATE_RECORD = 1,
    SAKE_REQUEST_GET_MY_RECORDS = 4,
    SAKE_REQUEST_GET_SPECIFIC_RECORDS = 5,
} SakeRequestKind;

// What a call has not been able to do before posting its request, which is also what the Sake keeps as its last error
typedef enum SakeRequestError {
    SAKE_REQUEST_ERROR_NONE = 0,
    SAKE_REQUEST_ERROR_NOT_SET_UP = 1,
    SAKE_REQUEST_ERROR_OUT_OF_MEMORY = 2,
    SAKE_REQUEST_ERROR_NO_INPUT = 3,
    SAKE_REQUEST_ERROR_NO_TABLE = 4,
    SAKE_REQUEST_ERROR_NO_FIELDS = 5,
    SAKE_REQUEST_ERROR_BAD_FIELD_NUM = 6,
    SAKE_REQUEST_ERROR_NO_FIELD_NAME = 7,
    SAKE_REQUEST_ERROR_BAD_FIELD_TYPE = 8,
    SAKE_REQUEST_ERROR_BAD_FIELD_VALUE = 9,
    SAKE_REQUEST_ERROR_BAD_OFFSET = 10,
    SAKE_REQUEST_ERROR_BAD_MAX = 11,
    SAKE_REQUEST_ERROR_NO_RECORD_IDS = 12,
    SAKE_REQUEST_ERROR_BAD_RECORD_NUM = 13,
} SakeRequestError;

typedef struct SakeRequestType SakeRequestType;

// The inputs of the requests that read records and that the Sake has no call for: searching a table, and getting
// random records
typedef struct SakeSearchRecordsInput {
    const char *tableId;
    const char **fieldNames;
    int fieldNum;
    const char *filter;
    const char *sort;
    int offset;
    int max;
    const char *targetFilter;
    int surrounding;
    int *ownerIds;
    int ownerNum;
    int cacheFlag;
} SakeSearchRecordsInput;

typedef struct SakeGetRandomRecordsInput {
    const char *tableId;
    const char **fieldNames;
    int fieldNum;
    const char *filter;
} SakeGetRandomRecordsInput;

// The input of the request to delete a record (the Sake has no call for it)
typedef struct SakeDeleteRecordInput {
    const char *tableId;
    int recordId;
} SakeDeleteRecordInput;

// A request in progress: the Sake it is for, what it asks and what it answers, the function to call, the message it
// posts and the answer it got, and its kind
struct SakeRequest {
    Sake *sake;
    int kind;
    void *input;
    void *output;
    SakeCallback callback;
    void *userData;
    SoapWriter *writer;
    SoapReader *response;
    SakeRequestType *type;
};

// How a kind of request goes: the size of its answer, the element that carries it, its SOAPAction header, the elements of
// the answer and of the result in it, and the functions that check the input, write the message, read the answer into
// the output and free the output's parts. The type also keeps the HTTP request that posts it, which can be stopped
struct SakeRequestType {
    int outputSize;
    const char *name;
    const char *soapAction;
    const char *responseName;
    const char *resultName;
    int (*validate)(SakeRequest *request);
    int (*build)(SakeRequest *request);
    int (*parse)(SakeRequest *request);
    void (*freeOutput)(SakeRequest *request);
    void *http;
};

// sake.c
void Sake_FreeRequest(SakeRequest *request);
int Sake_GetHttpResultError(int status);
// sake_error.c
void Sake_OnResponse(int status, int unused, SoapReader *reader, void *userData);
int Sake_BuildMessage(SakeRequest *request);
// sake_storage.c
int Sake_Submit(SakeRequest *request, SakeRequestType *type);
int Sake_SubmitCreateRecord(SakeRequest *request);
int Sake_SubmitUpdateRecord(SakeRequest *request);
// sake_records.c
int Sake_SubmitGetMyRecords(SakeRequest *request);
int Sake_SubmitGetSpecificRecords(SakeRequest *request);

#endif // POKEBW2_DWC_SAKE_INTERNAL_H
