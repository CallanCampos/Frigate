#include "sake_internal.h"

// The requests that write a record to the storage server: creating, updating and deleting. A message lists the
// record's fields, each with its name and a value of the field's type. The three have a description of their kind that
// the request takes the elements and the check, write and read functions from.

static SAKEStartRequestResult Sake_CheckFields(SAKEField *fields, int fieldNum);
static void Sake_WriteFields(SAKERequest request, SAKEField *fields, int fieldNum);
static SAKEStartRequestResult Sake_ValidateCreateRecord(SAKERequest request);
static SAKEStartRequestResult Sake_BuildCreateRecord(SAKERequest request);
static SAKERequestResult Sake_ParseCreateRecord(SAKERequest request);
static SAKEStartRequestResult Sake_ValidateUpdateRecord(SAKERequest request);
static SAKEStartRequestResult Sake_BuildUpdateRecord(SAKERequest request);
static SAKEStartRequestResult Sake_ValidateDeleteRecord(SAKERequest request);
static SAKEStartRequestResult Sake_BuildDeleteRecord(SAKERequest request);

static SAKEIRequestInfo sCreateRecordType = {
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

static SAKEIRequestInfo sUpdateRecordType = {
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

static SAKEIRequestInfo sDeleteRecordType = {
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

static SAKEStartRequestResult Sake_CheckFields(SAKEField *fields, int fieldNum) {
    int i;

    for (i = 0; i < fieldNum; i++) {
        SAKEField *field = &fields[i];

        if (field->name == NULL || field->name[0] == '\0') {
            return SAKEStartRequestResult_BAD_FIELD_NAME;
        }
        if (field->type >= SAKEFieldType_NUM_FIELD_TYPES) {
            return SAKEStartRequestResult_BAD_FIELD_TYPE;
        }
        if (field->type == SAKEFieldType_ASCII_STRING && field->value.asciiStringValue == NULL) {
            return SAKEStartRequestResult_BAD_FIELD_VALUE;
        }
        if (field->type == SAKEFieldType_UNICODE_STRING && field->value.unicodeStringValue == NULL) {
            return SAKEStartRequestResult_BAD_FIELD_VALUE;
        }
        if (field->type == SAKEFieldType_BINARY_DATA) {
            int length = field->value.binaryDataValue.length;

            if (length < 0) {
                return SAKEStartRequestResult_BAD_FIELD_VALUE;
            }
            if (field->value.binaryDataValue.data == NULL && length > 0) {
                return SAKEStartRequestResult_BAD_FIELD_VALUE;
            }
        }
    }
    return 0;
}

static void Sake_WriteFields(SAKERequest request, SAKEField *fields, int fieldNum) {
    int i;

    gsXmlWriteOpenTag(request->writer, "ns1", "values");
    for (i = 0; i < fieldNum; i++) {
        gsXmlWriteOpenTag(request->writer, "ns1", "RecordField");
        gsXmlWriteStringElement(request->writer, "ns1", "name", fields[i].name);
        gsXmlWriteOpenTag(request->writer, "ns1", "value");
        if (fields[i].type == SAKEFieldType_BYTE) {
            gsXmlWriteOpenTag(request->writer, "ns1", "byteValue");
            gsXmlWriteIntElement(request->writer, "ns1", "value", fields[i].value.byteValue);
            gsXmlWriteCloseTag(request->writer, "ns1", "byteValue");
        } else if (fields[i].type == SAKEFieldType_SHORT) {
            gsXmlWriteOpenTag(request->writer, "ns1", "shortValue");
            gsXmlWriteIntElement(request->writer, "ns1", "value", fields[i].value.shortValue);
            gsXmlWriteCloseTag(request->writer, "ns1", "shortValue");
        } else if (fields[i].type == SAKEFieldType_INT) {
            gsXmlWriteOpenTag(request->writer, "ns1", "intValue");
            gsXmlWriteIntElement(request->writer, "ns1", "value", fields[i].value.intValue);
            gsXmlWriteCloseTag(request->writer, "ns1", "intValue");
        } else if (fields[i].type == SAKEFieldType_INT64) {
            gsXmlWriteOpenTag(request->writer, "ns1", "int64Value");
            gsXmlWriteInt64Element(request->writer, "ns1", "value", fields[i].value.int64Value);
            gsXmlWriteCloseTag(request->writer, "ns1", "int64Value");
        } else if (fields[i].type == SAKEFieldType_FLOAT) {
            gsXmlWriteOpenTag(request->writer, "ns1", "floatValue");
            gsXmlWriteFloatElement(request->writer, "ns1", "value", fields[i].value.floatValue);
            gsXmlWriteCloseTag(request->writer, "ns1", "floatValue");
        } else if (fields[i].type == SAKEFieldType_ASCII_STRING) {
            gsXmlWriteOpenTag(request->writer, "ns1", "asciiStringValue");
            gsXmlWriteStringElement(request->writer, "ns1", "value", fields[i].value.asciiStringValue);
            gsXmlWriteCloseTag(request->writer, "ns1", "asciiStringValue");
        } else if (fields[i].type == SAKEFieldType_UNICODE_STRING) {
            gsXmlWriteOpenTag(request->writer, "ns1", "unicodeStringValue");
            gsXmlWriteUnicodeStringElement(request->writer, "ns1", "value", fields[i].value.unicodeStringValue);
            gsXmlWriteCloseTag(request->writer, "ns1", "unicodeStringValue");
        } else if (fields[i].type == SAKEFieldType_BOOLEAN) {
            gsXmlWriteOpenTag(request->writer, "ns1", "booleanValue");
            gsXmlWriteIntElement(request->writer, "ns1", "value", fields[i].value.booleanValue != gsi_false);
            gsXmlWriteCloseTag(request->writer, "ns1", "booleanValue");
        } else if (fields[i].type == SAKEFieldType_DATE_AND_TIME) {
            gsXmlWriteOpenTag(request->writer, "ns1", "dateAndTimeValue");
            gsXmlWriteDateTimeElement(request->writer, "ns1", "value", fields[i].value.dateAndTimeValue);
            gsXmlWriteCloseTag(request->writer, "ns1", "dateAndTimeValue");
        } else if (fields[i].type == SAKEFieldType_BINARY_DATA) {
            gsXmlWriteOpenTag(request->writer, "ns1", "binaryDataValue");
            gsXmlWriteBase64BinaryElement(request->writer, "ns1", "value", fields[i].value.binaryDataValue.data,
                                          fields[i].value.binaryDataValue.length);
            gsXmlWriteCloseTag(request->writer, "ns1", "binaryDataValue");
        }
        gsXmlWriteCloseTag(request->writer, "ns1", "value");
        gsXmlWriteCloseTag(request->writer, "ns1", "RecordField");
    }
    gsXmlWriteCloseTag(request->writer, "ns1", "values");
}

static SAKEStartRequestResult Sake_ValidateCreateRecord(SAKERequest request) {
    SAKECreateRecordInput *input = request->input;

    if (input->tableId == NULL) {
        return SAKEStartRequestResult_BAD_TABLEID;
    }
    if (input->fieldNum < 0) {
        return SAKEStartRequestResult_BAD_NUM_FIELDS;
    }
    if (input->fields == NULL && input->fieldNum > 0) {
        return SAKEStartRequestResult_BAD_FIELDS;
    }
    return Sake_CheckFields(input->fields, input->fieldNum);
}

static SAKEStartRequestResult Sake_BuildCreateRecord(SAKERequest request) {
    SAKECreateRecordInput *input = request->input;

    gsXmlWriteStringElement(request->writer, "ns1", "tableid", input->tableId);
    Sake_WriteFields(request, input->fields, input->fieldNum);
    return 0;
}

static SAKERequestResult Sake_ParseCreateRecord(SAKERequest request) {
    if (gsXmlReadChildAsInt(request->response, "recordid", request->output) == gsi_false) {
        return SAKERequestResult_MALFORMED_RESPONSE;
    }
    return 0;
}

SAKEStartRequestResult Sake_SubmitCreateRecord(SAKERequest request) {
    return Sake_Submit(request, &sCreateRecordType);
}

static SAKEStartRequestResult Sake_ValidateUpdateRecord(SAKERequest request) {
    SAKEUpdateRecordInput *input = request->input;

    if (input->tableId == NULL) {
        return SAKEStartRequestResult_BAD_TABLEID;
    }
    if (input->fieldNum <= 0) {
        return SAKEStartRequestResult_BAD_NUM_FIELDS;
    }
    if (input->fields != NULL) {
        return Sake_CheckFields(input->fields, input->fieldNum);
    }
    return SAKEStartRequestResult_BAD_FIELDS;
}

static SAKEStartRequestResult Sake_BuildUpdateRecord(SAKERequest request) {
    SAKEUpdateRecordInput *input = request->input;

    gsXmlWriteStringElement(request->writer, "ns1", "tableid", input->tableId);
    gsXmlWriteIntElement(request->writer, "ns1", "recordid", input->recordId);
    Sake_WriteFields(request, input->fields, input->fieldNum);
    return 0;
}

SAKEStartRequestResult Sake_SubmitUpdateRecord(SAKERequest request) {
    return Sake_Submit(request, &sUpdateRecordType);
}

static SAKEStartRequestResult Sake_ValidateDeleteRecord(SAKERequest request) {
    SAKEDeleteRecordInput *input = request->input;

    if (input->tableId != NULL) {
        return 0;
    }
    return SAKEStartRequestResult_BAD_TABLEID;
}

static SAKEStartRequestResult Sake_BuildDeleteRecord(SAKERequest request) {
    SAKEDeleteRecordInput *input = request->input;

    gsXmlWriteStringElement(request->writer, "ns1", "tableid", input->tableId);
    gsXmlWriteIntElement(request->writer, "ns1", "recordid", input->recordId);
    return 0;
}
