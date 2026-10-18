#ifndef POKEBW2_DWC_GPIPROFILE_H
#define POKEBW2_DWC_GPIPROFILE_H

#include "types.h"
#include "dwc/gpi.h"

// The profiles of GameSpy's GP (gpiProfile.c), in overlay 11, in a hash table by their ids (see dwc/gp.h for the names)

// What gpiProfileMap calls for each profile until it returns FALSE
typedef BOOL (*GPIProfileMapFunc)(GPConnection *connection, GPIProfile *profile, void *data);

BOOL gpiInitProfiles(GPConnection *connection);
int gpiProcessNewProfile(GPConnection *connection, GPIOperation *operation, const char *input);
GPIProfile *gpiProfileListAdd(GPConnection *connection, int id);
BOOL gpiGetProfile(GPConnection *connection, int profileid, GPIProfile **profile);
int gpiProcessDeleteProfle(GPConnection *connection, GPIOperation *operation, const char *input);
void gpiRemoveProfileByID(GPConnection *connection, int profileid);
void gpiRemoveProfile(GPConnection *connection, GPIProfile *profile);
// Finds the cached profile of a nick and an e-mail, or NULL
int gpiFindProfileByUser(GPConnection *connection, const char *nick, const char *email, GPIProfile **profile);
// TRUE when the function went through every profile
BOOL gpiProfileMap(GPConnection *connection, GPIProfileMapFunc func, void *data);
GPIProfile *gpiFindBuddy(GPConnection *connection, int buddyIndex);
void gpiRemoveBuddyStatus(GPIBuddyStatus *buddyStatus);
void gpiRemoveBuddyStatusInfo(GPIBuddyStatusInfo *buddyStatusInfo);
BOOL gpiCanFreeProfile(GPIProfile *profile);
// Handles the server's "\\blk\\" list of the blocked players
int gpiProcessRecvBlockedList(GPConnection *connection, const char *input);

#endif // POKEBW2_DWC_GPIPROFILE_H
