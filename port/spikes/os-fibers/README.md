# Spike: the SDK's threads on fibers, with the DS's scheduling

**Frozen.** This spike is a record: its answer is the live OS core, [port/hal/os](../../hal/os/port_os.h) (`core.c`,
`fiber.c`, `os_thread_host.c`), with its tests in [port/tests/os](../../tests/os/test_os.c). Nothing here is built by
Frigate's build or CI, and nothing here changes with the live code ([../README.md](../README.md)).

PLAN.md §4.5 ("OS") says SDK threads become fibers run by one scheduler with the DS's priority rules on one host
thread, with interrupt handlers dispatched where the game waits. This spike checks how much of that the decomp's own
SDK code can do by itself, and what the host has to supply.

## Questions

1. Can the SDK's own C scheduler (thread list, queues, priorities, `OSi_RescheduleThread`) stay as it is, with only
   the context save and switch (`OS_SaveContext`, `OS_LoadContext`, the IRQ return path) replaced by fiber switches,
   each `OSThread` mapped to a fiber in a side table and no struct changed? Which functions are assembly or
   hardware-bound and must be replaced, and which are reused?
2. Which fiber library: minicoro, libco or a hand-written switch? License, AddressSanitizer support, stack sizes,
   switch cost on the M2.
3. A prototype with create, sleep, wakeup, yield, join, a mutex, a message queue, preemption on the wakeup of a
   higher thread and a VBlank IRQ dispatched from wait points, with tests of the DS's ordering, under ASan and UBSan.

## Method

The prototype doesn't reimplement the SDK. It compiles the decomp's own files, read-only from their paths:
`lib/nitro/src/os/os_mutex.c`, `os_message.c` and `os_event.c` unchanged, and `os_thread.c` through
`src/os_thread_host.c`, which `#include`s it with four functions renamed out of the way (see question 1). The
host-side layer is in `src/`:

| File | What it is |
| --- | --- |
| `src/hal_fiber.c`, `hal_fiber.h` | Fibers over minicoro: one mapping per fiber, with a 64 KiB guard region under the stack and minicoro's header below the guard; the `HalFiber` itself in the C heap; ASan annotations, LeakSanitizer root regions, stack painting for measurements. |
| `src/mco_impl.c` | minicoro's implementation, built without ASan (see the ASan finding below). |
| `src/os_host.c`, `os_host.h` | The host side of the SDK: the side table, `OS_InitContext`/`OS_SaveContext`/`OS_LoadContext`, the CPSR functions, `OS_Halt`, the ITCM IRQ handler and its thread-switch tail rewritten in C from their assembly, a virtual clock with VBlank, HBlank, V-count and alarm events gated by `DISPSTAT` as on the hardware, the frame hook run on the dispatcher's stack, `OS_WaitIrq`, `OS_WaitInterrupt`, `OS_GetTick`, `OS_SetAlarm`. |
| `src/os_thread_host.c` | The unity include of `os_thread.c`, and host versions of `OS_InitThread`, `OS_CreateThread`, `OS_KillThread`, plus a wrapper of `OS_DestroyThread`. |
| `tests/test_os.c` | 22 scenarios, each run as the launcher thread, logging events and comparing with the order the DS gives. |
| `tests/bench_os.c` | Switch costs, frame costs, fiber creation cost, stack depths. |
| `tests/crash_os.c` | Overflows of a fiber's stack (small frames, frames larger than a page, a frame larger than the guard), a write past a local array for ASan, and two LeakSanitizer checks. |

The decomp files are compiled with PLAN.md §4.4's language model (`-std=gnu99 -fsigned-char -fwrapv
-fno-strict-aliasing -ffp-contract=off -fno-common -ftrivial-auto-var-init=zero`) against the decomp's include tree;
the host files reach the SDK headers through `-iquote` so that host libc headers win for `<...>` includes. Everything
is built with stack probes (below). minicoro comes from FetchContent, pinned to commit
`02dad0f8b7cbb12fe6e216ae7a76db15ca55cd7b` (2024-12-07).

```sh
cmake -S port/spikes/os-fibers -B build/port-spikes/os-fibers
cmake --build build/port-spikes/os-fibers
ctest --test-dir build/port-spikes/os-fibers --output-on-failure
# sanitizers (Debug, Apple clang; and RelWithDebInfo with Homebrew LLVM, which has LeakSanitizer)
cmake -S port/spikes/os-fibers -B build/port-spikes/os-fibers-asan -DOS_FIBERS_SANITIZE=ON -DCMAKE_BUILD_TYPE=Debug
cmake -S port/spikes/os-fibers -B build/port-spikes/os-fibers-llvm-asan -DOS_FIBERS_SANITIZE=ON \
      -DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang
