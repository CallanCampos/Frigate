#include "dwc/gsXML.h"
#include "dwc/ext.h"
// The length of a wide text, which gt2.c has
#include "dwc/gt2.h"
#include "nitro/std.h"

// GameSpy's XML (gsXML.c). The ROM names neither the file nor its functions: the file, gsXmlWriterGetData and
// gsXmlWriterGetDataLength are GameSpy's, as pret's pokerevo Wii build has them (gsXML.s), and the other functions are
// the later SDK's (the debug texts name gsXmlCreateStreamWriter and gsXmlCreateStream), but for gsiXmlUtilGrowBuffer,
// ours

// A part of the text of a message, which isn't ended
typedef struct XmlString {
    char *data;
    int len;
} XmlString;

// An element (or an attribute) that the reader found: its name and value, its index and the index of the element it
// is in (for an attribute, the element it belongs to), -1 for none
typedef struct XmlElement {
    XmlString name;
    XmlString value;
    int index;
    int parent;
} XmlElement;

// A writer: its text, the length and the size of the buffer, and whether the message was ended
struct SoapWriter {
    char *buffer;
    int length;
    int capacity;
    gsi_bool closed;
};

// A reader: the elements and the attributes that the text had, the element it is in, the child that was read last and
// the attribute that was read last, -1 for none
struct SoapReader {
    DArray *elements;
    DArray *attributes;
    int element;
    int child;
    int attribute;
};

static void gsiXmlUtilElementFree(void *element);
static void gsiXmlUtilAttributeFree(void *element);
static gsi_bool gsiXmlUtilSkipWhiteSpace(SoapReader *reader, const char *text, int length, int *pos);
static gsi_bool gsiXmlUtilParseElement(SoapReader *reader, char *text, int length, int *pos, int parent);
static gsi_bool gsiXmlUtilParseName(SoapReader *reader, const char *text, int length, int *pos, XmlString *out);
static gsi_bool gsiXmlUtilParseString(SoapReader *reader, const char *text, int length, int *pos, XmlString *out);
static gsi_bool gsiXmlUtilDecodeString(char *text, int *length);
static gsi_bool gsiXmlUtilParseValue(SoapReader *reader, char *text, int length, int *pos, XmlString *out);
static gsi_bool gsiXmlUtilWriteChar(SoapWriter *writer, int character);
static gsi_bool gsiXmlUtilWriteString(SoapWriter *writer, const char *text);
static gsi_bool gsiXmlUtilWriteUnicodeString(SoapWriter *writer, const gsi_u16 *text);
static gsi_bool gsiXmlUtilWriteXmlSafeString(SoapWriter *writer, const char *text);
static gsi_bool gsiXmlUtilGrowBuffer(SoapWriter *writer);
static gsi_bool gsiXmlUtilTagMatches(const char *name, const XmlElement *element);

// The namespaces that every message declares
const char *GS_XML_SOAP_DEFAULT_NAMESPACES[4] = {
    "SOAP-ENV=\"http://schemas.xmlsoap.org/soap/envelope/\"",
    "SOAP-ENC=\"http://schemas.xmlsoap.org/soap/encoding/\"",
    "xsi=\"http://www.w3.org/2001/XMLSchema-instance\"",
    "xsd=\"http://www.w3.org/2001/XMLSchema\"",
};

