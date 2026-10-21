#ifndef POKEBW2_DWC_GSXML_H
#define POKEBW2_DWC_GSXML_H

#include "dwc/gsLargeInt.h"
#include "dwc/nonport.h"

// GameSpy's XML (gsXML.c), in overlay 11: the writer of the SOAP messages that GameSpy's services in overlay 260
// (Sake, the competition and the authentication) post, and the reader of their answers. Every writing and reading
// function returns a gsi_bool that tells whether it worked. A message has a namespace, written as the prefix that a
// declaration such as `ns1="http://gamespy.net/sake"` gives, and elements, which open and close in nesting order

// A writer (GameSpy's GSXmlStreamWriter) and a reader (GSXmlStreamReader)
typedef struct SoapWriter SoapWriter;
typedef struct SoapReader SoapReader;

// Starts a message with the declarations of its namespaces and how many they are; frees it; and ends it so that it
// can be posted
SoapWriter *gsXmlCreateStreamWriter(const char **namespaces, int count);
void gsXmlFreeWriter(SoapWriter *writer);
gsi_bool gsXmlCloseWriter(SoapWriter *writer);
// The text that a writer has made, and its length
const char *gsXmlWriterGetData(SoapWriter *writer);
int gsXmlWriterGetDataLength(SoapWriter *writer);

// Opens and closes an element of a namespace
gsi_bool gsXmlWriteOpenTag(SoapWriter *writer, const char *prefix, const char *name);
gsi_bool gsXmlWriteCloseTag(SoapWriter *writer, const char *prefix, const char *name);
// Writes an element with a value: a text, a text again (the one of GameSpy's character type), a wide text, an int, a
// 64-bit int, a float, bytes as hexadecimal digits, bytes in base 64, a date and time, and a large integer
gsi_bool gsXmlWriteStringElement(SoapWriter *writer, const char *prefix, const char *name, const char *value);
gsi_bool gsXmlWriteAsciiStringElement(SoapWriter *writer, const char *prefix, const char *name, const char *value);
gsi_bool gsXmlWriteUnicodeStringElement(SoapWriter *writer, const char *prefix, const char *name, const gsi_u16 *value);
gsi_bool gsXmlWriteIntElement(SoapWriter *writer, const char *prefix, const char *name, int value);
gsi_bool gsXmlWriteInt64Element(SoapWriter *writer, const char *prefix, const char *name, gsi_i64 value);
gsi_bool gsXmlWriteFloatElement(SoapWriter *writer, const char *prefix, const char *name, float value);
gsi_bool gsXmlWriteHexBinaryElement(SoapWriter *writer, const char *prefix, const char *name, const void *data,
                                    int size);
gsi_bool gsXmlWriteBase64BinaryElement(SoapWriter *writer, const char *prefix, const char *name, const void *data,
                                       int size);
gsi_bool gsXmlWriteDateTimeElement(SoapWriter *writer, const char *prefix, const char *name, int seconds);
gsi_bool gsXmlWriteLargeIntElement(SoapWriter *writer, const char *prefix, const char *name, const BigNum *value);

// Makes a reader; reads a text of a length into it (the reader keeps pointers into the text); frees it; and empties it
SoapReader *gsXmlCreateStreamReader(void);
gsi_bool gsXmlParseXml(SoapReader *reader, char *text, int length);
void gsXmlFreeReader(SoapReader *reader);
void gsXmlResetReader(SoapReader *reader);

// Goes back to before the first element; to the next element of a name; out of the element it is in; and into a
// child of the name of the element it is in
gsi_bool gsXmlMoveToStart(SoapReader *reader);
gsi_bool gsXmlMoveToNext(SoapReader *reader, const char *name);
gsi_bool gsXmlMoveToParent(SoapReader *reader);
gsi_bool gsXmlMoveToChild(SoapReader *reader, const char *name);
// Reads the next child of a name of the element it is in: its text and length without copying, its text into a
// buffer of a size, bytes written as hexadecimal digits into a buffer of a size (and how many it read; with no buffer,
// how many there are), bytes in base 64 (the same), an int, a 64-bit int, a date and time as seconds, a float and a
// large integer
gsi_bool gsXmlReadChildAsString(SoapReader *reader, const char *name, char **text, int *length);
gsi_bool gsXmlReadChildAsStringNT(SoapReader *reader, const char *name, char *buffer, int size);
gsi_bool gsXmlReadChildAsHexBinary(SoapReader *reader, const char *name, void *buffer, int size, int *readSize);
gsi_bool gsXmlReadChildAsBase64Binary(SoapReader *reader, const char *name, void *buffer, int *size);
gsi_bool gsXmlReadChildAsInt(SoapReader *reader, const char *name, int *value);
gsi_bool gsXmlReadChildAsInt64(SoapReader *reader, const char *name, gsi_i64 *value);
gsi_bool gsXmlReadChildAsDateTimeElement(SoapReader *reader, const char *name, int *seconds);
gsi_bool gsXmlReadChildAsFloat(SoapReader *reader, const char *name, float *value);
gsi_bool gsXmlReadChildAsLargeInt(SoapReader *reader, const char *name, BigNum *value);
// How many children of the name (or of any name, for NULL) the element it is in has
int gsXmlCountChildren(SoapReader *reader, const char *name);

#endif // POKEBW2_DWC_GSXML_H
