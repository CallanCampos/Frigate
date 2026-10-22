#include "dwc/gsXML.h"
#include "dwc/ext.h"
#include "nitro/std.h"

// GameSpy's XML (gsXML.c). The ROM names neither the file nor its functions: the file, gsXmlWriterGetData and
// gsXmlWriterGetDataLength are GameSpy's, as pret's pokerevo Wii build has them (gsXML.s), and the other functions are
// the later SDK's (the debug texts name gsXmlCreateStreamWriter and gsXmlCreateStream)

// A part of the text of a message, which isn't ended
typedef struct GSIXmlString {
    const gsi_u8 *mData;
    int mLen;
} GSIXmlString;

// An element (or an attribute) that the reader found: its name and value, its index and the index of the element it
// is in (for an attribute, the element it belongs to), -1 for none
typedef struct GSIXmlElement {
    GSIXmlString mName;
    GSIXmlString mValue;
    int mIndex;
    int mParentIndex;
} GSIXmlElement;

// A writer: its text, the length and the size of the buffer, and whether the message was ended
typedef struct GSIXmlStreamWriter {
    char *mBuffer;
    int mLen;
    int mCapacity;
    gsi_bool mClosed;
} GSIXmlStreamWriter;

// A reader: the elements and the attributes that the text had, the element it is in, the child that was read last and
// the attribute that was read last, -1 for none
typedef struct GSIXmlStreamReader {
    DArray mElementArray;
    DArray mAttributeArray;
    int mElemReadIndex;
    int mValueReadIndex;
    int mAttribReadIndex;
} GSIXmlStreamReader;

static void gsiXmlUtilElementFree(void *element);
static void gsiXmlUtilAttributeFree(void *element);
static gsi_bool gsiXmlUtilSkipWhiteSpace(GSIXmlStreamReader *reader, const char *text, int length, int *pos);
static gsi_bool gsiXmlUtilParseElement(GSIXmlStreamReader *reader, char *text, int length, int *pos, int parent);
static gsi_bool gsiXmlUtilParseName(GSIXmlStreamReader *reader, const char *text, int length, int *pos,
                                    GSIXmlString *out);
static gsi_bool gsiXmlUtilParseString(GSIXmlStreamReader *reader, const char *text, int length, int *pos,
                                      GSIXmlString *out);
static gsi_bool gsiXmlUtilDecodeString(char *text, int *length);
static gsi_bool gsiXmlUtilParseValue(GSIXmlStreamReader *reader, char *text, int length, int *pos, GSIXmlString *out);
static gsi_bool gsiXmlUtilWriteChar(GSIXmlStreamWriter *writer, int character);
static gsi_bool gsiXmlUtilWriteString(GSIXmlStreamWriter *writer, const char *text);
static gsi_bool gsiXmlUtilWriteUnicodeString(GSIXmlStreamWriter *writer, const gsi_u16 *text);
static gsi_bool gsiXmlUtilWriteXmlSafeString(GSIXmlStreamWriter *writer, const char *text);
static gsi_bool gsiXmlUtilGrowBuffer(GSIXmlStreamWriter *writer);
static gsi_bool gsiXmlUtilTagMatches(const char *name, const GSIXmlElement *element);

// The namespaces that every message declares
const char *GS_XML_SOAP_DEFAULT_NAMESPACES[4] = {
    "SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\"",
    "SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\"",
    "xsi=\"http://www.w3.org/2001/XMLSchema-instance\"",
    "xsd=\"http://www.w3.org/2001/XMLSchema\"",
};

// A wide-character string's length
static int gsUnicodeStringLen(const gsi_u16 *str) {
    const gsi_u16 *p = str;

    while (*p++ != 0) {
    }
    return p - str - 1;
}

