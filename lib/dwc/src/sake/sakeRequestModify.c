#include "sake_internal.h"

// The requests that write a record to the storage server: creating, updating and deleting. A message lists the
// record's fields, each with its name and a value of the field's type. The three have a description of their kind that
// the request takes the elements and the check, write and read functions from. The file and its names are
// GameSpy's own, as its sakeRequestModify.c has them, but for the descriptions of the kinds, which are function-local
// statics named info there.

static SAKEStartRequestResult sakeiValidateRequestFields(SAKEField *fields, int fieldNum);
static void sakeiFillSoapRequestFieldValues(SAKERequest request, SAKEField *fields, int fieldNum);
static SAKEStartRequestResult sakeiCreateRecordValidateInput(SAKERequest request);
static SAKEStartRequestResult sakeiCreateRecordFillSoapRequest(SAKERequest request);
static SAKERequestResult sakeiCreateRecordProcessSoapResponse(SAKERequest request);
static SAKEStartRequestResult sakeiUpdateRecordValidateInput(SAKERequest request);
static SAKEStartRequestResult sakeiUpdateRecordFillSoapRequest(SAKERequest request);
static SAKEStartRequestResult sakeiDeleteRecordValidateInput(SAKERequest request);
static SAKEStartRequestResult sakeiDeleteRecordFillSoapRequest(SAKERequest request);

static SAKEIRequestInfo sCreateRecordType = {
    4,
    "CreateRecord",
    "SOAPAction: \"http://gamespy.net/sake/CreateRecord\"",
    "CreateRecordResponse",
    "CreateRecordResult",
    sakeiCreateRecordValidateInput,
    sakeiCreateRecordFillSoapRequest,
    sakeiCreateRecordProcessSoapResponse,
    NULL,
    NULL,
};

static SAKEIRequestInfo sUpdateRecordType = {
    0,
    "UpdateRecord",
    "SOAPAction: \"http://gamespy.net/sake/UpdateRecord\"",
    "UpdateRecordResponse",
    "UpdateRecordResult",
    sakeiUpdateRecordValidateInput,
    sakeiUpdateRecordFillSoapRequest,
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
    sakeiDeleteRecordValidateInput,
    sakeiDeleteRecordFillSoapRequest,
    NULL,
    NULL,
    NULL,
};

static SAKEStartRequestResult sakeiValidateRequestFields(SAKEField *fields, int fieldNum) {
    int i;

    for (i = 0; i < fieldNum; i++) {
        SAKEField *field = &fields[i];

        if (field->mName == NULL || field->mName[0] == '\0') {
            return SAKEStartRequestResult_BAD_FIELD_NAME;
        }
        if (field->mType >= SAKEFieldType_NUM_FIELD_TYPES) {
            return SAKEStartRequestResult_BAD_FIELD_TYPE;
        }
        if (field->mType == SAKEFieldType_ASCII_STRING && field->mValue.mAsciiString == NULL) {
            return SAKEStartRequestResult_BAD_FIELD_VALUE;
        }
        if (field->mType == SAKEFieldType_UNICODE_STRING && field->mValue.mUnicodeString == NULL) {
            return SAKEStartRequestResult_BAD_FIELD_VALUE;
        }
        if (field->mType == SAKEFieldType_BINARY_DATA) {
            int length = field->mValue.mBinaryData.mLength;

            if (length < 0) {
                return SAKEStartRequestResult_BAD_FIELD_VALUE;
            }
            if (field->mValue.mBinaryData.mValue == NULL && length > 0) {
                return SAKEStartRequestResult_BAD_FIELD_VALUE;
            }
        }
    }
    return 0;
}

