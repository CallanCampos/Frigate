// macOS's and Linux's half of the host's services (host.h): pages through mmap and mprotect, faults as SIGSEGV and
// SIGBUS on an alternate signal stack, backtraces through execinfo, names through dladdr, the watchdog as SIGALRM, and
// the program's static data and globals from its own image (Mach-O's __DATA segment and export trie, ELF's linker
// symbols and symbol table). It touches no part of the game: the trap placeholders (port/hal/trap.c), the fibers'
// guards (port/hal/os/fiber.c), the PXI pointers into static data (port/hal/pxi/pointer.c), --run-dump's globals
// (port/app/symbols.c) and the watchdog (port/app/main.c) stand on it. Windows has host_windows.c instead.
#ifndef _WIN32

// dladdr, Dl_info, dl_iterate_phdr, MAP_ANON and the signals' structs, which glibc hides from strict C11; the platform
// layer's own build defines it on the command line, ahead of port_host.h's headers
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include "hal/host/host.h"
#include <dlfcn.h>
#include <errno.h>
#include <execinfo.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(__APPLE__)
#include <mach-o/getsect.h>
#include <mach-o/ldsyms.h>
#elif defined(__linux__)
#include <elf.h>
#include <link.h>
// The start of the executable's writable data and the end of its .bss, which the GNU and LLVM linkers define
extern char __data_start[];
extern char _end[];
#endif

// --- Memory -------------------------------------------------------------------------------------------------------

size_t port_host_page_size(void) {
    static size_t page;

    if (page == 0) {
        page = (size_t)sysconf(_SC_PAGESIZE);
    }
    return page;
}

void *port_host_map(size_t size) {
    void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

    return p == MAP_FAILED ? NULL : p;
}

void *port_host_map_near(uintptr_t address, size_t size) {
    void *p = mmap((void *)address, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);

    return p == MAP_FAILED ? NULL : p;
}

void port_host_unmap(void *start, size_t size) {
    munmap(start, size);
}

void *port_host_map_file_copy(const char *path, size_t *size) {
    struct stat info;
    void *p;
    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        return NULL;
    }
    if (fstat(fd, &info) != 0) {
        close(fd);
        return NULL;
    }
    p = mmap(NULL, (size_t)info.st_size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    close(fd);
    if (p == MAP_FAILED) {
        return NULL;
    }
    *size = (size_t)info.st_size;
    return p;
}

int port_host_protect_none(void *start, size_t size, const char *what) {
    if (mprotect(start, size, PROT_NONE) != 0) {
        perror(what);
        return -1;
    }
    return 0;
}

// The signal handler runs on a stack of its own, so a guard of no access is enough: the fault is delivered there
int port_host_protect_stack_guard(void *start, size_t size, const char *what) {
    return port_host_protect_none(start, size, what);
}

void port_host_static_data(uintptr_t *start, uintptr_t *end) {
    *start = 0;
    *end = 0;
#if defined(__APPLE__)
    {
        unsigned long size = 0;
        uint8_t *data = getsegmentdata(&_mh_execute_header, "__DATA", &size);

        if (data != NULL) {
            *start = (uintptr_t)data;
            *end = (uintptr_t)data + size;
        }
    }
#elif defined(__linux__)
    *start = (uintptr_t)__data_start;
    *end = (uintptr_t)_end;
#endif
}

// --- Faults and backtraces ------------------------------------------------------------------------------------------

static PortHostFaultHandler sFaultHandler;

// A fault: the handler reports it, and unless it ends the program, the signal's default action does, as it would have
static void OnFault(int signal, siginfo_t *info, void *context) {
    char fault[32];
    struct sigaction action;

    (void)context;
    snprintf(fault, sizeof(fault), "signal %d", signal);
    if (sFaultHandler != NULL) {
        sFaultHandler(info->si_addr, fault);
    }
    memset(&action, 0, sizeof(action));
    action.sa_handler = SIG_DFL;
    sigaction(signal, &action, NULL);
    raise(signal);
}

void port_host_set_fault_handler(PortHostFaultHandler handler) {
    // The handler runs on a stack of its own, so that a stack overflowing into a fiber's guard is still reported
    static unsigned char altStack[64 * 1024];
    stack_t stack;
    struct sigaction action;

    sFaultHandler = handler;
    stack.ss_sp = altStack;
    stack.ss_size = sizeof(altStack);
    stack.ss_flags = 0;
    sigaltstack(&stack, NULL);
    memset(&action, 0, sizeof(action));
    action.sa_sigaction = OnFault;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    sigaction(SIGSEGV, &action, NULL);
    sigaction(SIGBUS, &action, NULL);
}

void port_host_print_backtrace(void) {
    void *frames[64];
    int count = backtrace(frames, 64);

    fprintf(stderr, "Backtrace:\n");
    fflush(stderr);
    backtrace_symbols_fd(frames, count, STDERR_FILENO);
}

int port_host_function_at(const void *address, const char **name, uintptr_t *offset) {
    Dl_info info;

    if (address == NULL || dladdr(address, &info) == 0 || info.dli_sname == NULL) {
        return 0;
    }
    *name = info.dli_sname;
    *offset = (uintptr_t)((const char *)address - (const char *)info.dli_saddr);
    return 1;
}

#if defined(__APPLE__)

// A Mach-O program's globals are in its export trie without any link option, and the two-level namespace keeps them
// from standing in for a library's own symbols
void *port_host_global_address(const char *name) {
    static void *program;

    if (program == NULL) {
        program = dlopen(NULL, RTLD_NOW);
    }
    return program != NULL ? dlsym(program, name) : NULL;
}

