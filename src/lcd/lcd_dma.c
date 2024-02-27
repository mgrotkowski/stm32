#include <stm32f411xe.h>
#include <delay.h>
#include <fonts.h>
#include <gpio.h>
#include <lcd_board_def.h>
#include <irq.h>
#include "lcd_dma.h"
#include "dma.h"
#include "gpio_configure.h"
#include "gif.h"
#include "globals.h"

/** The simple LCD driver (only text mode) for ST7735S controller
    and STM32F2xx or STM32F4xx **/

/* Microcontroller pin definitions:
constants LCD_*_GPIO_N are port letter codes (A, B, C, ...),
constants LCD_*_PIN_N are the port output numbers (from 0 to 15),
constants GPIO_LCD_* are memory pointers,
constants PIN_LCD_* and RCC_LCD_* are bit masks. */

#define GPIO_LCD_CS   xcat(GPIO, LCD_CS_GPIO_N)
#define GPIO_LCD_A0   xcat(GPIO, LCD_A0_GPIO_N)
#define GPIO_LCD_SDA  xcat(GPIO, LCD_SDA_GPIO_N)
#define GPIO_LCD_SCK  xcat(GPIO, LCD_SCK_GPIO_N)

#define PIN_LCD_CS    (1U << LCD_CS_PIN_N)
#define PIN_LCD_A0    (1U << LCD_A0_PIN_N)
#define PIN_LCD_SDA   (1U << LCD_SDA_PIN_N)
#define PIN_LCD_SCK   (1U << LCD_SCK_PIN_N)

#define RCC_LCD_CS    xcat3(RCC_AHB1ENR_GPIO, LCD_CS_GPIO_N, EN)
#define RCC_LCD_A0    xcat3(RCC_AHB1ENR_GPIO, LCD_A0_GPIO_N, EN)
#define RCC_LCD_SDA   xcat3(RCC_AHB1ENR_GPIO, LCD_SDA_GPIO_N, EN)
#define RCC_LCD_SCK   xcat3(RCC_AHB1ENR_GPIO, LCD_SCK_GPIO_N, EN)

/* Screen size in pixels, left top corner has coordinates (0, 0). */

#define LCD_PIXEL_WIDTH   128
#define LCD_PIXEL_HEIGHT  160

/* Some color definitions */

#define LCD_COLOR_WHITE    0xFFFF
#define LCD_COLOR_BLACK    0x0000
#define LCD_COLOR_GREY     0xF7DE
#define LCD_COLOR_BLUE     0x001F
#define LCD_COLOR_BLUE2    0x051F
#define LCD_COLOR_RED      0xF800
#define LCD_COLOR_MAGENTA  0xF81F
#define LCD_COLOR_GREEN    0x07E0
#define LCD_COLOR_CYAN     0x7FFF
#define LCD_COLOR_YELLOW   0xFFE0

/* Needed delay(s)  */

#define Tinit   150
#define T120ms  (MAIN_CLOCK_MHZ * 120000 / 4)
#define T6ms  (MAIN_CLOCK_MHZ * 6000 / 4)

/* Text mode globals */

static const font_t *CurrentFont;
static uint16_t TextColor = LCD_COLOR_BLACK;
static uint16_t BackColor = LCD_COLOR_WHITE;

/* Current character line and position, the number of lines, the
number of characters in a line, position 0 and line 0 offset on screen
in pixels */

static int Line, Position, TextHeight, TextWidth, XOffset, YOffset;

/** Internal functions **/

/* The following four functions are inlined and "if" statement is
eliminated during optimization if the "bit" argument is a constant. */


static void LCDwrite8DMA(uint32_t data, enum CommandFlag command) {
    DMA_transfer_request(data, DMA_BYTE, 1, command);  
}

static void LCDwrite16DMA(uint32_t data, enum CommandFlag command)
{
    DMA_transfer_request(data, DMA_HALF_WORD, 2, command);  
}

static void LCDwrite24DMA(uint32_t data, enum CommandFlag command)
{
    DMA_transfer_request(data, DMA_BYTE, 3, command);
}

