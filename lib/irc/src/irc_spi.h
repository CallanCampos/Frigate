#ifndef POKEBW2_IRC_IRC_SPI_H
#define POKEBW2_IRC_IRC_SPI_H

#include "types.h"

// irc_spi.c, in overlay 31: the card's infrared chip over the backup SPI bus (see net_devirc.h)

// Every byte on the wire is xored with this
#define IRC_XOR 0xaa

void IrcSpi_Init(void);
void IrcSpi_Release(void);
// Reads a frame into buf and gives its size, 0 if there is none
u8 IrcSpi_Recv(u8 *buf);
void IrcSpi_Send(const u8 *data, int size);

#endif // POKEBW2_IRC_IRC_SPI_H
