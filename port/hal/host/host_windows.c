// Windows' half of the host's services (host.h): pages through VirtualAlloc and VirtualProtect, faults through a
// vectored exception handler that moves the report to a stack of its own, crashes nobody handles through the
// unhandled-exception filter, backtraces by unwinding the x64 stack with the image's unwind tables, the names of the
// program's functions and globals from the COFF symbol table that lld keeps in an unstripped MinGW program, the
// watchdog as a thread, and the program's static data from the PE image's writable sections. It also sets up every
// program that links it before main(): stdout and stderr in binary mode, so that "\n" stays "\n" as on macOS and
// Linux; no error dialog on a crash or abort(), which would hold a test or a CI run forever; and both exception
// handlers. It touches no part of the game: the trap placeholders (port/hal/trap.c), the fibers' guards
// (port/hal/os/fiber.c), the PXI pointers into static data (port/hal/pxi/pointer.c), --run-dump's globals
// (port/app/main.c) and the watchdog (port/app/main.c) stand on it. macOS and Linux have host_posix.c instead
// (port/docs/adr/0013).
#ifdef _WIN32

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "hal/host/host.h"
#include <direct.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

// The top of a fiber's guard, next to its stack: one guard page, whose first touch raises a fault, then pages left
// writable under it. The system delivers the fault on the overflowing stack, writing its exception frame below the
// stack pointer (the context, with the processor's extended state, can pass 4 KiB), so that frame needs pages it can
// write; the handler then moves to a stack of its own. A fault in pages with no access at all, below the stack pointer,
// would have nowhere to go, and the process would end unreported
#define STACK_GUARD_TRIPWIRE (4 * 1024)
#define STACK_GUARD_LANDING (12 * 1024)
// The stack the fault handler runs on, and the frames a backtrace shows at most
#define FAULT_STACK_SIZE (256 * 1024)
#define BACKTRACE_FRAMES 64

// --- The image ------------------------------------------------------------------------------------------------------

static const unsigned char *ImageBase(void) {
    return (const unsigned char *)GetModuleHandleW(NULL);
}

static const IMAGE_NT_HEADERS *ImageHeaders(void) {
    const unsigned char *image = ImageBase();

    return (const IMAGE_NT_HEADERS *)(image + ((const IMAGE_DOS_HEADER *)image)->e_lfanew);
}

static int InImage(uintptr_t address) {
    uintptr_t base = (uintptr_t)ImageBase();

    return address >= base && address - base < ImageHeaders()->OptionalHeader.SizeOfImage;
}

// --- Memory -------------------------------------------------------------------------------------------------------

// VirtualProtect works on these 4 KiB pages, while VirtualAlloc places a mapping at the allocation granularity, 64 KiB
size_t port_host_page_size(void) {
    static size_t page;

    if (page == 0) {
        SYSTEM_INFO info;

        GetSystemInfo(&info);
        page = info.dwPageSize;
    }
    return page;
}

