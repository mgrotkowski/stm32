#include "dma.h"
#include "irq.h"
#include "queue.h"
#include "gpio_configure.h"
#include "globals.h"

DMApacket first_packet;

static void initiate_transfer(
                       uint32_t data, 
                       uint32_t num_items, 
                       enum CommandFlag command
                       )
{
    if (command != IMAGE) {
    for(int i = 0; i < num_items; i++)
        first_packet.packets[i] =  (uint8_t)(0xff & (data >> 8*(num_items - 1 -i)));
    first_packet.num_items = num_items;
    first_packet.command = command;
    DMA1_Stream5->M0AR = (uint32_t) first_packet.packets;
    }
    else 
    {
        DMA1_Stream5->M0AR = (uint32_t) frame_buffer;
        last_command = IMAGE;
        frame_buffer_lock = 1;
    }

    DMA1_Stream5->NDTR = num_items;
    
    A0(command);
    CS(0);
    DMA1_Stream5->CR |= DMA_SxCR_EN;
}

void DMA_transfer_request(uint32_t data, 
                          uint32_t num_items,
                          enum CommandFlag command)
{
    irq_level_t primask = IRQprotectAll(); 
    if ((DMA1_Stream5->CR & DMA_SxCR_EN) == 0 && (DMA1->HISR & DMA_HISR_TCIF5) == 0)
    {
        IRQunprotectAll(primask);
        initiate_transfer(data, num_items, command);
    }
    else
    {
        global_queue_insert(data, num_items, command);
        IRQunprotectAll(primask);
    }
}
