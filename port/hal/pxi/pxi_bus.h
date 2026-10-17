// The PXI FIFO between the ARM9 and the host's ARM7 (port/hal/pxi). The ARM9's side is NitroSDK's PXI API, which this
// module provides in place of pxi_fifo.c and pxi_init.c: the decomp's libraries (pm.c, tp.c, nvram.c, os_reset.c,
// ctrdg.c, snd_command.c, ...) send words with their tag and get the ARM7's words through the callback of the tag, as
// on the DS. The ARM7's side is host C: one service for each tag, which gets the ARM9's words in the order they are
// sent and answers with port_pxi_reply. Words carry 26 bits of data and an error bit, as on the DS.
//
// When an answer comes: a service runs inside the ARM9's PXI_SendWordByFifo, so it sees each word as the ARM9 sends it.
// What it answers is queued in the ARM9's receive FIFO, which raises the FIFO's interrupt (port_os_request_irq), and
// PXI_SendWordByFifo takes pending interrupts before it returns (port_os_poll_irq). So an answer reaches the ARM9's
// callback before the call that sent the request returns, unless the ARM9 has interrupts disabled, in which case the
// callback runs when they are enabled again, or when the ARM9 reads the FIFO itself (PXIi_HandlerRecvFifoNotEmpty, as
// PMi_WaitBusy does in a handler). That is the rule of the fibers spike (port/spikes/os-fibers): the ARM9 code that
// busy-waits for an answer, with no SDK call in its loop, finds it there. A service may also send words of its own
// accord, from the ARM7's frame work (port/hal/pxi/arm7.c): they are taken at the ARM9's next defined point.
//
// The ARM7's services start when the ARM9 starts the FIFO (PXI_InitFifo, the first thing OS_Init does), which is where
// the DS's ARM9 waits for its ARM7: arm7.c fills the system area of the shared memory as the ARM7 and the firmware
// leave it at boot and registers the services. Another module may register a service of its own at any time.
#ifndef PORT_HAL_PXI_PXI_BUS_H
#define PORT_HAL_PXI_PXI_BUS_H

#include "types.h"
#include <stddef.h>

// The 26 bits of data of a word
#define PORT_PXI_DATA_MASK 0x03ffffff

// The ARM7's side of a tag: called with each word the ARM9 sends with the tag, the error bit included (the ARM9 sends
// back with it a word of the ARM7 that it has no callback for)
typedef void (*PortPxiService)(u32 tag, u32 data, BOOL err);

// Sets the ARM7's service of a tag, and its bit in the shared memory's table of the tags the ARM7 is ready for
// (OSSystemWork's pxiHandleChecker), which the ARM9's libraries wait for (PXI_IsCallbackReady); NULL takes both away.
// A word of a tag that has no service is sent back with the error bit, as the DS's ARM7 does
void port_pxi_register_service(u32 tag, PortPxiService service);

// Sends a word from the ARM7's side to the ARM9's callback of the tag: queued in the ARM9's receive FIFO and taken
// at the ARM9's next defined point (see above). Data beyond 26 bits is dropped
void port_pxi_reply(u32 tag, u32 data, BOOL err);

// The words waiting in the ARM9's receive FIFO, for tests
u32 port_pxi_pending(void);

// Sets the whole bus back to before PXI_InitFifo: no services, no ARM9 callbacks, an empty FIFO. The services are
// registered again by the next PXI_InitFifo. For tests and a restart of the game in place; the game's libraries keep
// their own state (PM_Init and the like do nothing the second time)
void port_pxi_reset(void);

// The exit status of a program whose main memory arena breaks port_pxi_pointer's rule (below)
#define PORT_PXI_EXIT_STATUS 67

// A protocol that carries an ARM9 pointer as a 32-bit value cuts a 64-bit host pointer (nvram.c's buffers, the sound
// commands' arguments, card.h's and fs_memfile.c's addresses). port_pxi_pointer gets the host pointer back from the
// low 32 bits, from the places the ARM9 keeps such data: the program's static data and the main memory arena. Each is
// far smaller than 4 GiB, and the arena has no address with the low 32 bits of a static one (port_pxi_alloc_arena), so
// the low 32 bits name at most one address in both, wherever the host put them. NULL when neither has one. end, when
// not NULL, gets the end of the place, so that a service can bound its accesses (pointer.c)
void *port_pxi_pointer(u32 address, const u8 **end);

// The ARM9's side of a protocol may also note each pointer it is about to send, under the protocol's tag, with the
// size of the buffer when it is known (0 when not): that bounds the buffer, and finds one that is in neither place (a
// local on a thread's stack). port_pxi_noted_pointer looks for the low 32 bits in the tag's notes first, the newest
// first (a noted pointer with those bits, or a noted buffer that holds them), then as port_pxi_pointer does; end then
// gets the end of the noted buffer when its size is known, else of the place. Only the tag's own services see its
// notes, so a protocol that notes nothing never gets another protocol's buffer
void port_pxi_note_pointer(u32 tag, const void *pointer, u32 size);
void *port_pxi_noted_pointer(u32 tag, u32 address, const u8 **end);

// Memory for the main memory arena, for the OS layer: size bytes, zeroed, as calloc gives them (free releases them),
// where no address has the low 32 bits of one of the program's static data. NULL when the host has none.
// port_pxi_observe_arena (arm7.h) stops the program with PORT_PXI_EXIT_STATUS when it sees an arena that breaks the
// rule, before any of its pointers can be taken for a static one. port_pxi_unique_low_bits tells whether the memory
// from start to end keeps it
void *port_pxi_alloc_arena(size_t size);
BOOL port_pxi_unique_low_bits(const void *start, const void *end);

#endif // PORT_HAL_PXI_PXI_BUS_H