void *port_host_map(size_t size) {
    return VirtualAlloc(NULL, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

// A reservation starts at the system's allocation granularity, 64 KiB, so the address is rounded down to it; NULL when
// that range is taken
void *port_host_map_near(uintptr_t address, size_t size) {
    return VirtualAlloc((void *)address, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

static void ForgetRegions(void *start, size_t size);

void port_host_unmap(void *start, size_t size) {
    ForgetRegions(start, size);
    VirtualFree(start, 0, MEM_RELEASE);
}

// A copy-on-write view of the file, the equivalent of mmap's MAP_PRIVATE
void *port_host_map_file_copy(const char *path, size_t *size) {
    LARGE_INTEGER fileSize;
    HANDLE mapping = NULL;
    void *view = NULL;
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file == INVALID_HANDLE_VALUE) {
        return NULL;
    }
    if (GetFileSizeEx(file, &fileSize) && fileSize.QuadPart > 0) {
        mapping = CreateFileMappingA(file, NULL, PAGE_WRITECOPY, 0, 0, NULL);
    }
    CloseHandle(file);
    if (mapping == NULL) {
        return NULL;
    }
    view = MapViewOfFile(mapping, FILE_MAP_COPY, 0, 0, 0);
    CloseHandle(mapping);
    if (view != NULL) {
        *size = (size_t)fileSize.QuadPart;
    }
    return view;
}

// A view of a file is released by unmapping it; VirtualFree releases only what VirtualAlloc made
void port_host_unmap_file_copy(void *view, size_t size) {
    (void)size;
    UnmapViewOfFile(view);
}

// --- The memory the seam made inaccessible --------------------------------------------------------------------------

// The ranges that port_host_protect_none and port_host_protect_stack_guard made fault: the trap placeholders' pages and
// the fibers' guards. A fault at an address in one is the port's own, whatever code touched it (the C library's memset
// and memcpy run outside the program's image); and a thread whose stack, as its TEB has it, starts at the top of a
// guard runs on that guard's fiber, since minicoro sets the TEB's stack bounds to the fiber's stack (fiber.c puts the
// stack right above its guard). The vectored handler reads them only when the lock is free: the thread that faults may
// be the one holding it
typedef struct HostRegion {
    uintptr_t start;
    uintptr_t end;
    int stackGuard;
} HostRegion;

static SRWLOCK sRegionsLock = SRWLOCK_INIT;
static HostRegion *sRegions;
static size_t sRegionCount;
static size_t sRegionCapacity;

static void AddRegion(void *start, size_t size, int stackGuard) {
    AcquireSRWLockExclusive(&sRegionsLock);
    if (sRegionCount == sRegionCapacity) {
        size_t capacity = sRegionCapacity == 0 ? 64 : sRegionCapacity * 2;
        HostRegion *regions = realloc(sRegions, capacity * sizeof(*regions));

        if (regions != NULL) {
            sRegions = regions;
            sRegionCapacity = capacity;
        }
    }
    // Without room, a fault there is reported as any other is
    if (sRegionCount < sRegionCapacity) {
        sRegions[sRegionCount].start = (uintptr_t)start;
        sRegions[sRegionCount].end = (uintptr_t)start + size;
        sRegions[sRegionCount].stackGuard = stackGuard;
        sRegionCount++;
    }
    ReleaseSRWLockExclusive(&sRegionsLock);
}

// Forgets the regions in pages that are being released, such as a destroyed fiber's guard
static void ForgetRegions(void *start, size_t size) {
    uintptr_t low = (uintptr_t)start;
    uintptr_t high = low + size;

    AcquireSRWLockExclusive(&sRegionsLock);
    for (size_t i = 0; i < sRegionCount;) {
        if (sRegions[i].start >= low && sRegions[i].end <= high) {
            sRegions[i] = sRegions[--sRegionCount];
        } else {
            i++;
        }
    }
    ReleaseSRWLockExclusive(&sRegionsLock);
}

// Whether address is in a region, and whether a stack whose lowest address is stackLow is a fiber's
static void FindRegions(uintptr_t address, uintptr_t stackLow, int *inRegion, int *onFiber) {
    *inRegion = 0;
    *onFiber = 0;
    if (!TryAcquireSRWLockShared(&sRegionsLock)) {
        return;
    }
    for (size_t i = 0; i < sRegionCount; i++) {
        if (address >= sRegions[i].start && address < sRegions[i].end) {
            *inRegion = 1;
        }
        if (sRegions[i].stackGuard && sRegions[i].end == stackLow) {
            *onFiber = 1;
        }
    }
    ReleaseSRWLockShared(&sRegionsLock);
}

static int Protect(void *start, size_t size, DWORD protection, const char *what) {
    DWORD old;

    if (size != 0 && !VirtualProtect(start, size, protection, &old)) {
        fprintf(stderr, "%s: Windows error %lu\n", what, GetLastError());
        return -1;
    }
    return 0;
}

int port_host_protect_none(void *start, size_t size, const char *what) {
    if (Protect(start, size, PAGE_NOACCESS, what) != 0) {
        return -1;
    }
    AddRegion(start, size, 0);
    return 0;
}

// The pages are no longer the seam's to report: a fault there is any other's
int port_host_protect_read(void *start, size_t size, const char *what) {
    if (Protect(start, size, PAGE_READONLY, what) != 0) {
        return -1;
    }
    ForgetRegions(start, size);
    return 0;
}

int port_host_protect_stack_guard(void *start, size_t size, const char *what) {
    unsigned char *end = (unsigned char *)start + size;
    size_t tripwire = size < STACK_GUARD_TRIPWIRE ? size : STACK_GUARD_TRIPWIRE;
    size_t landing = size - tripwire < STACK_GUARD_LANDING ? size - tripwire : STACK_GUARD_LANDING;

    if (Protect(start, size - tripwire - landing, PAGE_NOACCESS, what) != 0 ||
        Protect(end - tripwire, tripwire, PAGE_READWRITE | PAGE_GUARD, what) != 0) {
        return -1;
    }
    AddRegion(start, size, 1);
    return 0;
}

// From the first writable section of the image to the end of the last: .data, .bss and the sections between them
void port_host_static_data(uintptr_t *start, uintptr_t *end) {
    const IMAGE_NT_HEADERS *headers = ImageHeaders();
    const IMAGE_SECTION_HEADER *section = IMAGE_FIRST_SECTION((IMAGE_NT_HEADERS *)headers);
    uintptr_t base = (uintptr_t)ImageBase();

    *start = 0;
    *end = 0;
    for (WORD i = 0; i < headers->FileHeader.NumberOfSections; i++, section++) {
        uintptr_t first = base + section->VirtualAddress;
        uintptr_t last = first + section->Misc.VirtualSize;

        if ((section->Characteristics & IMAGE_SCN_MEM_WRITE) == 0) {
            continue;
        }
        if (*start == 0 || first < *start) {
            *start = first;
        }
        if (last > *end) {
            *end = last;
        }
    }
}

// --- The program's symbols ------------------------------------------------------------------------------------------

// A symbol of the program's COFF symbol table, which lld writes into a MinGW program unless it is stripped: a global or
// a static, with its address in this process and whether it is in code
typedef struct HostSymbol {
    const char *name;
    uintptr_t address;
    unsigned char global;
    unsigned char code;
} HostSymbol;

static HostSymbol *sSymbols;
static size_t sSymbolCount;
static INIT_ONCE sSymbolsOnce = INIT_ONCE_STATIC_INIT;

// Reads the symbol table from the program's own file, mapped read-only for the rest of the run (the names stay in it)
static BOOL CALLBACK ReadSymbols(INIT_ONCE *once, void *parameter, void **context) {
    static WCHAR path[MAX_PATH * 8];
    const unsigned char *file;
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS *headers;
    const IMAGE_SECTION_HEADER *sections;
    const unsigned char *strings;
    LARGE_INTEGER fileSize;
    HANDLE handle;
    HANDLE mapping;
    size_t size;
    size_t stringsSize;
    DWORD table;
    DWORD count;
    char *shortNames;
    uintptr_t base = (uintptr_t)ImageBase();

    (void)once;
    (void)parameter;
    (void)context;
    if (GetModuleFileNameW(NULL, path, sizeof(path) / sizeof(path[0])) == 0) {
        return TRUE;
    }
    handle = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        return TRUE;
    }
    mapping = GetFileSizeEx(handle, &fileSize) ? CreateFileMappingW(handle, NULL, PAGE_READONLY, 0, 0, NULL) : NULL;
    CloseHandle(handle);
    if (mapping == NULL) {
        return TRUE;
    }
    file = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(mapping);
    if (file == NULL) {
        return TRUE;
    }
    size = (size_t)fileSize.QuadPart;
    dos = (const IMAGE_DOS_HEADER *)file;
    if (size < sizeof(*dos) || dos->e_magic != IMAGE_DOS_SIGNATURE ||
        (size_t)dos->e_lfanew + sizeof(IMAGE_NT_HEADERS) > size) {
        return TRUE;
    }
    headers = (const IMAGE_NT_HEADERS *)(file + dos->e_lfanew);
    table = headers->FileHeader.PointerToSymbolTable;
    count = headers->FileHeader.NumberOfSymbols;
    if (headers->Signature != IMAGE_NT_SIGNATURE || table == 0 || count == 0 ||
        (size_t)table + (size_t)count * IMAGE_SIZEOF_SYMBOL > size) {
        return TRUE;
    }
    sections = IMAGE_FIRST_SECTION((IMAGE_NT_HEADERS *)headers);
    strings = file + table + (size_t)count * IMAGE_SIZEOF_SYMBOL;
    stringsSize = size - (size_t)(strings - file);
    sSymbols = malloc(count * sizeof(HostSymbol));
    shortNames = malloc((size_t)count * 9);
    if (sSymbols == NULL || shortNames == NULL) {
        free(sSymbols);
        free(shortNames);
        sSymbols = NULL;
        return TRUE;
    }
    for (DWORD i = 0; i < count; i++) {
        const IMAGE_SYMBOL *symbol = (const IMAGE_SYMBOL *)(file + table + (size_t)i * IMAGE_SIZEOF_SYMBOL);
        SHORT section = symbol->SectionNumber;
        BYTE storage = symbol->StorageClass;
        const char *name = NULL;

        if (section >= 1 && section <= headers->FileHeader.NumberOfSections &&
            (storage == IMAGE_SYM_CLASS_EXTERNAL || storage == IMAGE_SYM_CLASS_STATIC)) {
            if (symbol->N.Name.Short != 0) {
                // A name of up to 8 bytes, not terminated when it uses all 8
                memcpy(shortNames + (size_t)i * 9, symbol->N.ShortName, 8);
                shortNames[(size_t)i * 9 + 8] = '\0';
                name = shortNames + (size_t)i * 9;
            } else if (symbol->N.Name.Long < stringsSize) {
                name = (const char *)strings + symbol->N.Name.Long;
            }
        }
        // Section and other assembler symbols start with '.'
        if (name != NULL && name[0] != '\0' && name[0] != '.') {
            HostSymbol *entry = &sSymbols[sSymbolCount++];

            entry->name = name;
            entry->address = base + sections[section - 1].VirtualAddress + symbol->Value;
            entry->global = storage == IMAGE_SYM_CLASS_EXTERNAL;
            entry->code = (sections[section - 1].Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0;
        }
        i += symbol->NumberOfAuxSymbols;
    }
    return TRUE;
}

static void LoadSymbols(void) {
    InitOnceExecuteOnce(&sSymbolsOnce, ReadSymbols, NULL, NULL);
}

int port_host_function_at(const void *address, const char **name, uintptr_t *offset) {
    uintptr_t target = (uintptr_t)address;
    const HostSymbol *best = NULL;

    if (address == NULL || !InImage(target)) {
        return 0;
    }
    LoadSymbols();
    for (size_t i = 0; i < sSymbolCount; i++) {
        const HostSymbol *symbol = &sSymbols[i];

        if (symbol->code && symbol->address <= target && (best == NULL || symbol->address > best->address)) {
            best = symbol;
        }
    }
    if (best == NULL) {
        return 0;
    }
    *name = best->name;
    *offset = target - best->address;
    return 1;
}

void *port_host_global_address(const char *name) {
    LoadSymbols();
    for (size_t i = 0; i < sSymbolCount; i++) {
        if (sSymbols[i].global && strcmp(sSymbols[i].name, name) == 0) {
            return (void *)sSymbols[i].address;
        }
    }
    return NULL;
}

// --- Backtraces ---------------------------------------------------------------------------------------------------

// Straight to stderr's handle: the watchdog writes while the main thread is suspended, maybe holding the C library's
// lock of stderr. stderr is unbuffered, so this keeps the order of what the program printed before
void port_host_write_error(const char *text) {
    DWORD written;

    WriteFile(GetStdHandle(STD_ERROR_HANDLE), text, (DWORD)strlen(text), &written, NULL);
}

// Whether the stack's bytes at address can be read: not in a guard page, which reading would trip again
static int Readable(uintptr_t address, size_t size) {
    MEMORY_BASIC_INFORMATION info;
    uintptr_t end = address + size;

    while (address < end) {
        if (VirtualQuery((const void *)address, &info, sizeof(info)) == 0 || info.State != MEM_COMMIT ||
            (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)) != 0 || info.Protect == 0) {
            return 0;
        }
        address = (uintptr_t)info.BaseAddress + info.RegionSize;
    }
    return 1;
}

// One frame, as macOS's backtrace_symbols_fd has it: its number, the address, and the function with the offset, or
// the module (SDL3.dll, ntdll.dll) where the program's symbols don't cover it
static void PrintFrame(int index, DWORD64 pc) {
    char line[512];
    char path[MAX_PATH];
    const char *name;
    const char *module;
    uintptr_t offset;
    HMODULE handle;

    if (port_host_function_at((const void *)(uintptr_t)pc, &name, &offset)) {
        snprintf(line, sizeof(line), "%-3d 0x%016llx %s + %llu\n", index, (unsigned long long)pc, name,
                 (unsigned long long)offset);
    } else if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                  (LPCSTR)(uintptr_t)pc, &handle) &&
               GetModuleFileNameA(handle, path, sizeof(path)) != 0) {
        module = strrchr(path, '\\');
        snprintf(line, sizeof(line), "%-3d 0x%016llx %s + 0x%llx\n", index, (unsigned long long)pc,
                 module != NULL ? module + 1 : path, (unsigned long long)(pc - (DWORD64)(uintptr_t)handle));
    } else {
        snprintf(line, sizeof(line), "%-3d 0x%016llx\n", index, (unsigned long long)pc);
    }
    port_host_write_error(line);
}

