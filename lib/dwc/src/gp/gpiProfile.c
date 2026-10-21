#include "dwc/gpiProfile.h"
#include "dwc/ext.h"
#include "dwc/gpiBuddy.h"
#include "dwc/gpiCallback.h"
#include "dwc/gpiInfo.h"
#include "dwc/gpiOperation.h"
#include "dwc/gpiUtility.h"

// GameSpy GP's profiles (gpiProfile.c). The functions' names, from pret/pokerevo's Wii build and GameSpy's later SDK,
// are in dwc/gp.h

static int gpiProfilesTableHash(const void *element, int numBuckets);
static int gpiProfilesTableCompare(const void *element1, const void *element2);
static void gpiProfilesTableFree(void *element);
static BOOL gpiCheckProfileForUser(GPConnection *connection, GPIProfile *profile, void *data);
static int gpiProfileMapCallback(void *element, void *clientData);
static BOOL gpiCheckForBuddy(GPConnection *connection, GPIProfile *profile, void *data);

// What gpiFindProfileByUser looks for, and where it puts what it found
typedef struct GPIFindProfileByUserData {
    const char *nick;
    const char *email;
    GPIProfile **profile;
    BOOL found;
} GPIFindProfileByUserData;

// What gpiProfileMap calls for each element of the table
typedef struct GPIProfileMapData {
    GPConnection *connection;
    GPIProfileMapFunc func;
    void *data;
} GPIProfileMapData;

// What gpiFindBuddy looks for, and what it found
typedef struct GPIFindBuddyData {
    int buddyIndex;
    GPIProfile *profile;
} GPIFindBuddyData;

static int gpiProfilesTableHash(const void *element, int numBuckets) {
    return ((const GPIProfile *)element)->profileId % numBuckets;
}

static int gpiProfilesTableCompare(const void *element1, const void *element2) {
    return ((const GPIProfile *)element1)->profileId - ((const GPIProfile *)element2)->profileId;
}

static void gpiProfilesTableFree(void *element) {
    GPIProfile *profile = element;

    if (profile->buddyStatus != NULL) {
        gsifree(profile->buddyStatus->statusString);
        profile->buddyStatus->statusString = NULL;
        gsifree(profile->buddyStatus->locationString);
        profile->buddyStatus->locationString = NULL;
        gsifree(profile->buddyStatus);
        profile->buddyStatus = NULL;
    }
    if (profile->buddyStatusInfo != NULL) {
        gsifree(profile->buddyStatusInfo->richStatus);
        profile->buddyStatusInfo->richStatus = NULL;
        gsifree(profile->buddyStatusInfo->gameType);
        profile->buddyStatusInfo->gameType = NULL;
        gsifree(profile->buddyStatusInfo->gameVariant);
        profile->buddyStatusInfo->gameVariant = NULL;
        gsifree(profile->buddyStatusInfo->gameMapName);
        profile->buddyStatusInfo->gameMapName = NULL;
        if (profile->buddyStatusInfo->extendedInfoKeys != NULL) {
            func_ov011_02171558(profile->buddyStatusInfo->extendedInfoKeys);
            profile->buddyStatusInfo->extendedInfoKeys = NULL;
        }
        gsifree(profile->buddyStatusInfo);
        profile->buddyStatusInfo = NULL;
    }
    gpiFreeInfoCache(profile);
    gsifree(profile->authSig);
    profile->authSig = NULL;
    gsifree(profile->peerSig);
    profile->peerSig = NULL;
}

BOOL gpiInitProfiles(GPConnection *connection) {
    GPIConnection *iconnection = *connection;

    iconnection->profileList.numBuddies = 0;
    iconnection->profileList.numBlocked = 0;
    iconnection->profileList.num = 0;
    iconnection->profileList.profileTable =
        TableNew(sizeof(GPIProfile), 32, gpiProfilesTableHash, gpiProfilesTableCompare, gpiProfilesTableFree);
    return iconnection->profileList.profileTable != NULL;
}

