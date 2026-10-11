#include "sake_internal.h"

// The requests that write a record to the storage server: creating, updating and deleting. A message lists the
// record's fields, each with its name and a value of the field's type. The three have a description of their kind that
// the request takes the elements and the check, write and read functions from.

static int Sake_CheckFields(const SakeField *fields, int fieldNum);
static void Sake_WriteFields(SakeRequest *request, SakeField *fields, int fieldNum);
static int Sake_ValidateCreateRecord(SakeRequest *request);
static int Sake_BuildCreateRecord(SakeRequest *request);
static int Sake_ParseCreateRecord(SakeRequest *request);
static int Sake_ValidateUpdateRecord(SakeRequest *request);
static int Sake_BuildUpdateRecord(SakeRequest *request);
static int Sake_ValidateDeleteRecord(SakeRequest *request);
static int Sake_BuildDeleteRecord(SakeRequest *request);

static SakeRequestType sCreateRecordType = {
    4,
    "CreateRecord",
    "SOAPAction: \"http://gamespy.net/sake/CreateRecord\"",
    "CreateRecordResponse",
    "CreateRecordResult",
    Sake_ValidateCreateRecord,
    Sake_BuildCreateRecord,
    Sake_ParseCreateRecord,
    NULL,
    NULL,
};

static SakeRequestType sUpdateRecordType = {
    0,
    "UpdateRecord",
    "SOAPAction: \"http://gamespy.net/sake/UpdateRecord\"",
    "UpdateRecordResponse",
    "UpdateRecordResult",
    Sake_ValidateUpdateRecord,
    Sake_BuildUpdateRecord,
    NULL,
    NULL,
    NULL,
};

static SakeRequestType sDeleteRecordType = {
    0,
    "DeleteRecord",
    "SOAPAction: \"http://gamespy.net/sake/DeleteRecord\"",
    "DeleteRecordResponse",
    "DeleteRecordResult",
    Sake_ValidateDeleteRecord,
    Sake_BuildDeleteRecord,
    NULL,
    NULL,
    NULL,
};

static int Sake_CheckFields(const SakeField *fields, int fieldNum) {
    int i;

    for (i = 0; i < fieldNum; i++) {
        const SakeField *field = &fields[i];

        if (field->name == NULL || field->name[0] == '\0') {
            return SAKE_REQUEST_ERROR_NO_FIELD_NAME;
        }
        if (field->type >= SAKE_FIELD_TYPE_NUM) {
            return SAKE_REQUEST_ERROR_BAD_FIELD_TYPE;
        }
        if (field->type == SAKE_FIELD_ASCII_STRING && field->value.asciiStringValue == NULL) {
            return SAKE_REQUEST_ERROR_BAD_FIELD_VALUE;
        }
        if (field->type == SAKE_FIELD_UNICODE_STRING && field->value.unicodeStringValue == NULL) {
            return SAKE_REQUEST_ERROR_BAD_FIELD_VALUE;
        }
        if (field->type == SAKE_FIELD_BINARY_DATA) {
            int length = field->value.binaryDataValue.length;

            if (length < 0) {
                return SAKE_REQUEST_ERROR_BAD_FIELD_VALUE;
            }
            if (field->value.binaryDataValue.data == NULL && length > 0) {
                return SAKE_REQUEST_ERROR_BAD_FIELD_VALUE;
            }
        }
    }
    return 0;
}

