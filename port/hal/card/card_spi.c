// The card slot's SPI bus and the infrared chip on it: the genuine-card check (overlay 338) and the IR library
// (overlay 31). ADR 0011 says why it is a model of the registers rather than a stub of the check.
//
// BW2's cartridges carry an infrared chip on the SPI bus that also reaches the save's flash. The ARM9 drives the bus
// through two registers (GBATEK, "DS Cartridge I/O Ports" and "DS Cart Infrared"):
// - AUXSPICNT (0x040001a0): bit 15 enables the slot, bit 13 selects its SPI bus, bit 6 keeps the chip selected after
//   the byte being sent ("hold"), bit 7 is busy while a byte goes out, bits 0-1 are the speed;
// - AUXSPIDATA (0x040001a2): a write sends a byte, and a read then gives the byte the chip sent back meanwhile.
// The first byte of each selection is a command for the infrared chip: 0x08 asks its ID, which a genuine chip answers
// with 0xaa on the next byte; 0x01 and 0x02 receive and send infrared data; 0x00 passes the rest of the selection
// through to the flash. src/ov338/ir_check.c (IrCheck_IsGenuineCard, run at every CONTINUE and NEW GAME and when a
// Pokémon faints in a wild or trainer battle) sends 0x08 twice and wants 0xaa; on failure the game hides the C-Gear's
// new-beacon marks for good (src/system/game_beacon.c) and battles give no experience (src/ov167/btl_main.c).
//
// This file does what melonDS 1.1 does, which the oracle runs (port/tools/oracle):
// - NDSCartSlot::WriteSPICnt: a write keeps bits 15, 14, 13, 6, 1 and 0; one that clears bit 13 while bits 13 and 6
//   are set deselects the chip;
// - NDSCartSlot::WriteSPIData: a byte goes out only while bits 15 and 13 are set; its position in the selection
//   follows the hold bit (the first byte after a selection starts is position 0, and a byte sent with hold clear ends
//   the selection);
// - NDSCartSlot::ReadSPIData: 0 unless bits 15 and 13 are set, else the chip's last answer;
// - CartRetailIR::SPIWrite: position 0 latches the command and answers 0; after it, command 0x08 answers 0xaa and
//   every other command 0 (melonDS has no infrared partner either, so receive gives a length of 0);
// - NDS::ARM9IORead16 and ARM9IOWrite16: the data register reads as 8 bits with the upper byte 0, and a write sends its
//   low byte.
// The transfer finishes as soon as it is carried out, so the busy bit always reads clear: the game's waits on it end at
// once, and its waits on OS_GetTick end on the port's virtual clock. EXMEMCNT's bit 11 (which CPU owns the slot) is
// not modelled: on the port the ARM9 always does. Pass-through (command 0x00) isn't modelled: no ARM9 code sends it,
// since the game reaches the flash through the ARM7 (port/hal/card/backup.c), so it answers 0 and says so once.
//
// Reads and writes. The game reaches each register through an accessor that returns the memory the access uses
// (port/tools/gen_host_tree.py's HW_REGISTER_POLICY), so the accessor runs before the access and can't see whether it
// was a load or a store. Each call therefore first resolves what the game did with the memory the previous call
// returned, then fills in the memory for this access:
// - AUXSPICNT's cell holds the register's value; if it holds another value at the next call, the game wrote it;
// - AUXSPIDATA's cell holds the chip's answer in its low byte and a marker in its upper byte; a 16-bit store of a byte
//   clears the marker, so a cell without it at the next call was written, even with the value it held (ir_check.c
//   sends 0 right after reading the answer 0, which a comparison of values would miss), and its low byte goes out.
// Each store is preceded by its own accessor call, so at most one store is pending at a call and resolving it there is
// exact. The marker is the one deviation: a 16-bit read sees it in the upper byte, where the hardware gives 0. Every
// reader in the tree keeps only the low byte (ir_check.c's IrCheck_SendByte returns a u8 and IrCheck_RecvByte stores
// one; irc_spi.c's IrcSpi_SendByte and IrcSpi_RecvByte do the same); a reader that compared all 16 bits would need the
// marker gone.
#include "hal/card/card_spi.h"
#include <stdint.h>
#include <stdio.h>

