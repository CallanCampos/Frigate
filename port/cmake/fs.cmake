# NitroSDK's real archive/file/default command flow on synthetic FAT/FNT data. Tests SystemInit's and DWC utility's
# caller-owned table caches without a ROM, renderer or scheduler. Synchronous OS/memory and ROM-header/DSi
# environment boundaries are fixtures.
add_executable(port_fs_table_tests tests/fs/test_archive_tables.c
    hal/fs/fs_command_default_host.c
    "${REPO_ROOT}/lib/nitro/src/fs/fs_archive.c"
    "${REPO_ROOT}/lib/nitro/src/fs/fs_file.c"
    "${REPO_ROOT}/lib/nitro/src/std/std_string.c")
target_link_libraries(port_fs_table_tests PRIVATE port_decomp_env)
target_include_directories(port_fs_table_tests PRIVATE "${REPO_ROOT}")
add_test(NAME fs_archive_tables COMMAND port_fs_table_tests)
