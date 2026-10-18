#include "dwc/gp.h"
#include "dwc/ext.h"
#include "dwc/gpi.h"
#include "dwc/gpiBuddy.h"
#include "dwc/gpiBuffer.h"
#include "dwc/gpiConnect.h"
#include "dwc/gpiInfo.h"
#include "dwc/gpiProfile.h"
#include "dwc/gpiSearch.h"
#include "dwc/gpiUtility.h"

// GameSpy GP's API (gp.c). The functions' names, from pret/pokerevo's Wii build (the later SDK's for
// gpGetReverseBuddiesList and gpGetLoginTicket), are in dwc/gp.h. A simulated connection answers everything with
// empty results

int gpInitialize(GPConnection *connection, int productID, int namespaceID, int partnerID) {
    if (data_ov011_02190698 != 1) {
        return 2;
    }
    if (connection == NULL) {
        return 2;
    }
    return gpiInitialize(connection, productID, namespaceID, partnerID);
}

void gpDestroy(GPConnection *connection) {
    if (connection != NULL && *connection != NULL) {
        gpiDestroy(connection);
    }
}

int gpProcess(GPConnection *connection) {
    GPIConnection *iconnection;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (iconnection->simulation) {
        return 0;
    }
    return gpiProcess(connection, 0);
}

int gpSetCallback(GPConnection *connection, int func, GPCallback callback, void *param) {
    GPIConnection *iconnection;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (func < 0 || func >= GPI_NUM_CALLBACKS) {
        gpiSetErrorString(connection, "Invalid func.");
        return 2;
    }
    iconnection->callbacks[func].callback = callback;
    iconnection->callbacks[func].param = param;
    return 0;
}

int gpConnectPreAuthenticated(GPConnection *connection, const char *authtoken, const char *partnerchallenge,
                              int firewall, int blocking, GPCallback callback, void *param) {
    GPIConnection *iconnection;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (authtoken == NULL || authtoken[0] == '\0') {
        return 2;
    }
    if (partnerchallenge == NULL || partnerchallenge[0] == '\0') {
        return 2;
    }
    if (callback == NULL) {
        gpiSetErrorString(connection, "No callback.");
        return 2;
    }
    if (iconnection->simulation) {
        GPConnectResponseArg arg = { 0 };

        callback(connection, &arg, param);
        return 0;
    }
    return gpiConnect(connection, "", "", "", "", authtoken, partnerchallenge, NULL, firewall, FALSE, blocking,
                      callback, param);
}

void gpDisconnect(GPConnection *connection) {
    GPIConnection *iconnection;
    int state;

    if (connection != NULL && (iconnection = *connection) != NULL && !iconnection->simulation) {
        state = iconnection->connectState;
        gpiDisconnect(connection, TRUE);
        gpiReset(connection);
        if (state == GPI_CONNECTED) {
            iconnection->connectState = GPI_DISCONNECTED;
        }
    }
}

int gpProfileSearch(GPConnection *connection, const char *nick, const char *uniquenick, const char *email,
                    const char *firstname, const char *lastname, int icquin, int blocking, GPCallback callback,
                    void *param) {
    GPIConnection *iconnection;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (callback == NULL) {
        gpiSetErrorString(connection, "No callback.");
        return 2;
    }
    if (iconnection->simulation) {
        GPProfileSearchResponseArg arg = { 0 };

        arg.more = 0x601;
        callback(connection, &arg, param);
        return 0;
    }
    return gpiProfileSearch(connection, nick, uniquenick, email, firstname, lastname, icquin, 0, blocking, callback,
                            param);
}

int gpGetInfo(GPConnection *connection, int profile, int checkCache, int blocking, GPCallback callback, void *param) {
    GPIConnection *iconnection;

    if (connection == NULL || (iconnection = *connection) == NULL || profile == 0) {
        return 2;
    }
    if (callback == NULL) {
        gpiSetErrorString(connection, "No callback.");
        return 2;
    }
    if (iconnection->simulation) {
        GPGetInfoResponseArg arg = { 0 };

        callback(connection, &arg, param);
        return 0;
    }
    if (iconnection->connectState == GPI_DISCONNECTED) {
        gpiSetErrorString(connection, "The connection has already been disconnected.");
        return 2;
    }
    return gpiGetInfo(connection, profile, checkCache, blocking, callback, param);
}

