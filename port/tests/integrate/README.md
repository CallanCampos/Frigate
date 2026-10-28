# Integration tests

These tests exercise actual game/SDK owners and their host interfaces without replacing the game interpreter.
Registered targets and tests live in [../../cmake/integrate.cmake](../../cmake/integrate.cmake).

Where a fixture includes an original source file to reach private state, its ordinary object is excluded from that
test executable. Other dependencies remain the actual objects.

| File | Actual owner and purpose | Test |
| --- | --- | --- |
| [tr_ai_tables.c](tr_ai_tables.c) | `src/ov170/tr_ai.c` commands and `src/system/vm.c`: effect tables and result lists use the PC after the offset operand. Thirteen owned-script cases cover forward/backward table offsets and destinations, defaults, matching/nonmatching lists, sentinel and callback/stop semantics. Full native script pointers are retained. Move-effect metadata and unused heap APIs are fixtures; the commands and VM are genuine. | `integrate_tr_ai_tables` |
| [script_offsets.c](script_offsets.c) | Field script jumps/calls/return slots and actor movement offsets | `integrate_script_offsets` |
| [vm_dispatch.c](vm_dispatch.c), [check_vm_dispatch.py](check_vm_dispatch.py) | Actual VM callback dispatch and narrow sanitizer exception boundaries | `integrate_vm_dispatch_*` |
| [encounter_dispatch.c](encounter_dispatch.c) | Actual encounter creator table and SDK-typed constructors | `integrate_encounter_dispatch` |
| [archive_seek.c](archive_seek.c) | Actual archive FATB/FNTB/FIMG seeking and bounded synthetic file I/O | `integrate_archive_seek*` |
| [test_integrate.c](test_integrate.c), [ovsec_probe.c](ovsec_probe.c) | DMA, heaps, input, screen output and overlay-section integration | Other `integrate_*` scenarios |
| [pokedex_touch.c](pokedex_touch.c), [pokedex_touch_scanner.c](pokedex_touch_scanner.c) | Actual habitat touch tables and GFL scanner, with fatal ASan/UBSan on the scanner itself | `integrate_pokedex_touch` |
| [pokedex_form.c](pokedex_form.c) | Actual list picture selection and the form passed to the personal-data loader | `integrate_pokedex_form` |

Run `ctest --test-dir <native-build> -R '^integrate_tr_ai_(tables|projection)$' --output-on-failure`.
The target compiles exact source slices of the five actual AI definitions and their original headers/types/prototypes,
selected by [project_tr_ai_tables.py](../../tools/project_tr_ai_tables.py). It omits the unrelated command table and
battle initialization at generation time, so COFF/ELF linkers and ASan global registration cannot retain them.
The helper fails on missing/duplicate definitions or malformed boundaries, and CMake regenerates after source changes.
`PORT_SANITIZE=ON` applies the same fatal sanitizer policy as other native tests, without a new exception. No ROM or
authentic trainer data is embedded in the fixture. `integrate_tr_ai_projection` checks source retention and fail-closed
generation. Passing these tests proves operand sequencing, not trainer decision timing or complete battle parity.

`integrate_pokedex_touch` checks all 256×192 input points against each of the two actual tables: 98,304 searches.
A separately bounded first-hit calculation preserves valid-area precedence and inclusive edges. End markers occupy
extra physical entries; logical counts, existing coordinates and order stay unchanged. Unmatched points return no hit.

`integrate_pokedex_form` includes the actual list owner and uses typed resource/UI spies to observe six `PutMon`
flows, including both picture slots, unknown form 0, saved Unown form 7 and no-flip behavior. The fixture alone uses
pattern initialization so implicit production zero-initialization cannot conceal a missing source-level assignment.
The production object retains its normal zero-init policy. This test does not load graphics or validate assets.

Ordinary and full fatal LLVM builds run both fixtures without ROMs. Private original-source negative controls prove
an out-of-bounds read in the original touch scanner and the unknown form's uninitialized value under pattern init;
those diagnostics do not become a source-rewriting generator or a Git dependency in the permanent build.
