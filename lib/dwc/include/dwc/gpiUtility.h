#ifndef POKEBW2_DWC_GPIUTILITY_H
#define POKEBW2_DWC_GPIUTILITY_H

#include "dwc/gpi.h"
#include "dwc/nonport.h"

// The helpers of GameSpy's GP (gpiUtility.c), in overlay 11: reading the values of the server's "\\key\\value"
// messages, the errors and the encoding of passwords (see dwc/gp.h for the names)

// strncpy that always ends the text
void strzcpy(char *dest, const char *src, gsi_u32 len);
// Whether the server sent an "\\error\\", which it records (and passes to the error callback if asked to)
gsi_bool gpiCheckForError(GPConnection *connection, const char *input, gsi_bool callErrorCallback);
// Read the value after a key, up to the next character like the key's first; the first from index on, which moves
// index past the value
gsi_bool gpiValueForKeyWithIndex(const char *command, const char *key, int *index, char *value, int len);
gsi_bool gpiValueForKey(const char *command, const char *key, char *value, int len);
// Whether a socket connected: state is 0 (not yet), 3 (connected) or 4 (refused)
int gpiCheckSocketConnect(GPConnection *connection, int sock, int *state);
int gpiReadKeyAndValue(GPConnection *connection, const char *buffer, int *index, char *key, char *value);
void gpiSetError(GPConnection *connection, int errorCode, const char *errorString);
void gpiSetErrorString(GPConnection *connection, const char *errorString);
// Scrambles a text with the generator seeded with "gsp" and writes it in base 64
void gpiEncodeString(const char *unencodedString, char *encodedString);

#endif // POKEBW2_DWC_GPIUTILITY_H
