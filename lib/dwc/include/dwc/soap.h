#ifndef POKEBW2_DWC_SOAP_H
#define POKEBW2_DWC_SOAP_H

#include "types.h"

// Overlay 11's SOAP message writer and reader, with the hash and the big numbers of the signature check, which
// GameSpy's services in overlay 260 (Sake, the competition and the authentication) talk through. The ROM names none
// of it, so these are the addresses with a word on what each does, read off the callers. Every writing and reading
// function returns a BOOL that tells whether it worked. A message has a namespace, written as the prefix that a
// declaration such as `ns1="http://gamespy.net/sake"` gives, and elements, which open and close in nesting order

typedef struct SoapWriter SoapWriter;
typedef struct SoapReader SoapReader;

// A big number of 0x104 bytes, as the signature check reads and writes them
typedef struct BigNum {
    u32 words[0x41];
} BigNum;
// The public key of the signature check, a modulus and an exponent
typedef struct RsaPublicKey {
    BigNum modulus;
    BigNum exponent;
} RsaPublicKey;
// The state of the hash that gives 16 bytes, and its digest
typedef struct HashContext {
    u32 words[0x16];
} HashContext;

// Starts a message with the declarations of its namespaces and how many they are; frees it; and ends it so that it
// can be posted, which gives a BOOL
SoapWriter *func_ov011_0216fbf8(const char **namespaceDeclarations, int declarationNum);
void func_ov011_0216fe64(SoapWriter *writer);
BOOL func_ov011_0216fea8(SoapWriter *writer);

// Opens and closes an element of a namespace
BOOL func_ov011_02170614(SoapWriter *writer, const char *prefix, const char *name);
BOOL func_ov011_02170660(SoapWriter *writer, const char *prefix, const char *name);
// Writes an element with a value: a text, a text of a certificate (the same), a wide text, an int, a 64-bit int, a
// float, a block of bytes written as text with the size, a block of bytes with the size, a date and time, and a
// big number
BOOL func_ov011_021706b8(SoapWriter *writer, const char *prefix, const char *name, const char *value);
BOOL func_ov011_0217071c(SoapWriter *writer, const char *prefix, const char *name, const char *value);
BOOL func_ov011_02170780(SoapWriter *writer, const char *prefix, const char *name, const u16 *value);
BOOL func_ov011_02170818(SoapWriter *writer, const char *prefix, const char *name, int value);
BOOL func_ov011_02170868(SoapWriter *writer, const char *prefix, const char *name, s64 value);
BOOL func_ov011_021708b4(SoapWriter *writer, const char *prefix, const char *name, float value);
BOOL func_ov011_0217090c(SoapWriter *writer, const char *prefix, const char *name, const void *data, int size);
BOOL func_ov011_0217098c(SoapWriter *writer, const char *prefix, const char *name, const void *data, int size);
BOOL func_ov011_02170a00(SoapWriter *writer, const char *prefix, const char *name, int dateAndTime);
BOOL func_ov011_02170a80(SoapWriter *writer, const char *prefix, const char *name, const BigNum *value);

// Checks the answer's envelope; goes into an element of the answer; goes out of the element it is in; and tells
// whether the element it is in has a child of the name
BOOL func_ov011_02170d18(SoapReader *reader);
BOOL func_ov011_02170d28(SoapReader *reader, const char *name);
BOOL func_ov011_02170d70(SoapReader *reader);
BOOL func_ov011_02170db8(SoapReader *reader, const char *name);
// Reads a child: its text and length without copying, its text into a buffer of a size, a block of bytes into a
// buffer of a size (and how many it was), a block of bytes of a size it tells, an int, a 64-bit int, a date and
// time, a float, a big number
BOOL func_ov011_02170e14(SoapReader *reader, const char *name, char **text, int *length);
BOOL func_ov011_02170e90(SoapReader *reader, const char *name, char *buffer, int size);
BOOL func_ov011_02170ed8(SoapReader *reader, const char *name, void *buffer, int size, int *readSize);
BOOL func_ov011_02170fec(SoapReader *reader, const char *name, void *buffer, int *size);
BOOL func_ov011_02171090(SoapReader *reader, const char *name, int *value);
BOOL func_ov011_02171108(SoapReader *reader, const char *name, s64 *value);
BOOL func_ov011_02171184(SoapReader *reader, const char *name, int *value);
BOOL func_ov011_0217123c(SoapReader *reader, const char *name, float *value);
BOOL func_ov011_02171330(SoapReader *reader, const char *name, BigNum *value);
// How many children of the name the element it is in has
int func_ov011_02171390(SoapReader *reader, const char *name);

// Makes a copy of a text as a wide text (0 when it could not)
u16 *func_ov011_0216edd0(const char *text);
// Sets a big number from a text of hexadecimal digits, and adds one to a hash
BOOL func_ov011_0216dc38(BigNum *number, const char *hex);
void func_ov011_0216dd04(const BigNum *number, HashContext *context);
// Starts a hash, hashes a block of bytes and gives its 16 bytes
void func_ov011_021719fc(HashContext *context);
void func_ov011_02171a24(HashContext *context, const void *data, u32 size);
void func_ov011_02171a9c(u8 *digest, HashContext *context);
// Checks the signature of a digest with a public key, returning 0 when it is right
int func_ov011_0216cac0(const RsaPublicKey *key, const u8 *digest, int digestSize, const u8 *signature,
                        int signatureSize);

#endif // POKEBW2_DWC_SOAP_H
