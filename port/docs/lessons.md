# Lessons

What port work taught that isn't obvious from the code, newest first. A lesson about MWCC or the decomp's own
workflow goes in the decomp checkout instead; this file is the port's ([CLAUDE.md](../CLAUDE.md), the iteration loop).

## 2026-10-18: the sound against the oracle, after review

- **A counter the hardware reloads keeps counting through a write to its reload.** The channel's timer was modelled as
  a count from the current step compared with the current period, which is the same until the driver changes the
  period mid-note (an LFO, a sweep, a bend: most of the opening's notes); then the note's phase moved. Model the
  counter as the hardware has it (count up from the reload, reload at the overflow), and test a write mid-count.
- **A comparison that only looks at what pairs can't fail on what's missing.** The runs were paired by their count
  and every pair was equal, but a play that ended early or late on the port would have passed: count what is on one
  side only, inside a window both sides cover, and fail on it.
- **melonDS's frame isn't the DS's.** `RunFrame` runs lines 0 to 262 and the VBlank comes at 192, so a state stamped
  with melonDS's frame number is up to 71 lines past the VBlank the port's frame ends with. Stamp by the VBlank, and
  say how many compared frames hold what the check is meant to see (here none had a player active).

## 2026-10-17: saves with melonDS, after review

- **A function's address used as a number is host state.** DWC adds `(u32)OS_GetTick`, the function's address, to the
  seed of the console's Wi-Fi ID (a `BUG` the decomp marks); on the DS it is a constant, on a host whatever ASLR made
  it at launch, so every run wrote another save. Two runs often agree by chance, since the slide reaches only the
  seed's high bits: hold such a value to the oracle's, not only to a second run (`replay_same_save` does both). A
  Thumb function's address has bit 0 set (`0x0207bb0d`), as the ROM's literal pool shows.
- **Count a difference twice before calling it a figure.** "10,718 bytes differ" came from one run of a
  nondeterministic port; a second run would have shown it move. Diff two runs of the port before diffing the port
  against the oracle, then sort what is left by cause.
- **Allow a stop by its trap's name, not by its exit status.** A sequence that let any trap stub pass from a frame on
  would have kept passing if the port began stopping on another gap; `stop_trap` names the one it stops on.
- **A rename over a symbolic link replaces the link.** The atomic write (temporary file, then rename) has to resolve
  the save path first, or a player's link to melonDS's `.sav` silently becomes a file of its own.
- **Every test that runs the program needs `--save-dir`**, even one that runs a frame: without it the program reads the
  player's own save directory, and a file there of the wrong size fails the test.
- **The cursor's pulse is a cycle, so a step is shown at many counts.** Saying the DS "doesn't show" a picture needs
  the whole run's hashes: White 2's press frame shows a pulse step the DS shows at four other counts of the same run.

## 2026-10-17: saves with melonDS

- **A save moves the oracle's frame numbers, not its counts.** The DS's start-up reads the save before the main loop
  starts: with the first save it starts at oracle frame 219, with a blank backup at 242, with a formatted one at 246.
  Known differences declared by oracle frame (`known_oracle`) move with it, 23 frames sooner for the title's torn
  frames from the first save; those declared by port frame or count don't.
- **Make a save on the oracle, keep only its SHA-1.** melonDS is deterministic, so one input file gives the same save
  memory at every run (`oracle.py save`): a test makes it at run time and checks it against the hash before using
  it, and nothing the game wrote is committed. Driving the field menu, the first DOWN only shows the cursor, and a
  press before the save prompt has opened is lost, so give each step room and look at the screens.
- **A press brings a pass's lag back to 0.** White 2's start menu runs a pass behind on the DS after the title's cry;
  CONTINUE's press reaches both on the same pass, and from the next frame the counts pair with lag 0 again, but the
  press's own frame shows the cursor a blink step apart on each.

## 2026-10-17: the sound against the oracle

- **A host whose CPU takes no time changes who wins a race the DS's CPU time decides.** The ARM7's alarms ran lazily
  (at each VBlank and each word of the ARM9), which is the same state whenever anyone looks, but not the same
  interrupts: two or three alarms a frame reached the ARM9 together, and NitroSystem's stream player, which drops a
  queued block when a third arrives before its thread has read one, played a block of silence every 7 frames or so
  and fell behind the DS's music. Anything the hardware raises as an interrupt on its own clock needs the OS core's
  timed work (`port_os_set_timed_work`), not a catch-up at the next VBlank. The same instant ARM9 then rewrote a
  stream's buffer two samples before the channel played them, which the DS's FIFO, read ahead, never sees.
- **Compare a sequencer by its own clock.** The driver's periodic runs fall at different moments of a frame on the two
  programs, so its state at a VBlank can't match; paired by the count of periodic runs since each play started, every
  field of every run matched, and the one real difference (a channel ending a run early) pointed straight at the
  sound unit's start delay.
- **Sample-exact audio needs the reference's own hardware model**, down to its quirks: melonDS starts a PCM sound on
  the third timer step, reads through an 8-word FIFO and counts a volume of 127 as 128. With those, a stream and a cry
  matched melonDS to the bit, which makes their hashes a test. The sequences don't, and the reason was measured, not
  assumed: the DS's register writes come 1 to 5 mixer samples after its alarm, the port's in the first, so they are
  held by their envelopes.

## 2026-10-17: New Game's intro, after review

- **The decomp's incomplete C can differ from the ROM where only aliasing shows it.** `MTX_Concat44` and its siblings
  read their first input after storing part of the result; the ROM loads each row first, so the game's products into
  their own input are right on the DS and wrong on a host, and every test with separate pointers passed. When a
  library function writes through one pointer and reads through another of the same type, give the asm oracle cases
  with the pointers equal (`port/tests/asm_oracle/test_fx_decomp.py`), and grep the game for in-place calls.
- **A replay survives without proving anything** until it checks what the screens show. `--run-dir` writes
  every frame's screens in the oracle's `frames.tsv` format, so the goldens are the oracle's own hashes and a test
  needs no images (`port/tests/replay/check_screens.py`).
- **Press in video frames through the boot, and give presses room.** The DS drops frames in the logo movie and at
  every load, so its game count reaches the title screen about 2,500 passes earlier than the port's: a press keyed
  to the game count lands on another moment of the title on each (the DS's title had restarted its animation). In
  video frames both show the title at 3,900, and both take 155 passes from START to the start menu. After that,
  presses that keep coming (A twice a second) find each program waiting, so an input file plays the same on both
  however far the DS falls behind.

## 2026-10-17: New Game's intro

- **ld64 doesn't keep a section's zero-fill objects in the order they are defined.** The trap stubs bracketed the
  data placeholders with two page-sized paddings, defined before and after them in one file; ld64 put both paddings
  after every placeholder, so no placeholder was protected and the game read zeros where it should have trapped. Work
  out a range from where each object landed (`port/hal/trap.c`), or take the section's own bounds, never from symbols
  defined around it.
- **The compile probe preprocesses as MWCC does**, without `PLATFORM_PC`, where `uptr` is `u32`: a `(u32)` cast made
  `(uptr)` leaves its counts as they were. Its `-Wpointer-to-int-cast` and `-Wint-to-pointer-cast` lists per file
  (`port/tools/compile-probe`) are still the quickest map of the pointers a path cuts.
- **A `u32` that carries a pointer hides behind an API more often than behind a cast in the crashing function**:
  setters with a value that is sometimes a pointer (`PokeParty_SetParam`), callback parameters (NitroSystem's
  animation functor, MCSS), handles returned as `u32` (`func_02005af4`) and VM stacks that hold return addresses.
  Retype the API and every sibling, then the `(u32)` casts at its callers; the callbacks' own parameters follow,
  since a function pointer of the old type no longer converts.
- **A fault far from a cut pointer**: a buffer sized for DS structs overflows quietly into the next heap block, and
  the crash comes later from whatever lived there (the particle system's camera, read back as NULL). Before
  chasing the crash, check the sizes of the buffers allocated just before the block that broke.
