# 0006: The sound driver as built

- **Status:** accepted
- **Date:** 2026-10-17
- **Plan:** D8, §4.7
- **Upstream:** `20d492180` (merge of PR #349)

## Context

PLAN.md D8 keeps the ARM9's sound libraries (NitroSDK's SND, NitroSystem's NNS_Snd and the game's own sound code) as
the decomp's C and replaces the ARM7's sound driver with host C. The ARM7 runs TwlSDK 5.3's stock SND driver, which
the decomp doesn't have and won't: it is the ARM7's binary. Phase 1 shipped a null driver behind the PXI sound tag
([ADR 0005](0005-platform-boundary-as-built.md)) that answered the ARM9's commands and played nothing, and the PXI
tests had settled on its timing. Sequences decide the game's music, jingles and sound effects, and the interactive
music changes tracks' volumes and mutes as the game plays, so a driver that differs tick by tick is audible and,
through the driver's shared random generator, changes every random note after the difference.

## Decision

- **The ARM7 is replaced at the command level, from its own code.** `port/hal/snd7` is the driver's C, written from
  the ARM7's machine code (`extract/<version>/arm7/arm7.bin`) and GBATEK's sound chapter, function by function with
  the ARM7 addresses in its comments (`snd7_internal.h` lists them): `SndThread`, `SND_CommandProc` and the alarms
  (`snd7.c`), the channels with their envelopes, sweeps, LFOs, locks and allocation by priority
  (`snd7_exchannel.c`), the players and tracks with every sequence command (`snd7_seq.c`), the decibel and pitch
  tables (`snd7_tables.c`), and the sound unit the DS has in hardware: 16 channels of PCM8, PCM16, IMA-ADPCM, PSG
  and noise, and the mixer (`snd7_hw.c`). The ARM9 sends the same packets of `SNDCommand`s through the same PXI tag,
  and reads the same shared work (`SNDSharedWork`) and driver state, as on the DS.
