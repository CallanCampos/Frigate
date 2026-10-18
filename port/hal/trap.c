// Reports the game reaching DS code or data that has no C yet. Function stubs call port_trap_function; data
// placeholders sit on pages with no access, and the fault handler names the placeholder that was touched.
#include "hal/trap.h"
#include <dlfcn.h>
#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static void print_backtrace(void) {
    void *frames[64];
    int count = backtrace(frames, 64);
    fprintf(stderr, "Backtrace:\n");
    backtrace_symbols_fd(frames, count, STDERR_FILENO);
}

static void print_caller(void *caller) {
    Dl_info info;
    if (caller != NULL && dladdr(caller, &info) != 0 && info.dli_sname != NULL) {
        fprintf(stderr, "  called from %s+0x%lx\n", info.dli_sname,
                (unsigned long)((char *)caller - (char *)info.dli_saddr));
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

static void on_fault(int signal, siginfo_t *info, void *context) {
    (void)context;
    const unsigned char *address = info->si_addr;
    if (address >= sDataBegin && address < sDataEnd) {
        size_t offset = 0;
        const PortTrapSymbol *symbol = find_data_symbol(address, &offset);
        if (symbol != NULL) {
            // Not async-signal-safe, but the process is about to stop and this is the report it exists for
            fprintf(stderr, "\nport: the game accessed %s+0x%zx (%s, 0x%08x, %u bytes), which has no C yet\n",
                    symbol->name, offset, symbol->module, symbol->address, symbol->size);
        } else {
            fprintf(stderr, "\nport: the game accessed the padding of the data placeholders at %p\n", info->si_addr);
        }
        print_backtrace();
        fflush(stderr);
        _Exit(PORT_TRAP_EXIT_STATUS);
    }
    if (!(address >= sDataBegin && address < sDataEnd)) {
        // Any other fault is a crash: say where, then let the default action end the program as it would have
        fprintf(stderr, "\nport: crash: signal %d at address %p\n", signal, info->si_addr);
        print_backtrace();
        fflush(stderr);
    }
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = SIG_DFL;
    sigaction(signal, &action, NULL);
    raise(signal);
}

void port_trap_protect_data(void) {
    long page = sysconf(_SC_PAGESIZE);
    find_data_range();
    uintptr_t begin = ((uintptr_t)sDataBegin + (uintptr_t)page - 1) & ~((uintptr_t)page - 1);
    uintptr_t end = (uintptr_t)sDataEnd & ~((uintptr_t)page - 1);
    if (end > begin && mprotect((void *)begin, end - begin, PROT_NONE) != 0) {
        perror("port: mprotect of the data placeholders");
    }
    // The handler runs on a stack of its own, so that a thread's stack overflowing (the game's threads run on fibers
    // with guard pages) is still reported
    static unsigned char altStack[64 * 1024];
    stack_t stack = { .ss_sp = altStack, .ss_size = sizeof(altStack), .ss_flags = 0 };
    sigaltstack(&stack, NULL);
    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = on_fault;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGBUS, &action, NULL);
}
