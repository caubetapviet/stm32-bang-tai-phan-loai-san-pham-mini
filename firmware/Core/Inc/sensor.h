#ifndef SENSOR_H
#define SENSOR_H

#include "stm32f1xx_hal.h"
#include <stdint.h>

typedef enum
{
    SENSOR_EVENT_NONE = 0,
    SENSOR_EVENT_PRODUCT_A,
    SENSOR_EVENT_PRODUCT_B
} SensorEvent_t;

/* Goi moi vong lap. enable = 1U khi STATE_RUNNING, 0U thi huy luot dang cho */
SensorEvent_t Sensor_Update(uint8_t enable);

/* 1U neu it nhat mot cam bien dang LOW (doc GPIO truc tiep) */
uint8_t Sensor_IsActive(void);

#endif