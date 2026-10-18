// clang-format off
// MSL's <string.h>, before dwc/ext.h's declarations: the ROM builds gpiFailedOpCallback's small memsets in place, as
// MWCC does only for the memset that <string.h> declares (clang-format would sort it after dwc/ext.h)
#include <string.h>
#include "dwc/gpiOperation.h"
#include "dwc/ext.h"
#include "dwc/gpiCallback.h"
#include "dwc/gpiConnect.h"
#include "dwc/gpiInfo.h"
#include "dwc/gpiProfile.h"
#include "dwc/gpiUnique.h"
#include "dwc/gpiUtility.h"

// clang-format on

// GameSpy GP's operations in progress (gpiOperation.c). The functions' names, from pret/pokerevo's Wii build, are in
// dwc/gp.h

static void gpiDestroyOperation(GPConnection *connection, GPIOperation *operation);

int gpiFailedOpCallback(GPConnection *connection, const GPIOperation *operation) {
    GPIConnection *iconnection = *connection;
    GPICallback callback = operation->callback;
    int result;

    if (callback.callback != NULL) {
        switch (operation->type) {
        case GPI_CONNECT: {
            GPConnectResponseArg *arg = gsimalloc(sizeof(GPConnectResponseArg));

            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            memset(arg, 0, sizeof(GPConnectResponseArg));
            arg->result = operation->result;
            // The error of a nick that is in use gives the profile that has it
            if (iconnection->errorCode == 0x201) {
                arg->profile = iconnection->profileid;
                iconnection->profileid = 0;
            }
            result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
            if (result != 0) {
                return result;
            }
            break;
        }
        case GPI_NEW_PROFILE: {
            GPNewProfileResponseArg *arg = gsimalloc(sizeof(GPNewProfileResponseArg));

            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            memset(arg, 0, sizeof(GPNewProfileResponseArg));
            arg->result = operation->result;
            result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
            if (result != 0) {
                return result;
            }
            break;
        }
        case GPI_DELETE_PROFILE: {
            GPDeleteProfileResponseArg *arg = gsimalloc(sizeof(GPDeleteProfileResponseArg));

            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            memset(arg, 0, sizeof(GPDeleteProfileResponseArg));
            arg->result = operation->result;
            result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
            if (result != 0) {
                return result;
            }
            break;
        }
        case GPI_GET_INFO: {
            GPGetInfoResponseArg *arg = gsimalloc(sizeof(GPGetInfoResponseArg));

            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            memset(arg, 0, sizeof(GPGetInfoResponseArg));
            arg->result = operation->result;
            result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
            if (result != 0) {
                return result;
            }
            break;
        }
        case GPI_PROFILE_SEARCH: {
            GPProfileSearchResponseArg *arg = gsimalloc(sizeof(GPProfileSearchResponseArg));

            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            memset(arg, 0, sizeof(GPProfileSearchResponseArg));
            arg->result = operation->result;
            arg->matches = NULL;
            result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
            if (result != 0) {
                return result;
            }
            break;
        }
        case GPI_REGISTER_UNIQUENICK: {
            GPRegisterUniqueNickResponseArg *arg = gsimalloc(sizeof(GPRegisterUniqueNickResponseArg));

            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            memset(arg, 0, sizeof(GPRegisterUniqueNickResponseArg));
            arg->result = operation->result;
            result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
            if (result != 0) {
                return result;
            }
            break;
        }
        case GPI_REGISTER_CDKEY: {
            GPRegisterCdKeyResponseArg *arg = gsimalloc(sizeof(GPRegisterCdKeyResponseArg));

            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            memset(arg, 0, sizeof(GPRegisterCdKeyResponseArg));
            arg->result = operation->result;
            result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
            if (result != 0) {
                return result;
            }
            break;
        }
        }
    }
    return 0;
}

