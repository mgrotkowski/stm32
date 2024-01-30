#include "dma.h"
#include "irq.h"
#include "queue.h"
#include "gpio_configure.h"
#include "globals.h"

DMApacket first_packet;

static void initiate_transfer(
                       uint32_t data_location, 
                       enum DMA_memory_width data_width, 
                       uint32_t num_items, 
                       enum CommandFlag command
                       )
{
//    //This is needed because of the following execution:
//    //1. DMA transfer is ongoing, no items in the queue
//    //2. Request another transfer while DMA is not done -> Try to Insert into the queue  -> DMA Interrupt before locking the queue
//    //3. DMA Interrupt finishes, founds no items in the queue it is done 
//    //4. Finish queue insertion, next DMA interrupt 
//    if (dma_queue_items)
//    {
//        global_queue_insert(data_location, data_width, num_items, command);
//        DMA1_Stream5->M0AR = (uint32_t) global_dma_queue[dma_queue_head].packets;
//        DMA1_Stream5->NDTR = global_dma_queue[dma_queue_head].num_items;
//        A0(global_dma_queue[dma_queue_head].command);
//        dma_queue_head++;
//        dma_queue_items--;
//        dma_queue_head %= QUEUE_SIZE;
//    }
//    else
//    {
//        for(int i = 0; i < num_items; i++)
//            first_packet.packets[i] =  (uint8_t)(0xff & (data_location >> 8*(num_items - 1 -i)));
//        first_packet.data_width = data_width;
//        first_packet.num_items = num_items;
//        first_packet.command = command;
//        DMA1_Stream5->M0AR = (uint32_t) first_packet.packets;
//        DMA1_Stream5->NDTR = num_items;
//        A0(command);
//    }
    if (command != IMAGE) {
    for(int i = 0; i < num_items; i++)
        first_packet.packets[i] =  (uint8_t)(0xff & (data_location >> 8*(num_items - 1 -i)));
    first_packet.data_width = data_width;
    first_packet.num_items = num_items;
    first_packet.command = command;
    DMA1_Stream5->M0AR = (uint32_t) first_packet.packets;
    }
    else 
        DMA1_Stream5->M0AR = (uint32_t) imageBytes;

    DMA1_Stream5->NDTR = num_items;
    
    A0(command);
    CS(0);
    DMA1_Stream5->CR |= DMA_SxCR_EN;
}

void DMA_transfer_request(uint32_t data_location, 
                          enum DMA_memory_width data_width, 
                          uint32_t num_items,
                          enum CommandFlag command)
{
    irq_level_t primask = IRQprotectAll(); 
    if ((DMA1_Stream5->CR & DMA_SxCR_EN) == 0 && (DMA1->HISR & DMA_HISR_TCIF5) == 0)
    {
        IRQunprotectAll(primask);
        initiate_transfer(data_location, data_width, num_items, command);
    }
    else
    {
        global_queue_insert(data_location, data_width, num_items, command);
        IRQunprotectAll(primask);
    }
}
