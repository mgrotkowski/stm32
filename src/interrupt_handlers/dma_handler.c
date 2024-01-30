#include "globals.h"
#include "delay.h"
#include "gpio_configure.h"

DMApacket global_dma_queue[];


void DMA1_Stream5_IRQHandler()
{
    uint32_t isr = DMA1->HISR;

    if (isr & DMA_HISR_TCIF5)
    {
        //Wait for SPI to finish
        while((SPI3->SR & SPI_SR_TXE) == 0)
            Delay(1);
        while(SPI3->SR & SPI_SR_BSY)
            Delay(1);
        //Important to set this flag before reenabling DMA stream
        DMA1->HIFCR = DMA_HIFCR_CTCIF5; 
       // Delay(1);

        if (dma_queue_items)
        {
            DMApacket packet = global_dma_queue[dma_queue_head];
            if (packet.command != IMAGE)
                DMA1_Stream5->M0AR = (uint32_t) packet.packets;
            else 
                DMA1_Stream5->M0AR = (uint32_t) imageBytes;
            DMA1_Stream5->NDTR = packet.num_items;
            
            dma_queue_head++;
            dma_queue_head %= QUEUE_SIZE;
            dma_queue_items--;
            CS(0);
            A0(packet.command);
            DMA1_Stream5->CR |= DMA_SxCR_EN;
            
        }
        else
            CS(1);

    }

}
