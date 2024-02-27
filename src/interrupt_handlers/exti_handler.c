#include "globals.h"
uint8_t prev_tim_cnt = 0;

uint8_t enc_input_head = 0;
uint8_t enc_input_insert_ptr = 0;
uint8_t enc_input_items = 0;

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

