#include <irq.h>
#include <delay.h>

#include "globals.h"
#include "queue.h"

uint32_t dma_queue_items = 0;
uint32_t dma_queue_insert_ptr = 0;
uint32_t dma_queue_head = 0;

//Insert a packet to global dma queue
void global_queue_insert( uint32_t data_location, 
                          uint32_t num_items,
                          enum CommandFlag command
                          )
{
    // queue should be locked when accessed
    // In an unfortunate event that the queue is full: spin
    if (dma_queue_items < QUEUE_SIZE)
    {
        if (command != IMAGE)
        {
        for (int i = 0; i < num_items; i++)
        {
            global_dma_queue[dma_queue_insert_ptr].packets[i] 
                = (uint8_t)(0xff & (data_location >> 8*(num_items - 1 - i)));
        }
        }
        global_dma_queue[dma_queue_insert_ptr].num_items = num_items;
        global_dma_queue[dma_queue_insert_ptr].command = command;
        ++dma_queue_insert_ptr;
        dma_queue_insert_ptr %= QUEUE_SIZE;
        ++dma_queue_items;
    }

}

//Remove and return a packet from global queue
DMApacket* global_queue_pop(void)
{
    irq_level_t primask = IRQprotectAll();
    if(dma_queue_items > 0)
    {
        --dma_queue_items;
        uint32_t dma_queue_head_temp = dma_queue_head;
        dma_queue_head++;
        dma_queue_head %= QUEUE_SIZE;
        return &global_dma_queue[dma_queue_head_temp];
    }
    else
        return (void *) 0;
    IRQunprotectAll(primask);

}
