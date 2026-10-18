# Lessons

What port work taught that isn't obvious from the code, newest first. A lesson about MWCC or the decomp's own
workflow goes in the decomp checkout instead; this file is the port's ([CLAUDE.md](../CLAUDE.md), the iteration loop).

## 2026-10-18: the test launcher, after review

- **ctest's `PASS_REGULAR_EXPRESSION` ignores the exit status.** A test with one passes on its output alone, so a
  test of a status runs the program through a checker that compares it (`port/tests/app/check_exit.py`, or
  `check_launch_smoke.py --status`).
- **A tool that stops short must say so in its status.** The launcher ended a run that never opened an app, or never
  opened the last one, with status 0; only the report lines that the oracle checks compared kept the tests honest.
  Status 0 now means every app opened and closed, and a run that ended first says what never did.
- **A stand-in does each thing the original does each time, not once.** The field menu's choice by a key sets the
  touch flag before every app; the launcher set it before the first only, and an app closed by a touch left it set
  for the next one, whose screen then differed. Read where a value is set in the code it stands in for, per call.

## 2026-10-18: the field menu's apps against the oracle

- **Align an app on its first frame, measured on both programs, not on its press.** The DS takes a different number
  of passes than the port to close the field and load an app; opening the app on the port at the oracle's first app
  frame's count less the port's own passes to its first frame lines up every later pass, and then a press inside the
  app reaches both on the same pass. The bag, the card and the party list then match pass for pass.
- **Check every pass of an app, not only its labelled screens.** The labelled rows of the trainer card matched while
  its opening wipe was missing; the span check (every picture the port shows at a count is one the oracle shows there)
  found it.
- **A screen that steps by VBlanks parts from the oracle wherever the DS takes long passes.** The summary scrolls its
  background by `OS_GetVBlankCount()`, and the DS loads it in a pass of 30 frames: its background stays some pixels
  ahead. Look for the VBlank count in the code before suspecting the renderer.
- **A route made for one version doesn't walk the other.** The starter route's input, written on Black 2's oracle,
  goes astray on White 2's: each version needs its own measured route.

## 2026-10-18: the H-blank interrupt and its flag

- **An interrupt handler that takes no time sees the hardware as it was when the interrupt came.** The port raised
  the H-blank interrupt 70 cycles before DISPSTAT's H-blank flag, where melonDS sets both at once. The game's line
  wipe checks `GX_IsHBlank()` before each window write, so on the port it never wrote one, quietly: the screens went
  from black to the picture. A flag and the interrupt it describes belong at the same moment on a host, whose handlers
  run in no time. The moment is GBATEK's 1606 cycles into the line; melonDS 1.1 uses 1584 for both, which the game
  can't tell apart.
- **A missing transition shows only frame by frame.** The trainer card's screens were right before and after its
  opening; only comparing every pass of the opening with the oracle's showed the wipe missing.

## 2026-10-18: the card's infrared chip

- **A register accessor can't tell a load from a store.** `port_hw_*` returns the memory the access uses before the
  access happens, so a register whose writes act has to be resolved at the next access: compare the cell with what
  it held, and when a store may write the value the cell already holds, give the cell a marker that every store
  clears. `ir_check.c` sends 0 right after reading the answer 0; comparing values alone lost that byte and the check
  failed. Each store is preceded by its own accessor call, so at most one is pending and resolving it at the next call
  is exact (`port/hal/card/card_spi.c`).
- **Plain memory for a register the game polls fails quietly.** AUXSPIDATA read back the last byte written, the
  genuine-card check failed, and nothing stopped: the game only installed an HBlank task for good and would have
  skipped battle experience. A breakpoint on the game's check against the oracle's outcome found it, not a test.

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