// The frames from a thread's context outward, by the unwind tables of the image each frame's code is in; a function
// without one is a leaf, whose return address is on top of the stack
static void PrintFrames(const CONTEXT *start) {
    CONTEXT context = *start;

    LoadSymbols();
    for (int i = 0; i < BACKTRACE_FRAMES && context.Rip != 0; i++) {
        DWORD64 imageBase = 0;
        PRUNTIME_FUNCTION function;

        PrintFrame(i, context.Rip);
        if (!Readable((uintptr_t)context.Rsp, sizeof(DWORD64))) {
            break;
        }
        function = RtlLookupFunctionEntry(context.Rip, &imageBase, NULL);
        if (function == NULL) {
            context.Rip = *(const DWORD64 *)(uintptr_t)context.Rsp;
            context.Rsp += sizeof(DWORD64);
        } else {
            void *handlerData;
            DWORD64 establisher;

            RtlVirtualUnwind(UNW_FLAG_NHANDLER, imageBase, context.Rip, function, &context, &handlerData, &establisher,
                             NULL);
        }
    }
}

// --- Faults ---------------------------------------------------------------------------------------------------------

static PortHostFaultHandler sFaultHandler;
// The exception being reported: taken once, so that a fault while reporting (a backtrace walking a broken stack) ends
// the program instead of starting again
static volatile LONG sFaultTaken;
static int sInFault;
static CONTEXT sFaultContext;
static const void *sFaultAddress;
static const void *sFaultInstruction;
static DWORD sFaultCode;
static char sFaultText[32];
static unsigned char sFaultStack[FAULT_STACK_SIZE] __attribute__((aligned(64)));

