// Experimental physical CARD_READY/DMA_COMMIT events for the native OS clock, serving NitroSDK CARD/MI DMA.
// TEST-ONLY activation: production card_rom.c remains unchanged. CPU wire timing/SDK DMA/polling are separate WIP.
// Future CPU and DMA integration must share this one bus, not create independent physical cartridges.
#ifndef PORT_CARD_HARDWARE_H
#define PORT_CARD_HARDWARE_H
#include "hal/card/port_card_bus.h"

typedef enum { PORT_CARD_HW_NONE, PORT_CARD_HW_READY, PORT_CARD_HW_COMMIT } PortCardHardwareEvent;
typedef struct {
    PortCardHardwareEvent event;
    uint64_t deadline, last_ready, last_commit;
    uint32_t commits;
    bool busy, ready, stalled;
    PortCardBusResult result;
} PortCardHardwareState;
// Test activation supplies immutable data only; the shared PortCardBusRequest is the future common CPU/DMA command.
void port_card_hardware_test_init(PortCardBusBackend backend);
PortCardBusResult port_card_hardware_test_start(const PortCardBusRequest *request, uint64_t now);
void port_card_hardware_reset(void);
void port_card_hardware_cancel(void);
PortCardHardwareState port_card_hardware_state(void);
// Core's closed physical-event dispatch; no SDK/host callback is accepted or invoked here.
uint64_t port_card_hardware_deadline(void);
void port_card_hardware_event(uint64_t now);
bool port_card_hardware_stalled(void);
#endif
