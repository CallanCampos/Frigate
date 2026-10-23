#ifndef POKEBW2_DWC_GPIKEYS_H
#define POKEBW2_DWC_GPIKEYS_H

#include "types.h"
#include "dwc/gpi.h"

// The keys and values of the status of GameSpy's GP (gpiKeys.c), in overlay 11, kept in sorted arrays of GPIKey (see
// dwc/gp.h for the names)

void gpiStatusInfoKeyFree(void *element);
GPResult gpiStatusInfoKeysInit(GPConnection *connection);
void gpiStatusInfoKeysDestroy(GPConnection *connection);
GPResult gpiStatusInfoAddKey(GPConnection *connection, DArray keys, const char *keyName, const char *keyValue);
GPResult gpiStatusInfoSetKey(GPConnection *connection, DArray keys, const char *keyName, const char *newValue);
GPResult gpiStatusInfoGetKey(GPConnection *connection, DArray keys, const char *keyName, char **keyValue);
// Writes the keys of the connection's status as "\\keys\\<count>" and the base-64 names and values
GPResult gpiSaveKeysToBuffer(GPConnection *connection, char **buffer);

#endif // POKEBW2_DWC_GPIKEYS_H
