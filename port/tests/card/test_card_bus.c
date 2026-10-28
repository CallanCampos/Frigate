// Synthetic NTR wire-controller checks for lib/nitro CARD ROM/ID timing, without game/SDK substitutes or a ROM.
// Expected deadlines are arithmetic from command bytes, word bytes and L1/L2 gaps; test backend stays native.
#include "hal/card/port_card_bus.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x)                                                                                                       \
    do {                                                                                                               \
        if (!(x)) {                                                                                                    \
            fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x);                                                    \
            exit(1);                                                                                                   \
        }                                                                                                              \
    } while (0)
typedef struct {
    unsigned reads, ids;
    bool fail;
    uint32_t last;
} Backend;
static bool Read(void *context, uint32_t address, uint8_t bytes[4]) {
    Backend *b = context;
    ++b->reads;
    b->last = address;
    for (unsigned i = 0; i < 4; ++i) {
        bytes[i] = (uint8_t)(address + i);
    }
    return !b->fail;
}
static bool Id(void *context, uint32_t *value) {
    Backend *b = context;
    ++b->ids;
    *value = 0x1234abcd;
    return !b->fail;
}
static PortCardBusRequest Request(uint32_t length, unsigned clock, unsigned l1, unsigned l2) {
    PortCardBusRequest r = { PORT_CARD_BUS_READ, 0x1000, length, (uint16_t)l1, (uint8_t)l2, (uint8_t)clock, true };
    return r;
}
static void Transfers(void) {
    bool highPointer = false;
    uint32_t lengths[] = { 0, 4, 512, 1024, 2048, 4096, 8192, 16384 };
    for (unsigned clock = 5; clock <= 8; clock += 3) {
        for (unsigned gap = 0; gap < 2; ++gap) {
            for (unsigned n = 0; n < sizeof(lengths) / sizeof(*lengths); ++n) {
                Backend *b = calloc(1, sizeof(*b));
                CHECK(b != NULL);
                highPointer |= (uintptr_t)b > UINT32_MAX;
                PortCardBus bus;
                port_card_bus_init(&bus, (PortCardBusBackend){ b, Read, Id });
                PortCardBusRequest r = Request(lengths[n], clock, gap ? 8191 : 0, gap ? 63 : 0);
                uint64_t t = 100 + clock * (8 + r.latency1 + (r.length ? r.latency2 + 4 : 0));
                CHECK(port_card_bus_start(&bus, &r, 100) == PORT_CARD_BUS_OK);
                CHECK(bus.busy && !bus.ready && !bus.irq && bus.deadline == t);
                uint32_t word = 99;
                CHECK(port_card_bus_read(&bus, t - 1, &word) == PORT_CARD_BUS_OK);
                CHECK(word == 0 && bus.consumed == 0 && b->reads == 0);
                if (!r.length) {
                    CHECK(port_card_bus_advance(&bus, t) == PORT_CARD_BUS_OK);
                    CHECK(!bus.busy && bus.irq && !bus.ready && !b->reads);
                }
                for (uint32_t offset = 0; offset < r.length; offset += 4) {
                    CHECK(bus.deadline == t);
                    CHECK(port_card_bus_advance(&bus, t) == PORT_CARD_BUS_OK);
                    CHECK(bus.ready && bus.busy && !bus.irq);
                    CHECK(b->reads == offset / 4 + 1 && b->last == r.address + offset);
                    // READY waits for consumption, even across many would-be word deadlines.
                    t += 37;
                    CHECK(port_card_bus_advance(&bus, t) == PORT_CARD_BUS_OK);
                    CHECK(bus.ready && b->reads == offset / 4 + 1);
                    CHECK(port_card_bus_read(&bus, t, &word) == PORT_CARD_BUS_OK);
                    uint32_t a = r.address + offset;
                    CHECK(word == ((a & 255) | ((a + 1) & 255) << 8 | ((a + 2) & 255) << 16 | ((a + 3) & 255) << 24));
                    CHECK(bus.consumed == offset + 4 && !bus.ready);
                    if (offset + 4 < r.length) {
                        t += clock * (4 + ((offset + 4) % 512 == 0 ? r.latency2 : 0));
                    }
                }
                CHECK(!bus.busy && bus.irq);
                port_card_bus_ack(&bus);
                CHECK(!bus.irq);
                free(b);
            }
        }
    }
    printf("high native backend pointer exercised: %s\n",
           highPointer ? "yes" : "no (allocator returned low addresses)");
}
static void Lifecycle(void) {
    Backend b = { 0 };
    PortCardBus bus;
    port_card_bus_init(&bus, (PortCardBusBackend){ &b, Read, Id });
    PortCardBusRequest r = Request(4, 5, 0, 1);
    r.command = PORT_CARD_BUS_ID;
    r.transfer_irq = false;
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_OK && bus.deadline == 65);
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_BUSY);
    uint32_t word;
    CHECK(port_card_bus_read(&bus, 65, &word) == PORT_CARD_BUS_OK && word == 0x1234abcd);
    CHECK(!bus.busy && !bus.irq && b.ids == 1 && b.reads == 0);
    CHECK(port_card_bus_read(&bus, 66, &word) == PORT_CARD_BUS_OK && word == 0x1234abcd && b.ids == 1);
    r.latency1 = 7;
    CHECK(port_card_bus_start(&bus, &r, 66) == PORT_CARD_BUS_OK && bus.deadline == 166);
    port_card_bus_cancel(&bus);
    CHECK(port_card_bus_advance(&bus, 200) == PORT_CARD_BUS_OK && b.ids == 1 && bus.latch == 0x1234abcd);
    CHECK(port_card_bus_advance(&bus, 199) == PORT_CARD_BUS_TIME);
    port_card_bus_reset(&bus);
    CHECK(!bus.busy && !bus.ready && !bus.irq && !bus.latch && !bus.now);
    r = Request(512, 5, 1623, 1);
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_OK && bus.deadline == 8180);
    // Known ideal B7 duration, if every word is consumed exactly when ready: 8180 +127*20 =10720.
    for (unsigned i = 0; i < 128; ++i) {
        CHECK(port_card_bus_read(&bus, 8180 + i * 20, &word) == PORT_CARD_BUS_OK);
    }
    CHECK(!bus.busy && bus.irq && bus.now == 10720);
    port_card_bus_cancel(&bus);
    CHECK(bus.irq); // cancellation must not acknowledge an already latched completion.
    port_card_bus_reset(&bus);
    b.fail = true;
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_OK);
    CHECK(port_card_bus_advance(&bus, 8180) == PORT_CARD_BUS_IO);
    CHECK(!bus.busy && !bus.ready && !bus.irq);
}
static void Invalid(void) {
    Backend b = { 0 };
    PortCardBus bus;
    port_card_bus_init(&bus, (PortCardBusBackend){ &b, Read, Id });
    PortCardBusRequest r = Request(8, 5, 0, 0);
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_INVALID);
    r = Request(4, 6, 0, 0);
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_INVALID);
    r = Request(4, 5, 8192, 0);
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_INVALID);
    r = Request(4, 5, 0, 64);
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_INVALID);
    r = Request(512, 5, 0, 0);
    r.command = PORT_CARD_BUS_ID;
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_INVALID);
    r.command = (PortCardBusCommand)99;
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_INVALID);
    r = Request(4, 5, 0, 0);
    r.address = UINT32_MAX - 2;
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_INVALID);
    r = Request(512, 5, 0, 0);
    CHECK(port_card_bus_start(&bus, &r, UINT64_MAX - 1) == PORT_CARD_BUS_TIME && !bus.busy);
    CHECK(port_card_bus_start(&bus, &r, UINT64_MAX - 60) == PORT_CARD_BUS_OK);
    CHECK(port_card_bus_advance(&bus, UINT64_MAX) == PORT_CARD_BUS_OK);
    uint32_t word;
    CHECK(port_card_bus_read(&bus, UINT64_MAX, &word) == PORT_CARD_BUS_TIME);
    CHECK(bus.ready && bus.consumed == 0 && bus.busy);
    CHECK(port_card_bus_read(&bus, UINT64_MAX, NULL) == PORT_CARD_BUS_INVALID);
    port_card_bus_reset(&bus);
    r = Request(4, 5, 0, 0);
    r.command = PORT_CARD_BUS_ID;
    b.fail = true;
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_OK);
    CHECK(port_card_bus_read(&bus, 60, &word) == PORT_CARD_BUS_IO);
    CHECK(!bus.busy && !bus.irq);
    port_card_bus_reset(&bus);
    bus.backend.id = NULL;
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_INVALID);
    r = Request(512, 5, 0, 0);
    bus.backend.read = NULL;
    CHECK(port_card_bus_start(&bus, &r, 0) == PORT_CARD_BUS_INVALID);
}
int main(void) {
    Transfers();
    Lifecycle();
    Invalid();
    puts("card bus PASS");
    return 0;
}
