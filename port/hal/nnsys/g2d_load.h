// The host's G2D file loaders (port/hal/nnsys/g2d_load.c). Every NitroSystem function that relocates a G2D file in
// place has a host version of the same name and signature there, which builds a tree of host structs from the file
// instead (port/hal/nnsys/reloc.h). This is the part under them, for the loaders and the tests.
#ifndef PORT_HAL_NNSYS_G2D_LOAD_H
#define PORT_HAL_NNSYS_G2D_LOAD_H

#include "hal/nnsys/reloc.h"
#include "nnsys/g2d.h"
#include <stddef.h>

// The tree of an NFTR file: a node for each FINF, CWDH and CMAP block, as NNSi_G2dUnpackNFT relocates each, and for any
// such struct a pointer reaches outside a block
typedef struct PortG2dFontNode {
    const void *at; // The struct in the file
    u32 kind;       // NNS_G2D_BINBLK_SIG_FINFDATA, _CWDHDATA or _CMAPDATA
    void *host;     // Its NNSG2dFontInformation, NNSG2dFontWidth or NNSG2dFontCodeMap
    BOOL inBlock;   // FALSE for a struct only a pointer reaches, whose own pointer the DS leaves an offset
    struct PortG2dFontNode *next;
} PortG2dFontNode;

typedef struct PortG2dFont {
    NNSG2dFontInformation info; // The first FINF block's, so that its address stays when the tree is rebuilt
    PortG2dFontNode *nodes;
} PortG2dFont;

// Builds the host tree of kind for the file struct at, in a file whose bytes are [file, file + fileSize), and registers
// it. A tree already registered for at is rebuilt in place: with the same host structs, every field set again from the
// file, when the file's bytes are still those it was built from, and with new ones when they aren't. With fileSize 0
// the file's extent is unknown (the Unpack functions that take a struct): reads start at at and aren't checked.
// Returns the root, the runtime struct of the kind (a PortG2dFont for PORT_RELOC_G2D_FONT, whose at is the file's
// header), which port_reloc_find(at, kind) returns too, or NULL when the file's offsets lead out of it;
// port_g2d_last_error then says where
void *port_g2d_unpack(PortRelocKind kind, const void *file, size_t fileSize, const void *at);
const char *port_g2d_last_error(void);

// The host struct a font tree built for the struct at, of kind, or NULL
void *port_g2d_font_node(const PortG2dFont *font, const void *at, u32 kind);

// How many of the banks' frames the DS relocates more than once (sequences that share them) or never (frames no
// sequence reaches, which keep their offset), over every animation bank built; the host relocates each once, or never
typedef struct PortG2dAnimStats {
    u32 framesSharedBySequences;
    u32 framesNeverRelocated;
} PortG2dAnimStats;
const PortG2dAnimStats *port_g2d_anim_stats(void);

#endif // PORT_HAL_NNSYS_G2D_LOAD_H
