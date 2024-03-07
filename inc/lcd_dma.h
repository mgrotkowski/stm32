#ifndef LCD_DMA_H
#define LCD_DMA_H
#include <stdint.h>

void LCDconfigure(void);
void LCDclearDMA(void);
void LCDgoto(int textLine, int charPos);
void LCDputcharDMA(char c);
void LCDputcharWrapDMA(char c);
void LCDsetRectangleDMA(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2); 
void LCDcubeEnc();
void LCDcube();

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


#endif
