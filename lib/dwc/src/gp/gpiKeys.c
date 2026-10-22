#include "dwc/gpiKeys.h"
#include "dwc/darray.h"
#include "dwc/ext.h"
#include "dwc/gpiUtility.h"

// GameSpy GP's keys of the status (gpiKeys.c). The functions' names, from GameSpy's later SDK, are in dwc/gp.h

static int gpiStatusInfoKeyCompFunc(const void *elem1, const void *elem2);

void gpiStatusInfoKeyFree(void *element) {
    GPIKey *key = element;

    gsifree(key->keyName);
    key->keyName = NULL;
    gsifree(key->keyValue);
    key->keyValue = NULL;
}

GPResult gpiStatusInfoKeysInit(GPConnection *connection) {
    GPIConnection *iconnection = *connection;

    iconnection->extendedInfoKeys = ArrayNew(sizeof(GPIKey), 1, gpiStatusInfoKeyFree);
    if (iconnection->extendedInfoKeys == NULL) {
        gpiSetErrorString(connection, "Out of memory.");
        return 1;
    }
    return 0;
}

void gpiStatusInfoKeysDestroy(GPConnection *connection) {
    GPIConnection *iconnection = *connection;

    if (iconnection->extendedInfoKeys != NULL) {
        ArrayFree(iconnection->extendedInfoKeys);
        iconnection->extendedInfoKeys = NULL;
    }
}

static int gpiStatusInfoKeyCompFunc(const void *elem1, const void *elem2) {
    return strcmp(((const GPIKey *)elem1)->keyName, ((const GPIKey *)elem2)->keyName);
}

GPResult gpiStatusInfoAddKey(GPConnection *connection, DArray keys, const char *keyName, const char *keyValue) {
    GPIKey key;

    if (keyName == NULL) {
        gpiSetErrorString(connection, "Invalid key name");
        return 2;
    }
    if (keyValue == NULL) {
        gpiSetErrorString(connection, "Invalid key value");
        return 2;
    }
    key.keyName = goastrdup(keyName);
    key.keyValue = goastrdup(keyValue);
    ArrayInsertSorted(keys, &key, gpiStatusInfoKeyCompFunc);
    return 0;
}

GPResult gpiStatusInfoSetKey(GPConnection *connection, DArray keys, const char *keyName, const char *newValue) {
    GPIKey key;
    int index;
    GPIKey *found;

    if (keyName == NULL) {
        gpiSetErrorString(connection, "Invalid key name");
        return 2;
    }
    key.keyName = goastrdup(keyName);
    index = ArraySearch(keys, &key, gpiStatusInfoKeyCompFunc, 0, 1);
    if (index != -1) {
        found = ArrayNth(keys, index);
        gsifree(found->keyValue);
        found->keyValue = goastrdup(newValue);
    }
    gsifree(key.keyName);
    key.keyName = NULL;
    return 0;
}

GPResult gpiStatusInfoGetKey(GPConnection *connection, DArray keys, const char *keyName, char **keyValue) {
    GPIKey key;
    int index;

    if (keyName == NULL) {
        gpiSetErrorString(connection, "Invalid key name");
        return 2;
    }
    key.keyName = goastrdup(keyName);
    index = ArraySearch(keys, &key, gpiStatusInfoKeyCompFunc, 0, 1);
    if (index != -1) {
        *keyValue = ((GPIKey *)ArrayNth(keys, index))->keyValue;
    }
    gsifree(key.keyName);
    key.keyName = NULL;
    return 0;
}

// The length of a text of length bytes in base 64, padded to whole groups of 4
#define GPI_B64_LENGTH(length) (((length) % 3) ? 4 - ((length) % 3) + (length) * 4 / 3 : (length) * 4 / 3)

GPResult gpiSaveKeysToBuffer(GPConnection *connection, char **buffer) {
    GPIConnection *iconnection = *connection;
    int len = 0;
    int num;
    int i;
    GPIKey *key;
    char *name;
    char *value;
    int nameLength;
    char keysHeader[0x40];
    char *pos;

    num = ArrayLength(iconnection->extendedInfoKeys);
    OS_SPrintf(keysHeader, "\\keys\\%d", num);
    for (i = 0; i < num; i++) {
        key = ArrayNth(iconnection->extendedInfoKeys, i);
        name = key->keyName;
        nameLength = GPI_B64_LENGTH((gsi_u32)strlen(name));
        value = key->keyValue;
        len += nameLength + 2 + GPI_B64_LENGTH((gsi_u32)strlen(value));
    }
    *buffer = gsimalloc(len + strlen(keysHeader) + 1);
    if (*buffer == NULL) {
        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Memory, GSIDebugLevel_HotError,
                      "gpiSaveKeysToBuffer: buffer Out of memory.");
        gpiSetErrorString(connection, "Out of memory.");
        return 1;
    }
    pos = *buffer + OS_SPrintf(*buffer, "%s", keysHeader);
    for (i = 0; i < num; i++) {
        key = ArrayNth(iconnection->extendedInfoKeys, i);
        strcat(pos, "\\");
        pos++;
        name = key->keyName;
        B64Encode(name, pos, strlen(name), 2);
        name = key->keyName;
        pos += GPI_B64_LENGTH((gsi_u32)strlen(name));
        strcat(pos, "\\");
        pos++;
        value = key->keyValue;
        B64Encode(value, pos, strlen(value), 2);
        value = key->keyValue;
        pos += GPI_B64_LENGTH((gsi_u32)strlen(value));
    }
    return 0;
}
