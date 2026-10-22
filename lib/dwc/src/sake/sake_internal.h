#ifndef POKEBW2_DWC_SAKE_INTERNAL_H
#define POKEBW2_DWC_SAKE_INTERNAL_H

#include "dwc/ext.h"
#include "dwc/sake.h"
#include "dwc/soap.h"

// The parts of Sake that its files share (sake.c, sake_error.c, sakeRequestModify.c and sakeRequestRead.c): a request,
// and the description of each kind of request that tells how to check, write and read it

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
    char *mTableId;
    char **mFieldNames;
    int mNumFields;
    char *mFilter;
    char *mSort;
    int mOffset;
    int mMaxRecords;
    char *mTargetRecordFilter;
    int mSurroundingRecordsCount;
    int *mOwnerIds;
    int mNumOwnerIds;
    gsi_bool mCacheFlag;
} SAKESearchForRecordsInput;

typedef struct {
    char *mTableId;
    char **mFieldNames;
    int mNumFields;
    char *mFilter;
} SAKEGetRandomRecordInput;

// The answer of a request for a random record: the record
typedef struct {
    SAKEField *mRecord;
} SAKEGetRandomRecordOutput;

// The input of the request to delete a record (the Sake has no call for it)
typedef struct {
    char *mTableId;
    int mRecordId;
} SAKEDeleteRecordInput;

// A request in progress: the Sake it is for, what it asks and what it answers, the function to call, the message it
// posts and the answer it got, and its kind
struct SAKERequestInternal {
    SAKE mSake;
    SAKEIRequestType mType;
    void *mInput;
    void *mOutput;
    SAKERequestCallback mCallback;
    void *mUserData;
    GSXmlStreamWriter mSoapRequest;
    GSXmlStreamReader mSoapResponse;
    SAKEIRequestInfo *mInfo;
};

// How a kind of request goes: the size of its answer, the element that carries it, its SOAPAction header, the elements
// of the answer and of the result in it, and the functions that check the input, write the message, read the answer
// into the output and free the output's parts. The type also keeps the HTTP request that posts it, which can be stopped
struct SAKEIRequestInfo {
    size_t mSakeOutputSize;
    const char *mFuncName;
    const char *mSoapAction;
    const char *mResponseTag;
    const char *mResultTag;
    SAKEStartRequestResult (*mValidateInputFunc)(SAKERequest request);
    SAKEStartRequestResult (*mFillSoapRequestFunc)(SAKERequest request);
    SAKERequestResult (*mProcessSoapResponseFunc)(SAKERequest request);
    void (*mFreeDataFunc)(SAKERequest request);
    GSTask *http;
};

// sake.c
void sakeiFreeRequest(SAKERequest request);
SAKERequestResult sakeiCheckHttpResult(GHTTPResult status);
// sake_error.c
void sakeiSoapCallback(GHTTPResult status, GSXmlStreamWriter unused, GSXmlStreamReader reader, void *userData);
SAKEStartRequestResult sakeiSetupRequest(SAKERequest request);
// sakeRequestModify.c
SAKEStartRequestResult sakeiStartRequest(SAKERequest request, SAKEIRequestInfo *type);
SAKEStartRequestResult sakeiStartCreateRecordRequest(SAKERequest request);
SAKEStartRequestResult sakeiStartUpdateRecordRequest(SAKERequest request);
// sakeRequestRead.c
SAKEStartRequestResult sakeiStartGetMyRecordsRequest(SAKERequest request);
SAKEStartRequestResult sakeiStartGetSpecificRecordsRequest(SAKERequest request);

#endif // POKEBW2_DWC_SAKE_INTERNAL_H