GSXmlStreamWriter gsXmlCreateStreamWriter(const char **namespaceDeclarations, int declarationNum) {
    GSIXmlStreamWriter *writer;
    int size;
    int capacity;
    int i;

    capacity = 0x400;
    size = 0;
    writer = gsimalloc(sizeof(GSIXmlStreamWriter));
    if (writer == NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "Out of memory in gsXmlCreateStreamWriter, needed %d bytes", sizeof(GSIXmlStreamWriter));
        return NULL;
    }
    for (i = 0; i < 4; i++) {
        size += strlen("xmlns:") + 1;
        size += strlen(GS_XML_SOAP_DEFAULT_NAMESPACES[i]);
    }
    for (i = 0; i < declarationNum; i++) {
        size += strlen("xmlns:") + 1;
        size += strlen(namespaceDeclarations[i]);
    }
    if (size > 0x400) {
        do {
            capacity += 0x400;
        } while (capacity < size);
    }
    writer->mBuffer = gsimalloc(capacity);
    if (writer->mBuffer == NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "Out of memory in gsXmlCreateStreamWriter, needed %d bytes", capacity);
        return NULL;
    }
    writer->mCapacity = capacity;
    writer->mLen = 0;
    writer->mBuffer[0] = 0;
    writer->mClosed = gsi_false;
    if (!gsiXmlUtilWriteString(writer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><SOAP-ENV:Envelope")) {
        gsifree(writer->mBuffer);
        gsifree(writer);
        return NULL;
    }
    for (i = 0; i < 4; i++) {
        if (!gsiXmlUtilWriteChar(writer, ' ') || !gsiXmlUtilWriteString(writer, "xmlns:") ||
            !gsiXmlUtilWriteString(writer, GS_XML_SOAP_DEFAULT_NAMESPACES[i])) {
            gsifree(writer->mBuffer);
            gsifree(writer);
            return NULL;
        }
    }
    for (i = 0; i < declarationNum; i++) {
        if (!gsiXmlUtilWriteChar(writer, ' ') || !gsiXmlUtilWriteString(writer, "xmlns:") ||
            !gsiXmlUtilWriteString(writer, namespaceDeclarations[i])) {
            gsifree(writer->mBuffer);
            gsifree(writer);
            return NULL;
        }
    }
    if (!gsiXmlUtilWriteChar(writer, '>') || !gsiXmlUtilWriteString(writer, "<SOAP-ENV:Body>")) {
        gsifree(writer->mBuffer);
        gsifree(writer);
        return NULL;
    }
    return writer;
}

GSXmlStreamReader gsXmlCreateStreamReader(void) {
    GSIXmlStreamReader *reader;

    reader = gsimalloc(sizeof(GSIXmlStreamReader));
    if (reader == NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "Out of memory in gsXmlCreateStream, needed %d bytes", sizeof(GSIXmlStreamReader));
        return NULL;
    }
    reader->mElementArray = ArrayNew(sizeof(GSIXmlElement), 32, gsiXmlUtilElementFree);
    if (reader->mElementArray == NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "Out of memory in gsXmlCreateStream with mElementArray=ArrayNew()");
        gsifree(reader);
        return NULL;
    }
    reader->mAttributeArray = ArrayNew(sizeof(GSIXmlElement), 16, gsiXmlUtilAttributeFree);
    if (reader->mAttributeArray == NULL) {
        gsDebugFormat(GSIDebugCat_Common, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "Out of memory in gsXmlCreateStream with mElementArray=ArrayNew()");
        ArrayFree(reader->mElementArray);
        gsifree(reader);
        return NULL;
    }
    gsXmlMoveToStart(reader);
    return reader;
}

gsi_bool gsXmlParseBuffer(GSXmlStreamReader stream, char *text, int length) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    int pos;

    pos = 0;
    gsXmlResetReader(reader);
    while (pos < length) {
        if (!gsiXmlUtilParseElement(reader, text, length, &pos, -1)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

void gsXmlFreeWriter(GSXmlStreamWriter stream) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;

    gsifree(writer->mBuffer);
    gsifree(writer);
}

void gsXmlFreeReader(GSXmlStreamReader stream) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;

    ArrayFree(reader->mAttributeArray);
    ArrayFree(reader->mElementArray);
    gsifree(reader);
}

void gsXmlResetReader(GSXmlStreamReader stream) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;

    ArrayClear(reader->mAttributeArray);
    ArrayClear(reader->mElementArray);
    gsXmlMoveToStart(reader);
}

gsi_bool gsXmlCloseWriter(GSXmlStreamWriter stream) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;

    if (!gsiXmlUtilWriteString(writer, "</SOAP-ENV:Body></SOAP-ENV:Envelope>")) {
        return gsi_false;
    }
    writer->mClosed = gsi_true;
    return gsi_true;
}

static void gsiXmlUtilElementFree(void *element) {
}

static void gsiXmlUtilAttributeFree(void *element) {
}

const char *gsXmlWriterGetData(GSXmlStreamWriter stream) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;

    return writer->mBuffer;
}

int gsXmlWriterGetDataLength(GSXmlStreamWriter stream) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;

    return writer->mLen;
}

static gsi_bool gsiXmlUtilSkipWhiteSpace(GSIXmlStreamReader *reader, const char *text, int length, int *pos) {
    while (*pos < length) {
        if (strchr(" \t\r\n", text[*pos]) == NULL) {
            return gsi_true;
        }
        (*pos)++;
    }
    return gsi_false;
}