```

## Results on this Mac

Apple M2, macOS 26.5.1, Apple clang 21.0.0, Homebrew clang 23.1.3, CMake 4.4.4, page size 16 KiB.

**Tests.** All 26 CTest entries pass in RelWithDebInfo (Apple clang). Under `-fsanitize=address,undefined` all 28
pass with Apple clang at `-O0` (two more: the ASan detection test, and every scenario rerun with
`detect_stack_use_after_return=1`), and all 30 with Homebrew clang at `-O2` (two LeakSanitizer tests more), also with
`ASAN_OPTIONS=detect_leaks=1` set for the whole run: the tests that set ASan options append theirs
(`ENVIRONMENT_MODIFICATION` `string_append`, CMake 3.22+), so the caller's stay on. Every scenario also passes with
every host stack cut to 16 KiB (one page), in all three builds. The scenarios, each checking an exact event order:

| Scenario | DS behaviour it shows |
| --- | --- |
| `wakeup_preempts` | Waking a higher thread switches to it inside `OS_WakeupThreadDirect`; a lower one runs only when the waker blocks (`OS_JoinThread`). |
| `yield_order` | `OSi_InsertThreadToList` puts a new thread before the others of its priority, so B (created after A) runs first; `OS_YieldThread` alternates them: B0 A0 B1 A1 B2 A2. |
| `queue_wake_order` | `OS_WakeupThread` readies a whole queue, and the threads then run in thread-list order (priority, newest first), not queue order. |
| `join_destructor` | A returning thread ends through `OS_ExitThread`; its destructor runs in it before joiners wake. |
| `mutex` | Recursive locking; waiters sorted by priority; the unlock's wakeup preempts the unlocker; no priority inheritance. |
| `message_queue` | A full two-message queue blocks the sender, each receive wakes it and it preempts the lower receiver; `OS_JamMessage` goes first. |
| `vblank_and_alarms` | The virtual clock orders `OS_Sleep(5)` alarms and VBlanks (12.2 ms into each 16.7 ms frame): wakes at 5, 10 ms, VBlank 1, 15, 20, 25 ms, VBlank 2, … The handler runs in IRQ mode at line 192. |
| `irq_wakeup_deferred` | A wakeup inside a handler only sets `isNeedRescheduling`; the tail switches after the handler. |
| `irq_while_disabled` | An IRQ raised while the I bit is set waits for `OS_RestoreInterrupts` (taken at line 51, 20 ms in); with I clear it is taken in the busy loop at the VBlank's own line. |
| `irq_on_switch_in` | Switching to a thread whose context has the I bit clear (here a new one) takes a pending IRQ before the thread's first instruction, as loading the DS's CPSR unmasks it. |
| `busy_wait_preempted` | A low thread busy-waiting 40 ms on `OS_GetTick` (the pattern of `ir_check.c` and `irc_spi.c`) is preempted by main at each VBlank. |
| `async_completion` | `backup_card.c`'s write: an async call, then a loop that makes no call until the completion callback sets a plain flag. It ends at once when the completion comes inside the call, or from a higher task thread that the call wakes (CARD's shape) with the reply given inside the request. A completion delivered as a later interrupt leaves the flag clear after 1,000,000 turns with the clock not moved; only a loop that waits through `hal_os_spin` (`OS_SpinWaitSysCycles`, `SVC_WaitByLoop`) sees it (after 34 turns). |
| `scheduler_lock` | `OS_DisableScheduler` holds back `OSi_RescheduleThread`, but the IRQ tail switches anyway: the ITCM assembly doesn't read the lock count. |
| `destroy_and_recreate` | `OS_DestroyThread` on a sleeping thread disposes of its suspended fiber; 1000 create-run-exit cycles on one `OSThread` (the sound loader's pattern) leave no fiber behind. |
| `kill_thread` | `OS_KillThread` restarts the context at `OSi_ExitThread` with the destructor's argument and takes it out of its queue. |
| `switch_callback` | The switch callback sees every switch, the tail's included (main→H, H→main, main→idle, idle→H, H→main). |
| `hblank_lines` | Per-line HBlanks while main waits for VBlank from line 0 with no logic: lines 0–191 in order, and main is switched in 193 times, since every IRQ wakes `OS_WaitIrq`'s queue. |
| `hblank_main_loop` | `main.c`'s loop as BW2 runs it, HBlank on for good: 100 `OS_GetTick` calls of logic take 3 lines after the VBlank, and main then waits through the other 260 lines; per frame 263 HBlanks and 261 switches into main. |
| `dispstat_gates_blanks` | `DISPSTAT`'s enables decide whether the display requests VBlank, HBlank and V-count IRQs; IE only whether a request is taken. With IE's HBlank bit set and `DISPSTAT`'s clear (as `net_err.c` and `save_error.c` leave them) no HBlank comes; with the VBlank's clear too, frames still end and are presented but IF stays 0; a V-count IRQ comes at the start of its line. |
| `frame_hook_on_host_stack` | The frame hook runs on the dispatcher's stack, never a fiber's, at each VBlank and before its handler: when a busy wait on `OS_GetTick` crosses it, when a spin with interrupts disabled crosses it (the frame is presented then, the handler waits for `OS_RestoreInterrupts`), and in the idle thread's halt. |
| `deadlock` | All threads waiting with nothing in IE ends the run with an error instead of a hang. |
| `determinism` | A frame loop shaped like `main.c` (sound thread slept around the VBlank wait, a stream thread on `OS_Sleep(4)`, a mutex, a message queue, `OS_Sleep(1)` polling) runs twice with identical logs. The hash, `7ce13618`, is the same in all three builds (Apple clang -O2, Apple clang ASan -O0, LLVM ASan -O2). |

**Costs** (RelWithDebInfo, three runs each, ranges; other sessions were building, load average about 8, and runs at
load 20–30 took up to twice as long):

| Measure | Time |
| --- | --- |
| Raw minicoro switch (one resume or one yield) | 16–18 ns |
| SDK thread switch through the decomp's scheduler (wake/sleep ping-pong; two fiber switches through the dispatcher each) | 58–59 ns |
| An empty frame: `OS_WaitIrq`, idle thread's halt, clock to VBlank, handler, tail, back to main | 112–115 ns |
| A frame of `main.c`'s loop with HBlank on: 100 `OS_GetTick` calls, then 260 HBlank wakeups of main | 28.5–28.9 µs |
| Fiber create + first run + destroy (`calloc`, `mmap`, `mprotect`, `munmap`), 64 or 256 KiB | 4.0–7.4 µs |

Under ASan an SDK switch costs 248 ns at `-O0` (Apple clang; unoptimized, instrumented SDK code and the spike's
linear side-table lookup) and 82 ns at `-O2` (LLVM); the HBlank frame 137 µs and 47 µs. That frame is 0.17% of a
16.7 ms frame optimized, and 0.8% under ASan at -O0.

**Stack depth** (bytes of host stack touched, measured by painting):

| Work | -O2 | ASan -O0 | ASan -O2 |
| --- | --- | --- | --- |
| SDK blocking paths (mutex, send, receive, `OS_Sleep`) | 404 | 2,840 | 944 |
| `snprintf` with `%f`, `%e` | 2,208 | 5,680 | 5,136 |
| Four nested 1 KiB-local frames (DS style) | 5,424 | 7,808 | 6,656 |

The DS stacks of BW2's threads are 0xc8 (idle), 0x400 (sound loader, card), 0x800 (CPS, POKE_NET, ND), 0x1000
(stream, camera, DSP) and 0x2000 (NHTTP) bytes. Host C uses more: 64-bit pointers and saved registers, libc formatting,
and ASan's redzones (2–3× here). These workloads are small; game code is deeper, so the numbers set a floor, not a
size.

### Finding: a one-page guard can be stepped over, and some compilers don't probe

The first version had a one-page guard with minicoro's header (the coroutine and its saved registers) right below it
in the same mapping. A frame larger than a page moves `sp` past such a guard without touching it unless the compiler
probes the stack, and then overwrites the fiber's own saved context instead of faulting. Which compilers probe, for a
40,000-byte local (`clang -O2 -S`):

| Compiler and target | Probes |
| --- | --- |
| Apple clang, arm64 macOS | `___chkstk_darwin` for frames over 4 KiB, but **none for frames that ASan realigns** (to 32 bytes, any frame with an instrumented local): with ASan, `crash_frame_over_guard`'s 160 KiB frame wrote below the fiber's mapping without a fault |
| Homebrew clang 23, arm64 macOS; x86-64 and arm64 Linux | none by default |
| Either clang with `-fstack-clash-protection`, macOS | none: the driver drops the flag for Mach-O ("argument unused during compilation") |
| Either clang with `-Xclang -fstack-clash-protection`, macOS | an inline probe per 4 KiB, with or without ASan |
| Homebrew clang with `-fstack-clash-protection`, Linux | an inline probe per 4 KiB |
| clang for x86-64 MinGW, x86-64 and arm64 MSVC | `__chkstk` by default |

The fix has two parts. The guard is now 64 KiB of `PROT_NONE` (4 pages here, 16 on x86-64 Linux), and the `HalFiber`
moved to the C heap, so only minicoro's header is below the guard; every frame of the game's code fit a DS stack of at
most 16 KiB, so none comes near 64 KiB even at 2–3 times the size. And the build turns stack probes on:
`-Xclang -fstack-clash-protection` on macOS (a frontend flag, the only way to get probes there for both compilers),
`-fstack-clash-protection` on Linux, the default `__chkstk` on Windows. minicoro's layout puts its header below the
stack (`_mco_create_context`: coroutine, context, storage, stack), so the header can't move above the stack without
another backend; the large guard is what keeps it out of reach. Three crash tests check this in every build:
`crash_guard_page` (1 KiB frames), `crash_big_frames` (two nested 44 KiB frames, caught by the guard alone) and
`crash_frame_over_guard` (one 160 KiB frame, larger than stack and guard together, caught only by the probes). Built
with Homebrew clang without the probe flag, the last one reports "fault outside the guard page"; with it, "fault in
guard page".

**ASan** reports a `stack-buffer-overflow` in a fiber located in the fiber's frame ("is located in stack of thread T0
at offset 48 in frame"). The test now requires that text and fails on "wild pointer", which is what ASan says of the
same write when it doesn't know the fiber's stack (checked with a copy of `hal_fiber.c` whose annotations were
removed); the earlier regular expression, `stack-buffer-overflow`, accepted both.

### Finding: minicoro's ASan annotations miss switches back to the host stack

minicoro annotates a switch into a coroutine and back only when the other side is also a coroutine; when the
resumer is the thread's own stack (`prev_co == NULL`), its `_mco_prepare_jumpout` calls
`__sanitizer_finish_switch_fiber` but never `__sanitizer_start_switch_fiber` for the return. ASan then believes the
thread runs on the last fiber's stack. With LeakSanitizer on, the leak check at exit logged "stack pointer not in
stack range" and scanned the last fiber's (unmapped) stack range: `bench_os` hung there for 100 s. Worse, a stock
minicoro build gives a clean leak check that means nothing: with minicoro's own annotations and LSan on, a program
that leaks 3,300 bytes after resuming a coroutine reports no leak at all, while the same program without the
coroutine reports it. This dispatcher design
resumes every fiber from the host stack, so it hits the gap on every switch. The fix in the spike: build minicoro's
implementation without ASan (`src/mco_impl.c`, `-fno-sanitize=address`) and make the annotations in `hal_fiber.c` in
the order the interface asks for (`start` on the old stack naming the new one, `finish` on the new stack as soon as it
runs, `NULL` for a fiber's last departure)
([common_interface_defs.h](https://raw.githubusercontent.com/llvm/llvm-project/main/compiler-rt/include/sanitizer/common_interface_defs.h)).
`lsan_reports_leak_after_fibers` checks that LSan still reports a real leak after 1,001 fiber runs with one fiber
suspended. Apple clang's ASan doesn't support LSan on macOS at all ("detect_leaks is not supported on this platform"),
although the [LeakSanitizer docs](https://clang.llvm.org/docs/LeakSanitizer.html) list macOS; Homebrew LLVM's does,
so the two LSan tests are registered for every sanitizer build except Apple clang's and Windows'.

### Finding: LeakSanitizer doesn't scan suspended fibers

LSan scans the stacks and registers of threads. A block that only a suspended fiber points to (a local of an SDK
thread that is waiting, which is every thread at exit) was reported as a direct leak. `hal_fiber.c` now registers
each fiber's whole mapping as a root region (`__lsan_register_root_region`), which covers the stack and minicoro's
saved registers; LSan skips the inaccessible guard between them
([lsan_interface.h](https://raw.githubusercontent.com/llvm/llvm-project/main/compiler-rt/include/sanitizer/lsan_interface.h)).
`lsan_scans_suspended_fibers` checks it.

## Answer 1: keep the SDK's scheduler, replace the switch and the hardware

Yes. Every scheduling decision in the tests is made by the decomp's own C. The SDK keeps the switch in a two-call
contract that maps onto fibers exactly:

- `OS_SaveContext` returns 0 having saved, and 1 when `OS_LoadContext` later continues from it. Its only caller in
  the whole tree is `OSi_RescheduleThread`, which returns at once on 1, and nothing in it follows `OS_LoadContext`.
  So the host `OS_SaveContext` notes that the thread can be continued and returns 0, and `OS_LoadContext` yields the
  running fiber to the dispatcher, which resumes the target's. When the thread is picked again its fiber returns
  from `OS_LoadContext`, which is the same as the DS's return with 1. A thread that loads without saving has ended
  (`OSi_ExitThread_Destroy`), so its fiber is disposed of.
- The IRQ return path (`func_01ff8ce0`, NitroSDK's `OS_IrqHandler_ThreadSwitch`) is assembly only. Its C version
  wakes `OSi_IrqThreadQueue`, honours `isNeedRescheduling`, picks the first ready thread of the list, calls the
  switch callback and sets `current`; the dispatcher then resumes that fiber, and the interrupted one stays where it
  yielded, which is its saved context.
- The side table is keyed by `&thread->context`; no SDK struct changes. Each entry has the fiber, the real entry and
  argument, and the CPSR I bit the thread left with, which is per thread on the DS (each context has its own CPSR).
  Resuming a thread whose I bit is clear first takes any pending IRQ, as loading its CPSR would.

What has to be replaced or reused, file by file (`lib/nitro/src/os/`):

| Function | Why | On the host |
| --- | --- | --- |
| `OS_InitContext`, `OS_SaveContext`, `OS_LoadContext` (`os_context.c`) | assembly | the fiber side table |
| `OS_IrqHandler`, `func_01ff8ce0` (ITCM, no C) | assembly | `DispatchIrqs` and `IrqThreadSwitch` in C, from the asm |
| `OS_Enable/Disable/RestoreInterrupts`, `_IrqAndFiq` pair, `OS_GetCpsrIrq`, `OS_GetProcMode` (`os_system.c`) | assembly (CPSR) | a per-fiber I bit; `OS_GetProcMode` gives 0x12 inside handlers, which is how `OSi_RescheduleThread` and 4 libraries (PM, SOC, FATFS) defer work |
| `OSi_SpinWaitCpuCycles` (`os_system.c`, so `OS_SpinWaitSysCycles` and the other `OS_SpinWait*`), `SVC_WaitByLoop` (BIOS) | assembly busy loops | **modelled busy loops**: `hal_os_spin` for the loop's length, which moves the clock and takes IRQs on the way. Their callers wait for state that an IRQ delivers: `PMi_SendPxiDataSync` (`pm.c:302`), `CARDi_Request`'s wait for the ARM7 (`card_request.c:26`), `PM_Init` (`pm.c:208`). A no-op would hang them |
| `OS_Halt` (`os_terminate_proc.c`) | assembly | yields to the dispatcher, which moves the clock to the next event |
| `OS_InitThread` | writes the launcher's stack magic into DTCM, reads linker-symbol sizes, stores a pointer at the 32-bit word `HW_THREADINFO_MAIN` (0x02ffffa0) | host version |
| `OS_CreateThread`, `OS_KillThread` + `OSi_KillThreadWithPriority` | `(u32)func`, `(u32)arg`, `(u32)stack`, `stackBottom - 8`: 64-bit pointers truncated (`os_thread.c` lines 282–293, 396–400) | host versions that give the side table the real entry |
| `OSi_ExitThread_ArgSpecified`'s exit-stack branch | same truncation (lines 317–320) | kept: dead in BW2, `sThreadState.exitStack` is never set; the host aborts if it ever runs |
| `OS_DestroyThread` | reused | wrapped so that the host disposes of the destroyed thread's fiber first |
| `OS_WaitIrq` (`os_irqHandler.c`), `OS_WaitInterrupt` (`os_system.c`) | C, but `HW_INTR_CHECK_BUF` is `HW_DTCM + 0x3ff8` and `HW_DTCM` is `(u32)SDK_AUTOLOAD_DTCM_START` | reusable unchanged once `HW_DTCM` is a `uptr` over a host DTCM block (PLAN.md §4.8 rule 3); the spike carries copies |
| `os_alarm.c`, `os_tick.c`, `os_timer.c`, `os_valarm.c` | timer registers, `OSi_AlarmHandler` is assembly | virtual clock (`OS_GetTick` is a modelled busy-loop step too); the spike has a minimal alarm list on timer 1 |
| everything else in `os_thread.c` (queues, sorted list, `OSi_RescheduleThread`, sleep, wakeup, yield, join, priorities, scheduler lock, `OS_Sleep`, the exit path), `os_mutex.c`, `os_message.c`, `os_event.c`, `os_irqTable.c` | pure C | reused unchanged |

`os_interrupt.c` is also reusable once `reg_OS_IE`/`IF`/`IME` point into the HAL's register file, and the clock reads
`DISPSTAT`'s interrupt enables and V-count setting from the same file (the spike stands in for it with
`hal_os_set_dispstat`). The divider and square-root context that the DS swaps at each switch
(`CP_SaveContext`/`CPi_RestoreContext`) has a home in `OSContext.cp_context`; if the HAL models the divider as
registers, the host switch should save and restore it there.

Two ways to deal with the 64-bit sites in `os_thread.c`:

- **What the spike does (recommended now):** the HAL compiles `os_thread.c` only through its own file, which
  `#define`s `OS_InitThread`, `OS_CreateThread`, `OS_KillThread` and `OS_DestroyThread` to other names, includes it,
  and defines the real ones next to the statics they need. `os_thread.c` goes in `hal/replaced.txt`. No decomp file
  changes and no game-changes rows; the cost is that the HAL file depends on `os_thread.c`'s static names, which are
  the SDK's and won't move.
