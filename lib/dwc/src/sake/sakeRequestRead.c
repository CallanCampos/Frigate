#include "sake_internal.h"

// The requests that read records from the storage server: those of the player, those of given IDs, a search of a
// table and random ones. A message lists the names of the fields to read, and the answer lists the records, each
// with a field of its own in the order of the names. The file and its names are
// GameSpy's own, as its sakeRequestRead.c has them, but for the descriptions of the kinds, which are function-local
// statics named info there.

static SAKEStartRequestResult sakeiValidateRequestFieldNames(char **names, int nameNum);
static void sakeiFillSoapRequestFieldNames(SAKERequest request, char **names, int nameNum);
static void sakeiFillSoapRequestRecordIds(SAKERequest request, int *recordIds, int recordNum);
static void sakeiFillSoapRequestOwnerIds(SAKERequest request, int *ownerIds, int ownerNum);
static SAKERequestResult sakeiReadOutputRecords(SAKERequest request, SAKEField ***records, int *recordNum, int fieldNum,
                                                char **fieldNames);
static void sakeiFreeOutputRecord(int fieldNum, SAKEField *fields);
static void sakeiFreeOutputRecords(int fieldNum, int recordNum, SAKEField **records);
static SAKEStartRequestResult sakeiSearchForRecordsValidateInput(SAKERequest request);
static SAKEStartRequestResult sakeiSearchForRecordsFillSoapRequest(SAKERequest request);
static SAKERequestResult sakeiSearchForRecordsProcessSoapResponse(SAKERequest request);
static void sakeiSearchForRecordsFreeData(SAKERequest request);
static SAKEStartRequestResult sakeiGetMyRecordsValidateInput(SAKERequest request);
static SAKEStartRequestResult sakeiGetMyRecordsFillSoapRequest(SAKERequest request);
static SAKERequestResult sakeiGetMyRecordsProcessSoapResponse(SAKERequest request);
static void sakeiGetMyRecordsFreeData(SAKERequest request);
static SAKEStartRequestResult sakeiGetSpecificRecordsValidateInput(SAKERequest request);
static SAKEStartRequestResult sakeiGetSpecificRecordsFillSoapRequest(SAKERequest request);
static SAKERequestResult sakeiGetSpecificRecordsProcessSoapResponse(SAKERequest request);
static void sakeiGetSpecificRecordsFreeData(SAKERequest request);
static SAKEStartRequestResult sakeiGetRandomRecordValidateInput(SAKERequest request);
static SAKEStartRequestResult sakeiGetRandomRecordFillSoapRequest(SAKERequest request);
static SAKERequestResult sakeiGetRandomRecordProcessSoapResponse(SAKERequest request);
static void sakeiGetRandomRecordFreeData(SAKERequest request);

static SAKEIRequestInfo sSearchRecordsType = {
    8,
    "SearchForRecords",
    "SOAPAction: \"http://gamespy.net/sake/SearchForRecords\"",
    "SearchForRecordsResponse",
    "SearchForRecordsResult",
    sakeiSearchForRecordsValidateInput,
    sakeiSearchForRecordsFillSoapRequest,
    sakeiSearchForRecordsProcessSoapResponse,
    sakeiSearchForRecordsFreeData,
    NULL,
};

static SAKEIRequestInfo sGetMyRecordsType = {
    8,
    "GetMyRecords",
    "SOAPAction: \"http://gamespy.net/sake/GetMyRecords\"",
    "GetMyRecordsResponse",
    "GetMyRecordsResult",
    sakeiGetMyRecordsValidateInput,
    sakeiGetMyRecordsFillSoapRequest,
    sakeiGetMyRecordsProcessSoapResponse,
    sakeiGetMyRecordsFreeData,
    NULL,
};

static SAKEIRequestInfo sGetSpecificRecordsType = {
    8,
    "GetSpecificRecords",
    "SOAPAction: \"http://gamespy.net/sake/GetSpecificRecords\"",
    "GetSpecificRecordsResponse",
    "GetSpecificRecordsResult",
    sakeiGetSpecificRecordsValidateInput,
    sakeiGetSpecificRecordsFillSoapRequest,
    sakeiGetSpecificRecordsProcessSoapResponse,
    sakeiGetSpecificRecordsFreeData,
    NULL,
};

static SAKEIRequestInfo sGetRandomRecordsType = {
    4,
    "GetRandomRecords",
    "SOAPAction: \"http://gamespy.net/sake/GetRandomRecords\"",
    "GetRandomRecordsResponse",
    "GetRandomRecordsResult",
    sakeiGetRandomRecordValidateInput,
    sakeiGetRandomRecordFillSoapRequest,
    sakeiGetRandomRecordProcessSoapResponse,
    sakeiGetRandomRecordFreeData,
    NULL,
};

