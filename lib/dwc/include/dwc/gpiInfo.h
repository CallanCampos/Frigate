#ifndef POKEBW2_DWC_GPIINFO_H
#define POKEBW2_DWC_GPIINFO_H

#include "types.h"
#include "dwc/gpi.h"

// The info of the profiles of GameSpy's GP (gpiInfo.c), in overlay 11: the server's answers, the cache of them and the
// changes to the player's own (see dwc/gp.h for the names)

int gpiProcessGetInfo(GPConnection *connection, GPIOperation *operation, const char *input);
// Adds the changes waiting in the update buffers to a message
int gpiAddLocalInfo(GPConnection *connection, GPIBuffer *buffer);
int gpiSetInfos(GPConnection *connection, int info, const char *value);
int gpiSendGetInfo(GPConnection *connection, int profileid, int operationid);
int gpiGetInfo(GPConnection *connection, int profile, int checkCache, int blocking, GPCallback callback, void *param);
void gpiFreeInfoCache(GPIProfile *profile);

#endif // POKEBW2_DWC_GPIINFO_H
