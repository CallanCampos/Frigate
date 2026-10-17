// The relocation registry: the host-layout trees built for NitroSystem files that the DS relocates in place
// (PLAN.md §4.8 rule 5). On the DS a loader such as NNS_G2dGetUnpackedCellBank adds a base address to every offset of a
// loaded file, so the file's bytes become the runtime structs. On a 64-bit host those structs hold 8-byte pointers and
// can't be the file's bytes, so the host's loaders (port/hal/nnsys/g2d_load.c now; G3D and sound in phase 2) copy the
// file into host structs instead, a tree whose raw data (pixels, palettes, OAM attributes, glyphs) stays in the file
// and whose root the game gets where the DS would hand it the file's struct. The file's bytes stay as they were.
//
// Each tree is registered under the address of the file struct it stands for and its kind, with the range of memory
// it depends on: the file's bytes that the tree was built from, from its header on, in the block its raw data lies in
// too. Two things end a tree:
//   - the memory goes back to its heap. port/hal/nnsys/fnd_hooks.c wraps every NNS FND function that frees memory
//     (expanded, frame and unit heaps, and destroying a heap) and calls port_reloc_release_range with what it freed, so
//     a tree dies with the heap block its file was loaded into, as the DS's struct would;
//   - the memory is reused without a free the platform layer can see: a static or stack buffer that a later file is
//     loaded into, or a heap whose memory comes back by other means. The loaders handle that one themselves: a file
//     unpacked where a registered tree's was gets that tree rebuilt in place from its bytes, with the same host structs
//     when they are the bytes the tree was built from and new ones when they aren't (port/hal/nnsys/g2d_load.c)
//
// Everything runs on the game's thread (the OS core runs the SDK's threads as fibers on one host thread), so the
// registry takes no locks.
#ifndef PORT_HAL_NNSYS_RELOC_H
#define PORT_HAL_NNSYS_RELOC_H

#include <stddef.h>
#include <stdint.h>

// What a tree stands for. A file struct can have one tree of each kind; the loaders of one format share a kind
typedef enum PortRelocKind {
    PORT_RELOC_G2D_CHARACTER,   // NNSG2dCharacterData (NCGR's CHAR)
    PORT_RELOC_G2D_PALETTE,     // NNSG2dPaletteData (NCLR's PLTT)
    PORT_RELOC_G2D_PALETTE_CMP, // NNSG2dPaletteCompressInfo (NCLR's PCMP)
    PORT_RELOC_G2D_CELL_BANK,   // NNSG2dCellDataBank (NCER's CEBK)
    PORT_RELOC_G2D_MULTICELL,   // NNSG2dMultiCellDataBank (NMCR's MCBK)
    PORT_RELOC_G2D_ANIM_BANK,   // NNSG2dAnimBankData (NANR's and NMAR's ABNK)
    PORT_RELOC_G2D_CELL_EXATTR, // NNSG2dUserExCellAttrBank on its own (NNSi_G2dUnpackUserExCellAttrBank)
    PORT_RELOC_G2D_FONT,        // An NFTR file: its FINF, CWDH and CMAP blocks
    PORT_RELOC_KIND_COUNT,
} PortRelocKind;

// Frees a tree and everything it owns
typedef void (*PortRelocDestructor)(void *tree);

// Registers tree as the one built for the file struct at key, of kind, from the memory [begin, begin + size), which
// must hold key. A tree already registered for key and kind is replaced: destroyed when it is another tree, and only
// given the new range when it is the same one
void port_reloc_register(const void *key, PortRelocKind kind, const void *begin, size_t size, void *tree,
                         PortRelocDestructor destroy);

// The tree registered for the file struct at key, of kind, or NULL
void *port_reloc_find(const void *key, PortRelocKind kind);

// The memory range a registered tree depends on; FALSE when none is registered for key and kind
int port_reloc_range(const void *key, PortRelocKind kind, const void **begin, size_t *size);

// Destroys every tree whose memory overlaps [begin, begin + size): that memory has gone back to its heap. Returns how
// many it destroyed
size_t port_reloc_release_range(const void *begin, size_t size);

// Destroys the tree registered for key and kind, if there is one
void port_reloc_release(const void *key, PortRelocKind kind);

// How many trees are registered, in all and of one kind
size_t port_reloc_count(void);
size_t port_reloc_count_kind(PortRelocKind kind);

// Destroys every tree (the tests)
void port_reloc_release_all(void);

#endif // PORT_HAL_NNSYS_RELOC_H
