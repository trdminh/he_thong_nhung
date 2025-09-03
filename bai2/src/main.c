#include "stm32f10x.h"

void TIM2_Init(void);
void GPIO_Config(void);
void delay1ms(void);
void delay_ms(unsigned int time_ms);
void task1(void);
void task2(void);
uint8_t led_state;


typedef struct {
	uint8_t old_state_btn;
	uint8_t new_state_btn;
} State_BTN;

State_BTN btn;
int main(){
	GPIO_Config();
	TIM2_Init();
	while(1)
	{
		task2();
	}
}


void TIM2_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	
	TIM_TimeBaseInitTypeDef timer_init;
	
	timer_init.TIM_CounterMode = TIM_CounterMode_Up;
	timer_init.TIM_Period = 0xffff;
	timer_init.TIM_Prescaler = 72 - 1;
	 
	TIM_TimeBaseInit(TIM2,&timer_init);
	TIM_Cmd(TIM2,ENABLE);
	
	
}

void GPIO_Config(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	GPIO_InitTypeDef led_init, btn_init;
	btn_init.GPIO_Mode = GPIO_Mode_IPU;
	btn_init.GPIO_Pin = GPIO_Pin_5;
	btn_init.GPIO_Speed = GPIO_Speed_50MHz;
	led_init.GPIO_Mode = GPIO_Mode_Out_PP;
	led_init.GPIO_Pin = GPIO_Pin_13;
	led_init.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(GPIOC,&led_init);
	GPIO_Init(GPIOA,&btn_init);
	btn.old_state_btn = 0;
	btn.new_state_btn = 0;
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
/*


*/

void task1(void)
{
		GPIO_ResetBits(GPIOC,GPIO_Pin_13);
		delay_ms(1000);
		GPIO_SetBits(GPIOC, GPIO_Pin_13);
		delay_ms(1000);	
}

void task2(void)
{
	led_state = GPIO_ReadOutputDataBit(GPIOC,GPIO_Pin_13);
	btn.old_state_btn = btn.new_state_btn;
	btn.new_state_btn = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_5);
	if (btn.new_state_btn != btn.old_state_btn && btn.new_state_btn == 1)
	{
		GPIO_WriteBit(GPIOC, GPIO_Pin_13, (led_state == Bit_SET) ? Bit_RESET : Bit_SET);
	}
}

