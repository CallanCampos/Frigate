#include "sake_internal.h"

// The requests that read records from the storage server: those of the player, those of given IDs, a search of a
// table and random ones. A message lists the names of the fields to read, and the answer lists the records, each
// with a field of its own in the order of the names.

static SAKEStartRequestResult Sake_CheckNames(char **names, int nameNum);
static void Sake_WriteNames(SAKERequest request, char **names, int nameNum);
static void Sake_WriteRecordIds(SAKERequest request, int *recordIds, int recordNum);
static void Sake_WriteOwnerIds(SAKERequest request, int *ownerIds, int ownerNum);
static SAKERequestResult Sake_ParseRecords(SAKERequest request, SAKEField ***records, int *recordNum, int fieldNum,
                                           char **fieldNames);
static void Sake_FreeFields(int fieldNum, SAKEField *fields);
static void Sake_FreeRecords(int fieldNum, int recordNum, SAKEField **records);
static SAKEStartRequestResult Sake_ValidateSearchRecords(SAKERequest request);
static SAKEStartRequestResult Sake_BuildSearchRecords(SAKERequest request);
static SAKERequestResult Sake_ParseSearchRecords(SAKERequest request);
static void Sake_FreeSearchRecords(SAKERequest request);
static SAKEStartRequestResult Sake_ValidateGetMyRecords(SAKERequest request);
static SAKEStartRequestResult Sake_BuildGetMyRecords(SAKERequest request);
static SAKERequestResult Sake_ParseGetMyRecords(SAKERequest request);
static void Sake_FreeGetMyRecords(SAKERequest request);
static SAKEStartRequestResult Sake_ValidateGetSpecificRecords(SAKERequest request);
static SAKEStartRequestResult Sake_BuildGetSpecificRecords(SAKERequest request);
static SAKERequestResult Sake_ParseGetSpecificRecords(SAKERequest request);
static void Sake_FreeGetSpecificRecords(SAKERequest request);
static SAKEStartRequestResult Sake_ValidateGetRandomRecords(SAKERequest request);
static SAKEStartRequestResult Sake_BuildGetRandomRecords(SAKERequest request);
static SAKERequestResult Sake_ParseGetRandomRecords(SAKERequest request);
static void Sake_FreeGetRandomRecords(SAKERequest request);

static SAKEIRequestInfo sSearchRecordsType = {
    8,
    "SearchForRecords",
    "SOAPAction: \"http://gamespy.net/sake/SearchForRecords\"",
    "SearchForRecordsResponse",
    "SearchForRecordsResult",
    Sake_ValidateSearchRecords,
    Sake_BuildSearchRecords,
    Sake_ParseSearchRecords,
    Sake_FreeSearchRecords,
    NULL,
};

static SAKEIRequestInfo sGetMyRecordsType = {
    8,
    "GetMyRecords",
    "SOAPAction: \"http://gamespy.net/sake/GetMyRecords\"",
    "GetMyRecordsResponse",
    "GetMyRecordsResult",
    Sake_ValidateGetMyRecords,
    Sake_BuildGetMyRecords,
    Sake_ParseGetMyRecords,
    Sake_FreeGetMyRecords,
    NULL,
};

static SAKEIRequestInfo sGetSpecificRecordsType = {
    8,
    "GetSpecificRecords",
    "SOAPAction: \"http://gamespy.net/sake/GetSpecificRecords\"",
    "GetSpecificRecordsResponse",
    "GetSpecificRecordsResult",
    Sake_ValidateGetSpecificRecords,
    Sake_BuildGetSpecificRecords,
    Sake_ParseGetSpecificRecords,
    Sake_FreeGetSpecificRecords,
    NULL,
};

static SAKEIRequestInfo sGetRandomRecordsType = {
    4,
    "GetRandomRecords",
    "SOAPAction: \"http://gamespy.net/sake/GetRandomRecords\"",
    "GetRandomRecordsResponse",
    "GetRandomRecordsResult",
    Sake_ValidateGetRandomRecords,
    Sake_BuildGetRandomRecords,
    Sake_ParseGetRandomRecords,
    Sake_FreeGetRandomRecords,
    NULL,
};

