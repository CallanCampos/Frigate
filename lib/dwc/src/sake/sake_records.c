#include "sake_internal.h"

// The requests that read records from the storage server: those of the player, those of given IDs, a search of a
// table and random ones. A message lists the names of the fields to read, and the answer lists the records, each
// with a field of its own in the order of the names.

static int Sake_CheckNames(const char **names, int nameNum);
static void Sake_WriteNames(SakeRequest *request, const char **names, int nameNum);
static void Sake_WriteRecordIds(SakeRequest *request, const int *recordIds, int recordNum);
static void Sake_WriteOwnerIds(SakeRequest *request, const int *ownerIds, int ownerNum);
static int Sake_ParseRecords(SakeRequest *request, SakeField ***records, int *recordNum, int fieldNum,
                             const char **fieldNames);
static void Sake_FreeFields(int fieldNum, SakeField *fields);
static void Sake_FreeRecords(int fieldNum, int recordNum, SakeField **records);
static int Sake_ValidateSearchRecords(SakeRequest *request);
static int Sake_BuildSearchRecords(SakeRequest *request);
static int Sake_ParseSearchRecords(SakeRequest *request);
static void Sake_FreeSearchRecords(SakeRequest *request);
static int Sake_ValidateGetMyRecords(SakeRequest *request);
static int Sake_BuildGetMyRecords(SakeRequest *request);
static int Sake_ParseGetMyRecords(SakeRequest *request);
static void Sake_FreeGetMyRecords(SakeRequest *request);
static int Sake_ValidateGetSpecificRecords(SakeRequest *request);
static int Sake_BuildGetSpecificRecords(SakeRequest *request);
static int Sake_ParseGetSpecificRecords(SakeRequest *request);
static void Sake_FreeGetSpecificRecords(SakeRequest *request);
static int Sake_ValidateGetRandomRecords(SakeRequest *request);
static int Sake_BuildGetRandomRecords(SakeRequest *request);
static int Sake_ParseGetRandomRecords(SakeRequest *request);
static void Sake_FreeGetRandomRecords(SakeRequest *request);

