/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "Vl6180x_i2c.h"
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
/* USER CODE BEGIN Variables */
osMessageQueueId_t distQueueHandle;
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for Vl6180x */
osThreadId_t Vl6180xHandle;
const osThreadAttr_t Vl6180x_attributes = {
  .name = "Vl6180x",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for LED_like */
osThreadId_t LED_likeHandle;
const osThreadAttr_t LED_like_attributes = {
  .name = "LED_like",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void StartTask02(void *argument);
void StartTask03(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  distQueueHandle = osMessageQueueNew(10, sizeof(uint16_t), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of Vl6180x */
  Vl6180xHandle = osThreadNew(StartTask02, NULL, &Vl6180x_attributes);

  /* creation of LED_like */
  LED_likeHandle = osThreadNew(StartTask03, NULL, &LED_like_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_StartTask02 */
/**
* @brief Function implementing the Vl6180x thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask02 */
void StartTask02(void *argument)
{
  extern volatile uint8_t measure_mode; 
  uint16_t distance = 0;
  uint8_t last_mode = 0;

  for(;;)
  {
    // --- 1. 处理模式切换的瞬间动作 ---
    if (measure_mode != last_mode) 
    {
      if (measure_mode == 1) {
          VL6180X_Write8(0x0018, 0x01); // 停止连续
          printf("\r\n[Task 2] Switch to SINGLE Mode\r\n");
      } 
      else if (measure_mode == 2) {
          VL6180X_Write8(0x0018, 0x03); // 启动硬件连续
          printf("\r\n[Task 2] Switch to CONTINUOUS Mode\r\n");
      }
      last_mode = measure_mode;
    }

    // --- 2. 正常的测距逻辑 ---
    if (measure_mode == 1) 
    {
        distance = VL6180X_Read_Distance();
        osDelay(200); // 单次模式慢一点
    } 
    else if (measure_mode == 2) 
    {
        uint8_t raw_dist = VL6180X_Read8(0x0062); 
        distance = (uint16_t)raw_dist;
        VL6180X_Write8(0x0015, 0x07); 
        osDelay(50); // 连续模式快一点
    }

    osMessageQueuePut(distQueueHandle, &distance, 0U, 10);
  }
}

/* USER CODE BEGIN Header_StartTask03 */
/**
* @brief Function implementing the LED_like thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask03 */
void StartTask03(void *argument)
{
  /* USER CODE BEGIN StartTask03 */
uint16_t received_dist = 0;
  printf("System UART Print Task Ready!\r\n");

  /* 只需要这一个无限循环 */
  for(;;)
  {
    // 1. 等待队列消息
    if (osMessageQueueGet(distQueueHandle, &received_dist, NULL, osWaitForever) == osOK) 
    {
        // 2. 执行打印
        if (received_dist <= 255) {
            printf("Dist: %d mm\r\n", received_dist);
            printf("Frtos running\r\n");
        } else {
            printf("Sensor Error: %d\r\n", received_dist);
        }
        if (received_dist<100)
        {
          HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        }
        else if (received_dist>=100)
        {
          HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
          /* code */
        }
        
    }
  }
  /* USER CODE END StartTask03 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

