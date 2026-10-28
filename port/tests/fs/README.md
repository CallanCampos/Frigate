# Archive-table cache regression

[test_archive_tables.c](test_archive_tables.c) runs the actual NitroSDK archive/file/default commands with synthetic
FAT/FNT bytes. Synchronous OS/memory primitives and ROM-header/DSi environment boundaries are fixtures;
unexpected scheduler waits fail the test.
[cmake/fs.cmake](../../cmake/fs.cmake) registers `fs_archive_tables` in every ordinary and sanitizer build.

It checks high native pointers, unaligned caller storage, snapshots instead of further metadata reads, payload reads
through storage, independent archives, a FAT at offset zero, adjacent and empty tables, zero-length boundaries,
query/reload/insufficient-buffer behavior, original-pointer unload, restored offsets and malformed ranges. A negative
`FS_ReadFile` result (the tested invalid-parameter case) zero-fills the cache range. Generic failure retains the
requested length and leaves its bytes unchanged, preserving the SDK's existing behavior.

```sh
cmake --build build/port --target port_fs_table_tests
ctest --test-dir build/port -R '^fs_archive_tables$' --output-on-failure --no-tests=error
```

Use a strict ASan/UBSan configuration as well. These tests verify representation and lifecycle; full game replays
and ROM verification remain separate checks.
