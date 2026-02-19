#ifndef __VL6180X_I2C_H
#define __VL6180X_I2C_H

#include "main.h"


// 设备地址
#define VL6180X_ADDR                (0x29 << 1)

// --- 核心寄存器地址 ---
#define REG_IDENT_MODEL_ID          0x000
#define REG_SYSTEM_INTERRUPT_CONFIG 0x014
#define REG_SYSTEM_INTERRUPT_CLEAR  0x015
#define REG_SYSTEM_FRESH_OUT_OF_RESET 0x016
#define REG_SYSRANGE_START          0x018  // <--- 解决报错的关键
#define REG_SYSALS_START            0x038
#define REG_RESULT_INTERRUPT_STATUS 0x04D
#define REG_RESULT_RANGE_VAL        0x062  // 距离结果存储在这里
#define REG_RESULT_ALS_VAL          0x050

// 函数原型
void VL6180X_Write8(uint16_t reg, uint8_t data);
uint8_t VL6180X_Read8(uint16_t reg);
HAL_StatusTypeDef VL6180X_Init(void);
uint16_t VL6180X_Read_Distance(void);

#endif