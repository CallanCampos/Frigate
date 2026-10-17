#ifndef POKEBW2_DWC_GPIUNIQUE_H
#define POKEBW2_DWC_GPIUNIQUE_H

#include "types.h"
#include "dwc/gpi.h"

// The answers of the server to the registrations of a unique nick and of a CD key, in GameSpy's GP (gpiUnique.c), in
// overlay 11 (see dwc/gp.h for the names)

int gpiProcessRegisterUniqueNick(GPConnection *connection, GPIOperation *operation, const char *input);
int gpiProcessRegisterCdKey(GPConnection *connection, GPIOperation *operation, const char *input);

#endif // POKEBW2_DWC_GPIUNIQUE_H
