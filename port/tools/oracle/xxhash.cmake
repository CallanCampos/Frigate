# The xxHash release whose single header gives the per-game-frame digest its hash (digest.c, XXH3-64 with seed 0; ADR
# 0031), pinned by tag and by the SHA-256 of GitHub's archive of it, as melonds.cmake pins melonDS. Read by Frigate's
# build (port/cmake/app.cmake), since its program compiles digest.c for --digest: it fetches the archive at configure
# time, as port/cmake/os.cmake fetches minicoro, puts its top directory, where xxhash.h is, on a system include path for
# the program and the digest's tests, and writes the archive's LICENSE into the notice the program carries. The digest
# touches every screen of the game: what the display hardware holds at each VBlank.
#
# 0.8.3's xxhash.h is byte for byte the one melonDS 1.1 bundles as src/xxhash/xxhash.h (both SHA-256
# 17973c0dc49d9854ca26caa191f0e12f7a424b68858d9a78de3860d959d85e4b), so the oracle compiles the same hash whichever
# copy it includes. The archive has no CMakeLists.txt at its top (xxHash's own CMake build is in cmake_unofficial/), so
# fetching it only unpacks it. xxHash is BSD-2-Clause, which this GPL-3.0 repository may include: a program given to
# others with xxHash compiled in carries its notice, which the archive's LICENSE holds. XXH3's output is fixed from
# xxHash 0.8.0; changing the pin means running digest_format_plain and digest_format_sanitized, whose known answers hold
# it (port/tests/oracle/test_digest.c).
set(DIGEST_XXHASH_TAG "0.8.3")
set(DIGEST_XXHASH_URL "https://github.com/Cyan4973/xxHash/archive/refs/tags/v${DIGEST_XXHASH_TAG}.tar.gz")
set(DIGEST_XXHASH_SHA256 "aae608dfe8213dfd05d909a57718ef82f30722c392344583d3f39050c7f29a80")
