#include "dwc/gpi.h"
#include "dwc/ext.h"
#include "dwc/gpiBuddy.h"
#include "dwc/gpiBuffer.h"
#include "dwc/gpiCallback.h"
#include "dwc/gpiConnect.h"
#include "dwc/gpiInfo.h"
#include "dwc/gpiKeys.h"
#include "dwc/gpiOperation.h"
#include "dwc/gpiPeer.h"
#include "dwc/gpiProfile.h"
#include "dwc/gpiSearch.h"
#include "dwc/gpiUtility.h"

// GameSpy GP's connection and its main loop (gpi.c). The functions' names, from pret/pokerevo's Wii build (the later
// SDK's for gpiInitialize and gpiDestroy), are in dwc/gp.h

static BOOL gpiResetProfile(GPConnection *connection, GPIProfile *profile, void *data);

int gpiInitialize(GPConnection *connection, int productID, int namespaceID, int partnerID) {
    GPIConnection *iconnection;
    int i;
    int result;

    *connection = NULL;
    iconnection = gsimalloc(sizeof(GPIConnection));
    if (iconnection == NULL) {
        return 1;
    }
    memset(iconnection, 0, sizeof(GPIConnection));
    iconnection->errorString[0] = '\0';
    iconnection->errorCode = 0;
    iconnection->infoCaching = TRUE;
    iconnection->infoCachingBuddyAndBlockOnly = FALSE;
    iconnection->simulation = FALSE;
    iconnection->firewall = FALSE;
    iconnection->productID = productID;
    iconnection->namespaceID = namespaceID;
    iconnection->partnerID = partnerID;
    if (!gpiInitProfiles((GPConnection *)&iconnection)) {
        gsifree(iconnection);
        return 1;
    }
    iconnection->diskCache = NULL;
    for (i = 0; i < GPI_NUM_CALLBACKS; i++) {
        iconnection->callbacks[i].callback = NULL;
        iconnection->callbacks[i].param = NULL;
    }
    result = gpiReset((GPConnection *)&iconnection);
    if (result != 0) {
        gpiDestroy((GPConnection *)&iconnection);
        return result;
    }
    SocketStartUp();
    srand(current_time());
    *connection = iconnection;
    return 0;
}

void gpiDestroy(GPConnection *connection) {
    GPIConnection *iconnection = *connection;

    gpiDisconnect(connection, TRUE);
    gpiStatusInfoKeysDestroy(connection);
    func_ov011_0217187c(iconnection->profileList.profileTable);
    gsifree(iconnection);
    *connection = NULL;
}

// Forgets what a profile knew of the last login, for gpiReset
static BOOL gpiResetProfile(GPConnection *connection, GPIProfile *profile, void *data) {
    profile->buddyStatus = NULL;
    profile->buddyStatusInfo = NULL;
    profile->authSig = NULL;
    profile->requestCount = 0;
    profile->peerSig = NULL;
    profile->blocked = FALSE;
    profile->buddyOrBlockCache = FALSE;
    return TRUE;
}

