#include "stm32f10x.h"
#include "string.h"

void Uart_Config(void);
void TIM2_Init(void);
void GPIO_Config(void);
void delay1ms(void);
void delay_ms(unsigned int time_ms);
void USART1_IRQHandler(void);
void Uart_SendChar(char _chr);
void Uart_SendStr(char *str);

char rx_buffer[10];   
uint8_t rx_index = 0;

int main()
{
    Uart_Config();
    TIM2_Init();
    GPIO_Config();   

    while(1)
    {
        Uart_SendStr("Hello from STM32!\r\n");   
        delay_ms(1000);                    
    }
}

void Uart_Config(void)
{
    GPIO_InitTypeDef gpio_init;
    USART_InitTypeDef uart_init;
    NVIC_InitTypeDef nvic_typedef;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    // TX - A9
    gpio_init.GPIO_Pin = GPIO_Pin_9;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);

    // RX - A10
    gpio_init.GPIO_Pin = GPIO_Pin_10;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);

    uart_init.USART_BaudRate = 115200;
    uart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    uart_init.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    uart_init.USART_Parity = USART_Parity_No;
    uart_init.USART_StopBits = USART_StopBits_1;
    uart_init.USART_WordLength = USART_WordLength_8b;

    USART_Init(USART1, &uart_init);

    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
    USART_Cmd(USART1, ENABLE);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    nvic_typedef.NVIC_IRQChannel = USART1_IRQn;
    nvic_typedef.NVIC_IRQChannelPreemptionPriority = 0;
    nvic_typedef.NVIC_IRQChannelSubPriority = 0;
    nvic_typedef.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&nvic_typedef);
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);

}

void TIM2_Init(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);

    TIM_TimeBaseInitTypeDef timer_init;
    TIM_TimeBaseStructInit(&timer_init);   
    timer_init.TIM_Prescaler = 72 - 1;     
    timer_init.TIM_CounterMode = TIM_CounterMode_Up;
    timer_init.TIM_Period = 0xFFFF;
    timer_init.TIM_ClockDivision = TIM_CKD_DIV1; 
    TIM_TimeBaseInit(TIM2, &timer_init);
    TIM_Cmd(TIM2, ENABLE);
}

void GPIO_Config(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_InitTypeDef gpio_init;
    gpio_init.GPIO_Pin = GPIO_Pin_13;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOC, &gpio_init);

    GPIO_SetBits(GPIOC, GPIO_Pin_13);  
}

void Uart_SendChar(char _chr){
    USART_SendData(USART1,_chr);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE)==RESET);
}

void Uart_SendStr(char *str){
    while(*str != '\0'){
        Uart_SendChar(*str++);		
    }
}
void delay1ms(void)
{
    TIM_SetCounter(TIM2,0);
    while(TIM_GetCounter(TIM2) < 1000);
}

void delay_ms(unsigned int time_ms)
{
    while(time_ms){
        delay1ms();
        time_ms--;
    }
}

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        char c = (char)USART_ReceiveData(USART1);
        Uart_SendChar(c);
     
        if (c == '\r' || c == '\n' || c == '!') 
        {
            rx_buffer[rx_index] = '\0';
            

            if (strcmp(rx_buffer, "ON") == 0)
            {
                GPIO_ResetBits(GPIOC, GPIO_Pin_13); 
                Uart_SendStr("LED turned ON\r\n");
            }
            else if (strcmp(rx_buffer, "OFF") == 0)
            {
                GPIO_SetBits(GPIOC, GPIO_Pin_13);   
                Uart_SendStr("LED turned OFF\r\n");
            }
            

            rx_index = 0;
            memset(rx_buffer, 0, sizeof(rx_buffer));
        }
        else
        {
            if (rx_index < sizeof(rx_buffer) - 1)
            {
                rx_buffer[rx_index++] = c;
            }
            else
            {
                rx_index = 0;
                memset(rx_buffer, 0, sizeof(rx_buffer));
            }
        }

        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}

