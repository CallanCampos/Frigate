// The host operating system's services that the platform layer, the program and the tests share, where macOS, Linux and
// Windows differ: memory pages and their protection, faults (a trap placeholder touched, a fiber's stack overflowing
// into its guard, a crash), backtraces and the names of the program's functions and globals, the watchdog that stops a
// frame which never ends, the monotonic clock that times the frames, the 3D renderer's threads and the signals they
// wait on, writes to stderr that take no lock, directories, the environment and the program's static data. It touches
// no part of the game itself: it is what the code above it stands on, so that trap.c, fiber.c and the tests are the
// same on every host (port/docs/adr/0013). A module whose own feature is a host's keeps its branches instead (the
// saves' folder and files in port/hal/card/backup.c, the VRAM's aliasing in port/hal/gx/vram_os.c, the mapped ROM in
// port/hal/rom/rom.c, the overlays' sections by object format in port/hal/fs/ovsec.c, the RTC's local time, the macOS
// app's log, the window's icon); port/hal/README.md lists them. Each host's half is one file, chosen by the compiler:
// host_posix.c for macOS and Linux, host_windows.c for Windows; the other compiles to nothing. Its users:
// port/hal/trap.c (traps and crashes), port/hal/os/fiber.c (the fibers' stacks and guards), port/hal/pxi/pointer.c (the
// static data), port/hal/os/os_terminate.c (backtraces), port/hal/gx (the page size), port/hal/os/frame_time.c (the
// clock), port/gfx/workers.c (the 3D renderer's threads and signals), port/app/main.c (the watchdog, --run-dump's
// globals, the game's thread's class), port/app/run_dir.c, and the tests' harnesses. Plain C types only.
//
// On Windows, every program that links host_windows.c also gets, before main(): stdout and stderr in binary mode, no
// error dialog on a crash or abort(), and both exception handlers, which report the crashes nobody handles (see
// port_host_set_fault_handler).
#ifndef PORT_HAL_HOST_HOST_H
#define PORT_HAL_HOST_HOST_H

#include <stddef.h>
#include <stdint.h>

// --- Memory -------------------------------------------------------------------------------------------------------

// The alignment of a static array that must start a page (port/hal/hw's memory windows, and the paddings around the
// trap stubs' placeholders that port/tools/gen_stubs.py writes): the largest page of the hosts, Apple Silicon's 16 KiB,
// but 4 KiB, the host's page, on Windows, since COFF aligns no static object beyond 8 KiB, and in an x86_64 Mach-O
// program (the macOS app's Intel slice, port/docs/adr/0015), whose linker holds a section's alignment to its 4 KiB page
// and warns at more. The preprocessor chooses, so each slice of a universal macOS program gets its own
#if defined(_WIN32) || (defined(__APPLE__) && defined(__x86_64__))
#define PORT_HOST_PAGE_ALIGNMENT 4096
#else
#define PORT_HOST_PAGE_ALIGNMENT 16384
#endif

// The size of the pages the protection works on: 16 KiB on Apple Silicon, 4 KiB on x86-64 Linux and on Windows
size_t port_host_page_size(void);

// Read-write pages of size bytes, zero-filled and committed, at an address the system chooses; NULL when there are
// none. size is a multiple of the page size. Released with port_host_unmap
void *port_host_map(size_t size);

// Read-write pages of size bytes at address, or near it: macOS and Linux take address as a hint, and Windows rounds it
// down to its 64 KiB allocation granularity. NULL when the system puts them nowhere, or, on Windows, when that range is
// taken. The tests use it to place memory where its low 32 bits meet the static data's (port/tests/pxi)
void *port_host_map_near(uintptr_t address, size_t size);

void port_host_unmap(void *start, size_t size);

// The file at path mapped readable and writable, but private: a write changes only this process's copy of a page. NULL
// when it can't be opened or mapped; *size is the file's size. The ROM's tests change their dump this way
// (port/tests/rom). Released with port_host_unmap_file_copy, never port_host_unmap: Windows releases a file's view
// another way than pages, and keeps the file open, so that it can't be removed, until then
void *port_host_map_file_copy(const char *path, size_t *size);

void port_host_unmap_file_copy(void *view, size_t size);

