#ifndef PORT_HAL_CARD_CARD_SPI_H
#define PORT_HAL_CARD_CARD_SPI_H

// The game card's SPI bus as the ARM9 sees it, AUXSPICNT (0x040001a0) and AUXSPIDATA (0x040001a2), with the infrared
// chip that BW2's cartridges have on that bus (card_spi.c, ADR 0011). The game's genuine-card check (overlay 338,
// src/ov338/ir_check.c) and the infrared library (overlay 31, lib/irc/src/irc_spi.c) talk to it.

// What an access to each register uses, as the host variants of the SDK's headers call them for reg_MI_MCCNT0 and
// reg_MI_MCD0 (port/tools/gen_host_tree.py's HW_REGISTER_POLICY): each call first carries out the store the game made
// through the pointer the previous call returned, if it made one, then returns the register's memory for this access
void *port_hw_auxspicnt(void);
void *port_hw_auxspidata(void);

// The bus and the chip as at power-on: no transfer under way, no command latched. The program starts so; tests call it
// between cases
void port_card_spi_reset(void);

#endif // PORT_HAL_CARD_CARD_SPI_H
