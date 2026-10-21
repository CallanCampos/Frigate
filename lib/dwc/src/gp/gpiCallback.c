#include "dwc/gpiCallback.h"
#include "dwc/ext.h"
#include "dwc/gpiBuddy.h"
#include "dwc/gpiUtility.h"

// GameSpy GP's callbacks that wait to be called (gpiCallback.c). The functions' names, from pret/pokerevo's Wii build,
// are in dwc/gp.h

static void gpiCallCallback(GPConnection *connection, GPICallbackData *data);

void gpiCallErrorCallback(GPConnection *connection, int result, int fatal) {
    GPIConnection *iconnection = *connection;
    GPICallback callback;
    GPErrorArg *arg;

    if (fatal == 1) {
        iconnection->fatalError = gsi_true;
    }
    callback = iconnection->callbacks[0];
    if (callback.callback != NULL) {
        arg = gsimalloc(sizeof(GPErrorArg));
        if (arg != NULL) {
            arg->result = result;
            arg->fatal = fatal;
            arg->errorCode = iconnection->errorCode;
            arg->errorString = iconnection->errorString;
        }
        gpiAddCallback(connection, callback, arg, NULL, GPI_ADD_ERROR);
    }
}

int gpiAddCallback(GPConnection *connection, GPICallback callback, void *arg, const GPIOperation *operation, int type) {
    GPIConnection *iconnection = *connection;
    GPICallbackData *data = gsimalloc(sizeof(GPICallbackData));

    if (data == NULL) {
        gpiSetErrorString(connection, "Out of memory.");
        return 1;
    }
    data->callback = callback;
    data->arg = arg;
    data->operationID = operation != NULL ? operation->id : 0;
    data->type = type;
    data->pnext = NULL;
    if (iconnection->callbackList == NULL) {
        iconnection->callbackList = data;
    }
    if (iconnection->lastCallback != NULL) {
        iconnection->lastCallback->pnext = data;
    }
    iconnection->lastCallback = data;
    return 0;
}

// Calls a callback, then frees its argument with what the argument holds, by its kind
static void gpiCallCallback(GPConnection *connection, GPICallbackData *data) {
    int type;

    data->callback.callback(connection, data->arg, data->callback.param);
    type = data->type;
    if (type == GPI_ADD_MESSAGE) {
        gsifree(((GPRecvBuddyMessageArg *)data->arg)->message);
        ((GPRecvBuddyMessageArg *)data->arg)->message = NULL;
    } else if (type == GPI_ADD_BUDDYUTM) {
        gsifree(((GPRecvBuddyUTMArg *)data->arg)->message);
        ((GPRecvBuddyUTMArg *)data->arg)->message = NULL;
    } else if (type == GPI_ADD_NICKS) {
        GPGetUserNicksResponseArg *arg = data->arg;
        int i;

        for (i = 0; i < arg->numNicks; i++) {
            gsifree(arg->nicks[i]);
            arg->nicks[i] = NULL;
            gsifree(arg->uniquenicks[i]);
            arg->uniquenicks[i] = NULL;
        }
        gsifree(arg->nicks);
        arg->nicks = NULL;
        gsifree(arg->uniquenicks);
        arg->uniquenicks = NULL;
    } else if (type == GPI_ADD_PMATCH) {
        GPFindPlayersResponseArg *arg = data->arg;

        gsifree(arg->matches);
        arg->matches = NULL;
    } else if (type == GPI_ADD_TRANSFER_CALLBACK) {
        GPTransferCallbackArg *arg = data->arg;

        if (arg->message != NULL) {
            gsifree(arg->message);
            arg->message = NULL;
        }
    } else if (type == GPI_ADD_REVERSE_BUDDIES) {
        GPGetReverseBuddiesResponseArg *arg = data->arg;

        if (arg->profiles != NULL) {
            gsifree(arg->profiles);
            arg->profiles = NULL;
        }
    } else if (type == GPI_ADD_SUGGESTED_UNIQUE) {
        int i;
        GPSuggestUniqueNickResponseArg *arg = data->arg;

        for (i = 0; i < arg->numSuggestedNicks; i++) {
            gsifree(arg->suggestedNicks[i]);
            arg->suggestedNicks[i] = NULL;
        }
        gsifree(arg->suggestedNicks);
        arg->suggestedNicks = NULL;
    } else if (type == GPI_ADD_BUDDYREVOKE) {
        // The buddy that revoked is deleted once the app knows, without telling the server
        gpiDeleteBuddy(connection, ((GPRecvBuddyRevokeArg *)data->arg)->profile, gsi_false);
    } else if (type == GPI_ADD_REVERSE_BUDDIES_LIST) {
        GPGetReverseBuddiesListResponseArg *arg = data->arg;

        if (arg->matches != NULL) {
            gsifree(arg->matches);
            arg->matches = NULL;
        }
    } else if (type == GPI_ADD_BUDDYKEYS) {
        GPGetBuddyStatusInfoKeysArg *arg = data->arg;
        int i;

        if (arg->numKeys != 0) {
            for (i = 0; i < arg->numKeys; i++) {
                gsifree(arg->keys[i]);
                arg->keys[i] = NULL;
                gsifree(arg->values[i]);
                arg->values[i] = NULL;
            }
            gsifree(arg->keys);
            arg->keys = NULL;
            gsifree(arg->values);
            arg->values = NULL;
        }
    } else if (type == GPI_ADD_PROFILE_LIST) {
        GPGetProfileListResponseArg *arg = data->arg;

        if (arg->profiles != NULL) {
            gsifree(arg->profiles);
            arg->profiles = NULL;
        }
    }
    gsifree(data->arg);
    data->arg = NULL;
    gsifree(data);
}

int gpiProcessCallbacks(GPConnection *connection, int blockingOperationID) {
    GPIConnection *iconnection = *connection;
    GPICallbackData *list;
    GPICallbackData *last;
    GPICallbackData *prev;
    GPICallbackData *curr;
    GPICallbackData *next;

    if (blockingOperationID != 0) {
        // Only the callbacks of that operation and the errors are called; the others go back on the list, behind the
        // ones that the calls added
        prev = NULL;
        list = iconnection->callbackList;
        last = iconnection->lastCallback;
        iconnection->callbackList = NULL;
        iconnection->lastCallback = NULL;
        for (curr = list; curr != NULL; curr = next) {
            next = curr->pnext;
            if (curr->operationID == blockingOperationID || curr->type == GPI_ADD_ERROR) {
                if (prev != NULL) {
                    prev->pnext = next;
                } else {
                    list = next;
                }
                if (last == curr) {
                    last = prev;
                }
                gpiCallCallback(connection, curr);
            } else {
                prev = curr;
            }
        }
        if (iconnection->callbackList != NULL) {
            iconnection->lastCallback->pnext = list;
            iconnection->lastCallback = last;
        } else {
            iconnection->callbackList = list;
            iconnection->lastCallback = last;
        }
        return 0;
    }
    // Every callback is called, and the ones that the calls add after them
    while (iconnection->callbackList != NULL) {
        curr = iconnection->callbackList;
        iconnection->callbackList = NULL;
        iconnection->lastCallback = NULL;
        while (curr != NULL) {
            next = curr->pnext;
            gpiCallCallback(connection, curr);
            curr = next;
        }
    }
    return 0;
}