// Makes whole pages inaccessible, so that any access faults: the trap placeholders (port/hal/trap.c). A fault there is
// reported whatever code made it, the C library's included. 0 when done, -1 with the system's reason printed after
// what
int port_host_protect_none(void *start, size_t size, const char *what);

// Makes whole pages readable, and only readable, again: the trap placeholders' pages at exit in the sanitized build,
// so that LeakSanitizer's scan of the program's data can read them (port/hal/trap.c). A fault there afterwards is
// reported as any other is. 0 when done, -1 with the system's reason printed after what
int port_host_protect_read(void *start, size_t size, const char *what);

// Makes the guard under a fiber's stack fault on any access (port/hal/os/fiber.c). On Windows the top of the guard,
// next to the stack, is a guard page, whose first touch raises a fault, over pages left writable for the exception
// frame that the system writes on the overflowing stack: a fault in pages with no access at all, below the stack
// pointer, has nowhere to go and ends the process unreported. 0 when done, -1 with the system's reason printed after
// what. The fiber's stack starts where the guard ends (Windows tells a thread on a fiber by that)
int port_host_protect_stack_guard(void *start, size_t size, const char *what);

// The program's writable static data, its .data and .bss (Mach-O's __DATA segment, ELF's from __data_start to _end,
// the PE image's writable sections), where the libraries' buffers and the sound heap g_GFLSndHeap are: the PXI
// protocols' 32-bit pointers into them are found again there (port/hal/pxi/pointer.c). Both 0 when unknown
void port_host_static_data(uintptr_t *start, uintptr_t *end);

// --- Faults and backtraces ------------------------------------------------------------------------------------------

// Called on a memory fault of the program: an access to memory with no access (a trap placeholder, a fiber's guard),
// to no memory, or one that overflowed a stack. address is the address accessed; fault says what the system reported,
// "signal 11" or "exception 0xc0000005", for a crash report. It runs on a stack of its own, so a fiber whose stack
// overflowed is still reported. It may end the program (_Exit); when it returns, the fault ends the program as it would
// have without a handler: the signal's default action, or the exception's status on Windows
typedef void (*PortHostFaultHandler)(const void *address, const char *fault);

// Sets the one handler of memory faults, in place of any earlier one. On macOS and Linux that is every SIGSEGV and
// SIGBUS, from any code; from then on the seam also reports, itself, the processor's other faults that end a program,
// on any stack, a fiber's too: SIGFPE (an integer division by zero, which faults on x86-64; arm64 gives 0 instead),
// SIGILL (an illegal instruction, x86-64's __builtin_trap among them) and SIGTRAP (a breakpoint, arm64's __builtin_trap
// among them), as "port: crash: signal N at address ..." with the instruction's address, then "  in NAME+0xOFFSET", the
// function that holds it, when port_host_function_at names one (a backtrace taken in the signal's handler may not show
// that function: under Rosetta it doesn't), and a backtrace, after which the signal ends the program as it would have.
// On Windows it is a memory fault at an address port_host_protect_none or port_host_protect_stack_guard made
// inaccessible, any memory fault on a fiber or in the program's own code, and one that nothing else handled; Windows
// also reports, with or without a handler, every exception nobody handles (an illegal instruction, a division by zero,
// a breakpoint, a memory fault without a handler), as "port: crash: exception ... at address ..." with a backtrace
// unwound from the faulting instruction, whose first frame names its function, on a fiber too, where the system itself
// would end the process silently
void port_host_set_fault_handler(PortHostFaultHandler handler);

// Prints "Backtrace:" and the frames of the running code to stderr, innermost first, each with its function's name
// where the program's symbols give one. Inside a fault handler, the frames are those of the code that faulted
void port_host_print_backtrace(void);

// The function whose code holds address: its name and how far into it address is. 0 when no symbol covers it: on
// macOS and Linux, dladdr names the exported functions of any library loaded (the C library's, SDL's); on macOS it
// names the program's own functions too, static ones among them, from its symbol table, but Linux's reads only the
// symbols a program exports, and an ELF program exports none of its functions without --export-dynamic, which
// Frigate's don't link with; on Windows, only the program's own COFF symbols name anything, so another module's code
// gives 0. A stripped program gives 0 for its own code too
int port_host_function_at(const void *address, const char **name, uintptr_t *offset);

