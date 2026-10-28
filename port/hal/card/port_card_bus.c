// NTR CARD wire timing only, for the ROM/ID boundary used by lib/nitro/src/card/card_rom.c.
// Independently expressed from pinned melonDS 1.1 NDSCart.cpp's ready/consume/end rules; see this directory's guide.
// Not integrated with card_rom.c: no CPU work, page cache, DMA, scheduler or SDK task/callback behavior is supplied.
#include "hal/card/port_card_bus.h"
#include <string.h>

static bool Add(PortCardCycle now, uint64_t delay, PortCardCycle *deadline) {
    if (UINT64_MAX - now < delay) {
        return false;
    }
    *deadline = now + delay;
    return true;
}
static bool Size(uint32_t length) {
    return length == 0 || length == 4 || (length >= 512 && length <= 16384 && (length & (length - 1)) == 0);
}
void port_card_bus_init(PortCardBus *bus, PortCardBusBackend backend) {
    memset(bus, 0, sizeof(*bus));
    bus->backend = backend;
}
void port_card_bus_reset(PortCardBus *bus) {
    PortCardBusBackend backend = bus->backend;
    port_card_bus_init(bus, backend);
}
void port_card_bus_cancel(PortCardBus *bus) {
    bus->busy = false;
    bus->ready = false;
    bus->deadline = 0;
}
void port_card_bus_ack(PortCardBus *bus) {
    bus->irq = false;
}
static void End(PortCardBus *bus) {
    bus->busy = false;
    bus->ready = false;
    bus->deadline = 0;
    if (bus->request.transfer_irq) {
        bus->irq = true;
    }
}
PortCardBusResult port_card_bus_start(PortCardBus *bus, const PortCardBusRequest *request, PortCardCycle now) {
    PortCardCycle deadline;
    uint64_t delay;
    if (bus->busy) {
        return PORT_CARD_BUS_BUSY;
    }
    if (now < bus->now) {
        return PORT_CARD_BUS_TIME;
    }
    if (request == NULL || (request->command != PORT_CARD_BUS_READ && request->command != PORT_CARD_BUS_ID) ||
        !Size(request->length) || request->latency1 > 8191 || request->latency2 > 63 ||
        (request->cycles_per_byte != 5 && request->cycles_per_byte != 8) ||
        (request->command == PORT_CARD_BUS_ID && (request->length != 4 || bus->backend.id == NULL)) ||
        (request->command == PORT_CARD_BUS_READ && request->length != 0 &&
         (bus->backend.read == NULL || request->address > UINT32_MAX - (request->length - 1)))) {
        return PORT_CARD_BUS_INVALID;
    }
    delay = 8 + request->latency1;
    if (request->length != 0) {
        delay += request->latency2 + 4;
    }
    if (!Add(now, delay * request->cycles_per_byte, &deadline)) {
        return PORT_CARD_BUS_TIME;
    }
    bus->request = *request;
    bus->now = now;
    bus->deadline = deadline;
    bus->consumed = 0;
    bus->busy = true;
    bus->ready = false;
    return PORT_CARD_BUS_OK;
}
PortCardBusResult port_card_bus_advance(PortCardBus *bus, PortCardCycle now) {
    uint32_t value;
    if (now < bus->now) {
        return PORT_CARD_BUS_TIME;
    }
    bus->now = now;
    if (!bus->busy || bus->ready || now < bus->deadline) {
        return PORT_CARD_BUS_OK;
    }
    if (bus->request.length == 0) {
        End(bus);
        return PORT_CARD_BUS_OK;
    }
    if (bus->request.command == PORT_CARD_BUS_ID) {
        if (!bus->backend.id(bus->backend.context, &value)) {
            port_card_bus_cancel(bus);
            return PORT_CARD_BUS_IO;
        }
    } else {
        uint8_t bytes[4];
        if (!bus->backend.read(bus->backend.context, bus->request.address + bus->consumed, bytes)) {
            port_card_bus_cancel(bus);
            return PORT_CARD_BUS_IO;
        }
        value = (uint32_t)bytes[0] | (uint32_t)bytes[1] << 8 | (uint32_t)bytes[2] << 16 | (uint32_t)bytes[3] << 24;
    }
    bus->latch = value;
    bus->ready = true;
    return PORT_CARD_BUS_OK;
}
PortCardBusResult port_card_bus_read(PortCardBus *bus, PortCardCycle now, uint32_t *value) {
    PortCardBusResult result;
    PortCardCycle deadline = 0;
    if (value == NULL) {
        return PORT_CARD_BUS_INVALID;
    }
    result = port_card_bus_advance(bus, now);
    if (result != PORT_CARD_BUS_OK) {
        return result;
    }
    *value = bus->latch;
    if (!bus->ready) {
        return PORT_CARD_BUS_OK;
    }
    if (bus->consumed + 4 < bus->request.length) {
        uint64_t delay = 4;
        if ((bus->consumed + 4) % 512 == 0) {
            delay += bus->request.latency2;
        }
        if (!Add(now, delay * bus->request.cycles_per_byte, &deadline)) {
            return PORT_CARD_BUS_TIME; // Leave the ready word unconsumed so caller can cancel explicitly.
        }
    }
    bus->consumed += 4;
    bus->ready = false;
    if (bus->consumed == bus->request.length) {
        End(bus);
    } else {
        bus->deadline = deadline;
    }
    return PORT_CARD_BUS_OK;
}