static SAKEStartRequestResult sakeiValidateRequestFieldNames(char **names, int nameNum) {
    int i;

    for (i = 0; i < nameNum; i++) {
        if (names[i] == NULL || names[i][0] == '\0') {
            return SAKEStartRequestResult_BAD_FIELD_NAME;
        }
    }
    return 0;
}

static void sakeiFillSoapRequestFieldNames(SAKERequest request, char **names, int nameNum) {
    int i;

    gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "fields");
    for (i = 0; i < nameNum; i++) {
        gsXmlWriteStringElement(request->mSoapRequest, "ns1", "string", names[i]);
    }
    gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "fields");
}

static void sakeiFillSoapRequestRecordIds(SAKERequest request, int *recordIds, int recordNum) {
    int i;

    gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "recordids");
    for (i = 0; i < recordNum; i++) {
        gsXmlWriteIntElement(request->mSoapRequest, "ns1", "int", recordIds[i]);
    }
    gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "recordids");
}

static void sakeiFillSoapRequestOwnerIds(SAKERequest request, int *ownerIds, int ownerNum) {
    int i;

    gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "ownerids");
    for (i = 0; i < ownerNum; i++) {
        gsXmlWriteIntElement(request->mSoapRequest, "ns1", "int", ownerIds[i]);
    }
    gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "ownerids");
}

