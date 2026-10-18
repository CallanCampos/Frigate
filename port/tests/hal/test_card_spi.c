// Tests of the card slot's SPI bus and its infrared chip (port/hal/card/card_spi.c), which the game's genuine-card
// check (src/ov338/ir_check.c) and the infrared library (lib/irc/src/irc_spi.c) talk to. The registers are driven as
// the host variants of the SDK's headers drive them: each access calls the register's accessor and loads or stores
// 16 bits through the pointer it returns.
//
// The expected values are melonDS 1.1's, which the oracle runs (src/NDSCart.cpp: NDSCartSlot::WriteSPICnt,
// WriteSPIData and ReadSPIData, and CartRetailIR::SPIWrite; src/NDS.cpp: ARM9IORead16 and ARM9IOWrite16 at 0x040001a0
// and 0x040001a2), and GBATEK's ("DS Cartridge I/O Ports", "DS Cart Infrared"): command 0x08 answers the chip's ID,
// 0xaa, on the byte after it; every other command answers 0 on the port, as on melonDS, which has no infrared partner.
#include "hal/card/card_spi.h"
#include <stdint.h>
#include <stdio.h>

// What reg_MI_MCCNT0 and reg_MI_MCD0 expand to in the host build
#define AUXSPICNT (*(volatile uint16_t *)port_hw_auxspicnt())
#define AUXSPIDATA (*(volatile uint16_t *)port_hw_auxspidata())

// AUXSPICNT's bits, as lib/nitro/include/nitro/hw.h names them
#define ENABLE 0x8000
#define SPI_BUS 0x2000
#define BUSY 0x0080
#define HOLD 0x0040
// The speed ir_check.c and irc_spi.c use (bits 0-1)
#define SPEED 0x0002

static int sFailed;

static void Check(int ok, const char *what) {
    if (!ok) {
        printf("FAIL: %s\n", what);
        sFailed = 1;
    }
}

// One byte out and the chip's answer back, as ir_check.c's IrCheck_SendByte does: the store, the wait on the busy
// bit, then the load, of which the game keeps the low byte
static uint8_t Transfer(uint8_t value) {
    AUXSPIDATA = value;
    while (AUXSPICNT & BUSY) {
    }
    return (uint8_t)AUXSPIDATA;
}

// IrCheck_ReadId's two bytes: the ID command with the chip held, then a byte that ends the selection
static void ReadId(uint8_t *first, uint8_t *second) {
    AUXSPICNT = ENABLE | SPI_BUS | HOLD | SPEED;
    *first = Transfer(0x08);
    AUXSPICNT = ENABLE | SPI_BUS | SPEED;
    *second = Transfer(0);
}

static void TestGenuineCheck(void) {
    uint8_t first;
    uint8_t second;

    port_card_spi_reset();
    for (int i = 0; i < 2; i++) {
        ReadId(&first, &second);
        Check(first == 0x00, "the ID command's own byte is answered with 0 (CartRetailIR::SPIWrite at position 0)");
        Check(second == 0xaa, "the byte after the ID command is answered with the chip's ID, 0xaa");
    }
}

// irc_spi.c's IrcSpi_Recv: command 0x01 with the chip held, the length byte, then the last byte
static void TestReceive(void) {
    port_card_spi_reset();
    AUXSPICNT = ENABLE | SPI_BUS | HOLD | SPEED;
    Check(Transfer(0x01) == 0, "the receive command's own byte is answered with 0");
    Check(Transfer(0) == 0, "receive gives a length of 0: there is no infrared partner");
    AUXSPICNT = ENABLE | SPI_BUS | SPEED;
    Check(Transfer(0) == 0, "the selection's last byte is answered with 0");

    // Send (0x02): every byte of the packet is answered with 0
    AUXSPICNT = ENABLE | SPI_BUS | HOLD | SPEED;
    Check(Transfer(0x02) == 0, "the send command's own byte is answered with 0");
    Check(Transfer(0x5a) == 0 && Transfer(0xa5) == 0, "the bytes sent are answered with 0");
    AUXSPICNT = ENABLE | SPI_BUS | SPEED;
    Check(Transfer(0x11) == 0, "the packet's last byte is answered with 0");

    // A command the chip doesn't know
    AUXSPICNT = ENABLE | SPI_BUS | HOLD | SPEED;
    Transfer(0x05);
    AUXSPICNT = ENABLE | SPI_BUS | SPEED;
    Check(Transfer(0) == 0, "an unknown command is answered with 0");
}

