#ifndef IOT_H
#define IOT_H

#include <stdint.h>
#include "status.h"

/* Goi sau MX_USART1_UART_Init() va Timer_Init() */
void IoT_Init(void);

/* Goi moi vong lap. Gui 1 dong van ban moi IOT_SEND_PERIOD_MS */
void IoT_Update(SystemState_t state,
                SystemMode_t  mode,
                ErrorCode_t   error,
                uint32_t      total,
                uint32_t      countA,
                uint32_t      countB);

#endif