static void sakeiFillSoapRequestFieldValues(SAKERequest request, SAKEField *fields, int fieldNum) {
    int i;

    gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "values");
    for (i = 0; i < fieldNum; i++) {
        gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "RecordField");
        gsXmlWriteStringElement(request->mSoapRequest, "ns1", "name", fields[i].mName);
        gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "value");
        if (fields[i].mType == SAKEFieldType_BYTE) {
            gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "byteValue");
            gsXmlWriteIntElement(request->mSoapRequest, "ns1", "value", fields[i].mValue.mByte);
            gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "byteValue");
        } else if (fields[i].mType == SAKEFieldType_SHORT) {
            gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "shortValue");
            gsXmlWriteIntElement(request->mSoapRequest, "ns1", "value", fields[i].mValue.mShort);
            gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "shortValue");
        } else if (fields[i].mType == SAKEFieldType_INT) {
            gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "intValue");
            gsXmlWriteIntElement(request->mSoapRequest, "ns1", "value", fields[i].mValue.mInt);
            gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "intValue");
        } else if (fields[i].mType == SAKEFieldType_INT64) {
            gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "int64Value");
            gsXmlWriteInt64Element(request->mSoapRequest, "ns1", "value", fields[i].mValue.mInt64);
            gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "int64Value");
        } else if (fields[i].mType == SAKEFieldType_FLOAT) {
            gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "floatValue");
            gsXmlWriteFloatElement(request->mSoapRequest, "ns1", "value", fields[i].mValue.mFloat);
            gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "floatValue");
        } else if (fields[i].mType == SAKEFieldType_ASCII_STRING) {
            gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "asciiStringValue");
            gsXmlWriteStringElement(request->mSoapRequest, "ns1", "value", fields[i].mValue.mAsciiString);
            gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "asciiStringValue");
        } else if (fields[i].mType == SAKEFieldType_UNICODE_STRING) {
            gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "unicodeStringValue");
            gsXmlWriteUnicodeStringElement(request->mSoapRequest, "ns1", "value", fields[i].mValue.mUnicodeString);
            gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "unicodeStringValue");
        } else if (fields[i].mType == SAKEFieldType_BOOLEAN) {
            gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "booleanValue");
            gsXmlWriteIntElement(request->mSoapRequest, "ns1", "value", fields[i].mValue.mBoolean != gsi_false);
            gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "booleanValue");
        } else if (fields[i].mType == SAKEFieldType_DATE_AND_TIME) {
            gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "dateAndTimeValue");
            gsXmlWriteDateTimeElement(request->mSoapRequest, "ns1", "value", fields[i].mValue.mDateAndTime);
            gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "dateAndTimeValue");
        } else if (fields[i].mType == SAKEFieldType_BINARY_DATA) {
            gsXmlWriteOpenTag(request->mSoapRequest, "ns1", "binaryDataValue");
            gsXmlWriteBase64BinaryElement(request->mSoapRequest, "ns1", "value", fields[i].mValue.mBinaryData.mValue,
                                          fields[i].mValue.mBinaryData.mLength);
            gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "binaryDataValue");
        }
        gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "value");
        gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "RecordField");
    }
    gsXmlWriteCloseTag(request->mSoapRequest, "ns1", "values");
}

static SAKEStartRequestResult sakeiCreateRecordValidateInput(SAKERequest request) {
    SAKECreateRecordInput *input = request->mInput;

    if (input->mTableId == NULL) {
        return SAKEStartRequestResult_BAD_TABLEID;
    }
    if (input->mNumFields < 0) {
        return SAKEStartRequestResult_BAD_NUM_FIELDS;
    }
    if (input->mFields == NULL && input->mNumFields > 0) {
        return SAKEStartRequestResult_BAD_FIELDS;
    }
    return sakeiValidateRequestFields(input->mFields, input->mNumFields);
}

static SAKEStartRequestResult sakeiCreateRecordFillSoapRequest(SAKERequest request) {
    SAKECreateRecordInput *input = request->mInput;

    gsXmlWriteStringElement(request->mSoapRequest, "ns1", "tableid", input->mTableId);
    sakeiFillSoapRequestFieldValues(request, input->mFields, input->mNumFields);
    return 0;
}

static SAKERequestResult sakeiCreateRecordProcessSoapResponse(SAKERequest request) {
    if (gsXmlReadChildAsInt(request->mSoapResponse, "recordid", request->mOutput) == gsi_false) {
        return SAKERequestResult_MALFORMED_RESPONSE;
    }
    return 0;
}

SAKEStartRequestResult sakeiStartCreateRecordRequest(SAKERequest request) {
    return sakeiStartRequest(request, &sCreateRecordType);
}

static SAKEStartRequestResult sakeiUpdateRecordValidateInput(SAKERequest request) {
    SAKEUpdateRecordInput *input = request->mInput;

    if (input->mTableId == NULL) {
        return SAKEStartRequestResult_BAD_TABLEID;
    }
    if (input->mNumFields <= 0) {
        return SAKEStartRequestResult_BAD_NUM_FIELDS;
    }
    if (input->mFields != NULL) {
        return sakeiValidateRequestFields(input->mFields, input->mNumFields);
    }
    return SAKEStartRequestResult_BAD_FIELDS;
}

static SAKEStartRequestResult sakeiUpdateRecordFillSoapRequest(SAKERequest request) {
    SAKEUpdateRecordInput *input = request->mInput;

    gsXmlWriteStringElement(request->mSoapRequest, "ns1", "tableid", input->mTableId);
    gsXmlWriteIntElement(request->mSoapRequest, "ns1", "recordid", input->mRecordId);
    sakeiFillSoapRequestFieldValues(request, input->mFields, input->mNumFields);
    return 0;
}

SAKEStartRequestResult sakeiStartUpdateRecordRequest(SAKERequest request) {
    return sakeiStartRequest(request, &sUpdateRecordType);
}

static SAKEStartRequestResult sakeiDeleteRecordValidateInput(SAKERequest request) {
    SAKEDeleteRecordInput *input = request->mInput;

    if (input->mTableId != NULL) {
        return 0;
    }
    return SAKEStartRequestResult_BAD_TABLEID;
}

static SAKEStartRequestResult sakeiDeleteRecordFillSoapRequest(SAKERequest request) {
    SAKEDeleteRecordInput *input = request->mInput;

    gsXmlWriteStringElement(request->mSoapRequest, "ns1", "tableid", input->mTableId);
    gsXmlWriteIntElement(request->mSoapRequest, "ns1", "recordid", input->mRecordId);
    return 0;
}
