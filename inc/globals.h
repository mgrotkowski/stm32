#ifndef GLOBALS_H
#define GLOBALS_H

#include <stm32f411xe.h>
#include <gpio.h>
#include "dma.h"


#define QUEUE_SIZE 1024 
#define MSIZE_flags (const uint32_t[]){0, DMA_SxCR_MSIZE_0, DMA_SxCR_MSIZE_1}

//typedef struct DMApacket
//{
//    uint32_t data_location;
//    enum DMA_memory_width data_width;
//    uint32_t num_items;
//    enum CommandFlag command;
//    
//} DMApacket;

typedef struct DMApacket
{
    uint8_t packets[4];
    enum DMA_memory_width data_width;
    uint16_t num_items;
    enum CommandFlag command;
    
} DMApacket;


extern uint32_t dma_queue_head;
extern uint32_t dma_queue_insert_ptr;
extern uint32_t dma_queue_items;
extern DMApacket global_dma_queue[QUEUE_SIZE];
extern DMApacket first_packet;
extern const uint8_t imageBytes[];
extern const uint8_t gifBytes[];

#endif
