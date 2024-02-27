#include "gpio_configure.h"

// enable clocks needed for LCD (GPIOC, GPIOD), DMA1, SPI3
void RCCconfigure(void) {

    /***************LCD******************/

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;

    /***************ENCODER******************/

    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN; // counter
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // encoder pins
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // SYSCFG for EXTI interrupts


}


//Call this function *AFTER* GPIOconfigure
void TIMconfigure(void)
{
    TIM3->CR1 = TIM_CR1_URS;
    TIM3->SMCR = TIM_SMCR_SMS_1 | TIM_SMCR_SMS_0; 
    TIM3->CCMR1 = TIM_CCMR1_CC1S_0 | TIM_CCMR1_CC2S_0 |
                  TIM_CCMR1_IC1F_0 | TIM_CCMR1_IC1F_1 |
                  TIM_CCMR1_IC2F_0 | TIM_CCMR1_IC2F_1 ;
    TIM3->CCER = TIM_CCER_CC1P | TIM_CCER_CC2P;
    TIM3->PSC = 0;
    TIM3->ARR = 95;
    TIM3->EGR = TIM_EGR_UG;

    GPIOafConfigure(GPIOB, 4, GPIO_OType_PP, GPIO_Low_Speed, 
                    GPIO_PuPd_UP, GPIO_AF_TIM3);
    GPIOafConfigure(GPIOB, 5, GPIO_OType_PP, GPIO_Low_Speed, 
                    GPIO_PuPd_UP, GPIO_AF_TIM3); 
    TIM3->CR1 |= TIM_CR1_CEN;


}


// Set SPI3 alternative functions to SCK, SDA pins
void GPIOconfigure(void)
{

    /****************************LCD*******************************/

    // Configure A0, CS outputs
    CS(1);
    GPIOoutConfigure(GPIO_LCD_CS, LCD_CS_PIN_N, GPIO_OType_PP,
                GPIO_High_Speed, GPIO_PuPd_NOPULL);

    A0(1);
    GPIOoutConfigure(GPIO_LCD_A0, LCD_A0_PIN_N, GPIO_OType_PP,
                GPIO_High_Speed, GPIO_PuPd_NOPULL);
    //set SPI3 alternative functions to SDA and SCK 
    SDA(0);
    GPIOoutConfigure(GPIO_LCD_SDA, LCD_SDA_PIN_N, GPIO_OType_PP,
                    GPIO_High_Speed, GPIO_PuPd_NOPULL);

    SCK(0);
    GPIOoutConfigure(GPIO_LCD_SCK, LCD_SCK_PIN_N, GPIO_OType_PP,
                  GPIO_High_Speed, GPIO_PuPd_NOPULL);

    /****************************ENCODER*******************************/
    GPIOinConfigure(GPIOB, 4, GPIO_PuPd_NOPULL, EXTI_Mode_Interrupt, 
                   EXTI_Trigger_Rising_Falling);
    GPIOinConfigure(GPIOB, 5, GPIO_PuPd_NOPULL,  EXTI_Mode_Interrupt, 
              EXTI_Trigger_Rising_Falling);
}



void DMAconfigure(void)
{
   DMA1_Stream5->CR = DMA_SxCR_TCIE | DMA_SxCR_DIR_0 | DMA_SxCR_MINC;
   DMA1_Stream5->PAR = (uint32_t) &SPI3->DR;
   
}

// LCD Communicates through SPI3, SPI is configured using the following bits in CR1 register:
// SPI_CR1_MSTR - Master configuration
// SPI_CR1_SSM | SPI_CR1_SSI - Software selects slave 
// SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE - Transmit only mode (1 line bidirectional data with ouput enabled)
// We would like to use DMA for transfer so we set the TXDMAEN bit in CR2 register -> This issues a DMA interrupt when the transfer has completed
void SPIconfigure(void)
{
    GPIOafConfigure(GPIO_LCD_SDA, LCD_SDA_PIN_N, GPIO_OType_PP, GPIO_High_Speed , GPIO_PuPd_NOPULL, GPIO_AF_SPI3);
    GPIOafConfigure(GPIO_LCD_SCK, LCD_SCK_PIN_N, GPIO_OType_PP, GPIO_High_Speed , GPIO_PuPd_NOPULL, GPIO_AF_SPI3); 
    SPI3->CR1 =  SPI_CR1_MSTR | 
                 SPI_CR1_SSM | 
                 SPI_CR1_SSI |
                 SPI_CR1_BIDIOE |
                 SPI_CR1_BIDIMODE;

    SPI3->CR1 |=  SPI_CR1_SPE;
}

void SPIconfigureDMA(void)
{
    GPIOafConfigure(GPIO_LCD_SDA, LCD_SDA_PIN_N, GPIO_OType_PP, GPIO_High_Speed , GPIO_PuPd_NOPULL, GPIO_AF_SPI3);
    GPIOafConfigure(GPIO_LCD_SCK, LCD_SCK_PIN_N, GPIO_OType_PP, GPIO_High_Speed , GPIO_PuPd_NOPULL, GPIO_AF_SPI3); 
    SPI3->CR1 =  SPI_CR1_MSTR | 
                 SPI_CR1_SSM | 
                 SPI_CR1_SSI |
                 SPI_CR1_BIDIOE |
                 SPI_CR1_BIDIMODE;

    SPI3->CR2 = SPI_CR2_TXDMAEN;
    SPI3->CR1 |=  SPI_CR1_SPE;
}


void NVICconfigure(void)
{
    DMA1->HISR = DMA_HIFCR_CTCIF5;
    NVIC_SetPriority(DMA1_Stream5_IRQn, 0);
    NVIC_SetPriority(EXTI4_IRQn, 1);
    NVIC_SetPriority(EXTI9_5_IRQn, 1);
    NVIC_EnableIRQ(DMA1_Stream5_IRQn);
    NVIC_EnableIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
}



// Helper functions for communication with LCD driver

void CS(uint32_t bit) {
  if (bit) {
    GPIO_LCD_CS->BSRR = PIN_LCD_CS; /* Activate chip select line. */
  }
  else {
    GPIO_LCD_CS->BSRR = PIN_LCD_CS<< 16; /* Deactivate chip select line. */
  }
}

void A0(uint32_t bit) {
  if (bit) {
    GPIO_LCD_A0->BSRR = PIN_LCD_A0; /* Set data/command line to data. */
  }
  else {
    GPIO_LCD_A0->BSRR = PIN_LCD_A0 << 16; /* Set data/command line to command. */
  }
}

void SDA(uint32_t bit) {
  if (bit) {
    GPIO_LCD_SDA->BSRR = PIN_LCD_SDA; /* Set data bit one. */
  }
  else {
    GPIO_LCD_SDA->BSRR = PIN_LCD_SDA<< 16; /* Set data bit zero. */
  }
}

void SCK(uint32_t bit) {
  if (bit) {
    GPIO_LCD_SCK->BSRR = PIN_LCD_SCK; /* Set data bit one. */
  }
  else {
    GPIO_LCD_SCK->BSRR = PIN_LCD_SCK<< 16; /* Set data bit zero. */
  }
}
