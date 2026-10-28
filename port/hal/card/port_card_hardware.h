// Experimental physical CARD_READY/DMA_COMMIT events for the native OS clock, serving NitroSDK CARD/MI DMA.
// Test-activated CPU/DMA ownership: production card_rom.c remains unchanged. CPU costs/SDK integration are separate.
// Both adapters use this one bus. CPU data consumption publishes transfer-end IF before the caller's memory store.
#ifndef PORT_CARD_HARDWARE_H
#define PORT_CARD_HARDWARE_H
#include "hal/card/port_card_bus.h"

typedef enum { PORT_CARD_HW_NONE, PORT_CARD_HW_READY, PORT_CARD_HW_COMMIT } PortCardHardwareEvent;
typedef enum { PORT_CARD_OWNER_NONE, PORT_CARD_OWNER_CPU, PORT_CARD_OWNER_DMA } PortCardHardwareOwner;
typedef struct {
    PortCardHardwareEvent event;
    PortCardHardwareOwner owner;
    uint64_t deadline, last_ready, last_commit;
    uint64_t generation;
    uint32_t commits;
    bool busy, ready, stalled;
    PortCardBusResult result;
} PortCardHardwareState;
// Configure only an idle controller; never replace an active command or armed DMA destination.
bool port_card_hardware_configure(PortCardBusBackend backend);
// CPU commands use the actual OS clock. Tokens cannot survive cancel/reset or authorize a later command.
// Backend failure cancels at READY, invalidates the token and retains IO in state.result; it raises no completion IF.
PortCardBusResult port_card_hardware_start_cpu(const PortCardBusRequest *request, uint64_t *generation);
PortCardBusResult port_card_hardware_read_cpu(uint64_t generation, uint32_t *value);
bool port_card_hardware_cancel_cpu(uint64_t generation);
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
