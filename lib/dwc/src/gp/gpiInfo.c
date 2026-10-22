#include "dwc/gpiInfo.h"
#include "dwc/ext.h"
#include "dwc/gpiBuffer.h"
#include "dwc/gpiCallback.h"
#include "dwc/gpiOperation.h"
#include "dwc/gpiProfile.h"
#include "dwc/gpiUtility.h"

// GameSpy GP's info of the profiles (gpiInfo.c). The functions' names, from pret/pokerevo's Wii build and GameSpy's
// later SDK, are in dwc/gp.h. info is GameSpy's GPEnum of what to set: 0x700 the nick, 0x701 the unique nick, 0x702 the
// e-mail, 0x703 the password, 0x704 and 0x705 the first and last names, 0x706 the ICQ number, 0x707 the homepage, 0x708
// the zip code, 0x709 the country code, 0x70b the sex, 0x70c to 0x716 the machine (CPU, memory, video cards,
// connection, network and OS), 0x717 the AIM name, and 0x718 to 0x71e the picture, occupation, industry, income,
// marriage, children and interests

static GPIBool gpiIsValidDate(int day, int month, int year);
static GPResult gpiIntToDate(GPConnection *connection, int date, int *day, int *month, int *year);
static void gpiInfoCacheToArg(const GPIInfoCache *cache, GPGetInfoResponseArg *arg);
static GPResult gpiSendLocalInfo(GPConnection *connection, const char *info, const char *value);
static GPResult gpiSendUserInfo(GPConnection *connection, const char *info, const char *value);
static int gpiSetInfoi(GPConnection *connection, int info, int value);
static gsi_bool gpiSetInfoCache(GPConnection *connection, GPIProfile *profile, const GPIInfoCache *cache);

// All zero is no date; the last date the server takes is 6 June 2079
static GPIBool gpiIsValidDate(int day, int month, int year) {
    if (day == 0 && month == 0 && year == 0) {
        return gsi_true;
    }
    if (day < 0 || month < 0 || year < 0) {
        return gsi_false;
    }
    switch (month) {
    case 0:
        if (day != 0) {
            return gsi_false;
        }
        break;
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
        if (day > 31) {
            return gsi_false;
        }
        break;
    case 4:
    case 6:
    case 9:
    case 11:
        if (day > 30) {
            return gsi_false;
        }
        break;
    case 2:
        if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0) {
            if (day > 29) {
                return gsi_false;
            }
        } else if (day > 28) {
            return gsi_false;
        }
        break;
    default:
        return gsi_false;
    }
    if (year < 1900) {
        return gsi_false;
    }
    if (year > 2079) {
        return gsi_false;
    }
    if (year == 2079) {
        if (month > 6) {
            return gsi_false;
        }
        if (month == 6 && day > 6) {
            return gsi_false;
        }
    }
    return gsi_true;
}

// A date is packed as the day, the month and the year in the bytes from the top
static GPResult gpiIntToDate(GPConnection *connection, int date, int *day, int *month, int *year) {
    int d = (gsi_u8)(date >> 24);
    int m = (gsi_u8)(date >> 16);
    int y = (gsi_u16)date;

    if (!gpiIsValidDate(d, m, y)) {
        gpiSetErrorString(connection, "Invalid date.");
        return 2;
    }
    *day = d;
    *month = m;
    *year = y;
    return 0;
}

