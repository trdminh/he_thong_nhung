#include "stm32f10x.h"

void GPIO_Config(void);
void EXTI_Config(void);
void TIM2_Config(void);

int ledState = 0; 

int main(void)
{
    GPIO_Config();
    EXTI_Config();
    TIM2_Config();

    while(1)
    {
        
    }
}

void GPIO_Config(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef gpio;

    gpio.GPIO_Pin = GPIO_Pin_13;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOC, &gpio);
    GPIO_SetBits(GPIOC, GPIO_Pin_13); 

    gpio.GPIO_Pin = GPIO_Pin_1;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_Init(GPIOA, &gpio);
    GPIO_SetBits(GPIOA, GPIO_Pin_1);


    gpio.GPIO_Pin = GPIO_Pin_5;
    gpio.GPIO_Mode = GPIO_Mode_IPU; 
    GPIO_Init(GPIOA, &gpio);
}

void EXTI_Config(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource5);

    EXTI_InitTypeDef exti;
    exti.EXTI_Line = EXTI_Line5;
    exti.EXTI_Mode = EXTI_Mode_Interrupt;
    exti.EXTI_Trigger = EXTI_Trigger_Falling; 
    exti.EXTI_LineCmd = ENABLE;
    EXTI_Init(&exti);

    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel = EXTI9_5_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 0;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}

void TIM2_Config(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitTypeDef tim;
    tim.TIM_Period = 10000 - 1;       // 10000 tick
    tim.TIM_Prescaler = 7200 - 1;    // 72MHz / 7200 = 10kHz
    tim.TIM_ClockDivision = 0;
    tim.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2, &tim);

    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    TIM_Cmd(TIM2, ENABLE);

    NVIC_InitTypeDef nvic;
    nvic.NVIC_IRQChannel = TIM2_IRQn;
    nvic.NVIC_IRQChannelPreemptionPriority = 1;
    nvic.NVIC_IRQChannelSubPriority = 0;
    nvic.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic);
}

void EXTI9_5_IRQHandler(void)
{
    if (EXTI_GetITStatus(EXTI_Line5) != RESET)
    {
        ledState = !ledState;
        if (ledState)
            GPIO_ResetBits(GPIOC, GPIO_Pin_13); 
        else
            GPIO_SetBits(GPIOC, GPIO_Pin_13);  

        EXTI_ClearITPendingBit(EXTI_Line5);
    }
}

void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        if (GPIO_ReadOutputDataBit(GPIOA, GPIO_Pin_1))
            GPIO_ResetBits(GPIOA, GPIO_Pin_1);
        else
            GPIO_SetBits(GPIOA, GPIO_Pin_1);   

        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