int gpiReset(GPConnection *connection) {
    GPIConnection *iconnection = *connection;
    int result;

    iconnection->nick[0] = '\0';
    iconnection->uniquenick[0] = '\0';
    iconnection->email[0] = '\0';
    iconnection->cmSocket = -1;
    iconnection->connectState = GPI_NOT_CONNECTED;
    // The original clears each buffer's pointer twice after freeing it
    iconnection->socketBuffer.len = 0;
    iconnection->socketBuffer.pos = 0;
    iconnection->socketBuffer.size = 0;
    gsifree(iconnection->socketBuffer.buffer);
    iconnection->socketBuffer.buffer = NULL;
    iconnection->socketBuffer.buffer = NULL;
    iconnection->inputBufferSize = 0;
    gsifree(iconnection->inputBuffer);
    iconnection->inputBuffer = NULL;
    iconnection->inputBuffer = NULL;
    iconnection->outputBuffer.len = 0;
    iconnection->outputBuffer.pos = 0;
    iconnection->outputBuffer.size = 0;
    gsifree(iconnection->outputBuffer.buffer);
    iconnection->outputBuffer.buffer = NULL;
    iconnection->outputBuffer.buffer = NULL;
    iconnection->updateproBuffer.len = 0;
    iconnection->updateproBuffer.pos = 0;
    iconnection->updateproBuffer.size = 0;
    gsifree(iconnection->updateproBuffer.buffer);
    iconnection->updateproBuffer.buffer = NULL;
    iconnection->updateproBuffer.buffer = NULL;
    iconnection->updateuiBuffer.len = 0;
    iconnection->updateuiBuffer.pos = 0;
    iconnection->updateuiBuffer.size = 0;
    gsifree(iconnection->updateuiBuffer.buffer);
    iconnection->updateuiBuffer.buffer = NULL;
    iconnection->updateuiBuffer.buffer = NULL;
    gpiStatusInfoKeysDestroy(connection);
    result = gpiStatusInfoKeysInit((GPConnection *)&iconnection);
    if (result != 0) {
        gpiDestroy((GPConnection *)&iconnection);
        return result;
    }
    iconnection->nextOperationID = 2;
    while (iconnection->operationList != NULL) {
        gpiRemoveOperation(connection, iconnection->operationList);
    }
    iconnection->operationList = NULL;
    iconnection->profileList.numBuddies = 0;
    iconnection->profileList.numBlocked = 0;
    gpiProfileMap(connection, gpiResetProfile, NULL);
    iconnection->userid = 0;
    iconnection->profileid = 0;
    iconnection->sessKey = 0;
    iconnection->numSearches = 0;
    iconnection->fatalError = FALSE;
    iconnection->peerList = NULL;
    iconnection->lastStatusState = -1;
    iconnection->lastStatusString[0] = '\0';
    iconnection->lastLocationString[0] = '\0';
    iconnection->kaTransmit = 0;
    return 0;
}

int gpiProcessConnectionManager(GPConnection *connection) {
    GPIConnection *iconnection;
    GPIOperation *operation;
    int len;
    BOOL connClosed;
    int result;
    BOOL loop;
    char *next;
    char *input;
    char *str;
    u32 now;

    connClosed = FALSE;
    iconnection = *connection;
    now = current_time();
    do {
        gpiAddLocalInfo(connection, &iconnection->outputBuffer);
        if (iconnection->outputBuffer.len > 0) {
            iconnection->kaTransmit = now;
        }
        result =
            gpiSendFromBuffer(connection, iconnection->cmSocket, &iconnection->outputBuffer, &connClosed, TRUE, "CM");
        if (result != 0) {
            return result;
        }
        result =
            gpiRecvToBuffer(connection, iconnection->cmSocket, &iconnection->socketBuffer, &len, &connClosed, "CM");
        if (result != 0) {
            if (result != 3) {
                return result;
            }
            gpiSetError(connection, 5, "There was an error reading from the server.");
            gpiCallErrorCallback(connection, 3, 1);
            return 3;
        }
        // Each message ends with "\final\": it is copied out of the socket's buffer and handled
        while ((next = strstr(iconnection->socketBuffer.buffer, "\\final\\")) != NULL) {
            iconnection->kaTransmit = now;
            *next = '\0';
            gsDebugFormat(1, 0, 0x20, "CMD: %s\n", iconnection->socketBuffer.buffer);
            len = next - iconnection->socketBuffer.buffer;
            if (len > iconnection->inputBufferSize) {
                int increase = len;
                void *tempPtr;

                if (len < 0x4000) {
                    increase = 0x4000;
                }
                iconnection->inputBufferSize += increase;
                tempPtr = gsirealloc(iconnection->inputBuffer, iconnection->inputBufferSize + 1);
                if (tempPtr == NULL) {
                    gpiSetErrorString(connection, "Out of memory.");
                    return 1;
                }
                iconnection->inputBuffer = tempPtr;
            }
            memcpy(iconnection->inputBuffer, iconnection->socketBuffer.buffer, len + 1);
            iconnection->socketBuffer.len -= (next + 7) - iconnection->socketBuffer.buffer;
            memmove(iconnection->socketBuffer.buffer, next + 7, iconnection->socketBuffer.len + 1);
            input = iconnection->inputBuffer;
            // A message with an id answers an operation; the others come from the server on their own
            str = strstr(input, "\\id\\");
            if (str != NULL) {
                int id = atol(str + 4);

                if (!gpiFindOperationByID(connection, &operation, id)) {
                    gsDebugFormat(1, 0, 1, "No matching operation found for id %d\n", id);
                } else {
                    result = gpiProcessOperation(connection, operation, iconnection->inputBuffer);
                    if (result != 0) {
                        return result;
                    }
                }
            } else {
                if (gpiCheckForError(connection, input, TRUE)) {
                    return 4;
                }
                input = iconnection->inputBuffer;
                if (strncmp(input, "\\bm\\", 4) == 0) {
                    result = gpiProcessRecvBuddyMessage(connection, input);
                    if (result != 0) {
                        return result;
                    }
                } else if (strncmp(input, "\\ka\\", 4) != 0) {
                    if (strncmp(input, "\\lt\\", 4) == 0) {
                        gpiValueForKey(input, "\\lt\\", iconnection->loginTicket.ticket, 25);
                    } else if (strncmp(input, "\\bsi\\", 5) == 0) {
                        result = gpiProcessRecvBuddyStatusInfo(connection, input);
                        if (result != 0) {
                            return result;
                        }
                    } else if (strncmp(input, "\\bdy\\", 5) == 0) {
                        result = gpiProcessRecvBuddyList(connection, input);
                        if (result != 0) {
                            return result;
                        }
                    } else if (strncmp(input, "\\blk\\", 5) == 0) {
                        result = gpiProcessRecvBlockedList(connection, input);
                        if (result != 0) {
                            return result;
                        }
                    } else if (strncmp(input, "\\rar\\", 5) == 0) {
                        result = gpiProcessRemoteAuthReply(connection, input);
                        if (result != 0) {
                            return result;
                        }
                    } else {
                        gsDebugFormat(1, 0, 1, "Received an unrecognized message.\n");
                    }
                }
            }
        }
        if (connClosed && iconnection->connectState != GPI_PROFILE_DELETING) {
            gpiSetError(connection, 7, "The server has closed the connection.");
            gpiCallErrorCallback(connection, 3, 1);
            return 0;
        }
        loop = gpiOperationsAreBlocking(connection);
        if (loop) {
            msleep(10);
        }
    } while (loop);
    // A keep-alive after two minutes without a message
    if (now - iconnection->kaTransmit > 120000) {
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\ka\\\\final\\");
        iconnection->kaTransmit = now;
    }
    return 0;
}