static SakeRequestType sSearchRecordsType = {
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

static SakeRequestType sGetMyRecordsType = {
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

static SakeRequestType sGetSpecificRecordsType = {
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

static SakeRequestType sGetRandomRecordsType = {
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

static int Sake_CheckNames(const char **names, int nameNum) {
    int i;

    for (i = 0; i < nameNum; i++) {
        if (names[i] == NULL || names[i][0] == '\0') {
            return SAKE_REQUEST_ERROR_NO_FIELD_NAME;
        }
    }
    return 0;
}

static void Sake_WriteNames(SakeRequest *request, const char **names, int nameNum) {
    int i;

    func_ov011_02170614(request->writer, "ns1", "fields");
    for (i = 0; i < nameNum; i++) {
        func_ov011_021706b8(request->writer, "ns1", "string", names[i]);
    }
    func_ov011_02170660(request->writer, "ns1", "fields");
}

static void Sake_WriteRecordIds(SakeRequest *request, const int *recordIds, int recordNum) {
    int i;

    func_ov011_02170614(request->writer, "ns1", "recordids");
    for (i = 0; i < recordNum; i++) {
        func_ov011_02170818(request->writer, "ns1", "int", recordIds[i]);
    }
    func_ov011_02170660(request->writer, "ns1", "recordids");
}

static void Sake_WriteOwnerIds(SakeRequest *request, const int *ownerIds, int ownerNum) {
    int i;

    func_ov011_02170614(request->writer, "ns1", "ownerids");
    for (i = 0; i < ownerNum; i++) {
        func_ov011_02170818(request->writer, "ns1", "int", ownerIds[i]);
    }
    func_ov011_02170660(request->writer, "ns1", "ownerids");
}

static int Sake_ParseRecords(SakeRequest *request, SakeField ***records, int *recordNum, int fieldNum,
                             const char **fieldNames) {
    SakeField **list;
    SakeField *record;
    int i;
    int j;
    int count;
    int listSize;

    if (func_ov011_02170db8(request->response, "values") == FALSE) {
        return SAKE_RESULT_MALFORMED_RESPONSE;
    }
    count = func_ov011_02171390(request->response, "ArrayOfRecordValue");
    *recordNum = count;
    if (count == 0) {
        *records = NULL;
        return 0;
    }
    listSize = count * sizeof(SakeField *);
    list = func_ov011_0216dddc(listSize);
    if (list == NULL) {
        return SAKE_RESULT_OUT_OF_MEMORY;
    }
    memset(list, 0, listSize);
    *records = list;
    for (i = 0; i < *recordNum; i++) {
        if (func_ov011_02170d28(request->response, "ArrayOfRecordValue") == FALSE) {
            return SAKE_RESULT_MALFORMED_RESPONSE;
        }
        record = func_ov011_0216dddc(fieldNum * sizeof(SakeField));
        if (record == NULL) {
            return SAKE_RESULT_OUT_OF_MEMORY;
        }
        memset(record, 0, fieldNum * sizeof(SakeField));
        list[i] = record;
        if (fieldNum != func_ov011_02171390(request->response, "RecordValue")) {
            return SAKE_RESULT_MALFORMED_RESPONSE;
        }
        for (j = 0; j < fieldNum; j++) {
            SakeField *field = &record[j];

            field->name = fieldNames[j];
            if (func_ov011_02170d28(request->response, "RecordValue") == FALSE) {
                return SAKE_RESULT_MALFORMED_RESPONSE;
            }
            if (func_ov011_02170db8(request->response, "byteValue")) {
                int value;

                if (func_ov011_02171090(request->response, "value", &value) == FALSE) {
                    return SAKE_RESULT_MALFORMED_RESPONSE;
                }
                field->type = SAKE_FIELD_BYTE;
                field->value.byteValue = value;
            } else if (func_ov011_02170db8(request->response, "shortValue")) {
                int value;

                if (func_ov011_02171090(request->response, "value", &value) == FALSE) {
                    return SAKE_RESULT_MALFORMED_RESPONSE;
                }
                field->type = SAKE_FIELD_SHORT;
                field->value.shortValue = value;
            } else if (func_ov011_02170db8(request->response, "intValue")) {
                int value;

                if (func_ov011_02171090(request->response, "value", &value) == FALSE) {
                    return SAKE_RESULT_MALFORMED_RESPONSE;
                }
                field->type = SAKE_FIELD_INT;
                field->value.intValue = value;
            } else if (func_ov011_02170db8(request->response, "int64Value")) {
                s64 value;

                if (func_ov011_02171108(request->response, "value", &value) == FALSE) {
                    return SAKE_RESULT_MALFORMED_RESPONSE;
                }
                field->type = SAKE_FIELD_INT64;
                field->value.int64Value = value;
            } else if (func_ov011_02170db8(request->response, "floatValue")) {
                float value;

                if (func_ov011_0217123c(request->response, "value", &value) == FALSE) {
                    return SAKE_RESULT_MALFORMED_RESPONSE;
                }
                field->type = SAKE_FIELD_FLOAT;
                field->value.floatValue = value;
            } else if (func_ov011_02170db8(request->response, "asciiStringValue")) {
                char *text;
                int length;

                if (func_ov011_02170e14(request->response, "value", &text, &length) == FALSE) {
                    return SAKE_RESULT_MALFORMED_RESPONSE;
                }
                if (text != NULL) {
                    text[length] = '\0';
                } else {
                    text = "";
                }
                field->type = SAKE_FIELD_ASCII_STRING;
                field->value.asciiStringValue = text;
            } else if (func_ov011_02170db8(request->response, "unicodeStringValue")) {
                char *text;
                int length;
                u16 *wide;

                if (func_ov011_02170e14(request->response, "value", &text, &length) == FALSE) {
                    return SAKE_RESULT_MALFORMED_RESPONSE;
                }
                if (text != NULL) {
                    text[length] = '\0';
                } else {
                    text = "";
                }
                wide = func_ov011_0216edd0(text);
                if (wide == NULL) {
                    return SAKE_RESULT_OUT_OF_MEMORY;
                }
                field->type = SAKE_FIELD_UNICODE_STRING;
                field->value.unicodeStringValue = wide;
            } else if (func_ov011_02170db8(request->response, "booleanValue")) {
                char *text;
                int length;
                BOOL value;

                if (func_ov011_02170e14(request->response, "value", &text, &length) == FALSE) {
                    return SAKE_RESULT_MALFORMED_RESPONSE;
                }
                if (text != NULL) {
                    text[length] = '\0';
                    value = strcmp(text, "true") == 0;
                } else {
                    value = FALSE;
                }
                field->type = SAKE_FIELD_BOOLEAN;
                field->value.booleanValue = value;
            } else if (func_ov011_02170db8(request->response, "dateAndTimeValue")) {
                int value;

                if (func_ov011_02171184(request->response, "value", &value) == FALSE) {
                    return SAKE_RESULT_MALFORMED_RESPONSE;
                }
                field->type = SAKE_FIELD_DATE_AND_TIME;
                field->value.dateAndTimeValue = value;
            } else if (func_ov011_02170db8(request->response, "binaryDataValue")) {
                void *data;
                int length;

                if (func_ov011_02170fec(request->response, "value", NULL, &length) == FALSE) {
                    return SAKE_RESULT_MALFORMED_RESPONSE;
                }
                if (length > 0) {
                    data = func_ov011_0216dddc(length);
                    if (data == NULL) {
                        return SAKE_RESULT_OUT_OF_MEMORY;
                    }
                    if (func_ov011_02170fec(request->response, "value", data, &length) == FALSE) {
                        func_ov011_0216ddfc(data);
                        return SAKE_RESULT_MALFORMED_RESPONSE;
                    }
                } else {
                    data = NULL;
                    length = 0;
                }
                field->type = SAKE_FIELD_BINARY_DATA;
                field->value.binaryDataValue.length = length;
                field->value.binaryDataValue.data = data;
            } else {
                return SAKE_RESULT_UNKNOWN;
            }
        }
    }
    return 0;
}

static void Sake_FreeFields(int fieldNum, SakeField *fields) {
    int i;

    if (fields == NULL) {
        return;
    }
    for (i = 0; i < fieldNum; i++) {
        SakeField *field = &fields[i];

        if (field->type == SAKE_FIELD_BINARY_DATA && field->value.binaryDataValue.data != NULL) {
            func_ov011_0216ddfc(field->value.binaryDataValue.data);
        }
        if (field->type == SAKE_FIELD_UNICODE_STRING) {
            func_ov011_0216ddfc(field->value.unicodeStringValue);
        }
    }
    func_ov011_0216ddfc(fields);
}

static void Sake_FreeRecords(int fieldNum, int recordNum, SakeField **records) {
    int i;
    int j;

    if (records == NULL) {
        return;
    }
    for (i = 0; i < recordNum; i++) {
        if (records[i] != NULL) {
            for (j = 0; j < fieldNum; j++) {
                if (records[i][j].type == SAKE_FIELD_BINARY_DATA && records[i][j].value.binaryDataValue.data != NULL) {
                    func_ov011_0216ddfc(records[i][j].value.binaryDataValue.data);
                }
                if (records[i][j].type == SAKE_FIELD_UNICODE_STRING) {
                    func_ov011_0216ddfc(records[i][j].value.unicodeStringValue);
                }
            }
            func_ov011_0216ddfc(records[i]);
            records[i] = NULL;
        }
    }
    func_ov011_0216ddfc(records);
}

static int Sake_ValidateSearchRecords(SakeRequest *request) {
    SakeSearchRecordsInput *input = request->input;

    if (input->tableId == NULL) {
        return SAKE_REQUEST_ERROR_NO_TABLE;
    }
    if (input->fieldNum <= 0) {
        return SAKE_REQUEST_ERROR_BAD_FIELD_NUM;
    }
    if (input->fieldNames == NULL) {
        return SAKE_REQUEST_ERROR_NO_FIELDS;
    }
    if (input->offset < 0) {
        return SAKE_REQUEST_ERROR_BAD_OFFSET;
    }
    if (input->max <= 0) {
        return SAKE_REQUEST_ERROR_BAD_MAX;
    }
    return Sake_CheckNames(input->fieldNames, input->fieldNum);
}

static int Sake_BuildSearchRecords(SakeRequest *request) {
    SakeSearchRecordsInput *input = request->input;

    func_ov011_021706b8(request->writer, "ns1", "tableid", input->tableId);
    if (input->filter != NULL) {
        func_ov011_021706b8(request->writer, "ns1", "filter", input->filter);
    }
    if (input->sort != NULL) {
        func_ov011_021706b8(request->writer, "ns1", "sort", input->sort);
    }
    func_ov011_02170818(request->writer, "ns1", "offset", input->offset);
    func_ov011_02170818(request->writer, "ns1", "max", input->max);
    if (input->targetFilter != NULL) {
        func_ov011_021706b8(request->writer, "ns1", "targetfilter", input->targetFilter);
    }
    func_ov011_02170818(request->writer, "ns1", "surrounding", input->surrounding);
    Sake_WriteOwnerIds(request, input->ownerIds, input->ownerNum);
    func_ov011_02170818(request->writer, "ns1", "cacheFlag", input->cacheFlag);
    Sake_WriteNames(request, input->fieldNames, input->fieldNum);
    return 0;
}

static int Sake_ParseSearchRecords(SakeRequest *request) {
    SakeSearchRecordsInput *input = request->input;
    SakeGetRecordsOutput *output = request->output;

    return Sake_ParseRecords(request, &output->records, &output->recordNum, input->fieldNum, input->fieldNames);
}

static void Sake_FreeSearchRecords(SakeRequest *request) {
    SakeSearchRecordsInput *input = request->input;
    SakeGetRecordsOutput *output = request->output;

    if (output != NULL) {
        Sake_FreeRecords(input->fieldNum, output->recordNum, output->records);
    }
}

static int Sake_ValidateGetMyRecords(SakeRequest *request) {
    SakeGetMyRecordsInput *input = request->input;

    if (input->tableId == NULL) {
        return SAKE_REQUEST_ERROR_NO_TABLE;
    }
    if (input->fieldNum <= 0) {
        return SAKE_REQUEST_ERROR_BAD_FIELD_NUM;
    }
    if (input->fieldNames != NULL) {
        return Sake_CheckNames(input->fieldNames, input->fieldNum);
    }
    return SAKE_REQUEST_ERROR_NO_FIELDS;
}

static int Sake_BuildGetMyRecords(SakeRequest *request) {
    SakeGetMyRecordsInput *input = request->input;

    func_ov011_021706b8(request->writer, "ns1", "tableid", input->tableId);
    Sake_WriteNames(request, input->fieldNames, input->fieldNum);
    return 0;
}

static int Sake_ParseGetMyRecords(SakeRequest *request) {
    SakeGetMyRecordsInput *input = request->input;
    SakeGetRecordsOutput *output = request->output;

    return Sake_ParseRecords(request, &output->records, &output->recordNum, input->fieldNum, input->fieldNames);
}

static void Sake_FreeGetMyRecords(SakeRequest *request) {
    SakeGetMyRecordsInput *input = request->input;
    SakeGetRecordsOutput *output = request->output;

    if (output != NULL) {
        Sake_FreeRecords(input->fieldNum, output->recordNum, output->records);
    }
}

int Sake_SubmitGetMyRecords(SakeRequest *request) {
    return Sake_Submit(request, &sGetMyRecordsType);
}

static int Sake_ValidateGetSpecificRecords(SakeRequest *request) {
    SakeGetSpecificRecordsInput *input = request->input;

    if (input->tableId == NULL) {
        return SAKE_REQUEST_ERROR_NO_TABLE;
    }
    if (input->recordNum <= 0) {
        return SAKE_REQUEST_ERROR_BAD_RECORD_NUM;
    }
    if (input->recordIds == NULL) {
        return SAKE_REQUEST_ERROR_NO_RECORD_IDS;
    }
    if (input->fieldNum <= 0) {
        return SAKE_REQUEST_ERROR_BAD_FIELD_NUM;
    }
    if (input->fieldNames != NULL) {
        return Sake_CheckNames(input->fieldNames, input->fieldNum);
    }
    return SAKE_REQUEST_ERROR_NO_FIELDS;
}

static int Sake_BuildGetSpecificRecords(SakeRequest *request) {
    SakeGetSpecificRecordsInput *input = request->input;

    func_ov011_021706b8(request->writer, "ns1", "tableid", input->tableId);
    Sake_WriteRecordIds(request, input->recordIds, input->recordNum);
    Sake_WriteNames(request, input->fieldNames, input->fieldNum);
    return 0;
}

static int Sake_ParseGetSpecificRecords(SakeRequest *request) {
    SakeGetSpecificRecordsInput *input = request->input;
    SakeGetRecordsOutput *output = request->output;

    return Sake_ParseRecords(request, &output->records, &output->recordNum, input->fieldNum, input->fieldNames);
}

static void Sake_FreeGetSpecificRecords(SakeRequest *request) {
    SakeGetSpecificRecordsInput *input = request->input;
    SakeGetRecordsOutput *output = request->output;

    if (output != NULL) {
        Sake_FreeRecords(input->fieldNum, output->recordNum, output->records);
    }
}

int Sake_SubmitGetSpecificRecords(SakeRequest *request) {
    return Sake_Submit(request, &sGetSpecificRecordsType);
}

static int Sake_ValidateGetRandomRecords(SakeRequest *request) {
    SakeGetRandomRecordsInput *input = request->input;

    if (input->tableId == NULL) {
        return SAKE_REQUEST_ERROR_NO_TABLE;
    }
    if (input->fieldNum <= 0) {
        return SAKE_REQUEST_ERROR_BAD_FIELD_NUM;
    }
    if (input->fieldNames != NULL) {
        return Sake_CheckNames(input->fieldNames, input->fieldNum);
    }
    return SAKE_REQUEST_ERROR_NO_FIELDS;
}

static int Sake_BuildGetRandomRecords(SakeRequest *request) {
    SakeGetRandomRecordsInput *input = request->input;

    func_ov011_021706b8(request->writer, "ns1", "tableid", input->tableId);
    if (input->filter != NULL) {
        func_ov011_021706b8(request->writer, "ns1", "filter", input->filter);
    }
    func_ov011_02170818(request->writer, "ns1", "max", 1);
    Sake_WriteNames(request, input->fieldNames, input->fieldNum);
    return 0;
}

static int Sake_ParseGetRandomRecords(SakeRequest *request) {
    SakeGetRandomRecordsInput *input = request->input;
    SakeField **output = request->output;
    SakeField **records;
    int recordNum;
    int error;

    error = Sake_ParseRecords(request, &records, &recordNum, input->fieldNum, input->fieldNames);
    if (error == 0) {
        if (records != NULL && recordNum > 0) {
            *output = records[0];
        } else {
            *output = NULL;
        }
    }
    func_ov011_0216ddfc(records);
    return error;
}

static void Sake_FreeGetRandomRecords(SakeRequest *request) {
    SakeGetRandomRecordsInput *input = request->input;
    SakeField **output = request->output;

    if (output != NULL) {
        Sake_FreeFields(input->fieldNum, *output);
    }
}
