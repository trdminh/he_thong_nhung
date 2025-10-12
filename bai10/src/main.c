#include "stm32f10x.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_pwr.h"

#define LED_PIN     GPIO_Pin_13    
#define LED_PORT    GPIOC
#define BUTTON_PIN  GPIO_Pin_0     
#define BUTTON_PORT GPIOA


void RCC_Configuration(void);
void GPIO_Configuration(void);
void EXTI_Configuration(void);
void NVIC_Configuration(void);
void EnterStopMode(void);
void Delay_ms(uint32_t ms);
void PWR_Configuration(void);


volatile uint8_t wakeup_flag = 0;

int main(void)
{

    RCC_Configuration();
    GPIO_Configuration();
    EXTI_Configuration();
    NVIC_Configuration();
    PWR_Configuration();

    for(int i = 0; i < 5; i++) {
        GPIO_SetBits(LED_PORT, LED_PIN);    
        Delay_ms(100);
        GPIO_ResetBits(LED_PORT, LED_PIN);  
        Delay_ms(100);
    }
    
    while(1) {

        if(wakeup_flag) {
            wakeup_flag = 0;              
            
           
            for(int i = 0; i < 3; i++) {
                GPIO_SetBits(LED_PORT, LED_PIN);
                Delay_ms(100);
                GPIO_ResetBits(LED_PORT, LED_PIN);
                Delay_ms(100);
            }

            GPIO_SetBits(LED_PORT, LED_PIN);

            EnterStopMode();
            
            
            for(int i = 0; i < 3; i++) {
                GPIO_SetBits(LED_PORT, LED_PIN);
                Delay_ms(200);
                GPIO_ResetBits(LED_PORT, LED_PIN);
                Delay_ms(200);
            }
        }
    }
}

void RCC_Configuration(void)
{

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | 
                           RCC_APB2Periph_AFIO, ENABLE);

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
}

void GPIO_Configuration(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    GPIO_InitStructure.GPIO_Pin = LED_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = BUTTON_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(BUTTON_PORT, &GPIO_InitStructure);
}

void EXTI_Configuration(void)
{
    EXTI_InitTypeDef EXTI_InitStructure;
    
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
    
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling; // Kích hoạt khi nút được nhấn (mức thấp)
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);
    
    EXTI_ClearITPendingBit(EXTI_Line0);
}

void NVIC_Configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
    
    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

void PWR_Configuration(void)
{
    PWR_BackupAccessCmd(ENABLE);
    
    PWR_PVDCmd(DISABLE);
}

void EnterStopMode(void)
{
    PWR_EnterSTOPMode(PWR_Regulator_ON, PWR_STOPEntry_WFI);
    
    SystemInit();
}

void Delay_ms(uint32_t ms)
{
    volatile uint32_t delay = ms * 10000;
    while(delay--);
}

void EXTI0_IRQHandler(void)
{
    // Kiểm tra nếu có ngắt từ EXTI0
    if(EXTI_GetITStatus(EXTI_Line0) != RESET) {
        // Đánh dấu cờ đánh thức
        wakeup_flag = 1;
        
        // Xóa cờ ngắt
        EXTI_ClearITPendingBit(EXTI_Line0);
    }
}