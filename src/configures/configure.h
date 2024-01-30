#include <stm32f411xe.h>
#include <gpio.h>
#include <lcd_board_def.h>
#include <lcd.h>

// enable peripherals needed for LCD (GPIOC, GPIOD) and DMA1
static void RCCconfigure(void) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_DMA1EN;
}


// Set SPI3 alternative functions to SCK, SDA pins
static void GPIOconfigure(void)
{
    GPIOafConfigure(GPIOC, LCD_SDA_PIN_N, GPIO_OType_PP, GPIO_High_Speed, GPIO_PuPd_NOPULL, GPIO_AF_SPI3);
    GPIOafConfigure(GPIOC, LCD_SCK_PIN_N, GPIO_OType_PP, GPIO_High_Speed, GPIO_PuPd_NOPULL, GPIO_AF_SPI3);
}

static void DMAconfigure(void)
{
   DMA1_Stream5->CR |= DMA_SxCR_TCIE | DMA_SxCR_DIR_0 | DMA_SxCR_MINC;
    // DMA1_Stream5->CR |= DMA_SxCR_MBURST_0 | DMA_SxCR_PL | DMA_Sx TODO: setting dma prio level,
    // should we transfer data in bursts ? Probably yes because we want to sent RGB triples 6 byte wide...
    // should we use fifo for transfers
   
   DMA1_Stream5->PAR = (uint32_t) &SPI3->DR;
   DMA1_Stream5->M0AR = (uint32_t) &frame_table;
   
}