static void Sake_WriteFields(SakeRequest *request, SakeField *fields, int fieldNum) {
    int i;

    func_ov011_02170614(request->writer, "ns1", "values");
    for (i = 0; i < fieldNum; i++) {
        func_ov011_02170614(request->writer, "ns1", "RecordField");
        func_ov011_021706b8(request->writer, "ns1", "name", fields[i].name);
        func_ov011_02170614(request->writer, "ns1", "value");
        if (fields[i].type == SAKE_FIELD_BYTE) {
            func_ov011_02170614(request->writer, "ns1", "byteValue");
            func_ov011_02170818(request->writer, "ns1", "value", fields[i].value.byteValue);
            func_ov011_02170660(request->writer, "ns1", "byteValue");
        } else if (fields[i].type == SAKE_FIELD_SHORT) {
            func_ov011_02170614(request->writer, "ns1", "shortValue");
            func_ov011_02170818(request->writer, "ns1", "value", fields[i].value.shortValue);
            func_ov011_02170660(request->writer, "ns1", "shortValue");
        } else if (fields[i].type == SAKE_FIELD_INT) {
            func_ov011_02170614(request->writer, "ns1", "intValue");
            func_ov011_02170818(request->writer, "ns1", "value", fields[i].value.intValue);
            func_ov011_02170660(request->writer, "ns1", "intValue");
        } else if (fields[i].type == SAKE_FIELD_INT64) {
            func_ov011_02170614(request->writer, "ns1", "int64Value");
            func_ov011_02170868(request->writer, "ns1", "value", fields[i].value.int64Value);
            func_ov011_02170660(request->writer, "ns1", "int64Value");
        } else if (fields[i].type == SAKE_FIELD_FLOAT) {
            func_ov011_02170614(request->writer, "ns1", "floatValue");
            func_ov011_021708b4(request->writer, "ns1", "value", fields[i].value.floatValue);
            func_ov011_02170660(request->writer, "ns1", "floatValue");
        } else if (fields[i].type == SAKE_FIELD_ASCII_STRING) {
            func_ov011_02170614(request->writer, "ns1", "asciiStringValue");
            func_ov011_021706b8(request->writer, "ns1", "value", fields[i].value.asciiStringValue);
            func_ov011_02170660(request->writer, "ns1", "asciiStringValue");
        } else if (fields[i].type == SAKE_FIELD_UNICODE_STRING) {
            func_ov011_02170614(request->writer, "ns1", "unicodeStringValue");
            func_ov011_02170780(request->writer, "ns1", "value", fields[i].value.unicodeStringValue);
            func_ov011_02170660(request->writer, "ns1", "unicodeStringValue");
        } else if (fields[i].type == SAKE_FIELD_BOOLEAN) {
            func_ov011_02170614(request->writer, "ns1", "booleanValue");
            func_ov011_02170818(request->writer, "ns1", "value", fields[i].value.booleanValue != FALSE);
            func_ov011_02170660(request->writer, "ns1", "booleanValue");
        } else if (fields[i].type == SAKE_FIELD_DATE_AND_TIME) {
            func_ov011_02170614(request->writer, "ns1", "dateAndTimeValue");
            func_ov011_02170a00(request->writer, "ns1", "value", fields[i].value.dateAndTimeValue);
            func_ov011_02170660(request->writer, "ns1", "dateAndTimeValue");
        } else if (fields[i].type == SAKE_FIELD_BINARY_DATA) {
            func_ov011_02170614(request->writer, "ns1", "binaryDataValue");
            func_ov011_0217098c(request->writer, "ns1", "value", fields[i].value.binaryDataValue.data,
                                fields[i].value.binaryDataValue.length);
            func_ov011_02170660(request->writer, "ns1", "binaryDataValue");
        }
        func_ov011_02170660(request->writer, "ns1", "value");
        func_ov011_02170660(request->writer, "ns1", "RecordField");
    }
    func_ov011_02170660(request->writer, "ns1", "values");
}

static int Sake_ValidateCreateRecord(SakeRequest *request) {
    SakeCreateRecordInput *input = request->input;

    if (input->tableId == NULL) {
        return SAKE_REQUEST_ERROR_NO_TABLE;
    }
    if (input->fieldNum < 0) {
        return SAKE_REQUEST_ERROR_BAD_FIELD_NUM;
    }
    if (input->fields == NULL && input->fieldNum > 0) {
        return SAKE_REQUEST_ERROR_NO_FIELDS;
    }
    return Sake_CheckFields(input->fields, input->fieldNum);
}

static int Sake_BuildCreateRecord(SakeRequest *request) {
    SakeCreateRecordInput *input = request->input;

    func_ov011_021706b8(request->writer, "ns1", "tableid", input->tableId);
    Sake_WriteFields(request, input->fields, input->fieldNum);
    return 0;
}

static int Sake_ParseCreateRecord(SakeRequest *request) {
    if (func_ov011_02171090(request->response, "recordid", request->output) == FALSE) {
        return SAKE_RESULT_MALFORMED_RESPONSE;
    }
    return 0;
}

int Sake_SubmitCreateRecord(SakeRequest *request) {
    return Sake_Submit(request, &sCreateRecordType);
}

static int Sake_ValidateUpdateRecord(SakeRequest *request) {
    SakeUpdateRecordInput *input = request->input;

    if (input->tableId == NULL) {
        return SAKE_REQUEST_ERROR_NO_TABLE;
    }
    if (input->fieldNum <= 0) {
        return SAKE_REQUEST_ERROR_BAD_FIELD_NUM;
    }
    if (input->fields != NULL) {
        return Sake_CheckFields(input->fields, input->fieldNum);
    }
    return SAKE_REQUEST_ERROR_NO_FIELDS;
}

static int Sake_BuildUpdateRecord(SakeRequest *request) {
    SakeUpdateRecordInput *input = request->input;

    func_ov011_021706b8(request->writer, "ns1", "tableid", input->tableId);
    func_ov011_02170818(request->writer, "ns1", "recordid", input->recordId);
    Sake_WriteFields(request, input->fields, input->fieldNum);
    return 0;
}

int Sake_SubmitUpdateRecord(SakeRequest *request) {
    return Sake_Submit(request, &sUpdateRecordType);
}

static int Sake_ValidateDeleteRecord(SakeRequest *request) {
    SakeDeleteRecordInput *input = request->input;

    if (input->tableId != NULL) {
        return 0;
    }
    return SAKE_REQUEST_ERROR_NO_TABLE;
}

static int Sake_BuildDeleteRecord(SakeRequest *request) {
    SakeDeleteRecordInput *input = request->input;

    func_ov011_021706b8(request->writer, "ns1", "tableid", input->tableId);
    func_ov011_02170818(request->writer, "ns1", "recordid", input->recordId);
    return 0;
}
