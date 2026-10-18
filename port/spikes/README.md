# port/spikes/

Phase 0's experiments, frozen: each answered one question about the design before the platform layer was written, with
its measurements and its review, and the live code took its answer from there. Nothing here is built by Frigate's
build or by CI, and nothing here changes with the live code: a fix goes in the live file, never in its spike copy.
Read a spike for why the live code is the way it is; edit the live code.

| Spike | The question it answered | The live code now | Game functionality |
| --- | --- | --- | --- |
| [os-fibers/](os-fibers/README.md) | Can the decomp's own SDK scheduler run its threads on fibers of one host thread, with interrupts dispatched where the game waits? (Yes, with the switch, the CPSR and the hardware replaced) | [../hal/os/core.c](../hal/os/core.c), [fiber.c](../hal/os/fiber.c), [os_thread_host.c](../hal/os/os_thread_host.c), the tests in [../tests/os](../tests/os/test_os.c) | Threads, interrupts, the frame |
| [overlay-sections/](overlay-sections/README.md) | Can each overlay's writable statics sit in sections of their own, so that loading an overlay sets them back as the DS does, within Mach-O's 255 sections? | [../hal/fs/ovsec.c](../hal/fs/ovsec.c), [../tools/gen_overlay_sections.py](../tools/gen_overlay_sections.py), [../cmake/overlay_sections.cmake](../cmake/overlay_sections.cmake) | Overlays |
| [vram-alias/](vram-alias/README.md) | Can the nine VRAM banks be mapped into every view by page aliasing on macOS, Linux and Windows, with copying as the fallback? | [../hal/gx/vram.c](../hal/gx/vram.c), [vram_layout.c](../hal/gx/vram_layout.c), [vram_os.c](../hal/gx/vram_os.c), the tests in [../tests/gx](../tests/gx/test_gx.c) | VRAM, every picture |

The decisions they led to are [ADR 0005](../docs/adr/0005-platform-boundary-as-built.md) and the design sections of
[PLAN.md §4.5](../docs/PLAN.md#45-hal-and-display-model).
