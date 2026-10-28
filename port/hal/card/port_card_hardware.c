// Physical READY/one-word COMMIT events beneath NitroSDK CARD/MI DMA, dispatched directly by hal/os/core.c.
// Test-activated CPU/DMA ownership: production card_rom.c remains unchanged. CPU costs/SDK adapters remain separate.
// Immutable byte/ID callbacks supply device data only; this boundary never runs SDK tasks, IRQ handlers or host work.
#include "hal/card/port_card_hardware.h"
#include "hal/dma/card_dma.h"
#include "hal/os/port_os.h"
#include <string.h>

static PortCardBus sBus;
static PortCardHardwareState sState;
static uint64_t sToken;
// Like DMA destination epochs, command tokens remain monotonic across resets and fail closed on exhaustion.
static uint64_t sGeneration;
static bool sConfigured;
// OS_IE_CARD_DATA, kept plain-C here rather than introducing SDK state into physical events.
#define CARD_TRANSFER_IRQ (1u << 19)

void port_card_hardware_reset(void) {
    if (sGeneration != UINT64_MAX) {
        ++sGeneration;
    }
    memset(&sBus, 0, sizeof(sBus));
    memset(&sState, 0, sizeof(sState));
    sState.deadline = UINT64_MAX;
    sState.generation = sGeneration;
    sToken = 0;
    sConfigured = false;
    port_card_dma_reset();
}
bool port_card_hardware_configure(PortCardBusBackend backend) {
    if (sBus.busy || sState.event != PORT_CARD_HW_NONE || sState.stalled || port_card_dma_state().armed ||
        (backend.read == NULL && backend.id == NULL)) {
        return false;
    }
    port_card_bus_init(&sBus, backend);
    sConfigured = true;
    return true;
}
void port_card_hardware_test_init(PortCardBusBackend backend) {
    port_card_hardware_reset();
    (void)port_card_hardware_configure(backend);
}
static void Schedule(PortCardHardwareEvent event, uint64_t when) {
    sState.event = event;
    sState.deadline = when;
}
static void Publish(void) {
    sState.busy = sBus.busy;
    sState.ready = sBus.ready;
    if (!sBus.busy) {
        sState.owner = PORT_CARD_OWNER_NONE;
    }
    if (sBus.irq) {
        port_card_bus_ack(&sBus);
        port_os_request_irq(CARD_TRANSFER_IRQ);
    }
}
static PortCardBusResult Start(const PortCardBusRequest *request, uint64_t now, PortCardHardwareOwner owner) {
    if (!sConfigured || sBus.busy || sState.event != PORT_CARD_HW_NONE || sState.stalled || sGeneration == UINT64_MAX ||
        (owner == PORT_CARD_OWNER_CPU && port_card_dma_state().armed)) {
        return PORT_CARD_BUS_BUSY;
    }
    PortCardBusResult result = port_card_bus_start(&sBus, request, now);
    sState.result = result;
    if (result == PORT_CARD_BUS_OK) {
        sState.generation = ++sGeneration;
        sState.owner = owner;
        Schedule(PORT_CARD_HW_READY, sBus.deadline);
        Publish();
    }
    return result;
}
PortCardBusResult port_card_hardware_test_start(const PortCardBusRequest *request, uint64_t now) {
    return Start(request, now, PORT_CARD_OWNER_DMA);
}
PortCardBusResult port_card_hardware_start_cpu(const PortCardBusRequest *request, uint64_t *generation) {
    if (generation == NULL) {
        return PORT_CARD_BUS_INVALID;
    }
    PortCardBusResult result = Start(request, port_os_cycles(), PORT_CARD_OWNER_CPU);
    if (result == PORT_CARD_BUS_OK) {
        *generation = sState.generation;
    }
    return result;
}
PortCardBusResult port_card_hardware_read_cpu(uint64_t generation, uint32_t *value) {
    if (sState.owner != PORT_CARD_OWNER_CPU || sState.generation != generation) {
        return PORT_CARD_BUS_BUSY;
    }
    sState.result = port_card_bus_read(&sBus, port_os_cycles(), value);
    if (sState.result != PORT_CARD_BUS_OK) {
        if (sState.result == PORT_CARD_BUS_IO) {
            port_card_hardware_cancel();
        }
        return sState.result;
    }
    Schedule(sBus.busy ? PORT_CARD_HW_READY : PORT_CARD_HW_NONE, sBus.busy ? sBus.deadline : UINT64_MAX);
    Publish();
    return PORT_CARD_BUS_OK;
}
bool port_card_hardware_cancel_cpu(uint64_t generation) {
    if (sState.owner != PORT_CARD_OWNER_CPU || sState.generation != generation) {
        return false;
    }
    port_card_hardware_cancel();
    return true;
}
void port_card_hardware_cancel(void) {
    if (sGeneration != UINT64_MAX) {
        ++sGeneration;
    }
    sState.generation = sGeneration;
    port_card_bus_cancel(&sBus);
    PortCardDmaState dma = port_card_dma_state();
    if (dma.armed) {
        port_card_dma_stop(dma.channel);
    }
    sState.stalled = false;
    Schedule(PORT_CARD_HW_NONE, UINT64_MAX);
    Publish();
}
PortCardHardwareState port_card_hardware_state(void) {
    return sState;
}
uint64_t port_card_hardware_deadline(void) {
    return sState.deadline;
}
bool port_card_hardware_stalled(void) {
    return sState.stalled;
}
void port_card_hardware_event(uint64_t now) {
    if (sState.event == PORT_CARD_HW_NONE || now < sState.deadline) {
        return;
    }
    PortCardHardwareEvent event = sState.event;
    Schedule(PORT_CARD_HW_NONE, UINT64_MAX);
    if (event == PORT_CARD_HW_READY) {
        sState.result = port_card_bus_advance(&sBus, now);
        if (sState.result != PORT_CARD_BUS_OK) {
            port_card_hardware_cancel();
            return;
        }
        sState.last_ready = now;
        if (sBus.ready && sState.owner == PORT_CARD_OWNER_DMA && port_card_dma_demand(&sToken)) {
            if (now > UINT64_MAX - PORT_CARD_DMA_MAIN_WORD_CYCLES) {
                sState.result = PORT_CARD_BUS_TIME;
                port_card_hardware_cancel();
                return;
            }
            sState.stalled = true;
            Schedule(PORT_CARD_HW_COMMIT, now + PORT_CARD_DMA_MAIN_WORD_CYCLES);
        }
    } else {
        PortCardDmaState dma = port_card_dma_state();
        sState.stalled = false;
        // A stopped/replaced request may not consume the latch, write its old pointer or signal completion.
        if (dma.armed && dma.servicing && dma.generation == sToken) {
            uint32_t word;
            sState.result = port_card_bus_read(&sBus, now, &word);
            if (sState.result != PORT_CARD_BUS_OK || !port_card_dma_commit(sToken, word)) {
                sState.result = PORT_CARD_BUS_IO;
                port_card_hardware_cancel();
                return;
            }
            ++sState.commits;
            sState.last_commit = now;
            if (sBus.busy) {
                Schedule(PORT_CARD_HW_READY, sBus.deadline);
            }
        }
    }
    Publish();
}
