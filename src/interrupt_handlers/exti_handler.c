#include "globals.h"

volatile EncoderInput direction = ENCODER_NO_INPUT;

void EXTI4_IRQHandler(void)
{

    if (EXTI->IMR & EXTI->PR & EXTI_PR_PR4)
    {
        EXTI->PR |= EXTI_PR_PR4;
        if(!(EXTI->PR & EXTI_PR_PR5))
        {
        if (!(TIM3->CR1 & TIM_CR1_DIR))
            direction = ENCODER_FORWARD;
        else
            direction = ENCODER_REVERSE;
        }
    }
}

void EXTI9_5_IRQHandler(void)
{

    if (EXTI->IMR & EXTI->PR & EXTI_PR_PR5)
    {
        EXTI->PR |= EXTI_PR_PR5;
        if(!(EXTI->PR & EXTI_PR_PR4))
        {
        if (!(TIM3->CR1 & TIM_CR1_DIR))
            direction = ENCODER_FORWARD;
        else
            direction = ENCODER_REVERSE;
        }
    }

}

