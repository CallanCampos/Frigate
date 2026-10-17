# 0007: NitroSystem's 2D files in host structs

- **Status:** accepted
- **Date:** 2026-10-17
- **Plan:** D3, D5, §4.5
- **Upstream:** `e484d5011` (merge of PR #353)

## Context

NitroSystem's G2D files (characters, palettes, cells, multi-cells, animations, fonts) are loaded whole into a heap
block, and an unpack function turns the file in place into the structs the renderer reads: each offset in a 32-bit
word becomes a pointer by adding the file's address. On a 64-bit host those structs are larger than the file's words,
so relocating in place writes over the data that follows, and the boot screens crashed copying a character block from
a cut pointer ([STATUS.md](../STATUS.md), 2026-10-17). [ADR 0005](0005-platform-boundary-as-built.md) keeps the SDK's C wherever it runs on a host, but these
18 functions can't.

## Decision

- **Host loaders replace the 18 functions that relocate in place** (`port/hal/nnsys/g2d_load.c`), with the SDK's names
  and signatures. Each builds host-layout structs in memory of its own, copies every value exactly, points raw data
  (characters, palettes, OAM attributes) into the file, and rebuilds arrays of pointers at host size. Offsets the DS
  never relocates stay offsets (`pStringBank`, frames no sequence reaches). `port/cmake/nnsys.cmake` compiles the
  SDK's files with those functions renamed `port_sdk_<name>`; no decomp file changes.
- **A tree lives as long as the memory it was built from.** A registry (`port/hal/nnsys/reloc.c`) keeps each tree by
  the file struct's address and kind, with the range of the file it read. Wrappers for every NNS FND function that
  gives memory back (`fnd_hooks.c`: expanded heap free, resize and destroy; frame heap free, free to a state and
  destroy; unit heap free) call the SDK's and then destroy the trees in what was freed. A tree's range ends at the
  last struct byte read, not at the header's size, which 3,562 character files overstate.
- **Unpacking a file again rebuilds its tree in place**, reusing every host struct when the bytes are the same, so the
  game's own edits to a tree (`clact.c` moves a palette's data and changes a character's mapping) go back to the
  file's values, as a fresh read and unpack does on the DS.
- **Proven on the ROM:** a test runs the host loaders and a 32-bit simulation of the DS's unpack on every G2D file of
  the player's ROM (30,078 in each version) and compares every reachable field and pointer target; unit scenarios
  cover what the ROM doesn't use (user extended data, VRAM transfer data, shared animation frames).

## Alternatives

- **Game changes that make every pointer field of the G2D file structs a 32-bit handle:** the structs are the SDK's
  public types, read by the renderer and the game in hundreds of places, each of which would need a conversion.
- **Larger heap blocks with the file moved to make room:** the game keeps the file's address and frees it by that
  address, and the structs still overlap whatever the game puts after them.
- **Copies made on every unpack and never freed:** the game unpacks the same files every time a screen opens, so the
  memory grows without bound, and a tree could outlive a freed file that a new one reuses.

## Consequences

- The renderer and the game read ordinary host structs; nothing else knows about the registry.
- Every FND function that frees memory needs a wrapper. A new one in the decomp fails the link until `nnsys.cmake`
  and `fnd_hooks.c` list it. `OS_FreeToHeap` isn't hooked: the game never loads a G2D file into the SDK's own heap.
- A second unpack of the same bytes with no read in between is treated as a reload; on the DS it would relocate the
  pointers twice, which no working code relies on.
- G3D and sound files need the same treatment when their loaders are ported: their own kinds in the registry, or, as
  the sound driver does ([ADR 0006](0006-sound-driver-as-built.md)), handles in the files' 32-bit words.
