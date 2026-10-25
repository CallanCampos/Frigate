#ifndef POKEBW2_DWC_QR2REGKEYS_H
#define POKEBW2_DWC_QR2REGKEYS_H

#include "dwc/nonport.h"

// GameSpy's QR2 key names (qr2regkeys.c), in overlay 11. A key is a number, and this table gives the name that the
// number has in a query reply: 1 to 32 are GameSpy's own (31 and 32, "country" and "region", only come from the master
// server), and qr2_register_keyA fills in a game's, from 50 up
#define MAX_REGISTERED_KEYS 254

extern const char *qr2_registered_key_list[MAX_REGISTERED_KEYS];

// Keep and free the key names that qr2_register_keyW allocates. Reconstructed from GameSpy's SDK (UniSpySDK's copy):
// the original link dead-stripped both (config/deadstrip.toml)
void qr2_internal_key_list_append(char *theKey);
void qr2_internal_key_list_free(void);

// Whether a key's name is one that only the master server gives, the country's or the region's
gsi_bool qr2_internal_is_master_only_key(const char *key);
void qr2_register_keyA(int keyid, const char *key);

#endif // POKEBW2_DWC_QR2REGKEYS_H