// The address of the program's global (a function or an object, by its C name), from the program's own symbols, for
// --run-dump (port/app/main.c). NULL without one, for a static, or in a stripped program
void *port_host_global_address(const char *name);

// Writes text to stderr as it is, straight to the system: it takes no lock of the C library's stderr, so a fault
// handler and the watchdog may call it wherever the program stopped (on POSIX it is write(2), async-signal-safe)
void port_host_write_error(const char *text);

// --- The watchdog ---------------------------------------------------------------------------------------------------

// After seconds of host time without a kick, prints message to stderr as it is, then the backtrace of the program's
// main thread, and ends the program with status. 0 seconds turns it off. A program has one watchdog; starting it again
// sets it anew. It reads the host's time and only reports: nothing of the game depends on it
void port_host_watchdog_start(unsigned seconds, const char *message, int status);

// Starts the watchdog's seconds again (port/app/main.c kicks it each frame)
void port_host_watchdog_kick(void);

// --- Time -----------------------------------------------------------------------------------------------------------

// The host's monotonic clock in nanoseconds, from an arbitrary start: how long the program's own work takes, for
// --frame-times (port/hal/os/frame_time.h), and how long the renderer's workers spin before they sleep
// (port/gfx/workers.c). Any thread may read it, the first time included. Nothing of the game reads it, so a run stays
// the same
uint64_t port_host_now_ns(void);

// --- Threads --------------------------------------------------------------------------------------------------------

// Frigate runs the game on one thread; only the renderer's workers run beside it (port/gfx/workers.c), and they run
// renderer code alone. These are what they need of the host.

// Starts a thread that runs entry(argument) until the program ends, in the class the host gives a program's
// interactive work (macOS's user-interactive QoS), with the asynchronous signals blocked, so that the watchdog's
// SIGALRM reaches the game's thread; 0 when it started
int port_host_thread_start(void (*entry)(void *), void *argument);

// Puts the calling thread in that class: on macOS the user-interactive QoS that an app's main thread gets, which keeps
// it on the performance cores whatever started the program (a shell, a test); nothing elsewhere
void port_host_thread_set_interactive(void);

// The processor cores that run a program's interactive work at full speed: macOS's performance cores
// (hw.perflevel0.physicalcpu, else every core), every online processor elsewhere; at least 1
int port_host_performance_cores(void);

// A signal threads wait on: a count that port_host_signal_raise adds one to, waking every thread waiting on it, and
// that port_host_signal_wait waits to see differ from a count the caller saw, so that a raise between the caller's
// look and its wait isn't lost. port_host_signal_count reads the count without waiting (with acquire order: what the
// raiser wrote before raising is seen), for a thread that spins a little before it waits. Made once, never freed; NULL
// when memory runs out
typedef struct PortHostSignal PortHostSignal;
PortHostSignal *port_host_signal_create(void);
unsigned port_host_signal_count(PortHostSignal *signal);
void port_host_signal_raise(PortHostSignal *signal);
unsigned port_host_signal_wait(PortHostSignal *signal, unsigned seen);

// --- Files and the environment ---------------------------------------------------------------------------------------

// Makes a directory; 0 when made, -1 with errno set otherwise (EEXIST when there is one already)
int port_host_make_directory(const char *path);

// Sets an environment variable of the process, or removes it with a NULL value; 0 when done
int port_host_set_environment(const char *name, const char *value);

// Makes path a symbolic link to target, which may be relative to the link's directory; 0 when made, -1 with errno set
// otherwise: ENOSYS on Windows, where the seam makes no links, since the saves follow none there
// (port/hal/card/backup.c). The saves' tests use it, and skip on ENOSYS (port/tests/rom)
int port_host_symbolic_link(const char *target, const char *path);

// Whether path is a symbolic link itself
int port_host_is_symbolic_link(const char *path);

// The directory for the tests' temporary files: TMPDIR, else /tmp; the user's temporary folder on Windows
const char *port_host_temporary_directory(void);

#endif // PORT_HAL_HOST_HOST_H
