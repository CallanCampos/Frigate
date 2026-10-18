#ifndef PORT_HAL_TRAP_H
#define PORT_HAL_TRAP_H

#include <stddef.h>
#include <stdint.h>

// What port/tools/gen_stubs.py knows about a DS symbol that has no C yet: its name, its address in the ROM's
// memory map, its size in bytes and the module it belongs to (main, itcm, ov036, ...)
typedef struct PortTrapSymbol {
    const char *name;
    uint32_t address;
    uint32_t size;
    const char *module;
} PortTrapSymbol;

// The exit status of a trap: EX_SOFTWARE. A trap ends the program with it instead of abort(), so that tests can tell a
// trap from a crash and no crash report is written; a debugger stops at port_trap_function
#define PORT_TRAP_EXIT_STATUS 70

// Called by a function stub: the game called a DS function that has no C yet. Reports it with the caller and stops
_Noreturn void port_trap_function(const PortTrapSymbol *symbol, void *caller);

// Whether an address lies in one of the data placeholders, DS data that has no C yet. Frigate's test launcher asks it
// of overlay 36's FIELDMAP_PROC_FUNCS, and steps aside once that table has C (port/app/launcher.c, ADR 0012)
int port_trap_is_placeholder(const void *address);

// Makes the data placeholders inaccessible, so that the first access to DS data with no C yet is reported by name
// instead of reading zeros. Called once at startup, before the game runs
void port_trap_protect_data(void);

// The tables gen_stubs.py writes: every data placeholder, in the DS's address order, and the paddings defined around
// them, which the linker may not keep around them (trap.c works out the range they occupy)
extern const PortTrapSymbol port_trap_data_symbols[];
extern const size_t port_trap_data_count;
extern unsigned char *const port_trap_data_begin;
extern unsigned char *const port_trap_data_end;
extern void *const port_trap_data_addresses[];

#endif // PORT_HAL_TRAP_H
