// Reports the game reaching DS code or data that has no C yet (trap.h): the part of the game that can't run until
// upstream writes its C. Function stubs call port_trap_function; data placeholders sit on pages with no access, and the
// fault handler names the placeholder that was touched. Either way the program stops with status 70, its message naming
// the symbol, its address and its caller. It also tells Frigate's test launcher whether a symbol is still a placeholder
// (port_trap_is_placeholder), so that the launcher steps aside once the field's process table has C (ADR 0012). The
// host's pages, faults, backtraces and names come from port/hal/host, the same code on every host here.
#include "hal/trap.h"
#include "hal/host/host.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_backtrace(void) {
    port_host_print_backtrace();
}

static void print_caller(void *caller) {
    const char *name;
    uintptr_t offset;
    if (port_host_function_at(caller, &name, &offset)) {
        fprintf(stderr, "  called from %s+0x%lx\n", name, (unsigned long)offset);
    } else {
        fprintf(stderr, "  called from %p\n", caller);
    }
}

_Noreturn void port_trap_function(const PortTrapSymbol *symbol, void *caller) {
    fprintf(stderr, "\nport: the game called %s (%s, 0x%08x), which has no C yet\n", symbol->name, symbol->module,
            symbol->address);
    print_caller(caller);
    print_backtrace();
    fflush(stderr);
    _Exit(PORT_TRAP_EXIT_STATUS);
}

// The placeholders' range: from the first byte of any of them to the last. gen_stubs.py defines them between two
// page-sized paddings, but ld64 doesn't keep a section's zero-fill objects in the order they are defined (it puts the
// page-aligned paddings after every placeholder), so the paddings alone don't bracket them. The section is
// page-aligned and holds nothing else, so the range starts a page, and a padding ends it whichever way the linker
// orders them
static const unsigned char *sDataBegin;
static const unsigned char *sDataEnd;

static void find_data_range(void) {
    sDataBegin = port_trap_data_begin;
    sDataEnd = port_trap_data_end;
    for (size_t i = 0; i < port_trap_data_count; i++) {
        const unsigned char *start = port_trap_data_addresses[i];
        size_t size = port_trap_data_symbols[i].size;
        if (start < sDataBegin) {
            sDataBegin = start;
        }
        if (start + size > sDataEnd) {
            sDataEnd = start + size;
        }
    }
}

// The placeholder containing an address, or NULL
static const PortTrapSymbol *find_data_symbol(const unsigned char *address, size_t *offset) {
    for (size_t i = 0; i < port_trap_data_count; i++) {
        const unsigned char *start = port_trap_data_addresses[i];
        size_t size = port_trap_data_symbols[i].size;
        if (address >= start && address < start + (size == 0 ? 1 : size)) {
            *offset = (size_t)(address - start);
            return &port_trap_data_symbols[i];
        }
    }
    return NULL;
}

int port_trap_is_placeholder(const void *address) {
    size_t offset;
    return find_data_symbol(address, &offset) != NULL;
}

// The fault handler: an access to a data placeholder is a trap, reported by the placeholder's name and offset, with a
// backtrace, and the program ends with PORT_TRAP_EXIT_STATUS; any other fault is a crash, reported the same way
// before the fault ends the program as it would have (a signal's default action, an exception's status on Windows)
static void on_fault(const void *fault_address, const char *fault) {
    const unsigned char *address = fault_address;
    if (address >= sDataBegin && address < sDataEnd) {
        size_t offset = 0;
        const PortTrapSymbol *symbol = find_data_symbol(address, &offset);
        if (symbol != NULL) {
            // Not async-signal-safe, but the process is about to stop and this is the report it exists for
            fprintf(stderr, "\nport: the game accessed %s+0x%zx (%s, 0x%08x, %u bytes), which has no C yet\n",
                    symbol->name, offset, symbol->module, symbol->address, symbol->size);
        } else {
            fprintf(stderr, "\nport: the game accessed the padding of the data placeholders at %p\n", fault_address);
        }
        print_backtrace();
        fflush(stderr);
        _Exit(PORT_TRAP_EXIT_STATUS);
    }
    // Any other fault is a crash: say where, then let the fault end the program as it would have
    fprintf(stderr, "\nport: crash: %s at address %p\n", fault, fault_address);
    print_backtrace();
    fflush(stderr);
}

void port_trap_protect_data(void) {
    uintptr_t page = (uintptr_t)port_host_page_size();
    find_data_range();
    uintptr_t begin = ((uintptr_t)sDataBegin + page - 1) & ~(page - 1);
    uintptr_t end = (uintptr_t)sDataEnd & ~(page - 1);
    if (end > begin) {
        port_host_protect_none((void *)begin, end - begin, "port: the data placeholders' protection");
    }
    // The handler runs on a stack of its own, so that a thread's stack overflowing (the game's threads run on fibers
    // with guard pages) is still reported
    port_host_set_fault_handler(on_fault);
}
