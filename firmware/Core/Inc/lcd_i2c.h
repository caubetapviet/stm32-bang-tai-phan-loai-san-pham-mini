#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "main.h"

/* LCD configuration */
#define LCD_I2C_ADDR       (0x20 << 1)
#define LCD_COLS           16
#define LCD_ROWS           2

/* Public functions */
void LCD_Init(I2C_HandleTypeDef *hi2c);
void LCD_Clear(void);
void LCD_SetCursor(uint8_t row, uint8_t col);
void LCD_SendString(char *str);
void LCD_SendChar(char data);

#endif