- **The sound unit is melonDS's model of the DS,** which the oracle comparison holds it to
  ([port/tools/oracle/README.md](../../tools/oracle/README.md#sound)): a sound's first sample comes on the channel's
  third timer step (an IMA-ADPCM sound's on its eleventh, a square wave's or noise's on its first); a channel reads
  its sound ahead through a FIFO of 8 words, refilled 4 at a time; the channel moves on before the mixer takes its
  sample; the mixer keeps GBATEK's widths (the fraction's low 10 bits dropped after the pan, the master volume over
  128, 8 bits dropped), with a volume, pan or master volume of 127 counting as 128; and the channels stand still
  while SOUNDCNT's enable is clear.
- **Time is the virtual clock's.** The driver's periodic run is the DS's OS alarm of 2728 ticks of 64 cycles, the
  first 0x10000 ticks after the driver starts; a wake-up word from the ARM9 runs a run that isn't periodic. The
  driver catches up to the virtual clock at each VBlank, whenever the ARM9 sends a word, and at its next alarm
  (`SND_SetupAlarm`'s, which the stream player uses), which is the OS core's timed work (`port_os_set_timed_work`):
  the alarm's word reaches the ARM9 when it goes off, as on the DS, so that its interrupt is taken then. In each
  catch-up the periodic runs, alarms and mixer samples that fall before it run in order. The same run of the game
  gives the same driver state and the same samples, whenever the host looks.
- **The output is the DS mixer's rate, 32,728.515625 Hz:** one stereo sample each 1024 cycles (512 ticks of the
  sound clock, half the 33.514 MHz bus clock), signed 16 bits. Samples are made on the game's thread as game time
  passes them and handed to `port/audio` at each VBlank, which queues them on an SDL3 audio stream at 32,728 Hz; SDL
  resamples to the device. The stream's speed follows how much is queued, within half a percent, and a queue past a
  second is emptied, which keeps the device's clock and the game's pace together without touching the samples. A WAV
  dump (`--audio-dump`) gets the samples as they are made.
- **Pointers in 32-bit fields become handles or notes.** The commands carry ARM9 pointers in 32-bit arguments: the
  ARM9's wrappers (`snd_interface_host.c`) note each pointer and its size under the sound's PXI tag, and the driver
  finds the host pointer from the low 32 bits (`port_pxi_noted_pointer`), bounded by the noted size. Sound files that
  keep addresses in their own 32-bit words, read in place by both sides, keep them as handles: `0x80000000` and an
  index into a table of host pointers, never 0, never below `0x02000000` where the files keep offsets, the same for
  the same pointer (`snd_bank_host.c`, `snd_handle.h`). That covers banks' links to wave archives, wave archives'
  links and their table of waves loaded one by one, and NitroSystem's file table; the decomp's structs for them
  declare those words `u32` on hosts ([game-changes.md](../game-changes.md)), so every file keeps the DS's layout.
- **Deliberate deviations,** each documented where it is made:
  - a packet's commands run as soon as the ARM9 sends it, before the run's register writes, where the DS runs them at
    its next run; the shared work's count of finished packets moves on before the send returns, as the ARM9's busy
    waits need. So that the driver's runs, and the random draws each makes, stay the DS's, the host's
    `SND_WaitForCommandProc` (`snd_command_host.c`) sends the wake-up word whenever the DS's driver would still hold
    the packet;
  - the driver reads a sequence, a bank and a wave only inside the memory they are in: a track that reads past its
    sequence ends there, an instrument outside its bank isn't found, a channel reads 0 past its wave's memory. The DS
    reads whatever is there, and through a 16-byte cache for sequences, which gives the same bytes unless the ARM9
    changes a sequence while it plays;
  - a track that reads 65,536 commands in one tick without waiting ends, where the DS's sound thread would hang;
  - the mixer keeps 16 bits, melonDS's output: the DS's 10 bits times 64, so that a channel at full volume and center
    pan is about half the range, clipped at the same level, where the DS's output is 10-bit PWM. The game's music
    reaches full scale on both (the opening's, up to some 420 samples a second at 16 bits in its loudest seconds);
  - the driver's runs take no time, where the DS's ARM7 reaches its register writes some way into each run: a note's
    start lands in the same mixer sample on both or up to 4 samples (120 µs) later on the DS, by the note starts the
    oracle comparison pairs, so the sequences' sound matches the DS's in its envelope but not to the sample, while a
    stream's and a cry's, which the ARM9 writes and the channel plays at its own timer, match to the bit;
  - `SND_ReadDriverInfo` fills the fields that the ARM9 reads, in the decomp's guessed layout of the driver's state,
    which differs from the ARM7's ([decomp-findings.md](../decomp-findings.md)).
- **Not modelled:** the capture units capture nothing (their registers and status are kept; Black 2 and White 2
  never start them, so no reverb); `SND_AllocCommand`'s wake-up when all 256 commands are sent; a wake-up for a tag
  the ARM9 took back before the DS's driver would have run its packet; the 10-bit output's quantization.

## Alternatives

- **Replace the sound libraries at NNS_Snd's API with a host sequencer:** loses NitroSystem's and the game's own C,
  which the decomp has as matching code, and the tick-for-tick behaviour that the interactive music and the shared
  random generator depend on.
- **A sequence player from another project (an emulator, an SSEQ converter, libntr's patterns):** references only
  under the port's rules, and none is the ARM7's own logic down to its rounding, allocation order and random draws.
- **Run the ARM7's code in a CPU emulator:** not native (PLAN.md §1).
- **Mix on the audio device's thread at the device's rate:** ties the samples to host timing, so runs stop being
  reproducible and replays and dumps differ.
- **Copy the sound files into host structs at load, or keep sound data below 4 GiB:** a copy desynchronizes from the
  files the ARM9 and the ARM7 both update in place, and a 32-bit address space for sound data can't be had portably
  (macOS on arm64 has no `MAP_32BIT`).

## Consequences

- Sequences play as the ARM7 plays them; `port/tests/snd` checks the decoder, channels, sequencer, controls, driver
  state, handles, PXI paths and the dump, and plays every sequence of the player's ROM when there is one.
- The handle table only grows; it holds one entry per distinct address that sound data was loaded at, which the
  sound heap bounds.
- A change to how the ARM9's sound libraries carry pointers needs the matching note or handle, and a game change
  for any further file struct that keeps an address in 32 bits.
- The output's scaling was revisited against the oracle (PLAN.md §6, T4): the oracle's sound traces and WAV dumps
  (`oracle.py sound`, ctest `oracle_sound_<name>` and `oracle_sound_<name>_hashes`) hold the driver's state run by run
  and the sound segment by segment to melonDS's. Revisit the capture units if a game path ever starts them, and a
  channel's hold, which the port keeps (GBATEK) and melonDS doesn't.