static SAKEStartRequestResult Sake_CheckNames(char **names, int nameNum) {
    int i;

    for (i = 0; i < nameNum; i++) {
        if (names[i] == NULL || names[i][0] == '\0') {
            return SAKEStartRequestResult_BAD_FIELD_NAME;
        }
    }
    return 0;
}

static void Sake_WriteNames(SAKERequest request, char **names, int nameNum) {
    int i;

    gsXmlWriteOpenTag(request->writer, "ns1", "fields");
    for (i = 0; i < nameNum; i++) {
        gsXmlWriteStringElement(request->writer, "ns1", "string", names[i]);
    }
    gsXmlWriteCloseTag(request->writer, "ns1", "fields");
}

static void Sake_WriteRecordIds(SAKERequest request, int *recordIds, int recordNum) {
    int i;

    gsXmlWriteOpenTag(request->writer, "ns1", "recordids");
    for (i = 0; i < recordNum; i++) {
        gsXmlWriteIntElement(request->writer, "ns1", "int", recordIds[i]);
    }
    gsXmlWriteCloseTag(request->writer, "ns1", "recordids");
}

static void Sake_WriteOwnerIds(SAKERequest request, int *ownerIds, int ownerNum) {
    int i;

    gsXmlWriteOpenTag(request->writer, "ns1", "ownerids");
    for (i = 0; i < ownerNum; i++) {
        gsXmlWriteIntElement(request->writer, "ns1", "int", ownerIds[i]);
    }
    gsXmlWriteCloseTag(request->writer, "ns1", "ownerids");
}

