#ifndef STATUS_H
#define STATUS_H

#include <stdint.h>

typedef enum
{
    STATE_STOP = 0,
    STATE_RUNNING,
    STATE_ERROR
} SystemState_t;

typedef enum
{
    MODE_AUTO = 0,
    MODE_MANUAL
} SystemMode_t;

typedef enum
{
    ERROR_NONE = 0,
    ERROR_TIMEOUT,
    ERROR_CONTINUOUSLY_ACTIVE
} ErrorCode_t;

/* Dat STATE_STOP, MODE_AUTO, ERROR_NONE. Goi sau Timer_Init() */
void Status_Init(void);

SystemState_t Status_GetState(void);
SystemMode_t  Status_GetMode(void);
ErrorCode_t   Status_GetError(void);

/* Cac ham duoi day tra 1U neu yeu cau hop le va da doi, 0U neu bi bo qua */
uint8_t Status_Start(void);        /* STOP -> RUNNING */
uint8_t Status_Stop(void);         /* RUNNING -> STOP */
uint8_t Status_ToggleMode(void);   /* chi doi duoc o STOP */
uint8_t Status_ClearError(void);   /* ERROR -> STOP */

/* Goi moi vong lap. Tra 1U dung mot lan khi vua vao STATE_ERROR */
uint8_t Status_Update(uint8_t conveyorRunning,
                      uint8_t sensorActive,
                      uint8_t classified);

#endif