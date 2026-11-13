#include "status.h"
#include "timer.h"

/* Tham so cau hinh (ms), chinh khi thu nghiem */
static const uint32_t timeout_limit       = 30000U;
static const uint32_t sensor_active_limit = 2000U;   /* T2, phai lon hon T1 va thoi gian san pham di qua */

static SystemState_t g_state      = STATE_STOP;
static SystemMode_t  g_mode       = MODE_AUTO;
static ErrorCode_t   g_error_code = ERROR_NONE;

static uint8_t  was_conveyor_running    = 0U;
static uint32_t timeout_start_tick      = 0U;
static uint8_t  sensor_active_tracking  = 0U;
static uint32_t sensor_active_start_tick = 0U;

/* Huy cac moc thoi gian dang tinh */
static void ClearTimers(void)
{
    was_conveyor_running   = 0U;
    sensor_active_tracking = 0U;
}

void Status_Init(void)
{
    g_state      = STATE_STOP;
    g_mode       = MODE_AUTO;
    g_error_code = ERROR_NONE;
    ClearTimers();
}

SystemState_t Status_GetState(void) { return g_state; }
SystemMode_t  Status_GetMode(void)  { return g_mode; }
ErrorCode_t   Status_GetError(void) { return g_error_code; }

uint8_t Status_Start(void)
{
    if (g_state != STATE_STOP) { return 0U; }

    g_state = STATE_RUNNING;
    ClearTimers();
    return 1U;
}

uint8_t Status_Stop(void)
{
    if (g_state != STATE_RUNNING) { return 0U; }

    g_state = STATE_STOP;
    ClearTimers();
    return 1U;
}

uint8_t Status_ToggleMode(void)
{
    if (g_state != STATE_STOP) { return 0U; }

    g_mode = (g_mode == MODE_AUTO) ? MODE_MANUAL : MODE_AUTO;
    return 1U;
}

uint8_t Status_ClearError(void)
{
    if (g_state != STATE_ERROR) { return 0U; }

    g_state      = STATE_STOP;
    g_error_code = ERROR_NONE;
    ClearTimers();
    return 1U;
}

uint8_t Status_Update(uint8_t conveyorRunning,
                      uint8_t sensorActive,
                      uint8_t classified)
{
    ErrorCode_t error = ERROR_NONE;

    /* Chi kiem tra khi RUNNING va bang tai dang chay */
    if ((g_state != STATE_RUNNING) || (conveyorRunning == 0U))
    {
        ClearTimers();
        return 0U;
    }

    /* Bang tai vua chay: bat dau tinh timeout */
    if (was_conveyor_running == 0U)
    {
        timeout_start_tick   = Timer_GetTick();
        was_conveyor_running = 1U;
    }

    /* Phan loai thanh cong: tinh lai timeout (truoc khi kiem tra) */
    if (classified != 0U)
    {
        timeout_start_tick = Timer_GetTick();
    }

    /* Thoi gian cam bien kich hoat lien tuc */
    if (sensorActive != 0U)
    {
        if (sensor_active_tracking == 0U)
        {
            sensor_active_tracking   = 1U;
            sensor_active_start_tick = Timer_GetTick();
        }
    }
    else
    {
        sensor_active_tracking = 0U;
    }

    /* Uu tien ERROR_CONTINUOUSLY_ACTIVE */
    if ((sensor_active_tracking != 0U) &&
        (Timer_IsElapsed(sensor_active_start_tick, sensor_active_limit) != 0U))
    {
        error = ERROR_CONTINUOUSLY_ACTIVE;
    }
    else if (Timer_IsElapsed(timeout_start_tick, timeout_limit) != 0U)
    {
        error = ERROR_TIMEOUT;
    }

    if (error != ERROR_NONE)
    {
        g_state      = STATE_ERROR;
        g_error_code = error;
        ClearTimers();
        return 1U;
    }

    return 0U;
}