static void gpiInfoCacheToArg(const GPIInfoCache *cache, GPGetInfoResponseArg *arg) {
    if (cache->nick != NULL) {
        strzcpy(arg->nick, cache->nick, sizeof(arg->nick));
    } else {
        arg->nick[0] = '\0';
    }
    if (cache->uniquenick != NULL) {
        strzcpy(arg->uniquenick, cache->uniquenick, sizeof(arg->uniquenick));
    } else {
        arg->uniquenick[0] = '\0';
    }
    if (cache->email != NULL) {
        strzcpy(arg->email, cache->email, sizeof(arg->email));
    } else {
        arg->email[0] = '\0';
    }
    if (cache->firstname != NULL) {
        strzcpy(arg->firstname, cache->firstname, sizeof(arg->firstname));
    } else {
        arg->firstname[0] = '\0';
    }
    if (cache->lastname != NULL) {
        strzcpy(arg->lastname, cache->lastname, sizeof(arg->lastname));
    } else {
        arg->lastname[0] = '\0';
    }
    if (cache->homepage != NULL) {
        strzcpy(arg->homepage, cache->homepage, sizeof(arg->homepage));
    } else {
        arg->homepage[0] = '\0';
    }
    arg->icquin = cache->icquin;
    strzcpy(arg->zipcode, cache->zipcode, sizeof(arg->zipcode));
    strzcpy(arg->countrycode, cache->countrycode, sizeof(arg->countrycode));
    arg->longitude = cache->longitude;
    arg->latitude = cache->latitude;
    if (cache->place != NULL) {
        strzcpy(arg->place, cache->place, sizeof(arg->place));
    } else {
        arg->place[0] = '\0';
    }
    arg->birthday = cache->birthday;
    arg->birthmonth = cache->birthmonth;
    arg->birthyear = cache->birthyear;
    arg->sex = cache->sex;
    arg->publicmask = cache->publicmask;
    if (cache->aimname != NULL) {
        strzcpy(arg->aimname, cache->aimname, sizeof(arg->aimname));
    } else {
        arg->aimname[0] = '\0';
    }
    // The numbers are copied a second time, as GameSpy's code does
    arg->icquin = cache->icquin;
    arg->longitude = cache->longitude;
    arg->latitude = cache->latitude;
    arg->birthday = cache->birthday;
    arg->birthmonth = cache->birthmonth;
    arg->birthyear = cache->birthyear;
    arg->sex = cache->sex;
    arg->publicmask = cache->publicmask;
    arg->pic = cache->pic;
    arg->occupationid = cache->occupationid;
    arg->industryid = cache->industryid;
    arg->incomeid = cache->incomeid;
    arg->marriedid = cache->marriedid;
    arg->childcount = cache->childcount;
    arg->interests1 = cache->interests1;
    arg->ownership1 = cache->ownership1;
    arg->conntypeid = cache->conntypeid;
}

