#ifndef POKEBW2_DWC_GPITRANSFER_H
#define POKEBW2_DWC_GPITRANSFER_H

#include "types.h"
#include "dwc/gpi.h"

// What is left of the file transfers of GameSpy's GP (gpiTransfer.c) in overlay 11: every transfer that a peer asks
// for is refused (see dwc/gp.h for the names)

void gpiHandleTransferMessage(GPConnection *connection, GPIPeer *peer, int type, const char *headers,
                              const char *buffer, int len);

#endif // POKEBW2_DWC_GPITRANSFER_H