#elif defined(__linux__)

// An ELF program exports nothing unless it links with --export-dynamic, and what it exports would stand in for any
// library's symbol of the same name (the game has MD5Init, as libbsd does). So the program's own symbol table, which
// the linker keeps unless the program is stripped, is read from the file, and a symbol's address is its value plus
// where the program was loaded

// The main program's load address: the first object dl_iterate_phdr reports
static int TakeFirst(struct dl_phdr_info *info, size_t size, void *data) {
    (void)size;
    *(ElfW(Addr) *)data = info->dlpi_addr;
    return 1;
}

static const ElfW(Ehdr) * sImage;
static size_t sImageSize;

// Maps the program's own file (/proc/self/exe) read-only, once, to read its ELF symbol table; 0 when it can't, or
// when the file has no section headers (a stripped program)
static int MapProgram(void) {
    struct stat st;
    int fd;
    void *image;

    if (sImage != NULL) {
        return 1;
    }
    fd = open("/proc/self/exe", O_RDONLY);
    if (fd < 0) {
        return 0;
    }
    if (fstat(fd, &st) != 0 || (size_t)st.st_size < sizeof(ElfW(Ehdr))) {
        close(fd);
        return 0;
    }
    image = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    if (image == MAP_FAILED) {
        return 0;
    }
    sImage = image;
    sImageSize = (size_t)st.st_size;
    if (memcmp(sImage->e_ident, ELFMAG, SELFMAG) != 0 || sImage->e_shoff == 0 ||
        sImage->e_shoff + (size_t)sImage->e_shnum * sizeof(ElfW(Shdr)) > sImageSize) {
        munmap(image, sImageSize);
        sImage = NULL;
        return 0;
    }
    return 1;
}

void *port_host_global_address(const char *name) {
    const ElfW(Shdr) * sections;
    ElfW(Addr) base = 0;

    if (!MapProgram()) {
        return NULL;
    }
    sections = (const ElfW(Shdr) *)((const char *)sImage + sImage->e_shoff);
    for (unsigned i = 0; i < sImage->e_shnum; i++) {
        const ElfW(Shdr) *symtab = &sections[i];
        const ElfW(Shdr) * strtab;
        const ElfW(Sym) * symbols;
        const char *names;

        if (symtab->sh_type != SHT_SYMTAB || symtab->sh_link >= sImage->e_shnum ||
            symtab->sh_offset + symtab->sh_size > sImageSize) {
            continue;
        }
        strtab = &sections[symtab->sh_link];
        if (strtab->sh_offset + strtab->sh_size > sImageSize) {
            continue;
        }
        symbols = (const ElfW(Sym) *)((const char *)sImage + symtab->sh_offset);
        names = (const char *)sImage + strtab->sh_offset;
        for (size_t j = 0; j < symtab->sh_size / sizeof(ElfW(Sym)); j++) {
            const ElfW(Sym) *symbol = &symbols[j];

            if (symbol->st_shndx == SHN_UNDEF || symbol->st_name >= strtab->sh_size ||
                ELF64_ST_BIND(symbol->st_info) == STB_LOCAL || strcmp(names + symbol->st_name, name) != 0) {
                continue;
            }
            dl_iterate_phdr(TakeFirst, &base);
            return (void *)(base + symbol->st_value);
        }
    }
    return NULL;
}

#else

void *port_host_global_address(const char *name) {
    (void)name;
    return NULL;
}

#endif

// --- The watchdog ---------------------------------------------------------------------------------------------------

static unsigned sWatchdogSeconds;
static const char *sWatchdogMessage;
static int sWatchdogStatus;

// SIGALRM: the message and the backtrace with async-signal-safe calls, since the program is stopped wherever it spun
static void OnWatchdog(int signal) {
    void *frames[64];
    int count;

    (void)signal;
    (void)!write(STDERR_FILENO, sWatchdogMessage, strlen(sWatchdogMessage));
    count = backtrace(frames, 64);
    backtrace_symbols_fd(frames, count, STDERR_FILENO);
    _Exit(sWatchdogStatus);
}

void port_host_watchdog_start(unsigned seconds, const char *message, int status) {
    struct sigaction action;

    sWatchdogSeconds = seconds;
    sWatchdogMessage = message;
    sWatchdogStatus = status;
    memset(&action, 0, sizeof(action));
    action.sa_handler = OnWatchdog;
    sigemptyset(&action.sa_mask);
    sigaction(SIGALRM, &action, NULL);
    alarm(seconds);
}

void port_host_watchdog_kick(void) {
    if (sWatchdogSeconds != 0) {
        alarm(sWatchdogSeconds);
    }
}

// --- Files and the environment ---------------------------------------------------------------------------------------

int port_host_make_directory(const char *path) {
    return mkdir(path, 0777);
}

int port_host_set_environment(const char *name, const char *value) {
    return value != NULL ? setenv(name, value, 1) : unsetenv(name);
}

int port_host_symbolic_link(const char *target, const char *path) {
    return symlink(target, path);
}

int port_host_is_symbolic_link(const char *path) {
    struct stat info;

    return lstat(path, &info) == 0 && S_ISLNK(info.st_mode);
}

const char *port_host_temporary_directory(void) {
    const char *dir = getenv("TMPDIR");

    return dir != NULL ? dir : "/tmp";
}

#endif // !_WIN32
