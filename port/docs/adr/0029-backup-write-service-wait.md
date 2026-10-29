<!-- The ARM7 backup service wait serves GameData saves through genuine NitroSDK CARD request/task/PXI code. -->
# 0029: Preserve the backup WRITE service's initial SDK wait

- **Status:** accepted for the initial nonempty WRITE wait and native blocking integration; broader timing remains unproved
- **Date:** 2026-10-28
- **Plan:** §4.5 (SDK boundary), §6 (verification), phase 5

## Evidence and boundary

A passive original Black 2 SAVE observation preserved all 37,000 frame records, 186,086 semantic payloads and the
resulting save. Across 136 WRITE packets, the scheduled final-byte deadline to next STATUS packet start spans
841,294–870,833 system cycles. Status WIP is zero in pinned melonDS 1.1; its ordinary flash model has no timed WIP.
That gap alone does not identify physical program time or CPU work.

A separately control-preserving first-write observer establishes the actual policy. ARM7 PCs 03806fdc/03806fe0 load
25/300 from shared `CARDiCommandArg.spec.writePage`/`writePageTotal` (command offsets 0x30/0x34), then call the
status-wait routine at 03806990. It calls the alarm-backed delay at 037fd0e0 with 25; the alarm receives 13,091 ticks.
These match the shared SDK's `OS_MilliSecondsToTicks` and `OS_Sleep` algorithm (`nitro/os.h`, `os_thread.c`). The
observed delay call spans 839,361 system cycles, including scheduling/interrupts; its alarm duration is 837,824.
A separate White 2 first-WRITE control preserves all 37,000 frames, 186,086 payloads and the save/all 73 checks.
Its actual command pointer is 0214b420; the same instruction sites load 25/300 and schedule 13,091 alarm ticks.
The delay span is 839,355 system cycles, including interrupts/scheduling. Both raw ARM7 firmware regions are
byte-identical (167,812 bytes); that static equality alone was not used as dynamic proof.
These first-gap traces do not establish every write's branch, physical hardware timing, or general CPU costs.

## Decision

After a successful nonempty `CARD_REQ_WRITE_BACKUP`, native `backup.c` has already validated and written the bytes. Retain
only command ownership and an absolute deadline; publish success and acknowledge through PXI after
`OS_MilliSecondsToTicks(spec.writePage) * 64` cycles. For the game's 25 ms field this is exactly 837,824 cycles.
The real SDK retains page splitting, retries, REQ sleeping, BUSY release, between-page cancellation and callbacks.
Host ARM7/timed callbacks never sleep or call the SDK. Sound and backup use distinct callback identities in the
existing multi-slot timed-work service. No READ/VERIFY/PROGRAM/ERASE latency, status-poll retry cost, serial wire
cost or fitted residual is introduced. This service wait is separate from the unfinished ROM CPU/DMA reader. Timed callbacks retain the existing OS
host-work dispatch rules (generic work can be deferred during an IRQ handler); this change does not introduce
independent SPI hardware progress or claim exact interrupt-phase fidelity.

Reset, backup boot and save-path replacement retire a pending reply before storage reuse. A queued old callback
has no captured command pointer: it checks the current owner/deadline, so it cannot complete a new request early.
Deadline overflow and the UINT64_MAX cancellation sentinel fail with an explicit result rather than hang. Already
written image bytes are not rolled back by reply reset or bus reset. Save-path replacement separately retains its
existing behavior: it frees the old image and clears dirty state, so unflushed old-path bytes are not promised to
persist. Whole-run stop does not promise to resume the stopped SDK task or call its
callback; new boot retires the reply. Disk flush failures retain the existing reported/retried host-file policy,
separate from the cartridge result. Unsupported/invalid chip requests retain immediate errors.

## Verification and remaining limits

`card_backup_deferred_reply` uses an explicit fake clock for callback ownership, not scheduler fidelity. Other
legacy `card_backup_*` functional scenarios set writePage to zero and retain byte/file/error assertions.
`card_backup_service_*` runs genuine CARD common/task/stream/request, PXI and OS: three-page write+verify, exact
service deadlines, real sound alarms and independent timed work, synchronous requests, cancellation, stop/reset,
invalid requests and host file replacement failure. Its only ROM-free seam is a test-owned `CARDi_InitRom`; a
separately compiled actual reader with that one entry renamed retains every other reader definition. The original
reader object alone is filtered from the fixture. No production test bypass exists.

Both original ROM hashes match after the game-wrapper change (fresh configure, 18 Ninja steps, zero warnings).
Fresh full native suites pass Black 2 370/28 explicit skips of 398 and White 2 367/28 of 395, zero failures;
actual assembly oracles run all five cases each. Ordinary/fatal configurations pass 112 backup/actual-game test
executions across both games; the registered house/starter/SAVE comparisons pass. Exact published-head verification
and hosted checks remain required before merge. No camera, boot, rendering, audio, AI or whole-game parity claim
follows. ADR 0023 remains proposed; no input, seed, golden, mask, performance budget or sanitizer policy changes.

## Blocking game wrapper and original formatting checkpoints

The actual `writeFlashWaitSuccess` loop only polls a callback-owned flag. On the native cooperative host that
cannot advance a pending service deadline. A `PLATFORM_PC` call to genuine `CARD_WaitBackupAsync` inside the loop
lets the real task sleep; its return is ignored and `finishBackupGetResult` retains errors and unlock ownership.
Actual-source success/error fixtures cover this boundary; no SDK replacement or CPU-time charge is introduced.

Unmodified pinned original DS runs with each committed `first_save_<version>.pbwinput`, its genuine seed,
default running clock and blank backup establish that video 5600 is unfinished: first-bank identity/egg bytes
are present, second-bank expected ranges are FF, and full 73-block/table/footer validation fails. At video 7800
both versions contain all four expected values and pass all 73 checks in both banks. Each 5600 run's 5,600 frame
rows and 23,486 payload rows exactly match its 7800 prefix; immutable inputs/ROM/tool hashes remain unchanged.
`replay_same_save` therefore retains the actual early-state assertions, while a separate two-run
`replay_same_save_complete` enforces the complete image at 7800. Recording's required-valid-save case explicitly
uses 7800. Inputs, seeds, goldens, masks, watchdogs and performance budgets are unchanged. This is a corrected
checkpoint contract, not a claim of whole-run native/DS parity.