int gpSetInfos(GPConnection *connection, int info, const char *value) {
    GPIConnection *iconnection;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (iconnection->simulation) {
        return 0;
    }
    if (iconnection->connectState == GPI_DISCONNECTED) {
        gpiSetErrorString(connection, "The connection has already been disconnected.");
        return 2;
    }
    return gpiSetInfos(connection, info, value);
}

int gpSendBuddyRequest(GPConnection *connection, int profile, const char *reason) {
    GPIConnection *iconnection;
    char reasonFixed[1025];
    int i;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (iconnection->simulation) {
        return 0;
    }
    if (iconnection->connectState == GPI_DISCONNECTED) {
        gpiSetErrorString(connection, "The connection has already been disconnected.");
        return 2;
    }
    if (reason == NULL) {
        gpiSetErrorString(connection, "Invalid reason.");
        return 2;
    }
    // The server's messages are "\key\value" texts, so a backslash in the reason becomes a slash
    strzcpy(reasonFixed, reason, 1025);
    for (i = 0; reasonFixed[i] != '\0'; i++) {
        if (reasonFixed[i] == '\\') {
            reasonFixed[i] = '/';
        }
    }
    gpiSetBuddyDeleted(connection, profile, FALSE);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\addbuddy\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\newprofileid\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, profile);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\reason\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, reasonFixed);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
    return 0;
}

int gpAuthBuddyRequest(GPConnection *connection, int profile) {
    GPIConnection *iconnection;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (iconnection->simulation) {
        return 0;
    }
    if (iconnection->connectState == GPI_DISCONNECTED) {
        gpiSetErrorString(connection, "The connection has already been disconnected.");
        return 2;
    }
    return gpiAuthBuddyRequest(connection, profile);
}

int gpDenyBuddyRequest(GPConnection *connection, int profile) {
    GPIConnection *iconnection;
    GPIProfile *pProfile;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (iconnection->simulation) {
        return 0;
    }
    if (iconnection->connectState == GPI_DISCONNECTED) {
        gpiSetErrorString(connection, "The connection has already been disconnected.");
        return 2;
    }
    if (!gpiGetProfile(connection, profile, &pProfile)) {
        return 0;
    }
    pProfile->requestCount--;
    if (!iconnection->infoCaching && pProfile->requestCount <= 0) {
        func_ov011_0216ddfc(pProfile->authSig);
        pProfile->authSig = NULL;
        if (gpiCanFreeProfile(pProfile)) {
            gpiRemoveProfile(connection, pProfile);
        }
    }
    return 0;
}

int gpGetNumBuddies(GPConnection *connection, int *numBuddies) {
    GPIConnection *iconnection;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (iconnection->simulation) {
        *numBuddies = 0;
        return 0;
    }
    *numBuddies = iconnection->profileList.numBuddies;
    return 0;
}

int gpGetBuddyStatus(GPConnection *connection, int index, GPBuddyStatus *status) {
    GPIConnection *iconnection;
    GPIProfile *profile;
    int numBuddies;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (iconnection->simulation) {
        memset(status, 0, sizeof(GPBuddyStatus));
        return 0;
    }
    if (status == NULL) {
        gpiSetErrorString(connection, "Invalid status.");
        return 2;
    }
    numBuddies = iconnection->profileList.numBuddies;
    if (index < 0 || index >= numBuddies) {
        gpiSetErrorString(connection, "Invalid index.");
        return 2;
    }
    profile = gpiFindBuddy(connection, index);
    if (profile == NULL) {
        gpiSetErrorString(connection, "Invalid index.");
        return 2;
    }
    status->profile = profile->profileId;
    status->status = profile->buddyStatus->status;
    if (profile->buddyStatus->statusString != NULL) {
        strzcpy(status->statusString, profile->buddyStatus->statusString, 256);
    } else {
        status->statusString[0] = '\0';
    }
    if (profile->buddyStatus->locationString != NULL) {
        strzcpy(status->locationString, profile->buddyStatus->locationString, 256);
    } else {
        status->locationString[0] = '\0';
    }
    status->ip = profile->buddyStatus->ip;
    status->port = profile->buddyStatus->port;
    status->quietModeFlags = profile->buddyStatus->quietModeFlags;
    return 0;
}

int gpGetBuddyIndex(GPConnection *connection, int profile, int *index) {
    GPIConnection *iconnection;
    GPIProfile *pProfile;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (iconnection->simulation) {
        *index = 0;
        return 0;
    }
    if (gpiGetProfile(connection, profile, &pProfile) && pProfile->buddyStatus != NULL) {
        *index = pProfile->buddyStatus->buddyIndex;
    } else if (gpiGetProfile(connection, profile, &pProfile) && pProfile->buddyStatusInfo != NULL) {
        *index = pProfile->buddyStatusInfo->buddyIndex;
    } else {
        *index = -1;
    }
    return 0;
}