static SAKERequestResult sakeiReadOutputRecords(SAKERequest request, SAKEField ***records, int *recordNum, int fieldNum,
                                                char **fieldNames) {
    SAKEField **list;
    SAKEField *record;
    int i;
    int j;
    int count;
    int listSize;

    if (gsXmlMoveToChild(request->mSoapResponse, "values") == gsi_false) {
        return SAKERequestResult_MALFORMED_RESPONSE;
    }
    count = gsXmlCountChildren(request->mSoapResponse, "ArrayOfRecordValue");
    *recordNum = count;
    if (count == 0) {
        *records = NULL;
        return 0;
    }
    listSize = count * sizeof(SAKEField *);
    list = gsimalloc(listSize);
    if (list == NULL) {
        return SAKERequestResult_OUT_OF_MEMORY;
    }
    memset(list, 0, listSize);
    *records = list;
    for (i = 0; i < *recordNum; i++) {
        if (gsXmlMoveToNext(request->mSoapResponse, "ArrayOfRecordValue") == gsi_false) {
            return SAKERequestResult_MALFORMED_RESPONSE;
        }
        record = gsimalloc(fieldNum * sizeof(SAKEField));
        if (record == NULL) {
            return SAKERequestResult_OUT_OF_MEMORY;
        }
        memset(record, 0, fieldNum * sizeof(SAKEField));
        list[i] = record;
        if (fieldNum != gsXmlCountChildren(request->mSoapResponse, "RecordValue")) {
            return SAKERequestResult_MALFORMED_RESPONSE;
        }
        for (j = 0; j < fieldNum; j++) {
            SAKEField *field = &record[j];

            field->mName = fieldNames[j];
            if (gsXmlMoveToNext(request->mSoapResponse, "RecordValue") == gsi_false) {
                return SAKERequestResult_MALFORMED_RESPONSE;
            }
            if (gsXmlMoveToChild(request->mSoapResponse, "byteValue")) {
                int value;

                if (gsXmlReadChildAsInt(request->mSoapResponse, "value", &value) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                field->mType = SAKEFieldType_BYTE;
                field->mValue.mByte = value;
            } else if (gsXmlMoveToChild(request->mSoapResponse, "shortValue")) {
                int value;

                if (gsXmlReadChildAsInt(request->mSoapResponse, "value", &value) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                field->mType = SAKEFieldType_SHORT;
                field->mValue.mShort = value;
            } else if (gsXmlMoveToChild(request->mSoapResponse, "intValue")) {
                int value;

                if (gsXmlReadChildAsInt(request->mSoapResponse, "value", &value) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                field->mType = SAKEFieldType_INT;
                field->mValue.mInt = value;
            } else if (gsXmlMoveToChild(request->mSoapResponse, "int64Value")) {
                gsi_i64 value;

                if (gsXmlReadChildAsInt64(request->mSoapResponse, "value", &value) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                field->mType = SAKEFieldType_INT64;
                field->mValue.mInt64 = value;
            } else if (gsXmlMoveToChild(request->mSoapResponse, "floatValue")) {
                float value;

                if (gsXmlReadChildAsFloat(request->mSoapResponse, "value", &value) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                field->mType = SAKEFieldType_FLOAT;
                field->mValue.mFloat = value;
            } else if (gsXmlMoveToChild(request->mSoapResponse, "asciiStringValue")) {
                char *text;
                int length;

                if (gsXmlReadChildAsString(request->mSoapResponse, "value", (const char **)&text, &length) ==
                    gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                if (text != NULL) {
                    text[length] = '\0';
                } else {
                    text = "";
                }
                field->mType = SAKEFieldType_ASCII_STRING;
                field->mValue.mAsciiString = text;
            } else if (gsXmlMoveToChild(request->mSoapResponse, "unicodeStringValue")) {
                char *text;
                int length;
                gsi_u16 *wide;

                if (gsXmlReadChildAsString(request->mSoapResponse, "value", (const char **)&text, &length) ==
                    gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                if (text != NULL) {
                    text[length] = '\0';
                } else {
                    text = "";
                }
                wide = UTF8ToUCS2StringAlloc(text);
                if (wide == NULL) {
                    return SAKERequestResult_OUT_OF_MEMORY;
                }
                field->mType = SAKEFieldType_UNICODE_STRING;
                field->mValue.mUnicodeString = wide;
            } else if (gsXmlMoveToChild(request->mSoapResponse, "booleanValue")) {
                char *text;
                int length;
                gsi_bool value;

                if (gsXmlReadChildAsString(request->mSoapResponse, "value", (const char **)&text, &length) ==
                    gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                if (text != NULL) {
                    text[length] = '\0';
                    value = strcmp(text, "true") == 0;
                } else {
                    value = gsi_false;
                }
                field->mType = SAKEFieldType_BOOLEAN;
                field->mValue.mBoolean = value;
            } else if (gsXmlMoveToChild(request->mSoapResponse, "dateAndTimeValue")) {
                int value;

                if (gsXmlReadChildAsDateTimeElement(request->mSoapResponse, "value", &value) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                field->mType = SAKEFieldType_DATE_AND_TIME;
                field->mValue.mDateAndTime = value;
            } else if (gsXmlMoveToChild(request->mSoapResponse, "binaryDataValue")) {
                void *data;
                int length;

                if (gsXmlReadChildAsBase64Binary(request->mSoapResponse, "value", NULL, &length) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                if (length > 0) {
                    data = gsimalloc(length);
                    if (data == NULL) {
                        return SAKERequestResult_OUT_OF_MEMORY;
                    }
                    if (gsXmlReadChildAsBase64Binary(request->mSoapResponse, "value", data, &length) == gsi_false) {
                        gsifree(data);
                        return SAKERequestResult_MALFORMED_RESPONSE;
                    }
                } else {
                    data = NULL;
                    length = 0;
                }
                field->mType = SAKEFieldType_BINARY_DATA;
                field->mValue.mBinaryData.mLength = length;
                field->mValue.mBinaryData.mValue = data;
            } else {
                return SAKERequestResult_UNKNOWN_ERROR;
            }
        }
    }
    return 0;
}

static void sakeiFreeOutputRecord(int fieldNum, SAKEField *fields) {
    int i;

    if (fields == NULL) {
        return;
    }
    for (i = 0; i < fieldNum; i++) {
        SAKEField *field = &fields[i];

        if (field->mType == SAKEFieldType_BINARY_DATA && field->mValue.mBinaryData.mValue != NULL) {
            gsifree(field->mValue.mBinaryData.mValue);
        }
        if (field->mType == SAKEFieldType_UNICODE_STRING) {
            gsifree(field->mValue.mUnicodeString);
        }
    }
    gsifree(fields);
}

static void sakeiFreeOutputRecords(int fieldNum, int recordNum, SAKEField **records) {
    int i;
    int j;

    if (records == NULL) {
        return;
    }
    for (i = 0; i < recordNum; i++) {
        if (records[i] != NULL) {
            for (j = 0; j < fieldNum; j++) {
                if (records[i][j].mType == SAKEFieldType_BINARY_DATA &&
                    records[i][j].mValue.mBinaryData.mValue != NULL) {
                    gsifree(records[i][j].mValue.mBinaryData.mValue);
                }
                if (records[i][j].mType == SAKEFieldType_UNICODE_STRING) {
                    gsifree(records[i][j].mValue.mUnicodeString);
                }
            }
            gsifree(records[i]);
            records[i] = NULL;
        }
    }
    gsifree(records);
}

static SAKEStartRequestResult sakeiSearchForRecordsValidateInput(SAKERequest request) {
    SAKESearchForRecordsInput *input = request->mInput;

    if (input->mTableId == NULL) {
        return SAKEStartRequestResult_BAD_TABLEID;
    }
    if (input->mNumFields <= 0) {
        return SAKEStartRequestResult_BAD_NUM_FIELDS;
    }
    if (input->mFieldNames == NULL) {
        return SAKEStartRequestResult_BAD_FIELDS;
    }
    if (input->mOffset < 0) {
        return SAKEStartRequestResult_BAD_OFFSET;
    }
    if (input->mMaxRecords <= 0) {
        return SAKEStartRequestResult_BAD_MAX;
    }
    return sakeiValidateRequestFieldNames(input->mFieldNames, input->mNumFields);
}

static SAKEStartRequestResult sakeiSearchForRecordsFillSoapRequest(SAKERequest request) {
    SAKESearchForRecordsInput *input = request->mInput;

    gsXmlWriteStringElement(request->mSoapRequest, "ns1", "tableid", input->mTableId);
    if (input->mFilter != NULL) {
        gsXmlWriteStringElement(request->mSoapRequest, "ns1", "filter", input->mFilter);
    }
    if (input->mSort != NULL) {
        gsXmlWriteStringElement(request->mSoapRequest, "ns1", "sort", input->mSort);
    }
    gsXmlWriteIntElement(request->mSoapRequest, "ns1", "offset", input->mOffset);
    gsXmlWriteIntElement(request->mSoapRequest, "ns1", "max", input->mMaxRecords);
    if (input->mTargetRecordFilter != NULL) {
        gsXmlWriteStringElement(request->mSoapRequest, "ns1", "targetfilter", input->mTargetRecordFilter);
    }
    gsXmlWriteIntElement(request->mSoapRequest, "ns1", "surrounding", input->mSurroundingRecordsCount);
    sakeiFillSoapRequestOwnerIds(request, input->mOwnerIds, input->mNumOwnerIds);
    gsXmlWriteIntElement(request->mSoapRequest, "ns1", "cacheFlag", input->mCacheFlag);
    sakeiFillSoapRequestFieldNames(request, input->mFieldNames, input->mNumFields);
    return 0;
}

static SAKERequestResult sakeiSearchForRecordsProcessSoapResponse(SAKERequest request) {
    SAKESearchForRecordsInput *input = request->mInput;
    SAKESearchForRecordsOutput *output = request->mOutput;

    return sakeiReadOutputRecords(request, &output->mRecords, &output->mNumRecords, input->mNumFields,
                                  input->mFieldNames);
}

static void sakeiSearchForRecordsFreeData(SAKERequest request) {
    SAKESearchForRecordsInput *input = request->mInput;
    SAKESearchForRecordsOutput *output = request->mOutput;

    if (output != NULL) {
        sakeiFreeOutputRecords(input->mNumFields, output->mNumRecords, output->mRecords);
    }
}

static SAKEStartRequestResult sakeiGetMyRecordsValidateInput(SAKERequest request) {
    SAKEGetMyRecordsInput *input = request->mInput;

    if (input->mTableId == NULL) {
        return SAKEStartRequestResult_BAD_TABLEID;
    }
    if (input->mNumFields <= 0) {
        return SAKEStartRequestResult_BAD_NUM_FIELDS;
    }
    if (input->mFieldNames != NULL) {
        return sakeiValidateRequestFieldNames(input->mFieldNames, input->mNumFields);
    }
    return SAKEStartRequestResult_BAD_FIELDS;
}

static SAKEStartRequestResult sakeiGetMyRecordsFillSoapRequest(SAKERequest request) {
    SAKEGetMyRecordsInput *input = request->mInput;

    gsXmlWriteStringElement(request->mSoapRequest, "ns1", "tableid", input->mTableId);
    sakeiFillSoapRequestFieldNames(request, input->mFieldNames, input->mNumFields);
    return 0;
}

static SAKERequestResult sakeiGetMyRecordsProcessSoapResponse(SAKERequest request) {
    SAKEGetMyRecordsInput *input = request->mInput;
    SAKEGetMyRecordsOutput *output = request->mOutput;

    return sakeiReadOutputRecords(request, &output->mRecords, &output->mNumRecords, input->mNumFields,
                                  input->mFieldNames);
}

static void sakeiGetMyRecordsFreeData(SAKERequest request) {
    SAKEGetMyRecordsInput *input = request->mInput;
    SAKEGetMyRecordsOutput *output = request->mOutput;

    if (output != NULL) {
        sakeiFreeOutputRecords(input->mNumFields, output->mNumRecords, output->mRecords);
    }
}

SAKEStartRequestResult sakeiStartGetMyRecordsRequest(SAKERequest request) {
    return sakeiStartRequest(request, &sGetMyRecordsType);
}

static SAKEStartRequestResult sakeiGetSpecificRecordsValidateInput(SAKERequest request) {
    SAKEGetSpecificRecordsInput *input = request->mInput;

    if (input->mTableId == NULL) {
        return SAKEStartRequestResult_BAD_TABLEID;
    }
    if (input->mNumRecordIds <= 0) {
        return SAKEStartRequestResult_BAD_NUM_RECORDIDS;
    }
    if (input->mRecordIds == NULL) {
        return SAKEStartRequestResult_BAD_RECORDIDS;
    }
    if (input->mNumFields <= 0) {
        return SAKEStartRequestResult_BAD_NUM_FIELDS;
    }
    if (input->mFieldNames != NULL) {
        return sakeiValidateRequestFieldNames(input->mFieldNames, input->mNumFields);
    }
    return SAKEStartRequestResult_BAD_FIELDS;
}

static SAKEStartRequestResult sakeiGetSpecificRecordsFillSoapRequest(SAKERequest request) {
    SAKEGetSpecificRecordsInput *input = request->mInput;

    gsXmlWriteStringElement(request->mSoapRequest, "ns1", "tableid", input->mTableId);
    sakeiFillSoapRequestRecordIds(request, input->mRecordIds, input->mNumRecordIds);
    sakeiFillSoapRequestFieldNames(request, input->mFieldNames, input->mNumFields);
    return 0;
}

static SAKERequestResult sakeiGetSpecificRecordsProcessSoapResponse(SAKERequest request) {
    SAKEGetSpecificRecordsInput *input = request->mInput;
    SAKEGetSpecificRecordsOutput *output = request->mOutput;

    return sakeiReadOutputRecords(request, &output->mRecords, &output->mNumRecords, input->mNumFields,
                                  input->mFieldNames);
}

static void sakeiGetSpecificRecordsFreeData(SAKERequest request) {
    SAKEGetSpecificRecordsInput *input = request->mInput;
    SAKEGetSpecificRecordsOutput *output = request->mOutput;

    if (output != NULL) {
        sakeiFreeOutputRecords(input->mNumFields, output->mNumRecords, output->mRecords);
    }
}

SAKEStartRequestResult sakeiStartGetSpecificRecordsRequest(SAKERequest request) {
    return sakeiStartRequest(request, &sGetSpecificRecordsType);
}

static SAKEStartRequestResult sakeiGetRandomRecordValidateInput(SAKERequest request) {
    SAKEGetRandomRecordInput *input = request->mInput;

    if (input->mTableId == NULL) {
        return SAKEStartRequestResult_BAD_TABLEID;
    }
    if (input->mNumFields <= 0) {
        return SAKEStartRequestResult_BAD_NUM_FIELDS;
    }
    if (input->mFieldNames != NULL) {
        return sakeiValidateRequestFieldNames(input->mFieldNames, input->mNumFields);
    }
    return SAKEStartRequestResult_BAD_FIELDS;
}

static SAKEStartRequestResult sakeiGetRandomRecordFillSoapRequest(SAKERequest request) {
    SAKEGetRandomRecordInput *input = request->mInput;

    gsXmlWriteStringElement(request->mSoapRequest, "ns1", "tableid", input->mTableId);
    if (input->mFilter != NULL) {
        gsXmlWriteStringElement(request->mSoapRequest, "ns1", "filter", input->mFilter);
    }
    gsXmlWriteIntElement(request->mSoapRequest, "ns1", "max", 1);
    sakeiFillSoapRequestFieldNames(request, input->mFieldNames, input->mNumFields);
    return 0;
}

static SAKERequestResult sakeiGetRandomRecordProcessSoapResponse(SAKERequest request) {
    SAKEGetRandomRecordInput *input = request->mInput;
    SAKEGetRandomRecordOutput *output = request->mOutput;
    SAKEField **records;
    int recordNum;
    int error;

    error = sakeiReadOutputRecords(request, &records, &recordNum, input->mNumFields, input->mFieldNames);
    if (error == 0) {
        if (records != NULL && recordNum > 0) {
            output->mRecord = records[0];
        } else {
            output->mRecord = NULL;
        }
    }
    gsifree(records);
    return error;
}

static void sakeiGetRandomRecordFreeData(SAKERequest request) {
    SAKEGetRandomRecordInput *input = request->mInput;
    SAKEGetRandomRecordOutput *output = request->mOutput;

    if (output != NULL) {
        sakeiFreeOutputRecord(input->mNumFields, output->mRecord);
    }
}
