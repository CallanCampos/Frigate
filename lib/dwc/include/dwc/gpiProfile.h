#ifndef POKEBW2_DWC_GPIPROFILE_H
#define POKEBW2_DWC_GPIPROFILE_H

#include "dwc/gpi.h"
#include "dwc/nonport.h"

// The profiles of GameSpy's GP (gpiProfile.c), in overlay 11, in a hash table by their ids (see dwc/gp.h for the names)

// What gpiProfileMap calls for each profile until it returns gsi_false
typedef GPIBool (*gpiProfileMapFunc)(GPConnection *connection, GPIProfile *profile, void *data);

GPIBool gpiInitProfiles(GPConnection *connection);
GPResult gpiProcessNewProfile(GPConnection *connection, GPIOperation *operation, const char *input);
GPIProfile *gpiProfileListAdd(GPConnection *connection, int id);
GPIBool gpiGetProfile(GPConnection *connection, GPProfile profileid, GPIProfile **profile);
GPResult gpiProcessDeleteProfle(GPConnection *connection, GPIOperation *operation, const char *input);
void gpiRemoveProfileByID(GPConnection *connection, int profileid);
void gpiRemoveProfile(GPConnection *connection, GPIProfile *profile);
// Finds the cached profile of a nick and an e-mail, or NULL
GPResult gpiFindProfileByUser(GPConnection *connection, char nick[GP_NICK_LEN], char email[GP_EMAIL_LEN],
                              GPIProfile **profile);
// gsi_true when the function went through every profile
GPIBool gpiProfileMap(GPConnection *connection, gpiProfileMapFunc func, void *data);
GPIProfile *gpiFindBuddy(GPConnection *connection, int buddyIndex);
void gpiRemoveBuddyStatus(GPIBuddyStatus *buddyStatus);
void gpiRemoveBuddyStatusInfo(GPIBuddyStatusInfo *buddyStatusInfo);
GPIBool gpiCanFreeProfile(GPIProfile *profile);
// Handles the server's "\\blk\\" list of the blocked players
GPResult gpiProcessRecvBlockedList(GPConnection *connection, const char *input);

#endif // POKEBW2_DWC_GPIPROFILE_H