static void TestDisabled(void) {
    uint8_t first;
    uint8_t second;

    port_card_spi_reset();
    // A whole ID exchange with the slot disabled, then with the ROM's bus selected: nothing goes out
    AUXSPICNT = SPI_BUS | HOLD | SPEED;
    Check(Transfer(0x08) == 0, "with bit 15 clear the data register reads 0");
    AUXSPICNT = SPI_BUS | SPEED;
    Check(Transfer(0) == 0, "with bit 15 clear no byte goes out");
    AUXSPICNT = ENABLE | HOLD | SPEED;
    Check(Transfer(0x08) == 0, "with bit 13 clear the data register reads 0");
    AUXSPICNT = ENABLE | SPEED;
    Check(Transfer(0) == 0, "with bit 13 clear no byte goes out");
    // The ignored bytes left no selection behind: the next exchange starts one
    ReadId(&first, &second);
    Check(first == 0x00 && second == 0xaa, "ignored bytes don't select the chip");
    // Answers aren't readable once the bus is off, though the chip keeps them
    AUXSPICNT = ENABLE | SPEED;
    Check((uint8_t)AUXSPIDATA == 0, "with bit 13 clear the last answer reads 0");
    AUXSPICNT = ENABLE | SPI_BUS | SPEED;
    Check((uint8_t)AUXSPIDATA == 0xaa, "with the bus back the last answer reads again");
}

static void TestDeselect(void) {
    uint8_t answer;

    // Held, the ID command, then a second ID command still held: it is the selection's second byte, so it gets the ID
    port_card_spi_reset();
    AUXSPICNT = ENABLE | SPI_BUS | HOLD | SPEED;
    Transfer(0x08);
    answer = Transfer(0x08);
    Check(answer == 0xaa, "a second byte of a held selection is answered as the command's");
    AUXSPICNT = ENABLE | SPI_BUS | SPEED;
    Transfer(0);

    // Held, the ID command, then bit 13 cleared while bits 13 and 6 are set: that deselects the chip
    // (NDSCartSlot::WriteSPICnt), so the next byte starts a selection and is latched as a command
    port_card_spi_reset();
    AUXSPICNT = ENABLE | SPI_BUS | HOLD | SPEED;
    Transfer(0x08);
    AUXSPICNT = ENABLE | HOLD | SPEED;
    AUXSPICNT = ENABLE | SPI_BUS | HOLD | SPEED;
    answer = Transfer(0x08);
    Check(answer == 0x00, "leaving the SPI bus while held deselects the chip: the next byte is a command");
    AUXSPICNT = ENABLE | SPI_BUS | SPEED;
    Check(Transfer(0) == 0xaa, "and the byte after it gets the ID");
}

static void TestControl(void) {
    port_card_spi_reset();
    Check(AUXSPICNT == 0, "AUXSPICNT reads 0 at power-on");
    AUXSPICNT = 0xffff;
    Check(AUXSPICNT == 0xe043, "a write keeps bits 15, 14, 13, 6, 1 and 0, and never sets busy");
    AUXSPICNT = ENABLE | SPI_BUS | HOLD | SPEED;
    AUXSPIDATA = 0x08;
    Check((AUXSPICNT & BUSY) == 0, "the busy bit reads clear right after a byte is sent");
    Check(AUXSPICNT == (ENABLE | SPI_BUS | HOLD | SPEED), "AUXSPICNT reads back what was written");
}

static void TestSameValue(void) {
    uint16_t value;

    // ir_check.c reads the answer 0 to the ID command, then sends 0: the same value as the cell held. It is a byte
    // sent all the same, which plain memory would lose, leaving the answer at 0
    port_card_spi_reset();
    AUXSPICNT = ENABLE | SPI_BUS | HOLD | SPEED;
    AUXSPIDATA = 0x08;
    value = AUXSPIDATA;
    Check((uint8_t)value == 0, "the ID command's answer is 0");
    AUXSPICNT = ENABLE | SPI_BUS | SPEED;
    AUXSPIDATA = (uint8_t)value;
    Check((uint8_t)AUXSPIDATA == 0xaa, "a byte equal to the last answer is still sent");

    // Reads alone send nothing
    port_card_spi_reset();
    AUXSPICNT = ENABLE | SPI_BUS | HOLD | SPEED;
    AUXSPIDATA = 0x08;
    for (int i = 0; i < 3; i++) {
        Check((uint8_t)AUXSPIDATA == 0, "reading the data register again sends nothing");
    }
    AUXSPICNT = ENABLE | SPI_BUS | SPEED;
    Check(Transfer(0) == 0xaa, "so the next byte sent is still the command's second");
}

static void TestLowByte(void) {
    port_card_spi_reset();
    AUXSPICNT = ENABLE | SPI_BUS | HOLD | SPEED;
    AUXSPIDATA = 0x08;
    Check((AUXSPIDATA & 0xff) == 0x00, "the low byte is the answer, 0, never the cell's marker");
    AUXSPICNT = ENABLE | SPI_BUS | SPEED;
    AUXSPIDATA = 0;
    Check((AUXSPIDATA & 0xff) == 0xaa, "the low byte is the answer, 0xaa");
}

int main(void) {
    TestGenuineCheck();
    TestReceive();
    TestDisabled();
    TestDeselect();
    TestControl();
    TestSameValue();
    TestLowByte();
    printf("card_spi: %s\n", sFailed ? "FAIL" : "ok");
    return sFailed;
}
