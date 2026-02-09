#include "lcd_i2c.h"
#include <string.h>
#include <stdio.h>

/* PCF8574 bit mapping */
#define LCD_RS 0x01
#define LCD_EN 0x04
#define LCD_BL 0x08

typedef enum
{
    LCD_IDLE = 0,
    LCD_LINE1,
    LCD_LINE2
} LCD_State_t;

typedef struct
{
    uint32_t last_tick;
    uint8_t  dirty;
    LCD_State_t state;
} LCD_Task_t;

static I2C_HandleTypeDef *lcd_i2c;
static LCD_Task_t lcd_task;

/* LCD buffers */
static char lcd_line1[17] = "Wave Player    ";
static char lcd_line2[17] = "Ready          ";

/* ---------- LOW LEVEL ---------- */

static void LCD_Write(uint8_t data)
{
    HAL_I2C_Master_Transmit(lcd_i2c, LCD_I2C_ADDR, &data, 1, HAL_MAX_DELAY);
}

static void LCD_Pulse(uint8_t data)
{
    LCD_Write(data | LCD_EN);
    LCD_Write(data & ~LCD_EN);
}

static void LCD_Write4Bits(uint8_t nibble, uint8_t rs)
{
    uint8_t data = nibble | LCD_BL;
    if (rs) data |= LCD_RS;

    LCD_Write(data);
    LCD_Pulse(data);
}

static void LCD_Send(uint8_t value, uint8_t rs)
{
    LCD_Write4Bits(value & 0xF0, rs);
    LCD_Write4Bits((value << 4) & 0xF0, rs);
}

static void LCD_Command(uint8_t cmd)
{
    LCD_Send(cmd, 0);
}

static void LCD_Data(uint8_t data)
{
    LCD_Send(data, 1);
}

/* ---------- INIT ---------- */

void LCD_Init(I2C_HandleTypeDef *hi2c)
{
    lcd_i2c = hi2c;
    HAL_Delay(50);

    LCD_Write4Bits(0x30, 0);
    HAL_Delay(5);
    LCD_Write4Bits(0x30, 0);
    HAL_Delay(1);
    LCD_Write4Bits(0x30, 0);
    HAL_Delay(1);
    LCD_Write4Bits(0x20, 0);   // 4-bit mode

    LCD_Command(0x28); // 2 line, 5x8
    LCD_Command(0x08); // display OFF
    LCD_Command(0x01); // clear
    HAL_Delay(2);
    LCD_Command(0x06); // entry mode
    LCD_Command(0x0C); // display ON
}

/* ---------- NON-BLOCKING TASK ---------- */

void LCD_TaskInit(void)
{
    lcd_task.state = LCD_IDLE;
    lcd_task.last_tick = 0;
    lcd_task.dirty = 1;
}

void LCD_Task(void)
{
    if (!lcd_task.dirty)
        return;

    if (HAL_GetTick() - lcd_task.last_tick < 5)
        return;

    lcd_task.last_tick = HAL_GetTick();

    switch (lcd_task.state)
    {
        case LCD_IDLE:
            lcd_task.state = LCD_LINE1;
            break;

        case LCD_LINE1:
            LCD_Command(0x80);   // line 1
            for (int i = 0; i < 16; i++)
                LCD_Data(lcd_line1[i]);
            lcd_task.state = LCD_LINE2;
            break;

        case LCD_LINE2:
            LCD_Command(0xC0);   // line 2
            for (int i = 0; i < 16; i++)
                LCD_Data(lcd_line2[i]);
            lcd_task.state = LCD_IDLE;
            lcd_task.dirty = 0;
            break;
    }
}

/* ---------- UPDATE API ---------- */

void LCD_Refresh(void)
{
    lcd_task.dirty = 1;
}

void LCD_SetLine1(char *text)
{
    snprintf(lcd_line1, 17, "%-16s", text);
    LCD_Refresh();
}

void LCD_SetLine2(char *text)
{
    snprintf(lcd_line2, 17, "%-16s", text);
    LCD_Refresh();
}
