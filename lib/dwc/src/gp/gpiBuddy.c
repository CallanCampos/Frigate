#include <string.h>
// Before dwc/ext.h: MWCC builds the memsets of the buddy statuses in place, as the ROM does, only when <string.h>
// declares memset first
#include "dwc/darray.h"
#include "dwc/ext.h"
#include "dwc/gpiBuddy.h"
#include "dwc/gpiBuffer.h"
#include "dwc/gpiCallback.h"
#include "dwc/gpiKeys.h"
#include "dwc/gpiPeer.h"
#include "dwc/gpiProfile.h"
#include "dwc/gpiUtility.h"

// GameSpy GP's buddy messages and buddy list (gpiBuddy.c). The functions' names, from pret/pokerevo's Wii build and
// GameSpy's later SDK, are in dwc/gp.h

static GPResult gpiSendAuthBuddyRequest(GPConnection *connection, GPIProfile *profile);
static GPIBool gpiFixBuddyIndices(GPConnection *connection, GPIProfile *profile, void *data);

// Swaps the bytes of an address or a port from the server's order to the network's
#define GPI_HTONL(x) (((x) << 24 & 0xff000000) | (((x) << 8 & 0xff0000) | ((gsi_u8)((x) >> 24) | ((x) >> 8 & 0xff00))))

static inline gsi_u16 gpiHtons(gsi_u16 x) {
    return (gsi_u16)((gsi_u8)(x >> 8) | (x << 8 & 0xff00));
}

// What a message that doesn't make sense gives
#define GPI_BAD_MESSAGE(connection)                                                                                    \
    do {                                                                                                               \
        gpiSetError(connection, 1, "Unexpected data was received from the server.");                                   \
        gpiCallErrorCallback(connection, 3, 1);                                                                        \
        return 3;                                                                                                      \
    } while (0)

static GPResult gpiSendAuthBuddyRequest(GPConnection *connection, GPIProfile *profile) {
    GPIConnection *iconnection = *connection;

    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\authadd\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\fromprofileid\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, profile->profileId);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sig\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, profile->authSig);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
    return 0;
}

