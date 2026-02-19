#include "Vl6180x_i2c.h"
#include "i2c.h"
#include <stdio.h>
#include <stdbool.h>

// 声明外部变量，方便在其他地方查看状态
bool sensor_online = false;

// --- 1. 基础读写封装 ---
void VL6180X_Write8(uint16_t reg, uint8_t data) {
    HAL_I2C_Mem_Write(&hi2c1, VL6180X_ADDR, reg, I2C_MEMADD_SIZE_16BIT, &data, 1, 100);
}

uint8_t VL6180X_Read8(uint16_t reg) {
    uint8_t data = 0;
    HAL_I2C_Mem_Read(&hi2c1, VL6180X_ADDR, reg, I2C_MEMADD_SIZE_16BIT, &data, 1, 100);
    return data;
}

// ---  加载设置 
void VL6180X_Load_Settings(void) {
    // 官方要求的私有寄存器初始化
    VL6180X_Write8(0x0207, 0x01); VL6180X_Write8(0x0208, 0x01);
    VL6180X_Write8(0x0096, 0x00); VL6180X_Write8(0x0097, 0xfd);
    VL6180X_Write8(0x00e3, 0x00); VL6180X_Write8(0x00e4, 0x04);
    VL6180X_Write8(0x00e5, 0x02); VL6180X_Write8(0x00e6, 0x01);
    VL6180X_Write8(0x00e7, 0x03); VL6180X_Write8(0x00f5, 0x02);
    VL6180X_Write8(0x00d9, 0x05); VL6180X_Write8(0x00db, 0xce);
    VL6180X_Write8(0x00dc, 0x03); VL6180X_Write8(0x00dd, 0xf8);
    VL6180X_Write8(0x009f, 0x00); VL6180X_Write8(0x00a3, 0x3c);
    VL6180X_Write8(0x00b7, 0x00); VL6180X_Write8(0x00bb, 0x3c);
    VL6180X_Write8(0x00b2, 0x09); VL6180X_Write8(0x00ca, 0x09);
    VL6180X_Write8(0x0198, 0x01); VL6180X_Write8(0x01b0, 0x17);
    VL6180X_Write8(0x01ad, 0x00); VL6180X_Write8(0x00ff, 0x05);
    VL6180X_Write8(0x0100, 0x05); VL6180X_Write8(0x0199, 0x05);
    VL6180X_Write8(0x01a6, 0x1b); VL6180X_Write8(0x01ac, 0x3e);
    VL6180X_Write8(0x01a7, 0x1f); VL6180X_Write8(0x0030, 0x00);

    // 专项修复与配置
    VL6180X_Write8(0x00f2, 0x01); // 禁用 VCSEL 测试
    VL6180X_Write8(0x0022, 0x00);
    VL6180X_Write8(0x0014, 0x04); // 配置中断
    VL6180X_Write8(0x0016, 0x00); // 退出复位
    VL6180X_Write8(0x001B, 0x32); // 增加收敛时间
    VL6180X_Write8(0x003F, 0x46); // 环境光校准
    
    VL6180X_Write8(0x024, 0xE3);
    VL6180X_Write8(0x002e, 0x01); // 负载校准
    VL6180X_Write8(0x0015, 0x07); // 清除中断
}

// --- 3. 初始化函数 (STM32版) ---
HAL_StatusTypeDef VL6180X_Init(void) {
    uint8_t id = 0;
    
    // 1. 尝试读取 ID
    // 直接用 HAL 函数判断通信是否正常
    if (HAL_I2C_Mem_Read(&hi2c1, VL6180X_ADDR, REG_IDENT_MODEL_ID, 
                         I2C_MEMADD_SIZE_16BIT, &id, 1, 100) != HAL_OK) {
        sensor_online = false;
        return HAL_ERROR;
    }

    if (id != 0xB4) {
        printf("Sensor Not Found! ID: 0x%02X\r\n", id);
        sensor_online = false;
        return HAL_ERROR;
    }

    // 2. 加载设置
    VL6180X_Load_Settings();

    sensor_online = true;
    printf("VL6180X Init OK! ID: 0x%02X\r\n", id);
    return HAL_OK;
}


uint16_t VL6180X_Read_Distance(void) {
    uint8_t status;
    uint8_t distance;
    uint32_t counter = 0;

    // 1. 启动单次测量
    VL6180X_Write8(REG_SYSRANGE_START, 0x01);

    // 2. 等待测量完成 (轮询状态寄存器)
    do {
        status = VL6180X_Read8(REG_RESULT_INTERRUPT_STATUS);
        counter++;
        HAL_Delay(1);
        if(counter > 1000) return 999; // 简单超时处理
    } while ((status & 0x07) == 0); // 等待中断标志位

    // 3. 读取结果
    distance = VL6180X_Read8(REG_RESULT_RANGE_VAL);

    // 4. 清除中断，准备下一次测量
    VL6180X_Write8(REG_SYSTEM_INTERRUPT_CLEAR, 0x07);

    return (uint16_t)distance;
}