static SAKERequestResult Sake_ParseRecords(SAKERequest request, SAKEField ***records, int *recordNum, int fieldNum,
                                           char **fieldNames) {
    SAKEField **list;
    SAKEField *record;
    int i;
    int j;
    int count;
    int listSize;

    if (gsXmlMoveToChild(request->response, "values") == gsi_false) {
        return SAKERequestResult_MALFORMED_RESPONSE;
    }
    count = gsXmlCountChildren(request->response, "ArrayOfRecordValue");
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
        if (gsXmlMoveToNext(request->response, "ArrayOfRecordValue") == gsi_false) {
            return SAKERequestResult_MALFORMED_RESPONSE;
        }
        record = gsimalloc(fieldNum * sizeof(SAKEField));
        if (record == NULL) {
            return SAKERequestResult_OUT_OF_MEMORY;
        }
        memset(record, 0, fieldNum * sizeof(SAKEField));
        list[i] = record;
        if (fieldNum != gsXmlCountChildren(request->response, "RecordValue")) {
            return SAKERequestResult_MALFORMED_RESPONSE;
        }
        for (j = 0; j < fieldNum; j++) {
            SAKEField *field = &record[j];

            field->name = fieldNames[j];
            if (gsXmlMoveToNext(request->response, "RecordValue") == gsi_false) {
                return SAKERequestResult_MALFORMED_RESPONSE;
            }
            if (gsXmlMoveToChild(request->response, "byteValue")) {
                int value;

                if (gsXmlReadChildAsInt(request->response, "value", &value) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                field->type = SAKEFieldType_BYTE;
                field->value.byteValue = value;
            } else if (gsXmlMoveToChild(request->response, "shortValue")) {
                int value;

                if (gsXmlReadChildAsInt(request->response, "value", &value) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                field->type = SAKEFieldType_SHORT;
                field->value.shortValue = value;
            } else if (gsXmlMoveToChild(request->response, "intValue")) {
                int value;

                if (gsXmlReadChildAsInt(request->response, "value", &value) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                field->type = SAKEFieldType_INT;
                field->value.intValue = value;
            } else if (gsXmlMoveToChild(request->response, "int64Value")) {
                gsi_i64 value;

                if (gsXmlReadChildAsInt64(request->response, "value", &value) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                field->type = SAKEFieldType_INT64;
                field->value.int64Value = value;
            } else if (gsXmlMoveToChild(request->response, "floatValue")) {
                float value;

                if (gsXmlReadChildAsFloat(request->response, "value", &value) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                field->type = SAKEFieldType_FLOAT;
                field->value.floatValue = value;
            } else if (gsXmlMoveToChild(request->response, "asciiStringValue")) {
                char *text;
                int length;

                if (gsXmlReadChildAsString(request->response, "value", (const char **)&text, &length) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                if (text != NULL) {
                    text[length] = '\0';
                } else {
                    text = "";
                }
                field->type = SAKEFieldType_ASCII_STRING;
                field->value.asciiStringValue = text;
            } else if (gsXmlMoveToChild(request->response, "unicodeStringValue")) {
                char *text;
                int length;
                gsi_u16 *wide;

                if (gsXmlReadChildAsString(request->response, "value", (const char **)&text, &length) == gsi_false) {
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
                field->type = SAKEFieldType_UNICODE_STRING;
                field->value.unicodeStringValue = wide;
            } else if (gsXmlMoveToChild(request->response, "booleanValue")) {
                char *text;
                int length;
                gsi_bool value;

                if (gsXmlReadChildAsString(request->response, "value", (const char **)&text, &length) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                if (text != NULL) {
                    text[length] = '\0';
                    value = strcmp(text, "true") == 0;
                } else {
                    value = gsi_false;
                }
                field->type = SAKEFieldType_BOOLEAN;
                field->value.booleanValue = value;
            } else if (gsXmlMoveToChild(request->response, "dateAndTimeValue")) {
                int value;

                if (gsXmlReadChildAsDateTimeElement(request->response, "value", &value) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                field->type = SAKEFieldType_DATE_AND_TIME;
                field->value.dateAndTimeValue = value;
            } else if (gsXmlMoveToChild(request->response, "binaryDataValue")) {
                void *data;
                int length;

                if (gsXmlReadChildAsBase64Binary(request->response, "value", NULL, &length) == gsi_false) {
                    return SAKERequestResult_MALFORMED_RESPONSE;
                }
                if (length > 0) {
                    data = gsimalloc(length);
                    if (data == NULL) {
                        return SAKERequestResult_OUT_OF_MEMORY;
                    }
                    if (gsXmlReadChildAsBase64Binary(request->response, "value", data, &length) == gsi_false) {
                        gsifree(data);
                        return SAKERequestResult_MALFORMED_RESPONSE;
                    }
                } else {
                    data = NULL;
                    length = 0;
                }
                field->type = SAKEFieldType_BINARY_DATA;
                field->value.binaryDataValue.length = length;
                field->value.binaryDataValue.data = data;
            } else {
                return SAKERequestResult_UNKNOWN_ERROR;
            }
        }
    }
    return 0;
}

static void Sake_FreeFields(int fieldNum, SAKEField *fields) {
    int i;

    if (fields == NULL) {
        return;
    }
    for (i = 0; i < fieldNum; i++) {
        SAKEField *field = &fields[i];

        if (field->type == SAKEFieldType_BINARY_DATA && field->value.binaryDataValue.data != NULL) {
            gsifree(field->value.binaryDataValue.data);
        }
        if (field->type == SAKEFieldType_UNICODE_STRING) {
            gsifree(field->value.unicodeStringValue);
        }
    }
    gsifree(fields);
}

static void Sake_FreeRecords(int fieldNum, int recordNum, SAKEField **records) {
    int i;
    int j;

    if (records == NULL) {
        return;
    }
    for (i = 0; i < recordNum; i++) {
        if (records[i] != NULL) {
            for (j = 0; j < fieldNum; j++) {
                if (records[i][j].type == SAKEFieldType_BINARY_DATA &&
                    records[i][j].value.binaryDataValue.data != NULL) {
                    gsifree(records[i][j].value.binaryDataValue.data);
                }
                if (records[i][j].type == SAKEFieldType_UNICODE_STRING) {
                    gsifree(records[i][j].value.unicodeStringValue);
                }
            }
            gsifree(records[i]);
            records[i] = NULL;
        }
    }
    gsifree(records);
}

static SAKEStartRequestResult Sake_ValidateSearchRecords(SAKERequest request) {
    SAKESearchForRecordsInput *input = request->input;

    if (input->tableId == NULL) {
        return SAKEStartRequestResult_BAD_TABLEID;
    }
    if (input->fieldNum <= 0) {
        return SAKEStartRequestResult_BAD_NUM_FIELDS;
    }
    if (input->fieldNames == NULL) {
        return SAKEStartRequestResult_BAD_FIELDS;
    }
    if (input->offset < 0) {
        return SAKEStartRequestResult_BAD_OFFSET;
    }
    if (input->max <= 0) {
        return SAKEStartRequestResult_BAD_MAX;
    }
    return Sake_CheckNames(input->fieldNames, input->fieldNum);
}

static SAKEStartRequestResult Sake_BuildSearchRecords(SAKERequest request) {
    SAKESearchForRecordsInput *input = request->input;

    gsXmlWriteStringElement(request->writer, "ns1", "tableid", input->tableId);
    if (input->filter != NULL) {
        gsXmlWriteStringElement(request->writer, "ns1", "filter", input->filter);
    }
    if (input->sort != NULL) {
        gsXmlWriteStringElement(request->writer, "ns1", "sort", input->sort);
    }
    gsXmlWriteIntElement(request->writer, "ns1", "offset", input->offset);
    gsXmlWriteIntElement(request->writer, "ns1", "max", input->max);
    if (input->targetFilter != NULL) {
        gsXmlWriteStringElement(request->writer, "ns1", "targetfilter", input->targetFilter);
    }
    gsXmlWriteIntElement(request->writer, "ns1", "surrounding", input->surrounding);
    Sake_WriteOwnerIds(request, input->ownerIds, input->ownerNum);
    gsXmlWriteIntElement(request->writer, "ns1", "cacheFlag", input->cacheFlag);
    Sake_WriteNames(request, input->fieldNames, input->fieldNum);
    return 0;
}

static SAKERequestResult Sake_ParseSearchRecords(SAKERequest request) {
    SAKESearchForRecordsInput *input = request->input;
    SAKESearchForRecordsOutput *output = request->output;

    return Sake_ParseRecords(request, &output->records, &output->recordNum, input->fieldNum, input->fieldNames);
}

static void Sake_FreeSearchRecords(SAKERequest request) {
    SAKESearchForRecordsInput *input = request->input;
    SAKESearchForRecordsOutput *output = request->output;

    if (output != NULL) {
        Sake_FreeRecords(input->fieldNum, output->recordNum, output->records);
    }
}

static SAKEStartRequestResult Sake_ValidateGetMyRecords(SAKERequest request) {
    SAKEGetMyRecordsInput *input = request->input;

    if (input->tableId == NULL) {
        return SAKEStartRequestResult_BAD_TABLEID;
    }
    if (input->fieldNum <= 0) {
        return SAKEStartRequestResult_BAD_NUM_FIELDS;
    }
    if (input->fieldNames != NULL) {
        return Sake_CheckNames(input->fieldNames, input->fieldNum);
    }
    return SAKEStartRequestResult_BAD_FIELDS;
}

static SAKEStartRequestResult Sake_BuildGetMyRecords(SAKERequest request) {
    SAKEGetMyRecordsInput *input = request->input;

    gsXmlWriteStringElement(request->writer, "ns1", "tableid", input->tableId);
    Sake_WriteNames(request, input->fieldNames, input->fieldNum);
    return 0;
}

static SAKERequestResult Sake_ParseGetMyRecords(SAKERequest request) {
    SAKEGetMyRecordsInput *input = request->input;
    SAKEGetMyRecordsOutput *output = request->output;

    return Sake_ParseRecords(request, &output->records, &output->recordNum, input->fieldNum, input->fieldNames);
}

static void Sake_FreeGetMyRecords(SAKERequest request) {
    SAKEGetMyRecordsInput *input = request->input;
    SAKEGetMyRecordsOutput *output = request->output;

    if (output != NULL) {
        Sake_FreeRecords(input->fieldNum, output->recordNum, output->records);
    }
}

SAKEStartRequestResult Sake_SubmitGetMyRecords(SAKERequest request) {
    return Sake_Submit(request, &sGetMyRecordsType);
}

static SAKEStartRequestResult Sake_ValidateGetSpecificRecords(SAKERequest request) {
    SAKEGetSpecificRecordsInput *input = request->input;

    if (input->tableId == NULL) {
        return SAKEStartRequestResult_BAD_TABLEID;
    }
    if (input->recordNum <= 0) {
        return SAKEStartRequestResult_BAD_NUM_RECORDIDS;
    }
    if (input->recordIds == NULL) {
        return SAKEStartRequestResult_BAD_RECORDIDS;
    }
    if (input->fieldNum <= 0) {
        return SAKEStartRequestResult_BAD_NUM_FIELDS;
    }
    if (input->fieldNames != NULL) {
        return Sake_CheckNames(input->fieldNames, input->fieldNum);
    }
    return SAKEStartRequestResult_BAD_FIELDS;
}

static SAKEStartRequestResult Sake_BuildGetSpecificRecords(SAKERequest request) {
    SAKEGetSpecificRecordsInput *input = request->input;

    gsXmlWriteStringElement(request->writer, "ns1", "tableid", input->tableId);
    Sake_WriteRecordIds(request, input->recordIds, input->recordNum);
    Sake_WriteNames(request, input->fieldNames, input->fieldNum);
    return 0;
}

static SAKERequestResult Sake_ParseGetSpecificRecords(SAKERequest request) {
    SAKEGetSpecificRecordsInput *input = request->input;
    SAKEGetSpecificRecordsOutput *output = request->output;

    return Sake_ParseRecords(request, &output->records, &output->recordNum, input->fieldNum, input->fieldNames);
}

static void Sake_FreeGetSpecificRecords(SAKERequest request) {
    SAKEGetSpecificRecordsInput *input = request->input;
    SAKEGetSpecificRecordsOutput *output = request->output;

    if (output != NULL) {
        Sake_FreeRecords(input->fieldNum, output->recordNum, output->records);
    }
}

SAKEStartRequestResult Sake_SubmitGetSpecificRecords(SAKERequest request) {
    return Sake_Submit(request, &sGetSpecificRecordsType);
}

static SAKEStartRequestResult Sake_ValidateGetRandomRecords(SAKERequest request) {
    SAKEGetRandomRecordInput *input = request->input;

    if (input->tableId == NULL) {
        return SAKEStartRequestResult_BAD_TABLEID;
    }
    if (input->fieldNum <= 0) {
        return SAKEStartRequestResult_BAD_NUM_FIELDS;
    }
    if (input->fieldNames != NULL) {
        return Sake_CheckNames(input->fieldNames, input->fieldNum);
    }
    return SAKEStartRequestResult_BAD_FIELDS;
}

static SAKEStartRequestResult Sake_BuildGetRandomRecords(SAKERequest request) {
    SAKEGetRandomRecordInput *input = request->input;

    gsXmlWriteStringElement(request->writer, "ns1", "tableid", input->tableId);
    if (input->filter != NULL) {
        gsXmlWriteStringElement(request->writer, "ns1", "filter", input->filter);
    }
    gsXmlWriteIntElement(request->writer, "ns1", "max", 1);
    Sake_WriteNames(request, input->fieldNames, input->fieldNum);
    return 0;
}

static SAKERequestResult Sake_ParseGetRandomRecords(SAKERequest request) {
    SAKEGetRandomRecordInput *input = request->input;
    SAKEGetRandomRecordOutput *output = request->output;
    SAKEField **records;
    int recordNum;
    int error;

    error = Sake_ParseRecords(request, &records, &recordNum, input->fieldNum, input->fieldNames);
    if (error == 0) {
        if (records != NULL && recordNum > 0) {
            output->record = records[0];
        } else {
            output->record = NULL;
        }
    }
    gsifree(records);
    return error;
}

static void Sake_FreeGetRandomRecords(SAKERequest request) {
    SAKEGetRandomRecordInput *input = request->input;
    SAKEGetRandomRecordOutput *output = request->output;

    if (output != NULL) {
        Sake_FreeFields(input->fieldNum, output->record);
    }
}
