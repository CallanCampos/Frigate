# Test source tools

| File | Actual owner and contract | Test |
| --- | --- | --- |
| [test_project_tr_ai_tables.py](test_project_tr_ai_tables.py) | [../../tools/project_tr_ai_tables.py](../../tools/project_tr_ai_tables.py) selects exact definitions/types from `src/ov170/tr_ai.c` for the actual-command/VM regression. Tests original-source byte retention, propagation of edits, comment/string/character braces, rejected missing/duplicate/malformed boundaries and preservation of an existing output on failure. | `integrate_tr_ai_projection` |
| [test_gen_stubs.py](test_gen_stubs.py) | [../../tools/gen_stubs.py](../../tools/gen_stubs.py) fails the build when one of the game's objects calls the host C library's `exit`, where the DS has MSL's, which returns ([../../cmake/msl.cmake](../../cmake/msl.cmake)). On three objects it compiles with the build's compiler: a game file that calls `exit` fails the generation, named, and no stubs are written; the same file with [../../hal/msl/exit_calls.h](../../hal/msl/exit_calls.h) first calls `port_msl_exit` and passes; a port file that calls the host's `exit` passes. | `msl_exit_game_objects` |

Run with `PYTHONDONTWRITEBYTECODE=1 python3 port/tests/tools/test_project_tr_ai_tables.py`, or the registered CTest.
No ROM, extracted data, game build or platform linker is needed. The companion native test compiles the generated
source with the normal SDK headers and fatal sanitizer policy; these tests do not replace that compile/runtime check.

`test_gen_stubs.py` needs the build's C compiler and, off macOS, the `--nm` and `--compiler` options the build gives
`gen_stubs.py` ([../../CMakeLists.txt](../../CMakeLists.txt)), so run it through its CTest,
`ctest --test-dir <native-build> -R msl_exit_game_objects`. No ROM is needed: the version's symbols come from
`config/`.