- **Later, if the `uptr` work reaches it:** retype `OSContext`'s registers, `OSThread.stackTop`/`stackBottom` and
  `OS_InitContext`'s parameters to `uptr`, and the ten `(u32)` casts and two `u32` locals that carry them, which MWCC
  compiles identically.
  Then only `OS_InitThread` needs a host version (or a `PLATFORM_PC` block for `HW_THREADINFO_MAIN`).

## Answer 2: minicoro, behind a four-function fiber API

| | minicoro | libco | Hand-written |
| --- | --- | --- | --- |
| License | Public domain or MIT-0 | ISC | ours |
| Backends | asm for arm64, ARM and x86-64 System V; x86-64 Windows as machine code in byte arrays placed in `.text` (`_MCO_ASM_BLOB`) and called through function pointers, saving the TIB's stack fields; Windows fibers; ucontext ([README](https://github.com/edubart/minicoro)) | asm for x86, amd64, ARM, AArch64 (AArch64 as machine code in an array placed in `.text`, or made executable with `mprotect` under `LIBCO_MPROTECT`), PPC; Windows fibers; setjmp ([README](https://github.com/higan-emu/libco)) | arm64 and x86-64 System V are ~25 instructions each; Windows x64 also needs the TIB's StackBase/StackLimit/DeallocationStack and XMM6–15 |
| Model | asymmetric resume/yield | symmetric `co_switch` | either |
| ASan | annotations present but incomplete for host-stack resumes (above) | none in the code | ours |
| Stack placement | one block, header below the stack; the spike lays it out so a 64 KiB guard fits between them | library-allocated | ours |