// AUXSPICNT's bits
#define SPICNT_SPEED 0x0003
#define SPICNT_HOLD 0x0040
#define SPICNT_BUSY 0x0080
#define SPICNT_SPI_BUS 0x2000
#define SPICNT_IRQ 0x4000
#define SPICNT_ENABLE 0x8000
// What a write keeps: melonDS's val & 0xE043
#define SPICNT_WRITABLE (SPICNT_ENABLE | SPICNT_IRQ | SPICNT_SPI_BUS | SPICNT_HOLD | SPICNT_SPEED)

// The infrared chip's commands that answer something other than 0
#define IR_COMMAND_PASS_THROUGH 0x00
#define IR_COMMAND_ID 0x08
#define IR_ID 0xaa

// The upper byte of AUXSPIDATA's cell between two accesses
#define DATA_MARKER 0xa5

typedef struct CardSpi {
    // AUXSPICNT as the hardware holds it, and the cell the game's last access to it used
    uint16_t control;
    uint16_t controlCell;
    // The cell the game's last access to AUXSPIDATA used: the marker and the chip's answer, until the game stores
    uint16_t dataCell;
    // The chip's answer to the last byte sent
    uint8_t answer;
    // The command the selection started with, and the position of the last byte sent in it
    uint8_t command;
    uint32_t position;
    // Whether the chip is still selected after the last byte (melonDS's SPIHold)
    int selected;
} CardSpi;

#define POWER_ON { .dataCell = DATA_MARKER << 8 }

static CardSpi sSpi = POWER_ON;
static int sPassThroughNoted;

void port_card_spi_reset(void) {
    sSpi = (CardSpi)POWER_ON;
}

static int BusEnabled(void) {
    return (sSpi.control & (SPICNT_ENABLE | SPICNT_SPI_BUS)) == (SPICNT_ENABLE | SPICNT_SPI_BUS);
}

static void WriteControl(uint16_t value) {
    // Leaving the SPI bus while the chip is held deselects it
    if ((sSpi.control & (SPICNT_SPI_BUS | SPICNT_HOLD)) == (SPICNT_SPI_BUS | SPICNT_HOLD) &&
        (value & SPICNT_SPI_BUS) == 0) {
        sSpi.selected = 0;
    }
    // The busy bit is the hardware's, and it is always clear here
    sSpi.control = (uint16_t)((sSpi.control & SPICNT_BUSY) | (value & SPICNT_WRITABLE));
}

// The infrared chip's answer to a byte at a position of the selection
static uint8_t ChipAnswer(uint8_t value, uint32_t position) {
    if (position == 0) {
        sSpi.command = value;
        return 0;
    }
    switch (sSpi.command) {
    case IR_COMMAND_PASS_THROUGH:
        if (!sPassThroughNoted) {
            sPassThroughNoted = 1;
            fprintf(stderr, "port: the game sent the card's infrared chip a byte to pass through to the flash, which "
                            "the port doesn't model; it answers 0 (port/hal/card/card_spi.c)\n");
        }
        return 0;
    case IR_COMMAND_ID:
        return IR_ID;
    }
    return 0;
}

static void WriteData(uint8_t value) {
    if (!BusEnabled()) {
        return;
    }
    if ((sSpi.control & SPICNT_HOLD) == 0) {
        // The selection's last byte
        sSpi.position = sSpi.selected ? sSpi.position + 1 : 0;
        sSpi.selected = 0;
    } else if (!sSpi.selected) {
        sSpi.selected = 1;
        sSpi.position = 0;
    } else {
        sSpi.position++;
    }
    sSpi.answer = ChipAnswer(value, sSpi.position);
}

// Carries out the store the game made through the cell the last call returned, if it made one
static void Resolve(void) {
    if (sSpi.controlCell != sSpi.control) {
        WriteControl(sSpi.controlCell);
    }
    if ((sSpi.dataCell >> 8) != DATA_MARKER) {
        WriteData((uint8_t)sSpi.dataCell);
    }
    sSpi.controlCell = sSpi.control;
    sSpi.dataCell = (uint16_t)(DATA_MARKER << 8 | (BusEnabled() ? sSpi.answer : 0));
}

void *port_hw_auxspicnt(void) {
    Resolve();
    return &sSpi.controlCell;
}

void *port_hw_auxspidata(void) {
    Resolve();
    return &sSpi.dataCell;
}
