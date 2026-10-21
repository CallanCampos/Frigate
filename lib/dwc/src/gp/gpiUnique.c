#include "dwc/gpiUnique.h"
#include "dwc/ext.h"
#include "dwc/gpiCallback.h"
#include "dwc/gpiOperation.h"
#include "dwc/gpiUtility.h"

// GameSpy GP's answers to the registrations of a unique nick and of a CD key (gpiUnique.c). The functions' names, from
// pret/pokerevo's Wii build and GameSpy's later SDK, are in dwc/gp.h

// The server's "\rn\" answer
int gpiProcessRegisterUniqueNick(GPConnection *connection, GPIOperation *operation, const char *input) {
    GPICallback callback;
    GPRegisterUniqueNickResponseArg *arg;
    int result;

    if (gpiCheckForError(connection, input, gsi_true)) {
        return 4;
    }
    if (strncmp(input, "\\rn\\", 4) != 0) {
        gpiSetError(connection, 1, "Unexpected data was received from the server.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    callback = operation->callback;
    if (callback.callback != NULL) {
        arg = gsimalloc(sizeof(GPRegisterUniqueNickResponseArg));
        if (arg == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
        arg->result = 0;
        result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
        if (result != 0) {
            return result;
        }
    }
    gpiRemoveOperation(connection, operation);
    return 0;
}

// The server's "\rc\" answer
int gpiProcessRegisterCdKey(GPConnection *connection, GPIOperation *operation, const char *input) {
    GPICallback callback;
    GPRegisterCdKeyResponseArg *arg;
    int result;

    if (gpiCheckForError(connection, input, gsi_true)) {
        return 4;
    }
    if (strncmp(input, "\\rc\\", 4) != 0) {
        gpiSetError(connection, 1, "Unexpected data was received from the server.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    callback = operation->callback;
    if (callback.callback != NULL) {
        arg = gsimalloc(sizeof(GPRegisterCdKeyResponseArg));
        if (arg == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
        arg->result = 0;
        result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
        if (result != 0) {
            return result;
        }
    }
    gpiRemoveOperation(connection, operation);
    return 0;
}
