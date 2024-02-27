#include "globals.h"

#define abs_diff(x,y) x > y ? x - y : y -x

uint8_t prev_tim_cnt = 0;

uint8_t enc_input_head = 0;
uint8_t enc_input_insert_ptr = 0;
uint8_t enc_input_items = 0;
//void EXTI4_IRQHandler(void)
//{
//
//    if (EXTI->IMR & EXTI->PR & EXTI_PR_PR4)
//    {
//        EXTI->PR |= EXTI_PR_PR5 | EXTI_PR_PR4;
//        uint8_t curr_tim = TIM3->CNT;
//        EncoderInput curr_input = ENCODER_NO_INPUT;
//        if (!(TIM3->CR1 & TIM_CR1_DIR))
//            curr_input = ENCODER_FORWARD;
//        else
//            curr_input = ENCODER_REVERSE;
//
//        if (enc_input_items < 10)
//        {
//            enc_input[enc_input_insert_ptr++] = curr_input;
//            enc_input_items++;
//            enc_input_insert_ptr %= 10;
//        }
//        prev_tim_cnt = curr_tim;
//        
//
//    }
//
//}

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

//void EXTI9_5_IRQHandler(void)
//{
//
//    if (EXTI->IMR & EXTI->PR & EXTI_PR_PR5)
//    {
//        EXTI->PR |= EXTI_PR_PR5 | EXTI_PR_PR4;
//        uint8_t curr_tim = TIM3->CNT;
//        EncoderInput curr_input = ENCODER_NO_INPUT;
//        if (!(TIM3->CR1 & TIM_CR1_DIR))
//            curr_input = ENCODER_FORWARD;
//        else
//            curr_input = ENCODER_REVERSE;
//
//        if (enc_input_items < 10)
//        {
//            enc_input[enc_input_insert_ptr++] = curr_input;
//            enc_input_items++;
//            enc_input_insert_ptr %= 10;
//        }
//
//        prev_tim_cnt = curr_tim;
//    }
//
//
//}