static int IsMemoryFault(DWORD code) {
    return code == EXCEPTION_ACCESS_VIOLATION || code == EXCEPTION_GUARD_PAGE || code == EXCEPTION_STACK_OVERFLOW ||
           code == EXCEPTION_IN_PAGE_ERROR;
}

// The exceptions that end a program unless a handler takes them: the memory faults and the processor's other faults,
// a breakpoint among them (INT3: __builtin_debugtrap, __debugbreak), as macOS and Linux report SIGTRAP
// (host_posix.c). A debugger takes a breakpoint of its own before any handler of the program's sees it
static int IsFatal(DWORD code) {
    switch (code) {
    case EXCEPTION_ILLEGAL_INSTRUCTION:
    case EXCEPTION_PRIV_INSTRUCTION:
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
    case EXCEPTION_INT_OVERFLOW:
    case EXCEPTION_DATATYPE_MISALIGNMENT:
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
    case EXCEPTION_BREAKPOINT:
        return 1;
    default:
        return IsMemoryFault(code);
    }
}

// An access violation's, an in-page error's and a guard page's second parameter is the address accessed
static uintptr_t AccessedAddress(const EXCEPTION_RECORD *record) {
    return record->NumberParameters >= 2 ? (uintptr_t)record->ExceptionInformation[1]
                                         : (uintptr_t)record->ExceptionAddress;
}

