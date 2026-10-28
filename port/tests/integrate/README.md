# Integration tests

These tests exercise actual game/SDK owners and their host interfaces without replacing the game interpreter.
Registered targets and tests live in [../../cmake/integrate.cmake](../../cmake/integrate.cmake).

| File | Actual owner and purpose | Test |
| --- | --- | --- |
| [tr_ai_tables.c](tr_ai_tables.c) | `src/ov170/tr_ai.c` commands and `src/system/vm.c`: effect tables and result lists use the PC after the offset operand. Thirteen owned-script cases cover forward/backward table offsets and destinations, defaults, matching/nonmatching lists, sentinel and callback/stop semantics. Full native script pointers are retained. Move-effect metadata and unused heap APIs are fixtures; the commands and VM are genuine. | `integrate_tr_ai_tables` |
| [script_offsets.c](script_offsets.c) | Field script jumps/calls/return slots and actor movement offsets | `integrate_script_offsets` |
| [vm_dispatch.c](vm_dispatch.c), [check_vm_dispatch.py](check_vm_dispatch.py) | Actual VM callback dispatch and narrow sanitizer exception boundaries | `integrate_vm_dispatch_*` |
| [encounter_dispatch.c](encounter_dispatch.c) | Actual encounter creator table and SDK-typed constructors | `integrate_encounter_dispatch` |
| [archive_seek.c](archive_seek.c) | Actual archive FATB/FNTB/FIMG seeking and bounded synthetic file I/O | `integrate_archive_seek*` |
| [test_integrate.c](test_integrate.c), [ovsec_probe.c](ovsec_probe.c) | DMA, heaps, input, screen output and overlay-section integration | Other `integrate_*` scenarios |

Run `ctest --test-dir <native-build> -R '^integrate_tr_ai_(tables|projection)$' --output-on-failure`.
The target compiles exact source slices of the five actual AI definitions and their original headers/types/prototypes,
selected by [project_tr_ai_tables.py](../../tools/project_tr_ai_tables.py). It omits the unrelated command table and
battle initialization at generation time, so COFF/ELF linkers and ASan global registration cannot retain them.
The helper fails on missing/duplicate definitions or malformed boundaries, and CMake regenerates after source changes.
`PORT_SANITIZE=ON` applies the same fatal sanitizer policy as other native tests, without a new exception. No ROM or
authentic trainer data is embedded in the fixture. `integrate_tr_ai_projection` checks source retention and fail-closed
generation. Passing these tests proves operand sequencing, not trainer decision timing or complete battle parity.
