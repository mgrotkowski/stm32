#include <stm32f411xe.h>
#include "spi.h"
#include "delay.h"
#include "gpio_configure.h"

// Blocking - used only in init
void SPITransmitBlocking(uint32_t data, enum CommandFlag command, uint32_t num_items)
{

    uint32_t iter = 0;
    while(num_items--)
    {
        while((SPI3->SR & SPI_SR_TXE) == 0)
           Delay(1);

        while(SPI3->SR & SPI_SR_BSY)
            Delay(1);
        
        SPI3->DR = (0xff & (data >> 8 * num_items));
        Delay(1); // GODLIKE FINALLY FIXED!!!!!!!!!
    }
}
