#ifndef DMA_H
#define DMA_H

#include <stm32f411xe.h>
#include <gpio.h>


enum DMA_memory_width {
    DMA_BYTE,
    DMA_HALF_WORD,
    DMA_WORD
};

enum CommandFlag
{
    COMMAND,
    DATA,
    IMAGE
};


void DMA_transfer_request(uint32_t data_location, 
                          enum DMA_memory_width data_width,
                          uint32_t num_items,
                          enum CommandFlag command);

#endif