static gsi_bool gsiXmlUtilParseElement(GSIXmlStreamReader *reader, char *text, int length, int *pos, int parent) {
    int tagStart;
    gsi_bool store;
    int nameLength;
    int childStart;

    store = gsi_true;
    gsiXmlUtilSkipWhiteSpace(reader, text, length, pos);
    if (*pos >= length) {
        return gsi_true;
    }
    GSIXmlElement element = { { NULL, 0 }, { NULL, 0 }, 0, 0 };
    if (text[*pos] != '<') {
        return gsi_false;
    }
    (*pos)++;
    if (*pos >= length) {
        return gsi_false;
    }
    if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
        return gsi_false;
    }
    if (text[*pos] == '!' || text[*pos] == '?' || text[*pos] == '%' || text[*pos] == '/') {
        store = gsi_false;
        (*pos)++;
        tagStart = *pos - 2;
    } else {
        tagStart = *pos - 1;
    }
    if (!gsiXmlUtilParseName(reader, text, length, pos, &element.mName)) {
        return gsi_false;
    }
    if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
        return gsi_false;
    }
    if (store) {
        element.mIndex = ArrayLength(reader->mElementArray);
        element.mParentIndex = parent;
        ArrayAppend(reader->mElementArray, &element);
    }
    while (*pos < length && isalnum(text[*pos])) {
        GSIXmlElement attribute = { { NULL, 0 }, { NULL, 0 }, 0, 0 };

        if (!gsiXmlUtilParseName(reader, text, length, pos, &attribute.mName)) {
            return gsi_false;
        }
        if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
            return gsi_false;
        }
        if (text[*pos] != '=') {
            return gsi_false;
        }
        (*pos)++;
        if (!gsiXmlUtilParseString(reader, text, length, pos, &attribute.mValue)) {
            return gsi_false;
        }
        if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
            return gsi_false;
        }
        if (store) {
            attribute.mIndex = ArrayLength(reader->mAttributeArray);
            attribute.mParentIndex = element.mIndex;
            ArrayAppend(reader->mAttributeArray, &attribute);
        }
    }
    if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
        return gsi_false;
    }
    if ((!isalnum(text[tagStart + 1]) && text[*pos] == text[tagStart + 1]) || text[*pos] == '/') {
        (*pos)++;
        if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
            return gsi_false;
        }
        if (text[*pos] != '>') {
            return gsi_false;
        }
        (*pos)++;
        return gsi_true;
    }
    if (text[*pos] != '>') {
        return gsi_false;
    }
    (*pos)++;
    if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
        return gsi_false;
    }
    if (text[*pos] != '<') {
        if (!gsiXmlUtilParseValue(reader, text, length, pos, &element.mValue)) {
            return gsi_false;
        }
        if (store) {
            ArrayReplaceAt(reader->mElementArray, &element, element.mIndex);
        }
        if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
            return gsi_false;
        }
    }
    for (childStart = *pos; childStart < length; childStart = *pos) {
        if (text[childStart] != '<') {
            return gsi_false;
        }
        (*pos)++;
        if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
            return gsi_false;
        }
        if (text[*pos] == '/') {
            (*pos)++;
            if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
                return gsi_false;
            }
            nameLength = element.mName.mLen;
            if (*pos + nameLength >= length) {
                return gsi_false;
            }
            if (strncmp(element.mName.mData, text + *pos, nameLength) != 0) {
                return gsi_false;
            }
            *pos += nameLength;
            if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
                return gsi_false;
            }
            if (text[*pos] != '>') {
                return gsi_false;
            }
            (*pos)++;
            return gsi_true;
        }
        if (element.mValue.mData != NULL) {
            return gsi_false;
        }
        *pos = childStart;
        if (!gsiXmlUtilParseElement(reader, text, length, pos, element.mIndex)) {
            return gsi_false;
        }
        gsiXmlUtilSkipWhiteSpace(reader, text, length, pos);
    }
    return gsi_false;
}

static gsi_bool gsiXmlUtilParseName(GSIXmlStreamReader *reader, const char *text, int length, int *pos,
                                    GSIXmlString *out) {
    gsi_bool colon;
    char *start;
    int c;

    colon = gsi_false;
    if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
        return gsi_false;
    }
    if (*pos >= length) {
        return gsi_false;
    }
    start = (char *)text + *pos;
    c = text[*pos];
    if (!isalnum(c) || isdigit(c)) {
        return gsi_false;
    }
    out->mData = start;
    out->mLen = 1;
    (*pos)++;
    while (*pos < length && strchr(" \t\r\n", c = text[*pos]) == NULL) {
        if (c == ':') {
            if (colon) {
                return gsi_false;
            }
            colon = gsi_true;
        } else if (c != '_' && c != '-' && !isalnum(c)) {
            return gsi_true;
        }
        out->mLen++;
        (*pos)++;
    }
    return gsi_true;
}

static gsi_bool gsiXmlUtilParseString(GSIXmlStreamReader *reader, const char *text, int length, int *pos,
                                      GSIXmlString *out) {
    char *start;
    int quote;

    if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
        return gsi_false;
    }
    quote = text[*pos];
    if (quote != '"' && quote != '\'') {
        return gsi_false;
    }
    (*pos)++;
    start = (char *)text + *pos;
    out->mLen = 0;
    if (*pos >= length) {
        return gsi_false;
    }
    if (quote == text[*pos]) {
        out->mData = start;
        (*pos)++;
        return gsi_true;
    }
    while (quote != text[*pos]) {
        if (*pos >= length) {
            return gsi_false;
        }
        (*pos)++;
        out->mLen++;
    }
    (*pos)++;
    if (!gsiXmlUtilDecodeString(start, &out->mLen)) {
        return gsi_false;
    }
    out->mData = start;
    return gsi_true;
}

