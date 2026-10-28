// Standalone NTR read/ID wire controller for NitroSDK CARD ROM reads (lib/nitro/src/card/card_rom.c).
// Caller supplies absolute device cycles and immutable card bytes; no SDK task, DMA, CPU cost or IRQ delivery yet.
#ifndef PORT_CARD_BUS_H
#define PORT_CARD_BUS_H
#include <stdbool.h>
#include <stdint.h>

typedef uint64_t PortCardCycle;
typedef enum { PORT_CARD_BUS_READ, PORT_CARD_BUS_ID } PortCardBusCommand;
typedef enum {
    PORT_CARD_BUS_OK,
    PORT_CARD_BUS_INVALID,
    PORT_CARD_BUS_BUSY,
    PORT_CARD_BUS_TIME,
    PORT_CARD_BUS_IO
} PortCardBusResult;

// read supplies exactly four little-endian wire bytes, including the ROM backend's address-remapping policy.
// Callbacks must not reenter/mutate this controller. Their context remains a native pointer, never a DS address.
typedef struct {
    void *context;
    bool (*read)(void *context, uint32_t address, uint8_t bytes[4]);
    bool (*id)(void *context, uint32_t *value);
} PortCardBusBackend;
typedef struct {
    PortCardBusCommand command;
    uint32_t address;
    uint32_t length;         // ROMCnt sizes: 0, 4, 512, 1024, 2048, 4096, 8192, 16384. ID requires 4.
    uint16_t latency1;       // 0..8191; SDK ID adapter supplies zero (the controller does not override it).
    uint8_t latency2;        // 0..63; before first word and at each 512-byte boundary.
    uint8_t cycles_per_byte; // 5 or 8 normalized system cycles.
    bool transfer_irq;
} PortCardBusRequest;
typedef struct {
    PortCardBusBackend backend;
    PortCardBusRequest request;
    PortCardCycle now, deadline;
    uint32_t consumed, latch;
    bool busy, ready, irq;
} PortCardBus;

void port_card_bus_init(PortCardBus *bus, PortCardBusBackend backend);
// reset clears the latch, pending IRQ and command, retaining the backend; cancel retains latch/latched IRQ.
void port_card_bus_reset(PortCardBus *bus);
void port_card_bus_cancel(PortCardBus *bus);
void port_card_bus_ack(PortCardBus *bus);
PortCardBusResult port_card_bus_start(PortCardBus *bus, const PortCardBusRequest *request, PortCardCycle now);
PortCardBusResult port_card_bus_advance(PortCardBus *bus, PortCardCycle now);
// A read before READY returns the old latch without consuming. Check ready before calling if consumption matters.
// Consuming the last word clears busy and optionally latches irq; this never invokes an SDK callback/handler.
PortCardBusResult port_card_bus_read(PortCardBus *bus, PortCardCycle now, uint32_t *value);
#endif