GPResult gpiProcessRecvBuddyMessage(GPConnection *connection, const char *input) {
    GPIConnection *iconnection;
    GPIProfile *profile;
    GPICallback callback;
    char buffer[0x1000];
    char intValue[0x10];
    char strTemp[0x100];
    int type;
    int profileid;
    gsi_u32 date;
    int result;
    char *str;

    iconnection = *connection;
    if (!gpiValueForKey(input, "\\bm\\", buffer, 0x1000)) {
        GPI_BAD_MESSAGE(connection);
    }
    type = atol(buffer);
    if (!gpiValueForKey(input, "\\f\\", buffer, 0x1000)) {
        GPI_BAD_MESSAGE(connection);
    }
    profileid = atol(buffer);
    if (!gpiValueForKey(input, "\\date\\", buffer, 0x1000)) {
        date = gsiTimeInSec(NULL);
    } else {
        date = atol(buffer);
    }
    switch (type) {
    case GPI_BM_MESSAGE: {
        GPRecvBuddyMessageArg *arg;

        callback = iconnection->callbacks[3];
        if (callback.callback != NULL) {
            arg = gsimalloc(sizeof(GPRecvBuddyMessageArg));
            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            if (!gpiValueForKey(input, "\\msg\\", buffer, 0x1000)) {
                GPI_BAD_MESSAGE(connection);
            }
            arg->message = gsimalloc(strlen(buffer) + 1);
            if (arg->message == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            strcpy(arg->message, buffer);
            arg->profile = profileid;
            arg->date = date;
            result = gpiAddCallback(connection, callback, arg, NULL, GPI_ADD_MESSAGE);
            if (result != 0) {
                return result;
            }
        }
        break;
    }
    case GPI_BM_UTM: {
        GPRecvBuddyUTMArg *arg;

        callback = iconnection->callbacks[4];
        if (callback.callback != NULL) {
            arg = gsimalloc(sizeof(GPRecvBuddyUTMArg));
            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            if (!gpiValueForKey(input, "\\msg\\", buffer, 0x1000)) {
                GPI_BAD_MESSAGE(connection);
            }
            arg->message = gsimalloc(strlen(buffer) + 1);
            if (arg->message == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            strcpy(arg->message, buffer);
            arg->profile = profileid;
            arg->date = date;
            result = gpiAddCallback(connection, callback, arg, NULL, GPI_ADD_BUDDYUTM);
            if (result != 0) {
                return result;
            }
        }
        break;
    }
    case GPI_BM_REQUEST: {
        GPRecvBuddyRequestArg *arg;

        profile = gpiProfileListAdd(connection, profileid);
        if (profile == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
        if (!gpiValueForKey(input, "\\msg\\", buffer, 0x1000)) {
            GPI_BAD_MESSAGE(connection);
        }
        str = strstr(buffer, "|signed|");
        if (str == NULL) {
            GPI_BAD_MESSAGE(connection);
        }
        *str = '\0';
        if (strlen(str + 8) != 0x20) {
            GPI_BAD_MESSAGE(connection);
        }
        gsifree(profile->authSig);
        profile->authSig = NULL;
        profile->authSig = goastrdup(str + 8);
        profile->requestCount++;
        callback = iconnection->callbacks[1];
        if (callback.callback != NULL) {
            arg = gsimalloc(sizeof(GPRecvBuddyRequestArg));
            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            strzcpy(arg->reason, buffer, sizeof(arg->reason));
            arg->profile = profileid;
            arg->date = date;
            result = gpiAddCallback(connection, callback, arg, NULL, GPI_ADD_BUDDDYREQUEST);
            if (result != 0) {
                return result;
            }
        }
        break;
    }
    case GPI_BM_AUTH: {
        GPRecvBuddyAuthArg *arg;

        callback = iconnection->callbacks[7];
        if (callback.callback != NULL) {
            arg = gsimalloc(sizeof(GPRecvBuddyAuthArg));
            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            arg->profile = profileid;
            arg->date = date;
            result = gpiAddCallback(connection, callback, arg, NULL, GPI_ADD_BUDDYAUTH);
            if (result != 0) {
                return result;
            }
        }
        break;
    }
    case GPI_BM_REVOKE: {
        GPRecvBuddyRevokeArg *arg;

        callback = iconnection->callbacks[8];
        if (callback.callback != NULL) {
            arg = gsimalloc(sizeof(GPRecvBuddyRevokeArg));
            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            arg->profile = profileid;
            arg->date = date;
            result = gpiAddCallback(connection, callback, arg, NULL, GPI_ADD_BUDDYREVOKE);
            if (result != 0) {
                return result;
            }
        }
        break;
    }
    case GPI_BM_STATUS: {
        GPIBuddyStatus *buddyStatus;
        GPRecvBuddyStatusArg *arg;

        profile = gpiProfileListAdd(connection, profileid);
        if (profile == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
        if (iconnection->connectState == GPI_CONNECTED && profile->deleted) {
            gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_WarmError,
                          "Deliberately ignoring Buddy Message Status update from profile id [%i], "
                          "\t\t\t\tbecause could not be found in the local buddy list. It must have been "
                          "removed \n",
                          profileid);
        } else if (!profile->blocked) {
            if (profile->buddyStatus == NULL) {
                profile->buddyStatus = gsimalloc(sizeof(GPIBuddyStatus));
                if (profile->buddyStatus == NULL) {
                    gpiSetErrorString(connection, "Out of memory.");
                    return 1;
                }
                memset(profile->buddyStatus, 0, sizeof(GPIBuddyStatus));
                if (profile->buddyStatusInfo != NULL) {
                    profile->buddyStatus->buddyIndex = profile->buddyStatusInfo->buddyIndex;
                    gpiRemoveBuddyStatusInfo(profile->buddyStatusInfo);
                    profile->buddyStatusInfo = NULL;
                } else {
                    profile->buddyStatus->buddyIndex = iconnection->profileList.numBuddies++;
                }
            }
            buddyStatus = profile->buddyStatus;
            if (!gpiValueForKey(input, "\\msg\\", buffer, 0x1000)) {
                GPI_BAD_MESSAGE(connection);
            }
            if (!gpiValueForKey(buffer, "|s|", intValue, 0x10)) {
                GPI_BAD_MESSAGE(connection);
            }
            buddyStatus->status = atol(intValue);
            gsifree(buddyStatus->statusString);
            buddyStatus->statusString = NULL;
            if (!gpiValueForKey(buffer, "|ss|", strTemp, 0x100)) {
                strTemp[0] = '\0';
            }
            buddyStatus->statusString = goastrdup(strTemp);
            if (buddyStatus->statusString == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            gsifree(buddyStatus->locationString);
            buddyStatus->locationString = NULL;
            if (!gpiValueForKey(buffer, "|ls|", strTemp, 0x100)) {
                strTemp[0] = '\0';
            }
            buddyStatus->locationString = goastrdup(strTemp);
            if (buddyStatus->locationString == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            if (!gpiValueForKey(buffer, "|ip|", intValue, 0x10)) {
                buddyStatus->ip = 0;
            } else {
                buddyStatus->ip = GPI_HTONL((gsi_u32)atol(intValue));
            }
            if (!gpiValueForKey(buffer, "|p|", intValue, 0x10)) {
                buddyStatus->port = 0;
            } else {
                buddyStatus->port = gpiHtons(atol(intValue));
            }
            if (!gpiValueForKey(buffer, "|qm|", intValue, 0x10)) {
                buddyStatus->quietModeFlags = 0;
            } else {
                buddyStatus->quietModeFlags = atol(intValue);
            }
            callback = iconnection->callbacks[2];
            if (callback.callback != NULL) {
                arg = gsimalloc(sizeof(GPRecvBuddyStatusArg));
                if (arg == NULL) {
                    gpiSetErrorString(connection, "Out of memory.");
                    return 1;
                }
                arg->profile = profileid;
                arg->index = buddyStatus->buddyIndex;
                arg->date = date;
                result = gpiAddCallback(connection, callback, arg, NULL, GPI_ADD_STATUS);
                if (result != 0) {
                    return result;
                }
            }
        }
        break;
    }
    case GPI_BM_INVITE: {
        GPRecvGameInviteArg *arg;
        int productID;
        char *location;

        if (!gpiValueForKey(input, "\\msg\\", buffer, 0x1000)) {
            GPI_BAD_MESSAGE(connection);
        }
        str = strstr(buffer, "|p|");
        if (str == NULL) {
            GPI_BAD_MESSAGE(connection);
        }
        if (str[3] == '\0') {
            GPI_BAD_MESSAGE(connection);
        }
        productID = atol(str + 3);
        location = strstr(buffer, "|l|");
        if (location != NULL) {
            strzcpy(strTemp, location + 3, 0x100);
        } else {
            strTemp[0] = '\0';
        }
        callback = iconnection->callbacks[5];
        if (callback.callback != NULL) {
            arg = gsimalloc(sizeof(GPRecvGameInviteArg));
            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            arg->profile = profileid;
            arg->productID = productID;
            gsiSafeStrcpyA(arg->location, strTemp, sizeof(arg->location));
            result = gpiAddCallback(connection, callback, arg, NULL, GPI_ADD_NORMAL);
            if (result != 0) {
                return result;
            }
        }
        break;
    }
    case GPI_BM_PING:
        if (!gpiValueForKey(input, "\\msg\\", buffer, 0x1000)) {
            GPI_BAD_MESSAGE(connection);
        }
        gpiSendBuddyMessage(connection, profileid, GPI_BM_PONG, "1", 0, NULL);
        break;
    }
    return 0;
}

GPResult gpiProcessRecvBuddyStatusInfo(GPConnection *connection, const char *input) {
    GPIProfile *profile;
    GPIBuddyStatusInfo *buddyStatusInfo;
    GPICallback callback;
    GPRecvBuddyStatusArg *arg;
    char buffer[0x400];
    int profileid;
    gsi_u32 date;
    GPIConnection *iconnection;

    iconnection = *connection;
    date = gsiTimeInSec(NULL);
    if (!gpiValueForKey(input, "\\profile\\", buffer, 0x400)) {
        GPI_BAD_MESSAGE(connection);
    }
    profileid = atol(buffer);
    profile = gpiProfileListAdd(connection, profileid);
    if (profile == NULL) {
        gpiSetErrorString(connection, "Out of memory.");
        return 1;
    }
    if (!profile->blocked && !profile->deleted) {
        buddyStatusInfo = profile->buddyStatusInfo;
        if (buddyStatusInfo == NULL) {
            profile->buddyStatusInfo = gsimalloc(sizeof(GPIBuddyStatusInfo));
            if (profile->buddyStatusInfo == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            memset(profile->buddyStatusInfo, 0, sizeof(GPIBuddyStatusInfo));
            if (profile->buddyStatus != NULL) {
                profile->buddyStatusInfo->buddyIndex = profile->buddyStatus->buddyIndex;
                gpiRemoveBuddyStatus(profile->buddyStatus);
                profile->buddyStatus = NULL;
            } else {
                profile->buddyStatusInfo->buddyIndex = iconnection->profileList.numBuddies++;
            }
            profile->buddyStatusInfo->extendedInfoKeys = func_ov011_02171514(sizeof(GPIKey), 1, gpiStatusInfoKeyFree);
            buddyStatusInfo = profile->buddyStatusInfo;
            if (buddyStatusInfo->extendedInfoKeys == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
        }
        if (!gpiValueForKey(input, "\\state\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->statusState = atol(buffer);
        if (!gpiValueForKey(input, "\\bip\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->buddyIp = GPI_HTONL((gsi_u32)atol(buffer));
        if (!gpiValueForKey(input, "\\bport\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->buddyPort = atol(buffer);
        if (!gpiValueForKey(input, "\\hostip\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->hostIp = GPI_HTONL((gsi_u32)atol(buffer));
        if (!gpiValueForKey(input, "\\hprivip\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->hostPrivateIp = GPI_HTONL((gsi_u32)atol(buffer));
        if (!gpiValueForKey(input, "\\qport\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->queryPort = atol(buffer);
        if (!gpiValueForKey(input, "\\hport\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->hostPort = atol(buffer);
        if (!gpiValueForKey(input, "\\sessflags\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->sessionFlags = atol(buffer);
        gsifree(buddyStatusInfo->richStatus);
        buddyStatusInfo->richStatus = NULL;
        if (!gpiValueForKey(input, "\\rstatus\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->richStatus = goastrdup(buffer);
        gsifree(buddyStatusInfo->gameType);
        buddyStatusInfo->gameType = NULL;
        if (!gpiValueForKey(input, "\\gameType\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->gameType = goastrdup(buffer);
        gsifree(buddyStatusInfo->gameVariant);
        buddyStatusInfo->gameVariant = NULL;
        if (!gpiValueForKey(input, "\\gameVnt\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->gameVariant = goastrdup(buffer);
        gsifree(buddyStatusInfo->gameMapName);
        buddyStatusInfo->gameMapName = NULL;
        if (!gpiValueForKey(input, "\\gameMn\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->gameMapName = goastrdup(buffer);
        if (!gpiValueForKey(input, "\\product\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->productId = atol(buffer);
        if (!gpiValueForKey(input, "\\qmodeflags\\", buffer, 0x400)) {
            GPI_BAD_MESSAGE(connection);
        }
        buddyStatusInfo->quietModeFlags = atol(buffer);
        callback = iconnection->callbacks[2];
        if (callback.callback != NULL) {
            arg = gsimalloc(sizeof(GPRecvBuddyStatusArg));
            if (arg == NULL) {
                gpiSetErrorString(connection, "Out of memory.");
                return 1;
            }
            arg->date = date;
            arg->index = buddyStatusInfo->buddyIndex;
            arg->profile = profileid;
            {
                int result = gpiAddCallback(connection, callback, arg, NULL, GPI_ADD_NORMAL);

                if (result != 0) {
                    return result;
                }
            }
        }
    }
    return 0;
}

GPResult gpiProcessRecvBuddyList(GPConnection *connection, const char *input) {
    int num;
    int profileid;
    GPIConnection *iconnection;
    GPIProfile *profile;
    char buffer[0x200];
    int index;
    int i;
    gsi_u32 j;
    const char *list;

    index = 0;
    iconnection = *connection;
    if (gpiCheckForError(connection, input, gsi_true)) {
        return 4;
    }
    if (!gpiValueForKeyWithIndex(input, "\\bdy\\", &index, buffer, 0x200)) {
        GPI_BAD_MESSAGE(connection);
    }
    num = atol(buffer);
    list = strstr(input, "\\list\\");
    if (list == NULL) {
        GPI_BAD_MESSAGE(connection);
    }
    index += 6;
    for (i = 0; i < num; i++) {
        if (i == 0) {
            for (j = 0; j < 0x200 && list[j + 6] != '\0' && list[j + 6] != ','; j++) {
                buffer[j] = list[j + 6];
            }
            buffer[j] = '\0';
            index += j;
        } else if (!gpiValueForKeyWithIndex(input, ",", &index, buffer, 0x200)) {
            GPI_BAD_MESSAGE(connection);
        }
        profileid = atol(buffer);
        profile = gpiProfileListAdd(connection, profileid);
        if (profile == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
        profile->buddyStatus = gsimalloc(sizeof(GPIBuddyStatus));
        if (profile->buddyStatus == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
        memset(profile->buddyStatus, 0, sizeof(GPIBuddyStatus));
        profile->buddyStatus->buddyIndex = iconnection->profileList.numBuddies++;
        profile->buddyStatus->status = 0;
        gpiSetBuddyDeleted(connection, profileid, gsi_false);
    }
    return 0;
}

GPResult gpiSendServerBuddyMessage(GPConnection *connection, int profileid, int type, const char *message) {
    GPIConnection *iconnection = *connection;
    char buffer[0xdad];

    strzcpy(buffer, message, 0xdad);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\bm\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, type);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\t\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, profileid);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\msg\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, buffer);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
    return 0;
}

GPResult gpiSendBuddyMessage(GPConnection *connection, int profileid, int type, const char *message, int sendOption,
                             GPIPeerOp *peerOp) {
    GPIProfile *profile;
    GPIPeer *peer;
    int result;

    peer = gpiGetPeerByProfile(connection, profileid);
    if (peer == NULL) {
        if (!gpiGetProfile(connection, profileid, &profile) || profile->buddyStatusInfo == NULL ||
            profile->buddyStatusInfo->buddyPort == 0) {
            if (sendOption == 0xb00) {
                return 3;
            }
            return gpiSendServerBuddyMessage(connection, profileid, type, message);
        }
        peer = gpiAddPeer(connection, profileid, gsi_true);
        if (peer == NULL) {
            return 1;
        }
        if (profile->peerSig == NULL) {
            result = gpiPeerGetSig(connection, peer);
            if (result != 0) {
                return result;
            }
        } else {
            result = gpiPeerStartConnect(connection, peer);
            if (result != 0) {
                return result;
            }
        }
    } else if (peer->state == GPI_PEER_DISCONNECTED && gpiGetProfile(connection, profileid, &profile)) {
        if (profile->buddyStatusInfo != NULL) {
            profile->buddyStatusInfo->buddyPort = 0;
        }
        if (sendOption == 0xb00) {
            return 3;
        }
        if (type < GPI_BM_STATUS) {
            return gpiSendServerBuddyMessage(connection, profileid, type, message);
        }
    }
    if (peerOp != NULL) {
        gpiPeerAddOp(peer, peerOp);
    }
    result = gpiPeerAddMessage(connection, peer, type, message);
    if (result != 0) {
        return result;
    }
    return 0;
}

GPResult gpiBuddyHandleKeyRequest(GPConnection *connection, GPIPeer *peer) {
    char *message;
    int result;

    result = gpiSaveKeysToBuffer(connection, &message);
    if (result != 0) {
        return result;
    }
    if (message == NULL) {
        message = "";
    }
    result = gpiSendBuddyMessage(connection, peer->profile, GPI_BM_KEYS_REPLY, message, 0xb00, NULL);
    if (result != 0) {
        return result;
    }
    if (strcmp(message, "") != 0) {
        gsifree(message);
    }
    return 0;
}

// Finds the operation of a peer that waits for the reply to a keys request
#define GPI_FIND_KEYS_OP(op, peer)                                                                                     \
    for ((op) = (peer)->peerOpQueue.opList; (op) != NULL; (op) = (op)->next) {                                         \
        if ((op)->state == GPI_BM_KEYS_REQUEST) {                                                                      \
            break;                                                                                                     \
        }                                                                                                              \
    }

GPResult gpiBuddyHandleKeyReply(GPConnection *connection, GPIPeer *peer, char *buffer) {
    // The order of these declarations sets the locals' stack slots
    GPIProfile *profile;
    GPIPeerOp *op;
    GPGetBuddyStatusInfoKeysArg *arg;
    char key[0x200];
    char value[0x200];
    char keyDecoded[0x200];
    char valueDecoded[0x200];
    GPICallback callbackNone;
    int decodedLength;
    int index;
    char *keyValue;
    GPICallback callbackEmpty;
    GPICallback callbackKeys;
    int numKeys;
    char **keys;
    char **values;
    int i;

    if (!gpiGetProfile(connection, peer->profile, &profile)) {
        gpiSetErrorString(connection, "Invalid profile.");
        return 2;
    }
    if (strcmp(buffer, "") == 0) {
        GPI_FIND_KEYS_OP(op, peer);
        if (op == NULL) {
            return 0;
        }
        if (op->state == GPI_BM_KEYS_REQUEST && op->callback != NULL) {
            arg = gsimalloc(sizeof(GPGetBuddyStatusInfoKeysArg));
            callbackNone.callback = op->callback;
            callbackNone.param = op->userData;
            arg->keys = NULL;
            arg->numKeys = 0;
            arg->values = NULL;
            arg->profile = peer->profile;
            gpiAddCallback(connection, callbackNone, arg, NULL, GPI_ADD_NORMAL);
            gpiPeerRemoveOp(peer, op);
        }
    } else {
        decodedLength = 0;
        index = 0;
        keyValue = NULL;
        gpiReadKeyAndValue(connection, buffer, &index, key, value);
        if (strcmp(key, "keys") != 0) {
            gpiSetError(connection, 1, "Error reading keys reply message");
            gpiCallErrorCallback(connection, 3, 0);
            return 3;
        }
        numKeys = atol(value);
        if (numKeys == 0) {
            GPI_FIND_KEYS_OP(op, peer);
            if (op == NULL) {
                return 0;
            }
            if (op->state == GPI_BM_KEYS_REQUEST && op->callback != NULL) {
                arg = gsimalloc(sizeof(GPGetBuddyStatusInfoKeysArg));
                callbackEmpty.callback = op->callback;
                callbackEmpty.param = op->userData;
                arg->keys = NULL;
                arg->numKeys = 0;
                arg->values = NULL;
                arg->profile = peer->profile;
                gpiAddCallback(connection, callbackEmpty, arg, NULL, GPI_ADD_NORMAL);
                gpiPeerRemoveOp(peer, op);
            }
        } else {
            keys = gsimalloc(numKeys * sizeof(char *));
            values = gsimalloc(numKeys * sizeof(char *));
            for (i = 0; i < numKeys; i++) {
                gpiReadKeyAndValue(connection, buffer, &index, key, value);
                B64Decode(key, keyDecoded, strlen(key), &decodedLength, 2);
                keyDecoded[decodedLength] = '\0';
                B64Decode(value, valueDecoded, strlen(value), &decodedLength, 2);
                valueDecoded[decodedLength] = '\0';
                keys[i] = goastrdup(keyDecoded);
                values[i] = goastrdup(valueDecoded);
                if (gpiStatusInfoGetKey(connection, profile->buddyStatusInfo->extendedInfoKeys, keyDecoded,
                                        &keyValue) == 0 &&
                    keyValue == NULL) {
                    gpiStatusInfoAddKey(connection, profile->buddyStatusInfo->extendedInfoKeys, keyDecoded,
                                        valueDecoded);
                } else {
                    gpiStatusInfoSetKey(connection, profile->buddyStatusInfo->extendedInfoKeys, keyDecoded,
                                        valueDecoded);
                }
            }
            GPI_FIND_KEYS_OP(op, peer);
            if (op == NULL) {
                return 0;
            }
            if (op->state == GPI_BM_KEYS_REQUEST && op->callback != NULL) {
                arg = gsimalloc(sizeof(GPGetBuddyStatusInfoKeysArg));
                callbackKeys.callback = op->callback;
                callbackKeys.param = op->userData;
                arg->numKeys = numKeys;
                arg->keys = keys;
                arg->values = values;
                arg->profile = peer->profile;
                gpiAddCallback(connection, callbackKeys, arg, NULL, GPI_ADD_BUDDYKEYS);
                gpiPeerRemoveOp(peer, op);
            }
        }
    }
    return 0;
}

GPResult gpiAuthBuddyRequest(GPConnection *connection, GPProfile profile) {
    GPIConnection *iconnection = *connection;
    GPIProfile *pProfile;
    int result;

    if (!gpiGetProfile(connection, profile, &pProfile)) {
        gpiSetErrorString(connection, "Invalid profile.");
        return 2;
    }
    if (pProfile->authSig == NULL) {
        gpiSetErrorString(connection, "Invalid profile.");
        return 2;
    }
    result = gpiSendAuthBuddyRequest(connection, pProfile);
    if (result != 0) {
        return result;
    }
    pProfile->requestCount--;
    if (!iconnection->infoCaching && pProfile->requestCount <= 0) {
        gsifree(pProfile->authSig);
        pProfile->authSig = NULL;
        if (gpiCanFreeProfile(pProfile)) {
            gpiRemoveProfile(connection, pProfile);
        }
    }
    return 0;
}

// Moves down the buddy indexes after the one of a deleted buddy (data)
static GPIBool gpiFixBuddyIndices(GPConnection *connection, GPIProfile *profile, void *data) {
    int baseIndex = (int)data;

    if (profile->buddyStatus != NULL && profile->buddyStatus->buddyIndex > baseIndex) {
        profile->buddyStatus->buddyIndex--;
    } else if (profile->buddyStatusInfo != NULL && profile->buddyStatusInfo->buddyIndex > baseIndex) {
        profile->buddyStatusInfo->buddyIndex--;
    }
    return gsi_true;
}

GPResult gpiDeleteBuddy(GPConnection *connection, GPProfile profile, GPIBool sendServerRequest) {
    GPIConnection *iconnection = *connection;
    GPIProfile *pProfile;
    GPIBuddyStatus *buddyStatus;
    GPIBuddyStatusInfo *buddyStatusInfo;
    int index;

    if (!gpiGetProfile(connection, profile, &pProfile)) {
        gpiSetErrorString(connection, "Invalid profile.");
        return 2;
    }
    if (sendServerRequest == gsi_true) {
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\delbuddy\\");
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
        gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\delprofileid\\");
        gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, pProfile->profileId);
        gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
    }
    gpiSetBuddyDeleted(connection, profile, gsi_true);
    buddyStatus = pProfile->buddyStatus;
    if (buddyStatus != NULL) {
        index = buddyStatus->buddyIndex;
        gsifree(buddyStatus->statusString);
        pProfile->buddyStatus->statusString = NULL;
        gsifree(pProfile->buddyStatus->locationString);
        pProfile->buddyStatus->locationString = NULL;
        gsifree(pProfile->buddyStatus);
        pProfile->buddyStatus = NULL;
        iconnection->profileList.numBuddies--;
        gpiProfileMap(connection, gpiFixBuddyIndices, (void *)index);
    }
    buddyStatusInfo = pProfile->buddyStatusInfo;
    if (buddyStatusInfo != NULL) {
        index = buddyStatusInfo->buddyIndex;
        gsifree(buddyStatusInfo->richStatus);
        pProfile->buddyStatusInfo->richStatus = NULL;
        gsifree(pProfile->buddyStatusInfo->gameType);
        pProfile->buddyStatusInfo->gameType = NULL;
        gsifree(pProfile->buddyStatusInfo->gameVariant);
        pProfile->buddyStatusInfo->gameVariant = NULL;
        gsifree(pProfile->buddyStatusInfo->gameMapName);
        pProfile->buddyStatusInfo->gameMapName = NULL;
        // BUG: the status info is freed and its pointer cleared before its keys are, so the keys are read through
        // NULL and never freed
#ifdef BUGFIX
        if (pProfile->buddyStatusInfo->extendedInfoKeys != NULL) {
            func_ov011_02171558(pProfile->buddyStatusInfo->extendedInfoKeys);
            pProfile->buddyStatusInfo->extendedInfoKeys = NULL;
        }
        gsifree(pProfile->buddyStatusInfo);
        pProfile->buddyStatusInfo = NULL;
#else
        gsifree(pProfile->buddyStatusInfo);
        pProfile->buddyStatusInfo = NULL;
        if (pProfile->buddyStatusInfo->extendedInfoKeys != NULL) {
            func_ov011_02171558(pProfile->buddyStatusInfo->extendedInfoKeys);
            pProfile->buddyStatusInfo->extendedInfoKeys = NULL;
        }
#endif
        iconnection->profileList.numBuddies--;
        gpiProfileMap(connection, gpiFixBuddyIndices, (void *)index);
    }
    return 0;
}

int gpiSetBuddyDeleted(GPConnection *connection, int profile, gsi_bool deleted) {
    GPIProfile *pProfile;

    if (profile <= 0) {
        gpiSetErrorString(connection, "Invalid profile.");
        return 2;
    }
    if (gpiGetProfile(connection, profile, &pProfile)) {
        pProfile->deleted = deleted;
    }
    return 0;
}
