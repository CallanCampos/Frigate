// The host operating system's services that the platform layer, the program and the tests share, where macOS, Linux
// and Windows differ: memory pages and their protection, faults (a trap placeholder touched, a fiber's stack
// overflowing into its guard, a crash), backtraces and the names of the program's functions and globals, the watchdog
// that stops a frame which never ends, writes to stderr that take no lock, directories, the environment and the
// program's static data. It touches no part of the game itself: it is what the code above it stands on, so that
// trap.c, fiber.c and the tests are the same on every host (port/docs/adr/0013). A module whose own feature is a
// host's keeps its branches instead (the saves' folder and files in port/hal/card/backup.c, the VRAM's aliasing in
// port/hal/gx/vram_os.c, the mapped ROM in port/hal/rom/rom.c, the overlays' sections by object format in
// port/hal/fs/ovsec.c, the RTC's local time, the macOS app's log, the window's icon); port/hal/README.md lists them.
// Each host's half is one file, chosen by the compiler: host_posix.c for macOS and Linux, host_windows.c for Windows;
// the other compiles to nothing. Its users: port/hal/trap.c (traps and crashes), port/hal/os/fiber.c (the fibers'
// stacks and guards), port/hal/pxi/pointer.c (the static data), port/hal/os/os_terminate.c (backtraces),
// port/hal/gx (the page size), port/app/main.c (the watchdog, --run-dump's globals), port/app/run_dir.c, and the tests'
// harnesses. Plain C types only.
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
// SIGBUS, from any code. On Windows it is a memory fault at an address port_host_protect_none or
// port_host_protect_stack_guard made inaccessible, any memory fault on a fiber or in the program's own code, and one
// that nothing else handled; Windows also reports, with or without a handler, every exception nobody handles (an
// illegal instruction, a division by zero, a memory fault without a handler), as "port: crash: exception ... at
// address ..." with a backtrace, on a fiber too, where the system itself would end the process silently
void port_host_set_fault_handler(PortHostFaultHandler handler);

// Prints "Backtrace:" and the frames of the running code to stderr, innermost first, each with its function's name
// where the program's symbols give one. Inside a fault handler, the frames are those of the code that faulted
void port_host_print_backtrace(void);

// The function whose code holds address: its name and how far into it address is. 0 when no symbol covers it: on
// macOS and Linux, dladdr names the exported functions of any library loaded (the C library's, SDL's) besides the
// program's; on Windows, only the program's own COFF symbols name anything, so another module's code gives 0. A
// stripped program gives 0 for its own code too
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