// Copies what the report needs; no more than that runs on the faulting stack
static void TakeFault(const EXCEPTION_POINTERS *info) {
    const EXCEPTION_RECORD *record = info->ExceptionRecord;

    sFaultContext = *info->ContextRecord;
    sFaultCode = record->ExceptionCode;
    sFaultAddress = (const void *)AccessedAddress(record);
    sFaultInstruction = record->ExceptionAddress;
}

// The report: a memory fault goes to the program's handler, which names a trap placeholder, when there is one; any
// other exception is a crash, reported here with a backtrace
static void Report(void) {
    sInFault = 1;
    snprintf(sFaultText, sizeof(sFaultText), "exception 0x%08lx", (unsigned long)sFaultCode);
    if (IsMemoryFault(sFaultCode) && sFaultHandler != NULL) {
        sFaultHandler(sFaultAddress, sFaultText);
    } else {
        fprintf(stderr, "\nport: crash: %s at address %p\n", sFaultText, sFaultInstruction);
        port_host_print_backtrace();
    }
    fflush(stderr);
}

// The report on the fault stack, as a signal handler's on its alternate stack, then the end of the program with the
// exception's status, as an exception nobody handles would end it
__attribute__((noreturn)) static void RunFaultHandler(void) {
    Report();
    TerminateProcess(GetCurrentProcess(), sFaultCode);
    for (;;) {
    }
}

