#include "stm32f10x.h"
#include "math.h"
#include "stdio.h"
void GPIO_Config(void);
void ADC_Config(void);
void UART_Config(void);
void TIM2_Init(void);
void delay1ms(void);
void delay_ms(unsigned int time_ms);
void UART_SendChar(char _chr);
void UART_SendStr(char *str);
unsigned int ADCx_Read(void);
int Map(int x, int in_min, int in_max, int out_min, int out_max);

float GetInternalTemperature(uint16_t adcValue);

char buffer[100];
int main()
{
    GPIO_Config();
    ADC_Config();
    UART_Config();
    TIM2_Init();
    uint16_t adc_value;
    float temperature;
    
    while(1)
    {
        adc_value = ADCx_Read();

        temperature = GetInternalTemperature(adc_value);
        
        sprintf(buffer, "Temperature: %.2f C \r\n\n", temperature);
        UART_SendStr(buffer);
        
        delay_ms(1000);
    }
}

float GetInternalTemperature(uint16_t adcValue)
{
    float V25 = 1.43;        
    float Avg_Slope = 4.3;   
    float adcVoltage;
    

    adcVoltage = ((float)adcValue * 3.3) / 4095.0;
    sprintf(buffer, "Voltage: %.2f V \r\n", adcVoltage);
    UART_SendStr(buffer);
    // T(°C) = (V25 - Vsense)/Avg_Slope + 25

    
    return ((V25 - adcVoltage) * 1000.0 / Avg_Slope) + 25.0;
}

void GPIO_Config(void){
  GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
}

void ADC_Config(void){
    ADC_InitTypeDef     ADC_InitStructure;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    
    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;    
    ADC_Init(ADC1, &ADC_InitStructure);
    

    ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_239Cycles5);
    

    ADC_TempSensorVrefintCmd(ENABLE);
    
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while(ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

unsigned int ADCx_Read(void){
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    return ADC_GetConversionValue(ADC1);
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