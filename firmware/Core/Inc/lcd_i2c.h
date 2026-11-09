#ifndef LCD16X2_H
#define LCD16X2_H

#include <stdint.h>
#include "status.h"

/* Goi sau MX_I2C1_Init(). */
void LCD_Init(void);

/* Goi moi vong lap. STOP va RUNNING cung mot man hinh. */
void LCD_Update(SystemState_t state,
                SystemMode_t  mode,
                ErrorCode_t   error,
                uint32_t      total,
                uint32_t      countA,
                uint32_t      countB);

#endif