SoapWriter *gsXmlCreateStreamWriter(const char **namespaceDeclarations, int declarationNum) {
    SoapWriter *writer;
    int size;
    int capacity;
    int i;

    capacity = 0x400;
    size = 0;
    writer = gsimalloc(sizeof(SoapWriter));
    if (writer == NULL) {
        gsDebugFormat(12, 2, 1, "Out of memory in gsXmlCreateStreamWriter, needed %d bytes", sizeof(SoapWriter));
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
    writer->buffer = gsimalloc(capacity);
    if (writer->buffer == NULL) {
        gsDebugFormat(12, 2, 1, "Out of memory in gsXmlCreateStreamWriter, needed %d bytes", capacity);
        return NULL;
    }
    writer->capacity = capacity;
    writer->length = 0;
    writer->buffer[0] = 0;
    writer->closed = gsi_false;
    if (!gsiXmlUtilWriteString(writer, "<?xml version=\"1.0\" encoding=\"UTF-8\"?><SOAP-ENV:Envelope")) {
        gsifree(writer->buffer);
        gsifree(writer);
        return NULL;
    }
    for (i = 0; i < 4; i++) {
        if (!gsiXmlUtilWriteChar(writer, ' ') || !gsiXmlUtilWriteString(writer, "xmlns:") ||
            !gsiXmlUtilWriteString(writer, GS_XML_SOAP_DEFAULT_NAMESPACES[i])) {
            gsifree(writer->buffer);
            gsifree(writer);
            return NULL;
        }
    }
    for (i = 0; i < declarationNum; i++) {
        if (!gsiXmlUtilWriteChar(writer, ' ') || !gsiXmlUtilWriteString(writer, "xmlns:") ||
            !gsiXmlUtilWriteString(writer, namespaceDeclarations[i])) {
            gsifree(writer->buffer);
            gsifree(writer);
            return NULL;
        }
    }
    if (!gsiXmlUtilWriteChar(writer, '>') || !gsiXmlUtilWriteString(writer, "<SOAP-ENV:Body>")) {
        gsifree(writer->buffer);
        gsifree(writer);
        return NULL;
    }
    return writer;
}

SoapReader *gsXmlCreateStreamReader(void) {
    SoapReader *reader;

    reader = gsimalloc(sizeof(SoapReader));
    if (reader == NULL) {
        gsDebugFormat(12, 2, 1, "Out of memory in gsXmlCreateStream, needed %d bytes", sizeof(SoapReader));
        return NULL;
    }
    reader->elements = func_ov011_02171514(sizeof(XmlElement), 32, gsiXmlUtilElementFree);
    if (reader->elements == NULL) {
        gsDebugFormat(12, 2, 1, "Out of memory in gsXmlCreateStream with mElementArray=ArrayNew()");
        gsifree(reader);
        return NULL;
    }
    reader->attributes = func_ov011_02171514(sizeof(XmlElement), 16, gsiXmlUtilAttributeFree);
    if (reader->attributes == NULL) {
        gsDebugFormat(12, 2, 1, "Out of memory in gsXmlCreateStream with mElementArray=ArrayNew()");
        func_ov011_02171558(reader->elements);
        gsifree(reader);
        return NULL;
    }
    gsXmlMoveToStart(reader);
    return reader;
}

gsi_bool gsXmlParseXml(SoapReader *reader, char *text, int length) {
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

void gsXmlFreeWriter(SoapWriter *writer) {
    gsifree(writer->buffer);
    gsifree(writer);
}

void gsXmlFreeReader(SoapReader *reader) {
    func_ov011_02171558(reader->attributes);
    func_ov011_02171558(reader->elements);
    gsifree(reader);
}

void gsXmlResetReader(SoapReader *reader) {
    ArrayClear(reader->attributes);
    ArrayClear(reader->elements);
    gsXmlMoveToStart(reader);
}

gsi_bool gsXmlCloseWriter(SoapWriter *writer) {
    if (!gsiXmlUtilWriteString(writer, "</SOAP-ENV:Body></SOAP-ENV:Envelope>")) {
        return gsi_false;
    }
    writer->closed = gsi_true;
    return gsi_true;
}

static void gsiXmlUtilElementFree(void *element) {
}

static void gsiXmlUtilAttributeFree(void *element) {
}

const char *gsXmlWriterGetData(SoapWriter *writer) {
    return writer->buffer;
}

int gsXmlWriterGetDataLength(SoapWriter *writer) {
    return writer->length;
}

static gsi_bool gsiXmlUtilSkipWhiteSpace(SoapReader *reader, const char *text, int length, int *pos) {
    while (*pos < length) {
        if (strchr(" \t\r\n", text[*pos]) == NULL) {
            return gsi_true;
        }
        (*pos)++;
    }
    return gsi_false;
}

static gsi_bool gsiXmlUtilParseElement(SoapReader *reader, char *text, int length, int *pos, int parent) {
    int tagStart;
    gsi_bool store;
    int nameLength;
    int childStart;

    store = gsi_true;
    gsiXmlUtilSkipWhiteSpace(reader, text, length, pos);
    if (*pos >= length) {
        return gsi_true;
    }
    XmlElement element = { { NULL, 0 }, { NULL, 0 }, 0, 0 };
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
    if (!gsiXmlUtilParseName(reader, text, length, pos, &element.name)) {
        return gsi_false;
    }
    if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
        return gsi_false;
    }
    if (store) {
        element.index = func_ov011_02171588(reader->elements);
        element.parent = parent;
        func_ov011_021715a4(reader->elements, &element);
    }
    while (*pos < length && isalnum(text[*pos])) {
        XmlElement attribute = { { NULL, 0 }, { NULL, 0 }, 0, 0 };

        if (!gsiXmlUtilParseName(reader, text, length, pos, &attribute.name)) {
            return gsi_false;
        }
        if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
            return gsi_false;
        }
        if (text[*pos] != '=') {
            return gsi_false;
        }
        (*pos)++;
        if (!gsiXmlUtilParseString(reader, text, length, pos, &attribute.value)) {
            return gsi_false;
        }
        if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
            return gsi_false;
        }
        if (store) {
            attribute.index = func_ov011_02171588(reader->attributes);
            attribute.parent = element.index;
            func_ov011_021715a4(reader->attributes, &attribute);
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
        if (!gsiXmlUtilParseValue(reader, text, length, pos, &element.value)) {
            return gsi_false;
        }
        if (store) {
            ArrayReplaceAt(reader->elements, &element, element.index);
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
            nameLength = element.name.len;
            if (*pos + nameLength >= length) {
                return gsi_false;
            }
            if (strncmp(element.name.data, text + *pos, nameLength) != 0) {
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
        if (element.value.data != NULL) {
            return gsi_false;
        }
        *pos = childStart;
        if (!gsiXmlUtilParseElement(reader, text, length, pos, element.index)) {
            return gsi_false;
        }
        gsiXmlUtilSkipWhiteSpace(reader, text, length, pos);
    }
    return gsi_false;
}

static gsi_bool gsiXmlUtilParseName(SoapReader *reader, const char *text, int length, int *pos, XmlString *out) {
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
    out->data = start;
    out->len = 1;
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
        out->len++;
        (*pos)++;
    }
    return gsi_true;
}

static gsi_bool gsiXmlUtilParseString(SoapReader *reader, const char *text, int length, int *pos, XmlString *out) {
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
    out->len = 0;
    if (*pos >= length) {
        return gsi_false;
    }
    if (quote == text[*pos]) {
        out->data = start;
        (*pos)++;
        return gsi_true;
    }
    while (quote != text[*pos]) {
        if (*pos >= length) {
            return gsi_false;
        }
        (*pos)++;
        out->len++;
    }
    (*pos)++;
    if (!gsiXmlUtilDecodeString(start, &out->len)) {
        return gsi_false;
    }
    out->data = start;
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

static gsi_bool gsiXmlUtilParseValue(SoapReader *reader, char *text, int length, int *pos, XmlString *out) {
    char *start;

    start = NULL;
    if (!gsiXmlUtilSkipWhiteSpace(reader, text, length, pos)) {
        return gsi_false;
    }
    if (text[*pos] != '<') {
        start = text + *pos;
        out->data = start;
    }
    while (*pos < length) {
        if (text[*pos] == '<') {
            return gsiXmlUtilDecodeString(start, &out->len) != 0;
        }
        (*pos)++;
        out->len++;
    }
    return gsi_false;
}

gsi_bool gsXmlWriteOpenTag(SoapWriter *writer, const char *prefix, const char *name) {
    if (!gsiXmlUtilWriteChar(writer, '<') || !gsiXmlUtilWriteString(writer, prefix) ||
        !gsiXmlUtilWriteChar(writer, ':') || !gsiXmlUtilWriteString(writer, name) ||
        !gsiXmlUtilWriteChar(writer, '>')) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteCloseTag(SoapWriter *writer, const char *prefix, const char *name) {
    if (!gsiXmlUtilWriteChar(writer, '<') || !gsiXmlUtilWriteChar(writer, '/') ||
        !gsiXmlUtilWriteString(writer, prefix) || !gsiXmlUtilWriteChar(writer, ':') ||
        !gsiXmlUtilWriteString(writer, name) || !gsiXmlUtilWriteChar(writer, '>')) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteStringElement(SoapWriter *writer, const char *prefix, const char *name, const char *value) {
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

gsi_bool gsXmlWriteAsciiStringElement(SoapWriter *writer, const char *prefix, const char *name, const char *value) {
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

gsi_bool gsXmlWriteUnicodeStringElement(SoapWriter *writer, const char *prefix, const char *name,
                                        const gsi_u16 *value) {
    int i;
    int length;

    length = Gt2_WStrLen(value);
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

gsi_bool gsXmlWriteIntElement(SoapWriter *writer, const char *prefix, const char *name, int value) {
    char text[32];

    sprintf(text, "%d", value);
    if (!gsXmlWriteOpenTag(writer, prefix, name) || !gsiXmlUtilWriteString(writer, text) ||
        !gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteInt64Element(SoapWriter *writer, const char *prefix, const char *name, gsi_i64 value) {
    char text[36];

    gsiInt64ToString(text, value);
    if (!gsXmlWriteOpenTag(writer, prefix, name) || !gsiXmlUtilWriteString(writer, text) ||
        !gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteFloatElement(SoapWriter *writer, const char *prefix, const char *name, float value) {
    char text[32];

    sprintf(text, "%f", (double)value);
    if (!gsXmlWriteOpenTag(writer, prefix, name) || !gsiXmlUtilWriteString(writer, text) ||
        !gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteHexBinaryElement(SoapWriter *writer, const char *prefix, const char *name, const void *data,
                                    int size) {
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

gsi_bool gsXmlWriteBase64BinaryElement(SoapWriter *writer, const char *prefix, const char *name, const void *data,
                                       int size) {
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

gsi_bool gsXmlWriteDateTimeElement(SoapWriter *writer, const char *prefix, const char *name, int dateAndTime) {
    char text[24];
    GSTime *time;

    time = gsiSecondsToDate(&dateAndTime);
    sprintf(text, "%d-%02d-%02dT%02d:%02d:%02dZ", time->year + 1900, time->mon + 1, time->mday, time->hour, time->min,
            time->sec);
    if (!gsXmlWriteOpenTag(writer, prefix, name) || !gsiXmlUtilWriteString(writer, text) ||
        !gsXmlWriteCloseTag(writer, prefix, name)) {
        return gsi_false;
    }
    return gsi_true;
}

gsi_bool gsXmlWriteLargeIntElement(SoapWriter *writer, const char *prefix, const char *name, const BigNum *value) {
    const gsi_u32 *words;
    int i;
    gsi_u32 j;
    gsi_bool leading;
    char hex[4];

    i = value->length - 1;
    words = &value->data[0];
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
                if ((gsi_u8)(value->data[i] >> ((3 - j) * 8)) != 0) {
                    break;
                }
            }
            leading = gsi_false;
        } else {
            j = 0;
        }
        for (; j < 4; j++) {
            sprintf(hex, "%02x", (gsi_u8)(value->data[i] >> ((3 - j) * 8)));
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

static gsi_bool gsiXmlUtilWriteChar(SoapWriter *writer, int character) {
    if (writer->length >= writer->capacity) {
        if (!gsiXmlUtilGrowBuffer(writer)) {
            return gsi_false;
        }
    }
    writer->buffer[writer->length] = character;
    writer->length++;
    return gsi_true;
}

static gsi_bool gsiXmlUtilWriteString(SoapWriter *writer, const char *text) {
    int length;

    length = strlen(text);
    if (length == 0) {
        return gsi_true;
    }
    while (writer->capacity - writer->length <= length) {
        if (!gsiXmlUtilGrowBuffer(writer)) {
            return gsi_false;
        }
    }
    strcpy(writer->buffer + writer->length, text);
    writer->length += length;
    return gsi_true;
}

static gsi_bool gsiXmlUtilWriteUnicodeString(SoapWriter *writer, const gsi_u16 *text) {
    char utf8[4] = { 0, 0, 0, 0 };
    int length;
    int i;

    length = Gt2_WStrLen(text);
    utf8[3] = 0;
    for (i = 0; i < length; i++) {
        utf8[_UCS2CharToUTF8String(text[i], utf8)] = 0;
        if (!gsiXmlUtilWriteXmlSafeString(writer, utf8)) {
            return gsi_false;
        }
    }
    return gsi_true;
}

static gsi_bool gsiXmlUtilWriteXmlSafeString(SoapWriter *writer, const char *text) {
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

static gsi_bool gsiXmlUtilGrowBuffer(SoapWriter *writer) {
    int newCapacity;
    char *newBuffer;

    newCapacity = writer->capacity + 0x400;
    newBuffer = gsirealloc(writer->buffer, newCapacity);
    if (newBuffer == NULL) {
        return gsi_false;
    }
    if (newBuffer != writer->buffer) {
        writer->buffer = newBuffer;
    }
    writer->capacity = newCapacity;
    return gsi_true;
}

gsi_bool gsXmlMoveToStart(SoapReader *reader) {
    reader->element = -1;
    reader->child = -1;
    reader->attribute = -1;
    return gsi_true;
}

gsi_bool gsXmlMoveToNext(SoapReader *reader, const char *name) {
    int i;

    for (i = reader->element + 1; i < func_ov011_02171588(reader->elements); i++) {
        if (gsiXmlUtilTagMatches(name, func_ov011_0217158c(reader->elements, i))) {
            reader->element = i;
            reader->child = -1;
            reader->attribute = -1;
            return gsi_true;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlMoveToParent(SoapReader *reader) {
    int element;
    int parent;

    element = reader->element;
    if (element >= func_ov011_02171588(reader->elements) || element == -1) {
        return gsi_false;
    }
    parent = ((XmlElement *)func_ov011_0217158c(reader->elements, element))->parent;
    if (parent == -1) {
        return gsi_false;
    }
    if (parent >= func_ov011_02171588(reader->elements)) {
        return gsi_false;
    }
    reader->element = parent;
    reader->child = -1;
    reader->attribute = -1;
    return gsi_true;
}

gsi_bool gsXmlMoveToChild(SoapReader *reader, const char *name) {
    XmlElement *element;
    int i;

    for (i = reader->element + 1; i < func_ov011_02171588(reader->elements); i++) {
        element = func_ov011_0217158c(reader->elements, i);
        if (element->parent == reader->element && gsiXmlUtilTagMatches(name, element)) {
            reader->element = i;
            reader->child = -1;
            reader->attribute = -1;
            return gsi_true;
        }
        if (element->parent < reader->element) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsString(SoapReader *reader, const char *name, char **text, int *length) {
    XmlElement *element;
    int i;

    if (reader->child == -1) {
        reader->child = reader->element;
    }
    for (i = reader->child + 1; i < func_ov011_02171588(reader->elements); i++) {
        element = func_ov011_0217158c(reader->elements, i);
        if (element->parent == reader->element && gsiXmlUtilTagMatches(name, element)) {
            reader->child = i;
            *text = element->value.data;
            *length = element->value.len;
            return gsi_true;
        }
        if (element->parent < reader->element) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsStringNT(SoapReader *reader, const char *name, char *buffer, int size) {
    char *text;
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

gsi_bool gsXmlReadChildAsHexBinary(SoapReader *reader, const char *name, void *buffer, int size, int *readSize) {
    XmlElement *element;
    int i;
    int hexLength;
    int written;
    int offset;
    int value;
    int textLength;

    if (reader->child == -1) {
        reader->child = reader->element;
    }
    for (i = reader->child + 1; i < func_ov011_02171588(reader->elements); i++) {
        element = func_ov011_0217158c(reader->elements, i);
        if (element->parent == reader->element && gsiXmlUtilTagMatches(name, element)) {
            written = 0;
            value = 0;
            offset = 0;
            hexLength = size * 2;
            textLength = element->value.len;
            if (hexLength >= textLength) {
                hexLength = textLength;
            }
            if (textLength == 0 || element->value.data == NULL) {
                *(gsi_u8 *)buffer = 0;
                *readSize = 0;
                return gsi_true;
            }
            if (buffer == NULL) {
                *readSize = textLength / 2;
                return gsi_true;
            }
            while (hexLength > 1) {
                STD_TSScanf(element->value.data + offset, "%02x", &value);
                ((gsi_u8 *)buffer)[written++] = value;
                offset += 2;
                hexLength -= 2;
            }
            if (hexLength == 1) {
                STD_TSScanf(element->value.data + offset, "%01x", &value);
                ((gsi_u8 *)buffer)[written++] = value;
            }
            if (readSize != NULL) {
                *readSize = written;
            }
            reader->child = i;
            return gsi_true;
        }
        if (element->parent < reader->element) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsBase64Binary(SoapReader *reader, const char *name, void *buffer, int *size) {
    XmlElement *element;
    int i;

    if (reader->child == -1) {
        reader->child = reader->element;
    }
    for (i = reader->child + 1; i < func_ov011_02171588(reader->elements); i++) {
        element = func_ov011_0217158c(reader->elements, i);
        if (element->parent == reader->element && gsiXmlUtilTagMatches(name, element)) {
            if (buffer != NULL) {
                reader->child = i;
                if (element->value.data != NULL) {
                    B64Decode(element->value.data, buffer, element->value.len, size, 0);
                } else {
                    *size = 0;
                }
            } else if (element->value.data != NULL) {
                *size = B64DecodeLen(element->value.data, 0);
            } else {
                *size = 0;
            }
            return gsi_true;
        }
        if (element->parent < reader->element) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsInt(SoapReader *reader, const char *name, int *value) {
    XmlElement *element;
    int i;

    if (reader->child == -1) {
        reader->child = reader->element;
    }
    for (i = reader->child + 1; i < func_ov011_02171588(reader->elements); i++) {
        element = func_ov011_0217158c(reader->elements, i);
        if (element->parent == reader->element && gsiXmlUtilTagMatches(name, element)) {
            reader->child = i;
            if (element->value.data == NULL) {
                return gsi_false;
            }
            *value = atol(element->value.data);
            return gsi_true;
        }
        if (element->parent < reader->element) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsInt64(SoapReader *reader, const char *name, gsi_i64 *value) {
    XmlElement *element;
    int i;

    if (reader->child == -1) {
        reader->child = reader->element;
    }
    for (i = reader->child + 1; i < func_ov011_02171588(reader->elements); i++) {
        element = func_ov011_0217158c(reader->elements, i);
        if (element->parent == reader->element && gsiXmlUtilTagMatches(name, element)) {
            reader->child = i;
            if (element->value.data == NULL) {
                return gsi_false;
            }
            *value = gsiStringToInt64(element->value.data);
            return gsi_true;
        }
        if (element->parent < reader->element) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsDateTimeElement(SoapReader *reader, const char *name, int *value) {
    XmlElement *element;
    GSTime time;
    int i;

    if (reader->child == -1) {
        reader->child = reader->element;
    }
    for (i = reader->child + 1; i < func_ov011_02171588(reader->elements); i++) {
        element = func_ov011_0217158c(reader->elements, i);
        if (element->parent == reader->element && gsiXmlUtilTagMatches(name, element)) {
            reader->child = i;
            if (element->value.data == NULL) {
                return gsi_false;
            }
            STD_TSScanf(element->value.data, "%i-%02d-%02dT%02d:%02d:%02d", &time.year, &time.mon, &time.mday,
                        &time.hour, &time.min, &time.sec);
            time.year -= 1900;
            time.mon -= 1;
            time.isdst = -1;
            *value = gsiDateToSeconds(&time);
            return gsi_true;
        }
        if (element->parent < reader->element) {
            return gsi_false;
        }
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsFloat(SoapReader *reader, const char *name, float *value) {
    XmlElement *element;
    int i;

    if (reader->child == -1) {
        reader->child = reader->element;
    }
    for (i = reader->child + 1; i < func_ov011_02171588(reader->elements); i++) {
        element = func_ov011_0217158c(reader->elements, i);
        if (element->parent == reader->element && gsiXmlUtilTagMatches(name, element)) {
            reader->child = i;
            if (element->value.data == NULL) {
                return gsi_false;
            }
            *value = atof(element->value.data);
            return gsi_true;
        }
        if (element->parent < reader->element) {
            return gsi_false;
        }
    }
    return gsi_false;
}

// Whether the name (with the namespace prefix that its last colon ends, if it has one) is the one of the element
static gsi_bool gsiXmlUtilTagMatches(const char *name, const XmlElement *element) {
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
    nameLength = element->name.len;
    while (i < nameLength && (gsi_u8)element->name.data[i] != ':') {
        i++;
    }
    if (i == nameLength) {
        i = 0;
    } else {
        i++;
    }
    elementName = element->name.data;
    compareLength = nameLength - i;
    if (compareLength <= strlen(part)) {
        compareLength = strlen(part);
    }
    if (strncmp(part, elementName + i, compareLength) == 0) {
        return gsi_true;
    }
    return gsi_false;
}

gsi_bool gsXmlReadChildAsLargeInt(SoapReader *reader, const char *name, BigNum *value) {
    int readSize;

    readSize = 0;
    memset(value, 0, sizeof(BigNum));
    if (!gsXmlReadChildAsHexBinary(reader, name, &value->data[0], 0x200, &readSize)) {
        return gsi_false;
    }
    value->length = (gsi_u32)readSize / 4;
    if (readSize & 3) {
        value->length++;
    }
    if (!gsLargeIntReverseBytes(value)) {
        return gsi_false;
    }
    return gsi_true;
}

int gsXmlCountChildren(SoapReader *reader, const char *name) {
    XmlElement *element;
    int count;
    int i;

    count = 0;
    for (i = reader->element + 1; i < func_ov011_02171588(reader->elements); i++) {
        element = func_ov011_0217158c(reader->elements, i);
        if (element->parent == reader->element) {
            if (name == NULL || gsiXmlUtilTagMatches(name, element)) {
                count++;
            }
        } else if (element->parent < reader->element) {
            break;
        }
    }
    return count;
}
