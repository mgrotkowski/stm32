#ifndef DMA_H
#define DMA_H

#include <stm32f411xe.h>
#include <gpio.h>

enum CommandFlag
{
    COMMAND,
    DATA,
    IMAGE
};


void DMA_transfer_request(uint32_t data, 
                          uint32_t num_items,
                          enum CommandFlag command);

#endif
