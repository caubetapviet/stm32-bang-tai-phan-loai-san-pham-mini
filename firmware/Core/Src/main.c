/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

	// libraries
#include <string.h>

	//module
#include "timer.h"
#include "buttons.h"
#include "sensor.h"
#include "status.h"
#include "data.h"
#include "conveyor.h"
#include "servo.h"
#include "indicator.h"
#include "lcd_i2c.h"		// dung i2c


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void Print(const char *s)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)s, (uint16_t)strlen(s), 100U);
}

static void PrintNum(uint32_t v)
{
    char    tmp[10];
    char    out[11];
    uint8_t n = 0U;
    uint8_t i;

    if (v == 0U) { Print("0"); return; }

    while (v > 0U) { tmp[n] = (char)('0' + (v % 10U)); n++; v /= 10U; }
    for (i = 0U; i < n; i++) { out[i] = tmp[n - 1U - i]; }
    out[n] = '\0';
    Print(out);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  MX_I2C1_Init();
  /* USER CODE BEGIN 2 */
	
	//init cac module
	/* khong co init cua sensor */ //sensor doi code moi thi ok
Timer_Init(); // ok - bao gom ca HAL_TIM_Base_Start_IT(&htim1);

Indicator_Init(); //ok
Conveyor_Init(); // ok - bao gom ca HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
Servo_Init(); // chua test xong - bao gom ca HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

Data_Init(); // chua test
Status_Init();	//chua test
Buttons_Init(); // ok

LCD_Init();	// chua test


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
		
			// ===== 1. Nut nhan ===== 
    switch (Buttons_Update())
    {
        case BUTTON_EVENT_START:
            if (Status_Start())
            {
                Servo_Reset();
                Indicator_SetRun(1U);
               if (Status_GetMode() == MODE_AUTO) { Conveyor_Start(); }
               else                               { Conveyor_Stop();  }
            }
            break;

        case BUTTON_EVENT_STOP:
            if (Status_Stop())
            {
                Conveyor_Stop();
                Servo_Reset();
                Indicator_SetRun(0U);
            }
            break;

        case BUTTON_EVENT_MODE:
            Status_ToggleMode();
            break;

        case BUTTON_EVENT_RESET:
            if (Status_GetState() == STATE_STOP)
            {
                Data_Reset();
            }
            else if (Status_ClearError())
            {
                Indicator_SetFault(0U);
            }
            break;

        case BUTTON_EVENT_CONVEYOR:
            if ((Status_GetState() == STATE_RUNNING) &&
                (Status_GetMode()  == MODE_MANUAL))
            {
                Conveyor_Toggle();
            }
            break;

        case BUTTON_EVENT_SERVO:
            if ((Status_GetState() == STATE_RUNNING) &&
               (Status_GetMode()  == MODE_MANUAL))
            {
                Servo_Toggle();
            }
            break;

        default:
            break;
    }

    // ===== 2. Cam bien va bo dem ===== 
    SensorEvent_t ev = Sensor_Update((Status_GetState() == STATE_RUNNING) ? 1U : 0U);
    uint8_t classified = 0U;

    if (ev == SENSOR_EVENT_PRODUCT_A)
    {
        Data_IncrementA();
        classified = 1U;
        if (Status_GetMode() == MODE_AUTO) { Servo_StartSort(); }
    }
    else if (ev == SENSOR_EVENT_PRODUCT_B)
    {
        Data_IncrementB();
        classified = 1U;
    }

    // ===== 3. Phat hien loi ===== 
    if (Status_Update(Conveyor_IsRunning(), Sensor_IsActive(), classified))
    {
        Conveyor_Stop();
        Servo_Reset();
        Indicator_SetRun(0U);
        Indicator_SetFault(1U);
    }

    // ===== 4. Viec nen ===== 
    Servo_Update();
	
    LCD_Update(	Status_GetState(), 
								Status_GetMode(), 
								Status_GetError(),
								Data_GetCountTotal(), 						
								Data_GetCountA(), 
								Data_GetCountB()
							);
	// */
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
