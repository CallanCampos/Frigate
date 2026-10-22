#ifndef POKEBW2_DWC_GSXML_H
#define POKEBW2_DWC_GSXML_H

#include "dwc/gsLargeInt.h"
#include "dwc/nonport.h"

// GameSpy's XML (gsXML.c), in overlay 11: the writer of the SOAP messages that GameSpy's services in overlay 260
// (Sake, the competition and the authentication) post, and the reader of their answers. Every writing and reading
// function returns a gsi_bool that tells whether it worked. A message has a namespace, written as the prefix that a
// declaration such as `ns1="http://gamespy.net/sake"` gives, and elements, which open and close in nesting order

// A writer and a reader, which gsXML.c alone sees inside
typedef void *GSXmlStreamReader;
typedef void *GSXmlStreamWriter;

// Starts a message with the declarations of its namespaces and how many they are; frees it; and ends it so that it
// can be posted
GSXmlStreamWriter gsXmlCreateStreamWriter(const char **namespaces, int count);
void gsXmlFreeWriter(GSXmlStreamWriter writer);
gsi_bool gsXmlCloseWriter(GSXmlStreamWriter writer);
// The text that a writer has made, and its length
const char *gsXmlWriterGetData(GSXmlStreamWriter writer);
int gsXmlWriterGetDataLength(GSXmlStreamWriter writer);

// Opens and closes an element of a namespace
gsi_bool gsXmlWriteOpenTag(GSXmlStreamWriter writer, const char *prefix, const char *name);
gsi_bool gsXmlWriteCloseTag(GSXmlStreamWriter writer, const char *prefix, const char *name);
// Writes an element with a value: a text, a text again (the one of GameSpy's character type), a wide text, an int, a
// 64-bit int, a float, bytes as hexadecimal digits, bytes in base 64, a date and time, and a large integer
gsi_bool gsXmlWriteStringElement(GSXmlStreamWriter writer, const char *prefix, const char *name, const char *value);
gsi_bool gsXmlWriteAsciiStringElement(GSXmlStreamWriter writer, const char *prefix, const char *name,
                                      const gsi_char *value);
gsi_bool gsXmlWriteUnicodeStringElement(GSXmlStreamWriter writer, const char *prefix, const char *name,
                                        const unsigned short *value);
gsi_bool gsXmlWriteIntElement(GSXmlStreamWriter writer, const char *prefix, const char *name, gsi_u32 value);
gsi_bool gsXmlWriteInt64Element(GSXmlStreamWriter writer, const char *prefix, const char *name, gsi_i64 value);
gsi_bool gsXmlWriteFloatElement(GSXmlStreamWriter writer, const char *prefix, const char *name, float value);
gsi_bool gsXmlWriteHexBinaryElement(GSXmlStreamWriter writer, const char *prefix, const char *name, const gsi_u8 *data,
                                    int size);
gsi_bool gsXmlWriteBase64BinaryElement(GSXmlStreamWriter writer, const char *prefix, const char *name,
                                       const gsi_u8 *data, int size);
gsi_bool gsXmlWriteDateTimeElement(GSXmlStreamWriter writer, const char *prefix, const char *name, int seconds);
gsi_bool gsXmlWriteLargeIntElement(GSXmlStreamWriter writer, const char *prefix, const char *name,
                                   const gsLargeInt_t *value);

// Makes a reader; reads a text of a length into it (the reader keeps pointers into the text); frees it; and empties it
GSXmlStreamReader gsXmlCreateStreamReader(void);
gsi_bool gsXmlParseXml(GSXmlStreamReader reader, char *text, int length);
void gsXmlFreeReader(GSXmlStreamReader reader);
void gsXmlResetReader(GSXmlStreamReader reader);

// Goes back to before the first element; to the next element of a name; out of the element it is in; and into a
// child of the name of the element it is in
gsi_bool gsXmlMoveToStart(GSXmlStreamReader reader);
gsi_bool gsXmlMoveToNext(GSXmlStreamReader reader, const char *name);
gsi_bool gsXmlMoveToParent(GSXmlStreamReader reader);
gsi_bool gsXmlMoveToChild(GSXmlStreamReader reader, const char *name);
// Reads the next child of a name of the element it is in: its text and length without copying, its text into a
// buffer of a size, bytes written as hexadecimal digits into a buffer of a size (and how many it read; with no buffer,
// how many there are), bytes in base 64 (the same), an int, a 64-bit int, a date and time as seconds, a float and a
// large integer
gsi_bool gsXmlReadChildAsString(GSXmlStreamReader reader, const char *name, const char **text, int *length);
gsi_bool gsXmlReadChildAsStringNT(GSXmlStreamReader reader, const char *name, char buffer[], int size);
gsi_bool gsXmlReadChildAsHexBinary(GSXmlStreamReader reader, const char *name, gsi_u8 buffer[], int size,
                                   int *readSize);
gsi_bool gsXmlReadChildAsBase64Binary(GSXmlStreamReader reader, const char *name, gsi_u8 buffer[], int *size);
gsi_bool gsXmlReadChildAsInt(GSXmlStreamReader reader, const char *name, int *value);
gsi_bool gsXmlReadChildAsInt64(GSXmlStreamReader reader, const char *name, gsi_i64 *value);
gsi_bool gsXmlReadChildAsDateTimeElement(GSXmlStreamReader reader, const char *name, int *seconds);
gsi_bool gsXmlReadChildAsFloat(GSXmlStreamReader reader, const char *name, float *value);
gsi_bool gsXmlReadChildAsLargeInt(GSXmlStreamReader reader, const char *name, gsLargeInt_t *value);
// How many children of the name (or of any name, for NULL) the element it is in has
int gsXmlCountChildren(GSXmlStreamReader reader, const char *name);

#endif // POKEBW2_DWC_GSXML_H
