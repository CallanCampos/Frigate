# 0011: The cartridge's infrared chip as a register model

- **Status:** accepted
- **Date:** 2026-10-18
- **Plan:** §4.5 (Stubs), D3
- **Upstream:** `045e97b90` (merged by the port's `5ad9a1e1d`)
- **Note (2026-10-24):** `src/ov338/ir_check.c` is complete in both versions since upstream's `9af3ac8172` (on
  `main` since the merge of upstream's `993e6c49b`), so the ROM links its C; upstream has since named
  `func_0202d6a8` `GameBeaconSys_CheckGenuineCard`.

## Context

BW2's cartridges carry an infrared chip on the card slot's SPI bus, the bus the save's flash is on. The game asks it
for its ID as a check that the cartridge is genuine: `IrCheck_IsGenuineCard` (`src/ov338/ir_check.c`, an incomplete
file whose C is a hand decompilation, not a matched one) sends command 0x08 twice through AUXSPICNT and AUXSPIDATA
(`0x040001a0`, `0x040001a2`) and wants the chip's ID, 0xaa. It runs at every CONTINUE and NEW GAME
(`src/ov162/startmenu.c:371,379` call `func_0202d6a8`, `src/system/game_beacon.c:1234-1243`) and when a Pokémon faints
in a wild or trainer battle (`func_ov167_0219bdfc`, `src/ov167/btl_main.c:1562-1576`). A failure has effects a player
sees: after CONTINUE or NEW GAME the game installs `GameBeaconSys_HideNewHBlank`, which hides the C-Gear's new-beacon
marks and never removes itself, and in battle it skips the faint's processing that gives experience.

On the port, I/O registers were plain host memory (ADR 0004), so AUXSPIDATA read back the last byte written, 0, and
the check failed: a run under lldb from the oracle's first save showed `IrCheck_IsGenuineCard` return FALSE and
`GameBeaconSys_HideNewHBlank` run from the HBlank task list after CONTINUE. On melonDS, which the oracle runs, it
passes: melonDS gives the game codes `IRxx` a cartridge with the chip (`CartRetailIR` in `src/NDSCart.cpp`), the
oracle's HBlank task list (`gfl_use.c/sWork` + 0x18, `0x021ffc6c` in Black 2) held no task after CONTINUE from each of
three saves, and battles there gave experience. PLAN.md §4.5 planned a stub that returns TRUE.

The infrared library (`lib/irc`, overlay 31, complete) talks to the same chip for infrared links (the Feeling Check,
infrared battles and trades), which phase 7 is to carry between two copies of the port.

## Decision

- **The two registers are a model of the bus and the chip, as melonDS 1.1 has them**, in
  `port/hal/card/card_spi.c`: AUXSPICNT keeps bits 15, 14, 13, 6, 1 and 0 of a write and deselects the chip when bit
  13 is cleared while bits 13 and 6 are set (`NDSCartSlot::WriteSPICnt`); a byte goes out only while bits 15 and 13
  are set, and its position in the selection follows the hold bit, bit 6 (`WriteSPIData`); AUXSPIDATA reads 0 unless
  both are set, else the chip's last answer (`ReadSPIData`); position 0 latches the infrared command and answers 0,
  after it command 0x08 answers 0xaa and every other command 0 (`CartRetailIR::SPIWrite`, which has no infrared
  partner either). A transfer finishes when it is carried out, so the busy bit always reads clear.
- **The game's own C runs**, `ir_check.c` and `irc_spi.c` alike, with no stub of the check. The two registers are
  accessors in `port/tools/gen_host_tree.py`'s `HW_REGISTER_POLICY`, `port_hw_auxspicnt` and `port_hw_auxspidata`.
- **A store is seen at the next access.** An accessor returns the memory an access uses before the access happens, so
  it can't tell a load from a store. Each call resolves what the game did with the memory the previous call returned:
  AUXSPICNT's cell holding another value than the register's was written; AUXSPIDATA's cell holds the answer in its
  low byte and a marker (`0xa5`) in its upper byte, which a 16-bit store of a byte clears. Every store is preceded by
  its own accessor call, so at most one is pending at a call, and resolving it there is exact. Nothing observes the
  chip between two accesses.
- **The marker is a documented deviation**: a 16-bit read of AUXSPIDATA sees it in the upper byte, where the hardware
  gives 0. Every reader in the tree keeps only the low byte (`ir_check.c`'s `IrCheck_SendByte` returns a `u8` and
  `IrCheck_RecvByte` stores one; `irc_spi.c`'s `IrcSpi_SendByte` and `IrcSpi_RecvByte` do the same).
- **Not modelled:** command 0x00, which passes the rest of the selection through to the flash (no ARM9 code sends it:
  the game reaches the flash through the ARM7, `port/hal/card/backup.c`; it answers 0 and says so once on stderr), and
  EXMEMCNT's bit 11 (on the port the ARM9 always owns the slot).

## Alternatives

- **PLAN.md §4.5's stub, `IrCheck_IsGenuineCard` returning TRUE.** It replaces game C that compiles and runs, so the
  check's own code (an incomplete file, whose equivalence with the ROM is what the port can show) never runs, and it
  gives phase 7 nothing: the infrared library would still read plain memory. It also needs a replaced file or a game
  change, where the model needs neither.
- **AUXSPIDATA always reading 0xaa.** It passes the ID check, but the infrared library reads the length of a received
  packet from the same register (`IrcSpi_Recv`), so it would see a packet of 0xaa bytes that no partner sent.
- **Telling loads from stores by their order** (each send followed by a read, as `ir_check.c` happens to do): fragile,
  since any reader that reads twice or polls the data register would shift the bytes; and it still can't see a store
  of the value the register held, which `ir_check.c` makes (it sends 0 right after reading the answer 0).

## Consequences

- The check passes as on melonDS: no HBlank task after CONTINUE or NEW GAME, and battles give experience once they run
  on the port. ctest `rom_game_ir_genuine_card` runs the game's `IrCheck_IsGenuineCard` on the whole program's objects
  and wants TRUE; `hal_card_spi_plain` and `hal_card_spi_sanitized` hold the model to melonDS's functions.
- Normal runs change on purpose: before, every CONTINUE and NEW GAME left `GameBeaconSys_HideNewHBlank` in the HBlank
  task list. The replay and oracle tests still pass (they compare screens the hidden marks didn't reach).
- A future reader of AUXSPIDATA that compares all 16 bits would see the marker: it is written in `card_spi.c` and here.
- Phase 7's infrared link has a place to go: a partner behind commands 0x01 and 0x02 in `ChipAnswer`.
- Other registers whose reads must act (status and busy bits) may still be plain memory: an audit of
  `HW_REGISTER_POLICY` against the registers the game polls is open.