static void LCDwrite32DMA(uint32_t data, enum CommandFlag command)
{
    DMA_transfer_request(data, DMA_WORD, 4, command);
}

static void LCDwriteimageDMA(uint16_t num_items)
{
   imageBytes_lock = 1;
   DMA_transfer_request(0, DMA_BYTE, num_items, IMAGE);
}

static void LCDwriteCommandDMA(uint32_t data)
{
    LCDwrite8DMA(data, COMMAND);
}



void LCDsetRectangleDMA(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  LCDwriteCommandDMA(0x2A);
  LCDwrite16DMA(x1, DATA);
  LCDwrite16DMA(x2, DATA);

  LCDwriteCommandDMA(0x2B);
  LCDwrite16DMA(y1, DATA);
  LCDwrite16DMA(y2, DATA);

  LCDwriteCommandDMA(0x2C);
}



static void LCDcontrollerConfigureDMA(void) {
  /* Activate chip select */

  Delay(Tinit);

  /* Sleep out */
  LCDwriteCommandDMA(0x11);

  Delay(T120ms);

  /* Frame rate */
  LCDwriteCommandDMA(0xB1);
  LCDwrite24DMA(0x053C3C, DATA);
  LCDwriteCommandDMA(0xB2);
  LCDwrite24DMA(0x053C3C, DATA);
  LCDwriteCommandDMA(0xB3);
  LCDwrite24DMA(0x053C3C, DATA);
  LCDwrite24DMA(0x053C3C, DATA);

  /* Dot inversion */
  LCDwriteCommandDMA(0xB4);
  LCDwrite8DMA(0x03, DATA);

  /* Power sequence */
  LCDwriteCommandDMA(0xC0);
  LCDwrite24DMA(0x280804, DATA);
  LCDwriteCommandDMA(0xC1);
  LCDwrite8DMA(0xC0, DATA);
  LCDwriteCommandDMA(0xC2);
  LCDwrite16DMA(0x0D00, DATA);
  LCDwriteCommandDMA(0xC3);
  LCDwrite16DMA(0x8D2A, DATA);
  LCDwriteCommandDMA(0xC4);
  LCDwrite16DMA(0x8DEE, DATA);

  /* VCOM */
  LCDwriteCommandDMA(0xC5);
  LCDwrite8DMA(0x1A, DATA);

  /* Memory and color write direction */
  LCDwriteCommandDMA(0x36);
  LCDwrite8DMA(0xC0, DATA);
  //LCDwrite8DMA(0x00, DATA);
  /* Color mode 16 bit per pixel */
  LCDwriteCommandDMA(0x3A);
  LCDwrite8DMA(0x05, DATA);

  /* Gamma sequence */
  LCDwriteCommandDMA(0xE0);
  LCDwrite32DMA(0x0422070A, DATA);
  LCDwrite32DMA(0x2E30252A, DATA);
  LCDwrite32DMA(0x28262E3A, DATA);
  LCDwrite32DMA(0x00010313, DATA);
  LCDwriteCommandDMA(0xE1);
  LCDwrite32DMA(0x0416060D, DATA);
  LCDwrite32DMA(0x2D262327, DATA);
  LCDwrite32DMA(0x27252D3B, DATA);
  LCDwrite32DMA(0x00010413, DATA);

  /* Display on */
  LCDwriteCommandDMA(0x29);

  /* Deactivate chip select */
}


static void LCDsetFont(const font_t *font) {
  CurrentFont = font;
  TextHeight = LCD_PIXEL_HEIGHT / CurrentFont->height;
  TextWidth  = LCD_PIXEL_WIDTH  / CurrentFont->width;
  XOffset = (LCD_PIXEL_WIDTH  - TextWidth  * CurrentFont->width)  / 2;
  YOffset = (LCD_PIXEL_HEIGHT - TextHeight * CurrentFont->height) / 2;
}

static void LCDsetColors(uint16_t text, uint16_t back) {
  TextColor = text;
  BackColor = back;
}

