#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "stm32f4xx_hal.h"   // change if needed

#define LCD_I2C_ADDR (0x27 << 1)

/* Public API */
void LCD_Init(I2C_HandleTypeDef *hi2c);
void LCD_TaskInit(void);
void LCD_Task(void);
void LCD_Refresh(void);

void LCD_SetLine1(char *text);
void LCD_SetLine2(char *text);

#endif
