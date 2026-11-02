<!-- The guide to port/third_party: the third-party code Frigate compiles from its own tree, each library's version,
origin, checksums, license and users, and how to add or update one (ADR 0031). -->
# port/third_party/

Third-party code that Frigate compiles from its own tree ([PLAN.md §4.3](../docs/PLAN.md#43-directory-structure)), kept
by the convention [ADR 0031](../docs/adr/0031-per-game-frame-digests-against-a-content-addressed-oracle-cache.md)
proposes:

- **One directory a library**, named after it, holding its files byte for byte as its release has them, and its
  license text in a file beside them (`LICENSE`). Its files keep their own headers: unlike Frigate's own files, they
  don't open with Frigate's description of them, so that their checksums stay the release's; this guide's row
  describes them instead.
- **A row below** for each library: its version, where its files came from, each file's SHA-256, what uses it and the
  part of the game that serves, and what checks it.
- **Updating one** replaces its files with another release's, unchanged, and updates its row and its checks in the same
  change.
- **Its license travels with what links it.** Every library here has a license that GPL-3.0 code may include. A
  program or app that is given to others carries the notices its libraries' licenses ask for.

Larger dependencies that Frigate builds against but doesn't keep are fetched instead, pinned by SHA-256, when no
installed copy serves: SDL3 ([../CMakeLists.txt](../CMakeLists.txt)) and, for the oracle alone, melonDS
([../tools/oracle/melonds.cmake](../tools/oracle/melonds.cmake)).

| Library | Version and license | Files | Origin | Used by | Checked by |
| --- | --- | --- | --- | --- | --- |
| [xxhash/](xxhash) | xxHash 0.8.3 (`XXH_VERSION_NUMBER` 803), BSD-2-Clause ([xxhash/LICENSE](xxhash/LICENSE), the notice from the header's first lines) | `xxhash.h`, SHA-256 `17973c0dc49d9854ca26caa191f0e12f7a424b68858d9a78de3860d959d85e4b` with its LF line endings, as committed | xxHash's single header as melonDS 1.1's source archive bundles it (`src/xxhash/xxhash.h`), the archive that [melonds.cmake](../tools/oracle/melonds.cmake) pins by its SHA-256, `61e339bc…26fd`; xxHash's home is [github.com/Cyan4973/xxHash](https://github.com/Cyan4973/xxHash) | [../tools/oracle/digest.c](../tools/oracle/digest.c), the per-game-frame digest's hash, XXH3-64 with seed 0, inlined with `XXH_INLINE_ALL` (the screens and everything they are made from, [ADR 0031](../docs/adr/0031-per-game-frame-digests-against-a-content-addressed-oracle-cache.md)); only its tests link it so far | `digest_format_plain` and `digest_format_sanitized` ([../tests/oracle/test_digest.c](../tests/oracle/test_digest.c)): XXH3-64 against xxHash's own known answers for each of its paths through an input, empty to 403 bytes. XXH3's output is fixed from xxHash 0.8.0, so a later release gives the digest's hashes unchanged |

The change that compiles the digest into Frigate's program also makes the program and the macOS app carry xxHash's
notice, as its license asks of a binary.
