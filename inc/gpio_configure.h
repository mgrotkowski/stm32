#ifndef GPIO_CONFIGURE_H
#define GPIO_CONFIGURE_H

#include <stm32f411xe.h>
#include <gpio.h>
#include <lcd_board_def.h>
#include <lcd.h>

#define GPIO_LCD_CS   xcat(GPIO, LCD_CS_GPIO_N)
#define GPIO_LCD_A0   xcat(GPIO, LCD_A0_GPIO_N)
#define GPIO_LCD_SDA  xcat(GPIO, LCD_SDA_GPIO_N)
#define GPIO_LCD_SCK  xcat(GPIO, LCD_SCK_GPIO_N)

#define PIN_LCD_CS    (1U << LCD_CS_PIN_N)
#define PIN_LCD_A0    (1U << LCD_A0_PIN_N)
#define PIN_LCD_SDA   (1U << LCD_SDA_PIN_N)
#define PIN_LCD_SCK   (1U << LCD_SCK_PIN_N)
//#define LCD_CS_GPIO GPIOC
//#define LCD_A0_GPIO GPIOD
//#define LCD_SDA_GPIO GPIOC
//#define LCD_SCK_GPIO GPIOC

void RCCconfigure(void);
void GPIOconfigure(void);
void SPIconfigure(void);
void SPIconfigureDMA(void);
void DMAconfigure(void);
void NVICconfigure(void);
void TIMconfigure(void);

void A0(uint32_t bit);
void CS(uint32_t bit);
void SDA(uint32_t bit);
void SCK(uint32_t bit);
#endif