int gpiProcess(GPConnection *connection, int blockingOperationID) {
    GPIConnection *iconnection = *connection;
    GPIOperation *operation;
    GPIOperation *delOperation;
    int result = 0;
    BOOL loop;
    int callbackResult;

    if (iconnection->connectState == GPI_CONNECTING) {
        do {
            result = gpiCheckConnect(connection);
            loop = result == 0 && blockingOperationID != 0 && iconnection->connectState == GPI_CONNECTING;
            if (loop) {
                msleep(10);
            }
        } while (loop);
        // A connect that failed ends its operation with a server error
        if (result != 0 && gpiFindOperationByID(connection, &operation, 1)) {
            operation->result = 4;
        }
    }
    if (iconnection->connectState == GPI_CONNECTED || iconnection->connectState == GPI_NEGOTIATING ||
        iconnection->connectState == GPI_PROFILE_DELETING) {
        if (result == 0) {
            result = gpiProcessConnectionManager(connection);
        }
        if (result == 0) {
            result = gpiProcessPeers(connection);
        }
    }
    if (result == 0) {
        result = gpiProcessSearches(connection);
    }
    // The operations that failed call back and are removed
    operation = iconnection->operationList;
    while (operation != NULL) {
        if (operation->result != 0) {
            gpiFailedOpCallback(connection, operation);
            delOperation = operation;
            operation = operation->pnext;
            gpiRemoveOperation(connection, delOperation);
        } else {
            operation = operation->pnext;
        }
    }
    callbackResult = gpiProcessCallbacks(connection, blockingOperationID);
    if (callbackResult != 0) {
        return callbackResult;
    }
    if (iconnection->fatalError) {
        gpiDisconnect(connection, FALSE);
        gpiReset(connection);
    }
    return result;
}