BOOL gpIsBuddy(GPConnection *connection, int profile) {
    GPIConnection *iconnection;
    GPIProfile *pProfile;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return FALSE;
    }
    if (iconnection->simulation) {
        return FALSE;
    }
    if (gpiGetProfile(connection, profile, &pProfile) && pProfile->buddyStatus != NULL) {
        return TRUE;
    }
    if (gpiGetProfile(connection, profile, &pProfile) && pProfile->buddyStatusInfo != NULL) {
        return TRUE;
    }
    return FALSE;
}

int gpDeleteBuddy(GPConnection *connection, int profile) {
    GPIConnection *iconnection;
    int result;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (iconnection->simulation) {
        return 0;
    }
    if (iconnection->connectState == GPI_DISCONNECTED) {
        gpiSetErrorString(connection, "The connection has already been disconnected.");
        return 2;
    }
    result = gpiDeleteBuddy(connection, profile, TRUE);
    if (result != 0) {
        return result;
    }
    return 0;
}

int gpSetStatus(GPConnection *connection, int status, const char *statusString, const char *locationString) {
    GPIConnection *iconnection;
    char statusStringFixed[256];
    char locationStringFixed[256];
    int i;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (iconnection->simulation) {
        return 0;
    }
    if (iconnection->connectState == GPI_DISCONNECTED) {
        gpiSetErrorString(connection, "The connection has already been disconnected.");
        return 2;
    }
    if (statusString == NULL) {
        gpiSetErrorString(connection, "Invalid statusString.");
        return 2;
    }
    if (locationString == NULL) {
        gpiSetErrorString(connection, "Invalid locationString.");
        return 2;
    }
    strzcpy(statusStringFixed, statusString, 256);
    for (i = 0; statusStringFixed[i] != '\0'; i++) {
        if (statusStringFixed[i] == '\\') {
            statusStringFixed[i] = '/';
        }
    }
    strzcpy(locationStringFixed, locationString, 256);
    for (i = 0; locationStringFixed[i] != '\0'; i++) {
        if (locationStringFixed[i] == '\\') {
            locationStringFixed[i] = '/';
        }
    }
    // Nothing is sent when the status doesn't change
    if (status == iconnection->lastStatusState && strcmp(statusStringFixed, iconnection->lastStatusString) == 0 &&
        strcmp(locationStringFixed, iconnection->lastLocationString) == 0) {
        return 0;
    }
    iconnection->lastStatusState = status;
    strzcpy(iconnection->lastStatusString, statusStringFixed, 256);
    strzcpy(iconnection->lastLocationString, locationStringFixed, 256);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\status\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, status);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\statstring\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, statusStringFixed);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\locstring\\");
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, locationStringFixed);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
    return 0;
}

int gpSendBuddyMessage(GPConnection *connection, int profile, const char *message) {
    GPIConnection *iconnection;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (iconnection->simulation) {
        return 0;
    }
    if (iconnection->connectState == GPI_DISCONNECTED) {
        gpiSetErrorString(connection, "The connection has already been disconnected.");
        return 2;
    }
    if (message == NULL) {
        gpiSetErrorString(connection, "Invalid message.");
        return 2;
    }
    return gpiSendBuddyMessage(connection, profile, GPI_BM_MESSAGE, message, 0, NULL);
}

int gpGetReverseBuddiesList(GPConnection *connection, int *targets, int numOfTargets, int blocking, GPCallback callback,
                            void *param) {
    GPIConnection *iconnection;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    if (callback == NULL) {
        gpiSetErrorString(connection, "No callback.");
        return 2;
    }
    if (iconnection->connectState == GPI_DISCONNECTED) {
        gpiSetErrorString(connection, "The connection has already been disconnected.");
        return 2;
    }
    if (iconnection->simulation) {
        GPGetReverseBuddiesListResponseArg arg = { 0 };

        callback(connection, &arg, param);
        return 0;
    }
    return gpiOthersBuddyList(connection, targets, numOfTargets, blocking, callback, param);
}

int gpGetLoginTicket(GPConnection *connection, GPLoginTicket *loginTicket) {
    GPIConnection *iconnection;

    if (connection == NULL || (iconnection = *connection) == NULL) {
        return 2;
    }
    *loginTicket = iconnection->loginTicket;
    return 0;
}
