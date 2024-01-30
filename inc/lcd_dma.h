#ifndef LCD_DMA_H
#define LCD_DMA_H

void LCDconfigure(void);
void LCDclearDMA(void);
void LCDgoto(int textLine, int charPos);
void LCDputcharDMA(char c);
void LCDputcharWrapDMA(char c);


#endif
