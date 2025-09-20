#include "stm32f10x.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_spi.h"
#include <stdio.h>

#define BMP280_CS_PORT    GPIOA
#define BMP280_CS_PIN     GPIO_Pin_4


#define BMP280_REG_ID            0xD0
#define BMP280_REG_RESET         0xE0
#define BMP280_REG_STATUS        0xF3
#define BMP280_REG_CTRL_MEAS     0xF4
#define BMP280_REG_CONFIG        0xF5
#define BMP280_REG_PRESS_MSB     0xF7
#define BMP280_REG_PRESS_LSB     0xF8
#define BMP280_REG_PRESS_XLSB    0xF9
#define BMP280_REG_TEMP_MSB      0xFA
#define BMP280_REG_TEMP_LSB      0xFB
#define BMP280_REG_TEMP_XLSB     0xFC


#define BMP280_REG_DIG_T1        0x88
#define BMP280_REG_DIG_T2        0x8A
#define BMP280_REG_DIG_T3        0x8C


#define BMP280_SPI_READ          0x80
#define BMP280_SPI_WRITE         0x7F


typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
} BMP280_CalibData;

BMP280_CalibData calib_data;
int32_t t_fine; 

void GPIO_Config(void);
void SPI_Config(void);
void UART_Config(void);
void UART_SendChar(char c);
void UART_SendString(char *str);
void delay_ms(uint32_t ms);

void BMP280_CS_Low(void);
void BMP280_CS_High(void);
uint8_t SPI_Transfer(uint8_t data);

uint8_t BMP280_ReadReg(uint8_t reg);
void BMP280_WriteReg(uint8_t reg, uint8_t value);
uint16_t BMP280_ReadReg16(uint8_t reg);
void BMP280_Init(void);
void BMP280_ReadCalibData(void);
int32_t BMP280_ReadTemperature(void);
float BMP280_CompensateTemperature(int32_t adc_T);

char buffer[50]; 
int main(void)
{
    GPIO_Config();
    SPI_Config();
    UART_Config();
    
    delay_ms(100);
    
    

    BMP280_Init();
    

    BMP280_ReadCalibData();
    
    UART_SendString("BMP280 Initialized! Reading temperature...\r\n");
    
    while(1)
    {
        int32_t raw_temp = BMP280_ReadTemperature();
        float temp = BMP280_CompensateTemperature(raw_temp);

        sprintf(buffer, "Temperature: %.2f °C\r\n", temp);
        UART_SendString(buffer);
        
        delay_ms(1000);
    }
}

void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    

    GPIO_InitStructure.GPIO_Pin = BMP280_CS_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(BMP280_CS_PORT, &GPIO_InitStructure);

    BMP280_CS_High();
}

void SPI_Config(void)
{
    SPI_InitTypeDef SPI_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
    SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
    SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;    
    SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;  
    SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;      
    SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_32; 
    SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
    SPI_InitStructure.SPI_CRCPolynomial = 7;
    
    SPI_Init(SPI1, &SPI_InitStructure);

    SPI_Cmd(SPI1, ENABLE);
}

void UART_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
    

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // Alternate function, Push-pull
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; // Input, floating
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    
    USART_Init(USART1, &USART_InitStructure);
    

    USART_Cmd(USART1, ENABLE);
}

void UART_SendChar(char c)
{
    while(USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);

    USART_SendData(USART1, c);
}

void UART_SendString(char *str)
{
    while(*str)
    {
        UART_SendChar(*str++);
    }
}

void delay_ms(uint32_t ms)
{
    volatile uint32_t i;
    for(; ms > 0; ms--)
    {
        for(i = 0; i < 12000; i++);
    }
}

void BMP280_CS_Low(void)
{
    GPIO_ResetBits(BMP280_CS_PORT, BMP280_CS_PIN); 
}

void BMP280_CS_High(void)
{
    GPIO_SetBits(BMP280_CS_PORT, BMP280_CS_PIN);
}

uint8_t SPI_Transfer(uint8_t data)
{

    SPI_I2S_SendData(SPI1, data);
    

    while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);

    return SPI_I2S_ReceiveData(SPI1);
}

uint8_t BMP280_ReadReg(uint8_t reg)
{
    uint8_t value;
    
    BMP280_CS_Low(); 
    
    SPI_Transfer(reg | BMP280_SPI_READ); 
    value = SPI_Transfer(0x00); 
    
    BMP280_CS_High();
    
    return value;
}

void BMP280_WriteReg(uint8_t reg, uint8_t value)
{
    BMP280_CS_Low(); 
    
    SPI_Transfer(reg & BMP280_SPI_WRITE); 
    SPI_Transfer(value); 
    
    BMP280_CS_High();
}

uint16_t BMP280_ReadReg16(uint8_t reg)
{
    uint16_t lsb, msb;
    
    BMP280_CS_Low();
    
    SPI_Transfer(reg | BMP280_SPI_READ); 
    lsb = SPI_Transfer(0x00); 
    msb = SPI_Transfer(0x00); 
    
    BMP280_CS_High(); 
    
    return (msb << 8) | lsb;
}

void BMP280_Init(void)
{
    uint8_t id = BMP280_ReadReg(BMP280_REG_ID);
    
    if(id == 0x58) {
        sprintf(buffer, "BMP280 detected!", id);
        UART_SendString(buffer);
    } else {
        sprintf(buffer, "Unknown device ID: 0x%02X. Expected 0x58\r\n", id);
        UART_SendString(buffer);
    }
    
    BMP280_WriteReg(BMP280_REG_RESET, 0xB6);
    delay_ms(10); 
    
    // Cấu hình thanh ghi control (0xF4): 
    // - Temperature oversampling x1 (bits [7:5] = 001)
    // - Normal mode (bits [1:0] = 11)
    BMP280_WriteReg(BMP280_REG_CTRL_MEAS, 0b00100111);
    
    // Cấu hình thanh ghi config (0xF5):
    // - Standby time 500ms (bits [7:5] = 100)
    // - IIR filter off (bits [4:2] = 000)
    BMP280_WriteReg(BMP280_REG_CONFIG, 0b10000000);
    
    UART_SendString("BMP280 configuration complete.\r\n");
}

void BMP280_ReadCalibData(void)
{
    calib_data.dig_T1 = BMP280_ReadReg16(BMP280_REG_DIG_T1);
    calib_data.dig_T2 = (int16_t)BMP280_ReadReg16(BMP280_REG_DIG_T2);
    calib_data.dig_T3 = (int16_t)BMP280_ReadReg16(BMP280_REG_DIG_T3);
    
    sprintf(buffer, "Calibration data: T1=%u, T2=%d, T3=%d\r\n", 
            calib_data.dig_T1, calib_data.dig_T2, calib_data.dig_T3);
    UART_SendString(buffer);
}

int32_t BMP280_ReadTemperature(void)
{
    uint8_t msb, lsb, xlsb;
    int32_t adc_T;
    
    msb = BMP280_ReadReg(BMP280_REG_TEMP_MSB);
    lsb = BMP280_ReadReg(BMP280_REG_TEMP_LSB);
    xlsb = BMP280_ReadReg(BMP280_REG_TEMP_XLSB);

    adc_T = ((uint32_t)msb << 12) | ((uint32_t)lsb << 4) | ((uint32_t)xlsb >> 4);
    
    return adc_T;
}

float BMP280_CompensateTemperature(int32_t adc_T)
{
    int32_t var1, var2, T;

    var1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1))) * ((int32_t)calib_data.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) * ((int32_t)calib_data.dig_T3)) >> 14;
    
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    
    return (float)T / 100.0f; 
}