static void LCDdrawCharDMA(unsigned c) {
  uint16_t const *p;
  uint16_t x, y, w;
  int      i, j;

  y = YOffset + CurrentFont->height * Line;
  x = XOffset + CurrentFont->width  * Position;
  LCDsetRectangleDMA(x, y, x + CurrentFont->width - 1, y + CurrentFont->height - 1);
  p = &CurrentFont->table[(c - FIRST_CHAR) * CurrentFont->height];
  for (i = 0; i < CurrentFont->height; ++i) {
    for (j = 0, w = p[i]; j < CurrentFont->width; ++j, w >>= 1) {
      LCDwrite16DMA(w & 1 ? TextColor : BackColor, DATA);
    }
  }
}


/** Public interface implementation **/
void LCDclearDMA() {
  int i, j;

  LCDsetRectangleDMA(0, 0, LCD_PIXEL_WIDTH - 1, LCD_PIXEL_HEIGHT - 1);
  for (i = 0; i < LCD_PIXEL_WIDTH; ++i) {
    for (j = 0; j < LCD_PIXEL_HEIGHT; ++j) {
      LCDwrite16DMA(BackColor, DATA);
    }
  }
  //for (i = 0; i < LCD_PIXEL_HEIGHT; ++i) {
   // for (j = 0; j < LCD_PIXEL_WIDTH; ++j) {
    //  LCDwrite16DMA(imageBytes[2*(i*LCD_PIXEL_WIDTH + j)] << 8 | imageBytes[2*(i*LCD_PIXEL_WIDTH + j) + 1], DATA);
    //}
  //}
  //LCDwriteimageDMA();

  LCDgoto(0, 0);
}

void LCDDisplayImage() {

  LCDgoto(0, 0);
  //decodeGIF();
  LCDsetRectangleDMA(0, 0, LCD_PIXEL_WIDTH - 1, LCD_PIXEL_HEIGHT - 1);
//  LCDwriteimageDMA();

}

void LCDDisplayFrame(FrameMetadata frame)

{

  LCDgoto(0, 0);
  //change write direction to work with image stored in 160 consecutive pixel strips
  LCDwriteCommandDMA(0x36);
  LCDwrite8DMA(0xA0, DATA);
  decodeGIF_args(frame);
  LCDsetRectangleDMA(frame.x_offset, 
                     frame.y_offset, 
                     frame.x_offset + frame.width - 1, 
                     frame.y_offset + frame.height - 1);
  LCDwriteimageDMA(frame.height * frame.width * 2);
  //revert back
  LCDwriteCommandDMA(0x36);
  LCDwrite8DMA(0xC0, DATA);

}

uint8_t imageBytes_lock = 0;

void LCDcube() {

  int idx = 0;
  while(1)
    {
        while(imageBytes_lock)
            Delay(1);
        LCDDisplayFrame(parseGIFHeaders(gce_offsets[idx++]));
        idx %= 30;

    }
}

EncoderInput enc_input[10];
void LCDcubeEnc()

