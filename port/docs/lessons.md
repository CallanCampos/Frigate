# Lessons

What port work taught that isn't obvious from the code, newest first. A lesson about MWCC or the decomp's own
workflow goes in the decomp checkout instead; this file is the port's ([CLAUDE.md](../CLAUDE.md), the iteration loop).

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
