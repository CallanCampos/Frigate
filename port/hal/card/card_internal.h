// What the files of port/hal/card share, in the SDK's types. Nothing outside port/hal/card includes it but its tests
// (port/tests/rom).
#ifndef PORT_HAL_CARD_CARD_INTERNAL_H
#define PORT_HAL_CARD_CARD_INTERNAL_H

#include "nitro/card.h"

// card_backup.c's page buffer, which every backup command moves its data through (card_backup_host.c)
u8 *port_card_page_buffer(void);
#define PORT_CARD_PAGE_SIZE 256

#endif // PORT_HAL_CARD_CARD_INTERNAL_H
