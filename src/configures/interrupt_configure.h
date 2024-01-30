#include <stm32f411xe.h>
#include <gpio.h>
#include <lcd_board_def.h>
#include <lcd.h>


static void NVICconfigure(void)
{
    NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}