// Replaces the character references of a text (such as &amp; and &#x41;) by the characters, in place
static gsi_bool gsiXmlUtilDecodeString(char *text, int *length) {
    int i;
    int removed;
    int j;

    i = 0;
    while (i < *length) {
        if (text[i] != '&') {
            i++;
            continue;
        }
        removed = 5;
        if (strncmp(&text[i], "&amp;", 5) == 0) {
            text[i++] = '&';
            removed = 4;
        } else if (strncmp(&text[i], "&quot;", 6) == 0) {
            text[i++] = '"';
        } else if (strncmp(&text[i], "&apos;", 6) == 0) {
            text[i++] = '\'';
        } else if (strncmp(&text[i], "&lt;", 4) == 0) {
            text[i++] = '<';
            removed = 3;
        } else if (strncmp(&text[i], "&gt;", 4) == 0) {
            text[i++] = '>';
            removed = 3;
        } else if (strncmp(&text[i], "&#x", 3) == 0) {
            gsi_bool started;
            int value;
            gsi_u32 mask;
            char *end;

            started = gsi_false;
            value = 0;
            mask = 0xff000000;
            end = strchr(&text[i + 3], ';');
            if (end == NULL) {
                return gsi_false;
            }
            if (end - &text[i + 3] > 8) {
                return gsi_false;
            }
            STD_TSScanf(&text[i + 3], "%08x", &value);
            for (j = 0; j < 4; j++) {
                char byte = ((gsi_u32)value & mask) >> ((3 - j) * 8);
                if (started || byte != 0) {
                    text[i++] = byte;
                    started = gsi_true;
                }
                mask >>= 8;
            }
            removed = end - &text[i] + 1;
        } else if (strncmp(&text[i], "&#", 2) == 0) {
            gsi_bool started;
            int value;
            gsi_u32 mask;
            char *end;

            started = gsi_false;
            mask = 0xff000000;
            end = strchr(&text[i + 2], ';');
            if (end == NULL) {
                return gsi_false;
            }
            value = atol(&text[i + 2]);
            for (j = 0; j < 4; j++) {
                char byte = ((gsi_u32)value & mask) >> ((3 - j) * 8);
                if (started || byte != 0) {
                    text[i++] = byte;
                    started = gsi_true;
                }
                mask >>= 8;
            }
            removed = end - &text[i] + 1;
        } else {
            return gsi_false;
        }
        memmove(&text[i], &text[i + removed], *length - (i + removed));
        memset(&text[*length - removed], ' ', removed);
        *length -= removed;
    }
    return gsi_true;
}

static gsi_bool gsiXmlUtilParseValue(GSIXmlStreamReader *reader, char *text, int length, int *pos, GSIXmlString *out) {
    char *start;

    start = NULL;
    if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
        return gsi_false;
    }
    if (text[*pos] != '<') {
        start = text + *pos;
        out->mData = start;
    }
    while (*pos < length) {
        if (text[*pos] == '<') {
            return gsiXmlUtilDecodeString(start, &out->mLen) != 0;
        }
        (*pos)++;
        out->mLen++;
    }
    return gsi_false;
}

