#include "stm32f10x.h"
#include "math.h"
#include "stdio.h"

void GPIO_Config(void);
void ADC_DMA_Config(void);
void UART_Config(void);
void TIM2_Init(void);
void delay1ms(void);
void delay_ms(unsigned int time_ms);
void UART_SendChar(char _chr);
void UART_SendStr(char *str);
int Map(int x, int in_min, int in_max, int out_min, int out_max);
float GetInternalTemperature(uint16_t adcValue);

#define ADC_BUFFER_SIZE 5  

volatile uint16_t ADC_ConvertedValue[ADC_BUFFER_SIZE]; 
volatile uint8_t ADC_ConversionComplete = 0;           

char buffer[100];

int main()
{
    float temperature;
    float average_adc;
    
    GPIO_Config();
    ADC_DMA_Config();
    UART_Config();
    TIM2_Init();

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    
    while(1)
    {
        if(ADC_ConversionComplete) 
        {
            average_adc = 0;
            for(int i = 0; i < ADC_BUFFER_SIZE; i++) {
                average_adc += ADC_ConvertedValue[i];
            }
            average_adc /= ADC_BUFFER_SIZE;
            
            UART_SendStr("ADC Values: ");
            for(int i = 0; i < ADC_BUFFER_SIZE; i++) {
                sprintf(buffer, "[%d]=%d ", i, ADC_ConvertedValue[i]);
                UART_SendStr(buffer);
            }
            UART_SendStr("\r\n");
            
            temperature = GetInternalTemperature((uint16_t)average_adc);
            
            sprintf(buffer, "Average ADC: %.2f, Temperature: %.2f C\r\n\n", average_adc, temperature);
            UART_SendStr(buffer);

            ADC_ConversionComplete = 0;

            ADC_SoftwareStartConvCmd(ADC1, ENABLE);
            
            delay_ms(1000);
        }
    }
}

float GetInternalTemperature(uint16_t adcValue)
{
    float V25 = 1.43;        
    float Avg_Slope = 4.3;   
    float adcVoltage;
    
    adcVoltage = ((float)adcValue * 3.3) / 4095.0;
    sprintf(buffer, "Voltage: %.2f V\r\n", adcVoltage);
    UART_SendStr(buffer);
    
    // T(°C) = (V25 - Vsense)/Avg_Slope + 25
    return ((V25 - adcVoltage) * 1000.0 / Avg_Slope) + 25.0;
}

void GPIO_Config(void){
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void ADC_DMA_Config(void){
    ADC_InitTypeDef ADC_InitStructure;
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 1. Enable clock for DMA1, ADC1, and AFIO
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1 | RCC_APB2Periph_AFIO, ENABLE);
    
    // 2. Configure DMA1 Channel1 for ADC1
    DMA_DeInit(DMA1_Channel1);
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADC_ConvertedValue;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = ADC_BUFFER_SIZE;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular; // Thay đổi sang chế độ Circular
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &DMA_InitStructure);
    
    // 3. Enable DMA1 Channel1 Transfer Complete interrupt
    DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);
    
    // 4. Configure NVIC for DMA1 Channel1 interrupt
    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 5. Enable DMA1 Channel1
    DMA_Cmd(DMA1_Channel1, ENABLE);
    
    // 6. Configure ADC1
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = ENABLE;                
    ADC_InitStructure.ADC_ContinuousConvMode = ENABLE;         
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;                     
    ADC_Init(ADC1, &ADC_InitStructure);
    
    // 7. Configure ADC1 channel for internal temperature sensor
    ADC_TempSensorVrefintCmd(ENABLE);   
    ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5);
    
    // 8. Enable ADC1 DMA
    ADC_DMACmd(ADC1, ENABLE);
    
    // 9. Enable ADC1
    ADC_Cmd(ADC1, ENABLE);
    
    // 10. Calibrate ADC1
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
}


void DMA1_Channel1_IRQHandler(void) 
{

    if(DMA_GetITStatus(DMA1_IT_TC1)) 
    {

        DMA_ClearITPendingBit(DMA1_IT_TC1);
        
        ADC_ConversionComplete = 1;
    }
}

int Map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void UART_Config(void)
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

void UART_SendChar(char _chr){
    USART_SendData(USART1,_chr);
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE)==RESET);
}

void UART_SendStr(char *str){
    while(*str != '\0'){
        UART_SendChar(*str++);        
    }
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