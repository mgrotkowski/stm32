#include <gpio.h>
//#include "lcd.h"
#include "delay.h"
#include "lcd_dma.h"

int main(void)
{
    LCDconfigure();
    int iter = 0;
    int array_iter = 0;
    char arr[2] = {'a', 'b'};
    char curr_char = arr[array_iter];
    while(1)
    {
        LCDputcharDMA(curr_char);
        Delay(1000000);
        iter++;
        if (!(iter % 8))
        {
            iter %= 8;
            array_iter++;
            array_iter %= 2;
            curr_char = arr[array_iter];
            LCDgoto(0, 0);
        }
    }

}