// The exceptions reported here, on the fault stack, rather than left to the code's own handlers and then to
// OnUnhandledException:
//   - a stack overflow, anywhere;
//   - a memory fault at an address the seam made inaccessible (a trap placeholder, a fiber's guard), whatever code
//     touched it: the C library's memcpy and memset, which the game calls, run outside the program's image;
//   - any fatal exception on a fiber: the system looks for a handler frame by frame up the faulting stack, and a
//     fiber's frames end at minicoro's entry, never reaching the frame at the bottom of the thread's own stack that
//     calls the unhandled-exception filter, so the process would end unreported;
//   - a memory fault of the program's own code, once the program has a fault handler.
// A fault in another module's code, off the fibers, may be one that module handles itself; one it doesn't reaches
// OnUnhandledException. The thread goes on in RunFaultHandler on the fault stack, with the TEB's stack bounds moved
// there too, which the system and the stack probes read
static LONG CALLBACK OnVectoredException(EXCEPTION_POINTERS *info) {
    const EXCEPTION_RECORD *record = info->ExceptionRecord;
    DWORD code = record->ExceptionCode;
    int memory = IsMemoryFault(code);
    uintptr_t top = ((uintptr_t)(sFaultStack + sizeof(sFaultStack)) & ~(uintptr_t)15) - 64;
    NT_TIB *tib = (NT_TIB *)NtCurrentTeb();
    int inRegion;
    int onFiber;

    if (!IsFatal(code)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    FindRegions(memory ? AccessedAddress(record) : 0, (uintptr_t)tib->StackLimit, &inRegion, &onFiber);
    if (code != EXCEPTION_STACK_OVERFLOW && !(memory && inRegion) && !onFiber &&
        !(memory && sFaultHandler != NULL && InImage((uintptr_t)record->ExceptionAddress))) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    if (InterlockedExchange(&sFaultTaken, 1) != 0) {
        // Another thread's fault is being reported, or this one faulted while reporting: this exception's own status
        TerminateProcess(GetCurrentProcess(), code);
    }
    TakeFault(info);
    tib->StackBase = (void *)(top + 64);
    tib->StackLimit = sFaultStack;
    // As if RunFaultHandler had been called: the return address's slot, with the caller's 32 bytes of home space above
    info->ContextRecord->Rsp = top - 8;
    info->ContextRecord->Rip = (DWORD64)(uintptr_t)RunFaultHandler;
    return EXCEPTION_CONTINUE_EXECUTION;
}

// An exception nobody handled, on a thread's own stack: reported on the faulting stack, which still has room (a stack
// overflow was taken first). Then the program ends with the exception's status
static LONG WINAPI OnUnhandledException(EXCEPTION_POINTERS *info) {
    if (InterlockedExchange(&sFaultTaken, 1) != 0) {
        return EXCEPTION_EXECUTE_HANDLER;
    }
    TakeFault(info);
    Report();
    return EXCEPTION_EXECUTE_HANDLER;
}

void port_host_set_fault_handler(PortHostFaultHandler handler) {
    sFaultHandler = handler;
}

void port_host_print_backtrace(void) {
    CONTEXT context;

    if (sInFault) {
        context = sFaultContext;
    } else {
        RtlCaptureContext(&context);
    }
    fprintf(stderr, "Backtrace:\n");
    fflush(stderr);
    PrintFrames(&context);
}

// Every program that links the platform layer, before main(): no error dialog, which nobody would close in a test or
// a CI run (a crash's, a missing file's, abort()'s); the two exception handlers, the vectored one last in the list, so
// that a sanitizer's own (ASan maps its shadow memory on faults) runs first; and stdout and stderr in binary mode, so
// that the program's output has the same line ends as on macOS and Linux
__attribute__((constructor)) static void StartHost(void) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stderr), _O_BINARY);
    SetUnhandledExceptionFilter(OnUnhandledException);
    AddVectoredExceptionHandler(0, OnVectoredException);
}