gsi_bool gsXmlWriteOpenTag(GSXmlStreamWriter stream, const char *prefix, const char *name) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;

    if (!gsiXmlUtilWriteChar(writer, '<') || !gsiXmlUtilWriteString(writer, prefix) ||
        !gsiXmlUtilWriteChar(writer, ':') || !gsiXmlUtilWriteString(writer, name) ||
        !gsiXmlUtilWriteChar(writer, '>')) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteCloseTag(GSXmlStreamWriter stream, const char *prefix, const char *name) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;

    if (!gsiXmlUtilWriteChar(writer, '<') || !gsiXmlUtilWriteChar(writer, '/') ||
        !gsiXmlUtilWriteString(writer, prefix) || !gsiXmlUtilWriteChar(writer, ':') ||
        !gsiXmlUtilWriteString(writer, name) || !gsiXmlUtilWriteChar(writer, '>')) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteStringElement(GSXmlStreamWriter stream, const char *prefix, const char *name, const char *value) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;
    int length;
    int i;

    length = strlen(value);
    for (i = 0; i < length; i++) {
        gsi_u8 c = value[i];
        if (c < 0x20 && c != '\t' && c != '\n' && c != '\r') {
            return gsi_false;
        }
    }
    if (!gsXmlWriteOpenTag(writer, prefix, name) || !gsiXmlUtilWriteXmlSafeString(writer, value) ||
        !gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteAsciiStringElement(GSXmlStreamWriter stream, const char *prefix, const char *name,
                                      const gsi_char *value) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;
    int length;
    int i;

    length = strlen(value);
    for (i = 0; i < length; i++) {
        gsi_u8 c = value[i];
        if (c < 0x20 && c != '\t' && c != '\n' && c != '\r') {
            return gsi_false;
        }
    }
    if (!gsXmlWriteOpenTag(writer, prefix, name) || !gsiXmlUtilWriteXmlSafeString(writer, value) ||
        !gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteUnicodeStringElement(GSXmlStreamWriter stream, const char *prefix, const char *name,
                                        const unsigned short *value) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;
    int i;
    int length;

    length = gsUnicodeStringLen(value);
    for (i = 0; i < length; i++) {
        gsi_u16 c = value[i];
        if (c < 0x20) {
            if (c != '\t' && c != '\n' && c != '\r') {
                return gsi_false;
            }
        } else if (c > 0xd7ff && c < 0xe000) {
            return gsi_false;
        } else if (c > 0xfffd) {
            return gsi_false;
        }
    }
    if (!gsXmlWriteOpenTag(writer, prefix, name) || !gsiXmlUtilWriteUnicodeString(writer, value) ||
        !gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteIntElement(GSXmlStreamWriter stream, const char *prefix, const char *name, gsi_u32 value) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;
    char text[32];

    sprintf(text, "%d", value);
    if (!gsXmlWriteOpenTag(writer, prefix, name) || !gsiXmlUtilWriteString(writer, text) ||
        !gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteInt64Element(GSXmlStreamWriter stream, const char *prefix, const char *name, gsi_i64 value) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;
    char text[36];

    gsiInt64ToString(text, value);
    if (!gsXmlWriteOpenTag(writer, prefix, name) || !gsiXmlUtilWriteString(writer, text) ||
        !gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteFloatElement(GSXmlStreamWriter stream, const char *prefix, const char *name, float value) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;
    char text[32];

    sprintf(text, "%f", (double)value);
    if (!gsXmlWriteOpenTag(writer, prefix, name) || !gsiXmlUtilWriteString(writer, text) ||
        !gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteHexBinaryElement(GSXmlStreamWriter stream, const char *prefix, const char *name, const gsi_u8 *data,
                                    int size) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;
    char hex[3];
    int i;

    i = 0;
    hex[2] = 0;
    if (!gsXmlWriteOpenTag(writer, prefix, name)) {
        return gsi_false;
    }
    while (i < size) {
        sprintf(hex, "%02x", ((const gsi_u8 *)data)[i]);
        i++;
        if (!gsiXmlUtilWriteChar(writer, hex[0])) {
            return gsi_false;
        }
        if (!gsiXmlUtilWriteChar(writer, hex[1])) {
            return gsi_false;
        }
    }
    if (!gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteBase64BinaryElement(GSXmlStreamWriter stream, const char *prefix, const char *name,
                                       const gsi_u8 *data, int size) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;
    B64StreamData coder;
    char chunk[5];

    if (!gsXmlWriteOpenTag(writer, prefix, name)) {
        return gsi_false;
    }
    B64InitEncodeStream(&coder, data, size, 0);
    while (B64EncodeStream(&coder, chunk)) {
        chunk[4] = 0;
        if (!gsiXmlUtilWriteString(writer, chunk)) {
            return gsi_false;
        }
    }
    if (!gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteDateTimeElement(GSXmlStreamWriter stream, const char *prefix, const char *name, time_t dateAndTime) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;
    char text[24];
    struct tm *time;

    time = gsiSecondsToDate(&dateAndTime);
    sprintf(text, "%d-%02d-%02dT%02d:%02d:%02dZ", time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour,
            time->tm_min, time->tm_sec);
    if (!gsXmlWriteOpenTag(writer, prefix, name) || !gsiXmlUtilWriteString(writer, text) ||
        !gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteLargeIntElement(GSXmlStreamWriter stream, const char *prefix, const char *name,
                                   const gsLargeInt_t *value) {
    GSIXmlStreamWriter *writer = (GSIXmlStreamWriter *)stream;
    const gsi_u32 *words;
    int i;
    gsi_u32 j;
    gsi_bool leading;
    char hex[4];

    i = value->mLength - 1;
    words = &value->mData[0];
    leading = gsi_true;
    if (!gsXmlWriteOpenTag(writer, prefix, name)) {
        return gsi_false;
    }
    while (i >= 0 && words[i] == 0) {
        i--;
    }
    for (; i >= 0; i--) {
        if (leading) {
            for (j = 0; j < 4; j++) {
                if ((gsi_u8)(value->mData[i] >> ((3 - j) * 8)) != 0) {
                    break;
                }
            }
            leading = gsi_false;
        } else {
            j = 0;
        }
        for (; j < 4; j++) {
            sprintf(hex, "%02x", (gsi_u8)(value->mData[i] >> ((3 - j) * 8)));
            if (!gsiXmlUtilWriteChar(writer, hex[0])) {
                return gsi_false;
            }
            if (!gsiXmlUtilWriteChar(writer, hex[1])) {
                return gsi_false;
            }
        }
    }
    if (!gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

static gsi_bool gsiXmlUtilWriteChar(GSIXmlStreamWriter *writer, int character) {
    if (writer->mLen >= writer->mCapacity) {
        if (!gsiXmlUtilGrowBuffer(writer)) {
            return gsi_false;
        }
    }
    writer->mBuffer[writer->mLen] = character;
    writer->mLen++;
    return gsi_true;
}

static gsi_bool gsiXmlUtilWriteString(GSIXmlStreamWriter *writer, const char *text) {
    int length;

    length = strlen(text);
    if (length == 0) {
        return gsi_true;
    }
    while (writer->mCapacity - writer->mLen <= length) {
        if (!gsiXmlUtilGrowBuffer(writer)) {
            return gsi_false;
        }
    }
    strcpy(writer->mBuffer + writer->mLen, text);
    writer->mLen += length;
    return gsi_true;
}

static gsi_bool gsiXmlUtilWriteUnicodeString(GSIXmlStreamWriter *writer, const gsi_u16 *text) {
    char utf8[4] = { 0, 0, 0, 0 };
    int length;
    int i;

    length = gsUnicodeStringLen(text);
    utf8[3] = 0;
    for (i = 0; i < length; i++) {
        utf8[_UCS2CharToUTF8String(text[i], utf8)] = 0;
        if (!gsiXmlUtilWriteXmlSafeString(writer, utf8)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

static gsi_bool gsiXmlUtilWriteXmlSafeString(GSIXmlStreamWriter *writer, const char *text) {
    char reference[8];
    int length;
    int i;
    gsi_bool ok;

    length = strlen(text);
    for (i = 0; i < length; i++) {
        char c = text[i];
        if (c == '&') {
            ok = gsiXmlUtilWriteString(writer, "&amp;");
        } else if (c == '\'') {
            ok = gsiXmlUtilWriteString(writer, "&apos;");
        } else if (c == '"') {
            ok = gsiXmlUtilWriteString(writer, "&quot;");
        } else if (c == '<') {
            ok = gsiXmlUtilWriteString(writer, "&lt;");
        } else if (c == '>') {
            ok = gsiXmlUtilWriteString(writer, "&gt;");
        } else if (c == ' ') {
            ok = gsiXmlUtilWriteString(writer, "&#x20;");
        } else if (c < ' ' || (gsi_u8)c > 0x7f) {
            sprintf(reference, "&#x%02x;", (gsi_u8)c);
            reference[6] = 0;
            ok = gsiXmlUtilWriteString(writer, reference);
        } else {
            ok = gsiXmlUtilWriteChar(writer, c);
        }
        if (!ok) {
            return gsi_false;
        }
    }
    return gsi_true;
}

static gsi_bool gsiXmlUtilGrowBuffer(GSIXmlStreamWriter *writer) {
    int newCapacity;
    char *newBuffer;

    newCapacity = writer->mCapacity + 0x400;
    newBuffer = gsirealloc(writer->mBuffer, newCapacity);
    if (newBuffer == NULL) {
        return gsi_false;
    }
    if (newBuffer != writer->mBuffer) {
        writer->mBuffer = newBuffer;
    }
    writer->mCapacity = newCapacity;
    return gsi_true;
}

gsi_bool gsXmlMoveToStart(GSXmlStreamReader stream) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;

    reader->mElemReadIndex = -1;
    reader->mValueReadIndex = -1;
    reader->mAttribReadIndex = -1;
    return gsi_true;
}

gsi_bool gsXmlMoveToNext(GSXmlStreamReader stream, const char *name) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    int i;

    for (i = reader->mElemReadIndex + 1; i < ArrayLength(reader->mElementArray); i++) {
        if (gsiXmlUtilTagMatches(name, ArrayNth(reader->mElementArray, i))) {
            reader->mElemReadIndex = i;
            reader->mValueReadIndex = -1;
            reader->mAttribReadIndex = -1;
            return gsi_true;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlMoveToParent(GSXmlStreamReader stream) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    int element;
    int parent;

    element = reader->mElemReadIndex;
    if (element >= ArrayLength(reader->mElementArray) || element == -1) {
        return gsi_false;
    }
    parent = ((GSIXmlElement *)ArrayNth(reader->mElementArray, element))->mParentIndex;
    if (parent == -1) {
        return gsi_false;
    }
    if (parent >= ArrayLength(reader->mElementArray)) {
        return gsi_false;
    }
    reader->mElemReadIndex = parent;
    reader->mValueReadIndex = -1;
    reader->mAttribReadIndex = -1;
    return gsi_true;
}

gsi_bool gsXmlMoveToChild(GSXmlStreamReader stream, const char *name) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    GSIXmlElement *element;
    int i;

    for (i = reader->mElemReadIndex + 1; i < ArrayLength(reader->mElementArray); i++) {
        element = ArrayNth(reader->mElementArray, i);
        if (element->mParentIndex == reader->mElemReadIndex && gsiXmlUtilTagMatches(name, element)) {
            reader->mElemReadIndex = i;
            reader->mValueReadIndex = -1;
            reader->mAttribReadIndex = -1;
            return gsi_true;
        }
        if (element->mParentIndex < reader->mElemReadIndex) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsString(GSXmlStreamReader stream, const char *name, const char **text, int *length) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    GSIXmlElement *element;
    int i;

    if (reader->mValueReadIndex == -1) {
        reader->mValueReadIndex = reader->mElemReadIndex;
    }
    for (i = reader->mValueReadIndex + 1; i < ArrayLength(reader->mElementArray); i++) {
        element = ArrayNth(reader->mElementArray, i);
        if (element->mParentIndex == reader->mElemReadIndex && gsiXmlUtilTagMatches(name, element)) {
            reader->mValueReadIndex = i;
            *text = element->mValue.mData;
            *length = element->mValue.mLen;
            return gsi_true;
        }
        if (element->mParentIndex < reader->mElemReadIndex) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsStringNT(GSXmlStreamReader stream, const char *name, char buffer[], int size) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    const char *text;
    int length;
    int end;

    text = NULL;
    length = 0;
    if (!gsXmlReadChildAsString(reader, name, &text, &length)) {
        buffer[0] = 0;
        return gsi_false;
    }
    strncpy(buffer, text, size < length ? size : length);
    end = size - 1 < length ? size - 1 : length;
    buffer[end] = 0;
    return gsi_true;
}

gsi_bool gsXmlReadChildAsHexBinary(GSXmlStreamReader stream, const char *name, gsi_u8 buffer[], int size,
                                   int *readSize) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    GSIXmlElement *element;
    int i;
    int hexLength;
    int written;
    int offset;
    int value;
    int textLength;

    if (reader->mValueReadIndex == -1) {
        reader->mValueReadIndex = reader->mElemReadIndex;
    }
    for (i = reader->mValueReadIndex + 1; i < ArrayLength(reader->mElementArray); i++) {
        element = ArrayNth(reader->mElementArray, i);
        if (element->mParentIndex == reader->mElemReadIndex && gsiXmlUtilTagMatches(name, element)) {
            written = 0;
            value = 0;
            offset = 0;
            hexLength = size * 2;
            textLength = element->mValue.mLen;
            if (hexLength >= textLength) {
                hexLength = textLength;
            }
            if (textLength == 0 || element->mValue.mData == NULL) {
                *(gsi_u8 *)buffer = 0;
                *readSize = 0;
                return gsi_true;
            }
            if (buffer == NULL) {
                *readSize = textLength / 2;
                return gsi_true;
            }
            while (hexLength > 1) {
                STD_TSScanf(element->mValue.mData + offset, "%02x", &value);
                ((gsi_u8 *)buffer)[written++] = value;
                offset += 2;
                hexLength -= 2;
            }
            if (hexLength == 1) {
                STD_TSScanf(element->mValue.mData + offset, "%01x", &value);
                ((gsi_u8 *)buffer)[written++] = value;
            }
            if (readSize != NULL) {
                *readSize = written;
            }
            reader->mValueReadIndex = i;
            return gsi_true;
        }
        if (element->mParentIndex < reader->mElemReadIndex) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsBase64Binary(GSXmlStreamReader stream, const char *name, gsi_u8 buffer[], int *size) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    GSIXmlElement *element;
    int i;

    if (reader->mValueReadIndex == -1) {
        reader->mValueReadIndex = reader->mElemReadIndex;
    }
    for (i = reader->mValueReadIndex + 1; i < ArrayLength(reader->mElementArray); i++) {
        element = ArrayNth(reader->mElementArray, i);
        if (element->mParentIndex == reader->mElemReadIndex && gsiXmlUtilTagMatches(name, element)) {
            if (buffer != NULL) {
                reader->mValueReadIndex = i;
                if (element->mValue.mData != NULL) {
                    B64Decode(element->mValue.mData, buffer, element->mValue.mLen, size, 0);
                } else {
                    *size = 0;
                }
            } else if (element->mValue.mData != NULL) {
                *size = B64DecodeLen(element->mValue.mData, 0);
            } else {
                *size = 0;
            }
            return gsi_true;
        }
        if (element->mParentIndex < reader->mElemReadIndex) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsInt(GSXmlStreamReader stream, const char *name, int *value) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    GSIXmlElement *element;
    int i;

    if (reader->mValueReadIndex == -1) {
        reader->mValueReadIndex = reader->mElemReadIndex;
    }
    for (i = reader->mValueReadIndex + 1; i < ArrayLength(reader->mElementArray); i++) {
        element = ArrayNth(reader->mElementArray, i);
        if (element->mParentIndex == reader->mElemReadIndex && gsiXmlUtilTagMatches(name, element)) {
            reader->mValueReadIndex = i;
            if (element->mValue.mData == NULL) {
                return gsi_false;
            }
            *value = atol(element->mValue.mData);
            return gsi_true;
        }
        if (element->mParentIndex < reader->mElemReadIndex) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsInt64(GSXmlStreamReader stream, const char *name, gsi_i64 *value) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    GSIXmlElement *element;
    int i;

    if (reader->mValueReadIndex == -1) {
        reader->mValueReadIndex = reader->mElemReadIndex;
    }
    for (i = reader->mValueReadIndex + 1; i < ArrayLength(reader->mElementArray); i++) {
        element = ArrayNth(reader->mElementArray, i);
        if (element->mParentIndex == reader->mElemReadIndex && gsiXmlUtilTagMatches(name, element)) {
            reader->mValueReadIndex = i;
            if (element->mValue.mData == NULL) {
                return gsi_false;
            }
            *value = gsiStringToInt64(element->mValue.mData);
            return gsi_true;
        }
        if (element->mParentIndex < reader->mElemReadIndex) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsDateTimeElement(GSXmlStreamReader stream, const char *name, time_t *value) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    GSIXmlElement *element;
    struct tm time;
    int i;

    if (reader->mValueReadIndex == -1) {
        reader->mValueReadIndex = reader->mElemReadIndex;
    }
    for (i = reader->mValueReadIndex + 1; i < ArrayLength(reader->mElementArray); i++) {
        element = ArrayNth(reader->mElementArray, i);
        if (element->mParentIndex == reader->mElemReadIndex && gsiXmlUtilTagMatches(name, element)) {
            reader->mValueReadIndex = i;
            if (element->mValue.mData == NULL) {
                return gsi_false;
            }
            STD_TSScanf(element->mValue.mData, "%i-%02d-%02dT%02d:%02d:%02d", &time.tm_year, &time.tm_mon,
                        &time.tm_mday, &time.tm_hour, &time.tm_min, &time.tm_sec);
            time.tm_year -= 1900;
            time.tm_mon -= 1;
            time.tm_isdst = -1;
            *value = gsiDateToSeconds(&time);
            return gsi_true;
        }
        if (element->mParentIndex < reader->mElemReadIndex) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsFloat(GSXmlStreamReader stream, const char *name, float *value) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    GSIXmlElement *element;
    int i;

    if (reader->mValueReadIndex == -1) {
        reader->mValueReadIndex = reader->mElemReadIndex;
    }
    for (i = reader->mValueReadIndex + 1; i < ArrayLength(reader->mElementArray); i++) {
        element = ArrayNth(reader->mElementArray, i);
        if (element->mParentIndex == reader->mElemReadIndex && gsiXmlUtilTagMatches(name, element)) {
            reader->mValueReadIndex = i;
            if (element->mValue.mData == NULL) {
                return gsi_false;
            }
            *value = atof(element->mValue.mData);
            return gsi_true;
        }
        if (element->mParentIndex < reader->mElemReadIndex) {
            return gsi_false;
        }
    }
    return gsi_false;
}

// Whether the name (with the namespace prefix that its last colon ends, if it has one) is the one of the element
static gsi_bool gsiXmlUtilTagMatches(const char *name, const GSIXmlElement *element) {
    int i;
    int nameLength;
    int compareLength;
    const char *part;
    const char *elementName;

    i = 0;
    if (name == NULL) {
        return gsi_true;
    }
    if ((name + strlen(name))[i - 1] == ':') {
        return gsi_false;
    }
    part = strchr(name, ':');
    if (part == NULL) {
        part = name;
    }
    nameLength = element->mName.mLen;
    while (i < nameLength && (gsi_u8)element->mName.mData[i] != ':') {
        i++;
    }
    if (i == nameLength) {
        i = 0;
    } else {
        i++;
    }
    elementName = element->mName.mData;
    compareLength = nameLength - i;
    if (compareLength <= strlen(part)) {
        compareLength = strlen(part);
    }
    if (strncmp(part, elementName + i, compareLength) == 0) {
        return gsi_true;
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsLargeInt(GSXmlStreamReader stream, const char *name, gsLargeInt_t *value) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    int readSize;

    readSize = 0;
    memset(value, 0, sizeof(gsLargeInt_t));
    if (!gsXmlReadChildAsHexBinary(reader, name, (gsi_u8 *)value->mData, 0x200, &readSize)) {
        return gsi_false;
    }
    value->mLength = (gsi_u32)readSize / 4;
    if (readSize & 3) {
        value->mLength++;
    }
    if (!gsLargeIntReverseBytes(value)) {
        return gsi_false;
    }
    return gsi_true;
}

int gsXmlCountChildren(GSXmlStreamReader stream, const char *name) {
    GSIXmlStreamReader *reader = (GSIXmlStreamReader *)stream;
    GSIXmlElement *element;
    int count;
    int i;

    count = 0;
    for (i = reader->mElemReadIndex + 1; i < ArrayLength(reader->mElementArray); i++) {
        element = ArrayNth(reader->mElementArray, i);
        if (element->mParentIndex == reader->mElemReadIndex) {
            if (name == NULL || gsiXmlUtilTagMatches(name, element)) {
                count++;
            }
        } else if (element->mParentIndex < reader->mElemReadIndex) {
            break;
        }
    }
    return count;
}
