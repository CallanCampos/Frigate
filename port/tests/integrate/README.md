# Integration tests of actual game and SDK code

These fixtures link Frigate's real game/SDK objects. Where a fixture includes an original source file to reach private
state, its ordinary object is excluded from that test executable. Other dependencies remain the actual objects.

| Module | Responsibility |
| --- | --- |
| [test_integrate.c](test_integrate.c) | Whole-program scenarios: DMA, arena/heap, input, screens and overlay reset. |
| [ovsec_probe.c](ovsec_probe.c) | Overlay-section state observed across loads. |
| [script_offsets.c](script_offsets.c) | Actual script jumps/calls and actor movement offsets. |
| [vm_dispatch.c](vm_dispatch.c), [check_vm_dispatch.py](check_vm_dispatch.py) | Actual VM callback signatures and fatal function-sanitizer cases. |
| [encounter_dispatch.c](encounter_dispatch.c) | Actual encounter callbacks and argument widths. |
| [archive_seek.c](archive_seek.c) | Actual archive member offsets and bounds refusal. |
| [pokedex_touch.c](pokedex_touch.c), [pokedex_touch_scanner.c](pokedex_touch_scanner.c) | Actual habitat touch tables and GFL scanner, with fatal ASan/UBSan on the scanner itself. |
| [pokedex_form.c](pokedex_form.c) | Actual list picture selection and the form passed to the personal-data loader. |

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