// The server's "\npr\" answer to the making of a profile
int gpiProcessNewProfile(GPConnection *connection, GPIOperation *operation, const char *input) {
    char buffer[16];
    int pid;
    GPICallback callback;
    GPNewProfileResponseArg *arg;
    int result;

    if (gpiCheckForError(connection, input, TRUE)) {
        return 4;
    }
    if (strncmp(input, "\\npr\\", 5) != 0) {
        gpiSetError(connection, 1, "Unexpected data was received from the server.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    if (!gpiValueForKey(input, "\\profileid\\", buffer, 16)) {
        gpiSetError(connection, 1, "Unexpected data was received from the server.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    pid = atol(buffer);
    callback = operation->callback;
    if (callback.callback != NULL) {
        arg = gsimalloc(sizeof(GPNewProfileResponseArg));
        if (arg == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
        arg->profile = pid;
        arg->result = 0;
        result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
        if (result != 0) {
            return result;
        }
    }
    gpiRemoveOperation(connection, operation);
    return 0;
}

// Gives the profile of an id, adding one if there is none (NULL for an id that isn't positive)
GPIProfile *gpiProfileListAdd(GPConnection *connection, int id) {
    GPIConnection *iconnection = *connection;
    GPIProfileList *profileList = &iconnection->profileList;
    GPIProfile *profile;

    if (id <= 0) {
        return NULL;
    }
    if (gpiGetProfile(connection, id, &profile)) {
        return profile;
    }
    GPIProfile newProfile = { 0 };
    GPIProfile *result = NULL;

    newProfile.profileId = id;
    newProfile.userId = 0;
    newProfile.cache = NULL;
    newProfile.authSig = NULL;
    newProfile.peerSig = NULL;
    newProfile.requestCount = 0;
    newProfile.deleted = FALSE;
    TableEnter(profileList->profileTable, &newProfile);
    profileList->num++;
    if (gpiGetProfile(connection, id, &profile)) {
        result = profile;
    }
    return result;
}

BOOL gpiGetProfile(GPConnection *connection, int profileid, GPIProfile **profile) {
    GPIConnection *iconnection = *connection;
    GPIProfile profileTemp;
    GPIProfile *pProfile;

    profileTemp.profileId = profileid;
    pProfile = TableLookup(iconnection->profileList.profileTable, &profileTemp);
    if (profile != NULL) {
        *profile = pProfile;
    }
    if (pProfile != NULL) {
        return TRUE;
    }
    return FALSE;
}

// The server's "\dpr\" answer to the deletion of the profile of the login
int gpiProcessDeleteProfle(GPConnection *connection, GPIOperation *operation, const char *input) {
    GPIConnection *iconnection = *connection;
    GPICallback callback;
    GPDeleteProfileResponseArg *arg;
    int result;

    if (gpiCheckForError(connection, input, TRUE)) {
        return 4;
    }
    if (strncmp(input, "\\dpr\\", 5) != 0) {
        gpiSetError(connection, 1, "Unexpected data was received from the server.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    callback = operation->callback;
    if (callback.callback != NULL) {
        arg = gsimalloc(sizeof(GPDeleteProfileResponseArg));
        if (arg == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
        arg->profile = iconnection->profileid;
        arg->result = 0;
        result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
        if (result != 0) {
            return result;
        }
    }
    gpiRemoveOperation(connection, operation);
    return 0;
}

void gpiRemoveProfileByID(GPConnection *connection, int profileid) {
    GPIConnection *iconnection = *connection;
    GPIProfile *profile;

    if (gpiGetProfile(connection, profileid, &profile)) {
        TableRemove(iconnection->profileList.profileTable, profile);
    }
}

void gpiRemoveProfile(GPConnection *connection, GPIProfile *profile) {
    GPIConnection *iconnection = *connection;

    TableRemove(iconnection->profileList.profileTable, profile);
}

static BOOL gpiCheckProfileForUser(GPConnection *connection, GPIProfile *profile, void *data) {
    GPIFindProfileByUserData *findData = data;
    GPIInfoCache *cache = profile->cache;

    if (cache != NULL && strcmp(findData->nick, cache->nick) == 0 && strcmp(findData->email, cache->email) == 0) {
        *findData->profile = profile;
        findData->found = TRUE;
        return FALSE;
    }
    return TRUE;
}

int gpiFindProfileByUser(GPConnection *connection, const char *nick, const char *email, GPIProfile **profile) {
    GPIFindProfileByUserData data;

    data.nick = nick;
    data.email = email;
    data.profile = profile;
    data.found = FALSE;
    gpiProfileMap(connection, gpiCheckProfileForUser, &data);
    if (!data.found) {
        *profile = NULL;
    }
    return 0;
}

static int gpiProfileMapCallback(void *element, void *clientData) {
    GPIProfileMapData *data = clientData;

    return data->func(data->connection, element, data->data);
}

BOOL gpiProfileMap(GPConnection *connection, GPIProfileMapFunc func, void *data) {
    GPIConnection *iconnection = *connection;
    GPIProfileMapData mapData;

    mapData.connection = connection;
    mapData.func = func;
    mapData.data = data;
    if (TableMapSafe2(iconnection->profileList.profileTable, gpiProfileMapCallback, &mapData) == NULL) {
        return TRUE;
    }
    return FALSE;
}

static BOOL gpiCheckForBuddy(GPConnection *connection, GPIProfile *profile, void *data) {
    GPIFindBuddyData *findData = data;

    if (profile->buddyStatus != NULL && findData->buddyIndex == profile->buddyStatus->buddyIndex) {
        findData->profile = profile;
        return FALSE;
    }
    if (profile->buddyStatusInfo != NULL && findData->buddyIndex == profile->buddyStatusInfo->buddyIndex) {
        findData->profile = profile;
        return FALSE;
    }
    return TRUE;
}

GPIProfile *gpiFindBuddy(GPConnection *connection, int buddyIndex) {
    GPIFindBuddyData data;

    data.buddyIndex = buddyIndex;
    data.profile = NULL;
    gpiProfileMap(connection, gpiCheckForBuddy, &data);
    return data.profile;
}

void gpiRemoveBuddyStatus(GPIBuddyStatus *buddyStatus) {
    gsifree(buddyStatus->locationString);
    buddyStatus->locationString = NULL;
    gsifree(buddyStatus->statusString);
    buddyStatus->statusString = NULL;
    gsifree(buddyStatus);
}

void gpiRemoveBuddyStatusInfo(GPIBuddyStatusInfo *buddyStatusInfo) {
    gsifree(buddyStatusInfo->richStatus);
    buddyStatusInfo->richStatus = NULL;
    gsifree(buddyStatusInfo->gameType);
    buddyStatusInfo->gameType = NULL;
    gsifree(buddyStatusInfo->gameVariant);
    buddyStatusInfo->gameVariant = NULL;
    gsifree(buddyStatusInfo->gameMapName);
    buddyStatusInfo->gameMapName = NULL;
    func_ov011_02171558(buddyStatusInfo->extendedInfoKeys);
    buddyStatusInfo->extendedInfoKeys = NULL;
    gsifree(buddyStatusInfo);
}

// Whether nothing keeps a profile in the table any more
BOOL gpiCanFreeProfile(GPIProfile *profile) {
    if (profile != NULL && profile->cache == NULL && profile->buddyStatus == NULL && profile->buddyStatusInfo == NULL &&
        profile->peerSig == NULL && profile->authSig == NULL && !profile->blocked) {
        return TRUE;
    }
    return FALSE;
}

// The list is "\blk\<count>\list\<id>,<id>,...", whose first id is read in place
int gpiProcessRecvBlockedList(GPConnection *connection, const char *input) {
    int i = 0;
    int index = 0;
    GPIConnection *iconnection = *connection;
    int num;
    int profileid;
    unsigned int j;
    char c;
    char *str;
    GPIProfile *profile;
    char buffer[512];

    if (gpiCheckForError(connection, input, TRUE)) {
        return 4;
    }
    if (!gpiValueForKeyWithIndex(input, "\\blk\\", &index, buffer, 512)) {
        gpiSetError(connection, 1, "Unexpected data was received from the server.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    num = atol(buffer);
    str = strstr(input, "\\list\\");
    if (str == NULL) {
        gpiSetError(connection, 1, "Unexpected data was received from the server.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    index += 6;
    for (i = 0; i < num; i++) {
        if (i == 0) {
            for (j = 0; j < 512 && (c = str[j + 6]) != '\0' && c != ','; j++) {
                buffer[j] = c;
            }
            buffer[j] = '\0';
            index += j;
        } else if (!gpiValueForKeyWithIndex(input, ",", &index, buffer, 512)) {
            gpiSetError(connection, 1, "Unexpected data was received from the server.");
            gpiCallErrorCallback(connection, 3, 1);
            return 3;
        }
        profileid = atol(buffer);
        profile = gpiProfileListAdd(connection, profileid);
        if (profile == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
        profile->blocked = TRUE;
        profile->blockIndex = iconnection->profileList.numBlocked++;
        gpiSetBuddyDeleted(connection, profileid, TRUE);
    }
    return 0;
}