{
    int idx = 0;
    LCDDisplayFrame(parseGIFHeaders(gce_offsets[0]));
    while(1)
    {
        irq_level_t primask = IRQprotectAll();
        if (direction == ENCODER_NO_INPUT)
        {
            IRQunprotectAll(primask);
        }
        else
        {
            if (direction == ENCODER_FORWARD)
                idx++;
            if (direction == ENCODER_REVERSE)
                idx--;
            direction = ENCODER_NO_INPUT;
            IRQunprotectAll(primask);
            if (idx < 0)
                idx += 30;
            idx %= 30;
            int temp = idx;
            LCDDisplayFrame(parseGIFHeaders(gce_offsets[idx]));
            if (temp / 10)
            {
                LCDputcharDMA(0x30 + temp/10);
                temp %= 10;
            }
                
            LCDputcharDMA(0x30 + temp);
            LCDgoto(0,0);
        }

    }

}
//void LCDcubeEnc()
//
//{
//
//  uint32_t gce_offsets[30] = {843, 7703, 14683, 21707, 28596, 37046, 45604,
//                              53773, 61564, 69542, 77604, 85719, 93802, 
//                                101769, 110020, 117810, 125414,
//                              133218, 141082, 148735, 155963, 163042,
//                              171240,  179192, 186898, 194720, 202536,
//                              210569, 217698, 226250};
//    int idx = 0;
//    EncoderInput command;
//    LCDDisplayFrame(parseGIFHeaders(gce_offsets[0]));
//    while(1)
//    {
//        irq_level_t primask = IRQprotectAll();
//        if (!enc_input_items)
//        {
//            IRQunprotectAll(primask);
//        }
//        else
//        {
//            command = enc_input[enc_input_head++];
//            enc_input_items--;
//            enc_input_head %= 10;
//            if (command == ENCODER_FORWARD)
//                idx++;
//            if (command == ENCODER_REVERSE)
//                idx--;
//            IRQunprotectAll(primask);
//            if (idx < 0)
//                idx += 30;
//            idx %= 30;
//            int temp = idx;
//            LCDDisplayFrame(parseGIFHeaders(gce_offsets[idx]));
//            if (temp / 10)
//            {
//                LCDputcharDMA(0x30 + temp/10);
//                temp %= 10;
//            }
//                
//            LCDputcharDMA(0x30 + temp);
//            LCDgoto(0,0);
//        }
//
//    }
//
//}

volatile EncoderInput direction = ENCODER_NO_INPUT;

void DebugTIM()

{

    volatile uint16_t temp = TIM3->CNT;
    while(1)
    {
            
    if (temp / 10)
        {
                LCDputcharDMA(0x30 + temp/10);
                temp %= 10;
                LCDputcharDMA(0x30 + temp);
        }
    else
    {
        LCDputcharDMA(0x30 + temp);
        LCDputcharDMA(' ');
    }
                
        LCDgoto(0,0);
        temp = TIM3->CNT;
    }
}


void LCDconfigure() {
  /* See Errata, 2.1.6 Delay after an RCC peripheral clock enabling */
  RCCconfigure();
  /* Initialize global variables. */
  LCDsetFont(&LCD_DEFAULT_FONT);
  LCDsetColors(LCD_COLOR_WHITE, LCD_COLOR_BLUE);
  /* Initialize hardware. */
  DMAconfigure();
  GPIOconfigure();
  TIMconfigure();
  SPIconfigureDMA();
  NVICconfigure();
  LCDcontrollerConfigureDMA();
  LCDclearDMA();
  //LCDDisplayImage();
  //DebugTIM();
  LCDcubeEnc();
  
}


void LCDgoto(int textLine, int charPos) {
  Line = textLine;
  Position = charPos;
}

void LCDputcharDMA(char c) {
  if (c == '\n')
    LCDgoto(Line + 1, 0); /* line feed */
  else if (c == '\r')
    LCDgoto(Line, 0); /* carriage return */
  else if (c == '\t')
    LCDgoto(Line, (Position + 8) & ~7); /* tabulator */
  else {
    if (c >= FIRST_CHAR && c <= LAST_CHAR &&
        Line >= 0 && Line < TextHeight &&
        Position >= 0 && Position < TextWidth) {
      LCDdrawCharDMA(c);
    }
    LCDgoto(Line, Position + 1);
  }
}


void LCDputcharWrapDMA(char c) {
  /* Check if, there is room for the next character,
  but does not wrap on white character. */
  if (Position >= TextWidth &&
      c != '\t' && c != '\r' &&  c != '\n' && c != ' ') {
    LCDputcharDMA('\n');
  }
  LCDputcharDMA(c);
}



void DebugTIMDir()

{

    LCDputcharDMA(' ');
    LCDputcharDMA('1');
    LCDgoto(0,0);
    EncoderInput dir;

    while(1)
    {
            
        dir = direction;
        if (dir == ENCODER_FORWARD)
            LCDputcharDMA('+');
        else if (dir == ENCODER_REVERSE)
            LCDputcharDMA('-');

        LCDgoto(0,0);
    }
        
}

