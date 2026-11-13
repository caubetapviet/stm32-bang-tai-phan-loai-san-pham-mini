
#ifndef BUTTONS_H
#define BUTTONS_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef enum
{
    BUTTON_EVENT_NONE = 0U,
    BUTTON_EVENT_START,
    BUTTON_EVENT_STOP,
    BUTTON_EVENT_MODE,
    BUTTON_EVENT_RESET,
    BUTTON_EVENT_CONVEYOR,
    BUTTON_EVENT_SERVO
} ButtonEvent_t;

// khoi tao module
void Buttons_Init(void);

// goi lien tuc moi vong lap, tra ve toi da 1 su kien nut nhan 1 luc
ButtonEvent_t Buttons_Update(void);

#endif
