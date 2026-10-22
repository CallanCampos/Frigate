#include "dwc/gpiUtility.h"
#include "dwc/ext.h"
#include "dwc/gpiCallback.h"

// GameSpy GP's helpers (gpiUtility.c). The functions' names, from pret/pokerevo's Wii build, are in dwc/gp.h

void strzcpy(char *dest, const char *src, size_t len) {
    strncpy(dest, src, len);
    *(dest + len - 1) = '\0';
}

GPIBool gpiCheckForError(GPConnection *connection, const char *input, GPIBool callErrorCallback) {
    char buffer[16];
    GPIConnection *iconnection = *connection;

    if (strncmp(input, "\\error\\", 7) == 0) {
        if (gpiValueForKey(input, "\\err\\", buffer, 16)) {
            iconnection->errorCode = atol(buffer);
        }
        if (!gpiValueForKey(input, "\\errmsg\\", iconnection->errorString, 256)) {
            iconnection->errorString[0] = '\0';
        }
        if (callErrorCallback) {
            int fatal = strstr(input, "\\fatal\\") != NULL ? 1 : 0;
            gpiCallErrorCallback(connection, 4, fatal ? 1 : 0);
        }
        return gsi_true;
    }
    return gsi_false;
}

GPIBool gpiValueForKeyWithIndex(const char *command, const char *key, int *index, char *value, int len) {
    const char *start;
    const char *found;
    char delimiter = key[0];
    int c;
    int i = 0;

    start = command += *index;
    found = strstr(command, key);
    if (found == NULL) {
        return gsi_false;
    }
    found += strlen(key);
    len--;
    while (i < len && (c = found[i]) != '\0' && c != delimiter) {
        value[i] = c;
        i++;
    }
    value[i] = '\0';
    *index += (found - start) + strlen(value);
    return gsi_true;
}

GPIBool gpiValueForKey(const char *command, const char *key, char *value, int len) {
    const char *found;
    char delimiter = key[0];
    int c;
    int i = 0;

    found = strstr(command, key);
    if (found == NULL) {
        return gsi_false;
    }
    found += strlen(key);
    len--;
    while (i < len && (c = found[i]) != '\0' && c != delimiter) {
        value[i] = c;
        i++;
    }
    value[i] = '\0';
    return gsi_true;
}

GPResult gpiCheckSocketConnect(GPConnection *connection, SOCKET sock, int *state) {
    int writeFlag = 0;
    int exceptFlag = 0;
    int result = GSISocketSelect(sock, NULL, &writeFlag, &exceptFlag);

    if (result == -1) {
        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_HotError, "Error connecting\n");
        gpiSetError(connection, 5, "There was an error checking for a completed connection.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    if (result > 0) {
        if (exceptFlag) {
            gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_HotError, "Connection rejected\n");
            *state = 4;
            return 0;
        }
        if (writeFlag) {
            gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_Notice, "Connection accepted\n");
            *state = 3;
            return 0;
        }
    }
    *state = 0;
    return 0;
}

// What a message that can't be parsed gives
#define GPI_PARSE_ERROR(connection)                                                                                    \
    do {                                                                                                               \
        gpiSetError(connection, 1, "Parse Error.");                                                                    \
        gpiCallErrorCallback(connection, 3, 1);                                                                        \
        return 3;                                                                                                      \
    } while (0)

GPResult gpiReadKeyAndValue(GPConnection *connection, const char *buffer, int *index, char key[512], char value[512]) {
    const char *start;
    int c;
    int i;

    start = buffer + *index;
    if (*start != '\\') {
        GPI_PARSE_ERROR(connection);
    }
    buffer = start + 1;
    i = 0;
    while ((c = *buffer++) != '\\') {
        if (c == '\0') {
            GPI_PARSE_ERROR(connection);
        }
        if (i == 511) {
            GPI_PARSE_ERROR(connection);
        }
        *key++ = c;
        i++;
    }
    *key = '\0';
    i = 0;
    while ((c = *buffer++) != '\\' && c != '\0') {
        if (i == 511) {
            GPI_PARSE_ERROR(connection);
        }
        *value++ = c;
        i++;
    }
    *value = '\0';
    *index += (buffer - start) - 1;
    return 0;
}

void gpiSetError(GPConnection *connection, GPErrorCode errorCode, const char *errorString) {
    GPIConnection *iconnection = *connection;

    strzcpy(iconnection->errorString, errorString, 256);
    iconnection->errorCode = errorCode;
}

void gpiSetErrorString(GPConnection *connection, const char *errorString) {
    strzcpy(((GPIConnection *)*connection)->errorString, errorString, 256);
}

void gpiEncodeString(const char *unencodedString, char *encodedString) {
    char scrambled[32];
    gsi_u32 i;
    gsi_u32 length;
    char key;

    length = strlen(unencodedString);
    Util_RandSeed(0x79707367);
    for (i = 0; i < length; i++) {
        key = Util_RandInt(0, 0xff);
        scrambled[i] = key ^ unencodedString[i];
    }
    scrambled[i] = '\0';
    B64Encode(scrambled, encodedString, length, 1);
}
