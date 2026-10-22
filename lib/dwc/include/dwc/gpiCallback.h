#ifndef POKEBW2_DWC_GPICALLBACK_H
#define POKEBW2_DWC_GPICALLBACK_H

#include "types.h"
#include "dwc/gpi.h"

// The callbacks of GameSpy's GP that wait to be called (gpiCallback.c), in overlay 11 (see dwc/gp.h for the names)

void gpiCallErrorCallback(GPConnection *connection, GPResult result, GPEnum fatal);
GPResult gpiAddCallback(GPConnection *connection, GPICallback callback, void *arg, const GPIOperation *operation,
                        int type);
// Calls the callbacks of an operation (and the errors), or all of them for an id of 0
GPResult gpiProcessCallbacks(GPConnection *connection, int blockingOperationID);

#endif // POKEBW2_DWC_GPICALLBACK_H
