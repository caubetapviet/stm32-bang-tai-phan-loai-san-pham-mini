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

typedef struct
{
    uint8_t cb_thap_active;   /* 1U: CB_Thap dang LOW (phat hien vat) */
    uint8_t cb_cao_active;    /* 1U: CB_Cao dang LOW (phat hien vat) */
} SensorState_t;

/* Dua module ve SENSOR_STOPPED */
void Sensor_Init(void);

/* Bat dau nhan dien, vao SENSOR_READY ngay (ke ca cam bien dang LOW) */
void Sensor_Start(void);

/* Dung nhan dien, huy luot dang cho T1 */
void Sensor_Stop(void);

/* Goi moi vong lap. Doc cam bien, cap nhat state machine, tra ve su kien */
SensorEvent_t Sensor_Update(void);

/* Lay muc hai cam bien o lan Sensor_Update() gan nhat (khong tu doc GPIO) */
void Sensor_GetState(SensorState_t *state);

#endif
