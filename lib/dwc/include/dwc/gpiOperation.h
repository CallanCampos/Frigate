#ifndef POKEBW2_DWC_GPIOPERATION_H
#define POKEBW2_DWC_GPIOPERATION_H

#include "types.h"
#include "dwc/gpi.h"

// The operations in progress of GameSpy's GP (gpiOperation.c), in overlay 11 (see dwc/gp.h for the names)

// Calls the app's callback of an operation that failed, with only the result
int gpiFailedOpCallback(GPConnection *connection, const GPIOperation *operation);
int gpiAddOperation(GPConnection *connection, int type, void *data, GPIOperation **op, BOOL blocking,
                    GPCallback callback, void *param);
void gpiRemoveOperation(GPConnection *connection, GPIOperation *operation);
BOOL gpiFindOperationByID(const GPConnection *connection, GPIOperation **operation, int id);
BOOL gpiOperationsAreBlocking(const GPConnection *connection);
int gpiProcessOperation(GPConnection *connection, GPIOperation *operation, const char *input);

#endif // POKEBW2_DWC_GPIOPERATION_H