// --- The watchdog ---------------------------------------------------------------------------------------------------

static HANDLE sWatchdogThread;
static HANDLE sWatchdogKick;
static HANDLE sMainThread;
static volatile DWORD sWatchdogMilliseconds;
static const char *volatile sWatchdogMessage;
static volatile int sWatchdogStatus;

// Waits for kicks; when one doesn't come in time, reports the main thread where it is, suspended, and ends the program
static DWORD WINAPI WatchdogMain(void *argument) {
    CONTEXT context;

    (void)argument;
    while (WaitForSingleObject(sWatchdogKick, sWatchdogMilliseconds) == WAIT_OBJECT_0) {
    }
    // The symbols first, while the main thread runs: reading them allocates, and the main thread could be suspended
    // holding the heap's lock
    LoadSymbols();
    port_host_write_error(sWatchdogMessage);
    if (SuspendThread(sMainThread) != (DWORD)-1) {
        memset(&context, 0, sizeof(context));
        context.ContextFlags = CONTEXT_FULL;
        if (GetThreadContext(sMainThread, &context)) {
            PrintFrames(&context);
        }
    }
    TerminateProcess(GetCurrentProcess(), (UINT)sWatchdogStatus);
    return 0;
}

// 0 seconds turns the watchdog off, as alarm(0) does on POSIX: no thread is started, and a running one waits for kicks
// without a limit
void port_host_watchdog_start(unsigned seconds, const char *message, int status) {
    sWatchdogMilliseconds = seconds != 0 ? seconds * 1000 : INFINITE;
    sWatchdogMessage = message;
    sWatchdogStatus = status;
    if (sMainThread == NULL) {
        DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &sMainThread, 0, FALSE,
                        DUPLICATE_SAME_ACCESS);
    }
    if (sWatchdogKick == NULL) {
        sWatchdogKick = CreateEventW(NULL, FALSE, FALSE, NULL);
    }
    if (sWatchdogThread != NULL) {
        SetEvent(sWatchdogKick);
    } else if (seconds != 0) {
        sWatchdogThread = CreateThread(NULL, 64 * 1024, WatchdogMain, NULL, 0, NULL);
    }
}

void port_host_watchdog_kick(void) {
    if (sWatchdogKick != NULL) {
        SetEvent(sWatchdogKick);
    }
}

// --- Files and the environment ---------------------------------------------------------------------------------------

int port_host_make_directory(const char *path) {
    return _mkdir(path);
}

int port_host_set_environment(const char *name, const char *value) {
    // An empty value removes the variable
    return _putenv_s(name, value != NULL ? value : "") == 0 ? 0 : -1;
}

// The saves follow no links on Windows (port/hal/card/backup.c), so their tests make none
int port_host_symbolic_link(const char *target, const char *path) {
    (void)target;
    (void)path;
    errno = ENOSYS;
    return -1;
}

int port_host_is_symbolic_link(const char *path) {
    DWORD attributes = GetFileAttributesA(path);

    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0;
}

const char *port_host_temporary_directory(void) {
    static char dir[MAX_PATH + 1];
    DWORD length;

    if (dir[0] == '\0') {
        length = GetTempPathA(sizeof(dir), dir);
        if (length == 0 || length >= sizeof(dir)) {
            return ".";
        }
        // Without its last backslash, so that "%s/name" makes a path
        if (dir[length - 1] == '\\') {
            dir[length - 1] = '\0';
        }
    }
    return dir;
}

#endif // _WIN32
