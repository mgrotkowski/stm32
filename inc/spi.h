#ifndef SPI_H
#define SPI_H

#include "globals.h"

void SPITransmitBlocking(uint32_t data, enum CommandFlag command, uint32_t num_items);

#endif
