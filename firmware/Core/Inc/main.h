/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define PWM_SERVO_Pin GPIO_PIN_0
#define PWM_SERVO_GPIO_Port GPIOA
#define LED_RUN_Pin GPIO_PIN_7
#define LED_RUN_GPIO_Port GPIOA
#define LED_FAULT_Pin GPIO_PIN_0
#define LED_FAULT_GPIO_Port GPIOB
#define BUZZER_Pin GPIO_PIN_1
#define BUZZER_GPIO_Port GPIOB
#define CB_Thap_Pin GPIO_PIN_10
#define CB_Thap_GPIO_Port GPIOB
#define CB_Cao_Pin GPIO_PIN_11
#define CB_Cao_GPIO_Port GPIOB
#define START_Pin GPIO_PIN_12
#define START_GPIO_Port GPIOB
#define STOP_Pin GPIO_PIN_13
#define STOP_GPIO_Port GPIOB
#define MODE_Pin GPIO_PIN_14
#define MODE_GPIO_Port GPIOB
#define RESET_Pin GPIO_PIN_15
#define RESET_GPIO_Port GPIOB
#define SERVO_Pin GPIO_PIN_11
#define SERVO_GPIO_Port GPIOA
#define CONVEYOR_Pin GPIO_PIN_12
#define CONVEYOR_GPIO_Port GPIOA
#define IN2_L298_Pin GPIO_PIN_4
#define IN2_L298_GPIO_Port GPIOB
#define IN1_L298_Pin GPIO_PIN_5
#define IN1_L298_GPIO_Port GPIOB
#define PWM_CONVEYOR_Pin GPIO_PIN_6
#define PWM_CONVEYOR_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
