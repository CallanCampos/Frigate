# The melonDS release the oracle is built on, pinned by tag and by the SHA-256 of GitHub's archive of it. Read by the
# oracle's own build (CMakeLists.txt here) and by the port's fragment (port/cmake/oracle.cmake), which fetches it at
# configure time. melonDS is GPL-3.0, as this repository is. Changing the pin means checking README.md's notes on
# FreeBIOS, direct boot and the scheduler slot oracle.cpp borrows, and the boot hashes in port/tests/oracle.
set(ORACLE_MELONDS_TAG "1.1")
set(ORACLE_MELONDS_URL "https://github.com/melonDS-emu/melonDS/archive/refs/tags/${ORACLE_MELONDS_TAG}.tar.gz")
set(ORACLE_MELONDS_SHA256 "61e339bcb18a68a17485973637d972ea628c5624d7e6b8adf6870f895d5e26fd")