// The server's "\pi\" answer to "\getprofile\": the info is cached if the connection caches info, and the profile's
// signature is kept for a peer that waits for it
GPResult gpiProcessGetInfo(GPConnection *connection, GPIOperation *operation, const char *input) {
    GPIConnection *iconnection = *connection;
    char buffer[0x40];
    char nick[31];
    char uniquenick[21];
    char email[51];
    char firstname[31];
    char lastname[31];
    char aimname[51];
    GPIProfile *profile;
    GPICallback callback;
    GPGetInfoResponseArg *arg;
    GPIPeer *peer;
    int pid;
    gsi_bool saveSig;
    int result;

    if (gpiCheckForError(connection, input, gsi_true)) {
        return 4;
    }
    if (strncmp(input, "\\pi\\", 4) != 0) {
        gpiSetError(connection, 1, "Unexpected data was received from the server.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    if (!gpiValueForKey(input, "\\profileid\\", buffer, 0x40)) {
        gpiSetError(connection, 1, "Unexpected data was received from the server.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    pid = atol(buffer);
    gpiGetProfile(connection, pid, &profile);
    GPIInfoCache cache = { 0 };
    char homepage[76];
    cache.nick = nick;
    cache.uniquenick = uniquenick;
    cache.email = email;
    cache.firstname = firstname;
    cache.lastname = lastname;
    cache.homepage = homepage;
    cache.aimname = aimname;
    if (!gpiValueForKey(input, "\\nick\\", cache.nick, 31)) {
        cache.nick[0] = '\0';
    }
    if (!gpiValueForKey(input, "\\uniquenick\\", cache.uniquenick, 21)) {
        cache.uniquenick[0] = '\0';
    }
    if (!gpiValueForKey(input, "\\email\\", cache.email, 51)) {
        cache.email[0] = '\0';
    }
    if (!gpiValueForKey(input, "\\firstname\\", cache.firstname, 31)) {
        cache.firstname[0] = '\0';
    }
    if (!gpiValueForKey(input, "\\lastname\\", cache.lastname, 31)) {
        cache.lastname[0] = '\0';
    }
    if (!gpiValueForKey(input, "\\icquin\\", buffer, 0x40)) {
        cache.icquin = -1;
    } else {
        cache.icquin = atol(buffer);
    }
    if (!gpiValueForKey(input, "\\homepage\\", cache.homepage, 76)) {
        cache.homepage[0] = '\0';
    }
    if (!gpiValueForKey(input, "\\zipcode\\", cache.zipcode, 11)) {
        cache.zipcode[0] = '\0';
    }
    if (!gpiValueForKey(input, "\\countrycode\\", cache.countrycode, 3)) {
        cache.countrycode[0] = '\0';
    }
    if (!gpiValueForKey(input, "\\lon\\", buffer, 0x40)) {
        cache.longitude = 0;
    } else {
        cache.longitude = (float)atof(buffer);
    }
    if (!gpiValueForKey(input, "\\lat\\", buffer, 0x40)) {
        cache.latitude = 0;
    } else {
        cache.latitude = (float)atof(buffer);
    }
    if (!gpiValueForKey(input, "\\loc\\", cache.place, 128)) {
        cache.place[0] = '\0';
    }
    if (!gpiValueForKey(input, "\\birthday\\", buffer, 0x40)) {
        cache.birthday = 0;
        cache.birthmonth = 0;
        cache.birthyear = 0;
    } else {
        result = gpiIntToDate(connection, atol(buffer), &cache.birthday, &cache.birthmonth, &cache.birthyear);
        if (result != 0) {
            return result;
        }
    }
    if (!gpiValueForKey(input, "\\sex\\", buffer, 0x40)) {
        cache.sex = 0x502;
    } else if (buffer[0] == '0') {
        cache.sex = 0x500;
    } else if (buffer[0] == '1') {
        cache.sex = 0x501;
    } else {
        cache.sex = 0x502;
    }
    if (!gpiValueForKey(input, "\\pmask\\", buffer, 0x40)) {
        cache.publicmask = -1;
    } else {
        cache.publicmask = atol(buffer);
    }
    if (!gpiValueForKey(input, "\\aim\\", cache.aimname, 51)) {
        cache.aimname[0] = '\0';
    }
    if (!gpiValueForKey(input, "\\pic\\", buffer, 0x40)) {
        cache.pic = 0;
    } else {
        cache.pic = atol(buffer);
    }
    if (!gpiValueForKey(input, "\\occ\\", buffer, 0x40)) {
        cache.occupationid = 0;
    } else {
        cache.occupationid = atol(buffer);
    }
    if (!gpiValueForKey(input, "\\ind\\", buffer, 0x40)) {
        cache.industryid = 0;
    } else {
        cache.industryid = atol(buffer);
    }
    if (!gpiValueForKey(input, "\\inc\\", buffer, 0x40)) {
        cache.incomeid = 0;
    } else {
        cache.incomeid = atol(buffer);
    }
    if (!gpiValueForKey(input, "\\mar\\", buffer, 0x40)) {
        cache.marriedid = 0;
    } else {
        cache.marriedid = atol(buffer);
    }
    if (!gpiValueForKey(input, "\\chc\\", buffer, 0x40)) {
        cache.childcount = 0;
    } else {
        cache.childcount = atol(buffer);
    }
    if (!gpiValueForKey(input, "\\i1\\", buffer, 0x40)) {
        cache.interests1 = 0;
    } else {
        cache.interests1 = atol(buffer);
    }
    if (!gpiValueForKey(input, "\\o1\\", buffer, 0x40)) {
        cache.ownership1 = 0;
    } else {
        cache.ownership1 = atol(buffer);
    }
    if (!gpiValueForKey(input, "\\conn\\", buffer, 0x40)) {
        cache.conntypeid = 0;
    } else {
        cache.conntypeid = atol(buffer);
    }
    if (!gpiValueForKey(input, "\\sig\\", buffer, 0x40)) {
        gpiSetError(connection, 1, "Unexpected data was received from the server.");
        gpiCallErrorCallback(connection, 3, 1);
        return 3;
    }
    saveSig = iconnection->infoCaching;
    for (peer = iconnection->peerList; peer != NULL; peer = peer->pnext) {
        if (peer->profile == pid && peer->state == GPI_PEER_GETTING_SIG) {
            if (profile == NULL) {
                profile = gpiProfileListAdd(connection, pid);
            }
            peer->state = GPI_PEER_GOT_SIG;
            saveSig = gsi_true;
        }
    }
    if (profile == NULL && iconnection->infoCaching) {
        profile = gpiProfileListAdd(connection, pid);
    }
    if (saveSig) {
        gsifree(profile->peerSig);
        profile->peerSig = NULL;
        profile->peerSig = goastrdup(buffer);
    }
    if (iconnection->infoCaching) {
        gpiSetInfoCache(connection, profile, &cache);
    }
    callback = operation->callback;
    if (callback.callback != NULL) {
        arg = gsimalloc(sizeof(GPGetInfoResponseArg));
        if (arg == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
        gpiInfoCacheToArg(&cache, arg);
        arg->result = 0;
        arg->profile = pid;
        result = gpiAddCallback(connection, callback, arg, operation, GPI_ADD_NORMAL);
        if (result != 0) {
            return result;
        }
    }
    gpiRemoveOperation(connection, operation);
    return 0;
}

GPResult gpiAddLocalInfo(GPConnection *connection, GPIBuffer *buffer) {
    GPIConnection *iconnection = *connection;

    if (iconnection->updateproBuffer.len > 0) {
        gpiAppendStringToBuffer(connection, buffer, "\\updatepro\\\\sesskey\\");
        gpiAppendIntToBuffer(connection, buffer, iconnection->sessKey);
        gpiAppendStringToBuffer(connection, buffer, iconnection->updateproBuffer.buffer);
        gpiAppendStringToBuffer(connection, buffer, "\\partnerid\\");
        gpiAppendIntToBuffer(connection, buffer, iconnection->partnerID);
        gpiAppendStringToBuffer(connection, buffer, "\\final\\");
        iconnection->updateproBuffer.len = 0;
    }
    if (iconnection->updateuiBuffer.len > 0) {
        gpiAppendStringToBuffer(connection, buffer, "\\updateui\\\\sesskey\\");
        gpiAppendIntToBuffer(connection, buffer, iconnection->sessKey);
        gpiAppendStringToBuffer(connection, buffer, iconnection->updateuiBuffer.buffer);
        gpiAppendStringToBuffer(connection, buffer, "\\final\\");
        iconnection->updateuiBuffer.len = 0;
    }
    return 0;
}

// Adds a key and its value to the changes of the profile (sent as "\updatepro\") or of the user ("\updateui\")
static GPResult gpiSendLocalInfo(GPConnection *connection, const char *info, const char *value) {
    GPIConnection *iconnection = *connection;
    int result;

    result = gpiAppendStringToBuffer(connection, &iconnection->updateproBuffer, info);
    if (result != 0) {
        return result;
    }
    result = gpiAppendStringToBuffer(connection, &iconnection->updateproBuffer, value);
    if (result != 0) {
        return result;
    }
    return 0;
}

static GPResult gpiSendUserInfo(GPConnection *connection, const char *info, const char *value) {
    GPIConnection *iconnection = *connection;
    int result;

    result = gpiAppendStringToBuffer(connection, &iconnection->updateuiBuffer, info);
    if (result != 0) {
        return result;
    }
    result = gpiAppendStringToBuffer(connection, &iconnection->updateuiBuffer, value);
    if (result != 0) {
        return result;
    }
    return 0;
}

static int gpiSetInfoi(GPConnection *connection, int info, int value) {
    char buffer[16];
    int result;

    switch (info) {
    case 0x708:
        if (value < 0) {
            gpiSetErrorString(connection, "Invalid zipcode.");
            return 2;
        }
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendLocalInfo(connection, "\\zipcode\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x70b:
        switch (value) {
        case 0x500:
            result = gpiSendLocalInfo(connection, "\\sex\\", "0");
            if (result != 0) {
                return result;
            }
            break;
        case 0x501:
            result = gpiSendLocalInfo(connection, "\\sex\\", "1");
            if (result != 0) {
                return result;
            }
            break;
        case 0x502:
            result = gpiSendLocalInfo(connection, "\\sex\\", "2");
            if (result != 0) {
                return result;
            }
            break;
        default:
            gpiSetErrorString(connection, "Invalid sex.");
            return 2;
        }
        break;
    case 0x706:
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendLocalInfo(connection, "\\icquin\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x70c:
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendUserInfo(connection, "\\cpubrandid\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x70d:
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendUserInfo(connection, "\\cpuspeed\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x70e:
        OS_SPrintf(buffer, "%d", value / 16);
        result = gpiSendUserInfo(connection, "\\memory\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x710:
        OS_SPrintf(buffer, "%d", value / 4);
        result = gpiSendUserInfo(connection, "\\videocard1ram\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x712:
        OS_SPrintf(buffer, "%d", value / 4);
        result = gpiSendUserInfo(connection, "\\videocard2ram\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x713:
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendUserInfo(connection, "\\connectionid\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x714:
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendUserInfo(connection, "\\connectionspeed\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x715:
        if (value != 0) {
            value = 1;
        }
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendUserInfo(connection, "\\hasnetwork\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x718:
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendLocalInfo(connection, "\\pic\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x719:
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendLocalInfo(connection, "\\occ\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x71a:
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendLocalInfo(connection, "\\ind\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x71b:
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendLocalInfo(connection, "\\inc\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x71c:
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendLocalInfo(connection, "\\mar\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x71d:
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendLocalInfo(connection, "\\chc\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x71e:
        OS_SPrintf(buffer, "%d", value);
        result = gpiSendLocalInfo(connection, "\\i1\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    default:
        gpiSetErrorString(connection, "Invalid info.");
        return 2;
    }
    return 0;
}

GPResult gpiSetInfos(GPConnection *connection, GPEnum info, const char *value) {
    GPIConnection *iconnection = *connection;
    char passwordEnc[45];
    char buffer[256];
    char sex;
    int result;

    if (value == NULL) {
        gpiSetErrorString(connection, "Invalid value.");
        return 2;
    }
    switch (info) {
    case 0x700:
        if (value[0] == '\0') {
            gpiSetErrorString(connection, "Invalid value.");
            return 2;
        }
        strzcpy(buffer, value, 31);
        strzcpy(iconnection->nick, buffer, 31);
        result = gpiSendLocalInfo(connection, "\\nick\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x701:
        if (value[0] == '\0') {
            gpiSetErrorString(connection, "Invalid value.");
            return 2;
        }
        strzcpy(buffer, value, 21);
        strzcpy(iconnection->uniquenick, buffer, 21);
        result = gpiSendLocalInfo(connection, "\\uniquenick\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x702:
        if (value[0] == '\0') {
            gpiSetErrorString(connection, "Invalid value.");
            return 2;
        }
        strzcpy(buffer, value, 51);
        _strlwr(buffer);
        strzcpy(iconnection->email, buffer, 51);
        result = gpiSendUserInfo(connection, "\\email\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x703:
        if (value[0] == '\0') {
            gpiSetErrorString(connection, "Invalid value.");
            return 2;
        }
        strzcpy(buffer, value, 31);
        strzcpy(iconnection->password, buffer, 31);
        gpiEncodeString(iconnection->password, passwordEnc);
        result = gpiSendUserInfo(connection, "\\passwordenc\\", passwordEnc);
        if (result != 0) {
            return result;
        }
        break;
    case 0x704:
        strzcpy(buffer, value, 31);
        result = gpiSendLocalInfo(connection, "\\firstname\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x705:
        strzcpy(buffer, value, 31);
        result = gpiSendLocalInfo(connection, "\\lastname\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x707:
        strzcpy(buffer, value, 76);
        result = gpiSendLocalInfo(connection, "\\homepage\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x708:
        strzcpy(buffer, value, 11);
        result = gpiSendLocalInfo(connection, "\\zipcode\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x709:
        if (strlen(value) != 2) {
            gpiSetErrorString(connection, "Invalid countrycode.");
            return 2;
        }
        strzcpy(buffer, value, 3);
        result = gpiSendLocalInfo(connection, "\\countrycode\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x70b:
        sex = toupper(value[0]);
        if (sex == 'M') {
            buffer[0] = '0';
        } else if (sex == 'F') {
            buffer[0] = '1';
        } else {
            buffer[0] = '2';
        }
        buffer[1] = '\0';
        result = gpiSendLocalInfo(connection, "\\sex\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x706:
        gsiSafeStrcpyA(buffer, value, 256);
        result = gpiSendLocalInfo(connection, "\\icquin\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x70d:
        result = gpiSetInfoi(connection, 0x70d, atol(value));
        if (result != 0) {
            return result;
        }
        break;
    case 0x70e:
        result = gpiSetInfoi(connection, 0x70e, atol(value));
        if (result != 0) {
            return result;
        }
        break;
    case 0x70f:
        strzcpy(buffer, value, 256);
        result = gpiSendLocalInfo(connection, "\\videocard1string\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x710:
        result = gpiSetInfoi(connection, 0x710, atol(value));
        if (result != 0) {
            return result;
        }
        break;
    case 0x711:
        strzcpy(buffer, value, 256);
        result = gpiSendLocalInfo(connection, "\\videocard2string\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x712:
        result = gpiSetInfoi(connection, 0x712, atol(value));
        if (result != 0) {
            return result;
        }
        break;
    case 0x714:
        result = gpiSetInfoi(connection, 0x714, atol(value));
        if (result != 0) {
            return result;
        }
        break;
    case 0x715:
        result = gpiSetInfoi(connection, 0x715, atol(value));
        if (result != 0) {
            return result;
        }
        break;
    case 0x716:
        strzcpy(buffer, value, 256);
        result = gpiSendLocalInfo(connection, "\\osstring\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x717:
        strzcpy(buffer, value, 51);
        result = gpiSendLocalInfo(connection, "\\aim\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x718:
        strzcpy(buffer, value, 256);
        result = gpiSendLocalInfo(connection, "\\pic\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x719:
        strzcpy(buffer, value, 256);
        result = gpiSendLocalInfo(connection, "\\occ\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x71a:
        strzcpy(buffer, value, 256);
        result = gpiSendLocalInfo(connection, "\\ind\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x71b:
        strzcpy(buffer, value, 256);
        result = gpiSendLocalInfo(connection, "\\inc\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x71c:
        strzcpy(buffer, value, 256);
        result = gpiSendLocalInfo(connection, "\\mar\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x71d:
        strzcpy(buffer, value, 256);
        result = gpiSendLocalInfo(connection, "\\chc\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    case 0x71e:
        strzcpy(buffer, value, 256);
        result = gpiSendLocalInfo(connection, "\\i1\\", buffer);
        if (result != 0) {
            return result;
        }
        break;
    default:
        gpiSetErrorString(connection, "Invalid info.");
        return 2;
    }
    return 0;
}

GPResult gpiSendGetInfo(GPConnection *connection, int profileid, int operationid) {
    GPIConnection *iconnection = *connection;

    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\getprofile\\\\sesskey\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\profileid\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, profileid);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\id\\");
    gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, operationid);
    gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
    return 0;
}

// Gives the info from the cache when it is asked for (checkCache 1) and there, and asks the server otherwise
GPResult gpiGetInfo(GPConnection *connection, GPProfile profile, GPEnum checkCache, GPEnum blocking,
                    GPCallback callback, void *param) {
    GPIConnection *iconnection = *connection;
    GPIProfile *pProfile;
    GPIOperation *operation = NULL;
    GPGetInfoResponseArg *arg;
    GPICallback gpiCallback;
    int id;
    int result;

    gsi_bool useCache = gsi_false;

    if (checkCache == 1) {
        useCache = gsi_true;
    }
    if (!iconnection->infoCaching) {
        useCache = gsi_false;
    }
    if (callback != NULL && useCache && gpiGetProfile(connection, profile, &pProfile) && pProfile->cache != NULL) {
        arg = gsimalloc(sizeof(GPGetInfoResponseArg));
        if (arg == NULL) {
            gpiSetErrorString(connection, "Out of memory.");
            return 1;
        }
        gpiInfoCacheToArg(pProfile->cache, arg);
        arg->result = 0;
        arg->profile = profile;
        gpiCallback.callback = callback;
        gpiCallback.param = param;
        result = gpiAddOperation(connection, GPI_GET_INFO, NULL, &operation, gsi_true, callback, param);
        if (result != 0) {
            return result;
        }
        id = operation->id;
        result = gpiAddCallback(connection, gpiCallback, arg, operation, GPI_ADD_NORMAL);
        if (result != 0) {
            return result;
        }
        gpiRemoveOperation(connection, operation);
    } else {
        result = gpiAddOperation(connection, GPI_GET_INFO, NULL, &operation, blocking, callback, param);
        if (result != 0) {
            return result;
        }
        id = operation->id;
        result = gpiSendGetInfo(connection, profile, id);
        if (result != 0) {
            return result;
        }
    }
    if (blocking) {
        result = gpiProcess(connection, id);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

static gsi_bool gpiSetInfoCache(GPConnection *connection, GPIProfile *profile, const GPIInfoCache *cache) {
    GPIConnection *iconnection = *connection;

    if (!iconnection->infoCaching) {
        return gsi_true;
    }
    gpiFreeInfoCache(profile);
    profile->cache = gsimalloc(sizeof(GPIInfoCache));
    if (profile->cache != NULL) {
        *profile->cache = *cache;
        profile->cache->nick = goastrdup(cache->nick);
        profile->cache->uniquenick = goastrdup(cache->uniquenick);
        profile->cache->email = goastrdup(cache->email);
        profile->cache->firstname = goastrdup(cache->firstname);
        profile->cache->lastname = goastrdup(cache->lastname);
        profile->cache->homepage = goastrdup(cache->homepage);
        profile->cache->aimname = goastrdup(cache->aimname);
    }
    if (profile->cache != NULL) {
        return gsi_true;
    }
    return gsi_false;
}

void gpiFreeInfoCache(GPIProfile *profile) {
    if (profile->cache != NULL) {
        gsifree(profile->cache->nick);
        profile->cache->nick = NULL;
        gsifree(profile->cache->uniquenick);
        profile->cache->uniquenick = NULL;
        gsifree(profile->cache->email);
        profile->cache->email = NULL;
        gsifree(profile->cache->firstname);
        profile->cache->firstname = NULL;
        gsifree(profile->cache->lastname);
        profile->cache->lastname = NULL;
        gsifree(profile->cache->homepage);
        profile->cache->homepage = NULL;
        gsifree(profile->cache->aimname);
        profile->cache->aimname = NULL;
        gsifree(profile->cache);
        profile->cache = NULL;
    }
}
