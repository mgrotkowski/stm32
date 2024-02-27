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


#endif
