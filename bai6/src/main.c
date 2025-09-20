#include "stm32f10x.h"
#include <stdio.h>


#define BMP280_ADDR              0x76


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

// Cấu trúc chứa các hệ số hiệu chỉnh nhiệt độ
typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;
} BMP280_CalibData;

BMP280_CalibData calib_data;
int32_t t_fine; 

void I2C_Config(void);
void GPIO_Config(void);
void UART_Config(void);
void UART_SendChar(char c);
void UART_SendString(char *str);
void I2C_ScanAddresses(void);
uint8_t I2C_CheckAddress(uint8_t address);
void delay_ms(uint32_t ms);

// Hàm mới cho BMP280
uint8_t BMP280_ReadReg(uint8_t reg);
void BMP280_WriteReg(uint8_t reg, uint8_t value);
uint16_t BMP280_ReadReg16(uint8_t reg);
void BMP280_Init(void);
void BMP280_ReadCalibData(void);
int32_t BMP280_ReadTemperature(void);
float BMP280_CompensateTemperature(int32_t adc_T);

char buffer[50]; // Buffer for string formatting

int main(void)
{
    GPIO_Config();
    I2C_Config();
    UART_Config();
    delay_ms(100);

    UART_SendString("Scanning I2C addresses...\r\n");
    I2C_ScanAddresses();

    BMP280_Init();

    BMP280_ReadCalibData();
    
    UART_SendString("BMP280 Initialized! Reading temperature...\r\n");
    
    while(1)
    {
        // Đọc nhiệt độ từ BMP280
        int32_t raw_temp = BMP280_ReadTemperature();
        float temp = BMP280_CompensateTemperature(raw_temp);
        
        // Hiển thị nhiệt độ qua UART
        sprintf(buffer, "Temperature: %.2f °C\r\n", temp);
        UART_SendString(buffer);
        
        delay_ms(1000); // Đọc mỗi giây
    }
}

void GPIO_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void I2C_Config(void)
{
    I2C_InitTypeDef I2C_InitStructure;
    
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE);
    
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, ENABLE);
    RCC_APB1PeriphResetCmd(RCC_APB1Periph_I2C1, DISABLE);
    
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00; 
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_InitStructure.I2C_ClockSpeed = 100000; 

    I2C_Init(I2C1, &I2C_InitStructure);
    I2C_Cmd(I2C1, ENABLE);
}

void UART_Config(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
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

uint8_t I2C_CheckAddress(uint8_t address)
{
    uint32_t timeout = 10000;
    uint8_t success = 0;
    
    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) && timeout)
    {
        timeout--;
    }
    
    if (timeout == 0) return 0; 
    
    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout)
    {
        timeout--;
    }
    
    if (timeout == 0) {
        I2C_GenerateSTOP(I2C1, ENABLE);
        return 0; 
    }

    I2C_Send7bitAddress(I2C1, address << 1, I2C_Direction_Transmitter);

    timeout = 10000;
    while(timeout--)
    {
        if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED))
        {
            success = 1;
            break;
        }

        if(I2C_GetFlagStatus(I2C1, I2C_FLAG_AF))
        {
            I2C_ClearFlag(I2C1, I2C_FLAG_AF);
            break;
        }
    }
    I2C_GenerateSTOP(I2C1, ENABLE);
    delay_ms(1); 
    
    return success;
}

void I2C_ScanAddresses(void)
{
    uint8_t address;
    uint8_t foundDevices = 0;

    for(address = 1; address < 128; address++)
    {
        if(I2C_CheckAddress(address))
        {
            sprintf(buffer, "Device found at address: 0x%02X (8-bit: 0x%02X)\r\n", address, address << 1);
            UART_SendString(buffer);
            
            if(address == 0x76 || address == 0x77) {
                UART_SendString("  This could be a BMP280 sensor!\r\n");
            }
            
            foundDevices++;
        }
    }
    
    if(foundDevices == 0)
    {
        UART_SendString("No I2C devices found. Check connections.\r\n");
    }
}


uint8_t BMP280_ReadReg(uint8_t reg)
{
    uint8_t value;
    uint32_t timeout = 10000;
    

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) && timeout--);
    

    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout--);
    

    I2C_Send7bitAddress(I2C1, BMP280_ADDR << 1, I2C_Direction_Transmitter);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout--);
    

    I2C_SendData(I2C1, reg);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout--);
    

    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout--);
    

    I2C_Send7bitAddress(I2C1, BMP280_ADDR << 1, I2C_Direction_Receiver);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) && timeout--);
    

    I2C_AcknowledgeConfig(I2C1, DISABLE);
    

    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) && timeout--);
    

    value = I2C_ReceiveData(I2C1);
    

    I2C_GenerateSTOP(I2C1, ENABLE);
    
    I2C_AcknowledgeConfig(I2C1, ENABLE);
    
    return value;
}


void BMP280_WriteReg(uint8_t reg, uint8_t value)
{
    uint32_t timeout = 10000;
    

    while(I2C_GetFlagStatus(I2C1, I2C_FLAG_BUSY) && timeout--);
    

    I2C_GenerateSTART(I2C1, ENABLE);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) && timeout--);
    

    I2C_Send7bitAddress(I2C1, BMP280_ADDR << 1, I2C_Direction_Transmitter);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) && timeout--);

    I2C_SendData(I2C1, reg);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout--);
    

    I2C_SendData(I2C1, value);
    timeout = 10000;
    while(!I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) && timeout--);
    

    I2C_GenerateSTOP(I2C1, ENABLE);
}

uint16_t BMP280_ReadReg16(uint8_t reg)
{
    uint16_t lsb = BMP280_ReadReg(reg);
    uint16_t msb = BMP280_ReadReg(reg + 1);
    
    return (msb << 8) | lsb;
}


void BMP280_Init(void)
{

    uint8_t id = BMP280_ReadReg(BMP280_REG_ID);
    
    if(id == 0x58) {
        UART_SendString("BMP280 detected!\r\n");
    } else {
        sprintf(buffer, "Unknown device ID: 0x%02X. Expected 0x58 for BMP280\r\n", id);
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

// Đọc giá trị nhiệt độ thô
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

// Bù trừ nhiệt độ theo công thức từ datasheet BMP280
float BMP280_CompensateTemperature(int32_t adc_T)
{
    int32_t var1, var2, T;
    
    var1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1))) * ((int32_t)calib_data.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) * ((int32_t)calib_data.dig_T3)) >> 14;
    
    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    
    return (float)T / 100.0f;
}