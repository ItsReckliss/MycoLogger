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
#include "stm32u0xx_hal.h"

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
#define Debug_Button_Pin GPIO_PIN_14
#define Debug_Button_GPIO_Port GPIOC
#define Debug_LED_Pin GPIO_PIN_3
#define Debug_LED_GPIO_Port GPIOF

/* USER CODE BEGIN Private defines */

/*
 * SX1262 nets on the STM32U031F6P6 TSSOP20 shared pads.  CubeMX calls the
 * first-listed GPIO on each package pin "StandardMode".
 */
#define Radio_MOSI_Pin             GPIO_PIN_8
#define Radio_MOSI_GPIO_Port       GPIOB
#define Radio_BUSY_Pin             GPIO_PIN_1
#define Radio_BUSY_GPIO_Port       GPIOA
#define Radio_NSS_Pin              GPIO_PIN_3
#define Radio_NSS_GPIO_Port        GPIOA
#define Radio_SCK_Pin              GPIO_PIN_5
#define Radio_SCK_GPIO_Port        GPIOA
#define Radio_MISO_Pin             GPIO_PIN_7
#define Radio_MISO_GPIO_Port       GPIOA
#define Radio_DIO1_Pin             GPIO_PIN_1
#define Radio_DIO1_GPIO_Port       GPIOB
#define Radio_REST_Pin             GPIO_PIN_8
#define Radio_REST_GPIO_Port       GPIOA

/* SCD41 load switch and software-I2C nets from the transmitter schematic. */
#define Sensor_Power_Pin           GPIO_PIN_7
#define Sensor_Power_GPIO_Port     GPIOB
#define Sensor_SDA_Pin             GPIO_PIN_11
#define Sensor_SDA_GPIO_Port       GPIOA
#define Sensor_SCL_Pin             GPIO_PIN_12
#define Sensor_SCL_GPIO_Port       GPIOA

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