minicoro is the choice PLAN.md already made and the spike confirms it, with two adjustments: build its
implementation without ASan and annotate in our layer, and map its block ourselves for the guard (the spike asserts
the layout at every creation, so a minicoro update that moves the stack fails loudly). A hand-written switch is
simple on arm64 and System V, but Windows x64's TIB handling is the part minicoro already gets right; it isn't
simpler overall. Both libraries switch through machine code kept in a data array on one of their targets (minicoro on
Windows x64, libco on AArch64), so that is no difference between them; minicoro's Windows x64 path is an opaque blob
that wasn't run here. It needs a build and a run in Windows CI before it is relied on. If the Windows build ever uses
Control Flow Guard (`/guard:cf`), check it there too: CFG accepts as indirect-call targets the functions the compiler
identifies ([Control Flow Guard](https://learn.microsoft.com/en-us/windows/win32/secbp/control-flow-guard)), and a byte
array called through a cast pointer is not one, so the call would likely terminate the program. libco has no
sanitizer annotations. The HAL should keep its own `hal_fiber.h` (create, destroy, resume, yield) so the backend can
change without touching the OS layer.

**Stack sizes.** All tests pass on one 16 KiB page per fiber, even under ASan; the deepest measured work used 7.6 KiB.
Reservations cost only address space: anonymous mappings are committed page by page on first touch, and Windows'
`MEM_COMMIT` doesn't allocate physical pages until they're accessed
([VirtualAlloc](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc)).
Recommended: 256 KiB for every SDK thread and 8 MiB for the launcher (the whole game runs on it, like a native main
thread), each with a 64 KiB guard, and a debug-build high-water report per thread like the heaps' (PLAN.md §4.5).

## Portability

- **Linux:** minicoro's asm path covers x86-64 and arm64; `mmap`/`mprotect` guards, the ASan fiber interface and
  LSan's root regions are the same as on macOS, and LeakSanitizer works there. Upstream clang emits no stack probes
  without `-fstack-clash-protection`, which the build passes; with it, clang probes every 4 KiB (checked by
  cross-compiling for x86-64 and arm64 Linux). Pages are 4 KiB, so the guard is 16 pages.
- **Windows x64 (clang-cl or mingw-clang):** minicoro uses its x86-64 machine-code blob for both `__GNUC__` and
  `_MSC_VER`, and saves and restores the TIB's stack fields that SEH and stack probes read. The compilers call
  `__chkstk` for frames over 4 KiB on x86 and 8 KiB on x64
  ([_chkstk](https://learn.microsoft.com/en-us/windows/win32/devnotes/-win32-chkstk)); a frame under that moves the
  stack pointer less than 8 KiB, well inside a 64 KiB guard. Guard pages work with `VirtualProtect` (`PAGE_NOACCESS`,
  4 KiB pages), but `VirtualAlloc` reservations start at 64 KiB granularity
  ([VirtualAlloc](https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc)), which is
  fine for stacks of 64 KiB and up. The `_WIN32` branch in `hal_fiber.c` shows the shape but wasn't built here, and
  whether MSVC-style ASan frames get probes wasn't checked (Apple clang's don't).
- **Windows on ARM64:** minicoro falls back to Windows fibers (`CreateFiber`/`SwitchToFiber`, cooperative, one stack
  per fiber, `ConvertThreadToFiber` first; [Fibers](https://learn.microsoft.com/en-us/windows/win32/procthread/fibers)).
  The stack is then the system's, so the guard layout doesn't apply (the layout assert would fire); the system's
  own guard page takes its place.
- **ASan on Windows** with fibers wasn't tried; the fiber interface is in the common sanitizer API, but it needs a
  check in CI before it's relied on. There is no LSan there.
- **Determinism** holds across compilers: the scenario hash was the same with Apple clang and Homebrew LLVM, at -O0
  and -O2, because nothing depends on addresses or host time.

## Recommended design for port/hal/os

1. **One host thread, one dispatcher on its own stack.** The game's `main` runs as the launcher on a fiber; every
   SDK thread is a fiber. Fibers only yield to the dispatcher, which resumes the next one. The dispatcher's stack is
   also where IRQ handlers and the frame hook (presentation, pacing, SDL) run, so platform code never runs on a fiber
   stack or inside the game's critical sections. A VBlank only marks its frame; a fiber whose clock crosses it yields
   to the dispatcher with a frame request, the frame is presented there, and the fiber goes on where it was, its
   context untouched (it may be inside a switch callback, between `OS_SaveContext` and `OS_LoadContext`). The hook
   must not call the SDK.
2. **The SDK schedules.** Compile `os_thread.c`, `os_mutex.c`, `os_message.c`, `os_event.c`, `os_irqTable.c` and,
   once the register file and `HW_DTCM` exist, `os_interrupt.c`, `os_irqHandler.c` and `os_system.c`'s C, unchanged.
   Replace only the table in Answer 1.
3. **Interrupts at defined points.** A handler runs only when a thread halts (the idle thread, `OS_WaitInterrupt`),
   when the I bit is cleared with an IRQ pending (`OS_EnableInterrupts`, `OS_RestoreInterrupts`, and the switch to a
   thread whose saved I bit is clear, a new thread's included), or when the clock passes an event inside a modelled
   busy loop. The modelled busy loops are `OS_GetTick`, `OSi_SpinWaitCpuCycles` (so `OS_SpinWaitSysCycles`) and
   `SVC_WaitByLoop`: each advances the clock by its length and polls. Handlers run in IRQ mode on the dispatcher's
   stack, one source at a time, lowest bit first, each followed by the tail written from `func_01ff8ce0`. The I bit is
   per thread.
4. **Async services finish before they return.** Every async API that game or SDK code waits for in a loop with no
   SDK call (a pure busy wait) must do its work and call its callback before the async call returns: inline, or on a
   higher-priority thread that the call wakes, as CARD's task thread (priority 4) would be with its ARM7 request
   answered inside the request. It must never be delivered as a later IRQ, since nothing inside a pure loop can run
   the clock. That also makes the loops safe from the compiler: `backup_card.c`'s `sWriteDone` isn't volatile, but it
   is set before the loop's first test, through an external call that the compiler must assume writes it. Registers
   that code polls must never read as busy. The pure busy waits in the tree (a grep for empty loops and register polls,
   not a proof that there are no others):

   | Where | Waits for | What the HAL does |
   | --- | --- | --- |
   | `src/gfl/backup_card.c:34` (`func_0203bfd4`, from `backup_system.c:225/981/1066`: save erase, footer erase, every write) | `sWriteDone`, set by `CARD_WriteAndVerifyBackupAsync`'s callback | the CARD backup replacement writes the save and calls the callback before returning; `CARD_ReadBackupAsync` too, so that `CARD_WaitBackupAsync` and `CARD_TryWaitBackupAsync` find it done |
   | `lib/nitro/src/mi/mi_dma_gxcommand.c:40,113`, `lib/nnsys/src/g3d/gecom.c:63` | the GX command DMA's completion callback clearing a busy flag | DMA to the geometry FIFO runs at the call and calls its callback |
   | `lib/nitro/src/spi/pm.c:163` (`PMi_WaitBusy`), `:195` (VBlank count), `lib/nitro/src/spi/tp.c:505` (`TP_WaitBusy`), `lib/nnsys/src/snd/snd_waveout.c:150` | a PXI reply from the ARM7, the VBlank counter in shared memory (`HW_VBLANK_COUNT_BUF`), sound channel status | PM and TP are ARM7-bound and replaced as wholes; the sound driver (PLAN.md §4.7) updates shared state before the command call returns |
   | `mi_dma.c` (`MI_DMA_ENABLE` polls), `g3x.c`, `nnsys/g3d` (`GXSTAT` busy, `G3X_Get*` results), `tp.c` and `vct_dsp.c` (`DIVCNT`/`SQRTCNT` busy), `card_rom.c`, `pm.c:551`, `ir_check.c:59`, `irc_spi.c` (`MCCNT` busy), `dsp_if.c` (`PSTS`), `pxi_fifo.c:50`, every `PXI_IsCallbackReady`/`PXI_SendWordByFifo` retry loop | hardware finishing an operation | the operation completes at the register write; the busy bits read clear and PXI reports ready |
   | `src/system/net_err.c:374/378` and `:527/530`, `src/system/save_error.c:76` | keys, a reset, with `GX_VBlankIntr(FALSE)` and `GX_HBlankIntr(FALSE)` | the screen still has to be presented and input read: reads of the keypad registers (`PAD_Read`, `PAD_DetectFold`) are modelled busy-loop steps too, so that frames end and input arrives inside these loops |

   `ir_check.c` itself is stubbed (PLAN.md §4.5), so its `OS_GetTick` waits and `MCCNT` poll never run;
   `busy_wait_preempted` shows what the model would do with them, and `irc_spi.c` has the same shape.
5. **A virtual clock.** Time moves when every thread waits (to the next event) and by each modelled busy-loop step,
   so that busy waits end. The events are as the display raises them: the VBlank at line 192 of every frame, which
   always ends the frame and presents it, and latches IF only while `DISPSTAT`'s VBlank IRQ enable is set; HBlanks
   and the V-count match only while their `DISPSTAT` enables are set; the earliest alarm. IE, IME and the I bit then
   decide delivery. Frames are paced to 59.83 Hz (560,190 cycles a frame at the SDK's 33.514 MHz: 263 lines of 2,130
   cycles; [GBATEK, DS Video Stuff](https://problemkaputt.de/gbatek.htm#dsvideostuff)).
6. **Fibers** from `hal_fiber` over minicoro: per-fiber mapping with a 64 KiB guard between minicoro's header and the
   stack, the `HalFiber` in the heap, 256 KiB (8 MiB for the launcher), ASan annotations and LSan root regions in our
   layer, minicoro's implementation built without ASan, created lazily at a context's first run and disposed of on
   the dispatcher's stack when the thread ends, is destroyed or is killed. The HAL and the game are built with stack
   probes: `-Xclang -fstack-clash-protection` on macOS, `-fstack-clash-protection` on Linux, `__chkstk` on Windows.
7. **A side-table index** instead of the spike's linear scan (an index stored on first lookup, or a small hash), so
   that ASan builds stay fast.

### A decision for the phase-0 HAL: where HBlank handlers run

BW2 keeps the HBlank IRQ on for good (`main.c:50-54`: `OS_EnableIrqMask(V_BLANK | H_BLANK)`, `GX_HBlankIntr(TRUE)`;
the handler runs GFL's per-line task list), so this is the normal case, not a corner. PLAN.md §4.5 has the renderer
call the handler before each line at scan-out. The spike shows the other way works:

- **In the scheduler (the spike):** per-line HBlank events, handlers through `DispatchIrqs` and the tail. Threads
  wake where the DS wakes them (main 261 times a frame in `main.c`'s loop), `OS_GetProcMode` is IRQ mode in the
  handler with no special case, and the renderer reads the registers the handler writes from a per-line snapshot
  that the HAL takes at each HBlank event. It costs 29 µs a frame here, 137 µs under ASan at -O0.
- **Called by the renderer:** no HBlank events, so no switches; but the handler then runs outside the IRQ path, so
  the HAL must fake IRQ mode for it, nothing wakes `OS_WaitIrq`'s waiters, and it has to be called for all 263 lines,
  not only 0–191, if the task list counts lines. The order against game logic is the same in both, since the
  frame's logic takes no virtual time and runs right after the VBlank.

The spike's recommendation is the scheduler, for fidelity at a cost under 0.2% of a frame. Whichever is chosen
changes PLAN.md §4.5, so it wants an ADR.

## Differences from the DS, and open risks

- **Interrupts don't come at arbitrary instructions.** A thread that computes for a long time without an SDK call is
  not preempted, and time doesn't pass while it runs. That is what makes runs deterministic, but timings differ: the
  sound loader in `snd_sys.c` will finish a load inside one `OS_Sleep(1)` of main, where the DS spreads it over
  frames. A pure busy wait hangs unless the HAL completes what it waits for first (design point 4); the inventory there
  comes from a grep and has to grow with each new wait loop found in game code.
- **The per-step clock advance** of `OS_GetTick` (one tick, 64 cycles, here) and of the keypad reads is a tuning
  choice: too large and code that calls it often within a frame gets VBlanks mid-logic; too small and long busy waits
  cost many calls. It should be measured on the real game.
- **The HBlank IRQ's own timing** is the end of the 256 visible dots (1,536 cycles) in the spike; GBATEK gives 1,606
  cycles for the ARM9's HBlank flag, which `hal_os_get_dispstat` uses, but the IRQ's cycle wasn't checked.
- **The IRQ tail ignores `OS_DisableScheduler`**, as the assembly does. A thread switched to while another holds the
  lock and then blocks would keep running, and one that exits under it reaches `OS_Terminate`. That's the DS's
  behaviour; the host aborts with a message there.
- **Destroyed or killed threads' fibers are freed while suspended.** ASan's fake stack of such a fiber leaks in
  `detect_stack_use_after_return` runs (there is no API to free it from outside).
- **Stack probes on macOS rely on a frontend flag** (`-Xclang -fstack-clash-protection`), which the drivers don't
  document for Mach-O. A compiler update that drops it shows as `crash_frame_over_guard` failing; the 64 KiB guard
  still catches every frame smaller than itself.
- **LSan root regions** make anything a fiber's mapping points to reachable, stale pointers in dead frames included,
  so a leak referenced only from a dead frame of a suspended fiber is missed. LSan's interface is meant for "a small
  number of relatively static regions"; the game has about 20 threads, but the sound loader's create-destroy cycle
  registers and unregisters one each time.
- **`OS_SetIrqCheckFlag` and every other `HW_DTCM` user truncate pointers** until `HW_DTCM` becomes a `uptr` over host
  memory; the spike uses its own check word. The same goes for `HW_THREADINFO_MAIN` and other shared-memory words.
- **minicoro's block layout** is relied on for the guard; the assert catches a change, and a minicoro update must
  rerun these tests. Its `mco_yield` overflow check compares the address of a local with the stack's bounds; it
  passes under ASan's fake stacks only because minicoro's implementation is built without ASan.
- **Windows** (both architectures) and Linux weren't built or run; the claims above come from the libraries' sources,
  documentation and cross-compilation to assembly.
