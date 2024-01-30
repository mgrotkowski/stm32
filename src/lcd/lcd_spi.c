#include <stm32f411xe.h>
#include <delay.h>
#include <fonts.h>
#include <gpio.h>
#include <lcd_board_def.h>
#include "lcd_dma.h"
#include "dma.h"
#include "gpio_configure.h"
#include "spi.h"

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





static void LCDwriteCommand(uint32_t data)
{
    CS(0);
    A0(0);
    SPITransmitBlocking(data, COMMAND, 1);
    A0(1);
    CS(1);
}

static void LCDwriteData8(uint8_t data)
{
    CS(0);
    SPITransmitBlocking((uint32_t) data, DATA, 1);
    CS(1);
}

static void LCDwriteData16(uint16_t data)
{

    CS(0);
    SPITransmitBlocking((uint32_t) data, DATA, 2);
    CS(1);
}

static void LCDwriteData24(uint32_t data)
{

    CS(0);
    SPITransmitBlocking((uint32_t) data, DATA, 3);
    CS(1);
}

static void LCDwriteData32(uint32_t data)
{

    CS(0);
    SPITransmitBlocking((uint32_t) data, DATA, 4);
    CS(1);
}

void LCDsetRectangle(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2) {
  LCDwriteCommand(0x2A);
  LCDwriteData16(x1);
  LCDwriteData16(x2);

  LCDwriteCommand(0x2B);
  LCDwriteData16(y1);
  LCDwriteData16(y2);

  LCDwriteCommand(0x2C);
}


static void LCDcontrollerConfigure(void) {
  /* Activate chip select */
  CS(0);

  Delay(Tinit);

  /* Sleep out */
  LCDwriteCommand(0x11);

  Delay(T120ms);

  /* Frame rate */
  LCDwriteCommand(0xB1);
  LCDwriteData24(0x053C3C);
  LCDwriteCommand(0xB2);
  LCDwriteData24(0x053C3C);
  LCDwriteCommand(0xB3);
  LCDwriteData24(0x053C3C);
  LCDwriteData24(0x053C3C);

  /* Dot inversion */
  LCDwriteCommand(0xB4);
  LCDwriteData8(0x03);

  /* Power sequence */
  LCDwriteCommand(0xC0);
  LCDwriteData24(0x280804);
  LCDwriteCommand(0xC1);
  LCDwriteData8(0xC0);
  LCDwriteCommand(0xC2);
  LCDwriteData16(0x0D00);
  LCDwriteCommand(0xC3);
  LCDwriteData16(0x8D2A);
  LCDwriteCommand(0xC4);
  LCDwriteData16(0x8DEE);

  /* VCOM */
  LCDwriteCommand(0xC5);
  LCDwriteData8(0x1A);

  /* Memory and color write direction */
  LCDwriteCommand(0x36);
  LCDwriteData8(0xC0);

  /* Color mode 16 bit per pixel */
  LCDwriteCommand(0x3A);
  LCDwriteData8(0x05);

  /* Gamma sequence */
  LCDwriteCommand(0xE0);
  LCDwriteData32(0x0422070A);
  LCDwriteData32(0x2E30252A);
  LCDwriteData32(0x28262E3A);
  LCDwriteData32(0x00010313);
  LCDwriteCommand(0xE1);
  LCDwriteData32(0x0416060D);
  LCDwriteData32(0x2D262327);
  LCDwriteData32(0x27252D3B);
  LCDwriteData32(0x00010413);

  /* Display on */
  LCDwriteCommand(0x29);

  /* Deactivate chip select */
  CS(1);
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

static void LCDdrawChar(unsigned c) {
  uint16_t const *p;
  uint16_t x, y, w;
  int      i, j;

  y = YOffset + CurrentFont->height * Line;
  x = XOffset + CurrentFont->width  * Position;
  LCDsetRectangle(x, y, x + CurrentFont->width - 1, y + CurrentFont->height - 1);
  p = &CurrentFont->table[(c - FIRST_CHAR) * CurrentFont->height];
  for (i = 0; i < CurrentFont->height; ++i) {
    for (j = 0, w = p[i]; j < CurrentFont->width; ++j, w >>= 1) {
      LCDwriteData16(w & 1 ? TextColor : BackColor);
    }
  }
}


/** Public interface implementation **/
void LCDclear() {
  int i, j;

  LCDsetRectangle(0, 0, LCD_PIXEL_WIDTH - 1, LCD_PIXEL_HEIGHT - 1);
  for (i = 0; i < LCD_PIXEL_WIDTH; ++i) {
    for (j = 0; j < LCD_PIXEL_HEIGHT; ++j) {
      LCDwriteData16(BackColor);
    }
  }

  LCDgoto(0, 0);
}


void LCDconfigure() {
  /* See Errata, 2.1.6 Delay after an RCC peripheral clock enabling */
  RCCconfigure();
  /* Initialize global variables. */
  LCDsetFont(&LCD_DEFAULT_FONT);
  LCDsetColors(LCD_COLOR_WHITE, LCD_COLOR_BLUE);
  /* Initialize hardware. */
  //DMAconfigure();
  //NVICconfigure();
  A0(1);
  GPIOconfigure();
  SPIconfigure();
  LCDcontrollerConfigure();
  LCDclear();
}


void LCDgoto(int textLine, int charPos) {
  Line = textLine;
  Position = charPos;
}

void LCDputchar(char c) {
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
      LCDdrawChar(c);
    }
    LCDgoto(Line, Position + 1);
  }
}


void LCDputcharWrap(char c) {
  /* Check if, there is room for the next character,
  but does not wrap on white character. */
  if (Position >= TextWidth &&
      c != '\t' && c != '\r' &&  c != '\n' && c != ' ') {
    LCDputchar('\n');
  }
  LCDputchar(c);
}