int gpiAddOperation(GPConnection *connection, int type, void *data, GPIOperation **op, BOOL blocking,
                    GPCallback callback, void *param) {
    GPIConnection *iconnection = *connection;
    GPIOperation *operation = gsimalloc(sizeof(GPIOperation));

    if (operation == NULL) {
        gpiSetErrorString(connection, "Out of memory.");
        return 1;
    }
    operation->type = type;
    operation->data = data;
    operation->blocking = blocking;
    operation->state = GPI_START;
    // The connect operation always has the id 1, which the login messages carry
    if (type == GPI_CONNECT) {
        operation->id = 1;
    } else {
        operation->id = iconnection->nextOperationID++;
        if (iconnection->nextOperationID < 2) {
            iconnection->nextOperationID = 2;
        }
    }
    operation->result = 0;
    operation->callback.callback = callback;
    operation->callback.param = param;
    operation->pnext = iconnection->operationList;
    iconnection->operationList = operation;
    *op = operation;
    return 0;
}

static void gpiDestroyOperation(GPConnection *connection, GPIOperation *operation) {
    GPIConnection *iconnection = *connection;

    if (operation->type == GPI_PROFILE_SEARCH) {
        GPISearchData *data = operation->data;

        iconnection->numSearches--;
        shutdown(data->sock, 2);
        closesocket(data->sock);
        gsifree(data->outputBuffer.buffer);
        data->outputBuffer.buffer = NULL;
        gsifree(data->inputBuffer.buffer);
        data->inputBuffer.buffer = NULL;
    }
    gsifree(operation->data);
    operation->data = NULL;
    gsifree(operation);
}

void gpiRemoveOperation(GPConnection *connection, GPIOperation *operation) {
    GPIConnection *iconnection = *connection;
    GPIOperation *prevOp = NULL;
    GPIOperation *op = iconnection->operationList;

    while (op != NULL) {
        if (op == operation) {
            if (prevOp == NULL) {
                iconnection->operationList = op->pnext;
            } else {
                prevOp->pnext = operation->pnext;
            }
            gpiDestroyOperation(connection, operation);
            return;
        }
        prevOp = op;
        op = op->pnext;
    }
}

BOOL gpiFindOperationByID(const GPConnection *connection, GPIOperation **operation, int id) {
    GPIConnection *iconnection = *connection;
    GPIOperation *op = iconnection->operationList;

    while (op != NULL) {
        if (op->id == id) {
            if (operation != NULL) {
                *operation = op;
            }
            return TRUE;
        }
        op = op->pnext;
    }
    if (operation != NULL) {
        *operation = NULL;
    }
    return FALSE;
}

// Searches don't block the connection manager's loop: gpiProcessSearches waits on them
BOOL gpiOperationsAreBlocking(const GPConnection *connection) {
    GPIConnection *iconnection = *connection;
    GPIOperation *operation = iconnection->operationList;

    while (operation != NULL) {
        if (operation->blocking && operation->type != GPI_PROFILE_SEARCH) {
            return TRUE;
        }
        operation = operation->pnext;
    }
    return FALSE;
}

int gpiProcessOperation(GPConnection *connection, GPIOperation *operation, const char *input) {
    int result = 0;

    switch (operation->type) {
    case GPI_CONNECT:
        result = gpiProcessConnect(connection, operation, input);
        break;
    case GPI_NEW_PROFILE:
        result = gpiProcessNewProfile(connection, operation, input);
        break;
    case GPI_DELETE_PROFILE:
        result = gpiProcessDeleteProfle(connection, operation, input);
        break;
    case GPI_GET_INFO:
        result = gpiProcessGetInfo(connection, operation, input);
        break;
    case GPI_REGISTER_UNIQUENICK:
        result = gpiProcessRegisterUniqueNick(connection, operation, input);
        break;
    case GPI_REGISTER_CDKEY:
        result = gpiProcessRegisterCdKey(connection, operation, input);
        break;
    default:
        gsDebugFormat(1, 4, 1, "gpiProcessOperation was passed an operation with an invalid type (%d)\n",
                      operation->type);
        break;
    }
    if (result != 0) {
        operation->result = result;
    }
    return result;
}
