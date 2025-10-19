#include "stm32f10x.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define STACK_SIZE 128
#define PRIORITY 1

void Led_Init(void);
void UART_Init(void);
void UART_SendString(char *str);

// Task functions for independent tasks
void vTaskLED1(void *pvParameters); // 3Hz
void vTaskLED2(void *pvParameters); // 10Hz
void vTaskLED3(void *pvParameters); // 25Hz


int main(void){
	
	RCC_DeInit(); // Reset RCC to default state
	RCC_HSEConfig(RCC_HSE_OFF); // Disable external oscillator
	RCC_HSICmd(ENABLE); // Use internal oscillator
	while(RCC_GetFlagStatus(RCC_FLAG_HSIRDY) == RESET); // Wait for HSI ready
	
	Led_Init();
	UART_Init();
	UART_SendString("Start \r\n");
	
	// Part 1: Three independent tasks
	if (xTaskCreate(vTaskLED1, "LED1", STACK_SIZE, NULL, PRIORITY, NULL) != pdPASS) {
		UART_SendString("Failed to create LED1 task\r\n");
	}
	if (xTaskCreate(vTaskLED2, "LED2", STACK_SIZE, NULL, PRIORITY, NULL) != pdPASS) {
		UART_SendString("Failed to create LED2 task\r\n");
	}
	if (xTaskCreate(vTaskLED3, "LED3", STACK_SIZE, NULL, PRIORITY, NULL) != pdPASS) {
		UART_SendString("Failed to create LED3 task\r\n");
	}

	
	vTaskStartScheduler();
}

void Led_Init(void){
	
	GPIO_InitTypeDef led;
	led.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	led.GPIO_Mode = GPIO_Mode_Out_PP;
	led.GPIO_Speed = GPIO_Speed_50MHz;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	GPIO_Init(GPIOC, &led);
}

void UART_Init(void) {
    GPIO_InitTypeDef gpio;
    USART_InitTypeDef usart;
    
    // Enable clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    
    // Configure PA9 (TX) and PA10 (RX)
    gpio.GPIO_Pin = GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);
    
    gpio.GPIO_Pin = GPIO_Pin_10;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);
    
    // Configure USART1
    usart.USART_BaudRate = 115200;
    usart.USART_WordLength = USART_WordLength_8b;
    usart.USART_StopBits = USART_StopBits_1;
    usart.USART_Parity = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode = USART_Mode_Tx;
    USART_Init(USART1, &usart);
    
    USART_Cmd(USART1, ENABLE);
}

void UART_SendString(char *str) {
    while (*str) {
        while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
        USART_SendData(USART1, *str++);
    }
}

// Part 1: Independent task functions
void vTaskLED1(void *pvParameters) {
    while (1) {
        GPIO_SetBits(GPIOC, GPIO_Pin_13);
        vTaskDelay(pdMS_TO_TICKS(166)); // 166ms on for 3Hz
        GPIO_ResetBits(GPIOC, GPIO_Pin_13);
        vTaskDelay(pdMS_TO_TICKS(166)); // 166ms off
    }
}

void vTaskLED2(void *pvParameters) {
    while (1) {
        GPIO_SetBits(GPIOC, GPIO_Pin_14);
        vTaskDelay(pdMS_TO_TICKS(50)); // 50ms on for 10Hz
        GPIO_ResetBits(GPIOC, GPIO_Pin_14);
        vTaskDelay(pdMS_TO_TICKS(50)); // 50ms off
    }
}

void vTaskLED3(void *pvParameters) {
    while (1) {
        GPIO_SetBits(GPIOC, GPIO_Pin_15);
        vTaskDelay(pdMS_TO_TICKS(20)); // 20ms on for 25Hz
        GPIO_ResetBits(GPIOC, GPIO_Pin_15);
        vTaskDelay(pdMS_TO_TICKS(20)); // 20ms off
    }
}
